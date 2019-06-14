#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_WINDOW_H
#define VM_WINDOW_H

namespace VoyaMedia
{
	namespace System
	{
		class VM_Window
		{
		private:
			VM_Window()  {}
			~VM_Window() {}

		public:
			static int                  BottomHeight;
			static SDL_Rect             Dimensions;
			static SDL_Rect             DimensionsBeforeFS;
			static Graphics::VM_Display Display;
			static bool                 FullScreenEnabled;
			static bool                 FullScreenMaximized;
			static bool                 Inactive;
			static bool                 IsDoneRendering;
			static StringMap            Labels;
			static SDL_Window*          MainWindow;
			static String               OpenURL;
			static bool                 PauseRendering;
			static bool                 Quit;
			static SDL_Renderer*        Renderer;
			static bool                 ResetRenderer;
			static uint32_t             ResizeTimestamp;
			static bool                 SaveToDB;
			static int                  StatusBarHeight;
			static char                 StatusString[DEFAULT_CHAR_BUFFER_SIZE];
			static bool                 SystemLocale;
			static String               WorkingDirectory;
			static WString              WorkingDirectoryW;

			#if defined _android
				static String               AndroidStoragePath;
				static Android::VM_JavaJNI* JNI;
				static SDL_Thread*          MedaPlayerThread;
				static bool                 MinimizeWindow;
				static bool                 StartWakeLock;
				static bool                 StopWakeLock;
			#endif
		
		private:
			static time_t currentTime;
			static bool   refreshPending;

		public:
			static void Close();
			static int  Open(const char* guiXML, const char* title);
			static void Refresh();
			static int  Render();
			static int  Reset(const char* guiXML, const char* title);
			static void Save();
			static void SetStatusProgress(uint32_t current, uint32_t total, const String &label);

			#if defined _android
				static int Minimize();
			#endif

		private:
			static void resize();
			static void saveToDB();
			static void showVersionMessage(VM_Version version);

		};
	}
}

#endif
