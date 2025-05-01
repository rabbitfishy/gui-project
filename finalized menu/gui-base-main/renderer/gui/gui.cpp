#include "gui.h"

using namespace gui;

text_translate* translate = new text_translate;
gui_input* gui::input = new gui_input;

void gui_input::setup(HWND handle)
{
	this->handle = handle;
}

void gui_input::poll_input()
{
	std::memcpy(this->old_key_state, this->key_state, sizeof(this->old_key_state));

	// check if input is within window.
	bool in_window = GetForegroundWindow() != this->handle;

	for (int i = 0; i < max_key_state; i++)
		this->key_state[i] = in_window ? false : (GetAsyncKeyState(i) & 0xFFFF);
}

bool gui_input::process_mouse(HWND handle, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
	case WM_MOUSEMOVE:
		this->mouse = { LOWORD(lparam), HIWORD(lparam) };
		return true;

	case WM_MOUSEWHEEL:
		this->set_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA);
		return true;
	}

	return false;
}

bool gui_input::key_down(const int key)
{
	return this->key_state[key] && this->old_key_state[key];
}

bool gui_input::key_pressed(const int key)
{
	return this->key_state[key] && !this->old_key_state[key];
}

bool gui_input::key_released(const int key)
{
	return !this->key_state[key] && this->old_key_state[key];
}

void gui_input::set_mouse_wheel(int mouse_wheel)
{
	this->mouse_wheel = mouse_wheel;
}

int gui_input::get_mouse_wheel()
{
	return this->mouse_wheel;
}

bool gui_input::in_bound(rect area)
{
	return this->mouse.x >= area.x && this->mouse.y >= area.y
		&& this->mouse.x <= area.x + area.w && this->mouse.y <= area.y + area.h + 1;
}

gui_event* gui::events = new gui_event;

void gui_event::set_focussed(control* focussed)
{
	this->focus = focussed;
}

control* gui_event::get_focussed()
{
	return this->focus;
}

bool gui_event::has_focus(control* focussed)
{
	if (focussed == nullptr)
		return this->focus != nullptr;

	return this->focus == focussed;
}

void gui_event::set_state(int bind)
{
	// set gui open key.
	if (input->key_pressed(bind))
		this->opened = !this->opened;
}

bool gui_event::get_state()
{
	return this->opened;
}

int gui_event::set_key(int bind)
{
	this->key = bind;
	return this->key;
}

int gui_event::get_key()
{
	return this->key;
}

gui_instance* gui::instance = new gui_instance;

gui_instance::~gui_instance()
{
	for (auto handle : this->windows)
		SAFE_DELETE(handle);
}

void gui_instance::draw()
{
	// we haven't pressed our menu key then don't draw.
	if (!events->get_state())
		return;

	std::sort(this->windows.begin(), this->windows.end(), [](const window* a, const window* b) {
		return a->last_input_time < b->last_input_time;
		});

	for (auto handle : this->windows)
	{
		// check if there are any instances of window creation.
		if (!handle)
			continue;

		handle->draw();
	}
}

void gui_instance::think()
{
	// handle gui input.
	input->poll_input();

	// set gui open key.
	events->set_state(events->get_key());

	// we haven't pressed our menu key then don't draw.
	if (!events->get_state())
		return;

	this->current_time += 0.01f;

	if (this->dragging && input->key_released(VK_LBUTTON))
		this->dragging = nullptr;

	// handle gui dragging input.
	if (this->dragging)
	{
		this->dragging->position.x = input->mouse.x - this->drag.x;
		this->dragging->position.y = input->mouse.y - this->drag.y;
	}

	for (auto main : this->windows)
	{
		// check if there are any instances of window creation.
		if (!main)
			continue;

		bool should_input = true;

		// allow us to have multiple windows drawn.
		for (auto other : this->windows)
		{
			if (!other || main == other)
				continue;

			if (other->last_input_time > main->last_input_time)
			{
				rect other_area = { other->position.x, other->position.y, other->size.w, other->size.h };

				if (input->in_bound(other_area))
				{
					should_input = false;
					break;
				}
			}
		}

		if (!should_input)
			continue;

		// allow dragging on edge only.
		int edge_size = 6;
		rect window_area = { main->position.x, main->position.y, main->size.w, main->size.h };
		bool in_edge = input->in_bound(window_area) && ((input->mouse.x > window_area.x + window_area.w - edge_size || input->mouse.x < window_area.x + edge_size) || (input->mouse.y > window_area.y + window_area.h - edge_size || input->mouse.y < window_area.y + edge_size));

		if (in_edge && input->key_down(VK_LBUTTON))
		{
			main->last_input_time = this->current_time;

			this->drag.x = input->mouse.x - window_area.x;
			this->drag.y = input->mouse.y - window_area.y;

			this->dragging = main;
		}

		main->think();
	}
}

void gui_instance::add(window* handle)
{
	if (!handle)
		return;

	this->windows.push_back(handle);
}

tab::tab(const char* title, window* child, bool has_sub)
{
	this->title		= title;
	this->has_sub	= has_sub;
	this->child		= child;
}

tab::~tab()
{
	// we have sub tab present then we draw sub tabs.
	if (this->has_sub)
	{
		for (auto sub_handle : this->sub_tabs)
			SAFE_DELETE(sub_handle);
	}
	// we don't have sub tab then do normal tab.
	else
	{
		for (auto handle : this->controls)
			SAFE_DELETE(handle);
	}
}

