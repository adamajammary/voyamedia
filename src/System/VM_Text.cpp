#include "VM_Text.h"

using namespace VoyaMedia::MediaPlayer;

String System::VM_Text::Decrypt(const String &message)
{
	String decrypted = String(message.size(), '\0');

	for (uint32_t i = 0; i < message.size(); i++)
		decrypted[i] = (i % (i + 5) == 0 ? message[i] - 7 : message[i] + 3);

	return decrypted;
}

String System::VM_Text::Encrypt(const String &message)
{
	String encrypted = String(message.size(), '\0');

	for (uint32_t i = 0; i < message.size(); i++)
		encrypted[i] = (i % (i + 5) == 0 ? message[i] + 7 : message[i] - 3);

	return encrypted;
}

String System::VM_Text::EscapeSQL(const String &unescapedString, const bool removeQuotes)
{
	char*  escapedStringBuffer = LIB_SQLITE::sqlite3_mprintf("%Q", unescapedString.c_str());
	String escapedString       = String(escapedStringBuffer);

	LIB_SQLITE::sqlite3_free(escapedStringBuffer);
	
	if (removeQuotes && (escapedString.size() > 2))
		escapedString = escapedString.substr(1, (escapedString.size() - 2));
	else if (removeQuotes && unescapedString.empty())
		escapedString = "";

	return escapedString;
}

String System::VM_Text::EscapeURL(const String &unescapedString)
{
	String escapedString = "";
	char   hex[3];

	for (auto c : unescapedString)
	{
		if (!std::isalnum(c, std::locale())) {
			snprintf(hex, 3, "%X", (c < 0 ? c + 256 : c));
			escapedString.append("%").append(hex);
		} else {
			escapedString.append({ c });
		}
	}

	return escapedString;
}

#if defined _windows
// https://docs.microsoft.com/en-us/windows/desktop/intl/code-page-identifiers
// https://www.freetype.org/freetype2/docs/reference/ft2-sfnt_names.html#FT_SfntName
// https://github.com/AgoraIO-Usecase/HQ/blob/master/libobs-for-HQ-Windows/plugins/text-freetype2/find-font-windows.c
UINT System::VM_Text::getCodePage(const FT_SfntName2 &ftName)
{
	switch (ftName.platform_id) {
	case TT_PLATFORM_APPLE_UNICODE:
		return 1201;
	case TT_PLATFORM_MACINTOSH:
		switch (ftName.encoding_id) {
		case TT_MAC_ID_JAPANESE:
			return 932;
		case TT_MAC_ID_SIMPLIFIED_CHINESE:
			return 936;
		case TT_MAC_ID_TRADITIONAL_CHINESE:
			return 950;
		case TT_MAC_ID_ARABIC:
			switch (ftName.language_id) {
			case TT_MAC_LANGID_URDU:
			case TT_MAC_LANGID_FARSI:
				return 0;
			default:
				return 10004;
			}
		case TT_MAC_LANGID_HEBREW:
			return 10005;
		case TT_MAC_ID_GREEK:
			return 10006;
		case TT_MAC_ID_RUSSIAN:
			return 10007;
		case TT_MAC_ID_ROMAN:
			switch (ftName.language_id) {
			case TT_MAC_LANGID_ROMANIAN:
				return 10010;
			case TT_MAC_LANGID_POLISH:
			case TT_MAC_LANGID_CZECH:
			case TT_MAC_LANGID_SLOVAK:
				return 10029;
			case TT_MAC_LANGID_ICELANDIC:
				return 10079;
			case TT_MAC_LANGID_TURKISH:
				return 10081;
			default:
				return 10000;
			}
		case TT_MAC_ID_KOREAN:
			return 51949;
		default:
			break;
		}
	case TT_PLATFORM_ISO:
		switch (ftName.encoding_id) {
		case TT_MAC_ID_ROMAN:
			return 20127;
		case TT_MAC_ID_TRADITIONAL_CHINESE:
			return 28591;
		default:
			break;
		}
	case TT_PLATFORM_MICROSOFT:
		switch (ftName.encoding_id) {
		case TT_MAC_ID_ROMAN:
		case TT_MAC_ID_JAPANESE:
		case TT_MAC_ID_GURMUKHI:
			return 1201;
		case TT_MAC_ID_TRADITIONAL_CHINESE:
			return 932;
		case TT_MAC_ID_ARABIC:
			return 950;
		default:
			break;
		}
	}

	return 0;
}
#endif

bool System::VM_Text::FontSupportsLanguage(const TTF_Font* font, const uint16_t* utf16, size_t size)
{
	if ((font == NULL) || (utf16 == NULL) || (size == 0))
		return false;

	for (size_t i = 0; ((i < size) && (utf16[i] != 0)); i++) {
		if (!TTF_GlyphIsProvided(font, utf16[i]))
			return false;
	}

	return true;
}

bool System::VM_Text::FontSupportsLanguage(const TTF_Font* font, const String &text)
{
	if ((font == NULL) || text.empty())
		return false;

	for (auto c : text) {
		if (!TTF_GlyphIsProvided(font, c))
			return false;
	}

	return true;
}

