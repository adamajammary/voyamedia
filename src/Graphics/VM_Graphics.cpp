#include "VM_Graphics.h"

using namespace VoyaMedia::Database;
using namespace VoyaMedia::System;

// http://oppei.tumblr.com/post/47346262/super-fast-blur-v11-by-mario-klingemann
int Graphics::VM_Graphics::Blur(uint8_t* pixelsRGB, int width, int height, int radius)
{
	if (radius < 1)
		return ERROR_INVALID_ARGUMENTS;

	const int channels    = 3;
	const int lastX       = (width  - 1);
	const int lastY       = (height - 1);
	const int pixelCount  = (width  * height);
	const int radius2     = (radius + radius + 1);
	const int divisorSize = (256 * radius2);

	auto minPixel = new int[max(width, height)];
	auto maxPixel = new int[max(width, height)];
	auto divisor  = new uint8_t[divisorSize];
	auto r        = new uint8_t[pixelCount];
	auto g        = new uint8_t[pixelCount];
	auto b        = new uint8_t[pixelCount];

	for (int i = 0; i < divisorSize; i++)
		divisor[i] = (i / radius2);

	int totalWidth = 0, pixelIndex = 0;

	// BLUR VERTICAL DIMENSION (Y)
	for (int y = 0; y < height; y++)
	{
		int sumR = 0, sumG = 0, sumB = 0;

		for (int i = -radius; i <= radius; i++)
		{
			int pixel = ((pixelIndex + min(lastX, max(i, 0))) * channels);

			sumR += pixelsRGB[pixel];
			sumG += pixelsRGB[pixel + 1];
			sumB += pixelsRGB[pixel + 2];
		}

		for (int x = 0; x < width; x++)
		{
			if (pixelIndex >= pixelCount)
				break;

			r[pixelIndex] = divisor[sumR];
			g[pixelIndex] = divisor[sumG];
			b[pixelIndex] = divisor[sumB];

			if (y == 0) {
				maxPixel[x] = min((x + radius + 1), lastX);
				minPixel[x] = max((x - radius), 0);
			}

			int p1 = ((totalWidth + maxPixel[x]) * channels);
			int p2 = ((totalWidth + minPixel[x]) * channels);

			sumR += pixelsRGB[p1]     - pixelsRGB[p2];
			sumG += pixelsRGB[p1 + 1] - pixelsRGB[p2 + 1];
			sumB += pixelsRGB[p1 + 2] - pixelsRGB[p2 + 2];

			pixelIndex++;
		}

		totalWidth += width;
	}

	// BLUR HORIZONTAL DIMENSION (X)
	for (int x = 0; x < width; x++)
	{
		int sumR = 0, sumG = 0, sumB = 0;
		int radiusWidth = (-radius * width);

		for (int i = -radius; i <= radius; i++)
		{
			int pixel = (max(0, radiusWidth) + x);

			sumR += r[pixel];
			sumG += g[pixel];
			sumB += b[pixel];

			radiusWidth += width;
		}

		int offsetX = x;

		for (int y = 0; y < height; y++)
		{
			pixelsRGB[offsetX * channels]     = divisor[sumR];
			pixelsRGB[offsetX * channels + 1] = divisor[sumG];
			pixelsRGB[offsetX * channels + 2] = divisor[sumB];

			if (x == 0) {
				maxPixel[y] = (min((y + radius + 1), lastY) * width);
				minPixel[y] = (max((y - radius),     0)     * width);
			}

			int p1 = (x + maxPixel[y]);
			int p2 = (x + minPixel[y]);

			if ((p1 >= pixelCount) || (p2 >= pixelCount))
				break;

			sumR += (r[p1] - r[p2]);
			sumG += (g[p1] - g[p2]);
			sumB += (b[p1] - b[p2]);

			offsetX += width;
		}
	}

	DELETE_POINTER_ARR(r);
	DELETE_POINTER_ARR(g);
	DELETE_POINTER_ARR(b);
	DELETE_POINTER_ARR(maxPixel);
	DELETE_POINTER_ARR(minPixel);
	DELETE_POINTER_ARR(divisor);

	return RESULT_OK;
}

bool Graphics::VM_Graphics::ButtonHovered(const SDL_Rect* mouseCoordinates, const SDL_Rect &button)
{
	if ((mouseCoordinates != NULL) &&
		(mouseCoordinates->x > button.x) && (mouseCoordinates->x < (button.x + button.w)) &&
		(mouseCoordinates->y > button.y) && (mouseCoordinates->y < (button.y + button.h)))		
	{
		return true;
	}

	return false;
}

bool Graphics::VM_Graphics::ButtonPressed(const SDL_Event* mouseEvent, const SDL_Rect &button, const bool rightClicked, const bool doubleClicked)
{
	if (mouseEvent == NULL)
		return false;

	int positionX = -1;
	int positionY = -1;

	if (doubleClicked) {
		#if defined _android || defined _ios
			//if (VM_EventManager::TouchEvent != TOUCH_EVENT_DOUBLE_TAP)
			return false;
		#else
			if (mouseEvent->button.clicks < 2)
				return false;
		#endif
	}
	
	if (rightClicked) {
		#if defined _android || defined _ios
			if (VM_EventManager::TouchEvent != TOUCH_EVENT_LONG_PRESS)
				return false;
		#else
			if (mouseEvent->button.button != SDL_BUTTON_RIGHT)
				return false;
		#endif
	}

	#if defined _android || defined _ios
		positionX = (int)(mouseEvent->tfinger.x * (float)VM_Window::Dimensions.w);
		positionY = (int)(mouseEvent->tfinger.y * (float)VM_Window::Dimensions.h);
    #elif defined _macosx
        int windowWidth, windowHeight;
        SDL_GetWindowSize(VM_Window::MainWindow, &windowWidth, &windowHeight);
		
        float scaleRatioX = ((float)VM_Window::Dimensions.w / (float)windowWidth);
        float scaleRatioY = ((float)VM_Window::Dimensions.h / (float)windowHeight);
		
        positionX = (int)((float)mouseEvent->button.x * scaleRatioX);
        positionY = (int)((float)mouseEvent->button.y * scaleRatioY);
	#else
		positionX = mouseEvent->button.x;
		positionY = mouseEvent->button.y;
	#endif

	if ((positionX < button.x) || (positionX > (button.x + button.w)) || 
		(positionY < button.y) || (positionY > (button.y + button.h))) 
	{
		return false;
	}

	return true;
}

