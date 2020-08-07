#include "VM_SubFontEngine.h"

using namespace VoyaMedia::Graphics;
using namespace VoyaMedia::System;

VM_Color                      MediaPlayer::VM_SubFontEngine::drawColor = VM_Color(SDL_COLOR_BLACK);
MediaPlayer::VM_SubTexturesId MediaPlayer::VM_SubFontEngine::subsBottom;
MediaPlayer::VM_SubTexturesId MediaPlayer::VM_SubFontEngine::subsMiddle;
MediaPlayer::VM_SubTexturesId MediaPlayer::VM_SubFontEngine::subsPosition;
MediaPlayer::VM_SubTexturesId MediaPlayer::VM_SubFontEngine::subsTop;

MediaPlayer::VM_SubTexture* MediaPlayer::VM_SubFontEngine::createSubFill(uint16_t* subString16, VM_Subtitle* sub)
{
	if (sub == NULL)
		return NULL;

	TTF_Font* font = sub->getFont();

	if (font == NULL)
		return NULL;

	// Disable font outline - Primary/Fill sub
	TTF_SetFontOutline(font, 0);

	if (sub->style != NULL)
		TTF_SetFontStyle(font, sub->style->fontStyle);

	SDL_Surface* surface = TTF_RenderUNICODE_Blended(font, subString16, sub->getColor());

	if (surface == NULL)
		return NULL;

	int blur = sub->getBlur();

	if ((sub->getOutline() == 0) && (blur > 0)) {
		VM_Surface surface2(surface);
		surface2.blur(blur);
	}

	VM_SubTexture* subFill = new VM_SubTexture(sub);

	subFill->textUTF16   = subString16;
	subFill->textureData = new VM_Texture(surface);

	FREE_SURFACE(surface);

	if (subFill->textureData->data == NULL) {
		DELETE_POINTER(subFill);
		return NULL;
	}

	// Scale texture in X and/or Y dimensions
	if (sub->style != NULL)
	{
		if (sub->style->fontScale.x > FLOAT_MIN_ZERO)
			subFill->textureData->width = (int)((float)subFill->textureData->width * sub->style->fontScale.x);

		if (sub->style->fontScale.y > FLOAT_MIN_ZERO)
			subFill->textureData->height = (int)((float)subFill->textureData->height * sub->style->fontScale.y);
	}

	return subFill;
}

MediaPlayer::VM_SubTexture* MediaPlayer::VM_SubFontEngine::createSubOutline(VM_SubTexture* subFill)
{
	if ((subFill == NULL) || (subFill->subtitle == NULL))
		return NULL;

	TTF_Font* font = subFill->subtitle->getFont();

	if (font == NULL)
		return NULL;

	TTF_SetFontOutline(font, subFill->subtitle->getOutline());

	if (subFill->subtitle->style != NULL)
		TTF_SetFontStyle(font, subFill->subtitle->style->fontStyle);

	SDL_Surface* surface = TTF_RenderUNICODE_Blended(
		font, subFill->textUTF16, subFill->subtitle->getColorOutline()
	);

	if (surface == NULL)
		return NULL;

	int blur = subFill->subtitle->getBlur();

	if (blur > 0) {
		VM_Surface surface2(surface);
		surface2.blur(blur);
	}

	VM_SubTexture* subOutline = new VM_SubTexture(*subFill);

	subOutline->textureData = new VM_Texture(surface);

	FREE_SURFACE(surface);

	if (subOutline->textureData->data == NULL)
		return NULL;

	// Scale texture in X and/or Y dimensions
	if (subFill->subtitle->style != NULL)
	{
		if (subFill->subtitle->style->fontScale.x > FLOAT_MIN_ZERO)
			subOutline->textureData->width = (int)((float)subOutline->textureData->width * subFill->subtitle->style->fontScale.x);

		if (subFill->subtitle->style->fontScale.y > FLOAT_MIN_ZERO)
			subOutline->textureData->height = (int)((float)subOutline->textureData->height * subFill->subtitle->style->fontScale.y);
	}

	int outline = TTF_GetFontOutline(font);

	subOutline->locationRender.x -= outline;
	subOutline->locationRender.y -= outline;
	subOutline->locationRender.w  = subOutline->textureData->width;
	subOutline->locationRender.h  = subOutline->textureData->height;

	return subOutline;
}

MediaPlayer::VM_SubTexture* MediaPlayer::VM_SubFontEngine::createSubShadow(VM_SubTexture* subFill)
{
	if (subFill == NULL)
		return NULL;

	TTF_Font* font = subFill->subtitle->getFont();

	if (font == NULL)
		return NULL;

	TTF_SetFontOutline(font, subFill->subtitle->getOutline());

	if (subFill->subtitle->style != NULL)
		TTF_SetFontStyle(font, subFill->subtitle->style->fontStyle);

	SDL_Surface* surface = TTF_RenderUNICODE_Blended(
		font, subFill->textUTF16, subFill->subtitle->getColorShadow()
	);

	if (surface == NULL)
		return NULL;

	int blur = subFill->subtitle->getBlur();

	if (blur > 0) {
		VM_Surface surface2(surface);
		surface2.blur(blur);
	}

	VM_SubTexture* subShadow = new VM_SubTexture(*subFill);

	subShadow->textureData = new VM_Texture(surface);

	FREE_SURFACE(surface);

	if (subShadow->textureData->data == NULL)
		return NULL;

	// Scale texture in X and/or Y dimensions
	if (subFill->subtitle->style != NULL)
	{
		if (subFill->subtitle->style->fontScale.x > FLOAT_MIN_ZERO)
			subShadow->textureData->width = (int)((float)subShadow->textureData->width * subFill->subtitle->style->fontScale.x);

		if (subFill->subtitle->style->fontScale.y > FLOAT_MIN_ZERO)
			subShadow->textureData->height = (int)((float)subShadow->textureData->height * subFill->subtitle->style->fontScale.y);
	}

	int       outline = TTF_GetFontOutline(font);
	SDL_Point shadow  = subFill->subtitle->getShadow();

	subShadow->locationRender.x += (shadow.x - outline);
	subShadow->locationRender.y += (shadow.y - outline);
	subShadow->locationRender.w  = subShadow->textureData->width;
	subShadow->locationRender.h  = subShadow->textureData->height;

	return subShadow;
}

bool MediaPlayer::VM_SubFontEngine::formatAnimationsContain(const Strings &animations, const String &string)
{
	for (const auto &animation : animations) {
		if (animation.find(string) != String::npos)
			return true;
	}

	return false;
}

String MediaPlayer::VM_SubFontEngine::formatDialogue(const String &dialogueText, Strings &dialogueSplit)
{
	String subText = String(dialogueText);

	// Format the dialogue properties
	if (dialogueSplit.size() >= SUB_DIALOGUE_TEXT)
	{
		// "Dialogue: 0" => "0"
		size_t findPos = dialogueSplit[SUB_DIALOGUE_LAYER].find(": ");

		if (findPos != String::npos)
			dialogueSplit[SUB_DIALOGUE_LAYER] = dialogueSplit[SUB_DIALOGUE_LAYER].substr(findPos + 2);

		int propOffset = VM_SubFontEngine::getDialoguePropOffset(dialogueSplit);

		// ",,,,,,,,,text" => "text"
		for (int i = 0; i < SUB_DIALOGUE_TEXT - propOffset; i++)
			subText = subText.substr(subText.find(",") + 1);
	} else {
		subText = subText.substr(subText.rfind(",,") + 2);
	}

	if (subText.empty())
		return subText;

	subText = VM_Text::Replace(subText, "\\N", "^");
	subText = VM_Text::Replace(subText, "\\n", (subText.find("\\q2") != String::npos ? "^" : " "));
	subText = VM_Text::Replace(subText, "\\h", " ");

	size_t findPos = subText.rfind("\r\n");

	if (findPos != String::npos)
		subText = subText.substr(0, findPos);

	// {=43}{\f} => {\f}
	if (subText.substr(0, 2) == "{=") {
		findPos = subText.find("}{");
		subText = subText.substr(findPos + 1);
	}

	// {\f1}{\f2} => {\f1\f2}
	subText = VM_Text::Replace(subText, "}{", "");

	return subText;
}

