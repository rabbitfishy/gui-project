#pragma once

class color
{
public:
	color() : r{ 0 }, g{ 0 }, b{ 0 }, a{ 255 } { };
	color(int r, int g, int b, int a = 255) : r{ r }, g{ g }, b{ b }, a{ a } { };

	// add color stuff here
	D3DCOLOR argb() { return D3DCOLOR_ARGB(this->a, this->r, this->g, this->b); }

	int r, g, b, a;
};