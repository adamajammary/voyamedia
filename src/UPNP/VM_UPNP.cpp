#include "VM_UPNP.h"

using namespace VoyaMedia::Database;
using namespace VoyaMedia::Graphics;
using namespace VoyaMedia::MediaPlayer;
using namespace VoyaMedia::System;
using namespace VoyaMedia::XML;

LIB_UPNP::UpnpClient_Handle UPNP::VM_UPNP::Client    = -1;
String                      UPNP::VM_UPNP::Device    = "";
Strings                     UPNP::VM_UPNP::Devices;
String                      UPNP::VM_UPNP::deviceUDN = "";
String                      UPNP::VM_UPNP::ClientIP  = "";
std::mutex                  UPNP::VM_UPNP::eventMutex;
UPNP::VM_UpnpFiles          UPNP::VM_UPNP::files;
LIB_UPNP::UpnpDevice_Handle UPNP::VM_UPNP::server    = -1;
String                      UPNP::VM_UPNP::ServerIP  = "";
uint32_t                    UPNP::VM_UPNP::updateID  = 0;

int UPNP::VM_UPNP::addDevice(const String &mediaURL)
{
	LIB_XML::xmlDoc* xmlDocument = VM_XML::Load(mediaURL.c_str());

	if (xmlDocument == NULL)
		return UPNP_E_INVALID_DESC;

	String deviceName = VM_XML::GetValue("/root/device/friendlyName", xmlDocument);
	String deviceType = VM_XML::GetValue("/root/device/deviceType",   xmlDocument);
	String server     = VM_Text::GetUrlRoot(mediaURL);
	String device     = String(mediaURL + "|" + deviceName + " (" + server + ")");

	// DON'T SCAN MY OWN DEVICE (IP) OR ALREADY ADDED DEVICES
	if ((VM_UPNP::ServerIP.empty() || (server.find(VM_UPNP::ServerIP) == String::npos)) &&
		(deviceType.rfind("MediaServer") != String::npos) && !VM_Text::VectorContains(VM_UPNP::Devices, device))
	{
		VM_UPNP::Devices.push_back(device);
	}

	FREE_XML_DOC(xmlDocument);
	LIB_XML::xmlCleanupParser();

	return UPNP_E_SUCCESS;
}

int UPNP::VM_UPNP::addFile(const int mediaID, VM_MediaType mediaType)
{
	if ((mediaID < 1) || (mediaType == MEDIA_TYPE_UNKNOWN))
		return ERROR_UNKNOWN;

	String duration = "";
	String filePath = "";
	String mimeType = "";
	String name     = "";
	String size     = "";

	int  dbResult;
	auto db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

	if (DB_RESULT_OK(dbResult))
	{
		duration = db->getValue(mediaID, "duration");
		filePath = db->getValue(mediaID, "full_path");
		mimeType = db->getValue(mediaID, "mime_type");
		name     = db->getValue(mediaID, "name");
		size     = db->getValue(mediaID, "size");
	}

	DELETE_POINTER(db);

	bool isConcat  = (String(filePath).substr(0, 7) == "concat:");
	bool isDropbox = (filePath.find("dropbox") != String::npos);
	bool isHttp    = VM_FileSystem::IsHttp(filePath);

	if (isConcat || isDropbox || isHttp)
		return ERROR_UNKNOWN;

	char virtualFilePath[DEFAULT_CHAR_BUFFER_SIZE];
	char virtualURL[DEFAULT_CHAR_BUFFER_SIZE];

	VM_UpnpFile* virtualFile = new VM_UpnpFile();

	sprintf(virtualURL,      "http://%s:%u",      LIB_UPNP::UpnpGetServerIpAddress(), LIB_UPNP::UpnpGetServerPort());
	sprintf(virtualFilePath, "/files/file_%d.%s", (int)VM_UPNP::files.size(), VM_FileSystem::GetFileExtension(filePath, false).c_str());

	if (mimeType.empty())
		mimeType = "application/octet-stream";

	virtualFile->duration  = (size_t)std::strtoull(duration.c_str(), NULL, 10);
	virtualFile->file      = filePath;
	virtualFile->id        = String("file_" + std::to_string(VM_UPNP::files.size()));
	virtualFile->mediaType = mediaType;
	virtualFile->name      = String(virtualFilePath);
	virtualFile->size      = (size_t)std::strtoull(size.c_str(), NULL, 10);
	virtualFile->title     = name;
	virtualFile->type      = mimeType;
	virtualFile->url       = String(virtualURL);

	VM_UPNP::files.push_back(virtualFile);

	return UPNP_E_SUCCESS;
}

int UPNP::VM_UPNP::addFiles()
{
	LIB_UPNP::UpnpAddVirtualDir("/files", NULL, NULL);

	while ((VM_GUI::ListTable->getState().dataRequested || VM_GUI::ListTable->getState().dataIsReady) && !VM_Window::Quit)
		SDL_Delay(DELAY_TIME_BACKGROUND);

	for (const auto &row : VM_GUI::ListTable->rows)
	{
		if (!VM_ThreadManager::Threads[THREAD_UPNP_SERVER]->start || VM_Window::Quit)
			break;

		VM_UPNP::addFile(row[0]->mediaID, VM_Top::Selected);
	}

	return UPNP_E_SUCCESS;
}

int UPNP::VM_UPNP::createDescriptionXML(const String &filePath, const String &hostName, const String &deviceUDN)
{
	if (filePath.empty() || hostName.empty() || deviceUDN.empty())
		return ERROR_UNKNOWN;

	std::ofstream descriptionXML(filePath);

	if (!descriptionXML.good())
		return ERROR_UNKNOWN;

	descriptionXML << "<?xml version=\"1.0\"?>";
	descriptionXML << "<root xmlns=\"urn:schemas-upnp-org:device-1-0\" xmlns:sec=\"" + APP_URL + "\" xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">";
	descriptionXML << "  <specVersion><major>1</major><minor>0</minor></specVersion>";
	descriptionXML << "  <device>";
	descriptionXML << "    <friendlyName>" << APP_NAME << " (" << hostName << ")</friendlyName>";
	descriptionXML << "    <manufacturer>" << APP_COMPANY << "</manufacturer>";
	descriptionXML << "    <manufacturerURL>" << APP_URL << "</manufacturerURL>";
	descriptionXML << "    <modelDescription>" << APP_NAME << " [UPnP Media Server]</modelDescription>";
	descriptionXML << "    <modelName>" << APP_NAME << "</modelName>";
	descriptionXML << "    <modelNumber>" << APP_VERSION << "</modelNumber>";
	descriptionXML << "    <modelURL>" << APP_URL << "</modelURL>";
	descriptionXML << "    <UDN>" << deviceUDN << "</UDN>";
	descriptionXML << "    <deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>";
	descriptionXML << "    <dlna:X_DLNADOC xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">DMS-1.50</dlna:X_DLNADOC>";
	descriptionXML << "    <dlna:X_DLNADOC xmlns:dlna=\"urn:schemas-dlna-org:device-1-0\">M-DMS-1.50</dlna:X_DLNADOC>";
	descriptionXML << "    <serviceList>";
	descriptionXML << "      <service>";
	descriptionXML << "        <serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>";
	descriptionXML << "        <serviceId>uurn:upnp-org:serviceId:ContentDirectory</serviceId>";
	descriptionXML << "        <SCPDURL>/ContentDirectory.xml</SCPDURL>";
	descriptionXML << "        <controlURL>/control/ContentDirectory</controlURL>";
	descriptionXML << "        <eventSubURL>/event/ContentDirectory</eventSubURL>";
	descriptionXML << "      </service>";
	descriptionXML << "      <service>";
	descriptionXML << "        <serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>";
	descriptionXML << "        <serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>";
	descriptionXML << "        <SCPDURL>/ConnectionManager.xml</SCPDURL>";
	descriptionXML << "        <controlURL>/control/ConnectionManager</controlURL>";
	descriptionXML << "        <eventSubURL>/event/ConnectionManager</eventSubURL>";
	descriptionXML << "      </service>";
	descriptionXML << "    </serviceList>";
	descriptionXML << "  </device>";
	descriptionXML << "</root>";

	descriptionXML.close();

	return UPNP_E_SUCCESS;
}

