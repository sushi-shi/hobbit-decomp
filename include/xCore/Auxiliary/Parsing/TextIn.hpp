#ifndef HOBBIT_XCORE_AUXILIARY_PARSING_TEXTIN_HPP
#define HOBBIT_XCORE_AUXILIARY_PARSING_TEXTIN_HPP

#include <xCore/Auxiliary/Parsing/tokenizer.hpp>
#include <xCore/x_files/x_color.hpp>
#include <xCore/x_files/x_math.hpp>

class text_in {
public:
    text_in();
    ~text_in();
    int OpenFile(const char* name);
    int OpenFile(X_FILE* file);
    void CloseFile();
    void Rewind();
    int ReadHeader();
    int ReadFields();
    int GetField(const char* name, ...);
    int GetFieldAsString(const char* name, char** strings);
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
    const char* GetHeaderName() const { return m_Record.Name; }
    int GetHeaderCount() const { return m_Record.Count; }
    const char* GetError() const { return s_Error; }
    int IsEOF() { return m_Tokenizer.IsEOF(); }
    const char* GetFileName() { return m_Tokenizer.GetFilename(); }
    int GetFieldCount() const { return m_Record.nFields; }
    const char* GetFieldName(int index) const { return m_Record.Field[index].Name; }
    int GetFieldTypeCount(int index) const { return m_Record.Field[index].nTypes; }
    int GetFieldTypeCount(const char* name);
    int FindField(const char* name);
    void GetFieldTypeStr(int index, char* string);
    void GetFieldTypeStr(const char* name, char* string);

protected:
    enum type {
        TYPE_NULL,
        TYPE_FLOAT,
        TYPE_INTEGER,
        TYPE_STRING,
        TYPE_GUID
    };
    struct data {
        int FieldID;
        int nFloats, nIntegers, nStrings, nGuids;
        float Float[16];
        int Integer[16];
        char String[4][256];
        unsigned __int64 Guid[1];
    };
    struct field {
        char Name[256];
        int ID, nTypes;
        type Type[16];
    };
    struct record {
        char Name[256];
        int Count, nFields, nFieldsAllocated;
        field* Field;
    };
    struct field_match {
        const char* UserString;
        int Index;
    };
    int ReadAllFields();
    int ReportError(char* format, ...);
    int Stricmp(const char* a, const char* b, int count);
    record m_Record;
    token_stream m_Tokenizer;
    int m_nValidFields;
    int m_nFieldMatches;
    int m_nFieldMatchesAllocated;
    field_match* m_FieldMatch;
    int m_RecordLineNumber;
    int m_RecordTypeNumber;
    data* m_pData;
    int m_nDataEntriesAllocated;
    static char s_Error[512];
};
#endif
