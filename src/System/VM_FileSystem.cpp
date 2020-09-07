#include "VM_FileSystem.h"
#include "VM_FS_Extensions.h"
#include "VM_FS_MimeTypes.h"

using namespace VoyaMedia::Database;
using namespace VoyaMedia::Graphics;
using namespace VoyaMedia::JSON;
using namespace VoyaMedia::MediaPlayer;
using namespace VoyaMedia::XML;

std::queue<String> System::VM_FileSystem::MediaFiles;
bool               System::VM_FileSystem::updateMeta = false;

int System::VM_FileSystem::addMediaFile(const String &fullPath)
{
	if (fullPath.empty() || (fullPath.find(VM_Window::WorkingDirectory) != String::npos) || VM_Window::Quit)
		return ERROR_INVALID_ARGUMENTS;

	int  dbResult;
	auto db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);
	int  mediaID = 0;

	if (DB_RESULT_OK(dbResult))
		mediaID = db->getID(fullPath);

	DELETE_POINTER(db);

	// CHECK FILE/MEDIA TYPE
	bool isPicture = VM_FileSystem::IsPicture(fullPath);
	bool isMedia   = (isPicture ? false : VM_FileSystem::IsMediaFile(fullPath));
	bool isConcat  = VM_FileSystem::IsConcat(fullPath);
	
	if (!isMedia && isConcat)
		isMedia = true;

	if (!isPicture && !isMedia)
		return ERROR_UNKNOWN;

	while (!VM_Player::State.isStopped && !VM_Window::Quit)
		SDL_Delay(DELAY_TIME_BACKGROUND);

	if (VM_Window::Quit)
		return ERROR_UNKNOWN;
	
	String fileName = VM_FileSystem::GetFileName(fullPath, false);

	VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["status.adding"].c_str(), fileName.c_str());

	// FILE IS VALID - ALREADY ADDED
	if (mediaID > 0) {
		VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["status.already_added"].c_str(), fileName.c_str());
		return RESULT_OK;
	}

	LIB_FFMPEG::AVFormatContext* formatContext = NULL;
	VM_MediaType                 mediaType     = MEDIA_TYPE_UNKNOWN;

	// FILE IS A VALID PICTURE
	if (isPicture) { 
		mediaType = MEDIA_TYPE_PICTURE;
	}
	// FILE IS NOT A VALID PICTURE, TRY OPENING URL|FILE AS AUDIO|VIDEO TO TEST IF IT IS VALID
	else if (isMedia)
	{
		formatContext = VM_FileSystem::GetMediaFormatContext(fullPath, false);

		if ((formatContext != NULL) && (formatContext->probe_score >= AVPROBE_SCORE_RETRY))
			mediaType = VM_FileSystem::GetMediaType(formatContext);
	}

	if ((mediaType == MEDIA_TYPE_UNKNOWN) || (isConcat && (mediaType != MEDIA_TYPE_VIDEO))) {
		FREE_AVFORMAT(formatContext);
		return ERROR_UNKNOWN;
	}

	// MEDIA TYPE VALIDATIONS
	bool validMediaFile = ((mediaType == MEDIA_TYPE_AUDIO) || (mediaType == MEDIA_TYPE_PICTURE) || (mediaType == MEDIA_TYPE_VIDEO));

	// WMA DRM
	if ((formatContext != NULL) && VM_FileSystem::IsDRM(formatContext->metadata))
		validMediaFile = false;

	FREE_AVFORMAT(formatContext);

	// FILE SIZE
	size_t fileSize = VM_FileSystem::GetFileSize(fullPath);

	// Bluray AACS
	if ((VM_FileSystem::GetFileExtension(fullPath, true) == "M2TS") && VM_FileSystem::IsBlurayAACS(fullPath, fileSize))
		validMediaFile = false;

	// DVD CSS
	if ((VM_FileSystem::GetFileExtension(fullPath, true) == "VOB") && VM_FileSystem::IsDVDCSS(fullPath, fileSize))
		validMediaFile = false;

	// FILE IS INVALID MEDIA (AUDIO|PICTURE|VIDEO)
	if (!validMediaFile)
		return ERROR_UNKNOWN;

	String mimeType      = VM_FileSystem::GetFileMIME(fullPath);
	String directoryName = fullPath.substr(0, fullPath.rfind(PATH_SEPERATOR));

	db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

	if (DB_RESULT_OK(dbResult))
		dbResult = db->addFile(fullPath, fileName, directoryName, fileSize, mediaType, mimeType);

	#if defined _macosx
		mediaID = db->getID(fullPath);
	#endif

	DELETE_POINTER(db);

	if (DB_RESULT_OK(dbResult))
	{
		// SAVE FILE URL IN BOOKMARK FILE
		#if defined _macosx
			VM_FileSystem::FileBookmarkSave(fullPath, mediaID);
		#endif

		VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["status.added"].c_str(), fileName.c_str());
	} else {
		VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add"].c_str(), fileName.c_str());
		return ERROR_UNKNOWN;
	}

	return RESULT_OK;
}

int System::VM_FileSystem::addMediaFilesInDirectory(const String &directoryPath)
{
	Strings files;
	String  formattedPath;
	bool    error = true;

	if (directoryPath.empty() || VM_Window::Quit)
		return ERROR_INVALID_ARGUMENTS;

	files = VM_FileSystem::GetDirectoryFiles(directoryPath);

	for (const auto &file : files)
	{
		while (!VM_Player::State.isStopped && !VM_Window::Quit)
			SDL_Delay(DELAY_TIME_BACKGROUND);

		if (VM_Window::Quit)
			break;

		formattedPath = VM_Text::GetFullPath(directoryPath, file);

		if (formattedPath.size() >= MAX_FILE_PATH)
			continue;

		if (VM_FileSystem::addMediaFile(formattedPath) == RESULT_OK)
			error = false;
	}

	return (error && !files.empty() ? ERROR_UNKNOWN : RESULT_OK);
}

int System::VM_FileSystem::AddMediaFilesRecursively(const String &dirPath)
{
	Strings files;
	String  formattedPath;
	Strings subDirectories;
	bool    error = true;

	if (VM_Window::Quit)
		return ERROR_INVALID_ARGUMENTS;

	if (dirPath.empty() || (dirPath.size() >= MAX_FILE_PATH) || (dirPath == VM_Window::WorkingDirectory))
		return ERROR_UNKNOWN;

	// BLURAY / DVD
	if ((dirPath.find("BDMV") != String::npos) || (dirPath.find("VIDEO_TS") != String::npos))
	{
		// BLURAY
		if (dirPath.find("BDMV") != String::npos)
			files = VM_FileSystem::getOpticalFileBluray(dirPath);
		// DVD
		else if (dirPath.find("VIDEO_TS") != String::npos)
			files = VM_FileSystem::getOpticalFileDVD(dirPath);

		for (const auto &file : files) {
			if (VM_FileSystem::addMediaFile(file) == RESULT_OK)
				error = false;
		}

		return (error && !files.empty() ? ERROR_UNKNOWN : RESULT_OK);
	// NORMAL DIRECTORY - ADD FILES IN DIRECTORY
	} else if (VM_FileSystem::addMediaFilesInDirectory(dirPath) == RESULT_OK) {
		error = false;
	}

	// RECURSIVELY ADD SUB DIRECTORIES
	subDirectories = VM_FileSystem::getDirectorySubDirectories(dirPath);

	for (const auto &dir : subDirectories)
	{
		while (!VM_Player::State.isStopped && !VM_Window::Quit)
			SDL_Delay(DELAY_TIME_BACKGROUND);

		if (VM_Window::Quit)
			break;

		formattedPath = VM_Text::GetFullPath(dirPath, dir);

		if (formattedPath.size() >= MAX_FILE_PATH)
			continue;

		VM_FileSystem::AddMediaFilesRecursively(formattedPath);
	}

	return (error ? ERROR_UNKNOWN : RESULT_OK);
}

int System::VM_FileSystem::CleanDB(void* userData)
{
	VM_ThreadManager::Threads[THREAD_CLEAN_DB]->completed = false;

	VM_Window::StatusString = VM_Window::Labels["status.clean.db"];

	VM_DBResult rows;
	int         dbResult;
	auto        db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

	if (DB_RESULT_OK(dbResult))
		rows = db->getRows("SELECT id, full_path FROM MEDIA_FILES;");

	DELETE_POINTER(db);

	for (int i = 0; i < (int)rows.size(); i++)
	{
		if (VM_Window::Quit)
			break;

		VM_Window::SetStatusProgress(
			(uint32_t)i, (uint32_t)rows.size(), VM_Window::Labels["status.clean.db"]
		);

		int    mediaID    = std::atoi(rows[i]["id"].c_str());
		String fullPath   = rows[i]["full_path"];
		bool   deleteFile = false;

		if (VM_FileSystem::IsHttp(fullPath) || VM_FileSystem::IsConcat(fullPath))
		{
			if (!VM_FileSystem::FileExists(fullPath, L""))
				deleteFile = true;
		}
		else
		{
			#if defined _windows
				db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

				if (DB_RESULT_OK(dbResult) && !VM_FileSystem::FileExists("", db->getValueW(mediaID, "full_path")))
					deleteFile = true;

				DELETE_POINTER(db);
			#else
				if (!VM_FileSystem::FileExists(fullPath, L""))
					deleteFile = true;
			#endif
		}

		if (deleteFile)
		{
			db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

			if (DB_RESULT_OK(dbResult))
				db->deleteMediaFile(mediaID);

			DELETE_POINTER(db);
		}
	}

	if (!VM_Window::Quit)
	{
		VM_Window::StatusString = VM_Window::Labels["status.clean.db.finished"];

		VM_GUI::ListTable->refreshRows();
		VM_FileSystem::RefreshMetaData();
	}

	if (VM_ThreadManager::Threads[THREAD_CLEAN_DB] != NULL) {
		VM_ThreadManager::Threads[THREAD_CLEAN_DB]->start     = false;
		VM_ThreadManager::Threads[THREAD_CLEAN_DB]->completed = true;
	}

	if (!VM_Window::Quit)
		VM_Window::Refresh();

	return RESULT_OK;
}

int System::VM_FileSystem::CleanThumbs(void* userData)
{
	VM_ThreadManager::Threads[THREAD_CLEAN_THUMBS]->completed = false;
	
	VM_Window::StatusString = VM_Window::Labels["status.clean.thumbs"];
	
	#if defined _windows
		String thumbsDir = VM_Text::ToUTF8(VM_FileSystem::GetPathThumbnailsDirW().c_str());
	#else
		String thumbsDir = VM_FileSystem::GetPathThumbnailsDir();
	#endif
	
	Strings thumbFiles = VM_FileSystem::GetDirectoryFiles(thumbsDir);
	
	for (int i = 0; i < (int)thumbFiles.size(); i++)
	{
		if (VM_Window::Quit)
			break;

		VM_Window::SetStatusProgress((uint32_t)i, (uint32_t)thumbFiles.size(), VM_Window::Labels["status.clean.thumbs"]);

		remove(String(thumbsDir + PATH_SEPERATOR + thumbFiles[i]).c_str());
	}
	
	VM_Window::StatusString = VM_Window::Labels["status.clean.thumbs.finished"];

	if (!thumbFiles.empty() && !VM_Window::Quit)
	{
		VM_GUI::ListTable->refreshRows();
		VM_FileSystem::RefreshMetaData();
	}

	if (VM_ThreadManager::Threads[THREAD_CLEAN_THUMBS] != NULL) {
		VM_ThreadManager::Threads[THREAD_CLEAN_THUMBS]->start     = false;
		VM_ThreadManager::Threads[THREAD_CLEAN_THUMBS]->completed = true;
	}

	return RESULT_OK;
}

void System::VM_FileSystem::CloseMediaFormatContext(LIB_FFMPEG::AVFormatContext* formatContext, int mediaID)
{
	FREE_AVFORMAT(formatContext);

	// UNLOAD BOOKMARKED FILE
	#if defined _macosx
		VM_FileSystem::FileBookmarkLoad(mediaID, false);
	#endif
}

int System::VM_FileSystem::CreateDefaultDirectoryStructure()
{
	Strings  copyDirs    = { "doc", "fonts", "gui", "img", "lang", "web" };
	WStrings copyDirsW   = { L"doc", L"fonts", L"gui", L"img", L"lang", L"web" };
	Strings  createDirs  = { "db", "doc", "fonts", "gui", "img", "lang", "thumbs", "web" };
	WStrings createDirsW = { L"db", L"doc", L"fonts", L"gui", L"img", L"lang", L"thumbs", L"web" };

	#if defined _android
		Strings      androidAssets;
		String       destinationFile, sourceFile;
		const mode_t directoryPermissions = (S_IRWXU|S_IRWXG);

		if (!VM_FileSystem::FileExists(String(VM_Window::WorkingDirectory + "/fonts"), L"")) {
			for (const auto &dir : createDirs)
				mkdir(String(VM_Window::WorkingDirectory + "/" + dir).c_str(), directoryPermissions);
		}

		for (const auto &dir : copyDirs)
		{
			androidAssets = VM_FileSystem::getAndroidAssets(dir);

			for (const auto &asset : androidAssets)
			{
				sourceFile      = String(dir + "/" + asset);
				destinationFile = String(VM_Window::WorkingDirectory + "/" + dir + "/" + asset);

				std::remove(destinationFile.c_str());
				VM_FileSystem::fileCopy(sourceFile, destinationFile);
			}
		}

		if (!VM_FileSystem::FileExists(String(VM_Window::WorkingDirectory + "/fonts/NotoSans-Merged.ttf"), L""))
			return ERROR_UNKNOWN;
	#elif defined _ios || defined _macosx
		NSString* destinationPath = [NSString stringWithUTF8String:VM_Window::WorkingDirectory.c_str()];
        NSString* sourcePath      = [[NSBundle mainBundle] resourcePath];

		for (const auto &dir : createDirs)
			[[NSFileManager defaultManager] createDirectoryAtPath:[destinationPath stringByAppendingPathComponent:[NSString stringWithUTF8String:dir.c_str()]] withIntermediateDirectories:YES attributes:NULL error:NULL];

		for (const auto &dir : copyDirs) {
			[[NSFileManager defaultManager] removeItemAtPath:[destinationPath stringByAppendingPathComponent:[NSString stringWithUTF8String:dir.c_str()]] error:NULL];
			[[NSFileManager defaultManager] copyItemAtPath:[sourcePath stringByAppendingPathComponent:[NSString stringWithUTF8String:dir.c_str()]] toPath:[destinationPath stringByAppendingPathComponent:[NSString stringWithUTF8String:dir.c_str()]] error:NULL];
		}

		if (![[NSFileManager defaultManager] fileExistsAtPath:[destinationPath stringByAppendingPathComponent:@"fonts/NotoSans-Merged.ttf"]])
			return ERROR_UNKNOWN;
	#else
		String appDirectory = "";

		#if defined _linux
			const mode_t directoryPermissions = (S_IRWXU|S_IRWXG);

			mkdir(VM_Window::WorkingDirectory.c_str(), directoryPermissions);

			for (const auto &dir : createDirs)
				mkdir(String(VM_Window::WorkingDirectory + "/" + dir).c_str(), directoryPermissions);

			appDirectory = "/opt/voyamedia";
		#elif defined _windows
			if (!CreateDirectoryW(VM_Window::WorkingDirectoryW.c_str(), NULL) && (GetLastError() != ERROR_ALREADY_EXISTS))
				return ERROR_UNKNOWN;

			for (const auto &dir : createDirsW)
				CreateDirectoryW(WString(VM_Window::WorkingDirectoryW + L"\\" + dir).c_str(), NULL);

			#if defined _windows
				char* basePath = SDL_GetBasePath();

				if ((basePath != NULL) && (strlen(basePath) > 0))
					appDirectory = String(basePath);

				if (VM_Text::GetLastCharacter(appDirectory) == PATH_SEPERATOR_C)
					appDirectory = appDirectory.substr(0, appDirectory.rfind(PATH_SEPERATOR));

				if (!VM_FileSystem::FileExists(appDirectory, L""))
					appDirectory = "";
			#endif
		#endif

		if (appDirectory.empty())
			return RESULT_OK;

		Strings files;

		#if defined _windows
			WString sourceFile, destinationFile;

			for (int i = 0; i < (int)copyDirs.size(); i++)
			{
				files = VM_FileSystem::GetDirectoryFiles(String(appDirectory + PATH_SEPERATOR + copyDirs[i]), false);

				for (const auto &file : files)
				{
					sourceFile      = (VM_Text::ToUTF16(appDirectory.c_str()) + L"\\" + copyDirsW[i] + L"\\" + VM_Text::ToUTF16(file.c_str()));
					destinationFile = (VM_Window::WorkingDirectoryW          + L"\\" + copyDirsW[i] + L"\\" + VM_Text::ToUTF16(file.c_str()));

					CopyFileW(sourceFile.c_str(), destinationFile.c_str(), false);
				}
			}
			
			if (!VM_FileSystem::FileExists("", (VM_Window::WorkingDirectoryW + L"\\fonts\\NotoSans-Merged.ttf")))
				return ERROR_UNKNOWN;
		#else
			String sourceFile, destinationFile;

			for (int i = 0; i < (int)copyDirs.size(); i++)
			{
				files = VM_FileSystem::GetDirectoryFiles(String(appDirectory + PATH_SEPERATOR + copyDirs[i]), false);

				for (uint32_t j = 0; j < files.size(); j++)
				{
					sourceFile      = String(appDirectory                + PATH_SEPERATOR + copyDirs[i] + PATH_SEPERATOR + files[j]);
					destinationFile = String(VM_Window::WorkingDirectory + PATH_SEPERATOR + copyDirs[i] + PATH_SEPERATOR + files[j]);

					VM_FileSystem::fileCopy(sourceFile, destinationFile);
				}
			}

			if (!VM_FileSystem::FileExists(String(VM_Window::WorkingDirectory + PATH_SEPERATOR + "fonts" + PATH_SEPERATOR + "NotoSans-Merged.ttf"), L""))
				return ERROR_UNKNOWN;
		#endif
	#endif

	return RESULT_OK;
}

#if defined _ios
void System::VM_FileSystem::DeleteFileITunes(const String &filePath)
{
	if (!filePath.empty())
	{
		NSString* filePathNS = [[NSString alloc] initWithUTF8String:filePath.c_str()];

		if (filePathNS != nil)
			[[NSFileManager defaultManager] removeItemAtPath:filePathNS error:nil];
		else
			std::remove(filePath.c_str());
	}
}

