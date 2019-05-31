#ifndef VM_GLOBAL_H
#define VM_GLOBAL_H

// Disable deprecation warnings
#if defined _windows
	#pragma warning(disable:4996)
#else
	_Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#endif

// C++ STL
#include <fstream> // ifstream
#include <list>
#include <unordered_map>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <regex>   // vector<x>, regex_*, match_default, smatch
#include <sstream> // stringstream, to_string(x)
#include <thread>

// C Libraries
extern "C"
{
	// C
	#include <ctime>   // time_t, time(x)
	#include <clocale> // setlocale(x)

	// SDL2
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_syswm.h>
	#include <SDL2/SDL_ttf.h>

	// CURL
	#include <curl/curl.h>

	// FREETYPE2
	#include <freetype/ttnameid.h>
}

// Platform-specific APIs
#if defined _android
	#include <dirent.h>								// mkdir(x), opendir(x)
	#include <fcntl.h>								// fcntl(x)
	#include <netdb.h>								// hostent, in_addr, gethostname(x), gethostbyname(x)
	#include <unistd.h>								// chdir(x)
	#include <android/asset_manager_jni.h>			// VM_FileSystem::GetAndroidAssets(x)
	#include <android/native_activity.h>			// VM_FileSystem::GetAndroidAssets(x)
	#include <sys/stat.h>							// stat64, lstat64(x), _stat64, _stat64(x)
#elif defined _ios
	#include <dirent.h>								// mkdir(x),  opendir(x)
	#include <netdb.h>								// hostent, in_addr, gethostname(x), gethostbyname(x)
	#include <AVFoundation/AVAssetExportSession.h>	// AVAssetExportSession*
	#include <AVFoundation/AVFoundation.h>			// AVAudioSession
	#include <Foundation/Foundation.h>				// NSString, NSArray, NSURL, NSUUID
	#include <MediaPlayer/Mediaplayer.h>			// MPMediaItem, MPMediaItemArtwork, MPMediaQuery
	#include <Photos/Photos.h>						// PHAsset, PHFetchResult, PHFetchOptions
	#include <sys/stat.h>							// stat64, lstat64(x), _stat64, _stat64(x)
	#include <os/log.h>								// os_log(x)
#elif defined _linux
	#include <dirent.h>								// opendir(x)
	#include <netdb.h>								// hostent, in_addr, gethostname(x), gethostbyname(x)
	#include <arpa/inet.h>							// inet_addr(x), inet_ntoa(x)
	#include <gtk/gtk.h>							// gtk_file_chooser_dialog_new(x), gtk_dialog_run(x), gtk_file_chooser_get_uri(x)
	#include <sys/fcntl.h>							// fcntl(x)
	#include <sys/socket.h>							// socket(x), bind(x), connect(x)
	#include <sys/stat.h>							// mkdir(x), stat64, lstat64(x), _stat64, _stat64(x)
	#include <sys/utsname.h>						// uname(x)
#elif defined _macosx
	#include <ifaddrs.h>
	#include <netdb.h>								// hostent, in_addr, gethostname(x), gethostbyname(x)
	#include <arpa/inet.h>							// inet_addr(x), inet_ntoa(x)
	#include <AppKit/AppKit.h>						// NSOpenPanel*
	#include <Foundation/Foundation.h>				// NSString, NSArray, NSURL
	#include <sys/dir.h>							// opendir(x)
	#include <sys/socket.h>							// socket(x), bind(x), connect(x)
	#include <sys/stat.h>							// mkdir(x), stat64, lstat64(x), _stat64, _stat64(x)
#elif defined _windows
	#include <Commdlg.h>							// GetOpenFileNameA(x)
	#include <direct.h>								// mkdir(x)
	#include <dirent.h>								// opendir(x)
	#include <Shellapi.h>							// ShellExecuteA(x)
	#include <Shlobj.h>								// SHBrowseForFolder(), SHGetPathFromIDListA
#endif

#if defined _ios || defined _macosx
	#include <SystemConfiguration/SCNetworkReachability.h>	// SCNetworkReachability
#endif

namespace LIB_FFMPEG
{
	extern "C"
	{
		#include <libavformat/avformat.h>
		#include <libavutil/opt.h>
		#include <libavutil/time.h>
		#include <libswresample/swresample.h>
		#include <libswscale/swscale.h>
	}
}

namespace LIB_FREEIMAGE
{
	#include <FreeImage.h>
}

namespace LIB_XML
{
	extern "C"
	{
		#include <libxml/xpath.h>
	}
}