void tab::draw()
{
	// we have sub tab present then we draw sub tabs.
	if (this->has_sub)
	{
		point control_position = this->child->draw_position() + this->position + point(90, 0);
		rect tabs_area = { control_position.x, control_position.y - this->child->get_size().h, this->child->get_size().w - 244, this->child->get_size().h };

		if (!this->sub_tabs.empty())
		{
			rect sub_handle_area = { tabs_area.x + 15, tabs_area.y + (tabs_area.h - 70), tabs_area.w + 98, 66 };

			// sub tabs background.
			render->filled_rect(sub_handle_area.x, sub_handle_area.y + sub_handle_area.h, sub_handle_area.w, sub_handle_area.h, color(12, 12, 12));

			// sub tabs outline.
			render->outlined_rect(sub_handle_area.x, sub_handle_area.y + sub_handle_area.h, sub_handle_area.w, sub_handle_area.h, color(35, 35, 35));

			// sometimes we want empty window.
			if (!this->selected_sub)
				return;

			for (int i = 0; i < this->sub_tabs.size(); i++)
			{
				sub_tab* sub_handle = this->sub_tabs[i];
				const int sub_tab_width = (sub_handle_area.w / (int)this->sub_tabs.size());
				point sub_tabs_area = { sub_handle_area.x + (i * sub_tab_width) + (sub_tab_width / 2), sub_handle_area.y + sub_handle_area.h + (sub_handle_area.h / 2) };
				dimension text_size = fonts->segoe_ui.text_size(sub_handle->get_title());

				if (this->selected_sub == sub_handle)
				{
					// menu text -> selected -> white.
					fonts->segoe_ui.text(sub_tabs_area.x - (text_size.w / 2), sub_tabs_area.y - (text_size.h / 2), sub_handle->get_title(), color(255, 255, 255));
				}
				else
				{
					// not selected -> grey.
					fonts->segoe_ui.text(sub_tabs_area.x - (text_size.w / 2), sub_tabs_area.y - (text_size.h / 2), sub_handle->get_title(), color(92, 92, 92));
				}
			}

			// handle sub tabs rendering.
			this->selected_sub->draw();
		}
	}
	// we don't have sub tab then do normal tab.
	else
	{
		for (auto handle : this->controls)
			handle->draw();
	}
}

void tab::think()
{
	// we have sub tab present then we draw sub tabs.
	if (this->has_sub)
	{
		point control_position = this->child->draw_position() + this->position + point(90, 0);
		rect tabs_area = { control_position.x, control_position.y - this->child->get_size().h, this->child->get_size().w - 238, this->child->get_size().h };
		rect sub_handle_area = { tabs_area.x + 15, tabs_area.y + (tabs_area.h - 70), tabs_area.w + 100, 66 };

		// sometimes we want empty window.
		if (!this->selected_sub)
			return;

		if (!events->get_focussed())
		{
			for (int i = 0; i < this->sub_tabs.size(); i++)
			{
				sub_tab* sub_handle = this->sub_tabs[i];
				dimension text_size = fonts->segoe_ui.text_size(sub_handle->get_title());
				const int sub_tab_width = (sub_handle_area.w / (int)this->sub_tabs.size());
				rect sub_tabs_area = { sub_handle_area.x + (i * sub_tab_width) + (sub_tab_width / 2), sub_handle_area.y + sub_handle_area.h + (sub_handle_area.h / 2), text_size.w, text_size.h };

				// fixed: (GetAsyncKeyState(VK_LBUTTON) & 1) causes to only select 2 tabs. - certified retard took 3 days to fix.
				if (input->in_bound({ sub_tabs_area.x - (text_size.w / 2), sub_tabs_area.y - (text_size.h / 2), sub_tabs_area.w, sub_tabs_area.h }) && input->key_pressed(VK_LBUTTON))
					this->selected_sub = sub_handle;
			}
		}

		// handle tabs input.
		this->selected_sub->think();
	}
	// we don't have sub tab then do normal tab.
	else
	{
		for (auto handle : this->controls)
			handle->think();
	}
}

void tab::add(control* handle)
{
	// add our control, i.e. groupbox.
	this->controls.push_back(handle);
	float content_width = ((this->child->get_size().w - 432) * this->controls.size()) / this->controls.size();
	this->offset.x += content_width + 16;
	handle->set_position(this->offset - point(content_width + 16, 0));
}

void tab::add(sub_tab* handle)
{
	if (!handle && !this->has_sub)
		return;

	this->sub_tabs.push_back(handle);
}

void tab::set_default_sub(sub_tab* handle)
{
	if (!handle && !this->has_sub)
		return;

	this->selected_sub = handle;
}

sub_tab::sub_tab(const char* title, tab* child)
{
	this->title		= title;
	this->child		= child;
}

sub_tab::~sub_tab()
{
	for (auto handle : this->controls)
		SAFE_DELETE(handle);
}

void sub_tab::draw()
{
	for (auto handle : this->controls)
		handle->draw();
}

void sub_tab::think()
{
	for (auto handle : this->controls)
		handle->think();
}

void sub_tab::add(control* handle)
{
	// add our control, i.e. groupbox.
	this->controls.push_back(handle);
	float content_width = ((this->child->get_size().w + 284) * this->controls.size()) / this->controls.size();
	this->offset.x += content_width;
	handle->set_position(this->offset - point(content_width + 16, 0));
}

window::window(const char* title, const point& position, const dimension& size)
{
	this->title		= title;
	this->position	= position;
	this->size		= size;
}

window::~window()
{
	for (auto handle : this->tabs)
		SAFE_DELETE(handle);
}

void window::draw()
{
	rect window_area = { this->position.x, this->position.y, this->size.w, this->size.h };

	// background.
	render->filled_rect(window_area.x, window_area.y, window_area.w, window_area.h, color(20, 20, 20));

	// handle tabs.
	if (!this->tabs.empty())
	{
		rect handle_area = { window_area.x, window_area.y, 100, window_area.h };

		// tabs background.
		render->filled_rect(handle_area.x, handle_area.y + 1, handle_area.w, handle_area.h - 1, color(12, 12, 12));

		// tabs outline.
		render->line(handle_area.x + handle_area.w, handle_area.y, handle_area.x + handle_area.w, handle_area.y + handle_area.h, color(35, 35, 35));

		// sometimes we want empty window.
		if (!this->selected)
			return;

		for (int i = 0; i < this->tabs.size(); i++)
		{
			rect tabs_area = { handle_area.x, (handle_area.y + 20) + (i * (90 + (int)this->tabs.size())), handle_area.w, 90 + (int)this->tabs.size() };
			tab* handle = this->tabs[i];

			if (this->selected == handle)
			{
				// tabs background.
				render->filled_rect(tabs_area.x, tabs_area.y, tabs_area.w + 1, tabs_area.h, color(20, 20, 20));
				// top outline.
				render->line(tabs_area.x - 1, tabs_area.y + (tabs_area.h - tabs_area.h) - 1, tabs_area.x + tabs_area.w, tabs_area.y + (tabs_area.h - tabs_area.h) - 1, color(35, 35, 35));
				// bottom outline.
				render->line(tabs_area.x - 1, tabs_area.y + tabs_area.h, tabs_area.x + tabs_area.w, tabs_area.y + tabs_area.h, color(35, 35, 35));
				// menu text -> selected -> white.
				fonts->segoe_ui_bold.text(tabs_area.x + (tabs_area.w / 2) - 15, (tabs_area.y - 25) + (tabs_area.h / 2) - 15, handle->get_title(), color(255, 255, 255));
			}
			else
			{
				// not selected -> grey.
				fonts->segoe_ui_bold.text(tabs_area.x + (tabs_area.w / 2) - 15, (tabs_area.y - 25) + (tabs_area.h / 2) - 15, handle->get_title(), color(92, 92, 92));
			}
		}

		// handle tabs rendering.
		this->selected->draw();
	}

	// draw on top of other controls, if selected.
	if (events->has_focus())
		events->get_focussed()->draw();

	// skeet rgb line.
	render->gradient(window_area.x, window_area.y + 1, window_area.w / 2, 1, color(99, 160, 200), color(179, 102, 181), gradient_direction::horizontal);
	render->gradient(window_area.x + (window_area.w / 2), window_area.y + 1, window_area.w / 2, 1, color(179, 102, 181), color(230, 217, 100), gradient_direction::horizontal);

	render->gradient(window_area.x, window_area.y + 2, window_area.w / 2, 1, color(49, 79, 99), color(89, 50, 90), gradient_direction::horizontal);
	render->gradient(window_area.x + (window_area.w / 2), window_area.y + 2, window_area.w / 2, 1, color(89, 50, 90), color(114, 108, 49), gradient_direction::horizontal);

	// border.
	// 					   ìnner,  middle, outter, border
	int border_colors[6] = { 60, 35, 35, 35, 60, 0 };
	for (int i = 0; i < 6; i++)
		render->outlined_rect(window_area.x - i, window_area.y - i, window_area.w + 2 * i, window_area.h + 2 * i, color(border_colors[i], border_colors[i], border_colors[i]));
}

