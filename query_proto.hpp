#ifndef QARMA_QUERY_PROTO_HPP
#define QARMA_QUERY_PROTO_HPP

#include <functional>

#include <windows.h>

#include "server_common.hpp"
#include "qsocket.hpp"

struct query_proto {
	enum {
		available,
		connecting,
		connected,
		init_sent,
		request_sent
	} state;

	qsocket socket;

	void begin (const server_endpoint& endpoint, HWND hwnd, UINT window_message);

	void state_connecting (WORD event, WORD error);
	void state_connected (WORD event, WORD error);
	void state_init_sent (WORD event, WORD error);
	void state_request_sent (WORD event, WORD error);

	std::function<void (const server_info&)> on_complete;
};

#endif

// vim: ts=4 sts=4 sw=4 noet