void MediaPlayer::VM_SubFontEngine::formatDrawCommand(
	const String &subText, const Strings &subSplit, size_t subID,
	const VM_SubStyles &subStyles, VM_Subtitles &subs
)
{
	// Custom draw operation (fill rect)
	if (subText.find("\\p1") != String::npos)
	{
		VM_SubStyle* style    = VM_SubFontEngine::getSubStyle(subStyles, subSplit);
		SDL_Rect     drawRect = VM_SubFontEngine::getDrawRect(subText, style);

		if (!SDL_RectEmpty(&drawRect))
		{
			VM_Subtitle* sub = new VM_Subtitle();

			sub->id       = subID;
			sub->drawRect = drawRect;
			sub->style    = style;

			subs.push_back(sub);
		} else {
			DELETE_POINTER(style);
		}
	}
}

Strings MediaPlayer::VM_SubFontEngine::formatGetAnimations(const String &subString)
{
	Strings animations;
	size_t  findPos1, findPos2;
	String  tempString1, tempString2;
	String  formattedString = String(subString);

	findPos1 = formattedString.find("\\t(");
	findPos2 = formattedString.rfind(")");

	while ((findPos1 != String::npos) && (findPos2 != String::npos) && (findPos2 > findPos1))
	{
		tempString1     = formattedString.substr(0, findPos1);
		tempString2     = formattedString.substr(findPos1);
		formattedString = tempString1.append(tempString2.substr(tempString2.find(")") + 1));

		animations.push_back(tempString2.substr(0, tempString2.find(")") + 1));

		findPos1 = formattedString.find("\\t(");
		findPos2 = formattedString.rfind(")");
	}

	return animations;
}

void MediaPlayer::VM_SubFontEngine::formatOverrideMargins(VM_Subtitle* sub)
{
	// Dialogue: Marked/Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, VM_Text
	if (!sub->customPos && (sub->textSplit.size() > SUB_DIALOGUE_MARGINV))
	{
		int propOffset = VM_SubFontEngine::getDialoguePropOffset(sub->textSplit);
		int marginL    = std::atoi(sub->textSplit[(int)SUB_DIALOGUE_MARGINL - propOffset].c_str());
		int marginR    = std::atoi(sub->textSplit[(int)SUB_DIALOGUE_MARGINR - propOffset].c_str());
		int marginV    = std::atoi(sub->textSplit[(int)SUB_DIALOGUE_MARGINV - propOffset].c_str());

		if ((marginL > 0) || (marginR > 0) || (marginV > 0))
		{
			sub->style->marginL = (marginL > 0 ? marginL : sub->style->marginL);
			sub->style->marginR = (marginR > 0 ? marginR : sub->style->marginR);
			sub->style->marginV = (marginV > 0 ? marginV : sub->style->marginV);

			sub->customMargins = true;
			sub->customPos     = true;
		}
	}
}

void MediaPlayer::VM_SubFontEngine::formatOverrideStyleCat1(const Strings &animations, VM_Subtitle* sub)
{
	// STYLE OVERRIDERS - Category 1 - Affects the entire sub
	Strings styleProps;
	size_t  findPos = sub->text3.find("{\\");

	if (findPos != String::npos)
		styleProps = VM_Text::Split(sub->text3, "\\");

	bool aligned = false;

	for (const auto &prop : styleProps)
	{
		// ALIGNMENT - NUMPAD
		if ((prop.substr(0, 2) == "an") && isdigit(prop[2]))
		{
			if (!aligned)
				sub->style->alignment = (VM_SubAlignment)std::atoi(prop.substr(2).c_str());

			aligned = true;
		}
		// ALIGNMENT - LEGACY
		else if ((prop[0] == 'a') && isdigit(prop[1]))
		{
			if (!aligned)
				sub->style->alignment = VM_SubStyle::ToSubAlignment(std::atoi(prop.substr(1).c_str()));

			aligned = true;
		}
		// CLIP
		else if ((prop.substr(0, 5) == "clip("))
		{
			Strings clipProps = VM_Text::Split(prop.substr(5, (prop.size() - 1)), ",");

			if ((clipProps.size() > 3) && !VM_SubFontEngine::formatAnimationsContain(animations, "\\clip"))
			{
				sub->clip.x = std::atoi(clipProps[0].c_str());
				sub->clip.y = std::atoi(clipProps[1].c_str());
				sub->clip.w = (std::atoi(clipProps[2].c_str()) - sub->clip.x);
				sub->clip.h = (std::atoi(clipProps[3].c_str()) - sub->clip.y);

				sub->customClip = true;
			}
		}
		// POSITION
		else if (prop.substr(0, 4) == "pos(")
		{
			sub->position.x = std::atoi(prop.substr(4).c_str());
			sub->position.y = std::atoi(prop.substr(prop.find(",") + 1).c_str());

			sub->customPos = true;
		}
		// MOVE (POSITION)
		else if (prop.substr(0, 5) == "move(")
		{
			Strings moveProps = VM_Text::Split(prop.substr(prop.find("(") + 1), ",");
				
			if (moveProps.size() > 3)
			{
				sub->position.x = std::atoi(moveProps[2].c_str());
				sub->position.y = std::atoi(moveProps[3].c_str());

				sub->customPos = true;
			}
		}
		// ROTATION POINT - ORIGIN
		else if (prop.substr(0, 4) == "org(")
		{
			sub->rotationPoint = {
				std::atoi(prop.substr(4).c_str()),
				std::atoi(prop.substr(prop.find(",") + 1).c_str())
			};

			sub->customRotation = true;
		}
	}
}