Graphics::VM_Image* Graphics::VM_Graphics::CreateSnapshot(const String &filePath, VM_MediaType mediaType, int mediaID)
{
	LIB_FFMPEG::AVFormatContext* formatContext = VM_FileSystem::GetMediaFormatContext(filePath, true);

	if (formatContext == NULL)
		return NULL;

	LIB_FREEIMAGE::FIBITMAP* snapshotImage = NULL;

	bool isM4A = VM_FileSystem::IsM4A(formatContext);

	// EXTRACT THUMBNAIL OR COVER ART FROM VIDEO STREAM
	if (!isM4A)
		snapshotImage = VM_Graphics::CreateSnapshotVideo(mediaID, formatContext, (mediaType == MEDIA_TYPE_VIDEO));

	VM_FileSystem::CloseMediaFormatContext(formatContext, mediaID);

	// MANUALLY EXTRACT M4A COVERT ART (JFIF)
	if (isM4A)
		snapshotImage = VM_Graphics::CreateSnapshotAudioJFIF(filePath);

	VM_Image* image = new VM_Image(true, 0, 0, 0);

	if (snapshotImage != NULL) {
		image->update(snapshotImage);
	}

	FREE_IMAGE(snapshotImage);

	return image;
}

LIB_FREEIMAGE::FIBITMAP* Graphics::VM_Graphics::CreateSnapshotAudioJFIF(const String &audioFile)
{
	if (audioFile.empty())
		return NULL;

	char tempFile[DEFAULT_CHAR_BUFFER_SIZE];
	snprintf(tempFile, DEFAULT_CHAR_BUFFER_SIZE, "%s_TEMP_IMAGE_.JPG", VM_FileSystem::GetPathImages().c_str());

	FILE* fileIn;
	char  tempFile2[DEFAULT_CHAR_BUFFER_SIZE] = "";

	if (VM_FileSystem::IsHttp(audioFile))
	{
		snprintf(
			tempFile2, DEFAULT_CHAR_BUFFER_SIZE, "%s_TEMP_AUDIO_.%s",
			VM_FileSystem::GetPathImages().c_str(), VM_FileSystem::GetFileExtension(audioFile, true).c_str()
		);

		fileIn = VM_FileSystem::DownloadToFile(audioFile, tempFile2);
	} else {
		fileIn = fopen(audioFile.c_str(), "rb");
	}

	FILE* fileOut   = NULL;
	bool  foundJFIF = false;

	if (fileIn != NULL)
	{
		do {
			// https://en.wikipedia.org/wiki/JPEG_File_Interchange_Format
			// SOI (Start of Image)	| FF D8
			// JFIF-APP0 marker		| FF E0
			if (!foundJFIF &&
				(std::fgetc(fileIn) == 0xFF) && (std::fgetc(fileIn) == 0xD8) && 
				(std::fgetc(fileIn) == 0xFF) && (std::fgetc(fileIn) == 0xE0)) 
			{
				fileOut = fopen(tempFile, "wb");

				if (fileOut == NULL)
					break;

				std::fputc(0xFF, fileOut);
				std::fputc(0xD8, fileOut);
				std::fputc(0xFF, fileOut);
				std::fputc(0xE0, fileOut);

				foundJFIF = true;
			}

			if (foundJFIF)
			{
				int c1 = std::fgetc(fileIn);
				int c2 = std::fgetc(fileIn);

				std::fputc(c1, fileOut);
				std::fputc(c2, fileOut);

				// EOI (End of Image) | FF D9
				if ((c1 == 0xFF) && (c2 == 0xD9))
					break;
			}
		} while (!std::feof(fileIn));

		if (fileOut != NULL)
			CLOSE_FILE(fileOut);

		CLOSE_FILE(fileIn);
	}

	if (!foundJFIF)
		return NULL;
	
	LIB_FREEIMAGE::FIBITMAP* image = NULL;

	fileOut = fopen(tempFile, "rb");

	if (fileOut != NULL)
	{
		LIB_FREEIMAGE::FreeImageIO imageIO;

		imageIO.read_proc  = [](auto b, auto s, auto c, auto h) { return (uint32_t)std::fread(b, s, c, static_cast<FILE*>(h)); };
		imageIO.seek_proc  = [](auto h, auto off, auto org)     { return fseek(static_cast<FILE*>(h), off, org); };
		imageIO.tell_proc  = [](auto h)                         { return std::ftell(static_cast<FILE*>(h)); };
		imageIO.write_proc = [](auto b, auto s, auto c, auto h) { return (uint32_t)std::fwrite(b, s, c, static_cast<FILE*>(h)); };

		LIB_FREEIMAGE::FREE_IMAGE_FORMAT imageFormat = LIB_FREEIMAGE::FreeImage_GetFileTypeFromHandle(&imageIO, (LIB_FREEIMAGE::fi_handle)fileOut, 0);

		if ((imageFormat != LIB_FREEIMAGE::FIF_UNKNOWN) && (imageFormat != LIB_FREEIMAGE::FIF_RAW))
			image = LIB_FREEIMAGE::FreeImage_LoadFromHandle(imageFormat, &imageIO, (LIB_FREEIMAGE::fi_handle)fileOut, 0);

		CLOSE_FILE(fileOut);
	}

	std::remove(tempFile);
	
	if (VM_FileSystem::IsHttp(audioFile))
		std::remove(tempFile2);

	return image;
}

