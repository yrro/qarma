#include <sstream>

#include "qsocket.hpp"

#include <windows.h>

#include <commctrl.h>

#include "explain.hpp"
#include "main_window.hpp"
#include "master_proto.hpp"

namespace {
	const wchar_t main_window_class[] = L"{078be6e6-0d3a-42f4-b21a-1d92c1c41c9e}";
}

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
	wcex.hbrBackground = GetSysColorBrush (COLOR_3DFACE);
	wcex.lpszClassName = main_window_class;
	if (!RegisterClassEx (&wcex)) {
		explain (L"RegisterClassEx failed");
		return 1;
	}

	window_data wd;
	HWND hWnd = CreateWindow (main_window_class, L"Qarma",
		WS_OVERLAPPEDWINDOW /*WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX */,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 720,
		0, 0, hInstance, &wd);
	if (!hWnd) {
		explain (L"CreateWindow failed");
		return 1;
	}

	ShowWindow (hWnd, nCmdShow);
	UpdateWindow (hWnd);

	while (true) {
		DWORD n = 0;
		DWORD r = MsgWaitForMultipleObjects (0, nullptr, false, INFINITE, QS_ALLINPUT);
		if (r == WAIT_FAILED) {
			explain (L"MsgWaitForMultipleObjects failed");
			return 1;
		} else if (r == WAIT_OBJECT_0 + n) {
			MSG msg;
			while (PeekMessage (&msg, nullptr, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT)
					return msg.wParam;

				TranslateMessage (&msg);
				DispatchMessage (&msg);
			}
		} else {
			std::wostringstream ss;
			ss << L"MsgWaitForMultipleObjects returned an unexpected value: " << r;
			MessageBox (hWnd, ss.str ().c_str (), L"Qarma", 0);
		}
	}
}

// vim: ts=4 sts=4 sw=4 noet
