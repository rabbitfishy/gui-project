#include "directx.h"

environment_directx* directx = new environment_directx;

bool environment_directx::setup(HWND handle_window)
{
	this->d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!this->d3d)
		return false;

	std::memset(&this->present_parameter, 0, sizeof(this->present_parameter));
	this->present_parameter.Windowed				= TRUE;
	this->present_parameter.SwapEffect				= D3DSWAPEFFECT_DISCARD;
	this->present_parameter.BackBufferFormat		= D3DFMT_UNKNOWN;
	this->present_parameter.EnableAutoDepthStencil	= TRUE;
	this->present_parameter.AutoDepthStencilFormat	= D3DFMT_D16;
	this->present_parameter.PresentationInterval	= D3DPRESENT_INTERVAL_IMMEDIATE;

	if (FAILED(this->d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, handle_window, D3DCREATE_HARDWARE_VERTEXPROCESSING, &this->present_parameter, &this->device)))
		return false;

	return true;
}

void environment_directx::restore()
{
	SAFE_RELEASE(this->device);
	SAFE_RELEASE(this->d3d);
}

void environment_directx::reset()
{
	render->lost_device();
	this->device->Reset(&this->present_parameter);
	render->reset_device();
}

void environment_directx::handle_screen(LPARAM lparam)
{
	this->present_parameter.BackBufferWidth		= (UINT)LOWORD(lparam);
	this->present_parameter.BackBufferHeight	= (UINT)HIWORD(lparam);
	this->reset();
}

bool environment_directx::render_start()
{
	// environment window background.
	this->device->Clear(0, nullptr, D3DCLEAR_TARGET, color(40, 40, 40).argb(), 1.f, 0);

	if (FAILED(this->device->BeginScene()))
		return false;

	// set render state.
	render->set_state();

	return true;
}

void environment_directx::render_end()
{
	this->device->EndScene();

	HRESULT handle_result = this->device->Present(nullptr, nullptr, nullptr, nullptr);

	if (handle_result == D3DERR_DEVICELOST && this->device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		this->reset();
}