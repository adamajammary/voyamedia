#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_FILESYSTEM_H
#define VM_FILESYSTEM_H

#if defined _android || defined _linux
	#include <ifaddrs.h> // ifaddrs, getifaddrs(x)
	#include <netdb.h>   // addrinfo, gethostname(x)
#endif

namespace VoyaMedia
{
	namespace System
	{
		class VM_FileSystem
		{
		private:
			VM_FileSystem()  {}
			~VM_FileSystem() {}

		public:
			static StringMap          MediaFileDLNAs;
			static std::queue<String> MediaFiles;

		private:
			static Strings   mediaFileExtensions;
			static StringMap mediaFileMimeTypes;
			static Strings   subFileExtensions;
			static Strings   systemFileExtensions;
			static bool      updateMeta;

		public:
			static int                          AddMediaFilesRecursively(const String &directoryPath);
			static int                          CleanDB(void* userData);
			static int                          CleanThumbs(void* userData);
			static void                         CloseMediaFormatContext(LIB_FFMPEG::AVFormatContext* formatContext, int mediaID);
			static int                          CreateDefaultDirectoryStructure();
			static VM_Bytes*                    DownloadToBytes(const String &mediaURL);
			static FILE*                        DownloadToFile(const String &mediaURL, const String &filePath);
			static String                       DownloadToString(const String &mediaURL);
			static bool                         FileExists(const String &filePath, const WString &filePathW);
			static SDL_RWops*                   FileOpenSDLRWops(FILE* file);
			static Strings                      GetDirectoryFiles(const String &directoryPath, bool checkSystemFiles = true);
			static String                       GetDropboxURL(const String &path);
			static String                       GetDropboxURL2(const String &mediaURL);
			static String                       GetFileDLNA(const String &filePath);
			static String                       GetFileExtension(const String &filePath, bool upperCase);
			static String                       GetFileMIME(const String &filePath);
			static String                       GetFileName(const String &filePath, bool removeExtension);
			static size_t                       GetFileSize(const String &filePath);
			static int64_t                      GetMediaDuration(LIB_FFMPEG::AVFormatContext* formatContext, LIB_FFMPEG::AVStream* audioStream);
			static LIB_FFMPEG::AVFormatContext* GetMediaFormatContext(const String &filePath, bool parseStreams);
			static double                       GetMediaFrameRate(LIB_FFMPEG::AVStream* stream);
			static LIB_FFMPEG::AVStream*        GetMediaStreamBest(LIB_FFMPEG::AVFormatContext* formatContext, VM_MediaType mediaType);
			static LIB_FFMPEG::AVStream*        GetMediaStreamByIndex(LIB_FFMPEG::AVFormatContext* formatContext, int streamIndex);
			static int                          GetMediaStreamCount(LIB_FFMPEG::AVFormatContext* formatContext, VM_MediaType mediaType);
			static int                          GetMediaStreamCountSubs();
			static String                       GetMediaStreamDescriptionAudio(LIB_FFMPEG::AVStream* stream);
			static String                       GetMediaStreamDescriptionSub(LIB_FFMPEG::AVStream* stream, bool external = false);
			static String                       GetMediaStreamDescriptionVideo(LIB_FFMPEG::AVStream* stream);
			static LIB_FFMPEG::AVStream*        GetMediaStreamFirst(LIB_FFMPEG::AVFormatContext* formatContext, VM_MediaType mediaType, bool open = true);
			static std::vector<int>             GetMediaStreamIndices(LIB_FFMPEG::AVFormatContext* formatContext, VM_MediaType mediaType);
			static StringMap                    GetMediaStreamMeta(LIB_FFMPEG::AVFormatContext* formatContext);
			static Strings                      GetMediaSubFiles(const String &videoFilePath);
			static VM_MediaType                 GetMediaType(LIB_FFMPEG::AVFormatContext* formatContext);
			static Strings                      GetNetworkInterfaces();
			static String                       GetPathDatabase();
			static String                       GetPathFont();
			static String                       GetPathFontArial();
			static String                       GetPathGUI();
			static String                       GetPathFromArgumentList(char* argv[], const int argc);
			static String                       GetPathImages();
			static String                       GetPathLanguages();
			static String                       GetPathThumbnailsDir();
			static String                       GetPathThumbnails(int mediaID);
			static String                       GetPathThumbnails(const String &fileName);
			static StringMap                    GetShoutCastDetails(const String &stationName, int stationID);
			static String                       GetShoutCastStation(int stationID);
			static StringMap                    GetTmdbDetails(int mediaID, VM_MediaType mediaType);
			static String                       GetURL(VM_UrlType urlType, const String &data = "");
			//static StringMap                    GetYouTubeDetails(const String &mediaID);
			//static Strings                      GetYouTubeVideos(const String &mediaURL);
			//static String                       GetYouTubeVideo(const String &videoID);
			static bool                         HasInternetConnection();
			static void                         InitFFMPEG();
			static int                          InitLibraries();
			static bool                         IsBlurayAACS(const String &filePath, size_t fileSize);
			static bool                         IsConcat(const String &filePath);
			static bool                         IsDRM(LIB_FFMPEG::AVDictionary* metaData);
			static bool                         IsDVDCSS(const String &filePath, size_t fileSize);
			static bool                         IsHttp(const String &filePath);
			static bool                         IsITunes(const String &filePath);
			static bool                         IsM4A(LIB_FFMPEG::AVFormatContext* formatContext);
			static bool                         IsMediaFile(const String &filePath);
			static bool                         IsPicture(const String &filePath);
			static bool                         IsSambaServer(const String &filePath);
			static bool                         IsServerAccessible(const String &serverAddress, const String &localAddress);
			static void                         OpenWebBrowser(const String &mediaURL);
			static int                          OpenWebBrowserT(const String &mediaURL);
			static String                       PostData(const String &mediaURL, const String &data, const Strings &headers);
			static void                         RefreshMetaData();
			static void                         SaveDropboxTokenOAuth2(const String &userCode);
			static int                          ScanDropboxFiles(void* userData);
			static int                          ScanMediaFiles(void* userData);
			static int                          SetWorkingDirectory();
		
