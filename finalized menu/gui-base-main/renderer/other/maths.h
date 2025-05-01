#pragma once
#include <cmath>

// this is vector 2d but renamed it from 'vector' to 'point' for less confusion.
// it was only made for drawing 2d objects and calculating 2d objects position.
class point
{
public:
	constexpr point() : x{ }, y{ } { }
	constexpr point(int x, int y) : x{ x }, y{ y } { }
	constexpr point(const point& p) : x{ p.x }, y{ p.y } { }

	// equality operators.
	bool operator==(const point& p) const { return p.x == this->x && p.y == this->y; }
	bool operator!=(const point& p) const { return p.x != this->x || p.y != this->y; }

	// copy assignment.
	constexpr point& operator=(const point& p)
	{
		this->x = p.x; this->y = p.y;
		return *this;
	}

	// negation-operator.
	constexpr point operator-() const { return point{ -this->x, -this->y }; }

	// arithmetic operators.
	point operator+(const point& p) const { return { this->x + p.x, this->y + p.y }; }
	point operator-(const point& p) const { return { this->x - p.x, this->y - p.y }; }
	point operator*(const point& p) const { return { this->x * p.x, this->y * p.y }; }
	point operator/(const point& p) const { return { this->x / p.x, this->y / p.y }; }

	int x, y;
};

class rect
{
public:
	constexpr rect() : x{ }, y{ }, w{ }, h{ } { }
	constexpr rect(int x, int y, int w, int h) : x{ x }, y{ y }, w{ w }, h{ h } { }
	int x, y, w, h;
};

class dimension
{
public:
	constexpr dimension() : w{ }, h{ } { }
	constexpr dimension(int w, int h) : w{ w }, h{ h } { }
	int w, h;
};

class vector_2d
{
public:
	constexpr vector_2d() : x{ }, y{ } { }
	constexpr vector_2d(const float x, const float y) : x{ x }, y{ y } { }

	// index operators.
	[[nodiscard]] float& operator[](const std::size_t index) { return reinterpret_cast<float*>(this)[index]; }
	[[nodiscard]] const float& operator[](const std::size_t index) const { return reinterpret_cast<const float*>(this)[index]; }

	// equality operators.
	bool operator==(const vector_2d& v) const { return v.x == this->x && v.y == this->y; }
	bool operator!=(const vector_2d& v) const { return v.x != this->x || v.y != this->y; }

	// copy assignment.
	constexpr vector_2d& operator=(const vector_2d& v)
	{
		this->x = v.x; this->y = v.y;
		return *this;
	}

	// negation-operator.
	constexpr vector_2d operator-() const { return vector_2d{ -this->x, -this->y }; }

	// arithmetic operators.
	vector_2d operator+(const vector_2d& v) const { return { this->x + v.x, this->y + v.y }; }
	vector_2d operator-(const vector_2d& v) const { return { this->x - v.x, this->y - v.y }; }
	vector_2d operator*(const vector_2d& v) const { return { this->x * v.x, this->y * v.y }; }
	vector_2d operator/(const vector_2d& v) const { return { this->x / v.x, this->y / v.y }; }

	// compound assignment operators.
	vector_2d operator+=(const vector_2d& v)
	{
		this->x += v.x; this->y += v.y;
		return *this;
	}

	vector_2d operator-=(const vector_2d& v)
	{
		this->x -= v.x; this->y -= v.y;
		return *this;
	}

	vector_2d operator*=(const vector_2d& v)
	{
		this->x *= v.x; this->y *= v.y;
		return *this;
	}

	vector_2d operator/=(const vector_2d& v)
	{
		this->x /= v.x; this->y /= v.y;
		return *this;
	}

	// arithmetic operators w/ float.
	vector_2d operator+(const float f) const { return { this->x + f, this->y + f }; }
	vector_2d operator-(const float f) const { return { this->x - f, this->y - f }; }
	vector_2d operator*(const float f) const { return { this->x * f, this->y * f }; }
	vector_2d operator/(const float f) const { return { this->x / f, this->y / f }; }

	// compound assignment operators w/ float.
	vector_2d& operator+=(float f)
	{
		this->x += f; this->y += f;
		return *this;
	}

	vector_2d& operator-=(float f)
	{
		this->x -= f; this->y -= f;
		return *this;
	}

	vector_2d& operator*=(float f)
	{
		this->x *= f; this->y *= f;
		return *this;
	}

	vector_2d& operator/=(float f)
	{
		this->x /= f; this->y /= f;
		return *this;
	}

	float x, y;
};

// NOTE: it must be float values for it to render otherwise shit won't render anything in directx.
// that is why i put it here to use vector 2d's to render so it must be float value!!!!
class vertex
{
public:
	vertex() : position{ }, coordinate{ }, colour{ } { }
	vertex(const vector_2d& position, const vector_2d& coordinate, const DWORD colour)
	{
		this->position		= position;
		this->coordinate	= coordinate;
		this->colour		= colour;
	}

	vector_2d	position;
	vector_2d	coordinate;
	DWORD		colour;
};
