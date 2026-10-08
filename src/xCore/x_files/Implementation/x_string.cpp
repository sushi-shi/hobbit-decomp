#include <rva.h>

#include <xCore/x_files/Implementation/x_files_private.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_stdio.hpp>
#include <xCore/x_files/x_string.hpp>

#include <stdarg.h>

// Allocation diagnostics preserve the PC source path and line arguments.
#define X_STRING_FILE "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_string.cpp"

static char* FormatIntoStringBuffer(const char* format, char* arguments);
static void ReleaseFromStringBuffer(const char* string);

RVA(0x00249860, 0x1d)
xvfs::xvfs(const char* format, char* arguments)
{
    m_pString = FormatIntoStringBuffer(format, arguments);
}

RVA(0x00249880, 0x3e)
static char* FormatIntoStringBuffer(const char* format, char* arguments)
{
    x_thread_globals* globals = x_GetThreadGlobals();
    int* length = reinterpret_cast<int*>(globals->StringBuffer + globals->NextOffset);
    char* result = globals->StringBuffer + globals->NextOffset + 4;
    *length = x_vsprintf(result, format, arguments);
    globals->NextOffset = (globals->NextOffset + *length + 8) & -4;
    return result;
}

RVA(0x002498c0, 0x3)
xvfs::operator const char*() { return m_pString; }

RVA(0x002498d0, 0xa)
xvfs::~xvfs() { ReleaseFromStringBuffer(m_pString); }

RVA(0x002498e0, 0x38)
static void ReleaseFromStringBuffer(const char* string)
{
    x_thread_globals* globals = x_GetThreadGlobals();
    int offset = string - globals->StringBuffer - 4;
    globals->NextOffset = offset;
    globals->StringBuffer[offset] = 0;
    globals->StringBuffer[offset + 1] = 0;
    globals->StringBuffer[offset + 2] = 0;
    globals->StringBuffer[offset + 3] = 0;
    globals->StringBuffer[offset + 4] = 0;
}

RVA(0x00249920, 0x1b)
xfs::xfs(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    m_pString = FormatIntoStringBuffer(format, arguments);
}

RVA(0x00249940, 0x3)
xfs::operator const char*() { return m_pString; }

RVA(0x00249950, 0xa)
xfs::~xfs() { ReleaseFromStringBuffer(m_pString); }

RVA(0x00249960, 0x68)
void xstring::EnsureCapacity(int capacity)
{
    int size = (capacity + 16) & -16;
    if (size > m_BufferSize)
    {
        if (m_pData == m_LocalData)
        {
            m_pData = static_cast<char*>(x_malloc_fn(size, X_STRING_FILE, 175));
            x_memcpy(m_pData, m_LocalData, m_Length + 1);
        }
        else
            m_pData = static_cast<char*>(x_realloc_fn(m_pData, size, X_STRING_FILE, 181));
        m_BufferSize = size;
    }
}

RVA(0x002499d0, 0x19)
xstring::xstring()
{
    m_pData = m_LocalData;
    m_BufferSize = sizeof(m_LocalData);
    m_Length = 0;
    m_pData[0] = 0;
}

RVA(0x002499f0, 0x5a)
void xstring::Init(int reserve)
{
    int size = (reserve + 16) & -16;
    if (size <= static_cast<int>(sizeof(m_LocalData)))
    {
        m_pData = m_LocalData;
        size = sizeof(m_LocalData);
    }
    else
        m_pData = static_cast<char*>(x_malloc_fn(size, X_STRING_FILE, 213));
    m_BufferSize = size;
    m_Length = 0;
    m_pData[0] = 0;
}

RVA(0x00249a50, 0x2d)
void xstring::Init(int length, const char* string)
{
    Init(length);
    x_memcpy(m_pData, string, length);
    m_Length = length;
    m_pData[length] = 0;
}

RVA(0x00249a80, 0x4a)
void xstring::Init(int length, const unsigned short* string)
{
    Init(length);
    for (int i = 0; i < length; ++i)
        if (string[i] >= 256) m_pData[i] = '.';
        else m_pData[i] = static_cast<char>(string[i]);
    m_Length = length;
    m_pData[length] = 0;
}

RVA(0x00249ad0, 0x2e)
xstring::xstring(int reserve, const char* string)
{
    if (string) Init(reserve, string);
    else Init(reserve);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00249b00, 0x15)
xstring::xstring(char character) { Init(1, &character); }

RVA(0x00249b20, 0x34)
xstring::xstring(const char* string)
{
    if (string) Init(x_strlen(string), string);
    else Init(0, static_cast<const char*>(0));
}

RVA(0x00249b60, 0x1b)
xstring::xstring(const xstring& string) { Init(string.m_Length, string.m_pData); }

RVA(0x00249b80, 0x1b)
xstring::xstring(const xwstring& string) { Init(string.m_Length, string.m_pData); }

