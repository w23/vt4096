#pragma once
#include <stdint.h>

#define MAX_GRID_WIDTH 512
#define MAX_GRID_HEIGHT 512
#define MAX_GRID_SIZE (MAX_GRID_WIDTH * MAX_GRID_HEIGHT)

typedef struct {
	unsigned char r, g, b;
} RGB;

typedef struct {
	unsigned char r, g, b, a;
} RGBA;

typedef struct {
	unsigned char row, col, plane, unused_;
} Char;

typedef enum {
	CursorShape_Hidden,
	CursorShape_Block,
	CursorShape_Underline,
	CursorShape_Bar
} CursorShape;

typedef struct {
	int cols, rows;
	int top_row; // grid is circular buffer. first (top) row starts with this row index.
	int dirty;

	// col can point to beyond grid.cols (would mean line wrap on the next write)
	// row must always point to a valid row
	struct { int col, row; CursorShape shape; } cursor;

	Char chars[MAX_GRID_SIZE];
	RGBA color[MAX_GRID_SIZE];
	RGBA bg[MAX_GRID_SIZE];
} Grid;

extern Grid grid;

void terminalInit(void);
void terminalResize(unsigned int w, unsigned int h);
void terminalClear(void);

// Parses esc seqences and other things there
void terminalWrite(const char* string, int len);

// Callback functions
void windowResize(int cols, int rows);