void MediaPlayer::VM_SubFontEngine::formatOverrideStyleCat2(const Strings &animations, VM_Subtitle* sub, const VM_SubStyles &subStyles)
{
	// STYLE OVERRIDERS - Category 2 - Affects only preceding text
	Strings styleProps;
	size_t  findPos = sub->text2.find("{\\");

	if (findPos == 0)
		styleProps = VM_Text::Split(sub->text2.substr((findPos + 2), (sub->text2.find("}") - (findPos + 2))), "\\");

	for (const auto &prop : styleProps)
	{
		// COLOR - PRIMARY FILL
		if (prop.substr(0, 3) == "c&H")
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\c&H")) {
				VM_Color color           = VM_Graphics::ToVMColor(prop.substr(1, 8));
				sub->style->colorPrimary = { color.r, color.g, color.b, sub->style->colorPrimary.a };
			}
		}
		// COLOR - RESET TO DEFAULT
		else if (prop == "c")
		{
			auto defaultStyle = sub->style->getDefault(subStyles);

			if (defaultStyle != NULL)
				sub->style->colorPrimary = defaultStyle->colorPrimary;
		}
		// COLOR - PRIMARY FILL
		else if (prop.substr(0, 4) == "1c&H")
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\1c&H")) {
				VM_Color color           = VM_Graphics::ToVMColor(prop.substr(2, 8));
				sub->style->colorPrimary = { color.r, color.g, color.b, sub->style->colorPrimary.a };
			}
		}
		// COLOR - SECONDARY FILL
		else if (prop.substr(0, 4) == "2c&H")
		{
			//
		}
		// COLOR - OUTLINE BORDER
		else if (prop.substr(0, 4) == "3c&H")
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\3c&H")) {
				VM_Color color           = VM_Graphics::ToVMColor(prop.substr(2, 8));
				sub->style->colorOutline = { color.r, color.g, color.b, sub->style->colorOutline.a };
			}
		}
		// COLOR - SHADOW
		else if (prop.substr(0, 4) == "4c&H")
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\4c&H")) {
				VM_Color color          = VM_Graphics::ToVMColor(prop.substr(2, 8));
				sub->style->colorShadow = { color.r, color.g, color.b, sub->style->colorShadow.a };
			}
		}
		// ALPHA
		else if (prop.substr(0, 7) == "alpha&H")
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\alpha&H")) {
				sub->style->colorPrimary.a = (0xFF - HEX_STR_TO_UINT(prop.substr(7, 2)));
				sub->style->colorOutline.a = sub->style->colorPrimary.a;
				sub->style->colorShadow.a  = sub->style->colorPrimary.a;
			}
		}
		else if (prop.substr(0, 4) == "1a&H")
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\1a&H"))
				sub->style->colorPrimary.a = (0xFF - HEX_STR_TO_UINT(prop.substr(4, 2)));
		}
		// ALPHA - SECONDARY FILL
		else if (prop.substr(0, 4) == "2a&H")
		{
			//
		}
		// ALPHA - OUTLINE BORDER
		else if (prop.substr(0, 4) == "3a&H")
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\3a&H"))
				sub->style->colorOutline.a = (0xFF - HEX_STR_TO_UINT(prop.substr(4, 2)));
		}
		// ALPHA - SHADOW
		else if (prop.substr(0, 4) == "4a&H")
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\4a&H"))
				sub->style->colorShadow.a = (0xFF - HEX_STR_TO_UINT(prop.substr(4, 2)));
		}
		// OUTLINE BORDER WIDTH
		else if ((prop.substr(0, 4) == "bord") && isdigit(prop[4]))
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\bord"))
				sub->style->outline = (int)std::round(std::atof(prop.substr(4).c_str()));
		}
		// SHADOW DEPTH
		else if ((prop.substr(0, 4) == "shad") && (isdigit(prop[4]) || (prop[4] == '-')))
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\shad")) {
				int shadow         = (int)std::round(std::atof(prop.substr(4).c_str()));
				sub->style->shadow = { shadow, shadow };
			}
		}
		else if ((prop.substr(0, 5) == "xshad") && (isdigit(prop[5]) || (prop[5] == '-')))
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\xshad"))
				sub->style->shadow.x = (int)std::round(std::atof(prop.substr(5).c_str()));
		}
		else if ((prop.substr(0, 5) == "yshad") && (isdigit(prop[5]) || (prop[5] == '-')))
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\yshad"))
				sub->style->shadow.y = (int)std::round(std::atof(prop.substr(5).c_str()));
		}
		// BLUR EFFECT
		else if ((prop.substr(0, 4) == "blur") && isdigit(prop[4]))
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\blur"))
				sub->style->blur = (int)std::round(std::atof(prop.substr(4).c_str()));
		}
		// FONT - NAME
		else if ((prop.substr(0, 2) == "fn") && isdigit(prop[2]))
		{
			String font = prop.substr(2);

			if (!font.empty() && (font[font.size() - 1] == '}'))
				font = font.substr(0, font.size() - 1);

			#if defined _windows
				sub->style->fontName = VM_Text::ToUTF16(font.c_str());
			#else
				sub->style->fontName = font;
			#endif
		}
		// FONT - SCALE X
		else if ((prop.substr(0, 4) == "fscx") && isdigit(prop[4]))
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\fscx"))
				sub->style->fontScale.x = (float)(std::atof(prop.substr(4).c_str()) * 0.01);
		}
		// FONT - SCALE Y
		else if ((prop.substr(0, 4) == "fscy") && isdigit(prop[4]))
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\fscy"))
				sub->style->fontScale.y = (float)(std::atof(prop.substr(4).c_str()) * 0.01);
		}
		// FONT - LETTER SPACING
		else if ((prop.substr(0, 3) == "fsp") && isdigit(prop[3]))
		{
			//
		}
		// FONT - SIZE
		else if ((prop.substr(0, 2) == "fs") && isdigit(prop[2]))
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\fs"))
				//sub->style->fontSize = (int)((float)std::atof(prop.substr(2).c_str()) * DEFAULT_FONT_DPI_RATIO);
				sub->style->fontSize = std::atoi(prop.substr(2).c_str());
		}
		// FONT STYLING - BOLD
		else if (prop == "b1")
		{
			sub->style->fontStyle |= TTF_STYLE_BOLD;
		}
		// FONT STYLING - RESET BOLD
		else if (prop == "b0")
		{
			sub->style->fontStyle &= ~TTF_STYLE_BOLD;
		}
		// FONT STYLING - ITALIC
		else if (prop == "i1")
		{
			sub->style->fontStyle |= TTF_STYLE_ITALIC;
		}
		// FONT STYLING - RESET ITALIC
		else if (prop == "i0")
		{
			sub->style->fontStyle &= ~TTF_STYLE_ITALIC;
		}
		// FONT STYLING - STRIKEOUT
		else if (prop == "s1")
		{
			sub->style->fontStyle |= TTF_STYLE_STRIKETHROUGH;
		}
		// FONT STYLING - RESET STRIKEOUT
		else if (prop == "s0")
		{
			sub->style->fontStyle &= ~TTF_STYLE_STRIKETHROUGH;
		}
		// FONT STYLING - UNDERLINE
		else if (prop == "u1")
		{
			sub->style->fontStyle |= TTF_STYLE_UNDERLINE;
		}
		// FONT STYLING - RESET UNDERLINE
		else if (prop == "u0")
		{
			sub->style->fontStyle &= ~TTF_STYLE_UNDERLINE;
		}
		// ROTATION - DEGREES
		else if ((prop.substr(0, 3) == "frx") && (isdigit(prop[3]) || (prop[3] == '-')))
		{
			//
		}
		else if ((prop.substr(0, 3) == "fry") && (isdigit(prop[3]) || (prop[3] == '-')))
		{
			//
		}
		else if ((prop.substr(0, 3) == "frz") && (isdigit(prop[3]) || (prop[3] == '-')))
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\frz"))
				sub->rotation = (360.0 - (double)(std::atoi(prop.substr(3).c_str()) % 360));
		}
		else if ((prop.substr(0, 2) == "fr") && (isdigit(prop[2]) || (prop[2] == '-')))
		{
			if (!VM_SubFontEngine::formatAnimationsContain(animations, "\\fr"))
				sub->rotation = (360.0 - (double)(std::atoi(prop.substr(2).c_str()) % 360));
		}
	}
}

String MediaPlayer::VM_SubFontEngine::formatRemoveAnimations(const String &subString)
{
	size_t findPos1, findPos2;
	String tempString1, tempString2;
	String formattedString = String(subString);

	findPos1 = formattedString.find("\\t(");
	findPos2 = formattedString.rfind(")");

	while ((findPos1 != String::npos) && (findPos2 != String::npos) && (findPos2 > findPos1))
	{
		tempString1 = formattedString.substr(0, findPos1);
		tempString2 = formattedString.substr(findPos1);
		tempString2 = tempString2.substr(tempString2.find(")") + 1);

		formattedString = tempString1.append(tempString2);

		findPos1 = formattedString.find("\\t(");
		findPos2 = formattedString.rfind(")");
	}

	return formattedString;
}

void MediaPlayer::VM_SubFontEngine::formatSetLayer(VM_Subtitle* sub, const VM_Subtitles &playerSubs)
{
	// Set rendering layer
	VM_Subtitles duplicateSubs = sub->getDuplicateSubs(playerSubs);

	if (!duplicateSubs.empty())
	{
		duplicateSubs.push_back(sub);

		for (auto dsub : duplicateSubs)
			dsub->layer = std::atoi(dsub->textSplit[0].c_str());
	}
}

bool MediaPlayer::VM_SubFontEngine::formatSplitStyling(const String &subText, Strings &subLines)
{
	bool splitStyling = false;

	// Split the sub string by partial formatting ({\f1}t1{\f2}t2)
	if ((subText.find("{") != subText.rfind("{")) && VM_Text::IsValidSubtitle(subText))
	{
		splitStyling = true;

		Strings subLines2 = VM_Text::Split(subText, "^", true);

		for (const auto &subLine2 : subLines2)
		{
			Strings subLines3 = VM_Text::Split(subLine2, "{", false);

			if (subLine2.empty())
				subLines.push_back("^");

			for (auto &subLine3 : subLines3)
			{
				String subLine4 = subLine3;

				size_t findPos = subLine3.rfind("}");

				if (findPos != String::npos) {
					subLine4 = subLine3.substr(findPos + 1);
					subLine3 = ("{" + subLine3);
				}

				if ((subLine4 != "\r\n") && !subLine4.empty())
					subLines.push_back(subLine3);
			}

			if (!subLines.empty() && !subLines3.empty())
				subLines[subLines.size() - 1].append("^");
		}
	// Split the sub string by newlines (\\N => ^)
	} else {
		subLines = VM_Text::Split(subText, "^");
	}

	return splitStyling;
}

int MediaPlayer::VM_SubFontEngine::getDialoguePropOffset(const Strings &dialogueSplit)
{
	// Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
	// Dialogue: 0,0:00:08.08,0:00:11.32,GitS 2nd Gig OP Credits,,0000,0000,0000,,{\i1}I'm a soldier,{\i0} znatchit ya...
	// 0,Marked=0,Style1,Comment,0000,0000,0000,,MOTOKO!
	// 2,,Wolf main,autre,0000,0000,0000,,Toujours rien.
	if ((dialogueSplit[SUB_DIALOGUE_START][1] != ':') && (dialogueSplit[SUB_DIALOGUE_END][1] != ':'))
		return 1;

	return 0;
}

