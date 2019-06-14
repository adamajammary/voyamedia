#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

using namespace VoyaMedia::Graphics;
using namespace VoyaMedia::MediaPlayer;
using namespace VoyaMedia::System;

// Developer mode on Windows
#if defined _windows && defined _DEBUG
int wmain(const int argc, wchar_t* argv[])
// Release mode on Windows
#elif defined _windows && defined NDEBUG
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
// Release mode on all other platforms
#else
int main(const int argc, char* argv[])
#endif
{
	// Get command line arguments in Windows Release mode
	#if defined _windows && defined NDEBUG
		int     argc;
		LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	#endif

	// Open media file if requested via command line arguments
	bool openFile = ((argc > 1) && !VM_Window::Quit);

	// Try to create window based on selected GUI XML
	const char* mainXML = "main.xml";

	if (VM_Window::Open(mainXML, (APP_NAME + " " + APP_VERSION).c_str()) != RESULT_OK) {
		VM_Window::Close();
		std::exit(ERROR_UNKNOWN);
	}

	// Handle special mobile events (Android/iOS)
	SDL_SetEventFilter(VM_EventManager::HandleEventsMobile, NULL);

	// Main loop: Handles events and UI rendering
	while (!VM_Window::Quit)
	{
		// Manage events: Check for user I/O and other window events
		VM_EventManager::HandleEvents();

		#if defined _android
			VM_EventManager::HandleHeadSetUnpluggedAndroid(VM_Window::JNI->getEnvironment());

			if (VM_Window::StartWakeLock)
				VM_EventManager::WakeLockStart();
		#endif

		// Manage list table data: Update data in background thread if requested by main UI thread
		VM_ThreadManager::HandleData();

		// Manage media player in main UI thread (open/play media can be requested from other threads)
		VM_EventManager::HandleMediaPlayer();

		// Manage background activity when window is not active/visible
		if (VM_Window::PauseRendering)
		{
			// Update list table data in the background (needed when playing music in the background)
			if (VM_GUI::ListTable != NULL)
				VM_GUI::ListTable->refresh();

			// Release allocated resources for the graphics renderer
			FREE_RENDERER(VM_Window::Renderer);

			// Sleep to save system resource usage
			SDL_Delay(DELAY_TIME_BACKGROUND);

			continue;
		}

		#if defined _android
		if (VM_Window::StopWakeLock)
			VM_EventManager::WakeLockStop();
		#endif

		// Manage threads: Check if registered threads need to be recreated or freed etc.
		VM_ThreadManager::HandleThreads();

		// Manage images: Create and rescale images in background thread if requested by main UI thread
		VM_ThreadManager::HandleImages();

		// Reset the graphics renderer and reload the GUI XML if requested by main UI thread (window resized etc.)
		if (VM_Window::ResetRenderer && !VM_Window::PauseRendering)
			VM_Window::Reset(mainXML, (APP_NAME + " " + APP_VERSION).c_str());

		// Enter fullscreen mode in main UI thread (can be requested from other threads)
		if (VM_Player::State.fullscreenEnter)
			VM_Player::FullScreenEnter();

		// Exit fullscreen mode in main UI thread (can be requested from other threads)
		if (VM_Player::State.fullscreenExit)
			VM_Player::FullScreenExit();

		// Save window dimension and location to DB
		if (VM_Window::SaveToDB)
			VM_Window::Save();

		// Open media file if requested via command line arguments
		if (openFile)
		{
			VM_FileSystem::OpenFile(VM_FileSystem::GetPathFromArgumentList(argv, argc));

			// Release memory allocated for command line arguments in Windows Release mode
			#if defined _windows && defined NDEBUG
				LocalFree(argv);
			#endif

			openFile = false;
		}

		// Try to open the requested URL in a web browser in the main UI thread (can be requested from other threads)
		if (!VM_Window::OpenURL.empty() && (VM_FileSystem::OpenWebBrowserT(VM_Window::OpenURL) != RESULT_OK))
		{
			snprintf(
				VM_Window::StatusString, DEFAULT_CHAR_BUFFER_SIZE, "%s '%s'",
				VM_Window::Labels["error.open"].c_str(), VM_Window::OpenURL.c_str()
			);

			VM_Window::OpenURL = "";
		}

		// Update the status text if it has changed
		VM_Button* statusText = dynamic_cast<VM_Button*>(VM_GUI::Components["status_text"]);

		if ((statusText != NULL) && (strcmp(VM_Window::StatusString, statusText->getText().c_str()) != 0))
			statusText->setText(VM_Window::StatusString);

		// Render the current UI if the window is active/visible
		VM_Window::Render();

		// Sleep to save system resource usage
		if ((VIDEO_IS_SELECTED || YOUTUBE_IS_SELECTED) && VM_Player::State.isPlaying)
			SDL_Delay(DELAY_TIME_DEFAULT);
		else
			SDL_Delay(DELAY_TIME_GUI_RENDER);
	}

	// Close the window and release all allocated resources
	VM_Window::Close();
	
	return RESULT_OK;
}