RVA(0x00249ba0, 0x21)
xstring::~xstring()
{
    if (m_pData && m_pData != m_LocalData)
        x_free_fn(m_pData, X_STRING_FILE, 297);
}

RVA(0x00249bd0, 0xd)
void xstring::Clear() { m_Length = 0; m_pData[0] = 0; }

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00249be0, 0x37)
void xstring::FreeExtra()
{
    int size = (m_Length + 16) & -16;
    if (m_pData != m_LocalData && size < m_BufferSize)
    {
        m_pData = static_cast<char*>(x_realloc_fn(m_pData, size, X_STRING_FILE, 319));
        m_BufferSize = size;
    }
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00249c20, 0x60)
void xstring::IndexToRowCol(int index, int& row, int& column) const
{
    int i = 0;
    if (index > m_Length) { row = -1; column = -1; return; }
    row = 1;
    column = 1;
    for (; i < index; ++i)
        if (m_pData[i] == '\n') { ++row; column = 1; }
        else ++column;
}

RVA(0x00249c80, 0x2a)
xstring xstring::Mid(int index, int count) const { return xstring(count, m_pData + index); }

RVA(0x00249cb0, 0x24)
xstring xstring::Left(int count) const { return xstring(count, m_pData); }

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00249ce0, 0x2b)
xstring xstring::Right(int count) const { return xstring(count, m_pData + m_Length - count); }

RVA(0x00249d10, 0x28)
void xstring::MakeUpper()
{
    int length = m_Length;
    for (int i = 0; i < length; ++i)
        m_pData[i] = x_UpperCase[static_cast<unsigned char>(m_pData[i])];
}

RVA(0x00249d40, 0x28)
void xstring::MakeLower()
{
    int length = m_Length;
    for (int i = 0; i < length; ++i)
        m_pData[i] = x_LowerCase[static_cast<unsigned char>(m_pData[i])];
}

RVA(0x00249d70, 0x3e)
void xstring::Insert(int index, char character)
{
    EnsureCapacity(m_Length + 1);
    x_memmove(m_pData + index + 1, m_pData + index, m_Length - index + 1);
    ++m_Length;
    m_pData[index] = character;
}

RVA(0x00249db0, 0x5b)
void xstring::Insert(int index, const char* string)
{
    int length = x_strlen(string);
    EnsureCapacity(m_Length + length);
    x_memmove(m_pData + length + index, m_pData + index, m_Length - index + 1);
    m_Length += length;
    x_memcpy(m_pData + index, string, length);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00249e10, 0x4f)
void xstring::Insert(int index, const xstring& string)
{
    int length = string.m_Length;
    EnsureCapacity(m_Length + length);
    x_memmove(m_pData + length + index, m_pData + index, m_Length - index + 1);
    m_Length += length;
    x_memcpy(m_pData + index, string.m_pData, length);
}

RVA(0x00249e60, 0x33)
void xstring::Delete(int index, int count)
{
    x_memmove(m_pData + index, m_pData + index + count, m_Length - index - count + 1);
    m_Length -= count;
}

RVA(0x00249ea0, 0x8f)
int xstring::Format(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    xvfs result(format, arguments);
    int length = x_strlen(result);
    EnsureCapacity(length);
    x_strcpy(m_pData, result);
    m_Length = length;
    return length;
}

RVA(0x00249f30, 0x8e)
int xstring::FormatV(const char* format, char* arguments)
{
    xvfs result(format, arguments);
    int length = x_strlen(result);
    EnsureCapacity(length);
    x_strcpy(m_pData, result);
    m_Length = length;
    return length;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x00249fc0, 0x9e)
int xstring::AddFormat(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    xvfs result(format, arguments);
    int length = x_strlen(result);
    EnsureCapacity(m_Length + length);
    x_strcpy(m_pData + m_Length, result);
    m_Length += length;
    return length;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024a060, 0x9d)
int xstring::AddFormatV(const char* format, char* arguments)
{
    xvfs result(format, arguments);
    int length = x_strlen(result);
    EnsureCapacity(m_Length + length);
    x_strcpy(m_pData + m_Length, result);
    m_Length += length;
    return length;
}

RVA(0x0024a100, 0x2c)
int xstring::Find(char character, int start) const
{
    char* begin = m_pData;
    char* position = begin + start;
    char* end = begin + m_Length;
    while (position != end)
    {
        if (*position == character) return position - begin;
        ++position;
    }
    return -1;
}

RVA(0x0024a130, 0x59)
int xstring::Find(const char* string, int start) const
{
    int length = x_strlen(string);
    char* position = m_pData + start;
    char* end = m_pData + (m_Length - length) + 1;
    while (position < end)
    {
        const char* haystack = position;
        const char* needle = string;
        for (;;)
        {
            if (!*needle) return position - m_pData;
            if (*haystack != *needle) break;
            ++haystack;
            ++needle;
        }
        ++position;
    }
    return -1;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024a190, 0x74)
