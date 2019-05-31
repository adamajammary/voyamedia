#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_BYTES_H
#define VM_BYTES_H

namespace VoyaMedia
{
	namespace System
	{
		class VM_Bytes
		{
		public:
			VM_Bytes();
			~VM_Bytes();

		public:
			uint8_t* data;
			size_t   size;

		public:
			size_t write(void* data, const size_t size);

		};
	}
}

#endif
