#include "VM_Database.h"

using namespace VoyaMedia::System;

Database::VM_Database::VM_Database(int &result, const VM_DatabaseType databaseType)
{
	VM_ThreadManager::DBMutex.lock();

	result = SQLITE_ERROR;

	#if defined _windows
		WString databasePath = VM_FileSystem::GetPathDatabaseW();
		WString databaseFile;

		switch (databaseType) {
			case DATABASE_MEDIALIBRARYv3: databaseFile = WString(databasePath + L"MediaLibrary3.db"); break;
			case DATABASE_PLAYLISTSv3:    databaseFile = WString(databasePath + L"Playlists3.db");    break;
			case DATABASE_SETTINGSv3:     databaseFile = WString(databasePath + L"Settings3.db");     break;
		}

		result = LIB_SQLITE::sqlite3_open16(databaseFile.c_str(), &this->connection);
	#else
		String databasePath = VM_FileSystem::GetPathDatabase();
		String databaseFile;
		uint16_t databaseFileUT16[DEFAULT_CHAR_BUFFER_SIZE];

		switch (databaseType) {
			case DATABASE_MEDIALIBRARYv3: databaseFile = String(databasePath + "MediaLibrary3.db"); break;
			case DATABASE_PLAYLISTSv3:    databaseFile = String(databasePath + "Playlists3.db");    break;
			case DATABASE_SETTINGSv3:     databaseFile = String(databasePath + "Settings3.db");     break;
		}

		VM_Text::ToUTF16(databaseFile, databaseFileUT16, DEFAULT_CHAR_BUFFER_SIZE);
		result = sqlite3_open16(databaseFileUT16, &this->connection);
	#endif

	if (DB_RESULT_OK(result))
	{
		this->createDefaultTables(databaseType);

		this->sqlExecute("PRAGMA count_changes=OFF;");
		this->sqlExecute("PRAGMA foreign_keys=ON;");
		this->sqlExecute("PRAGMA journal_mode=OFF;");
		this->sqlExecute("PRAGMA synchronous=OFF;");
	} else {
		result = SQLITE_ERROR;
	}

	this->statement = NULL;
}

Database::VM_Database::~VM_Database()
{
	FREE_DB(this->connection);
	VM_ThreadManager::DBMutex.unlock();
}

void Database::VM_Database::createDefaultTables(const VM_DatabaseType databaseType)
{
	int    result;
	String sqlQuery;

	switch (databaseType) {
	case DATABASE_MEDIALIBRARYv3:
		sqlQuery = "CREATE TABLE IF NOT EXISTS MEDIA_FILES (";
		sqlQuery.append("  id         INTEGER PRIMARY KEY, ");
		sqlQuery.append("  full_path  TEXT    UNIQUE COLLATE NOCASE NOT NULL, ");
		sqlQuery.append("  name       TEXT    COLLATE NOCASE, ");
		sqlQuery.append("  path       TEXT    COLLATE NOCASE, ");
		sqlQuery.append("  size       INTEGER DEFAULT 0, ");
		sqlQuery.append("  duration   INTEGER DEFAULT 0, ");
		sqlQuery.append("  media_type INTEGER DEFAULT " + std::to_string(DEFAULT_MEDIA_TYPE) + ", ");
		sqlQuery.append("  mime_type  TEXT    DEFAULT '', ");
		sqlQuery.append("  rotation   NUMERIC DEFAULT 0");
		sqlQuery.append(");");

		result = this->sqlExecute(sqlQuery);

		if (DB_RESULT_OK(result))
		{
			this->sqlExecute("CREATE INDEX idx_mediaFiles_fullPath_asc  ON MEDIA_FILES (full_path UNIQUE COLLATE NOCASE ASC);");
			this->sqlExecute("CREATE INDEX idx_mediaFiles_name_asc      ON MEDIA_FILES (name COLLATE NOCASE ASC);");
			this->sqlExecute("CREATE INDEX idx_mediaFiles_name_desc     ON MEDIA_FILES (name COLLATE NOCASE DESC);");
			this->sqlExecute("CREATE INDEX idx_mediaFiles_mediaType_asc ON MEDIA_FILES (media_type ASC);");
		}

		break;
	case DATABASE_PLAYLISTSv3:
		sqlQuery = "CREATE TABLE IF NOT EXISTS PLAYLISTS (";
		sqlQuery.append("  id     INTEGER PRIMARY KEY, ");
		sqlQuery.append("  name   TEXT    UNIQUE COLLATE NOCASE NOT NULL, ");
		sqlQuery.append("  search TEXT    COLLATE NOCASE NOT NULL");
		sqlQuery.append(");");

		result = this->sqlExecute(sqlQuery);

		if (DB_RESULT_OK(result))
		{
			this->sqlExecute("CREATE INDEX idx_playlists_name_asc    ON PLAYLISTS (name UNIQUE COLLATE NOCASE ASC);");
			this->sqlExecute("CREATE INDEX idx_playlists_name_desc   ON PLAYLISTS (name UNIQUE COLLATE NOCASE DESC);");
			this->sqlExecute("CREATE INDEX idx_playlists_search_asc  ON PLAYLISTS (search COLLATE NOCASE ASC);");
			this->sqlExecute("CREATE INDEX idx_playlists_search_desc ON PLAYLISTS (search COLLATE NOCASE DESC);");
		}

		break;
	case DATABASE_SETTINGSv3:
		sqlQuery = "CREATE TABLE IF NOT EXISTS SETTINGS (";
		sqlQuery.append("  id    INTEGER PRIMARY KEY, ");
		sqlQuery.append("  key   TEXT    UNIQUE NOT NULL, ");
		sqlQuery.append("  value TEXT    NOT NULL");
		sqlQuery.append(");");

		result = this->sqlExecute(sqlQuery);

		if (DB_RESULT_OK(result))
			this->sqlExecute("CREATE INDEX idx_settings_key_asc ON SETTINGS (key UNIQUE ASC);");

		break;
	}
}