int xstring::Find(const xstring& string, int start) const
{
    int length = string.m_Length;
    if (length > m_Length) return -1;
    char* position = m_pData + start;
    char* end = m_pData - length + m_Length + 1;
    while (position != end)
    {
        int i;
        char* haystack = position;
        char* needle = string.m_pData;
        for (i = 0; i < length; ++i)
        {
            if (*haystack != *needle) break;
            ++haystack;
            ++needle;
        }
        if (i == length) return position - m_pData;
        ++position;
    }
    return -1;
}

RVA(0x0024a210, 0x9d)
int xstring::Replace(const char* search, const char* replacement, int start)
{
    int position = Find(search, start);
    if (position == -1) return position;
    int i = 0;
    while (search[i])
    {
        if (!((*this)[i + position] = replacement[i])) break;
        ++i;
    }
    if (search[i])
    {
        int end = i;
        while (search[++end]) {}
        Delete(i + position, end - i);
    }
    else if (replacement[i])
        Insert(i + position, replacement[i]);
    return position;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024a2b0, 0x25)
const xstring& xstring::operator=(char character)
{
    EnsureCapacity(1);
    m_pData[0] = character;
    m_pData[1] = 0;
    m_Length = 1;
    return *this;
}

RVA(0x0024a2e0, 0x37)
const xstring& xstring::operator=(const char* string)
{
    if (string)
    {
        int length = x_strlen(string);
        EnsureCapacity(length);
        x_strcpy(m_pData, string);
        m_Length = length;
    }
    return *this;
}

RVA(0x0024a320, 0x33)
const xstring& xstring::operator=(const xstring& string)
{
    if (this != &string)
    {
        int length = string.m_Length;
        EnsureCapacity(length);
        x_memcpy(m_pData, string.m_pData, length + 1);
        m_Length = length;
    }
    return *this;
}

RVA(0x0024a360, 0x38)
const xstring& xstring::operator=(const xwstring& string)
{
    int length = string.m_Length;
    EnsureCapacity(length);
    int i;
    for (i = 0; i < length; ++i) m_pData[i] = static_cast<char>(string.m_pData[i]);
    m_pData[i] = 0;
    m_Length = length;
    return *this;
}

RVA(0x0024a3a0, 0xa5)
xstring operator+(const xstring& first, char second)
{
    int length = first.m_Length;
    xstring result(length + 1);
    x_memcpy(result.m_pData, first.m_pData, length);
    result.m_pData[length] = second;
    result.m_pData[length + 1] = 0;
    result.m_Length = length + 1;
    return result;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024a450, 0x99)
xstring operator+(char first, const xstring& second)
{
    int length = second.m_Length;
    xstring result(length + 1);
    result.m_pData[0] = first;
    x_memcpy(result.m_pData + 1, second.m_pData, (length + 1));
    result.m_Length = length + 1;
    return result;
}

RVA(0x0024a4f0, 0xb4)
xstring operator+(const xstring& first, const char* second)
{
    int length1 = first.m_Length;
    int length2 = x_strlen(second);
    xstring result(length1 + length2);
    x_memcpy(result.m_pData, first.m_pData, length1);
    x_memcpy(result.m_pData + length1, second, (length2 + 1));
    result.m_Length = length1 + length2;
    return result;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024a5b0, 0xb4)
xstring operator+(const char* first, const xstring& second)
{
    int length1 = x_strlen(first);
    int length2 = second.m_Length;
    xstring result(length1 + length2);
    x_memcpy(result.m_pData, first, length1);
    x_memcpy(result.m_pData + length1, second.m_pData, (length2 + 1));
    result.m_Length = length1 + length2;
    return result;
}

RVA(0x0024a670, 0xae)
xstring operator+(const xstring& first, const xstring& second)
{
    int length1 = first.m_Length;
    int length2 = second.m_Length;
    xstring result(length1 + length2);
    x_memcpy(result.m_pData, first.m_pData, length1);
    x_memcpy(result.m_pData + length1, second.m_pData, (length2 + 1));
    result.m_Length = length1 + length2;
    return result;
}

RVA(0x0024a720, 0x2c)
const xstring& xstring::operator+=(char character)
{
    EnsureCapacity(m_Length + 1);
    m_pData[m_Length] = character;
    m_pData[m_Length + 1] = 0;
    ++m_Length;
    return *this;
}

RVA(0x0024a750, 0x46)
const xstring& xstring::operator+=(const char* string)
{
    int length = x_strlen(string);
    EnsureCapacity(m_Length + length);
    x_memcpy(m_pData + m_Length, string, length + 1);
    m_Length += length;
    return *this;
}