String System::VM_Text::GetFileSizeString(const size_t fileSize)
{
	char fileSizeString[DEFAULT_CHAR_BUFFER_SIZE];

	if (fileSize == 0ll)
		return "N/A";

	if (fileSize > TERRA_BYTE) {
		snprintf(fileSizeString, DEFAULT_CHAR_BUFFER_SIZE, "%.2f TB", (float)((float)fileSize / (float)TERRA_BYTE));
	} else if (fileSize > GIGA_BYTE) {
		snprintf(fileSizeString, DEFAULT_CHAR_BUFFER_SIZE, "%.2f GB", (float)((float)fileSize / (float)GIGA_BYTE));
	} else if (fileSize > MEGA_BYTE) {
		snprintf(fileSizeString, DEFAULT_CHAR_BUFFER_SIZE, "%.2f MB", (float)((float)fileSize / (float)MEGA_BYTE));
	} else if (fileSize > KILO_BYTE) {
		snprintf(fileSizeString, DEFAULT_CHAR_BUFFER_SIZE, "%.2f KB", (float)((float)fileSize / (float)KILO_BYTE));
	} else {
		snprintf(fileSizeString, DEFAULT_CHAR_BUFFER_SIZE, "%zu bytes", fileSize);
	}

	return String(fileSizeString);
}

String System::VM_Text::GetFormattedSeparator(const String &number, const String &separator, int partLength)
{
	String  nr = "";
	Strings tempSplit;

	for (int i = (int)number.size(), j = 0; i >= 0; i--, j++) {
		if ((j > 0) && (j % partLength == 0))
			tempSplit.push_back(number.substr(i, 3));
	}

	if ((int)number.size() % partLength != 0)
		tempSplit.push_back(number.substr(0, number.size() % partLength));

	for (const auto &temp : tempSplit)
		nr = (temp + separator + nr);

	if (!nr.empty())
		nr = nr.substr(0, nr.rfind(separator));

	return nr;
}

String System::VM_Text::GetFullPath(const String &directory, const String &file)
{
	return String(directory + (VM_Text::GetLastCharacter(directory) != PATH_SEPERATOR_C ? PATH_SEPERATOR : "") + file);
}

StringMap System::VM_Text::GetLabels()
{
	StringMap labels;
	String    lang = "en";

	#if defined _windows
		String languageFilePath = VM_Text::ToUTF8(VM_FileSystem::GetPathLanguagesW().c_str(), false);
	#else
		String languageFilePath = VM_FileSystem::GetPathLanguages();
	#endif

	if (VM_Window::SystemLocale)
	{
		String locale = VM_Text::GetLocale();

		// "nb-NO", "nn-NO", "no", "Norwegian Bokmaal_Norway.1252"
		if ((locale.substr(0, 2) == "nb") || (locale.substr(0, 2) == "nn") || (locale.substr(0, 2) == "no"))// {
			lang = "no";
	} else {
		std::setlocale(LC_ALL, "C");
	}

	if (!lang.empty())
		languageFilePath.append(lang + ".lang");

	Strings       pair;
	std::ifstream languageFile(languageFilePath);

	if (languageFile.good())
	{
		String line;

		// GET LABEL/STRING PAIRS LINE-BY-LINE
		while (std::getline(languageFile, line))
		{
			if (line.empty() || line[0] == '#')
				continue;

			pair = VM_Text::Split(line, "=");

			if (pair.size() > 1)
				labels.insert({ pair[0], VM_Text::Replace(pair[1], "\\n", "\n") });
		}
	}

	languageFile.close();

	return labels;
}

String System::VM_Text::GetLanguage(const String &isoCode)
{
	if ((isoCode.size() < 2) || (isoCode.size() > 3))
		return isoCode;

	String iso      = VM_Text::ToLower(isoCode);
	String language = iso;

	#if defined _windows
		String filePath = VM_Text::ToUTF8(VM_FileSystem::GetPathLanguagesW().c_str(), false);
	#else
		String filePath = VM_FileSystem::GetPathLanguages();
	#endif

	std::ifstream isoFile(filePath + "iso_codes.lang");

	if (isoFile.good())
	{
		String  line;
		Strings row;
		int     column = (iso.size() == 2 ? 0 : 1);

		while (std::getline(isoFile, line))
		{
			if (line.empty() || line[0] == '#')
				continue;

			row = VM_Text::Split(line, ";");

			if ((row.size() > 3) && (row[column] == iso))
			{
				if (row[2] != row[3])
					language = (row[2] + " - " + row[3]);
				else
					language = row[2];

				break;
			}
		}
	}

	isoFile.close();

	return language;
}

char System::VM_Text::GetLastCharacter(const String &text)
{
	return (!text.empty() ? text[text.size() - 1] : '\0');
}

#if defined _windows
wchar_t System::VM_Text::GetLastCharacter(const WString &text)
{
	return (!text.empty() ? text[text.size() - 1] : '\0');
}
#endif