namespace LIB_JSON
{
	#include <json.h>
}

namespace LIB_SQLITE
{
	extern "C"
	{
		#include <sqlite3.h>
	}
}

namespace LIB_UPNP
{
	extern "C"
	{
		#undef EVENT_H
		#include <upnp/upnptools.h>
		#include <upnp/upnp.h>
	}
}

namespace VoyaMedia
{
	#if defined _android
		#define fstat       lstat64
		#define stat_t      struct stat64
		#define fseek       fseeko
		#define LOG(x, ...) SDL_Log(x, ##__VA_ARGS__);
	#elif defined _ios
		#define fstat       lstat
		#define stat_t      struct stat
		#define fseek       fseeko
		#define LOG(x, ...) os_log_error(OS_LOG_DEFAULT, x, ##__VA_ARGS__);
	#elif defined _macosx
		#define fstat       lstat64
		#define stat_t      struct stat64
		#define fseek       fseeko
		#define LOG(x, ...) NSLog(@x, ##__VA_ARGS__);
	#elif defined _linux
		#define fstat       lstat64
		#define stat_t      struct stat64
		#define fopen       fopen64
		#define fseek       fseeko64
		#define LOG(x, ...) fprintf(stderr, x, ##__VA_ARGS__);
	#elif defined _windows
		#define chdir       _chdir
		#define dirent      _wdirent
		#define DIR         _WDIR
		#define closedir    _wclosedir
		#define opendir     _wopendir
		#define readdir     _wreaddir
		#define mkdir       _mkdir
		#define fstat       _stat64
		#define fstatw      _wstat64
		#define stat_t      struct _stat64
		#define fseek       _fseeki64
		#define snprintf    _snprintf
		#define LOG(x, ...) SDL_Log(x, ##__VA_ARGS__);
	#endif

	#ifndef max
		#define max(a, b) (((a) > (b)) ? (a) : (b))
	#endif

	#ifndef min
		#define min(a, b) (((a) < (b)) ? (a) : (b))
	#endif

