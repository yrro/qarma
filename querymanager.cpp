#include "querymanager.hpp"

#include <cassert>

#include <windowsx.h>

#include "thiscomponent.hpp"

namespace {
	UINT_PTR timer_id = 1;

	BOOL on_create (HWND hWnd, LPCREATESTRUCT lpcs) {
		assert (lpcs->lpCreateParams);
		SetWindowLongPtr (hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (lpcs->lpCreateParams)); // XXX handle errors
		return TRUE;
	}
}

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

	UINT_PTR r = SetTimer (hwnd.get (), timer_id, 1000, nullptr);
	assert (r); // proper error handling
}

void querymanager::add_server (const server_endpoint& ep) {
	servers.insert (ep);
}

void querymanager::timer () {
	on_progress (75, 100);
}

LRESULT WINAPI querymanager::wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	HANDLE_MSG (hWnd, WM_CREATE, on_create);
	}

	if (uMsg == WM_TIMER) {
		querymanager* self = reinterpret_cast<querymanager*> (GetWindowLongPtr (hWnd, GWLP_USERDATA));
		assert (self);

		switch (uMsg) {
		case WM_TIMER:
			if (wParam == timer_id)
				self->timer ();
			return 0;
		}
	}

	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}

std::size_t querymanager::server_count () {
	return servers.size ();
}
