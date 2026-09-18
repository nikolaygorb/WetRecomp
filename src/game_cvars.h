// game_cvars.h - Game-specific CVAR definitions
#pragma once

#include <rex/cvar.h>

REXCVAR_DEFINE_STRING(graphics_backend, "any", "GPU",
                      "Graphics API backend: any, d3d12, vulkan")
    .allowed({"any", "d3d12", "vulkan"});

REXCVAR_DEFINE_BOOL(dev_debug_runtime, false, "Debug",
                    "Enable runtime debug tools (stub sweep, missing function scan)");

REXCVAR_DEFINE_BOOL(wet_fps60_unlock, false, "Gameplay", "Unlock 60 FPS");

REXCVAR_DEFINE_BOOL(wet_disable_motion_blur, false, "Graphics",
                    "Disable motion blur");

REXCVAR_DEFINE_BOOL(wet_disable_depth_of_field, false, "Graphics",
                    "Disable depth of field");

REXCVAR_DEFINE_BOOL(wet_disable_shaky_camera, false, "Graphics",
                    "Disable shaky camera and film effect");
