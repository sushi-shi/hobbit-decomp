#include <rva.h>

#include <xCore/Auxiliary/Parsing/bitstream.hpp>

#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>

// Variable-width integer code lengths; all sixteen words agree with PC data.
DATA(0x0034a7d0) static int s_VarLenBitOptions[16] =
    {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,32};

inline int GetHighestBit(unsigned int value)
{
    int bits = 0;
    while (value) { ++bits; value >>= 1; }
    return bits;
}

RVA(0x0023c8f0, 0x25)
bitstream::bitstream()
{
    m_Data = 0;
    m_DataSize = 0;
    m_DataSizeInBits = m_DataSize << 3;
    m_HighestBitWritten = -1;
    m_Cursor = 0;
    m_bOwnsData = 1;
    m_MaxGrowSize = 1024;
}
RVA(0x0023c920, 0x5)
bitstream::~bitstream() { Kill(); }
RVA(0x0023c930, 0x37)
void bitstream::Kill()
{
    if (m_bOwnsData)
        x_free_fn(m_Data, "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\bitstream.cpp", 55);
    m_Data = 0;
    m_DataSize = 0;
    m_DataSizeInBits = 0;
    m_HighestBitWritten = -1;
    m_Cursor = 0;
    m_bOwnsData = 0;
}
RVA(0x0023c970, 0x52)
void bitstream::Init(int bytes)
{
    int size = bytes > m_DataSize ? bytes : m_DataSize;
    if (size > m_DataSize)
    {
        m_Data = static_cast<unsigned char*>(x_realloc_fn(m_Data, size,
            "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\bitstream.cpp", 71));
        m_DataSize = size;
        m_DataSizeInBits = m_DataSize << 3;
    }
    m_HighestBitWritten = -1;
    m_Cursor = 0;
    m_bOwnsData = 1;
}
RVA(0x0023c9d0, 0x25)
void bitstream::Init(const unsigned char* data, int bytes)
{
    m_Data = const_cast<unsigned char*>(data);
    m_bOwnsData = 0;
    m_DataSize = bytes;
    m_DataSizeInBits = m_DataSize << 3;
    m_HighestBitWritten = -1;
    m_Cursor = 0;
}
RVA(0x0023ca00, 0x42)
void bitstream::Grow()
{
    int grow = 16 + m_DataSize / 2;
    grow = grow < m_MaxGrowSize ? grow : m_MaxGrowSize;
    m_DataSize += grow;
    m_DataSizeInBits = m_DataSize << 3;
    m_Data = static_cast<unsigned char*>(x_realloc_fn(m_Data, m_DataSize,
        "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\bitstream.cpp", 102));
}
RVA(0x0023ca50, 0xa)
void bitstream::SetMaxGrowSize(int bytes) { m_MaxGrowSize = bytes; }
RVA(0x0023ca60, 0x4)
int bitstream::GetNBytes() const { return m_DataSize; }
RVA(0x0023ca70, 0x4)
int bitstream::GetNBits() const { return m_DataSizeInBits; }
RVA(0x0023ca80, 0xa)
int bitstream::GetNBytesUsed() const { return (m_HighestBitWritten + 8) >> 3; }
RVA(0x0023ca90, 0xd)
int bitstream::GetNBytesFree() const { return m_DataSize - GetNBytesUsed(); }
RVA(0x0023caa0, 0x5)
int bitstream::GetNBitsUsed() const { return m_HighestBitWritten + 1; }
RVA(0x0023cab0, 0xd)
int bitstream::GetNBitsFree() const { return m_DataSizeInBits - GetNBitsUsed(); }
RVA(0x0023cac0, 0x3)
unsigned char* bitstream::GetDataPtr() const { return m_Data; }
RVA(0x0023cad0, 0x12)
int bitstream::IsFull() const { return m_Cursor >= m_DataSizeInBits; }
RVA(0x0023caf0, 0x4)
int bitstream::GetCursor() const { return m_Cursor; }
RVA(0x0023cb00, 0x7)
int bitstream::GetCursorRemaining() const { return m_DataSizeInBits - m_Cursor; }
RVA(0x0023cb10, 0xa)
void bitstream::SetCursor(int bit) { m_Cursor = bit; }

