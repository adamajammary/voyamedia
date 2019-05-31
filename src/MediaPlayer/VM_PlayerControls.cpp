#include "VM_PlayerControls.h"

using namespace VoyaMedia::Database;
using namespace VoyaMedia::Graphics;
using namespace VoyaMedia::System;
using namespace VoyaMedia::XML;

MediaPlayer::VM_MediaTime MediaPlayer::VM_PlayerControls::durationTime     = {};
float                     MediaPlayer::VM_PlayerControls::progressPercent  = 0;
MediaPlayer::VM_MediaTime MediaPlayer::VM_PlayerControls::progressTime     = {};
bool                      MediaPlayer::VM_PlayerControls::progressTimeLeft = false;
bool                      MediaPlayer::VM_PlayerControls::refreshPending   = false;
bool                      MediaPlayer::VM_PlayerControls::visible          = false;

int MediaPlayer::VM_PlayerControls::Hide(bool skipFS)
{
	VM_PlayerControls::visible = false;

	if (skipFS)
		return RESULT_OK;

	VM_Component* controls = VM_GUI::Components["bottom_player_controls"];
	VM_Component* snapshot = VM_GUI::Components["bottom_player_snapshot"];

	controls->visible = false;

	#if !defined _android && !defined _ios
	if (VM_Window::FullScreenMaximized) {
	#endif
		snapshot->backgroundArea.y = 0;
		snapshot->backgroundArea.h = VM_Window::Dimensions.h;
	#if !defined _android && !defined _ios
	}
	#endif

	VM_Player::FreeTextures();
	VM_Player::Refresh();

	return RESULT_OK;
}

int MediaPlayer::VM_PlayerControls::Init()
{
	int  dbResult;
	auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

	if (DB_RESULT_OK(dbResult))
		VM_PlayerControls::progressTimeLeft = (db->getSettings("progress_time_left") == "1");

	DELETE_POINTER(db);

	VM_PlayerControls::durationTime    = {};
	VM_PlayerControls::progressTime    = {};
	VM_PlayerControls::progressPercent = 0;

	if (VM_PlayerControls::visible)
		VM_PlayerControls::refreshPending = true;

	return RESULT_OK;
}

bool MediaPlayer::VM_PlayerControls::IsVisible()
{
	return VM_PlayerControls::visible;
}

void MediaPlayer::VM_PlayerControls::Refresh()
{
	VM_PlayerControls::refreshPending = true;
}

