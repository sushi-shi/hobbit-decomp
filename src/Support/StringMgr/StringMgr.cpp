// Reconstructed PC source; evidence and remaining uncertainties are documented
// in docs/stringmgr-reconstruction.md.

#include <rva.h>

#include <Support/StringMgr/StringMgr.hpp>

#include <xCore/Entropy/e_Virtual.hpp>
#include <xCore/x_files/x_context.hpp>
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_string.hpp>

DATA(0x00413158)
string_mgr g_StringMgr;
RVA_DYNINIT(0x002aa880, 0xa, g_StringMgr)
RVA_DYNINIT(0x002aa890, 0xa, g_StringMgr)
RVA_DYNINIT(0x002aa8a0, 0xc, g_StringMgr)
RVA_DYNINIT(0x002aa8b0, 0xa, g_StringMgr)

DATA(0x00413174)
int string_mgr::m_Initialized = 0;

RVA(0x002aa8c0, 0xd)
string_table::string_table() {
    m_pTableName = 0;
    m_pData = 0;
    m_nStrings = 0;
}

RVA(0x002aa8d0, 0x31)
string_table::~string_table() {
    if (m_pData) {
        vm_Free(m_pData);
        m_pData = 0;
        m_nStrings = 0;
        delete[] m_pTableName;
        m_pTableName = 0;
    }
}

RVA(0x002aa910, 0xbb)
int string_table::Load(const char* pTableName, const char* pFileName) {
    x_mem_owner Owner("string_table::Load");
    int Success = 0;
    X_FILE* pFile = x_fopen(pFileName, "rb");
    if (pFile) {
        int Length = x_flength(pFile);
        m_pData = static_cast<unsigned char*>(vm_Alloc(Length));
        x_fread(m_pData, 1, Length, pFile);
        x_fclose(pFile);
        char* pName = new char[x_strlen(pTableName) + 1];
        x_strcpy(pName, pTableName);
        m_pTableName = pName;
        // PC file storage begins with a 32-bit count followed by 32-bit string offsets.
        m_nStrings = *reinterpret_cast<int*>(m_pData);
        Success = 1;
    }
    return Success;
}

RVA(0x002aa9d0, 0x4)
int string_table::GetCount() const {
    return m_nStrings;
}

RVA(0x002aa9e0, 0x26)
const unsigned short* string_table::GetAt(int Index) const {
    int* pIndex = reinterpret_cast<int*>(m_pData + 4);
    const unsigned short* pString =
        reinterpret_cast<const unsigned short*>(m_pData + 4 + 4 * m_nStrings + pIndex[Index]);
    pString += x_wstrlen(pString);
    pString++;
    return pString;
}

RVA(0x002aaa10, 0x40)
int w2sstricmp(const unsigned short* pStr1, const char* pStr2) {
    int C1, C2;
    do {
        C2 = *(pStr1++);
        if (C2 >= 'A' && C2 <= 'Z') {
            C2 -= ('A' - 'a');
        }
        C1 = *(pStr2++);
        if (C1 >= 'A' && C1 <= 'Z') {
            C1 -= ('A' - 'a');
        }
    } while (C1 && (C1 == C2));
    return C2 - C1;
}

RVA(0x002aaa50, 0x3a)
unsigned short* Ansi2Wide(const char* pSrc) {
    // Capacity follows the adjacent PC storage; the loop has no bounds check.
    DATA(0x00413058)
    static unsigned short WideString[128];
    unsigned int Length = x_strlen(pSrc) + 1;
    unsigned short* pWide = WideString;
    // Each PC wide code unit is written as a character byte and a zero high byte.
    char* pTemp = reinterpret_cast<char*>(pWide);
    for (unsigned int i = 0; i < Length; i++) {
        pTemp[1] = 0;
        pTemp[0] = pSrc[i];
        pTemp += 2;
    }
    return pWide;
}