String UPNP::VM_UPNP::getContainerXML(const String &mediaID, const String &parentID, const String &title, const String &filter)
{
	if (mediaID.empty() || parentID.empty() || title.empty())
		return "";

	String xml = "";
	xml.append("<container id=\"" + mediaID + "\" parentID=\"" + parentID + "\" restricted=\"1\">");
	xml.append("<dc:title>" + title + "</dc:title>"); 
	xml.append("<upnp:class>object.container</upnp:class>");
	xml.append("</container>");

	return xml;
}

String UPNP::VM_UPNP::getItemXML(const VM_UpnpFile* item, const String &itemType, const String &parentID, const String &filter)
{
	if ((item == NULL) || itemType.empty() || parentID.empty())
		return "";

	VM_MediaTime durationTime = VM_MediaTime((double)item->duration);

	char duration[DEFAULT_CHAR_BUFFER_SIZE];
	snprintf(duration, DEFAULT_CHAR_BUFFER_SIZE, "%02d:%02d:%02d", durationTime.hours, durationTime.minutes, durationTime.seconds);

	String xml = "";

	xml.append("<item id=\"" + item->id + "\" parentID=\"" + parentID + "\" restricted=\"1\">");
	xml.append("<dc:title>" + item->title + "</dc:title>");

	xml.append("<upnp:class>object.item." + itemType + "</upnp:class>");
	xml.append("<res");

	if ((filter == "*") || (filter.find("@duration") != String::npos))
		xml.append(" duration=\"").append(duration).append("\"");

	if ((filter == "*") || (filter.find("@size") != String::npos))
		xml.append(" size=\"" + std::to_string(item->size) + "\"");

	xml.append(" protocolInfo=\"http-get:*:" + item->type + ":" + VM_FileSystem::GetFileDLNA(item->type) + "DLNA.ORG_OP=01;DLNA.ORG_CI=0\"");
	xml.append(">" + item->url + item->name + "</res></item>");

	return xml;
}

int UPNP::VM_UPNP::handleEvents(LIB_UPNP::Upnp_EventType eventType, const void* event, void* userData)
{
	int result = UPNP_E_SUCCESS;

	VM_UPNP::eventMutex.lock();

	switch (eventType) {
	case LIB_UPNP::UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
	case LIB_UPNP::UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
	case LIB_UPNP::UPNP_DISCOVERY_SEARCH_RESULT:
		result = VM_UPNP::handleEventDiscovery((LIB_UPNP::UpnpDiscovery*)event, eventType);
		break;
	case LIB_UPNP::UPNP_DISCOVERY_SEARCH_TIMEOUT:
		break;
	case LIB_UPNP::UPNP_CONTROL_ACTION_REQUEST:
		result = VM_UPNP::handleEventActionRequest((LIB_UPNP::UpnpActionRequest*)event);
		break;
	case LIB_UPNP::UPNP_CONTROL_ACTION_COMPLETE:
		result = VM_UPNP::handleEventActionComplete((LIB_UPNP::UpnpActionComplete*)event);
		break;
	case LIB_UPNP::UPNP_CONTROL_GET_VAR_REQUEST:
		result = VM_UPNP::handleEventStateVarRequest((LIB_UPNP::UpnpStateVarRequest*)event);
		break;
	case LIB_UPNP::UPNP_CONTROL_GET_VAR_COMPLETE:
		result = VM_UPNP::handleEventStateVarComplete((LIB_UPNP::UpnpStateVarComplete*)event);
		break;
		break;
	case LIB_UPNP::UPNP_EVENT_SUBSCRIPTION_REQUEST:
		result = VM_UPNP::handleEventSubscriptionRequest((LIB_UPNP::UpnpSubscriptionRequest*)event);
		break;
	case LIB_UPNP::UPNP_EVENT_RECEIVED:
		result = VM_UPNP::handleEventReceived((LIB_UPNP::UpnpEvent*)event);
		break;
	case LIB_UPNP::UPNP_EVENT_RENEWAL_COMPLETE:
		result = VM_UPNP::handleEventRenewalComplete((LIB_UPNP::UpnpEventSubscribe*)event);
		break;
	case LIB_UPNP::UPNP_EVENT_SUBSCRIBE_COMPLETE:
	case LIB_UPNP::UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
		result = VM_UPNP::handleEventSubscriptionComplete((LIB_UPNP::UpnpEventSubscribe*)event);
		break;
	case LIB_UPNP::UPNP_EVENT_AUTORENEWAL_FAILED:
	case LIB_UPNP::UPNP_EVENT_SUBSCRIPTION_EXPIRED:
		result = VM_UPNP::handleEventSubscriptionEnd((LIB_UPNP::UpnpEventSubscribe*)event);
		break;
	}

	VM_UPNP::eventMutex.unlock();

	return result;
}

int UPNP::VM_UPNP::handleEventActionComplete(LIB_UPNP::UpnpActionComplete* event)
{
	if (event == NULL)
		return UPNP_E_INVALID_ARGUMENT;

	int result = LIB_UPNP::UpnpActionComplete_get_ErrCode(event);

	if (result != UPNP_E_SUCCESS)
		return result;

	#ifdef _DEBUG
		LOG(
			"\nHandleEventActionComplete\n"
			"\tErrCode     =  %d\n"
			"\tCtrlUrl     =  %s\n",
			result,
			LIB_UPNP::UpnpActionComplete_get_CtrlUrl_cstr(event)
		);

		LIB_UPNP::IXML_Document* actionRequest = LIB_UPNP::UpnpActionComplete_get_ActionRequest(event);

		if (actionRequest != NULL)
		{
			char* xmlbuff = LIB_UPNP::ixmlPrintNode(reinterpret_cast<LIB_UPNP::IXML_Node*>(actionRequest));

			if (xmlbuff != NULL) {
				LOG("\tActRequest  =  %s\n", xmlbuff);
				LIB_UPNP::ixmlFreeDOMString(xmlbuff);
			}
		} else {
			LOG("\tActRequest  =  null\n");
		}

		LIB_UPNP::IXML_Document* actionResult = LIB_UPNP::UpnpActionComplete_get_ActionResult(event);

		if (actionResult != NULL)
		{
			char* xmlbuff = LIB_UPNP::ixmlPrintNode(reinterpret_cast<LIB_UPNP::IXML_Node*>(actionResult));

			if (xmlbuff != NULL) {
				LOG("\tActResult   =  %s\n", xmlbuff);
				LIB_UPNP::ixmlFreeDOMString(xmlbuff);
			}
		} else {
			LOG("\tActResult   =  null\n");
		}
	#endif

	return result;
}