RVA(0x0024a7a0, 0x3e)
const xstring& xstring::operator+=(const xstring& string)
{
    int length = string.m_Length;
    EnsureCapacity(m_Length + length);
    x_memmove(m_pData + m_Length, string.m_pData, length + 1);
    m_Length += length;
    return *this;
}

RVA(0x0024a7e0, 0x3a)
int operator==(const xstring& first, const char* second)
{
    return first.GetLength() == x_strlen(second) && x_strcmp(first, second) == 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024a820, 0x3a)
int operator==(const char* first, const xstring& second)
{
    return second.GetLength() == x_strlen(first) && x_strcmp(first, second) == 0;
}

RVA(0x0024a860, 0x2c)
int operator==(const xstring& first, const xstring& second)
{
    return first.GetLength() == second.GetLength() && x_memcmp(first.m_pData, second.m_pData, first.GetLength()) == 0;
}

RVA(0x0024a890, 0x38)
int operator!=(const xstring& first, const char* second)
{
    return !(first.GetLength() == x_strlen(second) && x_strcmp(first, second) == 0);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024a8d0, 0x38)
int operator!=(const char* first, const xstring& second)
{
    return !(second.GetLength() == x_strlen(first) && x_strcmp(first, second) == 0);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024a910, 0x2a)
int operator!=(const xstring& first, const xstring& second)
{
    return !(first.GetLength() == second.GetLength() && x_memcmp(first.m_pData, second.m_pData, first.GetLength()) == 0);
}

RVA(0x0024a940, 0x3e)
int operator<(const xstring& first, const char* second)
{
    int result = x_strcmp(first, second);
    if (result == 0) return first.GetLength() < x_strlen(second);
    return result < 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024a980, 0x3e)
int operator<(const char* first, const xstring& second)
{
    int result = x_strcmp(first, second);
    if (result == 0) return x_strlen(first) < second.GetLength();
    return result < 0;
}

RVA(0x0024a9c0, 0x41)
int operator<(const xstring& first, const xstring& second)
{
    int result = x_memcmp(first.m_pData, second.m_pData, (first.GetLength() < second.GetLength() ? first.GetLength() : second.GetLength()));
    if (result == 0) return first.GetLength() < second.GetLength();
    return result < 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024aa10, 0x3e)
int operator>(const xstring& first, const char* second)
{
    int result = x_strcmp(first, second);
    if (result == 0) return first.GetLength() > x_strlen(second);
    return result > 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024aa50, 0x3e)
int operator>(const char* first, const xstring& second)
{
    int result = x_strcmp(first, second);
    if (result == 0) return x_strlen(first) > second.GetLength();
    return result > 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024aa90, 0x41)
int operator>(const xstring& first, const xstring& second)
{
    int result = x_memcmp(first.m_pData, second.m_pData, (first.GetLength() < second.GetLength() ? first.GetLength() : second.GetLength()));
    if (result == 0) return first.GetLength() > second.GetLength();
    return result > 0;
}

RVA(0x0024aae0, 0x3e)
int operator<=(const xstring& first, const char* second)
{
    int result = x_strcmp(first, second);
    if (result == 0) return first.GetLength() <= x_strlen(second);
    return result < 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024ab20, 0x3e)
int operator<=(const char* first, const xstring& second)
{
    int result = x_strcmp(first, second);
    if (result == 0) return x_strlen(first) <= second.GetLength();
    return result < 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024ab60, 0x41)
int operator<=(const xstring& first, const xstring& second)
{
    int result = x_memcmp(first.m_pData, second.m_pData, (first.GetLength() < second.GetLength() ? first.GetLength() : second.GetLength()));
    if (result == 0) return first.GetLength() <= second.GetLength();
    return result < 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024abb0, 0x3e)
int operator>=(const xstring& first, const char* second)
{
    int result = x_strcmp(first, second);
    if (result == 0) return first.GetLength() >= x_strlen(second);
    return result > 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024abf0, 0x3e)
int operator>=(const char* first, const xstring& second)
{
    int result = x_strcmp(first, second);
    if (result == 0) return x_strlen(first) >= second.GetLength();
    return result > 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024ac30, 0x41)
int operator>=(const xstring& first, const xstring& second)
{
    int result = x_memcmp(first.m_pData, second.m_pData, (first.GetLength() < second.GetLength() ? first.GetLength() : second.GetLength()));
    if (result == 0) return first.GetLength() >= second.GetLength();
    return result > 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024ac80, 0x4a)
