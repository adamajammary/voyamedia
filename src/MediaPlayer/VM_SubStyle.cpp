#include "VM_SubStyle.h"

using namespace VoyaMedia::Graphics;
using namespace VoyaMedia::System;

MediaPlayer::VM_SubStyle::VM_SubStyle()
{
	this->alignment      = SUB_ALIGN_BOTTOM_CENTER;
	this->blur           = 0;
	this->colorPrimary   = { 0xFF, 0xFF, 0xFF, 0xFF };
	this->colorOutline   = {};
	this->colorShadow    = {};
	this->font           = NULL;
	this->fontScale      = {};
	this->fontSize       = 0;
	this->fontSizeScaled = 0;
	this->fontStyle      = 0;
	this->marginL        = 0;
	this->marginR        = 0;
	this->marginV        = 0;
	this->name           = "";
	this->outline        = 0;
	this->rotation       = 0;
	this->shadow         = {};

	#if defined _windows
		this->fontName = L"";
	#else
		this->fontName = "";
	#endif
}

MediaPlayer::VM_SubStyle::VM_SubStyle(Strings data, VM_SubStyleVersion version)
{
	this->alignment      = SUB_ALIGN_BOTTOM_CENTER;
	this->blur           = 0;
	this->font           = NULL;
	this->fontSizeScaled = 0;
	this->fontStyle      = 0;
	this->marginL        = 0;
	this->marginR        = 0;
	this->marginV        = 0;
	this->outline        = 0;
	this->rotation       = 0;
	this->shadow         = {};

	// STYLE NAME
	this->name = data[SUB_STYLE_V4_NAME];

	// FONT NAME
	#if defined _windows
		this->fontName = VM_Text::ToUTF16(data[SUB_STYLE_V4_FONT_NAME].c_str());
	#else
		this->fontName = data[SUB_STYLE_V4_FONT_NAME];
	#endif

	// FONT SIZE
	//this->fontSize = (int)(std::atof(data[SUB_STYLE_V4_FONT_SIZE].c_str()) * DEFAULT_FONT_DPI_RATIO);
	this->fontSize = std::atoi(data[SUB_STYLE_V4_FONT_SIZE].c_str());

	#if defined _windows
	if ((this->fontName == FONT_ARIAL) && (this->name == "Default") && (this->fontSize == 16))
	#else
	if ((this->fontName == FONT_ARIAL) && (this->name == "Default") && (this->fontSize == 16))
	#endif
		this->fontSize = 22;

	// FONT COLORS
	this->colorPrimary = VM_Graphics::ToVMColor(data[SUB_STYLE_V4_COLOR_PRIMARY]);
	this->colorOutline = VM_Graphics::ToVMColor(data[SUB_STYLE_V4_COLOR_BORDER]);
	this->colorShadow  = VM_Graphics::ToVMColor(data[SUB_STYLE_V4_COLOR_SHADOW]);

	int shadow;

	switch (version) {
	// Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour,
	// Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow,
	// Alignment, MarginL, MarginR, MarginV, Encoding
	case SUB_STYLE_VERSION_4PLUS_ASS:
		this->alignment = (VM_SubAlignment)std::atoi(data[SUB_STYLE_V4PLUS_FONT_ALIGNMENT].c_str());

		this->fontScale = {
			(float)(std::atof(data[SUB_STYLE_V4PLUS_FONT_SCALE_X].c_str()) * 0.01),
			(float)(std::atof(data[SUB_STYLE_V4PLUS_FONT_SCALE_Y].c_str()) * 0.01)
		};

		if (std::atoi(data[SUB_STYLE_V4_FONT_BOLD].c_str()) == 1)
			this->fontStyle |= TTF_STYLE_BOLD;
		if (std::atoi(data[SUB_STYLE_V4_FONT_ITALIC].c_str()) == 1)
			this->fontStyle |= TTF_STYLE_ITALIC;
		if (std::atoi(data[SUB_STYLE_V4PLUS_FONT_STRIKEOUT].c_str()) == 1)
			this->fontStyle |= TTF_STYLE_STRIKETHROUGH;
		if (std::atoi(data[SUB_STYLE_V4PLUS_FONT_UNDERLINE].c_str()) == 1)
			this->fontStyle |= TTF_STYLE_UNDERLINE;

		this->outline = (int)std::round(std::atof(data[SUB_STYLE_V4PLUS_FONT_OUTLINE].c_str()));

		shadow = (int)std::round(std::atof(data[SUB_STYLE_V4PLUS_FONT_SHADOW].c_str()));
		this->shadow = { shadow, shadow };

		this->marginL = (int)std::round(std::atof(data[SUB_STYLE_V4PLUS_FONT_MARGINL].c_str()));
		this->marginR = (int)std::round(std::atof(data[SUB_STYLE_V4PLUS_FONT_MARGINR].c_str()));
		this->marginV = (int)std::round(std::atof(data[SUB_STYLE_V4PLUS_FONT_MARGINV].c_str()));

		this->rotation  = (std::atof(data[SUB_STYLE_V4PLUS_FONT_ROTATION_ANGLE].c_str()) * -1.0f);


		break;
	// Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour, BackColour,
	// Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV,
	// AlphaLevel, Encoding
	case SUB_STYLE_VERSION_4_SSA:
		this->alignment = VM_SubStyle::ToSubAlignment(std::atoi(data[SUB_STYLE_V4_FONT_ALIGNMENT].c_str()));

		this->fontStyle |= std::atoi(data[SUB_STYLE_V4_FONT_BOLD].c_str());
		this->fontStyle |= std::atoi(data[SUB_STYLE_V4_FONT_ITALIC].c_str());

		this->outline = (int)std::round(std::atof(data[SUB_STYLE_V4_FONT_SHADOW].c_str()));

		shadow = (int)std::round(std::atof(data[SUB_STYLE_V4_FONT_SHADOW].c_str()));
		this->shadow = { shadow, shadow };

		this->marginL = (int)std::round(std::atof(data[SUB_STYLE_V4_FONT_MARGINL].c_str()));
		this->marginR = (int)std::round(std::atof(data[SUB_STYLE_V4_FONT_MARGINR].c_str()));
		this->marginV = (int)std::round(std::atof(data[SUB_STYLE_V4_FONT_MARGINV].c_str()));

		break;
	}
}

