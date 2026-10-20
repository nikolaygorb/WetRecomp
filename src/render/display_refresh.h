// display_refresh.h - Guest video mode refresh rate
#pragma once

namespace display_refresh
{
  // Sets video_mode_refresh_rate to the primary monitor's rate when
  // wet_match_display_refresh is on. Must run before the runtime is set up.
  void MatchDisplayRefreshRate();
} // namespace display_refresh