SDL_Rect MediaPlayer::VM_SubFontEngine::getDrawRect(const String &subLine, VM_SubStyle* style)
{
	SDL_Rect drawRect     = {};
	String   drawLine     = VM_SubFontEngine::RemoveFormatting(subLine);
	Strings  drawCommands = VM_Text::Split(drawLine, " ");

	// m  1494  212    l  1494 398     383   411       374 209
	// m  160.5 880.5  l  282  879     283.5 610.5     456 606  459 133.5  157.5 133.5
	// m  1494  212    l  1494 398  l  383   411    l  374 209
	if ((drawCommands.size() < 10) || (drawCommands[0] != "m") || (drawCommands[3] != "l"))
		return drawRect;

	SDL_Point startPosition   = {};
	SDL_Point minDrawPosition = { std::atoi(drawCommands[1].c_str()), std::atoi(drawCommands[2].c_str()) };
	SDL_Point maxDrawPosition = SDL_Point(minDrawPosition);

	for (size_t i = 4; i < drawCommands.size(); i += 2)
	{
		SDL_Point p = { std::atoi(drawCommands[i].c_str()), std::atoi(drawCommands[i + 1].c_str()) };

		if (p.x < minDrawPosition.x)
			minDrawPosition.x = p.x;
		if (p.y < minDrawPosition.y)
			minDrawPosition.y = p.y;

		if (p.x > maxDrawPosition.x)
			maxDrawPosition.x = p.x;
		if (p.y > maxDrawPosition.y)
			maxDrawPosition.y = p.y;

		if ((i + 2 < drawCommands.size()) && !drawCommands[i + 2].empty()) {
			if (drawCommands[i + 2] == "l")
				i++;
			else if (!isdigit(drawCommands[i + 2][0]))
				return drawRect;
		}
	}

	VM_SubAlignment alignment = (style != NULL ? style->alignment : SUB_ALIGN_TOP_LEFT);
	SDL_FPoint      fontScale = (style != NULL ? style->fontScale : SDL_FPoint { 1.0f, 1.0f });
	Strings         styleProps = VM_Text::Split(subLine, "\\");

	// {\pos(0,0)\an7\1c&HD9F0F5&\p1}m 1494 212 l 1494 398 383 411 374 209{\p0}
	for (const auto &prop : styleProps)
	{
		if (prop.substr(0, 4) == "1a&H") {
			VM_SubFontEngine::drawColor.a = (0xFF - HEX_STR_TO_UINT(prop.substr(4, 2)));
		} else if (prop.substr(0, 3) == "c&H") {
			VM_Color color = VM_Graphics::ToVMColor(prop.substr(1, 8));
			VM_SubFontEngine::drawColor = { color.r, color.g, color.b, VM_SubFontEngine::drawColor.a };
		} else if (prop.substr(0, 4) == "1c&H") {
			VM_Color color = VM_Graphics::ToVMColor(prop.substr(2, 8));
			VM_SubFontEngine::drawColor = { color.r, color.g, color.b, VM_SubFontEngine::drawColor.a };
		}

		if (prop.substr(0, 2) == "an") {
			alignment = (VM_SubAlignment)std::atoi(prop.substr(2).c_str());
		} else if (prop[0] == 'a') {
			alignment = VM_SubStyle::ToSubAlignment(std::atoi(prop.substr(1).c_str()));
		} else if (prop.substr(0, 4) == "fscx") {
			fontScale.x = (float)(std::atof(prop.substr(4).c_str()) * 0.01);
		} else if (prop.substr(0, 4) == "fscy") {
			fontScale.y = (float)(std::atof(prop.substr(4).c_str()) * 0.01);
		} else if (prop.substr(0, 4) == "pos(") {
			startPosition.x = std::atoi(prop.substr(4).c_str());
			startPosition.y = std::atoi(prop.substr(prop.find(",") + 1).c_str());
		}
	}

	if ((startPosition.x != 0) && (startPosition.y != 0)) {
		drawRect.x = (startPosition.x + minDrawPosition.x);
		drawRect.y = (startPosition.y + minDrawPosition.y);
		drawRect.w = (max(startPosition.x + maxDrawPosition.x, drawRect.x) - drawRect.x);
		drawRect.h = (max(startPosition.y + maxDrawPosition.y, drawRect.y) - drawRect.y);
	} else {
		drawRect.x = minDrawPosition.x;
		drawRect.y = minDrawPosition.y;
		drawRect.w = (max(maxDrawPosition.x, drawRect.x) - drawRect.x);
		drawRect.h = (max(maxDrawPosition.y, drawRect.y) - drawRect.y);
	}

	if (fontScale.x > FLOAT_MIN_ZERO)
		drawRect.w = (int)((float)drawRect.w * fontScale.x);

	if (fontScale.y > FLOAT_MIN_ZERO)
		drawRect.h = (int)((float)drawRect.h * fontScale.y);

	if (VM_SubStyle::IsAlignedRight(alignment))
		drawRect.x -= drawRect.w;
	else if (VM_SubStyle::IsAlignedCenter(alignment))
		drawRect.x -= (drawRect.w / 2);

	if (VM_SubStyle::IsAlignedBottom(alignment))
		drawRect.y -= drawRect.h;
	else if (VM_SubStyle::IsAlignedMiddle(alignment))
		drawRect.y -= (drawRect.h / 2);

	return drawRect;
}


MediaPlayer::VM_PTS MediaPlayer::VM_SubFontEngine::GetSubPTS(LIB_FFMPEG::AVPacket* packet, LIB_FFMPEG::AVSubtitle &subFrame, LIB_FFMPEG::AVStream* subStream)
{
	VM_PTS pts = {};

	if ((packet == NULL) || (subStream == NULL))
		return pts;

	bool useFrame = (packet->dts < subStream->cur_dts);

	// NO DURATION - UPDATE END PTS
	if ((subFrame.num_rects == 0) && (packet->size > 0))
	{
		if (useFrame)
			pts.end = (double)((double)subFrame.pts / AV_TIME_BASE_D);
		else
			pts.end = (double)((double)packet->pts * LIB_FFMPEG::av_q2d(subStream->time_base));
	}
	else
	{
		// SET START PTS
		if (useFrame) {
			pts.start = (double)((double)(subFrame.pts - subStream->start_time) / AV_TIME_BASE_D);
		} else {
			pts.start = (double)((double)(packet->pts  - subStream->start_time) * LIB_FFMPEG::av_q2d(subStream->time_base));

			if (pts.start < VM_Player::ProgressTime)
				pts.start += (double)((double)subStream->start_time * LIB_FFMPEG::av_q2d(subStream->time_base));
		}

		if (subFrame.start_display_time > 0)
			pts.start += (double)((double)subFrame.start_display_time / (double)ONE_SECOND_MS);

		// SET END PTS
		if (subFrame.end_display_time > 0)
			pts.end = (double)(pts.start + (double)((double)subFrame.end_display_time / (double)ONE_SECOND_MS));
		else if (packet->duration > 0)
			pts.end = (double)(pts.start + (double)((double)packet->duration * LIB_FFMPEG::av_q2d(subStream->time_base)));
		else
			pts.end = (pts.start + SUB_MAX_DURATION);
	}

	return pts;
}


MediaPlayer::VM_SubStyle* MediaPlayer::VM_SubFontEngine::getSubStyle(const VM_SubStyles &subStyles, const Strings &dialogueSplit)
{
	if (dialogueSplit.size() < SUB_DIALOGUE_TEXT)
		return NULL;

	int propOffset = VM_SubFontEngine::getDialoguePropOffset(dialogueSplit);

	for (auto style : subStyles) {
		if (VM_Text::ToLower(style->name) == VM_Text::ToLower(dialogueSplit[(int)SUB_DIALOGUE_STYLE - propOffset]))
			return new VM_SubStyle(*style);
	}

	return NULL;
}

String MediaPlayer::VM_SubFontEngine::RemoveFormatting(const String &subString)
{
	size_t findPos1, findPos2;
	String tempString;
	String newSubString = String(subString);

	findPos1 = newSubString.find("{");
	findPos2 = newSubString.find("}");

	while ((findPos1 != String::npos) && (findPos2 != String::npos) && (findPos2 > findPos1))
	{
		tempString   = newSubString.substr(0, findPos1);
		newSubString = tempString.append(newSubString.substr(findPos2 + 1));
		findPos1     = newSubString.find("{");
		findPos2     = newSubString.find("}");
	}

	return newSubString;
}

void MediaPlayer::VM_SubFontEngine::RemoveSubs()
{
	VM_SubFontEngine::removeSubs(VM_SubFontEngine::subsBottom);
	VM_SubFontEngine::removeSubs(VM_SubFontEngine::subsMiddle);
	VM_SubFontEngine::removeSubs(VM_SubFontEngine::subsPosition);
	VM_SubFontEngine::removeSubs(VM_SubFontEngine::subsTop);
}

