# GPU frame traces

A frame trace is one frame of a game as the console GPU saw it: the commands it ran, the
memory they read and the GPU state the frame started with. Replaying it draws that frame
again without the game, so a graphics change can be checked against saved reference
images in seconds, on every backend.

Traces contain game data (textures, models). Keep them and their reference images
private, do not commit them.

## What it checks

A replay runs the same GPU emulation as the game: shader translation, render targets and
EDRAM, textures, resolves, the swap and its post-processing. A change in any of them shows
up as a different frame.

It does not check anything outside the GPU: the recompiled code, the kernel, audio, input
or frame pacing. Each trace is one frame, so effects that build up over several frames
are only seen as they were at the start of that frame.

## Settings

| Setting | Default | What it does |
|---|---|---|
| `trace_gpu_prefix` | `""` (recording off) | Folder that recorded traces go to. |
| `bind_gpu_trace` | `F10` | Key that records the next frame. |
| `trace_replay` | `""` | A `.xtr` file to draw instead of running the game. |
| `trace_replay_output` | `""` (beside the trace) | Where to save the replayed frame as PNG. |
| `trace_replay_reference` | `""` (no comparison) | Reference PNG to compare the frame with. |
| `trace_replay_tolerance` | 2 | How much red, green or blue may differ (0 to 255) before a pixel counts as different. |
| `trace_replay_max_different_percent` | 0 | Share of different pixels, in percent, still accepted as a match. |

All of them except `bind_gpu_trace` are read at start.

## Recording

1. Set `trace_gpu_prefix` to a folder, for example `--trace_gpu_prefix=D:/traces`.
2. Keep `resolution_scale` at 1: with scaling the EDRAM and the scaled resolves are not
   recorded.
3. In the game, press `bind_gpu_trace` (F10 by default). The next frame is written to
   `<title id>_<vblank>.xtr` in the folder; the log says when it is done. The game keeps
   running, and pressing the key again records another frame.

The recorded frame stutters: everything it uses is uploaded to the GPU again so that it
lands in the trace. Traces are not compressed; a frame of a large scene can take hundreds
of megabytes.

Direct3D 12 and Vulkan record everything. Direct3D 11 records without the EDRAM and the
memory the GPU wrote in earlier frames, so frames that read last frame's render targets
come out wrong from a Direct3D 11 recording.

A trace starts at a swap, so the frame that ends with the next swap is recorded. Not
recorded: what guest threads write while the GPU waits on memory, and the predicated
tiling bin state; a frame depending on them may differ from the game.

## Replaying

Run an executable built with ReXGlue with the trace:

```
game.exe --game_data_root=<any folder> --trace_replay=D:/traces/4D5307E6_1234.xtr
```

The title is not loaded, so the executable of any game works, and `game_data_root` only
has to be an existing folder. The window opens, the frame is drawn, and the process exits
by itself. Shaders are compiled synchronously, so nothing is skipped, and waits of the GPU
on memory pass at once.

A trace can be replayed on another backend than the one it was recorded on. The EDRAM is
restored only on Direct3D 12 and Vulkan, and only with `resolution_scale` at 1.

The frame is saved as PNG beside the trace (or to `trace_replay_output`). The exit code
says how it went:

| Exit code | Meaning |
|---|---|
| 0 | Drawn, and matching the reference if one was given. |
| 1 | Different from `trace_replay_reference`, or the reference can't be read or has another size. When pixels differ, a `.diff.png` beside the output shows them in white over a dimmed copy of the frame. |
| 2 | Not drawn: the trace can't be read, is cut off, ends before its swap, or the frame didn't reach the presenter within 5 seconds. |

The log of the run says which one it is and why.

A pixel counts as different when its red, green or blue differs by more than
`trace_replay_tolerance`; alpha is ignored. `trace_replay_max_different_percent` accepts
that share of different pixels.

Reference images depend on the backend, the GPU and its driver: keep one set per
backend and machine.

Traces carry a format version. A build with another version refuses them (exit code 2,
the log names both versions); record them again.

## Finding a wrong value in a shader

On Vulkan, the `shader_bisect_*` settings ([faq.md](faq.md), "Debugging") make one pixel shader write a register to its color instead of its result. Replaying the same trace with `shader_bisect_instruction` moved step by step shows in the saved frame where the value goes wrong; `shader_bisect_probe` marks NaN, infinite or large values, which an 8-bit image would hide. The replay's log names the shader and the instruction count.

## Checking many traces

`thirdparty/rexglue-sdk/tools/gpu_traces/compare.py` (Python 3) replays every `.xtr` in a folder and compares each
frame with `<trace>.<label>.png`:

```
python thirdparty/rexglue-sdk/tools/gpu_traces/compare.py game.exe D:/traces --label vulkan -- --game_data_root=D:/game
python thirdparty/rexglue-sdk/tools/gpu_traces/compare.py game.exe D:/traces --label vulkan --update -- --game_data_root=D:/game
```

- `--label` names the set of references, one per backend and machine (default
  `reference`).
- `--update` saves the frames as the new references instead of comparing; only frames
  that were drawn are saved.
- `--timeout` is the time per trace in seconds (default 120); a trace that takes longer
  counts as not drawn.
- Arguments after `--` go to the game, for example the backend to use.

Each replayed frame is kept as `<trace>.<label>.last.png`, with `.last.diff.png` next to it
when it differs. A trace without a reference counts as failed. The script prints one line
per trace and exits with 0 when all of them pass, 1 otherwise.
