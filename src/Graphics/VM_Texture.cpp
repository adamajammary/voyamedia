#include "VM_Texture.h"

using namespace VoyaMedia::System;

Graphics::VM_Texture::VM_Texture(VM_Image* image)
{
	this->init();
	this->update(image);
}

Graphics::VM_Texture::VM_Texture(SDL_Surface* surface)
{
	this->init();

	if (surface != NULL)
	{
		if (VM_Window::Renderer != NULL)
			this->data = SDL_CreateTextureFromSurface(VM_Window::Renderer, surface);

		this->getProperties();
	}
}

Graphics::VM_Texture::VM_Texture(uint32_t format, int access, int width, int height)
{
	this->init();

	if (VM_Window::Renderer != NULL)
		this->data = SDL_CreateTexture(VM_Window::Renderer, format, access, width, height);

	this->getProperties();
}

Graphics::VM_Texture::~VM_Texture()
{
	FREE_TEXTURE(this->data);
}

void Graphics::VM_Texture::init()
{
	this->access = 0;
	this->width  = 0;
	this->height = 0;
	this->format = 0;
	this->data   = NULL;
	this->image  = NULL;
}

void Graphics::VM_Texture::getProperties()
{
	if (this->data != NULL)
		SDL_QueryTexture(this->data, &this->format, &this->access, &this->width, &this->height);
}

void Graphics::VM_Texture::update(VM_Image* image)
{
	if ((image != NULL) && (VM_Window::Renderer != NULL))
	{
		FREE_TEXTURE(this->data);

		this->image = image;

		SDL_Surface* surface = VM_Graphics::GetSurface(this->image);
		this->data = SDL_CreateTextureFromSurface(VM_Window::Renderer, surface);

		FREE_SURFACE(surface);

		this->getProperties();
	}
}
