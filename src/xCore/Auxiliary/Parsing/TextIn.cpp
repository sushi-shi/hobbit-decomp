#include <rva.h>

#include <xCore/Auxiliary/Parsing/TextIn.hpp>

#include <xCore/x_files/x_context.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>

RVA(0x0023ddb0, 0x7f)
text_in::text_in() {
    xcontext context("text_in::text_in");
    m_pData = 0;
    m_nDataEntriesAllocated = 0;
    m_Record.nFieldsAllocated = 0;
    m_Record.Field = 0;
    m_nFieldMatchesAllocated = 0;
    m_FieldMatch = 0;
}
RVA(0x0023de30, 0xa7)
text_in::~text_in() {
    xcontext context("text_in::~text_in");
    CloseFile();
    delete[] m_pData;
    x_free_fn(m_Record.Field, "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextIn.cpp", 46);
    x_free_fn(m_FieldMatch, "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextIn.cpp", 47);
}
RVA(0x0023dee0, 0x20)
int text_in::ReportError(char* format, ...) {
    xcontext context("text_in::ReportError");
    return 0;
}
RVA(0x0023df00, 0x5d)
void text_in::CloseFile() {
    xcontext context("text_in::CloseFile");
    m_Tokenizer.CloseFile();
}
RVA(0x0023df60, 0xbf)
int text_in::OpenFile(const char* name) {
    xcontext context("text_in::OpenFile");
    m_nValidFields = 0;
    m_Record.nFields = 0;
    m_Record.Name[0] = 0;
    m_Tokenizer.SetDelimeter(":,[]{}()<>");
    if (m_Tokenizer.OpenFile(name) == 0) {
        return ReportError("Unable to open file");
    }
    return 1;
}
RVA(0x0023e020, 0xcc)
int text_in::OpenFile(X_FILE* file) {
    xcontext context("text_in::OpenFile");
    m_nValidFields = 0;
    m_Record.nFields = 0;
    m_Record.Name[0] = 0;
    m_Tokenizer.SetDelimeter(":,[]{}()<>");
    if (m_Tokenizer.OpenFile(file, 0) == 0) {
        return ReportError("Unable to open file");
    }
    return 1;
}
RVA(0x0023e0f0, 0xce)
void text_in::Rewind() {
    xcontext context("text_in::Rewind");
    m_Tokenizer.Rewind();
    if (m_pData) {
        delete[] m_pData;
    }
    m_pData = 0;
    m_nDataEntriesAllocated = 0;
    if (m_Record.Field) {
        x_free_fn(
            m_Record.Field,
            "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextIn.cpp",
            150
        );
    }
    m_Record.nFieldsAllocated = 0;
    m_Record.Field = 0;
    if (m_FieldMatch) {
        x_free_fn(
            m_FieldMatch,
            "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextIn.cpp",
            154
        );
    }
    m_nFieldMatchesAllocated = 0;
    m_FieldMatch = 0;
}

