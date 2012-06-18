#include "main_window.hpp"

#include <array>
#include <cassert>
#include <iterator>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

#include <windows.h>

#include <commctrl.h>
#include <ws2tcpip.h>

#include "explain.hpp"

#if 0
# define REQUEST R"(\hostname\gamever\country\mapname\gametype\gamemode\numplayers\maxplayers)"
#else
# define REQUEST ""
#endif

static const int control_margin = 10;

static const int idc_main_master_load = 0;
static const int idc_main_progress = 1;
static const int idc_main_master_count = 2;

static const UINT WM_WSAASYNC = WM_USER + 0;

BOOL main_window_on_create (HWND hWnd, LPCREATESTRUCT /*lpcs*/) {
	window_data* wd = new window_data;
	SetWindowLongPtr (hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (wd));

	wd->metrics = NONCLIENTMETRICS ();
	wd->metrics.cbSize = sizeof wd->metrics;
	if (!SystemParametersInfo (SPI_GETNONCLIENTMETRICS, sizeof wd->metrics, &wd->metrics, 0)) {
		explain (L"SystemParametersInfo failed");
		return -1;
	}
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

	CreateWindow (PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
		control_margin, control_margin + 100, 200, 20,
		hWnd, reinterpret_cast<HMENU> (idc_main_progress), nullptr, nullptr);

	if (HWND s = CreateWindow (WC_STATIC, L"test", WS_CHILD | WS_VISIBLE,
		control_margin, control_margin + 120, 200, 24,
		hWnd, reinterpret_cast<HMENU> (idc_main_master_count), nullptr, nullptr))
	{
		SetWindowFont (s, wd->message_font.get (), true);
	}

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
			SendMessage (GetDlgItem (hWnd, idc_main_progress), PBM_SETMARQUEE, 1, 0);
			SetWindowText (GetDlgItem (hWnd, idc_main_master_count), L"Getting server list (0 KiB)");

			/* resolve */
			std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> lookup (nullptr, freeaddrinfo);
			{
				addrinfo hints = addrinfo ();
				hints.ai_family = AF_INET;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_protocol = IPPROTO_TCP;
				addrinfo* tmp;
				int r = getaddrinfo ("arma2oapc.ms1.gamespy.com", "28910", &hints, &tmp);
				if (r) {
					explain (L"getaddrinfo failure", WSAGetLastError ());
					return 0;
				}
				lookup.reset (tmp);
			}
			/*for (addrinfo* a = lookup.get (); a; a = a->ai_next) {
				std::wostringstream ss;
				ss << L"family:" << a->ai_family << "\ntype:" << a->ai_socktype << "\nprotocol:" << a->ai_protocol << " name:" << a->ai_canonname;
				MessageBox (hWnd, ss.str ().c_str (), main_window_title, 0);
			}*/

			/* connect */
			wd->master_stage = initial;
			wd->master_data.clear ();
			wd->master_socket = SOCKET_wrapper (AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (wd->master_socket == INVALID_SOCKET) {
				explain (L"socket failed", WSAGetLastError ());
				return 0;
			}
			/*{
				std::wostringstream ss;
				ss << L"created socket " << wd->master_socket;
				MessageBox (hWnd, ss.str ().c_str (), main_window_title, 0);
			}*/
			if (WSAAsyncSelect (wd->master_socket, hWnd, WM_WSAASYNC, FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE) == SOCKET_ERROR) {
				explain (L"WSAAsyncSelect failed", WSAGetLastError ());
				return 0;
			}
			std::vector<int> errors;
			for (addrinfo* a = lookup.get (); a; a = a->ai_next) {
				int r = connect (wd->master_socket, a->ai_addr, a->ai_addrlen);
				assert (r); // non-blocking so should never return 0
				if (r == SOCKET_ERROR && WSAGetLastError () == WSAEWOULDBLOCK) {
					errors.clear ();
					return 0;
				}
				errors.push_back (WSAGetLastError ());
			}
			if (! errors.empty ()) {
				std::wostringstream ss (L"Connection errors occurred:\n\n");
				for (int& e: errors) {
					ss << e << '\n';
				}
				MessageBox (hWnd, ss.str ().c_str (), L"connect failed", 0);
				return 0;
			}
			return 0;
		}
		return 0;
	}

	abort (); // unknown control
}

