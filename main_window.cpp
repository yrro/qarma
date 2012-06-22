#include "main_window.hpp"

#include <array>
#include <cassert>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

#include <windows.h>
#include <windowsx.h>

#include <commctrl.h>
#include <ws2tcpip.h>

#include "explain.hpp"

namespace {
	const int control_margin = 10;

	const int idc_main_master_load = 0;
	const int idc_main_master_progress = 1;
	const int idc_main_master_count = 2;
	const int idc_main_query_progress = 3;

	BOOL main_window_on_create (HWND hWnd, LPCREATESTRUCT /*lpcs*/) {
		window_data* wd = new window_data;
		SetWindowLongPtr (hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (wd));

		wd->metrics = NONCLIENTMETRICS ();
		wd->metrics.cbSize = sizeof wd->metrics;
		BOOL r = SystemParametersInfo (SPI_GETNONCLIENTMETRICS, sizeof wd->metrics, &wd->metrics, 0);
		assert (r);
		wd->message_font.reset (CreateFontIndirect (&wd->metrics.lfMessageFont));

		if (HWND b = CreateWindow (WC_BUTTON, L"Get more servers",
			WS_CHILD | WS_VISIBLE,
			control_margin, control_margin, 200, 24,
			hWnd, reinterpret_cast<HMENU> (idc_main_master_load),
			nullptr, nullptr))
		{
			SetWindowFont (b, wd->message_font.get (), true);
			PostMessage (hWnd, WM_COMMAND, MAKELONG(idc_main_master_load, BN_CLICKED), reinterpret_cast<LPARAM> (b));
		}

		HWND pmaster = CreateWindow (PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
			control_margin, control_margin + 100, 200, 20,
			hWnd, reinterpret_cast<HMENU> (idc_main_master_progress), nullptr, nullptr);
		HWND pquery = CreateWindow (PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
			control_margin, control_margin + 100, 200, 20,
			hWnd, reinterpret_cast<HMENU> (idc_main_query_progress), nullptr, nullptr);

		HWND s = CreateWindow (WC_STATIC, L"test", WS_CHILD | WS_VISIBLE,
			control_margin, control_margin + 120, 200, 24,
			hWnd, reinterpret_cast<HMENU> (idc_main_master_count), nullptr, nullptr);
		if (s) {
			SetWindowFont (s, wd->message_font.get (), true);
		}

		wd->mproto.on_begin = [wd, pmaster, pquery, s] () {
			wd->master_refreshing = true;

			SendMessage (pmaster, PBM_SETMARQUEE, 1, 0);
			ShowWindow (pmaster, SW_SHOW);
			ShowWindow (pquery, SW_HIDE);

			SetWindowText (s, L"Getting server list (0 KiB)");
		};
		wd->mproto.on_error = [wd, pmaster, pquery, s] (const std::wstring& msg) {
			wd->master_refreshing = false;

			SendMessage (pmaster, PBM_SETMARQUEE, 0, 0);
			ShowWindow (pmaster, SW_HIDE);
			ShowWindow (pquery, SW_SHOW);

			SetWindowText (s, msg.c_str ());
		};
		wd->mproto.on_progress = [s] (unsigned int p) {
			std::wostringstream ss;
			ss << "Getting server list (" << p / 1024 << " KiB)";
			SetWindowText (s, ss.str ().c_str ());
		};
		wd->mproto.on_found = [wd] (const server_endpoint& ep) {
			wd->qm.add_server (ep);
		};
		wd->mproto.on_complete = [wd, pmaster, pquery, s] () {
			wd->master_refreshing = false;

			SendMessage (pmaster, PBM_SETMARQUEE, 0, 0);
			ShowWindow (pmaster, SW_HIDE);
			ShowWindow (pquery, SW_SHOW);

			std::wostringstream ss;
			ss << wd->qm.server_count () << L" servers";
			SetWindowText (s, ss.str ().c_str ());
		};

		wd->qm.on_progress = [wd, pquery, s] (unsigned int ndone, unsigned int nqueued) {
			if (wd->master_refreshing)
				return;

			SendMessage (pquery, PBM_SETRANGE32, 0, nqueued);
			SendMessage (pquery, PBM_SETPOS, ndone, 0);

			std::wostringstream ss;
			ss << ndone << L" of " << nqueued << L" servers queried";
		};

		return TRUE; // later mangled by HANDLE_WM_CREATE macro
	}

	LRESULT main_window_on_destroy (HWND hWnd) {
		delete reinterpret_cast<window_data*> (GetWindowLongPtr (hWnd, GWLP_USERDATA));
		PostQuitMessage (0);
		return 0;
	}

	LRESULT main_window_on_command (HWND hWnd, int id, HWND /*hCtl*/, UINT codeNotify) {
		window_data* wd = reinterpret_cast<window_data*> (GetWindowLongPtr (hWnd, GWLP_USERDATA));

		switch (id) {
		case idc_main_master_load:
			switch (codeNotify) {
			case BN_CLICKED:
				wd->mproto.refresh ();
				return 0;
			}
			return 0;
		}

		assert (0); // unknown control, catch during development
		return 0;
	}
}

LRESULT CALLBACK main_window_wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	HANDLE_MSG(hWnd, WM_CREATE, main_window_on_create);
	HANDLE_MSG(hWnd, WM_DESTROY, main_window_on_destroy);
	HANDLE_MSG(hWnd, WM_COMMAND, main_window_on_command);
	}
	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}


window_data::window_data (): message_font (nullptr, DeleteObject), master_refreshing (false) {}

// vim: ts=4 sts=4 sw=4 noet
