#include <sstream>

#include "main_window.hpp"
#include "explain.hpp"

void explain (const wchar_t* msg, DWORD e) {
	std::wostringstream ss;
	ss << msg << ".\n\n" << e << ": ";

	wchar_t* errmsg = 0;
	if (FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		0, e, 0, reinterpret_cast<LPWSTR> (&errmsg), 0, 0)) {
		ss << errmsg;
	} else {
		ss << "[FormatMessage failure: " << GetLastError ();
	}

	MessageBox (0, ss.str ().c_str (), main_window_title, MB_ICONEXCLAMATION);

	if (errmsg)
		LocalFree (reinterpret_cast<LPVOID> (errmsg));
}
