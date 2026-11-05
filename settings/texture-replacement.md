# Texture replacement: HD texture packs

Titles built with ReXGlue can draw the game's textures from image files instead of the game's own data. This lets you use upscaled textures (for example made with an AI upscaler), fixed textures, or your own pictures without changing the game's files. The settings are listed in [faq.md](faq.md#texture-replacement-hd-texture-packs). This page explains how the feature works and how to make a pack.

## How it works

The Xbox 360 game loads its textures into console memory. When the GPU emulation first meets a texture, it takes the texture's top mip level, puts it into a fixed layout (untiled and byte-swapped) and hashes it (XXH64). That hash is the texture's **key**. It depends only on the texture's pixels, not on where the texture sits in memory, so it stays the same between runs and between levels.

A **pack** is a folder with an `index.tsv` file and DDS images. Each line of `index.tsv` maps a key to a DDS file. When a texture's key is in the pack, the emulator creates the GPU texture from the DDS file and never loads the game's data into it. The DDS may be larger than the original: 2x, 4x, 2.5x, any size with the same aspect ratio. Shaders still see the original size, so texture coordinates are unchanged, and the GPU picks the right mip level from the larger image.

The emulator keeps watching the original memory. When the game overwrites it (another texture streamed into the same place), the key is computed again. Then the texture goes back to the game's data, or becomes another replacement if the new data is in the pack too.

What can be replaced:

- 2D textures in DXT1 (BC1), DXT3 (BC2), DXT5 (BC3), DXN (BC5 / ATI2) or 32-bit RGBA (`k_8_8_8_8`). These hold most textures of Xbox 360 games.
- The DDS must be in the **same format** as the original: a DXT5 texture needs a DXT5 DDS.

What cannot be replaced: textures the game renders on the GPU (render targets, shadow maps, reflections, post-processing), cube maps, 3D textures and texture arrays, and textures in other formats (16-bit, float, signed).

## Making a pack

### 1. Dump the textures

Add to the title's settings file (for example `settings/hardware.toml`):

```toml
texture_replacement_dump_path = "D:/packs/my_game"
```

Start the game and play through the places whose textures you want. Every texture that can be replaced is written once, when it is first loaded:

```
D:/packs/my_game/
  index.tsv
  0a3f9c51d2e47b18.dds
  3b0e77a4c19f2d06.dds
  ...
```

`index.tsv` has one tab-separated line per texture: the key, the original width, height and format number (18 = DXT1, 19 = DXT3, 20 = DXT5, 49 = DXN, 6 = 32-bit RGBA), and the file name:

```
0a3f9c51d2e47b18	256	256	20	0a3f9c51d2e47b18.dds
3b0e77a4c19f2d06	512	128	18	3b0e77a4c19f2d06.dds
```

The dump folder is already a working pack: each texture is replaced by itself. Dumping skips textures that are already in the folder, so you can dump over several sessions. Turn dumping off when you are done: it costs time whenever a new texture is loaded.

### 2. Find the textures you want

To find the key of a texture you see on screen, set `texture_replacement_log = true` while a pack is loaded. Every texture that could be replaced is logged when it is created:

```
Texture replacement: 0a3f9c51d2e47b18 256x256 format 20 at 0x1A2B3000: replaced with D:/packs/my_game/0a3f9c51d2e47b18.dds (512x512, 2.00x, 10 levels)
Texture replacement: 5c12e0f7a98b3d44 128x128 format 18 at 0x1A2C0000: not in the pack
```

A DDS viewer with thumbnails (for example in Windows Explorer with a DDS plugin, or Paint.NET) makes browsing the dump easier.

### 3. Edit or upscale

Edit the DDS files you want to change. Keep:

- the **file name** (it is the key);
- the **format** (DXT1 stays DXT1, DXT5 stays DXT5, and so on);
- the **aspect ratio** (a 256x128 texture may become 512x256 or 1024x512, not 512x512);
- a **legacy DDS header** (no DX10 header).

Add **mip levels** too. Without them, a large texture flickers and shimmers at a distance. The emulator uses at most as many levels as needed to get back down to the original size.

