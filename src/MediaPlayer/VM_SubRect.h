#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_SUBRECT_H
#define VM_SUBRECT_H

namespace VoyaMedia
{
	namespace MediaPlayer
	{
		const int NR_OF_DATA_PLANES = 4;

		class VM_SubRect
		{
		public:
			VM_SubRect(const LIB_FFMPEG::AVSubtitleRect &subRect);
			VM_SubRect();
			~VM_SubRect();

		public:
			int                        x, y, w, h;
			int                        nb_colors;
			uint8_t*                   data[NR_OF_DATA_PLANES];
			int                        linesize[NR_OF_DATA_PLANES];
			LIB_FFMPEG::AVSubtitleType type;
			String                     text;
			String                     ass;
			int                        flags;
		};
	}
}

#endif
