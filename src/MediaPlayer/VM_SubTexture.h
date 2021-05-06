#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_SUBTEXTURE_H
#define VM_SUBTEXTURE_H

namespace VoyaMedia
{
	namespace MediaPlayer
	{
		class VM_SubTexture
		{
		public:
			VM_SubTexture();
			VM_SubTexture(const VM_SubTexture &textureR);
			VM_SubTexture(VM_Subtitle* subtitle);
			~VM_SubTexture();

		public:
			SDL_Rect              locationRender;
			bool                  offsetY;
			VM_SubTexture*        outline;
			VM_SubTexture*        shadow;
			VM_Subtitle*          subtitle;
			Graphics::VM_Texture* textureData;
			uint16_t*             textUTF16;
			SDL_Rect              total;

		};
	}
}

#endif