String System::VM_Text::GetLocale()
{
	String locale = "C";

	#if defined _android
		jclass    jniClass       = VM_Window::JNI->getClass();
		JNIEnv*   jniEnvironment = VM_Window::JNI->getEnvironment();
		jmethodID jniGetLocale   = jniEnvironment->GetStaticMethodID(jniClass, "GetLocale", "()Ljava/lang/String;");

		if (jniGetLocale == NULL)
			return locale;

		jstring     jString = (jstring)jniEnvironment->CallStaticObjectMethod(jniClass, jniGetLocale);
		const char* cString = jniEnvironment->GetStringUTFChars(jString, NULL);

		locale = VM_Text::ToLower(cString);
		
		jniEnvironment->ReleaseStringUTFChars(jString, cString);
	#elif defined _ios || defined _macosx
		locale = VM_Text::ToLower([[[NSLocale preferredLanguages] objectAtIndex:0] UTF8String]);
	#else
		locale = VM_Text::ToLower(std::setlocale(LC_ALL, ""));
	#endif

	return locale;
}

String System::VM_Text::GetMediaAudioLayoutString(const int channelCount)
{
	String layout = "";

	switch (channelCount) {
		case 1: layout = "1.0"; break;
		case 2: layout = "2.0"; break;
		case 3: layout = "2.1"; break;
		case 4: layout = "3.1"; break;
		case 5: layout = "4.1"; break;
		case 6: layout = "5.1"; break;
		case 7: layout = "6.1"; break;
		case 8: layout = "7.1"; break;
		default: layout = "N/A"; break;
	}

	return layout;
}

String System::VM_Text::GetMediaBitRateString(const int bitRate)
{
	if (bitRate <= 0)
		return "N/A";

	char bitRateString[DEFAULT_CHAR_BUFFER_SIZE];

	if (bitRate > ONE_MILLION) {
		snprintf(bitRateString, DEFAULT_CHAR_BUFFER_SIZE, "%.1f Mbps", (float)((float)bitRate / (float)ONE_MILLION));
	} else if (bitRate > ONE_THOUSAND) {
		snprintf(bitRateString, DEFAULT_CHAR_BUFFER_SIZE, "%.0f kbps", (float)((float)bitRate / (float)ONE_THOUSAND));
	} else {
		snprintf(bitRateString, DEFAULT_CHAR_BUFFER_SIZE, "%d bps", bitRate);
	}

	return String(bitRateString);
}

String System::VM_Text::GetMediaFrameRateString(const int frameRate)
{
	if (frameRate <= 0)
		return "N/A";

	char frameRateString[DEFAULT_CHAR_BUFFER_SIZE];
	snprintf(frameRateString, DEFAULT_CHAR_BUFFER_SIZE, "%d fps", frameRate);

	return String(frameRateString);
}

String System::VM_Text::GetMediaSampleRateString(const int sampleRate)
{
	char sampleRateString[DEFAULT_CHAR_BUFFER_SIZE];

	if (sampleRate <= 0)
		return "N/A";

	if (sampleRate > 1000000) {
		snprintf(sampleRateString, DEFAULT_CHAR_BUFFER_SIZE, "%.1f MHz", (float)((float)sampleRate / (float)ONE_MILLION));
	} else if (sampleRate > 1000) {
		snprintf(sampleRateString, DEFAULT_CHAR_BUFFER_SIZE, "%.1f kHz", (float)((float)sampleRate / (float)ONE_THOUSAND));
	} else {
		snprintf(sampleRateString, DEFAULT_CHAR_BUFFER_SIZE, "%d Hz", sampleRate);
	}

	return String(sampleRateString);
}

String System::VM_Text::GetNewLine(const String &text)
{
	String formattedString = String(text);

	if (!text.empty())
		formattedString.append("\n");

	return formattedString;
}

String System::VM_Text::GetSpaceHolder(const String &text)
{
	if (text.empty())
		return " ";

	return text;
}

Strings System::VM_Text::GetTags(const String &fullPath)
{
	String  directoryPath, file = String(fullPath), pathSeperator, title;
	Strings fileDetails, tags;

	// BLURAY / DVD: "concat:streamPath|stream1|stream2|streamN|duration|title|audioTrackCount|subTrackCount|"
	if (VM_FileSystem::IsConcat(file)) {
		pathSeperator = PATH_SEPERATOR;
		file          = file.substr(7);
		fileDetails   = VM_Text::Split(file, "|");
		tags          = VM_Text::Split(fileDetails[0], pathSeperator);
		title         = fileDetails[fileDetails.size() - 3];
		title         = title.substr(title.find("[ ") + 2);
		title         = title.substr(0, title.find(" ]"));
		tags.push_back(title);

		return tags;
	} else {
		pathSeperator = PATH_SEPERATOR;
	}

	// C:\Path\Sub\Path\File.ext => C:\Path\Sub\Path
	directoryPath = file.substr(0, file.rfind(pathSeperator));

	// C:\Path\Sub\Path => C:, Path, Sub, Path
	tags = VM_Text::Split(directoryPath, pathSeperator);

	return tags;
}

