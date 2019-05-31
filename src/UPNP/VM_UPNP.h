#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_UPNP_H
#define VM_UPNP_H

namespace VoyaMedia
{
	namespace UPNP
	{
		class VM_UPNP
		{
		private:
			VM_UPNP()  {}
			~VM_UPNP() {}

		public:
			static LIB_UPNP::UpnpClient_Handle Client;
			static String                      ClientIP;
			static String                      Device;
			static Strings                     Devices;
			static String                      ServerIP;

		private:
			static String                      deviceUDN;
			static std::mutex                  eventMutex;
			static VM_UpnpFiles                files;
			static LIB_UPNP::UpnpDevice_Handle server;
			static uint32_t                    updateID;

		public:
			static int ScanDevices(void* userData);
			static int Start(void* userData);

		private:
			static int     addDevice(const String &mediaURL);
			static int     addFile(const int mediaID, VM_MediaType mediaType);
			static int     addFiles();
			static int     createDescriptionXML(const String &filePath, const String &hostName, const String &deviceUDN);
			static String  getContainerXML(const String &mediaID, const String &parentID, const String &title, const String &filter);
			static String  getItemXML(const VM_UpnpFile* item, const String &itemType, const String &parentID, const String &filter);
			static int     handleEvents(LIB_UPNP::Upnp_EventType eventType, const void* event, void* userData);
			static int     handleEventActionComplete(LIB_UPNP::UpnpActionComplete* event);
			static int     handleEventActionRequest(LIB_UPNP::UpnpActionRequest* event);
			static int     handleEventDiscovery(LIB_UPNP::UpnpDiscovery* event, LIB_UPNP::Upnp_EventType eventType);
			static int     handleEventReceived(LIB_UPNP::UpnpEvent* event);
			static int     handleEventRenewalComplete(LIB_UPNP::UpnpEventSubscribe* event);
			static int     handleEventStateVarComplete(LIB_UPNP::UpnpStateVarComplete* event);
			static int     handleEventStateVarRequest(LIB_UPNP::UpnpStateVarRequest* event);
			static int     handleEventSubscriptionComplete(LIB_UPNP::UpnpEventSubscribe* event);
			static int     handleEventSubscriptionEnd(LIB_UPNP::UpnpEventSubscribe* event);
			static int     handleEventSubscriptionRequest(LIB_UPNP::UpnpSubscriptionRequest* event);
			static int     removeDevice(const String &mediaURL);
			static int     respondBrowse(LIB_UPNP::UpnpActionRequest* event);
			static int     scanFiles(const char* mediaURL);
			static int     stopClient(int result);
			static int     stopServer(int result);
			static int     webServerClose(void* fileHandle, const void* cookie);
			static int     webServerGetInfo(const char* filename, LIB_UPNP::UpnpFileInfo* info, const void* cookie);
			static void*   webServerOpen(const char* filename, LIB_UPNP::UpnpOpenFileMode mode, const void* cookie);
			static int     webServerRead(void* fileHandle, char* buffer, size_t bufferSize, const void* cookie);
			static int     webServerWrite(void* fileHandle, char *buffer, size_t bufferSize, const void* cookie);

			#if defined _windows
				static int webServerSeek(void* fileHandle, int64_t offset, int origin, const void* cookie);
			#else
				static int webServerSeek(void* fileHandle, off_t offset, int origin, const void* cookie);
			#endif

		};
	}
}

#endif
