// With vsync on, the SDK raises the guest vblank at video_mode_refresh_rate (60 Hz by
// default) and the game paces its frames off that vblank. wet_match_display_refresh
// takes the rate from the primary monitor instead, so a 144 Hz panel gets a 144 Hz
// vblank. The GPU vsync worker reads the rate once when the runtime is set up, which is
// why this runs from OnPreSetup. Out-of-range rates are clamped by the SDK (24-480 Hz).
#include "display_refresh.h"

#include <cstdint>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ui/flags.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

REXCVAR_DEFINE_BOOL(wet_match_display_refresh, false, "Graphics",
                    "Use the primary monitor's refresh rate as the guest video mode rate "
                    "instead of video_mode_refresh_rate (Windows only, needs vsync)")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

namespace
{
  // 0 when the rate can't be read.
  uint32_t QueryPrimaryDisplayHz()
  {
#ifdef _WIN32
    DEVMODEW mode{};
    mode.dmSize = sizeof(mode);
    // 0 and 1 stand for the hardware default rate, not a real one.
    if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1)
      return mode.dmDisplayFrequency;
#endif
    return 0;
  }
} // namespace

namespace display_refresh
{
  void MatchDisplayRefreshRate()
  {
    if (!REXCVAR_GET(wet_match_display_refresh))
      return;

    const uint32_t hz = QueryPrimaryDisplayHz();
    if (hz == 0)
    {
      REXLOG_WARN("Display refresh rate unavailable; keeping video_mode_refresh_rate = {}",
                  REXCVAR_GET(video_mode_refresh_rate));
      return;
    }

    REXCVAR_SET(video_mode_refresh_rate, static_cast<double>(hz));
    REXLOG_INFO("video_mode_refresh_rate = {} Hz (primary display)", hz);
  }
} // namespace display_refresh