int Database::VM_Database::addFile(const String &fullPath, const String &name, const String &path, size_t size, VM_MediaType mediaType, const String &mimeType)
{
	if (fullPath.empty() || name.empty() || path.empty() || (mediaType == MEDIA_TYPE_UNKNOWN))
		return SQLITE_ERROR;

	uint16_t fullPath16[DEFAULT_CHAR_BUFFER_SIZE];
	uint16_t name16[DEFAULT_CHAR_BUFFER_SIZE];
	uint16_t path16[DEFAULT_CHAR_BUFFER_SIZE];
	uint16_t mime16[DEFAULT_CHAR_BUFFER_SIZE];

	int result = this->sqlPrepare(
		"INSERT INTO MEDIA_FILES (full_path,name,path,size,media_type,mime_type) VALUES (?,?,?,?,?,?);"
	);

	if (DB_RESULT_OK(result))
	{
		VM_Text::ToUTF16(fullPath, fullPath16, DEFAULT_CHAR_BUFFER_SIZE);
		VM_Text::ToUTF16(name,     name16,     DEFAULT_CHAR_BUFFER_SIZE);
		VM_Text::ToUTF16(path,     path16,     DEFAULT_CHAR_BUFFER_SIZE);
		VM_Text::ToUTF16(mimeType, mime16,     DEFAULT_CHAR_BUFFER_SIZE);

		result = LIB_SQLITE::sqlite3_bind_text16(this->statement, 1, fullPath16, -1, NULL);
		result = LIB_SQLITE::sqlite3_bind_text16(this->statement, 2, name16,     -1, NULL);
		result = LIB_SQLITE::sqlite3_bind_text16(this->statement, 3, path16,     -1, NULL);
		result = LIB_SQLITE::sqlite3_bind_int64(this->statement,  4, size);
		result = LIB_SQLITE::sqlite3_bind_int64(this->statement,  5, mediaType);
		result = LIB_SQLITE::sqlite3_bind_text16(this->statement, 6, mime16,     -1, NULL);
	}

	result = this->sqlExecutePrepared();

	if (DB_RESULT_OK(result))
		VM_FileSystem::RefreshMetaData();

	return result;
}

int Database::VM_Database::addPlaylist(const String &name, const String &search)
{
	if (name.empty())
		return SQLITE_ERROR;

	uint16_t name16[DEFAULT_CHAR_BUFFER_SIZE];
	uint16_t search16[DEFAULT_CHAR_BUFFER_SIZE];

	int result = this->sqlPrepare("INSERT INTO PLAYLISTS (name,search) VALUES (?,?);");

	if (DB_RESULT_OK(result))
	{
		VM_Text::ToUTF16(name,   name16,   DEFAULT_CHAR_BUFFER_SIZE);
		VM_Text::ToUTF16(search, search16, DEFAULT_CHAR_BUFFER_SIZE);

		result = LIB_SQLITE::sqlite3_bind_text16(this->statement, 1, name16,   -1, NULL);
		result = LIB_SQLITE::sqlite3_bind_text16(this->statement, 2, search16, -1, NULL);
	}

	result = this->sqlExecutePrepared();

	return result;
}

