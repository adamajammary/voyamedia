#include "VM_Modal.h"

using namespace VoyaMedia::Database;
using namespace VoyaMedia::MediaPlayer;
using namespace VoyaMedia::System;
using namespace VoyaMedia::UPNP;
using namespace VoyaMedia::XML;

bool                      Graphics::VM_Modal::centered        = true;
Graphics::VM_ComponentMap Graphics::VM_Modal::Components;
SDL_Rect                  Graphics::VM_Modal::dimensions      = {};
String                    Graphics::VM_Modal::File            = "";
Graphics::VM_Table*       Graphics::VM_Modal::ListTable       = NULL;
String                    Graphics::VM_Modal::messageText     = "";
LIB_XML::xmlNode*         Graphics::VM_Modal::node            = NULL;
bool                      Graphics::VM_Modal::refreshPending  = false;
Graphics::VM_Panel*       Graphics::VM_Modal::rootPanel       = NULL;
Graphics::VM_Component*   Graphics::VM_Modal::TextInput       = NULL;
bool                      Graphics::VM_Modal::visible         = false;
LIB_XML::xmlDoc*          Graphics::VM_Modal::xmlDoc          = NULL;

int Graphics::VM_Modal::Apply(const String &buttonID)
{
	VM_Buttons selectedModalRow, selectedGuiRow;

	if (VM_Modal::ListTable != NULL)
		selectedModalRow = VM_Modal::ListTable->getSelectedRow();

	if (VM_GUI::ListTable != NULL)
		selectedGuiRow = VM_GUI::ListTable->getSelectedRow();

	// PLAYER SETTINGS - AUDIO TRACKS / SUBS
	if ((VM_Modal::File == "modal_player_settings_audio") ||
		(VM_Modal::File == "modal_player_settings_subs"))
	{
		if (!selectedModalRow.empty())
			VM_Player::SetStream(selectedModalRow[0]->mediaID);
	}
	// SAVE SEARCH TO PLAYLIST
	else if (VM_Modal::File == "modal_playlist_save")
	{
		int  dbResult;
		auto db           = new VM_Database(dbResult, DATABASE_PLAYLISTSv3);
		auto playlistName = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_playlist_name_input"]);
		auto searchValue  = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_playlist_search_value"]);

		if (DB_RESULT_OK(dbResult) && (playlistName != NULL) && (searchValue != NULL))
			dbResult = db->addPlaylist(playlistName->getText(), searchValue->getText());

		DELETE_POINTER(db);

		if (!DB_RESULT_OK(dbResult)) {
			VM_Modal::ShowMessage(VM_Window::Labels["error.save_playlist"]);
			return ERROR_UNKNOWN;
		}
	}
	// PLAYLISTS
	else if (VM_Modal::File == "modal_playlists")
	{
		if (VM_Modal::ListTable != NULL)
		{
			if (selectedModalRow.size() > 3)
			{
				// REMOVE
				if (buttonID == "modal_playlists_remove")
				{
					int  dbResult;
					auto db = new VM_Database(dbResult, DATABASE_PLAYLISTSv3);

					if (DB_RESULT_OK(dbResult))
						db->deletePlaylist(selectedModalRow[1]->getText());

					DELETE_POINTER(db);
				// SELECT
				} else {
					VM_GUI::ListTable->setSearch(selectedModalRow[2]->getText(), true);
				}
			}
		}
	}
	// DROPBOX
	else if (VM_Modal::File == "modal_dropbox")
	{
		VM_Button* authCode = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_dropbox_input"]);

		if (authCode != NULL) {
			VM_FileSystem::SaveDropboxTokenOAuth2(authCode->getText());
			VM_ThreadManager::Threads[THREAD_SCAN_DROPBOX]->start = true;
		}
	}
	// UPNP
	else if (VM_Modal::File == "modal_upnp")
	{
		String nic = "";

		if (VM_Modal::ListTable != NULL)
			nic = VM_Modal::ListTable->getSelectedMediaURL();

		if (!nic.empty())
		{
			if (buttonID == "modal_upnp_add")
			{
				VM_UPNP::ClientIP = nic;
				VM_ThreadManager::Threads[THREAD_UPNP_CLIENT]->start = true;
			}
			#if !defined _ios
			else if (buttonID == "modal_upnp_share")
			{
				if (VM_ThreadManager::Threads[THREAD_UPNP_SERVER]->completed) {
					VM_UPNP::ServerIP = nic;
					VM_ThreadManager::Threads[THREAD_UPNP_SERVER]->start = true;
				} else {
					VM_ThreadManager::Threads[THREAD_UPNP_SERVER]->start = false;
				}
			}
			#endif
		} else {
			snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s", VM_Window::Labels["error.no_nics"].c_str());
		}
	}
	// UPNP - DEVICES
	else if (VM_Modal::File == "modal_upnp_devices")
	{
		String device = VM_Modal::ListTable->getSelectedMediaURL();

		if (!device.empty())
			VM_UPNP::Device = device;
		else
			snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s", VM_Window::Labels["error.no_upnp"].c_str());
	}
	// SETTINGS
	else if (VM_Modal::File == "modal_settings")
	{
		int  dbResult;
		auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

		if (buttonID == "modal_settings_color")
		{
			VM_Window::ResetRenderer = true;

			if (DB_RESULT_OK(dbResult))
				db->updateSettings("color_theme", VM_GUI::ColorThemeFile == "dark" ? "light" : "dark");
		}
		else if (buttonID == "modal_settings_lang")
		{
			VM_Window::SystemLocale  = !VM_Window::SystemLocale;
			VM_Window::ResetRenderer = true;

			if (DB_RESULT_OK(dbResult))
				db->updateSettings("system_locale", (VM_Window::SystemLocale ? "1" : "0"));
		}

		DELETE_POINTER(db);
	}
	// RIGHT-CLICK
	else if (VM_Modal::File == "modal_right_click")
	{
		if (buttonID == "modal_right_click_remove_file")
		{
			String name    = "";
			int    mediaID = 0;

			if (!selectedGuiRow.empty()) {
				mediaID = selectedGuiRow[1]->mediaID;
				name    = selectedGuiRow[1]->getText();
			}

			snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s '%s'", VM_Window::Labels["status.removing"].c_str(), name.c_str());

			int  dbResult;
			auto db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

			if (DB_RESULT_OK(dbResult) && (mediaID > 0))
				dbResult = db->deleteMediaFile(mediaID);

			DELETE_POINTER(db);

			if ((mediaID > 0) && DB_RESULT_OK(dbResult))
				snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s '%s'", VM_Window::Labels["status.removed"].c_str(), name.c_str());
			else
				snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s '%s'", VM_Window::Labels["error.remove"].c_str(), name.c_str());
		}
		else if ((buttonID == "modal_right_click_tmbd_movie") || (buttonID == "modal_right_click_tmbd_tv"))
		{
			VM_MediaType mediaType = (buttonID == "modal_right_click_tmbd_movie" ? MEDIA_TYPE_TMDB_MOVIE : MEDIA_TYPE_TMDB_TV);
			VM_Button*   desc      = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_desc"]);

			if (desc != NULL)
			{
				Strings delims     = { ".", ",", "-", "_", ":", ";" };
				String  searchText = desc->getText();

				for (const auto &delim : delims)
					searchText = VM_Text::Replace(searchText, delim, " ");

				VM_Top::SelectType(mediaType);
				VM_GUI::ListTable->setSearch(searchText, true);
			}
		}
	}

	return RESULT_OK;
}

