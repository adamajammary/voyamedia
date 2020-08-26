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
			static VM_MediaTime   durationTime;
			static float          progressPercent;
			static VM_MediaTime   progressTime;
			static bool           progressTimeLeft;
			static VM_RefreshType refreshType;
			static bool           visible;

		public:
			static SDL_Rect GetSnapshotArea();
			static int      Init();
			static bool     IsVisible();
			static int      Hide();
			static void     Refresh(VM_RefreshType refreshType = REFRESH_ALL);
			static int      RefreshControls();
			static int      RefreshProgressBar();
			static int      RefreshTime(time_t time);
			static int      Show();
			static int      Seek(SDL_Event* mouseEvent);
			static int      SetVolume(SDL_Event* mouseEvent);
			static int      ToggleProgressTimeLeft();

		private:
			static void refreshVolume();

		};
	}
}

#endif
