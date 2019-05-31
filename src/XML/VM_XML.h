#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_XML_H
#define VM_XML_H

namespace VoyaMedia
{
	namespace XML
	{
		class VM_XML
		{
		private:
			VM_XML()  {}
			~VM_XML() {}

		public:
			static String            GetAttribute(LIB_XML::xmlNode* node, const char* attribute);
			static StringMap         GetAttributes(LIB_XML::xmlNode* node);
			static LIB_XML::xmlNode* GetChildNode(LIB_XML::xmlNode* node, const char* child, LIB_XML::xmlDoc* document);
			static VM_XmlNodes       GetChildNodes(LIB_XML::xmlNode* node, LIB_XML::xmlDoc* document);
			static String            GetChildValue(LIB_XML::xmlNode* node, const char* child, LIB_XML::xmlDoc* document);
			static LIB_XML::xmlNode* GetNode(const char* xpath, LIB_XML::xmlDoc* document);
			static VM_XmlNodes       GetNodes(const char* xpath, LIB_XML::xmlDoc* document);
			static String            GetValue(const char* xpath, LIB_XML::xmlDoc* document);
			static String            GetValue(LIB_XML::xmlNode* node, LIB_XML::xmlDoc* document);
			static LIB_XML::xmlDoc*  Load(const char* file);
			static LIB_XML::xmlDoc*  Load(const char* memory, int size);

		};
	}
}

#endif
