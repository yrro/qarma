#include <sstream>

#define STRICT
#include <windows.h>

static const wchar_t main_window_class[] = L"qarma.main";
static const wchar_t main_window_title[] = L"Arma 2 player hunter";

LRESULT CALLBACK wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:
		PostQuitMessage (0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	return 0;
}

static void explain (const wchar_t* msg, DWORD e = GetLastError ()) {
	std::wostringstream ss;
	ss << msg << ".\n\n" << e << ": ";

	wchar_t* errmsg = 0;
	if (FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		0, e, 0, reinterpret_cast<LPWSTR> (&errmsg), 0, 0)) {
		ss << errmsg;
	} else {
		ss << "[FormatMessage failure: " << GetLastError ();
	}

	MessageBox (0, ss.str ().c_str (), main_window_title, 0);

	if (errmsg)
		LocalFree (reinterpret_cast<LPVOID> (errmsg));
}

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

int WINAPI wWinMain (HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int nCmdShow) {
	winsock_wrapper winsock;
	if (winsock.error ()) {
		explain (L"WSAStartup failed", winsock.error ());
		return 1;
	}

	WNDCLASSEX wcex = WNDCLASSEX ();
	wcex.cbSize = sizeof wcex;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = wndproc;
	wcex.hInstance = hInstance;
	//wcex.hIcon = something;
	wcex.hCursor = LoadCursor (0, IDC_ARROW);
	wcex.hbrBackground = reinterpret_cast<HBRUSH> (1+COLOR_WINDOW);
	wcex.lpszClassName = main_window_class;
	if (!RegisterClassEx (&wcex)) {
		explain (L"RegisterClassEx failed");
		return 1;
	}

	HWND hWnd = CreateWindow (main_window_class, main_window_title,
		WS_OVERLAPPEDWINDOW /*WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX */,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 720,
		0, 0, hInstance, 0);
	if (!hWnd) {
		explain (L"CreateWindow failed");
		return 1;
	}

	ShowWindow (hWnd, nCmdShow);
	UpdateWindow (hWnd);

	BOOL r;
	MSG msg;
	while (r = GetMessage (&msg, 0, 0, 0) > 0) {
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	if (r == -1) {
		explain (L"GetMessage failed");
		return 1;
	}
	return msg.wParam;
}