void xstring::Dump(int lineFeed) const
{
    int length = m_Length;
    for (int i = 0; i < length; ++i)
    {
        char character = m_pData[i];
        if (character < 32) character = '.';
        x_printf("%c", character);
    }
    if (lineFeed) x_printf("\n");
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024acd0, 0xee)
void xstring::DumpHex() const
{
    char text[35];
    char digits[] = "0123456789ABCDEF";
    int column = 0;
    int length = m_Length;
    x_memset(text, ' ', 34);
    text[34] = 0;
    text[24] = '-';
    for (int i = 0; i < length; ++i)
    {
        column = i & 7;
        text[column * 3] = digits[(m_pData[i] >> 4) & 15];
        text[column * 3 + 1] = digits[m_pData[i] & 15];
        if (m_pData[i] >= 32) text[column + 26] = m_pData[i];
        else text[column + 26] = '.';
        if (column == 7)
        {
            x_printf("%s\n", text);
            x_memset(text, ' ', 34);
            text[24] = '-';
        }
    }
    if (column != 7) x_printf("%s\n", text);
}

RVA(0x0024adc0, 0x64)
int xstring::LoadFile(const char* filename)
{
    X_FILE* file = x_fopen(filename, "rb");
    if (!file) return 0;
    int size = x_flength(file);
    EnsureCapacity(size);
    int read = x_fread(m_pData, 1, size, file);
    m_Length = read;
    m_pData[read] = 0;
    x_fclose(file);
    return read == size;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024ae30, 0xf4)
int xstring::SaveFile(const char* filename) const
{
    int written = 0;
    xstring string(*this);
    X_FILE* file = x_fopen(filename, "wb");
    if (!file) return 0;
    int position = 0;
    while (position < string.m_Length)
    {
        position = string.Find('\n', position);
        if (position == -1) break;
        string.Insert(position, "\r");
        position += 2;
    }
    int size = string.m_Length;
    if (size > 0) written = x_fwrite(string.m_pData, 1, size, file);
    x_fclose(file);
    return written == size;
}

RVA(0x0024af30, 0x6c)
void xwstring::EnsureCapacity(int capacity)
{
    int size = (capacity * 2 + 16) & -16;
    if (size > m_BufferSize)
    {
        if (m_pData == m_LocalData)
        {
            m_pData = static_cast<unsigned short*>(x_malloc_fn(size, X_STRING_FILE, 1455));
            x_memcpy(m_pData, m_LocalData, (m_Length + 1) * 2);
        }
        else
            m_pData = static_cast<unsigned short*>(x_realloc_fn(m_pData, size, X_STRING_FILE, 1462));
        m_BufferSize = size;
    }
}

RVA(0x0024afa0, 0x1b)
xwstring::xwstring()
{
    m_pData = m_LocalData;
    m_BufferSize = sizeof(m_LocalData);
    m_Length = 0;
    m_pData[0] = 0;
}

RVA(0x0024afc0, 0x4b)
void xwstring::Init(int reserve)
{
    int size = (reserve * 2 + 16) & -16;
    if (size <= static_cast<int>(sizeof(m_LocalData)))
    {
        m_pData = m_LocalData;
        size = sizeof(m_LocalData);
    }
    else
        m_pData = static_cast<unsigned short*>(x_malloc_fn(size, X_STRING_FILE, 1493));
    m_BufferSize = size;
    m_Length = 0;
    m_pData[0] = 0;
}

RVA(0x0024b010, 0x41)
void xwstring::Init(int length, const char* string)
{
    Init(length);
    for (int i = 0; i < length; ++i) m_pData[i] = string[i];
    m_Length = length;
    m_pData[length] = 0;
}

RVA(0x0024b060, 0x34)
void xwstring::Init(int length, const unsigned short* string)
{
    Init(length);
    x_memcpy(m_pData, string, length * 2);
    m_Length = length;
    m_pData[length] = 0;
}

RVA(0x0024b0a0, 0x2e)
xwstring::xwstring(int reserve, const unsigned short* string)
{
    if (string) Init(reserve, string);
    else Init(reserve);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b0d0, 0x15)
xwstring::xwstring(unsigned short character) { Init(1, &character); }

RVA(0x0024b0f0, 0x34)
xwstring::xwstring(const unsigned short* string)
{
    if (string) Init(x_wstrlen(string), string);
    else Init(0, static_cast<const unsigned short*>(0));
}

RVA(0x0024b130, 0x34)
xwstring::xwstring(const char* string)
{
    if (string) Init(x_strlen(string), string);
    else Init(0, static_cast<const char*>(0));
}

RVA(0x0024b170, 0x1b)
xwstring::xwstring(const xwstring& string) { Init(string.m_Length, string.m_pData); }

RVA(0x0024b190, 0x21)
xwstring::~xwstring()
{
    if (m_pData && m_pData != m_LocalData)
        x_free_fn(m_pData, X_STRING_FILE, 1577);
}

RVA(0x0024b1c0, 0xf)
void xwstring::Clear() { m_Length = 0; m_pData[0] = 0; }

RVA(0x0024b1d0, 0x38)
void xwstring::FreeExtra()
{
    int size = (m_Length * 2 + 16) & -16;
    if (m_pData != m_LocalData && size < m_BufferSize)
    {
        m_pData = static_cast<unsigned short*>(x_realloc_fn(m_pData, size, X_STRING_FILE, 1600));
        m_BufferSize = size;
    }
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b210, 0x2d)
int xwstring::GetHashKey(int index, int count) const
{
    int hash = 0;
    while (count-- > 0)
        hash = ((hash << 4) ^ (hash >> 28)) + m_pData[index++];
    return hash;
}