int MediaPlayer::VM_PlayerControls::RefreshControls()
{
	if (!VM_PlayerControls::refreshPending)
		return RESULT_OK;

	VM_ThreadManager::Mutex.lock();

	// PREV
	VM_Button* button = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_prev"]);

	if ((button != NULL) && (button->setImage("prev-1-512.png", false) != RESULT_OK)) {
		VM_ThreadManager::Mutex.unlock();
		return ERROR_UNKNOWN;
	}

	// PLAY/PAUSE
	button = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_play"]);

	if (button != NULL)
		button->setImage((VM_Player::State.isPlaying ? "pause-1-512.png" : "play-2-512.png"), false);

	// NEXT
	button = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_next"]);

	if (button != NULL)
		button->setImage("next-1-512.png", false);

	// STOP
	button = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_stop"]);

	if (button != NULL)
		button->setImage("stop-1-512.png", false);

	// SETTINGS
	button = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_settings"]);

	if (button != NULL)
		button->setImage((VIDEO_IS_SELECTED ? "settings-2-512.png" : "settings-3-512.png"), false);

	// FULLSCREEN
	button = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_fullscreen"]);

	if (button != NULL)
		button->setImage((!VM_Player::State.isStopped ? "fullscreen-1-512.png" : "fullscreen-2-512.png"), false);

	// ASPECT RATIO / STRETCH
	button = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_stretch"]);

	if (button != NULL) {
		if (VIDEO_IS_SELECTED || YOUTUBE_IS_SELECTED)
			button->setImage((VM_Player::State.keepAspectRatio ? "stretch-7-512.png" : "stretch-2-512.png"), false);
		else
			button->setImage((VM_Player::State.keepAspectRatio ? "stretch-8-512.png" : "stretch-3-512.png"), false);
	}

	// ROTATE
	button = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_rotate"]);

	if (button != NULL)
		button->setImage((!PICTURE_IS_SELECTED ? "rotate-4-512.png" : "rotate-3-512.png"), false);

	// PLAYLIST
	button = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_playlist"]);

	if (button != NULL)
	{
		String imageFile = "";

		if (YOUTUBE_IS_SELECTED || SHOUTCAST_IS_SELECTED)
		{
			VM_Player::State.loopType = PLAY_TYPE_NORMAL;
			imageFile                   = "loop-7-512.png";
		}
		else
		{
			switch (VM_Player::State.loopType) {
				case PLAY_TYPE_NORMAL:  imageFile = "loop-1-512.png"; break;
				case PLAY_TYPE_LOOP:    imageFile = "loop-5-512.png"; break;
				case PLAY_TYPE_SHUFFLE: imageFile = "loop-3-512.png"; break;
			}
		}

		if (!imageFile.empty())
			button->setImage(imageFile, false);
	}

	// MUTE
	button = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_mute"]);

	if (button != NULL)
		button->setImage((VM_Player::State.isMuted ? "mute-2-512.png" : "volume-1-512.png"), false);

	VM_ThreadManager::Mutex.unlock();

	// DURATION
	button = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_duration"]);

	if ((button == NULL) || (button->setText("00:00:00") != RESULT_OK))
		return ERROR_UNKNOWN;

	// PROGRESS
	button = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_progress"]);

	if (button != NULL)
		button->setText("00:00:00");

	// VOLUME
	button             = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_volume_thumb"]);
	VM_Button* button2 = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_volume_bar"]);

	if ((button != NULL) && (button2 != NULL))
	{
		float  volumePercent = (float)((float)VM_Player::State.audioVolume / (float)SDL_MIX_MAXVOLUME);
		String orientation   = VM_XML::GetAttribute(button2->xmlNode, "orientation");

		if (orientation == "vertical")
		{
			button->backgroundArea.x = button2->backgroundArea.x;
			button->backgroundArea.w = button2->backgroundArea.w;
			button->backgroundArea.h = (int)((float)button2->backgroundArea.h * volumePercent);
			button->backgroundArea.y = (button2->backgroundArea.y + button2->backgroundArea.h - button->backgroundArea.h);
		}
		else
		{
			button->backgroundArea.x = button2->backgroundArea.x;
			button->backgroundArea.y = button2->backgroundArea.y;
			button->backgroundArea.h = button2->backgroundArea.h;
			button->backgroundArea.w = (int)((float)button2->backgroundArea.w * volumePercent);
		}
	}

	VM_PlayerControls::RefreshProgressBar();
	VM_PlayerControls::RefreshTime(time(NULL));

	VM_PlayerControls::refreshPending = false;

	return RESULT_OK;
}

