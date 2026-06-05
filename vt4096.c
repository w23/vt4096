#include "terminal.h"
#include "shell.h"
#include "common.h"

#include <gl/GL.h>
#include "glext.h"

#ifdef _DEBUG
#define FONT_NAME "Comic Mono"
#else
#define FONT_NAME "Courier New"
#endif
#define FONT_SIZE 18

#ifdef _DEBUG
#include <stdio.h>
static const char *getGlErrorStr(int error) {
	switch (error) {
	case GL_INVALID_ENUM: return "GL_INVALID_ENUM"; break;
	case GL_INVALID_VALUE: return "GL_INVALID_VALUE"; break;
	case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION"; break;
#ifdef GL_STACK_OVERFLOW
	case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW"; break;
#endif
#ifdef GL_STACK_UNDERFLOW
	case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW"; break;
#endif
	case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY"; break;
#ifdef GL_TABLE_TOO_LARGE
	case GL_TABLE_TOO_LARGE: return "GL_TABLE_TOO_LARGE"; break;
#endif
	case 1286: return "INVALID FRAMEBUFFER"; break;
	};
	return "UNKNOWN";
}

static void checkGLError(const char* file, int line, const char* func) {
	const int glerror = glGetError();
	if (glerror != GL_NO_ERROR) {
		char buf[256];
		snprintf(buf, sizeof(buf), "%s:%d (%s)", file, line, func);
		MessageBoxA(NULL, buf, getGlErrorStr(glerror), 0);
		ExitProcess(1);
	}
}

#define GL_CHECK() \
	checkGLError(__FILE__, __LINE__, __func__)

#else // no debug
#define GL_CHECK()
#endif

	//X(PFNGLCREATESHADERPROGRAMV, glCreateShaderProgramv) \

#define LIST_GL_FUNCS(X) \
	X(PFNGLATTACHSHADERPROC, glAttachShader) \
	X(PFNGLCOMPILESHADERPROC, glCompileShader) \
	X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
	X(PFNGLCREATESHADERPROC, glCreateShader) \
	X(PFNGLDRAWBUFFERSPROC, glDrawBuffers) \
	X(PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation) \
	X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
	X(PFNGLGETINFOLOGARBPROC, glGetInfoLogARB) \
	X(PFNGLGETOBJECTPARAMETERIVARBPROC, glGetObjectParameterivARB) \
	X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
	X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
	X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
	X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
	X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
	X(PFNGLSHADERSOURCEPROC, glShaderSource) \
	X(PFNGLUNIFORM1FPROC, glUniform1f) \
	X(PFNGLUNIFORM1FVPROC, glUniform1fv) \
	X(PFNGLUNIFORM1IPROC, glUniform1i) \
	X(PFNGLUNIFORM1IVPROC, glUniform1iv) \
	X(PFNGLUNIFORM2FPROC, glUniform2f) \
	X(PFNGLUNIFORM2FVPROC, glUniform2fv) \
	X(PFNGLUNIFORM2IVPROC, glUniform2iv) \
	X(PFNGLUNIFORM3FPROC, glUniform3f) \
	X(PFNGLUNIFORM3FVPROC, glUniform3fv) \
	X(PFNGLUNIFORM3IVPROC, glUniform3iv) \
	X(PFNGLUNIFORM4FPROC, glUniform4f) \
	X(PFNGLUNIFORM4FVPROC, glUniform4fv) \
	X(PFNGLUNIFORM4IVPROC, glUniform4iv) \
	X(PFNGLUSEPROGRAMPROC, glUseProgram) \
	X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \

#define X(t, n) static t n = NULL;
LIST_GL_FUNCS(X)
#undef X

#define LIST_TEXTURES(X) \
	X(FontAtlas) \
	X(GridChar) \
	X(GridColor) \
	X(GridBg) \

enum {
#define X(t) Tex##t,
LIST_TEXTURES(X)
#undef X

	Tex_COUNT
};

static struct GLOBALS__ {
	HDC hdc;
	int charWidth, charHeight;
	int atlasWidth, atlasHeight;
	void* atlasBits;

	GLuint textures[Tex_COUNT];

	GLint uniform_resolution;
	GLint uniform_topRow;
} g;

HWND mainWindow;