RVA(0x0024b240, 0x2b)
xwstring xwstring::Mid(int index, int count) const { return xwstring(count, m_pData + index); }

RVA(0x0024b270, 0x24)
xwstring xwstring::Left(int count) const { return xwstring(count, m_pData); }

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b2a0, 0x2c)
xwstring xwstring::Right(int count) const { return xwstring(count, m_pData + m_Length - count); }

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b2d0, 0x46)
void xwstring::Insert(int index, unsigned short character)
{
    EnsureCapacity(m_Length + 1);
    x_memmove(m_pData + index + 1, m_pData + index, (m_Length - index + 1) * 2);
    ++m_Length;
    m_pData[index] = character;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b320, 0x65)
void xwstring::Insert(int index, const unsigned short* string)
{
    int length = x_wstrlen(string);
    EnsureCapacity(m_Length + length);
    x_memmove(m_pData + (index + length), m_pData + index, (m_Length - index + 1) * 2);
    m_Length += length;
    x_memcpy(m_pData + index, string, length * 2);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b390, 0x59)
void xwstring::Insert(int index, const xwstring& string)
{
    int length = string.m_Length;
    EnsureCapacity(m_Length + length);
    x_memmove(m_pData + (index + length), m_pData + index, (m_Length - index + 1) * 2);
    m_Length += length;
    x_memcpy(m_pData + index, string.m_pData, length * 2);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b3f0, 0x3a)
void xwstring::Delete(int index, int count)
{
    x_memmove(m_pData + index, m_pData + (index + count), (m_Length - index - count + 1) * 2);
    m_Length -= count;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b430, 0x3b)
int xwstring::Find(unsigned short character, int start) const
{
    unsigned short* begin = m_pData;
    unsigned short* position = begin + start;
    unsigned short* end = begin + m_Length;
    while (position != end)
    {
        if (*position == character) return position - begin;
        ++position;
    }
    return -1;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b470, 0x64)
int xwstring::Find(const unsigned short* string, int start) const
{
    int length = x_wstrlen(string);
    unsigned short* position = m_pData + start;
    unsigned short* end = m_pData + (m_Length - length) + 1;
    while (position != end)
    {
        const unsigned short* haystack = position;
        const unsigned short* needle = string;
        for (;;)
        {
            if (!*needle) return position - m_pData;
            if (*haystack != *needle) break;
            ++haystack;
            ++needle;
        }
        ++position;
    }
    return -1;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b4e0, 0x75)
int xwstring::Find(const xwstring& string, int start) const
{
    int length = string.m_Length;
    if (length > m_Length) return -1;
    unsigned short* position = m_pData + start;
    unsigned short* end = m_pData + (m_Length - length) + 1;
    while (position != end)
    {
        int i;
        unsigned short* haystack = position;
        unsigned short* needle = string.m_pData;
        for (i = 0; i < length; ++i)
        {
            if (*haystack != *needle) break;
            ++haystack;
            ++needle;
        }
        if (i == length) return position - m_pData;
        ++position;
    }
    return -1;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b560, 0x12c)
int xwstring::LoadFile(const char* filename)
{
    int success = 0;
    int size;
    int read;
    int length;
    unsigned char byte;
    X_FILE* file = x_fopen(filename, "rb");
    if (!file) return 0;
    size = x_flength(file);
    length = (size - 2) / 2;
    read = x_fread(&byte, 1, 1, file);
    if (read == 1 && byte == 255)
    {
        read = x_fread(&byte, 1, 1, file);
        if (read == 1 && byte == 254)
        {
            EnsureCapacity(length);
            read = x_fread(m_pData, 2, length, file);
            m_Length = read;
            m_pData[read] = 0;
            success = read == length;
        }
    }
    x_fclose(file);
    if (!success)
    {
        xstring string;
        if (string.LoadFile(filename))
        {
            *this = string;
            success = 1;
        }
    }
    return success;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b690, 0xeb)
int xwstring::SaveFile(const char* filename) const
{
    int success = 0;
    xwstring string(*this);
    X_FILE* file = x_fopen(filename, "wb");
    if (!file) return 0;
    int size = string.GetLength();
    unsigned char byte = 255;
    if (x_fwrite(&byte, 1, 1, file) == 1)
    {
        byte = 254;
        if (x_fwrite(&byte, 1, 1, file) == 1)
        {
            int written = x_fwrite(string, 2, size, file);
            success = written == size;
        }
    }
    x_fclose(file);
    return success;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b780, 0x29)
