#include "grid.h"
#include <memory.h>
#include <assert.h>

Grid grid = { 0 };

//void gridClear() {
//	memset(grid.cells, 0, sizeof(grid.cells));
//}

void gridResize(unsigned int w, unsigned int h) {
	assert(w < MAX_GRID_WIDTH);
	assert(h < MAX_GRID_HEIGHT);
	grid.w = w;
	grid.h = h;
}

void gridPut(unsigned int x, unsigned int y, Char c, RGB color, RGB bg) {
	assert(x < MAX_GRID_WIDTH);
	assert(y < MAX_GRID_HEIGHT);
	const int offset = x + y * grid.w;

	grid.chars[offset] = c;
	grid.color[offset] = (RGBA){ .r = color.r, .g = color.g, .b = color.b };
	grid.bg[offset] = (RGBA){ .r = bg.r, .g = bg.g, .b = bg.b };
}