void Graphics::VM_Modal::hide()
{
	VM_ThreadManager::Mutex.lock();
	VM_Modal::ListTable = NULL;
	VM_ThreadManager::Mutex.unlock();

	VM_Modal::TextInput = NULL;

	VM_Modal::Components.clear();

	DELETE_POINTER(VM_Modal::rootPanel);

	FREE_XML_DOC(VM_Modal::xmlDoc);
	LIB_XML::xmlCleanupParser();

	VM_Modal::refreshPending = false;
}

void Graphics::VM_Modal::Hide()
{
	VM_Modal::refreshPending = true;
	VM_Modal::visible        = false;
}

bool Graphics::VM_Modal::IsVisible()
{
	return VM_Modal::visible;
}

int Graphics::VM_Modal::open()
{
	if (VM_Modal::File.empty())
		return ERROR_INVALID_ARGUMENTS;

	VM_Modal::hide();

	VM_Modal::xmlDoc = VM_XML::Load((VM_FileSystem::GetPathGUI() + VM_Modal::File + ".xml").c_str());

	if (VM_Modal::xmlDoc == NULL)
		return ERROR_UNKNOWN;

	VM_Modal::node     = VM_XML::GetNode("/modal", VM_Modal::xmlDoc);
	VM_Modal::centered = (VM_XML::GetAttribute(VM_Modal::node, "centered") == "true");

	VM_Modal::dimensions.w = (int)(std::atof(VM_XML::GetAttribute(VM_Modal::node, "width").c_str())  * VM_Window::Display.scaleFactor);
	VM_Modal::dimensions.h = (int)(std::atof(VM_XML::GetAttribute(VM_Modal::node, "height").c_str()) * VM_Window::Display.scaleFactor);

	if (VM_Modal::centered) {
		VM_Modal::dimensions.x = ALIGN_CENTER(0, VM_Window::Dimensions.w, VM_Modal::dimensions.w);
		VM_Modal::dimensions.y = ALIGN_CENTER(0, VM_Window::Dimensions.h, VM_Modal::dimensions.h);
	} else {
		VM_Modal::dimensions.x = std::atoi(VM_XML::GetAttribute(VM_Modal::node, "x").c_str());
		VM_Modal::dimensions.y = std::atoi(VM_XML::GetAttribute(VM_Modal::node, "y").c_str());
	}

	int result = VM_GUI::LoadComponentNodes(VM_Modal::node, NULL, VM_Modal::xmlDoc, VM_Modal::rootPanel, VM_Modal::Components);

	if (result != RESULT_OK)
		return result;

	if (VM_Player::State.isPlaying)
		VM_Player::Pause();

	VM_Modal::Refresh();

	return RESULT_OK;
}

