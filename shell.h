#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define VC_LEANMEAN
#define VC_EXTRALEAN
#include <Windows.h>

typedef struct {
	HPCON hPC;
	HANDLE userInput, shellOutput;
} Shell;

extern Shell shell;

void shellCreate(int cols, int rows, char* shell_exe);
void shellResize(int cols, int rows);
void shellWrite(const char* str, int len);
int shellRead(char* buf, int buf_len);
