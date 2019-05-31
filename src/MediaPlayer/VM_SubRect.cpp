#include "VM_SubRect.h"

// https://code.soundsoftware.ac.uk/projects/pmhd/embedded/pgssubdec_8c_source.html
MediaPlayer::VM_SubRect::VM_SubRect(const LIB_FFMPEG::AVSubtitleRect &subRect)
{
	this->x = subRect.x;
	this->y = subRect.y;
	this->w = subRect.w;
	this->h = subRect.h;

	this->nb_colors = subRect.nb_colors;
	this->flags     = subRect.flags;
	this->type      = subRect.type;

	this->ass  = (subRect.ass  != NULL ? String(subRect.ass)  : "");
	this->text = (subRect.text != NULL ? String(subRect.text) : "");

	for (int i = 0; i < NR_OF_DATA_PLANES; i++)
		this->linesize[i] = subRect.linesize[i];

	const size_t DATA_SIZE = (subRect.linesize[0] * subRect.h);

	this->data[0] = (uint8_t*)LIB_FFMPEG::av_memdup(subRect.data[0], DATA_SIZE);
	this->data[1] = (uint8_t*)LIB_FFMPEG::av_memdup(subRect.data[1], AVPALETTE_SIZE);
	this->data[2] = this->data[3] = NULL;
}

MediaPlayer::VM_SubRect::VM_SubRect()
{
	this->x = this->y = this->w = this->h = this->nb_colors = this->flags = 0;

	this->type = LIB_FFMPEG::SUBTITLE_NONE;
	this->text = this->ass = "";

	for (int i = 0; i < NR_OF_DATA_PLANES; i++)
		this->linesize[i] = 0;

	for (int i = 0; i < NR_OF_DATA_PLANES; i++)
		this->data[i] = NULL;
}

MediaPlayer::VM_SubRect::~VM_SubRect()
{
	this->x = this->y = this->w = this->h = this->nb_colors = this->flags = 0;

	this->type = LIB_FFMPEG::SUBTITLE_NONE;
	this->text = this->ass = "";

	for (int i = 0; i < NR_OF_DATA_PLANES; i++)
		this->linesize[i] = 0;

	for (int i = 0; i < NR_OF_DATA_PLANES; i++)
		FREE_AVPOINTER(this->data[i]);
}
