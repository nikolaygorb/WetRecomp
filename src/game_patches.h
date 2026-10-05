// game_patches.h - Game-specific memory patches
#pragma once

#include <cstdint>
#include <cstring>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/xmemory.h>

#include "game_cvars.h"
#include "game_constants.h"

namespace game_patches
{
  // Ported from xenia-canary game-patches for Wet (425307DB)
  // https://github.com/xenia-canary/game-patches/blob/main/patches/425307DB%20-%20Wet.patch.toml
  static bool WriteGuestBytes(uint32_t address, const uint8_t *bytes, uint32_t size)
  {
    auto *rt = rex::Runtime::instance();
    auto *memory = rt ? rt->memory() : nullptr;
    auto *heap = memory ? memory->LookupHeap(address) : nullptr;
    if (!heap)
    {
      REXLOG_WARN("Cannot apply game patch at guest address {:08X}: no guest heap", address);
      return false;
    }

    const uint64_t last_address = static_cast<uint64_t>(address) + size - 1;
    if (last_address > UINT32_MAX ||
        address / heap->page_size() != last_address / heap->page_size())
    {
      REXLOG_WARN("Cannot apply game patch at guest address {:08X}: range crosses a guest page",
                  address);
      return false;
    }

    constexpr uint32_t writable = rex::memory::kMemoryProtectRead |
                                  rex::memory::kMemoryProtectWrite;
    uint32_t old_protect{};
    if (!heap->Protect(address, size, writable, &old_protect))
    {
      REXLOG_WARN("Cannot make guest range {:08X}+{:X} writable for game patch", address, size);
      return false;
    }

    std::memcpy(memory->TranslateVirtual(address), bytes, size);
    if (!heap->Protect(address, size, old_protect, nullptr))
    {
      REXLOG_WARN("Cannot restore guest protection for game patch at {:08X}", address);
      return false;
    }
    return true;
  }

  static void ApplyBe8(const GameConstants::PatchConstants::Patch &patch)
  {
    const uint8_t bytes[1] = {static_cast<uint8_t>(patch.value)};
    WriteGuestBytes(static_cast<uint32_t>(patch.address), bytes, sizeof(bytes));
  }

  static void ApplyBe32(const GameConstants::PatchConstants::Patch &patch)
  {
    const uint8_t bytes[4] = {
        static_cast<uint8_t>(patch.value >> 24),
        static_cast<uint8_t>(patch.value >> 16),
        static_cast<uint8_t>(patch.value >> 8),
        static_cast<uint8_t>(patch.value)};
    WriteGuestBytes(static_cast<uint32_t>(patch.address), bytes, sizeof(bytes));
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
