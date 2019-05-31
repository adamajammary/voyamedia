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
			static String       RemoveFormatting(const String &subtitleString);
			static void         RemoveSubs(size_t id);
			static int          RenderSubText(const VM_Subtitles &subs, TTF_Font* fontMerged, TTF_Font* fontCJK);
			static VM_Subtitles SplitAndFormatSub(const VM_Subtitle &subtitle, const Strings &subTexts, const VM_SubStyles &subStyles, const VM_Subtitles &playerSubs);

		private:
			static VM_SubTexture* createSubFill(uint16_t* subString16, VM_Subtitle* sub);
			static VM_SubTexture* createSubOutline(VM_SubTexture* subFill);
			static VM_SubTexture* createSubShadow(VM_SubTexture* subFill);
			static bool           formatAnimationsContain(const Strings &animations, const String &string);
			static String         formatDialogue(const String &dialogueText, Strings &dialogueSplit);
			static void           formatDrawCommand(const String &subText, const Strings &subSplit, const VM_Subtitle &subtitle, size_t subID, const VM_SubStyles &subStyles, VM_Subtitles &subs);
			static Strings        formatGetAnimations(const String &subString);
			static void           formatOverrideMargins(VM_Subtitle* sub);
			static void           formatOverrideStyleCat1(const Strings &animations, VM_Subtitle* sub);
			static void           formatOverrideStyleCat2(const Strings &animations, VM_Subtitle* sub, const VM_SubStyles &subStyles);
			static String         formatRemoveAnimations(const String &subString);
			static void           formatSetLayer(VM_Subtitle* sub, const VM_Subtitles &playerSubs);
			static bool           formatSplitStyling(const String &subText, Strings &subLines);
			static int            getDialoguePropOffset(const Strings &dialogueSplit);
			static SDL_Rect       getDrawRect(const String &subLine, VM_SubStyle* style);
			static VM_SubStyle*   getSubStyle(const VM_SubStyles &subStyles, const Strings &subSplit);
			static void           removeSubs(VM_SubTexturesId &subs, size_t id);
			static int            renderSub(VM_SubTexture* subTexture);
			static int            renderSubBorderShadow(VM_SubTexture* subTexture);
			static void           renderSubs(VM_SubTexturesId &subs);
			static void           renderSubsPositionAbsolute(VM_SubTexturesId &subs);
			static void           renderSubsPositionRelative(VM_SubTexturesId &subs);
			static void           setSubPositionAbsolute(const VM_SubTexturesId &subs);
			static void           setSubPositionRelativeTop(const VM_SubTexturesId &subs);
			static void           setTotalWidthAbsolute(const VM_SubTexturesId &subs);
			static void           setTotalWidthRelative(const VM_SubTexturesId &subs);
			static int            splitSub(uint16_t* subStringUTF16, VM_Subtitle* sub, VM_Subtitle* prevSub, std::vector<uint16_t*> &subStrings16);

		};
	}
}

#endif
