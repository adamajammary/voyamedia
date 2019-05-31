#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_SURFACE_H
#define VM_SURFACE_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_Surface
		{
		public:
			VM_Surface(SDL_Surface* surface);
			~VM_Surface();

		public:
			const int    channelsRGBA = 4;
			const int    channelsRGB  = 3;
			int          pixPerLine;
			uint8_t*     rgb;
			uint8_t*     a;
			uint32_t*    pixels;
			SDL_Surface* surface;

		public:
			int blur(int blur);

		};
	}
}

#endif
