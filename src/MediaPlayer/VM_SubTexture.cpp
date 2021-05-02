#include "VM_SubTexture.h"

MediaPlayer::VM_SubTexture::VM_SubTexture()
{
	this->locationRender = {};
	this->offsetY        = false;
	this->outline        = NULL;
	this->shadow         = NULL;
	this->subtitle       = NULL;
	this->textureData    = NULL;
	this->total          = {};
}

MediaPlayer::VM_SubTexture::VM_SubTexture(const VM_SubTexture &textureR)
{
	this->locationRender = textureR.locationRender;
	this->offsetY        = textureR.offsetY;
	this->outline        = NULL;
	this->shadow         = NULL;
	this->subtitle       = textureR.subtitle;
	this->textureData    = NULL;
	this->textUTF16      = NULL;
	this->total          = textureR.total;
}

MediaPlayer::VM_SubTexture::VM_SubTexture(VM_Subtitle* subtitle)
{
	this->locationRender = {};
	this->offsetY        = false;
	this->outline        = NULL;
	this->shadow         = NULL;
	this->subtitle       = subtitle;
	this->textureData    = NULL;
	this->textUTF16      = NULL;
	this->total          = {};
}

MediaPlayer::VM_SubTexture::~VM_SubTexture()
{
	DELETE_POINTER(this->outline);
	DELETE_POINTER(this->shadow);
	DELETE_POINTER(this->textureData);
}