	#define ALIGN_CENTERED(offset, totalSize, size) max(0, ((offset) + (max(0, ((totalSize) - (size))) / 2)))
	#define BIT_AT_POS(i, p)       (((i) >> p) & 1)
	#define CAP(x, l, h)           (min(max((x), (l)), (h)))
	#define CLOSE_CURL(c)          if (c != NULL) { curl_easy_reset(c); curl_easy_cleanup(c); c = NULL; }
	#define CLOSE_FILE(f)          if (f != NULL) { fclose(f); f = NULL; }
	#define CLOSE_FILE_RW(f)       if (f != NULL) { SDL_RWclose(f); f = NULL; }
	#define CLOSE_FONT(f)          if (f != NULL) { TTF_CloseFont(f); f = NULL; }
	#define DB_BUSY_OR_LOCKED(r)   ((r == SQLITE_BUSY) || (r == SQLITE_LOCKED))
	#define DB_RESULT_OK(r)        ((r == SQLITE_OK)   || (r == SQLITE_DONE))
	#define DB_RESULT_ERROR(r)     ((r != SQLITE_OK)   && (r != SQLITE_DONE))
	#define DEC_TO_HEX(d)          ((((int)((d) / 16) * 10) + ((d) % 16)))
	#define HEX_STR_TO_UINT(h)     (uint8_t)std::strtoul(String("0x" + h).c_str(), NULL, 16)
	#define DELETE_POINTER(p)      if (p != NULL) { delete p; p = NULL; }
	#define DELETE_POINTER_ARR(p)  if (p != NULL) { delete[] p; p = NULL; }
	#define FREE_AVFORMAT(f)       if (f != NULL) { LIB_FFMPEG::avformat_close_input(&f); LIB_FFMPEG::avformat_free_context(f); f = NULL; }
	#define FREE_AVPICTURE(p)      if (p.linesize[0] > 0) { LIB_FFMPEG::avpicture_free(&p); p.linesize[0] = 0; }
	#define FREE_AVPOINTER(p)      if (p != NULL) { LIB_FFMPEG::av_free(p); p = NULL; }
	#define FREE_DB(d)             if (d != NULL) { LIB_SQLITE::sqlite3_close(d);  d = NULL; }
	#define FREE_JSON_DOC(d)       if (d != NULL) { LIB_JSON::json_free_value(&d); d = NULL; }
	#define FREE_IMAGE(i)          if (i != NULL) { LIB_FREEIMAGE::FreeImage_Unload(i); i = NULL; }
	#define FREE_FRAME(f, p)       if (f != NULL) { if (p) { LIB_FFMPEG::avpicture_free(reinterpret_cast<LIB_FFMPEG::AVPicture*>(f)); } LIB_FFMPEG::av_frame_free(&f); f = NULL; }
	#define FREE_MEMORY(m)         if (m != NULL) { LIB_FREEIMAGE::FreeImage_CloseMemory(m); m = NULL; }
	#define FREE_PACKET(p)         if (p != NULL) { LIB_FFMPEG::av_free_packet(p); LIB_FFMPEG::av_free(p); p = NULL; }
	#define FREE_POINTER(p)        if (p != NULL) { free(p); p = NULL; }
	#define FREE_RENDERER(r)       if (r != NULL) { SDL_DestroyRenderer(r); r = NULL; }
	#define FREE_STREAM(s)         if (s != NULL) { s->discard = LIB_FFMPEG::AVDISCARD_ALL; if (s->codec != NULL) { LIB_FFMPEG::avcodec_close(s->codec); } s = NULL; }
	#define FREE_SWR(s)            if (s != NULL) { LIB_FFMPEG::swr_free(&s); s = NULL; }
	#define FREE_SWS(s)            if (s != NULL) { LIB_FFMPEG::sws_freeContext(s); s = NULL; }
	#define FREE_SUB_FRAME(f)      if (f.num_rects > 0) { LIB_FFMPEG::avsubtitle_free(&f); f = { 0, 0, 0, 0 }; }
	#define FREE_SUB_RECT(r)       if (r != NULL) { for (uint32_t i = 0; i < 4; i++) { if (r->pict.data[i] != NULL) { LIB_FFMPEG::av_freep(&r->pict.data[i]); r->pict.data[i] = NULL; }}} r = NULL;
	#define FREE_SUB_TEXT(t)       if (t != NULL) { LIB_FFMPEG::av_freep(&t); }
	#define FREE_SURFACE(s)        if (s != NULL) { SDL_FreeSurface(s);    s = NULL; }
	#define FREE_TEXTURE(t)        if (t != NULL) { SDL_DestroyTexture(t); t = NULL; }
	#define FREE_THREAD(t)         if (t != NULL) { SDL_DetachThread(t);   t = NULL; }
	#define FREE_THREAD_COND(c)    if (c != NULL) { SDL_DestroyCond(c);    c = NULL; }
	#define FREE_THREAD_MUTEX(m)   if (m != NULL) { SDL_DestroyMutex(m);   m = NULL; }
	#define FREE_WINDOW(w)         if (w != NULL) { SDL_DestroyWindow(w);  w = NULL; }
	#define FREE_XML_DOC(d)        if (d != NULL) { LIB_XML::xmlFreeDoc(d);    d = NULL; }
	#define MATH_ROUND_UP(a)       (int)(a + 0.5)
	#define AUDIO_IS_SELECTED      (VM_Top::Selected == MEDIA_TYPE_AUDIO)
	#define PICTURE_IS_SELECTED    (VM_Top::Selected == MEDIA_TYPE_PICTURE)
	#define VIDEO_IS_SELECTED      (VM_Top::Selected == MEDIA_TYPE_VIDEO)
	#define YOUTUBE_IS_SELECTED    (VM_Top::Selected == MEDIA_TYPE_YOUTUBE)
	#define SHOUTCAST_IS_SELECTED  (VM_Top::Selected == MEDIA_TYPE_SHOUTCAST)
	#define TMDB_MOVIE_IS_SELECTED (VM_Top::Selected == MEDIA_TYPE_TMDB_MOVIE)
	#define TMDB_TV_IS_SELECTED    (VM_Top::Selected == MEDIA_TYPE_TMDB_TV)
	#define SEEK_FLAGS(i)          (((i->flags & AVFMT_TS_DISCONT) || (i->read_seek == NULL)) ? AVSEEK_FLAG_BYTE : 0)

	#if defined _windows
		#define OPEN_FONT(f, s) TTF_OpenFontRW(VM_FileSystem::FileOpenSDLRWops(_wfopen(f, L"rb")), 1, s)
		#define WRITE_FILE(f)   std::wofstream os(f, std::wofstream::out); os.close();
	#else
		#define OPEN_FONT(f, s) TTF_OpenFont(f, s)
		#define WRITE_FILE(f)   std::ofstream os(f, std::ofstream::out); os.close();
	#endif
	
