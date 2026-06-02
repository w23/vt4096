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

typedef struct {
	int w, h;

	Char chars[MAX_GRID_SIZE];
	RGBA color[MAX_GRID_SIZE];
	RGBA bg[MAX_GRID_SIZE];
} Grid;

extern Grid grid;

void gridResize(unsigned int w, unsigned int h);
//void gridClear(void);
void gridPut(unsigned int x, unsigned int y, Char c, RGB color, RGB bg);
