#ifndef QARMA_MAIN_WINDOW_HPP
#define QARMA_MAIN_WINDOW_HPP

#include <array>
#include <memory>
#include <set>
#include <vector>

#include "master_proto.hpp"
#include "qsocket.hpp"

#include <windows.h>
#include <windowsx.h>

extern "C" {
#include "enctypex_decoder.h"
}

static const wchar_t main_window_class[] = L"qarma.main";
static const wchar_t main_window_title[] = L"Arma 2 player hunter";

struct window_data {
	NONCLIENTMETRICS metrics;
	std::unique_ptr<HFONT__, decltype (&DeleteObject)> message_font;

	master_protocol_stage master_stage;
	SOCKET_wrapper master_socket;
	enctypex_data_t enctypex_data;
	std::array<unsigned char, 9> master_validate;
	std::vector<unsigned char> master_data;
	std::set<server_endpoint> server_list;

	window_data (): message_font (nullptr, DeleteObject), master_stage (error) {
		master_data.reserve (16384);
	}
};

BOOL main_window_on_create (HWND hWnd, LPCREATESTRUCT lpcs);
LRESULT main_window_on_destroy (HWND hWnd);
LRESULT main_window_on_command (HWND hWnd, int id, HWND hCtl, UINT codeNotify);
LRESULT main_window_on_socket (HWND hWnd, SOCKET socket, WORD wsa_event, WORD wsa_error);
LRESULT CALLBACK main_window_wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