void window::think()
{
	rect handle_area = { this->position.x, this->position.y, 100, this->size.h };

	// sometimes we want empty window.
	if (!this->selected)
		return;

	if (!events->get_focussed())
	{
		for (int i = 0; i < this->tabs.size(); i++)
		{
			rect tabs_area = { handle_area.x, (handle_area.y + 20) + (i * (90 + (int)this->tabs.size())), handle_area.w, 90 + (int)this->tabs.size() };
			tab* handle = this->tabs[i];

			// fixed: (GetAsyncKeyState(VK_LBUTTON) & 1) causes to only select 2 tabs. - certified retard took 3 days to fix.
			if (input->in_bound(tabs_area) && input->key_pressed(VK_LBUTTON))
				this->selected = handle;
		}
	}

	// handle tabs input.
	this->selected->think();
}

void window::add(tab* handle)
{
	if (!handle)
		return;

	this->tabs.push_back(handle);
}

void window::set_default_tab(tab* handle)
{
	if (!handle)
		return;

	this->selected = handle;
}

column::column(control* child, bool in_sub)
{
	this->child		= child;
	this->in_sub	= in_sub;
}

column::~column()
{
	for (auto handle : this->controls)
		SAFE_DELETE(handle);
}

void column::draw()
{
	for (auto handle : this->controls)
		handle->draw();
}

void column::think()
{
	for (auto handle : this->controls)
		handle->think();
}

void column::add(control* handle)
{
	// add our control, i.e. groupbox.
	this->controls.push_back(handle);

	// check if we are in sub tab.
	if (this->in_sub)
	{
		float sub_content_height = ((handle->get_size().h - 60) * this->controls.size()) / this->controls.size();
		this->offset.y += sub_content_height + 76;
		handle->set_position(this->offset - point(0, sub_content_height + 16));
	}
	// otherwise do normal tab layout.
	else
	{
		float content_height = ((handle->get_size().h - 16) * this->controls.size()) / this->controls.size();
		this->offset.y += content_height + 32;
		handle->set_position(this->offset - point(0, content_height + 32));
	}
}

group::group(const char* title, const dimension& size, column* child)
{
	this->title		= title;
	this->size		= size;
	this->child		= child;
}

void group::draw()
{
	point control_position = this->child->draw_position() + this->position + point(105, 0);
	rect group_area = { control_position.x, control_position.y, this->size.w, this->size.h };
	float content_height = this->offset.y - 16;

	// background.
	render->filled_rect(group_area.x, group_area.y, group_area.w, group_area.h, color(12, 12, 12));

	// handle element clipping.
	render->start_clip(group_area);
	{
		// handle element rendering.
		for (auto handle : this->controls)
			if (!events->has_focus() || (events->has_focus() && !events->has_focus(handle)))
				handle->draw();

		// separator / header.
		render->filled_rect(group_area.x + 1, group_area.y, group_area.w - 2, 5, color(12, 12, 12));
		render->gradient(group_area.x + 1, group_area.y + 5, group_area.w - 2, 12, color(12, 12, 12), color(12, 12, 12, 0));

		// blur.
		if (content_height > group_area.h - 36)
			render->gradient(group_area.x + 1, group_area.y + (group_area.h - 13), group_area.w - 2, 12, color(12, 12, 12, 0), color(12, 12, 12));
	}
	render->end_clip();

	// outline.
	render->outlined_rect(group_area.x, group_area.y, group_area.w, group_area.h, color(35, 35, 35));

	// handle scroll bar rendering.
	if (content_height > group_area.h - 36)
	{
		// scrolling logic (scroll bar animation).
		int max_scroll = content_height - group_area.h + 36;
		float scroll_position = this->scroll / content_height * (group_area.h - 36) * -1; // multiply by -1 -> otherwise when scrolled the bar will go upwards not down.
		float max_scroll_position = max_scroll / content_height * (group_area.h - 36);

		// show upper arrow, scrolled -> upper arrow, not scrolled then upper arrow disappears.
		if (scroll_position)
		{
			// fixed these upper and lower arrows relative to group box height, as before it messes up when group box height is being changed.
			render->filled_rect((group_area.x + group_area.w - 6) - 5, (group_area.y + (group_area.h + 8) - (group_area.h + 4)), 1, 1, color(255, 255, 255));
			render->filled_rect((group_area.x + group_area.w - 6) - 6, (group_area.y + (group_area.h + 8) - (group_area.h + 3)), 3, 1, color(255, 255, 255));
			render->filled_rect((group_area.x + group_area.w - 6) - 7, (group_area.y + (group_area.h + 8) - (group_area.h + 2)), 5, 1, color(255, 255, 255));
		}

		// show lower arrow, not scrolled or scrolled -> lower arrow, max scrolled then lower arrow disappears.
		if (max_scroll_position > scroll_position)
		{
			render->filled_rect((group_area.x + group_area.w - 6) - 7, (group_area.y + (group_area.h - 9) + 2), 5, 1, color(255, 255, 255));
			render->filled_rect((group_area.x + group_area.w - 6) - 6, (group_area.y + (group_area.h - 9) + 3), 3, 1, color(255, 255, 255));
			render->filled_rect((group_area.x + group_area.w - 6) - 5, (group_area.y + (group_area.h - 9) + 4), 1, 1, color(255, 255, 255));
		}

		// scroll bar rendering.
		dimension scroll_size = { 5, group_area.h };

		// scroll background.
		render->filled_rect((group_area.x + group_area.w) - scroll_size.w - 1, group_area.y + 1, scroll_size.w, scroll_size.h - 2, color(25, 25, 25));

		// scroll bar.
		render->filled_rect((group_area.x + group_area.w) - scroll_size.w + 1, group_area.y + scroll_position + 2, scroll_size.w - 3, scroll_size.h - max_scroll_position - 3, color(45, 45, 45));
	}

	// black line.
	dimension text_size = fonts->segoe_ui.text_size(this->get_title());
	render->filled_rect(group_area.x + 14, group_area.y, text_size.w + 7, 1, color(12, 12, 12));

	// group box title.
	fonts->segoe_ui.text(group_area.x + 20, group_area.y - 7, this->get_title(), color(255, 255, 255));
}

