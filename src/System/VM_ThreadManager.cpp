#include "VM_ThreadManager.h"

using namespace VoyaMedia::Graphics;
using namespace VoyaMedia::MediaPlayer;
using namespace VoyaMedia::UPNP;

std::mutex           System::VM_ThreadManager::DBMutex;
VM_ImageButtonMap    System::VM_ThreadManager::ImageButtons;
VM_ImageMap          System::VM_ThreadManager::Images;
SDL_Surface*         System::VM_ThreadManager::ImagePlaceholder = NULL;
std::mutex           System::VM_ThreadManager::Mutex;
System::VM_ThreadMap System::VM_ThreadManager::Threads;

void System::VM_ThreadManager::AddThreads()
{
	VM_ThreadManager::Threads.insert({ THREAD_CLEAN_DB,      new VM_Thread(VM_FileSystem::CleanDB) });
	VM_ThreadManager::Threads.insert({ THREAD_CLEAN_THUMBS,  new VM_Thread(VM_FileSystem::CleanThumbs) });
	VM_ThreadManager::Threads.insert({ THREAD_CREATE_IMAGES, new VM_Thread(VM_Graphics::OpenImagesThread) });
	VM_ThreadManager::Threads.insert({ THREAD_GET_DATA,      new VM_Thread(VM_ThreadManager::GetData) });
	VM_ThreadManager::Threads.insert({ THREAD_SCAN_DROPBOX,  new VM_Thread(VM_FileSystem::ScanDropboxFiles) });
	VM_ThreadManager::Threads.insert({ THREAD_SCAN_FILES,    new VM_Thread(VM_FileSystem::ScanMediaFiles) });
	VM_ThreadManager::Threads.insert({ THREAD_UPNP_CLIENT,   new VM_Thread(VM_UPNP::ScanDevices) });
	VM_ThreadManager::Threads.insert({ THREAD_UPNP_SERVER,   new VM_Thread(VM_UPNP::Start) });

	#if defined _android
		VM_ThreadManager::Threads.insert({ THREAD_SCAN_ANDROID, new VM_Thread(VM_FileSystem::ScanAndroid) });
	#elif defined _ios
		VM_ThreadManager::Threads.insert({ THREAD_SCAN_ITUNES, new VM_Thread(VM_FileSystem::ScanITunesLibrary) });
	#endif

	VM_ThreadManager::ImagePlaceholder = SDL_CreateRGBSurface(
		0, 1, 1, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, FI_RGBA_ALPHA_MASK
	);

	SDL_FillRect(VM_ThreadManager::ImagePlaceholder, NULL, 0);
}

int System::VM_ThreadManager::CreateThread(const char* threadName)
{
	if (threadName == NULL)
		return ERROR_INVALID_ARGUMENTS;

	VM_Thread* thread = VM_ThreadManager::Threads[threadName];

	if (thread == NULL)
		return ERROR_UNKNOWN;

	thread->thread = SDL_CreateThread(thread->function, threadName, NULL);

	return RESULT_OK;
}

void System::VM_ThreadManager::FreeResources(bool freeImages)
{
	VM_ThreadManager::Mutex.lock();

	if (freeImages)
	{
		for (auto &image : VM_ThreadManager::Images)
			DELETE_POINTER(image.second);

		VM_ThreadManager::Images.clear();
	}

	VM_ThreadManager::ImageButtons.clear();

	VM_ThreadManager::Mutex.unlock();
}

void System::VM_ThreadManager::FreeThreads()
{
	VM_ThreadManager::Mutex.lock();

	for (const auto &thread : VM_ThreadManager::Threads)
		FREE_THREAD(thread.second->thread);

	VM_ThreadManager::Threads.clear();

	VM_ThreadManager::Mutex.unlock();

	VM_ThreadManager::FreeResources();
}

void System::VM_ThreadManager::FreeThumbnails()
{
	for (auto &imageButton : VM_ThreadManager::ImageButtons) {
		if ((imageButton.first.size() > 17) && (imageButton.first.substr(0, 17) == "list_table_thumb_"))
			imageButton.second.clear();
	}
}

