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
			bool   dataIsReady;
			bool   dataRequested;
			int    offset;
			String pageToken;
			int    scrollOffset;
			String searchString;
			int    selectedRow;
			String sortColumn;
			String sortDirection;

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
				this->pageToken     = "";
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
			bool                    scrollDrag;

		private:
			int                   limit;
			int                   maxRows;
			String                pageTokenPrev;
			String                pageTokenNext;
			Database::VM_DBResult result;
			std::mutex            resultMutex;
			VM_Texture*           scrollPane;
			bool                  shouldRefreshRows;
			bool                  shouldRefreshSelected;
			bool                  shouldRefreshThumbs;
			VM_CacheResponses     response;
			VM_TableStates        states;

		public:
			VM_Button*    getButton(const String &buttonID);
			String        getSearch();
			int           getSelectedMediaID();
			String        getSelectedMediaURL();
			String        getSelectedFile();
			String        getSelectedShoutCast();
			String        getSelectedYouTube();
			VM_Buttons    getSelectedRow();
			String        getSort();
			String        getSQL();
			VM_TableState getState();
			bool          offsetNext();
			bool          offsetPrev();
			int           refresh();
			void          refreshRows();
			void          refreshSelected();
			void          refreshThumbs();
			virtual int   render();
			int           resetState(bool resetDataRequest = true);
			void          resetScroll(VM_Component* scrollBar);
			void          restoreState(const VM_TableState &state);
			void          setData();
			void          setHeader();
			void          scrollHome();
			void          scrollEnd();
			void          scrollNext();
			void          scrollPrev();
			void          scrollPageNext();
			void          scrollPagePrev();
			bool          scrollPage(SDL_Event* mouseEvent);
			void          scrollTo(int row);
			bool          scrollThumb(SDL_Event* mouseEvent);
			void          selectNext(bool loop = false);
			void          selectPrev(bool loop = false);
			void          selectRandom();
			bool          selectRow(SDL_Event* mouseEvent);
			void          setSearch(const String &searchString, bool saveDB = false);
			int           setRows(bool temp);
			void          sort(const String &buttonID);
			void          updateSearchInput();

		private:
			void                  init(const String &id);
			Database::VM_DBResult getNICs();
			Database::VM_DBResult getResult();
			int                   getRowsPerPage();
			Database::VM_DBResult getShoutCast();
			Database::VM_DBResult getTMDB(VM_MediaType mediaType);
			Database::VM_DBResult getTracks(VM_MediaType mediaType);
			Database::VM_DBResult getUPNP();
			Database::VM_DBResult getYouTube();
			void                  resetRows();
			void                  resetScrollPane();
			int                   scroll(int offset);
			int                   scroll(SDL_Event* mouseEvent);
			bool                  selectRow(int row);
			void                  setRows();

		};
	}
}

#endif
