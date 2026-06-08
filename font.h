#pragma once

typedef struct {
	int charWidth, charHeight;
	int atlasWidth, atlasHeight;
	void* atlasBits;
} Font;

extern Font font;

void fontInit(void);
