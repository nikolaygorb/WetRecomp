# Settings

Two checked-in config files loaded automatically at startup:

| File | Covers |
|---|---|
| [`hardware.toml`](hardware.toml) | Renderer, window, vsync, resolution, debug, gameplay patches |
| [`mapping.toml`](mapping.toml) | Input backend, keyboard bindings, native mouse look |

Flat `key = value` only — no `[Table]` headers (the loader reads root-level keys only).

Precedence: `--flag=value` > `REX_FLAG=value` > `settings/*.toml` > compiled default.

## `hardware.toml`

Main keys. "Value" is what the checked-in file sets; "Built-in default" is the compiled value used when the key is absent. Every ReXGlue (SDK) rendering option, with its values, is described in [`faq.md`](faq.md).

| Key | Value | Built-in default | Effect |
|---|---|---|---|
| `gpu_plugin` | `"xenos"` | `""` | GPU emulation plugin; empty disables GPU emulation |
| `graphics_backend` | `"any"` | `"any"` | `"any"` (D3D12 on Windows, Vulkan on Linux), `"d3d12"`, `"vulkan"`. `vulkan_*` keys only apply on Vulkan, `d3d12_*` only on D3D12 |
| `vsync` | `true` | `true` | Paces the game to the display |
| `resolution_scale` | `2` | `1` | Renders at a multiple of the original resolution |
| `present_effect` | `"fsr"` | `"bilinear"` | Output scaling: `bilinear`, `cas`, `fsr`, `nis` (Vulkan only). `present_fsr_*` apply to `fsr`, `present_cas_additional_sharpness` only to `cas`. `present_fsr_quality_mode` exists only in builds with `REXGLUE_ENABLE_FIDELITYFX=ON` |
| `swap_post_effect` | `"smaa"` | `"none"` | Anti-aliasing on the game image: `none`, `fxaa`, `fxaa_extreme`, `smaa` |
| `async_shader_compilation` | `true` | `true` | Compile shaders off the render thread |
| `native_2x_msaa` | `true` | `true` | Native 2x MSAA for games that use 2x MSAA |
| `anisotropic_override` | `5` | `3` | Anisotropic filtering: `-1` no override, `0` off, `1`–`5` = 1x, 2x, 4x, 8x, 16x |
| `texture_cache_memory_limit_soft` / `_hard` | `1024` / `4096` | `384` / `768` | Texture cache limits in MB |
| `clear_memory_page_state` | `true` | `false` | Fixes stale textures in games that read back GPU writes, at a large upload cost |
| `window_width` / `window_height` | `1920` / `1080` | `0` (title's own) | Host window size; overrides the window size from `resolution` |
| `fullscreen` | `true` | `true` | Host fullscreen |
| `resolution` | `"1920x1080"` | `""` | Sets both the console video mode the game sees and the window size |
| `present_letterbox` | `true` | `true` | Letterbox instead of stretch |
| `d3d12_queue_priority` | `2` | `1` | GPU queue priority: `0` normal, `1` high, `2` realtime (may need administrator rights) |
| `d3d12_break_on_error` / `d3d12_break_on_warning` | `false` | `false` | Only with `d3d12_debug = true` (D3D12 debug layer; not in the file, leave off) |
| `depth_float24_convert_in_pixel_shader` | not in the file | `false`; `true` on Vulkan | Exact 24-bit float depth in the pixel shader. Turned on by the game when it runs on Vulkan ([`src/render/graphics_backend.cpp`](../src/render/graphics_backend.cpp)), off on D3D12. It only changes the default: a line in this file or `--depth_float24_convert_in_pixel_shader` wins on both backends, and "Save to config" doesn't write the Vulkan value |
| `depth_float24_round` | not in the file | `false` | Round instead of truncate to 24-bit float depth |
| `log_level` | `"error"` | `"info"` | `trace`, `debug`, `info`, `warn`, `error`, `critical`, `off` |
| `perf_log_csv` | commented out | `""` | Per-frame metrics CSV; builds with performance counters only (not Release) |
| `dev_debug_runtime` | `false` | `false` | Enable runtime debug tools (stub sweep, missing function scan) |
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

Keyboard bindings (`keybind_a`, `keybind_lstick_up`, ...) have sensible WASD-style defaults in the file. Rebind any of them the same way.

## Caveats

- The in-game Settings overlay's "Save to config" writes to `hardware.toml` — `git checkout -- settings/hardware.toml` reverts it.
- Engine hotkeys (console/debug/achievements toggles) are registered after these files load — rebind them from the in-game overlay instead.
