#include "terminal.h"
#include "font.h"
#include "common.h"

#include <memory.h>
#include <assert.h>
#include <string.h> // strlen

#define DEFAULT_COLS 80
#define DEFAULT_ROWS 32

Grid grid = {
	// Makes Crinkler run out of memory and crash
	// TODO Report this
	//.cols = DEFAULT_COLS,
	//.rows = DEFAULT_ROWS,
	0
};

#define ESC '\x1b'

static const RGBA kDefaultForegroundColor = { 255, 255, 255, 255 };
static const RGBA kDefaultBackgroundColor = { 0 };

static RGBA gColorTable[256] = {
	// 0 - 7
	{.r = 0, .g = 0, .b = 0, .a = 255},
	{.r = 196, .g = 0, .b = 0, .a = 255},
	{.r = 0, .g = 196, .b = 0, .a = 255},
	{.r = 196, .g = 126, .b = 0, .a = 255},
	{.r = 0, .g = 0, .b = 196,.a = 255},
	{.r = 196, .g = 0, .b = 196, .a = 255},
	{.r = 0, .g = 196, .b = 196, .a = 255},
	{.r = 196, .g = 196, .b = 196, .a = 255},

	// 8 - 15
	{.r = 78, .g = 78, .b = 78, .a = 255},
	{.r = 220, .g = 78, .b = 78, .a = 255},
	{.r = 78, .g = 220, .b = 78, .a = 255},
	{.r = 243, .g = 243, .b = 78, .a = 255},
	{.r = 78, .g = 78, .b = 220, .a = 255},
	{.r = 243, .g = 78, .b = 243, .a = 255},
	{.r = 78, .g = 243, .b = 243, .a = 255},
	{.r = 255, .g = 255, .b = 255, .a = 255},

	// 16 - ...
	// TODO is table generation really smaller than just having it here as a constant?
};

static struct {
	RGBA color, bg;
	int swapped;

	//CRITICAL_SECTION mutex;
} term;

void terminalInit(void) {
	//InitializeCriticalSection(&term.mutex);
	term.color = kDefaultForegroundColor;
	term.bg = kDefaultBackgroundColor;
	grid.rows = DEFAULT_ROWS;
	grid.cols = DEFAULT_COLS;

	// Generate color table
	RGBA* color = gColorTable + 16;
	for (u8 r = 0; r < 6; ++r)
		for (u8 g = 0; g < 6; ++g)
			for (u8 b = 0; b < 6; ++b)
				*(color++) = (RGBA){ .r = r, .g = g, .b = b };
	for (u8 g = 0; g < 24; ++g) {
		const u8 level = 10 * g + 8;
		gColorTable[232 + g] = (RGBA){ .r = level, .g = level, .b = level };
	}
}

void terminalClear(void) {
	grid.top_row = 0;
	grid.dirty = 1;
	memset(grid.glyphs, 0, sizeof(grid.glyphs));
	for (int i = 0; i < MAX_GRID_SIZE; ++i) {
		grid.color[i] = term.color;
		grid.bg[i] = term.bg;
	}
}

void terminalResize(unsigned int w, unsigned int h) {
	assert(w < MAX_GRID_WIDTH);
	assert(h < MAX_GRID_HEIGHT);
	grid.cols = w;
	grid.rows = h;

	// TODO: verify that Windows+ConPTY pushes the contents itself on resize
	terminalClear();
}

static int computeOffset(int col, int row) {
	return col + ((row + grid.top_row) % grid.rows) * grid.cols;
}

static int cursorOffset(void) {
	return computeOffset(grid.cursor.col, grid.cursor.row);
}

static void addNewRow(void) {
	// Adds new row at `top_row` position and clears it
	const int row_offset = grid.cols * grid.top_row;
	for (int col = 0; col < grid.cols; ++col) {
		grid.glyphs[row_offset + col] = (AtlasGlyph){ 0 };
		grid.color[row_offset + col] = (RGBA){ 0 };
		grid.bg[row_offset + col] = term.bg;
	}
	grid.top_row = (grid.top_row + 1) % grid.rows;
}

static AtlasGlyph glyphForCodepoint(u32 codepoint) {
	const GlyphPos gpos = fontGetGlyphPos(codepoint);
	return (AtlasGlyph) { .u = (u8)gpos.x, .v = (u8)gpos.y };
}

static void newline(void) {
	grid.cursor.row++;
	if (grid.cursor.row >= grid.rows) {
		grid.cursor.row = grid.rows - 1;
		addNewRow();
	}
	// TODO do we need to clear new row?
}

static void tab(void) {
	grid.cursor.col = clampi(8 * (grid.cursor.col / 8 + 1), 0, grid.cols);
}


