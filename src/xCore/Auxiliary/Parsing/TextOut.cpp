#include <rva.h>

#include <xCore/Auxiliary/Parsing/TextOut.hpp>

#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_string.hpp>

#include <stdarg.h>

#define NULL 0
#define ASSERT(x) static_cast<void>(0)
#define x_va_list va_list
#define x_va_start va_start
#define x_va_arg va_arg
#define RAD_TO_DEG(A) static_cast<float>(((A) * 180.0f) / 3.1415926535897932384626433832795f)

//==============================================================================

#if !(defined(TARGET_PS2) && defined(CONFIG_RETAIL))

//==============================================================================

//=========================================================================

RVA(0x00242400, 0x17)
text_out::text_out(void) {
    x_memset(this, 0, sizeof(*this));
}

//=========================================================================

RVA(0x00242420, 0x5c)
text_out::~text_out(void) {
    if (m_pBlock) {
        x_free_fn(m_pBlock, "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextOut.cpp", 19);
    }
    if (m_pTypeEntry) {
        x_free_fn(
            m_pTypeEntry,
            "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextOut.cpp",
            20
        );
    }
    if (m_Fp) {
        x_fclose(m_Fp);
    }

    m_pBlock = NULL;
    m_pTypeEntry = NULL;
    m_Fp = NULL;
}

//=========================================================================

RVA(0x00242480, 0x5)
void text_out::CloseFile(void) {
    this->~text_out();
}

//=========================================================================

RVA(0x00242490, 0x24)
int text_out::OpenFile(const char* pFileName) {
    ASSERT(pFileName);
    m_Fp = x_fopen(pFileName, "wt");
    return m_Fp != NULL;
}

//=========================================================================

RVA(0x002424c0, 0xe)
int text_out::OpenFile(X_FILE* pFile) {
    m_Fp = pFile;
    return 1;
}

RVA(0x002424d0, 0xbf)
void text_out::AddHeader(const char* pHeaderName, int Count) {
    //
    // Set up
    //
    m_LineCount = (Count < 0) ? 1 : Count;
    m_iLine = 0;
    m_iTypeEntry = 0;
    m_nTypesPerLine = 512; // Bogus initial guess
    m_iFiled = 0;
    m_nFields = 0;
    m_iBlock = 0;
    m_BlockSize = m_nTypesPerLine * 1 * 640;
    m_pBlock = static_cast<char*>(x_malloc_fn(
        m_BlockSize,
        "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextOut.cpp",
        71
    ));
    m_pTypeEntry = static_cast<type_entry*>(x_malloc_fn(
        sizeof(type_entry) * m_BlockSize,
        "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextOut.cpp",
        72
    ));
    ASSERT(m_pBlock && m_pTypeEntry);

    //
    // Save the block name
    //
    if (Count < 0) {
        x_sprintf(m_BlockName, "[ %s ]\n", pHeaderName);
    } else {
        x_sprintf(m_BlockName, "[ %s : %d ]\n", pHeaderName, Count);
    }
    // If header has zero entries then just flush header
    if (Count == 0) {
        AddEndLine();
    }
}

//=========================================================================

