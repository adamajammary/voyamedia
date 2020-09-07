#if defined _android

#include "VM_JavaJNI.h"

Android::VM_JavaJNI::VM_JavaJNI()
{
	this->javaVM         = NULL;
	this->jniClass       = NULL;
	this->jniEnvironment = NULL;
	this->result         = -1;
	this->threadAttached = false;
}

Android::VM_JavaJNI::~VM_JavaJNI()
{
	this->destroy();
}

void Android::VM_JavaJNI::attachThread()
{
	this->attachThread(this->javaVM);
}

void Android::VM_JavaJNI::attachThread(JavaVM* javaVM)
{
	if (!this->threadAttached && (javaVM != NULL))
	{
		this->result = javaVM->AttachCurrentThread(&this->jniEnvironment, NULL);

		if ((this->result == 0) && (this->jniEnvironment != NULL))
			this->threadAttached = true;
	}
}

void Android::VM_JavaJNI::destroy()
{
	if ((this->jniEnvironment != NULL) && (this->jniClass != NULL)) {
		this->jniEnvironment->DeleteGlobalRef(this->jniClass);
		this->jniClass = NULL;
	}
}

void Android::VM_JavaJNI::detachThread()
{
	this->detachThread(this->javaVM);
}

void Android::VM_JavaJNI::detachThread(JavaVM* javaVM)
{
	if (this->threadAttached && (javaVM != NULL)) {
		javaVM->DetachCurrentThread();
		this->threadAttached = false;
	}
}

jclass Android::VM_JavaJNI::getClass()
{
	return this->jniClass;
}

JNIEnv* Android::VM_JavaJNI::getEnvironment()
{
	return this->jniEnvironment;
}

JavaVM* Android::VM_JavaJNI::getJavaVM()
{
	return this->javaVM;
}

int Android::VM_JavaJNI::init()
{
	this->jniEnvironment = (JNIEnv*)SDL_AndroidGetJNIEnv();

	if (this->jniEnvironment == NULL)
		return ERROR_UNKNOWN;

	this->jniClass = (jclass)this->jniEnvironment->NewGlobalRef(
		this->jniEnvironment->FindClass("com/voyamedia/android/VoyaMedia")
	);

	if (this->jniClass == NULL)
		return ERROR_UNKNOWN;

	this->jniEnvironment->GetJavaVM(&this->javaVM);

	return RESULT_OK;
}

bool Android::VM_JavaJNI::isAttached()
{
	return this->threadAttached;
}

#endif