int UPNP::VM_UPNP::handleEventActionRequest(LIB_UPNP::UpnpActionRequest* event)
{
	if (event == NULL)
		return UPNP_E_INVALID_ARGUMENT;

	int result = LIB_UPNP::UpnpActionRequest_get_ErrCode(event);

	if (result != UPNP_E_SUCCESS)
		return result;

	const char* actionName = LIB_UPNP::UpnpActionRequest_get_ActionName_cstr(event);
	const char* serviceID  = LIB_UPNP::UpnpActionRequest_get_ServiceID_cstr(event);
	const char* deviceUDN  = LIB_UPNP::UpnpActionRequest_get_DevUDN_cstr(event);

	#ifdef _DEBUG
		LOG(
			"\nHandleEventActionRequest\n"
			"\tErrCode     =  %d\n"
			"\tErrStr      =  %s\n"
			"\tActionName  =  %s\n"
			"\tUDN         =  %s\n"
			"\tServiceID   =  %s\n",
			result, LIB_UPNP::UpnpActionRequest_get_ErrStr_cstr(event), actionName, deviceUDN, serviceID
		);

		LIB_UPNP::IXML_Document* actionRequestDoc = LIB_UPNP::UpnpActionRequest_get_ActionRequest(event);

		if (actionRequestDoc != NULL)
		{
			char* xmlbuff = ixmlPrintNode(reinterpret_cast<LIB_UPNP::IXML_Node*>(actionRequestDoc));
			if (xmlbuff != NULL) {
				LOG("\tActRequest  =  %s\n", xmlbuff);
				LIB_UPNP::ixmlFreeDOMString(xmlbuff);
			}
		} else {
			LOG("\tActRequest  =  null\n");
		}

		LIB_UPNP::IXML_Document* actionResultDoc = LIB_UPNP::UpnpActionRequest_get_ActionResult(event);

		if (actionResultDoc != NULL)
		{
			char* xmlbuff = ixmlPrintNode(reinterpret_cast<LIB_UPNP::IXML_Node*>(actionResultDoc));
			if (xmlbuff != NULL) {
				LOG("\tActResult   =  %s\n", xmlbuff);
				LIB_UPNP::ixmlFreeDOMString(xmlbuff);
			}
		} else {
			LOG("\tActResult   =  null\n");
		}
	#endif

	//
	// https://github.com/nofxx/fuppes/blob/master/src/lib/ConnectionManager/ConnectionManager.cpp
	//

	if (strcmp(deviceUDN, VM_UPNP::deviceUDN.c_str()) != 0) {
		LIB_UPNP::UpnpActionRequest_set_ActionResult(event, NULL);
		return UPNP_E_INVALID_DEVICE;
	}

	// ConnectionManager
	if (strstr(serviceID, "ConnectionManager") != NULL)
	{
		// GetCurrentConnectionIDs
		if (strcmp(actionName, "GetCurrentConnectionIDs") == 0)
		{
			LIB_UPNP::UpnpActionRequest_set_ActionResult(
				event, LIB_UPNP::UpnpMakeActionResponse(actionName, strcat(const_cast<char*>(serviceID), ":1"), 1, "ConnectionIDs", "0")
			);

			return UPNP_E_SUCCESS;
		}
		// GetCurrentConnectionInfo
		else if (strcmp(actionName, "GetCurrentConnectionInfo") == 0)
		{
			LIB_UPNP::UpnpActionRequest_set_ActionResult(
				event, LIB_UPNP::UpnpMakeActionResponse(
					actionName, strcat(const_cast<char*>(serviceID), ":1"), 7,
					"RcsID",                 "-1",
					"AVTransportID",         "-1",
					"ProtocolInfo",          "",
					"PeerConnectionManager", "",
					"PeerConnectionID",      "-1",
					"Direction",             "Output",
					"Status",                "Unknown"
				)
			);

			return UPNP_E_SUCCESS;
		}
		// GetProtocolInfo
		else if (strcmp(actionName, "GetProtocolInfo") == 0)
		{
			String protocolInfo = "";

			for (const auto &i : VM_FileSystem::MediaFileDLNAs)
				protocolInfo.append("http-get:*:" + i.first + ":" + i.second + "DLNA.ORG_OP=01;DLNA.ORG_CI=0,");

			protocolInfo = protocolInfo.substr(0, protocolInfo.size() - 1);

			LIB_UPNP::UpnpActionRequest_set_ActionResult(
				event, LIB_UPNP::UpnpMakeActionResponse(actionName, strcat(const_cast<char*>(serviceID), ":1"), 2, "Source", protocolInfo.c_str(), "Sink", "")
			);

			return UPNP_E_SUCCESS;
		}
	}
	// ContentDirectory
	else if (strstr(serviceID, "ContentDirectory") != NULL)
	{
		// Browse
		if (strcmp(actionName, "Browse") == 0)
		{
			return VM_UPNP::respondBrowse(event);
		}
		// GetSearchCapabilities
		else if (strcmp(actionName, "GetSearchCapabilities") == 0)
		{
			LIB_UPNP::UpnpActionRequest_set_ActionResult(
				event, LIB_UPNP::UpnpMakeActionResponse(actionName, strcat(const_cast<char*>(serviceID), ":1"), 1, "SearchCaps", "")
			);

			return UPNP_E_SUCCESS;
		}
		// GetSortCapabilities
		else if (strcmp(actionName, "GetSortCapabilities") == 0)
		{
			LIB_UPNP::UpnpActionRequest_set_ActionResult(
				event, LIB_UPNP::UpnpMakeActionResponse(actionName, strcat(const_cast<char*>(serviceID), ":1"), 1, "SortCaps", "")
			);

			return UPNP_E_SUCCESS;
		}
		// GetSystemUpdateID
		else if (strcmp(actionName, "GetSystemUpdateID") == 0)
		{
			LIB_UPNP::UpnpActionRequest_set_ActionResult(
				event, LIB_UPNP::UpnpMakeActionResponse(actionName, strcat(const_cast<char*>(serviceID), ":1"), 1, "Id", std::to_string(VM_UPNP::updateID).c_str())
			);

			return UPNP_E_SUCCESS;
		}
		// Search
		else if (strcmp(actionName, "Search") == 0)
		{
			// NOT SUPPORTED
		}
	}

	LIB_UPNP::UpnpActionRequest_set_ActionResult(event, NULL);

	return UPNP_E_INVALID_ACTION;
}

