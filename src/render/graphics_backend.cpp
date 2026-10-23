#include "graphics_backend.h"

#include <string>

#include <rex/cvar.h>
#include <rex/cvar_file.h>
#include <rex/logging.h>
#include <rex/system/gpu_plugin.h>

REXCVAR_DEFINE_STRING(graphics_backend, "any", "GPU",
                      "Graphics API backend: any, d3d12, vulkan")
    .allowed({"any", "d3d12", "vulkan"});

namespace
{
  // Vulkan needs exact float24 depth conversion in the pixel shader. Changing the default (not
  // the value) keeps an explicit hardware.toml / --flag value in charge, and "Save to config"
  // doesn't write it, so a later D3D12 run doesn't pick it up.
  void ApplyVulkanDefaults()
  {
    rex::cvar::SetDefaultValue("depth_float24_convert_in_pixel_shader", "true");
  }
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

    if (vulkan)
      ApplyVulkanDefaults();
  }
} // namespace graphics_backend