String System::VM_Text::GetTimeFormatted(const time_t timeStamp, bool weekday, bool clock)
{
	return VM_Text::getTimeFormatted(localtime(&timeStamp), weekday, clock);
}

String System::VM_Text::GetTimeFormatted(const String &timeString, bool weekday, bool clock)
{
	if (timeString.size() < 19)
		return "";

	std::tm* timeStruct = VM_Text::getTimeStruct(timeString);

	if (timeStruct == NULL)
		return "";

	return VM_Text::getTimeFormatted(timeStruct, weekday, clock);
}

String System::VM_Text::getTimeFormatted(std::tm* time, bool weekday, bool clock)
{
	if (time == NULL)
		return "";

	// FORMAT USING SYSTEM LOCALE IF ENABLED
	#if defined _android
		if (VM_Window::SystemLocale)
		{
			jclass    jniClass            = VM_Window::JNI->getClass();
			JNIEnv*   jniEnvironment      = VM_Window::JNI->getEnvironment();
			jmethodID jniGetDateFormatted = jniEnvironment->GetStaticMethodID(jniClass, "GetDateFormatted",  "(ZZ)Ljava/lang/String;");

			if (jniGetDateFormatted == NULL)
				return "";

			jstring     jString    = (jstring)jniEnvironment->CallStaticObjectMethod(jniClass, jniGetDateFormatted, weekday, clock);
			const char* cString    = jniEnvironment->GetStringUTFChars(jString, NULL);
			String      timeString = String(cString);
			
			jniEnvironment->ReleaseStringUTFChars(jString, cString);

			return timeString;
		}
		else
		{
			char timeString[DEFAULT_CHAR_BUFFER_SIZE];

			if (weekday)
				std::strftime(timeString, DEFAULT_CHAR_BUFFER_SIZE, "%A, %d. %B %Y %H:%M", time);
			else if (clock)
				std::strftime(timeString, DEFAULT_CHAR_BUFFER_SIZE, "%d. %B %Y %H:%M", time);
			else
				std::strftime(timeString, DEFAULT_CHAR_BUFFER_SIZE, "%d. %B %Y", time);

			return String(timeString);
		}
	#elif defined _ios || defined _macosx
		NSAutoreleasePool* pool   = [[NSAutoreleasePool alloc] init];
		NSDate*            date   = [NSDate dateWithTimeIntervalSince1970:std::mktime(time)];
		NSDateFormatter*   format = [[NSDateFormatter alloc] init];

		if (weekday)
			[format setDateFormat:@"EEEE, d. MMMM yyyy HH:mm"];
		else if (clock)
			[format setDateFormat:@"d. MMMM yyyy HH:mm"];
		else
			[format setDateFormat:@"d. MMMM yyyy"];

		if (VM_Window::SystemLocale)
			[format setLocale:[[[NSLocale alloc] initWithLocaleIdentifier:[[NSLocale preferredLanguages] objectAtIndex:0]] autorelease]];

		wchar_t timeString[DEFAULT_WCHAR_BUFFER_SIZE];
		std::swprintf(timeString, DEFAULT_WCHAR_BUFFER_SIZE, L"%s", [[format stringFromDate:date] UTF8String]);

		[pool release];

		return VM_Text::ToUTF8(timeString);
	#else
		if (VM_Window::SystemLocale)
			std::setlocale(LC_ALL, "");

		wchar_t timeString[DEFAULT_WCHAR_BUFFER_SIZE];

		if (weekday)
			std::wcsftime(timeString, DEFAULT_WCHAR_BUFFER_SIZE, L"%A, %d. %B %Y %H:%M", time);
		else if (clock)
			std::wcsftime(timeString, DEFAULT_WCHAR_BUFFER_SIZE, L"%d. %B %Y %H:%M", time);
		else
			std::wcsftime(timeString, DEFAULT_WCHAR_BUFFER_SIZE, L"%d. %B %Y", time);

		std::setlocale(LC_ALL, "C");
		
		return VM_Text::ToUTF8(timeString);
	#endif
}

time_t System::VM_Text::GetTimeStamp(const String &timeString)
{
	std::tm* timeStruct = VM_Text::getTimeStruct(timeString);

	if (timeStruct == NULL)
		return 0;

	time_t timeStamp = std::mktime(timeStruct);
	DELETE_POINTER(timeStruct);

	return timeStamp;
}

