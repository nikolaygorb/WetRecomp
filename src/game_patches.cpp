// Ported from xenia-canary game-patches for Wet (425307DB)
// https://github.com/xenia-canary/game-patches/blob/main/patches/425307DB%20-%20Wet.patch.toml
//
// Xenia patches guest code bytes, but recompiled code never reads guest code back, so
// writing those bytes changes nothing. Each patch is a [[midasm_hook]] in
// patches.toml instead: the hook runs right after the original instruction
// and leaves the state the patched instruction would have left.
#include <bit>
#include <cstdint>

#include <rex/cvar.h>
#include <rex/memory/utils.h>
#include <rex/ppc/context.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xmemory.h>

REXCVAR_DEFINE_BOOL(wet_fps60_unlock, false, "Gameplay", "Unlock 60 FPS");

REXCVAR_DEFINE_BOOL(wet_disable_motion_blur, false, "Graphics",
                    "Disable motion blur");

REXCVAR_DEFINE_BOOL(wet_disable_depth_of_field, false, "Graphics",
                    "Disable depth of field");

REXCVAR_DEFINE_BOOL(wet_disable_shaky_camera, false, "Graphics",
                    "Disable shaky camera and film effect");

namespace
{
  uint8_t *GuestPointer(uint32_t address)
  {
    return rex::system::kernel_memory()->TranslateVirtual(address);
  }
} // namespace

// 0x83066E64: li r11,2 -> li r11,1. Present interval 2 vblanks becomes 1.
void WetFps60Hook(PPCRegister &r11)
{
  if (REXCVAR_GET(wet_fps60_unlock))
    r11.u64 = 1;
}

// 0x82CB09B8: lfs f31,-8132(r9) -> lfs f31,10912(r9).
void WetMotionBlurHook(PPCRegister &r9, PPCRegister &f31)
{
  if (!REXCVAR_GET(wet_disable_motion_blur))
    return;

  const uint32_t bits = rex::memory::load_and_swap<uint32_t>(GuestPointer(r9.u32 + 10912));
  f31.f64 = double(std::bit_cast<float>(bits));
}

// 0x82AE6A1C: lbz r11,1688(r4) -> li r11,0.
void WetDepthOfFieldHook(PPCRegister &r11)
{
  if (REXCVAR_GET(wet_disable_depth_of_field))
    r11.u64 = 0;
}

// 0x8274F58C: stb r11,21(r3) -> stb r10,21(r3).
void WetShakyCameraHook(PPCRegister &r3, PPCRegister &r10)
{
  if (REXCVAR_GET(wet_disable_shaky_camera))
    *GuestPointer(r3.u32 + 21) = r10.u8;
}

// 0x82751514: subfe r10,r10,r3 -> li r10,0.
void WetFilmEffectHook(PPCRegister &r10)
{
  if (REXCVAR_GET(wet_disable_shaky_camera))
    r10.u64 = 0;
}

// 0x82751660: lbz r10,21(r11) -> li r10,0.
void WetFilmEffect2Hook(PPCRegister &r10)
{
  if (REXCVAR_GET(wet_disable_shaky_camera))
    r10.u64 = 0;
}