String System::VM_FileSystem::DownloadFileFromITunes(const String url, bool download)
{
	NSAutoreleasePool* autoreleasePool   = [[NSAutoreleasePool alloc] init];
	NSString*          tmpPathNS         = [[[NSFileManager defaultManager] temporaryDirectory] path];
	NSString*          destinationPathNS = [tmpPathNS stringByAppendingPathComponent:[[NSUUID UUID] UUIDString]];
	String             filePath          = String([destinationPathNS UTF8String]);

	if (!download) {
		[autoreleasePool release];
		return filePath;
	}

	if (url.substr(0, 13) == "ipod-library:")
	{
		NSURL*                urlNS   = [[NSURL alloc] initWithString:[[NSString alloc] initWithUTF8String:url.c_str()]];
		AVURLAsset*           asset   = [[AVURLAsset alloc] initWithURL:urlNS options:nil];
		AVAssetExportSession* session = (asset != nil ? [[AVAssetExportSession alloc] initWithAsset:asset presetName:AVAssetExportPresetPassthrough] : nil);

		if (session != nil)
		{
			session.outputURL      = [NSURL fileURLWithPath:destinationPathNS];
			session.outputFileType = @"com.apple.quicktime-movie";

			[session exportAsynchronouslyWithCompletionHandler:^{}];

			while ((session.status != AVAssetExportSessionStatusCompleted) && (session.status != AVAssetExportSessionStatusFailed) && (session.status != AVAssetExportSessionStatusCancelled))
				SDL_Delay(DELAY_TIME_DEFAULT);
		}
	}
	else if (url.substr(0, 15) == "iphoto-library:")
	{
		PHFetchOptions* fetchOptions = [[PHFetchOptions alloc] init];

		fetchOptions.includeAssetSourceTypes = (
			PHAssetSourceTypeNone | PHAssetSourceTypeUserLibrary | PHAssetSourceTypeCloudShared | PHAssetSourceTypeiTunesSynced
		);

		String         identifier   = url.substr(15);
		NSString*      identifierNS = [[NSString alloc] initWithUTF8String:identifier.c_str()];
		NSArray*       identifiers  = @[ identifierNS ];
		PHFetchResult* result       = [PHAsset fetchAssetsWithLocalIdentifiers:identifiers options:fetchOptions];
		PHAsset*       asset        = [result firstObject];

		if ((asset != nil) && ([asset mediaType] == PHAssetMediaTypeImage))
		{
			PHImageRequestOptions* requestOptions = [[PHImageRequestOptions alloc] init];

			requestOptions.deliveryMode         = PHImageRequestOptionsDeliveryModeHighQualityFormat;
			requestOptions.networkAccessAllowed = YES;
			requestOptions.synchronous          = YES;
			requestOptions.version              = PHImageRequestOptionsVersionOriginal;

			[[PHImageManager defaultManager]
			requestImageDataForAsset:asset
			options:requestOptions
			resultHandler:^(NSData* imageData, NSString* dataUTI, UIImageOrientation orientation, NSDictionary* info)
			{
				[imageData writeToFile:destinationPathNS atomically:YES];
			}];
		}
	}

	[autoreleasePool release];

	return filePath;
}
#endif

System::VM_Bytes* System::VM_FileSystem::DownloadToBytes(const String &mediaURL)
{
	if (mediaURL.empty())
		return NULL;

	VM_Bytes* bytes = new VM_Bytes();
	CURL*     curl  = VM_FileSystem::openCURL(mediaURL);

	if (curl == NULL)
		return NULL;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, VM_FileSystem::downloadToBytesT);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,     bytes);

	curl_easy_perform(curl);
	CLOSE_CURL(curl);

	return bytes;
}

size_t System::VM_FileSystem::downloadToBytesT(void* data, size_t size, size_t bytes, VM_Bytes* outData)
{
	size_t newSize = (size * bytes);

	outData->write(data, newSize);

	return newSize;
}

FILE* System::VM_FileSystem::DownloadToFile(const String &mediaURL, const String &filePath)
{
	if (mediaURL.empty() || filePath.empty())
		return NULL;

	FILE* fileHandle;
	CURL* curl = VM_FileSystem::openCURL(mediaURL);

	if (curl == NULL)
		return NULL;
	
	fileHandle = fopen(filePath.c_str(), "wb");

	if (fileHandle == NULL) {
		curl_easy_cleanup(curl);
		return NULL;
	}

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, VM_FileSystem::downloadToFileT);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,     fileHandle);

	curl_easy_perform(curl);

	CLOSE_FILE(fileHandle);
	
	fileHandle = fopen(filePath.c_str(), "rb");

	CLOSE_CURL(curl);

	return fileHandle;
}

size_t System::VM_FileSystem::downloadToFileT(void* data, size_t size, size_t bytes, FILE* file)
{
	return std::fwrite(data, size, bytes, file);
}

String System::VM_FileSystem::DownloadToString(const String &mediaURL)
{
	CURL*  curl    = VM_FileSystem::openCURL(mediaURL);
	String content = "";

	if (curl == NULL)
		return "";

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, VM_FileSystem::downloadToStringT);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &content);

	curl_easy_perform(curl);
	CLOSE_CURL(curl);

	return content;
}

size_t System::VM_FileSystem::downloadToStringT(void* userData, size_t size, size_t bytes, void* userPointer)
{
	static_cast<String*>(userPointer)->append(static_cast<char*>(userData), (size * bytes));
	return (size * bytes);
}

#if defined _macosx
int System::VM_FileSystem::FileBookmarkLoad(int id, bool startAccess)
{
	if (id < 1)
		return ERROR_INVALID_ARGUMENTS;

	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	NSError*  error          = nil;
	String    bookmarkFile   = (std::to_string(id) + ".bookmark");
	NSString* bookmarkFileNS = [[NSString alloc] initWithUTF8String:bookmarkFile.c_str()];
	NSURL*    bookmarkPathNS = [[[[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask] lastObject] URLByAppendingPathComponent:bookmarkFileNS];
	NSData*   bookmarkDataNS = [NSData dataWithContentsOfURL:bookmarkPathNS];
	NSURL*    fileURLNS      = nil;

	if (bookmarkDataNS != nil) {
		fileURLNS = [NSURL URLByResolvingBookmarkData:bookmarkDataNS options:NSURLBookmarkResolutionWithSecurityScope relativeToURL:nil bookmarkDataIsStale:nil error:&error];
		[[NSUserDefaults standardUserDefaults] setURL:fileURLNS forKey:@"path"];
	}

	if (fileURLNS != nil)
	{
		if (startAccess)
			[fileURLNS startAccessingSecurityScopedResource];
		else
			[fileURLNS stopAccessingSecurityScopedResource];
	}

	[pool release];

	return RESULT_OK;
}

int System::VM_FileSystem::FileBookmarkSave(const String &filePath, int id)
{
	if (id < 1)
		return ERROR_INVALID_ARGUMENTS;

	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	NSError*  error          = nil;
	NSURL*    fileURLNS      = [[NSURL alloc] initFileURLWithPath:[[NSString alloc] initWithUTF8String:filePath.c_str()] isDirectory:NO];
	String    bookmarkFile   = (std::to_string(id) + ".bookmark");
	NSString* bookmarkFileNS = [[NSString alloc] initWithUTF8String:bookmarkFile.c_str()];
	NSURL*    bookmarkPathNS = [[[[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask] lastObject] URLByAppendingPathComponent:bookmarkFileNS];
	NSData*   bookmarkDataNS = nil;

	if (fileURLNS != nil)
		bookmarkDataNS = [fileURLNS bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope includingResourceValuesForKeys:nil relativeToURL:nil error:&error];

	if ((bookmarkDataNS != nil) && (bookmarkPathNS != nil))
		[bookmarkDataNS writeToURL:bookmarkPathNS atomically:YES];

	[pool release];

	return RESULT_OK;
}
#endif

int System::VM_FileSystem::fileCopy(const String &sourcePath, const String &destinationPath)
{
	const int fileBufferSize = (4 * KILO_BYTE);
	char      fileBuffer[fileBufferSize];
	size_t    fileReadSize   = fileBufferSize;

	if (sourcePath.empty() || destinationPath.empty())
		return ERROR_INVALID_ARGUMENTS;

	#if defined _android
		SDL_RWops* sourceFile = SDL_RWFromFile(sourcePath.c_str(), "rb");

		if (sourceFile == NULL)
			return ERROR_UNKNOWN;

		SDL_RWops* destinationFile = SDL_RWFromFile(destinationPath.c_str(), "wb");

		if (destinationFile == NULL) {
			CLOSE_FILE_RW(sourceFile);
			return ERROR_UNKNOWN;
		}

		while ((fileReadSize = SDL_RWread(sourceFile, fileBuffer, 1, fileBufferSize)))
			SDL_RWwrite(destinationFile, fileBuffer, 1, fileReadSize);

		CLOSE_FILE_RW(sourceFile);
		CLOSE_FILE_RW(destinationFile);
	#else
		FILE* sourceFile = fopen(sourcePath.c_str(), "rb");

		if (sourceFile == NULL)
			return ERROR_UNKNOWN;

		FILE* destinationFile = fopen(destinationPath.c_str(), "wb");

		if (destinationFile == NULL) {
			CLOSE_FILE(sourceFile);
			return ERROR_UNKNOWN;
		}
		
		while ((fileReadSize = std::fread(fileBuffer, sizeof(char), fileBufferSize, sourceFile)))
			std::fwrite(fileBuffer, sizeof(char), fileReadSize, destinationFile);

		CLOSE_FILE(sourceFile);
		CLOSE_FILE(destinationFile);
	#endif

	return RESULT_OK;
}

bool System::VM_FileSystem::FileExists(const String &filePath, const WString &filePathW)
{
	#if defined _ios
	if (!filePath.empty() && VM_FileSystem::IsITunes(filePath))
	{
		NSAutoreleasePool* autoreleasePool = [[NSAutoreleasePool alloc] init];
		NSURL*             urlNS           = [[NSURL alloc] initWithString:[[NSString alloc] initWithUTF8String:filePath.c_str()]];
		AVURLAsset*        mediaAsset      = [[AVURLAsset alloc] initWithURL:urlNS options:NULL];
		bool               fileExists      = (mediaAsset != NULL ? [mediaAsset isReadable]:NO);
		
		[autoreleasePool release];
		return fileExists;
	}
	else
	#endif
	// HTTP
	if (!filePath.empty() && VM_FileSystem::IsHttp(filePath))
	{
		CURL* curl = VM_FileSystem::openCURL(filePath);

		if (curl != NULL)
		{
			curl_easy_setopt(curl, CURLOPT_FAILONERROR,       1L);
			curl_easy_setopt(curl, CURLOPT_NOBODY,            1L);
			curl_easy_setopt(curl, CURLOPT_ACCEPTTIMEOUT_MS,  MAX_CURL_TIMEOUT);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, MAX_CURL_TIMEOUT);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,        MAX_CURL_TIMEOUT);

			curl_easy_perform(curl);

			long responseCode;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

			CLOSE_CURL(curl);

			if (responseCode == HTTP_RESPONSE_OK)
				return true;
		}
	}
	// BLURAY / DVD: "concat:streamPath|stream1|stream2|streamN|duration|title|audioTrackCount|subTrackCount|"
	else if (!filePath.empty() && VM_FileSystem::IsConcat(filePath))
	{
		Strings fileDetails = VM_Text::Split(filePath.substr(7), "|");

		if ((fileDetails.size() > 1) && VM_FileSystem::GetFileSize(String(fileDetails[0] + fileDetails[1])) > 0)
			return true;
	#if defined _windows
	}
	// FILE - UNICODE - WINDOWS
	else if (!filePathW.empty())
	{
		stat_t fileStruct;

		if (fstatw(filePathW.c_str(), &fileStruct) == 0)
			return true;
	#endif
	}
	// FILE - NON-UNICODE - ASCII
	else if (!filePath.empty())
	{
		stat_t fileStruct;

		if (fstat(filePath.c_str(), &fileStruct) == 0)
			return true;
	}

	return false;
}

#if defined _windows
int SDLCALL System::VM_FileSystem::SDL_RW_Close(SDL_RWops* rwops)
{
	int status = 0;
	if (rwops != NULL) {
		if (std::fclose(static_cast<FILE*>(rwops->hidden.windowsio.buffer.data)) != 0) { status = SDL_Error(SDL_EFWRITE); }
		SDL_FreeRW(rwops);
	}
	return status;
}

size_t SDLCALL System::VM_FileSystem::SDL_RW_Read(SDL_RWops* rwops, void* ptr, size_t size, size_t count)
{
	size_t readSize = std::fread(ptr, size, count, static_cast<FILE*>(rwops->hidden.windowsio.buffer.data));

	return (readSize == 0 && std::ferror(static_cast<FILE*>(rwops->hidden.windowsio.buffer.data)) != 0 ? SDL_Error(SDL_EFREAD) : readSize);
}

Sint64 SDLCALL System::VM_FileSystem::SDL_RW_Seek(SDL_RWops* rwops, Sint64 offset, int whence)
{
	int result = fseek(static_cast<FILE*>(rwops->hidden.windowsio.buffer.data), offset, whence);

	return (result != 0 ? SDL_Error(SDL_EFSEEK) : std::ftell(static_cast<FILE*>(rwops->hidden.windowsio.buffer.data)));
}

Sint64 SDLCALL System::VM_FileSystem::SDL_RW_Size(SDL_RWops* rwops)
{
	Sint64 position = SDL_RWseek(rwops, 0, RW_SEEK_CUR);

	if (position < 0)
		return -1;

	Sint64 size = SDL_RWseek(rwops, 0, RW_SEEK_END);

	SDL_RWseek(rwops, position, RW_SEEK_SET);

	return size;
}

size_t SDLCALL System::VM_FileSystem::SDL_RW_Write(SDL_RWops* rwops, const void* ptr, size_t size, size_t count)
{
	size_t writeSize = std::fwrite(ptr, size, count, static_cast<FILE*>(rwops->hidden.windowsio.buffer.data));

	return (writeSize == 0 && std::ferror(static_cast<FILE*>(rwops->hidden.windowsio.buffer.data)) != 0 ? SDL_Error(SDL_EFWRITE) : writeSize);
}

SDL_RWops* System::VM_FileSystem::FileOpenSDLRWops(FILE* file)
{
	if (file == NULL)
		return NULL;

	SDL_RWops* rwops = SDL_AllocRW();

	if (rwops == NULL)
		return NULL;

	rwops->close = VM_FileSystem::SDL_RW_Close;
	rwops->read  = VM_FileSystem::SDL_RW_Read;
	rwops->seek  = VM_FileSystem::SDL_RW_Seek;
	rwops->size  = VM_FileSystem::SDL_RW_Size;
	rwops->write = VM_FileSystem::SDL_RW_Write;
	rwops->type  = SDL_RWOPS_STDFILE;

	rwops->hidden.windowsio.buffer.data = file;

	return rwops;
}
#endif

#if defined _android
Strings System::VM_FileSystem::getAndroidAssets(const String &assetDirectory)
{
	// Get the JNI environment settings and create an assets manager
	Strings        androidAssets;
	JNIEnv*        jniEnvironment        = VM_Window::JNI->getEnvironment();
	jobject        jniActivity           = (jobject)SDL_AndroidGetActivity();
	jclass         jniClassActivity      = jniEnvironment->FindClass("android/app/Activity");
	jclass         jniClassResources     = jniEnvironment->FindClass("android/content/res/Resources");
	jmethodID      jniMethodGetAssets    = jniEnvironment->GetMethodID(jniClassResources, "getAssets",    "()Landroid/content/res/AssetManager;");
	jmethodID      jniMethodGetResources = jniEnvironment->GetMethodID(jniClassActivity,  "getResources", "()Landroid/content/res/Resources;");
	jobject        jniResources          = jniEnvironment->CallObjectMethod(jniActivity,  jniMethodGetResources);
	jobject        jniAssetManagerObject = jniEnvironment->CallObjectMethod(jniResources, jniMethodGetAssets);
	AAssetManager* jniAssetManager       = AAssetManager_fromJava(jniEnvironment, jniAssetManagerObject);

	if (jniAssetManager != NULL)
	{
		const char* jniAssetFilename  = NULL;
		AAssetDir*  jniAssetDirectory = AAssetManager_openDir(jniAssetManager, assetDirectory.c_str());

		while ((jniAssetFilename = AAssetDir_getNextFileName(jniAssetDirectory)) != NULL)
			androidAssets.push_back(String(jniAssetFilename));

		AAssetDir_close(jniAssetDirectory);
	}

	jniEnvironment->DeleteLocalRef(jniActivity);
	jniEnvironment->DeleteLocalRef(jniClassActivity);
	jniEnvironment->DeleteLocalRef(jniClassResources);
	jniEnvironment->DeleteLocalRef(jniResources);
	jniEnvironment->DeleteLocalRef(jniAssetManagerObject);

	return androidAssets;
}

Strings System::VM_FileSystem::GetAndroidMediaFiles()
{
	auto jni = new Android::VM_JavaJNI();

	jni->attachThread(VM_Window::JNI->getJavaVM());
	jni->init();

	Strings   files;
	jclass    jniClass       = jni->getClass();
	JNIEnv*   jniEnvironment = jni->getEnvironment();
	jmethodID jniMethod      = jniEnvironment->GetStaticMethodID(jniClass, "GetMediaFiles", "[Ljava/lang/String;");

	if (jniMethod == NULL)
	{
		jni->detachThread(VM_Window::JNI->getJavaVM());
		jni->destroy();
		DELETE_POINTER(jni);

		return files;
	}

	jobjectArray jArray      = (jobjectArray)jniEnvironment->CallStaticObjectMethod(jniClass, jniMethod);
	const int    arrayLength = jniEnvironment->GetArrayLength(jArray);

	for (int i = 0; i < arrayLength; i++)
	{
		jstring     jString = (jstring)(jniEnvironment->GetObjectArrayElement(jArray, i));
		const char* cString = jniEnvironment->GetStringUTFChars(jString, NULL);

		files.push_back(String(cString));

		jniEnvironment->ReleaseStringUTFChars(jString, cString);
	}

	jni->detachThread(VM_Window::JNI->getJavaVM());
	jni->destroy();
	DELETE_POINTER(jni);

	return files;
}

String System::VM_FileSystem::GetAndroidStoragePath()
{
	jclass    jniClass          = VM_Window::JNI->getClass();
	JNIEnv*   jniEnvironment    = VM_Window::JNI->getEnvironment();
	jmethodID jniGetStoragePath = jniEnvironment->GetStaticMethodID(jniClass, "GetStoragePath", "()Ljava/lang/String;");

	if (jniGetStoragePath == NULL)
		return "";

	jstring     jString = (jstring)jniEnvironment->CallStaticObjectMethod(jniClass, jniGetStoragePath);
	const char* cString = jniEnvironment->GetStringUTFChars(jString, NULL);
	String      path    = String(cString);
	
	jniEnvironment->ReleaseStringUTFChars(jString, cString);

	return path;
}
#endif

Strings System::VM_FileSystem::getDirectoryContent(const String &directoryPath, bool returnFiles, bool checkSystemFiles)
{
	Strings directoyContent;

	if (directoryPath.size() >= MAX_FILE_PATH)
		return directoyContent;

	#if defined _windows
		uint16_t directoryPathUTF16[DEFAULT_CHAR_BUFFER_SIZE];
		VM_Text::ToUTF16(directoryPath, directoryPathUTF16, DEFAULT_CHAR_BUFFER_SIZE);

		DIR* directory = opendir(reinterpret_cast<const wchar_t*>(directoryPathUTF16));
	#else
		DIR* directory = opendir(directoryPath.c_str());
	#endif

	if (directory == NULL)
		return directoyContent;

	dirent* file;
	int     fileType = (returnFiles ? DT_REG : DT_DIR);

	while ((file = readdir(directory)) != NULL) {
		#if defined _windows
			String fileName = VM_Text::ToUTF8(file->d_name, !checkSystemFiles);
		#else
			String fileName = String(file->d_name);
		#endif

		if ((file->d_type == fileType) && (!checkSystemFiles || !VM_FileSystem::isSystemFile(fileName))) {
			directoyContent.push_back(fileName);
		}
	}

	closedir(directory);

	return directoyContent;
}