RVA(0x002aaa90, 0xc8)
const unsigned short* string_table::FindString(const char* lookupString) const {
    unsigned short* pString = 0;
    int* pIndex = reinterpret_cast<int*>(m_pData + 4);
    unsigned char* pBase = reinterpret_cast<unsigned char*>(pIndex + m_nStrings);
    char LOOKUPSTRING[64];
    if (x_strlen(lookupString) <= 64) {
        x_strncpy(LOOKUPSTRING, lookupString, 64);
    }
    unsigned short* pLookupWide = Ansi2Wide(x_strtoupper(LOOKUPSTRING));
    int imax = m_nStrings;
    int imin = 0;
    int bFound = 0;
    while (imax >= imin) {
        int i = (imin + imax) >> 1;
        pString = reinterpret_cast<unsigned short*>(pBase + pIndex[i]);
        if (imax == imin + 1) {
            if (!x_wstrcmp(pString, pLookupWide)) {
                bFound = 1;
            } else {
                break;
            }
        } else {
            int cmpVal = x_wstrcmp(pString, pLookupWide);
            if (cmpVal == 0) {
                bFound = 1;
            } else if (cmpVal > 0) {
                imax = i;
            } else {
                imin = i;
            }
        }
        if (bFound) {
            pString += x_wstrlen(pString);
            pString++;
            return pString;
        }
    }
    return 0;
}

RVA(0x002aab60, 0x73)
const unsigned short* string_table::GetAt(const char* lookupString) const {
    char TARGETTAG[4] = "_PC";
    const unsigned short* pString = 0;
    char LOOKUPSTRING_TAG[64];
    if (x_strlen(TARGETTAG) + x_strlen(lookupString) <= 64) {
        x_strncpy(LOOKUPSTRING_TAG, lookupString, 64);
        x_strncat(LOOKUPSTRING_TAG, TARGETTAG, 7);
    }
    pString = FindString(LOOKUPSTRING_TAG);
    if (pString) {
        return pString;
    } else {
        pString = FindString(lookupString);
        if (pString) {
            return pString;
        }
    }
    return 0;
}

RVA(0x002aabe0, 0xea)
const unsigned short* string_table::GetSubTitleSpeaker(const char* lookupString) const {
    unsigned short* pString = 0;
    int* pIndex = reinterpret_cast<int*>(m_pData + 4);
    char LOOKUPSTRING[64];
    x_strcpy(LOOKUPSTRING, lookupString);
    unsigned short* pLookupWide = Ansi2Wide(x_strtoupper(LOOKUPSTRING));
    int imax = m_nStrings;
    int imin = 0;
    int bFound = 0;
    while (imax >= imin) {
        int i = (imin + imax) / 2;
        pString = reinterpret_cast<unsigned short*>(m_pData + 4 + 4 * m_nStrings + pIndex[i]);
        if (imax == imin + 1) {
            if (!x_wstrcmp(pString, pLookupWide)) {
                bFound = 1;
            } else {
                break;
            }
        }
        int cmpVal = x_wstrcmp(pString, pLookupWide);
        if (cmpVal == 0) {
            bFound = 1;
        } else if (cmpVal > 0) {
            imax = i;
        } else {
            imin = i;
        }
        if (bFound) {
            pString += x_wstrlen(pString);
            pString++;
            pString += x_wstrlen(pString);
            pString++;
            return pString;
        }
    }
    // Descriptive name: original spelling is unknown. PC stores this zero
    // UTF-16 terminator in readonly data; the following alignment is unclaimed.
    DATA(0x002fd324)
    static const unsigned short EmptySpeaker = 0;
    return &EmptySpeaker;
}