const xwstring& xwstring::operator=(unsigned short character)
{
    EnsureCapacity(1);
    m_pData[0] = character;
    m_pData[1] = 0;
    m_Length = 1;
    return *this;
}

RVA(0x0024b7b0, 0x33)
const xwstring& xwstring::operator=(const unsigned short* string)
{
    int length = x_wstrlen(string);
    EnsureCapacity(length);
    x_wstrcpy(m_pData, string);
    m_Length = length;
    return *this;
}

RVA(0x0024b7f0, 0x34)
const xwstring& xwstring::operator=(const xwstring& string)
{
    if (this != &string)
    {
        int length = string.m_Length;
        EnsureCapacity(length);
        x_memcpy(m_pData, string.m_pData, (length + 1) * 2);
        m_Length = length;
    }
    return *this;
}

RVA(0x0024b830, 0x48)
const xwstring& xwstring::operator=(const xstring& string)
{
    int length = string.m_Length;
    EnsureCapacity(length);
    int i;
    for (i = 0; i < string.m_Length; ++i)
        m_pData[i] = static_cast<unsigned char>(string.m_pData[i]);
    m_pData[i] = 0;
    m_Length = length;
    return *this;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b880, 0xae)
xwstring operator+(const xwstring& first, unsigned short second)
{
    int length = first.m_Length;
    xwstring result(length + 1);
    x_memcpy(result.m_pData, first.m_pData, length << 1);
    result.m_pData[length] = second;
    result.m_pData[length + 1] = 0;
    result.m_Length = length + 1;
    return result;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024b930, 0xa0)
xwstring operator+(unsigned short first, const xwstring& second)
{
    int length = second.m_Length;
    xwstring result(length + 1);
    result.m_pData[0] = first;
    x_memcpy(result.m_pData + 1, second.m_pData, (length + 1) << 1);
    result.m_Length = length + 1;
    return result;
}

RVA(0x0024b9d0, 0xc1)
xwstring operator+(const xwstring& first, const unsigned short* second)
{
    int length1 = first.m_Length;
    int length2 = x_wstrlen(second);
    xwstring result(length1 + length2);
    x_memcpy(result.m_pData, first.m_pData, length1 << 1);
    x_memcpy(result.m_pData + length1, second, (length2 + 1) << 1);
    result.m_Length = length1 + length2;
    return result;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024baa0, 0xc1)
xwstring operator+(const unsigned short* first, const xwstring& second)
{
    int length1 = x_wstrlen(first);
    int length2 = second.m_Length;
    xwstring result(length1 + length2);
    x_memcpy(result.m_pData, first, length1 << 1);
    x_memcpy(result.m_pData + length1, second.m_pData, (length2 + 1) << 1);
    result.m_Length = length1 + length2;
    return result;
}

RVA(0x0024bb70, 0xbb)
xwstring operator+(const xwstring& first, const xwstring& second)
{
    int length1 = first.m_Length;
    int length2 = second.m_Length;
    xwstring result(length1 + length2);
    x_memcpy(result.m_pData, first.m_pData, length1 << 1);
    x_memcpy(result.m_pData + length1, second.m_pData, (length2 + 1) << 1);
    result.m_Length = length1 + length2;
    return result;
}


RVA(0x0024bc30, 0x30)
const xwstring& xwstring::operator+=(unsigned short character)
{
    EnsureCapacity(m_Length + 1);
    m_pData[m_Length] = character;
    m_pData[m_Length + 1] = 0;
    ++m_Length;
    return *this;
}

RVA(0x0024bc60, 0x48)
const xwstring& xwstring::operator+=(const unsigned short* string)
{
    int length = x_wstrlen(string);
    EnsureCapacity(m_Length + length);
    x_memcpy(m_pData + m_Length, string, (length + 1) * 2);
    m_Length += length;
    return *this;
}

RVA(0x0024bcb0, 0x40)
const xwstring& xwstring::operator+=(const xwstring& string)
{
    int length = string.m_Length;
    EnsureCapacity(m_Length + length);
    x_memmove(m_pData + m_Length, string.m_pData, (length + 1) * 2);
    m_Length += length;
    return *this;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024bcf0, 0x3a)
int operator==(const xwstring& first, const unsigned short* second)
{
    return first.GetLength() == x_wstrlen(second) && x_wstrcmp(first, second) == 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024bd30, 0x3a)
int operator==(const unsigned short* first, const xwstring& second)
{
    return second.GetLength() == x_wstrlen(first) && x_wstrcmp(first, second) == 0;
}

RVA(0x0024bd70, 0x2e)
int operator==(const xwstring& first, const xwstring& second)
{
    return first.GetLength() == second.GetLength() && x_memcmp(first.m_pData, second.m_pData, first.GetLength() * 2) == 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024bda0, 0x38)
int operator!=(const xwstring& first, const unsigned short* second)
{
    return !(first.GetLength() == x_wstrlen(second) && x_wstrcmp(first, second) == 0);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024bde0, 0x38)
