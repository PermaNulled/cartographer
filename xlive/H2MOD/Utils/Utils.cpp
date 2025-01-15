#include "stdafx.h"
#include "Utils.h"

#include "H2MOD/Modules/Shell/Config.h"
#include "H2MOD/Modules/OnScreenDebug/OnscreenDebug.h"

#include <sys/timeb.h>

int FindLineStart(FILE* fp, int lineStrLen) {
	int fp_offset_orig = ftell(fp);
	for (int i = lineStrLen; i < 255; i++) {
		if (fp_offset_orig - i < 0) {
			fseek(fp, fp_offset_orig, SEEK_SET);
			return 0;
		}
		fseek(fp, fp_offset_orig - i, SEEK_SET);
		int c = 0;
		if ((c = fgetc(fp)) == '\r' || c == '\n') {
			fseek(fp, fp_offset_orig, SEEK_SET);
			return fp_offset_orig - i + 1;
		}
	}
	fseek(fp, fp_offset_orig, SEEK_SET);
	return fp_offset_orig - lineStrLen;
}

bool GetFileLine(FILE* fp, char** fileLine) {
	bool moreToGo = true;
	int c, fileLineLen = 256, fileLineCursor = 0;
	*fileLine = (char*)malloc(fileLineLen);
	while (c = fgetc(fp)) {
		if (c == EOF) {
			moreToGo = false;
			break;
		}
		else if (c == '\r') {

		}
		else if (c == '\n') {
			break;
		}
		else {
			(*fileLine)[fileLineCursor++] = c;
		}
		if (fileLineCursor >= fileLineLen - 2) {
			fileLineLen += 256;
			*fileLine = (char*)realloc(*fileLine, fileLineLen);
		}
	}
	(*fileLine)[fileLineCursor++] = 0;
	if (strlen(*fileLine) == 0) {
		free(*fileLine);
		*fileLine = 0;
	}
	return moreToGo || *fileLine != 0;
}

bool GetNTStringLine(char* text, int lineNum, char** line) {
	int inc_line_num = 0;
	char* line_begin = text;

	while (inc_line_num++ < lineNum) {
		line_begin = strchr(line_begin, '\n');
		if (line_begin++ == 0) {
			return false;
		}
	}

	size_t line_len = 0;
	char* line_next = strchr(line_begin, '\n');
	if (line_next == 0) {
		line_len = strlen(line_begin);
	}
	else {
		if (*(line_next - 1) == '\r') {
			line_next--;
		}
		line_len = line_next - line_begin;
	}

	if (line_len <= 0) {
		*line = NULL;
		return true;
	}

	*line = (char*)malloc(sizeof(char) * (line_len + 1));

	csmemcpy(*line, line_begin, line_len);
	(*line)[line_len] = 0;

	return true;
}

//if missing versioning parameters, major will always be on leftmost side.
char CmpVersions(const char* version_base, const char* version_alt) {

	if (strcmp(version_base, version_alt) == 0)
		return 0b00000;//same

	int versions[2][4];
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 4; i++) {
			versions[j][i] = 0;
		}
	}

	if (sscanf_s(version_base, "%d.%d.%d.%d", &versions[0][0], &versions[0][1], &versions[0][2], &versions[0][3]) <= 0) {
		return 0b10000;//invalid
	}

	if (sscanf_s(version_alt, "%d.%d.%d.%d", &versions[1][0], &versions[1][1], &versions[1][2], &versions[1][3]) <= 0) {
		return 0b10000;//invalid
	}

	for (int i = 0; i < 4; i++) {
		if (versions[0][i] == versions[1][i]) {
			continue;
		}
		else if (versions[0][i] > versions[1][i]) {//alt is old
			return 0b10000 | (0b1000 >> i);
		}
		else {//alt is new
			return 0b00000 | (0b1000 >> i);
		}
	}

	return 0b00000;//same
	//return 0b01000;//new major
	//return 0b00100;//new minor
	//return 0b00010;//new revision
	//return 0b00001;//new build
	//return 0b11000;//old major
	//return 0b10100;//old minor
	//return 0b10010;//old revision
	//return 0b10001;//old build
}

