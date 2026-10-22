// SPDX-FileCopyrightText: 2026 Angelos Katis (RADECTI)
// SPDX-License-Identifier: MIT
/* TRAINING program: wide entry point (-municode pulls crt2u.o's wmainCRTStartup). */
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
int wmain(int argc, wchar_t **argv) {
    wchar_t buf[128];
    swprintf(buf, 128, L"%ls:%d", argc > 1 ? argv[1] : L"none", argc);
    wprintf(L"%ls %zu\n", buf, wcslen(buf));
    return (int)wcstol(buf, NULL, 10);
}
