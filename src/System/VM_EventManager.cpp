#include "VM_EventManager.h"

using namespace VoyaMedia::Graphics;
using namespace VoyaMedia::MediaPlayer;
using namespace VoyaMedia::XML;

#if defined _android || defined _ios
float             System::VM_EventManager::swipeDistanceX     = 0.0f;
float             System::VM_EventManager::swipeDistanceY     = 0.0f;
uint32_t          System::VM_EventManager::touchDownTimestamp = 0;
uint32_t          System::VM_EventManager::touchUpTimestamp   = 0;
VM_TouchEventType System::VM_EventManager::TouchEvent         = TOUCH_EVENT_UNKNOWN;
#endif

#if defined _ios
int System::VM_EventManager::ConfigureAudioSessionIOS()
{
	AVAudioSession* audioSession = [AVAudioSession sharedInstance];

	if (audioSession == nil)
		return ERROR_UNKNOWN;

	// KEEP PLAYING AUDIO WHEN SCREEN IS LOCKED AND ALLOW AUDIO MIXING WITH OTHER APPS
	BOOL result = [audioSession setCategory:AVAudioSessionCategoryPlayback mode:AVAudioSessionModeMoviePlayback options:AVAudioSessionCategoryOptionMixWithOthers error:nil];

	if (result != YES)
		return ERROR_UNKNOWN;

	result = [audioSession setActive:YES error:nil];

	if (result != YES)
		return ERROR_UNKNOWN;

	NSNotificationCenter* notificationCenter = [NSNotificationCenter defaultCenter];

	if (notificationCenter == nil)
		return ERROR_UNKNOWN;

	NSOperationQueue* operationQueue = [NSOperationQueue mainQueue];

	if (operationQueue == nil)
		return ERROR_UNKNOWN;

	// DETECT IF HEADPHONES HAVE BEEN UNPLUGGED
	[notificationCenter addObserverForName:AVAudioSessionRouteChangeNotification
		object:audioSession 
		queue:operationQueue 
		usingBlock:^(NSNotification* notification) {
			VM_EventManager::handleHeadSetUnpluggedIOS(notification);
		}
	];

	return RESULT_OK;
}
#endif

