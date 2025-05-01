#pragma once
#include <cmath>

class color
{
public:
    color() : r{ 0 }, g{ 0 }, b{ 0 }, a{ 255 } { };
    color(int r, int g, int b, int a = 255) : r{ r }, g{ g }, b{ b }, a{ a } { };

    // get color function.
    D3DCOLOR argb() { return D3DCOLOR_ARGB(this->a, this->r, this->g, this->b); }
    color rgba() { return color(this->r, this->g, this->b, this->a); }

    // set color function.
    color set(int r, int g, int b, int a = 255) { return color(r, g, b, a); }

	/*
	* information credit: https://programmingdesignsystems.com/color/color-models-and-color-spaces/index.html#:~:text=HSV%20is%20a%20cylindrical%20color,on%20the%20RGB%20color%20circle.
	* 
	* Hue			-> specifies the angle of the color on the RGB color circle. A 0° hue results in red, 120° results in green, and 240° results in blue.
	* Saturation	-> controls the amount of color used. A color with 100% saturation will be the purest color possible, while 0% saturation yields grayscale.
	* Value			-> controls the brightness of the color. A color with 0% brightness is pure black while a color with 100% brightness has no black mixed into the color.
	* 
	* algorithm credit: https://github.com/Inseckto/HSV-to-RGB/blob/master/HSV2RGB.c
	*/
	color hsv(float hue, float saturation, float value, float alpha)
	{
		float r, g, b;

		/*
		float	h = hue / 360,
				s = saturation / 100,
				v = value / 100;
		*/

		float	h = hue,
				s = saturation,
				v = value;

		int		i = std::floor(h * 6);
		float	f = h * 6 - i,
				p = v * (1 - s),
				q = v * (1 - f * s),
				t = v * (1 - (1 - f) * s);

		switch (i % 6)
		{
		case 0: r = v, g = t, b = p; break;
		case 1: r = q, g = v, b = p; break;
		case 2: r = p, g = v, b = t; break;
		case 3: r = p, g = q, b = v; break;
		case 4: r = t, g = p, b = v; break;
		case 5: r = v, g = p, b = q; break;
		}

		color result;
		result.r = r * 255;
		result.g = g * 255;
		result.b = b * 255;
		result.a = alpha;

		return result;
	}

    int r, g, b, a;
};
