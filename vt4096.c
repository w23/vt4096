#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define VC_LEANMEAN
#define VC_EXTRALEAN
#include <Windows.h>

#include <gl/GL.h>
#include "glext.h"

#define FONT_NAME "Courier New"
#define FONT_SIZE 18

static struct GLOBALS__ {
	HDC hdc;
	int charWidth, charHeight;
	int atlasWidth, atlasHeight;
	void* atlasBits;
	GLint atlasTexture;
} g;

static __forceinline void makeFontAtlas() {
	void *bitmap_ptr = NULL;

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
			unsigned char c = (y << 4) | x;
			if (!isprint(c))
				c = '?';

			TextOutA(text_dc, x * g.charWidth, y * g.charHeight, &c, 1);
		}
	}
}

static /*__forceinline*/ void initTexture(GLuint tex, int w, int h, int comp, int type, void* data) {
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, comp, w, h, 0, GL_RGBA, type, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	/*
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	*/
}

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

#define X(t, n) static t n = NULL;
LIST_GL_FUNCS(X)
#undef X

static void loadGLFuncs(void) {
#define X(t, n) n = (t)wglGetProcAddress(#n);
	LIST_GL_FUNCS(X)
#undef X
}

static GLint compileShader(GLuint type, const char* src) {
	const int fsId = glCreateShader(type);
	glShaderSource(fsId, 1, &src, 0);
	glCompileShader(fsId);

#define SHADER_DEBUG
#ifdef SHADER_DEBUG
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

//static const char* kVert =
//"void main() {\n"
//"	\n"
//"}"
//;

static const char* kFrag =
"uniform sampler2D atlas;"
"uniform vec2 resolution;"
"void main() {\n"
"	vec2 uv = gl_FragCoord.xy / resolution;\n"
"	gl_FragColor = texture(atlas, uv);\n"
//"	gl_FragColor = vec4(0.,1.,0.,1.);\n"
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

static void resize(int w, int h) {
	glViewport(0, 0, w, h);
}

static void paint(void) {
	//glClearColor(1, 0, 0, 1);
	//glClear(GL_COLOR_BUFFER_BIT);
	glRects(-1, -1, 1, 1);
	SwapBuffers(g.hdc);
}

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_PAINT: paint(); break;
	case WM_DESTROY: PostQuitMessage(0); break;
	case WM_CLOSE: ExitProcess(0); break;
	default: return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	return 0;
}

static const PIXELFORMATDESCRIPTOR kPfd = { sizeof(kPfd), 0,
	PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL, PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0 };

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
	const WNDCLASSEX wndclass = {
		.cbSize = sizeof(wndclass),
		.lpszClassName = TEXT("vt4096"),
		.lpfnWndProc = wndProc,
		.hInstance = hInstance, // TODO NULL
		//.hCursor = LoadCursor(NULL, IDC_ARROW), // TODO NULL
		//.hIcon = LoadIcon(NULL, IDI_APPLICATION), // TODO NULL
		//.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
	};
	RegisterClassEx(&wndclass);

	makeFontAtlas();
	const int w = 80 * g.charWidth;
	const int h = 32 * g.charHeight;

	HWND hwnd = CreateWindowEx(
		0, // window styles
		wndclass.lpszClassName,
		TEXT("VT4096"), // title
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		CW_USEDEFAULT, CW_USEDEFAULT, w, h,
		NULL, NULL, hInstance, NULL);
	g.hdc = GetDC(hwnd);

	SetPixelFormat(g.hdc, ChoosePixelFormat(g.hdc, &kPfd), &kPfd);
	HGLRC hglrc = wglCreateContext(g.hdc);
	wglMakeCurrent(g.hdc, hglrc);
	loadGLFuncs();

	glGenTextures(1, &g.atlasTexture);
	initTexture(g.atlasTexture, g.atlasWidth, g.atlasHeight, GL_RGBA, GL_UNSIGNED_BYTE, g.atlasBits);
	const GLint prog = makeProgram(kFrag);
	const GLint uniform_atlas = glGetUniformLocation(prog, "atlas");
	const GLint uniform_resolution = glGetUniformLocation(prog, "resolution");
	glUseProgram(prog);
	glBindTexture(GL_TEXTURE_2D, g.atlasTexture);
	glUniform1i(uniform_atlas, 0);
	glUniform2f(uniform_resolution, (float)w, (float)h);

	resize(w, h);

	ShowWindow(hwnd, nCmdShow);

	for (;;) {
		MSG msg;
		if (GetMessage(&msg, hwnd, 0, 0) <= 0)
			break;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}