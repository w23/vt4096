#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define VC_LEANMEAN
#define VC_EXTRALEAN

#pragma warning(push)
// disable `memoryapi.h(1132,1): warning C4324: 'WIN32_MEMORY_PARTITION_INFORMATION': structure was padded due to alignment specifier`
#pragma warning(disable: 4324)
#include <Windows.h>
#pragma warning(pop)

typedef unsigned char u8;
typedef unsigned int u32;

extern HWND mainWindow;
