#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <sstream>
#include <vector>

#ifndef UNICODE
#error Must build in Unicode mode
#else
#define _UNICODE
#endif

#define STRICT
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>

#include <commctrl.h>
#include <ws2tcpip.h>

extern "C" {
#include "enctypex_decoder.h"
}

static const int control_margin = 10;

static const wchar_t main_window_class[] = L"qarma.main";
static const wchar_t main_window_title[] = L"Arma 2 player hunter";
static const int idc_main_master_load = 0;

static const UINT WM_WSAASYNC = WM_USER + 0;

#if 0
# define REQUEST R"(\hostname\gamever\country\mapname\gametype\gamemode\numplayers\maxplayers)"
#else
# define REQUEST ""
#endif

void explain (const wchar_t* msg, DWORD e = GetLastError ()) {
	std::wostringstream ss;
	ss << msg << ".\n\n" << e << ": ";

	wchar_t* errmsg = 0;
	if (FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		0, e, 0, reinterpret_cast<LPWSTR> (&errmsg), 0, 0)) {
		ss << errmsg;
	} else {
		ss << "[FormatMessage failure: " << GetLastError ();
	}

	MessageBox (0, ss.str ().c_str (), main_window_title, MB_ICONEXCLAMATION);

	if (errmsg)
		LocalFree (reinterpret_cast<LPVOID> (errmsg));
}

class SOCKET_wrapper {
	SOCKET s;
public:
	SOCKET_wrapper (): s (INVALID_SOCKET) {}

	SOCKET_wrapper (int af, int type, int protocol): s (socket (af, type, protocol)) {
		std::wostringstream ss;
		ss << L"socket " << s << " created";
		OutputDebugString (ss.str ().c_str ());
	}

	SOCKET_wrapper& operator= (SOCKET_wrapper&& other) {
		std::wostringstream ss;
		ss << L"socket " << this->s << " move assignment (other socket " << other.s << ')';
		OutputDebugString (ss.str ().c_str ());
		std::swap (this->s, other.s);
		return *this;
	}

	SOCKET_wrapper (SOCKET_wrapper&& other): s (INVALID_SOCKET) {
		std::wostringstream ss;
		ss << L"socket move constructor (other socket " << other.s;
		*this = std::move (other);
	}

	SOCKET_wrapper& operator= (const SOCKET_wrapper&) = delete;
	SOCKET_wrapper (const SOCKET_wrapper&) = delete;

	~SOCKET_wrapper () {
		if (s != INVALID_SOCKET) {
			std::wostringstream ss;
			ss << L"socket " << s << " destroyed";
			OutputDebugString (ss.str ().c_str ());
			closesocket (s);
		}
	}

	operator SOCKET () const {
		return s;
	}
};

enum master_protocol_stage {
	error,
	initial,
	request_sent,
	complete
};

struct server_endpoint {
	std::uint32_t ip;
	std::uint16_t port;
};

struct window_data {
	NONCLIENTMETRICS metrics;
	std::unique_ptr<HFONT__, decltype (&DeleteObject)> message_font;

	master_protocol_stage master_stage;
	SOCKET_wrapper master_socket;
	enctypex_data_t enctypex_data;
	std::array<unsigned char, 9> master_validate;
	std::vector<unsigned char> master_data;

	window_data (): message_font (0, DeleteObject), master_stage (error) {
		master_data.reserve (16384);
	}
};

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
		0, 0))
	{
		SetWindowFont (b, wd->message_font.get (), true);
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
			/* resolve */
			std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> lookup (0, freeaddrinfo);
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
				if (r == 0 || (r == SOCKET_ERROR && WSAGetLastError () == WSAEWOULDBLOCK)) {
					// XXX why does connect return 0 when it's in non-blocking mode?
					errors.clear ();
					return 0;
				} else {
					errors.push_back (WSAGetLastError ());
				}
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
			//MessageBox (hWnd, L"xmit", main_window_title, 0);
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
					wd->master_stage = error;
					wd->master_socket = SOCKET_wrapper ();

					std::wostringstream ss;
					ss << L"premature end-of-data. recieved " << wd->master_data.size () << " bytes in total";
					MessageBox (hWnd, ss.str ().c_str (), main_window_title, 0);
					abort (); // should not reach because we should receive FD_CLOSE instead
					return 0;
				}
				std::copy (buf.begin (), buf.begin () + r, std::back_inserter (wd->master_data));
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
			std::wostringstream ss;
			ss << L"recieved " << wd->master_data.size () << " bytes in total";
			MessageBox (hWnd, ss.str ().c_str (), main_window_title, 0);
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

class winsock_wrapper {
	int _error;
	WSADATA wsadata;

public:
	winsock_wrapper () {
		_error = WSAStartup (MAKEWORD (2, 2), &wsadata);
	}

	winsock_wrapper& operator= (const winsock_wrapper&) = delete;
	winsock_wrapper (const winsock_wrapper&) = delete;

	~winsock_wrapper () {
		WSACleanup ();
	}

	int error () const {
		return this->_error;
	}
};

int WINAPI wWinMain (HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int nCmdShow) {
	INITCOMMONCONTROLSEX icc = INITCOMMONCONTROLSEX ();
	icc.dwSize = sizeof icc;
	icc.dwICC = ICC_STANDARD_CLASSES;
	InitCommonControlsEx (&icc);

	winsock_wrapper winsock;
	if (winsock.error ()) {
		explain (L"WSAStartup failed", winsock.error ());
		return 1;
	}

	WNDCLASSEX wcex = WNDCLASSEX ();
	wcex.cbSize = sizeof wcex;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = main_window_wndproc;
	wcex.hInstance = hInstance;
	//wcex.hIcon = something;
	wcex.hCursor = LoadCursor (0, IDC_ARROW);
	wcex.hbrBackground = reinterpret_cast<HBRUSH> (1+COLOR_WINDOW);
	wcex.lpszClassName = main_window_class;
	if (!RegisterClassEx (&wcex)) {
		explain (L"RegisterClassEx failed");
		return 1;
	}

	HWND hWnd = CreateWindow (main_window_class, main_window_title,
		WS_OVERLAPPEDWINDOW /*WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX */,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 720,
		0, 0, hInstance, nullptr);
	if (!hWnd) {
		explain (L"CreateWindow failed");
		return 1;
	}

	ShowWindow (hWnd, nCmdShow);
	UpdateWindow (hWnd);

	BOOL r;
	MSG msg;
	while ((r = GetMessage (&msg, 0, 0, 0)) > 0) {
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	if (r == -1) {
		explain (L"GetMessage failed");
		return 1;
	}
	return msg.wParam;
}

// vim: ts=4 sts=4 sw=4 noet
