# WetRecomp

<div align="center">
  <img src="assets/icon.png" alt="WetRecomp" width="480">
</div>

A static recompilation of [**WET**](https://en.wikipedia.org/wiki/Wet_(video_game)) (2009, Xbox 360, Title ID `425307DB`) to native
Windows x86-64, built on the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk).

Static recompilation translates the Xbox 360 PowerPC code inside the game's
`default.xex` into native C++ that compiles and runs on a PC. There is no
emulator and no interpreter in the loop; file I/O, GPU commands, audio and
threading go through the ReXGlue runtime.

## Status

- Codegen runs clean (0 analysis errors).
- The build compiles and the executable boots to real Direct3D 12 rendering
  and audio; the game is playable in short bursts before hitting the next
  missing-function crash.
- Iterative, not finished: every session tends to surface one or two new
  missing addresses or access violations further into the game. See
  [Known issues](#known-issues-and-difficulties-encountered) below.
- Framerate was low (~20 FPS) due to a confirmed, fixed root cause: a
  busy-spin in a rate-limiter function that codegen accidentally turned into
  a zero-cost loop (millions of calls/sec instead of a real wait). See
  [Known issues](#known-issues-and-difficulties-encountered) below for
  details and the fragile-patch caveat.

## Requirements

- CMake 3.25+
- Ninja
- Clang / LLVM (clang-cl works too) - MSVC alone will not build this
- The [ReXGlue SDK release archive](https://github.com/rexglue/rexglue-sdk/releases) - see [`rexglue/README.md`](rexglue/README.md)
- [extract-xiso](https://github.com/XboxDev/extract-xiso/releases) to unpack the Xbox 360 ISO
- Your own legally-owned copy of WET, extracted from the Xbox 360 disc/ISO

## Getting the SDK

Download the `win-amd64` release from the
[ReXGlue SDK releases page](https://github.com/rexglue/rexglue-sdk/releases)
and extract it into `rexglue/win-amd64` at the repo root. Full steps in
[`rexglue/README.md`](rexglue/README.md). The SDK itself is gitignored; only
that README is checked in.

## Getting the game data

1. Extract the Xbox 360 ISO with
   [extract-xiso](https://github.com/XboxDev/extract-xiso/releases)
   (e.g. `extract-xiso.exe -x WET.iso`), which unpacks the disc's file tree.
2. Copy the extracted contents directly into `assets/`, so it looks like:
   ```
   assets/
     default.xex
     nxeart
     lu0/
     lu1/
     media/
     movies0/
     movies1/
     streams0/
     streams1/
   ```
3. `assets/` is gitignored (`assets/*` in `.gitignore`) - nothing from the
   disc is, or should be, committed to this repo.

## Build

```powershell
cmake --preset local-win-relwithdebinfo
cmake --build out\build\local-win-relwithdebinfo
```

```bash
cmake --preset local-lin-relwithdebinfo
cmake --build out/build/local-lin-relwithdebinfo
```

Other available presets: `local-win/lin-debug`, `local-win/lin-release`. These are the
`CMakeUserPresets.json` presets (gitignored file, already set up in this repo)
that inherit the platform presets from `CMakePresets.json` and add
`CMAKE_PREFIX_PATH` pointing at the vendored SDK. Building against the bare
`win-amd64-*` presets from `CMakePresets.json` directly will fail with
"ReXGlue SDK not found" - always use the `local-*` presets.

Codegen (translating `assets/default.xex` into `generated/default/*.cpp`) runs
automatically as a build step (`wetrecomp_codegen` CMake target) whenever
`wetrecomp_manifest.toml` or an included `.toml` changes.

## Run

```powershell
cd out\build\local-win-relwithdebinfo
.\wetrecomp.exe
```

```bash
cd out/build/local-lin-release
./wetrecomp
```

Both `--game_data_root` and `--gpu_plugin` are optional now: `OnConfigurePaths()`
in [`src/wetrecomp_app.h`](src/wetrecomp_app.h) defaults `game_data_root` to
`<repo_root>/assets` (found by walking up from the exe folder) when it isn't
set via flag/env var, and `gpu_plugin = "xenos"` already lives in
[`settings/hardware.toml`](settings/hardware.toml). Pass either flag to
override - useful for pointing at a game dump kept elsewhere.

Useful extra flags/env vars while developing:

| Flag / env var | Effect |
|---|---|
| `--game_data_root <path>` | Overrides the default `<repo_root>/assets` game-files location. |
| `--gpu_plugin xenos` | Overrides `settings/hardware.toml`'s `gpu_plugin`. Only needed if you want a different plugin than the file specifies. |
| `--graphics_backend d3d12\|vulkan\|any` | Forces the graphics API `rexgpu-xenos` uses (cvar, default `"any"`, which picks D3D12 first). See [`settings/README.md`](settings/README.md) for how this is wired up. |
| `--vsync=false` | Disables vsync (cvar, default `true`). Doesn't affect the busy-spin fix below - kept for reference from earlier perf testing. |
| `--wet_fps60_unlock=true` | Experimental, off by default - ported xenia-canary `game-patches` "60 FPS" patch. See [`settings/README.md`](settings/README.md). |
| `WET_NO_STUB_SWEEP=1` (env var) | Disables the safety-net stub sweep (see below) - useful to isolate whether it's contributing to the framerate issue, at the cost of hitting FATAL crashes on any address not yet in `default_functions.toml`. |

Logs are written to `out\build\<preset>\logs\*.log` (the exe is built `WIN32`,
so nothing prints to the console).

## Configuration

Rendering/window/vsync and input-backend defaults are checked in under
[`settings/`](settings/README.md) (`hardware.toml` / `mapping.toml`), loaded
automatically at startup. CLI flags and `REX_*` environment variables always
override them - see [`settings/README.md`](settings/README.md) for the full
reference and precedence rules.

## How this project was set up (history)

1. `rexglue init --project-name WetRecomp --xex-path assets\default.xex`
   generated `CMakeLists.txt`, `CMakePresets.json`, `wetrecomp_manifest.toml`,
   `generated/rexglue.cmake`, `src/main.cpp`, `src/wetrecomp_app.h`.
2. First `rexglue codegen wetrecomp_manifest.toml` failed analysis with 10
   `UnresolvedCall` errors (plain `b` branches to addresses the auto-analyzer
   never registered as functions, because nothing called them with `bl`).
   Fixed by adding each address to `default_functions.toml` under
   `[functions]` as `0xADDRESS = {}` (empty table - lets codegen discover the
   function's natural boundary itself instead of requiring a manual size),
   and including that file from `wetrecomp_manifest.toml`
   (`includes = ["default_functions.toml"]`).
3. Added `CMakeUserPresets.json` (`local-debug`/`local-release`/`local-relwithdebinfo`)
   so `CMAKE_PREFIX_PATH` finds the SDK without touching the generated,
   overwritable `CMakePresets.json`.
4. Added `GPU_PLUGINS xenos` to the `rexglue_setup_target()` call in
   `CMakeLists.txt` so `rexgpu-xenos*.dll` gets staged next to the exe.
5. First runtime boot hit a chain of `[FATAL] Call to invalid or unregistered
   function at guest address 0x...` crashes - addresses only ever reached
   through a function pointer / vtable slot, so static analysis had no call
   edge pointing at them. Each one had to be added to `default_functions.toml`
   the same way as step 2.
6. Replaced the one-crash-per-rebuild loop with a **stub sweep**
   (`OnPostSetup()` in `src/wetrecomp_app.h`): at startup, every address in
   the code range that isn't already a registered function gets a no-op
   safety-net stub instead of crashing, and every call to one is logged
   (deduplicated) to `logs/stub_sweep.txt`. One play session now surfaces
   several missing addresses at once instead of one FATAL per run. New
   addresses found this way get appended to `default_functions.toml` and the
   game is rebuilt/rerun; repeat until a session runs clean or hits a
   different kind of bug.

## Known issues and difficulties encountered

- **Manual function boundaries are an ongoing, iterative process.** WET's
  code has call sites the static analyzer cannot see ahead of time (plain
  branches to un-analyzed code, and indirect calls through vtables/callback
  tables resolved only at runtime). There is no way to find them all up
  front; `default_functions.toml` grows one play session at a time via the
  stub sweep described above.
- **Access violations from stubbed calls.** A stubbed function is a no-op -
  it does not set a return value. If the caller dereferences whatever was
  left in `r3`, that can crash with an unrelated-looking guest access
  violation until the real function is added to `default_functions.toml` and
  actually executes.
- **Low framerate (~20 FPS), root cause found and fixed.** GPU sat
  near-idle during play while the CPU stayed busy - a CPU-bound problem, not
  GPU/render. Traced via CPU-usage profiling plus a call counter (added by
  editing the generated `.cpp` directly, since these are direct C++ calls,
  not indirect calls through the `FunctionDispatcher` - `SetFunction()`
  hooks can't see them) to `sub_83066340`, a rate-limiter ("has enough time
  passed?") called from a retry loop in `sub_8305E9E0`. Its body has a
  `db16cyc`-based delay loop that was a real ~160ns hardware pause on Xenon;
  codegen drops `db16cyc` entirely, turning it into a zero-cost spin -
  measured at **~26.8 million calls/sec** in real gameplay. Fix: a real
  `std::this_thread::sleep_for(std::chrono::microseconds(15));` inserted at
  that point in `generated/default/wetrecomp_recomp.154.cpp` (`yield()` was
  tried first and barely helped - it returns instantly when nothing else is
  contending for the core). Dropped the call rate to ~500/sec in real
  gameplay; this function and its caller chain vanished from the CPU-usage
  profiler's hot list entirely (were 40-70%+ of total CPU before).
  - **Caveat: this patch lives in a codegen-generated file.** It survives
    normal rebuilds (nothing regenerates `wetrecomp_recomp.154.cpp` unless
    `wetrecomp_manifest.toml`/`default_functions.toml`/`assets/default.xex`
    change), but this SDK version (0.10.0) has no hook/override mechanism to
    make it regen-proof. If codegen ever regenerates that file, re-add
    `std::this_thread::sleep_for(std::chrono::microseconds(15));` (with
    `#include <chrono>` / `#include <thread>`) right after the `db16cyc`
    loop inside `DEFINE_REX_FUNC(sub_83066340)`, before the `// cctpm`
    comment.
  - Ruled out along the way: the stub sweep (benchmarked, no measurable
    difference), `--vsync=false` (no help), D3D12 debug layer (off by
    default already), and a Vulkan switch (rejected - the prebuilt Windows
    release SDK is D3D12-only, no Vulkan libraries are bundled in
    `rexglue/win-amd64/lib`, and it's a CMake-time flag, not a runtime one).
  - Remaining hot spots after the fix look like normal costs of a
    recompiled title: DXGI `Present` (vsync wait), the Xenos GPU-command
    emulation itself, and a spread of small per-object update calls - no
    single dominant bottleneck left.
- The `rexglue.exe` CLI must be invoked through the `local-*` CMake presets,
  not the bare presets in `CMakePresets.json` - see [Build](#build).

## Credits

- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) ([releases](https://github.com/rexglue/rexglue-sdk/releases))
- [extract-xiso](https://github.com/XboxDev/extract-xiso) ([releases](https://github.com/XboxDev/extract-xiso/releases)) -
  used to unpack the Xbox 360 ISO into the file tree copied into `assets/`
- [xenia](https://github.com/xenia-project/xenia) / [xenia-canary](https://github.com/xenia-canary/xenia-canary) -
  ReXGlue's runtime is derived from Xenia's, and the
  [compatibility list](https://github.com/xenia-canary/game-compatibility/issues)
  was used to help pick this game as a recompilation target.