int System::VM_EventManager::HandleEvents()
{
	SDL_Event event;
	int       scrollAmount;
	VM_Table* listTable = (VM_Modal::IsVisible() ? VM_Modal::ListTable : VM_GUI::ListTable);

	while (SDL_PollEvent(&event) && !VM_Window::Quit)
	{
		switch (event.type) {
		#if defined _android || defined _ios
		case SDL_FINGERMOTION:
			VM_Player::CursorLastVisible = SDL_GetTicks();
			VM_PlayerControls::Show();

			if (!VM_Player::State.isStopped)
				break;

			VM_EventManager::swipeDistanceX += event.tfinger.dx;
			VM_EventManager::swipeDistanceY += event.tfinger.dy;

			break;
		case SDL_FINGERDOWN:
			VM_Player::CursorLastVisible = SDL_GetTicks();
			VM_PlayerControls::Show();

			VM_EventManager::touchDownTimestamp = event.tfinger.timestamp;

			if (!VM_Player::State.isStopped)
				break;

			break;
		case SDL_FINGERUP:
			VM_Player::CursorLastVisible = SDL_GetTicks();
			VM_PlayerControls::Show();

			VM_EventManager::TouchEvent = TOUCH_EVENT_UNKNOWN;

			if (VM_Player::State.isStopped)
			{
				// SWIPE_RIGHT
				if (VM_EventManager::swipeDistanceX > 0.3f) {
					VM_EventManager::TouchEvent     = TOUCH_EVENT_SWIPE_RIGHT;
					VM_EventManager::swipeDistanceX = 0.0f;
				// SWIPE_LEFT
				} else if (VM_EventManager::swipeDistanceX < -0.3f) {
					VM_EventManager::TouchEvent     = TOUCH_EVENT_SWIPE_LEFT;
					VM_EventManager::swipeDistanceX = 0.0f;
				// SWIPE_DOWN
				} else if (VM_EventManager::swipeDistanceY > 0.3f) {
					VM_EventManager::TouchEvent     = TOUCH_EVENT_SWIPE_DOWN;
					VM_EventManager::swipeDistanceY = 0.0f;
				// SWIPE_UP
				} else if (VM_EventManager::swipeDistanceY < -0.3f) {
					VM_EventManager::TouchEvent     = TOUCH_EVENT_SWIPE_UP;
					VM_EventManager::swipeDistanceY = 0.0f;
				}
			}

			if (VM_EventManager::TouchEvent == TOUCH_EVENT_UNKNOWN)
			{
				// RIGHT-CLICK / LONG PRESS
				if ((event.tfinger.timestamp - VM_EventManager::touchDownTimestamp) > 500) {
					VM_EventManager::TouchEvent = TOUCH_EVENT_LONG_PRESS;
				// DOUBLE-CLICK
				} else if ((event.tfinger.timestamp - VM_EventManager::touchUpTimestamp) < 300) {
					VM_EventManager::TouchEvent = TOUCH_EVENT_DOUBLE_TAP;
				// NORMAL
				} else {
					VM_EventManager::TouchEvent = TOUCH_EVENT_NORMAL;
				}
			}

			VM_EventManager::touchDownTimestamp = 0;
			VM_EventManager::touchUpTimestamp   = event.tfinger.timestamp;

			switch (VM_EventManager::TouchEvent) {
				case TOUCH_EVENT_SWIPE_UP:
					listTable->selectRow(listTable->getSelectedRowIndex() + 1);

					if (!listTable->isRowVisible())
						listTable->scroll(1);

					break;
				case TOUCH_EVENT_SWIPE_DOWN:
					listTable->selectRow(listTable->getSelectedRowIndex() - 1);

					if (!listTable->isRowVisible())
						listTable->scroll(-1);

					break;
				case TOUCH_EVENT_SWIPE_LEFT:
					listTable->offsetNext();
					break;
				case TOUCH_EVENT_SWIPE_RIGHT:
					listTable->offsetPrev();
					break;
				default:
					VM_EventManager::handleMouseClick(&event);
					break;
			}

			break;
		#else
		case SDL_MOUSEWHEEL:
			VM_Player::CursorShow();

			// SCROLL MOUSE WHEEL
			if (event.wheel.y != 0)
			{
				scrollAmount = (std::signbit((double)event.wheel.y) ? 1 : -1);
				listTable->selectRow(listTable->getSelectedRowIndex() + scrollAmount);

				if (!listTable->isRowVisible())
					listTable->scroll(scrollAmount);
			}

			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEMOTION:
			VM_Player::CursorShow();
			break;
		case SDL_MOUSEBUTTONUP:
			VM_Player::CursorShow();

			if ((event.button.timestamp - VM_Window::ResizeTimestamp) > ONE_SECOND_MS)
				VM_EventManager::handleMouseClick(&event);

			break;
		#endif
		#if defined _windows
		case SDL_SYSWMEVENT:
			if ((event.syswm.msg != NULL) && (event.syswm.msg->subsystem == SDL_SYSWM_WINDOWS)) {
				switch (event.syswm.msg->msg.win.msg) {
					case WM_QUERYENDSESSION: case WM_ENDSESSION: VM_Window::Quit = true; break;
				}
			}
			break;
		#endif
		case SDL_QUIT:
			VM_Window::Quit = true;
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_CLOSE:
				VM_Window::Quit = true;
				break;
			case SDL_WINDOWEVENT_MINIMIZED:
				VM_Window::PauseRendering = true;

				if (!AUDIO_IS_SELECTED && !SHOUTCAST_IS_SELECTED && VM_Player::State.isPlaying)
					VM_Player::Pause();

				VM_Window::Refresh();

				break;
			case SDL_WINDOWEVENT_MOVED:
				VM_Window::SaveToDB = true;
				break;
			case SDL_WINDOWEVENT_RESTORED:
				VM_Player::CursorShow();

				VM_Window::PauseRendering = false;
				VM_Window::ResetRenderer  = true;

				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				VM_Window::ResetRenderer   = true;
				VM_Window::ResizeTimestamp = event.window.timestamp;

				break;
			default:
				break;
			}
			break;
		case SDL_DROPFILE:
			VM_FileSystem::OpenFile(event.drop.file);
			SDL_free(event.drop.file);
			break;
		case SDL_TEXTINPUT:
			VM_TextInput::Update(event.text.text);
			break;
		case SDL_KEYDOWN:
			if (VM_TextInput::IsActive())
			{
				switch (event.key.keysym.sym) {
					case SDLK_BACKSPACE: VM_TextInput::Backspace(); break;
					case SDLK_DELETE:    VM_TextInput::Delete();    break;
				}
			}
			break;
		case SDL_KEYUP:
			VM_Player::CursorShow();

			// TEXT INPUT / MODAL
			if (VM_TextInput::IsActive() || VM_Modal::IsVisible())
			{
				if (VM_TextInput::IsActive())
				{
					VM_EventManager::isKeyPressedTextInput(event.key.keysym.sym, event.key.keysym.mod);
				}
				else if (VM_Modal::IsVisible())
				{
					if (!VM_EventManager::isKeyPressedTable(event.key.keysym.sym, VM_Modal::ListTable))
					{
						switch (event.key.keysym.sym) {
							case SDLK_ESCAPE: case SDLK_AC_BACK: case SDLK_AUDIOSTOP:
								VM_Modal::Hide();
								break;
						}
					}
				}

			}
			// MEDIA PLAYER IS ACTIVE
			else if (!VM_Player::State.isStopped)
			{
				VM_EventManager::isKeyPressedPlayer(event.key.keysym.sym, event.key.keysym.mod);
			}
			else
			{
				VM_EventManager::isKeyPressedTable(event.key.keysym.sym, VM_GUI::ListTable);

				#if defined _android
					switch (event.key.keysym.sym) {
						case SDLK_AC_BACK: VM_Window::MinimizeWindow = true; break;
					}
				#endif
			}

			VM_Player::CursorShow();

			break;
		default:
			break;
		}
	}

	return RESULT_OK;
}

