#include "VM_Surface.h"

Graphics::VM_Surface::VM_Surface(SDL_Surface* surface)
{
	const int pixelCount = (surface->w * surface->h);

	this->pixPerLine = (surface->pitch / this->channelsRGBA);
	this->pixels     = static_cast<uint32_t*>(surface->pixels);
	this->surface    = surface;
	this->rgb        = new uint8_t[pixelCount * this->channelsRGB];
	this->a          = new uint8_t[pixelCount];

	// Convert 32-bit pixel color to separate 8-bit RGBA channels
	for (int y = 0; y < this->surface->h; y++)
	{
		int row = (y * this->pixPerLine);

		for (int x = 0; x < this->pixPerLine; x++)
		{
			int p  = (row + x);
			int p3 = (p * this->channelsRGB);

			SDL_GetRGBA(
				this->pixels[p], this->surface->format,
				&this->rgb[p3], &this->rgb[p3 + 1], &this->rgb[p3 + 2], &this->a[p]
			);
		}
	}
}

Graphics::VM_Surface::~VM_Surface()
{
	DELETE_POINTER_ARR(this->rgb);
	DELETE_POINTER_ARR(this->a);
}

int Graphics::VM_Surface::blur(int blur)
{
	if (blur < 1)
		return ERROR_INVALID_ARGUMENTS;

	VM_Graphics::Blur(this->rgb, this->surface->w, this->surface->h, blur);

	// Convert 8-bit channels to a 32-bit pixel color
	for (int y = 0; y < this->surface->h; y++)
	{
		int row = (y * this->pixPerLine);

		for (int x = 0; x < this->pixPerLine; x++) {

			int p  = (row + x);
			int p3 = (p * this->channelsRGB);

			this->pixels[p] = (
				(this->rgb[p3]     << surface->format->Rshift) | 
				(this->rgb[p3 + 1] << surface->format->Gshift) | 
				(this->rgb[p3 + 2] << surface->format->Bshift) | 
				(this->a[p]        << surface->format->Ashift)
			);
		}
	}

	return RESULT_OK;
}
