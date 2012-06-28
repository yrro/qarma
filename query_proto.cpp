#include "query_proto.hpp"

#include <cassert>
#include <sstream>

#include <windows.h>

#include "explain.hpp"

void query_proto::begin (const server_endpoint& ep, HWND hwnd, UINT window_message) {
	assert (state == available);
	//server_info i = server_info ();
	//i.when = GetTickCount64 ();
	//on_complete (i);
	
	socket = qsocket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket == INVALID_SOCKET) {
		server_info i = server_info ();
		i.when = GetTickCount64 ();
		i.error = wstrerror (WSAGetLastError ());
		on_complete (i);
		return;
	}
	
	if (WSAAsyncSelect (socket, hwnd, window_message, FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE) == SOCKET_ERROR) {
		server_info i = server_info ();
		i.when = GetTickCount64 ();
		i.error = wstrerror (WSAGetLastError ());
		return;
	}

	sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = ep.port;
	sa.sin_addr = ep.ip;
	int r = connect (socket, reinterpret_cast<const sockaddr*> (&sa), sizeof sa);
	assert (r == SOCKET_ERROR && WSAGetLastError () == WSAEWOULDBLOCK);

	state = connecting;
}

void query_proto::state_connecting (WORD event, WORD error) {
	switch (event) {
	case FD_CONNECT:
		if (!error) {
			state = connected;
			return;
		}
		/* fall through */

	default:
		state = available;
		server_info i = server_info ();
		i.when = GetTickCount64 ();
		i.error = wstrerror (WSAGetLastError ());
		on_complete (i);
	}
}

void query_proto::state_connected (WORD event, WORD error) {
	switch (event) {
	case FD_WRITE:
		if (!error) {
			std::string packet = "\xfe\xfd" // protocol identifier
			                     "\x09" // request challenge strings
			                     "\x00\x01\x04\x09"; // identifier
			int r = send (socket, &packet[0], packet.size (), 0);
			if (r == SOCKET_ERROR) {
				assert (0); abort (); // XXX is this even possible?
				state = available;
				server_info i = server_info ();
				i.when = GetTickCount64 ();
				i.error = wstrerror (WSAGetLastError ());
				on_complete (i);
				return;
			}
			if (r != static_cast<int> (packet.size ())) {
				state = available;
				server_info i = server_info ();
				i.when = GetTickCount64 ();
				i.error = L"short write";
				on_complete (i);
				return;
			}
			state = request_sent;
			return;
		}
		/* fall through */

	default:
		state = available;
		server_info i = server_info ();
		i.when = GetTickCount64 ();
		i.error = wstrerror (WSAGetLastError ());
		on_complete (i);
	}
}



// vim: ts=4 sts=4 sw=4 noet