Strings System::VM_FileSystem::GetDirectoryFiles(const String &directoryPath, bool checkSystemFiles)
{
	return VM_FileSystem::getDirectoryContent(directoryPath, true, checkSystemFiles);
}

Strings System::VM_FileSystem::getDirectorySubDirectories(const String &directoryPath)
{
	return VM_FileSystem::getDirectoryContent(directoryPath, false);
}

String System::VM_FileSystem::getDrive(const String &filePath)
{
	Strings pathSplits;

	if (filePath.rfind(PATH_SEPERATOR) != String::npos)
		pathSplits = VM_Text::Split(filePath, PATH_SEPERATOR);

	if (!pathSplits.empty())
		return pathSplits[0];

	return "";
}

#if defined _windows
WString System::VM_FileSystem::getDrive(const WString &filePath)
{
	WStrings pathSplits;

	if (filePath.rfind(PATH_SEPERATOR_W) != String::npos)
		pathSplits = VM_Text::Split(filePath, PATH_SEPERATOR_W);

	if (!pathSplits.empty())
		return pathSplits[0];

	return L"";
}
#endif

String System::VM_FileSystem::getDriveName(const String &drivePath)
{
	String driveName = String(drivePath);

	#if defined _ios || defined _macosx
		NSString* driveNameNS;
		NSURL*    pathNS = [[NSURL alloc] initWithString:[[NSString alloc] initWithUTF8String:drivePath.c_str()]];

		[pathNS getResourceValue:&driveNameNS forKey:NSURLVolumeNameKey error:nil];

		if (driveNameNS != NULL)
			driveName = String([driveNameNS UTF8String]);
	#elif defined _windows
		if (VM_Text::GetLastCharacter(driveName) != PATH_SEPERATOR_C)
			driveName.append(PATH_SEPERATOR);

		wchar_t buffer[DEFAULT_CHAR_BUFFER_SIZE];

		BOOL result = GetVolumeInformationW(
			VM_FileSystem::getDrive(VM_Text::ToUTF16(driveName.c_str())).c_str(),
			buffer, DEFAULT_CHAR_BUFFER_SIZE, NULL, NULL, NULL, NULL, 0
		);

		if (result)
			driveName = VM_Text::ToUTF8(buffer, false);
		else
			driveName = VM_FileSystem::getDrive(driveName);
	#endif

	return driveName;
}

String System::VM_FileSystem::GetDropboxURL(const String &path)
{
	int    dbResult;
	auto   db    = new VM_Database(dbResult, DATABASE_SETTINGSv3);
	String token = "";

	if (DB_RESULT_OK(dbResult))
		token = db->getSettings("dropbox_token");

	DELETE_POINTER(db);

	if (!token.empty())
		token = VM_Text::Decrypt(token);

	// CHECK IF TOKEN IS EXPIRED - RE-AUTHENTICATE
	if (token.empty() || VM_FileSystem::isExpiredDropboxTokenOAuth2(token))
		return "";

	Strings headers      = { String("Authorization: Bearer " + token), "Content-Type: application/json" };
	String  data         = "{ \"path\":\"" + path + "\" }";
	String  response     = VM_FileSystem::PostData("https://api.dropboxapi.com/2/files/get_temporary_link", data, headers);
	size_t  findPosition = response.find("\"link\": \"");
	String  mediaURL     = "";

	if (findPosition != String::npos) {
		mediaURL = response.substr(findPosition + 9);
		mediaURL = mediaURL.substr(0, mediaURL.find("\""));
	}

	return mediaURL;
}

String System::VM_FileSystem::GetDropboxURL2(const String &mediaURL)
{
	int    dbResult;
	auto   db      = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);
	int    mediaID = 0;
	String path    = "";

	if (DB_RESULT_OK(dbResult))
		mediaID = db->getID(mediaURL);

	if (mediaID > 0)
		path = db->getValue(mediaID, "path");

	DELETE_POINTER(db);

	String url2 = VM_FileSystem::GetDropboxURL(path);

	if (mediaID > 0)
	{
		db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

		if (DB_RESULT_OK(dbResult))
			db->updateText(mediaID, "full_path", url2);
	}

	DELETE_POINTER(db);

	return url2;
}

String System::VM_FileSystem::getFileContent(const String &filePath)
{
	std::ifstream     file;
	std::stringstream stringBuffer;
	
	file.open(filePath);
	stringBuffer << file.rdbuf();
	file.close();

	return String(stringBuffer.str());
}

String System::VM_FileSystem::GetFileDLNA(const String &mime)
{
	if (mime.empty())
		return "";

	auto dlnaIter = VM_FileSystem::MediaFileDLNAs.find(mime);

	if (dlnaIter != VM_FileSystem::MediaFileDLNAs.end())
		return dlnaIter->second;

	return "";
}

String System::VM_FileSystem::GetFileExtension(const String &filePath, bool upperCase)
{
	String fileExtension = "";

	if ((filePath.rfind(".") == String::npos) || (VM_Text::GetLastCharacter(filePath) == '.'))
		return "";

	fileExtension = String(filePath.substr(filePath.rfind(".") + 1));
	fileExtension = (upperCase ? VM_Text::ToUpper(fileExtension) : VM_Text::ToLower(fileExtension));

	return fileExtension;
}

#if defined _windows
WString System::VM_FileSystem::GetFileExtension(const WString &filePath, bool upperCase)
{
	WString fileExtension = L"";

	if ((filePath.rfind(L".") == String::npos) || (VM_Text::GetLastCharacter(filePath) == '.'))
		return L"";

	fileExtension = filePath.substr(filePath.rfind(L".") + 1);
	fileExtension = (upperCase ? VM_Text::ToUpper(fileExtension) : VM_Text::ToLower(fileExtension));

	return fileExtension;
}
#endif

String System::VM_FileSystem::GetFileName(const String &filePath, bool removeExtension)
{
	Strings fileDetails;
	String  fileName = String(filePath);

	// BLURAY / DVD: "concat:streamPath|stream1|stream2|streamN|duration|title|audioTrackCount|subTrackCount|"
	if (VM_FileSystem::IsConcat(filePath)) {
		fileDetails = VM_Text::Split(filePath, "|");

		if (fileDetails.size() > 2)
			fileName = fileDetails[fileDetails.size() - 3];
	// FILES
	} else if (filePath.rfind(PATH_SEPERATOR) != String::npos) {
		fileName = filePath.substr(filePath.rfind(PATH_SEPERATOR) + 1);
	}

	if (removeExtension)
		fileName = fileName.substr(0, fileName.rfind('.'));

	return fileName;
}

String System::VM_FileSystem::GetFileMIME(const String &filePath)
{
	if (filePath.empty())
		return "";

	String extension = VM_FileSystem::GetFileExtension(filePath, false);

	if (!extension.empty())
		return VM_FileSystem::mediaFileMimeTypes[extension];

	return "";
}

size_t System::VM_FileSystem::GetFileSize(const String &filePath)
{
	size_t fileSize = 0;
	bool   isConcat = VM_FileSystem::IsConcat(filePath);

	// BLURAY / DVD: "concat:streamPath|stream1|...|streamN|duration|title|audioTrackCount|subTrackCount|"
	if (isConcat)
	{
		Strings parts = VM_Text::Split(filePath.substr(7), "|");

		for (uint32_t i = 1; i < parts.size() - 4; i++)
			fileSize += VM_FileSystem::GetFileSize(parts[0] + parts[i]);
	}
	else
	{
		stat_t fileStruct;

		#if defined _windows
			uint16_t filePath16[DEFAULT_CHAR_BUFFER_SIZE];
			VM_Text::ToUTF16(filePath, filePath16, DEFAULT_CHAR_BUFFER_SIZE);

			int result = fstatw(reinterpret_cast<const wchar_t*>(filePath16), &fileStruct);
		#else
			int result = fstat(filePath.c_str(), &fileStruct);
		#endif

		if (result == 0)
			fileSize = (size_t)fileStruct.st_size;
	}

	return fileSize;
}

int64_t System::VM_FileSystem::GetMediaDuration(LIB_FFMPEG::AVFormatContext* formatContext, LIB_FFMPEG::AVStream* audioStream)
{
	int64_t avRescaleA, avRescaleB, avRescaleC;
	size_t  fileSize;

	if (formatContext == NULL)
		return 0;

	if (formatContext->duration > 0)
		return (size_t)((double)formatContext->duration / AV_TIME_BASE_D);

	if (audioStream == NULL)
		return 0;

	if (audioStream->duration > 0)
		return (size_t)((double)audioStream->duration * LIB_FFMPEG::av_q2d(audioStream->time_base));

	fileSize = VM_FileSystem::GetFileSize(formatContext->filename);

	if ((audioStream->codec == NULL) || (fileSize == 0))
		return 0;

	avRescaleA = (int64_t)(fileSize * 8ll);
	avRescaleB = (int64_t)audioStream->time_base.den;
	avRescaleC = (int64_t)(audioStream->codec->bit_rate * audioStream->codec->channels * audioStream->time_base.num);

	return (LIB_FFMPEG::av_rescale(avRescaleA, avRescaleB, avRescaleC) / AV_TIME_BASE_I64);
}

LIB_FFMPEG::AVFormatContext* System::VM_FileSystem::GetMediaFormatContext(const String &filePath, bool parseStreams)
{
	if (filePath.empty())
		return NULL;

	int64_t                      duration;
	Strings                      fileParts;
	String                       file          = String(filePath);
	LIB_FFMPEG::AVFormatContext* formatContext = LIB_FFMPEG::avformat_alloc_context();
	bool                         isConcat      = VM_FileSystem::IsConcat(filePath);

	// BLURAY / DVD: "concat:streamPath|stream1|stream2|streamN|duration|title|audioTrackCount|subTrackCount|"
	if (isConcat)
	{
		fileParts = VM_Text::Split(String(file).substr(7), "|");
		file      = "concat:";

		for (uint32_t i = 1; i < fileParts.size() - 4; i++)
			file.append(fileParts[i] + "|");

		chdir(fileParts[0].c_str());
	}

	if (VM_Player::TimeOut != NULL) {
		formatContext->flags                      |= AVFMT_FLAG_NONBLOCK;
		formatContext->interrupt_callback.callback = VM_TimeOut::InterruptCallback;
		formatContext->interrupt_callback.opaque   = VM_Player::TimeOut;
	}

	// LOAD FILE URL FORM BOOKMARK FILE
	#if defined _macosx
		int          dbResult;
		int          mediaID = -1;
		VM_Database* db      = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

		if (DB_RESULT_OK(dbResult))
			mediaID = db->getID(file);

		DELETE_POINTER(db);

		VM_FileSystem::FileBookmarkLoad(mediaID, true);
	#endif

	formatContext->max_analyze_duration = (10 * AV_TIME_BASE);

	if (avformat_open_input(&formatContext, file.c_str(), NULL, NULL) < 0) {
		VM_FileSystem::CloseMediaFormatContext(formatContext, -1);
		return NULL;
	}

	if (!parseStreams)
		return formatContext;

	// TRY TO FIX MP3 FILES WITH INVALID HEADER AND CODEC TYPE
	if (VM_FileSystem::GetFileExtension(file, true) == "MP3")
	{
		for (uint32_t i = 0; i < formatContext->nb_streams; i++)
		{
			LIB_FFMPEG::AVStream* stream = formatContext->streams[i];

			if ((stream == NULL) || (stream->codec == NULL))
				continue;

			if (((VM_MediaType)stream->codec->codec_type == MEDIA_TYPE_AUDIO) &&
				(stream->codec->codec_id == LIB_FFMPEG::AV_CODEC_ID_NONE))
			{
				stream->codec->codec_id = LIB_FFMPEG::AV_CODEC_ID_MP3;
			}
		}
	}

	if (formatContext->nb_streams == 0) {
		formatContext->max_analyze_duration = (15 * AV_TIME_BASE);
		formatContext->probesize            = (10 * MEGA_BYTE);
	}

	if (avformat_find_stream_info(formatContext, NULL) < 0) {
		VM_FileSystem::CloseMediaFormatContext(formatContext, -1);
		return NULL;
	}

	#if defined _DEBUG
		LIB_FFMPEG::av_dump_format(formatContext, -1, file.c_str(), 0);
		LOG("\n");
	#endif

	if (isConcat)
	{
		duration = std::atoll(fileParts[fileParts.size() - 4].c_str());

		if (duration > 0)
			formatContext->duration = duration;
	}

	return formatContext;
}

double System::VM_FileSystem::GetMediaFrameRate(LIB_FFMPEG::AVStream* stream)
{
	if ((stream == NULL) || (stream->codec == NULL))
		return 0;

	double                 frameRate = 0;
	LIB_FFMPEG::AVRational timeBase;

	timeBase      = av_inv_q(stream->codec->time_base);
	timeBase.den *= stream->codec->ticks_per_frame;

	// r_frame_rate IS WRONG - NEEDS ADJUSTMENT
	if ((timeBase.num > 0) && (timeBase.den > 0) &&
		(LIB_FFMPEG::av_q2d(timeBase) < (LIB_FFMPEG::av_q2d(stream->r_frame_rate) * 0.7)) &&
		(fabs(1.0 - LIB_FFMPEG::av_q2d(av_div_q(stream->avg_frame_rate, stream->r_frame_rate))) > 0.1))
	{
		frameRate = LIB_FFMPEG::av_q2d(timeBase);
	// r_frame_rate IS VALID
	} else if ((stream->r_frame_rate.num > 0) && (stream->r_frame_rate.den > 0)) {
		frameRate = LIB_FFMPEG::av_q2d(stream->r_frame_rate);
	// r_frame_rate IS NOT VALID - USE avg_frame_rate
	} else {
		frameRate = LIB_FFMPEG::av_q2d(stream->avg_frame_rate);
	}

	return frameRate;
}

String System::VM_FileSystem::getMediaResolutionStringVideo(LIB_FFMPEG::AVStream* stream)
{
	if ((stream == NULL) || (stream->codec == NULL))
		return "";

	return VM_Text::Format("%dp (%dx%d)", stream->codec->height, stream->codec->width, stream->codec->height);
}

LIB_FFMPEG::AVStream* System::VM_FileSystem::GetMediaStreamBest(LIB_FFMPEG::AVFormatContext* formatContext, VM_MediaType mediaType)
{
	if (formatContext == NULL)
		return NULL;

	int64_t               quality;
	LIB_FFMPEG::AVStream* stream;
	LIB_FFMPEG::AVStream* bestStream  = NULL;
	int64_t               bestQuality = 0;

	for (int i = 0; i < (int)formatContext->nb_streams; i++)
	{
		stream = formatContext->streams[i];

		if ((stream != NULL) && (stream->codec != NULL) && ((VM_MediaType)stream->codec->codec_type == mediaType))
		{
			switch (mediaType) {
			case MEDIA_TYPE_AUDIO:
				quality = ((int64_t)stream->codec->channels * (int64_t)stream->codec->sample_rate);

				if ((bestStream == NULL) || (quality > bestQuality)) {
					bestStream  = stream;
					bestQuality = quality;
				}

				break;
			case MEDIA_TYPE_VIDEO:
				quality = ((int64_t)stream->codec->width * (int64_t)stream->codec->height);

				if ((bestStream == NULL) || (quality > bestQuality)) {
					bestStream  = stream;
					bestQuality = quality;
				}

				break;
			default:
				if (bestStream == NULL)
					bestStream = stream;

				break;
			}
		}
	}

	return (bestStream != NULL ? VM_FileSystem::GetMediaStreamByIndex(formatContext, bestStream->index) : NULL);
}

LIB_FFMPEG::AVStream* System::VM_FileSystem::GetMediaStreamByIndex(LIB_FFMPEG::AVFormatContext* formatContext, int streamIndex)
{
	if ((formatContext == NULL) || (formatContext->nb_streams == 0) || 
		(streamIndex < 0) || (streamIndex >= (int)formatContext->nb_streams))
	{
		return NULL;
	}

	LIB_FFMPEG::AVStream* stream = formatContext->streams[streamIndex];

	if ((stream == NULL) || (stream->codec == NULL) || (stream->codec->codec_id == LIB_FFMPEG::AV_CODEC_ID_NONE))
		return NULL;

	LIB_FFMPEG::AVCodec* decoder = LIB_FFMPEG::avcodec_find_decoder(stream->codec->codec_id);

	if (decoder == NULL)
		return NULL;

	stream->codec->codec_id = decoder->id;

	int maxThreads = 1;

	// MULTI-THREADING MUST BE DISABLED FOR SOME MUSIC COVERS (PNG)
	if (stream->codec->codec_id != LIB_FFMPEG::AV_CODEC_ID_PNG)
		maxThreads = min(SDL_GetCPUCount(), MAX_DECODE_THREADS);

	stream->codec->thread_count = maxThreads;

	if (LIB_FFMPEG::avcodec_open2(stream->codec, decoder, NULL) < 0)
		return NULL;

	stream->codec->thread_count = maxThreads;

	// http://x265.readthedocs.io/en/default/presets.html
	if ((stream->codec->priv_data != NULL) && 
		((stream->codec->codec_id == LIB_FFMPEG::AV_CODEC_ID_H264) || 
		(stream->codec->codec_id  == LIB_FFMPEG::AV_CODEC_ID_HEVC)))
	{
		LIB_FFMPEG::av_opt_set(stream->codec->priv_data, "preset", "ultrafast",   0);
		LIB_FFMPEG::av_opt_set(stream->codec->priv_data, "tune",   "zerolatency", 0);
	}

	if (stream->codec->pix_fmt == LIB_FFMPEG::AV_PIX_FMT_NONE)
	{
		if ((VM_MediaType)stream->codec->codec_type == MEDIA_TYPE_SUBTITLE)
			stream->codec->pix_fmt = LIB_FFMPEG::AV_PIX_FMT_PAL8;
		else if ((VM_MediaType)stream->codec->codec_type == MEDIA_TYPE_VIDEO)
			stream->codec->pix_fmt = LIB_FFMPEG::AV_PIX_FMT_YUV420P;
	}

	stream->discard = LIB_FFMPEG::AVDISCARD_DEFAULT;

	return stream;
}

int System::VM_FileSystem::GetMediaStreamCount(LIB_FFMPEG::AVFormatContext* formatContext, VM_MediaType mediaType)
{
	if (formatContext == NULL)
		return 0;

	// PERFORM AN EXTRA SCAN IF NO STREAMS WERE FOUND IN THE INITIAL SCAN
	if (formatContext->nb_streams == 0) {
		if (avformat_find_stream_info(formatContext, NULL) < 0)
			return 0;
	}

	int streamCount = 0;

	for (uint32_t i = 0; i < formatContext->nb_streams; i++)
	{
		LIB_FFMPEG::AVStream* stream = formatContext->streams[i];

		if ((stream == NULL) || (stream->codec == NULL) || (stream->codec->codec_id == LIB_FFMPEG::AV_CODEC_ID_NONE) ||
			((mediaType == MEDIA_TYPE_VIDEO) && (stream->attached_pic.buf != NULL)))
		{
			continue;
		}

		if ((VM_MediaType)stream->codec->codec_type == mediaType)
			streamCount++;
	}

	return streamCount;
}

