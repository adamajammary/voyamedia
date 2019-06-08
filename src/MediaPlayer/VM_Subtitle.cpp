#include "VM_Subtitle.h"

using namespace VoyaMedia::Graphics;

MediaPlayer::VM_Subtitle::VM_Subtitle()
{
	this->clip           = {};
	this->customClip     = false;
	this->customMargins  = false;
	this->customPos      = false;
	this->customRotation = false;
	this->drawRect       = {};
	this->font           = NULL;
	this->id             = 0;
	this->layer          = -1;
	this->offsetX        = false;
	this->offsetY        = false;
	this->position       = {};
	this->ptsEnd         = 0;
	this->ptsStart       = 0;
	this->rotation       = 0.0;
	this->rotationPoint  = {};
	this->skip           = false;
	this->splitStyling   = false;
	this->style          = NULL;
	this->subRect        = NULL;
	this->text           = "";
	this->text2          = "";
	this->text3          = "";
	this->textUTF16      = NULL;
}

MediaPlayer::VM_Subtitle::~VM_Subtitle()
{
	DELETE_POINTER(this->style);
	DELETE_POINTER(this->subRect);
	DELETE_POINTER_ARR(this->textUTF16);
}

void MediaPlayer::VM_Subtitle::copy(const VM_Subtitle &subtitle)
{
	this->clip           = subtitle.clip;
	this->customClip     = subtitle.customClip;
	this->customMargins  = subtitle.customMargins;
	this->customPos      = subtitle.customPos;
	this->customRotation = subtitle.customRotation;
	this->drawRect       = subtitle.drawRect;
	this->font           = subtitle.font;
	this->position       = subtitle.position;
	this->ptsStart       = subtitle.ptsStart;
	this->ptsEnd         = subtitle.ptsEnd;
	this->rotation       = subtitle.rotation;
	this->rotationPoint  = subtitle.rotationPoint;
	this->subRect        = subtitle.subRect;

	this->style->copy(*subtitle.style);
}

VM_SubAlignment MediaPlayer::VM_Subtitle::getAlignment()
{
	return (this->style != NULL ? this->style->alignment : SUB_ALIGN_BOTTOM_CENTER);
}

int MediaPlayer::VM_Subtitle::getBlur()
{
	return (this->style != NULL ? this->style->blur : 0);
}

VM_Color MediaPlayer::VM_Subtitle::getColor()
{
	return (this->style != NULL ? this->style->colorPrimary : VM_Color(SDL_COLOR_WHITE));
}

VM_Color MediaPlayer::VM_Subtitle::getColorOutline()
{
	return (this->style != NULL ? this->style->colorOutline : VM_Color(SDL_COLOR_BLACK));
}

VM_Color MediaPlayer::VM_Subtitle::getColorShadow()
{
	return (this->style != NULL ? this->style->colorShadow : VM_Color(SDL_COLOR_BLACK));
}

MediaPlayer::VM_Subtitles MediaPlayer::VM_Subtitle::getDuplicateSubs(const VM_Subtitles &subs)
{
	VM_Subtitles duplicates;
	String       text3 = VM_SubFontEngine::RemoveFormatting(this->text3);

	for (auto sub : subs)
	{
		if ((sub == NULL) ||
			(this->id       == sub->id) ||
			(this->ptsStart != sub->ptsStart) ||
			(this->ptsEnd   != sub->ptsEnd) ||
			((this->text    != sub->text) && (text3 != VM_SubFontEngine::RemoveFormatting(sub->text3))) ||
			(std::atoi(this->textSplit[0].c_str()) == std::atoi(sub->textSplit[0].c_str())))
		{
			continue;
		}

		duplicates.push_back(sub);
	}

	return duplicates;
}

TTF_Font* MediaPlayer::VM_Subtitle::getFont()
{
	return (this->style != NULL ? this->style->getFont() : this->font);
}

SDL_Rect MediaPlayer::VM_Subtitle::getMargins()
{
	if (this->style != NULL)
	{
		VM_PointF scale = VM_Player::GetSubScale();

		return {
			(int)((float)this->style->marginL * scale.x),
			(int)((float)this->style->marginR * scale.x),
			(int)((float)this->style->marginV * scale.y),
			(int)((float)this->style->marginV * scale.y),
		};
	}

	return { DEFAULT_MARGIN, DEFAULT_MARGIN, DEFAULT_MARGIN, DEFAULT_MARGIN };

}

