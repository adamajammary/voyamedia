#include "VM_Player.h"

using namespace VoyaMedia::Database;
using namespace VoyaMedia::Graphics;
using namespace VoyaMedia::System;

MediaPlayer::VM_PlayerAudioContext MediaPlayer::VM_Player::audioContext;
uint32_t                           MediaPlayer::VM_Player::CursorLastVisible;
double                             MediaPlayer::VM_Player::DurationTime;
LIB_FFMPEG::AVFormatContext*       MediaPlayer::VM_Player::FormatContext;
LIB_FFMPEG::AVFormatContext*       MediaPlayer::VM_Player::formatContextExternal;
bool                               MediaPlayer::VM_Player::fullscreenEnterWindowed;
bool                               MediaPlayer::VM_Player::fullscreenExitStop;
bool                               MediaPlayer::VM_Player::isCursorHidden;
bool                               MediaPlayer::VM_Player::isStopping;
bool                               MediaPlayer::VM_Player::mediaCompleted;
int                                MediaPlayer::VM_Player::nrOfStreams;
SDL_Thread*                        MediaPlayer::VM_Player::packetsThread;
time_t                             MediaPlayer::VM_Player::PauseTime;
uint32_t                           MediaPlayer::VM_Player::pictureProgressTime;
time_t                             MediaPlayer::VM_Player::PlayTime;
double                             MediaPlayer::VM_Player::ProgressTime;
double                             MediaPlayer::VM_Player::progressTimeLast;
bool                               MediaPlayer::VM_Player::refreshSub;
LIB_FFMPEG::SwrContext*            MediaPlayer::VM_Player::resampleContext;
LIB_FFMPEG::SwsContext*            MediaPlayer::VM_Player::scaleContextSub;
LIB_FFMPEG::SwsContext*            MediaPlayer::VM_Player::scaleContextVideo;
bool                               MediaPlayer::VM_Player::seekRequested;
bool                               MediaPlayer::VM_Player::pausedVideoSeekRequested;
int64_t                            MediaPlayer::VM_Player::seekToPosition;
Graphics::VM_SelectedRow           MediaPlayer::VM_Player::SelectedRow;
MediaPlayer::VM_PlayerState        MediaPlayer::VM_Player::State;
MediaPlayer::VM_PlayerSubContext   MediaPlayer::VM_Player::subContext;
Strings                            MediaPlayer::VM_Player::SubsExternal;
VM_TimeOut*                        MediaPlayer::VM_Player::TimeOut;
MediaPlayer::VM_PlayerVideoContext MediaPlayer::VM_Player::videoContext;
SDL_Rect                           MediaPlayer::VM_Player::VideoDimensions;
bool                               MediaPlayer::VM_Player::videoFrameAvailable;

void MediaPlayer::VM_Player::Init()
{
	VM_Player::reset();

	VM_Player::audioContext.volumeBeforeMute = VM_Player::State.audioVolume;
	VM_Player::mediaCompleted                = false;
	VM_Player::State.quit                    = false;

	int  dbResult;
	auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

	if (DB_RESULT_OK(dbResult))
	{
		String isMuted = db->getSettings("is_muted");

		if (!isMuted.empty())
		{
			VM_Player::State.isMuted         = (isMuted == "1");
			VM_Player::State.audioVolume     = (VM_Player::State.isMuted ? 0 : std::atoi(db->getSettings("audio_volume").c_str()));
			VM_Player::State.loopType        = (VM_LoopType)std::atoi(db->getSettings("playlist_loop_type").c_str());
			VM_Player::State.keepAspectRatio = (db->getSettings("keep_aspect_ratio") == "1");
		}
		else
		{
			VM_Player::State.isMuted         = false;
			VM_Player::State.audioVolume     = SDL_MIX_MAXVOLUME;
			VM_Player::State.loopType        = DEFAULT_LOOP_TYPE;
			VM_Player::State.keepAspectRatio = true;

			db->updateSettings("is_muted",           (VM_Player::State.isMuted ? "1" : "0"));
			db->updateSettings("audio_volume",       std::to_string(VM_Player::State.audioVolume));
			db->updateSettings("playlist_loop_type", std::to_string(VM_Player::State.loopType));
			db->updateSettings("keep_aspect_ratio",  (VM_Player::State.keepAspectRatio ? "1" : "0"));
		}
	}

	DELETE_POINTER(db);
}

int MediaPlayer::VM_Player::Close()
{
	if (VM_Player::isStopping)
		return ERROR_UNKNOWN;

	VM_Player::Pause();
	VM_Player::Stop();

	VM_Player::State.isStopped = false;
	VM_Player::isStopping      = true;
	
	SDL_Delay(DELAY_TIME_BACKGROUND);

	VM_Player::closeThreads();
	VM_Player::closePackets();
	VM_Player::closeAudio();
	VM_Player::closeSub();
	VM_Player::closeVideo();
	VM_Player::closeStream(MEDIA_TYPE_AUDIO);
	VM_Player::closeStream(MEDIA_TYPE_SUBTITLE);
	VM_Player::closeStream(MEDIA_TYPE_VIDEO);
	VM_Player::closeFormatContext();

	DELETE_POINTER(VM_Player::TimeOut);

	// ITUNES (iOS) - DELETE FILE FROM DOCS FOLDER
	#if defined _ios
	if (VM_FileSystem::IsITunes(VM_GUI::ListTable->getSelectedMediaURL()))
		VM_FileSystem::DeleteFileITunes(VM_Player::State.filePath);
	#endif

	String filePath = VM_Player::State.filePath;

	VM_Player::CursorShow();
	VM_Player::reset();

	if (VM_GUI::ListTable != nullptr)
	{
		VM_GUI::ListTable->visible = true;

		// PLAY NEXT MEDIA FILE IN THE PLAYLIST.
		if (VM_Player::mediaCompleted)
		{
			VM_Player::State.filePath = filePath;
			VM_Player::mediaCompleted = false;

			VM_Player::OpenNext(false);
		}
	}

	VM_Player::isStopping      = false;
	VM_Player::State.isStopped = true;
	VM_Player::State.quit      = false;
	VM_Window::ResetRenderer   = true;

	return RESULT_OK;
}

int MediaPlayer::VM_Player::closeAudio()
{
	if ((VM_Player::audioContext.index < 0) || (VM_Player::audioContext.stream == NULL))
		return ERROR_UNKNOWN;

	LIB_FFMPEG::av_frame_free(&VM_Player::audioContext.frame);
	FREE_POINTER(VM_Player::audioContext.frameEncoded);
	FREE_SWR(VM_Player::resampleContext);
	FREE_THREAD_COND(VM_Player::audioContext.condition);
	FREE_THREAD_MUTEX(VM_Player::audioContext.mutex);

	return RESULT_OK;
}

void MediaPlayer::VM_Player::closeFormatContext()
{
	if (VM_Player::formatContextExternal != nullptr)
		VM_FileSystem::CloseMediaFormatContext(VM_Player::formatContextExternal, -1);

	if ((VM_Player::FormatContext != nullptr) && (VM_GUI::ListTable != nullptr))
		VM_FileSystem::CloseMediaFormatContext(VM_Player::FormatContext, VM_GUI::ListTable->getSelectedMediaID());
}

void MediaPlayer::VM_Player::closePackets()
{
	VM_Player::packetsClear(VM_Player::audioContext.packets, VM_Player::audioContext.mutex, VM_Player::audioContext.condition, VM_Player::audioContext.packetsAvailable);
	VM_Player::packetsClear(VM_Player::subContext.packets,   VM_Player::subContext.mutex,   VM_Player::subContext.condition,   VM_Player::subContext.packetsAvailable);
	VM_Player::packetsClear(VM_Player::videoContext.packets, VM_Player::videoContext.mutex, VM_Player::videoContext.condition, VM_Player::videoContext.packetsAvailable);
}

int MediaPlayer::VM_Player::closeStream(VM_MediaType streamType)
{
	switch (streamType) {
		case MEDIA_TYPE_AUDIO:
			FREE_STREAM(VM_Player::audioContext.stream);
			VM_Player::audioContext.index = -1;
			break;
		case MEDIA_TYPE_SUBTITLE:
			FREE_STREAM(VM_Player::subContext.stream);
			VM_Player::subContext.index   = -1;
			VM_Player::SubsExternal.clear();
			break;
		case MEDIA_TYPE_VIDEO:
			FREE_STREAM(VM_Player::videoContext.stream);
			VM_Player::videoContext.index = -1;
			break;
	}

	return RESULT_OK;
}

int MediaPlayer::VM_Player::closeSub()
{
	if ((VM_Player::subContext.index < 0) || (VM_Player::subContext.stream == NULL))
		return ERROR_UNKNOWN;

	SDL_LockMutex(VM_Player::subContext.subsMutex);
	VM_Player::subContext.available = false;

	for (auto sub : VM_Player::subContext.subs) {
		VM_SubFontEngine::RemoveSubs(sub->id);
		DELETE_POINTER(sub);
	}
	VM_Player::subContext.subs.clear();

	VM_Player::subContext.available = true;
	SDL_CondSignal(VM_Player::subContext.subsCondition);
	SDL_UnlockMutex(VM_Player::subContext.subsMutex);

	for (uint32_t i = 0; i < NR_OF_FONTS; i++)
		CLOSE_FONT(VM_Player::subContext.fonts[i]);

	for (auto &font : VM_Player::subContext.styleFonts)
		CLOSE_FONT(font.second);

	VM_Player::subContext.styleFonts.clear();
	VM_Player::subContext.styles.clear();

	DELETE_POINTER(VM_Player::subContext.texture);
	FREE_SWS(VM_Player::scaleContextSub);
	FREE_THREAD_COND(VM_Player::subContext.condition);
	FREE_THREAD_MUTEX(VM_Player::subContext.mutex);

	return RESULT_OK;
}

void MediaPlayer::VM_Player::closeThreads()
{
	SDL_CloseAudioDevice(VM_Player::audioContext.deviceID);

	FREE_THREAD(VM_Player::packetsThread);
	FREE_THREAD(VM_Player::subContext.thread);
	FREE_THREAD(VM_Player::videoContext.thread);
}

int MediaPlayer::VM_Player::closeVideo()
{
	if ((VM_Player::videoContext.index < 0) || (VM_Player::videoContext.stream == NULL))
		return ERROR_UNKNOWN;

	LIB_FFMPEG::av_frame_free(&VM_Player::videoContext.frame);
	LIB_FFMPEG::av_frame_free(&VM_Player::videoContext.frameEncoded);
	DELETE_POINTER(VM_Player::videoContext.texture);
	FREE_SWS(VM_Player::scaleContextVideo);
	FREE_THREAD_COND(VM_Player::videoContext.condition);
	FREE_THREAD_MUTEX(VM_Player::videoContext.mutex);

	return RESULT_OK;
}

int MediaPlayer::VM_Player::cursorHide()
{
	if (VM_Player::isCursorHidden || ((SDL_GetTicks() - VM_Player::CursorLastVisible) < CURSOR_HIDE_DELAY))
		return ERROR_INVALID_ARGUMENTS;

	#if defined _linux || defined _macosx || defined _windows
		SDL_Rect mousePosition = {};
		SDL_GetMouseState(&mousePosition.x, &mousePosition.y);

		if (!VM_Graphics::ButtonHovered(&mousePosition, VM_GUI::Components["bottom_player_snapshot"]->backgroundArea))
			return RESULT_OK;

		SDL_ShowCursor(0);
	#endif

	VM_Player::isCursorHidden = true;

	#if defined _linux || defined _macosx || defined _windows
	if (VM_Window::FullScreenMaximized)
	#endif
		VM_PlayerControls::Hide();

	return RESULT_OK;
}

int MediaPlayer::VM_Player::CursorShow()
{
	VM_Player::CursorLastVisible = SDL_GetTicks();

	if (VM_Window::Inactive)
		VM_Window::Refresh();

	if (!VM_Player::isCursorHidden)
		return RESULT_OK;

	#if defined _linux || defined _macosx || defined _windows
		SDL_ShowCursor(1);
	#endif
	
	VM_Player::isCursorHidden = false;

	if (!VM_Player::State.isStopped && !VM_PlayerControls::IsVisible())
		VM_PlayerControls::Show();

	return RESULT_OK;
}

int MediaPlayer::VM_Player::FreeTextures()
{
	VM_Button* snapshot = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_snapshot"]);

	if ((snapshot != NULL) && (snapshot->imageData != NULL))
		snapshot->removeImage();

	DELETE_POINTER(VM_Player::subContext.texture);
	DELETE_POINTER(VM_Player::videoContext.texture);

	return RESULT_OK;
}

int MediaPlayer::VM_Player::FullScreenEnter(bool windowMode)
{
	VM_Player::State.fullscreenEnter    = true;
	VM_Player::fullscreenEnterWindowed  = windowMode;

	return RESULT_OK;
}

