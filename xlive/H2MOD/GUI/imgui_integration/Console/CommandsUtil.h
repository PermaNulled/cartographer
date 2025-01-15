#pragma once

struct StringLineHeader;
typedef int StringHeaderFlags;

#define MAX_CONSOLE_INPUT_BUFFER 256

static bool InputTextContainsCommandSubstring(const char* command, const char* input_text, bool test_only_first_token)
{
	char input_text_upper_temp[MAX_CONSOLE_INPUT_BUFFER];
	char comand_text_upper[MAX_CONSOLE_INPUT_BUFFER];

	strncpy(input_text_upper_temp, input_text, MAX_CONSOLE_INPUT_BUFFER - 1);
	input_text_upper_temp[MAX_CONSOLE_INPUT_BUFFER - 1] = '\0';

	strncpy(comand_text_upper, command, MAX_CONSOLE_INPUT_BUFFER - 1);
	comand_text_upper[MAX_CONSOLE_INPUT_BUFFER - 1] = '\0';

	_strupr(input_text_upper_temp);
	_strupr(comand_text_upper);

	char* next_token_to_test = NULL;
	char* text_to_test = input_text_upper_temp;

	while (text_to_test != NULL)
	{
		// first we get the next token to test
		// and remove the spaces in the buffer
		char* input_text_space = strstr(text_to_test, " ");
		if (input_text_space == NULL)
		{
			next_token_to_test = NULL;
		}
		else
		{
			// remove the space and test the next string after the space
			*input_text_space = '\0';
			input_text_space++;
			next_token_to_test = input_text_space;
		}

		if (strstr(comand_text_upper, text_to_test) != NULL)
			return true;

		if (test_only_first_token)
			return false;

		// at the end set the next substring to test from the input, if we have spaces in it
		text_to_test = next_token_to_test;

		// check if there's nothing after the space, if so return false, since we didn't find anything close
		if ((text_to_test - input_text_upper_temp) >= MAX_CONSOLE_INPUT_BUFFER)
		{
			return false;
		}
	}

	// if we got this far, no match was found
	return false;
}

template<typename T>
static inline bool tokenize(const char* str, size_t str_length, const char* delimiters, std::vector<T>& out, int tokenize_count = 0)
{
    out.clear();
    size_t beg, pos = 0;
    std::string _str(str, str_length);
    while ((beg = _str.find_first_not_of(delimiters, pos)), beg != std::string::npos)
    {
        pos = _str.find_first_of(delimiters, beg);
        out.push_back(_str.substr(beg, pos - beg));
        if (tokenize_count > 0 && out.size() == tokenize_count)
            break;
    }

    return !out.empty();
}

struct StringLineHeader
{
    size_t idx;
    size_t size;
    StringHeaderFlags flags;
};

enum StringHeaderFlags_
{
    StringFlag_None,
    StringFlag_History = 1 << 0, // when going through history with key up/key down, it'll be displayed in the input console
    StringFlag_CopyToClipboard = 1 << 1, // when set, it'll copy this string to clipboard, then unset the flag
};

// pretty much a circular buffer
// not thread-safe
class CircularStringBuffer
{
public:
	// initial buffer size
	size_t m_line_buf_size;
	unsigned int m_line_count;

	size_t GetBufferSize() const { return m_line_count * m_line_buf_size; }
	size_t GetLineBufferSize() const { return m_line_buf_size; }

	CircularStringBuffer(unsigned int _line_count, size_t _max_buffer_size_per_line)
	{
		m_line_count = _line_count;
		m_line_buf_size = _max_buffer_size_per_line;

		// allocate once we know the size
		m_buf = (char*)malloc(GetBufferSize());
		csmemset(m_buf, 0, GetBufferSize());
		m_buffer_idx = 0;
		m_strings_headers.reserve(m_line_count);
	};

	CircularStringBuffer(const CircularStringBuffer& other)
		: CircularStringBuffer(other.m_line_count, other.m_line_buf_size)
	{
	}

	~CircularStringBuffer()
	{
		m_strings_headers.clear();
		free(m_buf);
		m_buf = NULL;
		m_buffer_idx = 0;
	}

	void Clear()
	{
		m_strings_headers.clear();
		m_buffer_idx = 0;
		m_buf[0] = '\0';
	}

	void StringBufferResize(int newLineCount, size_t newLineBufSize)
	{
		size_t oldBufferSize = GetBufferSize();

		m_line_count = newLineCount;
		m_line_buf_size = newLineBufSize;

		size_t newBufferSize = GetBufferSize();
		ASSERT(newBufferSize > oldBufferSize);
		char* new_buffer = (char*)malloc(newBufferSize);
		if (m_buf != NULL)
		{
			csmemcpy(new_buffer, m_buf, GetBufferSize());
			free(m_buf);
		}
		m_buf = new_buffer;
	}

