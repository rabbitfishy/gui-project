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

void gui_event::set_focussed(element* focussed)
{
	this->selected = focussed;
}

element* gui_event::get_focussed()
{
	return this->selected;
}

bool gui_event::has_focus(element* focussed)
{
	if (focussed == nullptr)
		return this->selected != nullptr;

	return this->selected == focussed;
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

	for (const auto& handle : this->windows)
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

	for (const auto& main : this->windows)
	{
		// check if there are any instances of window creation.
		if (!main)
			continue;

		bool should_input = true;

		// allow us to have multiple windows drawn.
		for (const auto& other : this->windows)
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

window::window(const char* title, const point& position, const dimension& size)
{
	this->title		= title;
	this->position	= position;
	this->size		= size;

	this->set_position(position);
	this->set_size(size);
}

window::~window()
{
	for (auto handle : this->tabs)
		SAFE_DELETE(handle);
}

void window::draw()
{
	rect window_area		= { this->position.x, this->position.y, this->size.w, this->size.h };

	// background.
	render->filled_rect(window_area.x, window_area.y, window_area.w, window_area.h, color(20, 20, 20));

	// add tabs here.
	if (!this->tabs.empty())
	{
		rect handle_area	= { window_area.x, window_area.y, 100, window_area.h };

		// tabs background.
		render->filled_rect(handle_area.x, handle_area.y + 1, handle_area.w, handle_area.h - 1, color(12, 12, 12));

		// tabs outline.
		render->line(handle_area.x + handle_area.w, handle_area.y, handle_area.x + handle_area.w, handle_area.y + handle_area.h, color(35, 35, 35));

		// sometimes we want empty window.
		if (!this->tab_selected)
			return;

		for (int i = 0; i < this->tabs.size(); i++)
		{
			rect tabs_area	= { handle_area.x, (handle_area.y + 20) + (i * (90 + (int)this->tabs.size())), handle_area.w, 90 + (int)this->tabs.size() };
			tab* handle		= this->tabs[i];

			if (this->tab_selected == handle)
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
		this->tab_selected->draw();
	}

	// bring selected elements to the top of the stack.
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
	rect handle_area	= { this->position.x, this->position.y, 100, this->size.h };

	// sometimes we want empty window.
	if (!this->tab_selected)
		return;

	// if we haven't selected any elements then do tabs input.
	if (!events->get_focussed())
	{
		for (int i = 0; i < this->tabs.size(); i++)
		{
			rect tabs_area = { handle_area.x, (handle_area.y + 20) + (i * (90 + (int)this->tabs.size())), handle_area.w, 90 + (int)this->tabs.size() };
			tab* handle = this->tabs[i];

			// fixed: (GetAsyncKeyState(VK_LBUTTON) & 1) causes to only select 2 tabs. - certified retard took 3 days to fix.
			if (input->in_bound(tabs_area) && input->key_pressed(VK_LBUTTON))
				this->tab_selected = handle;
		}
	}

	// handle tabs input.
	this->tab_selected->think();
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

	this->tab_selected = handle;
}

tab::tab(const char* title, window* parent, bool has_sub)
{
	this->title		= title;
	this->has_sub	= has_sub;
	this->parent	= parent;
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
		for (auto handle : this->elements)
			SAFE_DELETE(handle);
	}
}

void tab::draw()
{
	// we have sub tab present then we draw sub tabs.
	if (this->has_sub)
	{
		point sub_position				= this->parent->draw_position() + this->position + point(110, 24);
		rect tabs_area					= { sub_position.x + this->size.w, sub_position.y + this->size.h, this->size.w + 440, this->size.h };

		if (!this->sub_tabs.empty())
		{
			rect sub_handle_area		= { tabs_area.x + 15, tabs_area.y + (tabs_area.h - 70), tabs_area.w + 118, 66 };

			// sub tabs background.
			render->filled_rect(sub_handle_area.x, sub_handle_area.y + sub_handle_area.h, sub_handle_area.w, sub_handle_area.h, color(12, 12, 12));

			// sub tabs outline.
			render->outlined_rect(sub_handle_area.x, sub_handle_area.y + sub_handle_area.h, sub_handle_area.w, sub_handle_area.h, color(35, 35, 35));

			// sometimes we want empty window.
			if (!this->sub_selected)
				return;

			for (int i = 0; i < this->sub_tabs.size(); i++)
			{
				const int sub_tab_width		= (sub_handle_area.w / (int)this->sub_tabs.size());
				point sub_tabs_area			= { sub_handle_area.x + (i * sub_tab_width) + (sub_tab_width / 2), sub_handle_area.y + sub_handle_area.h + (sub_handle_area.h / 2) };

				sub_tab* sub_handle			= this->sub_tabs[i];
				dimension text_size			= fonts->segoe_ui.text_size(sub_handle->get_title());

				if (this->sub_selected == sub_handle)
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
			this->sub_selected->draw();
		}
	}
	// we don't have sub tab then do normal tab.
	else
	{
		for (const auto& handle : this->elements)
		{
			// skip elements that are invalid.
			if (!handle)
				continue;

			// draw our elements.
			handle->draw();
		}
	}
}

void tab::think()
{
	// we have sub tab present then we draw sub tabs.
	if (this->has_sub)
	{
		point sub_position		= this->parent->draw_position() + this->position + point(110, 24);
		rect tabs_area			= { sub_position.x + this->size.w, sub_position.y + this->size.h, this->size.w + 440, this->size.h };
		rect sub_handle_area	= { tabs_area.x + 15, tabs_area.y + (tabs_area.h - 70), tabs_area.w + 118, 66 };

		// sometimes we want empty window.
		if (!this->sub_selected)
			return;

		if (!events->get_focussed())
		{
			for (int i = 0; i < this->sub_tabs.size(); i++)
			{
				const int sub_tab_width = (sub_handle_area.w / (int)this->sub_tabs.size());

				sub_tab* sub_handle = this->sub_tabs[i];
				dimension text_size = fonts->segoe_ui.text_size(sub_handle->get_title());

				rect sub_tabs_area = { sub_handle_area.x + (i * sub_tab_width) + (sub_tab_width / 2), sub_handle_area.y + sub_handle_area.h + (sub_handle_area.h / 2), text_size.w, text_size.h };

				// fixed: (GetAsyncKeyState(VK_LBUTTON) & 1) causes to only select 2 tabs. - certified retard took 3 days to fix.
				if (input->in_bound({ sub_tabs_area.x - (text_size.w / 2), sub_tabs_area.y - (text_size.h / 2), sub_tabs_area.w, sub_tabs_area.h }) && input->key_pressed(VK_LBUTTON))
					this->sub_selected = sub_handle;
			}
		}

		// handle sub tabs input.
		this->sub_selected->think();
	}
	// we don't have sub tab then do normal tab.
	else
	{
		for (const auto& handle : this->elements)
		{
			// skip elements that are invalid.
			if (!handle)
				continue;

			// handle our elements inputs.
			handle->think();
		}
	}
}

void tab::add(column* handle)
{
	// add our columns.
	this->elements.push_back(handle);

	// set element position.
	float content_width = ((this->parent->get_size().w - 432) * this->elements.size()) / this->elements.size();
	this->offset.x += content_width + 20;
	handle->set_position(this->offset - point(content_width + 20, 0));
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

	this->sub_selected = handle;
}

sub_tab::sub_tab(const char* title, tab* parent)
{
	this->title		= title;
	this->parent	= parent;
}

sub_tab::~sub_tab()
{
	for (auto handle : this->elements)
		SAFE_DELETE(handle);
}

void sub_tab::draw()
{
	for (const auto& handle : this->elements)
	{
		// skip elements that are invalid.
		if (!handle)
			continue;

		// draw our elements.
		handle->draw();
	}
}

void sub_tab::think()
{
	for (const auto& handle : this->elements)
	{
		// skip elements that are invalid.
		if (!handle)
			continue;

		// handle our elements inputs.
		handle->think();
	}
}

void sub_tab::add(column* handle)
{
	// add our columns.
	this->elements.push_back(handle);

	// set element position.
	float content_width = ((this->parent->get_size().w + 268) * this->elements.size()) / this->elements.size();
	this->offset.x += content_width + 20;
	handle->set_position(this->offset - point(content_width + 20, 0));
}

column::column(tab* parent)
{
	this->parent	= parent;
}

column::column(sub_tab* parent)
{
	this->parent	= parent;
	this->in_sub	= true;
}

column::~column()
{
	for (auto handle : this->elements)
		SAFE_DELETE(handle);
}

void column::draw()
{
	for (const auto& handle : this->elements)
	{
		// skip elements that are invalid.
		if (!handle)
			continue;

		// draw our elements.
		handle->draw();
	}
}

void column::think()
{
	for (const auto& handle : this->elements)
	{
		// skip elements that are invalid.
		if (!handle)
			continue;

		// handle our elements inputs.
		handle->think();
	}
}

void column::add(group* handle)
{
	// add our groupbox.
	this->elements.push_back(handle);

	// check if we are in sub tab.
	if (this->in_sub)
	{
		// set element position.
		float sub_content_height = ((handle->get_size().h - 84) * this->elements.size()) / this->elements.size();
		this->offset.y += sub_content_height + 104;
		handle->set_position(this->offset - point(0, sub_content_height + 20));
	}
	// otherwise do normal tab layout.
	else
	{
		// set element position.
		float content_height = ((handle->get_size().h - 20) * this->elements.size()) / this->elements.size();
		this->offset.y += content_height + 40;
		handle->set_position(this->offset - point(0, content_height + 40));
	}
}

group::group(const char* title, const dimension& size, column* parent)
{
	this->title		= title;
	this->size		= size;
	this->parent	= parent;
}

group::~group()
{
	for (auto handle : this->elements)
		SAFE_DELETE(handle);
}

void group::draw()
{
	point offset_position	= this->parent->draw_position() + this->position + point(105, 0);
	rect group_area			= { offset_position.x, offset_position.y, this->size.w, this->size.h };
	float content_height	= this->offset.y - 20;

	// background.
	render->filled_rect(group_area.x, group_area.y, group_area.w, group_area.h, color(12, 12, 12));

	// handle element clipping.
	render->start_clip(group_area);
	{
		for (const auto& handle : this->elements)
		{
			// skip elements that are invalid.
			if (!handle)
				continue;

			// draw our elements.
			if (!events->has_focus() || (events->has_focus() && !events->has_focus(handle)))
				handle->draw();
		}

		// separator / header.
		render->filled_rect(group_area.x + 1, group_area.y, group_area.w - 2, 5, color(12, 12, 12));
		render->gradient(group_area.x + 1, group_area.y + 5, group_area.w - 2, 12, color(12, 12, 12), color(12, 12, 12, 0), gradient_direction::vertical);

		// blur.
		if (content_height > group_area.h - 45)
			render->gradient(group_area.x + 1, (group_area.y + group_area.h) - 13, group_area.w - 2, 12, color(12, 12, 12, 0), color(12, 12, 12), gradient_direction::vertical);
	}
	render->end_clip();

	// outline.
	render->outlined_rect(group_area.x, group_area.y, group_area.w, group_area.h, color(35, 35, 35));

	// handle scroll bar rendering.
	if (content_height > group_area.h - 45)
	{
		// scrolling logic (scroll bar animation).
		int max_scroll				= content_height - group_area.h + 45;
		float scroll_position		= this->scroll / content_height * (group_area.h - 45) * -1; // multiply by -1 -> otherwise when scrolled the bar will go upwards not down.
		float max_scroll_position	= max_scroll / content_height * (group_area.h - 45);

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
		dimension scroll_size	= { 5, group_area.h };

		// scroll background.
		render->filled_rect((group_area.x + group_area.w) - scroll_size.w - 1, group_area.y + 1, scroll_size.w, scroll_size.h - 2, color(25, 25, 25));

		// scroll bar.
		render->filled_rect((group_area.x + group_area.w) - scroll_size.w + 1, group_area.y + scroll_position + 2, scroll_size.w - 3, scroll_size.h - max_scroll_position - 3, color(45, 45, 45));
	}

	// black line.
	dimension text_size		= fonts->segoe_ui.text_size(this->get_title());
	render->filled_rect(group_area.x + 15, group_area.y, text_size.w + 10, 1, color(12, 12, 12));

	// group box title.
	fonts->segoe_ui.text(group_area.x + 21, group_area.y - 7, this->get_title(), color(255, 255, 255));
}

void group::think()
{
	point offset_position	= this->parent->draw_position() + this->position + point(105, 0);
	rect group_area			= { offset_position.x, offset_position.y, this->size.w, this->size.h };
	float content_height	= this->offset.y - 20;

	// handle scrolling.
	if (input->get_mouse_wheel() != 0 && input->in_bound(group_area))
	{
		if (content_height > group_area.h - 45)
		{
			// we haven't interacted with any elements then scroll.
			if (!events->has_focus(this))
				this->scroll += input->get_mouse_wheel() * 12;

			// set mouse scroll state.
			input->set_mouse_wheel(0);

			// scrolling logic (actual function).
			if (this->scroll > 0)
				this->scroll = 0;
			else if (this->scroll < (group_area.h - 45) - content_height)
				this->scroll = (group_area.h - 45) - content_height;
		}
	}

	// iterate through element inputs.
	for (const auto& handle : this->elements)
	{
		// skip elements that are invalid.
		if (!handle)
			continue;

		// handle element input.
		if ((!events->has_focus() && input->in_bound(group_area)) || (events->has_focus() && events->has_focus(handle)))
			handle->think();
	}
}

void group::add(element* handle)
{
	// add our element, i.e. checkbox and etc...
	this->elements.push_back(handle);

	// set element position.
	this->offset.y += handle->get_size().h + 20 + handle->get_distance().h;
	handle->set_position(this->offset - point(0, handle->get_size().h + 20 + handle->get_distance().h));
}

checkbox::checkbox(group* parent, const char* title, bool* value)
{
	this->title		= title;
	this->value		= value;
	this->distance	= { 0, 9 };
	this->parent	= parent;
}

void checkbox::draw()
{
	point control_position	= this->parent->draw_position() + this->position + point(105, 0);
	rect checkbox_area		= { control_position.x, control_position.y, 9, 9 };

	// background.
	render->filled_rect(checkbox_area.x, checkbox_area.y + 1, checkbox_area.w, checkbox_area.h, color(25, 25, 25));

	// value = true -> white.
	if (*this->value)
		render->filled_rect(checkbox_area.x, checkbox_area.y + 1, checkbox_area.w, checkbox_area.h, color(255, 255, 255));

	// outline.
	render->outlined_rect(checkbox_area.x, checkbox_area.y + 1, checkbox_area.w, checkbox_area.h, color(35, 35, 35));

	// title.
	fonts->segoe_ui.text((checkbox_area.x + 11) + checkbox_area.w, checkbox_area.y + (checkbox_area.w / 2) - 7, this->get_title(), color(255, 255, 255));
}

void checkbox::think()
{
	point control_position	= this->parent->draw_position() + this->position + point(105, 0);
	rect checkbox_area		= { control_position.x, control_position.y, 9, 9 };

	// allow click on checkbox text to enable.
	dimension text_size		= fonts->segoe_ui.text_size(this->get_title());
	rect text_area			= { (checkbox_area.x + 11) + checkbox_area.w, checkbox_area.y - (text_size.h / 2), text_size.w, text_size.h };

	if ((input->in_bound(checkbox_area) || input->in_bound(text_area)) && input->key_pressed(VK_LBUTTON))
		events->set_focussed(this);

	if (events->has_focus(this) && input->key_pressed(VK_LBUTTON))
		*this->value = !*this->value;

	if (events->has_focus(this) && input->key_released(VK_LBUTTON))
		events->set_focussed(nullptr);
}

slider_int::slider_int(group* parent, const char* title, int* value, int min, int max, const char* suffix)
{
	this->title		= title;
	this->value		= value;
	this->min		= min;
	this->max		= max;
	this->suffix	= suffix;
	this->distance	= { 0, 9 };
	this->parent	= parent;
}

void slider_int::draw()
{
	point control_position	= this->parent->draw_position() + this->position + point(125, 7);
	rect slider_area		= { control_position.x, control_position.y, 180, 6 };

	// title.
	dimension text_size		= fonts->segoe_ui.text_size(this->get_title());
	fonts->segoe_ui.text(slider_area.x, (slider_area.y - 2) - (text_size.h - 2), this->get_title(), color(255, 255, 255));

	// format slider value.
	std::string text_value	= translate->format("%d", *this->value);

	// slider background.
	render->filled_rect(slider_area.x, slider_area.y, slider_area.w, slider_area.h, color(25, 25, 25));

	// fixed: slider bar not showing whilst sliding until it hits the max value of slider -> only applies to int min, max values.
	float max_delta		= this->max - this->min;
	float value_delta	= *this->value - this->min;
	float value_mod		= (value_delta / max_delta) * slider_area.w;

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
	point control_position	= this->parent->draw_position() + this->position + point(125, 7);
	rect slider_area		= { control_position.x, control_position.y, 180, 6 };
	float max_delta			= this->max - this->min;

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

slider_float::slider_float(group* parent, const char* title, float* value, float min, float max, const char* suffix)
{
	this->title		= title;
	this->value		= value;
	this->min		= min;
	this->max		= max;
	this->suffix	= suffix;
	this->distance	= { 0, 9 };
	this->parent	= parent;
}

void slider_float::draw()
{
	point control_position	= this->parent->draw_position() + this->position + point(125, 7);
	rect slider_area		= { control_position.x, control_position.y, 180, 6 };

	// title.
	dimension text_size		= fonts->segoe_ui.text_size(this->get_title());
	fonts->segoe_ui.text(slider_area.x, (slider_area.y - 2) - (text_size.h - 2), this->get_title(), color(255, 255, 255));

	// format slider value.
	std::string text_value	= translate->format("%.1f", *this->value);

	// slider background.
	render->filled_rect(slider_area.x, slider_area.y, slider_area.w, slider_area.h, color(25, 25, 25));

	// fixed: slider bar not showing whilst sliding until it hits the max value of slider -> only applies to int min, max values.
	float max_delta		= this->max - this->min;
	float value_delta	= *this->value - this->min;
	float value_mod		= (value_delta / max_delta) * slider_area.w;

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
	point control_position	= this->parent->draw_position() + this->position + point(125, 7);
	rect slider_area		= { control_position.x, control_position.y, 180, 6 };
	float max_delta			= this->max - this->min;

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

combo::combo(group* parent, const char* title, int* value, const std::vector<const char*> list)
{
	this->title		= title;
	this->value		= value;
	this->list		= list;
	this->distance	= { 0, 21 };
	this->parent	= parent;
}

void combo::draw()
{
	point control_position	= this->parent->draw_position() + this->position + point(125, 7);
	rect combo_area			= { control_position.x, control_position.y, 180, 18 };

	// dropdown background.
	if (!events->has_focus(this) && input->in_bound(combo_area))
		render->filled_rect(combo_area.x, combo_area.y, combo_area.w, combo_area.h, color(34, 34, 34));
	else
		render->filled_rect(combo_area.x, combo_area.y, combo_area.w, combo_area.h, color(30, 30, 30));

	// dropdown outline.
	render->outlined_rect(combo_area.x, combo_area.y, combo_area.w + 1, combo_area.h + 1, color(35, 35, 35));

	// title.
	fonts->segoe_ui.text(combo_area.x, combo_area.y - (combo_area.h / 2) - 5, this->get_title(), color(255, 255, 255));

	// construct dropdown list.
	fonts->segoe_ui.text(combo_area.x + 8, combo_area.y + (combo_area.h / 2) - 8, this->list[*this->value], color(255, 255, 255));

	// dropdown opened.
	if (events->has_focus(this))
	{
		for (int i = 0; i < this->list.size(); i++)
		{
			rect list_area	= { combo_area.x, combo_area.y + 20 + (combo_area.h * i), combo_area.w, combo_area.h };
			bool in_bound	= input->in_bound(list_area);

			// dropdown list background.
			render->filled_rect(list_area.x, list_area.y, list_area.w + 1, list_area.h, in_bound ? color(25, 25, 25) : color(30, 30, 30));

			// selected -> black.
			if (*this->value == i || in_bound)
			{
				// list items text.
				fonts->segoe_ui.text(list_area.x + 8, list_area.y + (list_area.h / 2) - 8, this->list[i], color(255, 255, 255));
			}
			// not selected -> grey.
			else
			{
				// list items text.
				fonts->segoe_ui.text(list_area.x + 8, list_area.y + (list_area.h / 2) - 8, this->list[i], color(120, 120, 120));
			}
		}
	}

	// dropdown arrows.
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
	point control_position	= this->parent->draw_position() + this->position + point(125, 7);
	rect combo_area			= { control_position.x, control_position.y, 180, 18 };
	// add extra 20 height fixes last item in the dropdown not being registered.
	rect dropdown_area		= { combo_area.x, combo_area.y, combo_area.w, combo_area.h * (20 + (int)this->list.size()) };

	// toggle dropdown opening.
	// putting 'else if' gives the issue where the dropdown value doesn't change.
	if (!events->has_focus(this) && input->in_bound(combo_area) && input->key_pressed(VK_LBUTTON))
		events->set_focussed(this);
	else
	{
		// closes dropdowm if we didn't select anything.
		if (events->has_focus(this) && input->in_bound(combo_area) && input->key_pressed(VK_LBUTTON))
			events->set_focussed(nullptr);
	}

	// if we aren't in dropdown list and interacted elsewhere close dropdown list.
	if (events->has_focus(this) && !input->in_bound(dropdown_area) && input->key_pressed(VK_LBUTTON))
		events->set_focussed(nullptr);

	// open dropdown.
	if (events->has_focus(this))
	{
		for (int i = 0; i < this->list.size(); i++)
		{
			rect list_area		= { combo_area.x, combo_area.y + 20 + (combo_area.h * i), combo_area.w, combo_area.h };

			// selecting items in list area.
			if (input->in_bound(list_area) && input->key_pressed(VK_LBUTTON))
			{
				*this->value	= i;
				events->set_focussed(nullptr);
				break;
			}
		}
	}
}

multi::multi(group* parent, const char* title)
{
	this->title		= title;
	this->distance	= { 0, 21 };
	this->parent	= parent;
}

void multi::draw()
{
	point control_position	= this->parent->draw_position() + this->position + point(125, 7);
	rect multi_area			= { control_position.x, control_position.y, 180, 18 };

	// dropdown background.
	if (!events->has_focus(this) && input->in_bound(multi_area))
		render->filled_rect(multi_area.x, multi_area.y, multi_area.w, multi_area.h, color(34, 34, 34));
	else
		render->filled_rect(multi_area.x, multi_area.y, multi_area.w, multi_area.h, color(30, 30, 30));

	// dropdown outline.
	render->outlined_rect(multi_area.x, multi_area.y, multi_area.w + 1, multi_area.h + 1, color(35, 35, 35));

	// title.
	fonts->segoe_ui.text(multi_area.x, multi_area.y - (multi_area.h / 2) - 5, this->get_title(), color(255, 255, 255));

	// construct dropdown list.
	fonts->segoe_ui.text(multi_area.x + 8, multi_area.y + (multi_area.h / 2) - 8, this->construct_list(this->list).c_str(), color(255, 255, 255));

	// dropdown opened.
	if (events->has_focus(this))
	{
		for (int i = 0; i < this->list.size(); i++)
		{
			rect list_area	= { multi_area.x, multi_area.y + 20 + (multi_area.h * i), multi_area.w, multi_area.h };
			bool in_bound	= input->in_bound(list_area);

			// dropdown list background.
			render->filled_rect(list_area.x, list_area.y, list_area.w + 1, list_area.h, in_bound ? color(25, 25, 25) : color(30, 30, 30));

			// selected -> black.
			if (*this->list[i].value || in_bound)
			{
				// list items text.
				fonts->segoe_ui.text(list_area.x + 8, list_area.y + (list_area.h / 2) - 8, this->list[i].title, color(255, 255, 255));
			}
			// not selected -> grey.
			else
			{
				// list items text.
				fonts->segoe_ui.text(list_area.x + 8, list_area.y + (list_area.h / 2) - 8, this->list[i].title, color(120, 120, 120));
			}
		}
	}

	// dropdown arrows.
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
	point control_position	= this->parent->draw_position() + this->position + point(125, 7);
	rect multi_area			= { control_position.x, control_position.y, 180, 18 };
	// add extra 20 height fixes last item in the dropdown not being registered.
	rect dropdown_area		= { multi_area.x, multi_area.y, multi_area.w, multi_area.h * (20 + (int)this->list.size()) };

	// toggle dropdown opening.
	// putting 'else if' gives the issue where the dropdown value doesn't change.
	if (!events->has_focus(this) && input->in_bound(multi_area) && input->key_pressed(VK_LBUTTON))
		events->set_focussed(this);
	else
	{
		// closes dropdowm if we didn't select anything.
		if (events->has_focus(this) && input->in_bound(multi_area) && input->key_pressed(VK_LBUTTON))
			events->set_focussed(nullptr);
	}

	// if we aren't in dropdown list and interacted elsewhere close dropdown list.
	if (events->has_focus(this) && !input->in_bound(dropdown_area) && input->key_pressed(VK_LBUTTON))
		events->set_focussed(nullptr);

	// open dropdown.
	if (events->has_focus(this))
	{
		for (int i = 0; i < this->list.size(); i++)
		{
			rect list_area	= { multi_area.x, multi_area.y + 20 + (multi_area.h * i), multi_area.w, multi_area.h };

			// selecting items in list area.
			if (input->in_bound(list_area) && input->key_pressed(VK_LBUTTON))
				*this->list[i].value = !*this->list[i].value;
		}
	}
}

void multi::add(const char* title, bool* value)
{
	this->list.push_back(multi_info{ title, value });
}

keybind::keybind(group* parent, const char* title, bool* value, int* key_value, bool inlined, int key_type)
{
	this->title		= title;
	this->value		= value;
	this->key_value = key_value;
	this->key_type	= key_type;
	this->inlined	= inlined;
	this->distance	= { 0, this->inlined ? -19 : 0 };
	this->parent	= parent;

	// get our keybind actuation type.
	this->handle_key_type();
}

void keybind::draw()
{
	point control_position	= this->parent->draw_position() + this->position + point(125, this->inlined ? -25 : 0);
	point keybind_area		= { control_position.x, control_position.y };
	dimension text_size		= fonts->segoe_ui.text_size(this->get_title());

	// title.
	if (!this->inlined)
		fonts->segoe_ui.text(keybind_area.x, keybind_area.y - (text_size.h / 2), this->get_title(), color(255, 255, 255));

	// get our input key name from the key mapper.
	std::string key = "[" + this->get_key_name(*this->key_value) + "]";

	// set dots when we are picking our input key.
	if (this->picking)
		key = "[...]";

	// key value string.
	fonts->segoe_ui.text(keybind_area.x + 190, keybind_area.y - (text_size.h / 2) - 1, key.c_str(), color(255, 255, 255));

	// key type dropdown.
	if (events->has_focus(this) && this->type_list_opened)
	{
		rect list_area		= { keybind_area.x + 195, keybind_area.y - (text_size.h / 2) + 7, 60, 80 };

		// dropdown list background.
		render->filled_rect(list_area.x, list_area.y, list_area.w, list_area.h, color(30, 30, 30));

		for (int i = 0; i < this->list.size(); i++)
		{
			rect item_area	= { list_area.x, list_area.y + (20 * i), list_area.w, 20 };
			bool in_bound	= input->in_bound(item_area);

			// dropdown list background.
			render->filled_rect(item_area.x, item_area.y, item_area.w, item_area.h, in_bound ? color(25, 25, 25) : color(30, 30, 30));

			// selected -> white.
			if (this->key_type == i || in_bound)
			{
				// item text.
				fonts->segoe_ui.text(item_area.x + 8, item_area.y + 2, this->list[i], color(255, 255, 255));
			}
			// not selected -> grey.
			else
			{
				// item text.
				fonts->segoe_ui.text(item_area.x + 8, item_area.y + 2, this->list[i], color(120, 120, 120));
			}
		}
	}
}

void keybind::think()
{
	point control_position	= this->parent->draw_position() + this->position + point(125, this->inlined ? -25 : 0);
	point keybind_area		= { control_position.x, control_position.y };
	dimension text_size		= fonts->segoe_ui.text_size(this->get_title());
	rect title_area			= { keybind_area.x + 190, keybind_area.y - (text_size.h / 2) - 1, text_size.w, text_size.h };
	// add extra 20 height fixes last item in the dropdown not being registered.
	rect dropdown_area		= { title_area.x, title_area.y, title_area.w, title_area.h * (20 + (int)this->list.size()) };

	// key type dropdown.
	// toggle dropdown opening.
	// putting 'else if' gives the issue where the dropdown value doesn't change.
	if (!events->has_focus(this) && !this->type_list_opened && !this->picking && input->in_bound(title_area) && input->key_pressed(VK_RBUTTON))
	{
		this->picking			= false;
		this->type_list_opened	= true;
		events->set_focussed(this);
	}
	else
	{
		// closes dropdowm if we didn't select anything.
		if (events->has_focus(this) && this->type_list_opened && !this->picking && input->in_bound(title_area) && input->key_pressed(VK_LBUTTON))
		{
			this->picking			= false;
			this->type_list_opened	= false;
			events->set_focussed(nullptr);
		}
	}

	// if we aren't in dropdown list and interacted elsewhere close dropdown list.
	if (events->has_focus(this) && this->type_list_opened && !this->picking && !input->in_bound(dropdown_area) && input->key_pressed(VK_LBUTTON))
	{
		this->picking			= false;
		this->type_list_opened	= false;
		events->set_focussed(nullptr);
	}

	// dropdown list.
	if (this->type_list_opened)
	{
		rect list_area		= { title_area.x + 5, title_area.y + 8, 60, 80 };

		for (int i = 0; i < this->list.size(); i++)
		{
			rect item_area	= { list_area.x, list_area.y + (20 * i), list_area.w, 20 };

			// selecting items in list area.
			if (input->in_bound(item_area) && input->key_pressed(VK_LBUTTON))
			{
				this->key_type			= i;
				this->type_list_opened	= false;
				events->set_focussed(nullptr);
				break;
			}
		}
	}

	// keybind picking.
	if (!events->has_focus(this) && !this->type_list_opened && !this->picking && input->in_bound(title_area) && input->key_pressed(VK_LBUTTON))
	{
		this->picking		= true;
		events->set_focussed(this);
	}
	// we want to pick a keybind.
	else if (this->picking)
	{
		// press esc to stop picking if we don't want to bind a key anymore.
		if (input->key_pressed(VK_ESCAPE))
		{
			// set key value to invalid as we are not picking a key anymore.
			*this->key_value	= -1;

			// set picking to false cuz we are picking atm.
			this->picking		= false;
			events->set_focussed(nullptr);
			return;
		}

		for (int i = 0; i < max_key_state; i++)
		{
			// skip esc key as we are picking a keybind.
			if (i == VK_ESCAPE)
				continue;

			if (input->key_pressed(i))
			{
				// set key value to our picked keybind.
				*this->key_value	= i;

				// set picking to false cuz we picked a keybind.
				this->picking		= false;
				events->set_focussed(nullptr);
				break;
			}
		}
	}
}

void keybind::handle_key_type()
{
	// i don't think i need to clamp it anymore but just in case i'll leave it here.
	// this->key_type = std::clamp(this->key_type, 0, (int)this->list.size());

	switch (this->key_type)
	{
	case bind_type::none:
		*this->value = false;
		break;

	case bind_type::hold:
		*this->value = GetAsyncKeyState(*this->key_value);
		break;

	case bind_type::toggle:
		// have to do this way otherwise it will flash like crazy when turned on or off.
		if (GetKeyState(*this->key_value))
			*this->value = true;
		else
			*this->value = false;
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

color_picker::color_picker(group* parent, const char* title, color value, color* ptr, bool inlined)
{
	this->title				= title;
	this->value				= value;
	this->ptr				= ptr;
	this->inlined			= inlined;
	this->distance			= { 0, this->inlined ? -19 : 0 };
	this->parent			= parent;
}

void color_picker::draw()
{
	point control_position	= this->parent->draw_position() + this->position + point(125, this->inlined ? -25 : 0);
	rect picker_area		= { control_position.x, control_position.y, 20, 9 };
	dimension text_size		= fonts->segoe_ui.text_size(this->get_title());

	// title.
	if (!this->inlined)
		fonts->segoe_ui.text(picker_area.x, picker_area.y - (text_size.h / 2), this->get_title(), color(255, 255, 255));

	// update preview opacity.
	color preview = this->value;

	// get our preview area.
	rect preview_area = { picker_area.x + picker_area.w + 170, picker_area.y - (text_size.h / 2) + 4, picker_area.w, picker_area.h };

	// background.
	render->filled_rect(preview_area.x, preview_area.y, preview_area.w, preview_area.h, color(30, 30, 30));

	// picker preview color.
	render->filled_rect(preview_area.x, preview_area.y, preview_area.w, preview_area.h, preview);

	// shadows.
	render->gradient(preview_area.x, preview_area.y, preview_area.w, preview_area.h, color(50, 50, 35, 0), color(50, 50, 35, 150), gradient_direction::vertical);

	// outline.
	render->outlined_rect(preview_area.x, preview_area.y, preview_area.w + 1, preview_area.h, color(35, 35, 35));

	// open color picker.
	if (events->has_focus(this))
	{
		rect inner_area = { picker_area.x + 25, preview_area.y, 150, 150 };

		// generate our color palette.
		this->update();

		// background.
		render->filled_rect(inner_area.x, inner_area.y + 5, inner_area.w + 25, inner_area.h + 25, color(30, 30, 30));

		// outline.
		render->outlined_rect(inner_area.x, inner_area.y + 5, inner_area.w + 25, inner_area.h + 25, color(35, 35, 35));

		// picker palette.
		for (int i = 0; i < 150; i++)
			render->gradient(inner_area.x + 5 + i, inner_area.y + 10, 1, inner_area.h, this->gradient[i].rgba(), this->gradient[i].set(0, 0, 0), gradient_direction::vertical);

		// picker palette outline.
		render->outlined_rect(inner_area.x + 5, inner_area.y + 10, inner_area.w, inner_area.h, color(35, 35, 35));

		// color selector dot.
		float s		= this->saturation * (inner_area.w - 4);
		float v		= (1.f - this->color_value) * (inner_area.h - 4);

		render->filled_rect(inner_area.x + 5 + s, inner_area.y + 10 + v, 4, 4, color(255, 255, 255));

		// color selector dot outline.
		render->outlined_rect(inner_area.x + 5 + s, inner_area.y + 10 + v, 4, 4, color(10, 10, 10));

		// hue bar.
		for (int i = 0; i < 150; i++)
			render->filled_rect(inner_area.x + inner_area.w + 10, inner_area.y + 10 + i, 10, 1, color::hsv_to_rgb(i / 150.f, 1.f, 1.f));
		
		// hue bar outline.
		render->outlined_rect(inner_area.x + inner_area.w + 10, inner_area.y + 10, 11, inner_area.h, color(35, 35, 35));

		// hue bar slider -> optional.
		float h		= this->hue * (inner_area.h - 3);

		render->filled_rect(inner_area.x + inner_area.w + 10, inner_area.y + 10 + h, 11, 3, color(255, 255, 255, 0));

		// hue bar slider outline.
		render->outlined_rect(inner_area.x + inner_area.w + 10, inner_area.y + 10 + h, 11, 3, color(10, 10, 10));

		// update alpha opacity.
		this->alpha = this->value.a / 255.f;

		// alpha bar.
		render->filled_rect(inner_area.x + 5, inner_area.y + inner_area.h + 15, inner_area.w, 10, this->value);

		// alpha bar outline.
		render->outlined_rect(inner_area.x + 5, inner_area.y + inner_area.h + 15, inner_area.w, 11, color(35, 35, 35));

		// alpha bar slider -> optional.
		float a		= this->alpha * (inner_area.w - 3);

		render->filled_rect(inner_area.x + 5 + a, inner_area.y + inner_area.h + 15, 3, 11, color(255, 255, 255, 0));

		// alpha bar slider outline.
		render->outlined_rect(inner_area.x + 5 + a, inner_area.y + inner_area.h + 15, 3, 11, color(10, 10, 10));
	}
}

void color_picker::think()
{
	point control_position	= this->parent->draw_position() + this->position + point(125, this->inlined ? -25 : 0);
	rect picker_area		= { control_position.x, control_position.y, 20, 9 };
	dimension text_size		= fonts->segoe_ui.text_size(this->get_title());

	rect preview_area		= { picker_area.x + picker_area.w + 170, picker_area.y - (text_size.h / 2) + 4, picker_area.w, picker_area.h };
	rect inner_area			= { picker_area.x + 25, preview_area.y, 150, 150 };

	// toggle picker opening.
	if (!events->has_focus(this) && input->in_bound(preview_area) && input->key_pressed(VK_LBUTTON))
		events->set_focussed(this);
	else
	{
		// closes picker if we didn't select anything.
		if (events->has_focus(this) && input->in_bound(preview_area) && input->key_pressed(VK_LBUTTON))
			events->set_focussed(nullptr);
	}

	// open color picker.
	if (events->has_focus(this))
	{
		// color palette bounds.
		rect palette_area	= { inner_area.x + 5, inner_area.y + 10, inner_area.w, inner_area.h };

		// hue bar bounds.
		rect hue_area		= { inner_area.x + inner_area.w + 10, inner_area.y + 10, 10, inner_area.h };

		// alpha bar bounds.
		rect alpha_area		= { inner_area.x + 5, inner_area.y + inner_area.h + 15, inner_area.w, 10 };

		// we touched these areas.
		if (input->key_down(VK_LBUTTON))
		{
			// we are touching color palette area.
			if (input->in_bound(palette_area) || this->color_drag)
			{
				// update color saturation.
				this->saturation	= std::clamp<float>((input->mouse.x - palette_area.x) / (float)inner_area.w, 0.f, 1.f);

				// update color value.
				this->color_value	= std::clamp<float>(1.f - (input->mouse.y - palette_area.y) / (float)inner_area.h, 0.f, 1.f);

				// dragging color.
				this->color_drag	= true;
			}

			// we are touching hue bar area.
			else if (input->in_bound(hue_area) || this->hue_drag)
			{
				// update hue color.
				this->hue			= std::clamp<float>((input->mouse.y - hue_area.y) / (float)inner_area.h, 0.f, 1.f);

				// dragging hue bar.
				this->hue_drag		= true;
			}

			// we are touching alpha bar area.
			else if (input->in_bound(alpha_area) || this->alpha_drag)
			{
				// update alpha color.
				this->alpha			= std::clamp<float>((input->mouse.x - alpha_area.x) / (float)inner_area.w, 0.f, 1.f);

				// dragging alpha.
				this->alpha_drag	= true;
			}
		}
		else
		{
			// reset dragging states.
			this->reset();
		}

		// using color picker.
		if (this->color_drag || this->hue_drag || this->alpha_drag)
		{
			// set updated colors.
			this->value		= color::hsv_to_rgb(this->hue, this->saturation, this->color_value);
			this->value.a	= (this->alpha * 255.f);
		}
	}

	// update our color.
	*this->ptr = this->value;
}

void color_picker::update()
{
	const dimension picker_size = { 150, 150 };

	// allocate gradient.
	if (this->gradient.empty())
		this->gradient = std::vector<color>(picker_size.w * picker_size.h);

	// iterate width.
	for (int x = 0; x < picker_size.w; x++)
	{
		// iterate height.
		for (int y = 0; y < picker_size.h; y++)
		{
			float saturation	= y / float(picker_size.h);
			float value			= 1.f - (x / float(picker_size.w));

			// write back to array.
			this->gradient[x * picker_size.w + y] = color::hsv_to_rgb(this->hue, saturation, value);
		}
	}
}
