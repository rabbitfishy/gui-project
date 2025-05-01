#pragma once
#include "../render/render.h"
#include "../other/maths.h"
#include "../other/translate.h"
#include "../other/color.h"
#include <functional>

#define max_key_state 255

namespace gui
{
	class gui_input
	{
	public:
		void setup(HWND handle);
		void poll_input();
		bool process_mouse(HWND handle, UINT message, WPARAM wparam, LPARAM lparam);

		bool key_down(const int key);
		bool key_pressed(const int key);
		bool key_released(const int key);
		bool in_bound(rect area);

		point	mouse;

		void set_mouse_wheel(int mouse_wheel);
		int get_mouse_wheel();

	private:
		HWND	handle = nullptr;
		bool	key_state[max_key_state];
		bool	old_key_state[max_key_state];
		int		mouse_wheel = 0;
	};
	extern gui_input* input;

	class control
	{
	public:
		virtual void draw()			= 0;
		virtual void think()		= 0;

		virtual const char* get_title()
		{
			return this->title;
		}

		virtual void set_position(const point& handle)
		{
			this->position = handle;
		}

		virtual point get_position()
		{
			return this->position;
		}

		virtual void set_size(const dimension& handle)
		{
			this->size = handle;
		}

		virtual dimension get_size()
		{
			return this->size;
		}

		virtual point draw_position()
		{
			if (this->child == nullptr)
				return this->position;

			return this->child->draw_position() + this->position;
		}

	protected:
		const char*				title;
		point					position;
		point					offset;
		dimension				size;
		control*				child;
		std::vector<control*>	controls;
	};

	class gui_event
	{
	public:
		void set_state(int bind);
		bool get_state();
		int set_key(int bind);
		int get_key();

	public:
		void set_focussed(control* focussed);
		control* get_focussed();
		bool has_focus(control* focussed = nullptr);

	private:
		bool					opened = true;
		control*				focus;
		int						key;
	};
	extern gui_event* events;

	class window;
	class gui_instance
	{
	public:
		~gui_instance();

		void draw();
		void think();
		void add(window* handle);

	private:
		float					current_time = -1.f;
		std::vector<window*>	windows;
		window*					dragging = nullptr;
		point					drag;
	};
	extern gui_instance* instance;

	class sub_tab;
	class tab : public control
	{
	public:
		tab(const char* title, window* child, bool has_sub = false);
		~tab();

		void draw()				override;
		void think()			override;
		point draw_position()	override { return control::draw_position(); }

		void add(control* handle);

		void add(sub_tab* handle);
		void set_default_sub(sub_tab* handle);

	private:
		bool					has_sub;
		std::vector<sub_tab*>	sub_tabs;
		sub_tab*				selected_sub = nullptr;
	};

	class sub_tab : public control
	{
	public:
		sub_tab(const char* title, tab* child);
		~sub_tab();

		void draw()				override;
		void think()			override;
		point draw_position()	override { return this->child->draw_position() + this->position + point(16, 20); }

		void add(control* handle);
	};

	class window : public control
	{
		friend gui_instance;
	public:
		window(const char* title, const point& position, const dimension& size);
		~window();

		void draw()				override;
		void think()			override;
		point draw_position()	override { return this->position + point(19, 22); }

		void add(tab* handle);
		void set_default_tab(tab* handle);

	private:
		std::vector<tab*>		tabs;
		tab*					selected = nullptr;
		float					last_input_time = 0.f;
	};

	class column : public control
	{
	public:
		column(control* child, bool in_sub = false);
		~column();

		void draw()				override;
		void think()			override;

		void add(control* handle);

	private:
		bool	in_sub;
	};

	class group : public control
	{
	public:
		group(const char* title, const dimension& size, column* child);

		void draw()				override;
		void think()			override;
		point draw_position()	override { return this->child->draw_position() + this->position + point(16, 20 + this->scroll); }

		void add(control* handle);

	private:
		float	scroll;
	};

	class label : public control
	{
	public:
		label(control* child, const char* title, color colour = color(255, 255, 255));

