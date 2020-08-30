#include "VM_Display.h"

using namespace VoyaMedia::System;

Graphics::VM_Display::VM_Display()
{
	this->display        = {};
	this->dDPI           = 0;
	this->hDPI           = 0;
	this->vDPI           = 0;
	this->scaleFactor    = 0;
	this->scaleFactorDPI = 0;
	this->scaleFactorRes = 0;
	this->displayIndex   = 0;
}

SDL_Rect Graphics::VM_Display::getDimensions()
{
	SDL_Rect dimensions;

	SDL_GetWindowPosition(VM_Window::MainWindow,  &dimensions.x, &dimensions.y);

	#if defined _ios
		SDL_GetWindowSize(VM_Window::MainWindow, &dimensions.w, &dimensions.h);
	#else
		SDL_GL_GetDrawableSize(VM_Window::MainWindow, &dimensions.w, &dimensions.h);
	#endif
	
	SDL_Rect rect;

	SDL_GetWindowSize(VM_Window::MainWindow, &rect.w, &rect.h);
	VM_Modal::ShowMessage(VM_Text::Format("SDL_GetWindowSize: %d, %d", rect.w, rect.h));

	SDL_GL_GetDrawableSize(VM_Window::MainWindow, &rect.w, &rect.h);
	VM_Modal::ShowMessage(VM_Text::Format("SDL_GL_GetDrawableSize: %d, %d", rect.w, rect.h));

	SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(VM_Window::MainWindow), &rect);
	VM_Modal::ShowMessage(VM_Text::Format("SDL_GetDisplayBounds: %d, %d", rect.w, rect.h));

	SDL_GetDisplayUsableBounds(this->displayIndex, &rect);
	VM_Modal::ShowMessage(VM_Text::Format("SDL_GetDisplayUsableBounds: %d, %d", rect.w, rect.h));

	SDL_GetRendererOutputSize(VM_Window::Renderer, &rect.w, &rect.h);
	VM_Modal::ShowMessage(VM_Text::Format("SDL_GetRendererOutputSize: %d, %d", rect.w, rect.h));

	return dimensions;
}

float Graphics::VM_Display::getDPI()
{
	SDL_GetCurrentDisplayMode(this->displayIndex, &this->display);
	SDL_GetDisplayDPI(this->displayIndex, &this->dDPI, &this->hDPI, &this->vDPI);

	float dpi = max(max(this->hDPI, this->vDPI), this->dDPI);
	VM_Modal::ShowMessage(VM_Text::Format("dpi: %.2f, %.2f, %.2f, %.2f, wh: %d, %d", dpi, this->hDPI, this->vDPI, this->dDPI, this->display.w, this->display.h));

	#if defined _android
	if (dpi < 1.0f)
	{
		jclass    jniClass       = VM_Window::JNI->getClass();
		JNIEnv*   jniEnvironment = VM_Window::JNI->getEnvironment();
		jmethodID jniGetDPI      = jniEnvironment->GetStaticMethodID(jniClass, "GetDPI", "()I");

		if (jniGetDPI != NULL)
			dpi = (float)jniEnvironment->CallStaticIntMethod(jniClass, jniGetDPI);
	}
	#endif

	if (dpi < 1.0f)
		dpi = DEFAULT_DPI;

	return dpi;
}

int Graphics::VM_Display::setDisplayMode()
{
	if (VM_Window::MainWindow == NULL)
		return ERROR_INVALID_ARGUMENTS;

	SDL_Rect windowDimensions = this->getDimensions();

	this->displayIndex   = SDL_GetWindowDisplayIndex(VM_Window::MainWindow);
	this->scaleFactorDPI = (this->getDPI() / DEFAULT_DPI);
	this->scaleFactorRes = min((float)windowDimensions.w / (float)MIN_WINDOW_SIZE, (float)windowDimensions.h / (float)MIN_WINDOW_SIZE);

	#if defined _android
		this->scaleFactor = this->scaleFactorDPI;
    #elif defined _macosx || defined _windows
        this->scaleFactor = this->scaleFactorRes;
	// TODO: iOS? Linux?
	#else
		this->scaleFactor = (this->scaleFactorRes * this->scaleFactorDPI);
	#endif

	VM_Modal::ShowMessage(VM_Text::Format("setDisplayMode: %.2f, %.2f, %.2f", this->scaleFactor, this->scaleFactorDPI, this->scaleFactorRes));
	return RESULT_OK;
}
