#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_MEDIATIME_H
#define VM_MEDIATIME_H

namespace VoyaMedia
{
	namespace MediaPlayer
	{
		class VM_MediaTime
		{
		public:
			VM_MediaTime();
			VM_MediaTime(double seconds);
			~VM_MediaTime() {}

		public:
			int hours;
			int minutes;
			int seconds;
			int totalSeconds;

		};
	}
}

#endif
