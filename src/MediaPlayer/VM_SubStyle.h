#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_SUBSTYLE_H
#define VM_SUBSTYLE_H

namespace VoyaMedia
{
	namespace MediaPlayer
	{
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
			VM_SubAlignment     alignment;
			int                 blur;
			Graphics::VM_Color  colorOutline;
			Graphics::VM_Color  colorPrimary;
			Graphics::VM_Color  colorShadow;
			Graphics::VM_PointF fontScale;
			int                 fontSize;
			int                 fontStyle;
			int                 marginL;
			int                 marginR;
			int                 marginV;
			String              name;
			int                 outline;
			double              rotation;
			SDL_Point           shadow;

			#if defined _windows
				WString fontName;
			#else
				String  fontName;
			#endif

		private:
			TTF_Font* font;
			int       fontSizeScaled;

		public:
			void         copy(const VM_SubStyle &subStyle);
			VM_SubStyle* getDefault(const VM_SubStyles &subStyles);
			TTF_Font*    getFont();

			#if defined _windows
				void openFont(umap<WString, TTF_Font*> &styleFonts, const Graphics::VM_PointF &subScale, VM_Subtitle* sub = NULL);
			#else
				void openFont(umap<String, TTF_Font*> &styleFonts, const Graphics::VM_PointF &subScale, VM_Subtitle* sub = NULL);
			#endif

			static bool            IsAlignedBottom(VM_SubAlignment alignment);
			static bool            IsAlignedCenter(VM_SubAlignment alignment);
			static bool            IsAlignedLeft(VM_SubAlignment   alignment);
			static bool            IsAlignedMiddle(VM_SubAlignment alignment);
			static bool            IsAlignedRight(VM_SubAlignment  alignment);
			static bool            IsAlignedTop(VM_SubAlignment    alignment);
			static VM_SubAlignment ToSubAlignment(int              alignment);

		private:
			bool      isFontValid(TTF_Font* font);
			TTF_Font* openFont(LIB_FFMPEG::AVFormatContext* formatContext);
			TTF_Font* openFontDisk();
			TTF_Font* openFontInternal(LIB_FFMPEG::AVFormatContext* formatContext);
			bool      streamHasExtraData(LIB_FFMPEG::AVStream* stream);

		};
	}
}

#endif