void MediaPlayer::VM_SubStyle::copy(const VM_SubStyle &subStyle)
{
	this->alignment      = subStyle.alignment;
	this->blur           = subStyle.blur;
	this->colorOutline   = subStyle.colorOutline;
	this->colorPrimary   = subStyle.colorPrimary;
	this->colorShadow    = subStyle.colorShadow;
	this->font           = subStyle.font;
	this->fontName       = subStyle.fontName;
	this->fontScale      = subStyle.fontScale;
	this->fontSize       = subStyle.fontSize;
	this->fontSizeScaled = subStyle.fontSizeScaled;
	this->fontStyle      = subStyle.fontStyle;
	this->marginL        = subStyle.marginL;
	this->marginR        = subStyle.marginR;
	this->marginV        = subStyle.marginV;
	this->name           = subStyle.name;
	this->outline        = subStyle.outline;
	this->rotation       = subStyle.rotation;
	this->shadow         = subStyle.shadow;
}

MediaPlayer::VM_SubStyle* MediaPlayer::VM_SubStyle::getDefault(const VM_SubStyles &subStyles)
{
	for (auto style : subStyles) {
		if (VM_Text::ToLower(style->name) == VM_Text::ToLower(this->name))
			return style;
	}

	return NULL;
}

bool MediaPlayer::VM_SubStyle::IsAlignedBottom(VM_SubAlignment a)
{
	return ((a == SUB_ALIGN_BOTTOM_LEFT) || (a == SUB_ALIGN_BOTTOM_RIGHT) || (a == SUB_ALIGN_BOTTOM_CENTER));
}

bool MediaPlayer::VM_SubStyle::IsAlignedCenter(VM_SubAlignment a)
{
	return ((a == SUB_ALIGN_BOTTOM_CENTER) || (a == SUB_ALIGN_TOP_CENTER) || (a == SUB_ALIGN_MIDDLE_CENTER));
}

bool MediaPlayer::VM_SubStyle::IsAlignedLeft(VM_SubAlignment a)
{
	return ((a == SUB_ALIGN_BOTTOM_LEFT) || (a == SUB_ALIGN_TOP_LEFT) || (a == SUB_ALIGN_MIDDLE_LEFT));
}

bool MediaPlayer::VM_SubStyle::IsAlignedMiddle(VM_SubAlignment a)
{
	return ((a == SUB_ALIGN_MIDDLE_LEFT) || (a == SUB_ALIGN_MIDDLE_RIGHT) || (a == SUB_ALIGN_MIDDLE_CENTER));
}

bool MediaPlayer::VM_SubStyle::IsAlignedRight(VM_SubAlignment a)
{
	return ((a == SUB_ALIGN_BOTTOM_RIGHT) || (a == SUB_ALIGN_TOP_RIGHT) || (a == SUB_ALIGN_MIDDLE_RIGHT));
}

bool MediaPlayer::VM_SubStyle::IsAlignedTop(VM_SubAlignment a)
{
	return ((a == SUB_ALIGN_TOP_LEFT) || (a == SUB_ALIGN_TOP_RIGHT) || (a == SUB_ALIGN_TOP_CENTER));
}