void MediaPlayer::VM_SubFontEngine::removeSubs(VM_SubTexturesId &subs)
{
	for (auto &subsId : subs) {
		for (auto &sub : subsId.second)
			DELETE_POINTER(sub);
	}

	subs.clear();
}

void MediaPlayer::VM_SubFontEngine::RemoveSubs(size_t id)
{
	VM_SubFontEngine::removeSubs(VM_SubFontEngine::subsBottom,   id);
	VM_SubFontEngine::removeSubs(VM_SubFontEngine::subsMiddle,   id);
	VM_SubFontEngine::removeSubs(VM_SubFontEngine::subsPosition, id);
	VM_SubFontEngine::removeSubs(VM_SubFontEngine::subsTop,      id);
}

void MediaPlayer::VM_SubFontEngine::removeSubs(VM_SubTexturesId &subs, size_t id)
{
	if (subs.find(id) != subs.end())
	{
		for (auto &subTexture : subs[id])
			DELETE_POINTER(subTexture);

		subs.erase(id);
	}
}

void MediaPlayer::VM_SubFontEngine::RemoveSubsBottom()
{
	for (auto &subsId : VM_SubFontEngine::subsBottom) {
		for (auto &sub : subsId.second)
			DELETE_POINTER(sub);
	}

	VM_SubFontEngine::subsBottom.clear();
}

int MediaPlayer::VM_SubFontEngine::renderSub(VM_SubTexture* subTexture)
{
	if (subTexture == NULL)
		return ERROR_INVALID_ARGUMENTS;

	SDL_Rect* clip = NULL;

	if (subTexture->subtitle->customClip)
	{
		clip = &subTexture->subtitle->clip;

		subTexture->locationRender.x += subTexture->subtitle->clip.x;
		subTexture->locationRender.y += subTexture->subtitle->clip.y;
		subTexture->locationRender.w  = subTexture->subtitle->clip.w;
		subTexture->locationRender.h  = subTexture->subtitle->clip.h;
	}

	SDL_RenderCopyEx(
		VM_Window::Renderer, subTexture->textureData->data, clip, &subTexture->locationRender,
		subTexture->subtitle->rotation, &subTexture->subtitle->rotationPoint, SDL_FLIP_NONE
	);

	return RESULT_OK;
}

int MediaPlayer::VM_SubFontEngine::renderSubBorderShadow(VM_SubTexture* subTexture)
{
	if (subTexture == NULL)
		return ERROR_INVALID_ARGUMENTS;

	// SHADOW DROP
	SDL_Point shadow = subTexture->subtitle->getShadow();

	if (((shadow.x != 0) || (shadow.y != 0)) && (subTexture->subtitle->getColorShadow().a > 0))
	{
		if (subTexture->shadow == NULL)
			subTexture->shadow = VM_SubFontEngine::createSubShadow(subTexture);

		VM_SubFontEngine::renderSub(subTexture->shadow);
	}

	// BORDER OUTLINE
	if ((subTexture->subtitle->getOutline() > 0) && (subTexture->subtitle->getColorOutline().a > 0))
	{
		if (subTexture->outline == NULL)
			subTexture->outline = VM_SubFontEngine::createSubOutline(subTexture);

		VM_SubFontEngine::renderSub(subTexture->outline);
	}

	return RESULT_OK;
}

void MediaPlayer::VM_SubFontEngine::renderSubs(VM_SubTexturesId &subs)
{
	// LAYERED SUBS
	std::list<VM_SubTexture*> layeredSubs;
	std::vector<size_t>       layeredIds;

	// FIND LAYERED SUBS
	for (const auto &subTextures : subs)
	{
		for (auto subTexture : subTextures.second) {
			if (subTexture->subtitle->layer >= 0) {
				layeredIds.push_back(subTextures.first);
				layeredSubs.push_back(subTexture);
			}
		}
	}

	// REMOVE LAYERED SUBS FROM NON-LAYERED LIST
	for (auto id : layeredIds) {
		subs[id].clear();
		subs.erase(id);
	}

	// SORT LAYERED SUBS BY RENDER ORDER
	layeredSubs.sort([](VM_SubTexture* t1, VM_SubTexture* t2) {
		return t1->subtitle->layer < t2->subtitle->layer;
	});

	// RENDER NON-LAYERED SUBS
	for (const auto & subTextures : subs)
	{
		// BORDER / SHADOW
		for (auto subTexture : subTextures.second)
			VM_SubFontEngine::renderSubBorderShadow(subTexture);

		// FILL
		for (auto subTexture : subTextures.second) {
			if (subTexture->subtitle->getColor().a > 0)
				VM_SubFontEngine::renderSub(subTexture);
		}
	}
	
	// RENDER LAYERED BORDER / SHADOW SUBS
	for (auto subTexture : layeredSubs)
		VM_SubFontEngine::renderSubBorderShadow(subTexture);

	// RENDER LAYERED FILL SUBS
	for (auto subTexture : layeredSubs)
	{
		if (subTexture->subtitle->getColor().a > 0)
			VM_SubFontEngine::renderSub(subTexture);

		subs[subTexture->subtitle->id].push_back(subTexture);
	}
}

void MediaPlayer::VM_SubFontEngine::renderSubsPositionAbsolute(VM_SubTexturesId &subs)
{
	VM_SubFontEngine::setSubPositionAbsolute(subs);
	VM_SubFontEngine::renderSubs(subs);
}

void MediaPlayer::VM_SubFontEngine::renderSubsPositionRelative(VM_SubTexturesId &subs)
{
	VM_SubFontEngine::setSubPositionRelativeTop(subs);
	VM_SubFontEngine::renderSubs(subs);
}

int MediaPlayer::VM_SubFontEngine::RenderSubText(const VM_Subtitles &subs, TTF_Font* fontMerged, TTF_Font* fontCJK)
{
	if (subs.empty() || (fontMerged == NULL) || (fontCJK == NULL))
		return ERROR_UNKNOWN;

	#if defined _DEBUG
		uint32_t start = SDL_GetTicks();
	#endif

	for (auto sub : subs)
	{
		if ((VM_SubFontEngine::subsPosition.find(sub->id) != VM_SubFontEngine::subsPosition.end()) ||
			(VM_SubFontEngine::subsTop.find(sub->id)      != VM_SubFontEngine::subsTop.end()) ||
			(VM_SubFontEngine::subsMiddle.find(sub->id)   != VM_SubFontEngine::subsMiddle.end()) ||
			(VM_SubFontEngine::subsBottom.find(sub->id)   != VM_SubFontEngine::subsBottom.end()))
		{
			sub->skip = true;
		}
	}

	VM_Subtitle* prevSub = NULL;

	for (auto sub : subs)
	{
		if (sub->skip)
			continue;

		// Custom draw operation (fill rect)
		if (!SDL_RectEmpty(&sub->drawRect)) {
			VM_Graphics::FillArea(&VM_SubFontEngine::drawColor, &sub->drawRect);
			continue;
		}

		std::vector<uint16_t*> subStrings16;

		// Set default font for subs without a custom style
		if (sub->style == NULL)
		{
			// Try using the default font (Merged - All languages except CJK)
			sub->font = fontMerged;

			// Use the CJK font if the text contains characters not supported by the default merged font
			if (!VM_Text::FontSupportsLanguage(sub->font, sub->textUTF16, DEFAULT_CHAR_BUFFER_SIZE))
				sub->font = fontCJK;
		}

		// Split the sub if larger than video width
		if (sub->style != NULL)
			VM_SubFontEngine::splitSub(sub->textUTF16, sub, prevSub, subStrings16);
		else
			subStrings16.push_back(sub->textUTF16);

		// Create a new sub surface/texture for each sub line (if the sub was split)
		for (size_t i = 0; i < subStrings16.size(); i++)
		{
			VM_SubTexture* subFill = VM_SubFontEngine::createSubFill(subStrings16[i], sub);

			if (subFill == NULL)
				continue;

			// Organize subs by alignment for positioning
			if (subFill->subtitle->customPos)
				VM_SubFontEngine::subsPosition[subFill->subtitle->id].push_back(subFill);
			else if (subFill->subtitle->isAlignedTop())
				VM_SubFontEngine::subsTop[subFill->subtitle->id].push_back(subFill);
			else if (subFill->subtitle->isAlignedMiddle())
				VM_SubFontEngine::subsMiddle[subFill->subtitle->id].push_back(subFill);
			else
				VM_SubFontEngine::subsBottom[subFill->subtitle->id].push_back(subFill);
		}

		prevSub = sub;
	}

	// Calculate and set the total width for split subs
	VM_SubFontEngine::setTotalWidthAbsolute(VM_SubFontEngine::subsPosition);
	VM_SubFontEngine::setTotalWidthRelative(VM_SubFontEngine::subsTop);
	VM_SubFontEngine::setTotalWidthRelative(VM_SubFontEngine::subsMiddle);
	VM_SubFontEngine::setTotalWidthRelative(VM_SubFontEngine::subsBottom);

	// Render subs
	VM_SubFontEngine::renderSubsPositionAbsolute(VM_SubFontEngine::subsPosition);
	VM_SubFontEngine::renderSubsPositionRelative(VM_SubFontEngine::subsTop);
	VM_SubFontEngine::renderSubsPositionRelative(VM_SubFontEngine::subsMiddle);
	VM_SubFontEngine::renderSubsPositionRelative(VM_SubFontEngine::subsBottom);

	#if defined _DEBUG
		auto time = (SDL_GetTicks() - start);
		if (time > 0)
			LOG(String("VM_SubFontEngine::RenderSubText: TIME: " + std::to_string(time) + " ms").c_str());
	#endif

	return RESULT_OK;
}

