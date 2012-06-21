#ifndef QARMA_MASTER_PROTO_HPP
#define QARMA_MASTER_PROTO_HPP

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>

extern "C" {
#include "enctypex_decoder.h"
}

#include "qsocket.hpp"

struct server_endpoint {
	in_addr ip;
	std::uint16_t port;
} __attribute ((packed));

inline bool operator< (const in_addr& a, const in_addr& b) {
	return a.S_un.S_addr < b.S_un.S_addr;
}

inline bool operator== (const in_addr& a, const in_addr& b) {
	return a.S_un.S_addr == b.S_un.S_addr;
}

inline bool operator< (const server_endpoint& a, const server_endpoint& b) {
	return a.ip < b.ip || (a.ip == b.ip && a.port < b.port);
}

class master_protocol {
	std::unique_ptr<HWND__, decltype(&DestroyWindow)> hwnd;
	static LRESULT WINAPI wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

	enctypex_data_t enctypex_data;
	std::array<unsigned char, 9> master_validate;

	void state_connecting (WORD event, WORD error);
	void state_connected (WORD event, WORD error);
	void state_request_sent (WORD event, WORD error);
	void state_complete (WORD event, WORD error);

public:
	master_protocol& operator= (const master_protocol&) = delete;
	master_protocol (const master_protocol&) = delete;

	master_protocol ();

	void refresh ();

	std::function<void (const std::wstring&)> on_error;
	std::function<void (unsigned int)> on_progress;
	std::function<void (const server_endpoint&)> on_found;
	std::function<void ()> on_complete;
};

#endif