int System::VM_FileSystem::GetMediaStreamCountSubs()
{
	int streamCount = 0;

	for (int i = 0; i < (int)VM_Player::SubsExternal.size(); i++)
	{
		LIB_FFMPEG::AVFormatContext* formatContext = VM_FileSystem::GetMediaFormatContext(VM_Player::SubsExternal[i], true);

		if (formatContext != NULL) {
			streamCount += formatContext->nb_streams;
			VM_FileSystem::CloseMediaFormatContext(formatContext, -1);
		}
	}

	return streamCount;
}

String System::VM_FileSystem::GetMediaStreamDescriptionAudio(LIB_FFMPEG::AVStream* stream)
{
	if ((stream == NULL) || (stream->codec == NULL))
		return "";

	String description = String("[#" + std::to_string(stream->index) + "] ");

	if (stream->codec->codec_descriptor == NULL)
	{
		LIB_FFMPEG::AVCodec* decoder = avcodec_find_decoder(stream->codec->codec_id);

		if (decoder != NULL)
			description.append(VM_Text::ToUpper(decoder->name));
	} else {
		description.append(VM_Text::ToUpper(stream->codec->codec_descriptor->name));
	}

	description.append(" ");

	String channelLayout = VM_Text::GetMediaAudioLayoutString(stream->codec->channels);
	String sampleRate    = VM_Text::GetMediaSampleRateString(stream->codec->sample_rate);
	String bitRate       = VM_Text::GetMediaBitRateString((int)stream->codec->bit_rate);

	description.append(!channelLayout.empty() && (channelLayout != "N/A") ? channelLayout + " " : "");
	description.append(!sampleRate.empty()    && (sampleRate    != "N/A") ? sampleRate    + " " : "");
	description.append(!bitRate.empty()       && (bitRate       != "N/A") ? bitRate       + " " : "");

	LIB_FFMPEG::AVDictionaryEntry* language = NULL;

	if (stream->metadata != NULL)
		language = av_dict_get(stream->metadata, "language", NULL, 0);

	if ((language != NULL) && (strcmp(language->value, "und") != 0))
		description.append("(").append(VM_Text::GetLanguage(language->value)).append(")");

	// http://wiki.multimedia.cx/index.php?title=FFmpeg_Metadata

	return description;
}

String System::VM_FileSystem::GetMediaStreamDescriptionSub(LIB_FFMPEG::AVStream* stream, bool external)
{
	if ((stream == NULL) || (stream->codec == NULL))
		return "";

	// Codec name such as HDMV_PGS_SUB etc.
	String description = String("[#" + std::to_string(stream->index + (external ? SUB_STREAM_EXTERNAL : 0)) + "] ");

	if (stream->codec->codec_descriptor == NULL)
	{
		LIB_FFMPEG::AVCodec* decoder = avcodec_find_decoder(stream->codec->codec_id);

		if (decoder != NULL)
			description.append(VM_Text::ToUpper(decoder->name));
	} else {
		description.append(VM_Text::ToUpper(stream->codec->codec_descriptor->name));
	}

	description.append(" ");

	LIB_FFMPEG::AVDictionaryEntry* language = NULL;

	if (stream->metadata != NULL)
		language = av_dict_get(stream->metadata, "language", NULL, 0);

	if ((language != NULL) && (strcmp(language->value, "und") != 0))
		description.append("(").append(VM_Text::GetLanguage(language->value)).append(")");

	return description;
}

String System::VM_FileSystem::GetMediaStreamDescriptionVideo(LIB_FFMPEG::AVStream* stream)
{
	String description = "";

	if ((stream == NULL) || (stream->codec == NULL))
		return "";

	String res = VM_FileSystem::getMediaResolutionStringVideo(stream);
	String fps = VM_Text::GetMediaFrameRateString(MATH_ROUND_UP(VM_FileSystem::GetMediaFrameRate(stream)));
	String bps = VM_Text::GetMediaBitRateString((const int)stream->codec->bit_rate);

	// Codec name such as MP4, AVI, MPEG2 etc.
	description.append("[#" + std::to_string(stream->index) + "] ");
	description.append(VM_Text::ToUpper(stream->codec->codec_descriptor->name)).append(" ");
	description.append(!res.empty() && (res != "N/A") ? res + " " : "");
	description.append(!fps.empty() && (fps != "N/A") ? fps + " " : "");
	description.append(!bps.empty() && (bps != "N/A") ? bps + " " : "");

	return description;
}

LIB_FFMPEG::AVStream* System::VM_FileSystem::GetMediaStreamFirst(LIB_FFMPEG::AVFormatContext* formatContext, VM_MediaType mediaType, bool open)
{
	if (formatContext == NULL)
		return NULL;

	LIB_FFMPEG::AVStream* stream;

	for (int i = 0; i < (int)formatContext->nb_streams; i++)
	{
		stream = formatContext->streams[i];

		if ((stream != NULL) && (stream->codec != NULL) && ((VM_MediaType)stream->codec->codec_type == mediaType))
			return (open ? VM_FileSystem::GetMediaStreamByIndex(formatContext, i) : stream);
	}

	return NULL;
}

std::vector<int> System::VM_FileSystem::GetMediaStreamIndices(LIB_FFMPEG::AVFormatContext* formatContext, VM_MediaType mediaType)
{
	std::vector<int> indices;

	if (formatContext == NULL)
		return indices;

	for (int i = 0; i < (int)formatContext->nb_streams; i++)
	{
		if ((formatContext->streams[i]        != NULL) &&
			(formatContext->streams[i]->codec != NULL) &&
			((VM_MediaType)formatContext->streams[i]->codec->codec_type == mediaType))
		{
			indices.push_back(i);
		}
	}

	return indices;
}

#if defined _ios
StringMap System::VM_FileSystem::GetMediaStreamMetaItunes(int id)
{
	StringMap    meta;
	VM_DBResult  rows;
	int          dbResult;
	VM_Database* db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

	if (DB_RESULT_OK(dbResult) && (id > 0))
		rows = db->getRows("SELECT name, full_path FROM MEDIA_FILES WHERE id=" + std::to_string(id) + ";", 1);

	DELETE_POINTER(db);

	if (rows.empty())
		return meta;

	String             title = rows[0]["name"];
	String             url   = rows[0]["full_path"];
	NSAutoreleasePool* pool  = [[NSAutoreleasePool alloc] init];
	MPMediaQuery*      query = [[MPMediaQuery alloc] init];

	if ((pool == nil) || (query == nil))
		return meta;

	[query addFilterPredicate:[MPMediaPropertyPredicate predicateWithValue:[[NSString alloc] initWithUTF8String:title.substr(9).c_str()] forProperty:MPMediaItemPropertyTitle comparisonType:MPMediaPredicateComparisonEqualTo]];

	for (MPMediaItem* item in [query items])
	{
		if (String([[[item valueForProperty:MPMediaItemPropertyAssetURL] absoluteString] UTF8String]) != url)
			continue;

		NSString* titleNS  = [item valueForProperty:MPMediaItemPropertyTitle];
		NSString* artistNS = [item valueForProperty:MPMediaItemPropertyArtist];
		NSString* albumNS  = [item valueForProperty:MPMediaItemPropertyAlbumTitle];
		NSString* genreNS  = [item valueForProperty:MPMediaItemPropertyGenre];
		NSDate*   dateNS   = [item valueForProperty:MPMediaItemPropertyReleaseDate];

		if (titleNS != NULL)
			meta["title"] = String([titleNS UTF8String]);
		
		if (artistNS != NULL)
			meta["artist"] = String([artistNS UTF8String]);

		if (albumNS != NULL)
			meta["album"] = String([albumNS UTF8String]);
		
		if (genreNS != NULL)
			meta["genre"] = String([genreNS UTF8String]);
		
		NSDateFormatter* dateFormatter = [[NSDateFormatter alloc] init];

		if ((dateNS != NULL) && (dateFormatter != NULL)) {
			[dateFormatter setDateFormat:@"yyyy"];
			meta["date"] = String([[dateFormatter stringFromDate:dateNS] UTF8String]);
		}

		break;
	}

	[pool release];

	return meta;
}
#endif

StringMap System::VM_FileSystem::GetMediaStreamMeta(LIB_FFMPEG::AVFormatContext* formatContext)
{
	StringMap meta;

	if ((formatContext == NULL) || (formatContext->metadata == NULL))
		return meta;

	LIB_FFMPEG::AVDictionaryEntry* dictEntry;
	String                         entries[] = { "title", "artist", "album", "genre", "date" };

	// http://wiki.multimedia.cx/index.php?title=FFmpeg_Metadata
	for (const auto &entry : entries)
	{
		dictEntry = LIB_FFMPEG::av_dict_get(formatContext->metadata, entry.c_str(), NULL, 0);

		if (dictEntry != NULL)
			meta[entry] = dictEntry->value;
	}

	return meta;
}

Strings System::VM_FileSystem::GetMediaSubFiles(const String &videoFilePath)
{
	String  directory, idxFile = "", videoFileName;
	Strings filesInDirectory, subtitleFiles;

	directory        = videoFilePath.substr(0, videoFilePath.rfind(PATH_SEPERATOR));
	filesInDirectory = VM_FileSystem::GetDirectoryFiles(directory);
	videoFileName    = VM_Text::ToUpper(VM_FileSystem::GetFileName(videoFilePath, true));

	for (const auto &file : filesInDirectory)
	{
		if (!VM_FileSystem::isSubtitle(file) || (VM_Text::ToUpper(file).find(videoFileName) == String::npos))
			continue;

		if (!idxFile.empty() && (VM_Text::ToUpper(VM_FileSystem::GetFileName(file, true)) == idxFile)) {
			idxFile = "";
			continue;
		}

		subtitleFiles.push_back(VM_Text::Format("%s%s%s", directory.c_str(), PATH_SEPERATOR, file.c_str()));

		if (VM_FileSystem::GetFileExtension(file, true) == "IDX")
			idxFile = VM_Text::ToUpper(VM_FileSystem::GetFileName(file, true));
	}

	return subtitleFiles;
}

VM_MediaType System::VM_FileSystem::GetMediaType(LIB_FFMPEG::AVFormatContext* formatContext)
{
	if (formatContext == NULL)
		return MEDIA_TYPE_UNKNOWN;

	int audioStreamCount, subStreamCount, videoStreamCount;

	audioStreamCount = VM_FileSystem::GetMediaStreamCount(formatContext, MEDIA_TYPE_AUDIO);
	subStreamCount   = VM_FileSystem::GetMediaStreamCount(formatContext, MEDIA_TYPE_SUBTITLE);
	videoStreamCount = VM_FileSystem::GetMediaStreamCount(formatContext, MEDIA_TYPE_VIDEO);

	if ((audioStreamCount > 0) && (videoStreamCount == 0) && (subStreamCount == 0))
		return MEDIA_TYPE_AUDIO;
	else if ((subStreamCount > 0) && (audioStreamCount == 0) && (videoStreamCount == 0))
		return MEDIA_TYPE_SUBTITLE;
	else if ((audioStreamCount > 0) && (videoStreamCount > 0))
		return MEDIA_TYPE_VIDEO;

	return MEDIA_TYPE_UNKNOWN;
}

Strings System::VM_FileSystem::GetNetworkInterfaces()
{
	Strings interfaces;

	#if defined _windows
		WSADATA winSockData;

		if (WSAStartup(MAKEWORD(2, 2), &winSockData) != 0)
			return interfaces;

		const ULONG MAX_ITERATIONS = 3;
		const ULONG FLAGS          = (GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_UNICAST);

		ULONG bufferSize = 15 * KILO_BYTE;
		ULONG iteration  = 0;

		PIP_ADAPTER_ADDRESSES addresses;
		DWORD                 result;

		do {
			addresses = (IP_ADAPTER_ADDRESSES*)malloc(bufferSize);
			result    = GetAdaptersAddresses(AF_INET, FLAGS, NULL, addresses, &bufferSize);

			iteration++;

			if (result == ERROR_BUFFER_OVERFLOW) {
				FREE_POINTER(addresses);
				continue;
			}

			for (PIP_ADAPTER_ADDRESSES address = addresses; address != NULL; address = address->Next) {
				if ((address->OperStatus == IfOperStatusUp) && (address->IfType != IF_TYPE_SOFTWARE_LOOPBACK))
					interfaces.push_back(VM_Text::ToUTF8(address->FriendlyName));
			}

			break;
		} while ((result == ERROR_BUFFER_OVERFLOW) && (iteration < MAX_ITERATIONS));

		FREE_POINTER(addresses);

		WSACleanup();
	#else
		String   ifName;
		ifaddrs* addresses = NULL, *address;

		if (getifaddrs(&addresses) == 0)
		{
			for (address = addresses; address != NULL; address = address->ifa_next)
			{
				if ((address->ifa_name == NULL) || (address->ifa_addr == NULL) || (address->ifa_addr->sa_family != AF_INET))
					continue;

				#if defined _ios || defined _macosx
					ifName = String([[NSString stringWithUTF8String:address->ifa_name] UTF8String]);
				#else
					ifName = String(address->ifa_name);
				#endif

				if (ifName.substr(0, 2) != "lo")
					interfaces.push_back(ifName);
			}
		}

		freeifaddrs(addresses);
	#endif

	return interfaces;
}

Strings System::VM_FileSystem::getOpticalFileBluray(const String &path)
{
	String  bdmvPath;
	Strings files;

	if (path.find("BDMV") != String::npos)
		bdmvPath = String(path).substr(0, path.find("BDMV") + 4).append(PATH_SEPERATOR);
	else if (VM_Text::GetLastCharacter(path) != PATH_SEPERATOR_C)
		bdmvPath = String(path + PATH_SEPERATOR + "BDMV" + PATH_SEPERATOR);
	else
		bdmvPath = String(path + "BDMV" + PATH_SEPERATOR);

	// X:/BDMV/PLAYLIST/*.mpls | X:/BDMV/STREAM/*.m2ts
	String  metaFile     = String(bdmvPath + "META"     + PATH_SEPERATOR + "DL" + PATH_SEPERATOR + "bdmt_eng.xml");
	String  playlistPath = String(bdmvPath + "PLAYLIST" + PATH_SEPERATOR);
	String  streamPath   = String(bdmvPath + "STREAM"   + PATH_SEPERATOR);
	Strings mplsFiles    = VM_FileSystem::GetDirectoryFiles(playlistPath);

	// http://www.ezr8.com/bdmv.html

	// FIND THE MOVIE PLAYLIST WITH THE LONGEST DURATION
	for (const auto &mplsFile : mplsFiles)
	{
		FILE* mpls = fopen(String(playlistPath + mplsFile).c_str(), "rb");

		if (mpls == NULL)
			continue;

		// https://en.wikibooks.org/wiki/User:Bdinfo/mpls

		char    byteString[DEFAULT_CHAR_BUFFER_SIZE];
		int64_t mplsDuration = 0;
		String  trackString  = "";

		// MPLS | 4D 50 4C 53
		std::fgets(byteString, 5, mpls);
		byteString[4] = '\0';

		if (strncmp(byteString, "MPLS", 4) != 0) {
			CLOSE_FILE(mpls);
			continue;
		}

		// GET THE OFFSET TO THE PLAYLIST SECTION (0x08)
		fseek(mpls, 0x08, SEEK_SET);

		uint32_t m2tsSectionOffset = (std::fgetc(mpls) * FOURTH_BYTE + std::fgetc(mpls) * THIRD_BYTE + std::fgetc(mpls) * SECOND_BYTE + std::fgetc(mpls) * FIRST_BYTE);

		// SEEK TO THE PLAYLIST SECTION (SKIP FIRST 6 BYTES) AND GET THE M2TS COUNT
		fseek(mpls, (m2tsSectionOffset + 4 + 2), SEEK_SET);

		uint32_t m2tsCount = (std::fgetc(mpls) * SECOND_BYTE + std::fgetc(mpls) * FIRST_BYTE);

		// SKIP 2 BYTES (NR OF SUBPATHS)
		fseek(mpls, 2, SEEK_CUR);

		// CONSTRUCT THE CONCATENATED FILEPATH:
		// "concat:streamPath|stream1|stream2|streamN|duration|title|audioTrackCount|subTrackCount|"
		String file          = ("concat:" + streamPath + "|");
		int    nrAudioTracks = 0;
		int    nrSubTracks   = 0;

		// ITERATE THE M2TS STREAMS
		for (uint32_t i = 0; i < m2tsCount; i++)
		{
			// PLAYLIST ITEM:
			// Length				2 bytes
			// PlayItem				9 bytes	(M2TS)
			// ConnectionCondition	2 bytes
			// Reserved				1 byte
			// TimeIn				4 bytes (time offset) (start)
			// TimeOut				4 bytes	(duration)    (end)
			// Unknown				17 bytes
			// AudioTrackCount		1 byte
			// SubTrackCount		1 byte
			// Unknown				38 bytes
			// <AudioTrack#			3 bytes (eng)>
			// <Unknown				13 bytes>
			// Unknown				12 bytes
			// <SubTrack#			3 bytes (eng)>
			// <Unknown				13 byte>

			// GET THE LENGTH (IN BYTES) OF THE M2TS DETAILS
			int m2tsLength = (std::fgetc(mpls) * SECOND_BYTE + std::fgetc(mpls) * FIRST_BYTE);

			// GET M2TS FILE NAME (5 + 4 BYTES) AND ADD IT TO THE CONCATENATED FILEPATH
			std::fgets(byteString, 6, mpls); byteString[5] = '\0';

			if (file.rfind(byteString) == String::npos)
				file.append(byteString).append(".m2ts").append("|");

			fseek(mpls, 4, SEEK_CUR);
			
			m2tsLength -= 9;

			// SKIP 3 BYTES
			fseek(mpls, (2 + 1), SEEK_CUR);
			
			m2tsLength -= 3;

			// GET THE TIME OFFSET (4 BYTES): c1*256^3 + c2*256^2 + c3*256^1 + c4*256^0
			int m2tsDurationOffset = (std::fgetc(mpls) * FOURTH_BYTE + std::fgetc(mpls) * THIRD_BYTE + std::fgetc(mpls) * SECOND_BYTE + std::fgetc(mpls) * FIRST_BYTE);

			m2tsLength -= 4;

			// GET THE DURATION (4 BYTES): c1*256^3 + c2*256^2 + c3*256^1 + c4*256^0
			int m2tsDuration = (std::fgetc(mpls) * FOURTH_BYTE + std::fgetc(mpls) * THIRD_BYTE + std::fgetc(mpls) * SECOND_BYTE + std::fgetc(mpls) * FIRST_BYTE);
			m2tsDuration    -= m2tsDurationOffset;

			m2tsLength -= 4;

			// ADD THE MARK DURATION TO THE TOTAL PLAYLIST DURATION
			if (m2tsDuration > 0)
				mplsDuration += m2tsDuration;

			// ADD THE AUDIO/SUB TRACK LANGUAGES
			if (trackString.empty())
			{
				// SKIP 17 BYTES
				fseek(mpls, 17, SEEK_CUR);
				m2tsLength -= 17;

				// GET THE AUDIO/SUB TRACK COUNT
				nrAudioTracks = std::fgetc(mpls);
				nrSubTracks   = std::fgetc(mpls);

				m2tsLength -= 2;

				// SEEK TO THE FIRST AUDIO TRACK (SKIP 38 BYTES)
				if (nrAudioTracks > 0) {
					fseek(mpls, 38, SEEK_CUR);
					m2tsLength -= 38;
				}

				// ADD THE AUDIO TRACKS
				for (int i = 0; i < nrAudioTracks; i++)
				{
					std::fgets(byteString, 4, mpls);
					byteString[3] = '\0';

					m2tsLength -= 3;

					trackString.append(strlen(byteString) > 0 ? byteString : "N/A").append(",");

					if (i < nrAudioTracks - 1) {
						fseek(mpls, 13, SEEK_CUR);
						m2tsLength -= 13;
					}
				}

				trackString.append(nrAudioTracks > 0 ? "|" : "0|");

				// SEEK TO THE FIRST SUB TRACK (SKIP 12 BYTES)
				if (nrSubTracks > 0) {
					fseek(mpls, 12, SEEK_CUR);
					m2tsLength -= 12;
				}

				// ADD THE SUB TRACKS
				for (int i = 0; i < nrSubTracks; i++)
				{
					std::fgets(byteString, 4, mpls);
					byteString[3] = '\0';

					m2tsLength -= 3;

					trackString.append(strlen(byteString) > 0 ? byteString : "N/A").append(",");

					if (i < nrSubTracks - 1) {
						fseek(mpls, 13, SEEK_CUR);
						m2tsLength -= 13;
					}
				}

				trackString.append(nrSubTracks > 0 ? "|" : "0|");
			}

			// SEEK TO THE NEXT M2TS STREAM
			fseek(mpls, m2tsLength, SEEK_CUR);
		}

		CLOSE_FILE(mpls);

		// MPLS DURATIONS ARE MEASURED IN 45000'THS OF A SECOND
		mplsDuration /= 45000ll;

		if ((mplsDuration > 0) && (nrAudioTracks > 0))
		{
			// CONVERT DURATION TO FFMPEG'S AV_TIME_BASE/PTS/DTS
			mplsDuration *= AV_TIME_BASE_I64;

			// ADD THE TOTAL PLAYLIST DURATION TO THE CONCATENATED FILEPATH
			file.append(std::to_string(mplsDuration)).append("|");

			// TRY OPENING THE META FILE IF IT EXISTS AND GET THE MOVIE TITLE
			String        metaTitle;
			std::ifstream meta(metaFile);

			if (meta.good())
			{
				while (std::getline(meta, metaTitle))
				{
					size_t pos = metaTitle.find("<di:name>");

					if (pos != String::npos) {
						metaTitle = metaTitle.substr(pos + 9);
						metaTitle = metaTitle.substr(0, metaTitle.find("</di:name>"));
						break;
					}
				}

				meta.close();
			}

			// ADD ALTERNATIVE MOVIE TITLE IF MAIN TITLE WAS NOT FOUND
			String driveName  = VM_FileSystem::getDriveName(bdmvPath);
			String movieTitle = "";

			if (!metaTitle.empty() && !driveName.empty() && (metaTitle != driveName))
				movieTitle = (metaTitle + " (" + driveName + ")");
			else
				movieTitle = driveName;

			file.append("[Bluray-BDMV-" + VM_FileSystem::GetFileName(mplsFile, true) + "] " + movieTitle + "|");

			// ADD THE AUDIO/SUB TRACK LANGUAGES TO THE CONCATENATED FILEPATH
			file.append(trackString);

			// ADD THE FILE TO THE LIST
			files.push_back(file);
		}
	}

	return files;
}