void MediaPlayer::VM_SubFontEngine::setSubPositionAbsolute(const VM_SubTexturesId &subs)
{
	for (const auto &subTextures : subs)
	{
		VM_SubTextures subsX;
		int            offsetX     = 0;
		VM_SubTexture* prevSub     = NULL;
		SDL_FPoint     subScale    = VM_Player::GetSubScale();
		int            totalHeight = 0;

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			int rotationPointX = (int)((float)subTexture->subtitle->rotationPoint.x * subScale.x);

			SDL_Point position = {
				(int)((float)subTexture->subtitle->position.x * subScale.x),
				(int)((float)subTexture->subtitle->position.y * subScale.y),
			};

			if (offsetX == 0)
				offsetX = position.x;

			// PRIMARY (FILL) SUB
			subTexture->locationRender = {
				position.x, position.y,
				subTexture->textureData->width, subTexture->textureData->height
			};

			// Offset based on previous sub
			if (prevSub != NULL)
			{
				subTexture->locationRender.x = prevSub->locationRender.x;
				subTexture->locationRender.y = prevSub->locationRender.y;

				if (subTexture->subtitle->offsetX)
					subTexture->locationRender.x = (prevSub->locationRender.x + prevSub->locationRender.w);

				if (subTexture->subtitle->offsetY) {
					subTexture->locationRender.y = (prevSub->locationRender.y + prevSub->locationRender.h);
					subTexture->locationRender.x = offsetX;
				}
			}

			// FORCED BLANK NEW LINES (t1\N\N\Nt2)
			if (subTexture->subtitle->text == " ")
			{
				if (subTexture->subtitle->style != NULL) {
					subTexture->subtitle->style->blur    = 0;
					subTexture->subtitle->style->outline = 0;
					subTexture->subtitle->style->shadow  = {};
				}

				if (subTexture->subtitle->text2.find("\\fs") == String::npos)
					subTexture->locationRender.h /= 2;
			}

			// ROTATION
			if (subTexture->subtitle->customRotation)
				subTexture->subtitle->rotationPoint.x = (rotationPointX - position.x);
			else
				subTexture->subtitle->rotationPoint.x = (subTexture->total.w / 2);

			subTexture->subtitle->rotationPoint.x -= (subTexture->locationRender.x - position.x);

			// CLIP
			if (subTexture->subtitle->customClip)
			{
				int offsetX = 0;
				int offsetY = 0;

				if (subTexture->subtitle->isAlignedRight())
					offsetX = subTexture->locationRender.w;
				else if (subTexture->subtitle->isAlignedCenter())
					offsetX = (subTexture->locationRender.w / 2);

				if (subTexture->subtitle->isAlignedBottom())
					offsetY = subTexture->locationRender.h;
				else if (subTexture->subtitle->isAlignedMiddle())
					offsetY = (subTexture->locationRender.h / 2);

				subTexture->subtitle->clip.x = max((subTexture->subtitle->clip.x - (position.x - offsetX)), 0);
				subTexture->subtitle->clip.y = max((subTexture->subtitle->clip.y - (position.y - offsetY)), 0);
				subTexture->subtitle->clip.w = min(subTexture->subtitle->clip.w, subTexture->locationRender.w);
				subTexture->subtitle->clip.h = min(subTexture->subtitle->clip.h, subTexture->locationRender.h);
			}

			// CUSTOM POSITIONING BASED ON MARGINS AND ALIGNMENT
			if (subTexture->subtitle->customMargins)
			{
				auto margins = subTexture->subtitle->getMargins();

				if (subTexture->subtitle->isAlignedLeft())
					subTexture->locationRender.x = margins.x;
				else if (subTexture->subtitle->isAlignedRight())
					subTexture->locationRender.x = (VM_Player::VideoDimensions.w - margins.y);
				else
					subTexture->locationRender.x = ((VM_Player::VideoDimensions.w + margins.x - margins.y) / 2);

				if (subTexture->subtitle->offsetX)
					subTexture->locationRender.x += (position.x - subTexture->locationRender.x);

				if (subTexture->subtitle->isAlignedTop())
					subTexture->locationRender.y = margins.h;
				else if (subTexture->subtitle->isAlignedBottom())
					subTexture->locationRender.y = (VM_Player::VideoDimensions.h - margins.h);
				else
					subTexture->locationRender.y = ((VM_Player::VideoDimensions.h - margins.h) / 2);

				if (subTexture->subtitle->offsetY && (prevSub != NULL))
					subTexture->locationRender.y = (prevSub->locationRender.y + prevSub->locationRender.h);
			}

			// HORIZONTAL ALIGN

			// Add split subs belonging to the same sub line
			if (subTexture->subtitle->splitStyling || (subTexture->subtitle->splitStyling && !subTexture->subtitle->offsetX && !subTexture->subtitle->offsetY))
			{
				subsX.push_back(subTexture);
			}
			// Offset X by total width including split subs
			else
			{
				subsX.push_back(subTexture);

				for (size_t i = 0; i < subsX.size(); i++)
				{
					if (subTexture->subtitle->isAlignedRight())
						subsX[i]->locationRender.x -= subTexture->total.w;
					else if (subTexture->subtitle->isAlignedCenter())
						subsX[i]->locationRender.x -= (subTexture->total.w / 2);

					subsX[i]->total.x = subsX[0]->locationRender.x;
					subsX[i]->total.w = subTexture->total.w;
					subsX[i]->max.w   = subsX[i]->total.w;
				}

				totalHeight += subTexture->locationRender.h;

				subsX.clear();
			}

			prevSub = subTexture;
		}

		int minX = VM_Player::VideoDimensions.w;

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			int positionY      = (int)((float)subTexture->subtitle->position.y      * subScale.y);
			int rotationPointY = (int)((float)subTexture->subtitle->rotationPoint.y * subScale.y);

			subTexture->total.h = totalHeight;
			subTexture->max.h   = subTexture->total.h;

			if (subTexture->total.x < minX)
				minX = subTexture->total.x;

			// VERTICAL ALIGN
			if (subTexture->subtitle->isAlignedBottom())
				subTexture->locationRender.y -= subTexture->total.h;
			else if (subTexture->subtitle->isAlignedMiddle())
				subTexture->locationRender.y -= (subTexture->total.h / 2);

			if (subTexture->subtitle->customRotation)
				subTexture->subtitle->rotationPoint.y = (rotationPointY - positionY);
			else
				subTexture->subtitle->rotationPoint.y = (subTexture->total.h / 2);

			subTexture->subtitle->rotationPoint.y -= (subTexture->locationRender.y - positionY);
		}

		for (auto subTexture : subTextures.second)
		{
			if (!subTexture->subtitle->skip) {
				subTexture->total.y = subTextures.second[0]->locationRender.y;
				subTexture->max.y   = subTexture->total.y;
				subTexture->max.x   = minX;
			}
		}
	}
}

