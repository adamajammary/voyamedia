#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_UPNPFILE_H
#define VM_UPNPFILE_H

namespace VoyaMedia
{
	namespace UPNP
	{
		class VM_UpnpFile
		{
		public:
			VM_UpnpFile();
			~VM_UpnpFile();

		public:
			FILE*         data;
			size_t        duration;
			String        file;
			String        id;
			bool          iTunes;
			VM_MediaType  mediaType;
			std::mutex    mutex;
			String        name;
			int64_t       position;
			size_t        size;
			String        title;
			String        type;
			String        url;

		};
	}
}

#endif
