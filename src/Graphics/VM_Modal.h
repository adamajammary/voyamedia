#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_MODAL_H
#define VM_MODAL_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_Modal
		{
		private:
			VM_Modal()  {}
			~VM_Modal() {}

		public:
			static VM_ComponentMap Components;
			static String          File;
			static VM_Table*       ListTable;
			static VM_Component*   TextInput;

		private:
			static bool              centered;
			static SDL_Rect          dimensions;
			static String            messageText;
			static LIB_XML::xmlNode* node;
			static bool              refreshPending;
			static VM_Panel*         rootPanel;
			static bool              visible;
			static LIB_XML::xmlDoc*  xmlDoc;

		public:
			static int  Apply(const String &buttonID = "");
			static void Hide();
			static bool IsVisible();
			static int  Open(const String &id);
			static int  Refresh();
			static void Render();
			static int  ShowMessage(const String &messageText);
			static void Update();
			static int  UpdateLabelsDetails();
			static int  UpdateLabelsDetailsPicture();

		private:
			static void hide();
			static int  open();
			static bool updateMediaDB(LIB_FFMPEG::AVFormatContext* formatContext, const String &filePath, int mediaID, String &duration, String &size, String &path);
			static int  updateLabels();
			static int  updateLabelsDetailsAudio(int mediaID,   const String &filePath2, const String &filePath);
			static int  updateLabelsDetailsPicture(int mediaID, const String &filePath2, const String &filePath);
			static int  updateLabelsDetailsVideo(int mediaID,   const String &filePath2, const String &filePath);
			static void updateLabelsMessage();
			static void updateLabelsPlaylistSave();
			static void updateLabelsSettings();
			static void updateLabelsRightClick();
			static void updateLabelsUPNP();
		};
	}
}

#endif