void group::think()
{
	point control_position = this->child->draw_position() + this->position + point(105, 0);
	rect group_area = { control_position.x, control_position.y, this->size.w, this->size.h };
	float content_height = this->offset.y - 16;

	// handle scrolling.
	if (input->get_mouse_wheel() != 0 && input->in_bound(group_area))
	{
		if (content_height > group_area.h - 36)
		{
			if (!events->has_focus())
				this->scroll += input->get_mouse_wheel() * 12;

			input->set_mouse_wheel(0);

			// scrolling logic (actual function).
			if (this->scroll > 0)
				this->scroll = 0;
			else if (this->scroll < (group_area.h - 36) - content_height)
				this->scroll = (group_area.h - 36) - content_height;
		}
	}

	// handle element input.
	for (auto handle : this->controls)
		if ((!events->has_focus() && input->in_bound(group_area)) || (events->has_focus() && events->has_focus(handle)))
			handle->think();
}

void group::add(control* handle)
{
	// add our control, i.e. checkbox and etc...
	this->controls.push_back(handle);

	// set element position.
	this->offset.y += handle->get_size().h + 16;
	handle->set_position(this->offset - point(0, handle->get_size().h + 16));
}

label::label(control* child, const char* title, color colour)
{
	this->title		= title;
	this->colour	= colour;
	this->size		= dimension(0, 2);
	this->child		= child;
}

void label::draw()
{
	point control_position = this->child->draw_position() + this->position + point(123, 5);
	rect label_area = { control_position.x, control_position.y, this->size.w, this->size.h };

	// title.
	fonts->segoe_ui.text(label_area.x, label_area.y + (label_area.w / 2) - 7, this->get_title(), this->colour);
}

button::button(control* child, const char* title, std::function<void()> callback)
{
	this->title		= title;
	this->callback	= callback;
	this->size		= dimension(180, 16);
	this->child		= child;
}

void button::draw()
{
	point control_position = this->child->draw_position() + this->position + point(123, 0);
	rect button_area = { control_position.x, control_position.y, this->size.w, 25 };

	// button background.
	if (input->in_bound(button_area) && this->selected) // pressed -> light grey.
		render->filled_rect(button_area.x, button_area.y, button_area.w, button_area.h, color(34, 34, 34));
	else if (input->in_bound(button_area)) // hovered -> grey.
		render->filled_rect(button_area.x, button_area.y, button_area.w, button_area.h, color(32, 32, 32));
	else // not pressed -> black.
		render->filled_rect(button_area.x, button_area.y, button_area.w, button_area.h, color(30, 30, 30));

	// button outline.
	render->outlined_rect(button_area.x, button_area.y, button_area.w + 1, button_area.h + 1, color(35, 35, 35));

	// title.
	dimension text_size = fonts->segoe_ui.text_size(this->get_title());
	fonts->segoe_ui.text(button_area.x + (button_area.w / 2) - (text_size.w / 2), button_area.y + (button_area.h / 2) - 7, this->get_title(), color(255, 255, 255));
}

void button::think()
{
	point control_position = this->child->draw_position() + this->position + point(123, 0);
	rect button_area = { control_position.x, control_position.y, this->size.w, 25 };

	if (input->in_bound(button_area) && input->key_pressed(VK_LBUTTON))
	{
		events->set_focussed(this);
		this->selected = true;
	}

	if (this->selected && input->key_down(VK_LBUTTON) && this->callback)
	{
		this->callback();
		events->set_focussed(nullptr);
	}

	if (input->key_released(VK_LBUTTON))
	{
		events->set_focussed(nullptr);
		this->selected = false;
	}
}

checkbox::checkbox(control* child, const char* title, bool* value)
{
	this->title		= title;
	this->value		= value;
	this->size		= dimension(9, 2);
	this->child		= child;
}

void checkbox::draw()
{
	point control_position = this->child->draw_position() + this->position + point(105, 0);
	rect checkbox_area = { control_position.x, control_position.y, this->size.w, 9 };

	// background.
	render->filled_rect(checkbox_area.x, checkbox_area.y + 1, checkbox_area.w, checkbox_area.h, color(25, 25, 25));

	// value = true -> white.
	if (*this->value)
		render->filled_rect(checkbox_area.x, checkbox_area.y + 1, checkbox_area.w, checkbox_area.h, color(255, 255, 255));

	// outline.
	render->outlined_rect(checkbox_area.x, checkbox_area.y + 1, checkbox_area.w, checkbox_area.h, color(35, 35, 35));

	// title.
	fonts->segoe_ui.text((checkbox_area.x + 10) + checkbox_area.w, checkbox_area.y + (checkbox_area.w / 2) - 7, this->get_title(), color(255, 255, 255));
}

void checkbox::think()
{
	point control_position = this->child->draw_position() + this->position + point(105, 0);
	rect checkbox_area = { control_position.x, control_position.y, this->size.w, 9 };

	// allow click on checkbox text to enable.
	dimension text_size = fonts->segoe_ui.text_size(this->get_title());
	rect text_area = { (checkbox_area.x + 10) + checkbox_area.w, checkbox_area.y - (text_size.h / 2), text_size.w, text_size.h };

	if ((input->in_bound(checkbox_area) || input->in_bound(text_area)) && input->key_pressed(VK_LBUTTON))
		events->set_focussed(this);

	if (events->has_focus(this) && input->key_pressed(VK_LBUTTON))
		*this->value = !*this->value;

	if (events->has_focus(this) && input->key_released(VK_LBUTTON))
		events->set_focussed(nullptr);
}

