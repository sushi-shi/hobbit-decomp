// Reconstructed PC reader; see docs/binin-reconstruction.md.

#include <rva.h>

#include <xCore/Auxiliary/Parsing/BinIn.hpp>

#include <xCore/x_files/x_context.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>

#include <stdarg.h>

#define SOURCE_FILE "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\BinIn.cpp"
RVA(0x0023fb10, 0x3b)
bin_in::bin_in() {
    xcontext context("bin_in::bin_in");
    Strings = 0;
    File = 0;
    RowBuffer = 0;
    RowAllocated = 0;
    BufferCapacity = 65536;
    FileBuffer = 0;
}
RVA(0x0023fb50, 0x59)
bin_in::~bin_in() {
    xcontext context("bin_in::~bin_in");
    CloseFile();
}
RVA(0x0023fbb0, 0x9f)
void bin_in::CloseFile() {
    xcontext context("bin_in::CloseFile");
    if (File) {
        x_fclose(File);
        File = 0;
        x_free_fn(Strings, SOURCE_FILE, 54);
        Strings = 0;
        x_free_fn(RowBuffer, SOURCE_FILE, 57);
        RowBuffer = 0;
        RowAllocated = 0;
        x_free_fn(FileBuffer, SOURCE_FILE, 61);
        FileBuffer = 0;
    }
}
RVA(0x0023fc50, 0x1aa)
int bin_in::OpenFile(const char* name) {
    xcontext context("bin_in::OpenFile");
    CloseFile();
    File = x_fopen(name, "rb");
    if (!File) {
        return 0;
    }
    FileLength = x_flength(File);
    if (!FileLength) {
        return 0;
    }
    FileBuffer = static_cast<unsigned char*>(x_malloc_fn(BufferCapacity, SOURCE_FILE, 84));
    FilePosition = 0;
    BufferStart = 0;
    BufferValid = BufferCapacity < FileLength ? BufferCapacity : FileLength;
    x_fread(FileBuffer, BufferValid, 1, File);
    int magic, version, count, bytes;
    FileRead(&magic, 4);
    FileRead(&version, 4);
    if (magic != 666 || version != 0) {
        x_fclose(File);
        File = 0;
        return 0;
    }
    FileRead(&count, 4);
    FileRead(&bytes, 4);
    Strings = static_cast<char**>(x_malloc_fn(bytes + count * sizeof(char*), SOURCE_FILE, 107));
    // The allocation stores the pointer table first, then the raw string bytes.
    char* p = reinterpret_cast<char*>(Strings + count);
    FileRead(p, bytes);
    for (int i = 0; i < count; i++) {
        Strings[i] = p;
        while (*p++)
            ;
    }
    Header.NextBlockOffset = FilePosition;
    return 1;
}
RVA(0x0023fe00, 0x2b)
void bin_in::FileSeek(int position) {
    xcontext context("bin_in::FileSeek");
    FilePosition = position;
}
RVA(0x0023fe30, 0xea)
void bin_in::FileRead(void* destination, int bytes) {
    xcontext context("bin_in::FileRead");
    while (bytes > 0) {
        if (FilePosition < BufferStart || FilePosition >= BufferStart + BufferValid) {
            BufferStart = FilePosition;
            BufferValid = BufferCapacity < FileLength - FilePosition ? BufferCapacity
                                                                     : FileLength - FilePosition;
            x_fseek(File, FilePosition, 0);
            x_fread(FileBuffer, BufferValid, 1, File);
        }
        int n = bytes < BufferValid + (BufferStart - FilePosition)
                    ? bytes
                    : BufferValid + (BufferStart - FilePosition);
        x_memcpy(destination, FileBuffer + (FilePosition - BufferStart), n);
        bytes -= n;
        FilePosition += n;
        destination = static_cast<unsigned char*>(destination) + n;
    }
}
RVA(0x0023ff20, 0x96)
int bin_in::ReadHeader() {
    xcontext context("bin_in::ReadHeader");
    if (!Header.NextBlockOffset) {
        return 0;
    }
    FileSeek(Header.NextBlockOffset);
    FileRead(&Header, sizeof(Header));
    return 1;
}
RVA(0x0023ffc0, 0xd8)
int bin_in::ReadFields() {
    xcontext context("bin_in::ReadFields");
    FileRead(&Row, sizeof(Row));
    if (Row.ByteSize) {
        if (static_cast<unsigned int>(Row.ByteSize) > RowAllocated) {
            RowAllocated = static_cast<unsigned int>(Row.ByteSize) >= 1024
                               ? static_cast<unsigned int>(Row.ByteSize)
                               : 1024;
            RowBuffer = static_cast<unsigned char*>(
                x_realloc_fn(RowBuffer, RowAllocated, SOURCE_FILE, 201)
            );
        }
        FileRead(RowBuffer, Row.ByteSize);
        // Serialized rows contain eight-byte field headers followed by their data.
        CurrentField = reinterpret_cast<field*>(RowBuffer);
        CurrentData = RowBuffer + sizeof(field);
    } else {
        x_free_fn(RowBuffer, SOURCE_FILE, 212);
        RowAllocated = 0;
        RowBuffer = 0;
        CurrentField = 0;
        CurrentData = 0;
    }
    return 1;
}
RVA(0x002400a0, 0x46)
int FieldNameCompare(const char* a, const char* b) {
    int bc = 0, ac = 0;
    while (1) {
        if (*a == ':') {
            break;
        }
        ac = *a++;
        if (ac >= 'A' && ac <= 'Z') {
            ac += 32;
        }
        bc = *b++;
        if (bc >= 'A' && bc <= 'Z') {
            bc += 32;
        }
        if (!bc || bc != ac) {
            break;
        }
    }
    return ac - bc;
}
RVA(0x002400f0, 0xea)
int bin_in::FindField(const char* name) {
    xcontext context("bin_in::FindField");
    field* original = CurrentField;
    if (!original) {
        return 0;
    }
    field* f = original;
    unsigned char* p = CurrentData;
    do {
        f = reinterpret_cast<field*>(p + f->ByteSize);
        if (reinterpret_cast<unsigned char*>(f) >= RowBuffer + Row.ByteSize) {
            f = reinterpret_cast<field*>(RowBuffer);
        }
        p = reinterpret_cast<unsigned char*>(f + 1);
        if (!FieldNameCompare(Strings[f->NameIndex], name)) {
            break;
        }
    } while (f != original);
    if (f == CurrentField) {
        if (FieldNameCompare(Strings[original->NameIndex], name)) {
            return 0;
        }
    } else {
        CurrentField = f;
        CurrentData = p;
    }
    return 1;
}
// The RVA span covers instructions; alignment and compiler switch tables follow.
RVA(0x002401e0, 0x149)
int bin_in::GetField(const char* name, ...) {
    xcontext context("bin_in::GetField");
    if (!FindField(name)) {
        return 0;
    }
    unsigned char* p = CurrentData;
    char* format = x_strchr(name, ':') + 1;
    va_list args;
    va_start(args, name);
    for (int i = 0; i < x_strlen(format); i++) {
        switch (format[i]) {
            case 'F':
            case 'f': {
                float* v = va_arg(args, float*);
                *v = *reinterpret_cast<float*>(p);
                p += 4;
                break;
            }
            case 'D':
            case 'd': {
                int* v = va_arg(args, int*);
                *v = *reinterpret_cast<int*>(p);
                p += 4;
                break;
            }
            case 'G':
            case 'g': {
                unsigned __int64* v = va_arg(args, unsigned __int64*);
                // GUID data stores the high 32-bit word before the low word.
                *v = static_cast<unsigned __int64>(*reinterpret_cast<unsigned int*>(p)) << 32;
                p += 4;
                *v |= *reinterpret_cast<unsigned int*>(p);
                p += 4;
                break;
            }
            case 'S':
            case 's': {
                char* v = va_arg(args, char*);
                x_strcpy(v, reinterpret_cast<char*>(p));
                p += (x_strlen(reinterpret_cast<char*>(p)) + 36) & ~31;
                break;
            }
        }
    }
    va_end(args);
    return 1;
}
RVA(0x00240370, 0x23)
int bin_in::SkipToNextHeader() {
    xcontext context("bin_in::SkipToNextHeader");
    return 1;
}
RVA(0x002403a0, 0x35)
int bin_in::GetVector3(const char* name, vector3& v) {
    if (!FindField(name)) {
        return 0;
    }
    v = *reinterpret_cast<vector3*>(CurrentData);
    return 1;
}
RVA(0x002403e0, 0x3b)
int bin_in::GetColor(const char* name, xcolor& v) {
    if (!FindField(name)) {
        return 0;
    }
    int* p = reinterpret_cast<int*>(CurrentData);
    v.R = static_cast<unsigned char>(p[0]);
    v.G = static_cast<unsigned char>(p[1]);
    v.B = static_cast<unsigned char>(p[2]);
    v.A = static_cast<unsigned char>(p[3]);
    return 1;
}
RVA(0x00240420, 0x29)
int bin_in::GetF32(const char* name, float& v) {
    if (!FindField(name)) {
        return 0;
    }
    v = *reinterpret_cast<float*>(CurrentData);
    return 1;
}
RVA(0x00240450, 0x29)
int bin_in::GetS32(const char* name, int& v) {
    if (!FindField(name)) {
        return 0;
    }
    v = *reinterpret_cast<int*>(CurrentData);
    return 1;
}
RVA(0x00240480, 0x2f)
int bin_in::GetString(const char* name, char* v) {
    if (!FindField(name)) {
        return 0;
    }
    x_strcpy(v, reinterpret_cast<char*>(CurrentData));
    return 1;
}
RVA(0x002404b0, 0x47)
int bin_in::GetBBox(const char* name, bbox& v) {
    if (!FindField(name)) {
        return 0;
    }
    v = *reinterpret_cast<bbox*>(CurrentData);
    return 1;
}
// Descriptive inline helper for the observed two-stage x87 degree conversion.
inline float BinDegToRad(float angle) {
    float radians = angle * 3.14159265358979323846f;
    return radians / 180.0f;
}
RVA(0x00240500, 0x69)
int bin_in::GetRadian3(const char* name, radian3& v) {
    if (!FindField(name)) {
        return 0;
    }
    float* p = reinterpret_cast<float*>(CurrentData);
    v.Pitch = p[0];
    v.Yaw = p[1];
    v.Roll = p[2];
    v.Pitch = BinDegToRad(v.Pitch);
    v.Yaw = BinDegToRad(v.Yaw);
    v.Roll = BinDegToRad(v.Roll);
    return 1;
}
RVA(0x00240570, 0x3b)
int bin_in::GetQuaternion(const char* name, quaternion& v) {
    if (!FindField(name)) {
        return 0;
    }
    float* p = reinterpret_cast<float*>(CurrentData);
    v.X = p[0];
    v.Y = p[1];
    v.Z = p[2];
    v.W = p[3];
    return 1;
}
RVA(0x002405b0, 0x29)
int bin_in::GetBool(const char* name, int& v) {
    if (!FindField(name)) {
        return 0;
    }
    v = *reinterpret_cast<int*>(CurrentData);
    return 1;
}
RVA(0x002405e0, 0x42)
int bin_in::GetGuid(const char* name, unsigned __int64& v) {
    if (!FindField(name)) {
        return 0;
    }
    v = static_cast<unsigned __int64>(reinterpret_cast<unsigned int*>(CurrentData)[0]) << 32;
    v |= reinterpret_cast<unsigned int*>(CurrentData)[1];
    return 1;
}