void ReadIniFile(void* fileConfig, bool configIsFILE, const char* header, const char* headerVersion, int(interpretSettingFunc)(char* fileLine, char* version, int lineNumber)) {
	bool foundFirstHeader = false;
	char version[30] = "0";
	bool keepReading = true;
	int lineNumber = 0;
	char* fileLine;
	while (keepReading && ((configIsFILE && GetFileLine((FILE*)fileConfig, &fileLine)) || (!configIsFILE && GetNTStringLine((char*)fileConfig, lineNumber, &fileLine)))) {
		lineNumber++;
		if (fileLine) {
			if (fileLine[0] == header[0] && sscanf_s(fileLine, header, &version, 30)) {
				foundFirstHeader = true;
				addDebugText("Found header on line %d asserting version: %s", lineNumber, version);
				if (CmpVersions(headerVersion, version) == 0) {//does not send this line to interpreter.
					free(fileLine);
					continue;
				}
				else {
					addDebugText("Incorrect version! Continue searching!");
				}
			}
			int rtnCode = interpretSettingFunc(fileLine, foundFirstHeader ? version : 0, lineNumber);
			if (!(rtnCode & 0b10)) {
				free(fileLine);
				if (rtnCode & 0b1)
					keepReading = false;
			}
		}
	}
}

std::string GetVKeyCodeString(int vkey) {
	std::ostringstream strStream;
	strStream << "0x" << std::hex << vkey;
	if (vkey >= 0x70 && vkey <= 0x87) {
		int func_num = vkey - 0x70 + 1;
		strStream << " - VK_F" << std::dec << func_num;
	}
	else if (vkey == 0x24) {
		strStream << " - VK_Home";
	}
	return strStream.str();
}

void PadCStringWithChar(char* strToPad, size_t toFullLength, char c) {
	for (size_t i = strlen(strToPad); i < toFullLength - 1; i++) {
		strToPad[i] = c;
	}
	strToPad[toFullLength - 1] = '\0';
}

int GetWidePathFromFullWideFilename(const wchar_t* filepath, wchar_t* rtnpath) {
	const wchar_t* backslash_offset = wcsrchr(filepath, L'\\');
	const wchar_t* forward_slash_offset = wcsrchr(filepath, L'/');
	const wchar_t* most_right_slash_offset = 
		backslash_offset == NULL ? forward_slash_offset : ((forward_slash_offset != NULL && backslash_offset < forward_slash_offset) ? forward_slash_offset : backslash_offset);

	if (most_right_slash_offset != NULL) {
		wcsncpy(rtnpath, filepath, (most_right_slash_offset - filepath) + 1);
		return 0;
	}

	return -1;
}

LONG GetDWORDRegKey(HKEY hKey, wchar_t* strValueName, DWORD* nValue) {
	DWORD dwBufferSize(sizeof(DWORD));
	DWORD nResult(0);
	LONG nError = ::RegQueryValueExW(hKey,
		strValueName,
		0,
		NULL,
		reinterpret_cast<LPBYTE>(&nResult),
		&dwBufferSize);
	if (ERROR_SUCCESS == nError)
	{
		*nValue = nResult;
	}
	return nError;
}

char* custom_label_literal(char* label_escaped) {
	int label_escaped_length = strlen(label_escaped);
	int length_shift = 0;
	char* label_literal = (char*)malloc(label_escaped_length + 1);

	for (int i = 0; i < label_escaped_length; i++) {
		if (label_escaped[i] == '\\' && label_escaped[i + 1] == '\\') {
			label_literal[i + length_shift] = '\\';
			length_shift--;
			i++;
		}
		else if (label_escaped[i] == '\\' && label_escaped[i + 1] == 'n') {
			label_literal[i + length_shift] = '\n';
			length_shift--;
			i++;
		}
		else if (label_escaped[i] == '\\' && label_escaped[i + 1] == 'r') {
			label_literal[i + length_shift] = '\r';
			length_shift--;
			i++;
		}
		else {
			label_literal[i + length_shift] = label_escaped[i];
		}
	}

	label_literal[label_escaped_length + length_shift] = 0;
	return label_literal;
}

char* custom_label_escape(char* label_literal) {
	int label_literal_length = strlen(label_literal);
	int length_shift = 0;
	int label_escaped_length = label_literal_length + length_shift + 2;
	char* label_escaped = (char*)malloc(label_escaped_length);

	for (int i = 0; i < label_literal_length; i++) {
		if (label_escaped_length < i + length_shift + 2) {
			label_escaped = (char*)realloc(label_escaped, label_literal_length + length_shift + 2);
		}
		if (label_literal[i] == '\\') {
			label_escaped[i + length_shift] = '\\';
			length_shift++;
			label_escaped[i + length_shift] = '\\';
		}
		else if (label_literal[i] == '\n') {
			label_escaped[i + length_shift] = '\\';
			length_shift++;
			label_escaped[i + length_shift] = 'n';
		}
		else if (label_literal[i] == '\r') {
			label_escaped[i + length_shift] = '\\';
			length_shift++;
			label_escaped[i + length_shift] = 'r';
		}
		else {
			label_escaped[i + length_shift] = label_literal[i];
		}
	}

	label_escaped[label_literal_length + length_shift] = 0;
	return label_escaped;
}

