# vt4096 -- A "functional enough" Terminal Emulator for Windows in less than 4 kilobytes

This repo is a proof of concept that it is possible to make a useful (for some definition of useful) terminal app for Windows in less than 4 kilobytes.

## Features
- [x] Final release executable file is smaller than 4096 bytes. As of 2026-06-11 it is 4032 bytes (`Compress` target with slow Crinkler)
- [x] Absolutely no error checks or error messages whatsoever in Release mode.
- [x] Unicode output support.
- [x] Basic support for minimalistic subset of VT100/ANSI ESC sequences: cursor positioning, clear, color.
- [x] Unicode input support, including UTF-16 surrogates.
- [x] Runs:
    - `cmd.exe` (hardcoded) providing a fully functional shell.
    - [Lick Weed by Brainstorm (TMDC 2011)](https://www.pouet.net/prod.php?which=58419)
    - [Litterae Finis by Trauma (TMDC 2012)](https://www.pouet.net/prod.php?which=60938)
    - [Neovim](https://neovim.io/) (with minor glitches)
        - Functional enough to be 100% dogfooded -- significant portions of this terminal were written in neovim running inside it 😏.
    - [Far Manager](https://farmanager.com/) (basics are functional)
        - Lacks Ctrl/Alt-F# combination support

## TODO
- [ ] Clipboard.
- [x] Functional and extra keys support.
- [ ] Better VT100/ESC parser -- empirically figure out the minimal subset to support more apps, and to fix existing glitches. Run more demos.
- [x] Actually work on binary size. ~~Zero~~ Only minimal effort has been spent on making it small.
- [ ] Wide Unicode glyphs support.
- [ ] Fallback font support for emojis, etc.
- [ ] Run arbitrary programs specified by a CLI arg.
- [ ] Unit tests for the basic pseudoterminal ESC parser/state machine.

## Limitations and known issues
- Does not support many of well established ESC sequences, and has no intention of having a complete parser (good luck fitting xterm spec into 4 kilobytes).
- Does not support emojis out of the box, as Windows builtin fonts don't contain them.
- Won't support color fonts, as GDI functions are unable to render them, and DirectWrite is a COM nightmare.
- Dynamic font atlas is limited to a hardcoded number of (currently 4096) unique glyphs.

## How to build
This project Windows-only by design and requires Microsoft Visual Studio 2022.

### Download dependencies
This project needs:
1. [Crinkler](https://github.com/runestubbe/Crinkler) for final linking and compression
    - Download the latest version (3.0a is known to work), and copy `Win64/Crinkler.exe` as `link.exe` to the root of this repo.
2. [Shader Minifier](https://github.com/laurentlb/shader-minifier) for, gasp, shader minification
    - Download the latest version (1.6.0 is known to work), and save `shader-minifier.exe` to the root of this repo.

### Build
1. Open `vt4096.sln`
2. Select `Compress` solution configuration.
3. Hit `F7` or `Build -> Build Solution`.
4. Wait a few dozen seconds for it to be compressed, Crinkler will show some progress windows.
5. `Compress/vt4096.exe` will appear.

### Debug/development
Use `Debug` configuration. It dumps extra debugging data (unhandled ESC sequences, etc) into the Debug window.

Note that shader is compressed even in the Debug mode, so it is pretty obfuscated and unreadable. It would be possible
to avoid that, but I was lazy -- minification step was added later, after the shader was stabilized.

## Technical details
### Overview
- Uses [ConPTY API](https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/) (available since Windows 10 October 2018 update, 1809) to create a pseudoconsole session.
- Two pipes are used to communicate with the shell or other running app. ESC/VT100/ANSI sequences are used both ways to communicate metadata (cursor pos, colors, etc.).
- Uses OpenGL for rendering. Screen is a uniform grid, with each cell being a single glyph.
- A very simple fullscreen fragment shader picks a glyph from an atlas based on current cell coordinates, and draws it there.
- Regular WinAPI GDI functions (`CreateDIBSection()`, `CreateFontW()`, `TextOutW()`) are used to render glyphs into the atlas.
- Atlas is filled dynamicall as new glyphs are seen.
- Uses extremely efficient glyph cache hash table with `u32 hash(u32 codepoint){ return codepoint; }` perfect oracle hash function. There are only 64k UTF-16 Unicode chars, or just slightly more 1M in full Unicode spec with surrogates. Each entry in the table is just 2 bytes (and could be 4 bytes at worst), so the entire thing would be just a few megs max.
- Contains minimal VT100 ESC sequences parser, covering just the sequences I encountered while running random programs and demos I had lying around.
- Linked-compressed into <4k executable using [Crinkler](https://github.com/runestubbe/Crinkler).

### Architecture overview
- `vt4096.c` -- the main module.
    - contains entry point, initialization sequence, window creation, event loop and input handling.
- `font.c` -- font atlas.
    - provides dynamic font atlas bitmap and glyph sizes (to e.g. calculate window size).
    - uses oldschool GDI for glyph rendering.
- `terminal.c` -- main terminal state management.
    - Contains terminal escape sequence parsing.
    - Holds and modifies the screen grid contents, including codepoints and colors.
- `render.c` -- provides OpenGL-based grid rendering
    - Consumes `font.c` atlas bitmap, uploads it into OpenGL texture.
    - Consumes `terminal.c` grid, uploading it into OpenGL textures.
    - Uses `grid.glsl` (minified into `grid.h`) fullscreen fragment shader to render the grid based on the above textures. It determines current cell index based on pixel coordinates, and then samples corresponding glyph reference and colors textures.
- `shell.c` -- ConPTY and shell IPC.
    - Creates and maintains windows pseudo console with communication pipes.
    - Launches `cmd.exe` (hardcoded in `vt4096.c`) shell process under the pseudo console.
    - Launches a thread for blocking reads from console pipe.
        - Apparently noone at Microsoft heard the word about our lord and savior `select()` function. Because what WinAPI has instead is a complete unusable mess.
        - Read buffer is directly passed to `terminal.c`.
    - Provides blocking writes to pass user input.

## FAQ
### Why 4 kilobytes? 
It is a well established demoscene intro category.

### It eats up 1GiB of RAM at runtime! Less than 4k, my ass!
It is a well known behavior of Crinkler. It has a sophisticated code-dense decompressor that needs A LOT of RAM to run, so it will basically allocate 1G at startup even before any of the user code is ran, and will not free it, because why would you do that.
The terminal itself needs a just a few tens of megs, and even that is just because I avoid dynamic allocations and preallocate everything statically for reasonable headroom. You can verify that by running it in debug mode.

### Why even?
Why remain being alive?

## Links
- [Development streams in Russian](https://www.youtube.com/playlist?list=PLP0z1CQXyu5BIjKpqCIigkdj2uZCom9G4) -- initial PoC was lazily developed from scratch live in 7 streams a few hours each.
- [Alacritty](https://github.com/alacritty/alacritty) -- a serious, functionally complete multiplatform pseudoterminal that I use daily.
- [refterm](https://github.com/cmuratori/refterm/) -- another attempt at minimalistic terminal, with completely different goals.