int Graphics::VM_Modal::Open(const String &id)
{
	if (id.empty())
		return ERROR_INVALID_ARGUMENTS;

	VM_Modal::File           = id;
	VM_Modal::refreshPending = true;
	VM_Modal::visible        = true;

	if ((id == "modal_dropbox") && !VM_Window::ResetRenderer)
		VM_FileSystem::OpenWebBrowser(VM_FileSystem::GetURL(URL_DROPBOX_AUTH_CODE));

	return RESULT_OK;
}

int Graphics::VM_Modal::Refresh()
{
	if ((VM_Modal::rootPanel == NULL) || !VM_Modal::visible)
		return ERROR_INVALID_ARGUMENTS;

	VM_Modal::dimensions.w = (int)(std::atof(VM_XML::GetAttribute(VM_Modal::node, "width").c_str())  * VM_Window::Display.scaleFactor);
	VM_Modal::dimensions.h = (int)(std::atof(VM_XML::GetAttribute(VM_Modal::node, "height").c_str()) * VM_Window::Display.scaleFactor);

	if (VM_Modal::centered) {
		VM_Modal::dimensions.x = ALIGN_CENTER(0, VM_Window::Dimensions.w, VM_Modal::dimensions.w);
		VM_Modal::dimensions.y = ALIGN_CENTER(0, VM_Window::Dimensions.h, VM_Modal::dimensions.h);
	}

	VM_Modal::rootPanel->backgroundArea = VM_Modal::dimensions;

	int result = VM_GUI::LoadComponents(VM_Modal::rootPanel);

	if (result != RESULT_OK)
		return result;

	result = VM_GUI::LoadComponentsImage(VM_Modal::rootPanel);

	if (result != RESULT_OK)
		return result;

	result = VM_GUI::LoadComponentsText(VM_Modal::rootPanel);

	if (result != RESULT_OK)
		return result;

	result = VM_GUI::LoadTables(VM_Modal::rootPanel);

	if (result != RESULT_OK)
		return result;

	result = VM_Modal::updateLabels();

	if (result != RESULT_OK)
		return result;

	VM_Modal::refreshPending = false;

	return RESULT_OK;
}

void Graphics::VM_Modal::Render()
{
	VM_Color color      = VM_Graphics::ToVMColor(VM_GUI::ColorTheme["modal.back-color"]);
	SDL_Rect windowArea = { 0, VM_Window::StatusBarHeight, VM_Window::Dimensions.w, (VM_Window::Dimensions.h - VM_Window::StatusBarHeight) };

	VM_Graphics::FillArea(&color, &windowArea);

	if (VM_Modal::Components["modal_background_image"] != NULL)
		VM_Modal::Components["modal_background_image"]->render();

	if (VM_Modal::rootPanel != NULL)
		VM_Modal::rootPanel->render();
}

int Graphics::VM_Modal::ShowMessage(const String &messageText)
{
	if (messageText.empty())
		return ERROR_INVALID_ARGUMENTS;

	if (VM_GUI::Components.empty()) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, APP_NAME.c_str(), messageText.c_str(), NULL);
		return ERROR_UNKNOWN;
	}

	VM_Modal::File           = "modal_message";
	VM_Modal::messageText    = messageText;
	VM_Modal::refreshPending = true;
	VM_Modal::visible        = true;

	return RESULT_OK;
}