slider_int::slider_int(control* child, const char* title, int* value, int min, int max, const char* suffix)
{
	this->title		= title;
	this->value		= value;
	this->min		= min;
	this->max		= max;
	this->suffix	= suffix;
	this->size		= dimension(180, 10);
	this->child		= child;
}

void slider_int::draw()
{
	point control_position = this->child->draw_position() + this->position + point(123, 10);
	rect slider_area = { control_position.x, control_position.y, this->size.w, 6 };

	// title.
	dimension text_size = fonts->segoe_ui.text_size(this->get_title());
	fonts->segoe_ui.text(slider_area.x, (slider_area.y - 2) - (text_size.h - 2), this->get_title(), color(255, 255, 255));

	// format slider value.
	std::string text_value = translate->format("%d", *this->value);

	// slider background.
	render->filled_rect(slider_area.x, slider_area.y, slider_area.w, slider_area.h, color(25, 25, 25));

	// fixed: slider bar not showing whilst sliding until it hits the max value of slider -> only applies to int min, max values.
	float max_delta = this->max - this->min;
	float value_delta = *this->value - this->min;
	float value_mod = (value_delta / max_delta) * slider_area.w;

	// slider bar.
	render->filled_rect(slider_area.x, slider_area.y, value_mod, slider_area.h, color(255, 255, 255));

	// slider outline.
	render->outlined_rect(slider_area.x, slider_area.y, slider_area.w + 1, slider_area.h + 1, color(35, 35, 35));

	// slider value w/ suffix.
	if (this->suffix)
	{
		text_value += this->suffix;
		fonts->segoe_ui.text(slider_area.x + (int)value_mod, slider_area.y, text_value.c_str(), color(255, 255, 255));
	}
	// slider value.
	else
		fonts->segoe_ui.text(slider_area.x + (int)value_mod, slider_area.y, text_value.c_str(), color(255, 255, 255));
}

void slider_int::think()
{
	point control_position = this->child->draw_position() + this->position + point(123, 10);
	rect slider_area = { control_position.x, control_position.y, this->size.w, 6 };
	float max_delta = this->max - this->min;

	if (input->in_bound(slider_area) && input->key_pressed(VK_LBUTTON))
		events->set_focussed(this);

	if (events->has_focus(this) && input->key_down(VK_LBUTTON))
	{
		// fixed: ((inputs->mouse.x - groupbox->area.x) / groupbox->area.w * max_value) -> unable to accept negative numbers and unable to slide backwards, e.g. in range of -int to int.
		*this->value = std::clamp<float>(float(this->min + max_delta * (input->mouse.x - slider_area.x) / slider_area.w), (float)this->min, (float)this->max);
	}

	if (events->has_focus(this) && input->key_released(VK_LBUTTON))
		events->set_focussed(nullptr);
}

slider_float::slider_float(control* child, const char* title, float* value, float min, float max, const char* suffix)
{
	this->title		= title;
	this->value		= value;
	this->min		= min;
	this->max		= max;
	this->suffix	= suffix;
	this->size		= dimension(180, 10);
	this->child		= child;
}

void slider_float::draw()
{
	point control_position = this->child->draw_position() + this->position + point(123, 10);
	rect slider_area = { control_position.x, control_position.y, this->size.w, 6 };

	// title.
	dimension text_size = fonts->segoe_ui.text_size(this->get_title());
	fonts->segoe_ui.text(slider_area.x, (slider_area.y - 2) - (text_size.h - 2), this->get_title(), color(255, 255, 255));

	// format slider value.
	std::string text_value = translate->format("%.1f", *this->value);

	// slider background.
	render->filled_rect(slider_area.x, slider_area.y, slider_area.w, slider_area.h, color(25, 25, 25));

	// fixed: slider bar not showing whilst sliding until it hits the max value of slider -> only applies to int min, max values.
	float max_delta = this->max - this->min;
	float value_delta = *this->value - this->min;
	float value_mod = (value_delta / max_delta) * slider_area.w;

	// slider bar.
	render->filled_rect(slider_area.x, slider_area.y, value_mod, slider_area.h, color(255, 255, 255));

	// slider outline.
	render->outlined_rect(slider_area.x, slider_area.y, slider_area.w + 1, slider_area.h + 1, color(35, 35, 35));

	// slider value w/ suffix.
	if (this->suffix)
	{
		text_value += this->suffix;
		fonts->segoe_ui.text(slider_area.x + (int)value_mod, slider_area.y, text_value.c_str(), color(255, 255, 255));
	}
	// slider value.
	else
		fonts->segoe_ui.text(slider_area.x + (int)value_mod, slider_area.y, text_value.c_str(), color(255, 255, 255));
}

void slider_float::think()
{
	point control_position = this->child->draw_position() + this->position + point(123, 10);
	rect slider_area = { control_position.x, control_position.y, this->size.w, 6 };
	float max_delta = this->max - this->min;

	if (input->in_bound(slider_area) && input->key_pressed(VK_LBUTTON))
		events->set_focussed(this);

	if (events->has_focus(this) && input->key_down(VK_LBUTTON))
	{
		// fixed: ((inputs->mouse.x - groupbox->area.x) / groupbox->area.w * max_value) -> unable to accept negative numbers and unable to slide backwards, e.g. in range of -float to float.
		*this->value = std::clamp<float>(this->min + max_delta * (input->mouse.x - slider_area.x) / slider_area.w, this->min, this->max);
	}

	if (events->has_focus(this) && input->key_released(VK_LBUTTON))
		events->set_focussed(nullptr);
}

combo::combo(control* child, const char* title, int* value, const std::vector<const char*>& items)
{
	this->title		= title;
	this->items		= items;
	this->value		= value;
	this->size		= dimension(180, 20);
	this->child		= child;
}

