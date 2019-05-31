#include "VM_Panel.h"

Graphics::VM_Panel::VM_Panel(const VM_Component &component)
{
	this->active          = component.active;
	this->activeColor     = component.activeColor;
	this->backgroundArea  = component.backgroundArea;
	this->backgroundColor = component.backgroundColor;
	this->borderColor     = component.borderColor;
	this->borderWidth     = component.borderWidth;
	this->fontSize        = component.fontSize;
	this->id              = component.id;
	this->margin          = component.margin;
	this->parent          = component.parent;
	this->xmlDoc          = component.xmlDoc;
	this->xmlNode         = component.xmlNode;
}

Graphics::VM_Panel::VM_Panel(const String &id)
{
	this->id = id;
}

int Graphics::VM_Panel::render()
{
	VM_Component::render();

	return RESULT_OK;
}