bool FloatIsNaN(float vagueFloat) {
	DWORD* vague = (DWORD*)&vagueFloat;
	if ((*vague >= 0x7F800000 && *vague <= 0x7FFFFFFF) || (*vague >= 0xFF800000 && *vague <= 0xFFFFFFFF)) {
		return true;
	}
	return false;
}

bool isFloat(std::string myString)
{
	std::istringstream iss(myString);
	float f;
	iss >> std::noskipws >> f; // noskipws considers leading whitespace invalid
	// Check the entire string was consumed and if either failbit or badbit is set
	return iss.eof() && !iss.fail();
}
bool isFloat(std::wstring myString)
{
	std::wistringstream iss(myString);
	float f;
	iss >> std::noskipws >> f; // noskipws considers leading whitespace invalid
	// Check the entire string was consumed and if either failbit or badbit is set
	return iss.eof() && !iss.fail();
}

bool isInteger(std::string myString)
{
	return myString.find_first_not_of("0123456789") == std::string::npos;
}

bool isInteger(std::wstring myString)
{
	return myString.find_first_not_of(L"0123456789") == std::wstring::npos;
}

int HostnameToIp(char* hostname, char* ip) {
	struct hostent *he;
	struct in_addr **addr_list;
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	if ((he = gethostbyname(hostname)) == NULL) {
		return WSAGetLastError();
	}

	addr_list = (struct in_addr **) he->h_addr_list;

	for (int i = 0; addr_list[i] != NULL; i++)
	{
		//Return the first one;
		strncpy(ip, inet_ntoa(*addr_list[i]), 100);
		return 0;
	}

	return 1;
}

DWORD crc32buf(const char* buf, size_t len)
{
	DWORD oldcrc32;

	oldcrc32 = 0xFFFFFFFF;

	for (; len; --len, ++buf)
	{
		oldcrc32 = UPDC32(*buf, oldcrc32);
	}

	return ~oldcrc32;
}

bool ComputeFileCrc32Hash(wchar_t* filepath, DWORD &rtncrc32) {

	DWORD oldcrc32;

	oldcrc32 = 0xFFFFFFFF;

	const int BUFSIZE = 1024;
	BOOL bResult = FALSE;
	HANDLE hFile = NULL;
	BYTE rgbFile[BUFSIZE];
	DWORD cbRead = 0;

	hFile = CreateFile(filepath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		return false;
	}

	while (bResult = ReadFile(hFile, rgbFile, BUFSIZE, &cbRead, NULL))
	{
		if (cbRead == 0) {
			break;
		}

		for (unsigned int i = 0; i < cbRead; i++) {
			oldcrc32 = UPDC32(rgbFile[i], oldcrc32);
		}
	}

	CloseHandle(hFile);

	if (!bResult) {
		return false;
	}

	rtncrc32 = ~oldcrc32;

	return true;
}

static bool rfc3986_allow(char i) {
	//isalnum(i)//PoS crashes
	if ((i >= '0' && i <= '9') || 
		(i >= 'a' && i <= 'z') ||
		(i >= 'A' && i <= 'Z') ||
		i == '~' || i == '-' || i == '.' || i == '_')
		return true;
	return false;
}

char* encode_rfc3986(const char* label_literal, size_t label_literal_length) {
	if (label_literal_length == 0)
		label_literal_length = strlen(label_literal);
	int escaped_buflen = (label_literal_length * 3) + 1;
	char* label_escaped = (char*)malloc(escaped_buflen * sizeof(char));
	int escaped_buff_i = 0;

	for (size_t i = 0; i < label_literal_length; i++) {
		unsigned char uletter = label_literal[i];
		if (!rfc3986_allow(uletter)) {
			sprintf_s(label_escaped + escaped_buff_i, 4, "%%%02X", uletter);
			escaped_buff_i += 2;
		}
		else {
			label_escaped[escaped_buff_i] = label_literal[i];
		}
		escaped_buff_i++;
	}
	label_escaped[escaped_buff_i] = 0;
	return label_escaped;
}

