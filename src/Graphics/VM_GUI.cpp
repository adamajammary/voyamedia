#include "VM_GUI.h"

using namespace VoyaMedia::Database;
using namespace VoyaMedia::MediaPlayer;
using namespace VoyaMedia::System;
using namespace VoyaMedia::XML;

StringMap                 Graphics::VM_GUI::ColorTheme;
String                    Graphics::VM_GUI::ColorThemeFile = "";;
Graphics::VM_ComponentMap Graphics::VM_GUI::Components;
Graphics::VM_Table*       Graphics::VM_GUI::ListTable      = NULL;
Graphics::VM_Panel*       Graphics::VM_GUI::rootPanel      = NULL;
Graphics::VM_Component*   Graphics::VM_GUI::TextInput      = NULL;
LIB_XML::xmlDoc*          Graphics::VM_GUI::xmlDoc         = NULL;

void Graphics::VM_GUI::Close()
{
	VM_ThreadManager::Mutex.lock();

	VM_GUI::ListTable = NULL;

	VM_GUI::ColorTheme.clear();
	VM_GUI::Components.clear();

	DELETE_POINTER(VM_GUI::rootPanel);

	FREE_XML_DOC(VM_GUI::xmlDoc);
	LIB_XML::xmlCleanupParser();

	VM_ThreadManager::Mutex.unlock();
}

int Graphics::VM_GUI::hideComponents(VM_Component* component)
{
	if (component == NULL)
		return ERROR_INVALID_ARGUMENTS;

	component->backgroundArea = {};

	for (auto button : component->buttons)
	{
		VM_Button* button2 = dynamic_cast<VM_Button*>(button);

		button2->removeImage();
		button2->removeText();
		button2->backgroundArea = {};
	}

	for (auto child : component->children)
		VM_GUI::hideComponents(child);

	return RESULT_OK;
}

int Graphics::VM_GUI::Load(const char* file, const char* title)
{
	if (file == NULL)
		return ERROR_INVALID_ARGUMENTS;

	VM_GUI::xmlDoc = VM_XML::Load(VM_FileSystem::GetPathGUI().append(file).c_str());

	// FAILED TO LOAD XML FILE
	if (VM_GUI::xmlDoc == NULL)
		return ERROR_UNKNOWN;

	LIB_XML::xmlNode* windowNode = VM_XML::GetNode("/window", VM_GUI::xmlDoc);

	// LOAD WINDOW
	if ((windowNode == NULL) || (VM_GUI::loadWindow(windowNode, title) != RESULT_OK))
		return ERROR_UNKNOWN;

	// LOAD COMPOMNENT NODES (FROM XML)
	int result = VM_GUI::LoadComponentNodes(windowNode, NULL, VM_GUI::xmlDoc, VM_GUI::rootPanel, VM_GUI::Components);

	if (result != RESULT_OK)
		return result;

	// RESIZE GUI BASED ON WINDOW DIMENSIONS
	result = VM_GUI::refresh();

	if (result != RESULT_OK)
		return result;
	
	return result;
}

StringMap Graphics::VM_GUI::LoadColorTheme(const String &colorTheme)
{
	StringMap theme;

	if (colorTheme.empty())
		return theme;

	String        line;
	std::ifstream colorThemeFile;

	#if defined _windows
		WString colorThemePath = (VM_FileSystem::GetPathGUIW() + VM_Text::ToUTF16(colorTheme.c_str()) + L".colortheme");
	#else
		String colorThemePath = (VM_FileSystem::GetPathGUI() + colorTheme + ".colortheme");
	#endif

	colorThemeFile.open(colorThemePath.c_str());

	if (colorThemeFile.good())
	{
		Strings pair;

		while (std::getline(colorThemeFile, line))
		{
			if (line.empty() || line[0] == '#')
				continue;

			pair = VM_Text::Split(line.c_str(), "=");

			if (pair.size() > 1)
				theme.insert({ pair[0], pair[1] });
		}
	} else {
		colorThemeFile.close();
		return theme;
	}

	colorThemeFile.close();

	return theme;
}

