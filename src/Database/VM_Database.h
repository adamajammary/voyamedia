#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_DATABASE_H
#define VM_DATABASE_H

namespace VoyaMedia
{
	namespace Database
	{
		class VM_Database
		{
		public:
			VM_Database(int &result, const VM_DatabaseType databaseType);
			~VM_Database();

		private:
			LIB_SQLITE::sqlite3*      connection;
			LIB_SQLITE::sqlite3_stmt* statement;

		public:
			int         addFile(const String &fullPath, const String &name, const String &path, size_t size, VM_MediaType mediaType, const String &mimeType);
			int         addPlaylist(const String &name, const String &search);
			int         deleteMediaFile(int mediaID);
			int         deleteMediaPath(int mediaID);
			int         deletePlaylist(const String &name);
			int         getID(const String &filePath);
			VM_DBResult getRows(const String &sqlQuery, int maxFiles = MAX_DB_RESULT);
			int         getRowCount(const String &sqlQuery);
			String      getSettings(const String &key);
			String      getValue(int mediaID, const String &column);
			int         updateDouble(int64_t mediaID, const String &column, double value);
			int         updateInteger(int64_t mediaID, const String &column, int64_t value);
			int         updateText(int64_t mediaID, const String &column, const String &value);
			int         updateSettings(const String &key, const String &value);

			#if defined _windows
				WString getValueW(int mediaID, const String &column);
			#endif

		private:
			void     createDefaultTables(const VM_DatabaseType databaseType);
			VM_DBRow getRow(const String &sqlQuery);
			int      sqlExecute(const String &sqlQuery);
			int      sqlPrepare(const String &sqlQuery);
			int      sqlExecutePrepared();

			#if defined _windows
				WString getValueW(const String &sqlQuery);
			#endif

		};
	}
}

#endif