// Private commands
static int performCsiPrivate(int num, int enable) {
	switch (num) {
	case 25: // Show-hide cursor
		// TODO preserve old shape?
		grid.cursor.shape = enable ? CursorShape_Block : CursorShape_Hidden;
		break;
	case 1004:
		// Focus tracking. cmd.exe sends this.
		// TODO: https://terminfo.dev/modes/decset-1004-focus-tracking
		break;
	case 1049:
		// Alternate screen buffer. neovim uses this
		// TODO: https://terminfo.dev/modes/decset-1049-alt-screen-enter
		break;
	case 2004:
		// Bracketed paste. Neovim uses this.
		// No paste support yet, ignore.
		break;
	case 9001:
		// win32-input-mode, needed for extra key combination support, e.g. Shift+Enter
		// see https://github.com/microsoft/terminal/blob/main/doc/specs/%234999%20-%20Improved%20keyboard%20handling%20in%20Conpty.md
		break;
	default:
		return 0;
	}
	return 1;
}

static int handleCSIQuestion(const char* s, int len) {
	if (len == 0)
		return 0;

	{
		int num = 0;
		int i = 0;
		for (; i < len; ++i) {
			const char c = s[i];
			if (c >= '0' && c <= '9') {
				num = num * 10 + c - '0';
			} else if (c == 'l' || c == 'h') {
				const int enable = (c == 'h');
				if (performCsiPrivate(num, enable))
					return i + 1;
				break;
			}
		}
	}

	// DEBUG
	debugPrintf("ESC CSI ? ");
	int i = 0;
	for (; i < len; ++i) {
		const u8 c = s[i];
		// If another ESC is detected, consider it as a beginning of a new ESC seq
		if (c == ESC)
			break;
		debugPrintf("%c", c);
		if (c == 'h' || c == 'l') {
			++i; // eat the char
			break;
		}
	}
	debugPrintf("\n");
	return i;
}

static void performCSIEraseInDisplay(int n) {
	switch (n) {
	case 0:
		// TODO clear from cursor to end of screen
		debugPrintf("TODO CSI EraseInDisplay(%d)\n", n);
		break;
	case 1:
		// TODO clear from beginnnig of screen to cursor
		debugPrintf("TODO CSI EraseInDisplay(%d)\n", n);
		break;
	case 3:
		// As (2), but also clear scrollback
		// we have no scrollback
		// fallthrough
	case 2:
		// clear entire screen
		// AND move cursor to 1,1 (i.e. 0,0)
		terminalClear();
		grid.cursor.col = grid.cursor.row = 0;
		break;
	default:
		// ???
		debugPrintf("UNEXPECTED CSI EraseInDisplay(%d)\n", n);
		break;
	}
}

int performCsiSgrColorEx(RGBA *out, int argc, const int argv[]) {
	switch (argv[0]) {
	case 5: // table color
		if (argc != 2) {
			debugPrintf("Unexpected argument count for color table\n");
			return 0;
		}
		*out = gColorTable[((u8)argv[1])];
		return 1;
	case 2: // component color
		if (argc != 4) {
			debugPrintf("Unexpected argument count for component color\n");
		}
		out->r = (u8)argv[1];
		out->g = (u8)argv[2];
		out->b = (u8)argv[3];
		return 1;
	}

	return 0;
}

static void performCSISelectGraphicsRendition(int argc, const int argv[]) {
	(void)argc;
	switch (argv[0]) {
	case 0:
		// reset to default
		term.color = kDefaultForegroundColor;
		term.bg = kDefaultBackgroundColor;
		term.swapped = 0;
		break;
	case 7:
		term.swapped = 1;
		break;
	case 27:
		term.swapped = 0;
		break;
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
		term.color = gColorTable[argv[0] - 30];
		break;
	case 38:
		// set foreground rgb
		if (!performCsiSgrColorEx(&term.color, argc - 1, argv + 1))
			debugPrintf("UNSUPPORTED GRAPHICS RENDITION %d\n", argv[0]);
		break;
	case 39:
		// default foreground
		term.color = kDefaultForegroundColor;
		break;
	case 40:
	case 41:
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
		term.bg = gColorTable[8 + argv[0] - 40];
		break;
	case 48:
		// set background rgb
		if (!performCsiSgrColorEx(&term.bg, argc - 1, argv + 1))
			debugPrintf("UNSUPPORTED GRAPHICS RENDITION %d\n", argv[0]);
		break;
	case 49:
		// default background
		term.bg = kDefaultBackgroundColor;
		break;
	case 53:
	case 55:
		// TODO overline on-off
		break;
	case 90:
	case 91:
	case 92:
	case 93:
	case 94:
	case 95:
	case 96:
	case 97:
		term.color = gColorTable[argv[0] - 90];
		break;
	case 100:
	case 101:
	case 102:
	case 103:
	case 104:
	case 105:
	case 106:
	case 107:
		term.bg = gColorTable[8 + argv[0] - 100];
		break;
	default:
		debugPrintf("UNSUPPORTED GRAPHICS RENDITION %d\n", argv[0]);
	}
}

