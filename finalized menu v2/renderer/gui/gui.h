#pragma once
#include "../render/render.h"
#include "../other/maths.h"
#include "../other/translate.h"
#include "../other/color.h"

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

	class element;
	class gui_event
	{
	public:
		void set_state(int bind);
		bool get_state();

		int set_key(int bind);
		int get_key();

	public:
		void set_focussed(element* focussed);
		element* get_focussed();
		bool has_focus(element* focussed = nullptr);

	private:
		bool		opened = true;
		int			key;
		element*	selected = nullptr;
	};
	extern gui_event* events;

	class element
	{
	public:
		virtual void draw()		= 0;
		virtual void think()	= 0;

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

		virtual dimension get_distance()
		{
			return this->distance;
		}

		virtual point draw_position()
		{
			if (this->parent == nullptr)
				return this->position;

			return this->parent->draw_position() + this->position;
		}

	protected:
		const char*				title;
		point					position;
		point					offset;
		dimension				size;
		dimension				distance;
		std::vector<element*>	elements;
		element*				parent = nullptr;
	};

	class window;
	class gui_instance
	{
	public:
		gui_instance() { }
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

	class tab;
	class window : public element
	{
		friend gui_instance;
	public:
		window(const char* title, const point& position, const dimension& size);
		~window();

		void draw()				override;
		void think()			override;
		point draw_position()	override { return this->position; }

		void add(tab* handle);
		void set_default_tab(tab* handle);

	private:
		std::vector<tab*>		tabs;
		tab*					tab_selected = nullptr;
		float					last_input_time = 0.f;
	};

	class column;
	class sub_tab;
	class tab : public element
	{
	public:
		tab(const char* title, window* parent, bool has_sub = false);
		~tab();

		void draw()				override;
		void think()			override;
		point draw_position()	override { return this->parent->draw_position(); }

		void add(column* handle);
		void add(sub_tab* handle);
		void set_default_sub(sub_tab* handle);

	private:
		bool					has_sub;
		std::vector<sub_tab*>	sub_tabs;
		sub_tab*				sub_selected = nullptr;
	};

	class sub_tab : public element
	{
	public:
		sub_tab(const char* title, tab* parent);
		~sub_tab();

		void draw()				override;
		void think()			override;
		point draw_position()	override { return this->parent->draw_position(); }

		void add(column* handle);
	};

	class group;
	class column : public element
	{
	public:
		column(tab* parent);
		column(sub_tab* parent);
		~column();

		void draw()				override;
		void think()			override;
		point draw_position()	override { return this->parent->draw_position() + this->position + point(20, 20); }

		void add(group* handle);

	private:
		bool	in_sub;
	};

	class group : public element
	{
	public:
		group(const char* title, const dimension& size, column* parent);
		~group();

		void draw()				override;
		void think()			override;
		point draw_position()	override { return this->parent->draw_position() + this->position + point(20, 20 + this->scroll); }

		void add(element* handle);

	private:
		float	scroll;
	};

	class checkbox : public element
	{
	public:
		checkbox(group* parent, const char* title, bool* value);

		void draw()					override;
		void think()				override;

	private:
		bool* value;
	};

	class slider_int : public element
	{
	public:
		slider_int(group* parent, const char* title, int* value, int min, int max, const char* suffix = "\0");

		void draw()					override;
		void think()				override;

	private:
		int*			value;
		int				min;
		int				max;
		const char*		suffix;
	};

	class slider_float : public element
	{
	public:
		slider_float(group* parent, const char* title, float* value, float min, float max, const char* suffix = "\0");

		void draw()					override;
		void think()				override;

	private:
		float*			value;
		float			min;
		float			max;
		const char*		suffix;
	};

	class combo : public element
	{
	public:
		combo(group* parent, const char* title, int* value, const std::vector<const char*> list);

		void draw()					override;
		void think()				override;

	private:
		std::vector<const char*>	list;
		int*						value;
	};

	class multi : public element
	{
		struct multi_info
		{
			const char* title;
			bool*		value;
		};

	public:
		multi(group* parent, const char* title);

		void draw()					override;
		void think()				override;

		void add(const char* title, bool* value);

	private:
		std::vector<multi_info> list;

		std::string construct_list(const std::vector<multi_info> list)
		{
			// store our final result data.
			std::string result;

			for (int i = 0; i < list.size(); i++)
			{
				// does our multibox have more than 20 characters inside it.
				bool length_exceeded = result.length() >= 20;

				// if we have selected our values and the label length does not exceed 20 characters.
				if (*list[i].value && !length_exceeded)
				{
					// if selected more than 1 option then append a comma at the end of the first selected in multibox.
					if (!result.empty())
						result.append(", ");

					// append our selected options in multibox.
					result.append(list[i].title);
				}
				// if selected options exceeded 20 character length then append dots at the end of it.
				else if (length_exceeded)
				{
					result.append(" ...");
					break;
				}
			}

			// if we haven't selected any options then default a dash inside it.
			if (result.empty())
				result = "-";

			// finally retreat our final string result.
			return result;
		}
	};

	class keybind : public element
	{
		enum bind_type : int
		{
			none,
			hold,
			toggle,
			always
		};

	public:
		keybind(group* parent, const char* title, bool* value, int* key_value, bool inlined = false, int key_type = bind_type::none);

		void draw()					override;
		void think()				override;

	private:
		bool*	value;
		int*	key_value;
		int		key_type;
		bool	inlined;

	private:
		bool						picking = false;
		bool						type_list_opened = false;
		std::vector<const char*>	list = { "none", "hold", "toggle", "always" };
		virtual void handle_key_type();

		// stolen from pandora cuz it works and doesn't require ugly ass key name arrays.
		std::string get_key_name(const int key)
		{
			int result;
			char buffer[128];
			UINT key_code = MapVirtualKey(key, MAPVK_VK_TO_VSC);

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

	class color_picker : public element
	{
	public:
		color_picker(group* parent, const char* title, color value, color* ptr, bool inlined = false);

		void draw()					override;
		void think()				override;

	private:
		color				value;
		color*				ptr;

		float				hue;
		float				saturation;
		float				color_value;
		float				alpha;

		bool				alpha_drag;
		bool				hue_drag;
		bool				color_drag;
		bool				inlined;

		std::vector<color>	gradient;

		void reset()
		{
			this->alpha_drag	= false;
			this->hue_drag		= false;
			this->color_drag	= false;
		}

		void update();
	};
}