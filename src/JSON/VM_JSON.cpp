#include "VM_JSON.h"

std::vector<LIB_JSON::json_t*> JSON::VM_JSON::GetArray(LIB_JSON::json_t* object)
{
	std::vector<LIB_JSON::json_t*> result;

	if ((object == NULL) || (object->child == NULL) || (object->child->type != LIB_JSON::JSON_ARRAY))
		return result;

	LIB_JSON::json_t* jsonArray = object->child;

	if ((jsonArray->child == NULL) || (jsonArray->child->type != LIB_JSON::JSON_OBJECT))
		return result;

	LIB_JSON::json_t* arrayObject = jsonArray->child;

	while (arrayObject != NULL) {
		result.push_back(arrayObject);
		arrayObject = arrayObject->next;
	}

	return result;
}

std::vector<double> JSON::VM_JSON::GetArrayNumbers(LIB_JSON::json_t* object)
{
	std::vector<double> result;

	if ((object == NULL) || (object->child == NULL) || (object->child->type != LIB_JSON::JSON_ARRAY))
		return result;

	LIB_JSON::json_t* jsonArray = object->child;

	if ((jsonArray->child == NULL) || (jsonArray->child->type != LIB_JSON::JSON_NUMBER))
		return result;

	LIB_JSON::json_t* arrayObject = jsonArray->child;

	while (arrayObject != NULL) {
		result.push_back(std::atof(arrayObject->text));
		arrayObject = arrayObject->next;
	}

	return result;
}

std::vector<String> JSON::VM_JSON::GetArrayStrings(LIB_JSON::json_t* object)
{
	std::vector<String> result;

	if ((object == NULL) || (object->child == NULL) || (object->child->type != LIB_JSON::JSON_ARRAY))
		return result;

	LIB_JSON::json_t* jsonArray = object->child;

	if ((jsonArray->child == NULL) || (jsonArray->child->type != LIB_JSON::JSON_STRING))
		return result;

	LIB_JSON::json_t* arrayObject = jsonArray->child;

	while (arrayObject != NULL) {
		result.push_back(arrayObject->text);
		arrayObject = arrayObject->next;
	}

	return result;
}

std::vector<LIB_JSON::json_t*> JSON::VM_JSON::GetItems(LIB_JSON::json_t* object)
{
	std::vector<LIB_JSON::json_t*> result;

	if (object->child == NULL)
		return result;

	LIB_JSON::json_t* item = object->child;

	while (item != NULL) {
		result.push_back(item);
		item = item->next;
	}

	return result;
}

LIB_JSON::json_t* JSON::VM_JSON::GetItem(LIB_JSON::json_t* object, const char* item)
{
	if ((object != NULL) && (item != NULL))
		return LIB_JSON::json_find_first_label(object, item);

	return NULL;
}

String JSON::VM_JSON::GetKey(LIB_JSON::json_t* object)
{
	if ((object != NULL) && (object->text != NULL))
		return String(object->text);

	return "";
}

double JSON::VM_JSON::GetValueNumber(LIB_JSON::json_t* object)
{
	if ((object != NULL) && (object->child != NULL) && (object->child->type == LIB_JSON::JSON_NUMBER))
		return std::atof(object->child->text);

	return 0;
}

String JSON::VM_JSON::GetValueString(LIB_JSON::json_t* object)
{
	if ((object != NULL) && (object->child != NULL) && (object->child->type == LIB_JSON::JSON_STRING))
		return String(object->child->text);

	return "";
}

LIB_JSON::json_t* JSON::VM_JSON::Parse(const char* text)
{
	if (text == NULL)
		return NULL;

	LIB_JSON::json_t*    document = NULL;
	LIB_JSON::json_error error    = LIB_JSON::json_parse_document(&document, text);

	if (error != LIB_JSON::JSON_OK) {
		FREE_JSON_DOC(document);
		return NULL;
	}

	return document;
}
