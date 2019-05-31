#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_JSON_H
#define VM_JSON_H

namespace VoyaMedia
{
	namespace JSON
	{
		class VM_JSON
		{
		private:
			VM_JSON()  {}
			~VM_JSON() {}

		public:
			static std::vector<LIB_JSON::json_t*> GetArray(LIB_JSON::json_t* object);
			static std::vector<double>            GetArrayNumbers(LIB_JSON::json_t* object);
			static std::vector<String>            GetArrayStrings(LIB_JSON::json_t* object);
			static std::vector<LIB_JSON::json_t*> GetItems(LIB_JSON::json_t* object);
			static LIB_JSON::json_t*              GetItem(LIB_JSON::json_t* object, const char* item);
			static String                         GetKey(LIB_JSON::json_t* object);
			static double                         GetValueNumber(LIB_JSON::json_t* object);
			static String                         GetValueString(LIB_JSON::json_t* object);
			static LIB_JSON::json_t*              Parse(const char* text);

		};
	}
}

#endif