static void performCSIECH(int n) {
	const int cursor_offset = cursorOffset();
	for (int i = 0; i < n && (i + grid.cursor.col) < grid.cols; ++i) {
		const int off = cursor_offset + i;
		grid.glyphs[off] = glyphForCodepoint(' ');
		grid.color[off] = term.color;
		grid.bg[off] = term.bg;
	}
}

static void performCsiEl(int n) {
	int begin = 0, end = grid.cols;
	switch (n) {
	case 0: begin = grid.cursor.col; break;
	case 1: end = grid.cursor.col; break;
	}

	const int offset = computeOffset(0, grid.cursor.row);
	for (int i = begin; i < end; ++i) {
		grid.glyphs[offset + i] = (AtlasGlyph){ 0 };
	}
}

static void handleCSICommand(u8 cmd, int argc, const int argv[]) {
	switch (cmd) {
	case 'A': { // Up
		const int n = argv[0] % grid.rows;
		grid.cursor.row = (grid.cursor.row + grid.rows - n) % grid.rows;
		break;
	}
	case 'B': { // Down
		const int n = argv[0] % grid.rows;
		grid.cursor.row = (grid.cursor.row + n) % grid.rows;
		break;
	}
	case 'C': { // Forward/Right
		grid.cursor.col = clampi(grid.cursor.col + argv[0], 0, grid.cols);
		break;
	}
	case 'D': { // Backward/Left
		grid.cursor.col = clampi(grid.cursor.col - argv[0], 0, grid.cols);
		break;
	}
	case 'J':
		performCSIEraseInDisplay(argv[0]);
		break;
	case 'm':
		performCSISelectGraphicsRendition(argc, argv);
		break;
	case 'H':
		grid.cursor.col = clampi(argv[1] - 1, 0, grid.cols);
		grid.cursor.row = clampi(argv[0] - 1, 0, grid.rows);
		break;
	case 'l':
		// Cursor Horizontal (Forward) Tab
		tab();
		break;
	case 'X':
		// Erase Character
		performCSIECH(argv[0]);
		break;
	case 'K':
		// Erase In Line
		// Used by e.g. neofetch
		performCsiEl(argv[0]);
		break;
	case 't':
		if (argv[0] == 8) {
			const int cols = argv[2] ? argv[2] : grid.cols;
			const int rows = argv[1] ? argv[1] : grid.rows;
			windowResize(cols, rows);
			break;
		}
		else {
			debugPrintf("Unhandled CSI XTWINOPS 't' Ps=%d command\n", argv[0]);
			break;
		}
	case 'q':
		// TODO: Set cursor style
		break;
	default:
		debugPrintf("Unhandled CSI commmand='%c', argc=%d\n", cmd, argc);
	}
}

static int printCSI(const char* s, int len) {
	debugPrintf("ESC CSI ");
	int i = 0;
	for (; i < len; ++i) {
		const u8 c = s[i];
		// If another ESC is detected, consider it as a beginning of a new ESC seq
		if (c == ESC)
			break;
		debugPrintf("%c", c);
		if (c >= 0x40 && c <= 0x7E) {
			++i; // eat the char
			break;
		}
	}
	debugPrintf("\n");
	return i;
}

static int handleControlSequenceIntroducer(const char* s, int len) {
	if (len == 0)
		return 0;

	//printCSI(s, len);

	if (s[0] == '?') {
		return 1 + handleCSIQuestion(s + 1, len - 1);
	}

#define MAX_CSI_ARGS 16
	int argc = 0, argv[MAX_CSI_ARGS] = { 0 };
	for (int i = 0; i < len; ++i) {
		const u8 c = s[i];
		if (c == ';') {
			argc++;
			if (argc == MAX_CSI_ARGS) {
				debugPrintf("TOO MANY ESC CSI ARGS\n");
				goto unhandled;
			}
			continue;
		}
		if (c >= 0x40 && c <= 0x7E) {
			handleCSICommand(c, argc+1, argv);
			return i + 1;
		}
		if (c < '0' && c > '9') {
			debugPrintf("UNEXPECTED ESC CSI CHAR '%c' (%02x) in ARGS\n", c, c);
			goto unhandled;
		}
		argv[argc] *= 10;
		argv[argc] += c - '0';
	}

	// DEBUG
unhandled:
	return printCSI(s, len);
}

