# Settings

Two checked-in config files loaded automatically at startup:

| File | Covers |
|---|---|
| [`hardware.toml`](hardware.toml) | Renderer, window, vsync, resolution, debug, gameplay patches |
| [`mapping.toml`](mapping.toml) | Input backend, keyboard bindings |

Flat `key = value` only — no `[Table]` headers (the loader reads root-level keys only).

Precedence: `--flag=value` > `REX_FLAG=value` > `settings/*.toml` > compiled default.

## `hardware.toml`

| Key | Default | Effect |
|---|---|---|
| `gpu_plugin` | `"xenos"` | GPU emulation plugin |
| `graphics_backend` | `"any"` | `"any"` (D3D12 on Windows, Vulkan on Linux), `"d3d12"`, `"vulkan"` |
| `vsync` | `false` | Vertical sync |
| `resolution_scale` | `2` | Internal render-target supersampling |
| `async_shader_compilation` | `true` | Compile shaders off the render thread |
| `native_2x_msaa` | `true` | Native 2x MSAA |
| `anisotropic_override` | `1` | Force anisotropic filtering level (`0` = leave alone) |
| `window_width` / `window_height` | `1920` / `1080` | Host window size |
| `fullscreen` | `true` | Host fullscreen |
| `resolution` | `"1920x1080"` | Guest video mode reported to the game |
| `present_letterbox` | `true` | Letterbox instead of stretch |
| `d3d12_debug` | `false` | D3D12 debug layer — leave off |
| `dev_debug_runtime` | `false` | Enable runtime debug tools (stub sweep, missing function scan) |
| `wet_fps60_unlock` | `true` | Ported xenia-canary "60 FPS" patch |
| `wet_disable_motion_blur` | `false` | Disable motion blur |
| `wet_disable_depth_of_field` | `false` | Disable depth of field |
| `wet_disable_shaky_camera` | `false` | Disable shaky camera + film effect |

## `mapping.toml`

| Key | Default | Effect |
|---|---|---|
| `input_backend` | `"sdl"` | `"sdl"`, `"xinput"`, or `"nop"` |
| `hid_mappings_file` | `"gamecontrollerdb.txt"` | Extra SDL gamepad mappings, or `""` to skip |
| `guide_button` | `true` | Route Guide/PS button to guest |
| `mnk_mode` | `true` | Keyboard-as-controller, merged with any physical pad |
| `mnk_mouse` | `true` | Route mouse to right stick |
| `mnk_sensitivity` | `1.0` | Mouse sensitivity (`0.01`–`10.0`) |

Keyboard bindings (`keybind_a`, `keybind_lstick_up`, ...) have sensible WASD-style defaults in the file. Rebind any of them the same way.

## Caveats

- The in-game Settings overlay's "Save to config" writes to `hardware.toml` — `git checkout -- settings/hardware.toml` reverts it.
- Engine hotkeys (console/debug/achievements toggles) are registered after these files load — rebind them from the in-game overlay instead.
