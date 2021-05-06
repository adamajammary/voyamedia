#ifndef VM_GLOBAL_H
	#include "../Global/VM_Global.h"
#endif

#ifndef VM_TEXT_H
#define VM_TEXT_H

namespace VoyaMedia
{
	namespace System
	{
		class VM_Text
		{
		private:
			VM_Text()  {}
			~VM_Text() {}

		public:
			static String    Decrypt(const String &message);
			static String    Encrypt(const String &message);
			static bool      EndsWith(const String &text, char character);
			static String    EscapeSQL(const String &unescapedString, bool removeQuotes = false);
			static String    EscapeURL(const String &unescapedString);
			static bool      FontSupportsLanguage(const TTF_Font* font, const uint16_t* utf16, size_t size);
			static bool      FontSupportsLanguage(const TTF_Font* font, const String &text);
			static String    GetFileSizeString(const size_t fileSize);
			static String    GetFormattedSeparator(const String &number, const String &separator = " ", int partLength = 3);
			static String    GetFullPath(const String &directory, const String &file);
			static StringMap GetLabels();
			static String    GetLanguage(const String &isoCode);
			static char      GetLastCharacter(const String &text);
			static String    GetMediaAudioLayoutString(const int channelCount);
			static String    GetMediaBitRateString(const int bitRate);
			static String    GetMediaFrameRateString(const int frameRate);
			static String    GetMediaSampleRateString(const int sampleRate);
			static String    GetNewLine(const String &text);
			static String    GetSpaceHolder(const String &text);
			static int       GetSpaceWidth(TTF_Font* font);
			static Strings   GetTags(const String &fullPath);
			static String    GetTimeFormatted(const time_t timeStamp,   bool weekday, bool clock = true);
			static String    GetTimeFormatted(const String &timeString, bool weekday, bool clock = true);
			static time_t    GetTimeStamp(const String &timeString);
			static String    GetToken(String &source, const String &delimiter);
			static String    GetUrlRoot(const String &mediaURL);
			static int       GetWidth(const String &text, TTF_Font* font);
			static String    GetXmlValue(const String &xmlContent, const String &openString, const int openTagLength, const String &closeString);
			static bool      IsValidSubtitle(const String &subtitle);
			static String    Remove(const String &text, const String &removeSubstring);
			static WString   Remove(const WString &text, const WString &removeSubstring);
			static String    Replace(const String &text, const String &oldSubstring, const String &newSubstring);
			static WString   Replace(const WString &text, const WString &oldSubstring, const WString &newSubstring);
			static String    ReplaceHTML(const String &html);
			static Strings   Split(const String &text, const String &delimiter, bool returnEmpty = true);
			static String    ToDuration(const int64_t duration);
			static String    ToLower(const String &text);
			static String    ToViewCount(int64_t views);
			static String    ToUpper(const String &text);
			static String    ToUTF8(const WString &stringUNICODE, bool useWCSTOMBS = true);
			static String    ToUTF8(const String &stringUNICODE);
			static int       ToUTF16(const String &srcASCII, uint16_t* destUTF16, const size_t destSize);
			static WString   ToUTF16(uint8_t* string, uint32_t size);
			static String    Trim(const String &text);
			static String    Wrap(const String &text, TTF_Font* font, const uint32_t wrapLength);

			template<typename... Args>
			static String Format(const char* formatString, const Args&... args)
			{
				if (formatString == NULL)
					return "";

				char buffer[DEFAULT_CHAR_BUFFER_SIZE];
				snprintf(buffer, DEFAULT_CHAR_BUFFER_SIZE, formatString, args...);

				return String(buffer);
			}

			template<typename... Args>
			static WString FormatW(const wchar_t* formatString, const Args&... args)
			{
				if (formatString == NULL)
					return L"";

				wchar_t buffer[DEFAULT_CHAR_BUFFER_SIZE];
				swprintf(buffer, DEFAULT_CHAR_BUFFER_SIZE, formatString, args...);

				return WString(buffer);
			}

			template <class T>
			static bool VectorContains(const std::vector<T> &vector, const T &value)
			{
				return (std::find(vector.begin(), vector.end(), value) != vector.end());
			}

			#if defined _windows
				static wchar_t  GetLastCharacter(const WString &text);
				static WString  GetToken(WString &source, const WString &delimiter);
				static WStrings Split(const WString &text, const WString &delimiter);
				static WString  ToLower(const WString &text);
				static WString  ToUpper(const WString &text);
				static WString  ToUTF16(const String &stringASCII);
				static WString  ToUTF16(const FT_SfntName2 &ftName);
			#endif

		private:
			static String   getTimeFormatted(std::tm* time, bool weekday, bool clock = true);
			static std::tm* getTimeStruct(const String &timeString);
			static uint16_t toUTF8(int char1,  int char2);
			static uint16_t toUTF16(int char1, int char2);
			static uint16_t toUTF16(int char1, int char2, int char3);

			#if defined _windows
				static UINT getCodePage(const FT_SfntName2 &ftName);
			#endif
		};
	}
}

#endif
