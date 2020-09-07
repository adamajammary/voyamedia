#if defined _android

#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_JAVAJNI_H
#define VM_JAVAJNI_H

namespace VoyaMedia
{
	namespace Android
	{
		class VM_JavaJNI
		{
		public:
			VM_JavaJNI();
			~VM_JavaJNI();

		private:
			JavaVM* javaVM;
			jclass  jniClass;
			JNIEnv* jniEnvironment;
			int     result;
			bool    threadAttached;

		public:
			//void    attachThread();
			//void    attachThread(JavaVM* javaVM);
			void    destroy();
			//void    detachThread();
			//void    detachThread(JavaVM* javaVM);
			jclass  getClass();
			JNIEnv* getEnvironment();
			//JavaVM* getJavaVM();
			int     init();
			//bool    isAttached();

		};
	}
}

#endif

#endif
