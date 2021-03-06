#include "VM_Table.h"

using namespace VoyaMedia::Database;
using namespace VoyaMedia::JSON;
using namespace VoyaMedia::MediaPlayer;
using namespace VoyaMedia::System;
using namespace VoyaMedia::XML;

Graphics::VM_Table::VM_Table(const String &id)
{
	this->init(id);
}

Graphics::VM_Table::VM_Table(const VM_Component &component)
{
	this->active          = component.active;
	this->activeColor     = component.activeColor;
	this->backgroundArea  = component.backgroundArea;
	this->backgroundColor = component.backgroundColor;
	this->borderColor     = component.borderColor;
	this->borderWidth     = component.borderWidth;
	this->fontSize        = component.fontSize;
	this->margin          = component.margin;
	this->parent          = component.parent;
	this->xmlDoc          = component.xmlDoc;
	this->xmlNode         = component.xmlNode;

	this->init(component.id);
}

Graphics::VM_Table::~VM_Table()
{
	DELETE_POINTER(this->playIcon);
	this->resetScroll(this->scrollBar);
	this->resetRows();
}

void Graphics::VM_Table::init(const String &id)
{
	this->id                    = id;
	this->maxRows               = 0;
	this->playIcon              = NULL;
	this->shouldRefreshRows     = false;
	this->shouldRefreshSelected = false;
	this->shouldRefreshThumbs   = false;
	this->scrollPane            = NULL;
	this->scrollBar             = NULL;
	this->states                = {};

	if (this->id == "list_table")
	{
		String settings;
		int    dbResult;
		auto   db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

		if (DB_RESULT_OK(dbResult))
		{
			for (int i = 0; i < NR_OF_MEDIA_TYPES; i++)
			{
				VM_MediaType mediaType    = (VM_MediaType)i;
				String       mediaTypeStr = std::to_string(mediaType);

				this->states[mediaType].offset        = std::atoi(db->getSettings("list_offset_" + mediaTypeStr).c_str());
				this->states[mediaType].scrollOffset  = std::atoi(db->getSettings("list_scroll_offset_" + mediaTypeStr).c_str());
				this->states[mediaType].searchString  = db->getSettings("list_search_text_" + mediaTypeStr);
				this->states[mediaType].selectedRow   = std::atoi(db->getSettings("list_selected_row_" + mediaTypeStr).c_str());
				this->states[mediaType].sortColumn    = db->getSettings("list_sort_column_" + mediaTypeStr);
				this->states[mediaType].sortDirection = db->getSettings("list_sort_direction_" + mediaTypeStr);

				if (this->states[mediaType].sortDirection.empty())
					this->states[mediaType].sortDirection = "ASC";
			}
		} else {
			this->states[VM_Top::Selected].reset();
		}

		DELETE_POINTER(db);
	} else {
		this->resetState();
	}
}

Graphics::VM_Button* Graphics::VM_Table::getButton(const String &buttonID)
{
	for (int row = 0; row < (int)this->rows.size(); row++) {
		for (int col = 0; col < (int)this->rows[row].size(); col++) {
			if ((this->rows[row][col] != NULL) && (this->rows[row][col]->id == buttonID))
				return this->rows[row][col];
		}
	}

	return NULL;
}

VM_DBResult Graphics::VM_Table::getResult()
{
	VM_DBResult result;

	for (int i = 0; i < this->limit; i++)
	{
		VM_DBRow row = {
			{"name", "" }, { "id", "" }, { "full_path", "" }
		};

		result.push_back(row);
	}

	this->states[VM_Top::Selected].dataRequested = true;

	return result;
}

VM_DBResult Graphics::VM_Table::getResultLimited(const VM_DBResult &result)
{
	this->maxRows = (int)result.size();

	int off = this->states[VM_Top::Selected].offset;
	int end = ((off + this->limit) > this->maxRows ? (this->maxRows - off) : this->limit);

	return VM_DBResult((result.begin() + off), (result.begin() + off + end));
}

int Graphics::VM_Table::getRowHeight()
{
	return (!this->buttons.empty() ? this->buttons[0]->backgroundArea.h : 1);
}

int Graphics::VM_Table::getRowsPerPage()
{
	int rowHeight = this->getRowHeight();
	return max(((this->backgroundArea.h - rowHeight) / max(rowHeight, 1)), 0);
}

String Graphics::VM_Table::getSearch()
{
	return VM_GUI::ListTable->states[VM_Top::Selected].searchString;
}

int Graphics::VM_Table::getSelectedMediaID()
{
	if (this->states[VM_Top::Selected].isValidSelectedRow(this->rows))
		return this->rows[this->states[VM_Top::Selected].selectedRow][0]->mediaID;

	return 0;
}

String Graphics::VM_Table::getSelectedMediaURL()
{
	if (this->shouldRefreshRows || this->shouldRefreshSelected || this->states[VM_Top::Selected].dataRequested || this->states[VM_Top::Selected].dataIsReady)
		return REFRESH_PENDING;

	String mediaURL = "";

	if (this->states[VM_Top::Selected].isValidSelectedRow(this->rows))
		mediaURL = this->rows[this->states[VM_Top::Selected].selectedRow][0]->mediaURL;

	if (mediaURL.empty())
		return REFRESH_PENDING;

	return mediaURL;
}

