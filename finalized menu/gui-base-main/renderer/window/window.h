#pragma once
#include <Windows.h>
#include "../directx/directx.h"

class environment_window
{
public:
	bool setup(const char* class_name, const char* window_name, int width, int height);
	void restore();
	void display();
	bool run();

	HWND handle()
	{
		return this->window_handle;
	}

private:
	WNDCLASSEX window_class = { 0 };
	HWND window_handle = nullptr;
};

extern environment_window* window;