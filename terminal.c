#include "terminal.h"
#include <memory.h>
#include <assert.h>

Grid grid = { 0 };

//void gridClear() {
//	memset(grid.cells, 0, sizeof(grid.cells));
//}

static struct {
	// col can point to beyond grid.cols (would mean line wrap on the next write)
	// row must always point to a valid row
	struct { int col, row; } cursor;
	RGBA color, bg;
} term = {
	.color = {255, 255, 255, 255},
	.bg = {0,0,0,0},
};

void terminalResize(unsigned int w, unsigned int h) {
	assert(w < MAX_GRID_WIDTH);
	assert(h < MAX_GRID_HEIGHT);
	grid.cols = w;
	grid.rows = h;
}

void terminalPut(unsigned int x, unsigned int y, Char c, RGB color, RGB bg) {
	assert(x < MAX_GRID_WIDTH);
	assert(y < MAX_GRID_HEIGHT);
	const int offset = x + y * grid.cols;

	grid.chars[offset] = c;
	grid.color[offset] = (RGBA){ .r = color.r, .g = color.g, .b = color.b };
	grid.bg[offset] = (RGBA){ .r = bg.r, .g = bg.g, .b = bg.b };
}

static void addNewRow(void) {
	// Last row
	//term.cursor.row = grid.rows - 1;
	term.cursor.row = 0;

	/*
	// TODO Consider circus buffer
	memmove(grid.chars,
		grid.chars + grid.cols,
		sizeof(grid.chars[0]) * (grid.cols * (grid.rows - 1)));
	memset(&grid.chars[grid.cols * (grid.rows - 1)], 0, sizeof(grid.chars[0]) * grid.cols);
	// FIXME colors
	*/
}

static Char charForChar(unsigned int unicode_char) {
	// TODO unicode and dynamic glyph atlas support
	return (Char) {
		// hardcoded values for ANSI const atlas
		.row = unicode_char & 0x0f,
		.col = 15 - (unicode_char >> 4),
		.plane = 0,
	};
}

static void newline(void) {
	term.cursor.row++;
	if (term.cursor.row >= grid.rows) {
		addNewRow();
	}
	memset(grid.color + term.cursor.row * grid.cols, 0, sizeof(grid.color[0]) * grid.cols);
}

void terminalWrite(const char* string, int len) {
	if (len < 0)
		len = strlen(string);

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

}