bool MediaPlayer::VM_SubStyle::isFontValid(TTF_Font* font)
{
	// Get the font family name in ASCII encoding
	auto fn = TTF_FontFaceFamilyName(font);
	auto fs = TTF_FontFaceStyleName(font);

	#if defined _windows
		WString familyName = VM_Text::ToUTF16(fn);
		WString fontStyle  = VM_Text::ToUTF16(fs);
		size_t  findPos    = this->fontName.find(familyName + L" ");
	#else
		String familyName = String(fn).append(" ");
		String fontStyle  = String(fs);
		size_t findPos    = this->fontName.find(familyName + " ");
	#endif

	// Font name with explicit style: "Arial Bold" => "Arial" + "Bold"
	if ((findPos != String::npos) && (this->fontName.substr(findPos + familyName.size() + 1) == fontStyle))
		return true;

	// Font name without explicit style (Regular)
	if ((this->fontName == familyName) || (VM_Text::ToLower(this->fontName) == VM_Text::ToLower(familyName)))
		return true;

	// Try to get the font family names in UNICODE (UTF-16) encoding
	auto nrOfFaces = TTF_FontFaces2(font);

	for (long i = 0; i < nrOfFaces; i++)
	{
		auto ftName = TTF_FontFaceFamilyName2(font, i);

		#if defined _windows
			familyName = VM_Text::ToUTF16(ftName);
		#else
			familyName = String(reinterpret_cast<char*>(ftName.string));
		#endif

		if (familyName.empty() && (ftName.string_len > 0))
		{
			#if defined _windows
				familyName = VM_Text::ToUTF16(ftName.string, ftName.string_len);
			#else
				familyName = VM_Text::ToUTF8(VM_Text::ToUTF16(ftName.string, ftName.string_len));
			#endif
		}

		if (familyName.empty())
			continue;

		#if defined _windows
			findPos = this->fontName.find(familyName + L" ");
		#else
			findPos = this->fontName.find(familyName + " ");
		#endif

		// Font name with explicit style: "Arial Bold" => "Arial" + "Bold"
		if ((findPos != String::npos) && (this->fontName.substr(findPos + familyName.size() + 1) == fontStyle))
			return true;

		// Font name without explicit style (Regular)
		if ((this->fontName == familyName) || (VM_Text::ToLower(this->fontName) == VM_Text::ToLower(familyName)))
			return true;
	}

	return false;
}

TTF_Font* MediaPlayer::VM_SubStyle::getFont()
{
	return this->font;
}

#if defined _windows
void MediaPlayer::VM_SubStyle::openFont(umap<WString, TTF_Font*> &styleFonts, const VM_PointF &subScale, VM_Subtitle* sub)
#else
void MediaPlayer::VM_SubStyle::openFont(umap<String, TTF_Font*> &styleFonts, const VM_PointF &subScale, VM_Subtitle* sub)
#endif
{
	this->fontSizeScaled = (int)((float)this->fontSize * subScale.y);

	#if defined _windows
		WString fontName = (this->fontName + L"_" + std::to_wstring(this->fontSizeScaled));
	#else
		String fontName = (this->fontName + "_" + std::to_string(this->fontSizeScaled));
	#endif

	if (styleFonts[fontName] == NULL)
		styleFonts[fontName] = this->openFont(VM_Player::FormatContext);

	this->font = styleFonts[fontName];

	if ((sub != NULL) && (sub->textUTF16 != NULL) && 
		!VM_Text::FontSupportsLanguage(this->font, sub->textUTF16, DEFAULT_CHAR_BUFFER_SIZE))
	{
		#if defined _windows
			WStrings fonts = { FONT_ARIAL, FONT_NOTO_CJK, FONT_NOTO };
		#else
			Strings fonts = { FONT_ARIAL, FONT_NOTO_CJK, FONT_NOTO };
		#endif

		for (auto font : fonts)
		{
			#if defined _windows
				WString fontName = (font + L"_" + std::to_wstring(this->fontSizeScaled));
			#else
				String fontName = (font + "_" + std::to_string(this->fontSizeScaled));
			#endif

			if (styleFonts[fontName] == NULL)
			{
				#if defined _windows
				if (font == FONT_ARIAL)
					styleFonts[fontName] = OPEN_FONT(VM_FileSystem::GetPathFontArialW().c_str(), this->fontSizeScaled);
				else
					styleFonts[fontName] = OPEN_FONT((VM_FileSystem::GetPathFontW() + font).c_str(), this->fontSizeScaled);
				#else
				if (font == FONT_ARIAL)
					styleFonts[fontName] = OPEN_FONT(VM_FileSystem::GetPathFontArial().c_str(), this->fontSizeScaled);
				else
					styleFonts[fontName] = OPEN_FONT((VM_FileSystem::GetPathFont() + font).c_str(), this->fontSizeScaled);
				#endif
			}

			if (VM_Text::FontSupportsLanguage(styleFonts[fontName], sub->textUTF16, DEFAULT_CHAR_BUFFER_SIZE)) {
				this->font = styleFonts[fontName];
				break;
			}
		}
	}
}