RVA(0x00242590, 0x35c)
void text_out::AddField(const char* pName, ...) {
    int i;

    ASSERT(pName);
    ASSERT(!x_strchr(pName, ' '));
    ASSERT(m_iLine < m_LineCount);

    if (m_iLine == 0) {
        //
        // Cleat the filed just in case
        //
        x_memset(&m_Field[m_nFields], 0, sizeof(field));

        //
        // First some sanity check. Lets make sure that the user doesn't
        // give us the same filed twice.
        //
        for (i = 0; i < m_nFields; i++) {
            // This is an assert because it should not happen when if given to the user
            ASSERT(x_stricmp(pName, m_Field[i].Name) != 0);
        }

        //
        // Okay now lets find where the types as with in the file
        //
        {
            x_strcpy(m_Field[m_nFields].Name, pName);
            char* pOff = x_strstr(m_Field[m_nFields].Name, ":");
            ASSERT(pOff && "missing Puntuation");
            m_Field[m_nFields].iType = pOff - m_Field[m_nFields].Name + 1;
        }

        //
        // Make sure to conver to lower case the type ( much easier to deal with )
        //
        m_Field[m_nFields].nTypes = 0;
        for (i = m_Field[m_nFields].iType; m_Field[m_nFields].Name[i]; i++) {
            if (m_Field[m_nFields].Name[i] >= 'A' && m_Field[m_nFields].Name[i] <= 'Z') {
                m_Field[m_nFields].Name[i] += 'A' - 'a';
            }

            // increment the number of types
            m_Field[m_nFields].nTypes++;
        }

        // Increment the number of fields that we know about
        m_nFields++;
    }

    //
    // read the field description
    //
    x_va_list Args;
    field& Field = m_Field[m_iFiled++];

    x_va_start(Args, pName);

    for (i = 0; i < Field.nTypes; i++) {
        // UNUSED (SH): int   nInt = 0;
        switch (Field.Name[Field.iType + i]) {
            case 'f': {
                double p = (x_va_arg(Args, double)); // get the type

                if (p < 0) {
                    Field.HasNegative[i] = true;
                }

                m_pTypeEntry[m_iTypeEntry].Length = x_sprintf(&m_pBlock[m_iBlock], "%f", p);
                m_pTypeEntry[m_iTypeEntry].bDigit = true;

                break;
            }

            case 'd': {
                int p = (x_va_arg(Args, int)); // get the type

                if (p < 0) {
                    Field.HasNegative[i] = true;
                }

                m_pTypeEntry[m_iTypeEntry].Length = x_sprintf(&m_pBlock[m_iBlock], "%d", p);
                m_pTypeEntry[m_iTypeEntry].bDigit = true;

                break;
            }

            case 's': {
                const char* p = (x_va_arg(Args, const char*)); // get the type

                m_pTypeEntry[m_iTypeEntry].Length = x_sprintf(&m_pBlock[m_iBlock], "\"%s\"", p);
                m_pTypeEntry[m_iTypeEntry].bDigit = false;

                break;
            }
            case 'g': {
                unsigned __int64 g = (x_va_arg(Args, unsigned __int64));

                m_pTypeEntry[m_iTypeEntry].Length = x_sprintf(
                    &m_pBlock[m_iBlock],
                    "\"%08X:%08X\"",
                    static_cast<unsigned int>((g >> 32) & 0xFFFFFFFF),
                    static_cast<unsigned int>((g >> 0) & 0xFFFFFFFF)
                );
                m_pTypeEntry[m_iTypeEntry].bDigit = true;

                break;
            }
            default:
                ASSERT(0 && "Wrong type");
        }

        //
        // Fill up the rest of the properties
        //

        // Record the rest of the info
        m_pTypeEntry[m_iTypeEntry].iOffset = m_iBlock;
        m_pTypeEntry[m_iTypeEntry].iField = m_iFiled - 1;
        m_pTypeEntry[m_iTypeEntry].iType = i;
        m_pTypeEntry[m_iTypeEntry].iBackOffset = ((i + 1) == Field.nTypes) ? 0 : 1;

        // Track the total spaces
        int TS = m_pTypeEntry[m_iTypeEntry].Length + m_pTypeEntry[m_iTypeEntry].iBackOffset
                 + (m_pTypeEntry[m_iTypeEntry].bDigit && m_pBlock[m_iBlock] != '-');

        if (Field.TotalSpace[i] < TS) {
            Field.TotalSpace[i] = TS;
        }

        // move the block
        m_iBlock += m_pTypeEntry[m_iTypeEntry].Length;

        // move the entry
        m_iTypeEntry++;

        ASSERT(m_iBlock <= m_BlockSize);
        ASSERT(m_iTypeEntry <= m_nTypesPerLine * m_LineCount);
    }
}

//=========================================================================