RVA(0x0023cb20, 0x3d)
void bitstream::WriteU64(unsigned __int64 value, int bits)
{
    unsigned int low = static_cast<unsigned int>(value);
    unsigned int high = static_cast<unsigned int>(value >> 32);
    int lowBits = bits < 32 ? bits : 32;
    int highBits = bits - lowBits;
    WriteRaw32(low, lowBits);
    if (highBits > 0)
        WriteRaw32(high, highBits);
}
RVA(0x0023cb60, 0x5)
void bitstream::WriteS32(int value, int bits) { WriteRaw32(value, bits); }
RVA(0x0023cb70, 0x5)
void bitstream::WriteU32(unsigned int value, int bits) { WriteRaw32(value, bits); }
RVA(0x0023cb80, 0xe)
void bitstream::WriteS16(short value, int bits) { WriteRaw32(value, bits); }
RVA(0x0023cb90, 0xe)
void bitstream::WriteU16(unsigned short value, int bits) { WriteRaw32(value, bits); }

RVA(0x0023cba0, 0xd)
void bitstream::WriteMarker() { WriteU32(0xdeadbeef, 32); }
RVA(0x0023cbb0, 0xf)
void bitstream::ReadMarker() const { unsigned int value; ReadU32(value, 32); }
RVA(0x0023cbc0, 0x2a)
void bitstream::WriteRangedS32(int value, int minimum, int maximum)
{
    int bits = GetHighestBit(maximum - minimum);
    WriteU32(value - minimum, bits);
}
RVA(0x0023cbf0, 0x2a)
void bitstream::WriteRangedU32(unsigned int value, int minimum, int maximum)
{
    int bits = GetHighestBit(maximum - minimum);
    WriteU32(value - minimum, bits);
}
RVA(0x0023cc20, 0x5f)
void bitstream::WriteVariableLenS32(int value)
{
    if (WriteFlag(value < 0)) value = -value;
    int bits = GetHighestBit(value);
    int option;
    for (option = 0; option < 16; ++option)
        if (bits <= s_VarLenBitOptions[option]) break;
    WriteU32(option, 4);
    WriteU32(value, s_VarLenBitOptions[option]);
}
RVA(0x0023cc80, 0x4f)
void bitstream::WriteVariableLenU32(unsigned int value)
{
    int bits = GetHighestBit(value);
    int option;
    for (option = 0; option < 16; ++option)
        if (bits <= s_VarLenBitOptions[option]) break;
    WriteU32(option, 4);
    WriteU32(value, s_VarLenBitOptions[option]);
}
RVA(0x0023ccd0, 0x4c)
void bitstream::ReadU64(unsigned __int64& value, int bits) const
{
    int lowBits = bits < 32 ? bits : 32;
    int highBits = bits - lowBits;
    unsigned int low;
    unsigned int high = 0;
    low = ReadRaw32(lowBits);
    if (highBits > 0) high = ReadRaw32(highBits);
    value = (static_cast<unsigned __int64>(high) << 32) | low;
}
RVA(0x0023cd20, 0x3a)
void bitstream::ReadS32(int& value, int bits) const
{
    value = ReadRaw32(bits);
    if (bits != 32 && (value & (1 << (bits - 1))))
        value |= ~((1 << bits) - 1);
}
RVA(0x0023cd60, 0x13)
void bitstream::ReadU32(unsigned int& value, int bits) const { value = ReadRaw32(bits); }
RVA(0x0023cd80, 0x3f)
void bitstream::ReadS16(short& value, int bits) const
{
    value = static_cast<short>(ReadRaw32(bits));
    if (bits != 16 && (value & (1 << (bits - 1))))
        value |= ~((1 << bits) - 1);
}
RVA(0x0023cdc0, 0x14)
void bitstream::ReadU16(unsigned short& value, int bits) const { value = static_cast<unsigned short>(ReadRaw32(bits)); }
RVA(0x0023cde0, 0x34)
void bitstream::ReadRangedS32(int& value, int minimum, int maximum) const
{
    int bits = GetHighestBit(maximum - minimum);
    unsigned int raw;
    ReadU32(raw, bits);
    value = static_cast<int>(raw) + minimum;
}
RVA(0x0023ce20, 0x30)
void bitstream::ReadRangedU32(unsigned int& value, int minimum, int maximum) const
{
    int bits = GetHighestBit(maximum - minimum);
    ReadU32(value, bits);
    value += minimum;
}
RVA(0x0023ce50, 0x4a)
void bitstream::ReadVariableLenS32(int& value) const
{
    int negative = ReadFlag();
    unsigned int raw;
    ReadU32(raw, 4);
    ReadU32(raw, s_VarLenBitOptions[raw]);
    value = raw;
    if (negative) value = -value;
}
RVA(0x0023cea0, 0x37)
void bitstream::ReadVariableLenU32(unsigned int& value) const
{
    unsigned int raw;
    ReadU32(raw, 4);
    ReadU32(raw, s_VarLenBitOptions[raw]);
    value = raw;
}