TTF_Font* MediaPlayer::VM_SubStyle::openFont(LIB_FFMPEG::AVFormatContext* formatContext)
{
	if (formatContext == NULL)
		return NULL;

	TTF_Font* font = this->openFontInternal(formatContext);

	if (font == NULL)
		font = this->openFontDisk();

	if (font != NULL)
		TTF_SetFontStyle(font, this->fontStyle);

	return font;
}

TTF_Font* MediaPlayer::VM_SubStyle::openFontDisk()
{
	if (this->fontName.empty())
		return NULL;

	#if defined _android
		String arial    = "Arial.ttf";
		String fontPath = "/system/fonts/";
	#elif defined _ios
		String arial    = "Arial.ttf";
		String fontPath = "/System/Library/Fonts/Cache/";
	#elif defined _linux
		String arial    = "arial.ttf";
		String fontPath = "/usr/share/fonts/truetype/msttcorefonts/";
	#elif defined _macosx
		String arial    = "Arial.ttf";
		String fontPath = "/Library/Fonts/";
	#elif defined _windows
		WString arial     = L"arial.ttf";
		WString fontPath  = L"C:\\Windows\\Fonts\\";
		String  fontPath2 = "C:\\Windows\\Fonts\\";
	#endif

	#if defined _windows
		Strings files = VM_FileSystem::GetDirectoryFiles(fontPath2);
	#else
		Strings files = VM_FileSystem::GetDirectoryFiles(fontPath);
	#endif

	TTF_Font* font = NULL;

	// Try to open the specified custom font
	for (auto file : files)
	{
		#if defined _windows
			font = OPEN_FONT((fontPath + VM_Text::ToUTF16(file.c_str())).c_str(), this->fontSizeScaled);
		#else
			font = OPEN_FONT((fontPath + file.c_str()).c_str(), this->fontSizeScaled);
		#endif

		if ((font != NULL) && this->isFontValid(font))
			break;

		CLOSE_FONT(font);
	}

	// Try to open Arial font by default if custom font was not found
	if (font == NULL)
		font = OPEN_FONT((fontPath + arial).c_str(), this->fontSizeScaled);

	// Arial font alternatives on Linux systems
	#if defined _linux
	if (font == NULL)
		font = OPEN_FONT("/usr/share/fonts/liberation/LiberationSans-Regular.ttf", this->fontSizeScaled);

	if (font == NULL)
		font = OPEN_FONT("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", this->fontSizeScaled);
	#endif

	return font;
}

TTF_Font* MediaPlayer::VM_SubStyle::openFontInternal(LIB_FFMPEG::AVFormatContext* formatContext)
{
	if (formatContext == NULL)
		return NULL;

	TTF_Font* font = NULL;

	// Check if any of the streams has extra font data
	for (uint32_t i = 0; i < formatContext->nb_streams; i++)
	{
		LIB_FFMPEG::AVStream* stream = formatContext->streams[i];

		if (!this->streamHasExtraData(stream))
			continue;

		// Open font data from memory data
		SDL_RWops* fontMemory = SDL_RWFromConstMem(stream->codec->extradata, stream->codec->extradata_size);

		if (fontMemory != NULL)
			font = TTF_OpenFontRW(fontMemory, 0, this->fontSizeScaled);

		if (font == NULL)
			continue;

		// Close the font if the family and/or style doesn't match with the full font name
		if (this->isFontValid(font))
			break;

		CLOSE_FONT(font);
	}

	return font;
}

bool MediaPlayer::VM_SubStyle::streamHasExtraData(LIB_FFMPEG::AVStream* stream)
{
	return ((stream != NULL) && (stream->codec != NULL) && (stream->codec->extradata_size > 0) && ((VM_MediaType)stream->codec->codec_type == MEDIA_TYPE_ATTACHMENT));
}

VM_SubAlignment MediaPlayer::VM_SubStyle::ToSubAlignment(int alignment)
{
	switch (alignment) {
		case 1:  return SUB_ALIGN_BOTTOM_LEFT;
		case 2:  return SUB_ALIGN_BOTTOM_CENTER;
		case 3:  return SUB_ALIGN_BOTTOM_RIGHT;
		case 5:  return SUB_ALIGN_TOP_LEFT;
		case 6:  return SUB_ALIGN_TOP_CENTER;
		case 7:  return SUB_ALIGN_TOP_RIGHT;
		case 9:  return SUB_ALIGN_MIDDLE_LEFT;
		case 10: return SUB_ALIGN_MIDDLE_CENTER;
		case 11: return SUB_ALIGN_MIDDLE_RIGHT;
		default: return SUB_ALIGN_UNKNOWN;
	}
}