void Graphics::VM_Modal::Update()
{
	if (VM_Modal::TextInput == NULL)
	{
		if (VM_Modal::File == "modal_playlist_save")
			VM_Modal::TextInput = VM_Modal::Components["modal_playlist_input"];
		else if (VM_Modal::File == "modal_dropbox")
			VM_Modal::TextInput = VM_Modal::Components["modal_middle"];
	}

	if (VM_Modal::ListTable == NULL)
	{
		if (VM_Modal::File == "modal_playlists")
			VM_Modal::ListTable = dynamic_cast<VM_Table*>(VM_Modal::Components["modal_playlists_list_table"]);
		else if (VM_Modal::File == "modal_player_settings_audio")
			VM_Modal::ListTable = dynamic_cast<VM_Table*>(VM_Modal::Components["modal_player_settings_audio_list_table"]);
		else if (VM_Modal::File == "modal_player_settings_subs")
			VM_Modal::ListTable = dynamic_cast<VM_Table*>(VM_Modal::Components["modal_player_settings_subs_list_table"]);
		else if (VM_Modal::File == "modal_upnp")
			VM_Modal::ListTable = dynamic_cast<VM_Table*>(VM_Modal::Components["modal_upnp_list_table"]);
		else if (VM_Modal::File == "modal_upnp_devices")
			VM_Modal::ListTable = dynamic_cast<VM_Table*>(VM_Modal::Components["modal_upnp_devices_list_table"]);

		if (VM_Modal::ListTable != NULL) {
			VM_Modal::ListTable->refreshRows();
			VM_Modal::ListTable->refreshSelected();
		}
	}

	if (VM_Modal::ListTable != NULL)
		VM_Modal::ListTable->refresh();

	if (VM_Modal::refreshPending)
	{
		if (VM_Modal::visible) {
			if (VM_Modal::open() != RESULT_OK) {
				VM_Modal::ShowMessage((VM_Window::Labels["error.modal"] + " " + VM_Modal::File + ".xml"));
				VM_Modal::refreshPending = false;
			}
		} else {
			VM_Modal::hide();
		}
	}
}

bool Graphics::VM_Modal::updateMediaDB(LIB_FFMPEG::AVFormatContext* formatContext, const String &filePath, int mediaID, String &duration, String &size, String &path)
{
	VM_DBResult rows;
	int         dbResult;
	auto        db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

	if (DB_RESULT_OK(dbResult) && (mediaID > 0))
		rows = db->getRows("SELECT path, size, duration FROM MEDIA_FILES WHERE id=" + std::to_string(mediaID) + ";", 1);

	DELETE_POINTER(db);

	if (!rows.empty() && (rows[0].size() > 2))
	{
		path = rows[0]["path"];

		if ((rows[0]["duration"] == "0") || (rows[0]["size"] == "0"))
		{
			LIB_FFMPEG::AVStream* audioStream = NULL;
			int64_t               durationI   = 0;
			size_t                sizeI       = VM_FileSystem::GetFileSize(filePath);

			if (formatContext != NULL) {
				audioStream = VM_FileSystem::GetMediaStreamFirst(formatContext, MEDIA_TYPE_AUDIO);
				durationI   = VM_FileSystem::GetMediaDuration(formatContext, audioStream);
			}

			db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

			if (DB_RESULT_OK(dbResult))
			{
				if (formatContext != NULL)
					db->updateInteger(mediaID, "duration", durationI);

				db->updateInteger(mediaID, "size", sizeI);
			}

			DELETE_POINTER(db);

			duration = VM_Text::ToDuration(durationI);
			size     = VM_Text::GetFileSizeString(sizeI);
		} else {
			duration = VM_Text::ToDuration(std::atoll(rows[0]["duration"].c_str()));
			size     = VM_Text::GetFileSizeString((size_t)std::strtoull(rows[0]["size"].c_str(), NULL, 10));
		}
	}

	return !rows.empty();
}

int Graphics::VM_Modal::updateLabels()
{
	DELETE_POINTER(VM_Modal::Components["modal_background_image"]);

	if (VM_Modal::File == "modal_message")
		VM_Modal::updateLabelsMessage();
	else if (VM_Modal::File == "modal_playlist_save")
		VM_Modal::updateLabelsPlaylistSave();
	else if (VM_Modal::File == "modal_settings")
		VM_Modal::updateLabelsSettings();
	else if (VM_Modal::File == "modal_right_click")
		VM_Modal::updateLabelsRightClick();
	else if (VM_Modal::File == "modal_details")
		return VM_Modal::UpdateLabelsDetails();
	else
	{
		if (VM_Modal::File == "modal_upnp")
			VM_Modal::updateLabelsUPNP();

		if (VM_Modal::ListTable != NULL)
		{
			VM_Modal::ListTable->refreshRows();
			VM_Modal::ListTable->refreshSelected();
			VM_Modal::ListTable->refresh();
		}
	}

	return RESULT_OK;
}

