#ifndef HOBBIT_BIN_OUT_HPP
#define HOBBIT_BIN_OUT_HPP

#include <xCore/Auxiliary/Parsing/BinFormat.hpp>
#include <xCore/x_files/x_array.hpp>
#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_math.hpp>
#include <xCore/x_files/x_string.hpp>

// Xbox map names this real varargs value union; PC stores establish its alternatives.
union field_types {
    float F;
    int I;
    const char* S;
    unsigned __int64 G;
};

class bin_out {
public:
    struct i_field {
        int NameIndex;
        int ByteSize;
        unsigned char* Data;
    };
    typedef bin_format::row row;
    struct i_row {
        row Header;
        xarray<i_field> Fields;
    };
    typedef bin_format::block block;
    struct i_block {
        block Header;
        xarray<i_row> Rows;
    };
    bin_out();
    ~bin_out();
    void CloseFile();
    int OpenFile(const char* LittleName, const char* BigName);
    // PC writer identity and signature follow CloseFile calls; original name is unresolved.
    // Its complete PC body is reconstructed; exact code generation is still being refined.
    void WriteFile(int BigEndian, X_FILE* File);
    void AddVector3(const char*, const vector3&);
    void AddColor(const char*, xcolor);
    void AddF32(const char*, float);
    void AddS32(const char*, int);
    void AddString(const char*, const char*);
    void AddBBox(const char*, const bbox&);
    void AddRadian3(const char*, const radian3&);
    void AddQuaternion(const char*, const quaternion&);
    void AddBool(const char*, int);
    void AddGuid(const char*, unsigned __int64);
    void AddField(const char* pName, ...);
    void AddEndLine();
    void AddHeader(const char* pName, int Count);
    xarray<xstring> Names;
    xarray<i_block> Blocks;
    X_FILE* LittleEndianFile;
    X_FILE* BigEndianFile;
};

#endif