void combo::draw()
{
	point control_position = this->child->draw_position() + this->position + point(123, 10);
	rect combo_area = { control_position.x, control_position.y, this->size.w, 18 };

	// combo background.
	if (input->in_bound(combo_area) && !events->has_focus(this))
		render->filled_rect(combo_area.x, combo_area.y, combo_area.w, combo_area.h, color(34, 34, 34));
	else
		render->filled_rect(combo_area.x, combo_area.y, combo_area.w, combo_area.h, color(30, 30, 30));

	// combo outline.
	render->outlined_rect(combo_area.x, combo_area.y, combo_area.w + 1, combo_area.h + 1, color(35, 35, 35));

	// title.
	fonts->segoe_ui.text(combo_area.x, combo_area.y - (combo_area.h / 2) - 5, this->get_title(), color(255, 255, 255));

	// selected combo item(s).
	fonts->segoe_ui.text(combo_area.x + 8, combo_area.y + (combo_area.h / 2) - 8, this->items[*this->value], color(255, 255, 255));

	if (events->has_focus(this))
	{
		for (int i = 0; i < this->items.size(); i++)
		{
			rect item_area = { combo_area.x, combo_area.y + 20 + (combo_area.h * i), combo_area.w, combo_area.h };

			bool in_bound = input->in_bound(item_area);

			// selecting combo item(s) background.
			render->filled_rect(item_area.x, item_area.y, item_area.w + 1, item_area.h, in_bound ? color(25, 25, 25) : color(30, 30, 30));

			// selected -> black.
			if (*this->value == i || in_bound)
			{
				// combo items text.
				fonts->segoe_ui.text(item_area.x + 8, item_area.y + (item_area.h / 2) - 8, this->items[i], color(255, 255, 255));
			}
			// not selected -> grey.
			else
			{
				// combo items text.
				fonts->segoe_ui.text(item_area.x + 8, item_area.y + (item_area.h / 2) - 8, this->items[i], color(120, 120, 120));
			}
		}
	}

	// combo arrows.
	// open -> up arrow.
	if (events->has_focus(this))
	{
		render->filled_rect((combo_area.x + combo_area.w - 6) - 5, combo_area.y + ((combo_area.h / 2) + 3) - 4, 1, 1, color(255, 255, 255));
		render->filled_rect((combo_area.x + combo_area.w - 6) - 6, combo_area.y + ((combo_area.h / 2) + 3) - 3, 3, 1, color(255, 255, 255));
		render->filled_rect((combo_area.x + combo_area.w - 6) - 7, combo_area.y + ((combo_area.h / 2) + 3) - 2, 5, 1, color(255, 255, 255));
	}
	// close -> down arrow.
	else
	{
		render->filled_rect((combo_area.x + combo_area.w - 6) - 7, combo_area.y + ((combo_area.h / 2) - 3) + 2, 5, 1, color(255, 255, 255));
		render->filled_rect((combo_area.x + combo_area.w - 6) - 6, combo_area.y + ((combo_area.h / 2) - 3) + 3, 3, 1, color(255, 255, 255));
		render->filled_rect((combo_area.x + combo_area.w - 6) - 5, combo_area.y + ((combo_area.h / 2) - 3) + 4, 1, 1, color(255, 255, 255));
	}
}

void combo::think()
{
	point control_position = this->child->draw_position() + this->position + point(123, 10);
	rect combo_area = { control_position.x, control_position.y, this->size.w, 18 };

	// toggle dropdown opening.
	// putting 'else if' gives the issue where the combo value doesn't change.
	if (input->in_bound(combo_area) && input->key_pressed(VK_LBUTTON) && !events->has_focus(this))
		events->set_focussed(this);
	else
	{
		// closes dropdowm if we didn't select anything.
		if (input->in_bound(combo_area) && input->key_pressed(VK_LBUTTON) && events->has_focus(this))
			events->set_focussed(nullptr);
	}

	if (events->has_focus(this))
	{
		for (int i = 0; i < this->items.size(); i++)
		{
			rect item_area = { combo_area.x, combo_area.y + 20 + (combo_area.h * i), combo_area.w, combo_area.h };

			// selecting combo items in dropdown area.
			if (input->in_bound(item_area) && input->key_pressed(VK_LBUTTON))
			{
				*this->value = i;
				events->set_focussed(nullptr);
				break;
			}
		}
	}
}

multi::multi(control* child, const char* title, const std::vector<multi_data>& items)
{
	this->title		= title;
	this->items		= items;
	this->size		= dimension(180, 20);
	this->child		= child;
}

void multi::draw()
{
	point control_position = this->child->draw_position() + this->position + point(123, 10);
	rect multi_area = { control_position.x, control_position.y, this->size.w, 18 };

	// multi background.
	if (input->in_bound(multi_area) && !events->has_focus(this))
		render->filled_rect(multi_area.x, multi_area.y, multi_area.w, multi_area.h, color(34, 34, 34));
	else
		render->filled_rect(multi_area.x, multi_area.y, multi_area.w, multi_area.h, color(30, 30, 30));

	// multi outline.
	render->outlined_rect(multi_area.x, multi_area.y, multi_area.w + 1, multi_area.h + 1, color(35, 35, 35));

	// title.
	fonts->segoe_ui.text(multi_area.x, multi_area.y - (multi_area.h / 2) - 5, this->get_title(), color(255, 255, 255));

	// selected multi item(s).
	fonts->segoe_ui.text(multi_area.x + 8, multi_area.y + (multi_area.h / 2) - 8, this->construct_items(this->items).c_str(), color(255, 255, 255));

	if (events->has_focus(this))
	{
		for (int i = 0; i < this->items.size(); i++)
		{
			rect item_area = { multi_area.x, multi_area.y + 20 + (multi_area.h * i), multi_area.w, multi_area.h };

			bool in_bound = input->in_bound(item_area);

			// selecting multi item(s) background.
			render->filled_rect(item_area.x, item_area.y, item_area.w + 1, item_area.h, in_bound ? color(25, 25, 25) : color(30, 30, 30));

			// selected -> black.
			if (*this->items[i].value || in_bound)
			{
				// multi items text.
				fonts->segoe_ui.text(item_area.x + 8, item_area.y + (item_area.h / 2) - 8, this->items[i].title, color(255, 255, 255));
			}
			// not selected -> grey.
			else
			{
				// multi items text.
				fonts->segoe_ui.text(item_area.x + 8, item_area.y + (item_area.h / 2) - 8, this->items[i].title, color(120, 120, 120));
			}
		}
	}

	// multi arrows.
	// open -> up arrow.
	if (events->has_focus(this))
	{
		render->filled_rect((multi_area.x + multi_area.w - 6) - 5, multi_area.y + ((multi_area.h / 2) + 3) - 4, 1, 1, color(255, 255, 255));
		render->filled_rect((multi_area.x + multi_area.w - 6) - 6, multi_area.y + ((multi_area.h / 2) + 3) - 3, 3, 1, color(255, 255, 255));
		render->filled_rect((multi_area.x + multi_area.w - 6) - 7, multi_area.y + ((multi_area.h / 2) + 3) - 2, 5, 1, color(255, 255, 255));
	}
	// close -> down arrow.
	else
	{
		render->filled_rect((multi_area.x + multi_area.w - 6) - 7, multi_area.y + ((multi_area.h / 2) - 3) + 2, 5, 1, color(255, 255, 255));
		render->filled_rect((multi_area.x + multi_area.w - 6) - 6, multi_area.y + ((multi_area.h / 2) - 3) + 3, 3, 1, color(255, 255, 255));
		render->filled_rect((multi_area.x + multi_area.w - 6) - 5, multi_area.y + ((multi_area.h / 2) - 3) + 4, 1, 1, color(255, 255, 255));
	}
}