// POINTER ALLOCATED WITH C++ new - REMEMBER TO RELEASE WITH delete
std::tm* System::VM_Text::getTimeStruct(const String &timeString)
{
	std::tm* timeStruct = new std::tm();

	// CREATE A TIME STRUCTURE
	if (timeString.size() >= 10) {
		timeStruct->tm_year = (std::atoi(timeString.substr(0, 4).c_str()) - 1900);
		timeStruct->tm_mon  = (std::atoi(timeString.substr(5, 2).c_str()) - 1);
		timeStruct->tm_mday = std::atoi(timeString.substr(8, 2).c_str());
	} else {
		DELETE_POINTER(timeStruct);
		return NULL;
	}

	if (timeString.size() >= 19) {
		timeStruct->tm_hour = std::atoi(timeString.substr(11, 2).c_str());
		timeStruct->tm_min  = std::atoi(timeString.substr(14, 2).c_str());
		timeStruct->tm_sec  = std::atoi(timeString.substr(17, 2).c_str());
	} else {
		timeStruct->tm_hour = timeStruct->tm_min = timeStruct->tm_sec = 0;
	}

	if ((timeStruct->tm_year < 0) || (timeStruct->tm_mon < 0)) {
		DELETE_POINTER(timeStruct);
		return NULL;
	}

	return timeStruct;
}

// PS! Updates source by removing token from the start of the String
String System::VM_Text::GetToken(String &source, const String &delimiter)
{
	size_t delimPos;
	String token;

	delimPos = strcspn(source.c_str(), delimiter.c_str());
	token    = source.substr(0, delimPos);
	source   = source.substr(delimPos + (delimPos < source.size() ? 1 : 0));

	return token;
}

// PS! Updates source by removing token from the start of the String
#if defined _windows
WString System::VM_Text::GetToken(WString &source, const WString &delimiter)
{
	size_t  delimPos;
	WString token;

	delimPos = wcscspn(source.c_str(), delimiter.c_str());
	token    = source.substr(0, delimPos);
	source   = source.substr(delimPos + (delimPos < source.size() ? 1 : 0));

	return token;
}
#endif

String System::VM_Text::GetUrlRoot(const String &mediaURL)
{
	String root;
	root = VM_Text::Replace(mediaURL, "//", "_?_?_"); root = root.substr(0, root.find("/")); root = VM_Text::Replace(root, "_?_?_", "//");
	return root;
}

String System::VM_Text::GetXmlValue(const String &xmlContent, const String &openString, const int openTagLength, const String &closeString)
{
	size_t substringStart;
	String xmlValue = "";
	
	if (xmlContent.empty() || openString.empty() || closeString.empty() || (openTagLength <= 0)) { return ""; }

	substringStart = xmlContent.find(openString);

	if (substringStart != String::npos) {
		substringStart += (size_t)openTagLength;

		if (xmlContent.size() > substringStart) {
			xmlValue = xmlContent.substr(substringStart); xmlValue = xmlValue.substr(0, xmlValue.find(closeString));
	}}

	return xmlValue;
}

bool System::VM_Text::IsValidSubtitle(const String &subtitle)
{
	return ((subtitle.rfind("\\p0")  == String::npos) && (subtitle.rfind("\\p1") == String::npos) &&
			(subtitle.rfind("\\p2")  == String::npos) && (subtitle.rfind("\\p4") == String::npos) &&
			(subtitle.rfind("\\pbo") == String::npos));
}

String System::VM_Text::Remove(const String &text, const String &removeSubstring)
{
	return VM_Text::Replace(text, removeSubstring, "");;
}

WString System::VM_Text::Remove(const WString &text, const WString &removeSubstring)
{
	return VM_Text::Replace(text, removeSubstring, L"");;
}

String System::VM_Text::Replace(const String &text, const String &oldSubstring, const String &newSubstring)
{
	String formattedString = String(text);
	size_t lastPosition;

	lastPosition = formattedString.find(oldSubstring);

	while (lastPosition != String::npos) {
		formattedString = formattedString.replace(lastPosition, oldSubstring.size(), newSubstring);
		lastPosition    = formattedString.find(oldSubstring, (lastPosition + newSubstring.size()));
	}

	return formattedString;
}

WString System::VM_Text::Replace(const WString &text, const WString &oldSubstring, const WString &newSubstring)
{
	WString formattedString = WString(text);
	size_t  lastPosition;

	lastPosition = formattedString.find(oldSubstring);

	while (lastPosition != WString::npos) {
		formattedString = formattedString.replace(lastPosition, oldSubstring.size(), newSubstring);
		lastPosition    = formattedString.find(oldSubstring, (lastPosition + newSubstring.size()));
	}

	return formattedString;
}

Strings System::VM_Text::Split(const String &text, const String &delimiter, bool returnEmpty)
{
	Strings parts;
	String  fullString = String(text);

	if (fullString.empty())
		return parts;

	if (fullString.find(delimiter) == String::npos) {
		parts.push_back(fullString);
		return parts;
	}

	while (!fullString.empty())
	{
		String part = VM_Text::GetToken(fullString, delimiter);

		if (!part.empty() || returnEmpty)
			parts.push_back(part);
	}

	return parts;
}

#if defined _windows
WStrings System::VM_Text::Split(const WString &text, const WString &delimiter)
{
	WStrings parts;
	WString fullString = WString(text);

	if (fullString.empty())
		return parts;

	if (fullString.find(delimiter) == WString::npos) {
		parts.push_back(fullString);
		return parts;
	}

	while (!fullString.empty())
		parts.push_back(VM_Text::GetToken(fullString, delimiter));

	return parts;
}
#endif

