#include "VM_Image.h"

Graphics::VM_Image::VM_Image(bool thumb, int width, int height, int mediaID)
{
	this->degrees = 0;
	this->image   = NULL;
	this->pitch   = 0;
	this->pixels  = NULL;

	this->update(thumb, width, height, mediaID);
}

Graphics::VM_Image::~VM_Image()
{
	FREE_IMAGE(this->image);
}

#if defined _windows
void Graphics::VM_Image::open(const WString &file)
#else
void Graphics::VM_Image::open(const String &file)
#endif
{
	FREE_IMAGE(this->image);

	LIB_FREEIMAGE::FIBITMAP* img = VM_Graphics::OpenImage(file);
	LIB_FREEIMAGE::FreeImage_FlipVertical(img);
	this->update(img);
	FREE_IMAGE(img);
}

void Graphics::VM_Image::resize(int width, int height)
{
	this->width  = width;
	this->height = height;
	this->ready  = false;

	LIB_FREEIMAGE::FIBITMAP* img = VM_Graphics::ResizeImage(&this->image, this->width, this->height, true);
	this->update(img);
	FREE_IMAGE(img);
}

void Graphics::VM_Image::rotate(double degrees)
{
	this->degrees = degrees;
	this->ready   = false;

	LIB_FREEIMAGE::FIBITMAP* img = VM_Graphics::RotateImage(&this->image, this->degrees, true);
	this->update(img);
	FREE_IMAGE(img);
}

void Graphics::VM_Image::update(LIB_FREEIMAGE::FIBITMAP* image)
{
	this->image  = LIB_FREEIMAGE::FreeImage_ConvertTo32Bits(image);
	this->width  = LIB_FREEIMAGE::FreeImage_GetWidth(this->image);
	this->height = LIB_FREEIMAGE::FreeImage_GetHeight(this->image);
	this->pitch  = LIB_FREEIMAGE::FreeImage_GetPitch(this->image);
	this->pixels = LIB_FREEIMAGE::FreeImage_GetBits(this->image);
}

void Graphics::VM_Image::update(bool thumb, int width, int height, int mediaID)
{
	this->height = height;
	this->id     = mediaID;
	this->ready  = false;
	this->thumb  = thumb;
	this->width  = width;
}
