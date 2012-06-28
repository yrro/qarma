#include "master_proto.hpp"

#include <array>
#include <cassert>
#include <memory>
#include <sstream>
#include <vector>

#include <ws2tcpip.h>

extern "C" {
#include "enctypex_decoder.h"
}

#include "explain.hpp"
#include "qsocket.hpp"
#include "server_common.hpp"

const UINT qm_master_begin = RegisterWindowMessage (L"2337348e-fa22-4528-aac0-8dd90c36b816");
const UINT qm_master_error = RegisterWindowMessage (L"e9267a7b-63df-4ce5-9eda-8ac1d6def246");
const UINT qm_master_progress = RegisterWindowMessage (L"b1e08a3f-a391-4314-8b83-e43e6137649e");
const UINT qm_master_found = RegisterWindowMessage (L"8818b878-d8da-4367-aa66-1c73e42c8494");
const UINT qm_master_complete = RegisterWindowMessage (L"a1837bb5-2756-4bbb-b15d-0f182dcf3d8d");

#if 0
# define REQUEST R"(\hostname\gamever\country\mapname\gametype\gamemode\numplayers\maxplayers)"
#else
# define REQUEST ""
#endif

__stdcall unsigned int master_proto (void* _args) {
	auto args = reinterpret_cast<master_proto_args*> (_args);

	SendMessage (args->hwnd, qm_master_begin, args->id, 0);

	std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> lookup (nullptr, freeaddrinfo);
	{
		addrinfo hints = addrinfo ();
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		addrinfo* tmp;
		int r = getaddrinfo ("arma2oapc.ms1.gamespy.com", "28910", &hints, &tmp); // TODO load balance
		if (r) {
			SendMessage (args->hwnd, qm_master_error, args->id, reinterpret_cast<LPARAM> (wstrerror (WSAGetLastError ()).c_str ()));
			return 0;
		}
		lookup.reset (tmp);
	}

	qsocket socket {lookup->ai_family, lookup->ai_socktype, lookup->ai_protocol};
	if (socket == INVALID_SOCKET) {
		SendMessage (args->hwnd, qm_master_error, args->id, reinterpret_cast<LPARAM> (wstrerror (WSAGetLastError ()).c_str ()));
		return 0;
	}

	int r = connect (socket, lookup->ai_addr, lookup->ai_addrlen);
	if (r == SOCKET_ERROR) {
		SendMessage (args->hwnd, qm_master_error, args->id, reinterpret_cast<LPARAM> (wstrerror (WSAGetLastError ()).c_str ()));
		return 0;
	}

	// transmit request
	std::array<unsigned char, 9> master_validate;
	enctypex_decoder_rand_validate (&master_validate[0]);
	{
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
			SendMessage (args->hwnd, qm_master_error, args->id, reinterpret_cast<LPARAM> (wstrerror (WSAGetLastError ()).c_str ()));
			return 0;
		} else if (r != static_cast<int> (buf.size ())) {
			SendMessage (args->hwnd, qm_master_error, args->id, reinterpret_cast<LPARAM> (L"short send"));
			return 0;
		}
	}
	shutdown (socket, SD_SEND); // XXX error check

	// read response
	enctypex_data_t enctypex_data;
	enctypex_data.start = 0;
	std::vector<unsigned char> data;
	data.reserve (16384);
	while (true) {
		std::array<char, 8192> buf;
		int r = recv (socket, &buf[0], buf.size (), 0);
		if (r == SOCKET_ERROR) {
			SendMessage (args->hwnd, qm_master_error, args->id, reinterpret_cast<LPARAM> (wstrerror (WSAGetLastError ()).c_str ()));
			return 0;
		} else if (r == 0) {
			SendMessage (args->hwnd, qm_master_error, args->id, reinterpret_cast<LPARAM> (L"short recv"));
			return 0;
		}
		std::copy (buf.begin (), buf.begin () + r, std::back_inserter (data));
		SendMessage (args->hwnd, qm_master_progress, args->id, data.size ());

		int len = data.size ();
		unsigned char* endp = enctypex_decoder (reinterpret_cast<unsigned char*> (const_cast<char*> ("Xn221z")), &master_validate[0], &data[0], &len, &enctypex_data);
		assert (endp);
		if (endp && enctypex_decoder_convert_to_ipport (endp, data.size () - (reinterpret_cast<unsigned char*> (endp) - &data[0]), nullptr, nullptr, 0, 0)) {
			break;
		}
	}
	shutdown (socket, SD_RECEIVE); // XXX handle errors

	static_assert (sizeof (server_endpoint) == 6, "server_endpoint is a weird size");
	{
		std::vector<server_endpoint> decoded_data (data.size () / 5); // XXX size seems like a bit of a guess!
		int len = enctypex_decoder_convert_to_ipport (&data[0] + enctypex_data.start, data.size () - enctypex_data.start, reinterpret_cast<unsigned char*> (decoded_data.data ()), nullptr, 0, 0);
		assert (len >= 0); // XXX handle (see gsmyfunc.h line 715)
		for (auto ep: decoded_data)
			SendMessage (args->hwnd, qm_master_found, args->id, reinterpret_cast<LPARAM> (&ep));
	}

	SendMessage (args->hwnd, qm_master_complete, args->id, 0);
	return 0;
}

// vim: ts=4 sts=4 sw=4 noet
