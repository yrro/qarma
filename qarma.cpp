#include "qsocket.hpp"

#include <windows.h>

#include <commctrl.h>

#include "explain.hpp"
#include "main_window.hpp"
#include "master_proto.hpp"

int WINAPI wWinMain (HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int nCmdShow) {
	INITCOMMONCONTROLSEX icc = INITCOMMONCONTROLSEX ();
	icc.dwSize = sizeof icc;
	icc.dwICC = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS;
	InitCommonControlsEx (&icc);

	winsock_wrapper winsock;
	if (winsock.error ()) {
		explain (L"WSAStartup failed", winsock.error ());
		return 1;
	}

	WNDCLASSEX wcex = WNDCLASSEX ();
	wcex.cbSize = sizeof wcex;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = main_window_wndproc;
	wcex.hInstance = hInstance;
	//wcex.hIcon = something;
	wcex.hCursor = LoadCursor (nullptr, IDC_ARROW);
	wcex.hbrBackground = reinterpret_cast<HBRUSH> (1+COLOR_BTNFACE);
	wcex.lpszClassName = main_window_class;
	if (!RegisterClassEx (&wcex)) {
		explain (L"RegisterClassEx failed");
		return 1;
	}

	HWND hWnd = CreateWindow (main_window_class, L"Qarma",
		WS_OVERLAPPEDWINDOW /*WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX */,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 720,
		0, 0, hInstance, nullptr);
	if (!hWnd) {
		explain (L"CreateWindow failed");
		return 1;
	}

	ShowWindow (hWnd, nCmdShow);
	UpdateWindow (hWnd);

	BOOL r;
	MSG msg;
	while ((r = GetMessage (&msg, nullptr, 0, 0)) > 0) {
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	if (r == -1) {
		explain (L"GetMessage failed");
		return 1;
	}
	return msg.wParam;
}

// vim: ts=4 sts=4 sw=4 noet
