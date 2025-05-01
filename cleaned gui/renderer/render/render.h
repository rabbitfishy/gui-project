#pragma once
#include <vector>
#include "../include.h"
#include "font.h"

enum gradient_direction : bool
{
	vertical	= false,
	horizontal	= true
};

struct render_font
{
	environment_font segoe_ui		= environment_font("Segoe UI", 9, FW_NORMAL);
	environment_font segoe_ui_bold	= environment_font("Segoe UI", 39, FW_BOLD);
};

extern render_font* fonts;

class environment_render
{
public:
	void setup(IDirect3DDevice9* handle_device);
	void restore();

	void lost_device();
	void reset_device();

	void set_state();

public:
	void line(int x, int y, int w, int h, color color);
	void filled_rect(int x, int y, int w, int h, color color);
	void outlined_rect(int x, int y, int w, int h, color color);
	void gradient(int x, int y, int w, int h, color first, color second, gradient_direction direction = horizontal);

public:
	void set_viewport(D3DVIEWPORT9 viewport_handle);
	D3DVIEWPORT9 handle();

	const void start_clip(const rect area);
	const void end_clip();

	dimension screen;

private:
	void setup_screen();

private:
	std::vector<environment_font*>	font;
	IDirect3DDevice9*				device = nullptr;
	D3DVIEWPORT9					old_viewport;
};

extern environment_render* render;