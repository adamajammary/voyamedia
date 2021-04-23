#include "VM_Button.h"

using namespace VoyaMedia::System;
using namespace VoyaMedia::XML;

Graphics::VM_Button::VM_Button(const VM_Component &component)
{
	this->active          = component.active;
	this->activeColor     = component.activeColor;
	this->backgroundArea  = component.backgroundArea;
	this->backgroundColor = component.backgroundColor;
	this->borderColor     = component.borderColor;
	this->borderWidth     = component.borderWidth;
	this->fontSize        = component.fontSize;
	this->margin          = component.margin;
	this->overlayColor    = component.overlayColor;
	this->parent          = component.parent;
	this->xmlDoc          = component.xmlDoc;
	this->xmlNode         = component.xmlNode;

	this->init(component.id);
}

Graphics::VM_Button::VM_Button(const String &id)
{
	this->init(id);
}

void Graphics::VM_Button::init(const String &id)
{
	this->id        = id;
	this->mediaID   = 0;
	this->imageArea = {};
	this->imageData = NULL;
	this->inputText = "";
	this->mediaURL  = "";
	this->text      = "";
	this->textArea  = {};
	this->textData  = NULL;
	this->textColor = {};
}

Graphics::VM_Button::~VM_Button()
{
	DELETE_POINTER(this->imageData);
	DELETE_POINTER(this->textData);
}

String Graphics::VM_Button::getImageFile(const String &file)
{
	String file2  = file;
	int    height = this->backgroundArea.h;

	if (height <= 0)
		height = this->getParentHeight();

	if (height <= 64)
		file2 = VM_Text::Replace(file2, "-512.png", "-64.png");
	else if (height <= 128)
		file2 = VM_Text::Replace(file2, "-512.png", "-128.png");
	else if (height <= 256)
		file2 = VM_Text::Replace(file2, "-512.png", "-256.png");

	return file2;
}

int Graphics::VM_Button::getImageSize(const String &attribSize, int maxSize)
{
	if (attribSize.empty())
		return maxSize;
	else if (attribSize[attribSize.length() - 1] == '%')
		return (std::atoi(attribSize.c_str()) * maxSize / 100);

	return std::atoi(attribSize.c_str());
}

int Graphics::VM_Button::getParentHeight()
{
	int           height = this->backgroundArea.h;
	VM_Component* parent = this->parent;

	while ((parent != NULL) && (parent->backgroundArea.h <= 0))
		parent = parent->parent;

	if (parent != NULL)
		height = parent->backgroundArea.h;

	return height;
}

int Graphics::VM_Button::getParentWidth()
{
	int           width  = this->backgroundArea.w;
	VM_Component* parent = this->parent;

	while ((parent != NULL) && (parent->backgroundArea.w <= 0))
		parent = parent->parent;

	if (parent != NULL)
		width = parent->backgroundArea.w;

	return width;
}

String Graphics::VM_Button::getTextValue()
{
	String text = "";

	if (this->xmlDoc == NULL)
		return text;

	text = VM_XML::GetValue(this->xmlNode, this->xmlDoc);

	if (text.empty())
		text = VM_XML::GetAttribute(this->xmlNode, "text");

	if (text.empty())
		return text;

	if (text[0] == '%')
		text = VM_Window::Labels[text.substr(1)];

	return text;
}

String Graphics::VM_Button::getInputText()
{
	return this->inputText;
}

String Graphics::VM_Button::getText()
{
	return (this->text.empty() && !this->inputText.empty() ? this->inputText : this->text);
}

void Graphics::VM_Button::removeText()
{
	DELETE_POINTER(this->textData);
	this->setImageTextAlign();
}

void Graphics::VM_Button::removeImage()
{
	VM_ThreadManager::ImageButtons[this->id].clear();
	DELETE_POINTER(this->imageData);
	this->setImageTextAlign();
}

int Graphics::VM_Button::render()
{
	if (!this->visible)
		return ERROR_INVALID_ARGUMENTS;

	SDL_Rect area = SDL_Rect(this->backgroundArea);

	area.x += this->borderWidth.left;
	area.y += this->borderWidth.top;
	area.w -= (this->borderWidth.left + this->borderWidth.right);
	area.h -= (this->borderWidth.top  + this->borderWidth.bottom);

	// BACKGROUND
	if (this->selected)
		VM_Graphics::FillArea(&this->highlightColor, &area);
	else
		VM_Graphics::FillArea(&this->backgroundColor, &area);

	// BORDER
	if (this->active)
		VM_Graphics::FillBorder(&this->activeColor, &this->backgroundArea, this->borderWidth);
	else
		VM_Graphics::FillBorder(&this->borderColor, &this->backgroundArea, this->borderWidth);

	// TEXT
	if ((this->textData != NULL) && (this->textData->data != NULL))
	{
		SDL_Rect clip = { 0, 0, min(this->textArea.w, area.w), min(this->textArea.h, area.h) };
		SDL_Rect dest = { this->textArea.x, this->textArea.y, clip.w, clip.h };

		SDL_RenderCopy(VM_Window::Renderer, this->textData->data, &clip, &dest);
	}

	// IMAGE
	if ((this->imageData != NULL) && (this->imageData->data != NULL))
		SDL_RenderCopy(VM_Window::Renderer, this->imageData->data, NULL, &this->imageArea);

	// OVERLAY
	if (this->overlayColor.a > 0)
		VM_Graphics::FillArea(&this->overlayColor, &area);

	return RESULT_OK;
}