RVA(0x0023e1c0, 0x308)
int text_in::ReadAllFields() {
    xcontext context("text_in::ReadAllFields");
    token_stream::type token;
    m_nValidFields = 0;
    m_Record.nFields = 0;
    token = m_Tokenizer.Read();
    if (token != token_stream::TOKEN_DELIMITER) {
        return 0;
    }
    if (m_Tokenizer.Delimiter() != '{') {
        return ReportError("Puntuation missing. Expecting { found %c", m_Tokenizer.Delimiter());
    }
    if (m_Record.Count == 0) {
        token = m_Tokenizer.Read();
        if (token != token_stream::TOKEN_DELIMITER) {
            return 0;
        }
        if (m_Tokenizer.Delimiter() != '}') {
            return ReportError("Puntuation missing. Expecting } found %c", m_Tokenizer.Delimiter());
        }
        return 1;
    }
    while (1) {
        if (m_Record.nFields >= m_Record.nFieldsAllocated) {
            int count = (m_Record.nFields + 1) + (m_Record.nFields / 2);
            m_Record.nFieldsAllocated = 32 > count ? 32 : count;
            m_Record.Field = static_cast<field*>(x_realloc_fn(
                m_Record.Field,
                sizeof(field) * m_Record.nFieldsAllocated,
                "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextIn.cpp",
                216
            ));
        }
        field& f = m_Record.Field[m_Record.nFields];
        token = m_Tokenizer.Read();
        if (token != token_stream::TOKEN_SYMBOL) {
            if (token == token_stream::TOKEN_DELIMITER && m_Tokenizer.Delimiter() == '}') {
                break;
            }
            return ReportError(
                "Expecting to read a string for the name of the field but found something else"
            );
        }
        x_strncpy(f.Name, m_Tokenizer.String(), 255);
        token = m_Tokenizer.Read();
        if (token != token_stream::TOKEN_DELIMITER) {
            return ReportError("Puntuation missing. Expecting : found somthing else");
        }
        if (m_Tokenizer.Delimiter() != ':') {
            return ReportError("Puntuation missing. Expecting : found %c", m_Tokenizer.Delimiter());
        }
        token = m_Tokenizer.Read();
        if (token != token_stream::TOKEN_SYMBOL) {
            return ReportError("Expecting the type of the field but found something else");
        }
        {
            char* p = m_Tokenizer.String();
            f.nTypes = 0;
            f.ID = -1;
            while (*p) {
                switch (*p) {
                    case 'd':
                    case 'D':
                        f.Type[f.nTypes++] = TYPE_INTEGER;
                        break;
                    case 'f':
                    case 'F':
                        f.Type[f.nTypes++] = TYPE_FLOAT;
                        break;
                    case 'S':
                    case 's':
                        f.Type[f.nTypes++] = TYPE_STRING;
                        break;
                    case 'G':
                    case 'g':
                        f.Type[f.nTypes++] = TYPE_GUID;
                        break;
                    default:
                        return ReportError("Unkown type %c from %s field", *p, f.Name);
                }
                ++p;
            }
        }
        ++m_Record.nFields;
    }
    return 1;
}

RVA(0x0023e510, 0x186)
int text_in::ReadHeader() {
    xcontext context("text_in::ReadHeader");
    token_stream::type token;
    m_Record.Name[0] = 0;
    m_Record.Count = -1;
    m_RecordLineNumber = 0;
    m_nFieldMatches = 0;
    while (1) {
        token = m_Tokenizer.Read();
        if (token == token_stream::TOKEN_EOF || token == token_stream::TOKEN_NONE) {
            return 0;
        }
        if (token == token_stream::TOKEN_DELIMITER && m_Tokenizer.Delimiter() == '[') {
            break;
        }
    }
    token = m_Tokenizer.Read();
    if (token != token_stream::TOKEN_SYMBOL) {
        return ReportError("Expecting a string found something else");
    }
    x_strcpy(m_Record.Name, m_Tokenizer.String());
    token = m_Tokenizer.Read();
    if (token != token_stream::TOKEN_DELIMITER) {
        return ReportError("Expecting a ] or : But found something else");
    }
    if (m_Tokenizer.Delimiter() == ']') {
        return ReadAllFields();
    }
    if (m_Tokenizer.Delimiter() != ':') {
        return ReportError("Expecting a ] or : But found %c", m_Tokenizer.Delimiter());
    }
    token = m_Tokenizer.Read();
    if (token != token_stream::TOKEN_NUMBER) {
        return ReportError("After : specting an integer but found something else");
    }
    m_Record.Count = m_Tokenizer.Int();
    token = m_Tokenizer.Read();
    if (token != token_stream::TOKEN_DELIMITER) {
        return ReportError("Expecting ] But stead found something else");
    }
    return ReadAllFields();
}

