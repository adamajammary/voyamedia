#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_TOP_H
#define VM_TOP_H

namespace VoyaMedia
{
	namespace System
	{
		class VM_Top
		{
		public:
			static VM_MediaType Selected;

		private:
			VM_Top()  {}
			~VM_Top() {}

		public:
			static int          Init();
			static String       MediaTypeToId(VM_MediaType mediaType);
			static VM_MediaType IdToMediaType(const String &mediaType);
			static int          Refresh();
			static int          SelectType(VM_MediaType mediaType);

		};
	}
}

#endif
