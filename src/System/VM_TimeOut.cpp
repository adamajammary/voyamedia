#include "VM_TimeOut.h"

System::VM_TimeOut::VM_TimeOut(uint32_t timeOut)
{
	this->started   = false;
	this->startTime = 0;
	this->timeOut   = timeOut;
}

bool System::VM_TimeOut::isTimedOut()
{
	return (this->started && ((SDL_GetTicks() - this->startTime) >= this->timeOut));
}

void System::VM_TimeOut::start()
{
	this->startTime = SDL_GetTicks();
	this->started   = true;
}

void System::VM_TimeOut::stop()
{
	this->started = false;
}

int System::VM_TimeOut::InterruptCallback(void* data)
{
	VM_TimeOut* timeOut = static_cast<VM_TimeOut*>(data);

	return ((timeOut != NULL) && timeOut->isTimedOut() ? 1 : 0);
}
