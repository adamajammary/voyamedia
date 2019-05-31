#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_BUTTON_H
#define VM_BUTTON_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_Button : public VM_Component
		{
		public:
			VM_Button(const VM_Component &component);
			VM_Button(const String &id);
			~VM_Button();

		public:
			SDL_Rect    imageArea;
			VM_Texture* imageData;
			int         mediaID;
			String      mediaID2;
			String      mediaURL;
			SDL_Rect    textArea;
			VM_Texture* textData;
			VM_Color    textColor;

		private:
			String inputText;
			String text;

		public:
			String      getInputText();
			String      getText();
			void        removeImage();
			void        removeText();
			virtual int render();
			int         setImage(const String &file, bool thumb, int width = 0, int height = 0);
			int         setImage(VM_Image* image);
			int         setImageTextAlign();
			void        setInputText(const String &text);
			int         setText(const String &text);
			void        setThumb(const String &tableID = "");

		private:
			String getImageFile(const String &file);
			int    getImageSize(const String &attribSize, int maxSize);
			int    getParentHeight();
			int    getParentWidth();
			String getTextValue();
			void   init(const String &id);
			void   setImageSize();

		};
	}
}

#endif
