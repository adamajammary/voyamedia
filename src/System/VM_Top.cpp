#include "VM_Top.h"

using namespace VoyaMedia::Database;
using namespace VoyaMedia::Graphics;
using namespace VoyaMedia::MediaPlayer;

VM_MediaType System::VM_Top::Selected = DEFAULT_MEDIA_TYPE;

int System::VM_Top::Init()
{
	int    dbResult;
	auto   db        = new VM_Database(dbResult, DATABASE_SETTINGSv3);
	String mediaType = "";

	if (DB_RESULT_OK(dbResult))
		mediaType = db->getSettings("media_type");

	DELETE_POINTER(db);

	if (!mediaType.empty())
		VM_Top::SelectType((VM_MediaType)std::atoi(mediaType.c_str()));

	return RESULT_OK;
}

String System::VM_Top::MediaTypeToId(VM_MediaType mediaType)
{
	if (mediaType == MEDIA_TYPE_AUDIO)
		return "top_audio";
	else if (mediaType == MEDIA_TYPE_PICTURE)
		return "top_picture";
	else if (mediaType == MEDIA_TYPE_VIDEO)
		return "top_video";
	else if (mediaType == MEDIA_TYPE_SHOUTCAST)
		return "top_shoutcast";

	return "";
}

VM_MediaType System::VM_Top::IdToMediaType(const String &mediaType)
{
	if (mediaType == "top_audio")
		return MEDIA_TYPE_AUDIO;
	else if (mediaType == "top_picture")
		return MEDIA_TYPE_PICTURE;
	else if (mediaType == "top_video")
		return MEDIA_TYPE_VIDEO;
	else if (mediaType == "top_shoutcast")
		return MEDIA_TYPE_SHOUTCAST;

	return MEDIA_TYPE_UNKNOWN;
}

int System::VM_Top::Refresh()
{
	if (VM_GUI::Components["top"] == NULL)
		return ERROR_INVALID_ARGUMENTS;

	for (auto button : VM_GUI::Components["top"]->buttons)
		button->selected = (button->id == VM_Top::MediaTypeToId(VM_Top::Selected));

	return RESULT_OK;
}

int System::VM_Top::SelectType(VM_MediaType mediaType)
{
	// CHECK INTERNET CONNECTION
	VM_Window::StatusString = "";

	if ((mediaType >= MEDIA_TYPE_SHOUTCAST) && !VM_FileSystem::HasInternetConnection())
	{
		VM_Window::StatusString = VM_Window::Labels["error.no_nics"];
		VM_Modal::ShowMessage(VM_Window::StatusString);

		return ERROR_UNKNOWN;
	}

	VM_Top::Selected = mediaType;

	int  dbResult;
	auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

	if (DB_RESULT_OK(dbResult))
		db->updateSettings("media_type", std::to_string(VM_Top::Selected));

	DELETE_POINTER(db);

	if (VM_GUI::ListTable != NULL)
		VM_GUI::ListTable->updateSearchInput();

	VM_Player::Init();
	VM_Window::Refresh();

	return RESULT_OK;
}
