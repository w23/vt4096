#include "terminal.h"
#include "shell.h"
#include "render.h"
#include "font.h"
#include "common.h"

HWND mainWindow;

static void resize(int w, int h) {
	const int cols = w / font.charWidth;
	const int rows = h / font.charHeight;
	terminalResize(cols, rows);
	shellResize(cols, rows);
	renderResize(w, h);
}

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_PAINT: {
		//debugPrintf("PAIN\n");
		PAINTSTRUCT ps = { 0 };
		BeginPaint(hwnd, &ps);
		renderPaint();
		EndPaint(hwnd, &ps);
		return 0;
	}
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

	fontInit();
	const int w = 80 * font.charWidth;
	const int h = 32 * font.charHeight;

	RECT r = { 0, 0, w, h };
	//const unsigned int windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	const unsigned int windowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	AdjustWindowRectEx(&r, windowStyle, FALSE, 0);

	mainWindow = CreateWindowEx(
		0, // window styles
		wndclass.lpszClassName,
		TEXT("VT4096"), // title
		windowStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
		NULL, NULL, hInstance, NULL);

	renderInit();

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

		if (grid.dirty) {
			InvalidateRect(mainWindow, NULL, TRUE); // Request repaint
		}
	}

	return 0;
}