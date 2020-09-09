#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_TEXTINPUT_H
#define VM_TEXTINPUT_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_TextInput
		{
		private:
			VM_TextInput()  {}
			~VM_TextInput() {}

		private:
			static bool       active;
			static VM_Button* activeButton;
			static int        cursor;
			static bool       refreshPending;
			static String     text;

		public:
			static bool   IsActive();
			static int    SetActive(bool active, VM_Button* button = NULL);
			static void   Backspace();
			static void   Clear();
			static void   Delete();
			static void   Left();
			static void   Right();
			static void   Home();
			static void   End();
			static String GetText();
			static void   Paste();
			static void   Refresh();
			static void   SaveToDB();
			static int    Unfocus();
			static void   Update(const char* text);
			static void   Update();

		};
	}
}

#endif