LIB_FREEIMAGE::FIBITMAP* Graphics::VM_Graphics::CreateSnapshotVideo(int mediaID, LIB_FFMPEG::AVFormatContext* formatContext, bool seek)
{
	if (formatContext == NULL)
		return NULL;

	LIB_FFMPEG::AVStream* videoStream = VM_FileSystem::GetMediaStreamFirst(formatContext, MEDIA_TYPE_VIDEO);

	if ((videoStream == NULL) || (videoStream->codec == NULL)) {
		FREE_STREAM(videoStream);
		return NULL;
	}

	LIB_FFMPEG::AVFrame* videoFrameYUV = LIB_FFMPEG::av_frame_alloc();

	if (videoFrameYUV == NULL) {
		FREE_STREAM(videoStream);
		return NULL;
	}

	if (seek)
	{
		double  fileSize  = 0;
		int64_t seekPos   = 0;
		int     seekFlags = AV_SEEK_FLAGS(formatContext->iformat);

		if (seekFlags == AVSEEK_FLAG_BYTE)
		{
			int  dbResult;
			auto db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

			if (DB_RESULT_OK(dbResult))
				fileSize = std::atof(db->getValue(mediaID, "size").c_str());

			DELETE_POINTER(db);
		}

		const int64_t AV_TIME_100 = (100ll * AV_TIME_BASE_I64);
		const int64_t AV_TIME_50  = (50ll  * AV_TIME_BASE_I64);
		const int64_t AV_TIME_10  = (10ll  * AV_TIME_BASE_I64);

		if (formatContext->duration > AV_TIME_100)
			seekPos = (seekFlags == AVSEEK_FLAG_BYTE ? (int64_t)(AV_TIME_100 / formatContext->duration * fileSize) : AV_TIME_100);
		else if (formatContext->duration > AV_TIME_50)
			seekPos = (seekFlags == AVSEEK_FLAG_BYTE ? (int64_t)(AV_TIME_50  / formatContext->duration * fileSize) : AV_TIME_50);
		else if (formatContext->duration > AV_TIME_10)
			seekPos = (seekFlags == AVSEEK_FLAG_BYTE ? (int64_t)(AV_TIME_10  / formatContext->duration * fileSize) : AV_TIME_10);

		if (seekPos > 0)
			avformat_seek_file(formatContext, -1, INT64_MIN, seekPos, INT64_MAX, seekFlags);
	}

	int              frameReturned   = 0;
	uint32_t         frameReads      = 0;
	const uint32_t   MAX_FRAME_READS = 10000;
	LIB_FFMPEG::AVPacket packet;

	av_init_packet(&packet);

	while ((av_read_frame(formatContext, &packet) == 0) && !frameReturned && (frameReads < MAX_FRAME_READS))
	{
		if (packet.stream_index == videoStream->index)
			avcodec_decode_video2(videoStream->codec, videoFrameYUV, &frameReturned, &packet);

		LIB_FFMPEG::av_free_packet(&packet);
		frameReads++;
	}

	LIB_FFMPEG::SwsContext* videoScaleContext = NULL;
	LIB_FFMPEG::AVFrame*    videoFrameRGB     = LIB_FFMPEG::av_frame_alloc();

	if (frameReturned && (videoFrameYUV->linesize[0] > 0))
	{
		if (av_image_alloc(videoFrameRGB->data, videoFrameRGB->linesize, videoFrameYUV->width, videoFrameYUV->height, LIB_FFMPEG::AV_PIX_FMT_BGR24, 32) > 0)
		{
			videoScaleContext = sws_getContext(
				videoFrameYUV->width, videoFrameYUV->height, videoStream->codec->pix_fmt,
				videoFrameYUV->width, videoFrameYUV->height, LIB_FFMPEG::AV_PIX_FMT_BGR24, DEFAULT_SCALE_FILTER, NULL, NULL, NULL
			);
		}
	}

	int scaleResult = 0;

	if (videoScaleContext != NULL)
	{
		scaleResult = sws_scale(
			videoScaleContext, videoFrameYUV->data, videoFrameYUV->linesize, 0, videoFrameYUV->height,
			videoFrameRGB->data, videoFrameRGB->linesize
		);
	}

	LIB_FREEIMAGE::FIBITMAP* snapshotImage = NULL;

	if (scaleResult > 0)
	{
		snapshotImage = LIB_FREEIMAGE::FreeImage_ConvertFromRawBits(
			static_cast<uint8_t*>(videoFrameRGB->data[0]), videoFrameYUV->width, videoFrameYUV->height, videoFrameRGB->linesize[0],
			24, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, 1
		);
	}

	if (snapshotImage != NULL) {
		SDL_Rect scaledSize = VM_Graphics::GetScaledSize(videoFrameYUV->width, videoFrameYUV->height);
		snapshotImage       = VM_Graphics::ResizeImage(&snapshotImage, scaledSize.w, scaledSize.h);
	}

	FREE_STREAM(videoStream);
	FREE_SWS(videoScaleContext);
	LIB_FFMPEG::av_frame_free(&videoFrameRGB);
	LIB_FFMPEG::av_frame_free(&videoFrameYUV);

	return snapshotImage;
}

int Graphics::VM_Graphics::CreateThumbnail(int mediaID, VM_MediaType mediaType)
{
	VM_Image* image = NULL;

	// CREATE THUMB IMAGE
	if (mediaType == MEDIA_TYPE_PICTURE)
		image = VM_Graphics::CreateThumbnailPicture(mediaID, mediaType);
	else if ((mediaType == MEDIA_TYPE_AUDIO) || (mediaType == MEDIA_TYPE_VIDEO))
		image = VM_Graphics::CreateThumbnailAudioVideo(mediaID, mediaType);

	int result = ERROR_UNKNOWN;

	// SAVE THUMB IMAGE TO FILE
	if ((image != NULL) && (image->image != NULL) && (mediaID > 0))
	{
		#if defined _windows
			WString thumbFile = VM_FileSystem::GetPathThumbnailsW(mediaID);
		#else
			String thumbFile  = VM_FileSystem::GetPathThumbnails(mediaID);
		#endif

		result = VM_Graphics::SaveImage(image->image, thumbFile);
	}

	DELETE_POINTER(image);

	return result;
}

Graphics::VM_Image* Graphics::VM_Graphics::CreateThumbnailAudioVideo(int mediaID, VM_MediaType mediaType)
{
	int       dbResult;
	auto      db       = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);
	VM_Image* image    = NULL;
	String    filePath = "";

	if (DB_RESULT_OK(dbResult))
		filePath = db->getValue(mediaID, "full_path");

	DELETE_POINTER(db);

	#if defined _ios
		String filePath2 = filePath;

		if (VM_FileSystem::IsITunes(filePath2))
			filePath = VM_FileSystem::DownloadFileFromITunes(filePath2);
	#endif

	image = VM_Graphics::CreateSnapshot(filePath, mediaType, mediaID);

	#if defined _ios
		if (VM_FileSystem::IsITunes(filePath2))
			VM_FileSystem::DeleteFileITunes(filePath);
	#endif

	return image;
}

Graphics::VM_Image* Graphics::VM_Graphics::CreateThumbnailPicture(int mediaID, VM_MediaType mediaType)
{
	#if defined _windows
		WString filePath = L"";
	#else
		String  filePath = "";
	#endif

	int  dbResult;
	auto db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

	if (DB_RESULT_OK(dbResult))
	#if defined _windows
		filePath = db->getValueW(mediaID, "full_path");
	#else
		filePath = db->getValue(mediaID, "full_path");
	#endif

	DELETE_POINTER(db);

	if (filePath.empty())
		return NULL;

	#if defined _ios
		String filePath2 = filePath;

		if (VM_FileSystem::IsITunes(filePath2))
			filePath = VM_FileSystem::DownloadFileFromITunes(filePath2);
	#endif

	LIB_FREEIMAGE::FIBITMAP* imageData = VM_Graphics::OpenImage(filePath);

	if (imageData == NULL)
		return NULL;

	auto image = new VM_Image(true, 0, 0, 0);

	image->update(imageData);
	FREE_IMAGE(imageData);

	if (image->image != NULL)
	{
		SDL_Rect scaledSize = VM_Graphics::GetScaledSize(
			(int)LIB_FREEIMAGE::FreeImage_GetWidth(image->image),
			(int)LIB_FREEIMAGE::FreeImage_GetHeight(image->image)
		);

		image->resize(scaledSize.w, scaledSize.h);
	}

	#if defined _ios
		if (VM_FileSystem::IsITunes(filePath2))
			VM_FileSystem::DeleteFileITunes(filePath);
	#endif

	return image;
}