int System::VM_EventManager::HandleEventsMobile(void* userdata, SDL_Event* event)
{
	switch (event->type) {
	case SDL_APP_LOWMEMORY:
		VM_Modal::ShowMessage("The device is low on memory, please restart the application.");
		return RESULT_OK;
	case SDL_APP_TERMINATING:
		VM_Window::Quit = true;
		return RESULT_OK;
	case SDL_APP_WILLENTERBACKGROUND:
		VM_Window::PauseRendering = true;

		if (!AUDIO_IS_SELECTED && !SHOUTCAST_IS_SELECTED && VM_Player::State.isPlaying)
			VM_Player::PlayPauseToggle();

		#if defined _android
		if (VM_Player::State.isPlaying)
			VM_Window::StartWakeLock = true;
		#endif

		VM_Window::Refresh();

		return RESULT_OK;
	case SDL_APP_DIDENTERBACKGROUND:
		return RESULT_OK;
	case SDL_APP_WILLENTERFOREGROUND:
		return RESULT_OK;
	case SDL_APP_DIDENTERFOREGROUND:
		#if defined _android
			VM_Window::StopWakeLock = true;
		#endif

		VM_Window::PauseRendering = false;
		VM_Window::ResetRenderer  = true;
		VM_Player::CursorShow();

		return RESULT_OK;
	default:
		break;
	}

	return 1;
}

#if defined _android
int System::VM_EventManager::HandleHeadSetUnpluggedAndroid(JNIEnv* jniEnvironment)
{
	if (!VM_Player::State.isPlaying)
		return RESULT_OK;

	if (jniEnvironment == NULL)
		return ERROR_UNKNOWN;

	jclass    jniClass              = VM_Window::JNI->getClass();
	jmethodID jniIsHeadSetUnplugged = jniEnvironment->GetStaticMethodID(jniClass, "IsHeadSetUnplugged", "()Z");
	jmethodID jniIsAudioModeNormal  = jniEnvironment->GetStaticMethodID(jniClass, "IsAudioModeNormal",  "()Z");
	jmethodID jniResetHeadsetState  = jniEnvironment->GetStaticMethodID(jniClass, "ResetHeadsetState",  "()V");

	if ((jniIsHeadSetUnplugged == NULL) || (jniIsAudioModeNormal == NULL) || (jniResetHeadsetState == NULL))
		return ERROR_UNKNOWN;

	// Check for incoming/outgoing calls
	if (!jniEnvironment->CallStaticBooleanMethod(jniClass, jniIsAudioModeNormal))
		VM_Player::Pause();

	// Check if the headset has been unplugged
	if (jniEnvironment->CallStaticBooleanMethod(jniClass, jniIsHeadSetUnplugged)) {
		VM_Player::Pause();
		jniEnvironment->CallStaticVoidMethod(jniClass, jniResetHeadsetState);
	}

	return RESULT_OK;
}
#endif

#if defined _ios
int System::VM_EventManager::handleHeadSetUnpluggedIOS(NSNotification* notification)
{
	if (!VM_Player::State.isPlaying)
		return RESULT_OK;

	if (notification.userInfo != nil)
	{
		NSNumber* reason = [notification.userInfo objectForKey:AVAudioSessionRouteChangeReasonKey];

		// PAUSE PLAYER IF HEADSET WAS UNPLUGGED
		if ((reason != nil) && ([reason unsignedLongValue] != AVAudioSessionRouteChangeReasonNewDeviceAvailable))
			VM_Player::PlayPauseToggle();
	}

	return RESULT_OK;
}
#endif

int System::VM_EventManager::HandleMediaPlayer()
{
	// TRY OPENING THE FILE IF REQUESTED
	if (VM_Player::State.openFile)
	{
		// SAVE FILE PATH BEFORE CLOSING
		if (VM_Player::State.quit)
		{
			String filePath = VM_Player::State.filePath;
			VM_Player::Close();
			VM_Player::State.filePath = filePath;
		}

		// GET NEW FILEPATH IF THE TABLE IS REFRESHING
		if (VM_Player::State.filePath == REFRESH_PENDING)
			VM_Player::State.filePath = VM_GUI::ListTable->getSelectedFile();

		VM_Buttons row = VM_GUI::ListTable->getSelectedRow();

		// TRY OPENING THE FILE
		if (VM_Player::Open() != RESULT_OK)
		{
			// FAILED TO OPEN THE FILE
			VM_Player::State.openFile = false;
			VM_Player::FullScreenExit(true);

			String mediaID = "";
			String file    = VM_Player::State.filePath;

			if (!row.empty()) {
				file    = row[1]->getText();
				mediaID = row[1]->mediaID2;
			}

			snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s '%s'", VM_Window::Labels["error.open"].c_str(), file.c_str());
			VM_Modal::ShowMessage(VM_Window::StatusString);

			if (YOUTUBE_IS_SELECTED && !mediaID.empty())
				VM_FileSystem::OpenWebBrowser(String("https://www.youtube.com/embed/" + mediaID + "?autoplay=true"));
		}
		// UPDATE SELECTED ROW ON SUCCESS
		else if ((VM_Player::State.filePath != REFRESH_PENDING) && (row.size() > 1))
		{
			VM_Player::SelectedRow.mediaID  = row[1]->mediaID;
			VM_Player::SelectedRow.mediaID2 = row[1]->mediaID2;
			VM_Player::SelectedRow.mediaURL = row[1]->mediaURL;
			VM_Player::SelectedRow.title    = row[1]->getText();
		}
	}

	// CLOSE THE FILE IF REQUESTED
	if (VM_Player::State.quit)
	{
		String errorMessage = VM_Player::State.errorMessage;

		VM_Player::Close();

		if (!VM_Player::State.fullscreenEnter && !VM_Player::State.openFile && (VM_Player::State.filePath != REFRESH_PENDING))
			VM_Player::FullScreenExit();

		if (!errorMessage.empty()) {
			snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s", errorMessage.c_str());
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, APP_NAME.c_str(), VM_Window::StatusString, NULL);
		}
	}

	return RESULT_OK;
}