int System::VM_ThreadManager::GetData(void* userData)
{
	VM_ThreadManager::Threads[THREAD_GET_DATA]->completed = false;

	while (!VM_Window::Quit)
	{
		VM_ThreadManager::Mutex.lock();

		if ((VM_Modal::ListTable != NULL) && VM_Modal::ListTable->getState().dataRequested)
			VM_Modal::ListTable->setData();
		else if ((VM_GUI::ListTable != NULL) && VM_GUI::ListTable->getState().dataRequested)
			VM_GUI::ListTable->setData();

		VM_ThreadManager::Mutex.unlock();

		if (!VM_Window::Quit)
			SDL_Delay(DELAY_TIME_BACKGROUND);
	}

	if (VM_ThreadManager::Threads[THREAD_GET_DATA] != NULL) {
		VM_ThreadManager::Threads[THREAD_GET_DATA]->start     = false;
		VM_ThreadManager::Threads[THREAD_GET_DATA]->completed = true;
	}

	return RESULT_OK;
}

void System::VM_ThreadManager::HandleData()
{
	VM_ThreadManager::Mutex.lock();

	if ((VM_Modal::ListTable != NULL) && VM_Modal::ListTable->getState().dataIsReady)
	{
		VM_Modal::ListTable->setRows(false);

		if (VM_GUI::ListTable != NULL)
			VM_GUI::ListTable->refresh();
	}
	else if ((VM_GUI::ListTable != NULL) && VM_GUI::ListTable->getState().dataIsReady)
	{
		VM_GUI::ListTable->setRows(false);

		VM_GUI::ListTable->refresh();

		if (VM_Modal::IsVisible() && (VM_Modal::File == "modal_details"))
			VM_Modal::UpdateLabelsDetails();

		VM_PlayerControls::Refresh();
	}

	VM_ThreadManager::Mutex.unlock();
}

void System::VM_ThreadManager::HandleImages()
{
	VM_ThreadManager::Mutex.lock();

	for (const auto &imageButton : VM_ThreadManager::ImageButtons)
	{
		if (imageButton.second.empty())
			continue;

		VM_Image* image = VM_ThreadManager::Images[imageButton.second];

		if ((image == NULL) || !image->ready)
			continue;

		VM_Button* button = NULL;

		if ((imageButton.first == "list_table_play_icon") && (VM_GUI::ListTable != NULL))
			button = VM_GUI::ListTable->playIcon;
		else if (imageButton.first == "bottom_player_snapshot")
			button = dynamic_cast<VM_Button*>(VM_GUI::Components[imageButton.first]);
		else if (VM_Modal::IsVisible())
			button = dynamic_cast<VM_Button*>(VM_Modal::Components[imageButton.first]);
		else
			button = dynamic_cast<VM_Button*>(VM_GUI::Components[imageButton.first]);
		
		if ((button == NULL) && (VM_GUI::ListTable != NULL))
			button = VM_GUI::ListTable->getButton(imageButton.first);

		if ((button != NULL) && ((button->imageData == NULL) || (button->imageData->image == NULL)))
		{
			button->setImage(image);

			if (VM_Modal::IsVisible() && (VM_Modal::File == "modal_details") && (button->id == "bottom_player_snapshot"))
				VM_Modal::UpdateLabelsDetailsPicture();
		}
	}

	VM_ThreadManager::Mutex.unlock();
}

int System::VM_ThreadManager::HandleThreads()
{
	for (const auto &thread : VM_ThreadManager::Threads)
	{
		if (thread.first == THREAD_SCAN_FILES)
		{
			if ((thread.second->thread == NULL) && !thread.second->start && !VM_FileSystem::MediaFiles.empty())
				thread.second->thread = SDL_CreateThread(thread.second->function, thread.first.c_str(), thread.second->data);
		}
		else if (thread.second->start)
		{
			if ((thread.first == THREAD_UPNP_CLIENT) && VM_UPNP::ClientIP.empty())
				continue;
			else if ((thread.first == THREAD_UPNP_SERVER) && VM_UPNP::ServerIP.empty())
				continue;

			if ((thread.second->thread == NULL) && thread.second->completed)
				thread.second->thread = SDL_CreateThread(thread.second->function, thread.first.c_str(), thread.second->data);
		}

		if (!thread.second->start)
			FREE_THREAD(thread.second->thread);
	}

	return RESULT_OK;
}

bool System::VM_ThreadManager::ThreadsAreRunning()
{
	for (const auto &thread : VM_ThreadManager::Threads) {
		if (thread.second->start)
			return true;
	}

	return false;
}

void System::VM_ThreadManager::WaitIfBusy()
{
	while ((VM_ThreadManager::ThreadsAreRunning() || !VM_Player::State.isStopped || !VM_Window::IsDoneRendering) && !VM_Window::Quit)
		SDL_Delay(DELAY_TIME_DEFAULT);
}
