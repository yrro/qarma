#ifndef QARMA_MAIN_WINDOW_HPP
#define QARMA_MAIN_WINDOW_HPP

#include <memory>
#include <set>
#include <vector>

#include "master_proto.hpp"
#include "querymanager.hpp"

#include <windows.h>

struct window_data {
	NONCLIENTMETRICS metrics;
	std::unique_ptr<HFONT__, decltype (&DeleteObject)> message_font;

	master_protocol mproto;
	querymanager qm;

	bool master_refreshing;

	window_data ();
};

LRESULT CALLBACK main_window_wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
// vim: ts=4 sts=4 sw=4 noet