int MediaPlayer::VM_Player::FullScreenEnter()
{
	if (!VM_Player::fullscreenEnterWindowed)
	{
		VM_Window::FullScreenMaximized = true;

		#if defined _linux || defined _macosx || defined _windows
			int mouseX, mouseY;

			SDL_GetGlobalMouseState(&mouseX, &mouseY);
			SDL_SetWindowFullscreen(VM_Window::MainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
			SDL_WarpMouseGlobal(mouseX, mouseY);
		#endif
	}

	VM_Window::FullScreenEnabled       = true;
	VM_Player::State.fullscreenEnter   = false;
	VM_Player::fullscreenEnterWindowed = false;

	VM_PlayerControls::Refresh();
	VM_Window::Refresh();

	return RESULT_OK;
}
int MediaPlayer::VM_Player::FullScreenExit(bool stop)
{
	VM_Player::State.fullscreenExit = true;
	VM_Player::fullscreenExitStop   = stop;

	return RESULT_OK;
}

int MediaPlayer::VM_Player::FullScreenExit()
{
	if (VM_Player::fullscreenExitStop)
		VM_Player::Stop();

	VM_Window::FullScreenEnabled    = false;
	VM_Player::State.fullscreenExit = false;
	VM_Player::fullscreenExitStop   = false;

	if (VM_Window::FullScreenMaximized)
	{
		#if defined _linux || defined _macosx || defined _windows
			int mouseX, mouseY;

			SDL_GetGlobalMouseState(&mouseX, &mouseY);
			SDL_SetWindowFullscreen(VM_Window::MainWindow, 0);

			bool windowIsMaximized = ((SDL_GetWindowFlags(VM_Window::MainWindow) & SDL_WINDOW_MAXIMIZED) != 0);

			if (windowIsMaximized) {
				SDL_RestoreWindow(VM_Window::MainWindow);
				SDL_SetWindowPosition(VM_Window::MainWindow, VM_Window::DimensionsBeforeFS.x, VM_Window::DimensionsBeforeFS.y);
				SDL_SetWindowSize(VM_Window::MainWindow,     VM_Window::DimensionsBeforeFS.w, VM_Window::DimensionsBeforeFS.h);
				SDL_MaximizeWindow(VM_Window::MainWindow);
			}

			SDL_WarpMouseGlobal(mouseX, mouseY);
		#endif

		VM_Window::FullScreenMaximized = false;
	}

	VM_PlayerControls::Refresh();
	VM_Window::Refresh();

	return RESULT_OK;
}

int MediaPlayer::VM_Player::FullScreenToggle(bool windowMode)
{
	if (VM_Window::FullScreenEnabled && VM_Window::FullScreenMaximized)
		VM_Player::FullScreenExit(false);
	else
		VM_Player::FullScreenEnter(windowMode);

	return RESULT_OK;
}

LIB_FFMPEG::AVSampleFormat MediaPlayer::VM_Player::GetAudioSampleFormat(SDL_AudioFormat sampleFormat)
{
	switch (sampleFormat) {
		case AUDIO_U8:     return LIB_FFMPEG::AV_SAMPLE_FMT_U8;
		case AUDIO_S16SYS: return LIB_FFMPEG::AV_SAMPLE_FMT_S16;
		case AUDIO_S32SYS: return LIB_FFMPEG::AV_SAMPLE_FMT_S32;
		case AUDIO_F32SYS: return LIB_FFMPEG::AV_SAMPLE_FMT_FLT;
	}

	return LIB_FFMPEG::AV_SAMPLE_FMT_NONE;
}

SDL_AudioFormat MediaPlayer::VM_Player::GetAudioSampleFormat(LIB_FFMPEG::AVSampleFormat sampleFormat)
{
	switch (sampleFormat) {
		case LIB_FFMPEG::AV_SAMPLE_FMT_U8:
		case LIB_FFMPEG::AV_SAMPLE_FMT_U8P:  return AUDIO_U8;
		case LIB_FFMPEG::AV_SAMPLE_FMT_S16:
		case LIB_FFMPEG::AV_SAMPLE_FMT_S16P: return AUDIO_S16SYS;
		case LIB_FFMPEG::AV_SAMPLE_FMT_S32:
		case LIB_FFMPEG::AV_SAMPLE_FMT_S32P: return AUDIO_S32SYS;
		case LIB_FFMPEG::AV_SAMPLE_FMT_FLT:
		case LIB_FFMPEG::AV_SAMPLE_FMT_FLTP: return AUDIO_F32SYS;
	}

	return LIB_FFMPEG::AV_SAMPLE_FMT_NONE;
}

uint32_t MediaPlayer::VM_Player::GetVideoPixelFormat(LIB_FFMPEG::AVPixelFormat pixelFormat)
{
	// https://wiki.videolan.org/YUV/
	switch (pixelFormat) {
		case LIB_FFMPEG::AV_PIX_FMT_YUV420P:
		case LIB_FFMPEG::AV_PIX_FMT_YUVJ420P: return SDL_PIXELFORMAT_YV12;
		case LIB_FFMPEG::AV_PIX_FMT_YUYV422:  return SDL_PIXELFORMAT_YUY2;
		case LIB_FFMPEG::AV_PIX_FMT_UYVY422:  return SDL_PIXELFORMAT_UYVY;
		case LIB_FFMPEG::AV_PIX_FMT_YVYU422:  return SDL_PIXELFORMAT_YVYU;
		case LIB_FFMPEG::AV_PIX_FMT_NV12:     return SDL_PIXELFORMAT_NV12;
		case LIB_FFMPEG::AV_PIX_FMT_NV21:     return SDL_PIXELFORMAT_NV21;
	}

	return SDL_PIXELFORMAT_YV12;
}

LIB_FFMPEG::AVPixelFormat MediaPlayer::VM_Player::GetVideoPixelFormat(uint32_t pixelFormat)
{
	switch (pixelFormat) {
		case SDL_PIXELFORMAT_YV12: return LIB_FFMPEG::AV_PIX_FMT_YUV420P;
		case SDL_PIXELFORMAT_YUY2: return LIB_FFMPEG::AV_PIX_FMT_YUYV422;
		case SDL_PIXELFORMAT_UYVY: return LIB_FFMPEG::AV_PIX_FMT_UYVY422;
		case SDL_PIXELFORMAT_YVYU: return LIB_FFMPEG::AV_PIX_FMT_YVYU422;
		case SDL_PIXELFORMAT_NV12: return LIB_FFMPEG::AV_PIX_FMT_NV12;
		case SDL_PIXELFORMAT_NV21: return LIB_FFMPEG::AV_PIX_FMT_NV21;
	}

	return LIB_FFMPEG::AV_PIX_FMT_YUV420P;
}

int MediaPlayer::VM_Player::GetStreamIndex(VM_MediaType mediaType)
{
	switch (mediaType) {
		case MEDIA_TYPE_AUDIO:    return VM_Player::audioContext.index;
		case MEDIA_TYPE_SUBTITLE: return VM_Player::subContext.index;
	}

	return -1;
}

SDL_FPoint MediaPlayer::VM_Player::GetSubScale()
{
	return VM_Player::subContext.scale;
}

bool MediaPlayer::VM_Player::isPacketQueueFull(VM_MediaType streamType)
{
	switch (streamType) {
	case MEDIA_TYPE_AUDIO:
		return ((VM_Player::audioContext.stream != NULL) && (VM_Player::audioContext.packets.size() >= MIN_PACKET_QUEUE_SIZE));
		break;
	case MEDIA_TYPE_SUBTITLE:
		return ((VM_Player::subContext.stream != NULL) && (VM_Player::subContext.packets.size() >= MIN_PACKET_QUEUE_SIZE));
		break;
	case MEDIA_TYPE_VIDEO:
		return ((VM_Player::videoContext.stream != NULL) && (VM_Player::videoContext.packets.size() >= MIN_PACKET_QUEUE_SIZE));
		break;
	}

	return false;
}

bool MediaPlayer::VM_Player::IsYUV(LIB_FFMPEG::AVPixelFormat pixelFormat)
{
	switch (pixelFormat) {
		case LIB_FFMPEG::AV_PIX_FMT_YUV420P:
		case LIB_FFMPEG::AV_PIX_FMT_YUVJ420P:
		case LIB_FFMPEG::AV_PIX_FMT_YUYV422:
		case LIB_FFMPEG::AV_PIX_FMT_UYVY422:
		case LIB_FFMPEG::AV_PIX_FMT_YVYU422:
		case LIB_FFMPEG::AV_PIX_FMT_NV12:
		case LIB_FFMPEG::AV_PIX_FMT_NV21:
			return true;
	}

	return false;
}

void MediaPlayer::VM_Player::KeepAspectRatioToggle()
{
	//if (VIDEO_IS_SELECTED || YOUTUBE_IS_SELECTED)
	if (VIDEO_IS_SELECTED)
	{
		VM_Player::State.keepAspectRatio = !VM_Player::State.keepAspectRatio;

		int  dbResult;
		auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

		if (DB_RESULT_OK(dbResult))
			db->updateSettings("keep_aspect_ratio", (VM_Player::State.keepAspectRatio ? "1" : "0"));

		DELETE_POINTER(db);

		VM_Player::FreeTextures();
	}
}

int MediaPlayer::VM_Player::MuteToggle()
{
	if (!VM_Player::State.isMuted)
		VM_Player::State.audioVolume = 0;

	VM_Player::State.isMuted = !VM_Player::State.isMuted;

	int  dbResult;
	auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

	if (DB_RESULT_OK(dbResult))
	{
		db->updateSettings("is_muted", (VM_Player::State.isMuted ? "1" : "0"));

		if (!VM_Player::State.isMuted)
			VM_Player::State.audioVolume = std::atoi(db->getSettings("audio_volume").c_str());
	}

	DELETE_POINTER(db);

	return RESULT_OK;
}

int MediaPlayer::VM_Player::OpenFilePath(const String &filePath)
{
	VM_Player::State.openFile = true;
	VM_Player::State.filePath = filePath;

	VM_Player::FullScreenEnter(true);

	return RESULT_OK;
}

#if defined _windows
int MediaPlayer::VM_Player::OpenFilePath(const WString &filePath)
{
	VM_Player::State.openFile  = true;
	VM_Player::State.filePathW = filePath;
	VM_Player::State.filePath  = VM_Text::ToUTF8(filePath.c_str(), false);

	VM_Player::FullScreenEnter(true);

	return RESULT_OK;
}
#endif

int MediaPlayer::VM_Player::Open()
{
	if (VM_Player::State.filePath.empty())
		return ERROR_INVALID_ARGUMENTS;

	if (VM_Player::State.filePath == REFRESH_PENDING) {
		VM_Player::State.openFile = true;
		return RESULT_OK;
	}

	// ITUNES (iOS) - DOWNLOAD TEMP FILE
	#if defined _ios
		if (VM_FileSystem::IsITunes(VM_Player::State.filePath))
			VM_Player::State.filePath = VM_FileSystem::DownloadFileFromITunes(VM_Player::State.filePath);
	#endif

	// PICTURE
	if (PICTURE_IS_SELECTED)
	{
		VM_Player::DurationTime = PICTURE_SLIDESHOW_TIME;

		// URLS: DROPBOX / UPNP
		if (VM_FileSystem::IsHttp(VM_Player::State.filePath))
		{
			if (!VM_FileSystem::FileExists(VM_Player::State.filePath, L""))
			{
				// DROPBOX
				if (VM_Player::State.filePath.find("dropbox") != String::npos)
					VM_Player::State.filePath = VM_FileSystem::GetDropboxURL2(VM_Player::State.filePath);

				if (!VM_FileSystem::FileExists(VM_Player::State.filePath, L""))
					return ERROR_UNKNOWN;
			}
		}
		// FILES
		else
		{
			#if defined _windows
			if (!VM_FileSystem::FileExists("", VM_Text::ToUTF16(VM_Player::State.filePath.c_str())))
			#else
			if (!VM_FileSystem::FileExists(VM_Player::State.filePath, L""))
			#endif
				return ERROR_UNKNOWN;
		}
	}
	// AUDIO/VIDEO
	else
	{
		if (VM_Player::openFormatContext() != RESULT_OK)
			return ERROR_UNKNOWN;

		if (VM_Player::openStreams() != RESULT_OK)
			return ERROR_UNKNOWN;

		// DURATION
		VM_Player::DurationTime = (double)VM_FileSystem::GetMediaDuration(
			VM_Player::FormatContext, VM_Player::audioContext.stream
		);

		if (VM_Player::openThreads() != RESULT_OK)
			return ERROR_UNKNOWN;
	}

	if (VM_GUI::ListTable != NULL)
		VM_GUI::ListTable->visible = false;

	// PLAY
	VM_Player::Play();

	VM_Window::StatusString   = "";
	VM_Player::State.openFile = false;

	return RESULT_OK;
}

int MediaPlayer::VM_Player::openAudio()
{
	if ((VM_Player::audioContext.stream == NULL) || (VM_Player::audioContext.stream->codec == NULL))
		return ERROR_UNKNOWN;

	int      channelCount  = VM_Player::audioContext.stream->codec->channels;
	uint64_t channelLayout = VM_Player::audioContext.stream->codec->channel_layout;

	VM_Player::audioContext.frame     = LIB_FFMPEG::av_frame_alloc();
	VM_Player::audioContext.condition = SDL_CreateCond();
	VM_Player::audioContext.mutex     = SDL_CreateMutex();
	VM_Player::audioContext.index     = VM_Player::audioContext.stream->index;

	if ((VM_Player::audioContext.frame == NULL) || (VM_Player::audioContext.condition == NULL) || (VM_Player::audioContext.mutex == NULL))
		return ERROR_UNKNOWN;

	// INVALID CHANNEL COUNT - USE VALID CHANNEL LAYOUT
	if ((channelCount <= 0) && (channelLayout > 0))
		channelCount = LIB_FFMPEG::av_get_channel_layout_nb_channels(channelLayout);
	// INVALID CHANNEL LAYOUT - USE VALID CHANNEL COUNT
	else if ((channelLayout == 0) && (channelCount > 0))
		channelLayout = LIB_FFMPEG::av_get_default_channel_layout(channelCount);

	if ((VM_Player::audioContext.stream->codec->sample_rate <= 0) || (channelCount <= 0) || (channelLayout == 0))
		return ERROR_UNKNOWN;

	VM_Player::audioContext.stream->codec->channels       = channelCount;
	VM_Player::audioContext.stream->codec->channel_layout = channelLayout;

	VM_Player::audioContext.device = {};
	SDL_AudioSpec wantedSpecs      = {};

	// https://developer.apple.com/documentation/audiotoolbox/kaudiounitproperty_maximumframesperslice
	wantedSpecs.callback = VM_Player::threadAudio;
	wantedSpecs.channels = VM_Player::audioContext.stream->codec->channels;
	wantedSpecs.format   = VM_Player::GetAudioSampleFormat(VM_Player::audioContext.stream->codec->sample_fmt);
	wantedSpecs.freq     = VM_Player::audioContext.stream->codec->sample_rate;
	wantedSpecs.samples  = 4096;

	#if defined _DEBUG
		String msg = "VM_Player::openAudio\n";
		msg.append("\tCurrentAudioDriver: ").append(SDL_GetCurrentAudioDriver());
		for (int i = 0; i < SDL_GetNumAudioDrivers(); i++)
			msg.append("\n\t[" + std::to_string(i) + "] Driver: ").append(SDL_GetAudioDriver(i));
		for (int i = 0; i < SDL_GetNumAudioDevices(0); i++)
			msg.append("\n\t[" + std::to_string(i) + "] Device: ").append(SDL_GetAudioDeviceName(i, 0));
		LOG((msg + "\n").c_str());
	#endif

	// Try opening the default audio device
	VM_Player::audioContext.deviceID = VM_Player::openAudioDevice(wantedSpecs);

	// Try all available audio devices if default fails
	if (VM_Player::audioContext.deviceID < 2)
	{
		for (int i = 0; i < SDL_GetNumAudioDrivers(); i++)
		{
			SDL_AudioQuit();
			SDL_setenv("SDL_AUDIODRIVER", SDL_GetAudioDriver(i), 1);
			SDL_AudioInit(SDL_GetAudioDriver(i));

			#if defined _DEBUG
				LOG("[%d-%s] CurrentAudioDriver: %s", i, SDL_GetAudioDriver(i), SDL_GetCurrentAudioDriver());
			#endif

			wantedSpecs.channels = VM_Player::audioContext.stream->codec->channels;

			do {
				VM_Player::audioContext.deviceID = VM_Player::openAudioDevice(wantedSpecs);

				if (VM_Player::audioContext.deviceID < 2)
					wantedSpecs.channels--;
			} while ((VM_Player::audioContext.deviceID < 2) && (wantedSpecs.channels > 0));

			if ((VM_Player::audioContext.deviceID >= 2) && (wantedSpecs.channels > 0))
				break;
		}
	}

	if (VM_Player::audioContext.deviceID < 2)
		return ERROR_UNKNOWN;

	VM_Player::audioContext.bufferOffset    = 0;
	VM_Player::audioContext.bufferRemaining = 0;
	VM_Player::audioContext.bufferSize      = AUDIO_BUFFER_SIZE;
	VM_Player::audioContext.decodeFrame     = true;
	VM_Player::audioContext.frameEncoded    = (uint8_t*)malloc(VM_Player::audioContext.bufferSize);
	VM_Player::audioContext.writtenToStream = 0;

	if (VM_Player::audioContext.frameEncoded == NULL)
		return ERROR_UNKNOWN;

	return RESULT_OK;
}

SDL_AudioDeviceID MediaPlayer::VM_Player::openAudioDevice(SDL_AudioSpec &wantedSpecs)
{
	int  allowFlags = (SDL_AUDIO_ALLOW_FORMAT_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
	auto deviceID   = SDL_OpenAudioDevice(NULL, 0, &wantedSpecs, &VM_Player::audioContext.device, allowFlags);

	if (deviceID < 2)
	{
		#if defined _DEBUG
			LOG("VM_Player::openAudio: SDL_Error: %s\n", SDL_GetError());
		#endif

		SDL_CloseAudioDevice(deviceID);
	}

	return deviceID;
}

int MediaPlayer::VM_Player::openFormatContext()
{
	VM_Player::TimeOut = new VM_TimeOut();

	//if (YOUTUBE_IS_SELECTED)
	//{
	//	VM_Player::FormatContext = VM_FileSystem::GetMediaFormatContext(VM_Player::State.filePath, true);

	//	if (VM_Player::FormatContext == NULL)
	//	{
	//		for (const auto &mediaURL : VM_Player::State.urls)
	//		{
	//			VM_Player::FormatContext = VM_FileSystem::GetMediaFormatContext(mediaURL, true);

	//			if ((VM_Player::FormatContext != NULL) && 
	//				(VM_FileSystem::GetMediaStreamCount(VM_Player::FormatContext, MEDIA_TYPE_AUDIO) > 0) &&
	//				(VM_FileSystem::GetMediaStreamCount(VM_Player::FormatContext, MEDIA_TYPE_VIDEO) > 0))
	//			{
	//				VM_Player::State.filePath = mediaURL;
	//				break;
	//			}

	//			VM_Player::closeFormatContext();
	//		}
	//	}
	//}
	// DROPBOX
	if (VM_FileSystem::IsHttp(VM_Player::State.filePath) && (VM_Player::State.filePath.find("dropbox") != String::npos))
	{
		if (!VM_FileSystem::FileExists(VM_Player::State.filePath, L""))
			VM_Player::State.filePath = VM_FileSystem::GetDropboxURL2(VM_Player::State.filePath);

		VM_Player::FormatContext = VM_FileSystem::GetMediaFormatContext(VM_Player::State.filePath, true);
	}

	if (VM_Player::FormatContext == NULL)
		VM_Player::FormatContext = VM_FileSystem::GetMediaFormatContext(VM_Player::State.filePath, true);

	if (VM_Player::FormatContext == NULL)
		return ERROR_UNKNOWN;

	return RESULT_OK;
}

int MediaPlayer::VM_Player::OpenNext(bool stop)
{
	return VM_Player::openPlaylist(stop, true);
}

int MediaPlayer::VM_Player::OpenPrevious(bool stop)
{
	return VM_Player::openPlaylist(stop, false);
}

int MediaPlayer::VM_Player::openPlaylist(bool stop, bool next)
{
	if (stop)
		VM_Player::Stop();

	String previousFile = VM_GUI::ListTable->getSelectedFile();

	if (next) {
		switch (VM_Player::State.loopType) {
			case LOOP_TYPE_NORMAL:  VM_GUI::ListTable->selectNext();     break;
			case LOOP_TYPE_LOOP:    VM_GUI::ListTable->selectNext(true); break;
			case LOOP_TYPE_SHUFFLE: VM_GUI::ListTable->selectRandom();   break;
		}
	} else {
		switch (VM_Player::State.loopType) {
			case LOOP_TYPE_NORMAL:  VM_GUI::ListTable->selectPrev();     break;
			case LOOP_TYPE_LOOP:    VM_GUI::ListTable->selectPrev(true); break;
			case LOOP_TYPE_SHUFFLE: VM_GUI::ListTable->selectRandom();   break;
		}
	}

	String nextFile = VM_GUI::ListTable->getSelectedFile();

	if (nextFile.empty() ||
		((VM_Player::State.loopType != LOOP_TYPE_SHUFFLE) && (nextFile != REFRESH_PENDING) &&
		((nextFile == VM_Player::State.filePath) || (nextFile == previousFile))))
	{
		return ERROR_UNKNOWN;
	}

	if (VM_Player::OpenFilePath(nextFile) != RESULT_OK)
		return ERROR_UNKNOWN;

	return RESULT_OK;
}

int MediaPlayer::VM_Player::openStreams()
{
	VM_Player::audioContext.index = -1;
	VM_Player::subContext.index   = -1;
	VM_Player::videoContext.index = -1;

	// AUDIO STREAM
	VM_Player::audioContext.stream = VM_FileSystem::GetMediaStreamBest(
		VM_Player::FormatContext, MEDIA_TYPE_AUDIO
	);

	if (VM_Player::audioContext.stream == NULL)
		return ERROR_UNKNOWN;

	// VIDEO + SUB STREAMS
	//if (VIDEO_IS_SELECTED || YOUTUBE_IS_SELECTED)
	if (VIDEO_IS_SELECTED)
	{
		VM_Player::videoContext.stream = VM_FileSystem::GetMediaStreamBest(
			VM_Player::FormatContext, MEDIA_TYPE_VIDEO
		);

		if (VM_Player::videoContext.stream == NULL)
			return ERROR_UNKNOWN;

		VM_Player::subContext.stream = VM_FileSystem::GetMediaStreamFirst(
			VM_Player::FormatContext, MEDIA_TYPE_SUBTITLE
		);
	}

	return RESULT_OK;
}

int MediaPlayer::VM_Player::openSub()
{
	VM_Player::SubsExternal = VM_FileSystem::GetMediaSubFiles(VM_Player::State.filePath);

	// SUB - EXTERNAL
	if (VM_Player::subContext.stream == NULL)
	{
		VM_Player::subContext.stream = VM_Player::openSubExternal(SUB_STREAM_EXTERNAL);

		if (VM_Player::subContext.stream != NULL)
			VM_Player::subContext.index = (SUB_STREAM_EXTERNAL + VM_Player::subContext.stream->index);
	// SUB - INTERNAL
	} else {
		VM_Player::subContext.index = VM_Player::subContext.stream->index;
	}

	if ((VM_Player::subContext.stream == NULL) || (VM_Player::subContext.stream->codec == NULL))
		return ERROR_UNKNOWN;

	if (VM_Player::subContext.mutex  == NULL) {
		VM_Player::subContext.mutex     = SDL_CreateMutex();
		VM_Player::subContext.condition = SDL_CreateCond();
	}

	if ((VM_Player::subContext.mutex == NULL) || (VM_Player::subContext.condition == NULL))
		return ERROR_UNKNOWN;

	if (VM_Player::subContext.subsMutex  == NULL) {
		VM_Player::subContext.subsMutex     = SDL_CreateMutex();
		VM_Player::subContext.subsCondition = SDL_CreateCond();
	}

	if ((VM_Player::subContext.subsMutex == NULL) || (VM_Player::subContext.subsCondition == NULL))
		return ERROR_UNKNOWN;

	// ASS/SSA STYLES
	if (VM_Player::subContext.stream->codec->subtitle_header != NULL)
	{
		String subHeader = String(reinterpret_cast<char*>(VM_Player::subContext.stream->codec->subtitle_header));

		#if defined _DEBUG
			LOG("VM_Player::openSub:\n%s\n", subHeader.c_str());
		#endif

		// STREAM SIZES
		int subWidth    = (VM_Player::subContext.stream   != NULL ? VM_Player::subContext.stream->codec->width    : 0);
		int subHeight   = (VM_Player::subContext.stream   != NULL ? VM_Player::subContext.stream->codec->height   : 0);
		int videoWidth  = (VM_Player::videoContext.stream != NULL ? VM_Player::videoContext.stream->codec->width  : 0);
		int videoHeight = (VM_Player::videoContext.stream != NULL ? VM_Player::videoContext.stream->codec->height : 0);

		// WIDTH
		size_t findPos = subHeader.find("PlayResX:");

		if (findPos != String::npos)
			subWidth = std::atoi(subHeader.substr(findPos + 9).c_str());

		// HEIGHT
		findPos = subHeader.find("PlayResY:");

		if (findPos != String::npos)
			subHeight = std::atoi(subHeader.substr(findPos + 9).c_str());

		// SUB SCREEN SPACE SIZE
		if ((subWidth > 0) && (subHeight > 0)) {
			VM_Player::subContext.size = { subWidth, subHeight };
		} else if ((subHeight > 0) && (videoWidth > 0) && (videoHeight > 0)) {
			float subVideoScale        = ((float)subHeight / (float)videoHeight);
			VM_Player::subContext.size = { (int)((float)videoWidth * subVideoScale), subHeight };
		} else {
			VM_Player::subContext.size = DEFAULT_SUB_SCREEN_SIZE;
		}

		// STYLE VERSION
		if (!subHeader.empty())
		{
			VM_SubStyleVersion version = SUB_STYLE_VERSION_UNKNOWN;

			if (subHeader.find("[V4+ Styles]") != String::npos)
				version = SUB_STYLE_VERSION_4PLUS_ASS;
			else if (subHeader.find("[V4 Styles]") != String::npos)
				version = SUB_STYLE_VERSION_4_SSA;

			// STYLES
			std::stringstream subHeaderStream(subHeader);
			String            subHeaderLine;

			while (std::getline(subHeaderStream, subHeaderLine))
			{
				if (version == SUB_STYLE_VERSION_UNKNOWN)
					break;

				if (subHeaderLine.substr(0, 7) != "Style: ")
					continue;

				Strings subHeaderParts = VM_Text::Split(subHeaderLine.substr(7), ",");

				if (((version == SUB_STYLE_VERSION_4_SSA)     && (subHeaderParts.size() < NR_OF_V4_SUB_STYLES)) ||
					((version == SUB_STYLE_VERSION_4PLUS_ASS) && (subHeaderParts.size() < NR_OF_V4PLUS_SUB_STYLES)))
				{
					continue;
				}

				VM_SubStyle* subStyle = new VM_SubStyle(subHeaderParts, version);
				VM_Player::subContext.styles.push_back(subStyle);
			}
		}
	}

	#if defined _windows
		WString fontPath    = VM_FileSystem::GetPathFontW();
		WString fontCJK     = WString(fontPath + L"NotoSansCJK-Bold.ttc");
		WString fontDefault = WString(fontPath + L"NotoSans-Merged.ttf");
	#else
		String fontPath    = VM_FileSystem::GetPathFont();
		String fontCJK     = String(fontPath + "NotoSansCJK-Bold.ttc");
		String fontDefault = String(fontPath + "NotoSans-Merged.ttf");
	#endif

	if (VM_Player::subContext.fonts[FONT_MERGED] == NULL) {
		VM_Player::subContext.fonts[FONT_MERGED] = OPEN_FONT(fontDefault.c_str(), DEFAULT_FONT_SIZE_SUB);
		VM_Player::subContext.fonts[FONT_CJK]    = OPEN_FONT(fontCJK.c_str(),     DEFAULT_FONT_SIZE_SUB);
	}

	if (VM_Player::subContext.thread == NULL)
		VM_Player::subContext.thread = SDL_CreateThread(VM_Player::threadSub, "subContext.thread", NULL);

	if (VM_Player::subContext.thread == NULL)
		return ERROR_UNKNOWN;

	return RESULT_OK;
}

LIB_FFMPEG::AVStream* MediaPlayer::VM_Player::openSubExternal(int streamIndex)
{
	int                   fileIndex, fileStreamIndex;
	LIB_FFMPEG::AVStream* stream;

	fileIndex       = ((streamIndex - SUB_STREAM_EXTERNAL) / SUB_STREAM_EXTERNAL);
	fileStreamIndex = ((streamIndex - SUB_STREAM_EXTERNAL) % SUB_STREAM_EXTERNAL);

	if (VM_Player::SubsExternal.empty())
		return NULL;

	if (VM_Player::formatContextExternal != NULL)
		VM_FileSystem::CloseMediaFormatContext(VM_Player::formatContextExternal, -1);

	VM_Player::formatContextExternal = VM_FileSystem::GetMediaFormatContext(VM_Player::SubsExternal[fileIndex], true);

	if (VM_Player::formatContextExternal == NULL)
		return NULL;

	stream = VM_FileSystem::GetMediaStreamByIndex(VM_Player::formatContextExternal, fileStreamIndex);

	return stream;
}

int MediaPlayer::VM_Player::openThreads()
{
	// AUDIO THREAD
	if (VM_Player::openAudio() != RESULT_OK)
		return ERROR_UNKNOWN;

	if (VM_Player::videoContext.stream != NULL)
	{
		// VIDEO THREAD
		if (VM_Player::openVideo() < 0)
			return ERROR_UNKNOWN;

		// SUB THREAD
		VM_Player::openSub();
	}

	// PACKET THREAD
	VM_Player::packetsThread = SDL_CreateThread(VM_Player::threadPackets, "packetsThread", NULL);

	if (VM_Player::packetsThread == NULL)
		return ERROR_UNKNOWN;

	return RESULT_OK;
}

int MediaPlayer::VM_Player::openVideo()
{
	if ((VM_Player::videoContext.stream == NULL) || (VM_Player::videoContext.stream->codec == NULL))
		return ERROR_INVALID_ARGUMENTS;

	if (VM_Player::videoContext.frameEncoded != NULL)
		VM_Player::videoContext.frameEncoded->linesize[0] = 0;

	VM_Player::videoContext.index     = VM_Player::videoContext.stream->index;
	VM_Player::videoContext.frame     = LIB_FFMPEG::av_frame_alloc();
	VM_Player::videoContext.condition = SDL_CreateCond();
	VM_Player::videoContext.mutex     = SDL_CreateMutex();

	if ((VM_Player::videoContext.mutex == NULL) || (VM_Player::videoContext.condition == NULL))
		return ERROR_UNKNOWN;

	// VIDEO DECODE THREAD
	VM_Player::videoContext.thread = SDL_CreateThread(VM_Player::threadVideo, "videoContext.thread", NULL);

	if ((VM_Player::videoContext.thread == NULL) || VM_Player::State.quit)
		return ERROR_UNKNOWN;

	return RESULT_OK;
}

int MediaPlayer::VM_Player::packetAdd(LIB_FFMPEG::AVPacket* packet, VM_Packets &packetQueue, SDL_mutex* mutex, SDL_cond* condition, bool &queueAvailable)
{
	if ((packet == NULL) || (mutex == NULL) || (condition == NULL))
		return ERROR_INVALID_ARGUMENTS;

	SDL_LockMutex(mutex);

	queueAvailable = false;
	packetQueue.push(packet);
	queueAvailable = true;
	
	SDL_CondSignal(condition);
	SDL_UnlockMutex(mutex);

	return RESULT_OK;
}

LIB_FFMPEG::AVPacket* MediaPlayer::VM_Player::packetGet(VM_Packets &packetQueue, SDL_mutex* mutex, SDL_cond* condition, bool &queueAvailable)
{
	LIB_FFMPEG::AVPacket* packet;

	if ((mutex == NULL) || (condition == NULL) || packetQueue.empty())
		return NULL;

	SDL_LockMutex(mutex);

	if (!queueAvailable)
		SDL_CondWait(condition, mutex);

	packet = packetQueue.front(); packetQueue.pop();

	SDL_UnlockMutex(mutex);
	
	return packet;
}

int MediaPlayer::VM_Player::packetsClear(VM_Packets &packetQueue, SDL_mutex* mutex, SDL_cond* condition, bool &queueAvailable)
{
	if ((mutex == NULL) || (condition == NULL))
		return ERROR_INVALID_ARGUMENTS;

	SDL_LockMutex(mutex);
	
	queueAvailable = false;

	while (!packetQueue.empty()) {
		FREE_PACKET(packetQueue.front());
		packetQueue.pop();
	}
	
	queueAvailable = true;

	SDL_CondSignal(condition);
	SDL_UnlockMutex(mutex);

	return RESULT_OK;
}

int MediaPlayer::VM_Player::Pause()
{
	if (VM_Player::State.isPaused)
		return ERROR_INVALID_ARGUMENTS;

	VM_Player::State.isPaused  = true;
	VM_Player::State.isPlaying = false;
	VM_Player::State.isStopped = false;

	VM_Player::PauseTime = time(NULL);
	VM_Player::PlayTime  = 0;

	if (!PICTURE_IS_SELECTED)
		SDL_PauseAudioDevice(VM_Player::audioContext.deviceID, 1);

	return RESULT_OK;
}

int MediaPlayer::VM_Player::Play()
{
	if (VM_Player::State.isPlaying)
		return ERROR_INVALID_ARGUMENTS;

	VM_Player::State.isPlaying = true;
	VM_Player::State.isPaused  = false;
	VM_Player::State.isStopped = false;

	VM_Player::PlayTime  = time(NULL);
	VM_Player::PauseTime = 0;

	if (!PICTURE_IS_SELECTED)
		SDL_PauseAudioDevice(VM_Player::audioContext.deviceID, 0);
	else
		VM_Player::pictureProgressTime = SDL_GetTicks();

	return RESULT_OK;
}

int MediaPlayer::VM_Player::PlayPauseToggle()
{
	if (VM_Player::State.isPlaying)
		VM_Player::Pause();
	else
		VM_Player::Play();

	return RESULT_OK;
}

int MediaPlayer::VM_Player::PlaylistLoopTypeToggle()
{
	//if (YOUTUBE_IS_SELECTED || SHOUTCAST_IS_SELECTED) {
	if (SHOUTCAST_IS_SELECTED) {
		VM_Player::State.loopType = LOOP_TYPE_NORMAL;
		return RESULT_OK;
	}

	switch (VM_Player::State.loopType) {
		case LOOP_TYPE_NORMAL:  VM_Player::State.loopType = LOOP_TYPE_LOOP;    break;
		case LOOP_TYPE_LOOP:    VM_Player::State.loopType = LOOP_TYPE_SHUFFLE; break;
		case LOOP_TYPE_SHUFFLE: VM_Player::State.loopType = LOOP_TYPE_NORMAL;  break;
		default: return ERROR_UNKNOWN;
	}

	int  dbResult;
	auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

	if (DB_RESULT_OK(dbResult))
		db->updateSettings("playlist_loop_type", std::to_string(VM_Player::State.loopType));

	DELETE_POINTER(db);

	return RESULT_OK;
}

void MediaPlayer::VM_Player::Refresh()
{
	if (!SDL_RectEmpty(&VM_Player::VideoDimensions)) {
		VM_Player::subContext.scale = {
			(float)((float)VM_Player::VideoDimensions.w / (float)VM_Player::subContext.size.x * DEFAULT_FONT_DPI_RATIO),
			(float)((float)VM_Player::VideoDimensions.h / (float)VM_Player::subContext.size.y * DEFAULT_FONT_DPI_RATIO)
		};

		for (auto sub : VM_Player::subContext.subs)
		{
			sub->skip = false;

			if ((sub->style != NULL) && !sub->style->fontName.empty())
				sub->style->openFont(VM_Player::subContext.styleFonts, VM_Player::subContext.scale, sub);
		}

		VM_SubFontEngine::RemoveSubs();
		DELETE_POINTER(VM_Player::subContext.texture);

		VM_Player::refreshSub = true;
	}
}

int MediaPlayer::VM_Player::Render(const SDL_Rect &location)
{
	if (SDL_RectEmpty(&location))
		return ERROR_INVALID_ARGUMENTS;

	VM_Player::cursorHide();

	if (PICTURE_IS_SELECTED)
	{
		VM_Player::renderPicture();
	}
	//else if (VIDEO_IS_SELECTED || YOUTUBE_IS_SELECTED)
	else if (VIDEO_IS_SELECTED)
	{
		if (VM_Player::State.isPaused && (VM_Player::subContext.texture == NULL))
			VM_Player::refreshSub = true;

		if (!VM_Player::State.isStopped) {
			VM_Player::renderVideo(location);
			VM_Player::renderSub(location);
		}
	}

	return RESULT_OK;
}

void MediaPlayer::VM_Player::renderPicture()
{
	if (VM_Player::seekRequested)
	{
		VM_Player::ProgressTime        = ((double)VM_Player::seekToPosition / AV_TIME_BASE_D);
		VM_Player::pictureProgressTime = (uint32_t)(VM_Player::ProgressTime * (double)ONE_SECOND_MS);
		VM_Player::seekRequested       = false;

		VM_PlayerControls::Refresh(REFRESH_PROGRESS);
	}

	if (VM_Player::State.isPlaying)
	{
		if ((SDL_GetTicks() - VM_Player::pictureProgressTime) >= ONE_SECOND_MS) {
			VM_Player::ProgressTime++;
			VM_Player::pictureProgressTime = SDL_GetTicks();
		}

		if (VM_Player::ProgressTime >= VM_Player::DurationTime) {
			VM_Player::mediaCompleted = true;
			VM_Player::Stop();
		}
	}
}

int MediaPlayer::VM_Player::renderSub(const SDL_Rect &location)
{
	if ((VM_Player::subContext.index < 0))
		return ERROR_UNKNOWN;

	SDL_LockMutex(VM_Player::subContext.subsMutex);

	if (!VM_Player::subContext.available)
		SDL_CondWait(VM_Player::subContext.subsCondition, VM_Player::subContext.subsMutex);

	if (VM_Player::seekRequested)
	{
		for (auto sub : VM_Player::subContext.subs) {
			VM_SubFontEngine::RemoveSubs(sub->id);
			DELETE_POINTER(sub);
		}

		VM_Player::subContext.subs.clear();
	}

	// REMOVE EXPIRED SUBS
	for (auto sub = VM_Player::subContext.subs.begin(); sub != VM_Player::subContext.subs.end();)
	{
		if ((VM_Player::ProgressTime > ((*sub)->pts.end - DELAY_TIME_SUB_RENDER))) //|| // Expired
			//((*sub)->pts.start > VM_Player::ProgressTime))   // Should not be displayed yet
		{
			#if defined _DEBUG
				auto delay = (VM_Player::ProgressTime - (*sub)->pts.end);

				if (delay > 0) {
					LOG("VM_Player::renderSub:\n");
					LOG("\tREMOVE_SUB: %s\n", (*sub)->text.c_str());
					LOG("\tREMOVE_DELAY: %.3fs (%.3f - %.3f)\n", delay, VM_Player::ProgressTime, (*sub)->pts.end);
				}
			#endif

			bool bottom = (*sub)->isAlignedBottom();

			VM_SubFontEngine::RemoveSubs((*sub)->id);
			DELETE_POINTER(*sub);
			sub = VM_Player::subContext.subs.erase(sub);

			if (bottom)
				VM_SubFontEngine::RemoveSubsBottom();

			VM_Player::refreshSub = true;
			continue;
		}

		sub++;
	}

	switch (VM_Player::subContext.type) {
		case LIB_FFMPEG::SUBTITLE_ASS:
		case LIB_FFMPEG::SUBTITLE_TEXT:
			VM_Player::renderSubText(location);
			break;
		case LIB_FFMPEG::SUBTITLE_BITMAP:
			VM_Player::renderSubBitmap(location);
			break;
	}

	SDL_UnlockMutex(VM_Player::subContext.subsMutex);

	return RESULT_OK;
}

void MediaPlayer::VM_Player::renderSubBitmap(const SDL_Rect &location)
{
	if (VM_Player::refreshSub &&
		!VM_Player::seekRequested &&
		!VM_Player::State.isStopped &&
		!VM_Player::State.quit)
	{
		VM_Player::refreshSub = false;

		auto ffmpegPixelFormat = LIB_FFMPEG::AV_PIX_FMT_BGRA;
		auto sdlPixelFormat    = SDL_PIXELFORMAT_ARGB8888;
		auto subStream         = VM_Player::subContext.stream;
		auto videoStream       = VM_Player::videoContext.stream;

		if (VM_Player::subContext.texture == NULL)
		{
			VM_Player::subContext.texture = new VM_Texture(
				sdlPixelFormat, SDL_TEXTUREACCESS_TARGET,
				(subStream->codec->width  > 0 ? subStream->codec->width  : videoStream->codec->width),
				(subStream->codec->height > 0 ? subStream->codec->height : videoStream->codec->height)
			);
		}

		if (VM_Player::subContext.texture->data != NULL)
		{
			SDL_SetRenderTarget(VM_Window::Renderer, VM_Player::subContext.texture->data);
			SDL_SetRenderDrawColor(VM_Window::Renderer, 0, 0, 0, 0);
			SDL_RenderClear(VM_Window::Renderer);
			SDL_SetTextureBlendMode(VM_Player::subContext.texture->data, SDL_BLENDMODE_BLEND);

			for (auto subIter = VM_Player::subContext.subs.begin(); subIter != VM_Player::subContext.subs.end(); subIter++)
			{
				VM_Subtitle* sub = *subIter;

				if ((sub == NULL) || (sub->subRect->w <= 0) || (sub->subRect->h <= 0))
					continue;

				// SKIP IF THERE ARE COLLISIONS/OVERLAP
				bool skipSub = false;

				for (auto subIter2 = subIter; subIter2 != VM_Player::subContext.subs.end(); subIter2++)
				{
					VM_Subtitle* sub2     = *subIter2;
					SDL_Rect     sub2Rect = { sub2->subRect->x, sub2->subRect->y, sub2->subRect->w, sub2->subRect->h };
					SDL_Rect     subRect  = { sub->subRect->x,  sub->subRect->y,  sub->subRect->w,  sub->subRect->h };

					if ((subIter2 != subIter) && SDL_HasIntersection(&sub2Rect, &subRect))
					{
						skipSub      = true;
						sub->pts.end = VM_Player::ProgressTime;

						break;
					}
				}

				if (skipSub)
					continue;

				LIB_FFMPEG::AVFrame* frameEncoded = LIB_FFMPEG::av_frame_alloc();
				int                  scaleResult  = 0;
				VM_Texture*          texture      = NULL;

				if (av_image_alloc(frameEncoded->data, frameEncoded->linesize, sub->subRect->w, sub->subRect->h, ffmpegPixelFormat, 32) > 0)
				{
					VM_Player::scaleContextSub = sws_getCachedContext(
						VM_Player::scaleContextSub,
						sub->subRect->w, sub->subRect->h, subStream->codec->pix_fmt,
						sub->subRect->w, sub->subRect->h, ffmpegPixelFormat, DEFAULT_SCALE_FILTER,
						NULL, NULL, NULL
					);
				}

				if (VM_Player::scaleContextSub != NULL) {
					scaleResult = sws_scale(
						VM_Player::scaleContextSub,
						sub->subRect->data, sub->subRect->linesize, 0, sub->subRect->h,
						frameEncoded->data, frameEncoded->linesize
					);
				}

				if (scaleResult > 0)
					texture = new VM_Texture(sdlPixelFormat, SDL_TEXTUREACCESS_STATIC, sub->subRect->w, sub->subRect->h);

				if ((texture != NULL) && (texture->data != NULL))
				{
					SDL_Rect renderLocation = {
						sub->subRect->x, sub->subRect->y, sub->subRect->w, sub->subRect->h
					};

					SDL_SetTextureBlendMode(texture->data, SDL_BLENDMODE_BLEND);
					SDL_UpdateTexture(texture->data, NULL, frameEncoded->data[0], frameEncoded->linesize[0]);
					SDL_RenderCopy(VM_Window::Renderer, texture->data, NULL, &renderLocation);

					DELETE_POINTER(texture);
				}

				LIB_FFMPEG::av_frame_free(&frameEncoded);
			}

			SDL_SetRenderTarget(VM_Window::Renderer, NULL);
		}
	}

	if (!VM_Player::subContext.subs.empty() &&
		(VM_Player::subContext.texture != NULL) &&
		(VM_Player::subContext.texture->data != NULL))
	{
		SDL_RenderCopy(VM_Window::Renderer, VM_Player::subContext.texture->data, NULL, &location);
	}
}

void MediaPlayer::VM_Player::renderSubText(const SDL_Rect &location)
{
	// Only re-create the ubs if they have changed
	if (VM_Player::refreshSub &&
		!VM_Player::subContext.subs.empty() &&
		!VM_Player::seekRequested &&
		!VM_Player::State.isStopped &&
		!VM_Player::State.quit)
	{
		VM_Player::refreshSub = false;

		// Create the texture
		if (VM_Player::subContext.texture == NULL)
		{
			VM_Player::subContext.texture = new VM_Texture(
				SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, VM_Player::VideoDimensions.w, VM_Player::VideoDimensions.h
			);
		}

		if ((VM_Player::subContext.texture != NULL) && (VM_Player::subContext.texture->data != NULL))
		{
			// Set the texture as the rendering target, and clear the existing content.
			SDL_SetRenderTarget(VM_Window::Renderer, VM_Player::subContext.texture->data);
			SDL_SetRenderDrawColor(VM_Window::Renderer, 0, 0, 0, 0);
			SDL_RenderClear(VM_Window::Renderer);
			SDL_SetTextureBlendMode(VM_Player::subContext.texture->data, SDL_BLENDMODE_BLEND);

			VM_SubFontEngine::RenderSubText(
				VM_Player::subContext.subs, VM_Player::subContext.fonts[FONT_MERGED], VM_Player::subContext.fonts[FONT_CJK]
			);

			SDL_SetRenderTarget(VM_Window::Renderer, NULL);
		}

		#if defined _DEBUG
		for (auto sub : VM_Player::subContext.subs) {
			auto delay = (VM_Player::ProgressTime - sub->pts.start);
			if (delay > 0)
				LOG("VM_Player::renderSubText: DELAY: %.3fs (%.3f - %.3f)\n", delay, VM_Player::ProgressTime, sub->pts.start);
		}
		#endif
	}

	SDL_SetRenderDrawBlendMode(VM_Window::Renderer, SDL_BLENDMODE_BLEND);

	if (!VM_Player::subContext.subs.empty() &&
		(VM_Player::subContext.texture != NULL) && 
		(VM_Player::subContext.texture->data != NULL))
	{
		SDL_RenderCopy(VM_Window::Renderer, VM_Player::subContext.texture->data, NULL, &VM_Player::VideoDimensions);
	}
}

void MediaPlayer::VM_Player::renderVideo(const SDL_Rect &location)
{
	bool updateTexture             = VM_Player::videoFrameAvailable;
	VM_Player::videoFrameAvailable = false;

	if ((VM_Player::videoContext.texture == NULL) || (VM_Player::videoContext.texture->data == NULL)) {
		VM_Player::renderVideoCreateTexture();
		updateTexture = true;
	}

	VM_Player::renderVideoScaleRenderLocation(location);

	if ((VM_Player::videoContext.texture != NULL) && (VM_Player::videoContext.texture->data != NULL))
	{
		if (updateTexture)
			VM_Player::renderVideoUpdateTexture();

		if (!SDL_RectEmpty(&VM_Player::VideoDimensions) && !VM_Player::State.quit)
		{
			VM_Color backgroundColor = VM_Color(SDL_COLOR_BLACK);
			VM_Graphics::FillArea(&backgroundColor, &location);

			SDL_RenderCopy(VM_Window::Renderer, VM_Player::videoContext.texture->data, NULL, &VM_Player::VideoDimensions);
		}
	}
}

int MediaPlayer::VM_Player::renderVideoCreateTexture()
{
	auto videoFrame  = VM_Player::videoContext.frame;
	auto videoStream = VM_Player::videoContext.stream;

	if (!VM_Player::videoContext.frameDecoded || (videoFrame == NULL) || (videoStream == NULL) || VM_Player::State.quit)
		return ERROR_UNKNOWN;

	auto sdlPixelFormat    = VM_Player::GetVideoPixelFormat(videoStream->codec->pix_fmt);
	auto ffmpegPixelFormat = VM_Player::GetVideoPixelFormat(sdlPixelFormat);

	DELETE_POINTER(VM_Player::videoContext.texture);

	VM_Player::videoContext.texture = new VM_Texture(
		sdlPixelFormat, SDL_TEXTUREACCESS_STREAMING, videoFrame->width, videoFrame->height
	);

	// CREATE A SCALING CONTEXT FOR NON-YUV420P VIDEOS
	if ((videoStream->codec->pix_fmt != LIB_FFMPEG::AV_PIX_FMT_YUV420P) && (VM_Player::videoContext.texture->data != NULL))
	{
		VM_Player::videoContext.frameEncoded = LIB_FFMPEG::av_frame_alloc();

		if (av_image_alloc(VM_Player::videoContext.frameEncoded->data, VM_Player::videoContext.frameEncoded->linesize, videoFrame->width, videoFrame->height, ffmpegPixelFormat, 32) <= 0)
			DELETE_POINTER(VM_Player::videoContext.texture);

		VM_Player::scaleContextVideo = sws_getCachedContext(
			VM_Player::scaleContextVideo,
			videoFrame->width, videoFrame->height, videoStream->codec->pix_fmt,
			videoFrame->width, videoFrame->height, ffmpegPixelFormat,
			DEFAULT_SCALE_FILTER, NULL, NULL, NULL
		);
					
		if (VM_Player::scaleContextVideo == NULL) {
			LIB_FFMPEG::av_frame_free(&VM_Player::videoContext.frameEncoded);
			DELETE_POINTER(VM_Player::videoContext.texture);
		}
	}

	return RESULT_OK;
}

int MediaPlayer::VM_Player::renderVideoScaleRenderLocation(const SDL_Rect &location)
{
	auto videoStream = VM_Player::videoContext.stream;

	if ((videoStream == NULL) || (videoStream->codec == NULL) || VM_Player::State.quit)
		return ERROR_UNKNOWN;

	int scaledWidth  = location.w;
	int scaledHeight = location.h;

	// CALCULATE THE OUTPUT SIZE RELATIVE TO THE ASPECT RATIO
	if (VM_Player::State.keepAspectRatio)
	{
		int maxWidth  = (int)((float)videoStream->codec->width  / (float)videoStream->codec->height * (float)location.h);
		int maxHeight = (int)((float)videoStream->codec->height / (float)videoStream->codec->width  * (float)location.w);

		scaledWidth  = (maxWidth  <= location.w ? maxWidth  : location.w);
		scaledHeight = (maxHeight <= location.h ? maxHeight : location.h);
	}

	bool isVideoDimensionsEmpty = SDL_RectEmpty(&VM_Player::VideoDimensions);

	// CALCULATE THE RENDER LOCATION
	VM_Player::VideoDimensions.x = (location.x + ((location.w - scaledWidth)  / 2));
	VM_Player::VideoDimensions.y = (location.y + ((location.h - scaledHeight) / 2));
	VM_Player::VideoDimensions.w = scaledWidth;
	VM_Player::VideoDimensions.h = scaledHeight;

	if (isVideoDimensionsEmpty) {
		VM_Player::Refresh();
	}

	return RESULT_OK;
}

int MediaPlayer::VM_Player::renderVideoUpdateTexture()
{
	auto videoFrame  = VM_Player::videoContext.frame;
	auto videoStream = VM_Player::videoContext.stream;

	if (!VM_Player::videoContext.frameDecoded || !AVFRAME_VALID(videoFrame) || (videoStream == NULL) || (videoStream->codec == NULL) || VM_Player::State.quit)
		return ERROR_UNKNOWN;

	// FOR YUV420P VIDEOS, COPY THE FRAME DIRECTLY TO THE TEXTURE
	if (VM_Player::IsYUV(videoStream->codec->pix_fmt))
	{
		SDL_UpdateYUVTexture(
			VM_Player::videoContext.texture->data, NULL,
			videoFrame->data[0], videoFrame->linesize[0],
			videoFrame->data[1], videoFrame->linesize[1],
			videoFrame->data[2], videoFrame->linesize[2]
		);
	}
	// FOR NON-YUV420P VIDEOS, CONVERT THE PIXEL FORMAT TO YUV420P
	else
	{
		int scaleResult = sws_scale(
			VM_Player::scaleContextVideo, videoFrame->data, videoFrame->linesize, 0, videoFrame->height,
			VM_Player::videoContext.frameEncoded->data, VM_Player::videoContext.frameEncoded->linesize
		);

		// COPY THE CONVERTED FRAME TO THE TEXTURE
		if (scaleResult > 0) {
			SDL_UpdateYUVTexture(
				VM_Player::videoContext.texture->data, NULL, 
				VM_Player::videoContext.frameEncoded->data[0], VM_Player::videoContext.frameEncoded->linesize[0],
				VM_Player::videoContext.frameEncoded->data[1], VM_Player::videoContext.frameEncoded->linesize[1],
				VM_Player::videoContext.frameEncoded->data[2], VM_Player::videoContext.frameEncoded->linesize[2]
			);
		}
	}

	return RESULT_OK;
}

void MediaPlayer::VM_Player::reset()
{
	VM_Player::isCursorHidden           = false;
	VM_Player::CursorLastVisible        = 0;
	VM_Player::DurationTime             = 0.0;
	VM_Player::FormatContext            = NULL;
	VM_Player::formatContextExternal    = NULL;
	VM_Player::fullscreenEnterWindowed  = false;
	VM_Player::fullscreenExitStop       = false;
	VM_Player::nrOfStreams              = 0;
	VM_Player::packetsThread            = NULL;
	VM_Player::PauseTime                = 0;
	VM_Player::PlayTime                 = 0;
	VM_Player::pictureProgressTime      = 0;
	VM_Player::ProgressTime             = 0.0;
	VM_Player::progressTimeLast         = 0.0;
	VM_Player::refreshSub               = false;
	VM_Player::resampleContext          = NULL;
	VM_Player::scaleContextSub          = NULL;
	VM_Player::scaleContextVideo        = NULL;
	VM_Player::seekRequested            = false;
	VM_Player::pausedVideoSeekRequested = false;
	VM_Player::seekToPosition           = 0;
	VM_Player::SelectedRow              = {};
	VM_Player::TimeOut                  = NULL;
	VM_Player::videoFrameAvailable      = false;
	VM_Player::VideoDimensions          = {};

	VM_Player::audioContext.reset();
	VM_Player::subContext.reset();
	VM_Player::videoContext.reset();
	VM_Player::State.Reset();

	for (uint32_t i = 0; i < NR_OF_FONTS; i++)
		VM_Player::subContext.fonts[i] = NULL;
}

int MediaPlayer::VM_Player::RotatePicture()
{
	if (!PICTURE_IS_SELECTED)
		return ERROR_INVALID_ARGUMENTS;

	double degrees = -90.0;
	int    mediaID = VM_Player::SelectedRow.mediaID;

	if (mediaID > 0)
	{
		int  dbResult;
		auto db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

		if (DB_RESULT_OK(dbResult)) {
			double angle = std::atof(db->getValue(mediaID, "rotation").c_str());
			db->updateDouble(mediaID, "rotation", (angle > 260.0 ? 0 : degrees + angle));
		}

		DELETE_POINTER(db);
	}

	for (auto image : VM_ThreadManager::Images)
	{
		bool resize = false;

		#if defined _ios
		if (VM_FileSystem::IsITunes(VM_Player::SelectedRow.mediaURL) && PICTURE_IS_SELECTED) {
			if (image.first.find(VM_Player::SelectedRow.mediaURL) != String::npos)
				resize = true;
		} else {
		#endif
			if (VM_Player::State.filePath.empty())
				continue;

			#if defined _windows
			if (image.first.find(VM_Text::ToUTF16(VM_Player::State.filePath.c_str())) != String::npos)
				resize = true;
			#else
			if (image.first.find(VM_Player::State.filePath) != String::npos)
				resize = true;
			#endif
		#if defined _ios
		}
		#endif

		if (resize && (image.second != NULL))
		{
			image.second->id    = mediaID;
			image.second->ready = false;

			VM_GUI::ListTable->refreshSelected();
		}
	}

	return RESULT_OK;
}

int MediaPlayer::VM_Player::Seek(int64_t seekTime)
{
	VM_Player::seekToPosition = seekTime;
	VM_Player::seekRequested  = true;

	if (VIDEO_IS_SELECTED && VM_Player::State.isPaused)
		VM_Player::pausedVideoSeekRequested = true;

	return RESULT_OK;
}

void MediaPlayer::VM_Player::seek()
{
	int seekFlags = AV_SEEK_FLAGS(VM_Player::FormatContext->iformat);

	if (avformat_seek_file(VM_Player::FormatContext, -1, INT64_MIN, VM_Player::seekToPosition, INT64_MAX, seekFlags) >= 0)
	{
		if (VM_Player::subContext.index >= SUB_STREAM_EXTERNAL)
			avformat_seek_file(VM_Player::formatContextExternal, -1, INT64_MIN, VM_Player::seekToPosition, INT64_MAX, seekFlags);

		VM_Player::packetsClear(VM_Player::audioContext.packets, VM_Player::audioContext.mutex, VM_Player::audioContext.condition, VM_Player::audioContext.packetsAvailable);
		VM_Player::packetsClear(VM_Player::subContext.packets,   VM_Player::subContext.mutex,   VM_Player::subContext.condition,   VM_Player::subContext.packetsAvailable);
		VM_Player::packetsClear(VM_Player::videoContext.packets, VM_Player::videoContext.mutex, VM_Player::videoContext.condition, VM_Player::videoContext.packetsAvailable);

		VM_Player::audioContext.bufferOffset    = 0;
		VM_Player::audioContext.bufferRemaining = 0;
		VM_Player::audioContext.writtenToStream = 0;
		VM_Player::audioContext.decodeFrame     = true;
		VM_Player::ProgressTime                 = (double)((double)VM_Player::seekToPosition / AV_TIME_BASE_D);
		VM_Player::videoContext.pts             = 0.0;

		VM_PlayerControls::Refresh(REFRESH_PROGRESS);
	}

	VM_Player::seekRequested = false;
}

int MediaPlayer::VM_Player::SetAudioVolume(int volume)
{
	bool muteState = VM_Player::State.isMuted;

	if (volume <= 5) {
		VM_Player::State.audioVolume = 0;
		VM_Player::State.isMuted     = true;
	} else if (volume >= 120) {
		VM_Player::State.audioVolume = SDL_MIX_MAXVOLUME;
		VM_Player::State.isMuted     = false;
	} else {
		VM_Player::State.audioVolume = volume;
		VM_Player::State.isMuted     = false;
	}

	if (VM_Player::State.isMuted == muteState)
		VM_PlayerControls::Refresh(REFRESH_VOLUME);
	else
		VM_PlayerControls::Refresh(REFRESH_VOLUME_AND_MUTE);

	int  dbResult;
	auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

	if (DB_RESULT_OK(dbResult))
	{
		db->updateSettings("is_muted", (VM_Player::State.isMuted ? "1" : "0"));

		if (!VM_Player::State.isMuted)
			db->updateSettings("audio_volume", std::to_string(VM_Player::State.audioVolume));
	}

	DELETE_POINTER(db);

	return RESULT_OK;
}

int MediaPlayer::VM_Player::SetStream(int streamIndex, bool seek)
{
	// DISABLE SUB STREAM
	if (streamIndex < 0)
	{
		FREE_STREAM(VM_Player::subContext.stream);
		VM_Player::subContext.index = -1;

		return ERROR_UNKNOWN;
	}

	// CALCULATE SEEK POSITION
	int64_t lastTimeStamp = 0;

	if (seek)
	{
		double percent    = (VM_Player::ProgressTime / VM_Player::DurationTime), fileSize;
		bool   seekBinary = (AV_SEEK_FLAGS(VM_Player::FormatContext->iformat) == AVSEEK_FLAG_BYTE);

		if (seekBinary)
		{
			int  dbResult;
			auto db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

			fileSize = 0;

			if (DB_RESULT_OK(dbResult))
				fileSize = std::atof(db->getValue(VM_GUI::ListTable->getSelectedMediaID(), "size").c_str());

			DELETE_POINTER(db);

			lastTimeStamp = (int64_t)(fileSize * percent);
		} else {
			lastTimeStamp = (int64_t)(VM_Player::ProgressTime * AV_TIME_BASE_I64);
		}
	}

	// GET MEDIA TYPE
	VM_MediaType mediaType = MEDIA_TYPE_UNKNOWN;

	if (streamIndex < SUB_STREAM_EXTERNAL)
	{
		LIB_FFMPEG::AVStream* stream = VM_Player::FormatContext->streams[streamIndex];

		if ((stream != NULL) && (stream->codec != NULL))
			mediaType = (VM_MediaType)stream->codec->codec_type;
	} else {
		mediaType = MEDIA_TYPE_SUBTITLE;
	}

	// AUDIO
	switch (mediaType) {
	case MEDIA_TYPE_AUDIO:
		if (streamIndex == VM_Player::audioContext.index)
			return ERROR_UNKNOWN;

		VM_Player::closeAudio();
		VM_Player::closeStream(MEDIA_TYPE_AUDIO);

		VM_Player::audioContext.stream = VM_FileSystem::GetMediaStreamByIndex(VM_Player::FormatContext, streamIndex);

		if (VM_Player::audioContext.stream != NULL)
			VM_Player::audioContext.index = VM_Player::audioContext.stream->index;

		if (VM_Player::openAudio() != RESULT_OK) {
			VM_Player::Stop(VM_Text::Format("[MP-P-%d] %s '%s'", __LINE__, VM_Window::Labels["error.play"].c_str(), VM_Player::State.filePath.c_str()));
			return ERROR_UNKNOWN;
		}

		break;
	// SUB
	case MEDIA_TYPE_SUBTITLE:
		if (streamIndex == VM_Player::subContext.index)
			return ERROR_UNKNOWN;

		VM_Player::closeSub();
		VM_Player::closeStream(MEDIA_TYPE_SUBTITLE);

		if (streamIndex >= SUB_STREAM_EXTERNAL)
			VM_Player::subContext.stream = VM_Player::openSubExternal(streamIndex);
		else
			VM_Player::subContext.stream = VM_FileSystem::GetMediaStreamByIndex(VM_Player::FormatContext, streamIndex);

		VM_Player::openSub();

		if (VM_Player::subContext.stream != NULL)
			VM_Player::subContext.index = streamIndex;

		break;
	// INVALID MEDIA TYPE
	default:
		VM_Player::Stop(VM_Text::Format("[MP-P-%d] %s '%s'", __LINE__, VM_Window::Labels["error.play"].c_str(), VM_Player::State.filePath.c_str()));
		return ERROR_UNKNOWN;
	}

	if (seek)
		VM_Player::Seek(lastTimeStamp);

	return RESULT_OK;
}

int MediaPlayer::VM_Player::Stop(const String &errorMessage)
{
	VM_Player::State.quit         = true;
	VM_Player::State.errorMessage = errorMessage;

	if (!errorMessage.empty())
		VM_Player::mediaCompleted = false;

	return RESULT_OK;
}

void MediaPlayer::VM_Player::threadAudio(void* userData, Uint8* stream, int streamSize)
{
	int decodeSize, frameDecoded, samplesResampled, outSamples, outSamplesBytes, writeSize;

	auto inSampleFormat = VM_Player::GetAudioSampleFormat(VM_Player::audioContext.device.format);
	int  inSamplesBytes = (VM_Player::audioContext.device.channels * LIB_FFMPEG::av_get_bytes_per_sample(inSampleFormat));

	while (VM_Player::State.isPaused && !VM_Player::State.quit)
		SDL_Delay(DELAY_TIME_DEFAULT);

	while ((streamSize > 0) &&
		VM_Player::State.isPlaying &&
		!VM_Player::seekRequested &&
		!VM_Player::State.quit)
	{
		if (VM_Player::audioContext.decodeFrame)
		{
			// GET PACKET FROM QUEUE
			if (!VM_Player::audioContext.decodeReUsePacket) {
				VM_Player::audioContext.packet = VM_Player::packetGet(
					VM_Player::audioContext.packets,
					VM_Player::audioContext.mutex, VM_Player::audioContext.condition,
					VM_Player::audioContext.packetsAvailable
				);
			}

			if ((VM_Player::audioContext.packet == NULL) || VM_Player::State.quit){
				SDL_Delay(DELAY_TIME_DEFAULT);
				break;
			}

			// DECODE PACKET TO FRAME
			av_frame_unref(VM_Player::audioContext.frame);

			decodeSize = avcodec_decode_audio4(
				VM_Player::audioContext.stream->codec, VM_Player::audioContext.frame,
				&frameDecoded, VM_Player::audioContext.packet
			);

			// MULTIPLE FRAMES PER PACKET
			VM_Player::audioContext.decodeReUsePacket = (
				(decodeSize >= 0) && (decodeSize < VM_Player::audioContext.packet->size)
			);

			if (decodeSize > 0) {
				VM_Player::audioContext.packet->size -= decodeSize;
				VM_Player::audioContext.packet->data += decodeSize;
			}

			if (VM_Player::audioContext.packet->duration > 0) {
				VM_Player::audioContext.frameDuration = (double)(
					(double)VM_Player::audioContext.packet->duration * LIB_FFMPEG::av_q2d(VM_Player::audioContext.stream->time_base)
				);
			}

			if (!VM_Player::audioContext.decodeReUsePacket)
				FREE_PACKET(VM_Player::audioContext.packet);

			if (frameDecoded)
			{
				if (VM_Player::ProgressTime > 0)
					VM_Player::progressTimeLast = VM_Player::ProgressTime;

				// SET PROGRESS TIME
				double framePTS = (double)av_frame_get_best_effort_timestamp(VM_Player::audioContext.frame);

				if (AV_START_FLAGS(VM_Player::FormatContext->iformat))
					framePTS -= (double)VM_Player::audioContext.stream->start_time;

				framePTS *= LIB_FFMPEG::av_q2d(VM_Player::audioContext.stream->time_base);

				if (framePTS < 0)
					framePTS = (VM_Player::progressTimeLast + VM_Player::audioContext.frameDuration);

				if ((VM_Player::audioContext.frameDuration <= 0) &&
					(framePTS > 0) &&
					(VM_Player::progressTimeLast > 0) &&
					(framePTS != VM_Player::progressTimeLast))
				{
					VM_Player::audioContext.frameDuration = (framePTS - VM_Player::progressTimeLast);
					VM_Player::progressTimeLast           = framePTS;
				}

				VM_Player::ProgressTime = (double)framePTS;

				// SET RESAMPLE SETTINGS
				VM_Player::resampleContext = swr_alloc_set_opts(
					VM_Player::resampleContext, 
					LIB_FFMPEG::av_get_default_channel_layout(VM_Player::audioContext.device.channels),
					inSampleFormat,
					VM_Player::audioContext.device.freq,
					VM_Player::audioContext.frame->channel_layout,
					(LIB_FFMPEG::AVSampleFormat)VM_Player::audioContext.frame->format,
					VM_Player::audioContext.frame->sample_rate,
					0, NULL
				);

				if ((VM_Player::resampleContext == NULL) || (swr_init(VM_Player::resampleContext) < 0)) {
					VM_Player::Stop(VM_Text::Format("[MP-P-%d] %s '%s'", __LINE__, VM_Window::Labels["error.play"].c_str(), VM_Player::State.filePath.c_str()));
					break;
				}

				if (VM_Player::State.quit)
					break;

				// CALCULATE NEEDED FRAME BUFFER SIZE FOR RESAMPLING
				outSamples      = swr_get_out_samples(VM_Player::resampleContext, VM_Player::audioContext.frame->nb_samples);
				outSamplesBytes = (outSamples * inSamplesBytes);

				// INCREASE THE FRAME BUFFER SIZE IF NEEDED
				if (outSamplesBytes > VM_Player::audioContext.bufferSize) {
					VM_Player::audioContext.frameEncoded = (uint8_t*)realloc(VM_Player::audioContext.frameEncoded, outSamplesBytes);
					VM_Player::audioContext.bufferSize   = outSamplesBytes;
				}

				// RESAMPLE THE AUDIO FRAME
				samplesResampled = swr_convert(
					VM_Player::resampleContext,
					&VM_Player::audioContext.frameEncoded,
					outSamples,
					(const uint8_t**)VM_Player::audioContext.frame->extended_data,
					VM_Player::audioContext.frame->nb_samples
				);

				if (samplesResampled < 0) {
					VM_Player::Stop(VM_Text::Format("[MP-P-%d] %s '%s'", __LINE__, VM_Window::Labels["error.play"].c_str(), VM_Player::State.filePath.c_str()));
					break;
				}

				if (VM_Player::State.quit)
					break;

				// CALCULATE ACTUAL FRAME BUFFER SIZE AFTER RESAMPLING
				VM_Player::audioContext.bufferRemaining = (int)(
					samplesResampled * VM_Player::audioContext.device.channels * LIB_FFMPEG::av_get_bytes_per_sample(inSampleFormat)
				);
			} else {
				VM_Player::audioContext.bufferRemaining = 0;
			}

			// RESET BUFFER OFFSET
			VM_Player::audioContext.bufferOffset = 0;
		}

		if (VM_Player::seekRequested || VM_Player::State.quit)
			break;

		// CALCULATE HOW MUCH TO WRITE TO THE STREAM
		if (VM_Player::audioContext.writtenToStream > 0)
			writeSize = (streamSize - VM_Player::audioContext.writtenToStream);
		else if (VM_Player::audioContext.bufferRemaining > streamSize)
			writeSize = streamSize;
		else
			writeSize = VM_Player::audioContext.bufferRemaining;

		if (writeSize > VM_Player::audioContext.bufferRemaining)
			writeSize = VM_Player::audioContext.bufferRemaining;

		// WRITE THE DATA FROM THE BUFFER TO THE STREAM
		memset(stream, 0, writeSize);

		SDL_MixAudioFormat(
			stream,
			static_cast<const Uint8*>(VM_Player::audioContext.frameEncoded + VM_Player::audioContext.bufferOffset),
			VM_Player::audioContext.device.format, writeSize, VM_Player::State.audioVolume
		);

		// UPDATE COUNTERS
		stream                                  += writeSize;
		VM_Player::audioContext.writtenToStream += writeSize;
		VM_Player::audioContext.bufferRemaining -= writeSize;

		// STREAM IS FULL, BUFFER IS NOT => GET NEW STREAM, DON'T DECODE FRAME
		if (((VM_Player::audioContext.writtenToStream % streamSize) == 0) &&
			(VM_Player::audioContext.bufferRemaining > 0))
		{
			VM_Player::audioContext.decodeFrame     = false;
			VM_Player::audioContext.writtenToStream = 0;
			VM_Player::audioContext.bufferOffset   += writeSize;

			break;
		}
		// STREAM IS FULL, BUFFER IS FULL => GET NEW STREAM, DECODE FRAME
		else if (((VM_Player::audioContext.writtenToStream % streamSize) == 0) &&
			(VM_Player::audioContext.bufferRemaining <= 0))
		{
			VM_Player::audioContext.decodeFrame     = true;
			VM_Player::audioContext.writtenToStream = 0;

			break;
		}
		// STREAM IS NOT FULL, BUFFER IS FULL => DON'T GET NEW STREAM, DECODE FRAME
		else if (((VM_Player::audioContext.writtenToStream % streamSize) != 0) &&
			(VM_Player::audioContext.bufferRemaining <= 0))
		{
			VM_Player::audioContext.decodeFrame = true;
		}
		// STREAM IS NOT FULL, BUFFER IS NOT FULL => DON'T GET NEW STREAM, DON'T DECODE FRAME
		else if (((VM_Player::audioContext.writtenToStream % streamSize) != 0) &&
			(VM_Player::audioContext.bufferRemaining > 0))
		{
			VM_Player::audioContext.decodeFrame   = false;
			VM_Player::audioContext.bufferOffset += writeSize;
		}
	}
}

int MediaPlayer::VM_Player::threadPackets(void* userData)
{
	LIB_FFMPEG::AVPacket* packet;
	int                   result;
	bool                  endOfFile  = false;
	int                   errorCount = 0;

	while (!VM_Player::State.quit)
	{
		// SEEK
		if (VM_Player::seekRequested)
			VM_Player::seek();

		if (VM_Player::State.quit)
			break;

		// PACKET INIT
		packet = (LIB_FFMPEG::AVPacket*)LIB_FFMPEG::av_mallocz(sizeof(LIB_FFMPEG::AVPacket));

		if (packet == NULL) {
			VM_Player::Stop(VM_Text::Format("VM_Player::threadPackets: %s '%s'\n", VM_Window::Labels["error.play"].c_str(), VM_Player::State.filePath.c_str()));
			return ERROR_UNKNOWN;
		}

		av_init_packet(packet);
		VM_Player::TimeOut->start();

		if (VM_Player::State.quit)
			break;

		// PACKET READ
		if ((result = av_read_frame(VM_Player::FormatContext, packet)) < 0)
		{
			VM_Player::TimeOut->stop();

			#if defined _DEBUG
				char strerror[AV_ERROR_MAX_STRING_SIZE];
				LIB_FFMPEG::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
				LOG(VM_Text::Format("VM_Player::threadPackets: %s\n", strerror).c_str());
			#endif

			if ((result == AVERROR_EOF) || ((VM_Player::FormatContext->pb != NULL) && VM_Player::FormatContext->pb->eof_reached))
				endOfFile = true;
			else
				errorCount++;

			if (endOfFile || (errorCount >= MAX_ERRORS))
			{
				FREE_PACKET(packet);

				while (!VM_Player::audioContext.packets.empty() && !VM_Player::State.quit)
					SDL_Delay(DELAY_TIME_DEFAULT);

				if (VM_Player::State.quit)
					break;

				if (errorCount >= MAX_ERRORS) {
					VM_Player::Stop(VM_Text::Format("[MP-P-%d] %s '%s'", __LINE__, VM_Window::Labels["error.play"].c_str(), VM_Player::State.filePath.c_str()));
					return ERROR_UNKNOWN;
				} else {
					VM_Player::Stop();
				}

				VM_Player::mediaCompleted = true;

				break;
			}
		} else if (errorCount > 0) {
			errorCount = 0;
		}

		if (VM_Player::State.quit)
			break;

		VM_Player::TimeOut->stop();

		// WAIT IF THE QUEUES ARE FULL
		while ((VM_Player::isPacketQueueFull(MEDIA_TYPE_AUDIO) || VM_Player::isPacketQueueFull(MEDIA_TYPE_VIDEO)) && !VM_Player::seekRequested && !VM_Player::State.quit)
		{
			if (VIDEO_IS_SELECTED && (VM_Player::audioContext.packets.empty() || VM_Player::videoContext.packets.empty()))
				break;

			SDL_Delay(DELAY_TIME_ONE_MS);
		}

		if (VM_Player::State.quit)
			break;

		// AUDIO PACKET
		if (packet->stream_index == VM_Player::audioContext.index)
			VM_Player::packetAdd(packet, VM_Player::audioContext.packets, VM_Player::audioContext.mutex, VM_Player::audioContext.condition, VM_Player::audioContext.packetsAvailable);
		// VIDEO PACKET
		else if (packet->stream_index == VM_Player::videoContext.index)
			VM_Player::packetAdd(packet, VM_Player::videoContext.packets, VM_Player::videoContext.mutex, VM_Player::videoContext.condition, VM_Player::videoContext.packetsAvailable);
		// SUB PACKET - INTERNAL
		else if ((packet->stream_index == VM_Player::subContext.index) && (VM_Player::subContext.index != SUB_STREAM_EXTERNAL))
			VM_Player::packetAdd(packet, VM_Player::subContext.packets, VM_Player::subContext.mutex, VM_Player::subContext.condition, VM_Player::subContext.packetsAvailable);
		// UNKNOWN PACKET
		else
			FREE_PACKET(packet);

		if (VM_Player::State.quit)
			break;

		// SUB PACKET - EXTERNAL
		if ((VM_Player::subContext.index >= SUB_STREAM_EXTERNAL) && (VM_Player::subContext.packets.size() < MIN_PACKET_QUEUE_SIZE))
		{
			packet = (LIB_FFMPEG::AVPacket*)LIB_FFMPEG::av_mallocz(sizeof(LIB_FFMPEG::AVPacket));
			
			if (packet != NULL)
			{
				av_init_packet(packet);

				int extSubIntIdx = ((VM_Player::subContext.index - SUB_STREAM_EXTERNAL) % SUB_STREAM_EXTERNAL);

				if ((av_read_frame(VM_Player::formatContextExternal, packet) == 0) && (packet->stream_index == extSubIntIdx))
					VM_Player::packetAdd(packet, VM_Player::subContext.packets, VM_Player::subContext.mutex, VM_Player::subContext.condition, VM_Player::subContext.packetsAvailable);
				else
					FREE_PACKET(packet);
			}
		}
	}

	return RESULT_OK;
}

int MediaPlayer::VM_Player::threadSub(void* userData)
{
	int                    decodeResult, frameDecoded;
	LIB_FFMPEG::AVPacket*  packet;
	LIB_FFMPEG::AVSubtitle subFrame  = {};
	const char*            codecName = VM_Player::subContext.stream->codec->codec->name;

	while (!VM_Player::State.quit)
	{
		while ((VM_Player::subContext.packets.empty() ||
			VM_Player::State.isPaused || VM_Player::seekRequested ||
			(VM_Player::subContext.index < 0) || (VM_Player::audioContext.index < 0)) &&
			!VM_Player::State.quit)
		{
			SDL_Delay(DELAY_TIME_DEFAULT);
		}

		if (VM_Player::State.quit)
			break;

		// GET PACKET
		packet = VM_Player::packetGet(VM_Player::subContext.packets, VM_Player::subContext.mutex, VM_Player::subContext.condition, VM_Player::subContext.packetsAvailable);

		if ((packet == NULL) || (VM_Player::subContext.index < 0)) {
			FREE_PACKET(packet);
			continue;
		}

		if (VM_Player::State.quit)
			break;

		// DECODE PACKET TO FRAME
		decodeResult = avcodec_decode_subtitle2(VM_Player::subContext.stream->codec, &subFrame, &frameDecoded, packet);

		// REPLACE YELLOW DVD SUBS WITH WHITE SUBS IF IT'S MISSING THE COLOR PALETTE
		if (strcmp(codecName, "dvdsub") == 0)
		{
			VM_DVDSubContext* dvdSubContext = static_cast<VM_DVDSubContext*>(VM_Player::subContext.stream->codec->priv_data);

			if ((dvdSubContext != NULL) && !dvdSubContext->has_palette) {
				dvdSubContext->has_palette = 1;
				dvdSubContext->palette[dvdSubContext->colormap[3]] = 0xFF000000;	// BORDER
				dvdSubContext->palette[dvdSubContext->colormap[1]] = 0xFFFFFFFF;	// TEXT

				decodeResult = avcodec_decode_subtitle2(VM_Player::subContext.stream->codec, &subFrame, &frameDecoded, packet);
			}
		}

		// INVALID PACKET
		if ((decodeResult < 0) || !frameDecoded) {
			FREE_PACKET(packet);
			continue;
		}

		if (VM_Player::State.quit)
			break;

		SDL_LockMutex(VM_Player::subContext.subsMutex);
		VM_Player::subContext.available = false;

		// UPDATE END PTS FOR PREVIOUS SUB
		// Some sub types, like PGS, come in pairs:
		// - The first one with data but without duration or end PTS
		// - The second one has no data, but contains the end PTS
		if ((strcmp(codecName, "pgssub") == 0) && (packet->size < MIN_SUB_PACKET_SIZE) && !VM_Player::subContext.subs.empty())
			VM_Player::subContext.subs.back()->pts = VM_SubFontEngine::GetSubPTS(packet, subFrame, VM_Player::subContext.stream);

		VM_Player::subContext.available = true;
		SDL_CondSignal(VM_Player::subContext.subsCondition);
		SDL_UnlockMutex(VM_Player::subContext.subsMutex);

		// NO FRAMES DECODED
		if ((subFrame.num_rects == 0) || ((packet->duration == 0) && (subFrame.end_display_time == 0))) {
			FREE_PACKET(packet);
			continue;
		}

		// SET PTS
		VM_PTS pts = VM_SubFontEngine::GetSubPTS(packet, subFrame, VM_Player::subContext.stream);
		FREE_PACKET(packet);

		if ((pts.end - pts.start) < 0.3) {
			FREE_SUB_FRAME(subFrame);
			continue;
		}

		// EXTRACT TEXT/BITMAP FROM FRAME
		Strings      subTexts;
		VM_Subtitle* bitmapSub = NULL;

		for (uint32_t i = 0; i < subFrame.num_rects; i++)
		{
			if (subFrame.rects[i] == NULL)
				continue;

			VM_Player::subContext.type = subFrame.rects[i]->type;

			switch (VM_Player::subContext.type) {
			case LIB_FFMPEG::SUBTITLE_ASS:
				if ((subFrame.rects[i]->ass != NULL) && (strlen(subFrame.rects[i]->ass) > 0)) {
					subTexts.push_back(subFrame.rects[i]->ass);
					FREE_SUB_TEXT(subFrame.rects[i]->ass);
				}
				break;
			case LIB_FFMPEG::SUBTITLE_TEXT:
				if ((subFrame.rects[i]->text != NULL) && (strlen(subFrame.rects[i]->text) > 0)) {
					subTexts.push_back(subFrame.rects[i]->text);
					FREE_SUB_TEXT(subFrame.rects[i]->text);
				}
				break;
			case LIB_FFMPEG::SUBTITLE_BITMAP:
				bitmapSub          = new VM_Subtitle();
				bitmapSub->pts     = VM_PTS(pts.start, pts.end);
				bitmapSub->subRect = new VM_SubRect(*subFrame.rects[i]);
				break;
			}
		}

		// SPLIT AND FORMAT THE TEXT SUBS
		VM_Subtitles textSubs;

		switch (VM_Player::subContext.type) {
		case LIB_FFMPEG::SUBTITLE_ASS:
		case LIB_FFMPEG::SUBTITLE_TEXT:
			textSubs = VM_SubFontEngine::SplitAndFormatSub(subTexts, VM_Player::subContext.styles, VM_Player::subContext.subs);

			for (auto textSub : textSubs)
				textSub->pts = VM_PTS(pts.start, pts.end);

			break;
		}

		// WAIT BEFORE MAKING THE SUB AVAILABLE
		while (((pts.start - VM_Player::ProgressTime) > DELAY_TIME_SUB_RENDER) &&
			(VM_Player::subContext.index >= 0) &&
			(VM_Player::audioContext.index >= 0) &&
			!VM_Player::seekRequested &&
			!VM_Player::State.quit)
		{
			SDL_Delay(DELAY_TIME_ONE_MS);
		}

		#if defined _DEBUG
		if ((pts.start - VM_Player::ProgressTime) < 0)
			LOG("VM_Player::threadSub: MAKE_SUB_AVAIL_NEGATIVE: %.3f (%.3f - %.3f)", (pts.start - VM_Player::ProgressTime), pts.start, VM_Player::ProgressTime);
		#endif

		if (VM_Player::State.quit)
			break;

		if (VM_Player::seekRequested || (VM_Player::subContext.index < 0) || (VM_Player::audioContext.index < 0)) {
			FREE_SUB_FRAME(subFrame);
			continue;
		}

		// MAKE THE SUBS AVAILABLE TO THE RENDERER
		SDL_LockMutex(VM_Player::subContext.subsMutex);
		VM_Player::subContext.available = false;

		// BITMAP SUB
		if (bitmapSub != NULL)
			VM_Player::subContext.subs.push_back(bitmapSub);

		// TEXT SUBS
		for (auto textSub : textSubs)
		{
			// OPEN THE SUB STYLE FONTS
			if ((textSub->style != NULL) && !textSub->style->fontName.empty())
				textSub->style->openFont(VM_Player::subContext.styleFonts, VM_Player::subContext.scale, textSub);

			VM_Player::subContext.subs.push_back(textSub);
		}

		VM_Player::subContext.available = true;
		SDL_CondSignal(VM_Player::subContext.subsCondition);
		SDL_UnlockMutex(VM_Player::subContext.subsMutex);

		VM_Player::refreshSub = true;

		FREE_SUB_FRAME(subFrame);
	}

	// RELEASE SUB RESOURCES
	FREE_SUB_FRAME(subFrame);

	SDL_LockMutex(VM_Player::subContext.subsMutex);
	VM_Player::subContext.available = false;

	for (auto sub : VM_Player::subContext.subs) {
		VM_SubFontEngine::RemoveSubs(sub->id);
		DELETE_POINTER(sub);
	}
	VM_Player::subContext.subs.clear();

	VM_Player::subContext.available = true;
	SDL_CondSignal(VM_Player::subContext.subsCondition);
	SDL_UnlockMutex(VM_Player::subContext.subsMutex);

	return RESULT_OK;
}

int MediaPlayer::VM_Player::threadVideo(void* userData)
{
	int  errorCount = 0;
	auto frameRate  = VM_FileSystem::GetMediaFrameRate(VM_Player::videoContext.stream);

	if (frameRate <= 0) {
		VM_Player::Stop(VM_Text::Format("[MP-P-%d] %s '%s'", __LINE__, VM_Window::Labels["error.play"].c_str(), VM_Player::State.filePath.c_str()));
		return ERROR_UNKNOWN;
	}
	
	VM_Player::videoContext.frameDuration = (int)((1.0 / frameRate) * (double)ONE_SECOND_MS);

	// Wait until audio resources are ready
	while ((VM_Player::audioContext.frame->linesize[0] <= 0) && !VM_Player::State.quit)
		SDL_Delay(DELAY_TIME_DEFAULT);

	if (VM_Player::videoContext.frame == NULL) {
		VM_Player::Stop(VM_Text::Format("[MP-P-%d] %s '%s'", __LINE__, VM_Window::Labels["error.play"].c_str(), VM_Player::State.filePath.c_str()));
		return ERROR_UNKNOWN;
	}

	while (!VM_Player::State.quit)
	{
		while ((VM_Player::videoContext.packets.empty() || VM_Player::seekRequested) && !VM_Player::State.quit)
			SDL_Delay(DELAY_TIME_DEFAULT);

		if (VM_Player::State.quit)
			break;

		// Try to get a new video packet from the queue (re-use packet if paused)
		auto packet = VM_Player::packetGet(VM_Player::videoContext.packets, VM_Player::videoContext.mutex, VM_Player::videoContext.condition, VM_Player::videoContext.packetsAvailable);

		if ((packet == NULL) || VM_Player::State.quit)
			continue;

		// Try to decode the video packet to the video frame
		auto decodeResult = avcodec_decode_video2(VM_Player::videoContext.stream->codec, VM_Player::videoContext.frame, &VM_Player::videoContext.frameDecoded, packet);

		// Release the resources allocated for the packet
		FREE_PACKET(packet);

		if (VM_Player::State.quit)
			break;

		// Handle any errors decoding the packet
		if ((decodeResult < 0) || !VM_Player::videoContext.frameDecoded || !AVFRAME_VALID(VM_Player::videoContext.frame))
		{
			// Too many errors occured, exit the video thread.
			if (errorCount > MAX_ERRORS) {
				VM_Player::Stop(VM_Text::Format("[MP-P-%d] %s '%s'", __LINE__, VM_Window::Labels["error.play"].c_str(), VM_Player::State.filePath.c_str()));
				break;
			}
			
			errorCount++;
			VM_Player::videoContext.frameDecoded = 0;

			if (VM_Player::ProgressTime > 1.0) {
				av_frame_unref(VM_Player::videoContext.frame);
				VM_Player::videoContext.frame = LIB_FFMPEG::av_frame_alloc();
			}

			continue;
		}

		errorCount = 0;

		// Calculate video display time and any delay time
		VM_Player::videoContext.pts = (double)av_frame_get_best_effort_timestamp(VM_Player::videoContext.frame);

		if (AV_START_FLAGS(VM_Player::FormatContext->iformat))
			VM_Player::videoContext.pts -= (double)VM_Player::videoContext.stream->start_time;

		VM_Player::videoContext.pts *= LIB_FFMPEG::av_q2d(VM_Player::videoContext.stream->time_base);

		if (VM_Player::pausedVideoSeekRequested)
		{
			if (fabs(VM_Player::ProgressTime - VM_Player::videoContext.pts) < 1.0) {
				VM_Player::pausedVideoSeekRequested = false;
				VM_Player::videoFrameAvailable      = true;
			}
		}

		// Sleep while paused unless a seek is requested
		while (VM_Player::State.isPaused && !VM_Player::seekRequested && !VM_Player::pausedVideoSeekRequested && !VM_Player::State.quit)
			SDL_Delay(DELAY_TIME_DEFAULT);

		if (VM_Player::State.quit)
			break;

		if (VM_Player::pausedVideoSeekRequested)
			continue;

		// Video delay (audio is ahead of video)
		double delayTime = (VM_Player::ProgressTime - VM_Player::videoContext.pts);

		if ((delayTime > ((VM_Player::videoContext.frameDuration / (double)ONE_SECOND_MS) * 2.0)) &&
			(VM_Player::audioContext.frameDuration > 0) &&
			(delayTime > (VM_Player::audioContext.frameDuration * 2.0)))
		{
			VM_Player::videoContext.frameDecoded = 0;
			continue;
		}

		auto startWaitTime = SDL_GetTicks();

		// Wait until it's time to render the video frame
		while ((VM_Player::ProgressTime < VM_Player::videoContext.pts) && !VM_Player::seekRequested && !VM_Player::State.quit)
		{
			// Don't wait too long if the video display times are incorrect or updates infrequently
			if ((int)(SDL_GetTicks() - startWaitTime) > (VM_Player::videoContext.frameDuration * 2))
				break;

			SDL_Delay(DELAY_TIME_ONE_MS);
		}

		if (VM_Player::State.quit)
			break;

		// Notify the render thread to render the new video frame
		VM_Player::videoFrameAvailable = true;
	}

	return RESULT_OK;
}
