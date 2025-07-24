#include "SystemClass.h"

#include "Application.h"

#include "ImGui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND SystemClass::m_hwnd = 0;
DirectX::XMFLOAT2 SystemClass::m_MouseDelta = { 0.f, 0.f };
bool SystemClass::ms_bShouldProcessMouse = true;
POINT SystemClass::ms_Center;

bool SystemClass::Initialise()
{
	int ScreenWidth, ScreenHeight;
	bool Result;

	ScreenWidth = 0;
	ScreenHeight = 0;

	InitialiseWindows(ScreenWidth, ScreenHeight);

	InputClass::GetSingletonPtr();

	Result = Application::GetSingletonPtr()->Initialise(ScreenWidth, ScreenHeight, m_hwnd);
	if (!Result)
	{
		return false;
	}

	return true;
}

void SystemClass::Shutdown()
{
	Application::GetSingletonPtr()->Shutdown();

	ImGui_ImplWin32_Shutdown();

	ShutdownWindows();
}

void SystemClass::Run()
{
	MSG msg;
	bool ShouldClose, Result;

	ZeroMemory(&msg, sizeof(MSG));
	SetCursorPos(ms_Center.x, ms_Center.y);

	ShouldClose = false;
	while (!ShouldClose)
	{
		InputClass::GetSingletonPtr()->SetMouseWheelDelta(0);
		
		// handle the windows messages
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// if windows signals to end the application then exit out
		if (msg.message == WM_QUIT)
		{
			ShouldClose = true;
		}
		else
		{
			Result = Frame();
			if (!Result)
			{
				ShouldClose = true;
			}
		}
	}
}

bool SystemClass::Frame()
{
	bool Result;

	if (InputClass::GetSingletonPtr()->IsKeyDown(VK_ESCAPE))
	{
		return false;
	}

	Result = Application::GetSingletonPtr()->Frame();
	if (!Result)
	{
		return false;
	}

	return true;
}

LRESULT SystemClass::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, umsg, wparam, lparam))
	{
		return true;
	}
	
	switch (umsg)
	{
		case WM_KEYDOWN:
		{
			InputClass::GetSingletonPtr()->KeyDown((unsigned int)wparam);
			return 0;
		}
		case WM_KEYUP:
		{
			InputClass::GetSingletonPtr()->KeyUp((unsigned int)wparam);
			return 0;
		}
		case WM_MOUSEWHEEL:
		{
			InputClass::GetSingletonPtr()->SetMouseWheelDelta(GET_WHEEL_DELTA_WPARAM(wparam));
			return 0;
		}
		default:
		{
			return DefWindowProc(hwnd, umsg, wparam, lparam);
		}
	}
}

void SystemClass::InitialiseWindows(int& ScreenWidth, int& ScreenHeight)
{
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;

	ApplicationHandle = this;

	m_hInstance = GetModuleHandle(NULL);

	m_ApplicationName = L"DirectX 11 Renderer";

	// setup the windows class with default settings
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = m_ApplicationName;
	wc.cbSize = sizeof(WNDCLASSEX);

	// register the window class
	RegisterClassEx(&wc);

	ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	// setup the screen settings depending on whether it is running in full screen or in windowed mode
	if (FULL_SCREEN)
	{
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = (unsigned long)ScreenWidth;
		dmScreenSettings.dmPelsHeight = (unsigned long)ScreenHeight;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		posX = posY = 0;
	}
	else
	{
		ScreenWidth = 1920;
		ScreenHeight = 1080;

		posX = (GetSystemMetrics(SM_CXSCREEN) - ScreenWidth) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - ScreenHeight) / 2;
	}

	m_hwnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		m_ApplicationName,
		m_ApplicationName,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
		posX, posY,
		ScreenWidth, ScreenHeight,
		NULL,
		NULL,
		m_hInstance,
		NULL
	);

	ShowWindow(m_hwnd, SW_SHOW);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	ImGui_ImplWin32_Init(m_hwnd);

	ShowCursor(false);

	RECT rect;
	GetClientRect(m_hwnd, &rect);
	ms_Center = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
	ClientToScreen(m_hwnd, &ms_Center);
	SetCursorPos(ms_Center.x, ms_Center.y);
}

void SystemClass::ShutdownWindows()
{
	ShowCursor(true);
	ClipCursor(NULL);

	if (FULL_SCREEN)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	DestroyWindow(m_hwnd);
	m_hwnd = NULL;

	UnregisterClass(m_ApplicationName, m_hInstance);
	m_hInstance = NULL;

	ApplicationHandle = nullptr;
}

void SystemClass::ConfineCursorToWindow()
{
	RECT rect;
	GetClientRect(m_hwnd, &rect);
	POINT ul = { rect.left, rect.top };
	POINT lr = { rect.right, rect.bottom };

	ClientToScreen(m_hwnd, &ul);
	ClientToScreen(m_hwnd, &lr);

	RECT clipRect = { ul.x, ul.y, lr.x, lr.y };
	ClipCursor(&clipRect);
	ShowCursor(FALSE);
}

void SystemClass::ProcessMouseMovement()
{
	POINT currentMousePos;
	GetCursorPos(&currentMousePos);

	RECT rect;
	GetClientRect(m_hwnd, &rect);
	ms_Center = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
	ClientToScreen(m_hwnd, &ms_Center);

	m_MouseDelta = { (float)(currentMousePos.x - ms_Center.x), (float)(currentMousePos.y - ms_Center.y) };

	SetCursorPos(ms_Center.x, ms_Center.y);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
		case WM_MOUSEMOVE:
			if (SystemClass::ms_bShouldProcessMouse)
			{
				SystemClass::ProcessMouseMovement();
			}
			return 0;
		case WM_ACTIVATE:
			if (wParam == WA_INACTIVE)
			{
				ClipCursor(NULL);
				ShowCursor(TRUE);
			}
			else {
				SystemClass::ConfineCursorToWindow();
			}
			return 0;
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}
		default:
		{
			return ApplicationHandle->MessageHandler(hWnd, uMessage, wParam, lParam);
		}
	}
}