int Database::VM_Database::deleteMediaFile(int mediaID)
{
	if (mediaID < 1)
		return SQLITE_ERROR;

	// DELETE FROM DB
	int result = this->sqlPrepare("DELETE FROM MEDIA_FILES WHERE id=?;");

	if (DB_RESULT_OK(result))
		result = LIB_SQLITE::sqlite3_bind_int64(this->statement, 1, mediaID);

	result = this->sqlExecutePrepared();

	// DELETE THUMBNAIL FILE FROM DISK
	if (DB_RESULT_OK(result))
	{
	#if defined _windows
		_wremove(VM_FileSystem::GetPathThumbnailsW(std::to_wstring(mediaID)).c_str());
	#else
		remove(VM_FileSystem::GetPathThumbnails(std::to_string(mediaID)).c_str());
	#endif
	}

	return result;
}

int Database::VM_Database::deleteMediaPath(int mediaID)
{
	if (mediaID < 1)
		return SQLITE_ERROR;

	int         result;
	String      path = VM_Text::EscapeSQL(this->getValue(mediaID, "path"));
	String      sql  = VM_Text::Format("SELECT id FROM MEDIA_FILES WHERE path=%s;", path.c_str());
	VM_DBResult rows = this->getRows(sql);

	for (auto row : rows)
	{
		if (row.empty())
			continue;

		result = this->deleteMediaFile(std::atoi(row["id"].c_str()));

		if (DB_RESULT_ERROR(result))
			return result;
	}

	return SQLITE_OK;
}

int Database::VM_Database::deletePlaylist(const String &name)
{
	if (name.empty())
		return SQLITE_ERROR;

	int result = this->sqlPrepare("DELETE FROM PLAYLISTS WHERE name=?;");

	if (DB_RESULT_OK(result))
	{
		uint16_t name16[DEFAULT_CHAR_BUFFER_SIZE];
		VM_Text::ToUTF16(name, name16, DEFAULT_CHAR_BUFFER_SIZE);

		result = LIB_SQLITE::sqlite3_bind_text16(this->statement, 1, name16, -1, NULL);
	}

	result = this->sqlExecutePrepared();

	return result;
}

int Database::VM_Database::getID(const String &filePath)
{
	if (!filePath.empty())
	{
		VM_DBRow row = this->getRow(VM_Text::Format(
			"SELECT id FROM MEDIA_FILES WHERE full_path=%s;", VM_Text::EscapeSQL(filePath).c_str()
		));

		if (!row.empty())
			return std::atoi(row["id"].c_str());
	}

	return 0;
}

Database::VM_DBResult Database::VM_Database::getRows(const String &sqlQuery, int maxFiles)
{
	VM_DBResult rows;

	if (sqlQuery.empty())
		return rows;

	int result = this->sqlPrepare(sqlQuery);

	// TODO: BIND

	if (!DB_RESULT_OK(result))
		return rows;

	rows.reserve(maxFiles);

	int retry = 0;

	do {
		do {
			result = LIB_SQLITE::sqlite3_step(this->statement);

			if (result != SQLITE_ROW)
			{
				#ifdef _DEBUG
				if (!DB_RESULT_OK(result))
					LOG("Database::VM_Database::getRows: %s", LIB_SQLITE::sqlite3_errmsg(this->connection));
				#endif

				retry++;
				continue;
			}

			VM_DBRow row;

			for (int i = 0; i < LIB_SQLITE::sqlite3_column_count(this->statement); i++)
			{
				const char* header = LIB_SQLITE::sqlite3_column_name(this->statement, i);
				const char* column = reinterpret_cast<const char*>(LIB_SQLITE::sqlite3_column_text(this->statement, i));

				if (column != NULL)
					row[header] = column;
			}

			rows.push_back(row);
			retry++;
		} while ((result == SQLITE_ROW) && ((int)rows.size() <= maxFiles));
	} while (DB_BUSY_OR_LOCKED(result) && (retry < MAX_ERRORS));

	LIB_SQLITE::sqlite3_finalize(this->statement);

	return rows;
}