// Reconstructed descriptive name: the original PC name is not established.
RVA(0x002aacd0, 0xd3)
int string_table::FindIndex(const char* lookupString) const {
    int* pIndex = reinterpret_cast<int*>(m_pData + 4);
    char* lookupCopy = new char[x_strlen(lookupString) + 1];
    x_strcpy(lookupCopy, lookupString);
    for (int i = 0; i < m_nStrings; i++) {
        const unsigned short* pString =
            reinterpret_cast<const unsigned short*>(m_pData + 4 + 4 * m_nStrings + pIndex[i]);
        if (x_wstricmp(pString, xwstring(lookupCopy)) == 0) {
            pString += x_wstrlen(pString);
            pString++;
            return i;
        }
    }
    return -1;
}

RVA(0x002aadb0, 0x23)
string_mgr::string_mgr() {
    m_Initialized = 1;
}

RVA(0x002aade0, 0x1b)
string_mgr::~string_mgr() {
    m_Initialized = 0;
}

RVA(0x002aae00, 0x22)
int string_mgr::GetStringCount(const char* pTableName) {
    int Count = 0;
    const string_table* pTable = FindTable(pTableName);
    if (pTable) {
        Count = pTable->GetCount();
    }
    return Count;
}

RVA(0x002aae30, 0x1a0)
int string_mgr::LoadTable(const char* pTableName, const char* pFileName) {
    xcontext Context("string_mgr::LoadTable");
    x_mem_owner Owner("string_mgr::LoadTable");
    string_table* pTable = new string_table;
    int Success = pTable->Load(pTableName, pFileName);
    if (Success) {
        x_DebugMsg(7, xfs("********* %s Loaded\n", pFileName));
        m_Tables.Append(pTable);
    } else {
        delete pTable;
    }
    return Success;
}

RVA(0x002aafd0, 0x141)
void string_mgr::UnloadTable(const char* pTableName) {
    int iTable = -1;
    for (int i = 0; i < m_Tables.GetCount(); i++) {
        if (x_strcmp(m_Tables[i]->m_pTableName, pTableName) == 0) {
            iTable = i;
            break;
        }
    }
    if (iTable != -1) {
        x_DebugMsg(7, xfs("********* %s UnLoaded\n", pTableName));
        delete m_Tables[iTable];
        m_Tables.Delete(iTable);
    }
    m_Tables.FreeExtra();
}

RVA(0x002ab120, 0x44)
const string_table* string_mgr::FindTable(const char* pTableName) const {
    for (int i = 0; i < m_Tables.GetCount(); i++) {
        if (x_strcmp(m_Tables[i]->m_pTableName, pTableName) == 0) {
            return m_Tables[i];
        }
    }
    return 0;
}

RVA(0x002ab170, 0x27)
const unsigned short* string_mgr::operator()(const char* pTableName, int Index) const {
    const unsigned short* pString = 0;
    const string_table* pTable = FindTable(pTableName);
    if (pTable) {
        pString = pTable->GetAt(Index);
    }
    return pString;
}

RVA(0x002ab1a0, 0x2c)
const unsigned short*
string_mgr::operator()(const char* pTableName, const char* TitleString) const {
    const unsigned short* pString = 0;
    const string_table* pTable = FindTable(pTableName);
    if (pTable) {
        pString = pTable->GetAt(TitleString);
    }
    if (!pString) {
        pString = Ansi2Wide(TitleString);
    }
    return pString;
}

RVA(0x002ab1d0, 0x2c)
const unsigned short*
string_mgr::GetSubTitleSpeaker(const char* pTableName, const char* SpeakerString) const {
    const unsigned short* pString = 0;
    const string_table* pTable = FindTable(pTableName);
    if (pTable) {
        pString = pTable->GetSubTitleSpeaker(SpeakerString);
    }
    if (!pString) {
        pString = Ansi2Wide(SpeakerString);
    }
    return pString;
}

RVA(0x002ab200, 0x23)
int string_mgr::FindIndex(const char* pTableName, const char* lookupString) const {
    const string_table* pTable = FindTable(pTableName);
    if (pTable) {
        return pTable->FindIndex(lookupString);
    }
    return -1;
}
