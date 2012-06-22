#ifndef QARMA_QUERYMANAGER_HPP
#define QARMA_QUERYMANAGER_HPP

#include <functional>
#include <memory>
#include <set>

#include <windows.h>

#include "server_common.hpp"

struct server_info {};

class querymanager {
	std::unique_ptr<HWND__, decltype(&DestroyWindow)> hwnd;
	static LRESULT WINAPI wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	std::set<server_endpoint> servers;

	void timer ();

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