static __forceinline void makeFontAtlas(void) {
	const HFONT font = CreateFont(
		FONT_SIZE, // cHeight
		0, // cWidth
		0, // cEscapement,
		0, // cOrientation,
		FW_NORMAL, // cWeight,
		FALSE, // bItalic,
		FALSE, // bUnderline,
		FALSE, // bStrikeOut,
		ANSI_CHARSET, // iCharSet,
		OUT_DEFAULT_PRECIS, // iOutPrecision,
		CLIP_DEFAULT_PRECIS, // iClipPrecision,
		DEFAULT_QUALITY, // iQuality,
		//NONANTIALIASED_QUALITY, // iQuality,
		DEFAULT_PITCH | FF_DONTCARE, // iPitchAndFamily,
		TEXT(FONT_NAME) // pszFaceName
	);

	const HDC text_dc = CreateCompatibleDC(NULL);
	SelectObject(text_dc, font);

	TEXTMETRIC metrics;
	GetTextMetrics(text_dc, &metrics);
	g.charWidth = metrics.tmAveCharWidth;
	g.charHeight = metrics.tmHeight;

	g.atlasWidth = 16 * g.charWidth;
	g.atlasHeight = 16 * g.charHeight;

	const BITMAPINFO bitmap_info = { {
			/*bitmap_info.bmiHeader.biSize =*/ sizeof(BITMAPINFOHEADER),
			/*bitmap_info.bmiHeader.biWidth =*/ g.atlasWidth,
			/*bitmap_info.bmiHeader.biHeight =*/ g.atlasHeight,
			/*bmiHeader.biPlanes*/ 1,
			/*bitmap_info.bmiHeader.biBitCount =*/ 32,
			/*bitmap_info.bmiHeader.biCompression =*/ BI_RGB,
			0, 0, 0, 0, 0}, {0} };

	const HBITMAP dib = CreateDIBSection(text_dc, &bitmap_info, DIB_RGB_COLORS, &g.atlasBits, NULL, 0);
	SelectObject(text_dc, dib);

	SetTextColor(text_dc, RGB(255, 255, 255));
	SetBkMode(text_dc, TRANSPARENT);

	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			u8 c = (u8)((y << 4) | x);
			//if (!isprint(c))
			//	c = '?';

			TextOutA(text_dc, x * g.charWidth, (15 - y) * g.charHeight, (const char*)&c, 1);
		}
	}
}

static /*__forceinline*/ void initTexture(GLuint tex, int w, int h, int comp, int type, void* data) {
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, comp, w, h, 0, GL_RGBA, type, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	/*
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	*/
}

static void uploadTexture(unsigned int texture_index, int w, int h, void* data) {
	glActiveTexture(GL_TEXTURE0 + texture_index);
	glBindTexture(GL_TEXTURE_2D, g.textures[texture_index]);
	initTexture(g.textures[texture_index], w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);

}

static void loadGLFuncs(void) {
#define X(t, n) n = (t)(void*)wglGetProcAddress(#n);
	LIST_GL_FUNCS(X)
#undef X
}

static GLint compileShader(GLuint type, const char* src) {
	const int fsId = glCreateShader(type);
	glShaderSource(fsId, 1, &src, 0);
	glCompileShader(fsId);

#ifdef _DEBUG
	int result;
	char info[2048];
	glGetObjectParameterivARB(fsId, GL_OBJECT_COMPILE_STATUS_ARB, &result);
	glGetInfoLogARB(fsId, sizeof(info)-1, NULL, (char*)info);
	if (!result)
	{
		MessageBoxA(NULL, info, "COMPILE", 0x00000000L);
		ExitProcess(0);
	}
#endif
	return fsId;
}

static const char* kFrag =
"#version 130\n"
#define X(t) "uniform sampler2D " #t ";\n"
LIST_TEXTURES(X)
#undef X
"uniform vec2 charSize;\n"
"uniform vec2 resolution;\n"
"uniform float topRow;\n"
"vec4 tex(sampler2D T, vec2 pix) { return texture(T, pix / textureSize(T, 0)); }\n"
"void main() {\n"
"	vec2 char_coord = floor((gl_FragCoord.xy - .5) / charSize);\n"
"	vec2 cc = char_coord + .5;\n"
"	cc.y = textureSize(GridChar, 0).y - cc.y + topRow;\n"
"	vec4 char_loc = tex(GridChar, cc);\n"
"	vec4 color = tex(GridColor, cc);\n"
"	vec4 bg = tex(GridBg, cc);\n"
//"	bg = vec4(cc.y / textureSize(GridChar, 0).y);\n"
"	vec2 atlasCharSize = charSize;\n"
"	vec2 glyph_offset = char_loc.rg * 255. * atlasCharSize;\n"
//"	vec2 atlas_pix = gl_FragCoord.xy - char_coord * charSize;\n"
"	vec2 atlas_pix = gl_FragCoord.xy - char_coord * charSize + glyph_offset;\n"
"	vec4 glyph = tex(FontAtlas, atlas_pix);\n"
// "	vec4 glyph = tex(FontAtlas, gl_FragCoord.xy);\n"
"	gl_FragColor = mix(bg, color, glyph.r);\n"
//"	gl_FragColor = vec4(cc.y / textureSize(GridChar, 0).y);\n"
//"	gl_FragColor += vec4(1., 0., 0., 1.) * tex(FontAtlas, gl_FragCoord.xy);\n"
"}";

//GLint makeProgram(const char* vert, const char* frag) {
GLint makeProgram(const char* frag) {
	const GLint pid = glCreateProgram();
//	glAttachShader(pid, compileShader(GL_VERTEX_SHADER, vert));
	glAttachShader(pid, compileShader(GL_FRAGMENT_SHADER, frag));
	glLinkProgram(pid);

	return pid;

	// TODO const GLuint pid = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, sources);
}

