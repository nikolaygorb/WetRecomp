// graphics_backend.h - GPU plugin backend selection
#pragma once

#include <rex/runtime.h>

namespace graphics_backend
{
  // Loads config.gpu_plugin with the graphics_backend cvar's backend (falling back to "any")
  // and applies the per-backend cvar defaults. Must run from OnPreSetup.
  void LoadGpuPlugin(rex::RuntimeConfig &config);
} // namespace graphics_backend
