#ifndef QARMA_MASTER_PROTO_HPP
#define QARMA_MASTER_PROTO_HPP

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include <sstream>
#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>

extern "C" {
#include "enctypex_decoder.h"
}

#include "explain.hpp"
#include "qsocket.hpp"

#if 0
# define REQUEST R"(\hostname\gamever\country\mapname\gametype\gamemode\numplayers\maxplayers)"
#else
# define REQUEST ""
#endif

struct server_endpoint {
	in_addr ip;
	std::uint16_t port;
} __attribute ((packed));

inline bool operator< (const in_addr& a, const in_addr& b) {
	// assumes network (big) endianess!
	return a.S_un.S_addr < b.S_un.S_addr;
}

inline bool operator== (const in_addr& a, const in_addr& b) {
	return a.S_un.S_addr == b.S_un.S_addr;
}

inline bool operator< (const server_endpoint& a, const server_endpoint& b) {
	return a.ip < b.ip || (a.ip == b.ip && a.port < b.port);
}

class master_protocol {
	enum master_protocol_state {
		error,
		connecting,
		connected,
		request_sent,
		complete
	} state;
	std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> lookup;
	std::vector<unsigned char> data;
	SOCKET_wrapper socket;
	const UINT window_message;
	enctypex_data_t enctypex_data;
	std::array<unsigned char, 9> master_validate;

	void state_connecting (WORD event, WORD error) {
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

	void state_connected (WORD event, WORD error) {
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

	void state_request_sent (WORD event, WORD error) {
		switch (event) {
		case FD_READ:
			if (error) {
				state = master_protocol_state::error;
				on_error (wstrerror (error));
				return;
			}
			{
				int r;
				std::array<char, 8192> buf;
				r = recv (socket, &buf[0], buf.size (), 0);
				if (r == SOCKET_ERROR) {
					abort (); // XXX is this possible?
					state = master_protocol_state::error;
					on_error (wstrerror (WSAGetLastError ()));
					//wd->master_socket = SOCKET_wrapper ();
					return;
				}
				assert (r);
				/*else if (r == 0) {
					wd->master_stage = error;
					wd->master_socket = SOCKET_wrapper ();

					std::wostringstream ss;
					ss << L"premature end-of-data. recieved " << wd->data.size () << " bytes in total";
					MessageBox (hWnd, ss.str ().c_str (), main_window_title, 0);
					return 0;
				}*/
				std::copy (buf.begin (), buf.begin () + r, std::back_inserter (data));

				on_progress (data.size ());
			}

			{
				int len = data.size ();
				unsigned char* endp = enctypex_decoder (reinterpret_cast<unsigned char*> (const_cast<char*> ("Xn221z")), &master_validate[0], &data[0], &len, &enctypex_data);
				assert (endp);
				/*{
					std::wostringstream ss;
					ss << L"data:" << reinterpret_cast<void*> (&data[0]) << " len:" << len << " ipport:" << endp << " datalen:" << (data.size () - (reinterpret_cast<unsigned char*> (endp) - &data[0]));
					OutputDebugString (ss.str ().c_str ());
				}*/
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

	void state_complete (WORD event, WORD error) {
		switch (event) {
		case FD_CLOSE:
			if (error) {
				state = master_protocol_state::error;
				on_error (wstrerror (error));
				return;
			}

			socket = SOCKET_wrapper ();

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

public:
	master_protocol& operator= (const master_protocol&) = delete;
	master_protocol (const master_protocol&) = delete;

	master_protocol (): state (error), lookup (nullptr, freeaddrinfo), window_message (RegisterWindowMessage (L"{be193e7e-2b1f-4963-8a3a-671aa79746e8}")) {
		assert (window_message); // XXX proper error handling
		data.reserve (16384);
	}

	UINT get_window_message () {
		return window_message;
	}

	void refresh (HWND hwnd) {
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

		socket = SOCKET_wrapper (AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (socket == INVALID_SOCKET) {
			on_error (wstrerror (GetLastError ()));
			return;
		}
		
		if (WSAAsyncSelect (socket, hwnd, window_message, FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE) == SOCKET_ERROR) {
			on_error (wstrerror (GetLastError ()));
			return;
		}

		assert (lookup);
		int r = connect (socket, lookup->ai_addr, lookup->ai_addrlen);
		assert (r == SOCKET_ERROR && WSAGetLastError () == WSAEWOULDBLOCK);
		
		state = connecting;
	}

	std::function<void (const std::wstring&)> on_error;
	std::function<void (unsigned int)> on_progress;
	std::function<void (const server_endpoint&)> on_found;
	std::function<void ()> on_complete;

	bool churn (UINT uMsg, WPARAM wParam, LPARAM lParam) {
		std::wostringstream ss;
		ss << uMsg << ' ' << wParam << ' ' << lParam;
		if (uMsg != window_message) {
			return false;
		}

		if (socket == INVALID_SOCKET || wParam != socket) {
			return false;
		}

		if (socket == INVALID_SOCKET || uMsg != window_message || wParam != socket) {
			return false;
		}

		WORD event = WSAGETSELECTEVENT (lParam);
		WORD error = WSAGETSELECTERROR (lParam);

		switch (state) {
		case master_protocol_state::error:
			break;
		case connecting:
			state_connecting (event, error);
			break;
		case connected:
			state_connected (event, error);
			break;
		case request_sent:
			state_request_sent (event, error);
			break;
		case complete:
			state_complete (event, error);
			break;
		}

		return true;
	}
};

#endif
