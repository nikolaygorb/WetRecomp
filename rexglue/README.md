# rexglue - ReXGlue SDK (not checked in)

This folder holds the prebuilt ReXGlue SDK release used to build and run
WetRecomp. Everything under here except this file is gitignored (see
`.gitignore`: `rexglue/*` / `!rexglue/README.md`) - the SDK is ~200 MB of
binaries and every contributor fetches their own copy.

## Setup

1. Download the `win-amd64` release archive matching the SDK version pinned
   in [`wetrecomp_manifest.toml`](../wetrecomp_manifest.toml) (`sdk_version`,
   currently `0.10.0`) from the
   [ReXGlue SDK releases page](https://github.com/rexglue/rexglue-sdk/releases).
2. Extract it here so the layout looks like:
   ```
   rexglue/
     README.md          (this file)
     win-amd64/
       bin/
       include/
       lib/
       share/
       ...
   ```
3. That's it - [`CMakeUserPresets.json`](../CMakeUserPresets.json) already
   points `CMAKE_PREFIX_PATH` at `rexglue/win-amd64`, relative to the repo
   root, so no absolute paths need editing.

See the root [`README.md`](../README.md) for full build/run instructions.