#if defined _windows
int Graphics::VM_Graphics::CreateThumbFromCoverArtFile(const WString &fullPath, const WString &thumbPath)
{
	int      result        = ERROR_UNKNOWN;
	WString  coverArtDir   = fullPath.substr(0, fullPath.rfind(PATH_SEPERATOR_W) + 1);
	WStrings coverArtFiles = {
		L"album.jpg",  L"Album.jpg",  L"ALBUM.JPG",
		L"cover.jpg",  L"Cover.jpg",  L"COVER.JPG",
		L"folder.jpg", L"Folder.jpg", L"FOLDER.JPG"
	};

	for (const auto &coverArtfile : coverArtFiles)
	{
		WString file = (coverArtDir + coverArtfile);

		if (VM_FileSystem::FileExists("", file))
		{
			LIB_FREEIMAGE::FIBITMAP* image = VM_Graphics::OpenImage(file);

			if (image == NULL)
				continue;

			SDL_Rect size = VM_Graphics::GetScaledSize(
				(int)FreeImage_GetWidth(image), (int)FreeImage_GetHeight(image)
			);

			image  = VM_Graphics::ResizeImage(&image, (uint32_t)size.w, (uint32_t)size.h);
			result = VM_Graphics::SaveImage(image, thumbPath);

			FREE_IMAGE(image);

			if (result == RESULT_OK)
				break;
		}
	}

	return result;
}
#else
int Graphics::VM_Graphics::CreateThumbFromCoverArtFile(const String &fullPath, const String &thumbPath)
{
	int     result        = ERROR_UNKNOWN;
	String  coverArtDir   = fullPath.substr(0, fullPath.rfind(PATH_SEPERATOR) + 1);
	Strings coverArtFiles = {
		"album.jpg",  "Album.jpg",  "ALBUM.JPG",
		"cover.jpg",  "Cover.jpg",  "COVER.JPG",
		"folder.jpg", "Folder.jpg", "FOLDER.JPG"
	};

	for (const auto &coverArtfile : coverArtFiles)
	{
		String file = (coverArtDir + coverArtfile);

		if (VM_FileSystem::FileExists(file, L""))
		{
			LIB_FREEIMAGE::FIBITMAP* image = VM_Graphics::OpenImage(file);

			if (image == NULL)
				continue;

			SDL_Rect size = VM_Graphics::GetScaledSize(
				(int)FreeImage_GetWidth(image), (int)FreeImage_GetHeight(image)
			);

			image  = VM_Graphics::ResizeImage(&image, (uint32_t)size.w, (uint32_t)size.h);
			result = VM_Graphics::SaveImage(image, thumbPath);

			FREE_IMAGE(image);

			if (result == RESULT_OK)
				break;
		}
	}

	return result;
}
#endif

#if defined _ios
int Graphics::VM_Graphics::CreateThumbITunes(const String &fileName, const String &fullPath, const String &thumbPath)
{
	NSAutoreleasePool* pool        = [[NSAutoreleasePool alloc] init];
	NSString*          thumbPathNS = [NSString stringWithUTF8String:thumbPath.c_str()];

	if (thumbPathNS == nil)
		return ERROR_UNKNOWN;

	if (fullPath.substr(0, 13) == "ipod-library:")
	{
		MPMediaQuery* query = [[MPMediaQuery alloc] init];

		[query addFilterPredicate:[MPMediaPropertyPredicate predicateWithValue:[[NSString alloc] initWithUTF8String:fileName.substr(9).c_str()] forProperty:MPMediaItemPropertyTitle comparisonType:MPMediaPredicateComparisonEqualTo]];

		for (MPMediaItem* item in [query items])
		{
			if (String([[[item valueForProperty:MPMediaItemPropertyAssetURL] absoluteString] UTF8String]) != fullPath)
				continue;

			MPMediaItemArtwork* artwork      = [item valueForProperty:MPMediaItemPropertyArtwork];
			UIImage*            artworkImage = (artwork != nil ? [artwork imageWithSize:artwork.bounds.size] : nil);

			if (artworkImage == nil)
				break;

			[UIImageJPEGRepresentation(artworkImage, 1.0) writeToFile:thumbPathNS atomically:YES];

			break;
		}
	}
	else if (fullPath.substr(0, 15) == "iphoto-library:")
	{
		PHFetchOptions* fetchOptions = [[PHFetchOptions alloc] init];

		fetchOptions.includeAssetSourceTypes = (
			PHAssetSourceTypeNone | PHAssetSourceTypeUserLibrary | PHAssetSourceTypeCloudShared | PHAssetSourceTypeiTunesSynced
		);

		String         identifier   = fullPath.substr(15);
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
				[imageData writeToFile:thumbPathNS atomically:YES];
			}];
		}
	}

	[pool release];

	if (!VM_FileSystem::FileExists(thumbPath, L""))
		return ERROR_UNKNOWN;

	return RESULT_OK;
}
#endif

int Graphics::VM_Graphics::CreateThumbThread(void* userData)
{
	if (userData == NULL) {
		VM_GUI::ListTable->removeThumbThread();
		VM_GUI::ListTable->refreshThumbs();

		return ERROR_INVALID_ARGUMENTS;
	}

	int  result     = ERROR_UNKNOWN;
	auto threadData = static_cast<VM_ThreadData*>(userData);

	#if defined _windows
		WString thumbPath = threadData->dataW["thumb_path"];
	#else
		String thumbPath = threadData->data["thumb_path"];
	#endif

	// INTERNET MEDIA
	if (VM_Top::Selected >= MEDIA_TYPE_YOUTUBE)
	{
		LIB_FREEIMAGE::FIBITMAP* thumbImage = VM_Graphics::OpenImageHTTP(threadData->data["full_path"]);
		result = VM_Graphics::SaveImage(thumbImage, thumbPath);
		FREE_IMAGE(thumbImage);
	}
	// LOCAL MEDIA
	else
	{
		#if defined _ios
		if (VM_FileSystem::IsITunes(threadData->data["full_path"]))
			result = VM_Graphics::CreateThumbITunes(threadData->data["name"], threadData->data["full_path"], thumbPath);

		if (result != RESULT_OK) {
		#endif
			result = VM_Graphics::CreateThumbnail(std::atoi(threadData->data["id"].c_str()), VM_Top::Selected);

			if (result != RESULT_OK) {
			#if defined _windows
				VM_Graphics::CreateThumbFromCoverArtFile(VM_Text::ToUTF16(threadData->data["full_path"].c_str()), thumbPath);
			#else
				VM_Graphics::CreateThumbFromCoverArtFile(threadData->data["full_path"], thumbPath);
			#endif
			}
		#if defined _ios
		}
		#endif
	}

	DELETE_POINTER(threadData);

	VM_GUI::ListTable->removeThumbThread();
	VM_GUI::ListTable->refreshThumbs();

	return RESULT_OK;
}

