# vt4096 -- A "functional enough" Terminal Emulator for Windows in less than 4 kilobytes

This repo is a proof of concept that it is possible to make a useful (for some definition of useful) terminal app for Windows.

## Features
- [x] Final release executable file is smaller than 4096 bytes.
- [x] Absolutely no error checks or error messages whatsoever in Release mode.
- [x] Unicode output support.
- [x] Basic support for minimalistic subset of VT100/ANSI ESC sequences: cursor positioning, clear, color.
- [x] Runs `cmd.exe` (hardcoded) providing a fully functional shell.
- [x] Runs [Neovim](https://neovim.io/) (with glitches)
- [x] Runs [Far Manager](https://farmanager.com/) (with glitches)
- [x] Runs [Lick Weed by Brainstorm (TMDC 2011)](https://www.pouet.net/prod.php?which=58419) (with occasional glitches)

## TODO
- [ ] Unicode input support with UTF-16 surrogates.
- [ ] Functional and extra keys support.
- [ ] Better VT100/ESC parser -- empirically figure out the minimal subset to support more apps, and to fix existing glitches. Run more demos.
- [ ] Actually work on binary size. Zero effort had been spent on it being small.
- [ ] Wide Unicode glyphs support.
- [ ] Run arbitrary programs specified by a CLI arg.
- [ ] Unit tests for the basic pseudoterminal ESC parser/state machine.

## Limitations and known issues
- Doesn't run [Litterae Finis by Trauma (TMDC 2012)](https://www.pouet.net/prod.php?which=60938) -- it exits right after screen resize.
- Does not support many of well established ESC sequences, and has no intention of having a complete parser (good luck fitting xterm spec into 4 kilobytes).

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

### Architecture
TBD

## FAQ
### Why 4 kilobytes? 
It is a well established demoscene intro category.

### Why even?
Why remain being alive?

## Links
- [Alacritty](https://github.com/alacritty/alacritty) -- a serious, functionally complete multiplatform pseudoterminal that I use daily.
- [refterm](https://github.com/cmuratori/refterm/) -- another attempt at minimalistic terminal, with completely different goals.