int UPNP::VM_UPNP::handleEventDiscovery(LIB_UPNP::UpnpDiscovery* event, LIB_UPNP::Upnp_EventType eventType)
{
	if (event == NULL)
		return UPNP_E_INVALID_ARGUMENT;

	int result = LIB_UPNP::UpnpDiscovery_get_ErrCode(event);

	if (result != UPNP_E_SUCCESS)
		return result;

	#ifdef _DEBUG
		switch (eventType) {
		case LIB_UPNP::UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
			LOG("UPNP_DISCOVERY_ADVERTISEMENT_ALIVE");
			break;
		case LIB_UPNP::UPNP_DISCOVERY_SEARCH_RESULT:
			LOG("UPNP_DISCOVERY_SEARCH_RESULT");
			break;
		case LIB_UPNP::UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
			LOG("UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE");
			break;
		}

		LOG(
			"\nHandleEventDiscovery\n"
			"\tErrCode     =  %d\n"
			"\tExpires     =  %d\n"
			"\tDeviceId    =  %s\n"
			"\tDeviceType  =  %s\n"
			"\tServiceType =  %s\n"
			"\tServiceVer  =  %s\n"
			"\tLocation    =  %s\n"
			"\tOS          =  %s\n"
			"\tDate        =  %s\n"
			"\tExt         =  %s\n",
			result,
			LIB_UPNP::UpnpDiscovery_get_Expires(event),
			LIB_UPNP::UpnpDiscovery_get_DeviceID_cstr(event),
			LIB_UPNP::UpnpDiscovery_get_DeviceType_cstr(event),
			LIB_UPNP::UpnpDiscovery_get_ServiceType_cstr(event),
			LIB_UPNP::UpnpDiscovery_get_ServiceVer_cstr(event),
			LIB_UPNP::UpnpDiscovery_get_Location_cstr(event),
			LIB_UPNP::UpnpDiscovery_get_Os_cstr(event),
			LIB_UPNP::UpnpDiscovery_get_Date_cstr(event),
			LIB_UPNP::UpnpDiscovery_get_Ext_cstr(event)
		);
	#endif

	switch (eventType) {
	case LIB_UPNP::UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
	case LIB_UPNP::UPNP_DISCOVERY_SEARCH_RESULT:
		result = VM_UPNP::addDevice(LIB_UPNP::UpnpDiscovery_get_Location_cstr((LIB_UPNP::UpnpDiscovery*)event));
		break;
	case LIB_UPNP::UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
		result = VM_UPNP::removeDevice(LIB_UPNP::UpnpDiscovery_get_Location_cstr((LIB_UPNP::UpnpDiscovery*)event));
		break;
	}

	return result;
}

int UPNP::VM_UPNP::handleEventReceived(LIB_UPNP::UpnpEvent* event)
{
	if (event == NULL)
		return UPNP_E_INVALID_ARGUMENT;

	#ifdef _DEBUG
		char* xmlbuff = LIB_UPNP::ixmlPrintNode(reinterpret_cast<LIB_UPNP::IXML_Node*>(LIB_UPNP::UpnpEvent_get_ChangedVariables(event)));

		LOG(
			"\nHandleEventReceived\n"
			"\tSID         =  %s\n"
			"\tEventKey    =  %d\n"
			"\tChangedVars =  %s\n",
			LIB_UPNP::UpnpEvent_get_SID_cstr(event),
			LIB_UPNP::UpnpEvent_get_EventKey(event),
			xmlbuff
		);

		LIB_UPNP::ixmlFreeDOMString(xmlbuff);
	#endif

	return UPNP_E_SUCCESS;
}

int UPNP::VM_UPNP::handleEventRenewalComplete(LIB_UPNP::UpnpEventSubscribe* event)
{
	if (event == NULL)
		return UPNP_E_INVALID_ARGUMENT;

	int result = LIB_UPNP::UpnpEventSubscribe_get_ErrCode(event);

	if (result != UPNP_E_SUCCESS)
		return result;

	#ifdef _DEBUG
		LOG(
			"\nHandleEventRenewalComplete\n"
			"\tSID         =  %s\n"
			"\tErrCode     =  %d\n"
			"\tTimeOut     =  %d\n",
			LIB_UPNP::UpnpEventSubscribe_get_SID_cstr(event),
			result,
			LIB_UPNP::UpnpEventSubscribe_get_TimeOut(event)
		);
	#endif

	return result;
}

int UPNP::VM_UPNP::handleEventStateVarComplete(LIB_UPNP::UpnpStateVarComplete* event)
{
	if (event == NULL)
		return UPNP_E_INVALID_ARGUMENT;

	int result = LIB_UPNP::UpnpStateVarComplete_get_ErrCode(event);

	if (result != UPNP_E_SUCCESS)
		return result;

	#ifdef _DEBUG
		LOG(
			"\nHandleEventStateVarComplete\n"
			"\tErrCode     =  %d\n"
			"\tCtrlUrl     =  %s\n"
			"\tStateVarName=  %s\n"
			"\tCurrentVal  =  %s\n",
			result,
			LIB_UPNP::UpnpStateVarComplete_get_CtrlUrl_cstr(event),
			LIB_UPNP::UpnpStateVarComplete_get_StateVarName_cstr(event),
			LIB_UPNP::UpnpStateVarComplete_get_CurrentVal(event)
		);
	#endif

	return result;
}

int UPNP::VM_UPNP::handleEventStateVarRequest(LIB_UPNP::UpnpStateVarRequest* event)
{
	if (event == NULL)
		return UPNP_E_INVALID_ARGUMENT;

	int result = LIB_UPNP::UpnpStateVarRequest_get_ErrCode(event);

	if (result != UPNP_E_SUCCESS)
		return result;

	#ifdef _DEBUG
		LOG(
			"\nHandleEventStateVarRequest\n"
			"\tErrCode     =  %d\n"
			"\tErrStr      =  %s\n"
			"\tUDN         =  %s\n"
			"\tServiceID   =  %s\n"
			"\tStateVarName=  %s\n"
			"\tCurrentVal  =  %s\n",
			result,
			LIB_UPNP::UpnpStateVarRequest_get_ErrStr_cstr(event),
			LIB_UPNP::UpnpStateVarRequest_get_DevUDN_cstr(event),
			LIB_UPNP::UpnpStateVarRequest_get_ServiceID_cstr(event),
			LIB_UPNP::UpnpStateVarRequest_get_StateVarName_cstr(event),
			LIB_UPNP::UpnpStateVarRequest_get_CurrentVal(event)
		);
	#endif

	return result;
}

int UPNP::VM_UPNP::handleEventSubscriptionComplete(LIB_UPNP::UpnpEventSubscribe* event)
{
	if (event == NULL)
		return UPNP_E_INVALID_ARGUMENT;

	int result = LIB_UPNP::UpnpEventSubscribe_get_ErrCode(event);

	if (result != UPNP_E_SUCCESS)
		return result;

	#ifdef _DEBUG
		LOG(
			"\nHandleEventSubscriptionComplete\n"
			"\tSID         =  %s\n"
			"\tErrCode     =  %d\n"
			"\tPublisherURL=  %s\n"
			"\tTimeOut     =  %d\n",
			LIB_UPNP::UpnpEventSubscribe_get_SID_cstr(event),
			result,
			LIB_UPNP::UpnpEventSubscribe_get_PublisherUrl_cstr(event),
			LIB_UPNP::UpnpEventSubscribe_get_TimeOut(event)
		);
	#endif

	return UPNP_E_SUCCESS;
}

int UPNP::VM_UPNP::handleEventSubscriptionEnd(LIB_UPNP::UpnpEventSubscribe* event)
{
	if (event == NULL)
		return UPNP_E_INVALID_ARGUMENT;

	int result = LIB_UPNP::UpnpEventSubscribe_get_ErrCode(event);

	if (result != UPNP_E_SUCCESS)
		return result;

	#ifdef _DEBUG
		LOG(
			"\nHandleEventSubscriptionEnd\n"
			"\tSID         =  %s\n"
			"\tErrCode     =  %d\n"
			"\tPublisherURL=  %s\n"
			"\tTimeOut     =  %d\n",
			LIB_UPNP::UpnpEventSubscribe_get_SID_cstr(event),
			result,
			LIB_UPNP::UpnpEventSubscribe_get_PublisherUrl_cstr(event),
			LIB_UPNP::UpnpEventSubscribe_get_TimeOut(event)
		);
	#endif
	 
	return result;
}

