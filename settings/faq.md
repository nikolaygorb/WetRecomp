# Graphics options: resolution, upscaling, sharpening, frame generation

This page lists the image quality options of titles built with ReXGlue, and the driver-level tools that work with them without any support in the title. Options are settings (cvars) in the title's settings file, for example `settings/hardware.toml` (`key = value`, read at startup).

## Built-in options

| Setting | Values | What it does |
|---|---|---|
| `resolution_scale` (or `draw_resolution_scale_x` / `draw_resolution_scale_y`) | 1 (default), 2, 3, … | Renders the game at a multiple of its original resolution. The best image quality of all options, at a high GPU and memory cost. 2 or 3 is typical. Can be changed while the game runs (the settings overlay or the config): at the end of the next frame the GPU state is rebuilt for the new scale, with a short pause, and render targets and resolved textures start over; if the scale can't be set up (not enough GPU memory), 1x is used and the log says so. |
| `present_effect` | `bilinear` (default), `cas`, `fsr`, `nis` | How the game image is scaled to the window. `cas`: AMD FidelityFX CAS sharpening (with light upscaling up to 2x). `fsr`: AMD FidelityFX Super Resolution 1 (edge-preserving upscaling, then sharpening). `nis`: NVIDIA Image Scaling (one sharpening upscale of up to 2x, then bilinear); Vulkan only, D3D12 and GPUs without it use FSR's upscaler instead. All work on any GPU vendor. |
| `present_cas_additional_sharpness` | 0.0 (default) to 1.0 | Extra CAS sharpness. |
| `present_fsr_sharpness_reduction` | 0.2 (default) to 2.0 | FSR sharpening strength, in stops: higher is softer. |
| `present_nis_sharpness` | 0.5 (default), 0.0 to 1.0 | NIS sharpening strength: higher is sharper. |
| `present_fsr_max_upsampling_passes` | 1 to 4 (default 4) | Limits the chained FSR upscaling passes (each pass is up to 2x). |
| `present_dither` | `false` (default), `true` | Dithers the final image; removes banding in skies and gradients. |
| `swap_post_effect` | `none` (default), `fxaa`, `fxaa_extreme`, `smaa` | Anti-aliasing on the game image. `fxaa`: fast, slightly blurry. `smaa`: SMAA 1x (high preset), sharper edges and better on thin lines, a bit more GPU time. |
| `anisotropic_override` | -1 to 5 (default 3 = 4x) | Forces anisotropic texture filtering; 5 = 16x. |

`resolution_scale` and `present_effect` combine: render at 2x, then `fsr`, `nis` or `cas` to scale to a 4K window.

Builds made with the optional AMD FidelityFX SDK (`REXGLUE_ENABLE_FIDELITYFX=ON`) also offer `present_effect = fsr2` / `fsr3` and `present_fsr_quality_mode`. They are experimental: the title does not yet give the upscaler depth, motion vectors or camera jitter, so it works on one frame at a time and is usually no better than `fsr`.

DLSS, XeSS, FSR 2 and later as real temporal upscalers, frame generation inside the title, and ambient occlusion are not available yet. They need per-title support.

## Texture replacement (HD texture packs)

A pack is a folder with an `index.tsv` and DDS files: textures whose original data is in the pack are drawn with the pack's images, at any size with the original's aspect ratio (2x, 4x, 2.5x …). Only 2D textures in DXT1, DXT3, DXT5, DXN (ATI2) and 32-bit RGBA are replaced, and the image must be in the same format as the original. All backends. How it works, how to make a pack, and an example: [texture-replacement.md](texture-replacement.md).

| Setting | Values | What it does |
|---|---|---|
| `texture_replacement_path` | folder (default empty: off) | The pack to use. Read once at startup. |
| `texture_replacement_user_path` | folder (default empty) | A second pack, read after the first: a texture in both comes from this one. Its images may not be more see-through than the originals (an opaque original makes the image opaque). |
| `texture_replacement_enabled` | `true` (default), `false` | Turns replacement off and on while the game runs. |
| `texture_replacement_allow_list` | file (default empty) | A file of texture keys (one 16-digit hex key per line, `#` comments): only those are replaced, and every image is checked against the original's alpha. Can change while the game runs. |
| `texture_replacement_memory_limit` | 4096 MB (default), 0 = no limit | Most memory the replaced textures may take at once; a texture beyond it is drawn from the original data. A 2x pack takes 4x the memory of the originals. |
| `texture_replacement_log` | `false` (default), `true` | Logs every texture that could be replaced: its key, size, format, and whether it was. |
| `texture_replacement_dump_path` | folder (default empty: off) | Writes every texture that could be replaced to the folder as a pack: `<key>.dds` files and their `index.tsv`. Read once at startup. |

Making a pack, in short: play with `texture_replacement_dump_path` set, edit or upscale the DDS files you want (same format, file name and aspect ratio), then point `texture_replacement_path` at the folder. Details in [texture-replacement.md](texture-replacement.md).

## Shipped shader cache

