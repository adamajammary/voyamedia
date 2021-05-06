#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_SUBTITLE_H
#define VM_SUBTITLE_H

namespace VoyaMedia
{
	namespace MediaPlayer
	{
		class VM_Subtitle
		{
		public:
			VM_Subtitle();
			~VM_Subtitle();

		public:
			SDL_Rect     clip;
			bool         customClip;
			bool         customMargins;
			bool         customPos;
			bool         customRotation;
			SDL_Rect     drawRect;
			TTF_Font*    font;
			size_t       id;
			int          layer;
			bool         offsetX;
			bool         offsetY;
			SDL_Point    position;
			double       rotation;
			SDL_Point    rotationPoint;
			VM_PTS       pts;
			bool         skip;
			bool         splitStyling;
			VM_SubStyle* style;
			VM_SubRect*  subRect;
			String       text;
			String       text2;
			String       text3;
			Strings      textSplit;
			uint16_t*    textUTF16;

		public:
			void               copy(const VM_Subtitle &subtitle);
			int                getBlur();
			Graphics::VM_Color getColor();
			Graphics::VM_Color getColorOutline();
			Graphics::VM_Color getColorShadow();
			VM_Subtitles       getDuplicateSubs(const VM_Subtitles &subs);
			TTF_Font*          getFont();
			SDL_Rect           getMargins();
			int                getMaxWidth();
			int                getOutline();
			SDL_Point          getShadow();
			bool               isAlignedBottom();
			bool               isAlignedCenter();
			bool               isAlignedLeft();
			bool               isAlignedMiddle();
			bool               isAlignedRight();
			bool               isAlignedTop();
			bool               isExpired();

		private:
			VM_SubAlignment getAlignment();

		};
	}
}

#endif
