#include "menu.h"

environment_menu* menu = new environment_menu;
using namespace gui;

bool t_check;
int t_slider_i;
float t_slider_f;
int t_combo_index = 0;
bool b_1, b_2, b_3, b_4, b_5;
int b_6, b_7;
color fhjkd;

void environment_menu::setup()
{
	events->set_key(VK_INSERT);

	auto main = new gui::window("main", { 100, 100 }, { 700, 600 });
	{
		auto tab_1 = new tab("A", main);
		{
			auto left = new column(tab_1);
			{
				auto group_1 = new group("group 1", { 270, 170 }, left);
				{
					group_1->add(new keybind(group_1, "keybind", &t_check, &b_7));
					group_1->add(new checkbox(group_1, "checkbox", &t_check));
					group_1->add(new label(group_1, "color picker"));
					group_1->add(new color_picker(group_1, "color picker", &fhjkd));
					group_1->add(new slider_float(group_1, "slider float", &t_slider_f, -100.f, 100.f, "%"));
					group_1->add(new combo(group_1, "combo", &t_combo_index, { "apple", "banana", "pear" }));
					group_1->add(new multi(group_1, "multi", { {"apple", &b_1}, {"usb", &b_2}, {"bear", &b_3}, {"pen", &b_4}, {"cake", &b_5} }));
					group_1->add(new button(group_1, "button", []() {
						return render->filled_rect(100, 100, 200, 200, fhjkd);
						}));
					group_1->add(new label(group_1, "label"));
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
				auto group_3 = new group("group 3", { 270, 270 }, right);
				{

				}
				right->add(group_3);

				auto group_4 = new group("group 4", { 270, 270 }, right);
				{

				}
				right->add(group_4);
			}
			tab_1->add(right);
		}
		main->add(tab_1);

		auto tab_2 = new tab("B", main, true);
		{
			auto sub_tab_1 = new sub_tab("Player", tab_2);
			{
				auto left = new column(sub_tab_1, true);
				{
					auto group_1 = new group("Player ESP", { 270, 330 }, left);
					{

					}
					left->add(group_1);

					auto group_2 = new group("Colored Models", { 270, 130 }, left);
					{

					}
					left->add(group_2);
				}
				sub_tab_1->add(left);

				auto right = new column(sub_tab_1, true);
				{
					auto group_3 = new group("Effects", { 270, 230 }, right);
					{

					}
					right->add(group_3);

					auto group_4 = new group("Other ESP", { 270, 230 }, right);
					{

					}
					right->add(group_4);
				}
				sub_tab_1->add(right);
			}
			tab_2->add(sub_tab_1);

			auto sub_tab_2 = new sub_tab("Local", tab_2);
			{
				auto left = new column(sub_tab_2, true);
				{
					auto group_1 = new group("Local ESP", { 270, 330 }, left);
					{

					}
					left->add(group_1);

					auto group_2 = new group("Colored Models", { 270, 130 }, left);
					{

					}
					left->add(group_2);
				}
				sub_tab_2->add(left);

				auto right = new column(sub_tab_2, true);
				{
					auto group_3 = new group("Other ESP", { 270, 476 }, right);
					{

					}
					right->add(group_3);
				}
				sub_tab_2->add(right);
			}
			tab_2->add(sub_tab_2);

			auto sub_tab_3 = new sub_tab("World", tab_2);
			{
				auto left = new column(sub_tab_3, true);
				{
					auto group_1 = new group("World ESP", { 270, 230 }, left);
					{

					}
					left->add(group_1);

					auto group_2 = new group("World Effects", { 270, 230 }, left);
					{

					}
					left->add(group_2);
				}
				sub_tab_3->add(left);

				auto right = new column(sub_tab_3, true);
				{
					auto group_3 = new group("Other ESP", { 270, 476 }, right);
					{

					}
					right->add(group_3);
				}
				sub_tab_3->add(right);
			}
			tab_2->add(sub_tab_3);

			tab_2->set_default_sub(sub_tab_1);
		}
		main->add(tab_2);

		main->set_default_tab(tab_1);
	}
	instance->add(main);
}