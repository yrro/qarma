#include "master_proto.hpp"

#include <cassert>
#include <sstream>

#include <windowsx.h>

// <http://blogs.msdn.com/b/oldnewthing/archive/2004/10/25/247180.aspx>
extern "C" IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

#include "explain.hpp"

#if 0
# define REQUEST R"(\hostname\gamever\country\mapname\gametype\gamemode\numplayers\maxplayers)"
#else
# define REQUEST ""
#endif

namespace {
	const UINT window_message = RegisterWindowMessage (L"{be193e7e-2b1f-4963-8a3a-671aa79746e8}");

	BOOL on_create (HWND hWnd, LPCREATESTRUCT lpcs) {
		assert (lpcs->lpCreateParams);
		SetWindowLongPtr (hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (lpcs->lpCreateParams)); // XXX handle errors
		return TRUE;
	}
}
	
LRESULT WINAPI master_protocol::wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	HANDLE_MSG (hWnd, WM_CREATE, on_create);
	}
	
	assert (window_message);
	if (uMsg == window_message) {
		master_protocol* self = reinterpret_cast<master_protocol*> (GetWindowLongPtr (hWnd, GWLP_USERDATA));
		assert (self);

		if (self->socket == INVALID_SOCKET || wParam != self->socket) {
			return 0;
		}

		const WORD event = WSAGETSELECTEVENT (lParam);
		const WORD error = WSAGETSELECTERROR (lParam);
		switch (self->state) {
		case master_protocol_state::error:
			break;
		case connecting:
			self->state_connecting (event, error);
			break;
		case connected:
			self->state_connected (event, error);
			break;
		case request_sent:
			self->state_request_sent (event, error);
			break;
		case complete:
			self->state_complete (event, error);
			break;
		}
	}

	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}

master_protocol::master_protocol (): hwnd (nullptr, DestroyWindow), state (error), lookup (nullptr, freeaddrinfo) {
	static const wchar_t window_class[] = L"{e63f597b-c513-4fb2-b9db-62c2eb572d8b}";
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

	data.reserve (16384);
}

void master_protocol::refresh () {
	state = master_protocol_state::error; // failure is assumed

	data.clear ();

	// TODO async
	lookup.reset (nullptr);
	{
		addrinfo hints = addrinfo ();
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		addrinfo* tmp;
		int r = getaddrinfo ("arma2oapc.ms1.gamespy.com", "28910", &hints, &tmp); // TODO load balance
		if (r) {
			on_error (wstrerror (WSAGetLastError ()));
			return;
		}
		lookup.reset (tmp);
	}

	socket = qsocket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket == INVALID_SOCKET) {
		on_error (wstrerror (GetLastError ()));
		return;
	}
	
	if (WSAAsyncSelect (socket, hwnd.get (), window_message, FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE) == SOCKET_ERROR) {
		on_error (wstrerror (GetLastError ()));
		return;
	}

	assert (lookup);
	int r = connect (socket, lookup->ai_addr, lookup->ai_addrlen);
	assert (r == SOCKET_ERROR && WSAGetLastError () == WSAEWOULDBLOCK);
	
	state = connecting;
}

void master_protocol::state_connecting (WORD event, WORD error) {
	switch (event) {
	case FD_CONNECT:
		if (error) {
			state = master_protocol_state::error;
			on_error (wstrerror (error));
			return;
		}
		state = connected;
		return;

	default:
		std::wostringstream ss;
		ss << L"synchronization error: state:" << state << " event:" << event << " error:" << error;
		state = master_protocol_state::error;
		on_error (ss.str ());
	}
}

void master_protocol::state_connected (WORD event, WORD error) {
	switch (event) {
	case FD_WRITE:
		if (error) {
			state = master_protocol_state::error;
			on_error (wstrerror (error));
			return;
		}

		{
			enctypex_data.start = 0;
			enctypex_decoder_rand_validate (&master_validate[0]);
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
			std::copy (master_validate.begin (), master_validate.end () - 1, std::ostreambuf_iterator<char> (packet)); // note: don't copy the final '\0' byte of master_validate
			packet << "" << '\0' // filter (note, not preceeded by a '\0' separator either
				   << REQUEST << '\0'
				   << '\0'
				   << '\0'
				   << '\0'
				   << '\1'; // 1 = requested information
			std::vector<char> buf (2 + packet.str ().size ());
			WSAHtons (socket, buf.size (), reinterpret_cast<u_short*> (&buf[0]));
			const std::string s = packet.str ();
			std::copy (s.begin (), s.end (), 2 + buf.begin ());

			int r = send (socket, &buf[0], buf.size (), 0);
			if (r == SOCKET_ERROR) {
				abort (); // XXX is this even possible?
				state = master_protocol_state::error;
				on_error (wstrerror (WSAGetLastError ()));
				return;
			}
			if (r != static_cast<int> (buf.size ())) {
				state = master_protocol_state::error;
				on_error (L"short write");
				return;
			}
		}
		shutdown (socket, SD_SEND); // XXX error check
		state = request_sent;
		return;

	default:
		std::wostringstream ss;
		ss << L"synchronization error: state:" << state << " event:" << event << " error:" << error;
		state = master_protocol_state::error;
		on_error (ss.str ());
	}
}

void master_protocol::state_request_sent (WORD event, WORD error) {
	switch (event) {
	case FD_READ:
		if (error) {
			state = master_protocol_state::error;
			on_error (wstrerror (error));
			return;
		}
		{
			std::array<char, 8192> buf;
			int r = recv (socket, &buf[0], buf.size (), 0);
			assert (r != SOCKET_ERROR);
			assert (r > 0);
			std::copy (buf.begin (), buf.begin () + r, std::back_inserter (data));

			on_progress (data.size ());
		}

		{
			int len = data.size ();
			unsigned char* endp = enctypex_decoder (reinterpret_cast<unsigned char*> (const_cast<char*> ("Xn221z")), &master_validate[0], &data[0], &len, &enctypex_data);
			assert (endp);
			if (endp && enctypex_decoder_convert_to_ipport (endp, data.size () - (reinterpret_cast<unsigned char*> (endp) - &data[0]), nullptr, nullptr, 0, 0)) {
				shutdown (socket, SD_RECEIVE); // XXX handle errors
				state = complete;
			}
		}
		return;

	default:
		std::wostringstream ss;
		ss << L"synchronization error: state:" << state << " event:" << event << " error:" << error;
		state = master_protocol_state::error;
		on_error (ss.str ());
	}
}

void master_protocol::state_complete (WORD event, WORD error) {
	switch (event) {
	case FD_CLOSE:
		if (error) {
			state = master_protocol_state::error;
			on_error (wstrerror (error));
			return;
		}

		socket = qsocket ();

		static_assert (sizeof (server_endpoint) == 6, "server_endpoint is a weird size");
		{
			std::vector<server_endpoint> decoded_data (data.size () / 5); // XXX size seems like a bit of a guess!
			int len = enctypex_decoder_convert_to_ipport (&data[0] + enctypex_data.start, data.size () - enctypex_data.start, reinterpret_cast<unsigned char*> (decoded_data.data ()), nullptr, 0, 0);
			assert (len >= 0); // XXX handle
			std::for_each (decoded_data.cbegin (), decoded_data.cend (), on_found);
		}

		on_complete ();
		return;

	default:
		std::wostringstream ss;
		ss << L"synchronization error: state:" << state << " event:" << event << " error:" << error;
		state = master_protocol_state::error;
		on_error (ss.str ());
	}
}
// vim: ts=4 sts=4 sw=4 noet