The first time a game draws something new, its shader is translated and a pipeline is built for it, which can show as a hitch or as an object missing for a moment. The shader cache (`cache/shaders/shareable` in the game's data folder: `<title ID>.xsh` and one `<title ID>.*.xpso` per backend and path) remembers them, so later runs build them at startup. A title can ship a cache, so that even a first run starts with it:

1. Play the game through on the backends to ship, then take the files from `cache/shaders/shareable`.
2. Put them in the title project's `shader_cache` folder, or name a folder with `rexglue_configure_target(<target> SHADER_CACHE <dir>)`. The build copies them to `shader_cache` beside the executable. They hold the game's shaders: keep them out of the repository, like the game.
3. At startup, before the cache is opened, each shipped file is merged into the player's: a player without the file, or with one from another version, gets the shipped copy; otherwise the shipped records the player lacks are added. The log says how many.

Shipped files from another SDK version are ignored (the log says so): record them again after an SDK update. All backends.

| Setting | Values | What it does |
|---|---|---|
| `shader_cache_shipped` | folder (default empty: `shader_cache` beside the executable) | Where the shipped cache is. An empty folder turns the merge off. Read at start. |

## Direct3D 12 and Vulkan

Titles can render with Direct3D 12 (Windows only) or Vulkan (Windows, Linux). All built-in options above work the same on both.

### Choosing the backend

The backend is chosen by the title. Titles that let the player pick it usually have a setting such as `graphics_backend = "any" | "d3d12" | "vulkan"` (Perfect Dark Zero recomp does). With `any`, Direct3D 12 is tried first, then Vulkan. On Windows, Vulkan is only there if the title was built with `REXGLUE_USE_VULKAN=ON` (off by default).

Which one to use: Direct3D 12 is the default and usually the safer choice on Windows. Try Vulkan if a game shows a rendering bug or runs worse on Direct3D 12 on your GPU, or if a tool you want needs it. Report a bug that shows on only one backend together with the backend name.

### Direct3D 11 (experimental)

An SDK built with `REXGLUE_USE_D3D11=ON` (Windows, off by default) also has a Direct3D 11 backend, for GPUs that have neither Direct3D 12 nor Vulkan. It is chosen only by name (`graphics_backend = "d3d11"` in titles that pass the setting on), never by `any`. It needs Direct3D feature level 11.0; 11.1 is better (see below).

When to use it: only when the title doesn't start on Direct3D 12 or Vulkan on your GPU, for example an older GPU or driver without Direct3D 12. On a GPU with Direct3D 12 or Vulkan, use those: they are faster and more accurate, and get fixes first. Direct3D 11 is experimental: so far only Fuse has been checked through, and other games may show a wrong picture.

What works as on Direct3D 12: drawing with the game's textures, the game's output with its gamma ramp and `swap_post_effect`, `resolution_scale` (also changed while the game runs), `readback_resolve` (all modes, with `readback_resolve_full_deferred` and `readback_resolve_cpu_pages_only`), `readback_memexport` (with `readback_memexport_fast`), occlusion queries (also `occlusion_query_full_counters`, as with `render_target_path_d3d12 = "rtv"`), tessellation, texture replacement, `async_shader_compilation`, and `store_shaders` (its pipelines go to `<title>.rtv.d3d11.xpso`, next to the shader file shared with the other backends, and are created at start-up).

Known limits:

- Render targets are always host render targets, like `render_target_path_d3d12 = "rtv"`; there is no `rov` path, so blending and formats are less exact in the games that need it.
- On feature level 11.0, draws whose vertex shader writes memory (memexport) are skipped, with one log line per shader. Feature level 11.1 runs them. None of the checked games needs it so far.
- Compressed textures (DXT, DXN) stay compressed, as on Direct3D 12, only when their width and height are multiples of 4 (2D and cube textures); 3D textures, other sizes, and the CTX1 and DXT3A formats are decompressed when loaded and take 4 to 8 times the video memory.
- The GPU must give one 512 MB buffer for the console's memory; with `resolution_scale`, the scaled render target memory must also fit in 512 MB.

To report a problem, add the backend name, the GPU, and a log with `d3d11_log_frames` set (and with `d3d11_debug` if the Graphics Tools Windows feature is installed).

| Setting | Values | What it does |
|---|---|---|
| `d3d11_adapter` | -1 (auto), -2 (WARP), 0, 1, … | Which GPU to use, by DXGI adapter index. Auto prefers `gpu_vendor`, then the GPU with the most dedicated video memory. |
| `d3d11_feature_level` | `""` (highest), `11_0`, `11_1` | Highest feature level to use. For testing how the backend behaves on older GPUs. |
| `d3d11_optional_features` | `true` (default), `false` | `false` ignores the optional Direct3D 11.3 features the GPU reports, like GPUs without them. For testing. |
| `d3d11_allow_variable_refresh_rate_and_tearing` | `true` (default), `false` | Allows G-Sync/FreeSync and tearing when `vsync = false`. |
| `d3d11_block_compressed_textures` | `true` (default), `false` | Keeps DXT and DXN textures compressed in video memory when their size allows. `false` decompresses all of them (more video memory), to tell whether a texture problem comes from this. Read at start. |
| `d3d11_shader_creation_threads` | -1 (default: 3/4 of the logical processors), 0, 1, … | Threads that create the game's shaders in the background with `async_shader_compilation` (`async_pipeline_wait_ms` and `async_depth_only_pipelines` apply as on Direct3D 12). 0 creates them when drawn, with a pause the first time. |

### Backend settings

| Setting | Backend | Values | What it does |
|---|---|---|---|
| `render_target_path_d3d12` | D3D12 | `""` (auto), `rtv`, `rov` | How the console's EDRAM (its render target memory) is emulated. `rtv`: host render targets, fast; the default (auto picks `rov` only on Intel GPUs older than Arc, because of a driver bug). `rov`: rasterizer-ordered views, more accurate blending and formats, much slower. |
| `render_target_path_vulkan` | Vulkan | `""` (auto = `fbo`), `fsi` | Same choice for Vulkan. `fbo`: host render targets, fast. `fsi`: fragment shader interlock, more accurate, slower, needs GPU support (otherwise falls back). |
| `gpu_vendor` | both | `""` (default), `nvidia`, `amd`, `intel`, or a PCI vendor id such as `0x10de` | Which kind of GPU to prefer when several are installed. Empty prefers a discrete GPU over an integrated one. `d3d12_adapter` / `vulkan_device` win over it. |
| `d3d12_adapter` | D3D12 | -1 (auto), 0, 1, … | Which GPU to use, by DXGI adapter index; -2 = WARP (software, for debugging). |
| `vulkan_device` | Vulkan | -1 (auto), 0, 1, … | Which GPU to use, by Vulkan device index. |
| `d3d12_pipeline_creation_threads`, `vulkan_pipeline_creation_threads` | D3D12, Vulkan | -1 (auto) to 32 | Threads that create pipelines in the background. |
| `d3d12_pipeline_library` | D3D12 | `true` (default), `false` | Keeps the driver's compiled pipelines between runs (`shaders/local/*.pso_lib` in the cache folder). Later runs start faster and show fewer pauses or pop-in while pipelines are prepared. Specific to your GPU and driver: after a driver update it is rebuilt by itself. |
| `vulkan_resolve_into_textures`, `d3d11_resolve_into_textures` | Vulkan, D3D11 | `false` (default), `true` | At `resolution_scale = 1` (Vulkan: with `fbo`): an image the game renders and then samples (a 32-bit color surface without MSAA) is copied straight into its texture, instead of being loaded again from memory. Less GPU work in games that do this every frame; no effect at higher scales. Turn it off if colors or images look wrong. |
| `vulkan_texture_load_direct` | Vulkan | `false` (default), `true` | At `resolution_scale` 2 or more: an image the game renders and then samples (a 32-bit or 64-bit format, one mip level) is written into its texture by the loading shader itself, instead of through a temporary buffer and a copy. Less GPU work in games that do this every frame. These textures can be written by shaders, which on some GPUs costs them texture compression, so measure the GPU time with and without. Turn it off if colors or images look wrong. Read at start. |
| `vulkan_texture_load_direct_verify` | Vulkan | `0` (default) to `64` | For checking `vulkan_texture_load_direct`: for this many of its loads a frame, also loads the usual way (that is what the picture shows) and compares the two byte for byte; differences go to the log as `vulkan_texture_load_direct_verify:` lines, with a count every 100 loads. Costs GPU time and memory; leave at `0` for play. Takes effect while the game runs. |
| `vulkan_hoist_shared_memory_uploads` | Vulkan | `false` (default), `true` | When the game rewrites vertex or index data in the middle of drawing, uploads it before the current batch of draws instead of interrupting the batch for each upload. Less GPU and CPU work in games that do this a lot (tens of thousands of times a second in racing games). Turn it off if geometry flickers or shows stale shapes. |
| `gamma_render_target_as_unorm16` | D3D12, Vulkan | `true` (default), `false` | Stores the console's gamma render targets in 16 bits per channel instead of 8 bits: less banding in dark gradients. Fast render target paths (`rtv` / `fbo`); on Vulkan falls back to 8 bits if the GPU cannot blend that format. |
| `d3d12_bindless` | D3D12 | `true` (default), `false` | Binds textures and buffers through one large descriptor table where the GPU supports it. Turn off only to test a driver problem. |
| `d3d12_queue_priority` | D3D12 | 0 = normal, 1 = high (default), 2 = realtime | Priority of the GPU command queue. Realtime may need administrator rights. |
| `d3d12_submit_on_primary_buffer_end`, `vulkan_submit_on_primary_buffer_end` | D3D12, Vulkan | `true` (default), `false` | Sends recorded work to the GPU each time a command buffer of the game ends, not only at the end of the frame. |
| `vulkan_threaded_submission` | Vulkan | `true` (default), `false` | Prepares the work for the GPU and sends it on a thread of its own, so the thread that runs the game's graphics commands can go on meanwhile. Helps games limited by that thread, and needs a spare CPU core. Turn it off to rule it out if a game hangs or shows a device lost error on Vulkan. Read at start. |
| `vulkan_merge_adjacent_barriers` | Vulkan | `false` (default), `true` | Joins barriers (points where the graphics card waits for earlier work to finish) that come one after another into one. Can lower graphics card time a little in games with many render target copies and texture loads. Turn it off first if the picture shows flickering or missing parts after turning it on. Takes effect while the game runs. |
| `vulkan_present_on_submission_thread` | Vulkan | `true` (default), `false` | With `vulkan_threaded_submission`: each new frame no longer waits until the previous one has been prepared; showing it runs on that same thread. Gives more frames per second when the frame rate is not capped, nothing at a frame rate cap. Turn it off if the picture stutters, shows old frames or the game hangs on quitting. Takes effect while the game runs. |
| `precise_interpolation` | Vulkan | `true` (default), `false` | Interpolates pixel shader inputs in the shader instead of by the GPU (needs `VK_KHR_fragment_shader_barycentric`). Removes noise in games that compare interpolated values for equality, such as Perfect Dark Zero and Tenchu Z on NVIDIA. |
| `vulkan_async_skip_incomplete_frames` | Vulkan | `true` (default), `false` | With `async_shader_compilation`, does not show frames where some draws were skipped while their pipelines were being created. Avoids flashing. |
| `vulkan_async_skip_incomplete_frames_ms` | Vulkan | `0` (default) to `10000` | With `vulkan_async_skip_incomplete_frames`: once frames have been held back this many milliseconds in a row, shows them without the objects whose pipelines are still being created, instead of a frozen picture. For drivers that take long per pipeline (some AMD drivers on Windows, a few hundred milliseconds each) when a new area loads; `100` is a good start. Frames missing a depth-only draw or a full-screen pass are still held. `0`: held until the pipelines are ready, as before. Takes effect while the game runs. |
| `d3d12_allow_variable_refresh_rate_and_tearing` | D3D12 | `true` (default), `false` | Allows G-Sync/FreeSync and tearing when `vsync = false`. |
| `vulkan_allow_present_mode_immediate`, `vulkan_allow_present_mode_mailbox`, `vulkan_allow_present_mode_fifo_relaxed` | Vulkan | `true` (default), `false` | Which Vulkan present modes may be used: immediate (no vsync, tearing), mailbox (low latency without tearing), FIFO relaxed (vsync that tears when late). They are tried in this order, then plain FIFO. Turn one off if a driver handles it badly. |
| `vulkan_allow_present_mode_fifo_latest_ready` | Vulkan | `false` (default), `true` | Where plain FIFO would be used (the three above off or unavailable), uses FIFO latest ready when the GPU and driver support it: still vsync without tearing, but each refresh shows the newest finished frame instead of the oldest queued one. Smoother and lower latency when the game's frame rate does not divide the display's refresh rate. |
| `vulkan_max_frames_in_flight` | Vulkan | 0 (default: 3), 1 to 3 | How many frames the CPU may prepare ahead of the GPU. 2 lowers input latency and shortens waits in games that read rendered images back (`readback_resolve`); 1 lowers them more, but can cost frame rate when the GPU is the limit. |
| `vulkan_render_target_trim_percent` | Vulkan | 0 (default: off), 1 to 100 | Frees the cached render targets the game is not using when video memory use reaches this percentage of what the driver allows, or after video memory ran out. Try 90 at high `resolution_scale` if long sessions end with missing textures, black pictures or a crash; each trim pauses the game for a moment. |
| `vulkan_integrated_memory_spill` | Vulkan | `true` (default), `false` | On laptops and handhelds whose GPU has only a small slice of RAM reserved for it (often 512 MB, set in the BIOS), puts textures in ordinary system memory and keeps the reserved slice at most half full. Fixes black textures and "device lost" crashes there. No effect on graphics cards with their own memory. Its log line at start (`Vulkan memory heaps: ...`) says whether it applies. With it, also set `vulkan_render_target_trim_percent = 90`. |
| `vulkan_gpu_time`, `d3d11_gpu_time` | Vulkan, D3D11 | `true` (default), `false` | In builds with performance counters (not Release): measures how long the graphics card works each frame, in total and by kind (draws, render target copies, resolves, texture loads, uploads, the swap), shown in the debug overlay and written to the `perf_log_csv` file. Read at start. |

**Tearing on Vulkan with `vsync = true`.** On Vulkan, `vsync` only paces the game; the swapchain still uses the immediate present mode when the driver offers it, which allows tearing (for lower latency and G-Sync/FreeSync). If you see tearing lines, set:

`vulkan_allow_present_mode_immediate = false`

The presenter then uses mailbox (no tearing, low latency) or, without mailbox, FIFO (classic vsync). If tearing remains when frames are late, also set `vulkan_allow_present_mode_fifo_relaxed = false`. This is the same behaviour and the same settings as Xenia.

### Settings for both backends

| Setting | Values | What it does |
|---|---|---|
| `async_shader_compilation` | `true` (default), `false` | Compiles shaders in background threads: less stutter, but objects can be missing for a moment the first time they appear. |
| `async_pipeline_wait_ms` | 0 (default: never waits), 1 to 100 ms | With `async_shader_compilation`, how long a draw waits for its pipeline before it is skipped. 2 to 8 ms keep objects from popping in when their pipeline is ready quickly, at the cost of short pauses the first time new objects appear. Mostly matters on the first run, before the pipeline caches are filled. |
| `async_depth_only_pipelines` | `false` (default), `true` | With `async_shader_compilation`, also prepares depth-only pipelines (shadows, depth pre-passes) in the background. Removes rare long pauses when one is new to the graphics driver, but shadows or whole objects can be missing until it is ready. |
| `store_shaders` | `true` (default), `false` | Keeps compiled shaders on disk, so later runs stutter less. |
| `shared_memory_hot_pages` | `false` (default), `true` | Direct3D 12 and Vulkan. Memory the game rewrites every frame, such as animated vertex data, stops being write-protected: when it is used, it is compared with what was sent to the graphics card last and sent only if it changed, at most once per batch of commands from the game. Saves a page fault on each write and a protection change on each upload, so it helps games with a lot of per-frame vertex data on the CPU. Only memory the game writes counts, so `clear_memory_page_state` no longer makes every page a frame uses hot. Pages that stop changing or are no longer used go back to normal after 120 frames. Takes up to 64 MB. Read at start. |
| `shared_memory_skip_unchanged_uploads` | `false` (default), `true` | Direct3D 12 and Vulkan. Remembers a hash of what was last sent to the graphics card for each 4 KB page, and doesn't send a page again when the game's memory still holds the same bytes. With `clear_memory_page_state = true` most of what is sent again every frame is unchanged, so this keeps that setting's protection at a fraction of its cost. Costs hashing on the CPU (about 1 ms per 20 MB sent) and 1 MB of memory. Not used while a frame trace is recorded. Read at start. |
| `shared_memory_hash_uploads` | `false` (default), `true` | Direct3D 12 and Vulkan. Hashes what is sent to the graphics card as `shared_memory_skip_unchanged_uploads` does, but sends everything as before, so the upload line of `frame_stats_interval` shows how much memory `clear_memory_page_state = true` sent again with changed bytes: what would be stale with it off. 0 MB there in every scene means the game doesn't need `clear_memory_page_state`. Read at start. |
| `shared_memory_hash_threads` | `0` (default) to `15` | With one of the two settings above: hashes large batches of pages on this many extra CPU threads besides the GPU thread. Same result, less time on the GPU thread when it sends many megabytes a frame; needs spare CPU cores. `shared_memory_hash_parallel_min_pages` (default `512`, 2 MB; changes while the game runs) is the smallest batch spread over them. Read at start. |
| `shader_replacements` | `false` (default), `true` | Direct3D 12. Uses the host shaders in the `shader_replacements` folder beside the executable instead of the translated ones they name. A file is `<HASH>[_<MODIFICATION>].<vs\|ps_rtv\|ps_rov>.dxbc`, with hash and modification in 16 hex digits as `dump_shaders` writes them. A replacement must keep the translated shader's bindings. Read at start. |
| `vsync` | `true` (default), `false` | Paces the game to the display. Many games tie their speed to vsync: with `false` they can run too fast. |
| `fullscreen`, `fullscreen_exclusive` | `true` / `false` | Start in fullscreen; exclusive switches the display mode instead of borderless. |
| `fullscreen_refresh_rate` | 0 (default: the desktop's), up to 1000 | With `fullscreen_exclusive`, the display refresh rate in Hz, for example 120 so that a 60 Hz game fits it exactly. The closest rate the display has is used, and the log says so if it differs. Changes while the game runs. The game isn't told about it: that is `video_mode_refresh_rate`. |
| `native_2x_msaa` | `true` (default), `false` | Uses the GPU's native 2x MSAA for games that use 2x MSAA. |

## Window and display

| Setting | Values | What it does |
|---|---|---|
| `resolution` | `""` (default: the title's own), `720p`, `1080p`, `1440p`, `4k`, or `WIDTHxHEIGHT` such as `1280x720` | One setting for both the console video mode the game sees and the window size. Changing what the game renders at is `resolution_scale`; this sets the output size. |
| `window_width`, `window_height` | 0 (default: the title's own) to 8192 | Window size in logical pixels; overrides the size from `resolution`. |
| `monitor` | 0 (default), 1 = primary, 2 = second monitor, … | Which monitor the window opens on. |
| `video_mode_width`, `video_mode_height` | 1280 × 720 (default), from 640 × 480 | The video mode the game is told the console outputs. Most games render at their own size anyway; for a sharper image use `resolution_scale`. `resolution` sets these together with the window. |
| `avpack` | 8 = HDMI (default), 0 to 8 | The video cable the game is told is connected: 0 = PAL-60 component (SD), 2 = PAL-60 SCART, 3 = 480p component (HD), 4 = HDMI + audio, 5 = PAL-60 composite / S-Video, 6 = VGA, 7 = TV PAL-60, 8 = HDMI. Some games offer other resolutions or refresh rates per cable. |
| `video_mode_refresh_rate` | 60.0 (default), 24.0 to 480.0 | The refresh rate the game is told the console outputs, in Hz. |
| `guest_vblank_rate` | 0 (default: `video_mode_refresh_rate`), up to 1000 | How often the game gets its vblank, in Hz, without changing the refresh rate the game is told. A game that presents on vblank is capped to this rate, for example 30. Changes while the game runs. Ignored with `vsync = false`. |
| `display_output_transfer` | `none` (default), `bt1886` | `bt1886` shows the picture the way an HDTV does (BT.1886, gamma 2.4): Xbox 360 games are made for a TV, which shows dark tones darker than a PC monitor (sRGB). Use it if the image looks washed out or too bright in the shadows. Applied through the game's own gamma ramp, at no cost; can be changed while the game runs. |
| `kernel_display_gamma_type` | 2 = BT.709 HDTV (default), 1 = sRGB, 0 = linear, 3 = power | The display the game is told it draws for; the game adjusts its gamma ramp to it. With 2 (what a console on a TV reports) the console's Direct3D puts BT.709's curve on top of the game's ramp; with 1 the game's ramp is used as it is. Takes effect when the game next sets its gamma ramp. |
| `kernel_display_gamma_power` | 2.22222 (default), 1.0 to 3.0 | With `kernel_display_gamma_type = 3`, the display's gamma power. |
| `present_letterbox` | `true` (default), `false` | Keeps the game's aspect ratio with black bars when the window has another shape. `false` stretches the image to the window. |
| `present_allow_overscan_cutoff` | `false` (default), `true` | Crops the image edges like a TV's overscan, keeping the safe area below. Hides garbage some games draw at the very edges. |
| `present_safe_area_x`, `present_safe_area_y` | 90 (default), 0 to 100 | With `present_allow_overscan_cutoff`, the share of the width and height (in %) that stays visible. |
| `win32_high_resolution_timer`, `win32_mmcss` | `true` (default), `false` | Windows: ask for the finest system timer (usually 0.5 ms) and for multimedia CPU scheduling at start, as Xenia does, so frame pacing and sleeps wake on time. `false` only to compare or to save battery. |
| `video_driver` | `""` (default), `wayland`, `x11`, … | Linux: which SDL video driver to use. Empty picks SDL's default for the session. |
| `host_present_from_non_ui_thread` | `true` (default), `false` | Lets the GPU thread present frames itself instead of handing each one to the window thread. Turn off if the window freezes or flickers on your system. |
| `ui_refresh_rate` | 0 (default: the display's), up to 1000 | Linux and macOS, when presentation doesn't wait for vsync: how often the window repaints for the interface alone, such as an open settings window, in Hz. Without a limit it would repaint as fast as the graphics card can and take its time from the game. Game frames are shown as soon as they are ready either way. Changes while the game runs. Windows paces these repaints by the display itself. |

## Accuracy settings (per game)

The defaults suit most games. A title's own settings file usually sets the ones it needs; change these when a game shows a specific problem, and keep a note of what you changed.

| Setting | Values | What it does |
|---|---|---|
| `occlusion_query` | `fake`, `fast` (default), `fast-alt`, `strict`, `async` | How the console's visibility queries are answered (used for lens flares, sun glare, object culling, auto-exposure). `fake`: a made-up "visible" answer, fastest, effects may be wrong. `fast`: asks the GPU, answers at once from the last known result. `fast-alt`: like `fast`, keeps "hidden" results it is not sure about. `strict`: waits for the real answer, exact, slower. `async`: real answers without waiting, best for flares and glares; games that wait for the answer stall longer. |
| `readback_resolve` | `none` (default), `fast`, `some`, `full` | Copies images the game renders to textures back to the CPU, for games that read them there (screenshots, save thumbnails, some auto-exposure and streaming systems). `fast` / `some`: one frame late, cheap. `full`: exact, causes stalls. The older `d3d12_readback_resolve = true` / `vulkan_readback_resolve = true` mean `fast`, but only while `readback_resolve` itself is not set. |
| `readback_resolve_cpu_reads_only` | `true` (default), `false` | With `readback_resolve`, copies back only the images the game's CPU actually reads. The others stay on the GPU until the game first touches them; that first access waits while the image is copied back, and from then on it is copied back every time. Fewer copies and, with `full`, fewer stalls. `false`: every image, as before. |
| `readback_resolve_cpu_pages_only` | `true` (default), `false` | With the above, copies back only the 4 KB pieces of an image the game has read (a game that tests one pixel of a full-screen image gets that piece); touching another piece brings it in then. Images read for more than an eighth of their size are copied whole. |
| `readback_resolve_full_deferred` | `true` (default), `false` | With `readback_resolve = full`: the GPU thread no longer waits at each copy; the image goes into memory when the game next reads it, usually long finished by then. Same data as before, higher frame rate. |
| `readback_resolve_cpu_wait_limit_ms` | 100 to 600000 (default: 10000) | How long the game's first read of an image waits for its copy before going on with the older contents (logged as a warning). Raise it only for a very slow (software) GPU. |
| `readback_resolve_log` | `false` (default), `true` | Logs which images are copied back, which are skipped, and when the game first reads one (for diagnosing a game's `readback_resolve` needs). |
| `readback_resolve_half_pixel_offset` | `false` (default), `true` | With `resolution_scale` and `readback_resolve`, the CPU gets the center sample of each scaled pixel block instead of its top-left one. Can help thin lines and edges in data the game reads back (gamma correction, visibility checks). |
| `readback_memexport` | `true` (default), `false` | Copies data that GPU shaders write to memory back to the CPU. Some games need it (distant textures in Fuse). `false` is a little faster where a game does not need it. The older `d3d12_readback_memexport` / `vulkan_readback_memexport`, when set, override it on their backend. |
| `readback_memexport_fast` | `true` (default), `false` | Uses a faster double-buffered copy for `readback_memexport` when possible, with a fallback to the exact one. Only `false` keeps what the CPU wrote to the exported range during the draw: it writes back just the bytes the draw changed. |
| `render_target_reuse` | `true` (default), `false` | When a surface moves to a new EDRAM base, reuses an idle render target of the same shape instead of allocating another one, so video memory does not keep growing. Turn off if surfaces show wrong content on Direct3D 12, Vulkan or Direct3D 11. |
| `clear_memory_page_state` | `false` (default), `true` | Rechecks at the end of each frame which memory the GPU wrote. `true` fixes stale textures or missing models in games that read back what the GPU wrote (as in Team Ninja games), but re-uploads that memory every frame (up to gigabytes a second in heavy games), which can cost frame rate and cause stutter. It does not refresh textures already loaded. To see whether a game needs it, run with `true`, `shared_memory_hash_uploads = true` and `frame_stats_interval`; to keep it at less cost, use `shared_memory_skip_unchanged_uploads`. |
| `depth_bias_shader_offset` | `false` (default), `true` | Applies the depth bias in the shader with a constant unit, as the console does, instead of the graphics API's own bias. Fixes flickering (Z-fighting) decals, shadows and layered surfaces such as snow over rock. Which draws it covers is set by `depth_bias_shader_offset_scope`. Fast render target paths only (`rtv` / `fbo`); `rov` / `fsi` always do it in the shader. |
| `depth_bias_shader_offset_scope` | `auto` (default), `decals`, `all` | Which draws with depth bias `depth_bias_shader_offset` covers. Direct3D 12 and 11 have no way to keep the bias unit constant on float depth buffers: there it shrinks close to depth 0, so surfaces drawn over each other flicker. Vulkan keeps it constant with `VK_EXT_depth_bias_control` (see `vulkan_depth_bias_force_unorm`). `auto`: every such draw on Direct3D 12, Direct3D 11 and on Vulkan without that extension, only decals on Vulkan with it. `decals`: only decals (depth test LessEqual or GreaterEqual, colour writes, no stencil, no alpha to coverage), as before; the log then names each kind of draw left out once, in `depth_bias_shader_offset:` lines (at `log_level = "info"`). `all`: every such draw, also on Vulkan. Cost of the draws covered: no early depth test (more GPU time on depth-only passes such as shadows), and with MSAA one depth per pixel instead of per sample, so edges of hair or foliage can look coarser. Try `decals` if those look worse than on Vulkan. Changes while the game runs. |
| `texture_point_snap` | `false` (default), `true` | Snaps point-sampled texture coordinates to texel centers. Fixes texture seams in games with virtual textures; can break other games. |
| `xenos_sample_positions` | `false` (default), `true` | Uses the console's own MSAA sample positions on the fast render target paths (`rtv` / `fbo`), when the GPU supports it. Can fix edges and shadows in games with 2x/4x MSAA. |
| `use_fuzzy_alpha_epsilon` | `false` (default), `true` | Compares alpha test values with a small tolerance. Fixes flickering foliage, fences and hair on NVIDIA GPUs in some games. |
| `force_depth_clamp` | `false` (default), `true` | Clamps depth instead of cutting geometry at the near and far planes. Fixes geometry that disappears close to the camera or at the horizon in some games. On Vulkan needs `depthClamp` support. |
| `window_offset_in_edram` | `true` (default), `false` | Places every screen tile of games that render in tiles at the same position. Fixes depth artifacts in the bottom part of the screen; turn off if tiles start shifting or showing bands. |
| `execute_unclipped_draw_vs_on_cpu` | `true` (default), `false` | Computes on the CPU how much of the screen some full-screen draws cover. Fixes a black scene behind the HUD in some games. |
| `gpu_allow_invalid_fetch_constants` | `false` (default), `true` | Draws even when a game gives the GPU broken vertex or texture descriptors, instead of skipping the draw. Can bring back missing objects; can also show garbage. |
| `texture_cache_memory_limit_soft`, `texture_cache_memory_limit_hard` | 384 / 768 MB (defaults) | How much GPU memory textures may use before old ones are dropped (soft) or before dropping is forced (hard). Raise them with high `resolution_scale` on GPUs with plenty of memory if textures reload often. |
| `texture_cache_memory_limit_soft_lifetime` | 30 s (default), 1 to 3600 | Above the soft limit, how long a texture may stay unused before it is dropped. |
| `texture_cache_memory_limit_render_to_texture` | 24 MB (default), 1 to 256 | Memory limit for textures the game renders into, with `resolution_scale`. |
| `draw_resolution_scale_threshold` | 0 (default: off), pixels | Render targets this narrow or narrower are not scaled by `resolution_scale`. Small offscreen buffers (bloom, depth of field) often break when scaled; keep the value as low as the broken effect needs (it is compared after alignment to 80-pixel tiles). Fast render target paths only. |
| `scaled_resolve_min_coverage_percent` | 0 (default), 1 to 100 | With `resolution_scale`, a texture the game also renders into is read from the scaled copy only if the game's renders cover at least this percentage of it. 0 means any overlap counts. Try 50 when some textures turn black above 1x, typically large textures that a render touches only at their small mip levels. |
| `draw_resolution_scaled_half_pixel_offset` | `false` (default), `true` | With `resolution_scale`, shifts drawing by half a scaled pixel instead of half an original pixel. Removes thin seams or lines where a game splits the screen into parts (tiling) or copies the image in pieces; with it on, the edge fill of `resolve_resolution_scale_fill_half_pixel_offset` is skipped. Can move the image by a fraction of a pixel. |
| `depth_float24_convert_in_pixel_shader` | `false` (default), `true` | Converts depth to the console's 24-bit float format in the pixel shader, exactly. Fixes Z-fighting and flicker where the game's depth values differ only in the last bits; slower (no early depth test). Fast render target paths only. |
| `depth_float24_round` | `false` (default), `true` | Rounds depth to the 24-bit float format instead of truncating it. Changes which surface wins in close ties; try it together with the setting above. |
| `half_pixel_offset` | `true` (default), `false` | Applies the console's half-pixel offset to geometry, as games expect. Turn off only if the whole image is shifted by half a pixel. |
| `non_seamless_cube_map` | `false` (default), `true` | Samples cube maps without filtering across their face edges. Try it if reflections or skies show wrong edges. |
| `mulsc_round_toward_zero` | `false` (default), `true` | Rounds one shader multiply instruction toward zero. Fixes broken geometry in several Volition engine games (Red Faction, Saints Row); not confirmed on real hardware, so off by default. |
| `spirv_pixel_shader_no_contraction` | `true` (default), `false` | Vulkan only. `false` lets the graphics driver fuse multiplies and adds in pixel shaders, as the console's GPU does not. Much faster on Apple GPUs through MoltenVK, with last-bit colour differences. Vertex shaders are not affected. Changes while the game runs: pixel shaders are translated again. |
| `vulkan_exact_vertex_math` | `false` (default), `true` | Vulkan only. Computes vertex math as the console's GPU does where the last bit can change a result by a whole unit, or everywhere on GPUs with the round-toward-zero shader mode. Fixes vertices that jump or explode and some Z-fighting, in games that show them on Vulkan. Slower vertex shaders on GPUs without that mode. Read at start. |
| `occlusion_query_fake_lower_threshold`, `occlusion_query_fake_upper_threshold` | 80 / 100 (defaults) | With `occlusion_query = fake`, the range of the made-up sample count. 0 makes everything read as hidden; -1 writes nothing (some games then hang). |
| `occlusion_query_full_counters` | `false` (default), `true` | Also counts depth-fail, stencil-fail and total samples for visibility queries. Few games use them; slow. |
| `occlusion_query_viz` | `false` (default), `true` | Emulates the console's conditional rendering (VIZ queries): draws found hidden are skipped. Some games use it against overdraw or for lens flares. Costs time with the `rov` / `fsi` paths. |

## Debugging

For bug reports and development; they slow the game down. Leave them at their defaults for play.

| Setting | Backend | What it does |
|---|---|---|
| `d3d12_debug` | D3D12 | Turns on the Direct3D 12 and DXGI debug layer (needs the Graphics Tools Windows feature). Its messages go to the log. |
| `d3d12_break_on_error`, `d3d12_break_on_warning` | D3D12 | With `d3d12_debug`, stops in the debugger on a validation error or warning. |
| `d3d11_debug` | D3D11 | Turns on the Direct3D 11 debug layer (needs the Graphics Tools Windows feature). Its messages go to the log after each presented frame. |
| `d3d11_debug_white_textures` | D3D11 | Puts plain white textures in place of all the game's textures, to tell a texture problem from a drawing problem. For checking the backend only. |
| `d3d11_log_frames` | D3D11 | Logs, for the first this many frames, how many draws and resolves the backend did, how many textures it bound and which image it showed. For reporting a black or wrong picture. |
| `d3d12_dred` | D3D12 | When the graphics card drops the game ("device removed"), logs the last GPU operations and the faulting address, without the debug layer. Small cost; turn on to report a GPU crash. |
| `vulkan_log_debug_messages` | Vulkan | Logs messages of the Vulkan validation layers when they are enabled (default `true`). |
| `vulkan_validation_enabled` | Vulkan | Turns on the Vulkan validation layers (the Vulkan SDK must be installed). |
| `frame_stats_interval` | all | Every this many seconds (0, off, by default), logs the game's frame rate and frame times between its frames: average, 95th and 99th percentile, the longest, and how many frames missed one or two 60 Hz refreshes. A second line gives the memory sent to the graphics card per frame, by use (vertex, index, texture, resolve, memexport, other), how much of it was sent twice in one frame, and how much was needed again only because of `clear_memory_page_state` (with upload hashing, also how much of that had changed). Costs nothing noticeable; works in every build type. Changes while the game runs; switching it off drops the frames not yet logged, so the next window starts fresh. |
| `gpu_debug_markers` | both | Labels GPU work for PIX and RenderDoc. On by itself when RenderDoc is attached. |
| `tiled_shared_memory` | both | `false` turns off tiled (sparse) GPU memory for the console's memory, for graphics debuggers that do not support it. |
| `dump_shaders` | both | A folder path: writes every game shader there as it is translated. |
| `shader_bisect_ps_hash`, `shader_bisect_register`, `shader_bisect_instruction`, `shader_bisect_probe`, `shader_bisect_cut` | Vulkan | For finding where a pixel shader goes wrong. Names one pixel shader by its 16-digit hash (as in the `dump_shaders` file names) and writes one of its registers to its first color output instead of its color: the value after instruction `shader_bisect_instruction` (counted from 0 in translation order; -1, the default, the final value). `shader_bisect_probe` 1, 2 or 3 writes white where a component is NaN, infinite, or at least 1 in magnitude. `shader_bisect_cut` also drops everything after that instruction. Read when a shader is translated, so set them at start; best with a [frame trace](gpu-traces.md) replayed for each step. |
| `d3d12_dxbc_disasm`, `d3d12_dxbc_disasm_dxilconv` | D3D12 | Writes the disassembly of translated shaders to the log. |
| `spirv_disable_rounding_mode_rte` | Vulkan | Leaves the round-to-nearest-even capability out of translated shaders, so RenderDoc can debug them (it does not support it). |
| `dxbc_source_map` | D3D12 | Adds the original console shader code to translated shaders as comments, for reading them in a debugger. |
| `d3d12_tessellation_wireframe`, `vulkan_tessellation_wireframe`, `d3d11_tessellation_wireframe` | D3D12, Vulkan, D3D11 | Draws tessellated geometry as wireframe. |
| `trace_gpu_prefix` | all | A folder: `bind_gpu_trace` (F10) then records the next frame into a `.xtr` file there. Empty (default): recording is off. See [GPU frame traces](gpu-traces.md). |
| `trace_replay` | all | A `.xtr` file: draws that frame instead of running the game, saves it as PNG and exits. With `trace_replay_output`, `trace_replay_reference`, `trace_replay_tolerance` (default 2) and `trace_replay_max_different_percent` (default 0). See [GPU frame traces](gpu-traces.md). |

## Advanced: leave at defaults

Workarounds and internal switches for developers and for reports of driver bugs. Changing them without a reason usually breaks rendering or costs speed.

| Setting | Default | What it does |
|---|---|---|
| `vulkan_require_geometry_shader`, `vulkan_require_fill_mode_non_solid` | `true` (`false` on macOS) | Refuse Vulkan GPUs without geometry shaders or line/point fill modes. `false` allows them with slower or less exact fallbacks. |
| `vulkan_require_fragment_stores_and_atomics`, `vulkan_require_vertex_pipeline_stores_and_atomics` | `true` | Refuse Vulkan GPUs without shader memory stores in pixel or vertex shaders (needed for `fsi` and memory export). |
| `vulkan_prefer_geometry_shader`, `vulkan_prefer_fill_mode_non_solid`, `vulkan_prefer_fragment_stores_and_atomics`, `vulkan_prefer_vertex_pipeline_stores_and_atomics` | `true` | With `vulkan_device = -1`, rank GPUs with these features first. |
| `vulkan_dynamic_rendering` | `true` | Uses `VK_KHR_dynamic_rendering` where supported, otherwise render passes. |
| `vulkan_sparse_shared_memory` | `true` | Uses sparse memory for the console's memory on Vulkan. |
| `gpu_idle_spin_count` | -1 (auto) | How many times the idle GPU thread checks for new commands before it sleeps. Auto: 501 with 6 or more logical CPUs, 0 (sleep at once) on 2-4 core PCs, where spinning takes time from the game's threads. |
| `vulkan_reuse_texture_descriptors` | `true` | A draw that binds the same textures and samplers as an earlier draw in the same submission reuses its descriptor set instead of writing a new one. Less CPU per draw; `false` only to rule it out when chasing a texture bug. |
| `vulkan_semaphore_reuse_workaround` | `false` | Waits for the present queue before each frame. Can fix corruption on some drivers; large speed loss. |
| `vulkan_depth_bias_force_unorm` | `true` | Keeps depth bias the same size at every depth on float depth buffers where `VK_EXT_depth_bias_control` allows it. Otherwise decals may Z-fight, more so with reversed depth. With it off (or without the extension), `depth_bias_shader_offset_scope = auto` treats Vulkan like Direct3D. |
| `vulkan_force_convert_quad_lists_to_triangle_lists`, `vulkan_force_expand_point_sprites_in_vs`, `vulkan_force_expand_rectangle_lists_in_vs` | `false` | Use the vertex-shader fallbacks for quads, point sprites and rectangles even when geometry shaders exist. |
| `force_convert_quad_lists_to_triangle_lists`, `force_convert_triangle_fans_to_lists`, `force_convert_line_loops_to_strips` | `false` | Convert these primitive types on the CPU even when the GPU can draw them. |
| `primitive_processor_cache_min_indices` | 0 | Smallest index count whose converted index buffer is cached. |
| `native_stencil_value_output`, `native_stencil_value_output_d3d12_intel` | `true`, `false` | Writes stencil values directly from shaders where the GPU supports it (off on Intel with D3D12 because of driver bugs). |
| `snorm16_render_target_full_range` | `true` | Uses the full value range for 16-bit signed normalized render targets. |
| `direct_host_resolve` | `true` | Copies render targets straight to the console's memory when possible. |
| `pre_mask_resolve_l2_block` | `true` | With `resolution_scale`, masks scaled copy blocks to the written range before copying. |
| `resolve_resolution_scale_fill_half_pixel_offset` | `true` | With `resolution_scale`, fills the half-pixel border of scaled copies. |
| `mrt_edram_used_range_clamp_to_min` | `true` | With several render targets bound, limits the EDRAM range of each one to where the next one starts. |
| `depth_transfer_not_equal_test` | `true` | Uses a not-equal test when moving depth between render targets, so unchanged pixels are skipped. |
| `aliased_depth_read_only` | `true` | When color and depth share EDRAM and color writes only unused stencil bits, keeps depth bound read-only, as the exact paths do. |
| `no_discard_stencil_in_transfer_pipelines` | `false` | Skips stencil bit discard when moving render targets. Can be faster on some GPUs. |
| `draw_resolution_scaled_texture_offsets` | `true` | Scales texture coordinate offsets with `resolution_scale`. |
| `dxbc_switch` | `true` | D3D12: uses switch statements for shader flow control. |
| `gpu_3d_to_2d_texture` | `true` | Samples some 3D textures through 2D wrappers, which drivers handle better. |
| `gpu_allow_invalid_upload_range` | `false` | Lets games read GPU data from memory pages marked as no-access. |
| `execute_unclipped_draw_vs_on_cpu_with_scissor`, `execute_unclipped_draw_vs_on_cpu_for_psi_render_backend` | `false`, `true` | Variants of `execute_unclipped_draw_vs_on_cpu`: also with a scissor rectangle; also on the `rov` / `fsi` paths. |
| `present_render_pass_clear` | `true` | Vulkan: clears the window image before drawing the game image into it. |
| `gpu_plugin` | `""` | Which GPU plugin library to load; empty uses the one the title is built with. |

Settings with a title prefix (for example `pdz_`, `wet_`, `fuse_`, `rdr_`) and settings such as `graphics_backend` or `framerate_limit` belong to the title, not to ReXGlue: see the title's own documentation.

## Driver and external tools

These work on the final image of any title and need nothing from it.

| Tool | GPUs | Backends | What it does | Notes |
|---|---|---|---|---|
| AMD Fluid Motion Frames (AFMF) | AMD Radeon | D3D12; Vulkan with AFMF 2 | Frame generation in the driver | Enable in AMD Software. Adds latency; HUD can show artifacts. |
| NVIDIA Smooth Motion | NVIDIA RTX 40 and newer | D3D12, Vulkan | Frame generation in the driver | Enable in the NVIDIA app per program. Same trade-offs as AFMF. |
| Lossless Scaling | any | any | Frame generation and spatial upscaling in a separate window | Paid third-party app. |
| NVIDIA RTX HDR | NVIDIA RTX | D3D12, Vulkan | Converts the SDR image to HDR | Enable in the NVIDIA app. |
| Windows Auto HDR | any, HDR display | D3D12 only | Converts the SDR image to HDR | Windows 11 display settings. Use the D3D12 backend for it. |
| ReShade | any | D3D12, Vulkan | Post-processing shaders (sharpening, color, SMAA) | Install for the API the title uses. Effects that need depth (ambient occlusion, depth of field) are unreliable: the game's depth is not kept in a normal depth buffer. |

Driver features change between driver versions; check the vendor's current list if one does not appear for a title.

Frame generation happens after the game has drawn its frame, so it does not change game speed or timing. It works best when the game already runs at a stable frame rate (for example 30 or 60).
