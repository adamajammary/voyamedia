#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_TEXTURE_H
#define VM_TEXTURE_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_Texture
		{
		public:
			VM_Texture(VM_Image* image);
			VM_Texture(SDL_Surface* surface);
			VM_Texture(uint32_t format, int access, int width, int height);
			~VM_Texture();

		public:
			int          access;
			SDL_Texture* data;
			uint32_t     format;
			int          height;
			VM_Image*    image;
			int          width;

		public:
			void update(VM_Image* image);

		private:
			void getProperties();
			void init();

		};
	}
}

#endif
