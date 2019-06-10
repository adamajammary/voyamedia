#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_DISPLAY_H
#define VM_DISPLAY_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_Display
		{
		public:
			VM_Display();
			~VM_Display() {}

		public:
			float scaleFactor;

		private:
			SDL_DisplayMode display;
			float           dDPI;
			float           hDPI;
			float           vDPI;
			float           scaleFactorDPI;
			float           scaleFactorRes;
			int             displayIndex;

		public:
			SDL_Rect getDimensions();
			int      getDisplayMode();
			float    getDPI();

		};
	}
}

#endif
