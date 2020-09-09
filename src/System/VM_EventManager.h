#ifndef GLOBAL
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_EVENTMANAGER_H
#define VM_EVENTMANAGER_H

namespace VoyaMedia
{
	namespace System
	{
		class VM_EventManager
		{
		private:
			VM_EventManager()  {}
			~VM_EventManager() {}

		public:
			static VM_TouchEventType TouchEvent;

		private:
			#if defined _android || defined _ios
				//static float    swipeDistanceX;
				//static float    swipeDistanceY;
				static uint32_t touchDownTimestamp;
				static uint32_t touchUpTimestamp;
			#endif

		public:
			static int HandleEvents();
			static int HandleMediaPlayer();

			#if defined _android
				static int  HandleHeadSetUnpluggedAndroid();
				static int  HandleStoragePermissionAndroid();
				static void WakeLockStart();
				static void WakeLockStop();
			#elif defined _ios
				static int ConfigureAudioSessionIOS();
			#endif

			#if defined _android || defined _ios
				static int HandleEventsMobile(void* userdata, SDL_Event* deviceEvent);
			#endif

		private:
			static int  handleMouseClick(SDL_Event* mouseEvent);
			static bool isClickedBottom(SDL_Event* mouseEvent);
			static bool isClickedBottomControls(SDL_Event* mouseEvent);
			static bool isClickedBottomPlayerControls(SDL_Event* mouseEvent);
			static bool isClickedModal(SDL_Event* mouseEvent);
			static bool isClickedTable(SDL_Event* mouseEvent, Graphics::VM_Table* table, Graphics::VM_Component* scrollBar);
			static bool isClickedTableBottom(SDL_Event* mouseEvent, Graphics::VM_Component* bottomPanel, Graphics::VM_Table* table);
			static bool isClickedTextInput(SDL_Event* mouseEvent, Graphics::VM_Component* inputPanel);
			static bool isClickedTop(SDL_Event* mouseEvent);
			static bool isClickedTopBar(SDL_Event* mouseEvent);
			static bool isKeyPressedPlayer(SDL_Keycode key, uint16_t mod);
			static bool isKeyPressedTable(SDL_Keycode key, Graphics::VM_Table* table);
			static bool isKeyPressedTextInput(SDL_Keycode key, uint16_t mod);

			#if defined _ios
				static int handleHeadSetUnpluggedIOS(NSNotification* notification);
			#endif
		};
	}
}

#endif