	enum VM_DatabaseType
	{
		DATABASE_MEDIALIBRARYv3, DATABASE_PLAYLISTSv3, DATABASE_SETTINGSv3, DATABASE_TMDBv3
	};

	enum VM_ErrorType
	{
		ERROR_UNKNOWN = -1, RESULT_OK, RESULT_REFRESH_PENDING, ERROR_INVALID_ARGUMENTS
	};

	enum VM_FontType
	{
		FONT_MERGED, FONT_CJK, NR_OF_FONTS
	};

	enum VM_MediaType
	{
		MEDIA_TYPE_UNKNOWN = -1,
		MEDIA_TYPE_VIDEO,
		MEDIA_TYPE_AUDIO,
		MEDIA_TYPE_DATA,
		MEDIA_TYPE_SUBTITLE,
		MEDIA_TYPE_ATTACHMENT,
		MEDIA_TYPE_NB,
		MEDIA_TYPE_PICTURE,
		MEDIA_TYPE_YOUTUBE,
		MEDIA_TYPE_SHOUTCAST,
		MEDIA_TYPE_TMDB_MOVIE,
		MEDIA_TYPE_TMDB_TV,
		NR_OF_MEDIA_TYPES
	};

	enum VM_PlayType
	{
		PLAY_TYPE_NORMAL, PLAY_TYPE_LOOP, PLAY_TYPE_SHUFFLE
	};

	enum VM_SubAlignment
	{
		SUB_ALIGN_UNKNOWN,
		SUB_ALIGN_BOTTOM_LEFT, SUB_ALIGN_BOTTOM_CENTER, SUB_ALIGN_BOTTOM_RIGHT,
		SUB_ALIGN_MIDDLE_LEFT, SUB_ALIGN_MIDDLE_CENTER, SUB_ALIGN_MIDDLE_RIGHT,
		SUB_ALIGN_TOP_LEFT,    SUB_ALIGN_TOP_CENTER,    SUB_ALIGN_TOP_RIGHT
	};

	enum VM_SubDialogueProperty
	{
		SUB_DIALOGUE_LAYER,
		SUB_DIALOGUE_START, SUB_DIALOGUE_END,
		SUB_DIALOGUE_STYLE,
		SUB_DIALOGUE_NAME,
		SUB_DIALOGUE_MARGINL, SUB_DIALOGUE_MARGINR, SUB_DIALOGUE_MARGINV,
		SUB_DIALOGUE_EFFECTS,
		SUB_DIALOGUE_TEXT,
		NR_OF_SUB_DIALOGUE_PROPERTIES
	};

	// Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour,
	// Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow,
	// Alignment, MarginL, MarginR, MarginV, Encoding
	enum VM_SubStyleV4Plus
	{
		SUB_STYLE_V4PLUS_FONT_UNDERLINE = 9,
		SUB_STYLE_V4PLUS_FONT_STRIKEOUT,
		SUB_STYLE_V4PLUS_FONT_SCALE_X,
		SUB_STYLE_V4PLUS_FONT_SCALE_Y,
		SUB_STYLE_V4PLUS_FONT_LETTER_SPACING,
		SUB_STYLE_V4PLUS_FONT_ROTATION_ANGLE,
		SUB_STYLE_V4PLUS_FONT_BORDER_STYLE,
		SUB_STYLE_V4PLUS_FONT_OUTLINE,
		SUB_STYLE_V4PLUS_FONT_SHADOW,
		SUB_STYLE_V4PLUS_FONT_ALIGNMENT,
		SUB_STYLE_V4PLUS_FONT_MARGINL,
		SUB_STYLE_V4PLUS_FONT_MARGINR,
		SUB_STYLE_V4PLUS_FONT_MARGINV,
		SUB_STYLE_V4PLUS_FONT_ENCODING,
		NR_OF_V4PLUS_SUB_STYLES
	};

	// Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour, BackColour,
	// Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV,
	// AlphaLevel, Encoding
	enum VM_SubStyleV4
	{
		SUB_STYLE_V4_NAME,
		SUB_STYLE_V4_FONT_NAME,
		SUB_STYLE_V4_FONT_SIZE,
		SUB_STYLE_V4_COLOR_PRIMARY,
		SUB_STYLE_V4_COLOR_SECONDARY,
		SUB_STYLE_V4_COLOR_BORDER,
		SUB_STYLE_V4_COLOR_SHADOW,
		SUB_STYLE_V4_FONT_BOLD,
		SUB_STYLE_V4_FONT_ITALIC,
		SUB_STYLE_V4_FONT_BORDER_STYLE,
		SUB_STYLE_V4_FONT_OUTLINE,
		SUB_STYLE_V4_FONT_SHADOW,
		SUB_STYLE_V4_FONT_ALIGNMENT,
		SUB_STYLE_V4_FONT_MARGINL,
		SUB_STYLE_V4_FONT_MARGINR,
		SUB_STYLE_V4_FONT_MARGINV,
		SUB_STYLE_V4_FONT_ALPHA_LEVEL,
		SUB_STYLE_V4_FONT_ENCODING,
		NR_OF_V4_SUB_STYLES
	};

	enum VM_SubStyleVersion
	{
		SUB_STYLE_VERSION_UNKNOWN = -1, SUB_STYLE_VERSION_4_SSA, SUB_STYLE_VERSION_4PLUS_ASS
	};

	enum VM_TouchEventType
	{
		TOUCH_EVENT_UNKNOWN = -1,
		TOUCH_EVENT_DOUBLE_TAP, TOUCH_EVENT_LONG_PRESS, TOUCH_EVENT_NORMAL, 
		TOUCH_EVENT_SWIPE_DOWN, TOUCH_EVENT_SWIPE_LEFT, TOUCH_EVENT_SWIPE_RIGHT, TOUCH_EVENT_SWIPE_UP
	};

	enum VM_UrlType
	{
		URL_DROPBOX_AUTH_CODE, URL_DROPBOX_OAUTH2_TOKEN, URL_DROPBOX_OAUTH2_DATA,
		URL_DROPBOX_TOKEN_EXPIRED, URL_DROPBOX_FILES
	};

	enum VM_Version
	{
		VM_VERSION_0, VM_VERSION_1, VM_VERSION_2, VM_VERSION_3
	};

	template<typename K, typename V>
	using umap = std::unordered_map<K, V>;

	template<typename K, typename V, typename H>
	using umapEH = std::unordered_map<K, V, H>;

	struct VM_EnumHash
	{
		template <typename T>
		std::size_t operator()(T t) const { return static_cast<std::size_t>(t); }
	};

	typedef unsigned long         ulong;
	typedef std::vector<int>      ints;
	typedef std::string           String;
	typedef std::wstring          WString;
	typedef std::vector<String>   Strings;
	typedef std::vector<WString>  WStrings;
	typedef umap<String, String>  StringMap;
	typedef umap<String, WString> WStringMap;
	typedef umap<int, String>     IntStringMap;
	
	const String APP_COMPANY          = "Adam A. Jammary (Jammary Consulting)";
	const String APP_COPYRIGHT        = ("(c) 2012 " + APP_COMPANY + ". All rights reserved.");
	const String APP_DESCRIPTION      = "A cross-platform media player that easily plays all your music, pictures and videos.";
	const String APP_NAME             = "Voya Media";
	const String APP_PRIVACY          = (APP_COMPANY + " does not collect any information stored on the local system.");
	const String APP_THIRD_PARTY_LIBS = "FFmpeg (LGPL v.2.1), FreeImage (FIPL), Freetype2 (FTL), libcurl (MIT), libXML2 (MIT), libupnp (BSD), mJSON (LGPL), OpenSSL, SDL2 (zlib), SQLite, zLib, Dropbox, Google Maps API, Google Noto Fonts (OFL), SHOUTcast Radio Directory API, TMDb API, YouTube Data API";
	const String APP_UPDATE_V3_MSG    = ("Welcome to " + APP_NAME + " 3\n\nWhat's new:\n- Completely refactored the UI-rendering engine\n- UI is now DPI-aware and independent of screen sizes and resolutions\n- Completely restructured the database (*)\n\n(*) Settings have been reset\n(*) Media library is empty (files must to be re-added)!");
	const String APP_URL              = "http://www.voyamedia.com";
	const String APP_VERSION          = "__APP_VERSION__";
	const String APP_ABOUT_TEXT       = (APP_DESCRIPTION + "\n\n" + APP_PRIVACY + "\n\n" + APP_THIRD_PARTY_LIBS + "\n\n" + APP_COPYRIGHT);
	const String DROPBOX_API_KEY      = "__DROPBOX_API_KEY__";
	const String DROPBOX_API_SECRET   = "__DROPBOX_API_SECRET__";
	const String GOOGLE_MAPS_API_KEY  = "__GOOGLE_MAPS_API_KEY__";
	const String GOOGLE_MAPS_API_URL  = "https://maps.googleapis.com/maps/api/geocode/json?latlng=";
	const String SHOUTCAST_API_KEY    = "__SHOUTCAST_API_KEY__";
	const String SHOUTCAST_API_URL    = "https://api.shoutcast.com/legacy/";
	const String TMDB_API_KEY         = "__TMDB_API_KEY__";
	const String TMDB_API_URL         = "https://api.themoviedb.org/3/";
	const String YOUTUBE_API_KEY      = "__YOUTUBE_API_KEY__";
	const String YOUTUBE_API_URL      = "https://www.googleapis.com/youtube/v3/";

