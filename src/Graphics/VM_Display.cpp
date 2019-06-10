#include "VM_Display.h"

using namespace VoyaMedia::System;

Graphics::VM_Display::VM_Display()
{
	this->display        = {};
	this->dDPI           = 0;
	this->hDPI           = 0;
	this->vDPI           = 0;
	this->scaleFactorDPI = 0;
	this->scaleFactorRes = 0;
	this->displayIndex   = 0;
}

SDL_Rect Graphics::VM_Display::getDimensions()
{
	SDL_Rect dimensions;

	SDL_GetWindowPosition(VM_Window::MainWindow,  &dimensions.x, &dimensions.y);
	SDL_GL_GetDrawableSize(VM_Window::MainWindow, &dimensions.w, &dimensions.h);

	return dimensions;
}

int Graphics::VM_Display::getDisplayMode()
{
	if (VM_Window::MainWindow == NULL)
		return ERROR_INVALID_ARGUMENTS;

	SDL_Rect windowDimensions = this->getDimensions();

	this->displayIndex   = SDL_GetWindowDisplayIndex(VM_Window::MainWindow);
	this->scaleFactorDPI = (this->getDPI() / DEFAULT_DPI);
	this->scaleFactorRes = min((float)windowDimensions.w / (float)MIN_WINDOW_SIZE, (float)windowDimensions.h / (float)MIN_WINDOW_SIZE);

	#if defined _android
		this->scaleFactor = this->scaleFactorDPI;
    #elif defined _macosx
        this->scaleFactor = scaleFactorRes;
	#else
		this->scaleFactor = (this->scaleFactorRes * this->scaleFactorDPI);
	#endif

	return RESULT_OK;
}

float Graphics::VM_Display::getDPI()
{
	SDL_GetCurrentDisplayMode(this->displayIndex, &this->display);
	SDL_GetDisplayDPI(this->displayIndex, &this->dDPI, &this->hDPI, &this->vDPI);

	float dpi = max(max(this->hDPI, this->vDPI), this->dDPI);

	#if defined _android
	if (dpi < 1.0f)
	{
		jclass    jniClass       = VM_Window::JNI->getClass();
		JNIEnv*   jniEnvironment = VM_Window::JNI->getEnvironment();
		jmethodID jniGetDPI      = jniEnvironment->GetStaticMethodID(jniClass, "GetDPI", "()I");

		if (jniGetDPI == NULL)
			return ERROR_UNKNOWN;

		dpi = (float)jniEnvironment->CallStaticIntMethod(jniClass, jniGetDPI);
	}
	#endif

	if (dpi < 1.0f)
		dpi = DEFAULT_DPI;

	return dpi;
}