RVA(0x0023cee0, 0xf)
void bitstream::WriteF32(float value) { WriteRaw32(*reinterpret_cast<unsigned int*>(&value), 32); }
RVA(0x0023cef0, 0x55)
void bitstream::WriteRangedF32(float value, int bits, float minimum, float maximum)
{
    if (bits == 32) { WriteF32(value); return; }
    value -= minimum;
    float range = maximum - minimum;
    int scale = ~(-1 << bits);
    unsigned int raw = static_cast<unsigned int>((value / range) * static_cast<float>(scale) + 0.5f);
    WriteU32(raw, bits);
}
RVA(0x0023cf50, 0x83)
void bitstream::WriteVariableLenF32(float value)
{
    if (value == 0.0f) { WriteFlag(1); WriteFlag(1); return; }
    if (value == static_cast<float>(static_cast<int>(value)))
    {
        WriteFlag(1); WriteFlag(0); WriteVariableLenS32(static_cast<int>(value)); return;
    }
    WriteFlag(0); WriteF32(value);
}
RVA(0x0023cfe0, 0x1a)
void bitstream::ReadF32(float& value) const
{
    unsigned int raw = ReadRaw32(32);
    value = *reinterpret_cast<float*>(&raw);
}
RVA(0x0023d000, 0x69)
void bitstream::ReadRangedF32(float& value, int bits, float minimum, float maximum) const
{
    if (bits == 32) { ReadF32(value); return; }
    float range = maximum - minimum;
    int scale = ~(-1 << bits);
    unsigned int raw;
    ReadU32(raw, bits);
    value = (static_cast<float>(raw) / scale) * range + minimum;
}
RVA(0x0023d070, 0x51)
void bitstream::ReadVariableLenF32(float& value) const
{
    if (ReadFlag())
    {
        if (ReadFlag()) { value = 0.0f; return; }
        int integer;
        ReadVariableLenS32(integer);
        value = static_cast<float>(integer);
        return;
    }
    ReadF32(value);
}
RVA(0x0023d0d0, 0x7c)
void bitstream::TruncateRangedF32(float& value, int bits, float minimum, float maximum)
{
    if (bits == 32) return;
    if (minimum <= value && value <= maximum)
    {
        value -= minimum;
        float range = maximum - minimum;
        int scale = ~(-1 << bits);
        unsigned int raw = static_cast<unsigned int>((value / range) * static_cast<float>(scale) + 0.5f);
        value = (static_cast<float>(raw) / scale) * range + minimum;
    }
}
RVA(0x0023d150, 0x3d)
void bitstream::TruncateRangedVector(vector3& value, int bits, float minimum, float maximum)
{
    TruncateRangedF32(value.X, bits, minimum, maximum);
    TruncateRangedF32(value.Y, bits, minimum, maximum);
    TruncateRangedF32(value.Z, bits, minimum, maximum);
}
RVA(0x0023d190, 0xf)
void bitstream::WriteColor(xcolor value) { WriteU32(*reinterpret_cast<unsigned int*>(&value), 32); }
RVA(0x0023d1a0, 0x37)
void bitstream::ReadColor(xcolor& value) const
{
    unsigned int raw;
    ReadU32(raw,32);
    value = xcolor(raw);
}
RVA(0x0023d1e0, 0x36)
void bitstream::WriteQuaternion(const quaternion& value)
{
    WriteF32(value.X); WriteF32(value.Y); WriteF32(value.Z); WriteF32(value.W);
}
RVA(0x0023d220, 0x34)
void bitstream::ReadQuaternion(quaternion& value) const
{
    ReadF32(value.X); ReadF32(value.Y); ReadF32(value.Z); ReadF32(value.W);
}
RVA(0x0023d260, 0x2b)
void bitstream::WriteVector(const vector3& value)
{
    WriteVariableLenF32(value.X); WriteVariableLenF32(value.Y); WriteVariableLenF32(value.Z);
}
RVA(0x0023d290, 0x4e)
void bitstream::WriteRangedVector(const vector3& value, int bits, float minimum, float maximum)
{
    WriteRangedF32(value.X,bits,minimum,maximum);
    WriteRangedF32(value.Y,bits,minimum,maximum);
    WriteRangedF32(value.Z,bits,minimum,maximum);
}
// Complete body: one x87 store scheduling difference remains in the probe.
RVA(0x0023d2e0, 0x15d)
void bitstream::WriteUnitVector(const vector3& value, int bits)
{
    float pitch, yaw;
    value.GetPitchYaw(pitch,yaw);
    while (yaw < 0) yaw += 6.28318530717958647692f;
    while (yaw > 6.28318530717958647692f) yaw -= 6.28318530717958647692f;
    while (pitch < 0) pitch += 6.28318530717958647692f;
    while (pitch > 6.28318530717958647692f) pitch -= 6.28318530717958647692f;
    if (pitch > 3.14159265358979323846f) pitch -= 6.28318530717958647692f;
    if (pitch < -1.57079632679489661923f) pitch = -1.57079632679489661923f;
    if (pitch > 1.57079632679489661923f) pitch = 1.57079632679489661923f;
    WriteRangedF32(yaw,bits / 2,0.0f,6.28318530717958647692f);
    WriteRangedF32(pitch,bits / 2,-1.57079632679489661923f,1.57079632679489661923f);
}
RVA(0x0023d440, 0x29)
void bitstream::ReadVector(vector3& value) const
{
    ReadVariableLenF32(value.X); ReadVariableLenF32(value.Y); ReadVariableLenF32(value.Z);
}
RVA(0x0023d470, 0x4a)
void bitstream::ReadRangedVector(vector3& value, int bits, float minimum, float maximum) const
{
    ReadRangedF32(value.X,bits,minimum,maximum);
    ReadRangedF32(value.Y,bits,minimum,maximum);
    ReadRangedF32(value.Z,bits,minimum,maximum);
}
RVA(0x0023d4c0, 0x99)
void bitstream::ReadUnitVector(vector3& value, int bits) const
{
    float pitch,yaw;
    ReadRangedF32(yaw,bits / 2,0.0f,6.28318530717958647692f);
    ReadRangedF32(pitch,bits / 2,-1.57079632679489661923f,1.57079632679489661923f);
    value.Set(pitch,yaw);
}
RVA(0x0023d560, 0x2b)
void bitstream::WriteRadian3(const radian3& value)
{
    WriteVariableLenF32(value.Pitch); WriteVariableLenF32(value.Yaw); WriteVariableLenF32(value.Roll);
}
RVA(0x0023d590, 0x29)
void bitstream::ReadRadian3(radian3& value) const
{
    ReadVariableLenF32(value.Pitch); ReadVariableLenF32(value.Yaw); ReadVariableLenF32(value.Roll);
}
RVA(0x0023d5c0, 0xce)
void bitstream::WriteRangedRadian3(const radian3& value, int bits)
{
    float pitch = x_fmod(value.Pitch, 6.28318530717958647692f) * (1.0f / 6.28318530717958647692f) * 0.5f + 0.5f;
    float yaw = x_fmod(value.Yaw, 6.28318530717958647692f) * (1.0f / 6.28318530717958647692f) * 0.5f + 0.5f;
    float roll = x_fmod(value.Roll, 6.28318530717958647692f) * (1.0f / 6.28318530717958647692f) * 0.5f + 0.5f;
    float scale = static_cast<float>(static_cast<unsigned int>(~(-1 << bits)));
    unsigned int p = static_cast<unsigned int>(pitch * scale);
    unsigned int y = static_cast<unsigned int>(yaw * scale);
    unsigned int r = static_cast<unsigned int>(roll * scale);
    WriteU32(p,bits); WriteU32(y,bits); WriteU32(r,bits);
}
RVA(0x0023d690, 0xbd)
void bitstream::ReadRangedRadian3(radian3& value, int bits) const
{
    unsigned int p,y,r;
    ReadU32(p,bits); ReadU32(y,bits); ReadU32(r,bits);
    value.Pitch = (p * (1 / static_cast<float>(static_cast<unsigned int>(~(-1 << bits)))) - 0.5f) * 2 * 6.28318530717958647692f;
    value.Yaw = (y * (1 / static_cast<float>(static_cast<unsigned int>(~(-1 << bits)))) - 0.5f) * 2 * 6.28318530717958647692f;
    value.Roll = (r * (1 / static_cast<float>(static_cast<unsigned int>(~(-1 << bits)))) - 0.5f) * 2 * 6.28318530717958647692f;
}
RVA(0x0023d750, 0x3b)
void bitstream::WriteString(const char* value)
{
    const char* end = value;
    while (*end++) {}
    int length = end - value;
    WriteU32(length, 8);
    WriteRawBits(value, length * 8);
}
RVA(0x0023d790, 0x35)
void bitstream::ReadString(char* value) const
{
    unsigned int length;
    ReadU32(length, 8);
    ReadRawBits(value, length * 8);
    // PC writes at length, rather than the later sibling revision's length - 1.
    value[length] = 0;
}
RVA(0x0023d7d0, 0x3d)
void bitstream::WriteWString(const unsigned short* value)
{
    const unsigned short* end = value;
    while (*end++) {}
    int length = end - value;
    WriteU32(length, 8);
    WriteRawBits(value, length * 16);
}
RVA(0x0023d810, 0x35)
void bitstream::ReadWString(unsigned short* value) const
{
    unsigned int length;
    ReadU32(length, 8);
    ReadRawBits(value, length * 16);
    // PC writes at length, rather than the later sibling revision's length - 1.
    value[length] = 0;
}
RVA(0x0023d850, 0x2c)
void bitstream::WriteMatrix4(const matrix4& value)
{
    const float* current = reinterpret_cast<const float*>(&value);
    for (int i = 0; i < 16; ++i) WriteF32(*current++);
}
RVA(0x0023d880, 0x26)
void bitstream::ReadMatrix4(matrix4& value) const
{
    float* current = reinterpret_cast<float*>(&value);
    for (int i = 0; i < 16; ++i) ReadF32(*current++);
}