static void uploadGrid(void) {
	uploadTexture(TexGridChar, grid.cols, grid.rows, grid.chars);
	uploadTexture(TexGridColor, grid.cols, grid.rows, grid.color);
	uploadTexture(TexGridBg, grid.cols, grid.rows, grid.bg);
}

static void resize(int w, int h) {
	glUniform2f(g.uniform_resolution, (float)w, (float)h);
	glViewport(0, 0, w, h);
	terminalResize(w / g.charWidth, h / g.charHeight);
}

static void paint(void) {
	uploadGrid();
	grid.dirty = 0;
	glUniform1f(g.uniform_topRow, (float)grid.top_row);
	glRects(-1, -1, 1, 1);
	SwapBuffers(g.hdc);
}

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
#if 0
	case WM_PAINT: {
		debugPrintf("PAIN\n");
		PAINTSTRUCT ps = { 0 };
		BeginPaint(hwnd, &ps);
		paint();
		EndPaint(hwnd, &ps);
		return 0;
	}
#endif
	case WM_CHAR: {
		// TODO convert to UTF-8
		const char c = (char)wparam;
		if (c == '\r') {
			shellWrite("\r\n", 2);
		} else {
			shellWrite(&c, 1);
		}
		return 0; // 0 = processed this message
	}

	case WM_KEYDOWN:
		switch (wparam) {
		case VK_UP: shellWrite("\x1b[1A", -1); break;
		case VK_DOWN: shellWrite("\x1b[1B", -1); break;
		case VK_RIGHT: shellWrite("\x1b[1C", -1); break;
		case VK_LEFT: shellWrite("\x1b[1D", -1); break;
		}
		break;
	case WM_SIZE: {
		const unsigned int width = (unsigned int)(lparam & 0xffff), height = (unsigned int)(lparam >> 16);
		resize(width, height);
		break;
	}
	case WM_DESTROY: PostQuitMessage(0); break;
	case WM_CLOSE: ExitProcess(0); break;
	default: return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	return 0;
}

static const PIXELFORMATDESCRIPTOR kPfd = { sizeof(kPfd), 0,
	PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL, PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0 };

#ifdef _DEBUG
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
	(void)hPrevInstance; (void)lpCmdLine;
#else
int WinMainCRTStartup(void) {
	const HINSTANCE hInstance = GetModuleHandle(NULL);
	const int nCmdShow = SW_NORMAL;
#endif
	const WNDCLASSEX wndclass = {
		.cbSize = sizeof(wndclass),
		.lpszClassName = TEXT("VT4096"),
		.lpfnWndProc = wndProc,
		.hInstance = hInstance, // TODO NULL
	};
	// -W version makes WM_CHAR report UTF-16 chars
	// -A version would make it UTF-8 (unless user has specified a different code page)
	// TODO figure out if UTF-8 would suffice without manifest
	RegisterClassExW(&wndclass);

	makeFontAtlas();
	const int w = 80 * g.charWidth;
	const int h = 32 * g.charHeight;

	RECT r = { 0, 0, w, h };
	const unsigned int windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	AdjustWindowRectEx(&r, windowStyle, FALSE, 0);

	mainWindow = CreateWindowEx(
		0, // window styles
		wndclass.lpszClassName,
		TEXT("VT4096"), // title
		windowStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
		NULL, NULL, hInstance, NULL);
	g.hdc = GetDC(mainWindow);

	SetPixelFormat(g.hdc, ChoosePixelFormat(g.hdc, &kPfd), &kPfd);
	HGLRC hglrc = wglCreateContext(g.hdc);
	wglMakeCurrent(g.hdc, hglrc);
	loadGLFuncs();

	glGenTextures(Tex_COUNT, g.textures);
	uploadTexture(TexFontAtlas, g.atlasWidth, g.atlasHeight, g.atlasBits);
	GL_CHECK();

	const GLint prog = makeProgram(kFrag);
	g.uniform_resolution = glGetUniformLocation(prog, "resolution");
	g.uniform_topRow = glGetUniformLocation(prog, "topRow");

	glUseProgram(prog);

#ifdef _DEBUG
#define X(t) { \
		const GLint loc = glGetUniformLocation(prog, #t); \
		if (loc < 0) { \
			MessageBoxA(NULL, #t, "uniform not found", 0); \
		} else { \
			glUniform1i(loc, Tex##t); \
		} \
	}
#else
#define X(t) \
	glUniform1i(glGetUniformLocation(prog, #t), Tex##t);
#endif
	LIST_TEXTURES(X)
#undef X
	GL_CHECK();

	const GLint uniform_charSize = glGetUniformLocation(prog, "charSize");
	glUniform2f(uniform_charSize, (float)g.charWidth, (float)g.charHeight);

	terminalInit();
	resize(w, h);
	terminalClear();

	shellCreate(grid.cols, grid.rows, "cmd.exe");

	ShowWindow(mainWindow, nCmdShow);
	for (;;) {
		MSG msg;
		if (GetMessage(&msg, mainWindow, 0, 0) <= 0)
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		// FIXME this adds one frame of latency per each message.
		// TODO do this only once per batch of messages
		if (grid.dirty) {
			paint();
		}
	}

	return 0;
}