int UPNP::VM_UPNP::handleEventSubscriptionRequest(LIB_UPNP::UpnpSubscriptionRequest* event)
{
	if (event == NULL)
		return UPNP_E_INVALID_ARGUMENT;

	const char* udn       = LIB_UPNP::UpnpSubscriptionRequest_get_UDN_cstr(event);
	const char* serviceID = LIB_UPNP::UpnpSubscriptionRequest_get_ServiceId_cstr(event);
	const char* sid       = LIB_UPNP::UpnpSubscriptionRequest_get_SID_cstr(event);

	#ifdef _DEBUG
		LOG(
			"\nHandleEventSubscriptionRequest\n"
			"\tServiceID   =  %s\n"
			"\tUDN         =  %s\n"
			"\tSID         =  %s\n",
			serviceID, udn, sid
		);
	#endif

	int result = LIB_UPNP::UpnpAcceptSubscription(VM_UPNP::server, udn, serviceID, NULL, NULL, 0, sid);

	#ifdef _DEBUG
		LOG("\n\tUpnpAcceptSubscription\n\tresult=%d", result);
	#endif

	return result;
}

int UPNP::VM_UPNP::removeDevice(const String &mediaURL)
{
	LIB_XML::xmlDoc* xmlDocument = VM_XML::Load(mediaURL.c_str());

	if (xmlDocument == NULL)
		return UPNP_E_INVALID_DESC;

	String deviceName = VM_XML::GetValue("/root/device/friendlyName", xmlDocument);
	String server     = VM_Text::GetUrlRoot(mediaURL);
	String device     = String(mediaURL + "|" + deviceName + " (" + server + ")");

	auto deviceIter = std::find(VM_UPNP::Devices.begin(), VM_UPNP::Devices.end(), device);

	if (deviceIter != VM_UPNP::Devices.end())
		VM_UPNP::Devices.erase(deviceIter);

	FREE_XML_DOC(xmlDocument);
	LIB_XML::xmlCleanupParser();

	return UPNP_E_SUCCESS;
}

int UPNP::VM_UPNP::respondBrowse(LIB_UPNP::UpnpActionRequest* event)
{
	// http://www.nt.uni-saarland.de/fileadmin/file_uploads/theses/bachelor/bachelor-thesis-final_01.pdf

	if (event == NULL)
		return UPNP_E_INVALID_ARGUMENT;

	int result = LIB_UPNP::UpnpActionRequest_get_ErrCode(event);

	if (result != UPNP_E_SUCCESS)
		return result;

	LIB_UPNP::IXML_Document* actionRequest = UpnpActionRequest_get_ActionRequest(event);

	if ((actionRequest == NULL) || (actionRequest->n.firstChild == NULL) || (actionRequest->n.firstChild->firstChild == NULL)) {
		LIB_UPNP::UpnpActionRequest_set_ActionResult(event, NULL);
		return result;
	}

	// OBJECT NODE
	LIB_UPNP::Nodeptr objectNode = actionRequest->n.firstChild->firstChild;

	// OBJECT_ID
	String objectID = "";

	if (objectNode->firstChild != NULL) {
		objectID = String(objectNode->firstChild->nodeValue);
	} else {
		LIB_UPNP::UpnpActionRequest_set_ActionResult(event, NULL);
		return result;
	}

	// BROWSE FLAG
	String browseFlag = "";

	if ((objectNode->nextSibling != NULL) && (objectNode->nextSibling->firstChild != NULL)) {
		browseFlag = String(objectNode->nextSibling->firstChild->nodeValue);
	} else {
		LIB_UPNP::UpnpActionRequest_set_ActionResult(event, NULL);
		return result;
	}

	// FILTER
	String filter = "*";

	if ((objectNode->nextSibling->nextSibling != NULL) && (objectNode->nextSibling->nextSibling->firstChild != NULL))
		filter = String(objectNode->nextSibling->nextSibling->firstChild->nodeValue);

	// STARTING INDEX (OFFSET)
	uint32_t offset = 0;

	if ((objectNode->nextSibling->nextSibling->nextSibling != NULL) && (objectNode->nextSibling->nextSibling->nextSibling->firstChild != NULL))
		offset = std::atoi(objectNode->nextSibling->nextSibling->nextSibling->firstChild->nodeValue);

	// REQUESTED COUNT (MAX)
	uint32_t maxFiles = 0;

	if ((objectNode->nextSibling->nextSibling->nextSibling->nextSibling != NULL) && (objectNode->nextSibling->nextSibling->nextSibling->nextSibling->firstChild != NULL))
		maxFiles = std::atoi(objectNode->nextSibling->nextSibling->nextSibling->nextSibling->firstChild->nodeValue);

	if (maxFiles == 0)
		maxFiles = MAX_DB_RESULT;

	String   itemType     = "";
	uint32_t nrReturned   = 0;
	uint32_t totalMatches = 0;
	String   xml          = "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0/\" xmlns:sec=\"" + APP_URL + "\">";

	// BROWSE METADATA
	if (browseFlag == "BrowseMetadata") {
		// ROOT CONTAINER
		if (objectID == "0") {
			xml.append(VM_UPNP::getContainerXML("0", "-1", "Root", filter));
		// MEDIA ITEMS
		} else {
			for (int i = 0; i < (int)VM_UPNP::files.size(); i++)
			{
				if (VM_UPNP::files[i]->id != objectID)
					continue;

				switch (VM_UPNP::files[i]->mediaType) {
					case MEDIA_TYPE_AUDIO:   itemType = "audioItem"; break;
					case MEDIA_TYPE_PICTURE: itemType = "imageItem"; break;
					case MEDIA_TYPE_VIDEO:   itemType = "videoItem"; break;
					default: continue;
				}

				xml.append(VM_UPNP::getItemXML(VM_UPNP::files[i], itemType, "0", filter));

				break;
			}
		}

		nrReturned   = 1;
		totalMatches = nrReturned;
	// BROWSE DIRECT CHILDREN
	} else if (browseFlag == "BrowseDirectChildren") {
		// MEDIA CONTAINERS
		if (objectID == "0") {
			uint32_t offsetIndex = 0;

			for (int i = 0; i < (int)VM_UPNP::files.size(); i++)
			{
				// SKIP FILES NOT MATCHING REQUESTED MEDIA TYPE
				switch (VM_UPNP::files[i]->mediaType) {
					case MEDIA_TYPE_AUDIO:   itemType = "audioItem"; break;
					case MEDIA_TYPE_PICTURE: itemType = "imageItem"; break;
					case MEDIA_TYPE_VIDEO:   itemType = "videoItem"; break;
					default: continue;
				}

				// SKIP OFFSET AND RESTRICT RETURNED RESULT IF REQUESTED
				if ((offsetIndex >= offset) && (nrReturned < maxFiles)) {
					xml.append(VM_UPNP::getItemXML(VM_UPNP::files[i], itemType, objectID, filter));
					nrReturned++;
				}

				totalMatches++;
				offsetIndex++;
			}
		}
	} else {
		LIB_UPNP::UpnpActionRequest_set_ActionResult(event, NULL);

		return result;
	}

	// DIDL-LITE TRAILER
	xml.append("</DIDL-Lite>");

	LIB_UPNP::UpnpActionRequest_set_ActionResult(
		event,
		LIB_UPNP::UpnpMakeActionResponse(
			LIB_UPNP::UpnpActionRequest_get_ActionName_cstr(event), UPNP_SERVICE_TYPE, 4,
			"Result",         xml.c_str(),
			"NumberReturned", std::to_string(nrReturned).c_str(),
			"TotalMatches",   std::to_string(totalMatches).c_str(),
			"UpdateID",       std::to_string(VM_UPNP::updateID).c_str()
		)
	);

	return UPNP_E_SUCCESS;
}

