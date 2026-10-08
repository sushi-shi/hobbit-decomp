#ifndef HOBBIT_BIN_IN_HPP
#define HOBBIT_BIN_IN_HPP

#include <xCore/Auxiliary/Parsing/BinFormat.hpp>
#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_math.hpp>
#include <xCore/x_files/x_stdio.hpp>

class bin_in {
public:
    bin_in();
    ~bin_in();
    void CloseFile();
    int OpenFile(const char* name);
    int ReadHeader();
    int ReadFields();
    int GetField(const char* name, ...);
    int SkipToNextHeader();
    int GetVector3(const char* name, vector3& value);
    int GetColor(const char* name, xcolor& value);
    int GetF32(const char* name, float& value);
    int GetS32(const char* name, int& value);
    int GetString(const char* name, char* value);
    int GetBBox(const char* name, bbox& value);
    int GetRadian3(const char* name, radian3& value);
    int GetQuaternion(const char* name, quaternion& value);
    int GetBool(const char* name, int& value);
    int GetGuid(const char* name, unsigned __int64& value);

protected:
    // FileRead/FindField access is attested by Xbox decorated symbols.
    void FileSeek(int position);
    void FileRead(void* destination, int bytes);
    int FindField(const char* name);
    // Record/member spellings are descriptive, not recovered original names.
    typedef bin_format::field field;
    char** Strings;
    X_FILE* File;
    bin_format::block Header;
    bin_format::row Row;
    unsigned char* RowBuffer;
    unsigned int RowAllocated;
    field* CurrentField;
    unsigned char* CurrentData;
    int FileLength;
    unsigned char* FileBuffer;
    int BufferCapacity, BufferStart, BufferValid, FilePosition;
};
#endif
