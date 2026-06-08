## Required
### KNOWN ISSUES
- [x] incorrect row position when manipulating cursor -- probably ring buffer/top row issue. Needs refactoring: treat cursor position in screen coordinate, but grid writes are ring-buffer-offset-aware.

### Just enough VT100/ANSI ESC sequences support
- [x] DEBUG -- show unsupported ESC sequences in debug 
- [x] Color support

### Input
- [x] how to tell shell the grid size? -- resize pseudo terminal func
- [x] Cursor movement / arrow keys
- [ ] home, end
- [ ] PgUp, PgDn
- [ ] DEL
- [ ] Ctrl+C (Z, ...)
- [ ] ESC
- [ ] F1-F12
- [ ] Ins
- [ ] .. what else?

### Misc
- [ ] Display cursor (with different modes)
    - [x] simple block
    - [ ] use fg color for block, invert fg/bg colors for char?
- [ ] `exit` -- detect when child process dies
- [x] Window resize
- [ ] SGR 4/24: underline -- LickWeed TMDC entry needs this

## Unicode
- [ ] unicode support
    - [ ] dynamic atlas
    - [ ] unicode font
    - [ ] wide unicode support

## Хотелочки
- [ ] bold font -- trivial with dynamic atlas
- [ ] italic font -- --//--
- [ ] underline
- [ ] strikeout
- [ ] configuration file/args (font selection, size, colors, etc)
- [ ] dynamic font size change -- trivial if dynamic atlas support is there
