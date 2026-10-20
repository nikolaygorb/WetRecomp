// hook.cpp - mouse look straight into the third-person camera (wetcamera::Cam3rd).
//
// Cam3rd::Update (sub_829BA128, vtable 0x822DA3AC slot 2, this = camera) first gathers its
// input (sub_829B8078): the raw right stick from its WetCameraInput (camera+488, device at
// +4), run through WetCameraInputFilter::ModifyAndFilterRightStickValue (sub_829A64B8,
// filter at camera+492) with the in-game invert options (bytes +2/+3 of camera+496), lands
// at camera+164/+168. With no invert option set the filter flips both axes. The gather also
// sets "right stick touched" bits in the camera flags (camera+160: 0x2 raw, 0x80 filtered).
//
// Update runs the manual orbit step (sub_829B1960) only while one of those bits is set;
// otherwise it runs the auto-align behaviors, which pull the camera back behind a running
// player. The manual step turns yaw and pitch (radians) with sub_829AB470 each:
//     delta = dt * speed * filtered stick    (speed: Cam3rdData mYawSpeed / mPitchSpeed)
//     angle = sub_829A8168(angle, delta, limits)   - adds delta, clamps to the orbit limits
// yaw first (result at caller r1+84), then pitch (caller r1+80).
//
// So the mouse is taken in the gather, which runs on every camera update whatever the stick
// does: that keeps the hand-over from mnk alive, and while the mouse moves the touched bits
// are set so the manual step runs. There the turn is added to delta right before
// sub_829A8168, with the signs the stick would get (filter flip, in-game invert, the sign of
// the turn speed). Mouse counts are an angle, not a rate: the turn speed, slow motion dt and
// stick curve are skipped; the orbit limits still apply, and an axis whose speed is 0
// (locked by the camera data) stays locked.
#include <chrono>
#include <cmath>
#include <cstdint>

#include <rex/memory/utils.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

#include "mouse_look.h"

namespace
{
  constexpr uint32_t kCameraInput = 488;
  constexpr uint32_t kCameraFilter = 492;
  constexpr uint32_t kCameraOptions = 496;
  constexpr uint32_t kInputDevice = 4;
  constexpr uint32_t kOptionsInvertX = 2;
  constexpr uint32_t kOptionsInvertY = 3;

  constexpr uint32_t kRightStickTouched = 0x2 | 0x80;

  // sub_829AB470 frame is 144 bytes: the caller's r1+84 / r1+80 seen from inside.
  constexpr uint32_t kYawResult = 144 + 84;
  constexpr uint32_t kPitchResult = 144 + 80;

  // Mouse events arrive at the host rate, camera updates at the guest frame rate:
  // a frame without motion in the middle of a sweep must not hand the camera to
  // auto-align.
  constexpr std::chrono::milliseconds kTouchedWindow{100};

  // Taken by the gather, applied by the turn steps of the same camera update.
  struct PendingTurn
  {
    uint32_t camera = 0;
    double yaw = 0.0;
    double pitch = 0.0;
    bool yaw_pending = false;
    bool pitch_pending = false;
  };
  thread_local PendingTurn g_turn;

  uint8_t *GuestPointer(uint32_t address)
  {
    return rex::system::kernel_memory()->TranslateVirtual(address);
  }

  uint32_t LoadU32(uint32_t address)
  {
    return rex::memory::load_and_swap<uint32_t>(GuestPointer(address));
  }

  // Same check as sub_829A2FE8: no device, no stick.
  bool HasInput(uint32_t camera)
  {
    const uint32_t input = LoadU32(camera + kCameraInput);
    return input && LoadU32(input + kInputDevice);
  }

  struct StickSigns
  {
    double x;
    double y;
  };

  // The signs sub_829A64B8 gives the right stick: -1 per axis, +1 with its invert option.
  StickSigns FilterSigns(uint32_t camera)
  {
    if (!LoadU32(camera + kCameraFilter))
    {
      return {1.0, 1.0};
    }
    const uint32_t options = LoadU32(camera + kCameraOptions);
    if (!options)
    {
      return {-1.0, -1.0};
    }
    return {*GuestPointer(options + kOptionsInvertX) ? 1.0 : -1.0,
            *GuestPointer(options + kOptionsInvertY) ? 1.0 : -1.0};
  }

  // delta = dt * speed * stick: the turn goes the way the stick would push it.
  void AddTurn(PPCRegister &delta, double turn, double stick_sign, double speed)
  {
    if (speed == 0.0 || !std::isfinite(speed))
    {
      return;
    }
    const double sign = speed > 0.0 ? stick_sign : -stick_sign;
    delta.f64 = double(float(delta.f64 + turn * sign));
  }
} // namespace

// 0x829B80EC in sub_829B8078, the camera input gather, before the right stick is read; both
// paths of the device check meet here. r29 = camera.
void WetMouseLookGather(PPCRegister &r29)
{
  g_turn = {};
  auto &look = wet::mouse::MouseLook::Get();
  if (!look.Enabled() || !HasInput(r29.u32))
  {
    return;
  }
  const auto turn = look.Consume();
  if (!turn || (turn->yaw == 0.0 && turn->pitch == 0.0))
  {
    return;
  }
  g_turn = {r29.u32, turn->yaw, turn->pitch, true, true};
}

// 0x829B82D0 in sub_829B8078, the store of the camera flags with the right stick bits just
// worked out: r10 = flags, r29 = camera.
void WetMouseLookTouched(PPCRegister &r10, PPCRegister &r29)
{
  auto &look = wet::mouse::MouseLook::Get();
  if (!look.Enabled() || !HasInput(r29.u32))
  {
    return;
  }
  const bool turning = g_turn.camera == r29.u32 && g_turn.yaw_pending;
  if (!turning && !look.Moving(kTouchedWindow))
  {
    return;
  }
  r10.u64 = r10.u32 | kRightStickTouched;
}

// 0x829AB4DC in sub_829AB470, right before sub_829A8168: f2 = delta (radians), f28 = turn
// speed, r30 = result pointer, r31 = camera.
void WetMouseLookTurn(PPCRegister &r1, PPCRegister &r30, PPCRegister &r31, PPCRegister &f2,
                      PPCRegister &f28)
{
  if (g_turn.camera != r31.u32)
  {
    return;
  }
  const uint32_t result = r30.u32 - r1.u32;
  if (result == kYawResult && g_turn.yaw_pending)
  {
    g_turn.yaw_pending = false;
    AddTurn(f2, g_turn.yaw, FilterSigns(r31.u32).x, f28.f64);
  }
  else if (result == kPitchResult && g_turn.pitch_pending)
  {
    g_turn.pitch_pending = false;
    AddTurn(f2, g_turn.pitch, FilterSigns(r31.u32).y, f28.f64);
  }
}
