#include "window.h"

environment_window* window = new environment_window;

LRESULT CALLBACK wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

bool environment_window::setup(const char* class_name, const char* window_name, int width, int height)
{
	std::memset(&this->window_class, 0, sizeof(this->window_class));
	this->window_class.cbSize			= sizeof(this->window_class);
	this->window_class.style			= CS_CLASSDC;
	this->window_class.lpfnWndProc		= wndproc;
	this->window_class.hInstance		= GetModuleHandle(nullptr);
	this->window_class.hCursor			= LoadCursor(nullptr, IDC_ARROW);
	this->window_class.lpszClassName	= class_name;
	RegisterClassEx(&this->window_class);

	this->window_handle = CreateWindow(this->window_class.lpszClassName, window_name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, this->window_class.hInstance, nullptr);

	if (!this->window_handle)
		return false;

	return true;
}

void environment_window::restore()
{
	DestroyWindow(this->window_handle);
	UnregisterClass(this->window_class.lpszClassName, this->window_class.hInstance);
}

void environment_window::display()
{
	ShowWindow(this->window_handle, SW_SHOWDEFAULT);
	UpdateWindow(this->window_handle);
}

bool environment_window::run()
{
	MSG message = { 0 };

	while (message.message != WM_QUIT)
	{
		if (PeekMessage(&message, nullptr, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
			continue;
		}

		return true;
	}

	return false;
}

LRESULT CALLBACK wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
	case WM_SIZE:
		if (wparam == SIZE_MINIMIZED)
			return FALSE;

		// calculate screen size.
		directx->handle_screen(lparam);
		return FALSE;

	case WM_SYSCOMMAND:
		if ((wparam & 0xFFF0) == SC_KEYMENU)
			return FALSE;
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return FALSE;
	}

	// add wndproc functions here.
	if (gui::input->process_mouse(hwnd, message, wparam, lparam))
		return FALSE;

	return DefWindowProc(hwnd, message, wparam, lparam);
}