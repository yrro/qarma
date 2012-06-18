#ifndef QARMA_MASTER_PROTO_HPP
#define QARMA_MASTER_PROTO_HPP

#include <cstdint>

#include <winsock2.h>

enum master_protocol_stage {
	error,
	initial,
	request_sent,
	complete
};

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

#endif
