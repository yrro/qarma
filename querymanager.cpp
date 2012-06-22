#include "querymanager.hpp"

#include <algorithm>
#include <cassert>
#include <limits>

#include <windows.h>
#include <windowsx.h>

#include "thiscomponent.hpp"

namespace {
	UINT_PTR timer_id = 1;

	BOOL on_create (HWND hWnd, LPCREATESTRUCT lpcs) {
		assert (lpcs->lpCreateParams);
		SetWindowLongPtr (hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (lpcs->lpCreateParams)); // XXX handle errors
		return TRUE;
	}

	bool stale (const server_info& info, ULONGLONG reference) {
		return info.when < std::max (0ull, reference - 1000 * 60 * 5);
	}
}

querymanager::querymanager (): hwnd (nullptr, DestroyWindow), protos (10) {
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

void querymanager::queue_query (query_proto& proto, std::map<server_endpoint, server_info>::value_type& server) {
	server.second.when = std::numeric_limits<decltype(server.second.when)>::max ();
	proto.on_complete = [this, &server] (const server_info& info) {
		server.second = info;
		on_found (info);
	};
	proto.begin (server.first);
}

void querymanager::add_server (const server_endpoint& ep) {
	auto i = servers.insert (std::make_pair (ep, server_info ()));
	if (!i.second)
		return;

	for (auto& proto: protos) {
		if (proto.state == query_proto::available) {
			queue_query (proto, *i.first);
			break;
		}
	}
}

void querymanager::timer () {
	auto now = GetTickCount64 ();

	unsigned int ncomplete = 0;
	for (auto& server: servers)
		if (!stale (server.second, now))
			++ncomplete;
	on_progress (ncomplete, servers.size ());

	for (auto& proto: protos)
		if (proto.state == query_proto::available)
			for (auto& server: servers)
				if (stale (server.second, now))
					queue_query (proto, server);
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

// vim: ts=4 sts=4 sw=4 noet