	const int    AUDIO_BUFFER_SIZE      = 768000;
	const char   GOOGLE_IP[]            = "216.58.211.132";
	const int    HTTP_RESPONSE_OK       = 200;
	const double INVALID_COORDINATE     = 1000.0;
	const double PICTURE_SLIDESHOW_TIME = 10.0;
	const char   REFRESH_PENDING[]      = "REFRESH_PENDING";
	const int    SCREENSAVER_TIMEOUT    = 300000; // 5 minutes
	const double SUB_MAX_DURATION       = 20.0;   // 20 seconds
	const int    SUB_STREAM_EXTERNAL    = 100;

	const int     AV_TIME_BASE_I32 = 1000000;
	const int64_t AV_TIME_BASE_I64 = 1000000ll;
	const double  AV_TIME_BASE_D   = 1000000.0;
	const float   AV_TIME_BASE_F   = 1000000.0f;
	#define       AV_TIME_BASE_R   { 1, AV_TIME_BASE_I32 }

	const SDL_Color SDL_COLOR_BLACK = { 0, 0, 0, 0xFF };
	const SDL_Color SDL_COLOR_WHITE = { 0xFF, 0xFF, 0xFF, 0xFF };

	#if defined ANDROID || defined _ios
		const int CURSOR_HIDE_DELAY = 5000;
	#elif defined _linux || defined _macosx || defined _windows
		const int CURSOR_HIDE_DELAY = 2000;
	#endif

	const int          DEFAULT_MARGIN            = 30;
	const int          DEFAULT_CHAR_BUFFER_SIZE  = 1024;
	const int          DEFAULT_WCHAR_BUFFER_SIZE = 10240;
	const double       DEFAULT_FONT_DPI_RATIO    = 0.75; // (72.0 / 96.0)
	const int          DEFAULT_FONT_SIZE         = 11;
	const int          DEFAULT_FONT_SIZE_SUB     = 48;
	const VM_PlayType  DEFAULT_LOOP_TYPE         = PLAY_TYPE_NORMAL;
	const VM_MediaType DEFAULT_MEDIA_TYPE        = MEDIA_TYPE_AUDIO;
	const int          DEFAULT_SCALE_FILTER      = SWS_LANCZOS;

	#if defined _android
		const float DEFAULT_DPI = 160.0f;
	#elif defined _ios
		const float DEFAULT_DPI = 163.0f;
	#elif defined _macosx
		const float DEFAULT_DPI = 72.0f;
	#else
		const float DEFAULT_DPI = 96.0f;
	#endif

	const int DELAY_TIME_BACKGROUND = 200;
	const int DELAY_TIME_DEFAULT    = 15;
	const int DELAY_TIME_GUI_RENDER = 300;

	const float FLOAT_MAX_ONE  = 1.01f;
	const float FLOAT_MIN_ONE  = 0.99f;
	const float FLOAT_MIN_ZERO = 0.01f;

	#define FIRST_BYTE  (1 + 0x0)
	#define SECOND_BYTE (1 + 0xFF)
	#define THIRD_BYTE  (1 + 0xFFFF)
	#define FOURTH_BYTE (1 + 0xFFFFFF)

	const int     KILO_BYTE  = 1024;
	const int     MEGA_BYTE  = 1024000;
	const int     GIGA_BYTE  = 1024000000;
	const int64_t TERRA_BYTE = 1024000000000ll;

	const int     KILO  = 1000;
	const int     MEGA  = 1000000;
	const int     GIGA  = 1000000000;
	const int64_t TERRA = 1000000000000ll;

	#ifndef INT64_MIN
		#define INT64_MIN (-9223372036854775807 - 1)
	#endif

	#ifndef INT64_MAX
		#define INT64_MAX 9223372036854775807
	#endif