RVA(0x0023e6a0, 0x2b4)
int text_in::ReadFields() {
    xcontext context("text_in::ReadFields");
    if (m_Record.nFields > m_nDataEntriesAllocated) {
        delete[] m_pData;
        m_nDataEntriesAllocated = m_Record.nFields;
        m_pData = new data[m_nDataEntriesAllocated];
    }
    for (int f = 0; f < m_Record.nFields; ++f) {
        field& fieldEntry = m_Record.Field[f];
        data& values = m_pData[f];
        values.nFloats = 0;
        values.nIntegers = 0;
        values.nStrings = 0;
        values.nGuids = 0;
        for (int i = 0; i < fieldEntry.nTypes; ++i) {
            token_stream::type token = m_Tokenizer.Read();
            switch (fieldEntry.Type[i]) {
                case TYPE_FLOAT:
                    if (token != token_stream::TOKEN_NUMBER && m_Tokenizer.IsFloat() == 1) {
                        return ReportError("Expecting a FLOAT but found something else");
                    }
                    values.Float[values.nFloats++] = m_Tokenizer.Float();
                    break;
                case TYPE_INTEGER:
                    if (token != token_stream::TOKEN_NUMBER && m_Tokenizer.IsFloat() == 0) {
                        return ReportError("Expecting a INTEGER but found something else");
                    }
                    values.Integer[values.nIntegers++] = m_Tokenizer.Int();
                    break;
                case TYPE_STRING:
                    if (token != token_stream::TOKEN_STRING) {
                        return ReportError("Expecting a STRING but found something else");
                    }
                    x_strcpy(values.String[values.nStrings++], m_Tokenizer.String());
                    break;
                case TYPE_GUID: {
                    if (token != token_stream::TOKEN_STRING) {
                        return ReportError("Expecting a GUID but found something else");
                    }
                    unsigned __int64 guid = 0;
                    const char* p = m_Tokenizer.String();
                    while (*p) {
                        char c = *p;
                        ++p;
                        if (c == ':') {
                            continue;
                        }
                        unsigned int digit = 0;
                        if (c > '9') {
                            digit = (c - 'A') + 10;
                        } else {
                            digit = c - '0';
                        }
                        guid <<= 4;
                        guid |= digit & 15;
                    }
                    values.Guid[values.nGuids++] = guid;
                    break;
                }
                default:
                    break;
            }
        }
    }
    ++m_RecordLineNumber;
    m_RecordTypeNumber = 0;
    return 1;
}

RVA(0x0023e970, 0x99)
int text_in::Stricmp(const char* a, const char* b, int count) {
    xcontext context("text_in::Stricmp");
    char buffer[256];
    x_strncpy(buffer, b, count);
    buffer[count] = 0;
    return x_stricmp(a, buffer);
}

#include <stdarg.h>
#include <string.h>
RVA(0x0023ea10, 0x3fb)
int text_in::GetField(const char* name, ...) {
    xcontext context("text_in::GetField");
    if (m_RecordLineNumber == 1) {
        int index = -1;
        int j;
        char* format;
        format = x_strstr(name, ":");
        if (format == 0) {
            return ReportError("User forgot to put the type in %s field", name);
        }
        {
            int length = format - name;
            for (j = 0; j < m_Record.nFields; ++j) {
                if (Stricmp(m_Record.Field[j].Name, name, length) == 0) {
                    if (m_Record.Field[j].ID != -1) {
                        return ReportError("User register the same type (%s) twice", name);
                    }
                    m_Record.Field[j].ID = 0;
                    index = j;
                    ++m_nValidFields;
                    break;
                }
            }
            if (j == m_Record.nFields) {
                return ReportError("The field %s was not found in the file", name);
            }
        }
        {
            int validTypes = 0;
            field& f = m_Record.Field[index];
            ++format;
            while (format[validTypes]) {
                type t;
                switch (format[validTypes]) {
                    case 'd':
                    case 'D':
                        t = TYPE_INTEGER;
                        break;
                    case 'f':
                    case 'F':
                        t = TYPE_FLOAT;
                        break;
                    case 'S':
                    case 's':
                        t = TYPE_STRING;
                        break;
                    case 'G':
                    case 'g':
                        t = TYPE_GUID;
                        break;
                    default:
                        return ReportError(
                            "Unkown type %c from %s field",
                            format[validTypes],
                            f.Name
                        );
                }
                if (t != f.Type[validTypes++]) {
                    return ReportError("For %s field the types don't match.", f.Name);
                }
            }
            if (validTypes != f.nTypes) {
                return ReportError(
                    "For field %s the types are different from the user specify one.",
                    f.Name
                );
            }
            if (m_RecordTypeNumber >= m_nFieldMatchesAllocated) {
                int count = (m_RecordTypeNumber + 1) + (m_RecordTypeNumber / 2);
                m_nFieldMatchesAllocated = 32 > count ? 32 : count;
                m_FieldMatch = static_cast<field_match*>(x_realloc_fn(
                    m_FieldMatch,
                    sizeof(field_match) * m_nFieldMatchesAllocated,
                    "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextIn.cpp",
                    658
                ));
            }
            m_FieldMatch[m_RecordTypeNumber].UserString = name;
            m_FieldMatch[m_RecordTypeNumber].Index = index;
        }
    }
    if (m_FieldMatch[m_RecordTypeNumber].UserString == name) {
        data& values = m_pData[m_FieldMatch[m_RecordTypeNumber].Index];
        field& f = m_Record.Field[m_FieldMatch[m_RecordTypeNumber].Index];
        if (strstr(name, f.Name) == 0) {
            return 0;
        }
        va_list args;
        va_start(args, name);
        int I = 0, F = 0, S = 0, G = 0;
        for (int i = 0; i < f.nTypes; ++i) {
            switch (f.Type[i]) {
                case TYPE_INTEGER: {
                    int* p = va_arg(args, int*);
                    *p = values.Integer[I++];
                    break;
                }
                case TYPE_FLOAT: {
                    float* p = va_arg(args, float*);
                    *p = values.Float[F++];
                    break;
                }
                case TYPE_STRING: {
                    char* p = va_arg(args, char*);
                    x_strcpy(p, values.String[S++]);
                    break;
                }
                case TYPE_GUID: {
                    unsigned __int64* p = va_arg(args, unsigned __int64*);
                    *p = values.Guid[G++];
                    break;
                }
            }
        }
    } else {
        return 0;
    }
    ++m_RecordTypeNumber;
    return 1;
}

