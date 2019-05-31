#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_IMAGE_H
#define VM_IMAGE_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_Image
		{
		public:
			VM_Image(bool thumb, int width, int height, int mediaID);
			~VM_Image();

		public:
			double                   degrees;
			uint32_t                 height;
			int                      id;
			LIB_FREEIMAGE::FIBITMAP* image;
			uint32_t                 pitch;
			uint8_t*                 pixels;
			bool                     ready;
			bool                     thumb;
			uint32_t                 width;

		public:
			void resize(int width, int height);
			void rotate(double degrees);
			void update(LIB_FREEIMAGE::FIBITMAP* image);
			void update(bool thumb, int width, int height, int mediaID);

			#if defined _windows
				void open(const WString &file);
			#else
				void open(const String &file);
			#endif

		};
	}
}

#endif
