#include "font.h"
#include "common.h"

// Use the same Terminal font that cmd.exe uses by default
#define USE_TERMINAL_FONT
#define TERMINAL_FONT_SIZE 12, 8
//#define TERMINAL_FONT_SIZE 8, 8

#ifndef USE_TERMINAL_FONT
#define FONT_PRECIS OUT_DEFAULT_PRECIS // iOutPrecision,
//#define FONT_PRECIS OUT_RASTER_PRECIS // iOutPrecision,
#define FONT_QUALITY DEFAULT_QUALITY
//#define FONT_QUALITY NONANTIALIASED_QUALITY

#ifdef _DEBUG
#define FONT_NAME "Comic Mono"
//#define FONT_NAME "Consolas"
//#define FONT_NAME "Lucida Console"
#else
#define FONT_NAME "Consolas"
#endif
#define FONT_SIZE 18
#endif

Font font;

void fontInit(void) {
#ifdef USE_TERMINAL_FONT
	const HFONT hfont = CreateFont(
		TERMINAL_FONT_SIZE, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		OEM_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
		TEXT("Terminal")
	);
#else
	const HFONT hfont = CreateFont(
		FONT_SIZE, // cHeight
		0, // cWidth
		0, // cEscapement,
		0, // cOrientation,
		FW_NORMAL, // cWeight,
		FALSE, // bItalic,
		FALSE, // bUnderline,
		FALSE, // bStrikeOut,
		ANSI_CHARSET, // iCharSet,
		FONT_PRECIS, // iOutPrecision,
		CLIP_DEFAULT_PRECIS, // iClipPrecision,
		FONT_QUALITY, // iQuality,
		FIXED_PITCH | FF_MODERN, // iPitchAndFamily,
		TEXT(FONT_NAME) // pszFaceName
	);
#endif

	const HDC text_dc = CreateCompatibleDC(NULL);
	SelectObject(text_dc, hfont);

	TEXTMETRIC metrics;
	GetTextMetrics(text_dc, &metrics);
	font.charWidth = metrics.tmAveCharWidth;
	font.charHeight = metrics.tmHeight;

	font.atlasWidth = 16 * font.charWidth;
	font.atlasHeight = 16 * font.charHeight;

	const BITMAPINFO bitmap_info = { {
			/*bitmap_info.bmiHeader.biSize =*/ sizeof(BITMAPINFOHEADER),
			/*bitmap_info.bmiHeader.biWidth =*/ font.atlasWidth,
			/*bitmap_info.bmiHeader.biHeight =*/ font.atlasHeight,
			/*bmiHeader.biPlanes*/ 1,
			/*bitmap_info.bmiHeader.biBitCount =*/ 32,
			/*bitmap_info.bmiHeader.biCompression =*/ BI_RGB,
			0, 0, 0, 0, 0}, {0} };

	const HBITMAP dib = CreateDIBSection(text_dc, &bitmap_info, DIB_RGB_COLORS, &font.atlasBits, NULL, 0);
	SelectObject(text_dc, dib);

	SetTextColor(text_dc, RGB(255, 255, 255));
	SetBkMode(text_dc, TRANSPARENT);

	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			u8 c = (u8)((y << 4) | x);
			//if (!isprint(c))
			//	c = '?';

			TextOutA(text_dc, x * font.charWidth, (15 - y) * font.charHeight, (const char*)&c, 1);
		}
	}
}
