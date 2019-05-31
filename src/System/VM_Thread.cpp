#include "VM_Thread.h"

System::VM_Thread::VM_Thread()
{
	this->completed = true;
	this->refresh   = false;
	this->start     = false;
	this->data      = NULL;
	this->function  = NULL;
	this->thread    = NULL;
}

System::VM_Thread::VM_Thread(SDL_ThreadFunction function)
{
	this->completed = true;
	this->refresh   = false;
	this->start     = false;
	this->data      = NULL;
	this->function  = function;
	this->thread    = NULL;
}
