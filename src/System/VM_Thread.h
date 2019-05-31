#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_THREAD_H
#define VM_THREAD_H

namespace VoyaMedia
{
	namespace System
	{
		struct VM_ThreadData
		{
			StringMap  data;
			WStringMap dataW;
		};

		class VM_Thread
		{
		public:
			VM_Thread();
			VM_Thread(SDL_ThreadFunction function);
			~VM_Thread() {}

		public:
			bool               completed;
			VM_ThreadData*     data;
			SDL_ThreadFunction function;
			std::mutex         mutex;
			bool               refresh;
			bool               start;
			SDL_Thread*        thread;

		};
	}
}

#endif
