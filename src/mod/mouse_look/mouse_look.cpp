// mouse_look.cpp - see mouse_look.h.
#include "mouse_look.h"

#include <imgui.h>

#include <rex/cvar.h>
#include <rex/input/mnk/mnk_input_driver.h>
#include <rex/logging.h>
#include <rex/ui/imgui_drawer.h>
#include <rex/ui/windowed_app_context.h>

#include <algorithm>
#include <cmath>

REXCVAR_DEFINE_BOOL(wet_mouse_look, true, "Mouse",
                    "Native mouse look: mouse motion turns the third-person camera directly "
                    "(1:1, no stick response curve or turn speed) instead of emulating the "
                    "right stick. Where the camera hook does not run (menus, turrets) the "
                    "stick emulation stays in charge. Needs mnk_mode and mnk_mouse")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);
REXCVAR_DEFINE_DOUBLE(wet_mouse_sensitivity, 2.0, "Mouse",
                      "Mouse look sensitivity: camera turn per mouse count = 0.022 deg x this "
                      "(as in Source games)")
    .range(0.01, 100.0)
    .lifecycle(rex::cvar::Lifecycle::kHotReload);
REXCVAR_DEFINE_BOOL(wet_mouse_invert_x, false, "Mouse",
                    "Invert the horizontal mouse look axis (on top of the in-game option)")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);
REXCVAR_DEFINE_BOOL(wet_mouse_invert_y, false, "Mouse",
                    "Invert the vertical mouse look axis (on top of the in-game option)")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

namespace wet::mouse
{

  namespace
  {
    // If Consume() stops being called (the camera hook doesn't run in this game
    // state: menu, cutscene, turret...) the pointer must not stay hidden and
    // locked with nothing consuming its motion.
    constexpr std::chrono::milliseconds kConsumerTimeout{300};
    constexpr std::chrono::milliseconds kWatchdogPeriod{100};

    // After asking mnk to let go: never engage before the short one (a capture
    // update may already be queued on its side), and stop waiting for it to
    // release after the long one.
    constexpr std::chrono::milliseconds kHandoffMin{50};
    constexpr std::chrono::milliseconds kHandoffMax{300};

    constexpr double kRadiansPerDegree = 3.14159265358979323846 / 180.0;
    constexpr double kDegreesPerCount = 0.022;

    // Guards against a stalled frame or a pointer teleport turning into one
    // huge snap: no single Consume() turns the camera by more than this.
    constexpr double kMaxRadiansPerCall = 1.5;
  } // namespace

  MouseLook &MouseLook::Get()
  {
    static MouseLook instance;
    return instance;
  }

  MouseLook::~MouseLook()
  {
    {
      std::lock_guard lock(mu_);
      watchdog_stop_ = true;
    }
    watchdog_cv_.notify_all();
    if (watchdog_.joinable())
    {
      watchdog_.join();
    }
  }

  void MouseLook::Attach(rex::ui::Window *window, rex::ui::ImGuiDrawer *imgui)
  {
    if (!window)
    {
      return;
    }
    {
      std::lock_guard lock(mu_);
      if (window_)
      {
        return; // already attached
      }
      window_ = window;
      ui_ = &window->app_context();
      imgui_ = imgui;
      // Assumed focused until told otherwise, like the mnk driver does: the
      // window may not report its state yet and a focus event may never come.
      has_focus_ = true;
    }
    window->AddInputListener(this, 0);
    window->AddListener(this);
    watchdog_ = std::thread([this] { WatchdogLoop(); });
  }

  bool MouseLook::Enabled() const
  {
    return REXCVAR_GET(wet_mouse_look);
  }

  bool MouseLook::Moving(std::chrono::milliseconds window) const
  {
    std::lock_guard lock(mu_);
    return phase_ == Phase::kEngaged && Clock::now() - last_motion_ < window;
  }

  void MouseLook::OnMouseMove(rex::ui::MouseEvent &e)
  {
    std::lock_guard lock(mu_);
    if (phase_ != Phase::kEngaged)
    {
      return;
    }
    pending_dx_ += e.dx();
    pending_dy_ += e.dy();
    if (e.dx() != 0 || e.dy() != 0)
    {
      last_motion_ = Clock::now();
    }
  }

  void MouseLook::OnLostFocus(rex::ui::UISetupEvent &)
  {
    UiTask task;
    {
      std::lock_guard lock(mu_);
      has_focus_ = false;
      task = BeginReleaseLocked();
    }
    Post(std::move(task));
  }

  void MouseLook::OnGotFocus(rex::ui::UISetupEvent &)
  {
    std::lock_guard lock(mu_);
    has_focus_ = true;
  }

  void MouseLook::OnClosing(rex::ui::UIEvent &)
  {
    bool give_back = false;
    {
      std::lock_guard lock(mu_);
      // The window is going away with its capture; nothing to undo on it, only
      // the mnk driver to hand the mouse back to.
      give_back = phase_ != Phase::kIdle;
      phase_ = Phase::kIdle;
      captured_ = false;
      window_ = nullptr;
      ui_ = nullptr;
      watchdog_stop_ = true;
    }
    watchdog_cv_.notify_all();
    if (give_back)
    {
      rex::input::mnk::SetMouseLookActive(true);
    }
  }

  bool MouseLook::OverlayWantsMouse() const
  {
    return imgui_ && imgui_->GetIO().WantCaptureMouse;
  }