String System::VM_Text::ToDuration(int64_t duration)
{
	if (duration == 0)
		return "N/A";

	char      durationStringBuffer[DEFAULT_CHAR_BUFFER_SIZE];
	VM_MediaTime durationObject;

	durationObject = VM_MediaTime((double)duration);

	if (durationObject.hours > 0)
	{
		if (durationObject.seconds > 0)
			snprintf(durationStringBuffer, DEFAULT_CHAR_BUFFER_SIZE, "%dh %dm %ds", durationObject.hours, durationObject.minutes, durationObject.seconds);
		else
			snprintf(durationStringBuffer, DEFAULT_CHAR_BUFFER_SIZE, "%dh %dm", durationObject.hours, durationObject.minutes);
	}
	else if (durationObject.minutes > 0)
	{
		if (durationObject.seconds > 0)
			snprintf(durationStringBuffer, DEFAULT_CHAR_BUFFER_SIZE, "%dm %ds", durationObject.minutes, durationObject.seconds);
		else
			snprintf(durationStringBuffer, DEFAULT_CHAR_BUFFER_SIZE, "%dm", durationObject.minutes);
	} else {
		snprintf(durationStringBuffer, DEFAULT_CHAR_BUFFER_SIZE, "%llds", duration);
	}

	return String(durationStringBuffer);
}

String System::VM_Text::ToDuration(const String &text)
{
	String duration = "";

	// YOUTUBE: "PT1H23M45S"
	if ((text.size() > 2) && (text.substr(0, 2) == "PT"))
	{
		duration = text.substr(2);
		duration = VM_Text::Replace(duration, "H", "h ");
		duration = VM_Text::Replace(duration, "M", "m ");
		duration = VM_Text::Replace(duration, "S", "s ");
	} else {
		duration = "N/A";
	}

	return duration;
}

String System::VM_Text::ToViewCount(int64_t views)
{
	char duration[DEFAULT_CHAR_BUFFER_SIZE];

	if (views >= 0)
	{
		if (views > TERRA)
			snprintf(duration, DEFAULT_CHAR_BUFFER_SIZE, "%.1fT", (float)((float)views / (float)TERRA));
		else if (views > GIGA)
			snprintf(duration, DEFAULT_CHAR_BUFFER_SIZE, "%.1fG", (float)((float)views / (float)GIGA));
		else if (views > MEGA)
			snprintf(duration, DEFAULT_CHAR_BUFFER_SIZE, "%.1fM", (float)((float)views / (float)MEGA));
		else if (views > KILO)
			snprintf(duration, DEFAULT_CHAR_BUFFER_SIZE, "%.1fK", (float)((float)views / (float)KILO));
		else
			snprintf(duration, DEFAULT_CHAR_BUFFER_SIZE, "%lld", views);
	} else {
		snprintf(duration, DEFAULT_CHAR_BUFFER_SIZE, "N/A");
	}

	return String(duration);
}

String System::VM_Text::ToLower(const String &text)
{
	String lower = String(text);

	for (int i = 0; i < (int)lower.size(); i++)
		lower[i] = tolower(lower[i]);

	return lower;
}

#if defined _windows
WString System::VM_Text::ToLower(const WString &text)
{
	WString lower = WString(text);

	for (int i = 0; i < (int)lower.size(); i++)
		lower[i] = tolower(lower[i]);

	return lower;
}
#endif

String System::VM_Text::ToUpper(const String &text)
{
	String upper = String(text);

	for (int i = 0; i < (int)upper.size(); i++)
		upper[i] = toupper(upper[i]);

	return upper;
}

#if defined _windows
WString System::VM_Text::ToUpper(const WString &text)
{
	WString upper = WString(text);

	for (int i = 0; i < (int)upper.size(); i++)
		upper[i] = toupper(upper[i]);

	return upper;
}
#endif

uint16_t System::VM_Text::toUTF8(int char1, int char2)
{
	uint16_t charUTF8;

	if (char1 < 0)                      { char1 += 256; }
	if (char2 < 0)                      { char2 += 256; }
	if ((char1 < 194) || (char1 > 195)) { return (uint16_t)char1; }

	// http://www.utf8-chartable.de/unicode-utf8-table.pl
	// 8-BIT UNICODE (2 CHARS):	0x0080	194 128 -> 0x00FF	195 191
	charUTF8 = 0x0080;

	for (int char1Value = 194; char1Value <= 195; char1Value++) {
		for (int char2Value = 128; char2Value <= 191; char2Value++) {
			if ((char1Value == char1) && (char2Value == char2)) { return charUTF8; }
			charUTF8++;
	}}

	return (uint16_t)char1;
}