	/// <summary>
	/// Add a string entry in the buffer
	/// </summary>
	/// <param name="source"></param>
	/// Source of the string
	/// <param name="characterCount"></param>
	/// Character count of the string
	void AddString(StringHeaderFlags flags, const char* source, size_t characterCount = 0)
	{
		if (characterCount == 0)
			characterCount = strnlen_s(source, m_line_buf_size - 1);

		size_t nullCharIdx = characterCount;

		ASSERT(characterCount < m_line_buf_size - 1 || source[nullCharIdx] == '\0');

		char* destinationBuffer;
		size_t destinationBufferSize = GetNewlineBuffer(m_line_buf_size, &destinationBuffer);

		if (destinationBufferSize > 0)
		{
			// position has been updated, copy the source string
			strncpy_s(destinationBuffer, destinationBufferSize, source, characterCount);
			destinationBuffer[nullCharIdx] = '\0';
			m_strings_headers.push_back(StringLineHeader{ m_buffer_idx - 1, destinationBufferSize, flags });
		}
		else
		{
			// log error
			// printf("AddString failed, destination buffer size < 0");
		}
	}

	void AddStringFmt(StringHeaderFlags flags, const char* fmt, ...)
	{
		va_list valist;
		va_start(valist, fmt);
		int buffer_size_needed = _vsnprintf(NULL, 0, fmt, valist) + 1;
		if (buffer_size_needed)
		{
			if ((size_t)buffer_size_needed < m_line_buf_size)
			{
				char* buffer = (char*)_malloca(buffer_size_needed);
				int copied_characters = _vsnprintf(buffer, buffer_size_needed, fmt, valist);
				AddString(flags, buffer, copied_characters);
				_freea(buffer);
			}
		}
		va_end(valist);
	}

	void AddStringFmt(StringHeaderFlags flags, const char* fmt, va_list valist)
	{
		int buffer_size_needed = _vsnprintf(NULL, 0, fmt, valist) + 1;
		if (buffer_size_needed)
		{
			if ((size_t)buffer_size_needed < m_line_buf_size)
			{
				char* buffer = (char*)_malloca(buffer_size_needed);
				int copied_characters = _vsnprintf(buffer, buffer_size_needed, fmt, valist);
				AddString(flags, buffer, copied_characters);
				_freea(buffer);
			}
		}
	}

	const char* GetStringAtIndex(size_t headerIdx) const
	{
		ASSERT(headerIdx < GetHeaderCount());
		const StringLineHeader& string_header = GetHeader(headerIdx);
		return GetStringAtIdx(string_header.idx);
	}

	const char* GetStringAtIdx(int idx) const
	{
		return &m_buf[idx * m_line_buf_size];
	}

	const StringLineHeader& GetHeader(int headerIdx) const
	{
		return m_strings_headers.at(headerIdx);
	}

	StringLineHeader& GetHeader(int headerIdx)
	{
		return m_strings_headers.at(headerIdx);
	}

	size_t GetHeaderCount() const
	{
		return m_strings_headers.size();
	}

private:
	// private data
	char* m_buf; // buffer

	// buffer details
	// current position of the string
	size_t m_buffer_idx;
	// header for each line of characters
	std::vector<StringLineHeader> m_strings_headers;

	size_t GetNewlineBuffer(size_t sourceStringBufferSize, char** outBufferPtr)
	{
		if (sourceStringBufferSize > GetLineBufferSize())
			return 0; // we cannot copy this string, line buffer too small

		// check if can fit the 'size' in current position remaining buffer
		bool remainingSpaceAvailable = m_buffer_idx < m_line_count;
		if (remainingSpaceAvailable)
		{
			// use current position as the buffer, there's enough space
			*outBufferPtr = &m_buf[m_buffer_idx * m_line_buf_size];
			m_buffer_idx++;
			return sourceStringBufferSize;

		}
		else
		{
			m_buffer_idx = 0;

			// clear string range, otherwise we might leave bad string_positions inside the container
			for (auto it = m_strings_headers.begin(); it != m_strings_headers.end(); )
			{
				if (m_buffer_idx == it->idx)
				{
					it = m_strings_headers.erase(it);
				}
				else
				{
					it++;
				}
			}

			// then attempt one more time
			return GetNewlineBuffer(sourceStringBufferSize, outBufferPtr);
		}
	}
};