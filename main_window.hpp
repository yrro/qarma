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

	bool master_refreshing;
	master_proto_args mpa; // do not touch while mpt is running
	std::unique_ptr<void, decltype (&CloseHandle)> mpt; // if this is null then the thread is not running

	std::function<void ()> on_master_begin;
	std::function<void (const std::wstring&)> on_master_error;
	std::function<void (unsigned int)> on_master_progress;
	std::function<void (const server_endpoint&)> on_master_found;
	std::function<void ()> on_master_complete;

	querymanager qm;

	window_data ();
};

LRESULT CALLBACK main_window_wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
// vim: ts=4 sts=4 sw=4 noet
