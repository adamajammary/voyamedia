#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_TIMEOUT_H
#define VM_TIMEOUT_H

namespace VoyaMedia
{
	namespace System
	{
		class VM_TimeOut
		{
		public:
			VM_TimeOut(uint32_t timeOut = 5000U);

		private:
			bool     started;
			uint32_t startTime;
			uint32_t timeOut;

		public:
			static int InterruptCallback(void* data);
			bool       isTimedOut();
			void       start();
			void       stop();

		};
	}
}

#endif