int Graphics::VM_Button::setImage(const String &file, bool thumb, int width, int height)
{
	if (file.empty())
		return ERROR_INVALID_ARGUMENTS;

	#if defined _windows
		WString filePath = L"";
	#else
		String filePath = "";
	#endif

	// FILE_PATH
	if ((file.find("/") != String::npos) || (file.find("\\") != String::npos))
	{
		#if defined _windows
			filePath = VM_Text::ToUTF16(file.c_str());
		#else
			filePath = file;
		#endif
	}
	// THUMBNAILS
	else if (thumb)
	{
		#if defined _windows
			filePath = VM_FileSystem::GetPathThumbnailsW(VM_Text::ToUTF16(file.c_str()));

			if (!VM_FileSystem::FileExists("", filePath))
				filePath = L"";
		#else
			filePath = VM_FileSystem::GetPathThumbnails(file.c_str());

			if (!VM_FileSystem::FileExists(filePath, L""))
				filePath = "";
		#endif
	}
	// ICONS
	else
	{
		String file2 = file;

		if (file2.find("-512.png") != String::npos)
			file2 = this->getImageFile(file);

		#if defined _windows
			filePath = WString(VM_FileSystem::GetPathImagesW() + VM_Text::ToUTF16(file2.c_str()));

			if (!VM_FileSystem::FileExists("", filePath))
				filePath = WString(VM_FileSystem::GetPathImagesW() + VM_Text::ToUTF16(file.c_str()));
		#else
			filePath = String(VM_FileSystem::GetPathImages() + file2);

			if (!VM_FileSystem::FileExists(filePath, L""))
				filePath = String(VM_FileSystem::GetPathImages() + file);
		#endif
	}

	this->removeImage();

	if (filePath.empty())
		return ERROR_UNKNOWN;

	if ((width == 0) && (height == 0))
		this->setImageSize();

	// RESIZE
	if ((this->imageArea.w > 0) && (this->imageArea.h > 0))
	{
		width  = this->imageArea.w;
		height = this->imageArea.h;
	}
	else if ((width > 0) && (height > 0))
	{
		width  = (int)((float)width  * VM_Window::Display.scaleFactor);
		height = (int)((float)height * VM_Window::Display.scaleFactor);
	}

	// OPEN THE IMAGE IN A BACKGROUND THREAD
	#if defined _windows
		WString filePathKey = VM_Text::FormatW(L"%s_%dx%d", filePath.c_str(), width, height);
	#else
		String filePathKey = VM_Text::Format("%s_%dx%d", filePath.c_str(), width, height);
	#endif

	if ((VM_ThreadManager::Images[filePathKey] == NULL) ||
		!VM_ThreadManager::Images[filePathKey]->ready ||
		(VM_ThreadManager::Images[filePathKey]->width != width) ||
		(VM_ThreadManager::Images[filePathKey]->height != height))
	{
		int mediaID = 0;

		if (thumb && PICTURE_IS_SELECTED)
		{
			mediaID = (VM_ThreadManager::Images[filePathKey] != NULL ? VM_ThreadManager::Images[filePathKey]->id : 0);

			if (mediaID == 0)
				mediaID = (this->mediaID == 0 ? VM_GUI::ListTable->getSelectedMediaID() : this->mediaID);
		}

		if (VM_ThreadManager::Images[filePathKey] != NULL)
			VM_ThreadManager::Images[filePathKey]->update(thumb, width, height, mediaID);
		else
			VM_ThreadManager::Images[filePathKey] = new VM_Image(thumb, width, height, mediaID);
	}

	VM_ThreadManager::ImageButtons[this->id] = filePathKey;

	this->imageData         = new VM_Texture(VM_ThreadManager::ImagePlaceholder);
	this->imageData->width  = width;
	this->imageData->height = height;

	this->setImageTextAlign();

	return RESULT_OK;
}

int Graphics::VM_Button::setImage(VM_Image* image)
{
	if (image == NULL)
		return ERROR_UNKNOWN;

	this->imageArea = {};
	this->imageData = new VM_Texture(image);

	this->setImageTextAlign();

	return RESULT_OK;
}

