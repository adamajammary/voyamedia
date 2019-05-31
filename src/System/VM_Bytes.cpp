#include "VM_Bytes.h"

System::VM_Bytes::VM_Bytes()
{
	this->data = NULL;
	this->size = 0;
}

System::VM_Bytes::~VM_Bytes()
{
	FREE_POINTER(this->data);
}

size_t System::VM_Bytes::write(void* data, const size_t size)
{
	if (this->data == NULL)
	{
		this->data = (uint8_t*)std::malloc(size);
		memcpy(this->data, data, size);
		this->size = size;
	}
	else
	{
		this->data = (uint8_t*)std::realloc(this->data, this->size + size);
		memcpy(this->data + this->size, data, size);
		this->size += size;
	}

	return this->size;
}
