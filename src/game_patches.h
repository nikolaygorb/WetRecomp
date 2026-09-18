// game_patches.h - Game-specific memory patches
#pragma once

#include <cstdint>

#include <rex/cvar.h>
#include <rex/memory/utils.h>
#include <rex/runtime.h>

#include "game_cvars.h"
#include "game_constants.h"

namespace game_patches
{
  // Ported from xenia-canary game-patches for Wet (425307DB)
  // https://github.com/xenia-canary/game-patches/blob/main/patches/425307DB%20-%20Wet.patch.toml
  static uint8_t *PatchAddress(std::uintptr_t address)
  {
    auto *rt = rex::Runtime::instance();
    auto *base = rt ? rt->virtual_membase() : nullptr;
    return base ? base + address : nullptr;
  }

  static void ApplyBe8(const GameConstants::PatchConstants::Patch &patch)
  {
    auto *p = PatchAddress(patch.address);
    if (!p)
    {
      return;
    }

    rex::memory::PageAccess old_access{};
    rex::memory::Protect(p, sizeof(uint8_t), rex::memory::PageAccess::kReadWrite,
                         &old_access);
    p[0] = static_cast<uint8_t>(patch.value);
    rex::memory::Protect(p, sizeof(uint8_t), old_access, nullptr);
  }

  static void ApplyBe32(const GameConstants::PatchConstants::Patch &patch)
  {
    auto *p = PatchAddress(patch.address);
    if (!p)
    {
      return;
    }

    rex::memory::PageAccess old_access{};
    rex::memory::Protect(p, sizeof(uint32_t), rex::memory::PageAccess::kReadWrite,
                         &old_access);
    p[0] = static_cast<uint8_t>(patch.value >> 24);
    p[1] = static_cast<uint8_t>(patch.value >> 16);
    p[2] = static_cast<uint8_t>(patch.value >> 8);
    p[3] = static_cast<uint8_t>(patch.value);
    rex::memory::Protect(p, sizeof(uint32_t), old_access, nullptr);
  }

  static void Fps60()
  {
    if (!REXCVAR_GET(wet_fps60_unlock))
    {
      return;
    }

    ApplyBe8(GameConstants::PatchConstants::Fps60());
  }

  static void DisableMotionBlur()
  {
    if (REXCVAR_GET(wet_disable_motion_blur))
    {
      ApplyBe32(GameConstants::PatchConstants::DisableMotionBlur());
    }
  }

  static void DisableDepthOfField()
  {
    if (REXCVAR_GET(wet_disable_depth_of_field))
    {
      ApplyBe32(GameConstants::PatchConstants::DisableDepthOfField());
    }
  }

  static void DisableShakyCamera()
  {
    if (!REXCVAR_GET(wet_disable_shaky_camera))
    {
      return;
    }

    ApplyBe32(GameConstants::PatchConstants::DisableShakyCamera());
    ApplyBe32(GameConstants::PatchConstants::DisableFilmEffect());
    ApplyBe32(GameConstants::PatchConstants::DisableFilmEffect2());
  }
} // namespace game_patches