void Graphics::VM_Modal::updateLabelsMessage()
{
	VM_Button* title = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_title"]);
	VM_Button* text  = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_message_text"]);

	if (title != NULL)
		title->setText(APP_NAME + " " + APP_VERSION);

	if (text != NULL)
		text->setText(VM_Modal::messageText);
}

int Graphics::VM_Modal::UpdateLabelsDetails()
{
	if (VM_GUI::ListTable->getState().dataRequested)
		return RESULT_REFRESH_PENDING;

	VM_Button* desc = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_desc"]);
	VM_Buttons row  = VM_GUI::ListTable->getSelectedRow();

	if ((desc == NULL) || (row.size() < 2))
		return ERROR_INVALID_ARGUMENTS;

	desc->setText(row[1]->getText());

	VM_Button* detailsButton  = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_details_text"]);
	VM_Button* detailsButton2 = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_details_text_details"]);
	VM_Button* thumbButton    = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_details_thumb"]);

	if ((detailsButton == NULL) || (detailsButton2 == NULL) || (thumbButton == NULL))
		return ERROR_INVALID_ARGUMENTS;

	VM_DBResult rows;
	int         mediaID  = VM_GUI::ListTable->getSelectedMediaID();
	String      filePath = VM_GUI::ListTable->getSelectedMediaURL();
	String      text     = "";
	String      text2    = "";

	if (AUDIO_IS_SELECTED || PICTURE_IS_SELECTED || VIDEO_IS_SELECTED)
	{
		int  dbResult;
		auto db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

		if (DB_RESULT_OK(dbResult) && (mediaID > 0))
			rows = db->getRows("SELECT path, size FROM MEDIA_FILES WHERE id=" + std::to_string(mediaID) + ";", 1);

		DELETE_POINTER(db);

		if (rows.empty())
			return ERROR_UNKNOWN;
	}

	String filePath2 = filePath;

	#if defined _ios
	if (VM_FileSystem::IsITunes(filePath))
		filePath = VM_FileSystem::DownloadFileFromITunes(filePath2);
	#endif

	if (AUDIO_IS_SELECTED)
	{
		VM_Modal::updateLabelsDetailsAudio(mediaID, filePath2, filePath);
	}
	else if (PICTURE_IS_SELECTED)
	{
		VM_Modal::updateLabelsDetailsPicture(mediaID, filePath2, filePath);
	}
	else if (VIDEO_IS_SELECTED)
	{
		VM_Modal::updateLabelsDetailsVideo(mediaID, filePath2, filePath);
	}
	else if (YOUTUBE_IS_SELECTED)
	{
		StringMap details = VM_FileSystem::GetYouTubeDetails(row[1]->mediaID2);

		text.append(!details["date"].empty()        ? VM_Text::GetTimeFormatted(details["date"], false, false) + "\n" : "");
		text.append(!details["channel"].empty()     ? "#" + details["channel"] + "\n" : "");
		text.append(!details["duration_yt"].empty() ? VM_Text::ToDuration(details["duration_yt"]) + "\n" : "");
		text.append(!details["views"].empty()       ? VM_Text::ToViewCount(std::atoll(details["views"].c_str())) + " " + VM_Window::Labels["views"] + "\n" : "");
		text.append(!details["likes"].empty()       ? details["likes"] : "");

		text2.append(!details["description"].empty() ? details["description"] : "");
	}
	else if (SHOUTCAST_IS_SELECTED)
	{
		StringMap details = VM_FileSystem::GetShoutCastDetails(row[1]->getText(), mediaID);

		text.append(!details["genre"].empty()          ? details["genre"] + "\n"         : "");
		text.append(!details["bit_rate"].empty()       ? details["bit_rate"] + " kbps\n" : "");
		text.append(!details["media_type"].empty()     ? details["media_type"] + "\n"    : "");
		text.append(!details["listener_count"].empty() ? VM_Text::ToViewCount(std::atoll(details["listener_count"].c_str())) + " " + VM_Window::Labels["listeners"] : "");

		text2.append(!details["now_playing"].empty() ? details["now_playing"] : "");
	}
	else if (TMDB_MOVIE_IS_SELECTED || TMDB_TV_IS_SELECTED)
	{
		StringMap details = VM_FileSystem::GetTmdbDetails(mediaID, VM_Top::Selected);

		// TOP TEXT
		text.append(!details["rating"].empty()   ? details["rating"] + "\n" : "");
		text.append(!details["date"].empty()     ? details["date"] : "");
		text.append(!details["date"].empty()     && !details["duration"].empty() ? " | " : "");
		text.append(!details["duration"].empty() ? VM_Text::ToDuration(std::atoll(details["duration"].c_str())) + "\n" : "");
		text.append(!details["seasons"].empty()  ? details["seasons"]  + " " + (details["seasons"]  != "1" ? VM_Window::Labels["seasons"]  : VM_Window::Labels["season"]) : "");
		text.append(!details["seasons"].empty() && !details["episodes"].empty() ? " | " : "");
		text.append(!details["episodes"].empty() ? details["episodes"] + " " + (details["episodes"] != "1" ? VM_Window::Labels["episodes"] : VM_Window::Labels["episode"]) : "");
		text.append(!text.empty()                ? "\n" : "");
		text.append(!details["language"].empty() ? VM_Text::GetLanguage(details["language"]) + "\n" : "");
		text.append(!details["genres"].empty()   ? details["genres"] : "");

		// BOTTOM TEXT
		text2.append(!details["overview"].empty() ? details["overview"] : "");

		// BACKDROP IMAGE
		VM_Button* backgroundImage = new VM_Button("modal_background_image");

		backgroundImage->backgroundArea = { 0, VM_Window::StatusBarHeight, VM_Window::Dimensions.w, (VM_Window::Dimensions.h - VM_Window::StatusBarHeight) };
		backgroundImage->setImage(details["backdrop_url"], false);

		VM_Modal::Components["modal_background_image"] = backgroundImage;
	}

	#if defined _ios
	if (VM_FileSystem::IsITunes(filePath2))
		VM_FileSystem::DeleteFileITunes(filePath);
	#endif

	if (!text.empty())
		detailsButton->setText(text);

	if (!text2.empty())
		detailsButton2->setText(text2);

	if (!row.empty())
	{
		thumbButton->mediaID  = row[0]->mediaID;
		thumbButton->mediaID2 = row[0]->mediaID2;
		thumbButton->mediaURL = row[0]->mediaURL;

		thumbButton->setThumb(VM_GUI::ListTable->id);
	}

	return RESULT_OK;
}

