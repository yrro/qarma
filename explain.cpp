#include <sstream>

#include "explain.hpp"

std::wstring wstrerror (DWORD error) {
	wchar_t* errmsg = nullptr;
	if (FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		0, error, 0, reinterpret_cast<LPWSTR> (&errmsg), 0, 0))
	{
		std::wstring result = errmsg;
		LocalFree (reinterpret_cast<LPVOID> (errmsg));
		return result;
	} else {
		std::wostringstream ss;
		ss << L"[FormatMessage failure: " << GetLastError () << ']';
		return ss.str ();
	}
}

void explain (const wchar_t* msg, DWORD e) {
	std::wostringstream ss;
	ss << msg << ".\n\n" << e << ": " << wstrerror (e);
	MessageBox (0, ss.str ().c_str (), L"Qarma", MB_ICONEXCLAMATION);
}
// vim: ts=4 sts=4 sw=4 noet
