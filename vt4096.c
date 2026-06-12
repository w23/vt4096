#include "terminal.h"
#include "shell.h"
#include "render.h"
#include "font.h"
#include "common.h"
#include <winnls.h>

HWND mainWindow;

//static const unsigned int windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
static const unsigned int windowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

enum {
	WM_USER_RESIZE = WM_USER + 1, // LPARAM(cols, rows)
};

static void resize(int w, int h) {
	const int cols = w / font.charWidth;
	const int rows = h / font.charHeight;
	debugPrintf("resize(%d, %d) -> [%d, %d]\n", w, h, cols, rows);
	terminalResize(cols, rows);
	shellResize(cols, rows);
	renderResize(w, h);
}

static void userResize(int cols, int rows) {
	RECT r = { 0, 0, cols * font.charWidth, rows * font.charHeight };
	AdjustWindowRectEx(&r, windowStyle, FALSE, 0);
	SetWindowPos(mainWindow, NULL, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static wchar_t surrogate[2] = {0};
static void handleWmChar(WPARAM wparam, LPARAM lparam) {
	(void)lparam;
	const wchar_t wchar = (wchar_t)wparam;
	// OH WINDOWS WHY ARE YOU SO ~
	if (wchar == '\b') /* Backspace */ {
		shellWrite("\x7f", 1);
	} else if (wchar == '\x7f') /* Ctrl+Backspace */ {
		shellWrite("\b", 1);
	} else {
		// Assume that HIGH, LOW surrogate sequence always happens correctly
		// 👍
		const int is_low_surrogate = IS_LOW_SURROGATE(wchar);
		if (is_low_surrogate) {
			// 😊
			surrogate[1] = wchar;
		} else {
			surrogate[0] = wchar;
		}

		// Skip high surrogate, as it will be immediately followed by LOW surrogate
		if (IS_HIGH_SURROGATE(wchar))
			return;

		char utf8[4];
		const int len = WideCharToMultiByte(CP_UTF8, 0, surrogate, 1 + is_low_surrogate, utf8, sizeof(utf8), NULL, NULL);
		if (len == 0)
			debugPrintf("Unable to convert U+%04x to UTF-8\n", wchar);
		else
			shellWrite(utf8, len);
	}
}

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	const char* shell_cmd = NULL;
	switch (msg) {
	case WM_USER_RESIZE:
		userResize(LOWORD(lparam), HIWORD(lparam));
		return 0;
	case WM_PAINT: {
		//debugPrintf("PAIN\n");
		PAINTSTRUCT ps = { 0 };
		BeginPaint(hwnd, &ps);
		renderPaint();
		// Begin/End is needed to mark the window as not dirty anymore
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_CHAR: {
		handleWmChar(wparam, lparam);
		return 0; // 0 = processed this message
	}
	case WM_SIZE: {
		const unsigned int width = (unsigned int)(lparam & 0xffff), height = (unsigned int)(lparam >> 16);
		resize(width, height);
		return 0;
	}
	case WM_DESTROY: PostQuitMessage(0); return 0;
	case WM_CLOSE: ExitProcess(0); break;
	case WM_SYSKEYDOWN:
		// F10 is a special system key it seems
		switch (wparam) {
		case VK_F10: shell_cmd = "\x1b[21~"; break;
		}
		break;
	case WM_KEYDOWN:
		switch (wparam) {
		case VK_UP: shell_cmd = "\x1b[A"; break;
		case VK_DOWN: shell_cmd = "\x1b[B"; break;
		case VK_RIGHT: shell_cmd = "\x1b[C"; break;
		case VK_LEFT: shell_cmd = "\x1b[D"; break;
		case VK_HOME: shell_cmd = "\x1b[1~"; break;
		case VK_INSERT: shell_cmd = "\x1b[2~"; break;
		case VK_DELETE: shell_cmd = "\x1b[3~"; break;
		case VK_END: shell_cmd = "\x1b[4~"; break;
		case VK_PRIOR: shell_cmd = "\x1b[5~"; break;
		case VK_NEXT: shell_cmd = "\x1b[6~"; break;
		case VK_F1: shell_cmd = "\x1bOP"; break;
		case VK_F2: shell_cmd = "\x1bOQ"; break;
		case VK_F3: shell_cmd = "\x1bOR"; break;
		case VK_F4: shell_cmd = "\x1bOS"; break;
		case VK_F5: shell_cmd = "\x1b[15~"; break;
		case VK_F6: shell_cmd = "\x1b[17~"; break;
		case VK_F7: shell_cmd = "\x1b[18~"; break;
		case VK_F8: shell_cmd = "\x1b[19~"; break;
		case VK_F9: shell_cmd = "\x1b[20~"; break;
		case VK_F11: shell_cmd = "\x1b[23~"; break;
		case VK_F12: shell_cmd = "\x1b[24~"; break;
		}
		break;
	}

	if (shell_cmd) {
		shellWrite(shell_cmd, -1);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

static WNDCLASSEX wndclass = {
	.cbSize = sizeof(wndclass),
	.lpszClassName = TEXT("V"),
	.lpfnWndProc = wndProc,
};

#ifdef _DEBUG
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
	(void)hPrevInstance; (void)lpCmdLine;
#else
int WinMainCRTStartup(void) {
	const HINSTANCE hInstance = GetModuleHandle(NULL);
	const int nCmdShow = SW_NORMAL;
#endif
	// -W version makes WM_CHAR report UTF-16 chars
	// -A version would make it UTF-8 (unless user has specified a different code page)
	wndclass.hInstance = hInstance;
	RegisterClassExW(&wndclass);

	fontInit();
	terminalInit();
	const int w = grid.cols * font.charWidth;
	const int h = grid.rows * font.charHeight;

	RECT r = { 0, 0, w, h };
	AdjustWindowRectEx(&r, windowStyle, FALSE, 0);

	mainWindow = CreateWindowEx(
		0, // window styles
		wndclass.lpszClassName,
		TEXT("VT4096"), // title
		windowStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
		NULL, NULL, hInstance, NULL);

	renderInit();
	shellCreate(grid.cols, grid.rows, "cmd.exe");

	ShowWindow(mainWindow, nCmdShow);
	for (;;) {
		MSG msg;
		if (GetMessage(&msg, NULL, 0, 0) <= 0)
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (grid.dirty) {
			InvalidateRect(mainWindow, NULL, TRUE); // Request repaint
		}
	}

	return 0;
}

void windowSetTitle(const char* s, int len) {
	wchar_t buffer[256];
	int conv = MultiByteToWideChar(CP_UTF8, 0, s, len, buffer, sizeof(buffer) - 1);
	if (conv < 0) conv = 0;
	buffer[conv] = 0;
	SetWindowTextW(mainWindow, buffer);
}

void windowResize(int cols, int rows) {
	debugPrintf("windowResize(%d, %d)\n", cols, rows);
	PostMessage(mainWindow, WM_USER_RESIZE, 0, MAKELPARAM(cols, rows));
}
