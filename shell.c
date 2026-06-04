#include "shell.h"
#include "terminal.h"
#include "common.h"

#include <assert.h>

Shell shell = { 0 };

typedef struct {
	HANDLE read, write;
} Truba;

static Truba createTrubochku(void) {
	Truba ret;
#if 1
	CreatePipe(&ret.read, &ret.write, NULL, 0);
#else
	const DWORD flag = FILE_FLAG_OVERLAPPED;

	static DWORD serial = 0;
	UCHAR PipeNameBuffer[MAX_PATH];
	sprintf(PipeNameBuffer,
		"\\\\.\\Pipe\\vt4096.%08x.%08x",
		GetCurrentProcessId(),
		serial++
	);

	const DWORD size = 0; // use default
	const DWORD timeout_ms = 120000; // 120 seconds
	ret.read = CreateNamedPipeA(
		PipeNameBuffer,
		PIPE_ACCESS_INBOUND | flag,
		PIPE_TYPE_BYTE | PIPE_WAIT,
		1,
		size,
		size,
		timeout_ms,
		NULL
	);
	assert(ret.read != INVALID_HANDLE_VALUE);

	ret.write = CreateFileA(
		PipeNameBuffer,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | flag,
		NULL
	);
	assert(ret.write != INVALID_HANDLE_VALUE);
#endif
	return ret;
}

static void createConsole(COORD size) {
	Truba input = createTrubochku();
	Truba output = createTrubochku();
	CreatePseudoConsole(size, input.read, output.write, 0, &shell.hPC);
	shell.userInput = input.write;
	shell.shellOutput = output.read;
}

void PrepareStartupInformation(HPCON hpc, STARTUPINFOEXA* psi) {
	size_t bytesRequired;
	InitializeProcThreadAttributeList(NULL, 1, 0, &bytesRequired);

	// Prepare Startup Information structure
	psi->lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, bytesRequired);

	InitializeProcThreadAttributeList(psi->lpAttributeList, 1, 0, &bytesRequired);

	// Set the pseudoconsole information into the list
	UpdateProcThreadAttribute(psi->lpAttributeList,
		0,
		PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
		hpc,
		sizeof(hpc),
		NULL,
		NULL);
}

static DWORD WINAPI shellReadThread(LPVOID arg) {
	(void)arg;

	for (;;) {
		char buf[4096];
		DWORD bytes_read = 0;
		const BOOL result = ReadFile(shell.shellOutput, buf, sizeof(buf), &bytes_read, NULL);
		assert(result);
		if (bytes_read == 0)
			continue;

		terminalWrite(buf, bytes_read);
		PostMessage(mainWindow, WM_USER, 0, 0);
	}
}

void shellCreate(int cols, int rows, char *shell_exe) {
	createConsole((COORD){(SHORT)cols, (SHORT)rows});
	STARTUPINFOEXA sie = { .StartupInfo.cb = sizeof(sie), };
	PrepareStartupInformation(shell.hPC, &sie);

	PROCESS_INFORMATION pi = { 0 };

	CreateProcessA(NULL,
		shell_exe,
		NULL,
		NULL,
		FALSE,
		EXTENDED_STARTUPINFO_PRESENT,
		NULL,
		NULL,
		&sie.StartupInfo,
		&pi);

	CreateThread(NULL, 0, shellReadThread, NULL, 0, NULL);

}

void shellResize(int cols, int rows) {
	ResizePseudoConsole(shell.hPC, (COORD){(SHORT)cols, (SHORT)rows});
}

void shellWrite(const char* str, int len) {
	assert(len);
	// TODO async?
	DWORD written = 0;
	const BOOL result = WriteFile(shell.userInput, str, len, &written, NULL);
	assert(result);
	assert(written == (DWORD)len);
}
