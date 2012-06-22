#ifndef QARMA_QUERY_PROTO_HPP
#define QARMA_QUERY_PROTO_HPP

#include <functional>

#include "server_common.hpp"

struct query_proto {
	enum {
		available
	} state;

	void begin (const server_endpoint& endpoint);

	std::function<void (const server_info&)> on_complete;
};

#endif

// vim: ts=4 sts=4 sw=4 noet