RVA(0x0023d8b0, 0x5)
void bitstream::ReadBits(void* data, int bits) const { ReadRawBits(data, bits); }
RVA(0x0023d8c0, 0x5)
void bitstream::WriteBits(const void* data, int bits) { WriteRawBits(data, bits); }
RVA(0x0023d8d0, 0x24)
void bitstream::Clear()
{
    x_memset(m_Data, 0, m_DataSize);
    m_Cursor = 0;
    m_HighestBitWritten = -1;
}
RVA(0x0023d900, 0x23)
void bitstream::AlignCursor(int power)
{
    m_Cursor = (m_Cursor + ((1 << power) - 1)) & -(1 << power);
}
RVA(0x0023d930, 0x5f)
int bitstream::WriteFlag(int value)
{
    if (m_Cursor >= m_DataSizeInBits)
        Grow();
    if (value)
        m_Data[m_Cursor >> 3] |= 1 << (7 - (m_Cursor & 7));
    else
        m_Data[m_Cursor >> 3] &= ~(1 << (7 - (m_Cursor & 7)));
    ++m_Cursor;
    m_HighestBitWritten = m_Cursor - 1 > m_HighestBitWritten ? m_Cursor - 1 : m_HighestBitWritten;
    return value;
}
RVA(0x0023d990, 0x3c)
int bitstream::ReadFlag() const
{
    if (m_Cursor >= m_DataSizeInBits)
        return 0;
    int value = (m_Data[m_Cursor >> 3] & (1 << (7 - (m_Cursor & 7)))) != 0;
    ++m_Cursor;
    return value;
}

