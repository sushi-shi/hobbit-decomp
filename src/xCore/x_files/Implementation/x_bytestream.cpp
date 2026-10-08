#include <rva.h>

#include <xCore/x_files/x_bytestream.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_stdio.hpp>

#define BUFFER_SIZE (reinterpret_cast<int*>(m_pData)[-2])
#define STREAM_LENGTH (reinterpret_cast<int*>(m_pData)[-1])

RVA(0x00251e00, 0x44)
void xbytestream::EnsureCapacity(int capacity)
{
    int allocation = (capacity + 23) & ~15;
    capacity = allocation - 8;
    if (allocation > BUFFER_SIZE || capacity > STREAM_LENGTH)
    {
        m_pData = static_cast<unsigned char*>(x_realloc_fn(m_pData - 8, allocation, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bytestream.cpp", 82)) + 8;
        BUFFER_SIZE = allocation;
    }
}

RVA(0x00251e50, 0x2d)
xbytestream::xbytestream()
{
    m_pData = static_cast<unsigned char*>(x_malloc_fn(16, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bytestream.cpp", 92)) + 8;
    BUFFER_SIZE = 16;
    STREAM_LENGTH = 0;
}

RVA(0x00251e80, 0x42)
xbytestream::xbytestream(const void* data, int count)
{
    int allocation = (count + 23) & ~15;
    m_pData = static_cast<unsigned char*>(x_malloc_fn(allocation, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bytestream.cpp", 108)) + 8;
    BUFFER_SIZE = allocation;
    STREAM_LENGTH = count;
    x_memcpy(m_pData, data, count);
}

RVA(0x00251ed0, 0x4c)
xbytestream::xbytestream(const xbytestream& other)
{
    int count = other.GetLength();
    int allocation = (count + 23) & ~15;
    m_pData = static_cast<unsigned char*>(x_malloc_fn(allocation, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bytestream.cpp", 124)) + 8;
    BUFFER_SIZE = allocation;
    STREAM_LENGTH = count;
    x_memcpy(m_pData, other.m_pData, count);
}

RVA(0x00251f20, 0x1d)
xbytestream::~xbytestream()
{
    if (m_pData)
        x_free_fn(m_pData - 8, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bytestream.cpp", 137);
}

RVA(0x00251f40, 0x6)
int xbytestream::GetLength() const { return STREAM_LENGTH; }

RVA(0x00251f50, 0xa)
void xbytestream::Clear() { STREAM_LENGTH = 0; }

RVA(0x00251f60, 0x38)
void xbytestream::FreeExtra()
{
    int allocation = (STREAM_LENGTH + 24) & ~15;
    if (allocation > BUFFER_SIZE)
    {
        m_pData = static_cast<unsigned char*>(x_realloc_fn(m_pData - 8, allocation, "C:\\projects\\meridian\\xCore\\x_files\\Implementation\\x_bytestream.cpp", 162)) + 8;
        BUFFER_SIZE = allocation;
    }
}

RVA(0x00251fa0, 0xc)
unsigned char xbytestream::GetAt(int index) const { return m_pData[index]; }
RVA(0x00251fb0, 0x10)
void xbytestream::SetAt(int index, unsigned char value) { m_pData[index] = value; }
RVA(0x00251fc0, 0x3)
unsigned char* xbytestream::GetBuffer() const { return m_pData; }

RVA(0x00251fd0, 0x43)
void xbytestream::Insert(int index, unsigned char value)
{
    EnsureCapacity(STREAM_LENGTH + 1);
    x_memmove(m_pData + index + 1, m_pData + index, STREAM_LENGTH - index);
    STREAM_LENGTH += 1;
    m_pData[index] = value;
}

// The PC implementation explicitly skips zero-length raw inserts.
RVA(0x00252020, 0x52)
void xbytestream::Insert(int index, const void* data, int count)
{
    if (count)
    {
        EnsureCapacity(STREAM_LENGTH + count);
        x_memmove(m_pData + index + count, m_pData + index, STREAM_LENGTH - index);
        STREAM_LENGTH += count;
        x_memcpy(m_pData + index, data, count);
    }
}

RVA(0x00252080, 0x5a)
void xbytestream::Insert(int index, const xbytestream& other)
{
    int count = other.GetLength();
    EnsureCapacity(STREAM_LENGTH + count);
    x_memmove(m_pData + index + count, m_pData + index, STREAM_LENGTH - index);
    STREAM_LENGTH += count;
    x_memcpy(m_pData + index, other.m_pData, count);
}

RVA(0x002520e0, 0x35)
void xbytestream::Delete(int index, int count)
{
    x_memmove(m_pData + index, m_pData + index + count, STREAM_LENGTH - (index + count));
    STREAM_LENGTH -= count;
}

RVA(0x00252120, 0x13)
void xbytestream::Append(unsigned char value) { Insert(STREAM_LENGTH, value); }
RVA(0x00252140, 0x18)
void xbytestream::Append(const void* data, int count) { Insert(STREAM_LENGTH, data, count); }
RVA(0x00252160, 0x13)
void xbytestream::Append(const xbytestream& other) { Insert(STREAM_LENGTH, other); }

RVA(0x00252180, 0x2c)
int xbytestream::Find(unsigned char value, int start) const
{
    unsigned char* scan = m_pData + start;
    unsigned char* end = m_pData + STREAM_LENGTH;
    while (scan != end)
    {
        if (*scan == value)
            return scan - m_pData;
        ++scan;
    }
    return -1;
}

RVA(0x002521b0, 0x7a)
int xbytestream::Find(const xbytestream& pattern, int start) const
{
    int count = pattern.GetLength();
    if (count > STREAM_LENGTH)
        return -1;
    unsigned char* scan = m_pData + start;
    unsigned char* end = m_pData + STREAM_LENGTH - count + 1;
    while (scan != end)
    {
        int index;
        unsigned char* left = scan;
        unsigned char* right = pattern.m_pData;
        for (index = 0; index < count; ++index)
        {
            if (*left != *right)
                break;
            ++left;
            ++right;
        }
        if (index == count)
            return scan - m_pData;
        ++scan;
    }
    return -1;
}

RVA(0x00252230, 0x21)
const xbytestream& xbytestream::operator=(unsigned char value)
{
    EnsureCapacity(1);
    m_pData[0] = value;
    STREAM_LENGTH = 1;
    return *this;
}
RVA(0x00252260, 0x3a)
const xbytestream& xbytestream::operator=(const xbytestream& other)
{
    if (this != &other)
    {
        int count = other.GetLength();
        EnsureCapacity(count);
        x_memcpy(m_pData, other.m_pData, count);
        STREAM_LENGTH = count;
    }
    return *this;
}
RVA(0x002522a0, 0x47)
const xbytestream& xbytestream::operator+=(const xbytestream& other)
{
    int count = other.GetLength();
    EnsureCapacity(STREAM_LENGTH + count);
    x_memcpy(m_pData + STREAM_LENGTH, other.m_pData, count);
    STREAM_LENGTH += count;
    return *this;
}
RVA(0x002522f0, 0x28)
const xbytestream& xbytestream::operator+=(unsigned char value)
{
    EnsureCapacity(STREAM_LENGTH + 1);
    m_pData[STREAM_LENGTH] = value;
    STREAM_LENGTH += 1;
    return *this;
}
RVA(0x00252320, 0x47)
const xbytestream& xbytestream::operator<<(const char* text)
{
    int count = x_strlen(text);
    EnsureCapacity(STREAM_LENGTH + count);
    x_memcpy(m_pData + STREAM_LENGTH, text, count);
    STREAM_LENGTH += count;
    return *this;
}

RVA(0x00252370, 0x37)
const xbytestream& xbytestream::operator<<(unsigned char value)
{
    EnsureCapacity(STREAM_LENGTH + 1);
    x_memcpy(m_pData + STREAM_LENGTH, &value, 1);
    STREAM_LENGTH += 1;
    return *this;
}

RVA(0x002523b0, 0x37)
const xbytestream& xbytestream::operator<<(signed char value)
{
    EnsureCapacity(STREAM_LENGTH + 1);
    x_memcpy(m_pData + STREAM_LENGTH, &value, 1);
    STREAM_LENGTH += 1;
    return *this;
}

RVA(0x002523f0, 0x3b)
const xbytestream& xbytestream::operator<<(unsigned short value)
{
    EnsureCapacity(STREAM_LENGTH + 2);
    x_memcpy(m_pData + STREAM_LENGTH, &value, 2);
    STREAM_LENGTH += 2;
    return *this;
}

RVA(0x00252430, 0x3b)
const xbytestream& xbytestream::operator<<(short value)
{
    EnsureCapacity(STREAM_LENGTH + 2);
    x_memcpy(m_pData + STREAM_LENGTH, &value, 2);
    STREAM_LENGTH += 2;
    return *this;
}

RVA(0x00252470, 0x3b)
const xbytestream& xbytestream::operator<<(unsigned int value)
{
    EnsureCapacity(STREAM_LENGTH + 4);
    x_memcpy(m_pData + STREAM_LENGTH, &value, 4);
    STREAM_LENGTH += 4;
    return *this;
}

RVA(0x002524b0, 0x3b)
const xbytestream& xbytestream::operator<<(int value)
{
    EnsureCapacity(STREAM_LENGTH + 4);
    x_memcpy(m_pData + STREAM_LENGTH, &value, 4);
    STREAM_LENGTH += 4;
    return *this;
}

RVA(0x00252530, 0x4c)
const xbytestream& xbytestream::operator<<(const xbytestream& other)
{
    int count = other.GetLength();
    EnsureCapacity(STREAM_LENGTH + count);
    x_memcpy(m_pData + STREAM_LENGTH, other.GetBuffer(), count);
    STREAM_LENGTH += count;
    return *this;
}

RVA(0x00252580, 0x60)
int xbytestream::LoadFile(const char* name)
{
    X_FILE* file = x_fopen(name, "rb");
    if (!file)
        return 0;
    int count = x_flength(file);
    EnsureCapacity(count);
    int read = x_fread(m_pData, 1, count, file);
    STREAM_LENGTH = read;
    x_fclose(file);
    return read == count;
}

RVA(0x002525e0, 0x4e)
int xbytestream::SaveFile(const char* name) const
{
    X_FILE* file = x_fopen(name, "wb");
    if (!file)
        return 0;
    int written = x_fwrite(m_pData, 1, STREAM_LENGTH, file);
    x_fclose(file);
    return written == STREAM_LENGTH;
}