int MediaPlayer::VM_PlayerControls::RefreshProgressBar()
{
	if (!VM_Player::State.isPlaying && !VM_PlayerControls::refreshPending)
		return ERROR_UNKNOWN;

	// DURATION
	VM_PlayerControls::durationTime = VM_MediaTime(VM_Player::DurationTime);

	char durationString[DEFAULT_CHAR_BUFFER_SIZE];
	snprintf(durationString, DEFAULT_CHAR_BUFFER_SIZE, "%02d:%02d:%02d", VM_PlayerControls::durationTime.hours, VM_PlayerControls::durationTime.minutes, VM_PlayerControls::durationTime.seconds);

	dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_duration"])->setText(durationString);

	// PROGRESS
	if (VM_PlayerControls::progressTimeLeft && !SHOUTCAST_IS_SELECTED)
		VM_PlayerControls::progressTime = VM_MediaTime((double)durationTime.totalSeconds - VM_Player::ProgressTime);
	else
		VM_PlayerControls::progressTime = VM_MediaTime(VM_Player::ProgressTime);

	char progressString[DEFAULT_CHAR_BUFFER_SIZE];
	snprintf(progressString, DEFAULT_CHAR_BUFFER_SIZE, "%02d:%02d:%02d", VM_PlayerControls::progressTime.hours, VM_PlayerControls::progressTime.minutes, VM_PlayerControls::progressTime.seconds);

	dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_progress"])->setText(progressString);

	// PROGRESS BAR
	VM_PlayerControls::progressTime = VM_MediaTime(VM_Player::ProgressTime);

	if (VM_PlayerControls::durationTime.totalSeconds > 0)
		VM_PlayerControls::progressPercent = CAP((float)((float)VM_PlayerControls::progressTime.totalSeconds / (float)VM_PlayerControls::durationTime.totalSeconds), 0.0f, 1.0f);

	VM_Component* bar   = VM_GUI::Components["bottom_player_controls_middle_bar"];
	VM_Component* thumb = VM_GUI::Components["bottom_player_controls_middle_thumb"];

	thumb->backgroundArea.x = bar->backgroundArea.x;
	thumb->backgroundArea.w = (int)((float)bar->backgroundArea.w * progressPercent);

	return RESULT_OK;
}
int MediaPlayer::VM_PlayerControls::RefreshTime(time_t time)
{
	if ((time % 60 != 0) && !VM_PlayerControls::refreshPending)
		return ERROR_UNKNOWN;

	// DATE/TIME
	VM_Button* dateTimeButton = dynamic_cast<VM_Button*>(VM_GUI::Components["date_time_text"]);

	if (dateTimeButton != NULL)
		dateTimeButton->setText(VM_Text::GetTimeFormatted(time, true));

	// FILE NAME/TITLE
	VM_Button* fileButton = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_file"]);

	if ((VM_Player::State.isPlaying || VM_PlayerControls::refreshPending) && (fileButton != NULL))
	{
		// SHOUTCAST - NOW PLAYING
		if (SHOUTCAST_IS_SELECTED && !VM_Player::SelectedRow.title.empty())
		{
			StringMap details = VM_FileSystem::GetShoutCastDetails(VM_Player::SelectedRow.title, VM_Player::SelectedRow.mediaID);
			String    title   = VM_Player::SelectedRow.title;

			if (!details["now_playing"].empty())
				title.append(" | " + details["now_playing"]);

			if (!title.empty() && (fileButton->getText() != title))
				fileButton->setText(title);
		}
		// FILE
		else if (!VM_Player::SelectedRow.title.empty() && (fileButton->getText() != VM_Player::SelectedRow.title))
		{
			fileButton->setText(VM_Player::SelectedRow.title);
		}
	}

	return RESULT_OK;
}

int MediaPlayer::VM_PlayerControls::Seek(SDL_Event* mouseEvent)
{
	if (mouseEvent == NULL)
		return ERROR_INVALID_ARGUMENTS;

	int     barClickedPos, mousePos;
	double  barClickedPercent, fileSize;
	int64_t seekPos;
	bool    seekBinary;

	if (!PICTURE_IS_SELECTED && (VM_Player::FormatContext != NULL))
		seekBinary = (SEEK_FLAGS(VM_Player::FormatContext->iformat) == AVSEEK_FLAG_BYTE);
	else
		seekBinary = false;

	VM_Button* button      = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_middle_bar"]);
	String     orientation = VM_XML::GetAttribute(button->xmlNode, "orientation");

	if (orientation == "vertical")
	{
		mousePos          = ((mouseEvent->tfinger.type == SDL_FINGERUP) ? (int)(mouseEvent->tfinger.y * (float)VM_Window::Dimensions.h) : mouseEvent->button.y);
		barClickedPos     = (mousePos - button->backgroundArea.y);
		barClickedPercent = (double)(1.0 - ((double)barClickedPos / (double)button->backgroundArea.h));
	}
	else
	{
		mousePos          = ((mouseEvent->tfinger.type == SDL_FINGERUP) ? (int)(mouseEvent->tfinger.x * (float)VM_Window::Dimensions.w) : mouseEvent->button.x);
		barClickedPos     = (mousePos - button->backgroundArea.x);
		barClickedPercent = (double)((double)barClickedPos / (double)button->backgroundArea.w);
	}

	if (seekBinary)
	{
		int  dbResult;
		auto db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

		fileSize = 0;

		if (DB_RESULT_OK(dbResult))
			fileSize = std::atof(db->getValue(VM_GUI::ListTable->getSelectedMediaID(), "size").c_str());

		DELETE_POINTER(db);

		seekPos = (int64_t)(fileSize * barClickedPercent);
	} else {
		seekPos = (int64_t)((VM_Player::DurationTime * AV_TIME_BASE_D) * barClickedPercent);
	}

	VM_Player::Seek(seekPos);

	VM_PlayerControls::refreshPending = true;

	return RESULT_OK;
}