RVA(0x0023d9d0, 0x11f)
void bitstream::WriteRawBits(const void* data, int bits)
{
    if (!bits) return;
    while (bits + m_Cursor >= m_DataSizeInBits) Grow();
    int remaining = bits;
    int buffered = 0;
    unsigned int buffer = 0;
    const unsigned char* input = static_cast<const unsigned char*>(data);
    unsigned char* output = m_Data + (m_Cursor >> 3);
    int offset = m_Cursor & 7;
    while (remaining)
    {
        int count = remaining < 8 - offset ? remaining : 8 - offset;
        while (buffered < count)
        {
            buffer |= static_cast<unsigned int>(*input) << buffered;
            ++input;
            buffered += 8;
        }
        unsigned char mask = (0xff >> offset) & (0xff << (8 - (offset + count)));
        unsigned char byte = (buffer & (0xff >> (8 - count))) << ((8 - offset) - count);
        byte = (*output & ~mask) | (byte & mask);
        *output++ = byte;
        offset = 0;
        buffer >>= count;
        buffered -= count;
        remaining -= count;
    }
    m_Cursor += bits;
    m_HighestBitWritten = m_Cursor - 1 > m_HighestBitWritten ? m_Cursor - 1 : m_HighestBitWritten;
}
RVA(0x0023daf0, 0xda)
void bitstream::ReadRawBits(void* data, int bits) const
{
    if (!bits) return;
    int remaining = bits;
    int buffered = 0;
    unsigned int buffer = 0;
    int offset = m_Cursor & 7;
    const unsigned char* input = m_Data + (m_Cursor >> 3);
    unsigned char* output = static_cast<unsigned char*>(data);
    while (remaining)
    {
        int count = remaining < 8 - offset ? remaining : 8 - offset;
        unsigned char byte = *input >> ((8 - offset) - count);
        buffer |= (byte & ~(-1 << count)) << buffered;
        ++input;
        offset = 0;
        buffered += count;
        while (buffered >= 8)
        {
            *output++ = buffer & 0xff;
            buffer >>= 8;
            buffered -= 8;
        }
        remaining -= count;
    }
    if (buffered) *output++ = buffer;
    m_Cursor += bits;
}
RVA(0x0023dbd0, 0xfc)
void bitstream::WriteRaw32(unsigned int value, int bits)
{
    if (!bits) return;
    if (bits + m_Cursor >= m_DataSizeInBits) Grow();
    unsigned char* output = m_Data + (m_Cursor >> 3);
    unsigned char* end = m_Data + ((m_Cursor + bits - 1) >> 3) + 1;
    int left = m_Cursor & 7;
    int right = 40 - left - bits;
    m_Cursor += bits;
    m_HighestBitWritten = m_Cursor - 1 > m_HighestBitWritten ? m_Cursor - 1 : m_HighestBitWritten;
    unsigned __int64 mask = (0xffffffffff >> left) & (0xffffffffff << right);
    unsigned __int64 aligned = (static_cast<unsigned __int64>(value) << right) & mask;
    mask = ~mask;
    int shift = 32;
    while (output != end)
    {
        *output = static_cast<unsigned char>((aligned >> shift) | (*output & (mask >> shift)));
        ++output;
        shift -= 8;
    }
}
RVA(0x0023dcd0, 0xd7)
unsigned int bitstream::ReadRaw32(int bits) const
{
    if (!bits) return 0;
    int left = m_Cursor & 7;
    int right = 40 - left - bits;
    const unsigned char* input = m_Data + (m_Cursor >> 3);
    const unsigned char* end = m_Data + ((m_Cursor + bits - 1) >> 3) + 1;
    m_Cursor += bits;
    unsigned __int64 mask = (0xffffffffff >> left) & (0xffffffffff << right);
    unsigned __int64 accumulated = 0;
    int shift = 32;
    while (input != end)
    {
        accumulated |= static_cast<unsigned __int64>(*input++) << shift;
        shift -= 8;
    }
    return static_cast<unsigned int>((accumulated & mask) >> right);
}