int operator!=(const unsigned short* first, const xwstring& second)
{
    return !(second.GetLength() == x_wstrlen(first) && x_wstrcmp(first, second) == 0);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024be20, 0x2c)
int operator!=(const xwstring& first, const xwstring& second)
{
    return !(first.GetLength() == second.GetLength() && x_memcmp(first.m_pData, second.m_pData, first.GetLength() * 2) == 0);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024be50, 0x3e)
int operator<(const xwstring& first, const unsigned short* second)
{
    int result = x_wstrcmp(first, second);
    if (result == 0) return first.GetLength() < x_wstrlen(second);
    return result < 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024be90, 0x3e)
int operator<(const unsigned short* first, const xwstring& second)
{
    int result = x_wstrcmp(first, second);
    if (result == 0) return x_wstrlen(first) < second.GetLength();
    return result < 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024bed0, 0x49)
int operator<(const xwstring& first, const xwstring& second)
{
    int result = x_memcmp(first.m_pData, second.m_pData, (first.GetLength() < second.GetLength() ? first.GetLength() : second.GetLength()) * 2);
    if (result == 0) return first.GetLength() < second.GetLength();
    return result < 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024bf20, 0x3e)
int operator>(const xwstring& first, const unsigned short* second)
{
    int result = x_wstrcmp(first, second);
    if (result == 0) return first.GetLength() > x_wstrlen(second);
    return result > 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024bf60, 0x3e)
int operator>(const unsigned short* first, const xwstring& second)
{
    int result = x_wstrcmp(first, second);
    if (result == 0) return x_wstrlen(first) > second.GetLength();
    return result > 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024bfa0, 0x49)
int operator>(const xwstring& first, const xwstring& second)
{
    int result = x_memcmp(first.m_pData, second.m_pData, (first.GetLength() < second.GetLength() ? first.GetLength() : second.GetLength()) * 2);
    if (result == 0) return first.GetLength() > second.GetLength();
    return result > 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024bff0, 0x3e)
int operator<=(const xwstring& first, const unsigned short* second)
{
    int result = x_wstrcmp(first, second);
    if (result == 0) return first.GetLength() <= x_wstrlen(second);
    return result < 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024c030, 0x3e)
int operator<=(const unsigned short* first, const xwstring& second)
{
    int result = x_wstrcmp(first, second);
    if (result == 0) return x_wstrlen(first) <= second.GetLength();
    return result < 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024c070, 0x49)
int operator<=(const xwstring& first, const xwstring& second)
{
    int result = x_memcmp(first.m_pData, second.m_pData, (first.GetLength() < second.GetLength() ? first.GetLength() : second.GetLength()) * 2);
    if (result == 0) return first.GetLength() <= second.GetLength();
    return result < 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024c0c0, 0x3e)
int operator>=(const xwstring& first, const unsigned short* second)
{
    int result = x_wstrcmp(first, second);
    if (result == 0) return first.GetLength() >= x_wstrlen(second);
    return result > 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024c100, 0x3e)
int operator>=(const unsigned short* first, const xwstring& second)
{
    int result = x_wstrcmp(first, second);
    if (result == 0) return x_wstrlen(first) >= second.GetLength();
    return result > 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024c140, 0x49)
int operator>=(const xwstring& first, const xwstring& second)
{
    int result = x_memcmp(first.m_pData, second.m_pData, (first.GetLength() < second.GetLength() ? first.GetLength() : second.GetLength()) * 2);
    if (result == 0) return first.GetLength() >= second.GetLength();
    return result > 0;
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024c190, 0x10f)
void xwstring::DumpHex() const
{
    unsigned short text[40];
    char digits[] = "0123456789ABCDEF";
    int column = 0;
    int length = m_Length;
    // PC passes 39 bytes here, despite text being an array of 16-bit elements.
    x_memset(text, ' ', 39);
    text[39] = 0;
    for (int i = 0; i < length; ++i)
    {
        column = i & 7;
        text[column * 5] = digits[(m_pData[i] >> 12) & 15];
        text[column * 5 + 1] = digits[(m_pData[i] >> 8) & 15];
        text[column * 5 + 2] = digits[(m_pData[i] >> 4) & 15];
        text[column * 5 + 3] = digits[m_pData[i] & 15];
        if (column == 7)
        {
            x_printf("%s\n", text);
            x_memset(text, ' ', 39);
            text[39] = 0;
        }
    }
    if (column != 7) x_printf("%s\n", text);
}

// @dead-code
// Zero-ref: no E8/E9 rel32 targets in PC .text or absolute VA in the whole file.
RVA(0x0024c2a0, 0x83)
int xwstring::StartsWith(const xwstring& prefix) const
{
    if (prefix.GetLength() > m_Length) return 0;
    xwstring left = Left(prefix.GetLength());
    return left == prefix;
}