int Graphics::VM_GUI::loadWindow(LIB_XML::xmlNode* windowNode, const char* title)
{
	if ((windowNode == NULL) || (title == NULL))
		return ERROR_INVALID_ARGUMENTS;

	bool windowCentered  = true;
	bool windowMaximized = false;

	int  dbResult;
	auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

	if (DB_RESULT_ERROR(dbResult))
	{
		VM_Modal::ShowMessage(VM_Window::Labels["error.db_settings"]);
		DELETE_POINTER(db);

		return ERROR_UNKNOWN;
	}

	VM_GUI::ColorThemeFile = db->getSettings("color_theme");

	#if defined _android || defined _ios
		VM_Window::Dimensions.x = SDL_WINDOWPOS_UNDEFINED;
		VM_Window::Dimensions.y = SDL_WINDOWPOS_UNDEFINED;
		VM_Window::Dimensions.w = MAX_WINDOW_SIZE;
		VM_Window::Dimensions.h = MAX_WINDOW_SIZE;
	#else
		// USE SETTINGS STORED IN DATABASE
		if (!VM_GUI::ColorThemeFile.empty())
		{
			windowMaximized = (db->getSettings("window_maximized") == "1");

			VM_Window::Dimensions.x = std::atoi(db->getSettings("window_x").c_str());
			VM_Window::Dimensions.y = std::atoi(db->getSettings("window_y").c_str());
			VM_Window::Dimensions.w = std::atoi(db->getSettings("window_w").c_str());
			VM_Window::Dimensions.h = std::atoi(db->getSettings("window_h").c_str());
		}
		// USE DEFAULT SETTINGS FROM XML DESIGN FILE
		else
		{
			windowCentered  = (VM_XML::GetAttribute(windowNode, "centered")  == "true");
			windowMaximized = (VM_XML::GetAttribute(windowNode, "maximized") == "true");

			if (windowCentered) {
				VM_Window::Dimensions.x = SDL_WINDOWPOS_CENTERED;
				VM_Window::Dimensions.y = SDL_WINDOWPOS_CENTERED;
			} else {
				VM_Window::Dimensions.x = std::atoi(VM_XML::GetAttribute(windowNode, "x").c_str());
				VM_Window::Dimensions.y = std::atoi(VM_XML::GetAttribute(windowNode, "y").c_str());
			}

			VM_Window::Dimensions.w = std::atoi(VM_XML::GetAttribute(windowNode, "width").c_str());
			VM_Window::Dimensions.h = std::atoi(VM_XML::GetAttribute(windowNode, "height").c_str());
		}
	#endif
	
	// USE SETTINGS STORED IN DATABASE
	if (!VM_GUI::ColorThemeFile.empty())
	{
		VM_GUI::ColorThemeFile = db->getSettings("color_theme");
		VM_GUI::ColorTheme     = VM_GUI::LoadColorTheme(VM_GUI::ColorThemeFile);

		VM_Window::SystemLocale = (db->getSettings("system_locale") == "1");
		VM_Window::Labels       = VM_Text::GetLabels();
	}
	// USE DEFAULT SETTINGS FROM XML DESIGN FILE
	else
	{
		// COLOR THEME
		VM_GUI::ColorThemeFile = VM_XML::GetAttribute(windowNode, "color-theme");
		VM_GUI::ColorTheme     = VM_GUI::LoadColorTheme(VM_GUI::ColorThemeFile );

		if (VM_GUI::ColorTheme.empty())
			return ERROR_UNKNOWN;

		db->updateSettings("color_theme", VM_GUI::ColorThemeFile);
	}

	DELETE_POINTER(db);

	// CREATE WINDOW
	uint32_t flagsWindow = (SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

	#if defined _ios
		[[UIApplication sharedApplication] setStatusBarHidden:NO];
		[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
	#elif defined _linux || defined _macosx || defined _windows
		flagsWindow |= (SDL_WINDOW_MOUSE_FOCUS);
	#endif

	if (VM_Window::MainWindow == NULL)
	{
		VM_Window::MainWindow = SDL_CreateWindow(
			title,
			VM_Window::Dimensions.x, VM_Window::Dimensions.y,
			VM_Window::Dimensions.w, VM_Window::Dimensions.h,
			flagsWindow
		);
	}

	if (VM_Window::MainWindow == NULL) {
		VM_Modal::ShowMessage(VM_Window::Labels["error.window"]);
		return ERROR_UNKNOWN;
	}

	if (windowMaximized)
		SDL_MaximizeWindow(VM_Window::MainWindow);

	VM_Window::Display.setDisplayMode();
	VM_Window::StatusBarHeight = VM_Graphics::GetTopBarHeight();

	// RENDERER
	if (VM_Window::Renderer != NULL)
		FREE_RENDERER(VM_Window::Renderer);

	VM_Window::Renderer = SDL_CreateRenderer(VM_Window::MainWindow, -1, SDL_RENDERER_ACCELERATED);

	if (VM_Window::Renderer == NULL)
		VM_Window::Renderer = SDL_CreateRenderer(VM_Window::MainWindow, -1, SDL_RENDERER_SOFTWARE);

	if (VM_Window::Renderer == NULL) {
		VM_Modal::ShowMessage(VM_Window::Labels["error.renderer"]);
		return ERROR_UNKNOWN;
	}

	return RESULT_OK;
}

int Graphics::VM_GUI::LoadComponentNodes(LIB_XML::xmlNode* parentNode, VM_Panel* parentPanel, LIB_XML::xmlDoc* xmlDoc, VM_Panel* &rootPanel, VM_ComponentMap &components)
{
	if (parentNode == NULL)
		return ERROR_INVALID_ARGUMENTS;

	VM_XmlNodes panelNodes = VM_XML::GetChildNodes(parentNode, xmlDoc);

	for (auto node : panelNodes)
	{
		if (node->type != LIB_XML::XML_ELEMENT_NODE)
			continue;

		String id = VM_XML::GetAttribute(node, "id");

		// PANEL
		if (strcmp(reinterpret_cast<const char*>(node->name), "panel") == 0)
		{
			VM_Panel* panel = new VM_Panel(id);

			panel->xmlDoc  = xmlDoc;
			panel->xmlNode = node;

			components[panel->id] = panel;

			if (rootPanel == NULL) {
				rootPanel = panel;
			} else if (parentPanel != NULL) {
				parentPanel->children.push_back(panel);
				panel->parent = parentPanel;
			}

			VM_GUI::LoadComponentNodes(node, panel, xmlDoc, rootPanel, components);
		}
		// BUTTON
		else if (strcmp(reinterpret_cast<const char*>(node->name), "button") == 0)
		{
			VM_Button* button = new VM_Button(id);

			button->xmlDoc  = xmlDoc;
			button->xmlNode = node;

			components[button->id] = button;

			if (parentPanel != NULL) {
				parentPanel->buttons.push_back(button);
				button->parent = parentPanel;
			}
		}
		// TABLE
		else if (strcmp(reinterpret_cast<const char*>(node->name), "table") == 0)
		{
			VM_Table* table = new VM_Table(id);

			table->xmlDoc  = xmlDoc;
			table->xmlNode = node;

			VM_XmlNodes columnNodes = VM_XML::GetChildNodes(table->xmlNode, xmlDoc);

			for (auto columnNode : columnNodes)
			{
				if (columnNode->type != LIB_XML::XML_ELEMENT_NODE)
					continue;

				VM_Button* button = new VM_Button(VM_XML::GetAttribute(columnNode, "id"));

				button->xmlDoc  = xmlDoc;
				button->xmlNode = columnNode;

				components[button->id] = button;

				table->buttons.push_back(button);
				button->parent = table;
			}

			components[table->id] = table;

			if (parentPanel != NULL) {
				parentPanel->children.push_back(table);
				parentPanel->tables.push_back(table);
				table->parent = parentPanel;
			}
		}
	}

	return RESULT_OK;
}

int Graphics::VM_GUI::LoadComponents(VM_Component* component)
{
	if (component == NULL)
		return ERROR_INVALID_ARGUMENTS;

	VM_GUI::loadComponentsFixed(component);
	VM_GUI::loadComponentsRelative(component);

	return RESULT_OK;
}

int Graphics::VM_GUI::loadComponentsFixed(VM_Component* component)
{
	if (component == NULL)
		return ERROR_INVALID_ARGUMENTS;

	VM_GUI::setComponent(component);

	for (auto button : component->buttons)
		VM_GUI::setComponent(button);

	for (auto child : component->children)
		VM_GUI::loadComponentsFixed(child);

	return RESULT_OK;
}

int Graphics::VM_GUI::loadComponentsRelative(VM_Component* component)
{
	if (component == NULL)
		return ERROR_INVALID_ARGUMENTS;

	for (auto child : component->children) {
		if (child->visible)
			child->setSizePercent(component);
	}

	for (auto button : component->buttons) {
		if (button->visible)
			button->setSizePercent(component);
	}

	VM_GUI::setComponentSizeBlank(component, component->children);
	VM_GUI::setComponentSizeBlank(component, component->buttons);

	VM_GUI::setComponentPositionAlign(component, component->children);
	VM_GUI::setComponentPositionAlign(component, component->buttons);

	for (auto child : component->children)
		VM_GUI::loadComponentsRelative(child);

	return RESULT_OK;
}

int Graphics::VM_GUI::LoadComponentsImage(VM_Component* component)
{
	if (component == NULL)
		return ERROR_INVALID_ARGUMENTS;

	for (auto button : component->buttons)
		VM_GUI::setComponentImage(button);

	for (auto child : component->children)
		VM_GUI::LoadComponentsImage(child);

	return RESULT_OK;
}

int Graphics::VM_GUI::LoadComponentsText(VM_Component* component)
{
	if (component == NULL)
		return ERROR_INVALID_ARGUMENTS;

	for (auto button : component->buttons)
		VM_GUI::setComponentText(button);

	for (auto child : component->children)
		VM_GUI::LoadComponentsText(child);

	return RESULT_OK;
}

int Graphics::VM_GUI::LoadTables(VM_Component* component)
{
	if (component == NULL)
		return ERROR_INVALID_ARGUMENTS;

	for (auto table : component->tables)
		VM_GUI::setTable(component, table);

	for (auto child : component->children)
		VM_GUI::LoadTables(child);

	return RESULT_OK;
}

int Graphics::VM_GUI::refresh()
{
	if ((VM_GUI::xmlDoc == NULL) || (VM_GUI::rootPanel == NULL))
		return ERROR_INVALID_ARGUMENTS;

	VM_GUI::Components["bottom_player_controls_middle_thumb"]->backgroundArea.w = 0;
	VM_GUI::Components["bottom_player_controls_volume_thumb"]->backgroundArea.w = 0;

	VM_GUI::rootPanel->backgroundArea = {
		0, VM_Window::StatusBarHeight, VM_Window::Dimensions.w, (VM_Window::Dimensions.h - VM_Window::StatusBarHeight)
	};

	int result = VM_GUI::LoadComponents(VM_GUI::rootPanel);

	if (result != RESULT_OK)
		return result;

	result = VM_GUI::LoadComponentsImage(VM_GUI::rootPanel);

	if (result != RESULT_OK)
		return result;

	result = VM_GUI::LoadComponentsText(VM_GUI::rootPanel);

	if (result != RESULT_OK)
		return result;

	result = VM_GUI::LoadTables(VM_GUI::rootPanel);

	if (result != RESULT_OK)
		return result;

	VM_Top::Refresh();

	VM_GUI::TextInput = VM_GUI::Components["middle_search"];

	return RESULT_OK;
}

int Graphics::VM_GUI::render()
{
	if (VM_GUI::rootPanel != NULL)
		VM_GUI::rootPanel->render();

	return RESULT_OK;
}

int Graphics::VM_GUI::setComponent(VM_Component* component)
{
	if (component == NULL)
		return ERROR_INVALID_ARGUMENTS;

	component->setColors();
	component->setSizeFixed();

	return RESULT_OK;
}

int Graphics::VM_GUI::setComponentSizeBlank(VM_Component* parent, VM_Components components)
{
	if ((parent == NULL) || components.empty())
		return ERROR_INVALID_ARGUMENTS;

	String orientation = VM_XML::GetAttribute(parent->xmlNode, "orientation");
	int    sizeX       = (parent->backgroundArea.w - parent->borderWidth.left - parent->borderWidth.right);
	int    sizeY       = (parent->backgroundArea.h - parent->borderWidth.top  - parent->borderWidth.bottom);
	int    compsX      = 0;
	int    compsY      = 0;

	for (auto component : components)
	{
		if (!component->visible)
			continue;

		String width  = VM_XML::GetAttribute(component->xmlNode, "width");
		String height = VM_XML::GetAttribute(component->xmlNode, "height");

		// VERTICAL
		if (orientation == "vertical")
		{
			compsX = 1;

			if (height.empty()) {
				compsY++;
				sizeY -= parent->margin.bottom;
			} else {
				sizeY -= (component->backgroundArea.h + parent->margin.bottom);
			}
		}
		// HORIZONTAL
		else
		{
			compsY = 1;

			if (width.empty()) {
				compsX++;
				sizeX -= parent->margin.right;
			} else {
				sizeX -= (component->backgroundArea.w + parent->margin.right);
			}
		}
	}

	sizeX -= parent->margin.right;
	sizeY -= parent->margin.bottom;

	for (auto component : components) {
		if (component->visible)
			component->setSizeBlank(sizeX, sizeY, compsX, compsY);
	}

	return RESULT_OK;
}

int Graphics::VM_GUI::setComponentPositionAlign(VM_Component* parent, VM_Components components)
{
	if (parent == NULL)
		return ERROR_INVALID_ARGUMENTS;

	String orientation = VM_XML::GetAttribute(parent->xmlNode, "orientation");
	String hAlign      = VM_XML::GetAttribute(parent->xmlNode, "halign");
	String vAlign      = VM_XML::GetAttribute(parent->xmlNode, "valign");
	int    offsetX     = (parent->backgroundArea.x + parent->borderWidth.left + parent->margin.left);
	int    offsetY     = (parent->backgroundArea.y + parent->borderWidth.top  + parent->margin.top);
	int    maxX        = (parent->backgroundArea.w - parent->borderWidth.left - parent->borderWidth.right  - parent->margin.left - parent->margin.right);
	int    maxY        = (parent->backgroundArea.h - parent->borderWidth.top  - parent->borderWidth.bottom - parent->margin.top  - parent->margin.bottom);
	int    remainingX  = (maxX + parent->margin.right);
	int    remainingY  = (maxY + parent->margin.bottom);

	int x, y;

	// TOP-LEFT ALIGN
	for (auto component : components)
	{
		if (!component->visible)
			continue;

		component->setPositionAlign(offsetX, offsetY);

		x = (component->backgroundArea.w + parent->margin.right);
		y = (component->backgroundArea.h + parent->margin.bottom);

		if (orientation == "vertical") {
			offsetY    += y;
			remainingY -= y;
		} else {
			offsetX    += x;
			remainingX -= x;
		}
	}

	// ALIGN
	for (auto component : components)
	{
		if (!component->visible)
			continue;

		offsetX = component->backgroundArea.x;
		offsetY = component->backgroundArea.y;

		// VERTICAL
		if (orientation == "vertical")
		{
			// V-ALIGN
			if (vAlign == "middle")
				offsetY += (remainingY / 2);
			else if (vAlign == "bottom")
				offsetY += remainingY;

			// H-ALIGN
			if (hAlign == "center")
				offsetX += ((maxX - component->backgroundArea.w) / 2);
			else if (hAlign == "right")
				offsetX += (maxX - component->backgroundArea.w);
		}
		// HORIZONTAL
		else
		{
			// H-ALIGN
			if (hAlign == "center")
				offsetX += (remainingX / 2);
			else if (hAlign == "right")
				offsetX += remainingX;

			// V-ALIGN
			if (vAlign == "middle")
				offsetY += ((maxY - component->backgroundArea.h) / 2);
			else if (vAlign == "bottom")
				offsetY += (maxY - component->backgroundArea.h);
		}

		component->setPositionAlign(offsetX, offsetY);
	}

	return RESULT_OK;
}

int Graphics::VM_GUI::setComponentImage(VM_Component* component)
{
	if (component == NULL)
		return ERROR_INVALID_ARGUMENTS;

	VM_Button* button    = dynamic_cast<VM_Button*>(component);
	String     imageFile = VM_XML::GetAttribute(button->xmlNode, "image-file");

	if (!imageFile.empty())
		button->setImage(imageFile, false);

	return RESULT_OK;
}

int Graphics::VM_GUI::setComponentText(VM_Component* component)
{
	if (component == NULL)
		return ERROR_INVALID_ARGUMENTS;

	return dynamic_cast<VM_Button*>(component)->setText("");
}

int Graphics::VM_GUI::setTable(VM_Component* parent, VM_Component* component)
{
	if (component == NULL)
		return ERROR_INVALID_ARGUMENTS;

	VM_Table* table = dynamic_cast<VM_Table*>(component);

	if (table->id == "list_table")
	{
		VM_GUI::ListTable = table;

		if (!VM_Player::State.isStopped || VM_Player::State.openFile)
		{
			VM_GUI::hideComponents(VM_GUI::Components["bottom_controls"]);
			VM_GUI::Components["bottom_player_controls"]->backgroundArea = VM_GUI::Components["bottom"]->backgroundArea;
			VM_GUI::LoadComponents(VM_GUI::Components["bottom_player_controls"]);
			VM_GUI::showPlayerControls();
		}
		else if ((table != NULL) && table->visible)
		{
			VM_GUI::Components["bottom_controls"]->backgroundArea = VM_GUI::Components["bottom"]->backgroundArea;

			for (auto button : VM_GUI::Components["bottom_player_controls_controls_left"]->buttons)
				dynamic_cast<VM_Button*>(button)->removeImage();

			for (auto button : VM_GUI::Components["bottom_player_controls_controls_right"]->buttons)
				dynamic_cast<VM_Button*>(button)->removeImage();

			VM_PlayerControls::Hide();
			VM_PlayerControls::Init();
			VM_PlayerControls::Refresh();

			VM_GUI::LoadComponents(VM_GUI::Components["bottom_controls"]);
			VM_GUI::LoadComponentsImage(VM_GUI::Components["bottom_controls"]);
			VM_GUI::LoadComponentsText(VM_GUI::Components["bottom_controls"]);

			table->resetScroll(VM_GUI::Components["list_table_scrollbar"]);
			table->setHeader();
			table->refreshRows();
		}
	}
	else if (VM_Modal::IsVisible() && (table != NULL) && table->visible)
	{
		table->resetScroll(VM_Modal::Components["list_table_scrollbar"]);
		table->setHeader();
		table->refreshRows();
	}

	return RESULT_OK;
}

void Graphics::VM_GUI::showPlayerControls()
{
	VM_PlayerControls::Show();
	VM_PlayerControls::Init();
	VM_PlayerControls::Refresh();

	VM_Component* player = VM_GUI::Components["bottom_player"];

	player->backgroundArea = {};
	player->borderWidth    = {};
}
