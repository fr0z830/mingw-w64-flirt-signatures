// SPDX-FileCopyrightText: 2026 Angelos Katis (RADECTI)
// SPDX-License-Identifier: MIT
/* TRAINING program: a DLL (-shared pulls dllcrt2.o's DllMainCRTStartup/_CRT_INIT). */
#include <windows.h>
#include <stdio.h>
#include <string.h>
static long g_calls;
__declspec(dllexport) int tr_dll_add(int a, int b) { g_calls++; return a + b; }
__declspec(dllexport) int tr_dll_fmt(char *out, int cap, double v) { return snprintf(out, (size_t)cap, "%.4f/%ld", v, g_calls); }
BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r) { (void)h; (void)r; if (reason == DLL_PROCESS_ATTACH) g_calls = 0; return TRUE; }