Strings System::VM_FileSystem::getOpticalFileDVD(const String &path)
{
	Strings files;
	String  vtsPath;
	String  providerID = "";

	// X:/VIDEO_TS/*.ifo | X:/VIDEO_TS/*.vob
	if (path.find("VIDEO_TS") != String::npos)
		vtsPath = String(path).substr(0, path.find("VIDEO_TS") + 8).append(PATH_SEPERATOR);
	else if (VM_Text::GetLastCharacter(path) != PATH_SEPERATOR_C)
		vtsPath = String(path + PATH_SEPERATOR + "VIDEO_TS" + PATH_SEPERATOR);
	else
		vtsPath = String(path + "VIDEO_TS" + PATH_SEPERATOR);

	Strings vtsFiles = VM_FileSystem::GetDirectoryFiles(vtsPath);

	// FIND THE MOVIE PLAYLIST WITH THE LONGEST DURATION
	for (const auto &vtsFile : vtsFiles)
	{
		// Only parse IFO files
		if (VM_FileSystem::GetFileExtension(vtsFile, true) != "IFO")
			continue;

		FILE* ifo = fopen(String(vtsPath + vtsFile).c_str(), "rb");

		if (ifo == NULL)
			continue;

		// OFFSET BY SECTOR = 2048 BYTES PER SECTOR
		// https://en.wikibooks.org/wiki/Inside_DVD-Video/Directory_Structure
		// http://www.mpucoder.com/DVD/ifo.html
		// http://www.ifoedit.com/ifovts.html
		// http://forum.doom9.org/archive/index.php/t-112051.html
		// DVDVIDEO-VMG | 44 56 44 56 49 44 45 4F 2D 56 4D 47
		// DVDVIDEO-VTS | 44 56 44 56 49 44 45 4F 2D 56 54 53
		char byteString[DEFAULT_CHAR_BUFFER_SIZE];

		std::fgets(byteString, 13, ifo);
		byteString[12] = '\0';

		// VIDEO_TS.IFO (MENU AND DISC INFO)
		if (strncmp(byteString, "DVDVIDEO-VMG", 12) == 0)
		{
			// SEEK TO PROVIDER ID (POSITION 0x0040)
			fseek(ifo, 0x0040, SEEK_SET);

			// SAVE THE PROVIDER ID IF IT EXISTS (SOMETIMES THE DISC/MOVIE NAME)
			std::fgets(byteString, 33, ifo);
			byteString[32] = '\0';

			if (strlen(byteString) > 0)
				providerID = VM_Text::Trim(byteString);
		}
		// VIDEO_TS_XX_XX.VOB (VTS/VIDEO TITLE SET)
		else if (strncmp(byteString, "DVDVIDEO-VTS", 12) == 0)
		{
			// http://www.mpucoder.com/DVD/ifo.html
			// SEEK TO GET THE VTS_PGCI SECTOR POINTER (OFFSET LOCATION) (POSITION 0x00CC)
			// OFFSET BY SECTOR = 2048 BYTES PER SECTOR
			fseek(ifo, 0x00CC, SEEK_SET);

			uint32_t offsetVTS_PGCI = (std::fgetc(ifo) * FOURTH_BYTE + std::fgetc(ifo) * THIRD_BYTE + std::fgetc(ifo) * SECOND_BYTE + std::fgetc(ifo) * FIRST_BYTE);
			offsetVTS_PGCI     *= 2048;

			// http://stnsoft.com/DVD/ifo.html#vidatt
			// SEEK TO VIDEO ATTRIBUTES OF VTS_VOBS (0x0200)
			fseek(ifo, 0x0200, SEEK_SET);

			// GET VIDEO STANDARD (BIT POSITION 4): NTSC=0 (0xC0), PAL=1 (0x40)
			int videoStandard = BIT_AT_POS(std::fgetc(ifo), 4);
			
			// ADD THE AUDIO/SUB TRACK LANGUAGES
			String trackString = "";

			// http://stnsoft.com/DVD/ifo.html#audatt
			// GET THE AUDIO TRACK COUNT (POSITION 0x202)
			fseek(ifo, 0x202, SEEK_SET);

			int nrAudioTracks = (std::fgetc(ifo) * SECOND_BYTE + std::fgetc(ifo) * FIRST_BYTE);

			// ADD THE AUDIO TRACKS
			for (int i = 0; i < nrAudioTracks; i++)
			{
				fseek(ifo, 2, SEEK_CUR);

				std::fgets(byteString, 3, ifo);
				byteString[2] = '\0';

				trackString.append(strlen(byteString) > 0 ? byteString : "N/A").append(",");

				fseek(ifo, 4, SEEK_CUR);
			}
			
			trackString.append(nrAudioTracks > 0 ? "|" : "0|");

			// http://stnsoft.com/DVD/ifo.html#spatt
			// GET THE SUB TRACK COUNT (POSITION 0x254)
			fseek(ifo, 0x254, SEEK_SET);

			int nrSubTracks = (std::fgetc(ifo) * SECOND_BYTE + std::fgetc(ifo) * FIRST_BYTE);

			// ADD THE SUB TRACKS
			for (int i = 0; i < nrSubTracks; i++)
			{
				fseek(ifo, 2, SEEK_CUR);

				std::fgets(byteString, 3, ifo);
				byteString[2] = '\0';

				trackString.append(strlen(byteString) > 0 ? byteString : "N/A").append(",");

				fseek(ifo, 2, SEEK_CUR);
			}

			trackString.append(nrSubTracks > 0 ? "|" : "0|");

			// http://stnsoft.com/DVD/ifo_vts.html#pgci
			// SEEK TO VTS_PGCI (TITLE PROGRAM CHAIN TABLE) (POSITION 0x1000)
			fseek(ifo, offsetVTS_PGCI, SEEK_SET);

			int nrOfProgramChains = (std::fgetc(ifo) * SECOND_BYTE + std::fgetc(ifo) * FIRST_BYTE);

			fseek(ifo, (2 + 4 + 4), SEEK_CUR);

			// SEEK TO FIRST VTS_PGC (PROGRAM CHAIN CATEGORY)
			uint32_t offsetVTS_PGC = (std::fgetc(ifo) * FOURTH_BYTE + std::fgetc(ifo) * THIRD_BYTE + std::fgetc(ifo) * SECOND_BYTE + std::fgetc(ifo) * FIRST_BYTE);
			offsetVTS_PGC     += offsetVTS_PGCI;

			fseek(ifo, offsetVTS_PGC, SEEK_SET);

			// http://stnsoft.com/DVD/pgc.html
			// CALCULATE PLAYLIST DURATION (4 BYTES) (hh:mm:ss.frames/fps)
			int64_t playlistSizeTotal = 0;

			// CALCULATE THE TOTAL SUM OF ALL PROGRAM CHAINS OF THE VTS FILE
			// ONE VTS FILE (VTS_01_0.IFO) CAN CONTAIN MULTIPLE VOB FILES (VTS_01_x.VOB)
			// ONE VOB FILE CAN CONTAIN MULTIPLE PROGRAM CHAINS
			// (EACH PROGRAM CHAIN CAN CONTAIN MULTIPLE PROGRAMS AND CELLS)
			for (int i = 0; i < nrOfProgramChains; i++)
			{
				fseek(ifo, 2 + 1, SEEK_CUR);		// 2 = N/A, 1 = NR OF PROGRAMS

				int nrOfCells = std::fgetc(ifo);	// 1 = NR OF CELLS

				std::fgets(byteString, 5, ifo);		// 4 = PLAYBACK TIME (BCD)
				byteString[4] = '\0';

				// CALCULATE PLAYLIST DURATION (4 BYTES) (hh:mm:ss.frames/fps)
				int64_t playlistSize = (int64_t)((DEC_TO_HEX(byteString[0]) * ONE_HOUR_S + DEC_TO_HEX(byteString[1]) * ONE_MINUTE_S + DEC_TO_HEX(byteString[2])) * AV_TIME_BASE_I64);
				playlistSize        += (videoStandard == 0 ? (DEC_TO_HEX(byteString[3] - 0xC0) * AV_TIME_BASE_I64 / 30) : (DEC_TO_HEX(byteString[3] - 0x40) * AV_TIME_BASE_I64 / 25));
				playlistSizeTotal   += playlistSize;

				// http://stnsoft.com/DVD/pgc.html#pos
				// SEEK TO GET OFFSET WITHIN PGC TO CEL POSITION INFORMATION TABLE (OFFSET 0xEA FROM PGC)
				fseek(ifo, (offsetVTS_PGC + 0xEA), SEEK_SET);

				uint32_t offsetCellPosTale = (std::fgetc(ifo) * SECOND_BYTE + std::fgetc(ifo) * FIRST_BYTE);

				// SEEK TO NEXT PGC (SKIP 4 BYTES PER CELL)
				offsetVTS_PGC += (offsetCellPosTale + (nrOfCells * 4));

				fseek(ifo, offsetVTS_PGC, SEEK_SET);
			}

			// CONSTRUCT THE CONCATENATED FILEPATH - ADD THE VIDEO_TS PATH
			String file = ("concat:" + vtsPath + "|");

			// ADD THE VOB FILES
			for (const auto &vobFile : vtsFiles)
			{
				// EX: ADD VTS_09_x.VOB 
				if ((VM_FileSystem::GetFileExtension(vobFile, true) != "VOB") || (vobFile.substr(0, 6) != vtsFile.substr(0, 6)))
					continue;

				// EX: SKIP VTS_09_0.VOB (MENU)
				if (vobFile.substr(6, 3) != "_0.")
					file.append(vobFile + "|");
			}

			// ADD THE TOTAL PLAYLIST DURATION
			file.append(std::to_string(playlistSizeTotal) + "|");

			// ADD THE MOVIE TITLE
			String driveName  = VM_FileSystem::getDriveName(vtsPath);
			String movieTitle = "";

			if (!providerID.empty() && !driveName.empty() && (providerID != driveName))
				movieTitle = (providerID + " (" + driveName + ")");
			else
				movieTitle = driveName;

			file.append("[DVD-VIDEO_TS-" + vtsFile.substr(0, 6) + "] " + movieTitle + "|");

			// ADD THE AUDIO/SUB TRACK LANGUAGES
			file.append(trackString);

			// ADD THE FILE TO THE LIST
			files.push_back(file);
		}

		CLOSE_FILE(ifo);
	}

	return files;
}

String System::VM_FileSystem::GetPathDatabase()
{
	return VM_Text::Format("%s%cdb%c", VM_Window::WorkingDirectory.c_str(), PATH_SEPERATOR_C, PATH_SEPERATOR_C);
}

String System::VM_FileSystem::GetPathFont()
{
	return VM_Text::Format("%s%cfonts%c", VM_Window::WorkingDirectory.c_str(), PATH_SEPERATOR_C, PATH_SEPERATOR_C);
}

String System::VM_FileSystem::GetPathFontArial()
{
	#if defined _android
		return "/system/fonts/Arial.ttf";
	#elif defined _ios
		return "/System/Library/Fonts/Cache/Arial.ttf";
	#elif defined _linux
		return "/usr/share/fonts/truetype/msttcorefonts/arial.ttf";
	#elif defined _macosx
		return "/Library/Fonts/Arial.ttf";
	#elif defined _windows
		return "C:\\Windows\\Fonts\\arial.ttf";
	#endif
}

String System::VM_FileSystem::GetPathGUI()
{
	return VM_Text::Format("%s%cgui%c", VM_Window::WorkingDirectory.c_str(), PATH_SEPERATOR_C, PATH_SEPERATOR_C);
}

String System::VM_FileSystem::GetPathImages()
{
	return VM_Text::Format("%s%cimg%c", VM_Window::WorkingDirectory.c_str(), PATH_SEPERATOR_C, PATH_SEPERATOR_C);
}

String System::VM_FileSystem::GetPathLanguages()
{
	return VM_Text::Format("%s%clang%c", VM_Window::WorkingDirectory.c_str(), PATH_SEPERATOR_C, PATH_SEPERATOR_C);
}

String System::VM_FileSystem::GetPathThumbnailsDir()
{
	return VM_Text::Format("%s%cthumbs", VM_Window::WorkingDirectory.c_str(), PATH_SEPERATOR_C);
}

String System::VM_FileSystem::GetPathThumbnails(int mediaID)
{
	if (mediaID < 1)
		return "";

	return VM_Text::Format("%s%c%d.jpg", VM_FileSystem::GetPathThumbnailsDir().c_str(), PATH_SEPERATOR_C, mediaID);
}

String System::VM_FileSystem::GetPathThumbnails(const String &fileName)
{
	if (fileName.empty())
		return "";

	return VM_Text::Format("%s%c%s.jpg", VM_FileSystem::GetPathThumbnailsDir().c_str(), PATH_SEPERATOR_C, fileName.c_str());
}

#if defined _windows
WString System::VM_FileSystem::GetPathDatabaseW()
{
	return VM_Text::FormatW(L"%s%cdb%c", VM_Window::WorkingDirectoryW.c_str(), PATH_SEPERATOR_C, PATH_SEPERATOR_C);
}

WString System::VM_FileSystem::GetPathFontW()
{
	return VM_Text::FormatW(L"%s%cfonts%c", VM_Window::WorkingDirectoryW.c_str(), PATH_SEPERATOR_C, PATH_SEPERATOR_C);
}

WString System::VM_FileSystem::GetPathFontArialW()
{
	return L"C:\\Windows\\Fonts\\arial.ttf";
}

WString System::VM_FileSystem::GetPathGUIW()
{
	return VM_Text::FormatW(L"%s%cgui%c", VM_Window::WorkingDirectoryW.c_str(), PATH_SEPERATOR_C, PATH_SEPERATOR_C);
}

WString System::VM_FileSystem::GetPathImagesW()
{
	return VM_Text::FormatW(L"%s%cimg%c", VM_Window::WorkingDirectoryW.c_str(), PATH_SEPERATOR_C, PATH_SEPERATOR_C);
}

WString System::VM_FileSystem::GetPathLanguagesW()
{
	return VM_Text::FormatW(L"%s%clang%c", VM_Window::WorkingDirectoryW.c_str(), PATH_SEPERATOR_C, PATH_SEPERATOR_C);
}

WString System::VM_FileSystem::GetPathThumbnailsDirW()
{
	return VM_Text::FormatW(L"%s%cthumbs", VM_Window::WorkingDirectoryW.c_str(), PATH_SEPERATOR_C);
}

WString System::VM_FileSystem::GetPathThumbnailsW(int mediaID)
{
	if (mediaID < 1)
		return L"";

	return VM_Text::FormatW(L"%s%c%d.jpg", VM_FileSystem::GetPathThumbnailsDirW().c_str(), PATH_SEPERATOR_C, mediaID);
}

WString System::VM_FileSystem::GetPathThumbnailsW(const WString &fileName)
{
	if (fileName.empty())
		return L"";

	return VM_Text::FormatW(L"%s%c%s.jpg", VM_FileSystem::GetPathThumbnailsDirW().c_str(), PATH_SEPERATOR_C, fileName.c_str());
}
#endif

String System::VM_FileSystem::GetPathFromArgumentList(char* argv[], int argc)
{
	String filePath = "";

	#if defined _macosx
	if ((argc > 0) && (strncmp(argv[1], "-psn_0_", 7) == 0))
		return "";
	#endif

	for (int i = 1; i < argc; i++)
	{
		if ((argv[i] != NULL) && (strcmp(argv[i], "") != 0))
		{
			filePath.append(argv[i]);

			if (i != (argc - 1))
				filePath.append(" ");
		}
	}

	return filePath;
}

