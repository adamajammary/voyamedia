#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_TABLE_H
#define VM_TABLE_H

namespace VoyaMedia
{
	namespace Graphics
	{
		struct VM_SelectedRow
		{
			int    mediaID  = 0;
			String mediaID2 = "";
			String title    = "";
			String mediaURL = "";
		};

		struct VM_TableState
		{
			bool   dataIsReady   = false;
			bool   dataRequested = false;
			int    offset        = 0;
			int    scrollOffset  = 0;
			String searchString  = "";
			int    selectedRow   = 0;
			String sortColumn    = "";
			String sortDirection = "ASC";

			bool isValidSelectedRow(const std::vector<VM_Buttons> &rows)
			{
				int row = this->selectedRow;
				return ((row >= 0) && (row < (int)rows.size()) && (rows[row].size() > 2));
			}

			void reset(bool resetDataRequest = true)
			{
				if (resetDataRequest) {
					this->dataIsReady   = false;
					this->dataRequested = false;
				}

				this->offset        = 0;
				this->scrollOffset  = 0;
				this->selectedRow   = 0;
				this->sortColumn    = "";
				this->sortDirection = "ASC";
			}

			void restore(const VM_TableState &state)
			{
				this->offset        = state.offset;
				this->scrollOffset  = state.scrollOffset;
				this->selectedRow   = state.selectedRow;
				this->sortColumn    = state.sortColumn;
				this->sortDirection = state.sortDirection;
			}
		};

		class VM_Table : public VM_Component
		{
		public:
			VM_Table(const VM_Component &component);
			VM_Table(const String &id);
			~VM_Table();

		public:
			VM_Button*              playIcon;
			std::vector<VM_Buttons> rows;
			VM_Component*           scrollBar;

		private:
			const int                   limit = 10;
			int                         maxRows;
			Database::VM_DBResult       result;
			std::mutex                  resultMutex;
			VM_Texture*                 scrollPane;
			bool                        shouldRefreshRows;
			bool                        shouldRefreshSelected;
			bool                        shouldRefreshThumbs;
			VM_CacheResponses           response;
			VM_TableStates              states;
			std::queue<std::thread::id> thumbThreads;

		public:
			VM_Button*    getButton(const String &buttonID);
			String        getSearch();
			int           getSelectedMediaID();
			String        getSelectedMediaURL();
			String        getSelectedFile();
			VM_Buttons    getSelectedRow();
			int           getSelectedRowIndex();
			String        getSort();
			String        getSQL();
			VM_TableState getState();
			bool          isRowVisible();
			bool          offsetEnd();
			bool          offsetNext();
			bool          offsetPrev();
			bool          offsetStart();
			int           refresh();
			void          refreshRows();
			void          refreshSelected();
			void          refreshThumbs();
			void          removeThumbThread();
			virtual int   render();
			int           resetState(bool resetDataRequest = true);
			void          resetScroll(VM_Component* scrollBar);
			void          restoreState(const VM_TableState &state);
			void          setData();
			void          setHeader();
			int           scroll(int offset);
			void          scrollTo(int row);
			void          selectNext(bool loop = false);
			void          selectPrev(bool loop = false);
			void          selectRandom();
			void          selectRow(int row);
			bool          selectRow(SDL_Event* mouseEvent);
			void          setSearch(const String &searchString, bool saveDB = false);
			int           setRows(bool temp);
			void          sort(const String &buttonID);
			void          updateNavigation();
			void          updateSearchInput();

		private:
			void                  init(const String &id);
			Database::VM_DBResult getResult();
			Database::VM_DBResult getResultLimited(const Database::VM_DBResult &result);
			int                   getRowHeight();
			int                   getRowsPerPage();
			Database::VM_DBResult getTracks(VM_MediaType mediaType);
			void                  resetRows();
			void                  resetScrollPane();
			void                  setRows();
			void                  updateDetailsText(bool temp);

		};
	}
}

#endif
