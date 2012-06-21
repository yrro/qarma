#ifndef QARMA_QSOCKET_HPP
#define QARMA_QSOCKET_HPP

#include <algorithm>

#include <winsock2.h>
#include <windows.h>

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

class qsocket {
	SOCKET s;
public:
	qsocket (): s (INVALID_SOCKET) {}

	qsocket (int af, int type, int protocol): s (socket (af, type, protocol)) {
#ifdef QARMA_DEBUG_QSOCKET
		std::wostringstream ss;
		ss << L"socket " << s << " created";
		OutputDebugString (ss.str ().c_str ());
#endif
	}

	qsocket& operator= (qsocket&& other) {
#ifdef QARMA_DEBUG_QSOCKET
		std::wostringstream ss;
		ss << L"socket " << this->s << " move assignment (other socket " << other.s << ')';
		OutputDebugString (ss.str ().c_str ());
#endif
		std::swap (this->s, other.s);
		return *this;
	}

	qsocket (qsocket&& other): s (INVALID_SOCKET) {
#ifdef QARMA_DEBUG_QSOCKET
		std::wostringstream ss;
		ss << L"socket move constructor (other socket " << other.s;
#endif
		*this = std::move (other);
	}

	qsocket& operator= (const qsocket&) = delete;
	qsocket (const qsocket&) = delete;

	~qsocket () {
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
// vim: ts=4 sts=4 sw=4 noet
