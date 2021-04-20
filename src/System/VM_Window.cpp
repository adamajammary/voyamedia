#include "VM_Window.h"

using namespace VoyaMedia::Database;
using namespace VoyaMedia::Graphics;
using namespace VoyaMedia::MediaPlayer;

int              System::VM_Window::BottomHeight        = 0;
SDL_Rect         System::VM_Window::Dimensions          = { 0, 0, MIN_WINDOW_SIZE, MIN_WINDOW_SIZE };
SDL_Rect         System::VM_Window::DimensionsBeforeFS  = { 0, 0, 0, 0 };
VM_Display       System::VM_Window::Display;
bool             System::VM_Window::FullScreenEnabled   = false;
bool             System::VM_Window::FullScreenMaximized = false;
bool             System::VM_Window::Inactive            = false;
bool             System::VM_Window::IsDoneRendering     = false;
StringMap        System::VM_Window::Labels;
SDL_Window*      System::VM_Window::MainWindow          = NULL;
bool             System::VM_Window::PauseRendering      = false;
bool             System::VM_Window::Quit                = false;
SDL_Renderer*    System::VM_Window::Renderer            = NULL;
bool             System::VM_Window::ResetRenderer       = false;
uint32_t         System::VM_Window::ResizeTimestamp     = 0;
bool             System::VM_Window::SaveToDB            = false;
int              System::VM_Window::StatusBarHeight     = 0;
String           System::VM_Window::StatusString        = "";
bool             System::VM_Window::SystemLocale        = false;
String           System::VM_Window::WorkingDirectory    = "";
WString          System::VM_Window::WorkingDirectoryW   = L"";

#if defined _android
	using namespace VoyaMedia::Android;

	Strings     System::VM_Window::AndroidMediaFiles;
	VM_JavaJNI* System::VM_Window::JNI                = NULL;
	SDL_Thread* System::VM_Window::MedaPlayerThread   = NULL;
	bool        System::VM_Window::MinimizeWindow     = false;
	bool        System::VM_Window::StartWakeLock      = false;
	bool        System::VM_Window::StopWakeLock       = false;
#endif

time_t System::VM_Window::currentTime    = time(NULL);
bool   System::VM_Window::refreshPending = false;

void System::VM_Window::Close()
{
	VM_Window::Quit = true;

	SDL_Delay(DELAY_TIME_GUI_RENDER);

	VM_ThreadManager::FreeThreads();

	VM_Player::Close();
	VM_Player::State.quit = true;

	VM_GUI::Close();

	#if defined _android
		VM_Window::JNI->destroy();
		DELETE_POINTER(VM_Window::JNI);
	#endif

	curl_global_cleanup();
	LIB_FREEIMAGE::FreeImage_DeInitialise();
	TTF_Quit();

	FREE_RENDERER(VM_Window::Renderer);
	FREE_WINDOW(VM_Window::MainWindow);

	SDL_Quit();
}

#if defined _android
int System::VM_Window::Minimize()
{
	jclass    jniClass       = VM_Window::JNI->getClass();
	JNIEnv*   jniEnvironment = VM_Window::JNI->getEnvironment();
	jmethodID jniMethod      = jniEnvironment->GetStaticMethodID(jniClass, "MinimizeWindow", "()V");

	if (jniMethod == NULL)
		return ERROR_UNKNOWN;

	jniEnvironment->CallStaticVoidMethod(jniClass, jniMethod);

	VM_Window::MinimizeWindow = false;

	return RESULT_OK;
}
#endif

