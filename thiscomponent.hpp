#ifndef QARMA_THISCOMPONENT_H
#define QARMA_THISCOMPONENT_H

#include <windows.h>

// <http://blogs.msdn.com/b/oldnewthing/archive/2004/10/25/247180.aspx>
extern "C" IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

#endif