int Graphics::VM_Graphics::FillArea(VM_Color* fillColor, const SDL_Rect* areaToFill)
{
	if ((fillColor == NULL) || (areaToFill == NULL))
		return ERROR_INVALID_ARGUMENTS;

	SDL_SetRenderDrawBlendMode(VM_Window::Renderer, (fillColor->a < 0xFF ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE));
	SDL_SetRenderDrawColor(VM_Window::Renderer,     fillColor->r, fillColor->g, fillColor->b, fillColor->a);
	
	return SDL_RenderFillRect(VM_Window::Renderer, areaToFill);
}

int Graphics::VM_Graphics::FillBorder(VM_Color* color, const SDL_Rect* button, const int borderThickness)
{
	if ((color == NULL) || (button == NULL))
		return ERROR_INVALID_ARGUMENTS;

	return VM_Graphics::FillBorder(color, button, VM_Border(borderThickness));
}

int Graphics::VM_Graphics::FillBorder(VM_Color* color, const SDL_Rect* button, const VM_Border &borderThickness)
{
	SDL_Rect borderLine;
	int  result;

	if ((color == NULL) || (button == NULL))
		return ERROR_INVALID_ARGUMENTS;

	borderLine = SDL_Rect(*button);

	// TOP
	borderLine.h = borderThickness.top;
	result       = VM_Graphics::FillArea(color, &borderLine);

	// BOTTOM
	borderLine.y = (button->y + button->h - borderThickness.bottom);
	borderLine.h = borderThickness.bottom;
	result       = VM_Graphics::FillArea(color, &borderLine);

	// LEFT
	borderLine.y = button->y;
	borderLine.w = borderThickness.left;
	borderLine.h = button->h;
	result       = VM_Graphics::FillArea(color, &borderLine);

	// RIGHT
	borderLine.x = (button->x + button->w - borderThickness.right);
	borderLine.w = borderThickness.right;
	borderLine.h = button->h;
	result       = VM_Graphics::FillArea(color, &borderLine);

	return result;
}

void Graphics::VM_Graphics::FillGradient(const VM_Color &colorStart, const VM_Color &colorEnd, int width, int height)
{
	SDL_Color color;
	SDL_Rect  areaToFill = { 0, 0, width, 1 };

	for (int y = 0; y < height; y++)
	{
		areaToFill.y = y;

		color.r = ((colorEnd.r * y / height) + (colorStart.r * (height - y) / height));
		color.g = ((colorEnd.g * y / height) + (colorStart.g * (height - y) / height));
		color.b = ((colorEnd.b * y / height) + (colorStart.b * (height - y) / height));

		if ((colorStart.a < 0xFF) || (colorEnd.a < 0xFF))
		{
			color.a = ((colorEnd.a * y / height) + (colorStart.a * (height - y) / height));

			SDL_SetRenderDrawBlendMode(VM_Window::Renderer, SDL_BLENDMODE_BLEND);
		} else {
			SDL_SetRenderDrawBlendMode(VM_Window::Renderer, SDL_BLENDMODE_NONE);
		}

		SDL_SetRenderDrawColor(VM_Window::Renderer, color.r, color.g, color.b, color.a);
		SDL_RenderFillRect(VM_Window::Renderer,     &areaToFill);
	}
}

Graphics::VM_Texture* Graphics::VM_Graphics::GetButtonLabel(const String &label, VM_Color color, const uint32_t wrapLength, const int fontSize)
{
	if (label.empty())
		return NULL;

	#if defined _windows
		WString fontPath    = VM_FileSystem::GetPathFontW();
		WString fontCJK     = WString(fontPath + L"NotoSansCJK-Bold.ttc");
		WString fontDefault = WString(fontPath + L"NotoSans-Merged.ttf");
	#else
		String fontPath    = VM_FileSystem::GetPathFont();
		String fontCJK     = String(fontPath + "NotoSansCJK-Bold.ttc");
		String fontDefault = String(fontPath + "NotoSans-Merged.ttf");
	#endif

	TTF_Font* font = OPEN_FONT(fontDefault.c_str(), fontSize);

	if (font == NULL) {
		#ifdef _DEBUG
			LOG("%s: %s", VM_Window::Labels["error.open_font"].c_str(), SDL_GetError());
		#endif
		return NULL;
	}

	uint16_t label16[DEFAULT_WCHAR_BUFFER_SIZE];
	VM_Text::ToUTF16(label, label16, DEFAULT_WCHAR_BUFFER_SIZE);

	if (!VM_Text::FontSupportsLanguage(font, label16, DEFAULT_WCHAR_BUFFER_SIZE))
	{
		CLOSE_FONT(font);
		font = OPEN_FONT(fontCJK.c_str(), fontSize);

		if (font == NULL) {
			#ifdef _DEBUG
				LOG("%s: %s", VM_Window::Labels["error.open_font"].c_str(), SDL_GetError());
			#endif
			return NULL;
		}

		if (!VM_Text::FontSupportsLanguage(font, label16, DEFAULT_WCHAR_BUFFER_SIZE)) {
			CLOSE_FONT(font);
			font = OPEN_FONT(fontDefault.c_str(), fontSize);
		}
	}

	if (font == NULL) {
		#ifdef _DEBUG
			LOG("%s: %s", VM_Window::Labels["error.open_font"].c_str(), SDL_GetError());
		#endif
		return NULL;
	}

	TTF_SetFontHinting(font, TTF_HINTING_LIGHT);

	SDL_Surface* surface = NULL;
	VM_Texture*  texture = NULL;

	if (wrapLength > 0)
	{
		VM_Text::ToUTF16(VM_Text::Wrap(label, font, wrapLength), label16, DEFAULT_WCHAR_BUFFER_SIZE);

		surface = TTF_RenderUNICODE_Blended_Wrapped(font, label16, color, wrapLength);
	} else {
		surface = TTF_RenderUNICODE_Blended(font, label16, color);
	}

	if (surface != NULL) {
		texture = new VM_Texture(surface);
		FREE_SURFACE(surface);
	}

	CLOSE_FONT(font);

	return texture;
}

