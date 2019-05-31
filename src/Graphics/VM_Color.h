#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_COLOR_H
#define VM_COLOR_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_Color
		{
		public:
			VM_Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
			VM_Color(const VM_Color &color);
			VM_Color(const SDL_Color &color);
			VM_Color();

		public:
			bool operator ==(const VM_Color &color);
			bool operator !=(const VM_Color &color);
			operator SDL_Color();

		public:
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;

		};
	}
}

#endif