RVA(0x00242c70, 0x4b8)
void text_out::AddEndLine(void) {
    // Increment the line count
    m_iLine++;
    m_iFiled = 0;

    //
    // dump the hold thing to file if we are done
    //
    if ((m_LineCount == 0) || (m_iLine == m_LineCount)) {
        DATA(0x002f6be8)
        static const char SpaceArray[] = {
            "                                                                                      "
            "                                                                                      "
            "     "
        };
        char BlockInfo[6000] = {0};
        char OutLine[6000] = {0};

        // Prs32 the very to header
        x_fprintf(m_Fp, "%s", m_BlockName);

        // Get how big each block is going to be
        int j;

        for (j = 0; j < m_nFields; j++) {
            field& Field = m_Field[j];

            Field.TotalSpaceBlock = 0;
            for (int k = 0; k < Field.nTypes; k++) {
                Field.TotalSpaceBlock += Field.TotalSpace[k];
            }

            // Add one space to separe the fields
            m_Field[j].TotalSpaceBlock += 3;

            // The minimun size is the size of the string descriving the block
            int l1 = x_strlen(Field.Name) + 1;
            if (Field.TotalSpaceBlock < l1) {
                Field.TotalSpaceBlock = l1;
            }

            // Copy the block infd stuff
            int l2 = x_strlen(BlockInfo);
            int l3 = x_sprintf(&BlockInfo[l2], "%s", Field.Name);
            int l4 = Field.TotalSpaceBlock - l3;
            int m;

            for (m = 0; m < l4; m++) {
                ASSERT(l2 + l3 + m < 6000);
                BlockInfo[l2 + l3 + m] = ' ';
            }

            // End string for now
            BlockInfo[l2 + l3 + m] = 0;
        }

        // Create the outline
        {
            int l1 = x_strlen(BlockInfo);
            int bO = 0;
            for (j = 0; j < l1; j++) {
                OutLine[j] = '-';

                if (bO == 0 && BlockInfo[j + 1] == ' ') {
                    bO = 1;
                }
                if (bO == 1 && BlockInfo[j + 1] != ' ') {
                    bO = 2;
                }

                if (bO == 2 && BlockInfo[j + 1] != 0) {
                    OutLine[j] = ' ';
                    bO = 0;
                }
            }

            OutLine[j] = 0;
        }

        x_fprintf(m_Fp, " { %s }\n", BlockInfo);
        x_fprintf(m_Fp, "// %s\n   ", OutLine);

        int iTypeEntry = 0;
        for (int i = 0; i < m_LineCount; i++) {
            for (int j = 0; j < m_nFields; j++) {
                const field& Field = m_Field[j];
                int FieldTotal = 0;

                for (int k = 0; k < Field.nTypes; k++, iTypeEntry++) {
                    const type_entry& Entry = m_pTypeEntry[iTypeEntry];
                    // UNUSED(SH): int                iField   = Entry.iField;
                    int iType = Entry.iType;
                    int Count = 0;
                    int TotalSpace;

                    // Write spaces at front
                    if (Entry.bDigit && Field.HasNegative[k] && m_pBlock[Entry.iOffset] != '-') {
                        Count += x_fwrite(SpaceArray, 1, 1, m_Fp);
                    }

                    // Write field in file
                    Count += x_fwrite(&m_pBlock[Entry.iOffset], 1, Entry.Length, m_Fp);

                    // Set the spaces to separeate the filed blocks
                    TotalSpace = Field.TotalSpace[iType];

                    // Write spaces for the field
                    if (Count < TotalSpace) {
                        Count += x_fwrite(SpaceArray, 1, TotalSpace - Count, m_Fp);
                    }

                    // Set the total so far
                    FieldTotal += Count;
                }

                ASSERT(FieldTotal <= Field.TotalSpaceBlock);

                // Make sure that we have all the requiere spaces
                if (FieldTotal < Field.TotalSpaceBlock) {
                    x_fwrite(SpaceArray, 1, Field.TotalSpaceBlock - FieldTotal, m_Fp);
                }
            }

            // Add from time to time a info block
            if (((i + 1) % 80) == 0 && (m_LineCount - i) > 10) {
                x_fprintf(m_Fp, "\n");
                x_fprintf(m_Fp, "// %s\n", OutLine);
                x_fprintf(m_Fp, "// %s\n", BlockInfo);
                x_fprintf(m_Fp, "// %s\n   ", OutLine);
            } else {
                x_fprintf(m_Fp, "\n   ");
            }
        }

        // done here
        x_fprintf(m_Fp, "\n");

        x_free_fn(m_pBlock, "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextOut.cpp", 523);
        x_free_fn(
            m_pTypeEntry,
            "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextOut.cpp",
            524
        );
        m_pBlock = NULL;
        m_pTypeEntry = NULL;
        // done
        return;
    }

    //
    // Okay now we should have a pretty good idea on how many types per line
    // are gos32 to be we can allocate the right amount
    //

    if (m_iLine == 1) {
        m_nTypesPerLine = m_iTypeEntry;
        m_pTypeEntry = static_cast<type_entry*>(x_realloc_fn(
            m_pTypeEntry,
            m_nTypesPerLine * m_LineCount * sizeof(type_entry),
            "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextOut.cpp",
            539
        ));
        m_BlockSize = m_nTypesPerLine * m_LineCount * 256;
        m_pBlock = static_cast<char*>(x_realloc_fn(
            m_pBlock,
            m_BlockSize,
            "C:\\projects\\meridian\\xCore\\Auxiliary\\Parsing\\TextOut.cpp",
            541
        ));
        ASSERT(m_pBlock && m_pTypeEntry);
    }
}
//=========================================================================
// THIS ARE REALLY BAD FUNCTIONS TO BE CALLING. THEY WASTE WAY TO MUCH TIME
// CONSTRUCTING STRINGS. MUCH BETTER IF DONE WITH MACROS.
//=========================================================================

