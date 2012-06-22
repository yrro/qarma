#include "query_proto.hpp"

#include <windows.h>

void query_proto::begin (const server_endpoint& ep) {
	server_info i = server_info ();
	i.when = GetTickCount64 ();
	on_complete (i);
}

// vim: ts=4 sts=4 sw=4 noet