String Graphics::VM_Table::getSelectedFile()
{
	return VM_GUI::ListTable->getSelectedMediaURL();
}

Graphics::VM_Buttons Graphics::VM_Table::getSelectedRow()
{
	VM_Buttons row;

	if ((this->states[VM_Top::Selected].selectedRow >= 0) && (this->states[VM_Top::Selected].selectedRow < (int)this->rows.size()))
		row = this->rows[this->states[VM_Top::Selected].selectedRow];

	return row;
}

int Graphics::VM_Table::getSelectedRowIndex()
{
	return this->states[VM_Top::Selected].selectedRow;
}

String Graphics::VM_Table::getSort()
{
	String sort = "";

	if (this->states[VM_Top::Selected].sortColumn.empty() && (this->buttons.size() > 1))
		this->states[VM_Top::Selected].sortColumn = this->buttons[1]->id;

	if (this->states[VM_Top::Selected].sortColumn == "list_table_size")
		sort = "size";
	else if (this->states[VM_Top::Selected].sortColumn == "list_table_duration")
		sort = "duration";
	else if (this->states[VM_Top::Selected].sortColumn == "modal_playlists_search")
		sort = "search COLLATE NOCASE";
	else if (this->states[VM_Top::Selected].sortColumn == "modal_playlists_media_type")
		sort = "media_type COLLATE NOCASE";
	else if (this->states[VM_Top::Selected].sortColumn == "list_table_path")
		sort = "full_path COLLATE NOCASE";
	else
		sort = "name COLLATE NOCASE";

	return sort;
}

String Graphics::VM_Table::getSQL()
{
	String sql  = "";
	String sort = this->getSort();

	if (this->id == "list_table")
	{
		String  search      = VM_Text::EscapeSQL(VM_GUI::ListTable->getSearch(), true);
		Strings searchWords = VM_Text::Split(search, " ");

		sql = "SELECT name, id, full_path, path FROM MEDIA_FILES";
		sql.append(" WHERE media_type=" + std::to_string(VM_Top::Selected));
		
		for (const auto &word : searchWords)
			sql.append(" AND (full_path LIKE '%" + word + "%' OR name LIKE '%" + word + "%')");
	}
	else if (this->id == "modal_playlists_list_table")
	{
		sql = "SELECT name, search, id, name FROM PLAYLISTS";
	}
	else
	{
		return "";
	}

	sql.append(" ORDER BY " + sort + " " + this->states[VM_Top::Selected].sortDirection);
	sql.append(" LIMIT "    + std::to_string(this->limit));
	sql.append(" OFFSET "   + std::to_string(this->states[VM_Top::Selected].offset) + ";");

	return sql;
}

Graphics::VM_TableState Graphics::VM_Table::getState()
{
	return this->states[VM_Top::Selected];
}