int UPNP::VM_UPNP::ScanDevices(void* userData)
{
	VM_ThreadManager::Threads[THREAD_UPNP_CLIENT]->completed = false;

	snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s", VM_Window::Labels["status.scan.upnp"].c_str());

	// INITIALIZE THE VM_UPNP LIBRARY
	int result = LIB_UPNP::UpnpInit2(VM_UPNP::ClientIP.c_str(), 0);

	if ((result != UPNP_E_SUCCESS) && (result != UPNP_E_INIT)) {
		VM_UPNP::stopClient(result);
		return result;
	}

	// REGISTER THIS DEVICE AS A UPNP CLIENT WITH A SPECIFIED CALLBACK-FUNCTION/EVENT-HANDLER
	result = LIB_UPNP::UpnpRegisterClient(VM_UPNP::handleEvents, NULL, &VM_UPNP::Client);

	if (result != UPNP_E_SUCCESS) {
		VM_UPNP::stopClient(result);
		return result;
	}

	LIB_UPNP::UpnpSetMaxContentLength(0);

	// SCAN FOR VM_UPNP DEVICES ON MY NETWORK
	for (int i = 0; i < UPNP_SEARCH_MAX_COUNT; i++)
	{
		if (VM_Window::Quit)
			break;

		snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "[%d%%] %s [%s]", ((i * 100) / UPNP_SEARCH_MAX_COUNT), VM_Window::Labels["status.scan.upnp"].c_str(), VM_UPNP::ClientIP.c_str());

		if (LIB_UPNP::UpnpSearchAsync(VM_UPNP::Client, UPNP_SEARCH_TIMEOUT, "upnp:rootdevice", NULL) != UPNP_E_SUCCESS)
			break;

		for (int time = 0; time < UPNP_DELAY_TIME && !VM_Window::Quit; time += DELAY_TIME_DEFAULT)
			SDL_Delay(DELAY_TIME_DEFAULT);
	}

	if (VM_Window::Quit) {
		VM_UPNP::stopClient(UPNP_E_SUCCESS);
		return UPNP_E_SUCCESS;
	}

	snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "[100%%] %s [%s]", VM_Window::Labels["status.scan.upnp"].c_str(), VM_UPNP::ClientIP.c_str());

	if (VM_UPNP::Devices.empty()) {
		VM_UPNP::stopClient(UPNP_E_NO_WEB_SERVER);
		return result;
	}

	// WAIT FOR USER TO SELECT A DEVICE
	VM_Modal::Open("modal_upnp_devices");

	while (VM_Modal::IsVisible() && !VM_Window::Quit)
		SDL_Delay(DELAY_TIME_DEFAULT);

	if (!VM_UPNP::Device.empty())
	{
		snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s %s [UPnP]", VM_Window::Labels["status.scan"].c_str(), VM_Text::GetUrlRoot(VM_UPNP::Device).c_str());

		if (VM_UPNP::scanFiles(VM_UPNP::Device.c_str()) <= 0) {
			VM_UPNP::stopClient(UPNP_E_FILE_NOT_FOUND);
			return result;
		}
	}

	VM_UPNP::stopClient(UPNP_E_SUCCESS);

	return UPNP_E_SUCCESS;
}

// RETURNS: NR OF FILES ADDED TO THE DB
int UPNP::VM_UPNP::scanFiles(const char* mediaURL)
{
	int              dbResult;
	VM_Database*     db;
	int              filesAdded  = 0;
	LIB_XML::xmlDoc* xmlDocument = VM_XML::Load(mediaURL);
	String           deviceName  = VM_XML::GetValue("/root/device/friendlyName", xmlDocument);
	String           server      = VM_Text::GetUrlRoot(mediaURL);
	VM_XmlNodes      services    = VM_XML::GetNodes("/root/device/serviceList/service", xmlDocument);

	snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s %s [UPnP]", VM_Window::Labels["status.scan"].c_str(), String(deviceName + " (" + server + ")").c_str());

	for (auto service : services)
	{
		String serviceType = VM_XML::GetChildValue(service, "serviceType", xmlDocument);

		if (serviceType.rfind("ContentDirectory") == String::npos)
			continue;

		String controlURL = VM_XML::GetChildValue(service, "controlURL", xmlDocument);

		if (controlURL.empty())
			continue;

		controlURL = String(server + (controlURL[0] != '/' ? "/" : "") + controlURL);

		Strings containerIDs = { "0" };

		for (int i = 0; i < (int)containerIDs.size(); i++)
		{
			LIB_UPNP::IXML_Document* actionDocument = LIB_UPNP::UpnpMakeAction(
				"Browse",         serviceType.c_str(), 6,
				"ObjectID",       containerIDs[i].c_str(),
				"BrowseFlag",     "BrowseDirectChildren",
				"Filter",         "*",
				"StartingIndex",  "0",
				"RequestedCount", "0",
				"SortCriteria",   ""
			);

			LIB_UPNP::IXML_Document* responseDocument = NULL;
			String                   responseText     = "";

			int result = UpnpSendAction(VM_UPNP::Client, controlURL.c_str(), serviceType.c_str(), NULL, actionDocument, &responseDocument);

			LIB_UPNP::ixmlDocument_free(actionDocument);

			if ((result == UPNP_E_SUCCESS) && (responseDocument != NULL))
			{
				LIB_UPNP::IXML_Node* responseNode = responseDocument->n.firstChild;

				while (responseNode != NULL)
				{
					if ((responseNode->parentNode != NULL) && (strcmp(responseNode->parentNode->nodeName, "Result") == 0)) {
						responseText = String(responseNode->nodeValue);
						break;
					}

					responseNode = responseNode->firstChild;
				}

				LIB_UPNP::ixmlDocument_free(responseDocument);
			}

			if (responseText.empty())
				continue;

			LIB_XML::xmlDoc* xmlResponse = VM_XML::Load(responseText.c_str(), (int)responseText.size());
			VM_XmlNodes      containers  = VM_XML::GetNodes("/DIDL-Lite/container", xmlResponse);
			VM_XmlNodes      items       = VM_XML::GetNodes("/DIDL-Lite/item", xmlResponse);

			for (auto container : containers)
				containerIDs.push_back(VM_XML::GetAttribute(container, "id"));

			for (auto item : items)
			{
				String path = VM_XML::GetChildValue(item, "res", xmlDocument);

				if (!VM_FileSystem::IsMediaFile(path))
					continue;

				snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s '%s'", VM_Window::Labels["status.adding"].c_str(), path.c_str());

				db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

				int mediaID = 0;

				if (DB_RESULT_OK(dbResult))
					mediaID = db->getID(path);

				DELETE_POINTER(db);

				if (mediaID > 0) {
					snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s '%s'", VM_Window::Labels["status.already_added"].c_str(), path.c_str());
					continue;
				}

				// MEDIA TYPE
				VM_MediaType mediaType  = MEDIA_TYPE_UNKNOWN;
				String       mediaTypeS = VM_XML::GetChildValue(item, "class", xmlDocument);
				size_t       fileSize   = (size_t)std::strtoull(VM_XML::GetAttribute(VM_XML::GetChildNode(item, "res", xmlDocument), "size").c_str(), NULL, 10);
				bool         validMedia = false;

				if (VM_FileSystem::IsPicture(path))
				{
					mediaType  = MEDIA_TYPE_PICTURE;
					validMedia = true;
				}
				else
				{
					LIB_FFMPEG::AVFormatContext* formatContext = VM_FileSystem::GetMediaFormatContext(path, false);

					mediaType = VM_FileSystem::GetMediaType(formatContext);

					// GET MEDIA TYPE, AND PERFORM EXTRA VALIDATIONS
					validMedia = ((mediaType == MEDIA_TYPE_AUDIO) || (mediaType == MEDIA_TYPE_VIDEO));

					// WMA DRM
					if ((formatContext != NULL) && VM_FileSystem::IsDRM(formatContext->metadata))
						validMedia = false;

					FREE_AVFORMAT(formatContext);
				}

				if (!validMedia) {
					snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s '%s'", VM_Window::Labels["error.add"].c_str(), path.c_str());
					continue;
				}

				String fileName = ("[UPnP] " + VM_XML::GetChildValue(item, "title", xmlDocument));
				String fileMIME = VM_FileSystem::GetFileMIME(path);

				db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

				if (DB_RESULT_OK(dbResult))
					dbResult = db->addFile(path, fileName, ("[UPnP] " + deviceName), fileSize, mediaType, fileMIME);

				DELETE_POINTER(db);

				if (DB_RESULT_OK(dbResult))
				{
					filesAdded++;

					snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s '%s'", VM_Window::Labels["status.added"].c_str(), fileName.c_str());
				} else {
					snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s '%s'", VM_Window::Labels["error.add"].c_str(), fileName.c_str());
				}
			}

			FREE_XML_DOC(xmlResponse);
		}

		break;
	}

	FREE_XML_DOC(xmlDocument);
	LIB_XML::xmlCleanupParser();

	VM_GUI::ListTable->refreshRows();

	return filesAdded;
}

