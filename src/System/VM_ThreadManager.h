#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_THREADMANAGER_H
#define VM_THREADMANAGER_H

namespace VoyaMedia
{
	namespace System
	{
		class VM_ThreadManager
		{
		private:
			VM_ThreadManager()  {}
			~VM_ThreadManager() {}

		public:
			static std::mutex                  DBMutex;
			static Graphics::VM_ImageButtonMap ImageButtons;
			static Graphics::VM_ImageMap       Images;
			static SDL_Surface*                ImagePlaceholder;
			static std::mutex                  Mutex;
			static VM_ThreadMap                Threads;

		public:
			static void AddThreads();
			static int  CreateThread(const char* threadName);
			static void FreeResources(bool freeImages = true);
			static void FreeThreads();
			static void FreeThumbnails();
			static int  GetData(void* userData);
			static void HandleData();
			static void HandleImages();
			static int  HandleThreads();
			static bool ThreadsAreRunning();
			static void WaitIfBusy();

		};
	}
}

#endif
