#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_PLAYER_H
#define VM_PLAYER_H

namespace VoyaMedia
{
	namespace MediaPlayer
	{
		// libavcodec/dvdsubdec.c
		struct VM_DVDSubContext
		{
			LIB_FFMPEG::AVClass* av_class;
			uint32_t             palette[16];
			char*                palette_str;
			char*                ifo_str;
			int                  has_palette;
			uint8_t              colormap[4];
			uint8_t              alpha[256];
			uint8_t              buf[0x10000];
			int                  buf_size;
			int                  forced_subs_only;
		};

		struct VM_PlayerAudioContext
		{
			int                   bufferOffset;
			int                   bufferRemaining;
			int                   bufferSize;
			SDL_cond*             condition;
			bool                  decodeFrame;
			bool                  decodeReUsePacket;
			SDL_AudioSpec         device;
			SDL_AudioDeviceID     deviceID;
			LIB_FFMPEG::AVFrame*  frame;
			double                frameDuration;
			uint8_t*              frameEncoded;
			int                   index;
			SDL_mutex*            mutex;
			LIB_FFMPEG::AVPacket* packet;
			VM_Packets            packets;
			bool                  packetsAvailable;
			LIB_FFMPEG::AVStream* stream;
			int                   volumeBeforeMute;
			int                   writtenToStream;

			VM_PlayerAudioContext()
			{
				this->reset();
			}

			void reset()
			{
				this->bufferOffset      = 0;
				this->bufferRemaining   = 0;
				this->bufferSize        = AUDIO_BUFFER_SIZE;
				this->condition         = NULL;
				this->decodeFrame       = true;
				this->decodeReUsePacket = false;
				this->device            = {};
				this->deviceID          = -1;
				this->frame             = NULL;
				this->frameDuration     = 0.0;
				this->frameEncoded      = NULL;
				this->index             = -1;
				this->mutex             = NULL;
				this->packet            = NULL;
				this->packetsAvailable  = true;
				this->stream            = NULL;
				this->writtenToStream   = 0;
			}
		};

		struct VM_PlayerSubContext
		{
			bool                       available;
			SDL_cond*                  condition;
			TTF_Font*                  fonts[NR_OF_FONTS];
			int                        index;
			SDL_mutex*                 mutex;
			VM_Packets                 packets;
			bool                       packetsAvailable;
			Graphics::VM_PointF        scale;
			SDL_Point                  size;
			LIB_FFMPEG::AVStream*      stream;
			std::vector<VM_SubStyle*>  styles;
			VM_Subtitles               subs;
			SDL_cond*                  subsCondition;
			SDL_mutex*                 subsMutex;
			Graphics::VM_Texture*      texture;
			SDL_Thread*                thread;
			LIB_FFMPEG::AVSubtitleType type;

			#if defined _windows
				umap<WString, TTF_Font*> styleFonts;
			#else
				umap<String, TTF_Font*> styleFonts;
			#endif

			VM_PlayerSubContext()
			{
				this->reset();
			}

			void reset()
			{
				this->available        = true;
				this->condition        = NULL;
				this->index            = -1;
				this->mutex            = NULL;
				this->packetsAvailable = true;
				this->scale            = { 1.0f, 1.0f };
				this->size             = {};
				this->subsCondition    = NULL;
				this->subsMutex        = NULL;
				this->stream           = NULL;
				this->texture          = NULL;
				this->thread           = NULL;
				this->type             = LIB_FFMPEG::SUBTITLE_NONE;
			}
		};

		struct VM_PlayerVideoContext
		{
			SDL_cond*             condition;
			LIB_FFMPEG::AVFrame*  frame;
			int                   frameDecoded;
			int                   frameDuration;
			LIB_FFMPEG::AVPicture frameEncoded;
			int                   index;
			SDL_mutex*            mutex;
			VM_Packets            packets;
			bool                  packetsAvailable;
			double                pts;
			SDL_Rect              renderLocation;
			LIB_FFMPEG::AVStream* stream;
			Graphics::VM_Texture* texture;
			SDL_Thread*           thread;

			VM_PlayerVideoContext()
			{
				this->reset();
			}

			void reset()
			{
				this->condition        = NULL;
				this->frame            = NULL;
				this->frameDecoded     = 0;
				this->frameDuration    = 0;
				this->mutex            = NULL;
				this->packetsAvailable = true;
				this->pts              = 0.0;
				this->renderLocation   = {};
				this->stream           = NULL;
				this->index            = -1;
				this->texture          = NULL;
				this->thread           = NULL;
			}
		};

		struct VM_PlayerState
		{
			int         audioVolume;
			String      errorMessage;
			String      filePath;
			WString     filePathW;
			bool        fullscreenEnter;
			bool        fullscreenExit;
			bool        isMuted;
			bool        isPaused;
			bool        isPlaying;
			bool        isStopped;
			bool        keepAspectRatio;
			VM_PlayType loopType;
			bool        openFile;
			bool        quit;
			Strings     urls;

			VM_PlayerState()
			{
				this->isStopped = true;
				this->quit      = false;

				this->Reset();
			}

			void Reset()
			{
				this->openFile        = false;
				this->filePath        = "";
				this->filePathW       = L"";
				this->fullscreenEnter = false;
				this->fullscreenExit  = false;
				this->isPlaying       = false;
				this->isPaused        = false;

				this->urls.clear();
			}
		};