static int performOsc(int Pt, const char* Ps, int PsLen) {
	if (PsLen < 0)
		return 0;

	switch (Pt) {
	case 0:
		windowSetTitle(Ps, PsLen);
		break;
	case 9001:
		// cmd.exe sends this on e.g. unknown commands; ignore.
		break;
	default:
		return 0;
	}
	return 1;
}

static int handleOperatingSystemCommand(const char* s, int len) {
	if (len == 0)
		return 0;

	{
		int Ps = 0;
		int i = 0;
		for (; i < len; ++i) {
			const u8 c = s[i];
			if (c >= '0' && c <= '9') {
				Ps = Ps * 10 + c - '0';
				continue;
			}
			if (c == ';') {
				i++;
				break;
			}
			goto unhandled;
		}
		int PtBegin = i, PtEnd = 0;
		for (; i < len; ++i) {
			if (s[i] == ESC) {
				PtEnd = i;
				// Variable length command end, do not consume, will be ignored
				break;
			}
			if (s[i] == 0x07 || s[i] == 0x9c) {
				PtEnd = i;
				++i; // consume
				break;
			}
		}

		if (PtEnd > PtBegin) {
			if (performOsc(Ps, s + PtBegin, PtEnd - PtBegin)) {
				return i;
			}
		}
	}

unhandled:
	// DEBUG
	debugPrintf("ESC OSC ");
	int i;
	for (i = 0; i < len; ++i) {
		const u8 c = s[i];
		// If another ESC is detected, consider it as a beginning of a new ESC seq
		if (c == ESC)
			break;
		debugPrintf("%c", c);
		if (c == 0x07 || c == 0x9c) {
			debugPrintf("[%02x]", c);
			++i; // eat the char
			break;
		}
	}
	debugPrintf("\n");
	return i;
}

// s points to the next char after ESC
// returns number of glyphs consumed
static int handleEsc(const char* s, int len) {
	if (len == 0)
		return 0;

	switch (s[0]) {
	case '[': return 1 + handleControlSequenceIntroducer(s + 1, len - 1);
	case ']': return 1 + handleOperatingSystemCommand(s + 1, len - 1);
	case '\\': return 1; // ST, does nothing here, used as an end mark for variable length commands
	}

	debugPrintf("ESC %c\n", s[0]);
	return 0;
}

static void outputCodepoint(u32 codepoint) {
	if (grid.cursor.col >= grid.cols) {
		grid.cursor.col = 0;
		newline();
	}

	const int offset = cursorOffset();
	grid.glyphs[offset] = glyphForCodepoint(codepoint);
	grid.color[offset] = term.swapped ? term.bg : term.color;
	grid.bg[offset] = term.swapped ? term.color : term.bg;
	grid.cursor.col++;
}

static int readCodepoint(const char *s, int len) {
	const u8 c = s[0];
	u32 codepoint = 0;

	int i = 1;
	if ((c & 0x80) == 0) {
		codepoint = c;
	} else {
		if ((c >> 5) == 0x6) {
			// 2-byte sequence
			codepoint = c & 0x1f;
		}
		else if ((c >> 4) == 0xe) {
			// 3-byte sequence
			codepoint = c & 0x0f;
		}
		else if ((c >> 3) == 0x1e) {
			// 4-byte sequence
			codepoint = c & 0x07;
		}

		// Assume well-formed utf-8 with correct sequence length
		for (; i < len; ++i) {
			const u8 c2 = s[i];
			if ((c2 >> 6) != 0x02)
				break;
			codepoint = (codepoint << 6) | (c2 & 0x3f);
		}
	}

	outputCodepoint(codepoint);
	return i - 1;
}

// Expects `string` to be complete and sufficient, e.g. no ESC breaks, no UTF8 breaks.
void terminalWrite(const char* string, int len) {
	//EnterCriticalSection(&term.mutex);
	if (len < 0)
		len = (int)strlen(string);

	//debugPrintf("terminalWrite(\"%.*s\"\n", len, string);

	for (int i = 0; i < len; ++i) {
		const u8 c = string[i];


		switch (c) {
			case '\r': grid.cursor.col = 0; break;
			case '\n': newline(); break;
			case '\t': tab(); break;
			case '\a': /* TODO DING!!~ */ break;
			case '\b':
				if (grid.cursor.col > 0) {
					grid.cursor.col--;
					grid.glyphs[cursorOffset()] = (AtlasGlyph){0};
				}
			break;
			case '\x1b': i += handleEsc(string + i + 1, len - i - 1); break;
			default:
				i += readCodepoint(string + i, len - i);
			
		}
	}

	grid.dirty = 1;
	//LeaveCriticalSection(&term.mutex);
}
