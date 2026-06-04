#pragma once

void shellCreate(int cols, int rows, char* shell_exe);
void shellResize(int cols, int rows);
void shellWrite(const char* str, int len);
int shellRead(char* buf, int buf_len);
