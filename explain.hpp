#ifndef QARMA_EXPLAIN_HPP
#define QARMA_EXPLAIN_HPP

#include <windows.h>

void explain (const wchar_t* msg, DWORD e = GetLastError ());

#endif
