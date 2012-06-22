#ifndef QARMA_QUERYMANAGER_HPP
#define QARMA_QUERYMANAGER_HPP

#include <functional>
#include <memory>
#include <map>
#include <vector>

#include <windows.h>

#include "query_proto.hpp"
#include "server_common.hpp"

class querymanager {
	std::unique_ptr<HWND__, decltype(&DestroyWindow)> hwnd;
	static LRESULT WINAPI wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	std::map<server_endpoint, server_info> servers;
	std::vector<query_proto> protos;

	void timer ();
	void queue_query (query_proto& proto, std::map<server_endpoint, server_info>::value_type& server);

public:
	querymanager ();

	querymanager& operator= (const querymanager&) = delete;
	querymanager (const querymanager&) = delete;

	void add_server (const server_endpoint& ep);
	std::size_t server_count ();

	void inform (const std::wstring& name);

	std::function<void (unsigned int ndone, unsigned int nqueued)> on_progress;
	std::function<void (const server_info&)> on_found;
};

#endif

// vim: ts=4 sts=4 sw=4 noet