  MouseLook::UiTask MouseLook::BeginEngageLocked()
  {
    phase_ = Phase::kEngaging;
    return [this]
    {
      rex::ui::Window *window = nullptr;
      {
        std::lock_guard lock(mu_);
        if (phase_ != Phase::kEngaging)
        {
          return; // released again before this ran
        }
        window = window_;
      }
      if (!window)
      {
        return;
      }

      // Window state is touched on the UI thread only, and outside the lock.
      const auto visibility = window->GetCursorVisibility();
      const auto restore_to = visibility == rex::ui::Window::CursorVisibility::kHidden
                                  ? rex::ui::Window::CursorVisibility::kVisible
                                  : visibility;
      window->SetCursorVisibility(rex::ui::Window::CursorVisibility::kHidden);
      window->CaptureMouse();
      if (!window->SetRelativeMouseMode(true))
      {
        REXLOG_WARN("wet_mouse_look: this window backend has no pointer lock, mouse motion is "
                    "taken from raw pointer deltas without re-centering");
      }

      std::lock_guard lock(mu_);
      captured_ = true;
      saved_cursor_ = restore_to;
      pending_dx_ = pending_dy_ = 0.0;
      // A release posted meanwhile keeps its phase and undoes the capture.
      if (phase_ == Phase::kEngaging)
      {
        phase_ = Phase::kEngaged;
      }
    };
  }

  MouseLook::UiTask MouseLook::BeginReleaseLocked()
  {
    if (phase_ == Phase::kIdle || phase_ == Phase::kReleasing)
    {
      return {};
    }
    phase_ = Phase::kReleasing;
    return [this]
    {
      rex::ui::Window *window = nullptr;
      bool captured = false;
      auto restore_to = rex::ui::Window::CursorVisibility::kVisible;
      {
        std::lock_guard lock(mu_);
        window = window_;
        captured = captured_;
        restore_to = saved_cursor_;
      }
      if (captured && window)
      {
        window->SetRelativeMouseMode(false);
        window->ReleaseMouse();
        window->SetCursorVisibility(restore_to);
      }
      {
        std::lock_guard lock(mu_);
        captured_ = false;
        pending_dx_ = pending_dy_ = 0.0;
        phase_ = Phase::kIdle;
      }
      // Only now: mnk re-captures on its next poll, after the pointer is free.
      rex::input::mnk::SetMouseLookActive(true);
    };
  }

  void MouseLook::Post(UiTask task)
  {
    if (!task)
    {
      return;
    }
    rex::ui::WindowedAppContext *ui;
    {
      std::lock_guard lock(mu_);
      ui = ui_;
    }
    if (ui && ui->CallInUIThreadDeferred(std::move(task)))
    {
      return;
    }
    // No UI thread to run it on (shutting down): there is no window state left
    // worth undoing, just give the mouse back to mnk.
    {
      std::lock_guard lock(mu_);
      phase_ = Phase::kIdle;
      captured_ = false;
    }
    rex::input::mnk::SetMouseLookActive(true);
  }

  void MouseLook::WatchdogLoop()
  {
    std::unique_lock lock(mu_);
    while (!watchdog_stop_)
    {
      watchdog_cv_.wait_for(lock, kWatchdogPeriod);
      if (watchdog_stop_)
      {
        break;
      }
      if (phase_ != Phase::kIdle && phase_ != Phase::kReleasing &&
          Clock::now() - last_consume_ > kConsumerTimeout)
      {
        UiTask task = BeginReleaseLocked();
        lock.unlock();
        Post(std::move(task));
        lock.lock();
      }
    }
  }

  std::optional<Turn> MouseLook::Consume()
  {
    UiTask task;
    std::optional<Turn> result;
    {
      std::lock_guard lock(mu_);
      const auto now = Clock::now();
      last_consume_ = now;

      const bool want =
          REXCVAR_GET(wet_mouse_look) && window_ && has_focus_ && !OverlayWantsMouse();
      if (!want)
      {
        task = BeginReleaseLocked();
      }
      else
      {
        switch (phase_)
        {
        case Phase::kIdle:
          // Claim: mnk stops mapping the mouse to the stick and releases its
          // own capture on its next poll.
          rex::input::mnk::SetMouseLookActive(false);
          phase_ = Phase::kHandoff;
          handoff_since_ = now;
          break;

        case Phase::kHandoff:
        {
          const auto waited = now - handoff_since_;
          const bool mnk_let_go =
              window_->GetCursorVisibility() != rex::ui::Window::CursorVisibility::kHidden;
          if (waited >= kHandoffMin && (mnk_let_go || waited >= kHandoffMax))
          {
            task = BeginEngageLocked();
          }
          break;
        }

        case Phase::kEngaging:
        case Phase::kReleasing:
          break;

        case Phase::kEngaged:
          result = TakeTurnLocked();
          break;
        }
      }
    }
    Post(std::move(task));
    return result;
  }

  Turn MouseLook::TakeTurnLocked()
  {
    const double dx = pending_dx_;
    const double dy = pending_dy_;
    pending_dx_ = pending_dy_ = 0.0;

    const double radians_per_count =
        REXCVAR_GET(wet_mouse_sensitivity) * kDegreesPerCount * kRadiansPerDegree;
    const double x_sign = REXCVAR_GET(wet_mouse_invert_x) ? -1.0 : 1.0;
    // Pointer y grows downward, stick y grows upward.
    const double y_sign = REXCVAR_GET(wet_mouse_invert_y) ? 1.0 : -1.0;

    const double yaw = dx * radians_per_count * x_sign;
    const double pitch = dy * radians_per_count * y_sign;
    if (!std::isfinite(yaw) || !std::isfinite(pitch))
    {
      return Turn{0.0, 0.0};
    }
    return Turn{std::clamp(yaw, -kMaxRadiansPerCall, kMaxRadiansPerCall),
                std::clamp(pitch, -kMaxRadiansPerCall, kMaxRadiansPerCall)};
  }

} // namespace wet::mouse
