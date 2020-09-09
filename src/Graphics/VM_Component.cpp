#include "VM_Component.h"

using namespace VoyaMedia::System;
using namespace VoyaMedia::XML;

Graphics::VM_Component::VM_Component()
{
	this->active          = false;
	this->activeColor     = {};
	this->backgroundArea  = {};
	this->backgroundColor = {};
	this->borderColor     = {};
	this->borderWidth     = {};
	this->fontSize        = DEFAULT_FONT_SIZE;
	this->id              = "";
	this->margin          = {};
	this->overlayColor    = {};
	this->parent          = NULL;
	this->selected        = false;
	this->visible         = true;
	this->xmlDoc          = NULL;
	this->xmlNode         = NULL;
}

Graphics::VM_Component::~VM_Component()
{
	for (auto button : this->buttons)
		DELETE_POINTER(button);

	this->buttons.clear();

	for (auto child : this->children)
		DELETE_POINTER(child);

	this->children.clear();
}

Graphics::VM_Color Graphics::VM_Component::getColor(const String &attrib)
{
	auto color = VM_GUI::ColorTheme.find(this->id + "." + attrib + "-color");

	if (color != VM_GUI::ColorTheme.end())
		return VM_Graphics::ToVMColor(color->second);

	String colorAttrib = VM_XML::GetAttribute(this->xmlNode, String(attrib + "-color").c_str());

	if (!colorAttrib.empty())
		return VM_Graphics::ToVMColor(colorAttrib);

	if (this->parent != NULL)
		return this->parent->getColor(attrib);

	return VM_Color();
}

int Graphics::VM_Component::render()
{
	if (!this->visible)
		return ERROR_INVALID_ARGUMENTS;

	SDL_Rect area = SDL_Rect(this->backgroundArea);

	area.x += this->borderWidth.left;
	area.y += this->borderWidth.top;
	area.w -= (this->borderWidth.left + this->borderWidth.right);
	area.h -= (this->borderWidth.top  + this->borderWidth.bottom);

	VM_Graphics::FillArea(&this->backgroundColor, &area);
	VM_Graphics::FillBorder(&this->borderColor, &this->backgroundArea, this->borderWidth);

	for (auto button : this->buttons)
		button->render();

	for (auto child : this->children)
		child->render();

	return RESULT_OK;
}

void Graphics::VM_Component::setColors()
{
	this->activeColor     = this->getColor("active");
	this->backgroundColor = this->getColor("background");
	this->borderColor     = this->getColor("border");
	this->highlightColor  = this->getColor("highlight");
	this->overlayColor    = this->getColor("overlay");
}

void Graphics::VM_Component::setPositionAlign(int positionX, int positionY)
{
	String x = VM_XML::GetAttribute(this->xmlNode, "x");
	String y = VM_XML::GetAttribute(this->xmlNode, "y");

	if (x.empty())
		this->backgroundArea.x = positionX;
	else
		this->backgroundArea.x = std::atoi(x.c_str());

	if (y.empty())
		this->backgroundArea.y = positionY;
	else
		this->backgroundArea.y = std::atoi(y.c_str());
}

/**
* @param sizeX	Remaining size horizontally
* @param sizeY	Remaining size vertically
* @param compsX	Remaining components horizontally
* @param compsY	Remaining components vertically
*/
void Graphics::VM_Component::setSizeBlank(int sizeX, int sizeY, int compsX, int compsY)
{
	String width  = VM_XML::GetAttribute(this->xmlNode, "width");
	String height = VM_XML::GetAttribute(this->xmlNode, "height");

	if (width.empty() && (sizeX > 0) && (compsX > 0))
		this->backgroundArea.w = (sizeX / compsX);

	if (height.empty() && (sizeY > 0) && (compsY > 0))
		this->backgroundArea.h = (sizeY / compsY);
}

void Graphics::VM_Component::setSizeFixed()
{
	String width       = VM_XML::GetAttribute(this->xmlNode, "width");
	String height      = VM_XML::GetAttribute(this->xmlNode, "height");
	String borderWidth = VM_XML::GetAttribute(this->xmlNode, "border-width");
	String margin      = VM_XML::GetAttribute(this->xmlNode, "margin");
	String fontSize    = VM_XML::GetAttribute(this->xmlNode, "font-size");

	if (!width.empty() && (width[width.length() - 1] != '%') && (width != "0"))
		this->backgroundArea.w = (int)(std::atof(width.c_str()) * VM_Window::Display.scaleFactor);

	if (!height.empty() && (height[height.length() - 1] != '%') && (height != "0"))
		this->backgroundArea.h = (int)(std::atof(height.c_str()) * VM_Window::Display.scaleFactor);

	if (!borderWidth.empty())
		this->borderWidth = VM_Graphics::ToVMBorder(borderWidth);

	if (!margin.empty())
		this->margin = VM_Graphics::ToVMBorder(margin);

	if (!fontSize.empty())
		this->fontSize = (int)(std::atof(fontSize.c_str()) * VM_Window::Display.scaleFactor);
	else if (this->parent != NULL)
		this->fontSize = (int)((float)DEFAULT_FONT_SIZE * VM_Window::Display.scaleFactor);
}

int Graphics::VM_Component::setSizePercent(VM_Component* parent)
{
	if (parent == NULL)
		return ERROR_INVALID_ARGUMENTS;

	VM_Border parentBorder = parent->borderWidth;
	SDL_Rect  parentArea   = parent->backgroundArea;

	parentArea.w -= (parentBorder.left + parentBorder.right);
	parentArea.h -= (parentBorder.top  + parentBorder.bottom);

	String width  = VM_XML::GetAttribute(this->xmlNode, "width");
	String height = VM_XML::GetAttribute(this->xmlNode, "height");

	if (!width.empty() && (width[width.length() - 1] == '%'))
		this->backgroundArea.w = (parentArea.w * std::atoi(width.c_str()) / 100);

	if (!height.empty() && (height[height.length() - 1] == '%'))
		this->backgroundArea.h = (parentArea.h * std::atoi(height.c_str()) / 100);

	if (width == "height")
		this->backgroundArea.w = this->backgroundArea.h;

	if (height == "width")
		this->backgroundArea.h = this->backgroundArea.w;

	return RESULT_OK;
}
