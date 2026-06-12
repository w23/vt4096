#include "font.h"
#include "common.h"

#include "assert.h"

// Use the same Terminal font that cmd.exe uses by default.
// Note that this font is no Unicode, and codepage-specific. No such handling is implemented here.
// cmd.exe itself seems to be unable to support Unicode properly.
//#define USE_TERMINAL_FONT
#define TERMINAL_FONT_SIZE 12, 8
//#define TERMINAL_FONT_SIZE 8, 8

#ifndef USE_TERMINAL_FONT
#define FONT_PRECIS OUT_DEFAULT_PRECIS // iOutPrecision,
//#define FONT_PRECIS OUT_RASTER_PRECIS // iOutPrecision,
#define FONT_QUALITY DEFAULT_QUALITY
//#define FONT_QUALITY NONANTIALIASED_QUALITY

// Consolas is the best builtin Unicode font so far.
// Comic Mono is not complete it seems.
#ifdef _DEBUG
//#define FONT_NAME "Comic Mono"
#define FONT_NAME "Consolas"
//#define FONT_NAME "Lucida Console"
#else
#define FONT_NAME "Consolas"
#endif

#define FONT_SIZE 18
#define FONT_WIDTH 0
#endif

#define ATLAS_WIDTH_GLYPHS 64
#define ATLAS_HEIGHT_GLYPHS 64
#define MAX_UNICODE_CODEPOINTS (0x10ffff+1)

#pragma data_seg(".font.font")
Font font;

#pragma data_seg(".font.g")
static struct {
	HDC dc;

	// Next empty slot
	GlyphPos next;

	struct {
		GlyphPos pos;
	} cache[MAX_UNICODE_CODEPOINTS];
} g;

#pragma data_seg(".font.bitmap_info")
static BITMAPINFO bitmap_info = {
	.bmiHeader = {
		.biSize = sizeof(BITMAPINFOHEADER),
		.biPlanes = 1,
		.biBitCount = 32,
		.biCompression = BI_RGB,
	},
};

#pragma code_seg(".font.init")
void fontInit(void) {
#ifdef USE_TERMINAL_FONT
	const HFONT hfont = CreateFontW(
		TERMINAL_FONT_SIZE, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
		TEXT("Terminal")
	);
#else
	const HFONT hfont = CreateFontW(
		FONT_SIZE, // cHeight
		FONT_WIDTH, // cWidth
		0, // cEscapement,
		0, // cOrientation,
		FW_NORMAL, // cWeight,
		FALSE, // bItalic,
		FALSE, // bUnderline,
		FALSE, // bStrikeOut,
		DEFAULT_CHARSET, // iCharSet,
		FONT_PRECIS, // iOutPrecision,
		CLIP_DEFAULT_PRECIS, // iClipPrecision,
		FONT_QUALITY, // iQuality,
		FIXED_PITCH | FF_MODERN, // iPitchAndFamily,
		TEXT(FONT_NAME) // pszFaceName
	);
#endif

	g.dc = CreateCompatibleDC(NULL);
	SelectObject(g.dc, hfont);

	TEXTMETRIC metrics;
	GetTextMetrics(g.dc, &metrics);
	font.charWidth = metrics.tmAveCharWidth;
	font.charHeight = metrics.tmHeight;

	font.atlasWidth = ATLAS_WIDTH_GLYPHS * font.charWidth;
	font.atlasHeight = ATLAS_HEIGHT_GLYPHS * font.charHeight;

	bitmap_info.bmiHeader.biWidth = font.atlasWidth;
	bitmap_info.bmiHeader.biHeight = font.atlasHeight;

	const HBITMAP dib = CreateDIBSection(g.dc, &bitmap_info, DIB_RGB_COLORS, &font.atlasBits, NULL, 0);
	SelectObject(g.dc, dib);

	SetTextColor(g.dc, RGB(255, 255, 255));
	SetBkMode(g.dc, TRANSPARENT);

	// Leave one empty cell
	g.next.x = 1;
	g.next.y = 0;

#if 0
	const wchar_t test[] = {
	0x041c,
		0x0430,
		0x0439,
		0x043a,
		0x0440,
		0x043e,
		0x0441,
		0x043e,
		0x0444,
		0x0442,
	};
	TextOutW(g.dc, 0, 40, test, sizeof(test) / sizeof(wchar_t));
#endif

#if 0
	for (int y = 0; y < ATLAS_HEIGHT_GLYPHS; ++y) {
		for (int x = 0; x < ATLAS_WIDTH_GLYPHS; ++x) {
			//u8 c = (u8)((y << 4) | x);
			//if (!isprint(c))
			//	c = '?';
			//TextOutA(g.dc, x * font.charWidth, (15 - y) * font.charHeight, (const char*)&c, 1);

			const wchar_t c = /*0x0400 +*/ (wchar_t)(y * ATLAS_WIDTH_GLYPHS) + x;
			TextOutW(g.dc, x * font.charWidth, (ATLAS_HEIGHT_GLYPHS - 1 - y) * font.charHeight, &c, 1);

		}
	}
#endif
	font.dirty = 1;
}

#pragma code_seg(".font.getglyph")
GlyphPos fontGetGlyphPos(u32 codepoint) {
	if (codepoint >= MAX_UNICODE_CODEPOINTS) {
		return fontGetGlyphPos('?');
	}

	if (codepoint == 0)
		return (GlyphPos){ 0 };

	GlyphPos* pos = &g.cache[codepoint].pos;
	if (pos->x != 0 || pos->y != 0)
		return *pos;

	*pos = g.next;

	// TODO handle surrogate
	// TODO handle wide chars
	wchar_t wchar = (wchar_t)codepoint;
	// Invert Y for OpenGL's convenience:
	const int y = ATLAS_HEIGHT_GLYPHS - pos->y - 1;
	TextOutW(g.dc, pos->x * font.charWidth, y * font.charHeight, &wchar, 1);
	//debugPrintf("Char '%c' (%04x) at %d, %d\n", codepoint, codepoint, pos->x, pos->y);

	if (++g.next.x == ATLAS_WIDTH_GLYPHS) {
		g.next.x = 0;
		g.next.y++;

		// TODO handle overflow
		assert(g.next.y != ATLAS_HEIGHT_GLYPHS);
	}

	font.dirty = 1;
	return *pos;
}