String Graphics::VM_Graphics::GetImageCamera(LIB_FREEIMAGE::FIBITMAP* image)
{
	if (image == NULL)
		return "";

	StringMap metas = VM_Graphics::getImageMeta(image, LIB_FREEIMAGE::FIMD_EXIF_MAIN);

	if (metas.empty())
		return "";

	String make     = "";
	String model    = "";
	String software = "";

	for (const auto &meta : metas)
	{
		if (meta.first == "Make")
			make = String(meta.second);
		else if (meta.first == "Model")
			model = String(meta.second);
		else if (meta.first == "Software")
			software = String(meta.second);

		if (!make.empty() && !model.empty() && !software.empty())
			break;
	}

	String camera = "";

	if (!make.empty())
	{
		camera = String(make);

		if (!model.empty())
			camera.append(" " + model);

		if (!software.empty())
			camera.append(" (" + software + ")");
	}

	return camera;
}

String Graphics::VM_Graphics::GetImageDateTaken(LIB_FREEIMAGE::FIBITMAP* image)
{
	if (image == NULL)
		return "";

	StringMap metas = VM_Graphics::getImageMeta(image, LIB_FREEIMAGE::FIMD_EXIF_MAIN);

	if (metas.empty())
		return "";

	String dateTaken = "";

	for (const auto &meta : metas)
	{
		if (meta.first == "DateTime")
		{
			dateTaken = String(meta.second);

			if (dateTaken.size() > 7)
				dateTaken[4] = '-'; dateTaken[7] = '-';						// "2015:11:12" => "2015-11-12"

			dateTaken = std::to_string(VM_Text::GetTimeStamp(dateTaken));	// "2015:11:12" => "123456789"

			break;
		}
	}

	return dateTaken;
}

String Graphics::VM_Graphics::GetImageGPS(LIB_FREEIMAGE::FIBITMAP* image)
{
	if (image == NULL)
		return "";

	StringMap metas = VM_Graphics::getImageMeta(image, LIB_FREEIMAGE::FIMD_EXIF_GPS);

	if (metas.empty())
		return "";

	String address   = "";
	double latitude  = (INVALID_COORDINATE + 1.0);
	double longitude = (INVALID_COORDINATE + 1.0);

	for (const auto &meta : metas)
	{
		if (meta.first == "GPSLatitude" || meta.first == "GPSLongitude")
		{
			Strings textSplit = VM_Text::Split(meta.second, ":");

			if (textSplit.size() > 2)
			{
				double coordinates = std::atof(textSplit[0].c_str());

				coordinates += (((std::atof(textSplit[1].c_str()) * 60.0) + std::atof(textSplit[2].c_str())) / 3600.0);

				if (meta.first == "GPSLatitude")
					latitude = coordinates;
				else
					longitude = coordinates;
			}
		}

		if ((latitude < INVALID_COORDINATE) && (longitude < INVALID_COORDINATE))
		{
			String apiKey     = VM_Text::Decrypt(GOOGLE_MAPS_API_KEY);
			String apiURL     = (GOOGLE_MAPS_API_URL + std::to_string(latitude) + "," + std::to_string(longitude) + "&key=" + apiKey);
			String response   = VM_FileSystem::DownloadToString(apiURL);
			size_t addressPos = response.find("formatted_address");

			if (addressPos != String::npos) {
				address = response.substr(addressPos + 22);
				address = address.substr(0, address.find("\","));
			}

			break;
		}
	}

	return address;
}

StringMap Graphics::VM_Graphics::getImageMeta(LIB_FREEIMAGE::FIBITMAP* image, LIB_FREEIMAGE::FREE_IMAGE_MDMODEL metaType)
{
	StringMap metas;

	if (image == NULL)
		return metas;

	LIB_FREEIMAGE::FITAG*      tag  = NULL;
	LIB_FREEIMAGE::FIMETADATA* meta = LIB_FREEIMAGE::FreeImage_FindFirstMetadata(metaType, image, &tag);

	if (meta == NULL)
		return metas;

	do {
		const char* key = LIB_FREEIMAGE::FreeImage_GetTagKey(tag);

		if (key != NULL)
		{
			const char* value = LIB_FREEIMAGE::FreeImage_TagToString(metaType, tag);

			if (value != NULL)
				metas.insert({ key, value });
		}
	} while (LIB_FREEIMAGE::FreeImage_FindNextMetadata(meta, &tag));

	LIB_FREEIMAGE::FreeImage_FindCloseMetadata(meta);

	return metas;
}

String Graphics::VM_Graphics::GetImageResolutionString(int width, int height)
{
	char resolution[DEFAULT_CHAR_BUFFER_SIZE];
	snprintf(resolution, DEFAULT_CHAR_BUFFER_SIZE, "%.2f MegaPixels (%dx%d)", (float)((float)(width * height) / (float)ONE_MILLION), width, height);

	return String(resolution);
}

SDL_Rect Graphics::VM_Graphics::GetScaledSize(int width, int height)
{
	SDL_Rect scaledSize = {};
	int      maxWidth   = (int)((float)width  / (float)height * (float)MAX_THUMB_SIZE);
	int      maxHeight  = (int)((float)height / (float)width  * (float)MAX_THUMB_SIZE);

	scaledSize.w = min(width,  min(maxWidth,  MAX_THUMB_SIZE));
	scaledSize.h = min(height, min(maxHeight, MAX_THUMB_SIZE));

	return scaledSize;
}

SDL_Surface* Graphics::VM_Graphics::GetSurface(VM_Image* image)
{
	if (image == NULL)
		return NULL;

	uint32_t imageWidth  = image->width;
	uint32_t imageHeight = image->height;
	uint32_t imagePitch  = image->pitch;
	uint8_t* imagePixels = image->pixels;

	if ((imagePixels == NULL) || (imagePitch == 0) || (imageHeight == 0) || (imageWidth == 0))
		return NULL;

	SDL_Surface* surface = SDL_CreateRGBSurface(0, imageWidth, imageHeight, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, FI_RGBA_ALPHA_MASK);

	if (surface == NULL)
		return NULL;
	
	SDL_LockSurface(surface);
	surface->pixels = SDL_memcpy(surface->pixels, imagePixels, (sizeof(uint8_t) * imagePitch * imageHeight));
	SDL_UnlockSurface(surface);

	return surface;
}

int Graphics::VM_Graphics::GetTopBarHeight()
{
	int height = 0;

	#if defined _android
		jclass    jniClass       = VM_Window::JNI->getClass();
		JNIEnv*   jniEnvironment = VM_Window::JNI->getEnvironment();
		jmethodID jniGetHeight   = jniEnvironment->GetStaticMethodID(jniClass, "GetTopBarHeight", "()I");

		if (jniGetHeight != NULL)
			height = (float)jniEnvironment->CallStaticIntMethod(jniClass, jniGetHeight);
	#elif defined _ios
		height = (int)([UIApplication sharedApplication].statusBarFrame.size.height * [UIScreen mainScreen].scale);
	#endif

	return height;
}