		void draw()					override;
		void think()				override { };

	private:
		color	colour;
	};

	class button : public control
	{
	public:
		button(control* child, const char* title, std::function<void()> callback);

		void draw()					override;
		void think()				override;

	private:
		std::function<void()>	callback;
		bool					selected = false;
	};

	class checkbox : public control
	{
	public:
		checkbox(control* child, const char* title, bool* value);

		void draw()					override;
		void think()				override;

	private:
		bool*	value;
	};

	class slider_int : public control
	{
	public:
		slider_int(control* child, const char* title, int* value, int min, int max, const char* suffix = "\0");

		void draw()					override;
		void think()				override;

	private:
		int*			value;
		int				min;
		int				max;
		const char*		suffix;
	};

	class slider_float : public control
	{
	public:
		slider_float(control* child, const char* title, float* value, float min, float max, const char* suffix = "\0");

		void draw()					override;
		void think()				override;

	private:
		float*			value;
		float			min;
		float			max;
		const char*		suffix;
	};

	class combo : public control
	{
	public:
		combo(control* child, const char* title, int* value, const std::vector<const char*>& items);

		void draw()					override;
		void think()				override;

	private:
		std::vector<const char*>	items;
		int*						value;
	};

	class multi : public control
	{
		struct multi_data
		{
			const char*		title;
			bool*			value;
		};

	public:
		multi(control* child, const char* title, const std::vector<multi_data>& items);

		void draw()					override;
		void think()				override;

	private:
		std::vector<multi_data>		items;

		std::string construct_items(const std::vector<multi_data> items)
		{
			std::string result;

			for (int i = 0; i < items.size(); i++)
			{
				bool length_exceeded = result.length() >= 16;

				if (*items[i].value && !length_exceeded)
				{
					// isn't first item.
					if (!result.empty())
						result.append(", ");

					result.append(items[i].title);
				}
				else if (length_exceeded)
				{
					result.append("...");
					break;
				}
			}

			// not selected then don't show any result.
			if (result.empty())
				result = "-";

			return result;
		}
	};

	class keybind : public control
	{
		enum bind_type : int
		{
			none,
			hold,
			toggle,
			always
		};

	public:
		keybind(control* child, const char* title, bool* value, int* key_value, int type = bind_type::none);

		void draw()					override;
		void think()				override;

	private:
		bool*						value;
		bool						picking = false;
		bool						context_open = false;

		int*						key_value;
		int							type;

		std::vector<const char*>	items = { "none", "hold", "toggle", "always" };

		void handle_key_type();

		// stolen from pandora cuz it works and doesn't require ugly ass key name arrays.
		std::string key_name(const int key)
		{
			int result;
			char buffer[128];
			UINT key_code = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);

			switch (key)
			{
			case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
			case VK_RCONTROL: case VK_RMENU:
			case VK_LWIN: case VK_RWIN: case VK_APPS:
			case VK_PRIOR: case VK_NEXT:
			case VK_END: case VK_HOME:
			case VK_INSERT: case VK_DELETE:
			case VK_DIVIDE:
			case VK_NUMLOCK:
				key_code |= KF_EXTENDED;
			default:
				result = GetKeyNameText(key_code << 16, buffer, 128);
			}

			if (result == 0)
			{
				switch (key)
				{
				case VK_XBUTTON1:
					return "M4";
				case VK_XBUTTON2:
					return "M5";
				case VK_LBUTTON:
					return "M1";
				case VK_MBUTTON:
					return ("M3");
				case VK_RBUTTON:
					return "M2";
				default:
					return "-";
				}
			}

			return translate->upper(std::string(buffer).c_str());
		}
	};

	class color_picker : public control
	{
	public:
		color_picker(control* child, const char* title, color* value);

		void draw()					override;
		void think()				override;

	private:
		bool	context_open = false;
		color*	value;
		int		min;
		int		max;

		void color_slider(const char* title, int x, int y, int w, int h, color value, int format_color);
	};
}