#include <xCore/x_files/x_string.hpp>
RVA(0x0023ee60, 0x46e)
int text_in::GetFieldAsString(const char* name, char** strings) {
    xcontext context("text_in::GetFieldAsString");
    if (m_RecordLineNumber == 1) {
        int index = -1;
        int j;
        char* format;
        format = x_strstr(name, ":");
        if (format == 0) {
            return ReportError("User forgot to put the type in %s field", name);
        }
        {
            int length = format - name;
            for (j = 0; j < m_Record.nFields; ++j) {
                if (Stricmp(m_Record.Field[j].Name, name, length) == 0) {
                    if (m_Record.Field[j].ID != -1) {
                        return ReportError("User register the same type (%s) twice", name);
                    }
                    m_Record.Field[j].ID = 0;
                    index = j;
                    ++m_nValidFields;
                    break;
                }
            }
            if (j == m_Record.nFields) {
                return ReportError("The field %s was not found in the file", name);
            }
        }
        {
            int validTypes = 0;
            field& f = m_Record.Field[index];
            ++format;
            while (format[validTypes]) {
                type t;
                switch (format[validTypes]) {
                    case 'd':
                    case 'D':
                        t = TYPE_INTEGER;
                        break;
                    case 'f':
                    case 'F':
                        t = TYPE_FLOAT;
                        break;
                    case 'S':
                    case 's':
                        t = TYPE_STRING;
                        break;
                    case 'G':
                    case 'g':
                        t = TYPE_GUID;
                        break;
                    default:
                        return ReportError(
                            "Unkown type %c from %s field",
                            format[validTypes],
                            f.Name
                        );
                }
                if (t != f.Type[validTypes++]) {
                    return ReportError("For %s field the types don't match.", f.Name);
                }
            }
            if (validTypes != f.nTypes) {
                return ReportError(
                    "For field %s the types are different from the user specify one.",
                    f.Name
                );
            }
            if (m_RecordTypeNumber >= m_nFieldMatchesAllocated) {
                int count = (m_RecordTypeNumber + 1) + (m_RecordTypeNumber / 2);
                m_nFieldMatchesAllocated = 32 > count ? 32 : count;
                m_FieldMatch = static_cast<field_match*>(x_realloc_fn(
                    m_FieldMatch,
                    sizeof(field_match) * m_nFieldMatchesAllocated,
                    "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextIn.cpp",
                    841
                ));
            }
            m_FieldMatch[m_RecordTypeNumber].UserString = name;
            m_FieldMatch[m_RecordTypeNumber].Index = index;
        }
    }

    if (x_strcmp(m_FieldMatch[m_RecordTypeNumber].UserString, name) == 0) {
        data& values = m_pData[m_FieldMatch[m_RecordTypeNumber].Index];
        field& f = m_Record.Field[m_FieldMatch[m_RecordTypeNumber].Index];
        int I = 0, F = 0, S = 0, G = 0;
        for (int i = 0; i < f.nTypes; ++i) {
            switch (f.Type[i]) {
                case TYPE_INTEGER: {
                    char* p = strings[i];
                    x_strcpy(p, xfs("%d", values.Integer[I++]));
                    break;
                }
                case TYPE_FLOAT: {
                    char* p = strings[i];
                    x_strcpy(p, xfs("%f", values.Float[F++]));
                    break;
                }
                case TYPE_STRING: {
                    char* p = strings[i];
                    x_strcpy(p, values.String[S++]);
                    break;
                }
                case TYPE_GUID: {
                    char* p = strings[i];
                    x_strcpy(p, xfs("%ld", values.Guid[G++]));
                    break;
                }
            }
        }
    } else {
        return 0;
    }
    ++m_RecordTypeNumber;
    return 1;
}

