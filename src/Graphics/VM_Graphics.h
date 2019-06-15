#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_GRAPHICS_H
#define VM_GRAPHICS_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_Graphics
		{
		private:
			VM_Graphics()  {}
			~VM_Graphics() {}

		public:
			static int                      Blur(uint8_t* pixelsRGB, int width, int height, int radius);
			static bool                     ButtonHovered(const SDL_Rect* mouseCoordinates, const SDL_Rect &button);
			static bool                     ButtonPressed(const SDL_Event* mouseEvent, const SDL_Rect &button, const bool rightClicked = false, const bool doubleClicked = false);
			static VM_Image*                CreateSnapshot(const String &filePath, VM_MediaType mediaType, int mediaID = 0);
			static LIB_FREEIMAGE::FIBITMAP* CreateSnapshotAudioJFIF(const String &audioFile);
			static LIB_FREEIMAGE::FIBITMAP* CreateSnapshotVideo(int mediaID, LIB_FFMPEG::AVFormatContext* formatContext, bool seek);
			static int                      CreateThumbnail(int mediaID, VM_MediaType mediaType);
			static VM_Image*                CreateThumbnailAudioVideo(int mediaID, VM_MediaType mediaType);
			static VM_Image*                CreateThumbnailPicture(int mediaID, VM_MediaType mediaType);
			static int                      CreateThumbThread(void* userData);
			static int                      FillArea(VM_Color* fillColor, const SDL_Rect* areaToFill);
			static int                      FillBorder(VM_Color* color, const SDL_Rect* button, const int borderThickness);
			static int                      FillBorder(VM_Color* color, const SDL_Rect* button, const VM_Border &borderThickness);
			static void                     FillGradient(const VM_Color &colorStart, const VM_Color &colorEnd, int width, int height);
			static VM_Texture*              GetButtonLabel(const String &label, VM_Color color, const uint32_t wrapLength = 0, const int fontSize = DEFAULT_FONT_SIZE);
			static String                   GetImageCamera(LIB_FREEIMAGE::FIBITMAP* image);
			static String                   GetImageDateTaken(LIB_FREEIMAGE::FIBITMAP* image);
			static String                   GetImageGPS(LIB_FREEIMAGE::FIBITMAP* image);
			static String                   GetImageResolutionString(int width, int height);
			static SDL_Rect                 GetScaledSize(int width, int height);
			static SDL_Surface*             GetSurface(VM_Image* image);
			static int                      GetTopBarHeight();
			static LIB_FREEIMAGE::FIBITMAP* OpenImageHTTP(const String &mediaURL);
			static LIB_FREEIMAGE::FIBITMAP* OpenImageMemory(System::VM_Bytes* bytes);
			static int                      OpenImagesThread(void* userData);
			static LIB_FREEIMAGE::FIBITMAP* ResizeImage(LIB_FREEIMAGE::FIBITMAP** image, unsigned int width, unsigned int height, const bool freeImage = true);
			static LIB_FREEIMAGE::FIBITMAP* RotateImage(LIB_FREEIMAGE::FIBITMAP** image, double angle, const bool freeImage = true);
			static VM_Border                ToVMBorder(const String &borderWidth);
			static VM_Color                 ToVMColor(const String & colorHex);
			static uint8_t                  ToVMColorAlpha(const String & alphaHex);

			#if defined _windows
				static int                      CreateThumbFromCoverArtFile(const WString &fullPath, const WString &thumbPath);
				static LIB_FREEIMAGE::FIBITMAP* OpenImage(const WString &filePath);
				static int                      SaveImage(LIB_FREEIMAGE::FIBITMAP* image, const WString &filePath);
			#else		
				static int                      CreateThumbFromCoverArtFile(const String &fullPath, const String &thumbPath);
				static LIB_FREEIMAGE::FIBITMAP* OpenImage(const String &filePath);
				static int                      SaveImage(LIB_FREEIMAGE::FIBITMAP* image, const String &filePath);
			#endif

			#if defined _ios
				static int CreateThumbITunes(const String &fileName, const String &fullPath, const String &thumbPath);
			#endif

		private:
			static StringMap getImageMeta(LIB_FREEIMAGE::FIBITMAP* image, LIB_FREEIMAGE::FREE_IMAGE_MDMODEL metaType);

		};
	}
}

#endif
