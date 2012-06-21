#ifndef QARMA_SERVER_COMMON_HPP
#define QARMA_SERVER_COMMON_HPP

#include <cstdint>

#include <inaddr.h>

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

#endif
// vim: ts=4 sts=4 sw=4 noet
