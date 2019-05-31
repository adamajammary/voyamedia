#include "VM_MediaTime.h"

MediaPlayer::VM_MediaTime::VM_MediaTime()
{
	this->hours        = 0;
	this->minutes      = 0;
	this->seconds      = 0;
	this->totalSeconds = 0;
}

MediaPlayer::VM_MediaTime::VM_MediaTime(double seconds)
{
	this->totalSeconds = (const int)seconds;
	this->hours        = (this->totalSeconds / 3600);
	this->minutes      = ((this->totalSeconds % 3600) / 60);
	this->seconds      = ((this->totalSeconds % 3600) % 60);
}
