#ifndef QARMA_MAIN_WINDOW_HPP
#define QARMA_MAIN_WINDOW_HPP

#include <memory>
#include <set>
#include <vector>

#include "master_proto.hpp"

#include <windows.h>
#include <windowsx.h>

static const wchar_t main_window_class[] = L"{078be6e6-0d3a-42f4-b21a-1d92c1c41c9e}";

struct window_data {
	NONCLIENTMETRICS metrics;
	std::unique_ptr<HFONT__, decltype (&DeleteObject)> message_font;

	master_protocol mproto;
	std::set<server_endpoint> server_list;

	window_data (): message_font (nullptr, DeleteObject) {}
};

BOOL main_window_on_create (HWND hWnd, LPCREATESTRUCT lpcs);
LRESULT main_window_on_destroy (HWND hWnd);
LRESULT main_window_on_command (HWND hWnd, int id, HWND hCtl, UINT codeNotify);
LRESULT main_window_on_socket (HWND hWnd, SOCKET socket, WORD wsa_event, WORD wsa_error);
LRESULT CALLBACK main_window_wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