//=========================================================================

RVA(0x00243130, 0x82)
void text_out::AddVector3(const char* pName, const vector3& V) {
    AddField(xfs("%s:fff", pName), V.X, V.Y, V.Z);
}

//=========================================================================

RVA(0x002431c0, 0x87)
void text_out::AddColor(const char* pName, xcolor C) {
    AddField(xfs("%s:dddd", pName), C.R, C.G, C.B, C.A);
}

//=========================================================================

RVA(0x00243250, 0x72)
void text_out::AddF32(const char* pName, float F) {
    AddField(xfs("%s:f", pName), F);
}

//=========================================================================

RVA(0x002432d0, 0x70)
void text_out::AddS32(const char* pName, int I) {
    AddField(xfs("%s:d", pName), I);
}

//=========================================================================

RVA(0x00243340, 0x70)
void text_out::AddString(const char* pName, const char* pStr) {
    AddField(xfs("%s:s", pName), pStr);
}

//=========================================================================

RVA(0x002433b0, 0x9e)
void text_out::AddBBox(const char* pName, const bbox& BBox) {
    AddField(
        xfs("%s:ffffff", pName),
        BBox.Min.X,
        BBox.Min.Y,
        BBox.Min.Z,
        BBox.Max.X,
        BBox.Max.Y,
        BBox.Max.Z
    );
}

//=========================================================================

RVA(0x00243450, 0xc1)
void text_out::AddRadian3(const char* pName, const radian3& Orient) {
    float P = RAD_TO_DEG(Orient.Pitch);
    float Y = RAD_TO_DEG(Orient.Yaw);
    float R = RAD_TO_DEG(Orient.Roll);
    AddField(xfs("%s:fff", pName), P, Y, R);
}

//=========================================================================

RVA(0x00243520, 0x89)
void text_out::AddQuaternion(const char* pName, const quaternion& Q) {
    AddField(xfs("%s:ffff", pName), Q.X, Q.Y, Q.Z, Q.W);
}

//=========================================================================

RVA(0x002435b0, 0x77)
void text_out::AddBool(const char* pName, int Bool) {
    AddField(xfs("%s:d", pName), (Bool) ? (1) : (0));
}

//=========================================================================

RVA(0x00243630, 0x75)
void text_out::AddGuid(const char* pName, unsigned __int64 Guid) {
    AddField(xfs("%s:g", pName), Guid);
}

//=========================================================================

#endif // !( defined( TARGET_PS2 ) && defined( CONFIG_RETAIL ) )
