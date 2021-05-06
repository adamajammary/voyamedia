#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_SUBSTYLE_H
#define VM_SUBSTYLE_H

namespace VoyaMedia
{
	namespace MediaPlayer
	{
		struct VM_PlayerSubContext;

		/**
		* [V4 Styles]
		* Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour, BackColour,
		* Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV,
		* AlphaLevel, Encoding
		*
		* [V4+ Styles]
		* Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour,
		* Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow,
		* Alignment, MarginL, MarginR, MarginV, Encoding
		*/
		class VM_SubStyle
		{
		public:
			VM_SubStyle();
			VM_SubStyle(Strings data, VM_SubStyleVersion version);
			~VM_SubStyle() {}

		public:
			VM_SubAlignment    alignment;
			int                blur;
			VM_SubBorderStyle  borderStyle;
			Graphics::VM_Color colorOutline;
			Graphics::VM_Color colorPrimary;
			Graphics::VM_Color colorShadow;
			SDL_FPoint         fontScale;
			int                fontSize;
			int                fontStyle;
			int                marginL;
			int                marginR;
			int                marginV;
			String             name;
			int                outline;
			double             rotation;
			SDL_Point          shadow;

			#if defined _windows
				WString fontName;
			#else
				String  fontName;
			#endif

		private:
			TTF_Font* font;
			int       fontSizeScaled;

		public:
			void      copy(const VM_SubStyle &subStyle);
			TTF_Font* getFont();
			void      openFont(VM_PlayerSubContext &subContext, uint16_t* textUTF16);
			TTF_Font* openFont(LIB_FFMPEG::AVFormatContext* formatContext);
			TTF_Font* openFontArial();

			static int             GetOffsetX(VM_SubTexture* subTexture);
			static VM_SubStyle*    GetStyle(const String &name, const VM_SubStyles &subStyles);
			static bool            IsAlignedBottom(VM_SubAlignment a);
			static bool            IsAlignedCenter(VM_SubAlignment a);
			static bool            IsAlignedLeft(VM_SubAlignment   a);
			static bool            IsAlignedMiddle(VM_SubAlignment a);
			static bool            IsAlignedRight(VM_SubAlignment  a);
			static bool            IsAlignedTop(VM_SubAlignment    a);
			static VM_SubAlignment ToSubAlignment(int alignment);

		private:
			bool isFontValid(TTF_Font* font);
			bool streamHasExtraData(LIB_FFMPEG::AVStream* stream);

		};
	}
}

#endif