Database::VM_DBRow Database::VM_Database::getRow(const String &sqlQuery)
{
	VM_DBRow row;

	if (sqlQuery.empty())
		return row;

	int result = this->sqlPrepare(sqlQuery);

	// TODO: BIND

	if (!DB_RESULT_OK(result))
		return row;

	int retry = 0;

	do {
		result = LIB_SQLITE::sqlite3_step(this->statement);

		if (result != SQLITE_ROW)
		{
			#ifdef _DEBUG
			if (!DB_RESULT_OK(result))
				LOG("Database::VM_Database::getRow: %s", LIB_SQLITE::sqlite3_errmsg(this->connection));
			#endif

			retry++;
			continue;
		}

		for (int i = 0; i < LIB_SQLITE::sqlite3_column_count(this->statement); i++)
		{
			const char* header = LIB_SQLITE::sqlite3_column_name(this->statement, i);
			const char* column = reinterpret_cast<const char*>(LIB_SQLITE::sqlite3_column_text(this->statement, i));

			if (column != NULL)
				row[header] = column;
		}

		break;
	} while (DB_BUSY_OR_LOCKED(result) && (retry < MAX_ERRORS));

	LIB_SQLITE::sqlite3_finalize(this->statement);

	return row;
}

int Database::VM_Database::getRowCount(const String &sqlQuery)
{
	String sql = sqlQuery.substr(sqlQuery.find("FROM"));
	sql = sql.substr(0, sql.rfind("ORDER"));

	VM_DBRow row = this->getRow(VM_Text::Format("SELECT COUNT(*) AS count %s;", sql.c_str()));

	if (!row.empty())
		return std::atoi(row["count"].c_str());

	return 0;
}

String Database::VM_Database::getSettings(const String &key)
{
	if (key.empty())
		return "";

	VM_DBRow row = this->getRow(VM_Text::Format(
		"SELECT value FROM SETTINGS WHERE key=%s;", VM_Text::EscapeSQL(key).c_str()
	));

	if (!row.empty())
		return row["value"];

	return "";
}

String Database::VM_Database::getValue(int mediaID, const String &column)
{
	if ((mediaID < 1) || column.empty())
		return "";

	VM_DBRow row = this->getRow(VM_Text::Format(
		"SELECT %s FROM MEDIA_FILES WHERE id=%d;", VM_Text::EscapeSQL(column, true).c_str(), mediaID
	));

	if (!row.empty())
		return row[column];

	return "";
}

#if defined _windows
WString Database::VM_Database::getValueW(const String &sqlQuery)
{
	if (sqlQuery.empty())
		return L"";

	int result = this->sqlPrepare(sqlQuery);

	// TODO: BIND

	if (!DB_RESULT_OK(result))
		return L"";

	wchar_t value[DEFAULT_CHAR_BUFFER_SIZE];
	int     retry = 0;

	do {
		result = LIB_SQLITE::sqlite3_step(this->statement);

		if (result != SQLITE_ROW)
		{
			#ifdef _DEBUG
			if (!DB_RESULT_OK(result))
				LOG("Database::VM_Database::getValueW: %s", LIB_SQLITE::sqlite3_errmsg(this->connection));
			#endif

			retry++;
			continue;
		}

		std::swprintf(
			value, DEFAULT_CHAR_BUFFER_SIZE, L"%s",
			(wchar_t*)LIB_SQLITE::sqlite3_column_text16(this->statement, 0)
		);

		break;
	} while (DB_BUSY_OR_LOCKED(result) && (retry < MAX_ERRORS));

	LIB_SQLITE::sqlite3_finalize(this->statement);

	return WString(value);
}

WString Database::VM_Database::getValueW(int mediaID, const String &column)
{
	if ((mediaID < 1) || column.empty())
		return L"";

	return this->getValueW(VM_Text::Format(
		"SELECT %s FROM MEDIA_FILES WHERE id=%d;", VM_Text::EscapeSQL(column, true).c_str(), mediaID
	));
}
#endif

int Database::VM_Database::sqlExecute(const String &sqlQuery)
{
	int result = SQLITE_ERROR;

	if (sqlQuery.empty())
		return result;

	int retry = 0;

	do {
		result = sqlite3_exec(this->connection, sqlQuery.c_str(), NULL, NULL, NULL);
		retry++;
	} while (DB_BUSY_OR_LOCKED(result) && (retry < MAX_ERRORS));

	return result;
}

int Database::VM_Database::sqlPrepare(const String &sqlQuery)
{
	int result = SQLITE_ERROR;

	if (sqlQuery.empty())
		return result;

	uint16_t sqlUT16[DEFAULT_CHAR_BUFFER_SIZE];
	int      retry = 0;

	VM_Text::ToUTF16(sqlQuery, sqlUT16, DEFAULT_CHAR_BUFFER_SIZE);

	do {
		result = LIB_SQLITE::sqlite3_prepare16_v2(this->connection, sqlUT16, -1, &this->statement, NULL);

		#ifdef _DEBUG
		if (!DB_RESULT_OK(result))
			LOG("Database::VM_Database::sqlPrepare: %s", LIB_SQLITE::sqlite3_errmsg(this->connection));
		#endif

		retry++;
	} while (DB_BUSY_OR_LOCKED(result) && (retry < MAX_ERRORS));

	return result;
}

