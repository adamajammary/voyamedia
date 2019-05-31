#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_PLAYERCONTROLS_H
#define VM_PLAYERCONTROLS_H

namespace VoyaMedia
{
	namespace MediaPlayer
	{
		class VM_PlayerControls
		{
		private:
			VM_PlayerControls()  {}
			~VM_PlayerControls() {}

		private:
			static VM_MediaTime durationTime;
			static float        progressPercent;
			static VM_MediaTime progressTime;
			static bool         progressTimeLeft;
			static bool         refreshPending;
			static bool         visible;

		public:
			static int  Init();
			static bool IsVisible();
			static int  Hide(bool skipFS = false);
			static void Refresh();
			static int  RefreshControls();
			static int  RefreshProgressBar();
			static int  RefreshTime(time_t time);
			static int  Show(bool skipFS = false);
			static int  Seek(SDL_Event* mouseEvent);
			static int  SetVolume(SDL_Event* mouseEvent);
			static int  ToggleProgressTimeLeft();

		};
	}
}

#endif
