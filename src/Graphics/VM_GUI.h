#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_GUI_H
#define VM_GUI_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_GUI
		{
		private:
			VM_GUI()  {}
			~VM_GUI() {}

		public:
			static StringMap       ColorTheme;
			static String          ColorThemeFile;
			static VM_ComponentMap Components;
			static VM_Table*       ListTable;
			static VM_Component*   TextInput;

		private:
			static VM_Panel*        rootPanel;
			static LIB_XML::xmlDoc* xmlDoc;

		public:
			static void      Close();
			static int       Load(const char* file, const char* title);
			static StringMap LoadColorTheme(const String &colorTheme);
			static int       LoadComponentNodes(LIB_XML::xmlNode* parentNode, VM_Panel* parentPanel, LIB_XML::xmlDoc* xmlDoc, VM_Panel* &rootPanel, VM_ComponentMap &components);
			static int       LoadComponents(VM_Component* component);
			static int       LoadComponentsImage(VM_Component* component);
			static int       LoadComponentsText(VM_Component* component);
			static int       LoadTables(VM_Component* component);
			static int       render();
			static int       refresh();

		private:
			static int  hideComponents(VM_Component* component);
			static int  loadWindow(LIB_XML::xmlNode* windowNode, const char* title);
			static int  loadComponentsFixed(VM_Component* component);
			static int  loadComponentsRelative(VM_Component* component);
			static int  setComponent(VM_Component* component);
			static int  setComponentSizeBlank(VM_Component* parent, VM_Components components);
			static int  setComponentPositionAlign(VM_Component* parent, VM_Components components);
			static int  setComponentImage(VM_Component* button);
			static int  setComponentText(VM_Component* button);
			static int  setTable(VM_Component* parent, VM_Component* component);
			static void showPlayerControls();

		};
	}
}

#endif