#if defined _windows
LIB_FREEIMAGE::FIBITMAP* Graphics::VM_Graphics::OpenImage(const WString &filePath)
{
	if (filePath.empty())
		return NULL;

	LIB_FREEIMAGE::FIBITMAP* image = NULL;

	if (VM_FileSystem::IsHttp(filePath)) {
		image = VM_Graphics::OpenImageHTTP(VM_Text::ToUTF8(filePath.c_str()));
	}
	else
	{
		LIB_FREEIMAGE::FREE_IMAGE_FORMAT imageFormat = LIB_FREEIMAGE::FreeImage_GetFileTypeU(filePath.c_str());

		if (imageFormat == LIB_FREEIMAGE::FIF_UNKNOWN)
			imageFormat = LIB_FREEIMAGE::FreeImage_GetFIFFromFilenameU(filePath.c_str());

		if ((imageFormat != LIB_FREEIMAGE::FIF_UNKNOWN) && (imageFormat != LIB_FREEIMAGE::FIF_RAW))
			image = LIB_FREEIMAGE::FreeImage_LoadU(imageFormat, filePath.c_str());
	}

	if ((image == NULL) || (LIB_FREEIMAGE::FreeImage_GetWidth(image) == 0) || (LIB_FREEIMAGE::FreeImage_GetHeight(image) == 0)) {
		FREE_IMAGE(image);
		return NULL;
	}

	return image;
}
#else
LIB_FREEIMAGE::FIBITMAP* Graphics::VM_Graphics::OpenImage(const String &filePath)
{
	if (filePath.empty())
		return NULL;

	LIB_FREEIMAGE::FIBITMAP* image = NULL;

	// HTTP
	if (VM_FileSystem::IsHttp(filePath))
	{
		image = VM_Graphics::OpenImageHTTP(filePath);
	}
	// FILES
	else
	{
		// LOAD FILE URL FORM BOOKMARK FILE
		#if defined _macosx
			int          dbResult;
			VM_Database* db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

			if (DB_RESULT_OK(dbResult))
				VM_FileSystem::FileBookmarkLoad(db->getID(filePath), true);

			DELETE_POINTER(db);
		#endif

		LIB_FREEIMAGE::FREE_IMAGE_FORMAT imageFormat = LIB_FREEIMAGE::FreeImage_GetFileType(filePath.c_str());

		if (imageFormat == LIB_FREEIMAGE::FIF_UNKNOWN)
			imageFormat = LIB_FREEIMAGE::FreeImage_GetFIFFromFilename(filePath.c_str());

		if ((imageFormat != LIB_FREEIMAGE::FIF_UNKNOWN) && (imageFormat != LIB_FREEIMAGE::FIF_RAW))
			image = LIB_FREEIMAGE::FreeImage_Load(imageFormat, filePath.c_str());
	}

	if ((image == NULL) || (LIB_FREEIMAGE::FreeImage_GetWidth(image) == 0) || (LIB_FREEIMAGE::FreeImage_GetHeight(image) == 0)) {
		FREE_IMAGE(image);
		return NULL;
	}

	return image;
}
#endif

LIB_FREEIMAGE::FIBITMAP* Graphics::VM_Graphics::OpenImageHTTP(const String &mediaURL)
{
	if (mediaURL.empty())
		return NULL;

	VM_Bytes*                bytes = VM_FileSystem::DownloadToBytes(mediaURL);
	LIB_FREEIMAGE::FIBITMAP* image = NULL;

	if (bytes != NULL)
		image = VM_Graphics::OpenImageMemory(bytes);

	DELETE_POINTER(bytes);

	return image;
}

LIB_FREEIMAGE::FIBITMAP* Graphics::VM_Graphics::OpenImageMemory(VM_Bytes* bytes)
{
	if ((bytes == NULL) || (bytes->data == NULL) || (bytes->size == 0))
		return NULL;

	LIB_FREEIMAGE::FREE_IMAGE_FORMAT format = LIB_FREEIMAGE::FIF_UNKNOWN;
	LIB_FREEIMAGE::FIMEMORY*         memory = LIB_FREEIMAGE::FreeImage_OpenMemory(bytes->data, (ulong)bytes->size);
	LIB_FREEIMAGE::FIBITMAP*         image  = NULL;

	if (memory != NULL)
		format = LIB_FREEIMAGE::FreeImage_GetFileTypeFromMemory(memory, 0);

	if (format != LIB_FREEIMAGE::FIF_UNKNOWN)
		image = LIB_FREEIMAGE::FreeImage_LoadFromMemory(format, memory, 0);

	FREE_MEMORY(memory);

	return image;
}

int Graphics::VM_Graphics::OpenImagesThread(void* userData)
{
	VM_ThreadManager::Threads[THREAD_CREATE_IMAGES]->completed = false;

	LIB_FREEIMAGE::FreeImage_Initialise();

	while (!VM_Window::Quit)
	{
		VM_ThreadManager::Mutex.lock();

		for (const auto &image : VM_ThreadManager::Images)
		{
			if (VM_Window::Quit)
				break;

			if ((image.second == NULL) || image.second->ready)
				continue;

			int width  = (int)image.second->width;
			int height = (int)image.second->height;

			#if defined _windows
				WString filePath = image.first.substr(0, image.first.rfind(L"_"));
			#else
				String filePath = image.first.substr(0, image.first.rfind("_"));
			#endif

			#if defined _ios
			if (VM_FileSystem::IsITunes(image.first) && PICTURE_IS_SELECTED) {
				String url = VM_FileSystem::DownloadFileFromITunes(filePath);
				image.second->open(url);
				VM_FileSystem::DeleteFileITunes(url);
			} else {
			#endif
				image.second->open(filePath);
			#if defined _ios
			}
			#endif

			if (image.second->thumb && PICTURE_IS_SELECTED)
			{
				int mediaID = image.second->id;

				if (image.second->thumb && (mediaID == 0))
					mediaID = VM_GUI::ListTable->getSelectedMediaID();

				if (mediaID > 0)
				{
					int  dbResult;
					auto db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);

					if (DB_RESULT_OK(dbResult))
						image.second->rotate(std::atof(db->getValue(mediaID, "rotation").c_str()));

					DELETE_POINTER(db);
				}
			}

			if ((width > 0) && (height > 0))
			{
				if (image.second->thumb)
				{
					int maxWidth     = (int)((float)image.second->width  / (float)image.second->height * (float)height);
					int maxHeight    = (int)((float)image.second->height / (float)image.second->width  * (float)width);
					int scaledWidth  = min(min(maxWidth,  width),  (int)image.second->width);
					int scaledHeight = min(min(maxHeight, height), (int)image.second->height);

					image.second->resize(scaledWidth, scaledHeight);
				} else {
					image.second->resize(width, height);
				}
			}

			if (image.second->image != NULL)
				image.second->ready = true;
		}

		VM_ThreadManager::Mutex.unlock();

		if (!VM_Window::Quit)
			SDL_Delay(DELAY_TIME_BACKGROUND);
	}

	LIB_FREEIMAGE::FreeImage_DeInitialise();

	if (VM_ThreadManager::Threads[THREAD_CREATE_IMAGES] != NULL) {
		VM_ThreadManager::Threads[THREAD_CREATE_IMAGES]->start     = false;
		VM_ThreadManager::Threads[THREAD_CREATE_IMAGES]->completed = true;
	}

	return RESULT_OK;
}