VM_DBResult Graphics::VM_Table::getTracks(VM_MediaType mediaType)
{
	VM_DBResult      result;
	VM_DBRow         row;
	int              selected = VM_Player::GetStreamIndex(mediaType);
	std::vector<int> tracks   = VM_FileSystem::GetMediaStreamIndices(VM_Player::FormatContext, mediaType);

	Strings audioTracks, subTracks;
	int     audioIdx = 0;
	int     subIdx   = 0;

	// BLURAY / DVD: "concat:streamPath|stream1|...|streamN|duration|title|ja,ja,no,|N/A,fi,sv,|"
	if (VM_FileSystem::IsConcat(VM_Player::State.filePath))
	{
		Strings parts = VM_Text::Split(VM_Player::State.filePath.substr(7), "|");

		if (parts.size() >= 6) {
			audioTracks = VM_Text::Split(parts[parts.size() - 2], ",");
			subTracks   = VM_Text::Split(parts[parts.size() - 1], ",");
		}
	}

	for (auto track : tracks)
	{
		LIB_FFMPEG::AVStream* stream = VM_Player::FormatContext->streams[track];

		if (stream == NULL)
			continue;

		row.clear();

		if (stream->index == selected) {
			this->states[VM_Top::Selected].selectedRow  = (int)result.size();
			this->states[VM_Top::Selected].scrollOffset = max(min(((int)result.size() - this->getRowsPerPage()), this->states[VM_Top::Selected].selectedRow), 0);
		}

		if (mediaType == MEDIA_TYPE_AUDIO)
		{
			String streamDesc = VM_FileSystem::GetMediaStreamDescriptionAudio(stream);

			if (VM_FileSystem::IsConcat(VM_Player::State.filePath) && (audioIdx < (int)audioTracks.size()))
			{
				if (!streamDesc.empty())
					streamDesc.append(" (" + VM_Text::GetLanguage(audioTracks[audioIdx]) + ")");
				else
					streamDesc = VM_Text::GetLanguage(audioTracks[audioIdx]);
			}

			audioIdx++;

			row["name"] = streamDesc;
		}
		else if (mediaType == MEDIA_TYPE_SUBTITLE)
		{
			String streamDesc = VM_FileSystem::GetMediaStreamDescriptionSub(stream);

			if (VM_FileSystem::IsConcat(VM_Player::State.filePath) && (subIdx < (int)subTracks.size()))
			{
				if (!streamDesc.empty())
					streamDesc.append(" (" + VM_Text::GetLanguage(subTracks[subIdx]) + ")");
				else
					streamDesc = VM_Text::GetLanguage(subTracks[subIdx]);
			}

			subIdx++;

			row["name"] = streamDesc;
		}

		row["id"]        = std::to_string(stream->index);
		row["full_path"] = VM_Player::State.filePath;

		result.push_back(row);
	}

	if (mediaType != MEDIA_TYPE_SUBTITLE)
		return this->getResultLimited(result);

	for (const auto &file : VM_Player::SubsExternal)
	{
		LIB_FFMPEG::AVFormatContext* externalContext = VM_FileSystem::GetMediaFormatContext(file, true);

		if (externalContext == NULL)
			continue;

		for (int i = 0; i < (int)externalContext->nb_streams; i++)
		{
			LIB_FFMPEG::AVStream* stream = externalContext->streams[i];

			if (stream == NULL)
				continue;

			if (SUB_STREAM_EXTERNAL + stream->index == selected) {
				this->states[VM_Top::Selected].selectedRow  = (int)result.size();
				this->states[VM_Top::Selected].scrollOffset = max(min(((int)result.size() - this->getRowsPerPage()), this->states[VM_Top::Selected].selectedRow), 0);
			}

			row = {
				{ "name",      VM_FileSystem::GetMediaStreamDescriptionSub(stream, true) },
				{ "id",        std::to_string(SUB_STREAM_EXTERNAL + stream->index) },
				{ "full_path", VM_Player::State.filePath }
			};

			result.push_back(row);
		}

		FREE_AVFORMAT(externalContext);
	}

	if (selected == -1) {
		this->states[VM_Top::Selected].selectedRow  = (int)result.size();
		this->states[VM_Top::Selected].scrollOffset = max(((int)result.size() - this->getRowsPerPage()), 0);
	}

	row = {
		{ "name",     VM_Window::Labels["off"] },
		{ "id",       "-1" },
		{ "full_path", VM_Player::State.filePath }
	};

	result.push_back(row);

	return this->getResultLimited(result);
}

bool Graphics::VM_Table::isRowVisible()
{
	int scrollOffset = this->states[VM_Top::Selected].scrollOffset;
	int selectedRow  = this->states[VM_Top::Selected].selectedRow;
	int maxOffset    = (scrollOffset + this->getRowsPerPage() - 1);

	return ((selectedRow >= scrollOffset) && (selectedRow <= maxOffset));
}

bool Graphics::VM_Table::offsetEnd()
{
	int remainder = (this->maxRows % this->limit);
	int end       = (this->maxRows - (remainder > 0 ? remainder : this->limit));

	if (this->states[VM_Top::Selected].offset < end)
	{
		this->states[VM_Top::Selected].offset       = end;
		this->states[VM_Top::Selected].scrollOffset = 0;
		this->states[VM_Top::Selected].selectedRow  = 0;

		this->refreshRows();
		this->refreshSelected();

		return true;
	}

	return false;
}

bool Graphics::VM_Table::offsetNext()
{
	if (this->states[VM_Top::Selected].offset + this->limit < this->maxRows)
	{
		this->states[VM_Top::Selected].offset      += this->limit;
		this->states[VM_Top::Selected].scrollOffset = 0;
		this->states[VM_Top::Selected].selectedRow  = 0;

		this->refreshRows();
		this->refreshSelected();

		return true;
	}

	return false;
}

bool Graphics::VM_Table::offsetPrev()
{
	if (this->states[VM_Top::Selected].offset >= this->limit)
	{
		this->states[VM_Top::Selected].offset      -= this->limit;
		this->states[VM_Top::Selected].scrollOffset = 0;
		this->states[VM_Top::Selected].selectedRow  = 0;

		this->refreshRows();
		this->refreshSelected();

		return true;
	}

	return false;
}

bool Graphics::VM_Table::offsetStart()
{

	if (this->states[VM_Top::Selected].offset >= this->limit)
	{
		this->states[VM_Top::Selected].offset       = 0;
		this->states[VM_Top::Selected].scrollOffset = 0;
		this->states[VM_Top::Selected].selectedRow  = 0;

		this->refreshRows();
		this->refreshSelected();

		return true;
	}

	return false;
}