	const int  MAX_AUDIO_FRAMES      = 10;
	const long MAX_CURL_REDIRS       = 10L;
	const long MAX_CURL_TIMEOUT      = 20000L;
	const int  MAX_DB_RESULT         = 100000;
	const int  MAX_DECODE_THREADS    = 16;
	const int  MAX_ERRORS            = 100;
	const int  MAX_FILE_PATH         = 260;
	const int  MAX_PACKET_QUEUE_SIZE = 100;
	const int  MAX_WINDOW_SIZE       = 16384;
	const int  MAX_THUMB_SIZE        = 512;

	const int MIN_OUTLINE           = 3;
	const int MIN_PACKET_QUEUE_SIZE = 25;
	const int MIN_WINDOW_SIZE       = 360;

	const int ONE_THOUSAND   = 1000;
	const int ONE_MILLION    = 1000000;
	const int ONE_HOUR_S     = 3600;
	const int ONE_MINUTE_S   = 60;
	const int ONE_HOUR_MS    = 3600000;
	const int ONE_MINUTE_MS  = 60000;
	const int ONE_SECOND_MS  = ONE_THOUSAND;
	const int ONE_SECOND_uS  = ONE_MILLION;
	const int TEN_SECONDS_MS = 10000;

	#if defined _windows
		const char    PATH_SEPERATOR[]   = "\\";
		const char    PATH_SEPERATOR_C   = '\\';
		const wchar_t PATH_SEPERATOR_W[] = L"\\";
	#else
		const char PATH_SEPERATOR[] = "/";
		const char PATH_SEPERATOR_C = '/';
	#endif

	const char THREAD_CLEAN_DB[]      = "clean_db";
	const char THREAD_CLEAN_THUMBS[]  = "clean_thumbs";
	const char THREAD_CREATE_IMAGES[] = "create_images";
	const char THREAD_GET_DATA[]      = "get_data";
	const char THREAD_SCAN_ANDROID[]  = "scan_android";
	const char THREAD_SCAN_DROPBOX[]  = "scan_dropbox";
	const char THREAD_SCAN_FILES[]    = "scan_files";
	const char THREAD_SCAN_ITUNES[]   = "scan_itunes";
	const char THREAD_UPNP_CLIENT[]   = "upnp_client";
	const char THREAD_UPNP_SERVER[]   = "upnp_server";

	const int  UPNP_ALIVE_TIME       = 1800; // seconds (30 min)
	const int  UPNP_DELAY_TIME       = 5000; // ms (5s)
	const int  UPNP_SEARCH_TIMEOUT   = 5;    // s  (5s)
	const int  UPNP_SEARCH_MAX_COUNT = 4;
	const char UPNP_SERVICE_ID[]     = "uurn:upnp-org:serviceId:ContentDirectory";
	const char UPNP_SERVICE_TYPE[]   = "urn:schemas-upnp-org:service:ContentDirectory:1";

	namespace Database
	{
		typedef StringMap             VM_DBRow;
		typedef std::vector<VM_DBRow> VM_DBResult;
	}

	namespace Graphics
	{
		class  VM_Button;
		class  VM_Component;
		class  VM_Image;
		struct VM_TableState;
		class  VM_Texture;

		struct VM_PointF
		{
			float x, y;
			VM_PointF()                 { this->x = 0; this->y = 0; }
			VM_PointF(float x, float y) { this->x = x; this->y = y; }
		};

		typedef std::vector<VM_Button*>                          VM_Buttons;
		typedef umapEH<VM_MediaType, StringMap, VM_EnumHash>     VM_CacheResponses;
		typedef std::vector<VM_Texture*>                         VM_Textures;
		typedef std::vector<VM_Component*>                       VM_Components;
		typedef umap<String, VM_Component*>                      VM_ComponentMap;
		typedef umapEH<VM_MediaType, VM_TableState, VM_EnumHash> VM_TableStates;

		#if defined _windows
			typedef umap<WString, VM_Image*> VM_ImageMap;
			typedef umap<String,  WString>   VM_ImageButtonMap;
		#else
			typedef umap<String, VM_Image*> VM_ImageMap;
			typedef umap<String, String>    VM_ImageButtonMap;
		#endif
	}

	namespace MediaPlayer
	{
		class VM_Subtitle;
		class VM_SubStyle;
		class VM_SubTexture;

		typedef std::queue<LIB_FFMPEG::AVPacket*> VM_Packets;
		typedef std::list<VM_Subtitle*>           VM_Subtitles;
		typedef std::vector<VM_SubStyle*>         VM_SubStyles;
		typedef std::vector<VM_SubTexture*>       VM_SubTextures;
		typedef umap<size_t, VM_SubTextures>      VM_SubTexturesId;
	}