LIB_FREEIMAGE::FIBITMAP* Graphics::VM_Graphics::ResizeImage(LIB_FREEIMAGE::FIBITMAP** image, unsigned int width, unsigned int height, const bool freeImage)
{
	if (image == NULL)
		return NULL;

	LIB_FREEIMAGE::FIBITMAP* scaledImage = LIB_FREEIMAGE::FreeImage_Rescale(*image, width, height, LIB_FREEIMAGE::FILTER_LANCZOS3);

	if (scaledImage == NULL)
		return NULL;

	if (freeImage) {
		FREE_IMAGE(*image);
		*image = scaledImage;
	}

	return scaledImage;
}

LIB_FREEIMAGE::FIBITMAP* Graphics::VM_Graphics::RotateImage(LIB_FREEIMAGE::FIBITMAP** image, double angle, const bool freeImage)
{
	if (image == NULL)
		return NULL;

	LIB_FREEIMAGE::FIBITMAP* rotatedImage = LIB_FREEIMAGE::FreeImage_Rotate(*image, -angle, NULL);

	if (rotatedImage == NULL)
		return NULL;

	if (freeImage) {
		FREE_IMAGE(*image);
		*image = rotatedImage;
	}

	return rotatedImage;
}

#if defined _windows
int Graphics::VM_Graphics::SaveImage(LIB_FREEIMAGE::FIBITMAP* image, const WString &filePath)
{
	if ((image == NULL) || filePath.empty())
		return ERROR_INVALID_ARGUMENTS;

	int result = LIB_FREEIMAGE::FreeImage_SaveU(LIB_FREEIMAGE::FIF_JPEG, image, filePath.c_str(), JPEG_QUALITYGOOD);

	if (!result)
		result = LIB_FREEIMAGE::FreeImage_SaveU(LIB_FREEIMAGE::FIF_PNG, image, filePath.c_str(), PNG_DEFAULT);

	return (result ? RESULT_OK : ERROR_UNKNOWN);
}
#else
int Graphics::VM_Graphics::SaveImage(LIB_FREEIMAGE::FIBITMAP* image, const String &filePath)
{
	if ((image == NULL) || filePath.empty())
		return ERROR_INVALID_ARGUMENTS;

	int result = LIB_FREEIMAGE::FreeImage_Save(LIB_FREEIMAGE::FIF_JPEG, image, filePath.c_str(), JPEG_QUALITYGOOD);

	if (!result)
		result = LIB_FREEIMAGE::FreeImage_Save(LIB_FREEIMAGE::FIF_PNG, image, filePath.c_str(), PNG_DEFAULT);

	return (result ? RESULT_OK : ERROR_UNKNOWN);
}
#endif

/**
* Valid border strings:
* "1"       => 1px border
* "1 2 3 4"	=> 1px bottom, 2px left, 3px right, 4px top
*/
Graphics::VM_Border Graphics::VM_Graphics::ToVMBorder(const String &borderWidth)
{
	VM_Border border = {};
	Strings   parts  = VM_Text::Split(borderWidth, " ");

	if (parts.size() == 4)
		border = VM_Border(std::atoi(parts[0].c_str()), std::atoi(parts[1].c_str()), std::atoi(parts[2].c_str()), std::atoi(parts[3].c_str()));
	else if (parts.size() == 1)
		border = VM_Border(std::atoi(parts[0].c_str()));

	return border;
}

/**
* Valid color string:
* "#00ff00ff" => 0 red, 255 green, 0 blue, 255 alpha
* "&HFF0000FF" (ABGR)
* "\c&H0000FF&" / "\1c&H0000FF&" (BGR)
*/
Graphics::VM_Color Graphics::VM_Graphics::ToVMColor(const String &colorHex)
{
	VM_Color color   = { 0, 0, 0, 0xFF };
	bool     invertA = false;
	off_t    offsetR = -1;
	off_t    offsetG = -1;
	off_t    offsetB = -1;
	off_t    offsetA = -1;

	if (colorHex.size() < 2)
		return color;

	// RED (RGBA): "#FF0000FF" / "#FF0000"
	if (colorHex[0] == '#')
	{
		// "#FF0000"
		if (colorHex.size() >= 7)
		{
			offsetR = 1;
			offsetG = 3;
			offsetB = 5;
		}

		// "#FF0000FF"
		if (colorHex.size() >= 9)
			offsetA = 7;
	}
	// RED (ABGR): "&HFF0000FF" / "&H0000FF"
	else if (colorHex.substr(0, 2) == "&H")
	{
		// "&HFF0000FF"
		if (colorHex.size() >= 10)
		{
			offsetR = 8;
			offsetG = 6;
			offsetB = 4;
			offsetA = 2;
			invertA = true;
		}
		// "&H0000FF"
		else if (colorHex.size() >= 8)
		{
			offsetR = 6;
			offsetG = 4;
			offsetB = 2;
		}
		// "&HFF00" 
		else if (colorHex.size() >= 6)
		{
			offsetR = 4;
			offsetG = 2;
		}
		// "&HFF" 
		else if (colorHex.size() >= 4)
		{
			offsetR = 2;
		}
		// "&H0" 
	}

	if (offsetR != -1)
		color.r = (uint8_t)std::strtoul(String("0x" + colorHex.substr(offsetR, 2)).c_str(), NULL, 16);

	if (offsetG != -1)
		color.g = (uint8_t)std::strtoul(String("0x" + colorHex.substr(offsetG, 2)).c_str(), NULL, 16);

	if (offsetB != -1)
		color.b = (uint8_t)std::strtoul(String("0x" + colorHex.substr(offsetB, 2)).c_str(), NULL, 16);

	if (offsetA != -1)
	{
		color.a = (uint8_t)std::strtoul(String("0x" + colorHex.substr(offsetA, 2)).c_str(), NULL, 16);

		if (invertA)
			color.a = (0xFF - color.a);
	}

	return color;
}

/**
* Valid aplha string:
* "#ff" => 255 alpha => 100%
*/
uint8_t Graphics::VM_Graphics::ToVMColorAlpha(const String &alphaHex)
{
	return (alphaHex.size() >= 2 ? (uint8_t)std::strtoul(String("0x" + alphaHex.substr(alphaHex.size() - 2, 2)).c_str(), NULL, 16) : 0xFF);
}