void MediaPlayer::VM_SubFontEngine::setSubPositionRelativeTop(const VM_SubTexturesId &subs)
{
	VM_SubTexture* previousSub = NULL;

	for (const auto &subTextures : subs)
	{
		VM_SubTextures subLine;
		int maxHeight   = 0;
		int offsetX     = 0;
		int totalHeight = 0;

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			subTexture->locationRender.x = offsetX;
			subTexture->locationRender.y = totalHeight;
			subTexture->locationRender.w = subTexture->textureData->width;
			subTexture->locationRender.h = subTexture->textureData->height;

			maxHeight = (subTexture->locationRender.h > maxHeight ? subTexture->locationRender.h : maxHeight);

			// ROTATION
			subTexture->subtitle->rotationPoint.x = ((subTexture->total.w / 2) - offsetX);

			subLine.push_back(subTexture);

			if (subTexture->subtitle->splitStyling) {
				offsetX += subTexture->locationRender.w;
				continue;
			}

			auto margins = subTexture->subtitle->getMargins();

			// OFFSET SUBS HORIZONTALLY (X) BASED ON ALIGNMENT
			for (auto subWord : subLine)
			{
				if (subTexture->subtitle->isAlignedLeft())
					subWord->locationRender.x += margins.x;
				else if (subTexture->subtitle->isAlignedRight())
					subWord->locationRender.x += (VM_Player::VideoDimensions.w - subTexture->total.w - margins.y);
				else if (subTexture->subtitle->isAlignedCenter())
					subWord->locationRender.x += ALIGN_CENTER(0, VM_Player::VideoDimensions.w, subTexture->total.w);

				subWord->total.x = subLine[0]->locationRender.x;
				subWord->total.w = subTexture->total.w;
				subWord->max.w   = subWord->total.w;
			}

			totalHeight += maxHeight;
			offsetX      = 0;
			maxHeight    = 0;

			subLine.clear();
		}

		int minX = VM_Player::VideoDimensions.w;

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			subTexture->total.h = totalHeight;
			subTexture->max.h   = subTexture->total.h;

			if (subTexture->total.x < minX)
				minX = subTexture->total.x;

			// OFFSET SUBS VERTICALLY (Y) BASED ON ALIGNMENT
			auto margins = subTexture->subtitle->getMargins();
			int  offsetY = 0;

			if (subTexture->subtitle->isAlignedTop())
				offsetY = margins.h;
			else if (subTexture->subtitle->isAlignedMiddle())
				offsetY = ((VM_Player::VideoDimensions.h - subTexture->total.h) / 2);
			else
				offsetY = (VM_Player::VideoDimensions.h - subTexture->total.h - margins.h);

			if (offsetY > 0)
				subTexture->locationRender.y += offsetY;
		}

		for (auto subTexture : subTextures.second)
		{
			if (!subTexture->subtitle->skip) {
				subTexture->total.y = subTextures.second[0]->locationRender.y;
				subTexture->max.y   = subTexture->total.y;
				subTexture->max.x   = minX;

				subTexture->subtitle->rotationPoint.y  = (subTexture->total.h / 2);
				subTexture->subtitle->rotationPoint.y -= (subTexture->locationRender.y - subTexture->total.y);
			}
		}

		// CHECK COLLISION WITH PREVIOUS SUBS
		if (previousSub != NULL)
		{
			SDL_Rect coll = {};

			for (const auto &sub : subs)
			{
				if ((subTextures.first == sub.first) ||
					!SDL_IntersectRect(&subTextures.second[0]->max, &sub.second[0]->max, &coll))
				{
					continue;
				}

				int offsetY = 0;

				for (auto subTexture : subTextures.second)
				{
					if (subTexture->subtitle->layer >= 0)
						continue;

					if (subTexture->subtitle->isAlignedTop())
						subTexture->locationRender.y = (previousSub->total.y + previousSub->total.h + offsetY);
					else
						subTexture->locationRender.y = (previousSub->total.y - subTexture->total.h + offsetY);

					subTexture->total.y = subTextures.second[0]->locationRender.y;
					offsetY            += subTexture->locationRender.h;
				}

				break;
			}
		}

		if (!subTextures.second.empty())
			previousSub = subTextures.second[0];
	}
}

void MediaPlayer::VM_SubFontEngine::setTotalWidthAbsolute(const VM_SubTexturesId &subs)
{
	for (const auto &subTextures : subs)
	{
		VM_SubTextures subsX;
		int            maxWidth = 0;

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			if (subTexture->subtitle->splitStyling || (subTexture->subtitle->splitStyling && !subTexture->subtitle->offsetX && !subTexture->subtitle->offsetY))
			{
				subsX.push_back(subTexture);
			}
			else
			{
				int totalWidth = subTexture->textureData->width;

				for (auto subX : subsX)
					totalWidth += subX->textureData->width;

				subTexture->total.w = totalWidth;

				for (auto subX : subsX)
					subX->total.w = totalWidth;

				if (totalWidth > maxWidth)
					maxWidth = totalWidth;

				subsX.clear();
			}
		}

		for (auto subTexture : subTextures.second) {
			if (!subTexture->subtitle->skip)
				subTexture->max.w = maxWidth;
		}
	}
}

void MediaPlayer::VM_SubFontEngine::setTotalWidthRelative(const VM_SubTexturesId &subs)
{
	for (const auto &subTextures : subs)
	{
		VM_SubTextures subLine;
		int            maxWidth   = 0;
		int            totalWidth = 0;

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			totalWidth += subTexture->textureData->width;

			subLine.push_back(subTexture);

			if (subTexture->subtitle->splitStyling)
				continue;

			for (size_t i = 0; i < subLine.size(); i++)
				subLine[i]->total.w = totalWidth;

			if (totalWidth > maxWidth)
				maxWidth = totalWidth;

			totalWidth = 0;
			subLine.clear();
		}

		for (auto subTexture : subTextures.second) {
			if (!subTexture->subtitle->skip)
				subTexture->max.w = maxWidth;
		}
	}
}

int MediaPlayer::VM_SubFontEngine::splitSub(uint16_t* subStringUTF16, VM_Subtitle* sub, VM_Subtitle* prevSub, std::vector<uint16_t*> &subStrings16)
{
	if ((subStringUTF16 == NULL) || (sub == NULL))
		return ERROR_UNKNOWN;

	// Calculate total sub width
	int  width, height;
	auto margins  = sub->getMargins();
	int  maxWidth = (VM_Player::VideoDimensions.w - DEFAULT_MARGIN - DEFAULT_MARGIN - margins.x - margins.y);

	TTF_Font* font = sub->getFont();

	if (font == NULL)
		return ERROR_UNKNOWN;

	// SPLIT STYLING
	if ((sub->text3.find("{") != sub->text3.rfind("{")) && VM_Text::IsValidSubtitle(sub->text3) &&
		(sub->text3.find("^") == String::npos))
	{
		uint16_t utf16[DEFAULT_CHAR_BUFFER_SIZE];
		String   text3 = VM_SubFontEngine::RemoveFormatting(sub->text3);

		VM_Text::ToUTF16(text3, utf16, DEFAULT_CHAR_BUFFER_SIZE);
		TTF_SizeUNICODE(font, utf16, &width, &height);

		if ((width > maxWidth) && (sub->text3.find("\\q2") == String::npos))
		{
			int     width2 = 0;
			Strings split  = VM_Text::Split(sub->text3, "{");

			for (const auto &s : split)
			{
				size_t f  = s.rfind("}");
				String s2 = (f != String::npos ? s.substr(f + 1) : s);

				VM_Text::ToUTF16(s2, utf16, DEFAULT_CHAR_BUFFER_SIZE);
				TTF_SizeUNICODE(font, utf16, &width, &height);

				width2 += width;

				if (s2 == sub->text)
					break;

				if (width2 > maxWidth)
					width2 = width;
			}

			if (width2 > maxWidth)
			{
				sub->offsetY = true; // Absolute position: force new line

				if (prevSub != NULL)
					prevSub->splitStyling = false; // Relative align: force new line
			}
		}

		subStrings16.push_back(subStringUTF16);

		return RESULT_OK;
	}

	// NORMAL SPLIT
	TTF_SizeUNICODE(font, subStringUTF16, &width, &height);

	// Split sub if larger than video width
	if ((width >= maxWidth) && (sub->text3.find("\\q2") == String::npos))
	{
		Strings words   = VM_Text::Split(VM_Text::Trim(sub->text), " ");
		size_t  nrLines = splitSubGetNrLines(words, font, maxWidth);

		if (splitSubDistributeByLines(words, nrLines, font, maxWidth, subStrings16) != RESULT_OK)
			splitSubDistributeByWidth(words, font, maxWidth, subStrings16);

		sub->splitStyling = false; // Relative align:    force new line
		sub->offsetY      = true;  // Absolute position: force new line
	} else {
		subStrings16.push_back(subStringUTF16);
	}

	return RESULT_OK;
}

