#include "terminal.h"
#include "common.h"

#include <memory.h>
#include <assert.h>
#include <string.h> // strlen

Grid grid = { 0 };

static const RGBA kDefaultForegroundColor = { 255, 255, 255, 255 };
static const RGBA kDefaultBackgroundColor = { 0 };

static const RGBA kColorTable[] = {
	{.r = 0, .g = 0, .b = 0, .a = 255},
	{.r = 196, .g = 0, .b = 0, .a = 255},
	{.r = 0, .g = 196, .b = 0, .a = 255},
	{.r = 196, .g = 126, .b = 0, .a = 255},
	{.r = 0, .g = 0, .b = 196,.a = 255},
	{.r = 196, .g = 0, .b = 196, .a = 255},
	{.r = 0, .g = 196, .b = 196, .a = 255},
	{.r = 196, .g = 196, .b = 196, .a = 255},
};

static const RGBA kBrightColorTable[] = {
	{.r = 78, .g = 78, .b = 78, .a = 255},
	{.r = 220, .g = 78, .b = 78, .a = 255},
	{.r = 78, .g = 220, .b = 78, .a = 255},
	{.r = 243, .g = 243, .b = 78, .a = 255},
	{.r = 78, .g = 78, .b = 220, .a = 255},
	{.r = 243, .g = 78, .b = 243, .a = 255},
	{.r = 78, .g = 243, .b = 243, .a = 255},
	{.r = 255, .g = 255, .b = 255, .a = 255},
};

static struct {
	// col can point to beyond grid.cols (would mean line wrap on the next write)
	// row must always point to a valid row
	struct { int col, row; } cursor;
	RGBA color, bg;

	CRITICAL_SECTION mutex;
} term = {
	.color = {255, 255, 255, 255},
	.bg = {0,0,0,0},
};

void terminalInit(void) {
	InitializeCriticalSection(&term.mutex);
}

void terminalResize(unsigned int w, unsigned int h) {
	assert(w < MAX_GRID_WIDTH);
	assert(h < MAX_GRID_HEIGHT);
	grid.cols = w;
	grid.rows = h;

	// FIXME Handle top_row
	// - smaller?
	// - bigger?
}

void terminalClear(void) {
	grid.top_row = grid.rows - 1;
	grid.dirty = 1;
	memset(grid.chars, 0, sizeof(grid.chars));
}

static int cursorOffset(void) {
	return term.cursor.col + term.cursor.row * grid.cols;
}

static void addNewRow(void) {
	term.cursor.row = grid.top_row;
	const int offset = grid.cols * term.cursor.row;
	for (int col = 0; col < grid.cols; ++col) {
		grid.chars[offset + col] = (Char){ 0 };
		grid.color[offset + col] = (RGBA){ 0 };
		grid.bg[offset + col] = term.bg;
	}
	grid.top_row = (grid.top_row + 1) % grid.rows;
}

static Char charForChar(unsigned int unicode_char) {
	// TODO unicode and dynamic glyph atlas support
	return (Char) {
		// hardcoded values for ANSI const atlas
		.row = (u8)(unicode_char & 0x0f),
		.col = (u8)(unicode_char >> 4),
		.plane = 0,
	};
}

static void newline(void) {
	term.cursor.row++;
	if (term.cursor.row >= grid.top_row) {
		addNewRow();
	}
	memset(grid.chars + term.cursor.row * grid.cols, 0, sizeof(grid.chars[0]) * grid.cols);
}

static int handleCSIQuestion(const char* s, int len) {
	if (len == 0)
		return 0;

	// DEBUG
	debugPrintf("ESC CSI ? ");
	int i = 0;
	for (; i < len; ++i) {
		const u8 c = s[i];
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
		term.cursor.col = term.cursor.row = 0;
		break;
	default:
		// ???
		debugPrintf("UNEXPECTED CSI EraseInDisplay(%d)\n", n);
		break;
	}
}

static void performCSISelectGraphicsRendition(int argc, const int argv[]) {
	(void)argc;
	switch (argv[0]) {
	case 0:
		// reset to default
		term.color = kDefaultForegroundColor;
		term.bg = kDefaultBackgroundColor;
		break;
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
		term.color = kColorTable[argv[0] - 30];
		break;
	case 38:
		// set foreground rgb
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
		term.bg = kColorTable[argv[0] - 40];
		break;
	case 48:
		// set background rgb
		debugPrintf("UNSUPPORTED GRAPHICS RENDITION %d\n", argv[0]);
		break;
	case 49:
		// default background
		term.bg = kDefaultBackgroundColor;
		break;
	default:
		debugPrintf("UNSUPPORTED GRAPHICS RENDITION %d\n", argv[0]);
	}
}

static void handleCSICommand(u8 cmd, int argc, const int argv[]) {
	switch (cmd) {
	case 'J':
		performCSIEraseInDisplay(argv[0]);
		break;
	case 'm':
		performCSISelectGraphicsRendition(argc, argv);
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

#define MAX_CSI_ARGS 4
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

static int handleOperatingSystemCommand(char* s, int len) {
	if (len == 0)
		return 0;

	if (s[0] == '0') {
		// Set window title
		for (int i = 2; i < len; ++i) {
			if (s[i] == 0x07 || s[i] == 0x9c) {
				s[i] = '\0';
				SetWindowTextA(mainWindow, s + 2); // s+2 skips '0;'
				return i + 1;
			}
		}
	}

	// DEBUG
	debugPrintf("ESC OSC ");
	int i;
	for (i = 0; i < len; ++i) {
		const u8 c = s[i];
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
// returns number of chars consumed
static int handleEsc(char* s, int len) {
	if (len == 0)
		return 0;

	switch (s[0]) {
	case '[': return 1 + handleControlSequenceIntroducer(s + 1, len - 1);
	case ']': return 1 + handleOperatingSystemCommand(s + 1, len - 1);
	}

	debugPrintf("ESC %c\n", s[0]);
	return 0;
}

void terminalWrite(const char* string, int len) {
	EnterCriticalSection(&term.mutex);
	if (len < 0)
		len = (int)strlen(string);

	for (int i = 0; i < len; ++i) {
		const unsigned int c = string[i];

		if (c == '\r') {
			term.cursor.col = 0;
			continue;
		}

		if (c == '\n') {
			newline();
			continue;
		}

		if (c == '\t') {
			term.cursor.col = 8 * (term.cursor.col / 8 + 1);
			continue;
		}

		if (c == '\a') {
			// TODO DING!!~
			continue;
		}

		if (c == '\b') {
			if (term.cursor.col > 0) {
				term.cursor.col--;
				grid.chars[cursorOffset()] = (Char){0};
			}
			continue;
		}

		if (c == '\x1b') {
			i += handleEsc((char*)string + i + 1, len - i - 1);
			continue;
		}

		// Output this char

		if (term.cursor.col >= grid.cols) {
			term.cursor.col = 0;
			newline();
		}

		const int offset = term.cursor.col + term.cursor.row * grid.cols;
		grid.chars[offset] = charForChar(c);
		grid.color[offset] = term.color;
		grid.bg[offset] = term.bg;
		term.cursor.col++;
	}

	grid.dirty = 1;
	LeaveCriticalSection(&term.mutex);
}