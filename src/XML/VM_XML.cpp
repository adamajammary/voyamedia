#include "VM_XML.h"

using namespace VoyaMedia::System;

String XML::VM_XML::GetAttribute(LIB_XML::xmlNode* node, const char* attribute)
{
	String result = "";

	if ((node == NULL) || (attribute == NULL))
		return result;

	LIB_XML::xmlAttr* attrib = node->properties;

	while (attrib != NULL)
	{
		if (LIB_XML::xmlStrcmp(attrib->name, reinterpret_cast<const LIB_XML::xmlChar*>(attribute)) == 0)
		{
			result = String(reinterpret_cast<char*>(attrib->xmlChildrenNode->content));
			break;
		}

		attrib = attrib->next;
	}

	return result;
}

StringMap XML::VM_XML::GetAttributes(LIB_XML::xmlNode* node)
{
	StringMap result;

	if (node == NULL)
		return result;

	LIB_XML::xmlAttr* attrib = node->properties;

	while (attrib != NULL) {
		result.insert({
			String((char*)attrib->name),
			String(reinterpret_cast<char*>(attrib->xmlChildrenNode->content))
		});

		attrib = attrib->next;
	}

	return result;
}

LIB_XML::xmlNode* XML::VM_XML::GetChildNode(LIB_XML::xmlNode* node, const char* child, LIB_XML::xmlDoc* document)
{
	LIB_XML::xmlNode* result = NULL;

	if ((node == NULL) || (child == NULL) || (document == NULL))
		return result;

	LIB_XML::xmlNode* childNode = node->xmlChildrenNode;

	while (childNode != NULL)
	{
		if (LIB_XML::xmlStrcmp(childNode->name, reinterpret_cast<const LIB_XML::xmlChar*>(child)) == 0) {
			result = childNode;
			break;
		}

		childNode = childNode->next;
	}

	return result;
}

XML::VM_XmlNodes XML::VM_XML::GetChildNodes(LIB_XML::xmlNode* node, LIB_XML::xmlDoc* document)
{
	VM_XmlNodes result;

	if ((node == NULL) || (document == NULL))
		return result;

	LIB_XML::xmlNode* childNode = node->xmlChildrenNode;

	while (childNode != NULL) {
		result.push_back(childNode);
		childNode = childNode->next;
	}

	return result;
}

String XML::VM_XML::GetChildValue(LIB_XML::xmlNode* node, const char* child, LIB_XML::xmlDoc* document)
{
	String result = "";

	if ((node == NULL) || (child == NULL) || (document == NULL))
		return result;

	LIB_XML::xmlNode* childNode = node->xmlChildrenNode;

	while (childNode != NULL)
	{
		if (LIB_XML::xmlStrcmp(childNode->name, reinterpret_cast<const LIB_XML::xmlChar*>(child)) == 0)
		{
			LIB_XML::xmlChar* childValue = xmlNodeListGetString(
				document, childNode->xmlChildrenNode, 1
			);

			result = String(reinterpret_cast<char*>(childValue));

			LIB_XML::xmlFree(childValue);

			break;
		}

		childNode = childNode->next;
	}

	return result;
}

LIB_XML::xmlNode* XML::VM_XML::GetNode(const char* xpath, LIB_XML::xmlDoc* document)
{
	LIB_XML::xmlNode* result = NULL;

	if ((xpath == NULL) || (document == NULL))
		return result;

	LIB_XML::xmlXPathContext* context = LIB_XML::xmlXPathNewContext(document);

	if (context == NULL)
		return result;

	LIB_XML::xmlXPathObject* object = LIB_XML::xmlXPathEvalExpression(
		reinterpret_cast<const LIB_XML::xmlChar*>(xpath), context
	);

	LIB_XML::xmlXPathFreeContext(context);

	if (object == NULL)
		return result;

	if (object->nodesetval->nodeNr > 0)
		result = object->nodesetval->nodeTab[0];

	LIB_XML::xmlXPathFreeObject(object);

	return result;
}

XML::VM_XmlNodes XML::VM_XML::GetNodes(const char* xpath, LIB_XML::xmlDoc* document)
{
	VM_XmlNodes result;

	if ((xpath == NULL) || (document == NULL))
		return result;

	LIB_XML::xmlXPathContext* context = LIB_XML::xmlXPathNewContext(document);

	if (context == NULL)
		return result;

	LIB_XML::xmlXPathObject* object = LIB_XML::xmlXPathEvalExpression(
		reinterpret_cast<const LIB_XML::xmlChar*>(xpath), context
	);

	LIB_XML::xmlXPathFreeContext(context);

	if (object == NULL)
		return result;

	for (int i = 0; i < object->nodesetval->nodeNr; i++) {
		if (object->nodesetval->nodeTab[i] != NULL)
			result.push_back(object->nodesetval->nodeTab[i]);
	}

	LIB_XML::xmlXPathFreeObject(object);

	return result;
}

String XML::VM_XML::GetValue(const char* xpath, LIB_XML::xmlDoc* document)
{
	String result = "";

	if ((xpath == NULL) || (document == NULL))
		return result;

	LIB_XML::xmlXPathContext* context = xmlXPathNewContext(document);

	if (context == NULL)
		return result;

	LIB_XML::xmlXPathObject* object = LIB_XML::xmlXPathEvalExpression(
		reinterpret_cast<const LIB_XML::xmlChar*>(xpath), context
	);

	LIB_XML::xmlXPathFreeContext(context);

	if (object == NULL)
		return result;

	if (object->nodesetval->nodeNr > 0)
	{
		LIB_XML::xmlChar* node = LIB_XML::xmlNodeListGetString(document, object->nodesetval->nodeTab[0]->xmlChildrenNode, 1);

		if (node != NULL)
			result = String(reinterpret_cast<char*>(node));

		LIB_XML::xmlFree(node);
	}

	LIB_XML::xmlXPathFreeObject(object);

	return result;
}

String XML::VM_XML::GetValue(LIB_XML::xmlNode* node, LIB_XML::xmlDoc* document)
{
	String result = "";

	if ((node == NULL) || (document == NULL))
		return result;

	LIB_XML::xmlChar* value = xmlNodeListGetString(document, node->children, 1);

	if (value != NULL)
		result = String(reinterpret_cast<char*>(value));

	LIB_XML::xmlFree(value);

	return result;
}

LIB_XML::xmlDoc* XML::VM_XML::Load(const char* file)
{
	return (file != NULL ? LIB_XML::xmlParseFile(file) : NULL);
}

LIB_XML::xmlDoc* XML::VM_XML::Load(const char* memory, int size)
{
	LIB_XML::xmlDoc* document = NULL;

	if ((memory == NULL) || (size <= 0))
		return NULL;

	String response = VM_Text::Replace(memory, "xmlns=", "ns=");
	document        = LIB_XML::xmlParseMemory(response.c_str(), (int)response.size());

	return document;
}
