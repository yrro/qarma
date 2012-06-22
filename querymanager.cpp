#include "querymanager.hpp"

#include <cassert>

#include "thiscomponent.hpp"

querymanager::querymanager (): hwnd (nullptr, DestroyWindow) {
	static const wchar_t window_class[] = L"{f99a0821-8383-4b65-b26c-d76dce7cec7d}";
	WNDCLASS wndclass;
	if (!GetClassInfo (HINST_THISCOMPONENT, window_class, &wndclass)) {
		wndclass = WNDCLASS ();
		wndclass.lpfnWndProc = wndproc;
		wndclass.hInstance = HINST_THISCOMPONENT;
		wndclass.lpszClassName = window_class;
		ATOM r = RegisterClass (&wndclass);
		assert (r);
	}

	hwnd.reset (CreateWindow (window_class, nullptr, 0,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		HWND_MESSAGE, nullptr, HINST_THISCOMPONENT, this));
	assert (hwnd); // XXX proper error handling
}

void querymanager::add_server (const server_endpoint& ep) {
	servers.insert (ep);
}

LRESULT WINAPI querymanager::wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}

std::size_t querymanager::server_count () {
	return servers.size ();
}
