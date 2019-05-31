#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_BORDER_H
#define VM_BORDER_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_Border
		{
		public:
			VM_Border();
			VM_Border(int borderWidth);
			VM_Border(int bottom, int left, int right, int top);
			~VM_Border() {}

		public:
			int bottom;
			int left;
			int right;
			int top;

		};
	}
}

#endif