size_t MediaPlayer::VM_SubFontEngine::splitSubGetNrLines(const Strings &words, TTF_Font* font, const int maxWidth)
{
	uint16_t lineString16[DEFAULT_CHAR_BUFFER_SIZE];
	int      lineWidth, lineHeight;
	size_t   nrLines     = 0;
	String   lineString1 = "";
	String   lineString2 = "";

	for (size_t i = 0; i < words.size(); i++)
	{
		lineString2.append(words[i]);

		VM_Text::ToUTF16(lineString2, lineString16, DEFAULT_CHAR_BUFFER_SIZE);
		TTF_SizeUNICODE(font, lineString16, &lineWidth, &lineHeight);

		if (i == words.size() - 1)
			nrLines++;

		if (lineWidth > maxWidth) {
			nrLines++;

			lineString1 = "";
			lineString2 = (words[i] + " ");
		}

		lineString1.append(words[i] + " ");
		lineString2.append(" ");
	}

	return nrLines;
}

int MediaPlayer::VM_SubFontEngine::splitSubDistributeByLines(const Strings &words, size_t nrLines, TTF_Font* font, const int maxWidth, std::vector<uint16_t*> &subStrings16)
{
	int    lineWidth, lineHeight;
	String line         = "";
	int    wordsPerLine = (int)ceilf((float)words.size() / (float)nrLines);

	for (size_t i = 0; i < words.size(); i++)
	{
		line.append(words[i]);

		if (((i + 1) % wordsPerLine == 0) || (i == words.size() - 1)) {
			uint16_t* line16 = new uint16_t[DEFAULT_CHAR_BUFFER_SIZE];
			VM_Text::ToUTF16(line, line16, DEFAULT_CHAR_BUFFER_SIZE);

			TTF_SizeUNICODE(font, line16, &lineWidth, &lineHeight);
			
			if (lineWidth > maxWidth) {
				for (auto sub16 : subStrings16)
					DELETE_POINTER(sub16);

				subStrings16.clear();

				return ERROR_UNKNOWN;
			}

			subStrings16.push_back(line16);

			line = "";

			continue;
		}

		line.append(" ");
	}

	return RESULT_OK;
}

void MediaPlayer::VM_SubFontEngine::splitSubDistributeByWidth(const Strings &words, TTF_Font* font, const int maxWidth, std::vector<uint16_t*> &subStrings16)
{
	uint16_t lineString16[DEFAULT_CHAR_BUFFER_SIZE];
	int      lineWidth, lineHeight;
	String   lineString1 = "";
	String   lineString2 = (!words.empty() ? words[0] : "");

	for (size_t i = 0; i < words.size(); i++)
	{
		VM_Text::ToUTF16(lineString2, lineString16, DEFAULT_CHAR_BUFFER_SIZE);
		TTF_SizeUNICODE(font, lineString16, &lineWidth, &lineHeight);

		if (lineWidth > maxWidth) {
			uint16_t* line16 = new uint16_t[DEFAULT_CHAR_BUFFER_SIZE];
			VM_Text::ToUTF16(lineString1, line16, DEFAULT_CHAR_BUFFER_SIZE);

			subStrings16.push_back(line16);

			lineString1 = "";
			lineString2 = words[i];
		}

		if (i == words.size() - 1) {
			uint16_t* endLine = new uint16_t[DEFAULT_CHAR_BUFFER_SIZE];
			VM_Text::ToUTF16(lineString2, endLine, DEFAULT_CHAR_BUFFER_SIZE);

			subStrings16.push_back(endLine);
		}

		lineString1.append(words[i] + " ");

		if (i < words.size() - 1)
			lineString2.append(" " + words[i + 1]);
	}
}

// http://docs.aegisub.org/3.2/ASS_Tags/
// https://en.wikipedia.org/wiki/SubStation_Alpha
MediaPlayer::VM_Subtitles MediaPlayer::VM_SubFontEngine::SplitAndFormatSub(
	const Strings &subTexts, const VM_SubStyles &subStyles, const VM_Subtitles &playerSubs
)
{
	VM_Subtitles  subs;
	static size_t id = 0;

	if (subTexts.empty())
		return subs;

	// Split and format sub strings
	for (auto subText : subTexts)
	{
		if (subText.size() > DEFAULT_CHAR_BUFFER_SIZE)
			continue;

		#ifdef _DEBUG
			LOG("VM_SubFontEngine::SplitAndFormatSub:\n%s", subText.c_str());
			Uint32 start = SDL_GetTicks();
		#endif

		VM_Subtitle* prevSub = NULL;
		
		// Split and format the dialogue properties
		// Dialogue: Marked/Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
		Strings dialogueSplit = VM_Text::Split(subText, ",");
		subText               = VM_SubFontEngine::formatDialogue(subText, dialogueSplit);
		String subText2       = VM_SubFontEngine::RemoveFormatting(subText);

		// Split the sub string by partial formatting ({\f1}t1{\f2}t2)
		Strings subLines;
		bool    splitStyling = VM_SubFontEngine::formatSplitStyling(subText, subLines);

		// Add lines to the list of sub strings
		for (auto &subLine : subLines)
		{
			// Custom draw operation (fill rect)
			VM_SubFontEngine::formatDrawCommand(subLine, dialogueSplit, id, subStyles, subs);

			// Skip unsupported draw operations
			if (!VM_Text::IsValidSubtitle(subLine))
				continue;

			VM_Subtitle* sub = new VM_Subtitle();

			sub->id = id;

			// Sub line was split for partial styling of letters/words
			if (!subLine.empty() && (subLine[subLine.size() - 1] == '^')) {
				subLine           = subLine.substr(0, subLine.size() - 1);
				sub->splitStyling = false;
			} else {
				sub->splitStyling = splitStyling;
			}

			if (subLine.empty())
				subLine = " ";

			// Dialogue: Marked/Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
			sub->text2     = String(subLine);
			sub->text3     = subText;
			sub->textSplit = dialogueSplit;

			// Set sub style
			sub->style = VM_SubFontEngine::getSubStyle(subStyles, sub->textSplit);

			// Set sub rotation from style
			if (sub->style != NULL)
				sub->rotation = sub->style->rotation;

			// Format the sub string by removing formatting tags
			sub->text = VM_SubFontEngine::RemoveFormatting(subLine);

			// Skip empty lines
			if (sub->text.empty()) {
				DELETE_POINTER(sub);
				continue;
			}

			// UTF-16
			sub->textUTF16 = new uint16_t[DEFAULT_CHAR_BUFFER_SIZE];

			if (VM_Text::ToUTF16(sub->text, sub->textUTF16, DEFAULT_CHAR_BUFFER_SIZE) <= 0) {
				DELETE_POINTER(sub);
				continue;
			}

			// No style available
			if (sub->style == NULL) {
				subs.push_back(sub);
				continue;
			}

			// Set rendering layer
			VM_SubFontEngine::formatSetLayer(sub, playerSubs);

			// Multiple-line sub - Inherit from previous sub
			if (prevSub != NULL)
				sub->copy(*prevSub);

			// REMOVE ANIMATIONS
			Strings animations = VM_SubFontEngine::formatGetAnimations(sub->text3);
			sub->text2         = VM_SubFontEngine::formatRemoveAnimations(sub->text2);

			// STYLE OVERRIDERS - Category 1 - Affects the entire sub
			VM_SubFontEngine::formatOverrideStyleCat1(animations, sub);

			// STYLE OVERRIDERS - Category 2 - Affects only preceding text
			VM_SubFontEngine::formatOverrideStyleCat2(animations, sub, subStyles);

			// STYLE OVERRIDER - MARGINS
			VM_SubFontEngine::formatOverrideMargins(sub);

			// Multiple lines with custom positioning
			if ((prevSub != NULL) && prevSub->customPos)
			{
				if (subText.find("{") != subText.rfind("{")) {
					if (!prevSub->splitStyling)
						sub->offsetY = true;
					else
						sub->offsetX = true;
				} else if (subText.find("^") != String::npos) {
					sub->offsetY = true;
				}
			}

			subs.push_back(sub);

			prevSub = sub;
		}

		id++;

		#if defined _DEBUG
			auto time = (SDL_GetTicks() - start);
			if (time > 0)
				LOG(String("VM_SubFontEngine::SplitAndFormatSub: TIME: " + std::to_string(time) + " ms").c_str());
		#endif
	}

	return subs;
}
