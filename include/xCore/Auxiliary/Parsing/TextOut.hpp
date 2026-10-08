
#ifndef HOBBIT_XCORE_AUXILIARY_PARSING_TEXT_OUT_HPP
#define HOBBIT_XCORE_AUXILIARY_PARSING_TEXT_OUT_HPP

//=========================================================================
// INCLUDES
//=========================================================================

#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_math.hpp>
#include <xCore/x_files/x_stdio.hpp>

//=========================================================================
// text_out
//=========================================================================
class text_out {
public:
    text_out(void);
    ~text_out(void);

    int OpenFile(const char* pFileName);
    int OpenFile(X_FILE* pFile);
    void CloseFile(void);

    void AddHeader(const char* pHeaderName, int Count = -1);

    void AddField(const char* pName, ...);
    void AddVector3(const char* pName, const vector3& V);
    void AddColor(const char* pName, xcolor C);
    void AddF32(const char* pName, float F);
    void AddS32(const char* pName, int I);
    void AddString(const char* pName, const char* pStr);
    void AddBBox(const char* pName, const bbox& BBox);
    void AddRadian3(const char* pName, const radian3& Orient);
    void AddQuaternion(const char* pName, const quaternion& Q);
    void AddBool(const char* pName, int Bool);
    void AddGuid(const char* pName, unsigned __int64 Guid);

    void AddEndLine(void);

    inline X_FILE* GetFp(void) {
        return m_Fp;
    }

    //=========================================================================
    // END PUBLIC INTERFACE
    //=========================================================================
protected:
    struct field {
        char Name[256];      // Field description
        int iType;           // where the types starts with in the fieldDesc
        int nTypes;          // How many types in the field
        int HasNegative[64]; // Whether it countains negative digits
        int TotalSpace[64];  // How much space is requiere for each field
        int TotalSpaceBlock; // Total space for this block
    };

    struct type_entry {
        int iOffset;
        int iType;
        short iField;
        short iBackOffset;
        short Length;
        short bDigit;
    };

protected:
    X_FILE* m_Fp; // Pointer to the file

    char* m_pBlock;  // Pointer to the memory allocated
    int m_BlockSize; // How much memory is allocated
    int m_iBlock;    // Current offset from the block

    int m_nFields;      // Number of active fields
    field m_Field[256]; // Fields that we know about

    int m_nTypesPerLine;      // number of types that are for each line
                              // ( m_nTypesPerLine * m_LineCount ) == nTypeEntries
    int m_iTypeEntry;         // Curent entry
    type_entry* m_pTypeEntry; // memory allocated for each of the types

    int m_LineCount; // How many lines does the user says it is going to be
    int m_iLine;     // Curent line that we are doing
    int m_iFiled;    // Curent filed

    char m_BlockName[256]; // Block name
};

//=========================================================================
// end
//=========================================================================
#endif
