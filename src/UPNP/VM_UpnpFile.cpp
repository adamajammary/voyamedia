#include "VM_UpnpFile.h"

UPNP::VM_UpnpFile::VM_UpnpFile()
{
	this->file      = "";
	this->id        = "";
	this->name      = "";
	this->title     = "";
	this->type      = "";
	this->url       = "";
	this->iTunes    = false;
	this->duration  = 0;
	this->position  = 0;
	this->size      = 0;
	this->mediaType = MEDIA_TYPE_UNKNOWN;
	this->data      = NULL;
}

UPNP::VM_UpnpFile::~VM_UpnpFile()
{
	CLOSE_FILE(this->data);
}