// http://www.utf8-chartable.de/unicode-utf8-table.pl?utf8=dec
// http://www.utf8-chartable.de/unicode-utf8-table.pl?start=9728&utf8=dec
String System::VM_Text::ToUTF8(const String &stringUNICODE)
{
	String    hexString   = VM_Text::Replace(stringUNICODE, "\\u", "0x");
	uint16_t* bufferUTF16 = (uint16_t*)malloc((stringUNICODE.size() + 1) * sizeof(uint16_t));

	for (int i = 0, j = 0; i < (int)hexString.size();)
	{
		if (i > 0 && hexString[i - 1] == '0' && hexString[i] == 'x') {
			bufferUTF16[--j] = (uint16_t)strtoul(hexString.substr(i - 1, 6).c_str(), NULL, 16);
			i += 5;
		} else {
			bufferUTF16[j] = hexString[i];
			i++;
		}

		j++;
	}

	bufferUTF16[stringUNICODE.size()] = 0;

	WString stringUTF16 = WString(reinterpret_cast<wchar_t*>(bufferUTF16));
	FREE_POINTER(bufferUTF16);

	return VM_Text::ToUTF8(stringUTF16, false);
}

String System::VM_Text::ToUTF8(const WString &stringUNICODE, bool useWCSTOMBS)
{
	if (stringUNICODE.empty())
		return "";

	String stringUTF8;

	if (useWCSTOMBS) {
		char*  stringBuffer = (char*)malloc((stringUNICODE.size() + 1) * sizeof(char));
		size_t sizeUTF8     = std::wcstombs(stringBuffer, stringUNICODE.c_str(), stringUNICODE.size());

		stringBuffer[sizeUTF8] = 0;

		stringUTF8 = String(stringBuffer);
		FREE_POINTER(stringBuffer);
	} else {
		char* stringBuffer = SDL_iconv_string(
			"UTF-8", "UTF-16LE",
			reinterpret_cast<const char*>(stringUNICODE.c_str()),
			(stringUNICODE.size() + 1) * sizeof(wchar_t)
		);

		stringUTF8 = String(stringBuffer);
		SDL_free(stringBuffer);
	}

	return stringUTF8;
}

#if defined _windows
WString System::VM_Text::ToUTF16(const String &stringASCII)
{
	if (stringASCII.empty())
		return L"";

	uint16_t* bufferUTF16 = (uint16_t*)malloc((stringASCII.size() + 1) * sizeof(uint16_t));
	VM_Text::ToUTF16(stringASCII, bufferUTF16, stringASCII.size());

	WString stringUTF16 = WString(reinterpret_cast<wchar_t*>(bufferUTF16));
	FREE_POINTER(bufferUTF16);

	return stringUTF16;
}

WString System::VM_Text::ToUTF16(const FT_SfntName2 &ftName)
{
	wchar_t* bufferUTF16 = NULL;
	UINT     codePage    = VM_Text::getCodePage(ftName);
	size_t   sizeUTF16   = MultiByteToWideChar(codePage, 0, reinterpret_cast<char*>(ftName.string), ftName.string_len, NULL, 0);

	if (sizeUTF16 == 0)
		return L"";

	bufferUTF16 = (wchar_t*)malloc((sizeUTF16 + 1) * sizeof(wchar_t));
	sizeUTF16   = MultiByteToWideChar(codePage, 0, reinterpret_cast<char*>(ftName.string), ftName.string_len, bufferUTF16, (int)sizeUTF16);

	bufferUTF16[sizeUTF16] = 0;

	WString stringUTF16 = WString(bufferUTF16);
	FREE_POINTER(bufferUTF16);

	return stringUTF16;
}
#endif

WString System::VM_Text::ToUTF16(uint8_t* string, uint32_t size)
{
	// Convert string to UTF-16: { 0, 97, 0, 98 } => { 97, 98 } => { 'a', 'b' }
	std::vector<uint16_t> bufferUTF16;

	for (uint32_t i = 1; i < size; i += 2)
		bufferUTF16.push_back((uint8_t)string[i] | ((uint8_t)string[i - 1] << 8));

	bufferUTF16.push_back(0);

	return WString(reinterpret_cast<wchar_t*>(bufferUTF16.data()));
}

uint16_t System::VM_Text::toUTF16(int char1, int char2)
{
	uint16_t charUTF16;

	if (char1 < 0)                      { char1 += 256; }
	if (char2 < 0)                      { char2 += 256; }
	if ((char1 < 196) || (char1 > 223)) { return (uint16_t)char1; }

	// http://www.utf8-chartable.de/unicode-utf8-table.pl
	// 16-BIT UNICODE (2 CHARS):	0x0100	196 128 -> 0x07FF	223 191
	charUTF16 = 0x0100;

	for (int char1Value = 196; char1Value <= 223; char1Value++) {
		for (int char2Value = 128; char2Value <= 191; char2Value++) {
			if ((char1Value == char1) && (char2Value == char2))
				return charUTF16;

			charUTF16++;
	}}

	return (uint16_t)char1;
}