int UPNP::VM_UPNP::Start(void* userData)
{
	VM_ThreadManager::Threads[THREAD_UPNP_SERVER]->completed = false;

	snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s UPnP Server [%s] ...", VM_Window::Labels["upnp.starting"].c_str(), VM_UPNP::ServerIP.c_str());

	// INITIALIZE THE UPNP LIBRARY
	int result = LIB_UPNP::UpnpInit2(VM_UPNP::ServerIP.c_str(), 0);

	if ((result != UPNP_E_SUCCESS) && (result != UPNP_E_INIT)) {
		VM_UPNP::stopServer(result);
		return result;
	}

	// GENERATE UNIQUE DEVICE ID
	char hostName[DEFAULT_CHAR_BUFFER_SIZE];
	gethostname(hostName, DEFAULT_CHAR_BUFFER_SIZE);

	VM_UPNP::deviceUDN = String("uuid:UPnP-" + APP_NAME + "-" + APP_VERSION + "-").append(VM_UPNP::ServerIP).append("-").append(hostName);

	// CREATE DESCRIPTION XML
	String webServerPath = String(VM_Window::WorkingDirectory).append(PATH_SEPERATOR).append("web");
	result = VM_UPNP::createDescriptionXML(String(webServerPath).append(PATH_SEPERATOR).append("description.xml"), hostName, VM_UPNP::deviceUDN);

	if (result != RESULT_OK) {
		VM_UPNP::stopServer(result);
		return result;
	}

	// GENERATE THE SERVER DESCRIPTION URL
	char mediaURL[DEFAULT_CHAR_BUFFER_SIZE];
	snprintf(mediaURL, DEFAULT_CHAR_BUFFER_SIZE, "http://%s:%d/description.xml", LIB_UPNP::UpnpGetServerIpAddress(), LIB_UPNP::UpnpGetServerPort());

	// LET EVERYONE KNOW THE FILES HAVE BEEN UPDATED
	VM_UPNP::updateID++;

	// TRY STARTING THE WEB SERVER
	result = LIB_UPNP::UpnpSetWebServerRootDir(webServerPath.c_str());

	if (result != UPNP_E_SUCCESS) {
		VM_UPNP::stopServer(result);
		return result;
	}

	// TRY STARTING THE VM_UPNP MEDIA SERVER
	result = LIB_UPNP::UpnpRegisterRootDevice(mediaURL, VM_UPNP::handleEvents, NULL, &VM_UPNP::server);

	if (result != UPNP_E_SUCCESS) {
		VM_UPNP::stopServer(result);
		return result;
	}

	// DISABLE MAX CONTENT LENGTH
	LIB_UPNP::UpnpSetMaxContentLength(0);

	// SET CUSTOM VIRTUAL (WEB SERVER) CALLBACK FUNCTIONS
	LIB_UPNP::UpnpVirtualDir_set_CloseCallback(VM_UPNP::webServerClose);
	LIB_UPNP::UpnpVirtualDir_set_GetInfoCallback(VM_UPNP::webServerGetInfo);
	LIB_UPNP::UpnpVirtualDir_set_OpenCallback(VM_UPNP::webServerOpen);
	LIB_UPNP::UpnpVirtualDir_set_ReadCallback(VM_UPNP::webServerRead);
	LIB_UPNP::UpnpVirtualDir_set_SeekCallback(VM_UPNP::webServerSeek);
	LIB_UPNP::UpnpVirtualDir_set_WriteCallback(VM_UPNP::webServerWrite);

	// SHARE FILES
	VM_UPNP::addFiles();

	if (VM_UPNP::files.empty()) {
		VM_UPNP::stopServer(UPNP_E_FILE_NOT_FOUND);
		return result;
	}

	// START SENDING OUT SSDP (DISCOVERY) MESSAGES
	LIB_UPNP::UpnpSendAdvertisement(VM_UPNP::server, UPNP_ALIVE_TIME);

	// KEEP RUNNING UNTIL SERVER IS STOPPED
	while (VM_ThreadManager::Threads[THREAD_UPNP_SERVER]->start && !VM_Window::Quit)
	{
		if (VM_ThreadManager::Threads[THREAD_UPNP_CLIENT]->completed)
			snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "UPnP Server %s [%s:%u]", VM_Window::Labels["upnp.running"].c_str(), LIB_UPNP::UpnpGetServerIpAddress(), LIB_UPNP::UpnpGetServerPort());

		SDL_Delay(DELAY_TIME_BACKGROUND);
	}

	VM_UPNP::stopServer(UPNP_E_SUCCESS);

	return UPNP_E_SUCCESS;
}

