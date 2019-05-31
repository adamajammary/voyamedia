#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_PANEL_H
#define VM_PANEL_H

namespace VoyaMedia
{
	namespace Graphics
	{
		class VM_Panel : public VM_Component
		{
		public:
			VM_Panel(const VM_Component &component);
			VM_Panel(const String &id);
			~VM_Panel() {}
	
		public:
			virtual int render();

		};
	}
}

#endif