int Graphics::VM_Modal::updateLabelsDetailsAudio(int mediaID, const String &filePath2, const String &filePath)
{
	VM_Button* details  = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_details_text"]);
	VM_Button* details2 = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_details_text_details"]);

	if (!AUDIO_IS_SELECTED || (details == NULL) || (details2 == NULL))
		return ERROR_INVALID_ARGUMENTS;

	StringMap meta;
	auto      formatContext = VM_FileSystem::GetMediaFormatContext(filePath, true);
	auto      stream        = VM_FileSystem::GetMediaStreamFirst(formatContext, MEDIA_TYPE_AUDIO);
	String    streamDesc    = VM_FileSystem::GetMediaStreamDescriptionAudio(stream);
	String    text          = "";
	String    text2         = "";

	#if defined _ios
	if (VM_FileSystem::IsITunes(filePath2))
		meta = VM_FileSystem::GetMediaStreamMetaItunes(mediaID);
	else
	#endif
		meta = VM_FileSystem::GetMediaStreamMeta(formatContext);

	if (!streamDesc.empty())
		streamDesc = streamDesc.substr(streamDesc.find("] ") + 2);

	text.append(!meta["artist"].empty() ? meta["artist"] + "\n" : "");
	text.append(!meta["title"].empty()                          ? meta["title"] + "\n"  : "");
	text.append(!meta["album"].empty()                          ? meta["album"] : "");
	text.append(!meta["album"].empty() && !meta["date"].empty() ? " (" : "");
	text.append(!meta["date"].empty()                           ? std::to_string(std::atoi(meta["date"].c_str())) : "");
	text.append(!meta["album"].empty() && !meta["date"].empty() ? ")" : "");
	text.append(!meta["album"].empty()                          ? "\n" : "");
	text.append(!meta["genre"].empty()                          ? meta["genre"] : "");

	if (!streamDesc.empty())
		text2.append(streamDesc + "\n");

	String duration = "";
	String path     = "";
	String size     = "";

	if (VM_Modal::updateMediaDB(formatContext, filePath, mediaID, duration, size, path))
	{
		text2.append(!path.empty()                      ? path + "\n" : "");
		text2.append(!size.empty()                      ? size : "");
		text2.append(!size.empty() && !duration.empty() ? " (" : "");
		text2.append(!duration.empty()                  ? duration : "");
		text2.append(!size.empty() && !duration.empty() ? ")" : "");
	}

	VM_FileSystem::CloseMediaFormatContext(formatContext, mediaID);

	if (!text.empty())
		details->setText(text);

	if (!text2.empty())
		details2->setText(text2);

	return RESULT_OK;
}