			#if defined _android
				static String    GetAndroidStoragePath();
				static int       ScanAndroid(void* userData);
			#elif defined _ios
				static void      DeleteFileITunes(const String &filePath);
				static String    DownloadFileFromITunes(const String url, bool download = true);
				static StringMap GetMediaStreamMetaItunes(int id);
				static int       ScanITunesLibrary(void* userData);
			#elif defined _macosx
				static int       FileBookmarkLoad(int id, bool startAccess = true);
				static int       FileBookmarkSave(const String &filePath, int id);
			#elif defined _windows
				static WString   GetFileExtension(const WString &filePath, bool upperCase);
				static WString   GetPathDatabaseW();
				static WString   GetPathFontW();
				static WString   GetPathFontArialW();
				static WString   GetPathGUIW();
				static WString   GetPathImagesW();
				static WString   GetPathLanguagesW();
				static WString   GetPathFromArgumentList(wchar_t* argv[], const int argc);
				static WString   GetPathThumbnailsDirW();
				static WString   GetPathThumbnailsW(int mediaID);
				static WString   GetPathThumbnailsW(const WString &fileName);
				static bool      IsHttp(const WString &filePath);
				static bool      IsITunes(const WString &filePath);
				static bool      IsMediaFile(const WString &filePath);
				static bool      IsPicture(const WString &filePath);
				static bool      IsSambaServer(const WString &filePath);
			#endif

			#if defined _windows
				static int     OpenFile(const WString &filePath);
				static WString OpenFileBrowser(bool selectDirectory);
			#else
				static int     OpenFile(const String &filePath);
				static String  OpenFileBrowser(bool selectDirectory);
			#endif

		private:
			static int     addMediaFile(const String &fullPath);
			static int     addMediaFilesInDirectory(const String &dirPath);
			static size_t  downloadToFileT(void* userData, size_t size, size_t bytes, FILE* file);
			static size_t  downloadToBytesT(void* data, size_t size, size_t bytes, VM_Bytes* outData);
			static size_t  downloadToStringT(void* userData, size_t size, const size_t bytes, void* userPointer);
			static int     fileCopy(const String &sourcePath, const String &destinationPath);
			static Strings getDirectoryContent(const String &directoryPath, bool returnFiles, bool checkSystemFiles = true);
			static Strings getDirectorySubDirectories(const String &directoryPath);
			static String  getDrive(const String &filePath);
			static String  getDriveName(const String &filePath);
			static String  getFileContent(const String &filePath);
			static String  getMediaResolutionStringVideo(LIB_FFMPEG::AVStream* stream);
			static Strings getOpticalFileBluray(const String &path);
			static Strings getOpticalFileDVD(const String &path);
			static bool    hasFileExtension(const String &filePath);
			static bool    isDirectory(uint16_t statMode);
			static bool    isExpiredDropboxTokenOAuth2(const String &oauth2);
			static bool    isFile(uint16_t statMode);
			static bool    isRootDrive(const String &filePath);
			static bool    isSubtitle(const String &filePath);
			static bool    isSystemFile(const String &fileName);
			static bool    isYouTube(const String &mediaURL);
			static CURL*   openCURL(const String &mediaURL);

			#if defined _android
				static Strings        getAndroidAssets(const String &assetDirectory);
			#elif defined _windows
				static WString        getDrive(const WString &filePath);
				static int SDLCALL    SDL_RW_Close(SDL_RWops* rwops);
				static size_t SDLCALL SDL_RW_Read(SDL_RWops*  rwops, void* ptr, size_t size, size_t count);
				static Sint64 SDLCALL SDL_RW_Seek(SDL_RWops*  rwops, Sint64 offset, int whence);
				static Sint64 SDLCALL SDL_RW_Size(SDL_RWops*  rwops);
				static size_t SDLCALL SDL_RW_Write(SDL_RWops* rwops, const void* ptr, size_t size, size_t count);
			#endif

		};
	}
}

#endif
