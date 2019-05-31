#include "VM_Table.h"

using namespace VoyaMedia::Database;
using namespace VoyaMedia::JSON;
using namespace VoyaMedia::MediaPlayer;
using namespace VoyaMedia::System;
using namespace VoyaMedia::UPNP;
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
	this->limit                 = 10;
	this->maxRows               = 0;
	this->pageTokenNext         = "";
	this->pageTokenPrev         = "";
	this->playIcon              = NULL;
	this->shouldRefreshRows     = false;
	this->shouldRefreshSelected = false;
	this->shouldRefreshThumbs   = false;
	this->scrollDrag            = false;
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

				this->states[mediaType].pageToken     = db->getSettings("list_page_token_" + mediaTypeStr);
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

VM_DBResult Graphics::VM_Table::getNICs()
{
	VM_DBResult result;
	Strings     nics = VM_FileSystem::GetNetworkInterfaces();

	for (const auto &nic : nics) {
		VM_DBRow row = { { "name", (nic + "/24") }, { "id", "0" }, { "full_path", nic } };
		result.push_back(row);
	}

	return result;
}

VM_DBResult Graphics::VM_Table::getResult()
{
	VM_DBResult result;

	if (this->id == "modal_upnp_list_table")
	{
		result = this->getNICs();

		this->states[VM_Top::Selected].dataIsReady   = true;
		this->states[VM_Top::Selected].dataRequested = false;
	}
	else
	{
		for (int i = 0; i < this->limit; i++)
		{
			VM_DBRow row = {
				{"name", "" }, { "id", "" },
				{ (VM_Top::Selected >= MEDIA_TYPE_YOUTUBE ? "thumb_url" : "full_path"), "" }
			};

			result.push_back(row);
		}

		this->states[VM_Top::Selected].dataRequested = true;
	}

	return result;
}

int Graphics::VM_Table::getRowsPerPage()
{
	int rowHeight = (!this->buttons.empty() ? this->buttons[0]->backgroundArea.h : 1);

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
	if (YOUTUBE_IS_SELECTED)
		return VM_GUI::ListTable->getSelectedYouTube();
	else if (SHOUTCAST_IS_SELECTED)
		return VM_GUI::ListTable->getSelectedShoutCast();

	return VM_GUI::ListTable->getSelectedMediaURL();
}

String Graphics::VM_Table::getSelectedShoutCast()
{
	if (this->shouldRefreshRows || this->shouldRefreshSelected || this->states[VM_Top::Selected].dataRequested || this->states[VM_Top::Selected].dataIsReady)
		return REFRESH_PENDING;

	String mediaID = "";

	if (this->states[VM_Top::Selected].isValidSelectedRow(this->rows))
		mediaID = VM_FileSystem::GetShoutCastStation(this->rows[this->states[VM_Top::Selected].selectedRow][0]->mediaID);

	return mediaID;
}

String Graphics::VM_Table::getSelectedYouTube()
{
	if (this->shouldRefreshRows || this->shouldRefreshSelected || this->states[VM_Top::Selected].dataRequested || this->states[VM_Top::Selected].dataIsReady)
		return REFRESH_PENDING;

	String mediaID2 = "";

	if (this->states[VM_Top::Selected].isValidSelectedRow(this->rows))
		mediaID2 = VM_FileSystem::GetYouTubeVideo(this->rows[this->states[VM_Top::Selected].selectedRow][0]->mediaID2);

	return mediaID2;
}

