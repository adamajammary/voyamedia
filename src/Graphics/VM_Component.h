#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_COMPONENT_H
#define VM_COMPONENT_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_Component
		{
		public:
			VM_Component();
			~VM_Component();

		public:
			bool              active;
			VM_Color          activeColor;
			SDL_Rect          backgroundArea;
			VM_Color          backgroundColor;
			VM_Color          borderColor;
			VM_Border         borderWidth;
			VM_Components     buttons;
			VM_Components     children;
			int               fontSize;
			VM_Color          highlightColor;
			String            id;
			VM_Border         margin;
			VM_Color          overlayColor;
			VM_Component*     parent;
			bool              selected;
			VM_Components     tables;
			bool              visible;
			LIB_XML::xmlDoc*  xmlDoc;
			LIB_XML::xmlNode* xmlNode;

		public:
			virtual int render();
			void        setColors();
			void        setPositionAlign(int positionX, int positionY);
			void        setSizeBlank(int sizeX, int sizeY, int compsX, int compsY);
			void        setSizeFixed();
			int         setSizePercent(VM_Component* parent);

		protected:
			VM_Color getColor(const String &attrib);

		};
	}
}

#endif