#if defined _windows
WString System::VM_FileSystem::GetPathFromArgumentList(wchar_t* argv[], int argc)
{
	WString filePath = L"";

	for (int i = 1; i < argc; i++)
	{
		if ((argv[i] != NULL) && (wcscmp(argv[i], L"") != 0))
		{
			filePath.append(argv[i]);

			if (i != (argc - 1))
				filePath.append(L" ");
		}
	}

	return filePath;
}
#endif

StringMap System::VM_FileSystem::GetShoutCastDetails(const String &stationName, int stationID)
{
	// http://wiki.shoutcast.com/wiki/SHOUTcast_Developer

	StringMap        details;
	String           apiKey   = VM_Text::Decrypt(SHOUTCAST_API_KEY);
	String           mediaURL = (SHOUTCAST_API_URL + "stationsearch?search=" + VM_Text::EscapeURL(stationName) + "&k=" + apiKey);
	LIB_XML::xmlDoc* document = VM_XML::Load(mediaURL.c_str());

	// FAILED TO LOAD XML FILE
	if (document == NULL)
		return details;

	LIB_XML::xmlNode* stationList = VM_XML::GetNode("/stationlist", document);

	if (stationList == NULL)
	{
		FREE_XML_DOC(document);
		LIB_XML::xmlCleanupParser();

		return details;
	}

	VM_XmlNodes stations = VM_XML::GetChildNodes(stationList, document);

	for (auto station : stations)
	{
		if ((station == NULL) || (strcmp(reinterpret_cast<const char*>(station->name), "station") != 0))
			continue;

		if (VM_XML::GetAttribute(station, "id") == std::to_string(stationID))
		{
			details["genre"]          = VM_XML::GetAttribute(station, "genre");
			details["bit_rate"]       = VM_XML::GetAttribute(station, "br");
			details["media_type"]     = VM_XML::GetAttribute(station, "mt");
			details["listener_count"] = VM_XML::GetAttribute(station, "lc");

			details["now_playing"] = VM_XML::GetAttribute(station, "ct");
			details["now_playing"] = VM_Text::Replace(details["now_playing"], "\\\"",   "\"");
			details["now_playing"] = VM_Text::Replace(details["now_playing"], "\\n\\n", "\n");
			details["now_playing"] = VM_Text::Replace(details["now_playing"], "\\n",    "\n");

			break;
		}
	}

	FREE_XML_DOC(document);
	LIB_XML::xmlCleanupParser();

	return details;
}

String System::VM_FileSystem::GetShoutCastStation(int stationID)
{
	if (stationID < 1)
		return "";

	// URL = https://yp.shoutcast.com<base>?id=[Station_id]

	const String API_URL           = "http://yp.shoutcast.com/sbin/tunein-station.";
	const int    NR_BASES          = 3;
	String       bases[NR_BASES]   = { "m3u", "pls", "xspf" };
	String       delims[NR_BASES]  = { "\n", "\n", ">" };
	int          offsets[NR_BASES] = { 0, 0, 6 };
	String       stationLink       = "";

	for (int i = 0; i < NR_BASES; i++)
	{
		String mediaURL = (API_URL + bases[i] + "?id=" + std::to_string(stationID));
		String response = VM_FileSystem::DownloadToString(mediaURL);

		if (response.empty())
			continue;

		Strings textSplit = VM_Text::Split(response, delims[i]);

		for (const auto &line : textSplit)
		{
			if (line.substr(offsets[i], 4) == "http") {
				stationLink = line.substr(offsets[i]);
				stationLink = stationLink.substr(0, stationLink.rfind("<"));
				break;
			}
		}

		if (!stationLink.empty())
			break;
	}

	return stationLink;
}

StringMap System::VM_FileSystem::GetTmdbDetails(int mediaID, VM_MediaType mediaType)
{
	StringMap details;

	if ((mediaID < 1) || ((mediaType != MEDIA_TYPE_TMDB_MOVIE) && (mediaType != MEDIA_TYPE_TMDB_TV)))
		return details;

	// https://developers.themoviedb.org/3/movies/get-movie-details

	String            apiKey     = VM_Text::Decrypt(TMDB_API_KEY);
	String            mediaTypeS = (mediaType == MEDIA_TYPE_TMDB_MOVIE ? "movie" : "tv");
	String            mediaURL   = (TMDB_API_URL + mediaTypeS + "/" + std::to_string(mediaID) + "?api_key=" + apiKey);
	String            response   = VM_FileSystem::DownloadToString(mediaURL);
	LIB_JSON::json_t* document   = VM_JSON::Parse(response.c_str());

	if (document == NULL)
		return details;

	// GENRES
	std::vector<LIB_JSON::json_t*> genresArray = VM_JSON::GetArray(VM_JSON::GetItem(document, "genres"));

	details["genres"] = "";

	for (int i = 0; i < (int)genresArray.size(); i++)
	{
		details["genres"].append(VM_JSON::GetValueString(VM_JSON::GetItem(genresArray[i], "name")));

		if (i < (int)genresArray.size() - 1)
			details["genres"].append(", ");
	}

	// OVERVIEW / DESCRIPTION
	details["overview"] = VM_JSON::GetValueString(VM_JSON::GetItem(document, "overview"));
	details["overview"] = VM_Text::Replace(details["overview"], "\\\"",   "\"");
	details["overview"] = VM_Text::Replace(details["overview"], "\\n\\n", "\n");
	details["overview"] = VM_Text::Replace(details["overview"], "\\n",    "\n");

	// RELEASE DATE
	if (mediaType == MEDIA_TYPE_TMDB_MOVIE)
		details["date"] = std::to_string(std::atoi(VM_JSON::GetValueString(VM_JSON::GetItem(document, "release_date")).c_str()));
	else
		details["date"] = std::to_string(std::atoi(VM_JSON::GetValueString(VM_JSON::GetItem(document, "first_air_date")).c_str()));

	// RATING
	const signed char star[4] = { 0xE2 - 256, 0x98 - 256, 0x85 - 256, 0 };
	double            voteAvg = VM_JSON::GetValueNumber(VM_JSON::GetItem(document, "vote_average"));
	int64_t           votes   = (int64_t)VM_JSON::GetValueNumber(VM_JSON::GetItem(document, "vote_count"));

	details["rating"] = VM_Text::Format("%s %.1f/10 (%lld %s)", star, voteAvg, votes, VM_Window::Labels["votes"].c_str());

	// DURATION
	if (mediaType == MEDIA_TYPE_TMDB_TV)
	{
		std::vector<double> durationArray = VM_JSON::GetArrayNumbers(VM_JSON::GetItem(document, "episode_run_time"));

		if (!durationArray.empty())
			details["duration"] = std::to_string((int64_t)durationArray[0] * 60);
		else
			details["duration"] = "0";
	} else {
		details["duration"] = std::to_string((int64_t)VM_JSON::GetValueNumber(VM_JSON::GetItem(document, "runtime")) * 60);
	}

	// TV SEASONS / EPISODES
	if (mediaType == MEDIA_TYPE_TMDB_TV) {
		details["seasons"]  = std::to_string((int64_t)VM_JSON::GetValueNumber(VM_JSON::GetItem(document, "number_of_seasons")));
		details["episodes"] = std::to_string((int64_t)VM_JSON::GetValueNumber(VM_JSON::GetItem(document, "number_of_episodes")));
	}

	// LANGUAGE
	details["language"] = VM_JSON::GetValueString(VM_JSON::GetItem(document, "original_language"));

	// BACKDROP IMAGE
	details["backdrop_url"] = VM_JSON::GetValueString(VM_JSON::GetItem(document, "backdrop_path"));

	if (!details["backdrop_url"].empty())
		details["backdrop_url"] = ("https://image.tmdb.org/t/p/original/" + details["backdrop_url"].substr(1));

	FREE_JSON_DOC(document);

	return details;
}

String System::VM_FileSystem::GetURL(VM_UrlType urlType, const String &data)
{
	String apiKey    = VM_Text::Decrypt(DROPBOX_API_KEY);
	String apiSecret = VM_Text::Decrypt(DROPBOX_API_SECRET);
	String mediaURL  = "";

	switch (urlType) {
	case URL_DROPBOX_AUTH_CODE:
		mediaURL = ("https://www.dropbox.com/1/oauth2/authorize?response_type=code&client_id=" + apiKey);
		break;
	case URL_DROPBOX_OAUTH2_TOKEN:
		mediaURL = "https://api.dropboxapi.com/1/oauth2/token";
		break;
	case URL_DROPBOX_OAUTH2_DATA:
		mediaURL = ("code=" + data + "&grant_type=authorization_code&client_id=" + apiKey + "&client_secret=" + apiSecret);
		break;
	case URL_DROPBOX_TOKEN_EXPIRED:
		mediaURL = "https://api.dropboxapi.com/2/users/get_current_account";
		break;
	case URL_DROPBOX_FILES:
		mediaURL = "https://api.dropboxapi.com/2/files/list_folder";
		break;
	}

	return mediaURL;
}

/*StringMap System::VM_FileSystem::GetYouTubeDetails(const String &mediaID)
{
	StringMap         details;
	String            apiKey   = VM_Text::Decrypt(YOUTUBE_API_KEY);
	String            mediaURL = (YOUTUBE_API_URL + "videos?part=snippet,contentDetails,statistics&id=" + mediaID + "&key=" + apiKey);
	String            response = VM_FileSystem::DownloadToString(mediaURL);
	LIB_JSON::json_t* document = VM_JSON::Parse(response.c_str());

	details["id"] = mediaID;

	if (document == NULL)
		return details;

	std::vector<LIB_JSON::json_t*> objectItems;
	LIB_JSON::json_t*              items      = VM_JSON::GetItem(document, "items");
	std::vector<LIB_JSON::json_t*> itemsArray = VM_JSON::GetArray(items);

	for (auto itemsObject : itemsArray)
	{
		objectItems = VM_JSON::GetItems(itemsObject);

		for (auto item : objectItems)
		{
			if (VM_JSON::GetKey(item) == "snippet")
			{
				details["channel"]     = VM_JSON::GetValueString(VM_JSON::GetItem(item->child, "channelTitle"));
				details["date"]        = VM_JSON::GetValueString(VM_JSON::GetItem(item->child, "publishedAt"));

				details["description"] = VM_JSON::GetValueString(VM_JSON::GetItem(item->child, "description"));
				details["description"] = VM_Text::Replace(details["description"], "\\\"",   "\"");
				details["description"] = VM_Text::Replace(details["description"], "\\n\\n", "\n");
				details["description"] = VM_Text::Replace(details["description"], "\\n",    "\n");
			}
			else if (VM_JSON::GetKey(item) == "contentDetails")
			{
				details["duration_yt"] = VM_JSON::GetValueString(VM_JSON::GetItem(item->child, "duration"));
			}
			else if (VM_JSON::GetKey(item) == "statistics")
			{
				details["views"] = VM_JSON::GetValueString(VM_JSON::GetItem(item->child, "viewCount"));

				String likeCount    = VM_Text::ToViewCount(std::atoll(VM_JSON::GetValueString(VM_JSON::GetItem(item->child, "likeCount")).c_str()));
				String dislikeCount = VM_Text::ToViewCount(std::atoll(VM_JSON::GetValueString(VM_JSON::GetItem(item->child, "dislikeCount")).c_str()));

				details["likes"] = VM_Text::Format("%s likes   %s dislikes", likeCount.c_str(), dislikeCount.c_str());

			}
		}
	}

	FREE_JSON_DOC(document);

	return details;
}

Strings System::VM_FileSystem::GetYouTubeVideos(const String &videoID)
{
	Strings links;

	if (videoID.empty())
		return links;

	String mediaURL = String("http://www.youtube.com/get_video_info?video_id=" + videoID + "&gl=US&hl=en");
	String response = VM_FileSystem::DownloadToString(mediaURL);

	// ERROR MESSAGE
	size_t findPos = response.find("reason=");

	if (findPos != String::npos)
	{
		String error = response.substr(findPos + 7);
		error = error.substr(0, error.find("%"));
		error = error.substr(0, error.find("&"));
		error = VM_Text::Replace(error, "+", " ");

		VM_Window::StatusString = error;
	}

	if ((response.find("status=ok") == String::npos) || (response.find("signature=True") != String::npos))
		return links;

	// TODO: Allow selecting 3D youtube video stream if found
	ints validItagIDs = {
		 22, // MP4  1280 x 720
		 18, // MP4   640 x 360
		 43, // WEBM  640 x 360
		  5, // FLV   400 x 240
		 36, // 3GP   320 x 240
		 17, // 3GP   176 x 144
		 84, // MP4  1280 x 720 3D
		 82, // MP4   640 x 360 3D
		100  // WEBM  640 x 360 3D
	};

	response = VM_Text::Replace(response, "http", "^");

	IntStringMap validItags, invalidTags;
	Strings      textSplit = VM_Text::Split(response, "^");

	for (const auto &line : textSplit)
	{
		int itagI = 0;
		findPos   = line.find("itag%253D");

		if (findPos != String::npos) {
			String itagS = line.substr(findPos + 9);
			itagI        = std::atoi(itagS.c_str());
		}

		if (itagI == 0)
			continue;

		String videoLink = "";

		if (line.find("%253A%252F%252Fr") != String::npos)
		{
			videoLink = String("http" + line);
			videoLink = VM_Text::Replace(videoLink, "%253A", ":");
			videoLink = VM_Text::Replace(videoLink, "%253F", "?");
			videoLink = VM_Text::Replace(videoLink, "%252F", "/");
			videoLink = VM_Text::Replace(videoLink, "%253D", "=");
			videoLink = VM_Text::Replace(videoLink, "%2526", "&");

			findPos = videoLink.find("%26");
			
			if (findPos != String::npos)
				videoLink = videoLink.substr(0, findPos);

			findPos = videoLink.find("%2C");

			if (findPos != String::npos)
				videoLink = videoLink.substr(0, findPos);

			videoLink = VM_Text::Replace(videoLink, "%2525", "%");
		}
		// LIVE STREAM - HLS PLAYLIST
		else if (line.find(".m3u8") != String::npos)
		{
			videoLink = String("http" + line);
			videoLink = VM_Text::Replace(videoLink, "%3A", ":");
			videoLink = VM_Text::Replace(videoLink, "%3F", "?");
			videoLink = VM_Text::Replace(videoLink, "%2F", "/");
			videoLink = VM_Text::Replace(videoLink, "%3D", "=");
			videoLink = VM_Text::Replace(videoLink, "%26", "&");
			videoLink = VM_Text::Replace(videoLink, "%25", "%");

			videoLink = videoLink.substr(0, videoLink.find(".m3u8") + 5);
		}

		if (VM_Text::VectorContains(validItagIDs, itagI))
			validItags[itagI] = videoLink;
		else
			invalidTags[itagI] = videoLink;
	}

	// ADD VALID LINKS SORTED BY ITAGS
	for (auto itag : validItagIDs) {
		if (!validItags[itag].empty())
			links.push_back(validItags[itag]);
	}

	// ADD INVALID (NON-OPTIMAL) LINKS
	for (const auto &link : invalidTags)
		links.push_back(link.second);

	return links;
}

String System::VM_FileSystem::GetYouTubeVideo(const String &videoID)
{
	VM_Player::State.urls = VM_FileSystem::GetYouTubeVideos(videoID);

	return (!VM_Player::State.urls.empty() ? VM_Player::State.urls[0] : "");
}*/

bool System::VM_FileSystem::hasFileExtension(const String &filePath)
{
	return (!VM_FileSystem::GetFileExtension(filePath, false).empty());
}

bool System::VM_FileSystem::HasInternetConnection()
{
	#if defined _android
		jclass    jniClass                 = VM_Window::JNI->getClass();
		JNIEnv*   jniEnvironment           = VM_Window::JNI->getEnvironment();
		jmethodID jniHasInternetConnection = jniEnvironment->GetStaticMethodID(jniClass, "HasInternetConnection", "()Z");

		if (jniHasInternetConnection == NULL)
			return false;

		return jniEnvironment->CallStaticBooleanMethod(jniClass, jniHasInternetConnection);
	#elif defined _ios || defined _macosx
		SCNetworkReachabilityFlags flags;
		SCNetworkReachabilityRef   address = SCNetworkReachabilityCreateWithName(NULL, "www.google.com");

		if (!address)
			return false;

		Boolean success   = SCNetworkReachabilityGetFlags(address, &flags);
		Boolean needsConn = (flags & kSCNetworkReachabilityFlagsConnectionRequired);
		Boolean reachable = (flags & kSCNetworkReachabilityFlagsReachable);

		CFRelease(address);

		return (success && reachable && !needsConn);
	#else
		Strings nics = VM_FileSystem::GetNetworkInterfaces();

		for (const auto &nic : nics) {
			if (VM_FileSystem::IsServerAccessible(GOOGLE_IP, nic))
				return true;
		}
	#endif

	return false;
}

void System::VM_FileSystem::InitFFMPEG()
{
	LIB_FFMPEG::avcodec_register_all();
	LIB_FFMPEG::av_register_all();

	#if defined _DEBUG
		LIB_FFMPEG::av_log_set_level(AV_LOG_VERBOSE);
	#else
		LIB_FFMPEG::av_log_set_level(AV_LOG_QUIET);
	#endif
}

int System::VM_FileSystem::InitLibraries()
{
	#if defined _android
		SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "0");
	#elif defined _ios
		SDL_SetHint(SDL_HINT_AUDIO_CATEGORY, "AVAudioSessionCategoryPlayback");
	#elif defined _macosx
		SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
	#elif defined _linux
		SDL_setenv("SDL_VIDEO_X11_LEGACY_FULLSCREEN", "0", 1);

		if ((std::getenv("DISPLAY") == NULL) || (strlen(std::getenv("DISPLAY")) == 0))
			SDL_setenv("DISPLAY", ":0", 1);
	#endif

	SDL_setenv("SDL_VIDEO_YUV_DIRECT",  "1", 1);
	SDL_setenv("SDL_VIDEO_YUV_HWACCEL", "1", 1);

	SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE,        "3");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,         "2");
	SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED,       "0");
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

	// SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		VM_Modal::ShowMessage(VM_Window::Labels["error.sdl"]);
		return ERROR_UNKNOWN;
	}

	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

	// SDL_TTF
	if (TTF_Init() < 0) {
		VM_Modal::ShowMessage(VM_Window::Labels["error.sdl_ttf"]);
		return ERROR_UNKNOWN;
	}

	// CURL
	CURLcode curlResult = curl_global_init(CURL_GLOBAL_ALL);

	if (curlResult != CURLE_OK) {
		VM_Modal::ShowMessage(VM_Window::Labels["error.curl"]);
		return ERROR_UNKNOWN;
	}

	// FREEIMAGE
	LIB_FREEIMAGE::FreeImage_Initialise();

	if (LIB_FREEIMAGE::FreeImage_GetVersion() == NULL) {
		VM_Modal::ShowMessage(VM_Window::Labels["error.freeimage"]);
		return ERROR_UNKNOWN;
	}

	// FFMPEG
	VM_FileSystem::InitFFMPEG();

	if ((LIB_FFMPEG::av_version_info() == NULL) || (LIB_FFMPEG::avcodec_version() == 0) || (LIB_FFMPEG::avformat_version() == 0)) {
		VM_Modal::ShowMessage(VM_Window::Labels["error.ffmpeg"]);
		return ERROR_UNKNOWN;
	}

	return RESULT_OK;
}