Graphics::VM_Buttons Graphics::VM_Table::getSelectedRow()
{
	VM_Buttons row;

	if ((this->states[VM_Top::Selected].selectedRow >= 0) && (this->states[VM_Top::Selected].selectedRow < (int)this->rows.size()))
		row = this->rows[this->states[VM_Top::Selected].selectedRow];

	return row;
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

		sql = "SELECT name, id, full_path FROM MEDIA_FILES";
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

VM_DBResult Graphics::VM_Table::getShoutCast()
{
	// http://wiki.shoutcast.com/wiki/SHOUTcast_Developer

	VM_DBResult result;
	String      apiKey   = VM_Text::Decrypt(SHOUTCAST_API_KEY);
	String      search   = VM_Text::EscapeSQL(VM_GUI::ListTable->getSearch(), true);
	String      mediaURL = String(SHOUTCAST_API_URL);

	// SEARCH
	if (!search.empty())
		mediaURL.append("stationsearch?search=" + VM_Text::EscapeURL(search) + "&");
	// POPULAR
	else
		mediaURL.append("Top500?");

	mediaURL.append("limit=" + std::to_string(this->states[VM_Top::Selected].offset) + "," + std::to_string(this->limit) + "&k=" + apiKey);

	this->maxRows = 500;

	if (this->response[VM_Top::Selected][mediaURL].empty())
		this->response[VM_Top::Selected][mediaURL] = VM_FileSystem::DownloadToString(mediaURL);

	LIB_XML::xmlDoc* document = VM_XML::Load(
		this->response[VM_Top::Selected][mediaURL].c_str(), (int)this->response[VM_Top::Selected][mediaURL].size()
	);

	// FAILED TO LOAD XML FILE
	if (document == NULL)
		return result;

	LIB_XML::xmlNode* stationList = VM_XML::GetNode("/stationlist", document);

	if (stationList == NULL)
	{
		FREE_XML_DOC(document);
		LIB_XML::xmlCleanupParser();

		return result;
	}

	VM_XmlNodes stations = VM_XML::GetChildNodes(stationList, document);

	for (auto station : stations)
	{
		if ((station == NULL) || (strcmp(reinterpret_cast<const char*>(station->name), "station") != 0))
			continue;

		VM_DBRow row = {
			{ "name", VM_Text::Replace(VM_XML::GetAttribute(station, "name"), "\\\"", "\"") },
			{ "id",   VM_XML::GetAttribute(station, "id") },
			{ "full_path", VM_XML::GetAttribute(station, "logo") }
		};
		result.push_back(row);
	}

	FREE_XML_DOC(document);
	LIB_XML::xmlCleanupParser();

	return result;
}

VM_DBResult Graphics::VM_Table::getTMDB(VM_MediaType mediaType)
{
	VM_DBResult result;

	if ((mediaType != MEDIA_TYPE_TMDB_MOVIE) && (mediaType != MEDIA_TYPE_TMDB_TV))
		return result;

	// https://developers.themoviedb.org/3/movies/get-top-rated-movies
	// https://developers.themoviedb.org/3/tv/get-top-rated-tv
	// https://developers.themoviedb.org/3/search/multi-search

	String  apiKey      = VM_Text::Decrypt(TMDB_API_KEY);
	String  mediaTypeS  = (mediaType == MEDIA_TYPE_TMDB_MOVIE ? "movie" : "tv");
	String  search      = VM_Text::EscapeSQL(VM_GUI::ListTable->getSearch(), true);
	Strings searchWords = VM_Text::Split(search, " ");
	String  mediaURL    = String(TMDB_API_URL);

	// SEARCH
	if (!searchWords.empty())
	{
		mediaURL.append("search/" + mediaTypeS + "?query=");

		for (int i = 0; i < (int)searchWords.size(); i++)
		{
			mediaURL.append(searchWords[i]);

			if (i != (int)searchWords.size() - 1)
				mediaURL.append("%20");
		}

		mediaURL.append("&");
	// TOP RATED
	} else {
		mediaURL.append(mediaTypeS + "/top_rated?");
	}

	mediaURL.append("page=" + std::to_string((this->states[VM_Top::Selected].offset / 20) + 1) + "&api_key=" + apiKey);

	if (this->response[VM_Top::Selected][mediaURL].empty())
		this->response[VM_Top::Selected][mediaURL] = VM_FileSystem::DownloadToString(mediaURL);

	LIB_JSON::json_t* document = VM_JSON::Parse(this->response[VM_Top::Selected][mediaURL].c_str());

	if (document == NULL)
		return result;

	this->maxRows = (int)VM_JSON::GetValueNumber(VM_JSON::GetItem(document, "total_results"));

	LIB_JSON::json_t*              items      = VM_JSON::GetItem(document, "results");
	std::vector<LIB_JSON::json_t*> itemsArray = VM_JSON::GetArray(items);

	for (int i = (this->states[VM_Top::Selected].offset % 20); i < (int)itemsArray.size(); i++)
	{
		if (i >= this->states[VM_Top::Selected].offset % 20 + this->limit)
			break;

		String title, title2;

		if (mediaType == MEDIA_TYPE_TMDB_MOVIE) {
			title  = VM_JSON::GetValueString(VM_JSON::GetItem(itemsArray[i], "title"));
			title2 = VM_JSON::GetValueString(VM_JSON::GetItem(itemsArray[i], "original_title"));
		} else {
			title  = VM_JSON::GetValueString(VM_JSON::GetItem(itemsArray[i], "name"));
			title2 = VM_JSON::GetValueString(VM_JSON::GetItem(itemsArray[i], "original_name"));
		}

		if (title.empty())
			title = title2;
		else if (title2 != title)
			title.append(" (" + title2 + ")");

		title = VM_Text::Replace(title, "\\\"", "\"");

		String thumbURL = VM_JSON::GetValueString(VM_JSON::GetItem(itemsArray[i], "poster_path"));

		if (!thumbURL.empty())
			thumbURL = ("https://image.tmdb.org/t/p/w342/" + thumbURL.substr(2));

		VM_DBRow row = {
			{ "name",      title },
			{ "id",        std::to_string((int64_t)VM_JSON::GetValueNumber(VM_JSON::GetItem(itemsArray[i], "id"))) },
			{ "full_path", thumbURL }
		};

		result.push_back(row);
	}

	FREE_JSON_DOC(document);

	return result;
}

VM_DBResult Graphics::VM_Table::getTracks(VM_MediaType mediaType)
{
	VM_DBResult      result;
	VM_DBRow         row;
	int              selected = VM_Player::GetStreamIndex(mediaType);
	std::vector<int> tracks   = VM_FileSystem::GetMediaStreamIndices(VM_Player::FormatContext, mediaType);

	if (selected < 0)
		return result;

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
		return result;

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

	return result;
}

VM_DBResult Graphics::VM_Table::getUPNP()
{
	VM_DBResult result;

	for (const auto &device : VM_UPNP::Devices)
	{
		Strings parts = VM_Text::Split(device, "|");

		if (parts.size() < 2)
			continue;

		VM_DBRow row = { { "name", parts[1] }, { "id", "0" }, { "full_path", parts[0] } };
		result.push_back(row);
	}

	return result;
}

VM_DBResult Graphics::VM_Table::getYouTube()
{
	// https://developers.google.com/youtube/v3/docs/search/list
	// https://developers.google.com/youtube/v3/docs/videos/list

	VM_DBResult result;
	String      apiKey      = VM_Text::Decrypt(YOUTUBE_API_KEY);
	String      search      = VM_Text::EscapeSQL(VM_GUI::ListTable->getSearch(), true);
	Strings     searchWords = VM_Text::Split(search, " ");
	String      mediaURL    = String(YOUTUBE_API_URL);

	// SEARCH
	if (!searchWords.empty())
	{
		mediaURL.append("search?type=video&part=snippet&q=");

		for (int i = 0; i < (int)searchWords.size(); i++)
		{
			mediaURL.append(searchWords[i]);

			if (i != (int)searchWords.size() - 1)
				mediaURL.append("%20");
		}
	// POPULAR
	} else {
		mediaURL.append("videos?chart=mostPopular&part=snippet");
	}

	mediaURL.append("&maxResults=" + std::to_string(this->limit) + "&safeSearch=strict&videoEmbeddable=true&videoSyndicated=true");

	if (!this->states[VM_Top::Selected].pageToken.empty())
		mediaURL.append("&pageToken=" + this->states[VM_Top::Selected].pageToken);

	mediaURL.append("&key=" + apiKey);

	if (this->response[VM_Top::Selected][mediaURL].empty())
		this->response[VM_Top::Selected][mediaURL] = VM_FileSystem::DownloadToString(mediaURL);

	LIB_JSON::json_t* document = VM_JSON::Parse(this->response[VM_Top::Selected][mediaURL].c_str());

	if (document == NULL)
		return result;

	this->pageTokenNext = VM_JSON::GetValueString(VM_JSON::GetItem(document, "nextPageToken"));
	this->pageTokenPrev = VM_JSON::GetValueString(VM_JSON::GetItem(document, "prevPageToken"));

	LIB_JSON::json_t* pageInfo = VM_JSON::GetItem(document, "pageInfo");

	if (pageInfo != NULL)
	{
		std::vector<LIB_JSON::json_t*> infoItems = VM_JSON::GetItems(pageInfo->child);

		for (auto infoItem : infoItems) {
			if (VM_JSON::GetKey(infoItem) == "totalResults")
				this->maxRows = (int)VM_JSON::GetValueNumber(infoItem);
		}
	}

	std::vector<LIB_JSON::json_t*> objectItems;
	LIB_JSON::json_t*              items      = VM_JSON::GetItem(document, "items");
	std::vector<LIB_JSON::json_t*> itemsArray = VM_JSON::GetArray(items);

	for (auto itemsObject : itemsArray)
	{
		objectItems = VM_JSON::GetItems(itemsObject);

		VM_DBRow row;
		String   mediaID = "";
		String   kind    = "";

		for (auto item : objectItems)
		{
			if (VM_JSON::GetKey(item) == "kind")
				kind = VM_JSON::GetValueString(item);

			// ID
			if (VM_JSON::GetKey(item) == "id")
			{
				if (kind == "youtube#video")
					mediaID = VM_JSON::GetValueString(item);
				else if (kind == "youtube#searchResult")
					mediaID = VM_JSON::GetValueString(VM_JSON::GetItem(item->child, "videoId"));

				row["id"] = mediaID;
			}
			// TITLE, THUMB
			else if (VM_JSON::GetKey(item) == "snippet")
			{
				// TITLE
				row["name"] = VM_JSON::GetValueString(VM_JSON::GetItem(item->child, "title"));
				row["name"] = VM_Text::Replace(row["name"], "\\\"", "\"");

				// THUMB
				LIB_JSON::json_t* thumb  = NULL;
				LIB_JSON::json_t* thumbs = VM_JSON::GetItem(item->child, "thumbnails");

				if (thumbs != NULL) 
					thumb = VM_JSON::GetItem(thumbs->child, "high");

				if (thumb == NULL) thumb = VM_JSON::GetItem(thumbs->child, "medium");
				if (thumb == NULL) thumb = VM_JSON::GetItem(thumbs->child, "default");

				if (thumb != NULL)
					row["full_path"] = VM_JSON::GetValueString(VM_JSON::GetItem(thumb->child, "url"));
			}
		}

		if (!mediaID.empty())
			result.push_back(row);
	}

	FREE_JSON_DOC(document);

	return result;
}

bool Graphics::VM_Table::offsetNext()
{
	if (this->states[VM_Top::Selected].offset + this->limit < this->maxRows)
	{
		if ((this->id == "list_table") && YOUTUBE_IS_SELECTED)
			this->states[VM_Top::Selected].pageToken = this->pageTokenNext;

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
	if (this->states[VM_Top::Selected].offset - this->limit >= 0)
	{
		if ((this->id == "list_table") && YOUTUBE_IS_SELECTED)
			this->states[VM_Top::Selected].pageToken = this->pageTokenPrev;

		this->states[VM_Top::Selected].offset      -= this->limit;
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
	this->shouldRefreshThumbs = true;
}

int Graphics::VM_Table::render()
{
	if (!this->visible)
		return ERROR_INVALID_ARGUMENTS;

	// SCROLL PANE TEXTURE
	if (this->scrollPane == NULL)
		this->resetScrollPane();

	// RENDER TO SCROLL PANE TEXTURE
	SDL_SetRenderTarget(VM_Window::Renderer, this->scrollPane->data);

	SDL_Rect backgroundArea = { 0, 0, this->scrollPane->width, this->scrollPane->height };
	VM_Graphics::FillArea(&this->backgroundColor, &backgroundArea);

	for (int row = 0; row < (int)this->rows.size();      row++) {
	for (int col = 0; col < (int)this->rows[row].size(); col++)
	{
		this->rows[row][col]->selected = (row == this->states[VM_Top::Selected].selectedRow);
		this->rows[row][col]->render();

		if (TMDB_MOVIE_IS_SELECTED || TMDB_TV_IS_SELECTED)
			continue;

		if ((this->id == "list_table") && (col == 0) && this->rows[row][col]->selected && !this->rows[row][1]->getText().empty())
		{
			if (this->playIcon == NULL)
			{
				this->playIcon     = new VM_Button(*dynamic_cast<VM_Component*>(this->rows[row][0]));
				this->playIcon->id = "list_table_play_icon";
				int height         = (int)((float)this->playIcon->backgroundArea.h / VM_Window::Display.scaleFactor * 0.3f);

				this->playIcon->backgroundColor = { 0, 0, 0, 0xA0 };

				VM_ThreadManager::Mutex.lock();
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
		int rowHeight = (!this->buttons.empty() ? this->buttons[0]->backgroundArea.h : 0);

		SDL_Rect clip = {
			this->backgroundArea.x,
			(this->backgroundArea.y + this->borderWidth.top + (this->states[VM_Top::Selected].scrollOffset * rowHeight)),
			this->backgroundArea.w,
			(this->backgroundArea.h - this->borderWidth.top - this->borderWidth.bottom)
		};

		SDL_Rect dest = { clip.x, (this->backgroundArea.y + this->borderWidth.top), clip.w, clip.h };

		SDL_RenderCopy(VM_Window::Renderer, this->scrollPane->data, &clip, &dest);
	}

	for (auto button : this->buttons)
		button->render();

	VM_Graphics::FillBorder(&this->borderColor, &this->backgroundArea, this->borderWidth);

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
	int rowHeight     = (!this->buttons.empty() ? this->buttons[0]->backgroundArea.h : 0);
	int textureHeight = (VM_Window::Dimensions.h - this->backgroundArea.h + (((int)this->rows.size() + 1) * rowHeight));

	this->scrollPane = new VM_Texture(
		SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET,
		VM_Window::Dimensions.w, max(VM_Window::Dimensions.h, textureHeight)
	);
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

			db->updateSettings(("list_page_token_"     + selected), state.pageToken);
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
	if (this->scrollBar == NULL)
		return ERROR_INVALID_ARGUMENTS;

	// SCROLLBAR BUTTONS
	VM_Component* next  = NULL;
	VM_Component* prev  = NULL;
	VM_Component* thumb = NULL;

	for (auto button : this->scrollBar->buttons)
	{
		if (button->id.find("_scrollbar_next") != String::npos)
			next = button;
		else if (button->id.find("_scrollbar_prev") != String::npos)
			prev = button;
		else if (button->id.find("_scrollbar_thumb") != String::npos)
			thumb = button;
	}

	if ((next == NULL) || (prev == NULL) || (thumb == NULL))
		return ERROR_UNKNOWN;

	// SELECT ROW
	int nextRow = (this->states[VM_Top::Selected].selectedRow + offset);

	if (nextRow < 0)
		this->selectRow(0);
	else if (nextRow >= (int)this->rows.size())
		this->selectRow((int)(this->rows.size() - 1));
	else
		this->selectRow(nextRow);

	// THUMB HEIGHT
	int rowsPerPage     = this->getRowsPerPage();
	int remainingRows   = max(((int)this->rows.size() - rowsPerPage + 1), 0);
	int scrollBarHeight = this->scrollBar->backgroundArea.h;

	scrollBarHeight -= (this->scrollBar->margin.top      + this->scrollBar->margin.bottom);
	scrollBarHeight -= (this->scrollBar->borderWidth.top + this->scrollBar->borderWidth.bottom);
	scrollBarHeight -= (next->backgroundArea.h + prev->backgroundArea.h);

	if (remainingRows == 0) {
		thumb->backgroundArea.h = 0;
		return RESULT_OK;
	} else {
		thumb->backgroundArea.h = (int)ceilf((float)scrollBarHeight / (float)(int)remainingRows);
	}

	// THUMB POSITION
	int startY = (prev->backgroundArea.y + prev->backgroundArea.h);

	// RESET/INIT
	if (offset == 0)
	{
		thumb->backgroundArea.y                     = startY;
		this->states[VM_Top::Selected].scrollOffset = 0;
	}
	else
	{
		int  nextY      = (thumb->backgroundArea.y + (thumb->backgroundArea.h * offset));
		bool validPrevY = (nextY >= startY);
		bool validNextY = (nextY + thumb->backgroundArea.h <= next->backgroundArea.y);

		// NEXT/PREVIOUS ROW/PAGE
		if (validPrevY && validNextY) {
			thumb->backgroundArea.y                      = nextY;
			this->states[VM_Top::Selected].scrollOffset += offset;
		// PREVIOUS PAGE (HOME)
		} else if (!validPrevY) {
			thumb->backgroundArea.y                     = startY;
			this->states[VM_Top::Selected].scrollOffset = 0;
		// NEXT PAGE (END)
		} else if (!validNextY) {
			thumb->backgroundArea.y                     = (next->backgroundArea.y - thumb->backgroundArea.h);
			this->states[VM_Top::Selected].scrollOffset = ((int)this->rows.size() - rowsPerPage);
		}
	}

	return RESULT_OK;
}

int Graphics::VM_Table::scroll(SDL_Event* mouseEvent)
{
	if ((mouseEvent == NULL) || (this->scrollBar == NULL))
		return ERROR_INVALID_ARGUMENTS;

	#if defined _android || defined _ios
		int y = (int)(mouseEvent->tfinger.y * (float)VM_Window::Dimensions.h);
	#else
		int y = mouseEvent->motion.y;
	#endif

	// SCROLLBAR BUTTONS
	VM_Component* next  = NULL;
	VM_Component* prev  = NULL;
	VM_Component* thumb = NULL;

	for (auto button : this->scrollBar->buttons)
	{
		if (button->id.find("_scrollbar_next") != String::npos)
			next = button;
		else if (button->id.find("_scrollbar_prev") != String::npos)
			prev = button;
		else if (button->id.find("_scrollbar_thumb") != String::npos)
			thumb = button;
	}

	if ((next == NULL) || (prev == NULL) || (thumb == NULL))
		return false;

	if (max(((int)this->rows.size() - this->getRowsPerPage() + 1), 0) == 0)
		return true;

	int  startY       = (prev->backgroundArea.y + prev->backgroundArea.h);
	int  prevY        = (y - (thumb->backgroundArea.h / 2));
	int  nextY        = (y + (thumb->backgroundArea.h / 2));
	bool validPrevY   = (prevY >= startY);
	bool validNextY   = (nextY <= next->backgroundArea.y);

	if ((y >= startY) && (y < next->backgroundArea.y)) {
		this->states[VM_Top::Selected].scrollOffset = ((y - startY) / thumb->backgroundArea.h);
		this->selectRow(this->states[VM_Top::Selected].scrollOffset);
	}

	if (!validPrevY)
		thumb->backgroundArea.y = startY;
	else if (!validNextY)
		thumb->backgroundArea.y = (next->backgroundArea.y - thumb->backgroundArea.h);
	else
		thumb->backgroundArea.y = prevY;

	return RESULT_OK;
}

void Graphics::VM_Table::scrollHome()
{
	this->states[VM_Top::Selected].scrollOffset = 0;
	this->scroll(-((int)this->rows.size()));
}

void Graphics::VM_Table::scrollEnd()
{
	this->states[VM_Top::Selected].scrollOffset = ((int)this->rows.size() - this->getRowsPerPage());
	this->scroll((int)this->rows.size());
}

void Graphics::VM_Table::scrollNext()
{
	this->scroll(1);
}

void Graphics::VM_Table::scrollPrev()
{
	this->scroll(-1);
}

void Graphics::VM_Table::scrollPageNext()
{
	this->scroll(5);
}

void Graphics::VM_Table::scrollPagePrev()
{
	this->scroll(-5);
}

bool Graphics::VM_Table::scrollPage(SDL_Event* mouseEvent)
{
	#if defined _android || defined _ios
		int y = (int)(mouseEvent->tfinger.y * (float)VM_Window::Dimensions.h);
	#else
		int y = mouseEvent->button.y;
	#endif

	for (auto button : this->scrollBar->buttons)
	{
		if (button->id.find("_scrollbar_thumb") == String::npos)
			continue;

		if (y < button->backgroundArea.y)
			this->scrollPagePrev();
		else if (y > button->backgroundArea.y + button->backgroundArea.h)
			this->scrollPageNext();
		else
			return false;

		return true;
	}

	return false;
}

void Graphics::VM_Table::scrollTo(int row)
{
	this->scroll(0);
	this->scroll(row);
}

bool Graphics::VM_Table::scrollThumb(SDL_Event* mouseEvent)
{
	if (mouseEvent == NULL)
		return false;

	// SCROLL MOUSE WHEEL
	if (mouseEvent->type == SDL_MOUSEWHEEL)
	{
		if (mouseEvent->wheel.y != 0) 
			this->scroll((mouseEvent->wheel.y / std::abs(mouseEvent->wheel.y)) * -1);

		return true;
	}
	// DRAG THUMB
	else if ((mouseEvent->type == SDL_MOUSEMOTION) || (mouseEvent->type == SDL_FINGERMOTION))
	{
		#if defined _android || defined _ios
			int dy = (int)(mouseEvent->tfinger.dy * (float)VM_Window::Dimensions.h);
		#else
			int dy = mouseEvent->motion.yrel;
		#endif

		if (this->scrollDrag && (dy != 0))
			this->scroll(mouseEvent);

		return true;
	}
	// PRESS THUMB
	else if (((mouseEvent->type == SDL_MOUSEBUTTONDOWN) || (mouseEvent->type == SDL_FINGERDOWN)) && (this->scrollBar != NULL))
	{
		for (auto button : this->scrollBar->buttons)
		{
			if (button->id.find("_scrollbar_thumb") == String::npos)
				continue;

			if (VM_Graphics::ButtonPressed(mouseEvent, &button->backgroundArea)) {
				this->scrollDrag = true;
				return true;
			}
		}
	}

	return false;
}

void Graphics::VM_Table::selectNext(bool loop)
{
	bool lastRow = (this->states[VM_Top::Selected].selectedRow == (int)this->rows.size() - 1);
	bool refresh = true;

	if ((this->states[VM_Top::Selected].offset + this->limit < this->maxRows) && lastRow)
	{
		if ((this->id == "list_table") && YOUTUBE_IS_SELECTED)
			this->states[VM_Top::Selected].pageToken = this->pageTokenNext;

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
		if ((this->id == "list_table") && YOUTUBE_IS_SELECTED)
			this->states[VM_Top::Selected].pageToken = this->pageTokenPrev;

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

bool Graphics::VM_Table::selectRow(int row)
{
	if ((row < 0) || (row >= (int)this->rows.size()))
		return false;

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

			db->updateSettings(("list_page_token_"    + selected), state.pageToken);
			db->updateSettings(("list_offset_"        + selected), std::to_string(state.offset));
			db->updateSettings(("list_scroll_offset_" + selected), std::to_string(state.scrollOffset));
			db->updateSettings(("list_selected_row_"  + selected), std::to_string(state.selectedRow));
		}

		DELETE_POINTER(db);
	}

	return true;
}

bool Graphics::VM_Table::selectRow(SDL_Event* mouseEvent)
{
	// CHECK IF THE SELECTED ROW IS CLICKABLE (VISIBLE WITHIN SCROLLED AREA)
	#if defined _android || defined _ios
		int positionY = (int)(mouseEvent->tfinger.y * (float)VM_Window::Dimensions.h);
	#else
		int positionY = mouseEvent->button.y;
	#endif

	int rowHeight = (!this->buttons.empty() ? this->buttons[0]->backgroundArea.h : 0);
	int startY    = (this->backgroundArea.y + rowHeight);
	int row       = ((positionY - startY + (this->states[VM_Top::Selected].scrollOffset * rowHeight)) / rowHeight);

	if ((row < this->states[VM_Top::Selected].scrollOffset) || (row >= this->states[VM_Top::Selected].scrollOffset + this->getRowsPerPage()))
		return false;

	// CHECK IF THE SELECTED ROW IS VALID (PART OF TOTAL ROWS)
	if (!this->selectRow(row) || (this->rows[row].size() < 3))
		return false;

	if ((this->id != "list_table") || VM_Modal::IsVisible())
		return true;

	VM_Button* thumb     = this->rows[row][0];
	VM_Button* info      = this->rows[row][this->rows[row].size() - 1];
	SDL_Rect   areaThumb = SDL_Rect(thumb->backgroundArea);
	SDL_Rect   areaInfo  = SDL_Rect(info->backgroundArea);

	areaThumb.y -= (this->states[VM_Top::Selected].scrollOffset * rowHeight);
	areaInfo.y  -= (this->states[VM_Top::Selected].scrollOffset * rowHeight);

	// SINGLE-CLICKED INFO/DETAILS ICON
	if (info->selected && VM_Graphics::ButtonPressed(mouseEvent, &areaInfo))
	{
		VM_Modal::Open(VM_XML::GetAttribute(info->xmlNode, "modal"));
	}
	else if (!TMDB_MOVIE_IS_SELECTED && !TMDB_TV_IS_SELECTED)
	{
		// SINGLE-CLICKED PLAY ICON
		if (thumb->selected && VM_Graphics::ButtonPressed(mouseEvent, &areaThumb))
		{
			if (YOUTUBE_IS_SELECTED)
				VM_Player::OpenFilePath(VM_FileSystem::GetYouTubeVideo(thumb->mediaID2));
			else if (SHOUTCAST_IS_SELECTED)
				VM_Player::OpenFilePath(VM_FileSystem::GetShoutCastStation(thumb->mediaID));
			else
				VM_Player::OpenFilePath(thumb->mediaURL);
		}
		// DOUBLE- OR RIGHT-CLICKED ROW
		else
		{
			for (auto button : this->rows[row])
			{
				SDL_Rect area = SDL_Rect(button->backgroundArea);
				area.y       -= (this->states[VM_Top::Selected].scrollOffset * rowHeight);

				if (VM_Graphics::ButtonPressed(mouseEvent, &area, false, true))
				{
					if (YOUTUBE_IS_SELECTED)
						VM_Player::OpenFilePath(VM_FileSystem::GetYouTubeVideo(button->mediaID2));
					else if (SHOUTCAST_IS_SELECTED)
						VM_Player::OpenFilePath(VM_FileSystem::GetShoutCastStation(button->mediaID));
					else
						VM_Player::OpenFilePath(button->mediaURL);
				}
				else if (VM_Graphics::ButtonPressed(mouseEvent, &area, true, false))
				{
					if (!YOUTUBE_IS_SELECTED && !SHOUTCAST_IS_SELECTED)
						VM_Modal::Open("modal_right_click");
				}
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
	else if (this->id == "modal_upnp_devices_list_table")
		this->result = this->getUPNP();
	else if ((this->id == "list_table") && YOUTUBE_IS_SELECTED)
		this->result = this->getYouTube();
	else if ((this->id == "list_table") && SHOUTCAST_IS_SELECTED)
		this->result = this->getShoutCast();
	else if ((this->id == "list_table") && (TMDB_MOVIE_IS_SELECTED || TMDB_TV_IS_SELECTED))
		this->result = this->getTMDB(VM_Top::Selected);
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

	// DOWNLOAD/CREATE THUMBS
	for (auto &row : this->result)
	{
		String thumbFile = "";

		if ((this->id == "list_table") && YOUTUBE_IS_SELECTED)
			thumbFile = ("youtube_" + row["id"]);
		else if ((this->id == "list_table") && SHOUTCAST_IS_SELECTED)
			thumbFile = ("shoutcast_" + row["id"]);
		else if ((this->id == "list_table") && TMDB_MOVIE_IS_SELECTED)
			thumbFile = ("tmbd_movie_" + row["id"]);
		else if ((this->id == "list_table") && TMDB_TV_IS_SELECTED)
			thumbFile = ("tmdb_tv_" + row["id"]);
		else if ((this->id == "list_table") || (this->id == "modal_playlists_list_table"))
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

			std::thread createThumb(VM_Graphics::CreateThumbThread, threadData);
			createThumb.detach();
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
	for (int col = 0; col < (int)this->buttons.size(); col++)
	{
		if (!VM_Player::State.isStopped || (VM_Modal::IsVisible() && (VM_Modal::ListTable == NULL)))
			break;

		VM_Button* button = dynamic_cast<VM_Button*>(this->buttons[col]);

		if (button->imageData != NULL)
			button->removeImage();

		button->setText("");

		if ((this->id == "list_table") && (VM_Top::Selected >= MEDIA_TYPE_YOUTUBE) && (col == 1))
		{
			button->setText(VM_Window::Labels["title"]);
		}
		else if (!temp && !this->result.empty() && (button->id == this->states[VM_Top::Selected].sortColumn) &&
			((this->id == "list_table") || (this->id == "modal_playlists_list_table")))
		{
			button->margin.left = 0;
			button->setImage((this->states[VM_Top::Selected].sortDirection == "ASC" ? "triangle-4-64.png" : "triangle-3-64.png"), false, 10, 10);
		}

		if (col > 0)
			button->margin.left = 10;

		button->borderWidth = VM_Border(3, 0, 0, 3);
	}

	this->resultMutex.lock();

	// FILE ROWS
	for (int row = 0; row < (int)this->result.size(); row++)
	{
		offsetY += this->buttons[0]->backgroundArea.h;

		VM_Buttons buttonsRow;

		for (int col = 0; col < (int)this->buttons.size(); col++)
		{
			VM_Button* buttonColumn = new VM_Button(*this->buttons[col]);

			buttonColumn->backgroundArea.y += offsetY;
			buttonColumn->backgroundColor   = (row % 2 == 0 ? row1Color : row2Color);
			buttonColumn->borderWidth       = VM_Border(3, 0, 0, 0);
			buttonColumn->id               += ("_" + std::to_string(row) + "_" + std::to_string(col));
			buttonColumn->highlightColor    = this->getColor("highlight");
			buttonColumn->mediaID           = std::atoi(this->result[row]["id"].c_str());
			buttonColumn->mediaID2          = this->result[row]["id"];
			buttonColumn->mediaURL          = this->result[row]["full_path"];
			buttonColumn->parent            = this;

			// FIRST COLUMN - THUMB
			if ((col == 0) && (this->id == "list_table"))
			{
				buttonColumn->borderWidth     = VM_Border(5);
				buttonColumn->borderColor     = buttonColumn->backgroundColor;
				buttonColumn->backgroundColor = { 0, 0, 0, 0xFF };
				buttonColumn->highlightColor  = buttonColumn->backgroundColor;

				if (!temp)
					buttonColumn->setThumb(this->id);
			// LAST COLUMN - ABOUT/INFO
			} else if ((col == (int)this->buttons.size() - 1) && !temp) {
				buttonColumn->setImage((VM_GUI::ColorThemeFile == "dark" ? "about-1-512.png" : "about-2-512.png"), false);
			// REMAINING COLUMNS
			} else if (!temp) {
				buttonColumn->setText(this->result[row]["name"]);
			}

			buttonsRow.push_back(buttonColumn);
		}

		this->rows.push_back(buttonsRow);
	}

	this->resultMutex.unlock();

	int row = this->states[VM_Top::Selected].selectedRow;

	if (!temp)
		this->scrollTo(this->states[VM_Top::Selected].scrollOffset);

	this->selectRow(row);

	if (((int)this->rows.size() < this->limit) && (this->maxRows > (this->states[VM_Top::Selected].offset + (int)this->rows.size())))
		this->maxRows = (this->states[VM_Top::Selected].offset + (int)this->rows.size());

	char detailsText[DEFAULT_CHAR_BUFFER_SIZE];

	if (temp)
		snprintf(detailsText, DEFAULT_CHAR_BUFFER_SIZE, "[ Loading ... ]");
	else if ((int)this->rows.size() < this->maxRows)
		snprintf(detailsText, DEFAULT_CHAR_BUFFER_SIZE, "[ %d - %d / %d ]", (this->states[VM_Top::Selected].offset + 1), (this->states[VM_Top::Selected].offset + (int)this->rows.size()), this->maxRows);
	else if (!this->rows.empty())
		snprintf(detailsText, DEFAULT_CHAR_BUFFER_SIZE, "[ %d ]", (int)this->rows.size());
	else
		snprintf(detailsText, DEFAULT_CHAR_BUFFER_SIZE, "[ 0 ]");

	if ((this->id == "list_table") && !temp && (this->maxRows > 0) &&
		(this->states[VM_Top::Selected].offset >= this->maxRows))
	{
		this->resetState(false);
		this->refreshRows();

		return ERROR_UNKNOWN;
	}

	VM_Button* button = NULL;

	if (this->id == "list_table")
		button = dynamic_cast<VM_Button*>(VM_GUI::Components["list_details_text"]);
	else
		button = dynamic_cast<VM_Button*>(VM_Modal::Components["list_details_text"]);

	if (button != NULL)
		button->setText(detailsText);

	if (!temp)
		this->states[VM_Top::Selected].dataIsReady = false;

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

void Graphics::VM_Table::updateSearchInput()
{
	VM_Button* searchInput = dynamic_cast<VM_Button*>(VM_GUI::Components["middle_search_input"]);

	if (searchInput != NULL)
		searchInput->setInputText(VM_GUI::ListTable->getSearch());
}