int Graphics::VM_Table::refresh()
{
	if (this->shouldRefreshRows)
	{
		VM_ThreadManager::FreeThumbnails();
		this->setRows();

		this->shouldRefreshRows = false;
	}

	if (this->shouldRefreshSelected)
	{
		VM_Button* fileButton = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_controls_file"]);
		VM_Button* snapshot   = dynamic_cast<VM_Button*>(VM_GUI::Components["bottom_player_snapshot"]);

		// SNAPSHOT IMAGE
		if (snapshot != NULL)
			snapshot->removeImage();

		// SELECTED FILE TEXT
		if (fileButton != NULL)
			fileButton->setText("");

		// SELECTED PLAY ICON
		DELETE_POINTER(this->playIcon);

		this->shouldRefreshSelected = false;
	}

	if (!this->visible)
		return ERROR_UNKNOWN;

	if (this->shouldRefreshThumbs)
	{
		for (const auto &row : this->rows) {
			if (!row.empty())
				row[0]->setThumb(this->id);
		}

		this->shouldRefreshThumbs = false;
	}

	return RESULT_OK;
}

void Graphics::VM_Table::refreshRows()
{
	this->shouldRefreshRows = true;
}

void Graphics::VM_Table::refreshSelected()
{
	this->shouldRefreshSelected = true;
}

void Graphics::VM_Table::refreshThumbs()
{
	this->shouldRefreshThumbs = this->thumbThreads.empty();
}

void Graphics::VM_Table::removeThumbThread()
{
	if (!this->thumbThreads.empty())
		this->thumbThreads.pop();
}

int Graphics::VM_Table::render()
{
	if (!this->visible)
		return ERROR_INVALID_ARGUMENTS;

	// SCROLL PANE TEXTURE
	if (this->scrollPane == NULL)
		this->resetScrollPane();

	if (this->scrollPane == nullptr)
		return ERROR_UNKNOWN;

	// RENDER TO SCROLL PANE TEXTURE
	SDL_SetRenderTarget(VM_Window::Renderer, this->scrollPane->data);

	SDL_Rect backgroundArea = { 0, 0, this->scrollPane->width, this->scrollPane->height };
	VM_Graphics::FillArea(this->backgroundColor, backgroundArea);

	for (int row = 0; row < (int)this->rows.size();      row++) {
	for (int col = 0; col < (int)this->rows[row].size(); col++)
	{
		this->rows[row][col]->selected = (row == this->states[VM_Top::Selected].selectedRow);
		this->rows[row][col]->render();

		if ((this->id == "list_table") && (col == 0) && this->rows[row][col]->selected && !this->rows[row][1]->getText().empty())
		{
			if (this->playIcon == NULL)
			{
				this->playIcon     = new VM_Button(*dynamic_cast<VM_Component*>(this->rows[row][0]));
				this->playIcon->id = "list_table_play_icon";

				this->playIcon->backgroundColor = { 0, 0, 0, 0xA0 };

				VM_ThreadManager::Mutex.lock();

				const int height = (int)((float)this->playIcon->backgroundArea.h / VM_Window::Display.scaleFactor * 0.3f);
				this->playIcon->setImage("play-3-64.png", false, height, height);

				VM_ThreadManager::Mutex.unlock();
			}

			if (this->playIcon != NULL)
				this->playIcon->render();
		}
	}}

	SDL_SetRenderTarget(VM_Window::Renderer, NULL);

	// RENDER TO SCREEN
	if (!this->rows.empty() && !this->rows[0][1]->getText().empty())
	{
		SDL_Rect dest = {
			this->backgroundArea.x,
			(this->backgroundArea.y + this->borderWidth.top),
			this->backgroundArea.w,
			(this->backgroundArea.h - this->borderWidth.top - this->borderWidth.bottom)
		};

		SDL_Rect clip        = SDL_Rect(dest);
		int      offset      = this->states[VM_Top::Selected].scrollOffset;
		int      rowsPerPage = this->getRowsPerPage();

		clip.y += (this->getRowHeight() * offset);

		SDL_RenderCopy(VM_Window::Renderer, this->scrollPane->data, &clip, &dest);
	}

	for (auto button : this->buttons) {
		button->borderWidth = {};
		button->render();
	}

	VM_Graphics::FillBorder(this->borderColor, this->backgroundArea, this->borderWidth);

	return RESULT_OK;
}

void Graphics::VM_Table::resetScroll(VM_Component* scrollBar)
{
	DELETE_POINTER(this->scrollPane);
	this->init(this->id);
	this->scrollBar = scrollBar;
}

void Graphics::VM_Table::resetScrollPane()
{
	if (!this->shouldRefreshRows)
	{
		int textureHeight = (
			VM_Window::Dimensions.h - this->backgroundArea.h + 
			(this->getRowHeight() * ((int)this->rows.size() + 1))
		);

		this->scrollPane = new VM_Texture(
			SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET,
			VM_Window::Dimensions.w, max(VM_Window::Dimensions.h, textureHeight)
		);
	}
}

void Graphics::VM_Table::resetRows()
{
	for (auto &row : this->rows)
	{
		for (auto col : row)
			DELETE_POINTER(col);

		row.clear();
	}

	this->rows.clear();
}