bool System::VM_FileSystem::IsConcat(const String &filePath)
{
	return (filePath.substr(0, 7) == "concat:");
}

bool System::VM_FileSystem::IsBlurayAACS(const String &filePath, size_t fileSize)
{
	if (filePath.empty() || (fileSize == 0))
		return false;

	const int SECTOR_SIZE = 6144;
	uint8_t   buffer[SECTOR_SIZE];
	size_t    nrSectors   = min(1000, fileSize / SECTOR_SIZE);
	FILE*     file        = fopen(filePath.c_str(), "rb");
	size_t    result      = SECTOR_SIZE;
	size_t    sector      = 0;

	std::rewind(file);

	while ((sector < nrSectors) && (result == SECTOR_SIZE))
	{
		result = std::fread(&buffer, 1, SECTOR_SIZE, file);

		if (((buffer[0] & 0xC0) != 0) || (buffer[4] != 0x47))
		{
			for (int i = 0; i < 4; i++) {
				if (buffer[4 + (i * 0xC0)] != 0x47) {
					result = 0;
					break;
				}
			}
		}

		sector++;
	}

	std::fclose(file);

	return (sector < nrSectors);
}

bool System::VM_FileSystem::isDirectory(uint16_t statMode)
{
	return (S_ISDIR(statMode) && !(S_ISLNK(statMode)));		// DT_DIR / S_ISDIR = A directory
}

bool System::VM_FileSystem::IsDVDCSS(const String &filePath, size_t fileSize)
{
	if (filePath.empty() || (fileSize == 0))
		return false;

	const int SECTOR_SIZE = 2048;
	char      buffer[SECTOR_SIZE];
	size_t    nrSectors   = min(1000, fileSize / SECTOR_SIZE);
	FILE*     file        = fopen(filePath.c_str(), "rb");
	size_t    result      = SECTOR_SIZE;
	size_t    sector = 0;

	std::rewind(file);

	while ((sector < nrSectors) && (result == SECTOR_SIZE))
	{
		result = std::fread(&buffer, 1, SECTOR_SIZE, file);

		//if ((buffer[(buffer[13] & 0x07) + 20] & 0x30) != 0)
		if ((buffer[0x14] & 0x30) != 0)
			break;

		sector++;
	}

	std::fclose(file);

	return (sector < nrSectors);
}

bool System::VM_FileSystem::IsDRM(LIB_FFMPEG::AVDictionary* metaData)
{
	return (av_dict_get(metaData, "encryption", NULL, 0) != NULL);
}

bool System::VM_FileSystem::isExpiredDropboxTokenOAuth2(const String &oauth2)
{
	Strings headers = { ("Authorization: Bearer " + oauth2), "Content-Type: application/json" };
	String  response = VM_FileSystem::PostData(VM_FileSystem::GetURL(URL_DROPBOX_TOKEN_EXPIRED), "null", headers);

	return response.empty();
}

bool System::VM_FileSystem::isFile(uint16_t statMode)
{
	return (S_ISREG(statMode) && !(S_ISLNK(statMode)));		// DT_REG / S_ISREG = A regular file
}

bool System::VM_FileSystem::IsHttp(const String &filePath)
{
	return ((filePath.size() > 4) && (filePath.substr(0, 4) == "http"));
}

#if defined _windows
bool System::VM_FileSystem::IsHttp(const WString &filePath)
{
	return ((filePath.size() > 4) && (filePath.substr(0, 4) == L"http"));
}
#endif

bool System::VM_FileSystem::IsITunes(const String &filePath)
{
	return ((filePath.size() > 15) && ((filePath.substr(0, 13) == "ipod-library:") || (filePath.substr(0, 15) == "iphoto-library:")));
}

#if defined _windows
bool System::VM_FileSystem::IsITunes(const WString &filePath)
{
	return ((filePath.size() > 15) && ((filePath.substr(0, 13) == L"ipod-library:") || (filePath.substr(0, 15) == L"iphoto-library:")));
}
#endif

bool System::VM_FileSystem::IsM4A(LIB_FFMPEG::AVFormatContext* formatContext)
{
	LIB_FFMPEG::AVStream* videoStream = VM_FileSystem::GetMediaStreamFirst(formatContext, MEDIA_TYPE_VIDEO, false);

	return ((videoStream == NULL) && (formatContext->iformat != NULL) && (strcmp(formatContext->iformat->name, "mov,mp4,m4a,3gp,3g2,mj2") == 0));
}

bool System::VM_FileSystem::IsMediaFile(const String &filePath)
{
	return VM_Text::VectorContains(VM_FileSystem::mediaFileExtensions, VM_FileSystem::GetFileExtension(filePath, true));
}

#if defined _windows
bool System::VM_FileSystem::IsMediaFile(const WString &filePath)
{
	return VM_Text::VectorContains(VM_FileSystem::mediaFileExtensions, VM_Text::ToUTF8(VM_FileSystem::GetFileExtension(filePath, true).c_str(), false));
}
#endif

bool System::VM_FileSystem::IsPicture(const String &filePath)
{
	LIB_FREEIMAGE::FREE_IMAGE_FORMAT imageFormat = LIB_FREEIMAGE::FreeImage_GetFIFFromFilename(filePath.c_str());
	return ((imageFormat != LIB_FREEIMAGE::FIF_UNKNOWN) && (imageFormat != LIB_FREEIMAGE::FIF_RAW));
}

#if defined _windows
bool System::VM_FileSystem::IsPicture(const WString &filePath)
{
	LIB_FREEIMAGE::FREE_IMAGE_FORMAT imageFormat = LIB_FREEIMAGE::FreeImage_GetFIFFromFilenameU(filePath.c_str());
	return ((imageFormat != LIB_FREEIMAGE::FIF_UNKNOWN) && (imageFormat != LIB_FREEIMAGE::FIF_RAW));
}
#endif

bool System::VM_FileSystem::isRootDrive(const String &filePath)
{
	// https://server/sub
	if (VM_FileSystem::IsHttp(filePath) && (filePath.rfind("/") < 8)) { return true; }
	// \\server\sub, //server/sub
	else if (VM_FileSystem::IsSambaServer(filePath) && ((filePath.rfind("\\") < 2) || (filePath.rfind("/") < 2))) { return true; }
	// C:\sub
	else if (filePath.rfind(PATH_SEPERATOR) < 3) { return true; }

	return false;
}

bool System::VM_FileSystem::IsSambaServer(const String &filePath)
{
	return ((filePath.substr(0, 2) == "\\\\") || (filePath.substr(0, 2) == "//"));
}

#if defined _windows
bool System::VM_FileSystem::IsSambaServer(const WString &filePath)
{
	return ((filePath.substr(0, 2) == L"\\\\") || (filePath.substr(0, 2) == L"//"));
}
#endif

bool System::VM_FileSystem::IsServerAccessible(const String &serverAddress, const String &localAddress)
{
	unsigned long blockMode      = 1;
	addrinfo*     local          = NULL;
	addrinfo*     remote         = NULL;
	addrinfo      hints          = {};
	bool          isAccessible   = false;
	String        serverName     = String(serverAddress);
	String        serverPort     = "80";
	timeval       timeout        = { 0, ONE_SECOND_uS };
	fd_set        writeStructure = {};

	if (serverName.empty())
		return false;

	// HTTP
	if (VM_FileSystem::IsHttp(serverName)) {
		serverName = serverName.substr(serverName.find("//") + 2);
	}
	// SMB/CIFS
	else if (VM_FileSystem::IsSambaServer(serverName))
	{
		serverPort = "445";

		if (serverName.find("\\\\") == 0) {
			serverName = serverName.substr(serverName.find("\\\\") + 2);
			serverName = serverName.substr(0, serverName.find("\\"));
		} else if (serverName.find("//") == 0) {
			serverName = serverName.substr(serverName.find("//") + 2);
			serverName = serverName.substr(0, serverName.find("/"));
		}
	}

	// PORT
	if (serverName.find(":") != String::npos) {
		serverPort = String(serverName);
		serverPort = serverPort.substr(serverPort.find(":") + 1);
		serverName = serverName.substr(0, serverName.find(":"));
		serverPort = serverPort.substr(0, serverPort.find("/"));
	}

	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(serverName.c_str(), serverPort.c_str(), &hints, &remote) != 0)
		return false;

	#if defined _windows
		SOCKET sock;
	#else
		int sock;
	#endif

	// SOCKET
	sock = socket(remote->ai_family, remote->ai_socktype, remote->ai_protocol);

	// ENABLE NON-BLOCKING MODE ON THE SOCKET
	#if defined _windows
		ioctlsocket(sock, FIONBIO, &blockMode);
	#else
		fcntl(sock, F_SETFL, O_NONBLOCK);
	#endif

	// BIND
	if (!localAddress.empty() && getaddrinfo(localAddress.c_str(), NULL, &hints, &local) == 0)
	{
		bind(sock, local->ai_addr, (int)local->ai_addrlen);
		
		if (local != NULL)
			freeaddrinfo(local);
	}

	// CONNECT
	connect(sock, remote->ai_addr, (int)remote->ai_addrlen);
	FD_SET(sock, &writeStructure);

	if (remote != NULL)
		freeaddrinfo(remote);

	// ENABLE TIMEOUT
	if (select((int)(sock + 1), NULL, &writeStructure, NULL, &timeout) == 1)
		isAccessible = true;

	#if defined _windows
		closesocket(sock);
	#else
		close(sock);
	#endif

	return isAccessible;
}

bool System::VM_FileSystem::isSubtitle(const String &filePath)
{
	if (!VM_FileSystem::hasFileExtension(filePath) || (filePath.size() >= MAX_FILE_PATH))
		return false;

	String extension = VM_FileSystem::GetFileExtension(filePath, true);

	if (VM_Text::VectorContains(VM_FileSystem::subFileExtensions, extension))
		return true;

	return false;
}

bool System::VM_FileSystem::isSystemFile(const String &fileName)
{
	if (fileName.empty())
		return true;

	String extension = VM_FileSystem::GetFileExtension(fileName, true);

	if (VM_Text::VectorContains(VM_FileSystem::systemFileExtensions, extension))
		return true;

	// HIDDEN SYSTEM FILES/DIRECTORIES
	String file = VM_Text::ToUpper(fileName);
	return ((file[0] == '.') || (file[0] == '$') || (file == "RECYCLER") || (file == "WINSXS"));
}

bool System::VM_FileSystem::isYouTube(const String &mediaURL)
{
	return (((mediaURL.find("youtube.com") != String::npos) || (mediaURL.find("youtu.be") != String::npos)) && (mediaURL.find("watch?v=") != String::npos));
}

CURL* System::VM_FileSystem::openCURL(const String &mediaURL)
{
	if (mediaURL.empty())
		return NULL;

	CURL* curl = curl_easy_init();

	if (curl == NULL)
		return NULL;

	curl_easy_setopt(curl, CURLOPT_URL,            mediaURL.c_str());
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

	char hostName[DEFAULT_CHAR_BUFFER_SIZE];
	gethostname(hostName, DEFAULT_CHAR_BUFFER_SIZE);

	curl_easy_setopt(curl, CURLOPT_USERAGENT, String(APP_NAME + " (").append(hostName).append(") / " + APP_VERSION).c_str());

	curl_easy_setopt(curl, CURLOPT_MAXREDIRS,         MAX_CURL_REDIRS);
	curl_easy_setopt(curl, CURLOPT_ACCEPTTIMEOUT_MS,  MAX_CURL_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, MAX_CURL_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,        MAX_CURL_TIMEOUT);

	return curl;
}

#if defined _windows
int System::VM_FileSystem::OpenFile(const WString &filePath)
#else
int System::VM_FileSystem::OpenFile(const String &filePath)
#endif
{
	if (filePath.empty() || !VM_Player::State.isStopped)
		return ERROR_UNKNOWN;

	stat_t fileStruct;

	#if defined _windows
		String file = VM_Text::ToUTF8(filePath.c_str(), false);
		fstatw(filePath.c_str(), &fileStruct);
	#else
		String file = String(filePath);
		fstat(filePath.c_str(), &fileStruct);
	#endif

	// FILE
	if (VM_FileSystem::isFile(fileStruct.st_mode))
	{
		if (VM_FileSystem::addMediaFile(file) != RESULT_OK) {
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add"].c_str(), file.c_str());
			return ERROR_UNKNOWN;
		}

		VM_GUI::ListTable->refreshRows();
	}
	// DIRECTORY
	else if (VM_FileSystem::isDirectory(fileStruct.st_mode))
	{
		VM_ThreadManager::Threads[THREAD_SCAN_FILES]->mutex.lock();
		VM_FileSystem::MediaFiles.push(file);
		VM_ThreadManager::Threads[THREAD_SCAN_FILES]->mutex.unlock();
	}
	// UNKNOWN
	else
	{
		VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add"].c_str(), file.c_str());
		return ERROR_UNKNOWN;
	}

	return RESULT_OK;
}

#if defined _windows
WString System::VM_FileSystem::OpenFileBrowser(bool selectDirectory)
#else
String System::VM_FileSystem::OpenFileBrowser(bool selectDirectory)
#endif
{
	#if defined _windows
		WString directoryPath = L"";
	#else
		String directoryPath = "";
	#endif

	#if defined _linux
		GtkWidget* dialog;
		char*      selectedPath;
		
		// Set the display environment variable if not already set
		if (strlen(std::getenv("DISPLAY")) == 0) { SDL_setenv("DISPLAY", ":0", 1); }

		// Initialize GTK+
		if (gtk_init_check(0, NULL)) {
			// Create the dialog window with OK and CANCEL buttons
			if (selectDirectory) {
				dialog = gtk_file_chooser_dialog_new(
					"Select a directory", NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, 
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL
				);
			} else {
				dialog = gtk_file_chooser_dialog_new(
					"Select a file", NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL
				);
			}

			// Display the dialog and wait for a response.
			if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
				// Save the chosen file path
				selectedPath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
				if (selectedPath != NULL) { directoryPath = String(selectedPath); g_free(selectedPath);}
			}

			// Close and destroy the dialog window
			gtk_widget_destroy(GTK_WIDGET(dialog));
			while (gtk_events_pending()) { gtk_main_iteration(); }
		}
	#elif defined _macosx
		NSOpenPanel* panel = [NSOpenPanel openPanel];
		CFURLRef     selectedURL;
		char         selectedPath[MAX_FILE_PATH];

		if (panel != NULL) {
			[panel setAllowsMultipleSelection:NO];

			if (selectDirectory) {
				[panel setCanChooseDirectories:YES]; [panel setCanChooseFiles:YES];
			} else {
				[panel setCanChooseDirectories:NO];  [panel setCanChooseFiles:YES];
			}

			if ([panel runModal] == NSOKButton) {
				selectedURL = (CFURLRef)[[panel URLs] firstObject];
				if (selectedURL != NULL) {
					CFURLGetFileSystemRepresentation(selectedURL, TRUE, (UInt8*)selectedPath, MAX_FILE_PATH);
					directoryPath = String(selectedPath);
				}
			}
		}
	#elif defined _windows
		wchar_t       selectedPath[MAX_FILE_PATH];
		SDL_SysWMinfo sdlInfo;

		SDL_VERSION(&sdlInfo.version);
		SDL_GetWindowWMInfo(VM_Window::MainWindow, &sdlInfo);

		if (selectDirectory) {
			BROWSEINFOW  browseDialog;
			LPITEMIDLIST dialogItem;

			memset(&browseDialog, 0, sizeof(browseDialog));

			browseDialog.hwndOwner = sdlInfo.info.win.window;
			browseDialog.ulFlags   = (BIF_BROWSEINCLUDEFILES | BIF_BROWSEINCLUDEURLS | BIF_NEWDIALOGSTYLE | BIF_UAHINT);
			dialogItem             = SHBrowseForFolderW(&browseDialog);

			if (dialogItem != NULL) {
				if (SHGetPathFromIDListW(dialogItem, selectedPath)) { directoryPath = WString(selectedPath); }
				CoTaskMemFree(dialogItem);
			}
		} else {
			OPENFILENAMEW browseDialog;

			memset(&browseDialog, 0, sizeof(browseDialog));

			browseDialog.lStructSize  = sizeof(browseDialog);
			browseDialog.hwndOwner    = sdlInfo.info.win.window;
			browseDialog.lpstrFile    = selectedPath;
			browseDialog.lpstrFile[0] = '\0';
			browseDialog.nMaxFile     = sizeof(selectedPath);
			browseDialog.lpstrFilter  = L"All\0*.*\0";
			browseDialog.nFilterIndex = 1;
			browseDialog.Flags        = (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS | OFN_EXPLORER);

			if (GetOpenFileNameW(&browseDialog)) { directoryPath = WString(selectedPath); }
		}
	#endif

	#if defined _windows
		if (directoryPath.substr(0, 7) == L"file://") { directoryPath = directoryPath.substr(7); }
	#else
		if (directoryPath.substr(0, 7) == "file://") { directoryPath = directoryPath.substr(7); }
	#endif

	return directoryPath;
}

void System::VM_FileSystem::OpenWebBrowser(const String &mediaURL)
{
	VM_Window::OpenURL = mediaURL;
}

int System::VM_FileSystem::OpenWebBrowserT(const String &mediaURL)
{
	if (!VM_FileSystem::IsHttp(mediaURL))
		return ERROR_INVALID_ARGUMENTS;

	#if defined _android
		jclass    jniClass          = VM_Window::JNI->getClass();
		JNIEnv*   jniEnvironment    = VM_Window::JNI->getEnvironment();
		jmethodID jniOpenWebBrowser = jniEnvironment->GetStaticMethodID(jniClass, "OpenWebBrowser", "(Ljava/lang/String;)V");

		if (jniOpenWebBrowser == NULL)
			return ERROR_UNKNOWN;

		jstring jURL = jniEnvironment->NewStringUTF(mediaURL.c_str());

		jniEnvironment->CallStaticVoidMethod(jniClass, jniOpenWebBrowser, jURL);
	#elif defined _ios || defined _macosx
		NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

		#if defined _ios
			[[UIApplication sharedApplication] openURL:[[NSURL alloc] initWithString:[[NSString alloc] initWithUTF8String:mediaURL.c_str()]]];
		#elif defined _macosx
			[[NSWorkspace sharedWorkspace] openURL:[[NSURL alloc] initWithString:[[NSString alloc] initWithUTF8String:mediaURL.c_str()]]];
		#endif

		[pool release];
	#elif defined _linux
		std::system(String("xdg-open '" + mediaURL + "'").c_str());
	#elif defined _windows
		ShellExecuteA(NULL, "open", mediaURL.c_str(), NULL, NULL, SW_SHOWNORMAL);
	#endif

	VM_Window::OpenURL = "";

	return RESULT_OK;
}