int System::VM_EventManager::handleMouseClick(SDL_Event* mouseEvent)
{
	int result = ERROR_UNKNOWN;

	//if (VM_Modal::IsVisible())
	//{
	//	if (VM_EventManager::isClickedTextInput(mouseEvent, VM_Modal::TextInput))
	//		result = RESULT_OK;
	//	else if (VM_EventManager::isClickedModal(mouseEvent))
	//		result = RESULT_OK;
	//	else if (VM_EventManager::isClickedTableBottom(mouseEvent, VM_Modal::Components["list_table_bottom"], VM_Modal::ListTable))
	//		result = RESULT_OK;
	//	else if (VM_EventManager::isClickedTable(mouseEvent, VM_Modal::ListTable, VM_Modal::Components["list_table_scrollbar"]))
	//		result = RESULT_OK;
	//}
	//else
	//{
	//	if (VM_EventManager::isClickedTextInput(mouseEvent, VM_GUI::TextInput))
	//		result = RESULT_OK;
	//	else if (VM_EventManager::isClickedTopBar(mouseEvent))
	//		result = RESULT_OK;
		/*else*/ if (VM_EventManager::isClickedBottom(mouseEvent))
			result = RESULT_OK;
	//	else if (VM_EventManager::isClickedTop(mouseEvent))
	//		result = RESULT_OK;
	//	else if (VM_EventManager::isClickedTableBottom(mouseEvent, VM_GUI::Components["list_table_bottom"], VM_GUI::ListTable))
	//		result = RESULT_OK;
	//	else if (VM_EventManager::isClickedTable(mouseEvent, VM_GUI::ListTable, VM_GUI::Components["list_table_scrollbar"]))
	//		result = RESULT_OK;
	//}

	return result;
}

bool System::VM_EventManager::isClickedBottom(SDL_Event* mouseEvent)
{
	bool result = false;

	if ((mouseEvent == NULL) || (VM_GUI::Components["bottom"] == NULL))
		return result;

	if (VM_Player::State.isStopped)
		result = VM_EventManager::isClickedBottomControls(mouseEvent);
	else
		result = VM_EventManager::isClickedBottomPlayerControls(mouseEvent);

	if (!result && VM_Graphics::ButtonPressed(mouseEvent, VM_GUI::Components["bottom"]->backgroundArea)) {
		VM_TextInput::SetActive(false);
		return true;
	}

	return result;
}

