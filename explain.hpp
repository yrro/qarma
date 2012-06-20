#ifndef QARMA_EXPLAIN_HPP
#define QARMA_EXPLAIN_HPP

#include <windows.h>

std::wstring wstrerror (DWORD error);
void explain (const wchar_t* msg, DWORD e = GetLastError ());

#endif