void multi::think()
{
	point control_position = this->child->draw_position() + this->position + point(123, 10);
	rect multi_area = { control_position.x, control_position.y, this->size.w, 18 };

	// toggle dropdown opening.
	// putting 'else if' gives the issue where the multi value doesn't change.
	if (input->in_bound(multi_area) && input->key_pressed(VK_LBUTTON) && !events->has_focus(this))
		events->set_focussed(this);
	else
	{
		// closes dropdowm if we didn't select anything.
		if (input->in_bound(multi_area) && input->key_pressed(VK_LBUTTON) && events->has_focus(this))
			events->set_focussed(nullptr);
	}

	if (events->has_focus(this))
	{
		for (int i = 0; i < this->items.size(); i++)
		{
			rect item_area = { multi_area.x, multi_area.y + 20 + (multi_area.h * i), multi_area.w, multi_area.h };

			// selecting combo items in dropdown area.
			if (input->in_bound(item_area) && input->key_pressed(VK_LBUTTON))
				*this->items[i].value = !*this->items[i].value;
		}
	}
}

keybind::keybind(control* child, const char* title, bool* value, int* key_value, int type)
{
	this->title = title;
	this->value = value;
	this->key_value = key_value;
	this->type = type;
	this->size = dimension(20, -19);
	this->child = child;
	this->handle_key_type();
}

void keybind::draw()
{
	// have to be diffent set of element area size.
	std::string bind_value = "[" + this->key_name(*this->key_value) + "]";
	dimension text_size = fonts->segoe_ui.text_size(bind_value.c_str());

	point control_position = this->child->draw_position() + this->position + point(315, -16);
	rect keybind_area = { control_position.x - (text_size.w / 2), control_position.y, this->size.w, 32 };

	// we have to know if we are picking a bind or not.
	if (this->picking)
		bind_value = "[...]";

	// bind name title.
	fonts->segoe_ui.text(keybind_area.x, keybind_area.y + (keybind_area.w / 2), bind_value.c_str(), color(255, 255, 255));

	// keybind type dropdown.
	if (events->has_focus(this) && this->context_open)
	{
		rect bind_type_area = { keybind_area.x + 5, (keybind_area.y + keybind_area.h) - 8, 60, 80 };

		// context dropdown background.
		render->filled_rect(bind_type_area.x, bind_type_area.y, bind_type_area.w, bind_type_area.h, color(30, 30, 30));

		for (int i = 0; i < this->items.size(); i++)
		{
			rect context_item = { bind_type_area.x, bind_type_area.y + (20 * i), bind_type_area.w, 20 };

			// selected -> white.
			if (this->type == i)
			{
				// item dropdown background.
				render->filled_rect(context_item.x, context_item.y, context_item.w, context_item.h, color(25, 25, 25));

				// item text.
				fonts->segoe_ui.text(context_item.x + 8, context_item.y + 2, this->items[i], color(255, 255, 255));
			}
			// not selected -> grey.
			else
			{
				// item text.
				fonts->segoe_ui.text(context_item.x + 8, context_item.y + 2, this->items[i], color(120, 120, 120));
			}
		}
	}
}

void keybind::think()
{
	std::string bind_value = "[" + this->key_name(*this->key_value) + "]";
	dimension text_size = fonts->segoe_ui.text_size(bind_value.c_str());
	bool length_exceeded = bind_value.length() >= 5;

	point control_position = this->child->draw_position() + this->position + point(315, -16);
	rect keybind_area = { control_position.x - (text_size.w / 2), control_position.y, length_exceeded ? this->size.w + (int)bind_value.length() + 16 : this->size.w, 32 };
	rect bind_type_area = { keybind_area.x + 5, (keybind_area.y + keybind_area.h) - 8, 60, 80 };

	// context dropdown logic.
	if (input->in_bound(keybind_area) && !events->has_focus(this) && !this->context_open && input->key_pressed(VK_RBUTTON) && !this->picking)
	{
		events->set_focussed(this);
		this->picking = false;
		this->context_open = true;
		return;
	}
	else
	{
		if (input->in_bound(keybind_area) && events->has_focus(this) && this->context_open && input->key_pressed(VK_LBUTTON) && !this->picking)
		{
			events->set_focussed(nullptr);
			this->picking = false;
			this->context_open = false;
			return;
		}
	}

	// context item logic.
	if (this->context_open)
	{
		for (int i = 0; i < this->items.size(); i++)
		{
			rect context_item = { bind_type_area.x, bind_type_area.y + (20 * i), bind_type_area.w, 20 };

			// selecting context items in dropdown area.
			if (input->in_bound(context_item) && input->key_pressed(VK_LBUTTON))
			{
				this->type = i;
				this->context_open = false;
				events->set_focussed(nullptr);
			}
		}
	}

	// picking bind logic.
	if (input->in_bound(keybind_area) && !events->has_focus(this) && !this->context_open && input->key_pressed(VK_LBUTTON) && !this->picking)
	{
		events->set_focussed(this);
		this->picking = true;
		return;
	}
	// keybind logic.
	else if (this->picking)
	{
		for (int i = 0; i < max_key_state; i++)
		{
			if (input->key_pressed(i))
			{
				this->picking = false;
				events->set_focussed(nullptr);

				if (i != VK_ESCAPE)
					*this->key_value = i;
				// reset bind.
				else
					*this->key_value = -1;
			}
		}
	}
}