int Graphics::VM_Modal::UpdateLabelsDetailsPicture()
{
	int    mediaID  = VM_GUI::ListTable->getSelectedMediaID();
	String filePath = VM_GUI::ListTable->getSelectedMediaURL();

	return VM_Modal::updateLabelsDetailsPicture(mediaID, filePath, filePath);
}

int Graphics::VM_Modal::updateLabelsDetailsPicture(int mediaID, const String &filePath2, const String &filePath)
{
	VM_Button* details  = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_details_text"]);
	VM_Button* details2 = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_details_text_details"]);
	VM_Button* snapshot = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_snapshot"]);

	if (!PICTURE_IS_SELECTED || (details == NULL) || (details2 == NULL) || (snapshot == NULL))
		return ERROR_INVALID_ARGUMENTS;

	if ((snapshot != NULL) && ((snapshot->imageData == NULL) || (snapshot->imageData->image == NULL)))
		snapshot->setImage(filePath, true);

	VM_Buttons row   = VM_GUI::ListTable->getSelectedRow();
	String     text  = "";
	String     text2 = "";
	String     title = (row.size() > 1 ? row[1]->getText() : "");

	if ((snapshot->imageData != NULL) && (snapshot->imageData->image != NULL))
	{
		String   camera     = VM_Graphics::GetImageCamera(snapshot->imageData->image->image);
		int64_t  dateI      = std::atoll(VM_Graphics::GetImageDateTaken(snapshot->imageData->image->image).c_str());
		String   dateS      = (dateI > 0 ? VM_Text::GetTimeFormatted((time_t)dateI, false, false) : "");
		String   address    = VM_Graphics::GetImageGPS(snapshot->imageData->image->image);
		String   resolution = VM_Graphics::GetImageResolutionString(snapshot->imageData->width, snapshot->imageData->height);
		uint32_t bpp        = FreeImage_GetBPP(snapshot->imageData->image->image);
		String   extension  = VM_FileSystem::GetFileExtension(title, true);

		text.append(!camera.empty()  ? camera  + "\n" : "");
		text.append(!dateS.empty()   ? dateS   + "\n" : "");
		text.append(!address.empty() ? address : "");

		text2.append(!extension.empty()  ? extension  + " " : "");
		text2.append(!resolution.empty() ? resolution + " " : "");
		text2.append(bpp > 0             ? std::to_string(bpp) + " bpp\n" : "");
	}

	String duration = "";
	String path     = "";
	String size     = "";

	if (VM_Modal::updateMediaDB(NULL, filePath, mediaID, duration, size, path)) {
		text2.append(!path.empty() ? path + "\n" : "");
		text2.append(!size.empty() ? size : "");
	}

	if (!text.empty())
		details->setText(text);

	if (!text2.empty())
		details2->setText(text2);

	return RESULT_OK;
}