int Graphics::VM_Table::resetState(bool resetDataRequest)
{
	this->states[VM_Top::Selected].reset(resetDataRequest);

	if (this->id == "list_table")
	{
		int  dbResult;
		auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

		if (DB_RESULT_OK(dbResult))
		{
			VM_TableState state    = this->getState();
			String        selected = std::to_string(VM_Top::Selected);

			db->updateSettings(("list_offset_"         + selected), std::to_string(state.offset));
			db->updateSettings(("list_scroll_offset_"  + selected), std::to_string(state.scrollOffset));
			db->updateSettings(("list_selected_row_"   + selected), std::to_string(state.selectedRow));
			db->updateSettings(("list_sort_column_"    + selected), state.sortColumn);
			db->updateSettings(("list_sort_direction_" + selected), state.sortDirection);
		}

		DELETE_POINTER(db);
	}

	return RESULT_OK;
}

void Graphics::VM_Table::restoreState(const VM_TableState &state)
{
	this->states[VM_Top::Selected].restore(state);
	VM_GUI::ListTable->setSearch(state.searchString);

	this->refreshRows();
	this->refreshSelected();
}

int Graphics::VM_Table::scroll(int offset)
{
	bool next          = (offset > 0);
	bool prev          = (offset < 0);
	int  rowsPerPage   = this->getRowsPerPage();;
	int  currentOffset = this->states[VM_Top::Selected].scrollOffset;
	int  nextOffset    = (currentOffset + offset);
	bool validNext     = (nextOffset <= (int)this->rows.size() - rowsPerPage);
	bool validPrev     = (nextOffset >= 0);

	if ((next && validNext) || (prev && validPrev))
		this->states[VM_Top::Selected].scrollOffset = nextOffset;
	else if (next && !validNext)
		this->states[VM_Top::Selected].scrollOffset = ((int)this->rows.size() - rowsPerPage);
	else if (prev && !validPrev)
		this->states[VM_Top::Selected].scrollOffset = 0;

	return RESULT_OK;
}

void Graphics::VM_Table::scrollTo(int row)
{
	this->scroll(row - this->states[VM_Top::Selected].scrollOffset);
}

void Graphics::VM_Table::selectNext(bool loop)
{
	bool lastRow = (this->states[VM_Top::Selected].selectedRow == (int)this->rows.size() - 1);
	bool refresh = true;

	if ((this->states[VM_Top::Selected].offset + this->limit < this->maxRows) && lastRow)
	{
		this->states[VM_Top::Selected].offset      += this->limit;
		this->states[VM_Top::Selected].scrollOffset = 0;
		this->states[VM_Top::Selected].selectedRow  = 0;
	}
	else if (loop && lastRow)
	{
		this->states[VM_Top::Selected].offset       = 0;
		this->states[VM_Top::Selected].scrollOffset = 0;
		this->states[VM_Top::Selected].selectedRow  = 0;
	}
	else if (this->states[VM_Top::Selected].selectedRow + 1 < (int)this->rows.size())
	{
		if (this->states[VM_Top::Selected].scrollOffset + 1 <= ((int)this->rows.size() - this->getRowsPerPage()))
			this->states[VM_Top::Selected].scrollOffset++;

		this->selectRow(this->states[VM_Top::Selected].selectedRow + 1);
	} else {
		refresh = false;
	}

	if (refresh) {
		this->refreshRows();
		this->refreshSelected();
	}
}

void Graphics::VM_Table::selectPrev(bool loop)
{
	bool refresh = true;

	if ((this->states[VM_Top::Selected].offset >= this->limit) && (this->states[VM_Top::Selected].selectedRow == 0))
	{
		this->states[VM_Top::Selected].offset      -= this->limit;
		this->states[VM_Top::Selected].scrollOffset = (this->limit - this->getRowsPerPage());
		this->states[VM_Top::Selected].selectedRow  = (this->limit - 1);
	}
	else if (loop && (this->states[VM_Top::Selected].selectedRow == 0))
	{
		this->states[VM_Top::Selected].offset       = (((this->maxRows - 1) / this->limit) * this->limit);
		this->states[VM_Top::Selected].scrollOffset = max(((this->maxRows % this->limit) - this->getRowsPerPage()), 0);
		this->states[VM_Top::Selected].selectedRow  = max(((this->maxRows % this->limit) - 1), 0);
	}
	else if (this->states[VM_Top::Selected].selectedRow - 1 >= 0)
	{
		if (this->states[VM_Top::Selected].scrollOffset - 1 >= 0)
			this->states[VM_Top::Selected].scrollOffset--;

		this->selectRow(this->states[VM_Top::Selected].selectedRow - 1);
	} else {
		refresh = false;
	}

	if (refresh) {
		this->refreshRows();
		this->refreshSelected();
	}
}

void Graphics::VM_Table::selectRandom()
{
	int next = (rand() % this->maxRows);

	this->states[VM_Top::Selected].offset       = (((next - 1) / this->limit) * this->limit);
	this->states[VM_Top::Selected].selectedRow  = max(((next % this->limit) - 1), 0);
	this->states[VM_Top::Selected].scrollOffset = max(min((this->limit - this->getRowsPerPage()), this->states[VM_Top::Selected].selectedRow), 0);

	this->refreshRows();
	this->refreshSelected();
}

