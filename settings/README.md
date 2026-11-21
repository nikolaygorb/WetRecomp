# Settings

Two checked-in config files loaded automatically at startup:

| File | Covers |
|---|---|
| [`hardware.toml`](hardware.toml) | Renderer, window, vsync, resolution, debug, gameplay patches |
| [`mapping.toml`](mapping.toml) | Input backend, keyboard bindings, native mouse look |

Reference pages copied from the ReXGlue SDK (`thirdparty/rexglue-sdk/docs/`):

| File | Covers |
|---|---|
| [`faq.md`](faq.md) | Every SDK graphics option (SDK `graphics-options.md`) |
| [`texture-replacement.md`](texture-replacement.md) | HD texture packs: dumping, making and loading a pack |

Flat `key = value` only — no `[Table]` headers (the loader reads root-level keys only).

Precedence: `--flag=value` > `REX_FLAG=value` > `settings/*.toml` > compiled default.

## `hardware.toml`

Main keys. "Value" is what the checked-in file sets; "Built-in default" is the compiled value used when the key is absent. Every ReXGlue (SDK) rendering option, with its values, is described in [`faq.md`](faq.md).

| Key | Value | Built-in default | Effect |
|---|---|---|---|
| `gpu_plugin` | `"native"` | `""` | GPU plugin: `"native"` (SDK native renderer, Vulkan only, draws without EDRAM emulation; early, see SDK `docs/gpu-plugins.md`) or `"xenos"` (Xenos GPU emulation on D3D12/Vulkan/D3D11, the fallback if something draws wrong). Empty disables GPU emulation |
| `graphics_backend` | `"vulkan"` | `"any"` | `"any"` (D3D12 on Windows, Vulkan on Linux), `"d3d12"`, `"vulkan"`, `"d3d11"` (experimental, Windows only, for GPUs without D3D12/Vulkan; never picked by `"any"`). An unavailable backend falls back to `"any"` with a warning. `vulkan_*` keys only apply on Vulkan, `d3d12_*` only on D3D12, `d3d11_*` only on D3D11 |
| `vsync` | `true` | `true` | Paces the game to the display |
| `resolution_scale` | `2` | `1` | Renders at a multiple of the original resolution |
| `present_effect` | `"bilinear"` | `"bilinear"` | Output scaling: `bilinear`, `cas`, `fsr`, `nis` (Vulkan only). `present_fsr_*` apply to `fsr`, `present_cas_additional_sharpness` only to `cas`. `present_fsr_quality_mode` exists only in builds with `REXGLUE_ENABLE_FIDELITYFX=ON` (off here: the file's `"auto"` line is ignored with a warning) |
| `native_resolution_scale` | commented out (`2.25`) | `0` (= `resolution_scale`) | Native renderer only: render scale `1`–`4` in steps of `0.25` (1.5 = 1080p from the game's 720p), instead of `resolution_scale`. Read at start |
| `present_dither` | `true` | `false` | Dithers the final image; removes banding in skies and gradients |
| `swap_post_effect` | `"smaa"` | `"none"` | Anti-aliasing on the game image: `none`, `fxaa`, `fxaa_extreme`, `smaa` |
| `async_shader_compilation` | `true` | `true` | Compile shaders off the render thread |
| `native_2x_msaa` | `true` | `true` | Native 2x MSAA for games that use 2x MSAA |
| `anisotropic_override` | `5` | `3` | Anisotropic filtering: `-1` no override, `0` off, `1`–`5` = 1x, 2x, 4x, 8x, 16x |
| `texture_cache_memory_limit_soft` / `_hard` | not in the file | `384` / `768` | Texture cache limits in MB; raise them at high `resolution_scale` if textures reload often |
| `clear_memory_page_state` | not in the file | `false` | Fixes stale textures in games that read back GPU writes, at a large upload cost |
| `readback_memexport` / `readback_memexport_fast` | `true` / `true` | `true` / `true` | Copy shader memory writes back to the CPU (fast double-buffered path) |
| `readback_resolve` | `"fast"` | `"none"` | Copy render-to-texture results back to the CPU: `none`, `fast`, `some`, `full`. The legacy `d3d12_readback_resolve` in the file is ignored while this is set |
| `shared_memory_skip_unchanged_uploads` | `true` | `false` | D3D12/Vulkan: don't re-upload 4 KB pages whose bytes are unchanged |
| `async_depth_only_pipelines` | `true` | `false` | With `async_shader_compilation`, also build depth-only pipelines (shadows) in the background; shadows can be missing for a moment when one is new |
| `depth_bias_shader_offset` | `true` | `false` | Applies depth bias in the shader (fixes flickering decals/shadows) |
| `gpu_allow_invalid_fetch_constants` | `true` | `false` | Draw even with broken vertex/texture descriptors instead of skipping the draw |
| `vulkan_allow_present_mode_mailbox` / `_immediate` / `_fifo_relaxed` / `_fifo_latest_ready` | `true` / `false` / `false` / `true` | `true` / `true` / `true` / `false` | Which Vulkan present modes may be used |
| `vulkan_max_frames_in_flight` | not in the file | `0` (= 3) | Frames the CPU may queue ahead of the GPU on Vulkan and the native renderer; lower = less input latency |
| `vulkan_device` | `-1` | `-1` | Vulkan GPU by device index, `-1` auto (also the native renderer) |
| `vulkan_hoist_shared_memory_uploads` | `true` | `false` | Vulkan: upload rewritten vertex/index data before the current batch of draws |
| `vulkan_resolve_into_textures` / `d3d11_resolve_into_textures` | `true` | `false` | At `resolution_scale = 1` only: copy resolved surfaces straight into their textures. No effect at the file's `resolution_scale = 2` |
| `d3d11_adapter` / `d3d11_debug` | `-1` / `false` | `-1` / `false` | D3D11 GPU (DXGI index, `-1` auto, `-2` WARP) and debug layer |
| `window_width` / `window_height` | `1920` / `1080` | `0` (title's own) | Host window size; overrides the window size from `resolution` |
| `fullscreen` | `true` | `true` | Host fullscreen |
| `resolution` | `"1920x1080"` | `""` | Sets both the console video mode the game sees and the window size |
| `present_letterbox` | `true` | `true` | Letterbox instead of stretch |
| `d3d12_queue_priority` | `2` | `1` | GPU queue priority: `0` normal, `1` high, `2` realtime (may need administrator rights) |
| `d3d12_break_on_error` / `d3d12_break_on_warning` | `false` | `false` | Only with `d3d12_debug = true` (D3D12 debug layer; not in the file, leave off) |
| `depth_float24_convert_in_pixel_shader` | not in the file | `false` | Exact 24-bit float depth in the pixel shader (also the native renderer). The game no longer turns it on by itself on Vulkan; set it here if Z-fighting shows |
| `depth_float24_round` | not in the file | `false` | Round instead of truncate to 24-bit float depth |
| `log_level` | `"error"` | `"info"` | `trace`, `debug`, `info`, `warn`, `error`, `critical`, `off` |
| `perf_log_csv` | commented out | `""` | Per-frame metrics CSV; builds with performance counters only (not Release) |
| `frame_stats_interval` | commented out (`5`) | `0` | Every N seconds logs fps and frame-time percentiles, the renderer's draw and GPU times and uploads; `0` off |
| `dev_debug_runtime` | `false` | `false` | Enable runtime debug tools (stub sweep, missing function scan) |
| `unregistered_function_log` | `"unregistered_functions.txt"` | `""` | Appends each guest address called without a recompiled function to this file, as a `[functions]` entry for the project config |
| `trace_gpu_prefix` | `"traces"` | `""` | Folder for GPU frame traces (recorded with `bind_gpu_trace`); empty turns recording off |
| `wet_fps60_unlock` | `true` | `false` | Ported xenia-canary "60 FPS" patch; can affect physics/animation timing |
| `wet_disable_motion_blur` | `false` | `false` | Disable motion blur |
| `wet_disable_depth_of_field` | `false` | `false` | Disable depth of field |
| `wet_disable_shaky_camera` | `false` | `false` | Disable shaky camera + film effect |
| `wet_match_display_refresh` | `true` | `false` | Guest video mode at the primary monitor's refresh rate instead of `video_mode_refresh_rate` (60 Hz). Paces frames only with `vsync = true`; Windows only; see [`src/render/display_refresh.cpp`](../src/render/display_refresh.cpp). Restart after changing it. |
| `wet_gpu_wait_mode` | `2` | `1` | Render thread wait for the GPU: `0` busy spin, `1` yield, `2` sleep 200 µs |
| `user_language` | `1` | `1` | Game language: `1` English, `3` German, `4` French, `5` Spanish, `6` Italian. SDK cvar, returned to the game by `ExGetXConfigSetting`; [`src/platform/language_hook.cpp`](../src/platform/language_hook.cpp) forces anything not on the disc back to English. Restart after changing it. |

Unknown keys (not defined by the SDK or the title in this build) are ignored with a `Config: unknown cvar` warning in the log. A line that is not valid TOML makes the loader skip the whole file (`Failed to parse config` in the log).

## `mapping.toml`

| Key | Default | Effect |
|---|---|---|
| `input_backend` | `"sdl"` | `"sdl"`, `"xinput"`, or `"nop"` |
| `hid_mappings_file` | `"gamecontrollerdb.txt"` | Extra SDL gamepad mappings, or `""` to skip |
| `guide_button` | `true` | Route Guide/PS button to guest |
| `mnk_mode` | `true` | Keyboard-as-controller, merged with any physical pad |
| `mnk_mouse` | `true` | Route mouse to right stick |
| `mnk_sensitivity` | `1.0` | Mouse sensitivity (`0.01`–`10.0`). With `wet_mouse_look` on it only applies where native mouse look doesn't run (menus, turrets) |
| `wet_mouse_look` | `true` | Native mouse look: the mouse turns the third-person camera directly, skipping the stick response curve, turn speed and slow motion. Orbit limits and the in-game invert options still apply. Hooks in [`src/mod/mouse_look/hook.cpp`](../src/mod/mouse_look/hook.cpp). Needs `mnk_mode` and `mnk_mouse` |
| `wet_mouse_sensitivity` | `2.0` | Native mouse look: camera turn per mouse count = 0.022° × this (`0.01`–`100.0`) |
| `wet_mouse_invert_x` / `wet_mouse_invert_y` | `false` | Invert native mouse look axes, on top of the in-game options |

Keyboard bindings (`keybind_a`, `keybind_lstick_up`, ...) have sensible WASD-style defaults in the file. Rebind any of them the same way; besides keys they take `LMB`, `RMB`, `MMB`, the side buttons `MB4` / `MB5` and the wheel `WheelUp` / `WheelDown`. The file differs from the SDK's compiled defaults on four keys: X = `Q`, Y = `E` (SDK: `L`, `P`), left trigger = `I,RMB`, right trigger = `O,LMB` (SDK: `Q,I`, `E,O`).

## Caveats

- The in-game Settings overlay's "Save to config" writes to `hardware.toml` — `git checkout -- settings/hardware.toml` reverts it.
- The launcher (`wetrecomp_launcher`) edits `hardware.toml` and `mapping.toml` in place on Save/Play (only the changed lines; comments and order stay). Its pages and rows come from [`../launcher/launcher.toml`](../launcher/launcher.toml) on top of the SDK's `common.toml`.
- Engine hotkeys (console/debug/achievements toggles) are registered after these files load — rebind them from the in-game overlay instead.
