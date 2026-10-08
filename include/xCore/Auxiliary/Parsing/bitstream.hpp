#ifndef HOBBIT_BITSTREAM_HPP
#define HOBBIT_BITSTREAM_HPP

#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_math.hpp>

class bitstream
{
public:
    bitstream();
    ~bitstream();
    void Kill();
    void Init(int bytes);
    void Init(const unsigned char* data, int bytes);
    void Grow();
    void SetMaxGrowSize(int bytes);
    int GetNBytes() const;
    int GetNBits() const;
    int GetNBytesUsed() const;
    int GetNBytesFree() const;
    int GetNBitsUsed() const;
    int GetNBitsFree() const;
    unsigned char* GetDataPtr() const;
    int IsFull() const;
    int GetCursor() const;
    int GetCursorRemaining() const;
    void SetCursor(int bit);
    void WriteU64(unsigned __int64 value, int bits = 64);
    void WriteS32(int value, int bits = 32);
    void WriteU32(unsigned int value, int bits = 32);
    void WriteS16(short value, int bits = 16);
    void WriteU16(unsigned short value, int bits = 16);
    void WriteMarker();
    void ReadMarker() const;
    void WriteRangedS32(int value, int minimum, int maximum);
    void WriteRangedU32(unsigned int value, int minimum, int maximum);
    void WriteVariableLenS32(int value);
    void WriteVariableLenU32(unsigned int value);
    void ReadU64(unsigned __int64& value, int bits = 64) const;
    void ReadS32(int& value, int bits = 32) const;
    void ReadU32(unsigned int& value, int bits = 32) const;
    void ReadS16(short& value, int bits = 16) const;
    void ReadU16(unsigned short& value, int bits = 16) const;
    void ReadRangedS32(int& value, int minimum, int maximum) const;
    void ReadRangedU32(unsigned int& value, int minimum, int maximum) const;
    void ReadVariableLenS32(int& value) const;
    void ReadVariableLenU32(unsigned int& value) const;
    void WriteF32(float value);
    void WriteRangedF32(float value, int bits, float minimum, float maximum);
    void WriteVariableLenF32(float value);
    void ReadF32(float& value) const;
    void ReadRangedF32(float& value, int bits, float minimum, float maximum) const;
    void ReadVariableLenF32(float& value) const;
    static void TruncateRangedF32(float& value, int bits, float minimum, float maximum);
    static void TruncateRangedVector(vector3& value, int bits, float minimum, float maximum);
    void WriteColor(xcolor value);
    void ReadColor(xcolor& value) const;
    void WriteQuaternion(const quaternion& value);
    void ReadQuaternion(quaternion& value) const;
    void WriteVector(const vector3& value);
    void WriteRangedVector(const vector3& value, int bits, float minimum, float maximum);
    void ReadVector(vector3& value) const;
    void ReadRangedVector(vector3& value, int bits, float minimum, float maximum) const;
    void WriteRadian3(const radian3& value);
    void ReadRadian3(radian3& value) const;
    void WriteRangedRadian3(const radian3& value, int bits);
    void ReadRangedRadian3(radian3& value, int bits) const;
    void WriteUnitVector(const vector3& value, int bits);
    void ReadUnitVector(vector3& value, int bits) const;
    void WriteString(const char* value);
    void ReadString(char* value) const;
    void WriteWString(const unsigned short* value);
    void ReadWString(unsigned short* value) const;
    void WriteMatrix4(const matrix4& value);
    void ReadMatrix4(matrix4& value) const;
    void Clear();
    void AlignCursor(int power);
    int WriteFlag(int value);
    int ReadFlag() const;
    void WriteBits(const void* data, int bits);
    void ReadBits(void* data, int bits) const;
private:
    void WriteRawBits(const void* data, int bits);
    void ReadRawBits(void* data, int bits) const;
    void WriteRaw32(unsigned int value, int bits);
    unsigned int ReadRaw32(int bits) const;
    // PC ctor/accessors/raw-bit methods establish this seven-word layout.
    unsigned char* m_Data;
    int m_DataSize;
    int m_DataSizeInBits;
    int m_HighestBitWritten;
    int m_bOwnsData;
    int m_MaxGrowSize;
    mutable int m_Cursor;
};
#endif
