#include "VM_Color.h"

Graphics::VM_Color::VM_Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

Graphics::VM_Color::VM_Color(const VM_Color &color)
{
	this->r = color.r;
	this->g = color.g;
	this->b = color.b;
	this->a = color.a;
}

Graphics::VM_Color::VM_Color(const SDL_Color &color)
{
	this->r = color.r;
	this->g = color.g;
	this->b = color.b;
	this->a = color.a;
}

Graphics::VM_Color::VM_Color()
{
	this->r = this->g = this->b = this->a = 0;
}

bool Graphics::VM_Color::operator ==(const VM_Color &color)
{
	return ((color.r == this->r) && (color.g == this->g) && (color.b == this->b) && (color.a == this->a));
}

bool Graphics::VM_Color::operator !=(const VM_Color &color)
{
	return !((VM_Color)color == *this);
}

Graphics::VM_Color::operator SDL_Color()
{
	return { this->r, this->g, this->b, this->a };
}