int MediaPlayer::VM_Subtitle::getOutline()
{
	if (this->style != NULL)
		return (int)((float)this->style->outline * VM_Player::GetSubScale().y);

	return MIN_OUTLINE;
}

SDL_Point MediaPlayer::VM_Subtitle::getShadow()
{
	if (this->style != NULL)
	{
		VM_PointF scale = VM_Player::GetSubScale();

		return {
			(int)((float)this->style->shadow.x * scale.x),
			(int)((float)this->style->shadow.y * scale.y),
		};
	}

	return {};
}

bool MediaPlayer::VM_Subtitle::isAlignedBottom()
{
	VM_SubAlignment a = this->getAlignment();
	return ((a == SUB_ALIGN_BOTTOM_LEFT) || (a == SUB_ALIGN_BOTTOM_RIGHT) || (a == SUB_ALIGN_BOTTOM_CENTER));
}

bool MediaPlayer::VM_Subtitle::isAlignedCenter()
{
	VM_SubAlignment a = this->getAlignment();
	return ((a == SUB_ALIGN_BOTTOM_CENTER) || (a == SUB_ALIGN_TOP_CENTER) || (a == SUB_ALIGN_MIDDLE_CENTER));
}

bool MediaPlayer::VM_Subtitle::isAlignedLeft()
{
	VM_SubAlignment a = this->getAlignment();
	return ((a == SUB_ALIGN_BOTTOM_LEFT) || (a == SUB_ALIGN_TOP_LEFT) || (a == SUB_ALIGN_MIDDLE_LEFT));
}

bool MediaPlayer::VM_Subtitle::isAlignedMiddle()
{
	VM_SubAlignment a = this->getAlignment();
	return ((a == SUB_ALIGN_MIDDLE_LEFT) || (a == SUB_ALIGN_MIDDLE_RIGHT) || (a == SUB_ALIGN_MIDDLE_CENTER));
}

bool MediaPlayer::VM_Subtitle::isAlignedRight()
{
	VM_SubAlignment a = this->getAlignment();
	return ((a == SUB_ALIGN_BOTTOM_RIGHT) || (a == SUB_ALIGN_TOP_RIGHT) || (a == SUB_ALIGN_MIDDLE_RIGHT));
}

bool MediaPlayer::VM_Subtitle::isAlignedTop()
{
	VM_SubAlignment a = this->getAlignment();
	return ((a == SUB_ALIGN_TOP_LEFT) || (a == SUB_ALIGN_TOP_RIGHT) || (a == SUB_ALIGN_TOP_CENTER));
}

int MediaPlayer::VM_Subtitle::setPTS(LIB_FFMPEG::AVPacket* packet, LIB_FFMPEG::AVSubtitle &subFrame, LIB_FFMPEG::AVStream* subStream)
{
	if ((packet == NULL) || (subStream == NULL))
		return ERROR_INVALID_ARGUMENTS;

	bool useFrame = (packet->dts < subStream->cur_dts);

	// NO DURATION - UPDATE END PTS
	if ((packet->duration == 0) && (packet->size < 100))
	{
		if (useFrame)
			this->ptsEnd = (double)((double)subFrame.pts / AV_TIME_BASE_D);
		else
			this->ptsEnd = (double)((double)packet->pts * LIB_FFMPEG::av_q2d(subStream->time_base));

		return RESULT_OK;
	}

	// SET START PTS
	if (useFrame) {
		this->ptsStart = (double)((double)(subFrame.pts - subStream->start_time) / AV_TIME_BASE_D);
	} else {
		this->ptsStart = (double)((double)(packet->pts  - subStream->start_time) * LIB_FFMPEG::av_q2d(subStream->time_base));

		if (this->ptsStart < VM_Player::ProgressTime)
			this->ptsStart += (double)((double)subStream->start_time * LIB_FFMPEG::av_q2d(subStream->time_base));
	}

	if (subFrame.start_display_time > 0)
		this->ptsStart += (double)((double)subFrame.start_display_time / (double)ONE_SECOND_MS);

	// SET END PTS
	if ((packet->duration > 0) && (subFrame.end_display_time > 0))
		this->ptsEnd = (double)(this->ptsStart + (double)((double)subFrame.end_display_time / (double)ONE_SECOND_MS));
	// NO DURATION - SET DEFAULT END PTS BASED ON PACKET SIZE
	else
		this->ptsEnd = (this->ptsStart + SUB_MAX_DURATION);

	return RESULT_OK;
}
