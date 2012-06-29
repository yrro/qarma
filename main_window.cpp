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
#include <process.h>
#include <ws2tcpip.h>

#include "explain.hpp"

namespace {
	const int control_margin = 10;

	const int idc_main_master_load = 0;
	const int idc_main_master_progress = 1;
	const int idc_main_master_count = 2;
	const int idc_main_query_progress = 3;

	BOOL main_window_on_create (HWND hWnd, LPCREATESTRUCT lpcs) {
		window_data* wd = reinterpret_cast<window_data*> (lpcs->lpCreateParams);
		SetWindowLongPtr (hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (wd));

		wd->metrics = NONCLIENTMETRICS ();
		wd->metrics.cbSize = sizeof wd->metrics;
		BOOL r = SystemParametersInfo (SPI_GETNONCLIENTMETRICS, sizeof wd->metrics, &wd->metrics, 0);
		assert (r);
		wd->message_font.reset (CreateFontIndirect (&wd->metrics.lfMessageFont));

		HWND b = CreateWindow (WC_BUTTON, L"Get more servers",
			WS_CHILD | WS_VISIBLE,
			control_margin, control_margin, 200, 24,
			hWnd, reinterpret_cast<HMENU> (idc_main_master_load),
			nullptr, nullptr);
		if (b) {
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

		wd->on_master_begin = [wd, pmaster, pquery, b, s] () {
			wd->master_refreshing = true;
			Button_Enable (b, false);

			SendMessage (pmaster, PBM_SETMARQUEE, 1, 0);
			ShowWindow (pmaster, SW_SHOW);
			ShowWindow (pquery, SW_HIDE);

			SetWindowText (s, L"Getting server list (0 KiB)");
		};
		wd->on_master_error = [wd, pmaster, pquery, b, s] (const std::wstring& msg) {
			wd->master_refreshing = false;
			Button_Enable (b, true);

			SendMessage (pmaster, PBM_SETMARQUEE, 0, 0);
			ShowWindow (pmaster, SW_HIDE);
			ShowWindow (pquery, SW_SHOW);

			SetWindowText (s, msg.c_str ());
		};
		wd->on_master_progress = [s] (unsigned int p) {
			std::wostringstream ss;
			ss << "Getting server list (" << p / 1024 << " KiB)";
			SetWindowText (s, ss.str ().c_str ());
		};
		wd->on_master_found = [wd] (const server_endpoint& ep) {
			wd->qm.add_server (ep);
		};
		wd->on_master_complete = [wd, pmaster, pquery, b, s] () {
			wd->master_refreshing = false;
			Button_Enable (b, true);

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
			SetWindowText (s, ss.str ().c_str ());
		};
		static int c = 0;
		wd->qm.on_found = [&c] (const server_info& /*info*/) {
			++c;
			//std::wostringstream ss; ss << c;
			//OutputDebugString (ss.str ().c_str ());
		};

		return TRUE; // later mangled by HANDLE_WM_CREATE macro
	}

	LRESULT main_window_on_destroy (HWND) {
		PostQuitMessage (0);
		return 0;
	}

	LRESULT main_window_on_command (HWND hWnd, int id, HWND /*hCtl*/, UINT codeNotify) {
		window_data* wd = reinterpret_cast<window_data*> (GetWindowLongPtr (hWnd, GWLP_USERDATA));

		switch (id) {
		case idc_main_master_load:
			switch (codeNotify) {
			case BN_CLICKED:
				if (wd->mpt)
					return 0;
				wd->mpa.hwnd = hWnd;
				wd->mpt.reset (reinterpret_cast<void*> (_beginthreadex (nullptr, 0, &master_proto, &wd->mpa, CREATE_SUSPENDED, nullptr)));
				if (!wd->mpt) {
					std::wostringstream ss;
					ss << L"_beginthreadex: " << wstrerror (errno);
					wd->on_master_error (ss.str ());
					return 0;
				}
				if (ResumeThread (wd->mpt.get ()) == static_cast<DWORD> (-1)) {
					std::wostringstream ss;
					ss << L"ResumeThread: " << wstrerror (GetLastError ());
					wd->on_master_error (ss.str ());
					return 0;
				}
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

	window_data* wd = reinterpret_cast<window_data*> (GetWindowLongPtr (hWnd, GWLP_USERDATA));
	if (wd) {
		if (uMsg == qm_master_begin)
			wd->on_master_begin ();
		else if (uMsg == qm_master_error) {
			wd->on_master_error (*reinterpret_cast<const std::wstring*> (lParam));
		}
		else if (uMsg == qm_master_progress)
			wd->on_master_progress (lParam);
		else if (uMsg == qm_master_found)
			wd->on_master_found (*reinterpret_cast<const server_endpoint*> (lParam));
		else if (uMsg == qm_master_complete) {
			wd->on_master_complete ();
		}
	}

	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}


window_data::window_data (): message_font (nullptr, DeleteObject), master_refreshing (false), mpt (nullptr, CloseHandle) {}

// vim: ts=4 sts=4 sw=4 noet
