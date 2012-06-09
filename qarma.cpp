#include <memory>
#include <sstream>

#define STRICT
#include <windows.h>

#include <commctrl.h>

static const int control_margin = 10;

static const wchar_t main_window_class[] = L"qarma.main";
static const wchar_t main_window_title[] = L"Arma 2 player hunter";
static const int idc_main_master_load = 0;

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

class SOCKET_wrapper {
	SOCKET s;
	int _error;
public:
	SOCKET_wrapper (): s (INVALID_SOCKET), _error (0) {}

	SOCKET_wrapper (int af, int type, int protocol): s (socket (af, type, protocol)), _error (s == INVALID_SOCKET ? WSAGetLastError () : 0) {}

	SOCKET_wrapper& operator= (SOCKET_wrapper&& other) {
		if (this != &other) {
			if (s != INVALID_SOCKET)
				closesocket (s);

			s = other.s;
			_error = other._error;

			other.s = INVALID_SOCKET;
			other._error = 0;
		}
	}

	SOCKET_wrapper (SOCKET_wrapper&& other): s (INVALID_SOCKET) {
		*this = std::move (other);
	}

	SOCKET_wrapper& operator= (const SOCKET_wrapper&) = delete;
	SOCKET_wrapper (const SOCKET_wrapper&) = delete;

	~SOCKET_wrapper () {
		if (s != INVALID_SOCKET)
			closesocket (s);
	}

	operator SOCKET () const {
		return s;
	}

	int error () const {
		return _error;
	}
};

struct window_data {
	NONCLIENTMETRICS metrics;
	std::unique_ptr<HFONT__, decltype (&DeleteObject)> message_font;
	SOCKET_wrapper master_socket;

	window_data (): message_font (0, DeleteObject) {}
};

LRESULT CALLBACK wndproc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	window_data* wd;
	if (uMsg != WM_CREATE) {
		wd = reinterpret_cast<window_data*> (GetWindowLongPtr (hWnd, GWLP_USERDATA));
	}

	// used by WM_CREATE
	HWND b;

	switch (uMsg) {
	case WM_CREATE:
		wd = reinterpret_cast<window_data*> (reinterpret_cast<CREATESTRUCT*> (lParam)->lpCreateParams);
		SetWindowLongPtr (hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (wd));

		b = CreateWindow (WC_BUTTON, L"Get more servers",
			WS_CHILD | WS_VISIBLE,
			control_margin, control_margin, 200, 24,
			hWnd, reinterpret_cast<HMENU> (idc_main_master_load),
			0, 0);
		if (b) {
			SendMessage (b, WM_SETFONT, reinterpret_cast<WPARAM> (wd->message_font.get ()), true);
		} else {
			explain (L"CreateWindow failed");
		}

		break;

	case WM_DESTROY:
		PostQuitMessage (0);
		break;

	case WM_COMMAND:
		switch (HIWORD (wParam)) {
		case BN_CLICKED:
			switch (LOWORD (wParam)) {
			case idc_main_master_load:
				wd->master_socket = SOCKET_wrapper (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				if (wd->master_socket == INVALID_SOCKET) {
					explain (L"socket failed", wd->master_socket.error ());
				}
			}
			break;
		}

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	return 0;
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

	window_data wd;
	wd.metrics = NONCLIENTMETRICS ();
	wd.metrics.cbSize = sizeof wd.metrics;
	if (!SystemParametersInfo (SPI_GETNONCLIENTMETRICS, sizeof wd.metrics, &wd.metrics, 0)) {
		explain (L"SystemParametersInfo failed");
		return 1;
	}
	wd.message_font.reset (CreateFontIndirect (&wd.metrics.lfMessageFont));

	HWND hWnd = CreateWindow (main_window_class, main_window_title,
		WS_OVERLAPPEDWINDOW /*WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX */,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 720,
		0, 0, hInstance, &wd);
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
