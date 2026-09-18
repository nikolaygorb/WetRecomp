// game_constants.h - Game-specific constants
#pragma once

#include <cstdint>

namespace GameConstants
{
  constexpr uint32_t kCodeBase = 0x82510000;
  constexpr uint32_t kCodeEnd = 0x8339AB00;
}

namespace GameConstants::PatchConstants
{
  struct Patch
  {
    std::uintptr_t address;
    std::uint32_t value;
  };

  constexpr Patch Fps60()
  {
    return Patch{
        0x83066E67,
        0x01};
  }

  constexpr Patch DisableMotionBlur()
  {
    return Patch{
        0x82CB09B8,
        0xC3E92AA0};
  }

  constexpr Patch DisableDepthOfField()
  {
    return Patch{
        0x82AE6A1C,
        0x39600000};
  }

  constexpr Patch DisableShakyCamera()
  {
    return Patch{
        0x8274F58C,
        0x99430015};
  }

  constexpr Patch DisableFilmEffect()
  {
    return Patch{
        0x82751514,
        0x39400000};
  }

  constexpr Patch DisableFilmEffect2()
  {
    return Patch{
        0x82751660,
        0x39400000};
  }
}