bool System::VM_EventManager::isClickedBottomControls(SDL_Event* mouseEvent)
{
	if ((mouseEvent == NULL) || (VM_GUI::Components["bottom_controls"] == NULL) || !VM_Player::State.isStopped)
		return false;

	for (auto button : VM_GUI::Components["bottom_controls"]->buttons)
	{
		if (!VM_Graphics::ButtonPressed(mouseEvent, button->backgroundArea))
			continue;

		if (button->id == "bottom_controls_browse")
		{
			#if defined _android
				VM_ThreadManager::Threads[THREAD_SCAN_ANDROID]->start = true;
			#elif defined _ios
				VM_ThreadManager::Threads[THREAD_SCAN_ITUNES]->start = true;
			#else
				VM_FileSystem::OpenFile(VM_FileSystem::OpenFileBrowser(true));
			#endif
		}
		else if (button->id == "bottom_controls_playlists")
		{
			VM_Modal::Open(VM_XML::GetAttribute(button->xmlNode, "modal"));
		}
		else if (button->id == "bottom_controls_upnp")
		{
			// INVALID MEDIA TYPE
			if (VM_Top::Selected >= MEDIA_TYPE_YOUTUBE) {
				snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s", VM_Window::Labels["error.share_invalid"].c_str());
				VM_Modal::ShowMessage(VM_Window::StatusString);
			// NO NICS
			} else if (VM_FileSystem::GetNetworkInterfaces().empty()) {
				snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s", VM_Window::Labels["error.no_nics"].c_str());
				VM_Modal::ShowMessage(VM_Window::StatusString);
			// VALID
			} else {
				VM_Modal::Open(VM_XML::GetAttribute(button->xmlNode, "modal"));
			}
		}
		else if (button->id == "bottom_controls_dropbox")
		{
			// NO INTERNET
			if (!VM_FileSystem::HasInternetConnection()) {
				snprintf(VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s", VM_Window::Labels["error.no_nics"].c_str());
				VM_Modal::ShowMessage(VM_Window::StatusString);
			// VALID
			} else {
				VM_Modal::Open(VM_XML::GetAttribute(button->xmlNode, "modal"));
			}
		}
		else if (button->id == "bottom_controls_settings")
		{
			VM_Modal::Open(VM_XML::GetAttribute(button->xmlNode, "modal"));
		}

		return true;
	}

	return false;
}

bool System::VM_EventManager::isClickedBottomPlayerControls(SDL_Event* mouseEvent)
{
	if ((mouseEvent == NULL) || (VM_GUI::Components["bottom_player_controls"] == NULL) || VM_Player::State.isStopped)
		return false;

	if ((VM_GUI::Components["bottom_player_controls_controls_right"] != NULL))
	{
		for (auto button : VM_GUI::Components["bottom_player_controls_controls_right"]->buttons)
		{
			if (!VM_Graphics::ButtonPressed(mouseEvent, button->backgroundArea))
				continue;

			// MUTE
			if (button->id == "bottom_player_controls_mute") {
				VM_Player::MuteToggle();
			// VOLUME
			} else if (button->id == "bottom_player_controls_volume_bar") {
				VM_PlayerControls::SetVolume(mouseEvent);
			}
			else if (!VM_Player::State.isStopped)
			{
				// SETTINGS
				if ((button->id == "bottom_player_controls_settings") && VIDEO_IS_SELECTED)
					VM_Modal::Open(VM_XML::GetAttribute(button->xmlNode, "modal"));
				// FULLSCREEN
				else if (button->id == "bottom_player_controls_fullscreen")
					#if defined _android || defined _ios
						VM_PlayerControls::Hide();
					#else
						VM_Player::FullScreenToggle(false);
					#endif
				// STRETCH
				else if (button->id == "bottom_player_controls_stretch")
					VM_Player::KeepAspectRatioToggle();
				// ROTATE
				else if (button->id == "bottom_player_controls_rotate")
					VM_Player::RotatePicture();
				// PLAYLIST
				else if (button->id == "bottom_player_controls_playlist")
					VM_Player::PlaylistLoopTypeToggle();
			}

			VM_PlayerControls::Refresh();

			return true;
		}
	}

	if (VM_Player::State.isStopped)
		return false;

	if ((VM_GUI::Components["bottom_player_controls_controls_left"] != NULL))
	{
		for (auto button : VM_GUI::Components["bottom_player_controls_controls_left"]->buttons)
		{
			if (!VM_Graphics::ButtonPressed(mouseEvent, button->backgroundArea))
				continue;

			// PREVIOUS
			if (button->id == "bottom_player_controls_prev")
				VM_Player::OpenPrevious(true);
			// NEXT
			else if (button->id == "bottom_player_controls_next")
				VM_Player::OpenNext(true);
			// PLAY/PAUSE
			else if (button->id == "bottom_player_controls_play")
				VM_Player::PlayPauseToggle();
			// STOP
			else if (button->id == "bottom_player_controls_stop")
				VM_Player::FullScreenExit(true);

			VM_PlayerControls::Refresh();

			return true;
		}
	}

	if (VM_GUI::Components["bottom_player_controls_left"] != NULL)
	{
		for (auto button : VM_GUI::Components["bottom_player_controls_left"]->buttons)
		{
			if (!VM_Graphics::ButtonPressed(mouseEvent, button->backgroundArea))
				continue;

			// PROGRESS: TIME PASSED VS. TIME LEFT
			if ((button->id == "bottom_player_controls_progress") && !VM_Player::State.isStopped && !SHOUTCAST_IS_SELECTED)
				VM_PlayerControls::ToggleProgressTimeLeft();

			return true;
		}
	}

	if (VM_GUI::Components["bottom_player_controls_middle"] != NULL)
	{
		for (auto button : VM_GUI::Components["bottom_player_controls_middle"]->buttons)
		{
			if (!VM_Graphics::ButtonPressed(mouseEvent, button->backgroundArea))
				continue;

			// SEEK BAR
			if ((button->id == "bottom_player_controls_middle_bar") && !VM_Player::State.isStopped && !SHOUTCAST_IS_SELECTED)
				VM_PlayerControls::Seek(mouseEvent);

			return true;
		}
	}

	VM_Component* snapshot = VM_GUI::Components["bottom_player_snapshot"];

	if (snapshot != NULL)
	{
		#if !defined _android && !defined _ios
		// VIDEO-DOUBLE-CLICK -> FULLSCREEN
		if (VM_Graphics::ButtonPressed(mouseEvent, snapshot->backgroundArea, false, true))
		{
			VM_Player::FullScreenToggle(false);

			if (VM_Player::State.isPaused && (VM_Player::State.fullscreenEnter || VM_Player::State.fullscreenExit))
				VM_Player::Play();

			VM_PlayerControls::Refresh();

			return true;
		}
		else
		#endif
		// VIDEO-SINGLE-CLICK -> PLAY/PAUSE TOGGLE
		if (VM_Graphics::ButtonPressed(mouseEvent, snapshot->backgroundArea))
		{
			VM_Player::PlayPauseToggle();
			VM_PlayerControls::Refresh();
			return true;
		}
	}

	return false;
}

bool System::VM_EventManager::isClickedModal(SDL_Event* mouseEvent)
{
	if ((mouseEvent == NULL) ||
		(VM_Modal::Components["modal_bottom"] == NULL) ||
		(VM_Modal::Components["modal_middle"] == NULL) ||
		!VM_Modal::IsVisible())
	{
		return false;
	}

	for (auto component : VM_Modal::Components["modal_middle"]->buttons)
	{
		if (!component->visible)
			continue;

		if (VM_Graphics::ButtonPressed(mouseEvent, component->backgroundArea))
		{
			VM_Button* button = dynamic_cast<VM_Button*>(component);

			bool hide = false;

			if (button->id == "modal_player_settings_audio") {
				VM_Modal::Open(VM_XML::GetAttribute(button->xmlNode, "modal"));
			} else if (button->id == "modal_player_settings_subs") {
				VM_Modal::Open(VM_XML::GetAttribute(button->xmlNode, "modal"));
			} else if (button->id == "modal_player_settings_info") {
				VM_Modal::Open(VM_XML::GetAttribute(button->xmlNode, "modal"));
			} else if (button->id == "modal_settings_clean_db") {
				VM_ThreadManager::Threads[THREAD_CLEAN_DB]->start = true;
				hide = true;
			} else if (button->id == "modal_settings_clean_thumbs") {
				VM_ThreadManager::Threads[THREAD_CLEAN_THUMBS]->start = true;
				hide = true;
			} else if (button->id == "modal_settings_color") {
				VM_Modal::Apply(button->id);
				hide = true;
			} else if (button->id == "modal_settings_lang") {
				VM_Modal::Apply(button->id);
				hide = true;
			} else if (button->id == "modal_right_click_remove_file") {
				VM_Modal::Apply(button->id);
				hide = true;
			} else if ((button->id == "modal_right_click_tmbd_movie") || (button->id == "modal_right_click_tmbd_tv")) {
				VM_Modal::Apply(button->id);
				hide = true;
			}

			if (hide) {
				VM_TextInput::SetActive(false);
				VM_Modal::Hide();
				VM_Window::Refresh();
			}

			return true;
		}
	}

	for (auto component : VM_Modal::Components["modal_bottom"]->buttons)
	{
		if (!component->visible)
			continue;

		if (VM_Graphics::ButtonPressed(mouseEvent, component->backgroundArea))
		{
			VM_Button* button = dynamic_cast<VM_Button*>(component);

			int result = RESULT_OK;

			if ((button->id == "modal_ok")       || (button->id == "modal_playlists_remove") ||
				(button->id == "modal_app_url")  || (button->id == "modal_paypal_url")       ||
				(button->id == "modal_upnp_add") || (button->id == "modal_upnp_share"))
			{
				result = VM_Modal::Apply(button->id);
			}

			if (result == RESULT_OK)
			{
				if (VM_Modal::File.find("modal_player_settings_") != String::npos) {
					VM_Modal::Open("modal_player_settings");
				} else if ((VM_Modal::File == "modal_details") && !VM_Player::State.isStopped) {
					VM_Modal::Open("modal_player_settings");
				} else if (button->id == "modal_playlists_remove") {
					VM_Modal::Open("modal_playlists");
				} else {
					VM_TextInput::SetActive(false);
					VM_Modal::Hide();
					VM_Window::Refresh();
				}
			}

			return true;
		}
	}

	return false;
}

bool System::VM_EventManager::isClickedTable(SDL_Event* mouseEvent, VM_Table* table, VM_Component* scrollBar)
{
	if ((mouseEvent == NULL) || (table == NULL) || (table->scrollBar == NULL) || (scrollBar == NULL))
		return false;

	// SCROLL TABLE
	for (auto button : table->scrollBar->buttons)
	{
		if (!VM_Graphics::ButtonPressed(mouseEvent, button->backgroundArea))
			continue;

		int scrollAmount = 0;

		if (button->id.find("_scrollbar_next") != String::npos)
			scrollAmount = 1;
		else if (button->id.find("_scrollbar_prev") != String::npos)
			scrollAmount = -1;

		if (scrollAmount != 0)
		{
			table->selectRow(table->getSelectedRowIndex() + scrollAmount);

			if (!table->isRowVisible())
				table->scroll(scrollAmount);
		}

		return true;
	}

	// SORT TABLE BY COLUMN
	if ((table->id == "list_table") || (table->id == "modal_playlists_list_table"))
	{
		for (int i = 1; i < (int)table->buttons.size() - 1; i++)
		{
			if (!VM_Graphics::ButtonPressed(mouseEvent, table->buttons[i]->backgroundArea))
				continue;

			table->sort(table->buttons[i]->id);

			return true;
		}
	}

	// SELECT TABLE ROW
	table->selectRow(mouseEvent);

	if (VM_Graphics::ButtonPressed(mouseEvent, table->backgroundArea)) {
		VM_TextInput::SetActive(false);
		return true;
	}

	return false;
}

bool System::VM_EventManager::isClickedTableBottom(SDL_Event* mouseEvent, VM_Component* bottomPanel, VM_Table* table)
{
	if ((mouseEvent == NULL) || (bottomPanel == NULL) || (table == NULL))
		return false;

	for (auto button : bottomPanel->buttons)
	{
		if (!VM_Graphics::ButtonPressed(mouseEvent, button->backgroundArea))
			continue;

		if (button->id == "list_offset_prev")
			table->offsetPrev();
		else if (button->id == "list_offset_next")
			table->offsetNext();

		return true;
	}

	if (VM_Graphics::ButtonPressed(mouseEvent, bottomPanel->backgroundArea)) {
		VM_TextInput::SetActive(false);
		return true;
	}

	return false;
}

bool System::VM_EventManager::isClickedTextInput(SDL_Event* mouseEvent, VM_Component* inputPanel)
{
	if ((mouseEvent == NULL) || (inputPanel == NULL) || !VM_Player::State.isStopped)
		return false;

	VM_Button* input = NULL;

	for (auto component : inputPanel->buttons)
	{
		VM_Button* button = dynamic_cast<VM_Button*>(component);

		if (button->id.find("_input") != String::npos)
			input = button;

		if (VM_Graphics::ButtonPressed(mouseEvent, button->backgroundArea))
		{
			// INPUT
			if (button->id.find("_input") != String::npos)
			{
				VM_TextInput::SetActive(true, button);
			}
			// PASTE
			else if (button->id.find("_paste") != String::npos)
			{
				bool activate = (!VM_TextInput::IsActive() && (input != NULL));

				if (activate)
					VM_TextInput::SetActive(true, input);

				VM_TextInput::Paste();

				if (activate)
					VM_TextInput::Unfocus();
			}
			// SEARCH
			else if (button->id == "middle_search_button")
			{
				VM_TextInput::SaveToDB();
				VM_TextInput::SetActive(false);

				if (!VM_Modal::IsVisible())
				{
					if (VM_GUI::ListTable != NULL)
						VM_GUI::ListTable->resetState();

					VM_Window::Refresh();
				}
			}
			// SAVE SEARCH TO PLAYLIST
			else if (button->id == "middle_search_playlist_save")
			{
				VM_Modal::Open(VM_XML::GetAttribute(button->xmlNode, "modal"));
			}

			return true;
		}
	}

	VM_TextInput::SetActive(false);

	return false;
}

bool System::VM_EventManager::isClickedTop(SDL_Event* mouseEvent)
{
	if ((mouseEvent == NULL) || !VM_Player::State.isStopped)
		return false;

	for (auto button : VM_GUI::Components["top"]->buttons)
	{
		if (!VM_Graphics::ButtonPressed(mouseEvent, button->backgroundArea))
			continue;

		VM_Top::SelectType(VM_Top::IdToMediaType(button->id));

		return true;
	}

	if (VM_Graphics::ButtonPressed(mouseEvent, VM_GUI::Components["top"]->backgroundArea)) {
		VM_TextInput::SetActive(false);
		return true;
	}

	return false;
}

bool System::VM_EventManager::isClickedTopBar(SDL_Event* mouseEvent)
{
	if ((mouseEvent == NULL) || (VM_GUI::Components["top_bar"] == NULL))
		return false;

	for (auto component : VM_GUI::Components["top_bar"]->buttons)
	{
		if (VM_Graphics::ButtonPressed(mouseEvent, component->backgroundArea))
		{
			VM_Button* button = dynamic_cast<VM_Button*>(component);

			if (button->id == "about")
			{
				VM_Modal::ShowMessage(APP_ABOUT_TEXT);

				if (VM_Player::State.isPlaying)
					VM_Player::Pause();
			}

			return true;
		}
	}

	if (VM_Graphics::ButtonPressed(mouseEvent, VM_GUI::Components["top_bar"]->backgroundArea)) {
		VM_TextInput::SetActive(false);
		return true;
	}

	return false;
}

bool System::VM_EventManager::isKeyPressedPlayer(SDL_Keycode key, uint16_t mod)
{
	switch (key) {
	case SDLK_ESCAPE:
	case SDLK_AC_BACK:
	case SDLK_AUDIOSTOP:
		VM_Player::FullScreenExit(true);
		return true;
	case SDLK_SPACE:
	case SDLK_RETURN:
	case SDLK_RETURN2:
	case SDLK_KP_ENTER:
	case SDLK_AUDIOPLAY:
		VM_Player::PlayPauseToggle();
		return true;
	case SDLK_DOWN:
	case SDLK_RIGHT:
	case SDLK_AUDIONEXT:
		VM_Player::OpenNext(true);
		return true;
	case SDLK_UP:
	case SDLK_LEFT:
	case SDLK_AUDIOPREV:
		VM_Player::OpenPrevious(true);
		return true;
	case SDLK_a:
		VM_Modal::Open("modal_player_settings_audio");
		return true;
	case SDLK_f:
		VM_Player::FullScreenToggle(false);
		return true;
	case SDLK_s:
		VM_Modal::Open("modal_player_settings_subs");
		return true;
	}

	return false;
}

bool System::VM_EventManager::isKeyPressedTable(SDL_Keycode key, VM_Table* table)
{
	if (table == NULL)
		return false;

	switch (key) {
	case SDLK_RETURN:
	case SDLK_RETURN2:
	case SDLK_KP_ENTER:
	case SDLK_AUDIOPLAY:
		if (TMDB_MOVIE_IS_SELECTED || TMDB_TV_IS_SELECTED)
		{
			auto row = table->getSelectedRow();

			if (!row.empty())
				VM_Modal::Open(VM_XML::GetAttribute(row[row.size() - 1]->xmlNode, "modal"));
		}
		else if (table->id == "list_table")
		{
			#if defined _windows
				VM_Player::OpenFilePath(VM_Text::ToUTF16(table->getSelectedMediaURL().c_str()));
			#else
				VM_Player::OpenFilePath(table->getSelectedMediaURL());
			#endif
		}
		else
		{
			if (VM_Modal::Apply("modal_ok") == RESULT_OK) {
				VM_TextInput::SetActive(false);
				VM_Modal::Hide();
				VM_Window::Refresh();
			}
		}

		return true;
	case SDLK_AUDIONEXT:
	case SDLK_DOWN:
		table->selectRow(table->getSelectedRowIndex() + 1);

		if (!table->isRowVisible())
			table->scroll(1);

		return true;
	case SDLK_AUDIOPREV:
	case SDLK_UP:
		table->selectRow(table->getSelectedRowIndex() - 1);

		if (!table->isRowVisible())
			table->scroll(-1);

		return true;
	case SDLK_RIGHT:
		table->offsetNext();
		return true;
	case SDLK_LEFT:
		table->offsetPrev();
		return true;
	case SDLK_PAGEDOWN:
		table->selectRow(table->getSelectedRowIndex() + 5);

		if (!table->isRowVisible())
			table->scroll(5);

		return true;
	case SDLK_PAGEUP:
		table->selectRow(table->getSelectedRowIndex() - 5);

		if (!table->isRowVisible())
			table->scroll(-5);

		return true;
	case SDLK_HOME:
		table->selectRow(0);

		if (!table->isRowVisible())
			table->scrollTo(0);

		return true;
	case SDLK_END:
		table->selectRow((int)table->rows.size() - 1);

		if (!table->isRowVisible())
			table->scrollTo((int)table->rows.size() - 1);

		return true;
	}

	return false;
}

bool System::VM_EventManager::isKeyPressedTextInput(SDL_Keycode key, uint16_t mod)
{
	switch (key) {
	case SDLK_LEFT:
	case SDLK_AUDIOPREV:
		VM_TextInput::Left();
		return true;
	case SDLK_RIGHT:
	case SDLK_AUDIONEXT:
		VM_TextInput::Right();
		return true;
	case SDLK_HOME:
		VM_TextInput::Home();
		return true;
	case SDLK_END:
		VM_TextInput::End();
		return true;
	case SDLK_KP_ENTER:
	case SDLK_RETURN:
	case SDLK_RETURN2:
	case SDLK_AUDIOPLAY:
		VM_TextInput::SaveToDB();
		VM_TextInput::SetActive(false);

		if (!VM_Modal::IsVisible())
		{
			if (VM_GUI::ListTable != NULL)
				VM_GUI::ListTable->resetState();

			VM_Window::Refresh();
		}

		return true;
	case SDLK_ESCAPE:
	case SDLK_AC_BACK:
	case SDLK_AUDIOSTOP:
		VM_TextInput::SetActive(false);
		return true;
	case SDLK_v:
		if (mod & KMOD_CTRL)
			VM_TextInput::Paste();

		return true;
	}

	return false;
}

#if defined _android
void System::VM_EventManager::WakeLockStart()
{
	jclass    jniClass         = VM_Window::JNI->getClass();
	JNIEnv*   jniEnvironment   = VM_Window::JNI->getEnvironment();
	jmethodID jniWakeLockStart = jniEnvironment->GetStaticMethodID(jniClass, "WakeLockStart", "()V");

	if (jniWakeLockStart != NULL)
		jniEnvironment->CallStaticVoidMethod(jniClass, jniWakeLockStart);

	VM_Window::StartWakeLock = false;
}

void System::VM_EventManager::WakeLockStop()
{
	jclass    jniClass        = VM_Window::JNI->getClass();
	JNIEnv*   jniEnvironment  = VM_Window::JNI->getEnvironment();
	jmethodID jniWakeLockStop = jniEnvironment->GetStaticMethodID(jniClass, "WakeLockStop", "()V");

	if (jniWakeLockStop != NULL)
		jniEnvironment->CallStaticVoidMethod(jniClass, jniWakeLockStop);

	VM_Window::StopWakeLock = false;
}
#endif