void keybind::handle_key_type()
{
	this->type = std::clamp(this->type, 0, (int)this->items.size());

	switch (this->type)
	{
	case bind_type::none:
		*this->value = false;
		break;

	case bind_type::hold:
		*this->value = GetAsyncKeyState(*this->key_value);
		break;

	case bind_type::toggle:
		if (GetAsyncKeyState(*this->key_value))
			*this->value = !*this->value;
		break;

	case bind_type::always:
		*this->value = true;
		break;

		// set default to none.
	default:
		*this->value = false;
		break;
	}
}

color_picker::color_picker(control* child, const char* title, color* value)
{
	this->title = title;
	this->value = value;
	this->min = 0;
	this->max = 255;
	this->size = dimension(20, -19);
	this->child = child;
}

void color_picker::draw()
{
	point control_position = this->child->draw_position() + this->position + point(315, -16);
	rect color_picker_area = { control_position.x - this->size.w, control_position.y, this->size.w, 10 };

	// background.
	render->filled_rect(color_picker_area.x, color_picker_area.y, color_picker_area.w, color_picker_area.h, color(30, 30, 30));

	// preview of color picker.
	render->filled_rect(color_picker_area.x, color_picker_area.y, color_picker_area.w, color_picker_area.h, *this->value);

	// outline.
	render->outlined_rect(color_picker_area.x, color_picker_area.y, color_picker_area.w + 1, color_picker_area.h + 1, color(35, 35, 35));

	// open our color picker.
	if (this->context_open)
	{
		rect picker_area = { color_picker_area.x + 13, color_picker_area.y + 2, 180, 6 };

		// picker background.
		render->filled_rect(picker_area.x - 3, picker_area.y + 5, picker_area.w, 165, color(30, 30, 30));

		// picker outline.
		render->outlined_rect(picker_area.x - 3, picker_area.y + 5, picker_area.w + 1, 166, color(35, 35, 35));

		// text value.
		const char* p_text = "Preview";

		// color preview.
		dimension p_text_size = fonts->segoe_ui.text_size(p_text);
		fonts->segoe_ui.text(picker_area.x + 3, (picker_area.y - 2) - (p_text_size.h - 2) + 25, p_text, color(255, 255, 255));
		// preview background.
		render->filled_rect(picker_area.x + 2, picker_area.y + 25, picker_area.w - 10, 50, color(25, 25, 25));
		// color preview.
		render->filled_rect(picker_area.x + 2, picker_area.y + 25, picker_area.w - 10, 50, *this->value);
		// preview outline.
		render->outlined_rect(picker_area.x + 2, picker_area.y + 25, (picker_area.w - 10) + 1, 51, color(35, 35, 35));

		// slider bar.
		// red.
		this->color_slider("r", picker_area.x + 2, picker_area.y + 91, picker_area.w - 10, picker_area.h, color(255, 0, 0), this->value->r);

		// green.
		this->color_slider("g", picker_area.x + 2, picker_area.y + 113, picker_area.w - 10, picker_area.h, color(0, 255, 0), this->value->g);

		// blue.
		this->color_slider("b", picker_area.x + 2, picker_area.y + 135, picker_area.w - 10, picker_area.h, color(0, 0, 255), this->value->b);

		// alpha.
		this->color_slider("a", picker_area.x + 2, picker_area.y + 157, picker_area.w - 10, picker_area.h, color(255, 255, 255, this->value->a), this->value->a);
	}
}

void color_picker::think()
{
	point control_position = this->child->draw_position() + this->position + point(315, -16);
	rect color_picker_area = { control_position.x - this->size.w, control_position.y, this->size.w, 10 };

	// toggle dropdown opening.
	if (input->in_bound(color_picker_area) && !events->has_focus(this) && !this->context_open && input->key_pressed(VK_LBUTTON))
	{
		events->set_focussed(this);
		this->context_open = true;
		return;
	}
	else
	{
		// closes dropdowm if we didn't select anything.
		if (input->in_bound(color_picker_area) && events->has_focus(this) && this->context_open && input->key_pressed(VK_LBUTTON))
		{
			events->set_focussed(nullptr);
			this->context_open = false;
			return;
		}
	}

	if (this->context_open)
	{
		rect picker_area = { color_picker_area.x + 15, color_picker_area.y + 2, 170, 6 };
		float max_delta = this->max - this->min;

		// red.
		if (input->in_bound({ picker_area.x, picker_area.y + 91, picker_area.w, picker_area.h }) && input->key_down(VK_LBUTTON))
			this->value->r = std::clamp<float>(float(this->min + max_delta * (input->mouse.x - picker_area.x) / picker_area.w), (float)this->min, (float)this->max);

		// green.
		if (input->in_bound({ picker_area.x, picker_area.y + 113, picker_area.w, picker_area.h }) && input->key_down(VK_LBUTTON))
			this->value->g = std::clamp<float>(float(this->min + max_delta * (input->mouse.x - picker_area.x) / picker_area.w), (float)this->min, (float)this->max);

		// blue.
		if (input->in_bound({ picker_area.x, picker_area.y + 135, picker_area.w, picker_area.h }) && input->key_down(VK_LBUTTON))
			this->value->b = std::clamp<float>(float(this->min + max_delta * (input->mouse.x - picker_area.x) / picker_area.w), (float)this->min, (float)this->max);

		// alpha.
		if (input->in_bound({ picker_area.x, picker_area.y + 157, picker_area.w, picker_area.h }) && input->key_down(VK_LBUTTON))
			this->value->a = std::clamp<float>(float(this->min + max_delta * (input->mouse.x - picker_area.x) / picker_area.w), (float)this->min, (float)this->max);
	}
}

void color_picker::color_slider(const char* title, int x, int y, int w, int h, color value, int format_color)
{
	// format slider value.
	std::string text_value = " : " + translate->format("%d", format_color);

	// fixed: slider bar not showing whilst sliding until it hits the max value of slider -> only applies to int min, max values.
	float max_delta = this->max - this->min;
	float value_delta = format_color - this->min;
	float value_mod = (value_delta / max_delta) * w;

	// slider title.
	dimension text_size = fonts->segoe_ui.text_size(translate->upper(title).c_str());
	fonts->segoe_ui.text(x + 1, (y - 2) - (text_size.h - 2), translate->upper(title).c_str(), color(255, 255, 255));

	// slider background.
	render->filled_rect(x, y, w, h, color(25, 25, 25));

	// slider bar.
	render->filled_rect(x, y, value_mod, h, value);

	// slider outline.
	render->outlined_rect(x, y, w + 1, h + 1, color(35, 35, 35));

	// slider value.
	fonts->segoe_ui.text((x + text_size.w), (y - 2) - (text_size.h - 2), text_value.c_str(), color(255, 255, 255));
}