int UPNP::VM_UPNP::stopClient(int result)
{
	LIB_UPNP::UpnpUnRegisterClient(VM_UPNP::Client);

	if (VM_ThreadManager::Threads[THREAD_UPNP_SERVER]->completed)
		LIB_UPNP::UpnpFinish();

	VM_UPNP::Devices.clear();

	VM_UPNP::Device   = "";
	VM_UPNP::ClientIP = "";

	if (result == UPNP_E_FILE_NOT_FOUND)
		snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s", VM_Window::Labels["error.no_upnp_items"].c_str());
	else if (result == UPNP_E_NO_WEB_SERVER)
		snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s", VM_Window::Labels["error.no_upnp"].c_str());
	else if (result == UPNP_E_SUCCESS)
		snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s", VM_Window::Labels["status.scan.finished"].c_str());
	else
		snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s %s", VM_Window::Labels["error.init_upnp"].c_str(), LIB_UPNP::UpnpGetErrorMessage(result));

	VM_ThreadManager::Threads[THREAD_UPNP_CLIENT]->start     = false;
	VM_ThreadManager::Threads[THREAD_UPNP_CLIENT]->completed = true;

	return UPNP_E_SUCCESS;
}

int UPNP::VM_UPNP::stopServer(int result)
{
	LIB_UPNP::UpnpUnRegisterRootDevice(VM_UPNP::server);

	if (VM_ThreadManager::Threads[THREAD_UPNP_CLIENT]->completed)
		LIB_UPNP::UpnpFinish();

	for (uint32_t i = 0; i < VM_UPNP::files.size(); i++)
		DELETE_POINTER(VM_UPNP::files[i]);

	VM_UPNP::files.clear();

	VM_UPNP::ServerIP  = "";
	VM_UPNP::deviceUDN = "";

	if (result == UPNP_E_FILE_NOT_FOUND)
		snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s", VM_Window::Labels["error.share_no_files"].c_str());
	else if (result != UPNP_E_SUCCESS)
		snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s UPnP Server: %s", VM_Window::Labels["error.start"].c_str(), LIB_UPNP::UpnpGetErrorMessage(result));
	else
		snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "UPnP Server %s", VM_Window::Labels["upnp.stopped"].c_str());

	VM_ThreadManager::Threads[THREAD_UPNP_SERVER]->start     = false;
	VM_ThreadManager::Threads[THREAD_UPNP_SERVER]->completed = true;

	return UPNP_E_SUCCESS;
}

int UPNP::VM_UPNP::webServerClose(void* fileHandle, const void* cookie, const void* request_cookie)
{
	VM_UpnpFile* file = static_cast<VM_UpnpFile*>(fileHandle);

	if (file == NULL)
		return ERROR_INVALID_ARGUMENTS;

	file->mutex.lock();

	CLOSE_FILE(file->data);

	if (file->iTunes)
		std::remove(file->file.c_str());

	file->mutex.unlock();

	return UPNP_E_SUCCESS;
}

int UPNP::VM_UPNP::webServerGetInfo(const char* filename, LIB_UPNP::UpnpFileInfo* info, const void* cookie, const void** request_cookie)
{ 
	if ((filename == NULL) || (info == NULL) || VM_UPNP::files.empty())
		return ERROR_INVALID_ARGUMENTS;

	Strings fileDetails;

	for (uint32_t i = 0; i < VM_UPNP::files.size(); i++)
	{
		if (strcmp(filename, VM_UPNP::files[i]->name.c_str()) != 0)
			continue;

		VM_UPNP::files[i]->mutex.lock();

		#if defined _windows
			LIB_UPNP::UpnpFileInfo_set_FileLength(info, (size_t)VM_UPNP::files[i]->size);
		#else
			LIB_UPNP::UpnpFileInfo_set_FileLength(info, (off_t)VM_UPNP::files[i]->size);
		#endif

		LIB_UPNP::UpnpFileInfo_set_LastModified(info, 0);
		LIB_UPNP::UpnpFileInfo_set_IsDirectory(info,  0);
		LIB_UPNP::UpnpFileInfo_set_IsReadable(info,   1);
		LIB_UPNP::UpnpFileInfo_set_ContentType(info,  VM_UPNP::files[i]->type.c_str());

		VM_UPNP::files[i]->mutex.unlock();

		return RESULT_OK;
	}

	return UPNP_E_SUCCESS;
}

void* UPNP::VM_UPNP::webServerOpen(const char* filename, LIB_UPNP::UpnpOpenFileMode mode, const void* cookie, const void* request_cookie)
{ 
	if ((filename == NULL) || (mode != LIB_UPNP::UPNP_READ))
		return NULL;

	for (uint32_t i = 0; i < VM_UPNP::files.size(); i++)
	{
		if (strcmp(filename, VM_UPNP::files[i]->name.c_str()) != 0)
			continue;

		VM_UPNP::files[i]->mutex.lock();

		if (VM_UPNP::files[i]->data == NULL) {
			VM_UPNP::files[i]->data     = fopen(VM_UPNP::files[i]->file.c_str(), "rb");
			VM_UPNP::files[i]->position = 0;
		}

		VM_UPNP::files[i]->mutex.unlock();

		if (VM_UPNP::files[i]->data != NULL)
			return VM_UPNP::files[i];
	}

	return NULL;
}

int UPNP::VM_UPNP::webServerRead(void* fileHandle, char* buffer, size_t bufferSize, const void* cookie, const void* request_cookie)
{
	if ((fileHandle == NULL) || (buffer == NULL))
		return 0;

	VM_UpnpFile* file = static_cast<VM_UpnpFile*>(fileHandle);

	file->mutex.lock();

	size_t remaining = (size_t)(file->size - file->position);
	size_t size      = (bufferSize < remaining ? bufferSize : remaining);

	if ((size > 0) && (file->data != NULL)) {
		fread(buffer, sizeof(char), size, file->data);
		file->position += size;
	}

	file->mutex.unlock();

	return (int)size;
}

#if defined _windows
int UPNP::VM_UPNP::webServerSeek(void* fileHandle, int64_t offset, int origin, const void* cookie, const void* request_cookie)
#else
int UPNP::VM_UPNP::webServerSeek(void* fileHandle, off_t offset, int origin, const void* cookie, const void* request_cookie)
#endif
{
	if (fileHandle == NULL)
		return ERROR_INVALID_ARGUMENTS;

	VM_UpnpFile* file = static_cast<VM_UpnpFile*>(fileHandle);

	file->mutex.lock();

	int64_t newPosition = -1;
	int     result      = -1;

	switch (origin) {
		case SEEK_SET: newPosition = (int64_t)offset;                    break;
		case SEEK_CUR: newPosition = (int64_t)(file->position + offset); break;
		case SEEK_END: newPosition = (int64_t)(file->size     + offset); break;
	}

	if ((newPosition >= 0) && (newPosition <= (int64_t)file->size) && (file->data != NULL))
		result = fseek(file->data, newPosition, SEEK_SET);

	if ((result == 0) && (file->data != NULL) && (ferror(file->data) == 0))
		file->position = newPosition;

	file->mutex.unlock();

	return UPNP_E_SUCCESS;
}

int UPNP::VM_UPNP::webServerWrite(void* fileHandle, char* buffer, size_t bufferSize, const void* cookie, const void* request_cookie)
{
	return ERROR_UNKNOWN;
}