int MediaPlayer::VM_PlayerControls::SetVolume(SDL_Event* mouseEvent)
{
	if (mouseEvent == NULL)
		return ERROR_INVALID_ARGUMENTS;

	double barClickedPercent;
	int    barClickedPos;
	int    mousePos = -1;

	VM_Button* button      = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_volume_bar"]);
	String     orientation = VM_XML::GetAttribute(button->xmlNode, "orientation");

	if (orientation == "vertical")
	{
		if (mouseEvent->tfinger.type == SDL_FINGERUP)
			mousePos = (int)(mouseEvent->tfinger.y * (double)VM_Window::Dimensions.h);
		else if (mouseEvent->button.button == SDL_BUTTON_LEFT)
			mousePos = mouseEvent->button.y;

		barClickedPos     = (mousePos - button->backgroundArea.y);
		barClickedPercent = (float)(1.0 - (double)((double)barClickedPos / (double)button->backgroundArea.h));
	}
	else
	{
		if (mouseEvent->tfinger.type == SDL_FINGERUP)
			mousePos = (int)(mouseEvent->tfinger.x * (double)VM_Window::Dimensions.w);
		else if (mouseEvent->button.button == SDL_BUTTON_LEFT)
			mousePos = mouseEvent->button.x;

		barClickedPos     = (mousePos - button->backgroundArea.x);
		barClickedPercent = (double)((double)barClickedPos / (double)button->backgroundArea.w);
	}

	VM_Player::SetAudioVolume((int)((double)SDL_MIX_MAXVOLUME * barClickedPercent));

	VM_PlayerControls::refreshPending = true;

	return RESULT_OK;
}

int MediaPlayer::VM_PlayerControls::Show(bool skipFS)
{
	VM_PlayerControls::visible = true;

	if (skipFS)
		return RESULT_OK;

	VM_Component* bottom   = VM_GUI::Components["bottom"];
	VM_Component* controls = VM_GUI::Components["bottom_player_controls"];
	VM_Component* snapshot = VM_GUI::Components["bottom_player_snapshot"];
	VM_Component* topBar   = VM_GUI::Components["top_bar"];

	controls->visible = true;

	#if !defined _android && !defined _ios
	if (VM_Window::FullScreenMaximized) {
	#endif
		snapshot->backgroundArea.y = (topBar->backgroundArea.y + topBar->backgroundArea.h);
		snapshot->backgroundArea.h = (VM_Window::Dimensions.h - snapshot->backgroundArea.y - bottom->backgroundArea.h);
	#if !defined _android && !defined _ios
	}
	#endif

	VM_Player::FreeTextures();
	VM_Player::Refresh();

	return RESULT_OK;
}

int MediaPlayer::VM_PlayerControls::ToggleProgressTimeLeft()
{
	VM_PlayerControls::progressTimeLeft = !VM_PlayerControls::progressTimeLeft;

	int  dbResult;
	auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

	if (DB_RESULT_OK(dbResult))
		db->updateSettings("progress_time_left", (VM_PlayerControls::progressTimeLeft ? "1" : "0"));

	DELETE_POINTER(db);

	VM_PlayerControls::refreshPending = true;

	return RESULT_OK;
}
