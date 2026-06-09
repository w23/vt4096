#pragma once

#include "common.h"

typedef struct {
	int charWidth, charHeight;
	int atlasWidth, atlasHeight;
	void* atlasBits;

	// TODO dirty rect
	int dirty;
} Font;

typedef struct {
	u8 x, y;
} GlyphPos;

extern Font font;

void fontInit(void);
GlyphPos fontGetGlyphPos(u32 codepoint);
