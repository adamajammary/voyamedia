#include "VM_TextInput.h"

using namespace VoyaMedia::Database;
using namespace VoyaMedia::System;

bool                 Graphics::VM_TextInput::active         = false;
Graphics::VM_Button* Graphics::VM_TextInput::activeButton   = NULL;
int                  Graphics::VM_TextInput::cursor         = 0;
bool                 Graphics::VM_TextInput::refreshPending = false;
String               Graphics::VM_TextInput::text           = "";

void Graphics::VM_TextInput::Backspace()
{
	if (VM_TextInput::active && (VM_TextInput::cursor > 0))
	{
		String text1 = VM_TextInput::text.substr(0, VM_TextInput::cursor - 1);
		String text2 = VM_TextInput::text.substr(VM_TextInput::cursor);

		VM_TextInput::text = (text1 + text2);
		VM_TextInput::cursor--;

		VM_TextInput::Refresh();
	}
}

void Graphics::VM_TextInput::Clear(VM_Button* input)
{
	VM_TextInput::text = "";

	if (input != NULL)
	{
		input->setText("");
		input->setInputText("");

		if (!VM_TextInput::active)
			VM_TextInput::SetActive(true, input);
	}
}

void Graphics::VM_TextInput::Delete()
{
	if (VM_TextInput::active && (VM_TextInput::cursor < (int)VM_TextInput::text.size()))
	{
		String text1 = VM_TextInput::text.substr(0, VM_TextInput::cursor);
		String text2 = VM_TextInput::text.substr(VM_TextInput::cursor + 1);

		VM_TextInput::text = (text1 + text2);

		VM_TextInput::Refresh();
	}
}

void Graphics::VM_TextInput::End()
{
	if (VM_TextInput::active) {
		VM_TextInput::cursor = (int)VM_TextInput::text.size();
		VM_TextInput::Refresh();
	}
}

String Graphics::VM_TextInput::GetText()
{
	return VM_TextInput::text;
}

void Graphics::VM_TextInput::Home()
{
	if (VM_TextInput::active) {
		VM_TextInput::cursor = 0;
		VM_TextInput::Refresh();
	}
}

bool Graphics::VM_TextInput::IsActive()
{
	return VM_TextInput::active;
}

void Graphics::VM_TextInput::Left()
{
	if (VM_TextInput::active && (VM_TextInput::cursor - 1 >= 0))
		VM_TextInput::cursor--;

	VM_TextInput::Refresh();
}

void Graphics::VM_TextInput::Paste()
{
	if (SDL_HasClipboardText())
		VM_TextInput::Update(SDL_GetClipboardText());
}

void Graphics::VM_TextInput::Refresh()
{
	VM_TextInput::refreshPending = true;
}

void Graphics::VM_TextInput::Right()
{
	if (VM_TextInput::active && (VM_TextInput::cursor + 1 <= (int)VM_TextInput::text.size()))
		VM_TextInput::cursor++;

	VM_TextInput::Refresh();
}

void Graphics::VM_TextInput::SaveToDB()
{
	if ((VM_TextInput::activeButton != NULL) && (VM_TextInput::activeButton->id == "middle_search_input"))
	{
		int    dbResult;
		auto   db   = new VM_Database(dbResult, DATABASE_SETTINGSv3);
		String text = VM_Text::Trim(VM_TextInput::activeButton->getInputText());

		if (DB_RESULT_OK(dbResult))
			db->updateSettings(("list_search_text_" + std::to_string(VM_Top::Selected)), text);

		DELETE_POINTER(db);

		VM_GUI::ListTable->setSearch(text);
	}
}

int Graphics::VM_TextInput::SetActive(bool active, VM_Button* button)
{
	VM_TextInput::active = active;

	if (VM_TextInput::active)
	{
		VM_TextInput::activeButton = button;

		if (VM_TextInput::activeButton != NULL)
		{
			SDL_StartTextInput();
			SDL_SetTextInputRect(&VM_TextInput::activeButton->backgroundArea);

			VM_TextInput::activeButton->active = true;
			VM_TextInput::text                 = VM_TextInput::activeButton->getInputText();
		}
	}
	else
	{
		SDL_StopTextInput();
		SDL_SetTextInputRect(NULL);

		if (VM_TextInput::activeButton != NULL)
		{
			VM_TextInput::activeButton->active = false;
			VM_TextInput::activeButton->setText(VM_TextInput::text);

			VM_TextInput::activeButton = NULL;
			VM_TextInput::cursor       = 0;
			VM_TextInput::text         = "";
		}
	}

	VM_TextInput::End();

	return RESULT_OK;
}

int Graphics::VM_TextInput::Unfocus()
{
	VM_TextInput::active = false;

	SDL_StopTextInput();
	SDL_SetTextInputRect(NULL);

	if (VM_TextInput::activeButton != NULL) {
		VM_TextInput::activeButton->active = false;
		VM_TextInput::activeButton->setText(VM_TextInput::text);
	}

	VM_TextInput::End();

	return RESULT_OK;
}

void Graphics::VM_TextInput::Update(const char* text)
{
	if (VM_TextInput::active && (text != NULL))
	{
		String text1 = VM_TextInput::text.substr(0, VM_TextInput::cursor);
		String text2 = VM_TextInput::text.substr(VM_TextInput::cursor);

		VM_TextInput::text = (text1 + text + text2);

		VM_TextInput::cursor += (int)strlen(text);
	}

	VM_TextInput::Refresh();
}

void Graphics::VM_TextInput::Update()
{
	if (VM_TextInput::refreshPending && (VM_TextInput::activeButton != NULL))
	{
		String text1 = VM_TextInput::text.substr(0, VM_TextInput::cursor);
		String text2 = VM_TextInput::text.substr(VM_TextInput::cursor);

		VM_TextInput::activeButton->setInputText(text1 + text2);

		if (VM_TextInput::activeButton->active)
			VM_TextInput::activeButton->setText(text1 + "|" + text2);

		VM_TextInput::refreshPending = false;
	}
}
