#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define VC_LEANMEAN
#define VC_EXTRALEAN

#include <Windows.h>
#include <gl/GL.h>

static const PIXELFORMATDESCRIPTOR kPfd = { sizeof(kPfd), 0,
	PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL, PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0 };

static struct GLOBALS__ {
	HDC hdc;
} g;

static void resize(int w, int h) {
	glViewport(0, 0, w, h);
}

static void paint(void) {
	glClearColor(1, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
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

	// TODO calc window size based on font size
	const int w = 1280;
	const int h = 720;

	HWND hwnd = CreateWindowEx(
		0, // window styles
		wndclass.lpszClassName,
		TEXT("VT4096"), // title
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		CW_USEDEFAULT, CW_USEDEFAULT, w, h,
		NULL, NULL, hInstance, NULL);
	g.hdc = GetDC(hwnd);

	SetPixelFormat(g.hdc, ChoosePixelFormat(g.hdc, &kPfd), &kPfd);
	HGLRC hglrc = wglCreateContext(g.hdc);
	wglMakeCurrent(g.hdc, hglrc);

	ShowWindow(hwnd, nCmdShow);

	resize(w, h);

	for (;;) {
		MSG msg;
		if (GetMessage(&msg, hwnd, 0, 0) <= 0)
			break;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}