uint16_t System::VM_Text::toUTF16(int char1, int char2, int char3)
{
	uint16_t charUTF16;

	if (char1 < 0)                      { char1 += 256; }
	if (char2 < 0)                      { char2 += 256; }
	if (char3 < 0)                      { char3 += 256; }
	if ((char1 < 224) || (char1 > 239)) { return (uint16_t)char1; }

	// http://www.utf8-chartable.de/unicode-utf8-table.pl
	// 16-BIT UNICODE (3 CHARS):	0x0800	224 160 128 -> 0xFFFF	239 191 191
	charUTF16 = 0x0800;

	for (int char1Value = 224; char1Value <= 239; char1Value++) {
		for (int char2Value = 128; char2Value <= 191; char2Value++) {
			if ((char1Value == 224) && (char2Value < 160)) { continue; }

			for (int char3Value = 128; char3Value <= 191; char3Value++) {
				if ((char1Value == char1) && (char2Value == char2) && (char3Value == char3))
					return charUTF16;

				charUTF16++;
	}}}

	return (uint16_t)char1;
}

int System::VM_Text::ToUTF16(const String &srcASCII, uint16_t* destUTF16, const size_t destSize)
{
	int      charTemp;
	uint32_t incrementASCII;
	uint32_t charIndexUTF16 = 0;

	if (srcASCII.empty())
		return 0;

	// Convert ASCII characters to UTF8: { 196, 128 } => 0x0100 => 'Ā'
	for (size_t charIndexASCII = 0; charIndexASCII < srcASCII.size(); charIndexASCII++)
	{
		if (charIndexASCII >= destSize)
			break;

		incrementASCII = 0;
		charTemp       = srcASCII[charIndexASCII];

		if (charTemp < 0)
			charTemp += 256;
		
		destUTF16[charIndexUTF16] = charTemp;

		// CONVERT 2 ASCII CHARACTERS TO UTF8: { 194, 161 } => 0x00A1 => '¡'
		if ((charTemp >= 194) && (charTemp <= 195) && ((charIndexASCII + 1) < srcASCII.size())) {
			destUTF16[charIndexUTF16] = VM_Text::toUTF8(srcASCII[charIndexASCII], srcASCII[charIndexASCII + 1]);
			incrementASCII = 1;
		// CONVERT 2 ASCII CHARACTERS TO UTF16: { 196, 128 } => 0x0100 => 'Ā'
		} else if ((charTemp >= 196) && (charTemp <= 223) && ((charIndexASCII + 1) < srcASCII.size())) {
			destUTF16[charIndexUTF16] = VM_Text::toUTF16(srcASCII[charIndexASCII], srcASCII[charIndexASCII + 1]);
			incrementASCII = 1;
		// CONVERT 3 ASCII CHARACTERS TO UTF16: { 224, 162, 160 } => 0x08A0 => 'ࢠ'
		} else if ((charTemp >= 224) && (charTemp <= 239) && ((charIndexASCII + 2) < srcASCII.size())) {
			destUTF16[charIndexUTF16] = VM_Text::toUTF16(srcASCII[charIndexASCII], srcASCII[charIndexASCII + 1], srcASCII[charIndexASCII + 2]);
			incrementASCII = 2;
		}

		// INCREMENT BY AN EXTRA CHAR POS OR TWO IF TO_UTF8/TO_UTF16 CONVERTED 2/3 CHARS TO 1 CHAR
		if (destUTF16[charIndexUTF16] != charTemp)
			charIndexASCII += incrementASCII;

		// INCREMENT TO THE NEXT AVAILABLE CHAR POS
		charIndexUTF16++;
	}

	destUTF16[charIndexUTF16] = 0;

	return charIndexUTF16;
}

String System::VM_Text::Trim(const String &text)
{
	String stringTrimmed = String(text);

	// TRIM FRONT
	while (!stringTrimmed.empty() && std::isspace(stringTrimmed[0], std::locale()))
		stringTrimmed = stringTrimmed.substr(1);

	// TRIM END
	while (!stringTrimmed.empty() && std::isspace(stringTrimmed[stringTrimmed.size() - 1], std::locale()))
		stringTrimmed = stringTrimmed.substr(0, stringTrimmed.size() - 1);
	
	return stringTrimmed;
}

String System::VM_Text::Wrap(const String &text, TTF_Font* font, const uint32_t wrapLength)
{
	String  text2 = "";
	Strings lines = VM_Text::Split(text, "\n");

	for (const auto &line : lines)
	{
		Strings words = VM_Text::Split(line, " ");

		for (const auto &word : words)
		{
			int w, h;
			TTF_SizeUTF8(font, word.c_str(), &w, &h);

			String word2 = "";

			if (w > (int)wrapLength)
			{
				float percent   = ((float)(wrapLength - 20) / (float)w);
				int   maxLength = (int)floor((float)word.size() * percent);

				for (int i = 0; i < (int)word.size(); i++)
				{
					if ((i % maxLength != 0) || (i == 0))
						word2.append({ word[i] });
					else
						word2.append({ word[i], '\n' });	// WRAP
				}
			} else {
				word2 = String(word);
			}

			text2.append(word2 + " ");
		}

		text2.append("\n");
	}

	return text2;
}