int Database::VM_Database::sqlExecutePrepared()
{
	int result = SQLITE_ERROR;
	int retry  = 0;

	do {
		result = LIB_SQLITE::sqlite3_step(this->statement);

		#ifdef _DEBUG
		if (!DB_RESULT_OK(result))
			LOG("Database::VM_Database::sqlExecutePrepared: %s", LIB_SQLITE::sqlite3_errmsg(this->connection));
		#endif

		retry++;
	} while (DB_BUSY_OR_LOCKED(result) && (retry < MAX_ERRORS));

	retry = 0;

	do {
		result = LIB_SQLITE::sqlite3_finalize(this->statement);

		#ifdef _DEBUG
		if (!DB_RESULT_OK(result))
			LOG("Database::VM_Database::sqlExecutePrepared: %s", LIB_SQLITE::sqlite3_errmsg(this->connection));
		#endif

		retry++;
	} while (DB_BUSY_OR_LOCKED(result) && (retry < MAX_ERRORS));

	return result;
}

int Database::VM_Database::updateDouble(int64_t mediaID, const String &column, double value)
{
	if (mediaID < 1)
		return SQLITE_ERROR;

	int result = this->sqlPrepare(VM_Text::Format(
		"UPDATE MEDIA_FILES SET %s=? WHERE id=?;", VM_Text::EscapeSQL(column, true).c_str()
	));

	if (DB_RESULT_OK(result)) {
		result = LIB_SQLITE::sqlite3_bind_double(this->statement, 1, value);
		result = LIB_SQLITE::sqlite3_bind_int64(this->statement,  2, mediaID);
	}

	return this->sqlExecutePrepared();
}

int Database::VM_Database::updateInteger(int64_t mediaID, const String &column, int64_t value)
{
	if (mediaID < 1)
		return SQLITE_ERROR;

	int result = this->sqlPrepare(VM_Text::Format(
		"UPDATE MEDIA_FILES SET %s=? WHERE id=?;", VM_Text::EscapeSQL(column, true).c_str()
	));

	if (DB_RESULT_OK(result)) {
		result = LIB_SQLITE::sqlite3_bind_int64(this->statement, 1, value);
		result = LIB_SQLITE::sqlite3_bind_int64(this->statement, 2, mediaID);
	}

	return this->sqlExecutePrepared();
}

int Database::VM_Database::updateText(int64_t mediaID, const String &column, const String &value)
{
	if (mediaID < 1)
		return SQLITE_ERROR;

	uint16_t value16[DEFAULT_CHAR_BUFFER_SIZE];

	int result = this->sqlPrepare(VM_Text::Format(
		"UPDATE MEDIA_FILES SET %s=? WHERE id=?;", VM_Text::EscapeSQL(column, true).c_str()
	));

	if (DB_RESULT_OK(result))
	{
		VM_Text::ToUTF16(value, value16, DEFAULT_CHAR_BUFFER_SIZE);

		result = LIB_SQLITE::sqlite3_bind_text16(this->statement, 1, value16, -1, NULL);
		result = LIB_SQLITE::sqlite3_bind_int64(this->statement,  2, mediaID);
	}

	return this->sqlExecutePrepared();
}

int Database::VM_Database::updateSettings(const String &key, const String &value)
{
	int result = SQLITE_ERROR;

	if (key.empty())
		return result;

	VM_DBRow row = this->getRow(VM_Text::Format(
		"SELECT key FROM SETTINGS WHERE key=%s;", VM_Text::EscapeSQL(key).c_str()
	));

	if (!row.empty() && !row["key"].empty())
		result = this->sqlPrepare("UPDATE SETTINGS SET value=? WHERE key=?;");
	else
		result = this->sqlPrepare("INSERT INTO SETTINGS (value, key) VALUES (?,?);");

	if (DB_RESULT_OK(result)) {
		result = LIB_SQLITE::sqlite3_bind_text(this->statement, 1, value.c_str(), -1, NULL);
		result = LIB_SQLITE::sqlite3_bind_text(this->statement, 2, key.c_str(),   -1, NULL);
	}

	return this->sqlExecutePrepared();
}
