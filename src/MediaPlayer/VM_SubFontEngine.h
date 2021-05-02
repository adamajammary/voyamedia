#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_SUBFONTENGINE_H
#define VM_SUBFONTENGINE_H

namespace VoyaMedia
{
	namespace MediaPlayer
	{
		class VM_SubFontEngine
		{
		private:
			VM_SubFontEngine()  {}
			~VM_SubFontEngine() {}

		private:
			static Graphics::VM_Color drawColor;
			static VM_SubTexturesId   subsBottom;
			static VM_SubTexturesId   subsMiddle;
			static VM_SubTexturesId   subsPosition;
			static VM_SubTexturesId   subsTop;

		public:
			static double       GetSubEndPTS(LIB_FFMPEG::AVPacket* packet, LIB_FFMPEG::AVSubtitle &subFrame, LIB_FFMPEG::AVStream* subStream);
			static VM_PTS       GetSubPTS(LIB_FFMPEG::AVPacket* packet, LIB_FFMPEG::AVSubtitle &subFrame, LIB_FFMPEG::AVStream* subStream);
			static String       RemoveFormatting(const String &subtitleString);
			static void         RemoveSubs();
			static void         RemoveSubs(size_t id);
			static void         RemoveSubsBottom();
			static int          RenderSubText(const VM_PlayerSubContext &subContext);
			static VM_Subtitles SplitAndFormatSub(const Strings &subTexts, const VM_PlayerSubContext &subContext);

		private:
			static VM_SubTexture* createSubFill(uint16_t* subString16, VM_Subtitle* sub);
			static VM_SubTexture* createSubOutline(VM_SubTexture* subFill);
			static VM_SubTexture* createSubShadow(VM_SubTexture* subFill);
			static bool           formatAnimationsContain(const Strings &animations, const String &string);
			static String         formatDialogue(const String &dialogueText, Strings &dialogueSplit, size_t nrStyles);
			static void           formatDrawCommand(const String &subText, const Strings &subSplit, size_t subID, VM_Subtitles &subs, const VM_PlayerSubContext &subContext);
			static Strings        formatGetAnimations(const String &subString);
			static void           formatOverrideMargins(VM_Subtitle* sub);
			static void           formatOverrideStyleCat1(const Strings &animations, VM_Subtitle* sub, const VM_SubStyles &subStyles);
			static void           formatOverrideStyleCat2(const Strings &animations, VM_Subtitle* sub, const VM_SubStyles &subStyles);
			static String         formatRemoveAnimations(const String &subString);
			static void           formatSetLayer(VM_Subtitle* sub, const VM_Subtitles &playerSubs);
			static Strings        formatSplitStyling(const String &subText, VM_SubStyle* subStyle);
			static size_t         getDialoguePropOffset(const Strings &dialogueSplit);
			static SDL_Rect       getDrawRect(const String &subLine, VM_SubStyle* style);
			static VM_SubStyle*   getSubStyle(const VM_SubStyles &subStyles, const Strings &subSplit);
			static void           handleSubCollisions(const VM_SubTextureId &subTextures, const VM_SubTexturesId &subs);
			static int            handleSubsOutOfBound(const VM_SubTextureId &subTextures);
			static String         removeInvalidFormatting(const String &subtitleString);
			static void           removeSubs(VM_SubTexturesId &subs);
			static void           removeSubs(VM_SubTexturesId &subs, size_t id);
			static int            renderSub(VM_SubTexture* subTexture);
			static int            renderSubBorderShadow(VM_SubTexture* subTexture);
			static void           renderSubs(VM_SubTexturesId &subs);
			static void           setSubPositionAbsolute(const VM_SubTexturesId &subs);
			static void           setSubPositionRelative(const VM_SubTexturesId &subs);
			static void           setTotalWidth(const VM_SubTexturesId &subs);
			static Strings16      splitSub(uint16_t* subStringUTF16, int subStringWidth, VM_Subtitle* sub, TTF_Font* font, int subWidth, int maxWidth);
			static size_t         splitSubGetNrLines(const Strings &words, TTF_Font* font, int maxWidth);
			static Strings16      splitSubDistributeByLines(const Strings &words, size_t nrLines, TTF_Font* font, int maxWidth);
			static Strings16      splitSubDistributeByWidth(const Strings &words, TTF_Font* font, int remainingWidth, int maxWidth);

		};
	}
}

#endif