		class VM_Player
		{
		private:
			VM_Player()  {}
			~VM_Player() {}

		public:
			static uint32_t                     CursorLastVisible;
			static double                       DurationTime;
			static LIB_FFMPEG::AVFormatContext* FormatContext;
			static time_t                       PauseTime;
			static time_t                       PlayTime;
			static double                       ProgressTime;
			static Graphics::VM_SelectedRow     SelectedRow;
			static VM_PlayerState               State;
			static Strings                      SubsExternal;
			static System::VM_TimeOut*          TimeOut;

		private:
			static VM_PlayerAudioContext        audioContext;
			static LIB_FFMPEG::AVFormatContext* formatContextExternal;
			static bool                         fullscreenEnterWindowed;
			static bool                         fullscreenExitStop;
			static bool                         isCursorHidden;
			static bool                         isStopping;
			static bool                         mediaCompleted;
			static int                          nrOfStreams;
			static SDL_Thread*                  packetsThread;
			static uint32_t                     pictureProgressTime;
			static double                       progressTimeLast;
			static bool                         refreshSub;
			static LIB_FFMPEG::SwrContext*      resampleContext;
			static LIB_FFMPEG::SwsContext*      scaleContextSub;
			static LIB_FFMPEG::SwsContext*      scaleContextVideo;
			static bool                         seekRequested;
			static bool                         pausedVideoSeekRequested;
			static int64_t                      seekToPosition;
			static VM_PlayerSubContext          subContext;
			static VM_PlayerVideoContext        videoContext;
			static bool                         videoFrameAvailable;

		public:
			static int                        Close();
			static int                        CursorShow();
			static int                        FreeTextures();
			static int                        FullScreenEnter(bool windowMode);
			static int                        FullScreenEnter();
			static int                        FullScreenExit(bool stop);
			static int                        FullScreenExit();
			static int                        FullScreenToggle(bool windowMode);
			static LIB_FFMPEG::AVSampleFormat GetAudioSampleFormat(SDL_AudioFormat sdlFormat);
			static SDL_AudioFormat            GetAudioSampleFormat(LIB_FFMPEG::AVSampleFormat sampleFormat);
			static uint32_t                   GetVideoPixelFormat(LIB_FFMPEG::AVPixelFormat pixelFormat);
			static LIB_FFMPEG::AVPixelFormat  GetVideoPixelFormat(uint32_t pixelFormat);
			static int                        GetStreamIndex(VM_MediaType mediaType);
			static Graphics::VM_PointF        GetSubScale();
			static void                       Init();
			static bool                       IsYUV(LIB_FFMPEG::AVPixelFormat pixelFormat);
			static void                       KeepAspectRatioToggle();
			static int                        MuteToggle();
			static int                        Open();
			static int                        OpenFilePath(const String &filePath);
			static int                        OpenNext(bool stop);
			static int                        OpenPrevious(bool stop);
			static int                        Pause();
			static int                        Play();
			static int                        PlayPauseToggle();
			static int                        PlaylistLoopTypeToggle();
			static int                        Render(const SDL_Rect &location);
			static void                       Refresh();
			static int                        RotatePicture();
			static int                        Seek(int64_t seekTime);
			static int                        SetAudioVolume(int volume);
			static int                        SetStream(int streamIndex, bool seek = true);
			static int                        Stop(const String &errorMessage = "");

			#if defined _windows
				static int OpenFilePath(const WString &filePath);
			#endif

		private:
			static int                   closeAudio();
			static void                  closeFormatContext();
			static void                  closePackets();
			static int                   closeStream(VM_MediaType streamType);
			static int                   closeSub();
			static void                  closeThreads();
			static int                   closeVideo();
			static int                   cursorHide();
			static bool                  isPacketQueueFull(VM_MediaType streamType);
			static int                   openAudio();
			static SDL_AudioDeviceID     openAudioDevice(SDL_AudioSpec& wantedSpecs);
			static int                   openFormatContext();
			static int                   openPlaylist(bool stop, bool next);
			static int                   openStreams();
			static int                   openSub();
			static LIB_FFMPEG::AVStream* openSubExternal(int streamIndex);
			static int                   openThreads();
			static int                   openVideo();
			static int                   packetAdd(LIB_FFMPEG::AVPacket* packet, VM_Packets &packetQueue, SDL_mutex* mutex, SDL_cond* condition, bool &queueAvailable);
			static LIB_FFMPEG::AVPacket* packetGet(VM_Packets &packetQueue, SDL_mutex* mutex, SDL_cond* condition, bool &queueAvailable);
			static int                   packetsClear(VM_Packets &packetQueue, SDL_mutex* mutex, SDL_cond* condition, bool &queueAvailable);
			static int                   renderSub(const SDL_Rect &location);
			static void                  renderPicture();
			static void                  renderSubBitmap(const SDL_Rect &location);
			static void                  renderSubText(const SDL_Rect &location);
			static void                  renderVideo(const SDL_Rect &location);
			static int                   renderVideoCreateTexture();
			static int                   renderVideoScaleRenderLocation(const SDL_Rect &location);
			static int                   renderVideoUpdateTexture();
			static void                  reset();
			static void                  seek();
			static void                  threadAudio(void* userData, Uint8* outputStream, int outputStreamSize);
			static int                   threadPackets(void* userData);
			static int                   threadSub(void* userData);
			static int                   threadVideo(void* userData);

		};
	}
}

#endif
