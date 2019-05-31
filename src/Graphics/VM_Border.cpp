#include "VM_Border.h"

using namespace VoyaMedia::System;

Graphics::VM_Border::VM_Border()
{
	this->bottom = 0;
	this->left   = 0;
	this->right  = 0;
	this->top    = 0;
}

Graphics::VM_Border::VM_Border(int borderWidth)
{
	this->bottom = (int)((float)borderWidth * VM_Window::Display.scaleFactor);
	this->left   = (int)((float)borderWidth * VM_Window::Display.scaleFactor);
	this->right  = (int)((float)borderWidth * VM_Window::Display.scaleFactor);
	this->top    = (int)((float)borderWidth * VM_Window::Display.scaleFactor);
}

Graphics::VM_Border::VM_Border(int bottom, int left, int right, int top)
{
	this->bottom = (int)((float)bottom * VM_Window::Display.scaleFactor);
	this->left   = (int)((float)left   * VM_Window::Display.scaleFactor);
	this->right  = (int)((float)right  * VM_Window::Display.scaleFactor);
	this->top    = (int)((float)top    * VM_Window::Display.scaleFactor);
}
