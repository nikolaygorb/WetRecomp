// game_cvars.h - Game-specific CVAR definitions
#pragma once

#include <rex/cvar.h>

REXCVAR_DEFINE_BOOL(dev_debug_runtime, false, "Debug",
                    "Enable runtime debug tools (stub sweep, missing function scan)");
