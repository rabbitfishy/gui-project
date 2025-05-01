#include "render.h"

environment_render* render	= new environment_render;
render_font* fonts			= new render_font;

void environment_render::setup(IDirect3DDevice9* handle_device)
{
	this->device = handle_device;

	// setup viewport.
	this->handle();

	// get our screen size.
	this->setup_screen();

	// create our fonts.
	font.push_back(&fonts->segoe_ui);
	font.push_back(&fonts->segoe_ui_bold);

	// setup our fonts.
	for (auto f : this->font)
	{
		f->setup_device_objects(this->device);
		f->restore_device_objects();
	}
}

void environment_render::restore()
{
	// destroy font.
	for (auto f : this->font)
	{
		f->invalidate_device_objects();
		f->delete_device_objects();
		SAFE_DELETE(f);
	}
}

void environment_render::lost_device()
{
	// destroy font if device not located.
	for (auto f : this->font)
	{
		f->invalidate_device_objects();
		f->delete_device_objects();
	}
}

void environment_render::reset_device()
{
	// re-setup our viewport.
	this->handle();

	// re-setup our fonts.
	for (auto f : this->font)
	{
		f->setup_device_objects(this->device);
		f->restore_device_objects();
	}
}

void environment_render::line(int x, int y, int w, int h, color color)
{
	std::vector<vertex> vertices = { };

	vertices.emplace_back(vertex({ float(x), float(y) }, { 0.f, 1.f }, color.argb()));
	vertices.emplace_back(vertex({ float(w), float(h) }, { 0.f, 1.f }, color.argb()));

	this->device->DrawPrimitiveUP(D3DPT_LINELIST, 1, vertices.data(), sizeof(vertex));

	vertices.clear();
}

void environment_render::filled_rect(int x, int y, int w, int h, color color)
{
	std::vector<vertex> vertices = { };

	vertices.emplace_back(vertex({ x - 0.5f, y - 0.5f }, { 0.f, 1.f }, color.argb()));
	vertices.emplace_back(vertex({ x + w - 0.5f, y - 0.5f }, { 0.f, 1.f }, color.argb()));
	vertices.emplace_back(vertex({ x - 0.5f, y + h - 0.5f }, { 0.f, 1.f }, color.argb()));
	vertices.emplace_back(vertex({ x + w - 0.5f, y + h - 0.5f }, { 0.f, 1.f }, color.argb()));

	this->device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices.data(), sizeof(vertex));

	vertices.clear();
}

void environment_render::outlined_rect(int x, int y, int w, int h, color color)
{
	// idfk why this shit has a missing cornor if i do it the normal way, so i'm using this method.
	// credit: https://github.com/otvv/fgui/blob/a2c9b8c90277717864cea1659c618fabdc0d0ff9/examples/d3d9/renderer.hpp#L66C2-L66C2
	this->filled_rect(x, y, w, 1, color);
	this->filled_rect(x, y, 1, h, color);
	this->filled_rect(x + w - 1, y, 1, h, color);
	this->filled_rect(x, y + h - 1, w, 1, color);
}

void environment_render::gradient(int x, int y, int w, int h, color first, color second, gradient_direction direction)
{
	color colour[4] = { };

	switch (direction)
	{
	case gradient_direction::vertical:
		colour[0] = first;
		colour[1] = first;
		colour[2] = second;
		colour[3] = second;
		break;

	case gradient_direction::horizontal:
		colour[0] = first;
		colour[1] = second;
		colour[2] = first;
		colour[3] = second;
		break;

	default:
		colour[0] = color(255, 255, 255);
		colour[3] = colour[2] = colour[1] = colour[0];
		break;
	}

	std::vector<vertex> vertices = { };

	vertices.emplace_back(vertex({ x - 0.5f, y - 0.5f }, { 0.f, 1.f }, colour[0].argb()));
	vertices.emplace_back(vertex({ x + w - 0.5f, y - 0.5f }, { 0.f, 1.f }, colour[1].argb()));
	vertices.emplace_back(vertex({ x - 0.5f, y + h - 0.5f }, { 0.f, 1.f }, colour[2].argb()));
	vertices.emplace_back(vertex({ x + w - 0.5f, y + h - 0.5f }, { 0.f, 1.f }, colour[3].argb()));

	this->device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices.data(), sizeof(vertex));

	vertices.clear();
}

void environment_render::set_viewport(D3DVIEWPORT9 viewport_handle)
{
	if (!this->device)
		return;

	this->device->SetViewport(&viewport_handle);
}

D3DVIEWPORT9 environment_render::handle()
{
	D3DVIEWPORT9 viewport_handle;
	this->device->GetViewport(&viewport_handle);
	return viewport_handle;
}

const void environment_render::start_clip(const rect area)
{
	// save the original viewport to use it later.
	this->old_viewport		= this->handle();
	D3DVIEWPORT9 handle		= { area.x, area.y, area.w, area.h, 0.f, 1.f };
	this->set_viewport(handle);
}

const void environment_render::end_clip()
{
	// reset our clipping.
	this->set_viewport(this->old_viewport);
}

void environment_render::setup_screen()
{
	D3DVIEWPORT9 view_handle	= this->handle();
	this->screen.w				= view_handle.Width;
	this->screen.h				= view_handle.Height;
}

void environment_render::set_state()
{
	this->device->SetVertexShader(nullptr);
	this->device->SetPixelShader(nullptr);
	this->device->SetTexture(NULL, nullptr);

	this->device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	this->device->SetRenderState(D3DRS_LIGHTING, FALSE);
	this->device->SetRenderState(D3DRS_FOGENABLE, FALSE);
	this->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	this->device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	this->device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	this->device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
	this->device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	this->device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

	this->device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
	this->device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	this->device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	this->device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	this->device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
	this->device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	this->device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_INVDESTALPHA);
	this->device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	this->device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE);

	this->device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
	this->device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
}