LRESULT main_window_on_socket (HWND hWnd, SOCKET socket, WORD wsa_event, WORD wsa_error) {
	window_data* wd = reinterpret_cast<window_data*> (GetWindowLongPtr (hWnd, GWLP_USERDATA));

	if (error) {
		explain (L"socket error", error);
		wd->master_stage = error;
		return 0;
	}

	assert (socket == wd->master_socket);
	/*if (wParam != wd->master_socket) {
		std::wostringstream ss;
		ss << L"Unrecognised (socket?) event.\numsg:" << uMsg << "\nsocket:" << wParam << " (expected " << wd->master_socket << ").\nevent:" << WSAGETSELECTEVENT (lParam) << "\nerror:" << WSAGETSELECTERROR (lParam);
		MessageBox (hWnd, ss.str ().c_str (), main_window_title, 0);
		abort ();
		break;
	}*/

	switch (wd->master_stage) {
	case error:
		/* ignore */
		return 0;

	case initial:
		switch (wsa_event) {
		case FD_CONNECT:
			/* ignore the event; it was only sent because we wanted `connect` to be non-blocking */
			return 0;

		case FD_WRITE:
			wd->enctypex_data.start = 0;
			enctypex_decoder_rand_validate (&wd->master_validate[0]);
			std::ostringstream packet;
			packet << '\0'
			       << '\1'
			       << '\3'
			       << '\0' // 32-bit
			       << '\0'
			       << '\0'
			       << '\0'
			       << "arma2oapc" << '\0'
			       << "gslive" << '\0';
			std::copy (wd->master_validate.begin (), wd->master_validate.end () - 1, std::ostreambuf_iterator<char> (packet)); // note: don't copy the final '\0' byte of master_validate
			packet << "" << '\0' // filter (note, not preceeded by a '\0' separator either
			       << REQUEST << '\0'
			       << '\0'
			       << '\0'
			       << '\0'
			       << '\1'; // 1 = requested information
			std::vector<char> buf (2 + packet.str ().size ());
			WSAHtons (wd->master_socket, buf.size (), reinterpret_cast<u_short*> (&buf[0]));
			const std::string s = packet.str ();
			std::copy (s.begin (), s.end (), 2 + buf.begin ());
			int r = send (wd->master_socket, &buf[0], buf.size (), 0);
			if (r == SOCKET_ERROR && r != WSAEWOULDBLOCK) {
				explain (L"send failure", r);
				wd->master_stage = error;
			} else {
				wd->master_stage = request_sent;
				shutdown (wd->master_socket, SD_SEND);
			}
			return 0;
		}
		break;

	case request_sent:
		switch (wsa_event) {
		case FD_READ:
			{
				int r;
				std::array<char, 8192> buf;
				r = recv (wd->master_socket, &buf[0], buf.size (), 0);
				if (r == SOCKET_ERROR) {
					explain (L"recv failed");
					wd->master_stage = error;
					wd->master_socket = SOCKET_wrapper ();
					return 0;
				} else if (r == 0) {
					abort (); // should not reach because we should receive FD_CLOSE instead
					wd->master_stage = error;
					wd->master_socket = SOCKET_wrapper ();

					std::wostringstream ss;
					ss << L"premature end-of-data. recieved " << wd->master_data.size () << " bytes in total";
					MessageBox (hWnd, ss.str ().c_str (), main_window_title, 0);
					return 0;
				}
				std::copy (buf.begin (), buf.begin () + r, std::back_inserter (wd->master_data));

				std::wostringstream ss;
				ss << "Getting server list (" << wd->master_data.size () / 1024 << " KiB)";
				SetWindowText (GetDlgItem (hWnd, idc_main_master_count), ss.str ().c_str ());
			}

			{
				int len = wd->master_data.size ();
				unsigned char* endp = enctypex_decoder (reinterpret_cast<unsigned char*> (const_cast<char*> ("Xn221z")), &wd->master_validate[0], &wd->master_data[0], &len, &wd->enctypex_data);
				assert (endp);
				{
					std::wostringstream ss;
					ss << L"master_data:" << reinterpret_cast<void*> (&wd->master_data[0]) << " len:" << len << " ipport:" << endp << " datalen:" << (wd->master_data.size () - (reinterpret_cast<unsigned char*> (endp) - &wd->master_data[0]));
					OutputDebugString (ss.str ().c_str ());
				}
				if (endp && enctypex_decoder_convert_to_ipport (endp, wd->master_data.size () - (reinterpret_cast<unsigned char*> (endp) - &wd->master_data[0]), nullptr, nullptr, 0, 0)) {
					wd->master_stage = complete;
					shutdown (wd->master_socket, SD_RECEIVE);
				}
			}
			return 0;
		}
		break;

	case complete:
		switch (wsa_event) {
		case FD_CLOSE:
			wd->master_socket = SOCKET_wrapper ();

			SendMessage (GetDlgItem (hWnd, idc_main_progress), PBM_SETMARQUEE, 0, 0);

			static_assert (sizeof (server_endpoint) == 6, "server_endpoint is a weird size");
			std::vector<server_endpoint> decoded_data (wd->master_data.size () / 5); // XXX size seems like a bit of a guess!
			int len = enctypex_decoder_convert_to_ipport (&wd->master_data[0] + wd->enctypex_data.start, wd->master_data.size () - wd->enctypex_data.start, reinterpret_cast<unsigned char*> (decoded_data.data ()), nullptr, 0, 0);
			assert (len >= 0); // XXX handle
			std::copy (decoded_data.begin (), decoded_data.end (), std::inserter (wd->server_list, wd->server_list.begin ()));
			/*for (server_endpoint& e: decoded_data) {
				wd->server_list.insert (e);
				std::wostringstream ss;
				ss << inet_ntoa (e.ip) << ':' << ntohs (e.port);
				MessageBox (hWnd, ss.str ().c_str (), L"", 0);
			}*/

			std::wostringstream ss;
			ss << wd->server_list.size () << " servers";
			SetWindowText (GetDlgItem (hWnd, idc_main_master_count), ss.str ().c_str ());

			return 0;
		break;
		}
	}

	{
		std::wostringstream ss;
		ss << L"Master protocol sync error.\n\nmaster_stage:" << wd->master_stage << "\nevent:" << wsa_event << "\nerror:" << wsa_error;
		MessageBox (hWnd, ss.str ().c_str (), main_window_title, 0);
	}
	return 0;
}

LRESULT CALLBACK main_window_wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	/*std::wostringstream ss;
	ss << L"socket " << s << " created";
	OutputDebugString (ss.str ().c_str ());*/

	switch (uMsg) {
	HANDLE_MSG(hWnd, WM_CREATE, main_window_on_create);
	HANDLE_MSG(hWnd, WM_DESTROY, main_window_on_destroy);
	HANDLE_MSG(hWnd, WM_COMMAND, main_window_on_command);
	case WM_WSAASYNC:
		return main_window_on_socket (hWnd, wParam, WSAGETSELECTEVENT (lParam), WSAGETSELECTERROR (lParam));
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