void Graphics::VM_Table::selectRow(int row)
{
	if (row < 0)
		row = 0;
	else if (row >= (int)this->rows.size())
		row = ((int)this->rows.size() - 1);

	this->states[VM_Top::Selected].selectedRow = row;

	this->refreshSelected();

	if (this->id == "list_table")
	{
		int  dbResult;
		auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

		if (DB_RESULT_OK(dbResult))
		{
			VM_TableState state    = this->getState();
			String        selected = std::to_string(VM_Top::Selected);

			db->updateSettings(("list_offset_"        + selected), std::to_string(state.offset));
			db->updateSettings(("list_scroll_offset_" + selected), std::to_string(state.scrollOffset));
			db->updateSettings(("list_selected_row_"  + selected), std::to_string(state.selectedRow));
		}

		DELETE_POINTER(db);
	}
}

bool Graphics::VM_Table::selectRow(SDL_Event* mouseEvent)
{
	if ((mouseEvent == nullptr) || !VM_Graphics::ButtonPressed(mouseEvent, this->backgroundArea))
		return false;

	// CHECK IF THE SELECTED ROW IS CLICKABLE (VISIBLE WITHIN SCROLLED AREA)
	SDL_Point position = VM_EventManager::GetMousePosition(mouseEvent);

	int offset    = this->states[VM_Top::Selected].scrollOffset;
	int rowHeight = this->getRowHeight();
	int startY    = (this->backgroundArea.y + rowHeight);
	int offsetY   = (rowHeight * offset);
	int row       = ((position.y + offsetY - startY) / rowHeight);

	if ((row < 0) || (row >= (int)this->rows.size()))
		return false;

	this->selectRow(row);

	// CHECK IF THE SELECTED ROW IS VALID (PART OF TOTAL ROWS)
	if (this->rows[row].size() < 3)
		return false;

	if ((this->id != "list_table") || VM_Modal::IsVisible())
		return true;

	VM_Button* thumb     = this->rows[row][0];
	VM_Button* info      = this->rows[row][this->rows[row].size() - 1];
	SDL_Rect   areaThumb = SDL_Rect(thumb->backgroundArea);
	SDL_Rect   areaInfo  = SDL_Rect(info->backgroundArea);

	areaThumb.y -= offsetY;
	areaInfo.y  -= offsetY;

	// SINGLE-CLICKED INFO/DETAILS ICON
	if (info->selected && VM_Graphics::ButtonPressed(mouseEvent, areaInfo))
	{
		VM_Modal::Open(VM_XML::GetAttribute(info->xmlNode, "modal"));
	}
	// SINGLE-CLICKED PLAY/THUMB ICON
	else if (thumb->selected && VM_Graphics::ButtonPressed(mouseEvent, areaThumb))
	{
		VM_Player::OpenFilePath(thumb->mediaURL);
	}
	else
	{
		for (auto button : this->rows[row])
		{
			SDL_Rect area = SDL_Rect(button->backgroundArea);
			area.y       -= offsetY;

			// DOUBLE-CLICKED ROW
			if (VM_Graphics::ButtonPressed(mouseEvent, area, false, true)) {
				VM_Player::OpenFilePath(thumb->mediaURL);
				break;
			// RIGHT-CLICKED ROW
			} else if (VM_Graphics::ButtonPressed(mouseEvent, area, true, false)) {
				VM_Modal::Open("modal_right_click");
				break;
			}
		}
	}

	return true;
}

void Graphics::VM_Table::setData()
{
	this->resultMutex.lock();

	// DOWNLOAD/FETCH DATA
	if (this->id == "modal_player_settings_audio_list_table")
		this->result = this->getTracks(MEDIA_TYPE_AUDIO);
	else if (this->id == "modal_player_settings_subs_list_table")
		this->result = this->getTracks(MEDIA_TYPE_SUBTITLE);
	else if ((this->id == "list_table") || (this->id == "modal_playlists_list_table"))
	{
		int          dbResult = SQLITE_ERROR;
		VM_Database* db       = NULL;

		if (this->id == "list_table")
			db = new VM_Database(dbResult, DATABASE_MEDIALIBRARYv3);
		else if (this->id == "modal_playlists_list_table")
			db = new VM_Database(dbResult, DATABASE_PLAYLISTSv3);

		if (DB_RESULT_OK(dbResult))
		{
			String sql = this->getSQL();

			if (!sql.empty()) {
				this->result  = db->getRows(sql);
				this->maxRows = db->getRowCount(sql);
			}
		}

		DELETE_POINTER(db);
	}

	while (!this->thumbThreads.empty())
		this->thumbThreads.pop();

	// DOWNLOAD/CREATE THUMBS
	for (auto &row : this->result)
	{
		String thumbFile = "";

		if ((this->id == "list_table") || (this->id == "modal_playlists_list_table"))
			thumbFile = row["id"];

		if (thumbFile.empty())
			continue;

		#if defined _windows
		WString thumbPath = VM_FileSystem::GetPathThumbnailsW(VM_Text::ToUTF16(thumbFile.c_str()));

		if (!VM_FileSystem::FileExists("", thumbPath))
		#else
		String thumbPath = VM_FileSystem::GetPathThumbnails(thumbFile);

		if (!VM_FileSystem::FileExists(thumbPath, L""))
		#endif
		{
			VM_ThreadData* threadData = new VM_ThreadData();

			threadData->data = row;

			#if defined _windows
				threadData->dataW["thumb_path"] = thumbPath;
			#else
				threadData->data["thumb_path"] = thumbPath;
			#endif

			std::thread thumbThread(VM_Graphics::CreateThumbThread, threadData);

			this->thumbThreads.push(thumbThread.get_id());

			thumbThread.detach();
		}
	}

	this->states[VM_Top::Selected].dataIsReady   = true;
	this->states[VM_Top::Selected].dataRequested = false;

	this->resultMutex.unlock();
}