Example with Microsoft's [texconv](https://github.com/microsoft/DirectXTex/wiki/Texconv) and any image upscaler. For a DXT5 texture:

```bat
rem DDS to PNG
texconv -ft png -o work 0a3f9c51d2e47b18.dds

rem upscale work\0a3f9c51d2e47b18.png 2x with your upscaler, into upscaled\

rem PNG back to DDS: same format (BC3 = DXT5), full mip chain, legacy header
texconv -f BC3_UNORM -m 0 -dx9 -y -o D:\packs\my_game upscaled\0a3f9c51d2e47b18.png
```

Use `BC1_UNORM` for DXT1, `BC2_UNORM` for DXT3, `BC3_UNORM` for DXT5, `BC5_UNORM` for DXN and `B8G8R8A8_UNORM` for 32-bit textures. `-dx9` writes the legacy header the emulator reads.

DXN (BC5) textures are usually normal maps: upscale their two channels as data, not as a picture. Many upscalers damage normal maps.

32-bit textures are dumped in the byte order the game uses for A8R8G8B8. A few games keep other channel orders in that format, and their dumps look color-swapped in an editor. Edit them as they are and save them in the same order: the round trip is exact.

### 4. Use the pack

Delete the lines and files of textures you did not change (optional, it saves memory), and point the title at the folder:

```toml
texture_replacement_path = "D:/packs/my_game"
```

Restart the game: the pack is read once at startup. The log says how many textures the pack has. A broken file is skipped with one log line, and the original texture is used.

## Settings

| Setting | Default | Use |
|---|---|---|
| `texture_replacement_path` | empty (off) | The pack. Read once at startup. |
| `texture_replacement_user_path` | empty | A second pack, read after the first. A texture in both comes from this one. Its images may not be more see-through than the originals: an opaque original makes the image opaque. Use it for your own changes on top of a shared pack. |
| `texture_replacement_enabled` | `true` | Turns replacement off and on while the game runs, to compare with the originals. |
| `texture_replacement_allow_list` | empty | A file of keys, one per line (`#` starts a comment): only these textures are replaced, and each is checked against the original's alpha. Can change while the game runs. Meant for online play, where only a few harmless textures should change. |
| `texture_replacement_memory_limit` | 4096 (MB) | Most GPU memory the replaced textures may use at once. A texture over the limit is drawn from the game's data. A 2x pack uses about 4 times the memory of the originals, 4x about 16 times. 0 = no limit. |
| `texture_replacement_log` | `false` | Logs every texture that could be replaced and what happened to it. For making packs, not for play. |
| `texture_replacement_dump_path` | empty (off) | Writes textures to this folder as a pack. Read once at startup. |

## Pack format

For pack tools:

- `index.tsv`: UTF-8, one entry per line: `<key>\t<width>\t<height>\t<format>\t<file>`. `key` is 16 hex digits, `width`, `height` and `format` are the original texture's (the format as the Xenos number above), and `file` is a path relative to the pack folder with `/` separators, no `..`, no drive letter, at most 255 bytes. Lines starting with `#` are comments; malformed lines are skipped. At most 64 MB.
- The key is XXH64 (seed 0) of the original top mip level: untiled into rows of whole blocks without padding, then byte-swapped as the texture fetch constant's endianness says. A top level of 16 texels or less is read from its place in the packed mip tile.
- DDS: legacy header only (FourCC `DXT1`, `DXT2`/`DXT3`, `DXT4`/`DXT5`, `ATI2`/`BC5U`, or 32-bit RGB with whole-byte masks), no cube map or volume, sides 1 to 16384, the file exactly as long as its declared levels. 32-bit images are rearranged into B, G, R, A, the memory order of the original after the byte swap.

## Performance and limits

- With no pack and no dump set, nothing changes.
- With a pack, every new or reloaded texture of a supported format is hashed once, which is about the cost of copying it.
- The first use of a replaced texture reads its file from disk on the GPU thread: a large pack can add short hitches the first time new textures appear.
- Only textures whose pixels the game loads from its files can be replaced. A texture the game builds or changes at run time gets a new key every time it changes.
