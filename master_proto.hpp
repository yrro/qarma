#ifndef QARMA_MASTER_PROTO_HPP
#define QARMA_MASTER_PROTO_HPP

#include <windows.h>

extern const UINT qm_master_begin; // WPARAM: id, LPARAM: undefined
extern const UINT qm_master_error; // WPARAM: id, LPARAM: error message (const std::wstring*)
extern const UINT qm_master_progress; // WPARAM: id, LPARAM: number of bytes received (unsigned int)
extern const UINT qm_master_found; // WPARAM: id, LPARAM: server data (const server_endpoint*)
extern const UINT qm_master_complete; // WPARAM: id, LPARAM: undefined

struct master_proto_args {
	unsigned int id; // for user reference
	HWND hwnd; // notifications will be sent to this window
};

extern __stdcall unsigned int master_proto (void* args);

#endif

// vim: ts=4 sts=4 sw=4 noet
