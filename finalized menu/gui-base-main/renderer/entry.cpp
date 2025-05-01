#include "include.h"

int WINAPI main(HINSTANCE handle, HINSTANCE prev_handle, LPSTR cmd_line, int cmd_show)
{
	// valid window check.
	if (!window->setup("renderer environment", "renderer", 1920, 1080))
	{
		MessageBox(nullptr, "failed to create window!", "renderer", MB_OK | MB_ICONERROR);
		return TRUE;
	}

	// valid directx creation check.
	if (!directx->setup(window->handle()))
	{
		MessageBox(nullptr, "failed to create directx device!", "renderer", MB_OK | MB_ICONERROR);
		return TRUE;
	}

	// install renderer.
	render->setup(directx->handle());

	// install gui input handle.
	gui::input->setup(window->handle());

	// install gui framework.
	menu->setup();

	// handle environment window screen.
	window->display();

	// handle rendering.
	while (window->run())
	{
		// start drawing.
		if (directx->render_start())
		{
			// add render functions here.
			gui::instance->think();
			gui::instance->draw();

			// end drawing.
			directx->render_end();
		}
	}

	// clean up.
	render->restore();
	directx->restore();
	window->restore();

	return FALSE;
}