void Graphics::VM_Table::setHeader()
{
	for (auto component : this->buttons)
		dynamic_cast<VM_Button*>(component)->backgroundColor = this->getColor("header");
}

void Graphics::VM_Table::setRows()
{
	this->resultMutex.lock();
	this->result = this->getResult();
	this->resultMutex.unlock();

	this->setRows(true);
}

int Graphics::VM_Table::setRows(bool temp)
{
	this->resetRows();

	int      offsetY   = 0;
	VM_Color row1Color = this->getColor("row1");
	VM_Color row2Color = this->getColor("row2");

	// HEADER ROW
	for (size_t col = 0; col < this->buttons.size(); col++)
	{
		if (!VM_Player::State.isStopped || (VM_Modal::IsVisible() && (VM_Modal::ListTable == NULL)))
			break;

		VM_Button* headerColumn = dynamic_cast<VM_Button*>(this->buttons[col]);

		if (headerColumn->imageData != NULL)
			headerColumn->removeImage();

		headerColumn->setText("");

		if (!temp &&
			!this->result.empty() &&
			(headerColumn->id == this->states[VM_Top::Selected].sortColumn) &&
			((this->id == "list_table") || (this->id == "modal_playlists_list_table")))
		{
			String image = (this->states[VM_Top::Selected].sortDirection == "ASC" ? "triangle-4-64.png" : "triangle-3-64.png");
			headerColumn->setImage(image, false, 8, 8);
		}

		if (col > 0)
			headerColumn->margin.left = 10;
	}

	this->resultMutex.lock();

	// FILE ROWS
	for (size_t row = 0; row < this->result.size(); row++)
	{
		offsetY += this->buttons[0]->backgroundArea.h;

		VM_Buttons rowColumns;

		for (size_t col = 0; col < this->buttons.size(); col++)
		{
			VM_Button* rowColumn = new VM_Button(*this->buttons[col]);

			rowColumn->backgroundArea.y += offsetY;
			rowColumn->backgroundColor   = (row % 2 == 0 ? row1Color : row2Color);
			rowColumn->borderColor       = { 0x10, 0x10, 0x10, 0xFF };
			rowColumn->borderWidth       = VM_Border(2, 0, 0, 0);
			rowColumn->id               += ("_" + std::to_string(row) + "_" + std::to_string(col));
			rowColumn->highlightColor    = this->getColor("highlight");
			rowColumn->mediaID           = std::atoi(this->result[row]["id"].c_str());
			rowColumn->mediaURL          = this->result[row]["full_path"];
			rowColumn->parent            = this;

			// FIRST COLUMN - THUMB
			if ((col == 0) && (this->id == "list_table"))
			{
				rowColumn->borderWidth     = VM_Border(2);
				rowColumn->borderColor     = rowColumn->backgroundColor;
				rowColumn->backgroundColor = VM_Color(SDL_COLOR_BLACK);
				rowColumn->highlightColor  = rowColumn->backgroundColor;

				if (!temp && this->thumbThreads.empty())
					rowColumn->setThumb(this->id);
			}
			// LAST COLUMN - ABOUT/INFO
			else if ((col == this->buttons.size() - 1) && !temp)
			{
				rowColumn->setImage((VM_GUI::ColorThemeFile == "dark" ? "ellipsis-vertical-1-512.png" : "ellipsis-vertical-2-512.png"), false);
			}
			// REMAINING COLUMNS
			else if ((col == 1) && !temp)
			{
				rowColumn->setText(this->result[row]["name"]);
			}
			else if ((col == 2) && !temp)
			{
				if (this->id == "modal_playlists_list_table")
					rowColumn->setText(this->result[row]["search"]);
				else
					rowColumn->setText(this->result[row]["path"]);
			}

			rowColumns.push_back(rowColumn);
		}

		this->rows.push_back(rowColumns);
	}

	this->resultMutex.unlock();

	int row = this->states[VM_Top::Selected].selectedRow;

	if (!temp)
		this->scrollTo(this->states[VM_Top::Selected].scrollOffset);

	this->selectRow(row);

	if (((int)this->rows.size() < this->limit) && (this->maxRows > (this->states[VM_Top::Selected].offset + (int)this->rows.size())))
		this->maxRows = (this->states[VM_Top::Selected].offset + (int)this->rows.size());

	if ((this->id == "list_table") && !temp && (this->maxRows > 0) && (this->states[VM_Top::Selected].offset >= this->maxRows))
	{
		this->resetState(false);
		this->refreshRows();

		return ERROR_UNKNOWN;
	}

	this->updateDetailsText(temp);

	if (!temp) {
		this->updateNavigation();

		this->states[VM_Top::Selected].dataIsReady = false;
	}

	return RESULT_OK;
}