RVA(0x0023f330, 0x79)
int text_in::SkipToNextHeader() {
    xcontext context("text_in::SkipToNextHeader");
    int i, n;
    if (m_Record.Count == -1) {
        n = 1;
    } else {
        n = m_Record.Count;
    }
    for (i = 0; i < n; ++i) {
        ReadFields();
    }
    return 1;
}
RVA(0x0023f3b0, 0x7c)
int text_in::GetVector3(const char* name, vector3& value) {
    return GetField(xfs("%s:fff", name), &value.X, &value.Y, &value.Z);
}
RVA(0x0023f430, 0xa5)
int text_in::GetColor(const char* name, xcolor& value) {
    int R, G, B, A;
    int result = GetField(xfs("%s:dddd", name), &R, &G, &B, &A);
    value.R = static_cast<unsigned char>(R & 255);
    value.G = static_cast<unsigned char>(G & 255);
    value.B = static_cast<unsigned char>(B & 255);
    value.A = static_cast<unsigned char>(A & 255);
    return result;
}
RVA(0x0023f4e0, 0x74)
int text_in::GetF32(const char* name, float& value) {
    return GetField(xfs("%s:f", name), &value);
}
RVA(0x0023f560, 0x74)
int text_in::GetS32(const char* name, int& value) {
    return GetField(xfs("%s:d", name), &value);
}
RVA(0x0023f5e0, 0x74)
int text_in::GetString(const char* name, char* value) {
    return GetField(xfs("%s:s", name), value);
}
RVA(0x0023f660, 0x88)
int text_in::GetBBox(const char* name, bbox& value) {
    return GetField(
        xfs("%s:ffffff", name),
        &value.Min.X,
        &value.Min.Y,
        &value.Min.Z,
        &value.Max.X,
        &value.Max.Y,
        &value.Max.Z
    );
}
inline float DegToRad(float angle) {
    float radians = angle * 3.14159265358979323846f;
    return radians / 180.0f;
}
RVA(0x0023f6f0, 0xbd)
int text_in::GetRadian3(const char* name, radian3& value) {
    float P, Y, R;
    int result = GetField(xfs("%s:fff", name), &P, &Y, &R);
    value.Pitch = DegToRad(P);
    value.Yaw = DegToRad(Y);
    value.Roll = DegToRad(R);
    return result;
}
RVA(0x0023f7b0, 0x80)
int text_in::GetQuaternion(const char* name, quaternion& value) {
    return GetField(xfs("%s:ffff", name), &value.X, &value.Y, &value.Z, &value.W);
}
RVA(0x0023f830, 0x74)
int text_in::GetBool(const char* name, int& value) {
    return GetField(xfs("%s:d", name), &value);
}
RVA(0x0023f8b0, 0x74)
int text_in::GetGuid(const char* name, unsigned __int64& value) {
    return GetField(xfs("%s:g", name), &value);
}
RVA(0x0023f930, 0x30)
int text_in::GetFieldTypeCount(const char* name) {
    int index = FindField(name);
    if (index != -1) {
        return m_Record.Field[index].nTypes;
    }
    return -1;
}
RVA(0x0023f960, 0x52)
int text_in::FindField(const char* name) {
    for (int i = 0; i < m_Record.nFields; ++i) {
        if (x_stricmp(m_Record.Field[i].Name, name) == 0) {
            return i;
        }
    }
    return -1;
}
RVA(0x0023f9c0, 0x106)
void text_in::GetFieldTypeStr(int index, char* string) {
    xcontext context("text_in::GetFieldTypeStr");
    if (!string) {
        return;
    }
    x_memset(string, 0, m_Record.Field[index].nTypes + 1);
    for (int i = 0; i < m_Record.Field[index].nTypes; ++i) {
        switch (m_Record.Field[index].Type[i]) {
            case TYPE_NULL:
                string[i] = '*';
                break;
            case TYPE_FLOAT:
                string[i] = 'f';
                break;
            case TYPE_INTEGER:
                string[i] = 'd';
                break;
            case TYPE_STRING:
                string[i] = 's';
                break;
            case TYPE_GUID:
                string[i] = 'g';
                break;
        }
    }
}
RVA(0x0023fae0, 0x23)
void text_in::GetFieldTypeStr(const char* name, char* string) {
    int index = FindField(name);
    if (index != -1) {
        GetFieldTypeStr(index, string);
    }
}