	namespace System
	{
		class VM_Thread;

		typedef umap<String, VM_Thread*> VM_ThreadMap;
	}

	namespace UPNP
	{
		class VM_UpnpFile;

		typedef std::vector<VM_UpnpFile*> VM_UpnpFiles;
	}

	namespace XML
	{
		typedef std::vector<LIB_XML::xmlNode*> VM_XmlNodes;
	}
}

using namespace VoyaMedia;

#if defined _android
	#ifndef VM_JAVAJNI_H
		#include "../Android/VM_JavaJNI.h"
	#endif
#endif

#ifndef VM_DATABASE_H
	#include "../Database/VM_Database.h"
#endif
#ifndef VM_JSON_H
	#include "../JSON/VM_JSON.h"
#endif
#ifndef VM_XML_H
	#include "../XML/VM_XML.h"
#endif
#ifndef VM_BORDER_H
	#include "../Graphics/VM_Border.h"
#endif
#ifndef VM_COLOR_H
	#include "../Graphics/VM_Color.h"
#endif
#ifndef VM_COMPONENT_H
	#include "../Graphics/VM_Component.h"
#endif
#ifndef VM_BUTTON_H
	#include "../Graphics/VM_Button.h"
#endif
#ifndef VM_BYTES_H
	#include "../System/VM_Bytes.h"
#endif
#ifndef VM_DISPLAY_H
	#include "../Graphics/VM_Display.h"
#endif
#ifndef VM_MEDIATIME_H
	#include "../MediaPlayer/VM_MediaTime.h"
#endif
#ifndef VM_PANEL_H
	#include "../Graphics/VM_Panel.h"
#endif
#ifndef VM_SUBSTYLE_H
	#include "../MediaPlayer/VM_SubStyle.h"
#endif
#ifndef VM_SUBRECT_H
	#include "../MediaPlayer/VM_SubRect.h"
#endif
#ifndef VM_SUBTITLE_H
	#include "../MediaPlayer/VM_Subtitle.h"
#endif
#ifndef VM_TABLE_H
	#include "../Graphics/VM_Table.h"
#endif
#ifndef VM_SUBTEXTURE_H
	#include "../MediaPlayer/VM_SubTexture.h"
#endif
#ifndef VM_THREAD_H
	#include "../System/VM_Thread.h"
#endif
#ifndef VM_GUI_H
	#include "../Graphics/VM_GUI.h"
#endif
#ifndef VM_WINDOW_H
	#include "../System/VM_Window.h"
#endif
#ifndef VM_EVENTMANAGER_H
	#include "../System/VM_EventManager.h"
#endif
#ifndef VM_FILESYSTEM_H
	#include "../System/VM_FileSystem.h"
#endif
#ifndef VM_IMAGE_H
	#include "../Graphics/VM_Image.h"
#endif
#ifndef VM_GRAPHICS_H
	#include "../Graphics/VM_Graphics.h"
#endif
#ifndef VM_SURFACE_H
	#include "../Graphics/VM_Surface.h"
#endif
#ifndef VM_MODAL_H
	#include "../Graphics/VM_Modal.h"
#endif
#ifndef VM_TIMEOUT_H
	#include "../System/VM_TimeOut.h"
#endif
#ifndef VM_PLAYER_H
	#include "../MediaPlayer/VM_Player.h"
#endif
#ifndef VM_PLAYERCONTROLS_H
	#include "../MediaPlayer/VM_PlayerControls.h"
#endif
#ifndef VM_SUBFONTENGINE_H
	#include "../MediaPlayer/VM_SubFontEngine.h"
#endif
#ifndef VM_TEXT_H
	#include "../System/VM_Text.h"
#endif
#ifndef VM_TEXTINPUT_H
	#include "../Graphics/VM_TextInput.h"
#endif
#ifndef VM_TEXTURE_H
	#include "../Graphics/VM_Texture.h"
#endif
#ifndef VM_THREADMANAGER_H
	#include "../System/VM_ThreadManager.h"
#endif
#ifndef VM_TOP_H
	#include "../System/VM_Top.h"
#endif
#ifndef VM_UPNPFILE_H
	#include "../UPNP/VM_UpnpFile.h"
#endif
#ifndef VM_UPNP_H
	#include "../UPNP/VM_UPNP.h"
#endif

#endif