String System::VM_FileSystem::PostData(const String &mediaURL, const String &data, const Strings &headers)
{
	String      content     = "";
	CURL*       curl        = VM_FileSystem::openCURL(mediaURL);
	curl_slist* curlHeaders = NULL;

	if (curl == NULL)
		return "";

	for (const auto &header : headers)
		curlHeaders = curl_slist_append(curlHeaders, header.c_str());

	if (curlHeaders != NULL)
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curlHeaders);

	if (!data.empty())
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, VM_FileSystem::downloadToStringT);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &content);

	curl_easy_perform(curl);
	CLOSE_CURL(curl);

	return content;
}

void System::VM_FileSystem::RefreshMetaData()
{
	VM_FileSystem::updateMeta = true;
}

void System::VM_FileSystem::SaveDropboxTokenOAuth2(const String &userCode)
{
	// USE THE USER TOKEN TO GET AN OAUTH2 TOKEN
	String urlData  = VM_FileSystem::GetURL(URL_DROPBOX_OAUTH2_DATA, userCode);
	String urlToken = VM_FileSystem::GetURL(URL_DROPBOX_OAUTH2_TOKEN);
	String oauth2   = VM_FileSystem::PostData(urlToken, urlData, {});
	String token    = "";

	// SAVE AND RE-USE THE OAUTH2 TOKEN FOR REMAINING REQUESTS TO DROPBOX API
	LIB_JSON::json_t* document = VM_JSON::Parse(oauth2.c_str());

	if (document != NULL)
		token = VM_JSON::GetValueString(VM_JSON::GetItem(document, "access_token"));

	if (!token.empty())
	{
		int  dbResult;
		auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

		if (DB_RESULT_OK(dbResult))
			db->updateSettings("dropbox_token", VM_Text::Encrypt(token));

		DELETE_POINTER(db);
	}

	FREE_JSON_DOC(document);
}

#if defined _android
int System::VM_FileSystem::ScanAndroid(void* userData)
{
	VM_ThreadManager::Threads[THREAD_SCAN_ANDROID]->completed = false;

	VM_FileSystem::InitFFMPEG();

	VM_Window::StatusString = VM_Window::Labels["status.scan.local"];

	Strings files  = VM_FileSystem::GetAndroidMediaFiles();
	int     result = RESULT_OK;

	VM_Modal::ShowMessage(VM_Text::Format("FILES: %ulld", files.size()));
	LOG("FILES: %ulld", files.size());

	for (const auto &file : files)
	{
		VM_Modal::ShowMessage(VM_Text::Format("FILES: %s", file.c_str()));
		LOG("FILE: %s", file.c_str());

		result = VM_FileSystem::addMediaFile(file);

		if (result != RESULT_OK) {
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add"].c_str(), file.c_str());
			break;
		}
	}

	//if (VM_FileSystem::AddMediaFilesRecursively(VM_Window::AndroidStoragePath) == RESULT_OK)
	if (result == RESULT_OK)
		VM_Window::StatusString = VM_Window::Labels["status.scan.finished"];
	//else
	//	VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add"].c_str(), VM_Window::AndroidStoragePath.c_str());

	VM_GUI::ListTable->refreshRows();

	if (VM_ThreadManager::Threads[THREAD_SCAN_ANDROID] != NULL) {
		VM_ThreadManager::Threads[THREAD_SCAN_ANDROID]->start     = false;
		VM_ThreadManager::Threads[THREAD_SCAN_ANDROID]->completed = true;
	}

	return RESULT_OK;
}
#endif

int System::VM_FileSystem::ScanDropboxFiles(void* userData)
{
	VM_ThreadManager::Threads[THREAD_SCAN_DROPBOX]->completed = false;

	VM_Window::StatusString = VM_Window::Labels["status.validate_dropbox"];

	int    dbResult;
	auto   db    = new VM_Database(dbResult, DATABASE_SETTINGSv3);
	String token = "";

	if (DB_RESULT_OK(dbResult))
		token = db->getSettings("dropbox_token");

	DELETE_POINTER(db);

	if (!token.empty())
		token = VM_Text::Decrypt(token);

	// CHECK IF TOKEN IS EXPIRED - RE-AUTHENTICATE
	if (token.empty() || VM_FileSystem::isExpiredDropboxTokenOAuth2(token))
	{
		VM_Window::StatusString = VM_Window::Labels["error.validate_dropbox"];

		VM_ThreadManager::Threads[THREAD_SCAN_DROPBOX]->start     = false;
		VM_ThreadManager::Threads[THREAD_SCAN_DROPBOX]->completed = true;

		return ERROR_UNKNOWN;
	}

	VM_Window::StatusString = VM_Window::Labels["status.validate_dropbox.finished"];

	// https://www.dropbox.com/developers/documentation/http/documentation#files-list_folder

	String            data     = "{ \"path\":\"\", \"recursive\":true, \"include_media_info\":false, \"include_deleted\":false, \"include_has_explicit_shared_members\":false, \"include_mounted_folders\":true }";
	Strings           headers  = { String("Authorization: Bearer " + token), "Content-Type: application/json" };
	String            response = VM_FileSystem::PostData(VM_FileSystem::GetURL(URL_DROPBOX_FILES), data, headers);
	LIB_JSON::json_t* document = VM_JSON::Parse(response.c_str());

	VM_Window::StatusString = VM_Window::Labels["status.scan.dropbox"];

	std::vector<LIB_JSON::json_t*> entryItems;
	LIB_JSON::json_t*              entries      = VM_JSON::GetItem(document, "entries");
	std::vector<LIB_JSON::json_t*> entriesArray = VM_JSON::GetArray(entries);

	for (auto entry : entriesArray)
	{
		if (VM_Window::Quit)
			break;

		if (VM_JSON::GetValueString(VM_JSON::GetItem(entry, ".tag")) != "file")
			continue;

		String name = VM_JSON::GetValueString(VM_JSON::GetItem(entry, "name"));

		if (name.find("\\u") != String::npos)
			name = VM_Text::ToUTF8(name);

		name = ("[Dropbox] " + name);

		if (!VM_FileSystem::IsMediaFile(name)) {
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add"].c_str(), name.c_str());
			continue;
		}

		String path = VM_JSON::GetValueString(VM_JSON::GetItem(entry, "path_display"));
		size_t size = (size_t)VM_JSON::GetValueNumber(VM_JSON::GetItem(entry, "size"));
		String mime = VM_FileSystem::GetFileMIME(name);
		String mediaURL  = VM_FileSystem::GetDropboxURL(path);

		if (mediaURL.empty()) {
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add"].c_str(), name.c_str());
			continue;
		}

		VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["status.adding"].c_str(), name.c_str());

		db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

		int mediaID = 0;

		if (DB_RESULT_OK(dbResult))
			mediaID = db->getID(mediaURL);

		DELETE_POINTER(db);

		if (mediaID > 0) {
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["status.already_added"].c_str(), name.c_str());
			continue;
		}

		VM_MediaType mediaType  = MEDIA_TYPE_UNKNOWN;
		bool         validMedia = false;

		if (VM_FileSystem::IsPicture(name))
		{
			mediaType  = MEDIA_TYPE_PICTURE;
			validMedia = true;
		}
		else
		{
			LIB_FFMPEG::AVFormatContext* formatContext = VM_FileSystem::GetMediaFormatContext(mediaURL, false);

			mediaType = VM_FileSystem::GetMediaType(formatContext);

			// GET MEDIA TYPE, AND PERFORM EXTRA VALIDATIONS
			validMedia = ((mediaType == MEDIA_TYPE_AUDIO) || (mediaType == MEDIA_TYPE_VIDEO));

			// WMA DRM
			if ((formatContext != NULL) && VM_FileSystem::IsDRM(formatContext->metadata))
				validMedia = false;

			FREE_AVFORMAT(formatContext);
		}

		if (!validMedia) {
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add"].c_str(), name.c_str());
			continue;
		}

		db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

		if (DB_RESULT_OK(dbResult))
			dbResult = db->addFile(mediaURL, name, path, size, mediaType, mime);

		DELETE_POINTER(db);

		if (DB_RESULT_OK(dbResult))
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["status.added"].c_str(), name.c_str());
		else
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add"].c_str(), name.c_str());
	}

	if (!VM_Window::Quit)
	{
		VM_Window::StatusString = VM_Window::Labels["status.scan.finished"];

		VM_GUI::ListTable->refreshRows();
	}

	if (VM_ThreadManager::Threads[THREAD_SCAN_DROPBOX] != NULL) {
		VM_ThreadManager::Threads[THREAD_SCAN_DROPBOX]->start     = false;
		VM_ThreadManager::Threads[THREAD_SCAN_DROPBOX]->completed = true;
	}

	return RESULT_OK;
}

#if defined _ios
int System::VM_FileSystem::ScanITunesLibrary(void* userData)
{
	VM_ThreadManager::Threads[THREAD_SCAN_ITUNES]->completed = false;

	VM_Window::StatusString = VM_Window::Labels["status.scan.itunes"];

	NSAutoreleasePool*                autoreleasePool = [[NSAutoreleasePool alloc] init];
	MPMediaQuery*                     query           = [[MPMediaQuery      alloc] init];
	MPMediaLibraryAuthorizationStatus authStatus      = [MPMediaLibrary authorizationStatus];

	while ((authStatus == MPMediaLibraryAuthorizationStatusNotDetermined) && !VM_Window::Quit) {
		SDL_Delay(ONE_SECOND_MS);
		authStatus = [MPMediaLibrary authorizationStatus];
	}

	if (authStatus != MPMediaLibraryAuthorizationStatusAuthorized)
	{
		VM_Window::StatusString = VM_Window::Labels["status.scan.failed"];

		VM_ThreadManager::Threads[THREAD_SCAN_ITUNES]->start     = false;
		VM_ThreadManager::Threads[THREAD_SCAN_ITUNES]->completed = true;

		return ERROR_UNKNOWN;
	}

	int          dbResult;
	VM_Database* db;
	String       fileName, mimeType, url, url2;
	VM_MediaType mediaType;
	NSString*    fileNameNS;
	NSInteger    mediaTypeNS;
	NSURL*       urlNS;
	NSArray*     items = [query items];

	for (MPMediaItem* item in items)
	{
		if (VM_Window::Quit)
			break;

		urlNS = [item valueForProperty:MPMediaItemPropertyAssetURL];
		url   = (urlNS != nil ? String([[urlNS absoluteString] UTF8String]) : "");
		url2  = (!url.empty() ? url.substr(0, url.find("?")) : "");

		if (!VM_FileSystem::IsMediaFile(url2))
			continue;

		fileNameNS = [item valueForProperty:MPMediaItemPropertyTitle];
		fileName   = (fileNameNS != nil ? String([fileNameNS UTF8String]) : "");
		mimeType   = VM_FileSystem::GetFileMIME(url2);

		VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["status.adding"].c_str(), fileName.c_str());

		db     = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);
		int id = 0;

		if (DB_RESULT_OK(dbResult))
			id = db->getID(url);

		DELETE_POINTER(db);

		if (id > 0) {
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["status.already_added"].c_str(), fileName.c_str());
			continue;
		}

		mediaType   = MEDIA_TYPE_UNKNOWN;
		mediaTypeNS = [[item valueForProperty:MPMediaItemPropertyMediaType] intValue];

		if (mediaTypeNS <= MPMediaTypeAnyAudio)
			mediaType = MEDIA_TYPE_AUDIO;
		else if (mediaTypeNS <= MPMediaTypeAnyVideo)
			mediaType = MEDIA_TYPE_VIDEO;
		else if (VM_FileSystem::IsPicture(url2))
			mediaType = MEDIA_TYPE_PICTURE;

		if (mediaType == MEDIA_TYPE_UNKNOWN) {
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add"].c_str(), fileName.c_str());
			continue;
		}

		db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

		if (DB_RESULT_OK(dbResult))
			dbResult = db->addFile(url, ("[iTunes] " + fileName), "[iTunes]", 0, mediaType, mimeType);

		DELETE_POINTER(db);

		if (DB_RESULT_OK(dbResult))
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["status.added"].c_str(), fileName.c_str());
		else
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add"].c_str(), fileName.c_str());
	}

	[PHPhotoLibrary requestAuthorization:^(PHAuthorizationStatus status) {}];

	PHAuthorizationStatus authStatusPhotos = [PHPhotoLibrary authorizationStatus];

	while ((authStatusPhotos == PHAuthorizationStatusNotDetermined) && !VM_Window::Quit) {
		SDL_Delay(ONE_SECOND_MS);
		authStatusPhotos = [PHPhotoLibrary authorizationStatus];
	}

	if (authStatusPhotos != PHAuthorizationStatusAuthorized)
	{
		VM_Window::StatusString = VM_Window::Labels["status.scan.failed"];

		VM_ThreadManager::Threads[THREAD_SCAN_ITUNES]->start     = false;
		VM_ThreadManager::Threads[THREAD_SCAN_ITUNES]->completed = true;

		return ERROR_UNKNOWN;
	}

	PHFetchOptions* fetchOptions = [[PHFetchOptions alloc] init];

	fetchOptions.includeAssetSourceTypes = (
		PHAssetSourceTypeNone | PHAssetSourceTypeUserLibrary | PHAssetSourceTypeCloudShared | PHAssetSourceTypeiTunesSynced
	);

	PHFetchResult* result = [PHAsset fetchAssetsWithOptions:fetchOptions];

	NSString*        idNS;
	NSArray*         resources;
	PHAssetResource* resource;

	for (PHAsset* asset in result)
	{
		if (([asset mediaType] != PHAssetMediaTypeImage) || VM_Window::Quit)
			break;

		idNS       = [asset localIdentifier];
		url        = (idNS != nil ? String("iphoto-library:").append([idNS UTF8String]) : "");
		resources  = [PHAssetResource assetResourcesForAsset:asset];
		resource   = [resources firstObject];
		fileNameNS = (resource != nil ? [resource originalFilename] : nil);
		fileName   = (fileNameNS != nil ? String([fileNameNS UTF8String]) : "");
		mimeType   = VM_FileSystem::GetFileMIME(fileName);

		VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["status.adding"].c_str(), fileName.c_str());

		db     = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);
		int id = 0;

		if (DB_RESULT_OK(dbResult))
			id = db->getID(url);

		if (id > 0) {
			DELETE_POINTER(db);
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["status.already_added"].c_str(), fileName.c_str());
			continue;
		}

		dbResult = db->addFile(url, ("[iTunes] " + fileName), "[iTunes]", 0, MEDIA_TYPE_PICTURE, mimeType);

		if (DB_RESULT_OK(dbResult))
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["status.added"].c_str(), fileName.c_str());
		else
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add"].c_str(), fileName.c_str());

		DELETE_POINTER(db);
	}

	[autoreleasePool release];

	if (!VM_Window::Quit) {
		VM_Window::StatusString = VM_Window::Labels["status.scan.finished"];

		VM_GUI::ListTable->refreshRows();
	}

	if (VM_ThreadManager::Threads[THREAD_SCAN_ITUNES] != NULL) {
		VM_ThreadManager::Threads[THREAD_SCAN_ITUNES]->start     = false;
		VM_ThreadManager::Threads[THREAD_SCAN_ITUNES]->completed = true;
	}

	return RESULT_OK;
}
#endif

int System::VM_FileSystem::ScanMediaFiles(void* userData)
{
	VM_ThreadManager::Threads[THREAD_SCAN_FILES]->completed = false;

	VM_FileSystem::InitFFMPEG();

	VM_Window::StatusString = VM_Window::Labels["status.scan"];

	VM_ThreadManager::Threads[THREAD_SCAN_FILES]->start = true;

	while (!VM_FileSystem::MediaFiles.empty() && !VM_Window::Quit)
	{
		VM_ThreadManager::Threads[THREAD_SCAN_FILES]->mutex.lock();

		String filePath = VM_FileSystem::MediaFiles.front();
		VM_FileSystem::MediaFiles.pop();

		VM_ThreadManager::Threads[THREAD_SCAN_FILES]->mutex.unlock();

		if (filePath.empty())
			continue;

		if (VM_FileSystem::AddMediaFilesRecursively(filePath) != RESULT_OK)
			VM_Window::StatusString = VM_Text::Format("%s '%s'", VM_Window::Labels["error.add_dir"].c_str(), filePath.c_str());
	}

	if (!VM_Window::Quit)
	{
		VM_Window::StatusString = VM_Window::Labels["status.scan.finished"];

		VM_GUI::ListTable->refreshRows();
	}

	if (VM_ThreadManager::Threads[THREAD_SCAN_FILES] != NULL) {
		VM_ThreadManager::Threads[THREAD_SCAN_FILES]->start     = false;
		VM_ThreadManager::Threads[THREAD_SCAN_FILES]->completed = true;
	}

	return RESULT_OK;
}

int System::VM_FileSystem::SetWorkingDirectory()
{
	#if defined _android
		VM_Window::WorkingDirectory = SDL_AndroidGetInternalStoragePath();
	#elif defined _ios || defined _macosx
		NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

		#if defined _ios
			NSString* app = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];

			if (app != NULL)
				VM_Window::WorkingDirectory = String([app UTF8String]);
		#elif defined _macosx
			NSString* home = NSHomeDirectory();

			if (home != NULL)
			#if defined DEBUG
				VM_Window::WorkingDirectory = String([home UTF8String]).append("/voyamedia-dev");
			#else
				VM_Window::WorkingDirectory = String([home UTF8String]).append("/voyamedia");
			#endif
		#endif
		[pool release];
	#else
		char* basePath = SDL_GetBasePath();

		#if defined _linux
			char* home = std::getenv("HOME");

			if ((home != NULL) && (strlen(home) > 0))
			#if defined DEBUG
				VM_Window::WorkingDirectory = String(home).append("/voyamedia-dev");
			#else
				VM_Window::WorkingDirectory = String(home).append("/voyamedia");
			#endif
			else if ((basePath != NULL) && (strlen(basePath) > 0))
				VM_Window::WorkingDirectory = String(basePath);
		#elif defined _windows
			wchar_t* appData = _wgetenv(L"APPDATA");

			// VM IS RUNNING FROM READ-ONLY PROG_FILES DIR
			if ((appData != NULL) && (wcslen(appData) > 0)) {
			#if defined _DEBUG
				VM_Window::WorkingDirectory  = VM_Text::ToUTF8(appData).append("\\VoyaMedia-dev");
				VM_Window::WorkingDirectoryW = WString(appData).append(L"\\VoyaMedia-dev");
			#else
				VM_Window::WorkingDirectory  = VM_Text::ToUTF8(appData).append("\\VoyaMedia");
				VM_Window::WorkingDirectoryW = WString(appData).append(L"\\VoyaMedia");
			#endif
			}

			if (VM_Window::WorkingDirectory.empty() && (basePath != NULL) && (strlen(basePath) > 0))
				VM_Window::WorkingDirectory = String(basePath);
		#endif

		if (basePath != NULL)
			SDL_free(basePath);
	#endif

	if (VM_Window::WorkingDirectory.substr(0, 7) == "file://")
		VM_Window::WorkingDirectory = VM_Window::WorkingDirectory.substr(7);

	if (VM_Text::GetLastCharacter(VM_Window::WorkingDirectory) == PATH_SEPERATOR_C)
		VM_Window::WorkingDirectory = VM_Window::WorkingDirectory.substr(0, VM_Window::WorkingDirectory.rfind(PATH_SEPERATOR));

	return RESULT_OK;
}