int System::VM_Window::Open(const char* guiXML, const char* title)
{
	VM_ThreadManager::AddThreads();

	srand((uint32_t)time(NULL));

	if ((VM_FileSystem::SetWorkingDirectory() != RESULT_OK) || VM_Window::WorkingDirectory.empty()) {
		VM_Modal::ShowMessage("ERROR! Failed to set the working directory.");
		return ERROR_UNKNOWN;
	}

	#if defined _android
		VM_Window::JNI = new VM_JavaJNI();

		if (VM_Window::JNI->init() != RESULT_OK) {
			VM_Modal::ShowMessage(VM_Window::Labels["error.init_libs"]);
			return ERROR_UNKNOWN;
		}

	#elif defined _windows
		SetProcessDPIAware();
	#endif

	if (VM_FileSystem::CreateDefaultDirectoryStructure() != RESULT_OK) {
		VM_Modal::ShowMessage("ERROR! Failed to create default directory structure.");
		return ERROR_UNKNOWN;
	}

	VM_Window::Labels = VM_Text::GetLabels();

	if (VM_FileSystem::InitLibraries() != RESULT_OK) {
		VM_Modal::ShowMessage(VM_Window::Labels["error.init_libs"]);
		return ERROR_UNKNOWN;
	}

	#if defined _ios
		VM_EventManager::ConfigureAudioSessionIOS();
	#endif

	VM_Top::Init();
	VM_Player::Init();

	if (VM_GUI::Load(guiXML, title) != RESULT_OK) {
		VM_Modal::ShowMessage(String(VM_Window::Labels["error.load_gui"] + ": " + guiXML));
		return ERROR_UNKNOWN;
	}

	if (VM_GUI::Components.empty()) {
		VM_Modal::ShowMessage(VM_Window::Labels["error.gui_objects"]);
		return ERROR_UNKNOWN;
	}

	VM_GUI::ListTable->updateSearchInput();

	VM_ThreadManager::CreateThread(THREAD_CREATE_IMAGES);
	VM_ThreadManager::CreateThread(THREAD_GET_DATA);

	VM_Window::showVersionMessage(VM_VERSION_3);
	VM_Window::Refresh();

	return RESULT_OK;
}

void System::VM_Window::Refresh()
{
	VM_Window::refreshPending = true;
}

