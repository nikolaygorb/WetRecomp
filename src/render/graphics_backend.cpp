#include "graphics_backend.h"

#include <string>

#include <rex/cvar.h>
#include <rex/cvar_file.h>
#include <rex/logging.h>
#include <rex/system/gpu_plugin.h>

REXCVAR_DEFINE_STRING(graphics_backend, "any", "GPU",
                      "Graphics API backend: any, d3d11, d3d12, vulkan")
    .allowed({"any", "d3d11", "d3d12", "vulkan"});

namespace
{
} // namespace

namespace graphics_backend
{
  void LoadGpuPlugin(rex::RuntimeConfig &config)
  {
    if (config.gpu_plugin.empty())
      return;

    const std::string backend = REXCVAR_GET(graphics_backend);
    bool vulkan = false;
    if (backend != "any")
    {
      config.graphics = rex::system::LoadGpuPlugin(config.gpu_plugin, backend);
      if (config.graphics)
        vulkan = backend == "vulkan";
      else
        REXLOG_WARN("graphics_backend '{}' unavailable, falling back to automatic selection",
                    backend);
    }
    if (!config.graphics)
    {
      config.graphics = rex::system::LoadGpuPlugin(config.gpu_plugin);
#if !REX_HAS_D3D12
      // "any" takes D3D12 when it's built in (Windows), otherwise Vulkan (as plugin_main.cpp).
      vulkan = config.graphics != nullptr;
#endif
    }
  }
} // namespace graphics_backend