void Graphics::VM_Table::setSearch(const String &searchString, bool saveDB)
{
	VM_GUI::ListTable->states[VM_Top::Selected].searchString = (searchString != "N/A" ? searchString : "");
	VM_GUI::ListTable->updateSearchInput();

	if (saveDB)
	{
		if (!VM_TextInput::IsActive())
		{
			VM_Button* searchInput = dynamic_cast<VM_Button*>(VM_GUI::Components["middle_search_input"]);

			if (searchInput != NULL)
				VM_TextInput::SetActive(true, searchInput);
		}

		VM_TextInput::SaveToDB();
		VM_TextInput::SetActive(false);

		if (VM_GUI::ListTable != NULL)
			VM_GUI::ListTable->resetState();
	}
}

void Graphics::VM_Table::sort(const String &buttonID)
{
	if (this->states[VM_Top::Selected].sortColumn != buttonID)
		this->states[VM_Top::Selected].sortDirection = "ASC";
	else if (this->states[VM_Top::Selected].sortDirection == "ASC")
		this->states[VM_Top::Selected].sortDirection = "DESC";
	else
		this->states[VM_Top::Selected].sortDirection = "ASC";

	this->states[VM_Top::Selected].sortColumn = buttonID;

	this->refreshRows();

	if (this->id == "list_table")
	{
		int  dbResult;
		auto db = new VM_Database(dbResult, DATABASE_SETTINGSv3);

		if (DB_RESULT_OK(dbResult))
		{
			VM_TableState state    = this->getState();
			String        selected = std::to_string(VM_Top::Selected);

			db->updateSettings(("list_sort_column_"    + selected), state.sortColumn);
			db->updateSettings(("list_sort_direction_" + selected), state.sortDirection);
		}

		DELETE_POINTER(db);
	}
}

void Graphics::VM_Table::updateDetailsText(bool temp)
{
	VM_Button* button;
	String     detailsText;

	if (temp)
		detailsText = "Loading ...";
	else if ((int)this->rows.size() < this->maxRows)
		detailsText = VM_Text::Format("%d - %d / %d", (this->states[VM_Top::Selected].offset + 1), (this->states[VM_Top::Selected].offset + (int)this->rows.size()), this->maxRows);
	else if (!this->rows.empty())
		detailsText = VM_Text::Format("%d", (int)this->rows.size());
	else
		detailsText = "0";

	if (this->id == "list_table")
		button = dynamic_cast<VM_Button*>(VM_GUI::Components["list_details_text"]);
	else
		button = dynamic_cast<VM_Button*>(VM_Modal::Components["list_details_text"]);

	if (button != NULL)
		button->setText(detailsText);
}

void Graphics::VM_Table::updateNavigation()
{
	VM_Button* button1 = dynamic_cast<VM_Button*>(this->id == "list_table" ? VM_GUI::Components["list_offset_start"] : VM_Modal::Components["list_offset_start"]);
	VM_Button* button2 = dynamic_cast<VM_Button*>(this->id == "list_table" ? VM_GUI::Components["list_offset_prev"]  : VM_Modal::Components["list_offset_prev"]);
	bool       enabled = (this->states[VM_Top::Selected].offset >= this->limit);

	if ((button1 != NULL) && (button2 != NULL)) {
		button1->overlayColor   = VM_Color(button1->backgroundColor);
		button1->overlayColor.a = (enabled ? 0 : 0xA0);
		button2->overlayColor   = button1->overlayColor;
	}

	button1 = dynamic_cast<VM_Button*>(this->id == "list_table" ? VM_GUI::Components["list_offset_end"]  : VM_Modal::Components["list_offset_end"]);
	button2 = dynamic_cast<VM_Button*>(this->id == "list_table" ? VM_GUI::Components["list_offset_next"] : VM_Modal::Components["list_offset_next"]);
	enabled = (this->states[VM_Top::Selected].offset < this->maxRows - this->limit);

	if ((button1 != NULL) && (button2 != NULL)) {
		button1->overlayColor   = VM_Color(button1->backgroundColor);
		button1->overlayColor.a = (enabled ? 0 : 0xA0);
		button2->overlayColor   = button1->overlayColor;
	}
}

void Graphics::VM_Table::updateSearchInput()
{
	VM_Button* searchInput = dynamic_cast<VM_Button*>(VM_GUI::Components["middle_search_input"]);

	if (searchInput != NULL)
		searchInput->setInputText(VM_GUI::ListTable->getSearch());
}
