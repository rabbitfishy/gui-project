#include "menu.h"

environment_menu* menu = new environment_menu;
using namespace gui;

bool t_check;
int t_slider_int = 0;
float t_slider_float = 0.f;
int t_combo = 0;
bool t_multi[4];
bool t_key;
int t_key_value = -1;
color t_color_picker;

void environment_menu::draw_test()
{
	if (t_check)
		render->filled_rect(100, 100, 100, 100, t_color_picker);
}

void environment_menu::setup()
{
	events->set_key(VK_INSERT);

	auto main = new gui::window("main", { 100, 100 }, { 700, 600 });
	{
		auto tab_2 = new tab("B", main, true);
		{
			auto sub_tab_1 = new sub_tab("Player", tab_2);
			{
				auto left = new column(sub_tab_1);
				{
					auto group_1 = new group("group 1", { 270, 330 }, left);
					{

					}
					left->add(group_1);

					auto group_2 = new group("group 2", { 270, 130 }, left);
					{

					}
					left->add(group_2);
				}
				sub_tab_1->add(left);

				auto right = new column(sub_tab_1);
				{
					auto group_3 = new group("group 3", { 270, 230 }, right);
					{

					}
					right->add(group_3);

					auto group_4 = new group("group 4", { 270, 230 }, right);
					{

					}
					right->add(group_4);
				}
				sub_tab_1->add(right);
			}
			tab_2->add(sub_tab_1);

			auto sub_tab_2 = new sub_tab("Local", tab_2);
			{

			}
			tab_2->add(sub_tab_2);

			auto sub_tab_3 = new sub_tab("World", tab_2);
			{

			}
			tab_2->add(sub_tab_3);

			tab_2->set_default_sub(sub_tab_1);
		}
		main->add(tab_2);

		main->set_default_tab(tab_2);
	}
	instance->add(main);

	auto other = new gui::window("other", { 200, 200 }, { 700, 600 });
	{
		auto tab_1 = new tab("A", other);
		{
			auto left = new column(tab_1);
			{
				auto group_1 = new group("group 1", { 270, 170 }, left);
				{
					group_1->add(new checkbox(group_1, "checkbox", &t_check));
					group_1->add(new color_picker(group_1, "picker", color(255, 0, 0), &t_color_picker, true));
					group_1->add(new keybind(group_1, "keybind", &t_check, &t_key_value));
					group_1->add(new slider_int(group_1, "slider int", &t_slider_int, 0, 100, "%"));
					group_1->add(new slider_float(group_1, "slider float", &t_slider_float, 0.f, 100.f, "%"));
					group_1->add(new combo(group_1, "combo", &t_combo, { "apple", "banana", "pineapple", "orange" }));

					auto multi_t = new multi(group_1, "multi");
					const char* str_multi[4] = { "apple", "banana", "pineapple", "orange" };
					for (int i = 0; i < 4; i++)
						multi_t->add(str_multi[i], &t_multi[i]);
					group_1->add(multi_t);
				}
				left->add(group_1);

				auto group_2 = new group("group 2", { 270, 234 }, left);
				{

				}
				left->add(group_2);

				auto group_3 = new group("group 3", { 270, 120 }, left);
				{

				}
				left->add(group_3);
			}
			tab_1->add(left);

			auto right = new column(tab_1);
			{
				auto group_4 = new group("group 4", { 270, 272 }, right);
				{

				}
				right->add(group_4);

				auto group_5 = new group("group 5", { 270, 272 }, right);
				{

				}
				right->add(group_5);
			}
			tab_1->add(right);
		}
		other->add(tab_1);

		other->set_default_tab(tab_1);
	}
	instance->add(other);
}