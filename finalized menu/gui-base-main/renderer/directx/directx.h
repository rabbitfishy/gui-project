#pragma once
#include "../include.h"
#include <string>

class environment_directx
{
public:
	bool setup(HWND handle_window);
	void restore();
	void reset();
	void handle_screen(LPARAM lparam);
	bool render_start();
	void render_end();

	IDirect3DDevice9* handle()
	{
		return this->device;
	}

private:
	IDirect3D9* d3d = nullptr;
	IDirect3DDevice9* device = nullptr;
	D3DPRESENT_PARAMETERS present_parameter = { 0 };
};

extern environment_directx* directx;