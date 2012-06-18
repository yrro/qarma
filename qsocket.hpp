#ifndef QARMA_QSOCKET_HPP
#define QARMA_QSOCKET_HPP

#define STRICT
#include <winsock2.h>
#include <windows.h>

class SOCKET_wrapper {
	SOCKET s;
public:
	SOCKET_wrapper (): s (INVALID_SOCKET) {}

	SOCKET_wrapper (int af, int type, int protocol): s (socket (af, type, protocol)) {
#ifdef QARMA_DEBUG_QSOCKET
		std::wostringstream ss;
		ss << L"socket " << s << " created";
		OutputDebugString (ss.str ().c_str ());
#endif
	}

	SOCKET_wrapper& operator= (SOCKET_wrapper&& other) {
#ifdef QARMA_DEBUG_QSOCKET
		std::wostringstream ss;
		ss << L"socket " << this->s << " move assignment (other socket " << other.s << ')';
		OutputDebugString (ss.str ().c_str ());
#endif
		std::swap (this->s, other.s);
		return *this;
	}

	SOCKET_wrapper (SOCKET_wrapper&& other): s (INVALID_SOCKET) {
#ifdef QARMA_DEBUG_QSOCKET
		std::wostringstream ss;
		ss << L"socket move constructor (other socket " << other.s;
#endif
		*this = std::move (other);
	}

	SOCKET_wrapper& operator= (const SOCKET_wrapper&) = delete;
	SOCKET_wrapper (const SOCKET_wrapper&) = delete;

	~SOCKET_wrapper () {
		if (s != INVALID_SOCKET) {
#ifdef QARMA_DEBUG_QSOCKET
			std::wostringstream ss;
			ss << L"socket " << s << " destroyed";
			OutputDebugString (ss.str ().c_str ());
#endif
			closesocket (s);
		}
	}

	operator SOCKET () const {
		return s;
	}
};

#endif