void wcstombs2(wchar_t* source, char* out_buffer, size_t buf_len) {
	size_t loop_len = wcslen(source);
	if (loop_len >= buf_len)
		loop_len = buf_len - 1; // - 1 for null

	size_t out_buffer_idx = 0;
	for (size_t i = 0; i < loop_len; i++) {
		if (source[i] >= 0 && source[i] <= 0xFF) {
			out_buffer[out_buffer_idx++] = (char)source[i];
		}
	}
	out_buffer[out_buffer_idx] = 0;
}

std::string ToNarrow(const wchar_t *s, char dfault,	const std::locale& loc)
{
	std::ostringstream stm;

	while (*s != L'\0') {
		stm << std::use_facet< std::ctype<wchar_t> >(loc).narrow(*s++, dfault);
	}
	return stm.str();
}

char* wcstombs2r(wchar_t* text)
{
	size_t output_size = (wcslen(text) + 1) * sizeof(char);
	char* output = (char*)malloc(output_size);
	wcstombs2(text, output, output_size);
	return output;
}

struct stringMe {
	char *ptr;
	size_t len;
};

static void init_string(struct stringMe *s) {
	s->len = 0;
	s->ptr = (char*)malloc(s->len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}

static size_t writefunc(void *ptr, size_t size, size_t nmemb, struct stringMe *s)
{
	size_t new_len = s->len + size*nmemb;
	s->ptr = (char*)realloc(s->ptr, new_len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	csmemcpy(s->ptr + s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size*nmemb;
}

int MasterHttpResponse(const char* url, const char* http_request, char** rtn_response) {
	TEST_N_DEF(LC1);
	int result = ERROR_CODE_CURL_SOCKET_FAILED;//Socket failed to connect to server

	CURL *curl;
	CURLcode res;

	/* get a curl handle */
	curl = curl_interface_init_no_verify();
	if (curl) {
		/* First set the URL that is about to receive our POST. This URL can
		just as well be a https:// URL if that is what should receive the
		data. */
		curl_easy_setopt(curl, CURLOPT_URL, url);

		struct stringMe s;
		init_string(&s);

		//FIXME: <Insert Pinned Public Key Here>

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

		/* Now specify the POST data */
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, http_request);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK) {
			result = ERROR_CODE_CURL_EASY_PERF;//curl_easy_perform() issue
			addDebugText("curl_easy_perform() failed: %s", curl_easy_strerror(res));
			free(s.ptr);
		}
		else {
			*rtn_response = s.ptr;
			result = 0;//successful connection.
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	else {
		result = ERROR_CODE_CURL_HANDLE;//curl handle fail
	}

	return result;
}

void CreateDirTree(const wchar_t* path) {
	size_t buflen = wcslen(path) + 1;
	wchar_t* temp_path = (wchar_t*)malloc(sizeof(wchar_t) * buflen);
	wcscpy_s(temp_path, buflen, path);

	for (size_t i = 1; i < buflen; i++) {
		if (temp_path[i] == L'/' || temp_path[i] == L'\\') {
			wchar_t temp_cut = 0;
			if (temp_path[i + 1] != 0) {
				temp_cut = temp_path[i + 1];
				temp_path[i + 1] = 0;
			}
			CreateDirectoryW(temp_path, NULL);
			if (temp_cut) {
				temp_path[i + 1] = temp_cut;
			}
		}
	}

	free(temp_path);
}

int TrimRemoveConsecutiveSpaces(char* text) {
	int text_pos = 0;
	size_t text_len = strlen(text);
	for (size_t j = 0; j < text_len; j++) {
		if (text_pos == 0) {
			if (text[j] != ' ')
				text[text_pos++] = text[j];
			continue;
		}
		if (!(text[j] == ' ' && text[text_pos - 1] == ' '))
			text[text_pos++] = text[j];
	}
	text[text_pos] = 0;
	if (text[text_pos - 1] == ' ')
		text[--text_pos] = 0;
	return text_pos;//new length
}

int stripWhitespace(wchar_t *inputStr) {
	wchar_t *start;
	start = inputStr;
	while (*start && *start == ' ') start++;
	printf("It is %p and %p\n", inputStr, start);
	printf("Strlen + 1 is: %i\n", wcslen(start) + 1);
	memmove(inputStr, start, wcslen(start) + 1);
	return 0;
}

void pushHostLobby() {
	typedef bool(__cdecl* should_send_broadcast_reply_t)(void* session);
	auto p_should_send_broadcast_reply = Memory::GetAddress<should_send_broadcast_reply_t>(0x1ADA7B, 0x1A69DB);

	if (p_should_send_broadcast_reply(NULL))
	{
		char msg[100] = { 0x00, 0x43, 0x05 };
		sprintf(&msg[3], "push clientlobby %d", H2Config_base_port + 1);

		addDebugText("Pushing open lobby.");

		SOCKET socketDescriptor;
		struct sockaddr_in serverAddress;
		if ((socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
			addDebugText("ERROR: Could not create socket.");
		}
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = H2Config_master_ip;
		serverAddress.sin_port = htons(H2Config_master_port_relay);

		if (sendto(socketDescriptor, msg, strlen(&msg[3]) + 3, 0, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) <= 0) {
			//returns -1 if it wasn't successful. Note that it doesn't return -1 if the connection couldn't be established (UDP)
			addDebugText("ERROR: Failed to push open lobby.");
		}

		closesocket(socketDescriptor);
	}
}

int GetCurrentTimeMS()
{
	timeb tb;
	ftime(&tb);
	int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
	return nCount;
}

int TimeElapsedMS(int startms)
{
	int nSpan = GetCurrentTimeMS() - startms;
	if (nSpan < 0)
		nSpan += 0x100000 * 1000;
	return nSpan;
}

bool FileTypeCheck(const std::string& file_path, const std::string& file_type)
{
	auto a = file_path.substr(file_path.find_last_of('.') + 1, file_path.length() - file_path.find_last_of('.') - 1);
	return a == file_type;
}

// possible format: '0Xx01234556789ABCDEFabcdefHh'
bool HexStrToBytes(const char* hexStr, size_t hexStrLen, uint8_t* outByteBuf, size_t outBufLen)
{
	// ASCII character index map to hex value
	static const uint8_t lutStrToHex[23] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0,
		0, 0, 0, 10, 11, 12, 13, 14, 15
	};

	if (hexStr == NULL
		|| outByteBuf == NULL)
	{
		return false;
	}

	const uint8_t* byteBufEnd = outByteBuf + outBufLen;
	hexStrLen = hexStrLen != 0u ? hexStrLen : strlen(hexStr);

	// handle prefixes
	if (hexStrLen > 2
		&& hexStr[0] == '0' && (hexStr[1] == 'x' || hexStr[1] == 'X'))
	{
		hexStr += 2;
		hexStrLen -= 2;
	}

	// handle suffixes
	if (hexStrLen > 1
		&& (hexStr[hexStrLen - 1] == 'h' || (hexStr[hexStrLen - 1] == 'H')))
	{
		hexStrLen -= 1;
	}

	// continue only if string is at least 1
	if (!(hexStrLen >= 1u))
	{
		return false;
	}

	bool hexStrOddLen = (hexStrLen % 2) != 0;
	const char tmpBuf1[3] = { '0', hexStr[0], '\0' };
	const char* hexStrBeg = &hexStr[0];
	if (hexStrOddLen)
	{
		// if we're dealing with a odd hex string bytes number
		// just add the 0 in front
		hexStrLen += 1;
		hexStrBeg = tmpBuf1;
	}

	// check if we have enough byte buffer
	if (hexStrLen / 2 > outBufLen)
	{
		return false;
	}

	bool result_success = true;
	for (size_t i = 0; i < hexStrLen; i += 2)
	{
		int upper = toupper(hexStrBeg[i]);
		int lower = toupper(hexStrBeg[i + 1]);

		if (!isxdigit(upper) || !isxdigit(lower))
		{
			result_success = false;
			break;
		}

		*outByteBuf++ = (uint8_t)((lutStrToHex[upper - '0'] << 4) | (lutStrToHex[lower - '0']));
		hexStrBeg = hexStr; // get to the next byte, remember this might be on tmpBuf1 ptr if we're dealing with hexStrOddLen
	}

	return result_success;
}

// TODO: this function checks the input hexStr before converting it to bytes
// hence being slower if we don't trust the input
bool HexStrToBytes(const std::string& hexStr, uint8_t* byteBuf, size_t bufLen) {
	return HexStrToBytes(hexStr.c_str(), hexStr.length(), byteBuf, bufLen);
}

std::string ByteToHexStr(const uint8_t* buffer, size_t size) {
	std::stringstream str;
	str.fill('0');
	str.setf(std::ios_base::uppercase);
	str.setf(std::ios_base::hex, std::ios::basefield);
	for (size_t i = 0; i < size; i++) {
		str << std::setw(2) << (uint16_t)buffer[i];
	}
	return str.str();
}