void Graphics::VM_Button::setImageSize()
{
	String imageWidth  = VM_XML::GetAttribute(this->xmlNode, "image-width");
	String imageHeight = VM_XML::GetAttribute(this->xmlNode, "image-height");
	int    maxWidth    = (this->backgroundArea.w - this->borderWidth.left - this->borderWidth.right);
	int    maxHeight   = (this->backgroundArea.h - this->borderWidth.top  - this->borderWidth.bottom);

	if (imageWidth == "image-height")
		this->imageArea.w = this->getImageSize(imageHeight, maxHeight);
	else if (!imageWidth.empty())
		this->imageArea.w = this->getImageSize(imageWidth, maxWidth);
	else
		this->imageArea.w = maxWidth;

	if (imageHeight == "image-width")
		this->imageArea.h = this->getImageSize(imageWidth, maxWidth);
	else if (!imageHeight.empty())
		this->imageArea.h = this->getImageSize(imageHeight, maxHeight);
	else
		this->imageArea.h = maxHeight;
}

int Graphics::VM_Button::setImageTextAlign()
{
	// IMAGE AREA
	if (this->imageData != NULL)
	{
		this->imageArea.x = (this->backgroundArea.x + this->borderWidth.left + this->margin.left);
		this->imageArea.y = (this->backgroundArea.y + this->borderWidth.top  + this->margin.top);
		this->imageArea.w = this->imageData->width;
		this->imageArea.h = this->imageData->height;
	}

	// TEXT AREA
	if (this->textData != NULL)
	{
		this->textArea.x = (this->backgroundArea.x + this->borderWidth.left + this->margin.left);
		this->textArea.y = (this->backgroundArea.y + this->borderWidth.top  + this->margin.top);
		this->textArea.w = this->textData->width;
		this->textArea.h = this->textData->height;
	}

	String orientation = VM_XML::GetAttribute(this->xmlNode, "orientation");
	String hAlign      = VM_XML::GetAttribute(this->xmlNode, "halign");
	String vAlign      = VM_XML::GetAttribute(this->xmlNode, "valign");

	int maxX = (this->backgroundArea.w - this->borderWidth.left - this->borderWidth.right  - this->margin.left - this->margin.right);
	int maxY = (this->backgroundArea.h - this->borderWidth.top  - this->borderWidth.bottom - this->margin.top  - this->margin.bottom);

	// H-ALIGN
	if (hAlign == "center") {
		this->imageArea.x += ((maxX - this->imageArea.w) / 2);
		this->textArea.x  += ((maxX - this->textArea.w)  / 2);
	} else if (hAlign == "right"){
		this->imageArea.x += (maxX - this->imageArea.w);
		this->textArea.x  += (maxX - this->textArea.w);
	}

	// V-ALIGN
	if (vAlign == "middle") {
		this->imageArea.y += ((maxY - this->imageArea.h) / 2);
		this->textArea.y  += ((maxY - this->textArea.h)  / 2);
	} else if (vAlign == "bottom") {
		this->imageArea.y += (maxY - this->imageArea.h);
		this->textArea.y  += (maxY - this->textArea.h);
	}

	// ALIGN TEXT RELATIVE TO IMAGE
	if ((this->textData != NULL) && (this->imageData != NULL))
	{
		// VERTICAL
		if (orientation == "vertical") {
			this->textArea.y   = (this->imageArea.y + this->imageArea.h + DEFAULT_IMG_TXT_SPACING);
			this->imageArea.y -= (this->textArea.h / 2);
			this->textArea.y  -= (this->textArea.h / 2);
		// HORIZONTAL
		} else {
			this->textArea.x = (this->imageArea.x + this->imageArea.w + DEFAULT_IMG_TXT_SPACING);
		}
	}

	if (this->imageData != NULL) {
		this->imageArea.x = max(this->imageArea.x, this->backgroundArea.x);
		this->imageArea.y = max(this->imageArea.y, this->backgroundArea.y);
	}

	if (this->textData != NULL) {
		this->textArea.x = max(this->textArea.x, this->backgroundArea.x);
		this->textArea.y = max(this->textArea.y, this->backgroundArea.y);
	}

	return RESULT_OK;
}

void Graphics::VM_Button::setInputText(const String &text)
{
	this->inputText = text;
}

int Graphics::VM_Button::setText(const String &text)
{
	if (this->textData != NULL)
		DELETE_POINTER(this->textData);

	this->text = text;

	if (this->text.empty())
		this->text = this->getTextValue();

	if (this->text.empty())
		this->text = this->getText();

	if (this->text.empty())
		return ERROR_UNKNOWN;

	this->textColor = this->getColor("text");
	int maxWidth    = (this->backgroundArea.w - this->borderWidth.left - this->borderWidth.right - this->margin.left - this->margin.right);
	int wrap        = (VM_XML::GetAttribute(this->xmlNode, "wrap") == "true" ? maxWidth : 0);
	this->textData  = VM_Graphics::GetButtonLabel(this->text.c_str(), this->textColor, wrap, this->fontSize);

	if (this->textData == NULL)
		return ERROR_UNKNOWN;

	this->setImageTextAlign();

	return RESULT_OK;
}

void Graphics::VM_Button::setThumb(const String &tableID)
{
	this->setImage(std::to_string(this->mediaID), true);
}