int Graphics::VM_Modal::updateLabelsDetailsVideo(int mediaID, const String &filePath2, const String &filePath)
{
	VM_Button* details  = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_details_text"]);
	VM_Button* details2 = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_details_text_details"]);

	if (!VIDEO_IS_SELECTED || (details == NULL) || (details2 == NULL))
		return ERROR_INVALID_ARGUMENTS;

	auto   formatContext = VM_FileSystem::GetMediaFormatContext(filePath, true);
	String text          = "";
	String text2         = "";

	if (formatContext != NULL)
	{
		Strings audioTracks, subTracks;
		int     audioIdx = 0;
		int     subIdx   = 0;

		// BLURAY / DVD: "concat:streamPath|stream1|...|streamN|duration|title|ja,ja,no,|N/A,fi,sv,|"
		if (VM_FileSystem::IsConcat(filePath))
		{
			Strings parts = VM_Text::Split(filePath.substr(7), "|");

			if (parts.size() >= 6) {
				audioTracks = VM_Text::Split(parts[parts.size() - 2], ",");
				subTracks   = VM_Text::Split(parts[parts.size() - 1], ",");
			}
		}

		for (uint32_t i = 0; i < min(formatContext->nb_streams, 7); i++)
		{
			String            streamDesc = "";
			LIB_FFMPEG::AVStream* stream     = VM_FileSystem::GetMediaStreamByIndex(formatContext, i);

			if ((stream != NULL) && (stream->codec != NULL))
			{
				if (stream->codec->codec_type == LIB_FFMPEG::AVMEDIA_TYPE_AUDIO)
				{
					streamDesc = VM_FileSystem::GetMediaStreamDescriptionAudio(stream);

					if (VM_FileSystem::IsConcat(filePath) && (audioIdx < (int)audioTracks.size()))
					{
						if (!streamDesc.empty())
							streamDesc.append(" (" + VM_Text::GetLanguage(audioTracks[audioIdx]) + ")");
						else
							streamDesc = VM_Text::GetLanguage(audioTracks[audioIdx]);
					}

					audioIdx++;
				}
				else if (stream->codec->codec_type == LIB_FFMPEG::AVMEDIA_TYPE_VIDEO)
				{
					streamDesc = VM_FileSystem::GetMediaStreamDescriptionVideo(stream);
				}
				else if (stream->codec->codec_type == LIB_FFMPEG::AVMEDIA_TYPE_SUBTITLE)
				{
					streamDesc = VM_FileSystem::GetMediaStreamDescriptionSub(stream);

					if (VM_FileSystem::IsConcat(filePath) && (subIdx < (int)subTracks.size()))
					{
						if (!streamDesc.empty())
							streamDesc.append(" (" + VM_Text::GetLanguage(subTracks[subIdx]) + ")");
						else
							streamDesc = VM_Text::GetLanguage(subTracks[subIdx]);
					}

					subIdx++;
				}

				FREE_STREAM(stream);

				if (!streamDesc.empty())
					text.append(streamDesc + "\n");
			}
		}

		String duration = "";
		String path     = "";
		String size     = "";

		if (VM_Modal::updateMediaDB(formatContext, filePath, mediaID, duration, size, path))
		{
			text2.append(!path.empty()                      ? path + "\n" : "");
			text2.append(!size.empty()                      ? size : "");
			text2.append(!size.empty() && !duration.empty() ? " (" : "");
			text2.append(!duration.empty()                  ? duration : "");
			text2.append(!size.empty() && !duration.empty() ? ")" : "");
		}

		VM_FileSystem::CloseMediaFormatContext(formatContext, mediaID);
	}

	if (!text.empty())
		details->setText(text);

	if (!text2.empty())
		details2->setText(text2);

	return RESULT_OK;
}

void Graphics::VM_Modal::updateLabelsPlaylistSave()
{
	VM_Button* playlistName = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_playlist_name_input"]);
	VM_Button* searchValue  = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_playlist_search_value"]);

	if ((playlistName != NULL) && (searchValue != NULL))
	{
		String search = VM_GUI::ListTable->getSearch();

		if (playlistName->getInputText().empty()) {
			playlistName->setInputText(search);
			playlistName->setText(search);
		}

		searchValue->setText(search);
	}
}

void Graphics::VM_Modal::updateLabelsSettings()
{
	VM_Button* color = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_settings_color"]);

	if (color != NULL)
		color->setText(VM_GUI::ColorThemeFile == "dark" ? VM_Window::Labels["settings.color.light"] : VM_Window::Labels["settings.color.dark"]);
}

void Graphics::VM_Modal::updateLabelsRightClick()
{
	VM_Button* desc = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_desc"]);
	VM_Buttons row  = VM_GUI::ListTable->getSelectedRow();

	if ((desc != NULL) && (row.size() > 1))
		desc->setText(row[1]->getText());

	VM_Modal::Components["modal_right_click_tmbd_movie"]->visible = VIDEO_IS_SELECTED;
	VM_Modal::Components["modal_right_click_tmbd_tv"]->visible    = VIDEO_IS_SELECTED;
}

void Graphics::VM_Modal::updateLabelsUPNP()
{
	VM_Button* share = dynamic_cast<VM_Button*>(VM_Modal::Components["modal_upnp_share"]);

	if (share != NULL)
	{
		#if !defined _ios
			share->setText(VM_ThreadManager::Threads[THREAD_UPNP_SERVER]->completed ? VM_Window::Labels["upnp.share.files"] : VM_Window::Labels["upnp.share.none"]);
		#else
			share->visible = false;
		#endif
	}
}