int System::VM_Window::Render()
{
	VM_Window::IsDoneRendering = false;
	VM_Window::Inactive        = false;

	#if defined _android
	if (VM_Window::MinimizeWindow)
		VM_Window::Minimize();
	#endif

	// REFRESH/RESIZE WINDOW UI
	if (VM_Window::refreshPending) {
		VM_Window::resize();
		VM_Window::refreshPending = false;
	}

	// SCREENSAVER
	int idleTime = (VM_Player::CursorLastVisible > 0 ? SDL_GetTicks() - VM_Player::CursorLastVisible : 0);
	
	if (!VM_Player::State.isPlaying && (idleTime > SCREENSAVER_TIMEOUT))
		VM_Window::Inactive = true;

	SDL_SetRenderDrawColor(VM_Window::Renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(VM_Window::Renderer);

	if (!VM_Window::Inactive)
	{
		// LIST TABLE
		if (VM_GUI::ListTable != NULL)
			VM_GUI::ListTable->refresh();

		// MODAL MODE
		VM_Modal::Update();

		if (VM_Modal::IsVisible())
		{
			VM_Modal::Render();
			VM_TextInput::Update();
		}
		// MAIN/NORMAL MODE
		else
		{
			VM_Button* snapshot = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_snapshot"]);
			time_t     cTime    = time(NULL);

			// PLAYER CONTROLS
			if (cTime != VM_Window::currentTime)
			{
				VM_Window::currentTime = cTime;

				if (VM_PlayerControls::IsVisible())
					VM_PlayerControls::RefreshProgressBar();
			}

			VM_PlayerControls::RefreshTime(cTime);
			VM_PlayerControls::RefreshControls();

			VM_GUI::render();
			VM_TextInput::Update();

			// SNAPSHOT IMAGE
			if (!VM_Player::State.isStopped && (snapshot != NULL))
			{
				if ((snapshot->imageData == NULL) && (VM_GUI::ListTable != NULL) && !VM_GUI::ListTable->rows.empty())
				{
					#if defined _ios
					String mediaURL = (VM_Player::State.isStopped ? VM_GUI::ListTable->getSelectedMediaURL() : VM_Player::SelectedRow.mediaURL);
					#endif

					if (AUDIO_IS_SELECTED)
					{
						VM_ThreadManager::Mutex.lock();
						snapshot->setImage(std::to_string(VM_Player::State.isStopped ? VM_GUI::ListTable->getSelectedMediaID() : VM_Player::SelectedRow.mediaID), true);
						VM_ThreadManager::Mutex.unlock();
					}
					#if defined _ios
					else if (PICTURE_IS_SELECTED && VM_FileSystem::IsITunes(mediaURL))
					{
						VM_ThreadManager::Mutex.lock();
						snapshot->setImage(mediaURL, true);
						VM_ThreadManager::Mutex.unlock();
					}
					#endif
					else if (PICTURE_IS_SELECTED || SHOUTCAST_IS_SELECTED)
					{
						VM_ThreadManager::Mutex.lock();
						snapshot->setImage((VM_Player::State.isStopped ? VM_GUI::ListTable->getSelectedMediaURL() : VM_Player::SelectedRow.mediaURL), true);
						VM_ThreadManager::Mutex.unlock();
					}
				}

				VM_Player::Render(snapshot->backgroundArea);
			}
		}
	}

	// RENDER
	SDL_RenderPresent(VM_Window::Renderer);

	VM_Window::IsDoneRendering = true;

	return RESULT_OK;
}

int System::VM_Window::Reset(const char* guiXML, const char* title)
{
	VM_TableState tableState = {};

	if (VM_GUI::ListTable != NULL)
		tableState = VM_GUI::ListTable->getState();

	if (!VM_Player::State.isStopped)
		VM_Player::FreeTextures();

	VM_GUI::Close();
	
	String modalFile = "";

	if (VM_Modal::IsVisible()) {
		modalFile = VM_Modal::File;
		VM_Modal::Hide();
	}

	VM_Window::saveToDB();
	VM_ThreadManager::FreeResources(false);

	if (VM_GUI::Load(guiXML, title) != RESULT_OK) {
		VM_Modal::ShowMessage(String(VM_Window::Labels["error.load_gui"] + ": " + guiXML).c_str());
		return ERROR_UNKNOWN;
	}

	if (VM_GUI::Components.empty()) {
		VM_Modal::ShowMessage(VM_Window::Labels["error.gui_objects"].c_str());
		return ERROR_UNKNOWN;
	}

	if (VM_GUI::ListTable != NULL)
		VM_GUI::ListTable->restoreState(tableState);

	if (!modalFile.empty())
		VM_Modal::Open(modalFile);

	VM_Window::ResetRenderer = false;

	VM_Window::Refresh();

	return RESULT_OK;
}

void System::VM_Window::resize()
{
	VM_Window::Dimensions = VM_Window::Display.getDimensions();

	SDL_Rect dimensions;
	SDL_GetWindowPosition(VM_Window::MainWindow, &dimensions.x, &dimensions.y);
	SDL_GetWindowSize(VM_Window::MainWindow, &dimensions.w, &dimensions.h);

	bool windowIsMaximized = ((SDL_GetWindowFlags(VM_Window::MainWindow) & SDL_WINDOW_MAXIMIZED) != 0);

	if (!windowIsMaximized && !VM_Window::FullScreenMaximized)
	{
		bool setMinSize = false;

	    #if defined _linux || defined _macosx || defined _windows
		if (dimensions.w < MIN_WINDOW_SIZE) {
			dimensions.w = MIN_WINDOW_SIZE;
			setMinSize   = true;
		}

		if (dimensions.h < MIN_WINDOW_SIZE) {
			dimensions.h = MIN_WINDOW_SIZE;
			setMinSize   = true;
		}

		int maxWidth  = (int)((float)dimensions.h * MAX_ASPECT_RATIO);
		int maxHeight = (int)((float)dimensions.w * MAX_ASPECT_RATIO);

		if (dimensions.w > maxWidth) {
			dimensions.w = maxWidth;
			setMinSize   = true;
		}

		if (dimensions.h > maxHeight) {
			dimensions.h = maxHeight;
			setMinSize   = true;
		}
		#endif

		if (setMinSize)
			SDL_SetWindowSize(VM_Window::MainWindow, dimensions.w, dimensions.h);

		SDL_GetWindowPosition(VM_Window::MainWindow, &VM_Window::DimensionsBeforeFS.x, &VM_Window::DimensionsBeforeFS.y);
		SDL_GetWindowSize(VM_Window::MainWindow, &VM_Window::DimensionsBeforeFS.w, &VM_Window::DimensionsBeforeFS.h);
	}

	VM_Window::saveToDB();
	VM_ThreadManager::FreeResources(false);
	VM_GUI::refresh();

	if (VM_GUI::Components["bottom"] != NULL)
		VM_Window::BottomHeight = VM_GUI::Components["bottom"]->backgroundArea.h;

	if (VM_GUI::ListTable != NULL) {
		VM_GUI::ListTable->updateNavigation();
		VM_GUI::ListTable->refreshRows();
		VM_GUI::ListTable->refreshSelected();
		VM_GUI::ListTable->refresh();
	}

	if (!VM_Player::State.isStopped || VM_Player::State.openFile) {
		VM_Player::Render(VM_PlayerControls::GetSnapshotArea());
		VM_Player::Refresh();
		VM_PlayerControls::Refresh();
		VM_PlayerControls::RefreshControls();
		VM_GUI::LoadComponents(VM_GUI::Components["bottom_player_controls"]);
	}
}

void System::VM_Window::Save()
{
	VM_Window::Dimensions = VM_Window::Display.getDimensions();

	VM_Window::saveToDB();
}

void System::VM_Window::saveToDB()
{
	int  dbResult;
	auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

	if (DB_RESULT_OK(dbResult))
	{
		bool windowIsMaximized = ((SDL_GetWindowFlags(VM_Window::MainWindow) & SDL_WINDOW_MAXIMIZED) != 0);
		db->updateSettings("window_maximized", (windowIsMaximized ? "1" : "0"));

		if (!windowIsMaximized && !VM_Window::FullScreenEnabled)
		{
			db->updateSettings("window_x", std::to_string(VM_Window::Dimensions.x));
			db->updateSettings("window_y", std::to_string(VM_Window::Dimensions.y));
			db->updateSettings("window_w", std::to_string(VM_Window::Dimensions.w));
			db->updateSettings("window_h", std::to_string(VM_Window::Dimensions.h));
		}
	}

	DELETE_POINTER(db);

	VM_Window::Display.setDisplayMode();

	VM_Window::SaveToDB = false;
}

void System::VM_Window::SetStatusProgress(uint32_t current, uint32_t total, const String &label)
{
	const uint32_t percent  = ((uint32_t)(current * 100) / total);
	VM_Window::StatusString = VM_Text::Format("[%u%%] %s (%u/%u)", percent, label.c_str(), current, total);
}

void System::VM_Window::showVersionMessage(VM_Version version)
{
	#if defined _windows
		WString versionFile;
	#else
		String versionFile;
	#endif

	switch (version) {
	case VM_VERSION_0:
	case VM_VERSION_1:
	case VM_VERSION_2:
		break;
	case VM_VERSION_3:
	#if defined _windows
		versionFile = (VM_FileSystem::GetPathDatabaseW() + L"v3.db");

		if (!VM_FileSystem::FileExists("", versionFile)) {
	#else
		versionFile = (VM_FileSystem::GetPathDatabase() + "v3.db");

		if (!VM_FileSystem::FileExists(versionFile, L"")) {
	#endif
			VM_Modal::ShowMessage(APP_UPDATE_V3_MSG);
			WRITE_FILE(versionFile);
			VM_ThreadManager::Threads[THREAD_CLEAN_THUMBS]->start = true;
		}

		break;
	default:
		break;
	}
}
