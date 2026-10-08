// Imported engine source reference: Area51 pristine431f72b9, Support/Animation/AnimEvent.cpp
// Provisional Hobbit adaptation unless an explicit PC RVA is recorded. See IMPORT-NOTES.md.
//=========================================================================
//
//  ANIMDATA.CPP
//
//=========================================================================

//=========================================================================
// INCLUDES
//=========================================================================

#include <rva.h>
#include <Support/Animation/AnimData.hpp>
#include <xCore/x_files/x_bytestream.hpp>
#include <xCore/x_files/x_memory.hpp>

#if defined(HOBBIT_ANIMATION_LATER_EVENTS)
xbytestream* anim_event::m_pEventByteStream;
xbytestream* anim_event::m_pEventTypeNameStrings;

void SwapEndian(f32& V);
void SwapEndian(u16& V);
void SwapEndian(s16& V);
void SwapEndian(s32& V);
void SwapEndian(u32& V);
void SwapEndian(u64& V);
template<class T> void SwapEndian(T*& V);
void SwapEndian(f32* pF, s32 nFloats);
void SwapEndian(vector3& V);
void SwapEndian(quaternion& V);
void SwapEndian(matrix4& V);

//=========================================================================
//=========================================================================
//=========================================================================
// EVENT_DATA_FORMAT
//=========================================================================
//=========================================================================
//=========================================================================

event_data_format::event_data_format(void) {
    m_Flags = 0;
}

//=========================================================================

s32 event_data_format::CountSetBits(s32 NumBitsBefore, s32 NumBitsPossible) const {
    s32 i;
    s32 Count = 0;
    u32 Mask = 1 << NumBitsBefore;
    for (i = 0; i < NumBitsPossible; ++i) {
        if (m_Flags & Mask) {
            ++Count;
        }

        Mask <<= 1;
    }

    return Count;
}

//=========================================================================

void event_data_format::SetInt(s32 Idx) {
    u32 Mask = 1;
    Mask <<= Idx;
    m_Flags |= Mask;
}

//=========================================================================

void event_data_format::SetFloat(s32 Idx) {
    u32 Mask = 1;
    Mask <<= EVENT_MAX_INTS + Idx;
    m_Flags |= Mask;
}

//=========================================================================

void event_data_format::SetPoint(s32 Idx) {
    u32 Mask = 1;
    Mask <<= EVENT_MAX_INTS + EVENT_MAX_FLOATS + Idx;
    m_Flags |= Mask;
}

//=========================================================================

void event_data_format::SetBool(s32 Idx) {
    u32 Mask = 1;
    Mask <<= EVENT_MAX_INTS + EVENT_MAX_FLOATS + EVENT_MAX_POINTS + Idx;
    m_Flags |= Mask;
}

//=========================================================================

void event_data_format::SetString(s32 Idx) {
    u32 Mask = 1;
    Mask <<= EVENT_MAX_INTS + EVENT_MAX_FLOATS + EVENT_MAX_POINTS + EVENT_MAX_BOOLS + Idx;
    m_Flags |= Mask;
}

//=========================================================================

//=========================================================================
//=========================================================================
//=========================================================================
// EVENT_DATA
//=========================================================================
//=========================================================================
//=========================================================================

event_data::event_data(void) {
    m_Type[0] = '\0';
    m_Name[0] = '\0';
}

//=========================================================================

void event_data::SetType(const char* Type) {
    x_strncpy(m_Type, Type, EVENT_MAX_STRING_LENGTH);
}

//=========================================================================

void event_data::SetName(const char* Name) {
    x_strncpy(m_Name, Name, EVENT_MAX_STRING_LENGTH);
}

//=========================================================================

void event_data::StoreInt(s32 Idx, s32 Value) {
    m_Ints[Idx] = Value;
    m_DataFormat.SetInt(Idx);
}

//=========================================================================

void event_data::StoreFloat(s32 Idx, f32 Value) {
    m_Floats[Idx] = Value;
    m_DataFormat.SetFloat(Idx);
}

//=========================================================================

void event_data::StorePoint(s32 Idx, const vector3& Value) {
    m_Points[Idx] = Value;
    m_DataFormat.SetPoint(Idx);
}

//=========================================================================

void event_data::StoreBool(s32 Idx, xbool Value) {
    m_Bools[Idx] = Value;
    m_DataFormat.SetBool(Idx);
}

//=========================================================================

void event_data::StoreString(s32 Idx, const char* String) {
    if (NULL == String) {
        return;
    }
    x_strncpy(m_Strings[Idx], String, EVENT_MAX_STRING_LENGTH);
    m_DataFormat.SetString(Idx);
}

//=========================================================================

void event_data::SwitchEndian(void) {
    s32 i;

    SwapEndian(m_DataFormat.m_Flags);

    for (i = 0; i < EVENT_MAX_INTS; ++i) {
        SwapEndian(m_Ints[i]);
    }

    for (i = 0; i < EVENT_MAX_FLOATS; ++i) {
        SwapEndian(m_Floats[i]);
    }

    for (i = 0; i < EVENT_MAX_POINTS; ++i) {
        SwapEndian(m_Points[i]);
    }

    for (i = 0; i < EVENT_MAX_BOOLS; ++i) {
        SwapEndian(m_Bools[i]);
    }
}

//=========================================================================
//=========================================================================
//=========================================================================
// ANIM_EVENT
//=========================================================================
//=========================================================================
//=========================================================================

s32 PaddedStringLength(const char* String);
s32 StringPadding(const char* String);

anim_event::anim_event() {
    m_ByteStreamDataOffset = 0;
    m_TypeOffset = 0;
    m_NameOffset = 0;
}

//=========================================================================
anim_event::~anim_event() {}

//=========================================================================
void anim_event::Init(void) {
    if (NULL == m_pEventByteStream) {
        m_pEventByteStream = new xbytestream;
    }
    if (NULL == m_pEventTypeNameStrings) {
        m_pEventTypeNameStrings = new xbytestream;
    }

    ASSERT(m_pEventByteStream);
    ASSERT(m_pEventTypeNameStrings);
}

//=========================================================================
void anim_event::SetData(const event_data& Data) {
    s32 i;
    u32 Mask = 1;
    m_DataFormat.m_Flags = 0;

    if (Data.GetType()) {
        m_TypeOffset = SaveTypeNameString(Data.GetType());
    }

    if (Data.GetName()) {
        m_NameOffset = SaveTypeNameString(Data.GetName());
    }

    //
    // Set flags
    //
    for (i = 0; i < EVENT_MAX_INTS; ++i) {
        if (i < Data.nInts()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }
    for (i = 0; i < EVENT_MAX_FLOATS; ++i) {
        if (i < Data.nFloats()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }
    for (i = 0; i < EVENT_MAX_POINTS; ++i) {
        if (i < Data.nPoints()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }
    for (i = 0; i < EVENT_MAX_BOOLS; ++i) {
        if (i < Data.nBools()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }
    for (i = 0; i < EVENT_MAX_STRINGS; ++i) {
        if (i < Data.nStrings()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }

    //
    // Store all the data
    //
    m_ByteStreamDataOffset = m_pEventByteStream->GetLength();

    m_pEventByteStream->Append((byte*)(Data.Ints()), sizeof(s32) * Data.nInts());
    m_pEventByteStream->Append((byte*)(Data.Floats()), sizeof(f32) * Data.nFloats());
    m_pEventByteStream->Append((byte*)(Data.Points()), sizeof(vector3) * Data.nPoints());
    m_pEventByteStream->Append((byte*)(Data.Bools()), sizeof(xbool) * Data.nBools());

    for (i = 0; i < Data.nStrings(); ++i) {
        const s32 Length = x_strlen(Data.String(i));
        m_pEventByteStream->Append((byte*)(Data.String(i)), sizeof(char) * Length);
        m_pEventByteStream->Append('\0');

        // dword align...
        m_pEventByteStream->Append((byte*)"@@@@", StringPadding(Data.String(i)));
    }

#ifdef X_ASSERT
    //
    // Self test
    //

    // Check DataFlags
    ASSERT(GetNInts() == Data.nInts());
    ASSERT(GetNFloats() == Data.nFloats());
    ASSERT(GetNPoints() == Data.nPoints());
    ASSERT(GetNBools() == Data.nBools());
    ASSERT(GetNStrings() == Data.nStrings());

    // Check Type and Name
    if (Data.GetType()) {
        ASSERT(!x_strcmp(GetType(), Data.GetType()));
    }

    if (Data.GetName()) {
        ASSERT(!x_strcmp(GetName(), Data.GetName()));
    }

    // Check for data...
    for (i = 0; i < Data.nInts(); ++i) {
        ASSERT(GetInt(i) == (Data.Ints())[i]);
    }

    for (i = 0; i < Data.nFloats(); ++i) {
        ASSERT(GetFloat(i) == (Data.Floats())[i]);
    }

    for (i = 0; i < Data.nPoints(); ++i) {
        const vector3 Point1 = GetPoint(i);
        const vector3 Point2 = (Data.Points())[i];
        ASSERT((Point1 - Point2).LengthSquared() < 0.0001f);
    }

    for (i = 0; i < Data.nBools(); ++i) {
        ASSERT(GetBool(i) == (Data.Bools())[i]);
    }

    for (i = 0; i < Data.nStrings(); ++i) {
        ASSERT(!x_strcmp(GetString(i), Data.String(i)));
    }

    // Now just check that GetData() works
    event_data NewData = GetData();
    ASSERT(x_strlen(Data.GetType()) <= EVENT_MAX_STRING_LENGTH);
    ASSERT(x_strlen(Data.GetName()) <= EVENT_MAX_STRING_LENGTH);
    ASSERT(x_strlen(NewData.GetType()) <= EVENT_MAX_STRING_LENGTH);
    ASSERT(x_strlen(NewData.GetName()) <= EVENT_MAX_STRING_LENGTH);
    ASSERT(!x_strcmp(NewData.GetType(), Data.GetType()));
    ASSERT(!x_strcmp(NewData.GetName(), Data.GetName()));

    ASSERT(NewData.nInts() == Data.nInts());
    for (i = 0; i < NewData.nInts(); ++i) {
        ASSERT((NewData.Ints())[i] == (Data.Ints())[i]);
    }

    ASSERT(NewData.nFloats() == Data.nFloats());
    for (i = 0; i < NewData.nFloats(); ++i) {
        ASSERT((NewData.Floats())[i] == (Data.Floats())[i]);
    }

    ASSERT(NewData.nPoints() == Data.nPoints());
    for (i = 0; i < NewData.nPoints(); ++i) {
        ASSERT((NewData.Points())[i] == (Data.Points())[i]);
    }

    ASSERT(NewData.nBools() == Data.nBools());
    for (i = 0; i < NewData.nBools(); ++i) {
        ASSERT((NewData.Bools())[i] == (Data.Bools())[i]);
    }

    ASSERT(NewData.nStrings() == Data.nStrings());
    for (i = 0; i < NewData.nStrings(); ++i) {
        ASSERT(!x_strcmp(NewData.String(i), Data.String(i)));
    }
#endif // #ifdef X_ASSERT
}

//=========================================================================

event_data anim_event::GetData(void) const {
    event_data Data;
    s32 i;

    Data.SetType(GetType());
    Data.SetName(GetName());

    for (i = 0; i < GetNInts(); ++i) {
        Data.StoreInt(i, GetInt(i));
    }

    for (i = 0; i < GetNFloats(); ++i) {
        Data.StoreFloat(i, GetFloat(i));
    }

    for (i = 0; i < GetNPoints(); ++i) {
        Data.StorePoint(i, GetPoint(i));
    }

    for (i = 0; i < GetNBools(); ++i) {
        Data.StoreBool(i, GetBool(i));
    }

    for (i = 0; i < GetNStrings(); ++i) {
        Data.StoreString(i, GetString(i));
    }

    return Data;
}

//=========================================================================

const char* anim_event::GetType(void) const {
    return GetTypeNameString(m_TypeOffset);
}

//=========================================================================

const char* anim_event::GetName(void) const {
    return GetTypeNameString(m_NameOffset);
}

//=========================================================================

inline const byte* anim_event::GetDataBuffer(void) const {
    return (byte*)(m_pEventByteStream->GetBuffer() + m_ByteStreamDataOffset);
}

//=========================================================================

inline s32 event_data_format::GetNInts(void) const {
    return CountSetBits(0, EVENT_MAX_INTS);
}

//=========================================================================

inline s32 event_data_format::GetNFloats(void) const {
    return CountSetBits(EVENT_MAX_INTS, EVENT_MAX_FLOATS);
}

//=========================================================================

inline s32 event_data_format::GetNPoints(void) const {
    return CountSetBits((EVENT_MAX_INTS + EVENT_MAX_FLOATS), EVENT_MAX_POINTS);
}

//=========================================================================

inline s32 event_data_format::GetNBools(void) const {
    return CountSetBits((EVENT_MAX_INTS + EVENT_MAX_FLOATS + EVENT_MAX_POINTS), EVENT_MAX_BOOLS);
}

//=========================================================================

inline s32 event_data_format::GetNStrings(void) const {
    return CountSetBits(
        (EVENT_MAX_INTS + EVENT_MAX_FLOATS + EVENT_MAX_POINTS + EVENT_MAX_BOOLS),
        EVENT_MAX_STRINGS
    );
}

//=========================================================================

s32 anim_event::GetInt(s32 Idx) const {
    const s32 Offset = Idx * sizeof(s32);

    return *(s32*)(GetDataBuffer() + Offset);
}

//=========================================================================

f32 anim_event::GetFloat(s32 Idx) const {
    const s32 Offset = (GetNInts() * sizeof(s32)) + (Idx * sizeof(f32));

    return *(f32*)(GetDataBuffer() + Offset);
}

//=========================================================================

vector3 anim_event::GetPoint(s32 Idx) const {
    const s32 Offset =
        (GetNInts() * sizeof(s32)) + (GetNFloats() * sizeof(f32)) + (Idx * sizeof(vector3));

    vector3p* pVec = (vector3p*)(GetDataBuffer() + Offset);
    return vector3(*pVec);
}

//=========================================================================

xbool anim_event::GetBool(s32 Idx) const {
    const s32 Offset = (GetNInts() * sizeof(s32)) + (GetNFloats() * sizeof(f32))
                       + (GetNPoints() * sizeof(vector3)) + (Idx * sizeof(xbool));

    return *(xbool*)(GetDataBuffer() + Offset);
}

//=========================================================================
const char* anim_event::GetString(s32 Idx) const {
    // First, find the offset to the first string
    const s32 Offset = (GetNInts() * sizeof(s32)) + (GetNFloats() * sizeof(f32))
                       + (GetNPoints() * sizeof(vector3)) + (GetNBools() * sizeof(xbool));

    char* pString = (char*)(GetDataBuffer() + Offset);

    // Count up to the correct string
    s32 i;
    for (i = 0; i < Idx; ++i) {
        //Get to the end of the string
        while (*pString != '\0') {
            ++pString;
        }
        ++pString;

        // Get past the padding
        while (*pString == '@') {
            ++pString;
        }
    }

    return pString;
}

//=========================================================================

const char* anim_event::GetTypeNameString(s32 Offset) {
    ASSERT(Offset < m_pEventTypeNameStrings->GetLength());
    return ((char*)(m_pEventTypeNameStrings->GetBuffer() + Offset));
}

//=========================================================================

s32 anim_event::SaveTypeNameString(const char* String) {
    ASSERT(String);
    if (NULL == String) {
        return NULL;
    }

    // First, see if we have the string saved yet
    char* CurString = (char*)m_pEventTypeNameStrings->GetBuffer();
    s32 Offset = 0;

    while (Offset < m_pEventTypeNameStrings->GetLength()) {
        ASSERT(CurString);
        if (NULL == CurString) {
            // problem -- m_EventStrings is corrupted
            return -1;
        }

        if (x_strcmp(CurString, String) == 0) {
            // found it
            return Offset;
        }

        const s32 PaddedLength = PaddedStringLength(CurString);
        Offset += PaddedLength;
        CurString += PaddedLength * sizeof(char);
    }

    // We didn't find it, add it
    Offset = m_pEventTypeNameStrings->GetLength();
    const s32 Length = x_strlen(String);
    m_pEventTypeNameStrings->Append((byte*)String, Length);
    m_pEventTypeNameStrings->Append((byte)'\0');

    // dword align...
    m_pEventTypeNameStrings->Append((byte*)"@@@@", StringPadding(String));

    return Offset;
}

//=========================================================================

void anim_event::ResetByteStreams(void) {
    if (m_pEventByteStream) {
        delete m_pEventByteStream;
    }
    if (m_pEventTypeNameStrings) {
        delete m_pEventTypeNameStrings;
    }
    m_pEventByteStream = NULL;
    m_pEventTypeNameStrings = NULL;
}

//=========================================================================
// returns strlen() + null terminator + padding ===> which is total mem used
s32 PaddedStringLength(const char* String) {
    ASSERT(String);
    return x_strlen(String) + 1 + StringPadding(String);
}

//=========================================================================

s32 StringPadding(const char* String) {
    return (4 - ((x_strlen(String) + 1) % 4));
}
#else
// PC event revision: complete real24B object; two instance xbytestreams.
// Xbox map identifies original AnimEvent.obj global s_nEvents; PC367098 count.
DATA(0x00367098)
s32 s_nEvents = 0;

void SwapEndian(f32& V);
void SwapEndian(u16& V);
void SwapEndian(s16& V);
void SwapEndian(s32& V);
void SwapEndian(u32& V);
void SwapEndian(u64& V);
template<class T> void SwapEndian(T*& V);
void SwapEndian(f32* pF, s32 nFloats);
void SwapEndian(vector3& V);
void SwapEndian(quaternion& V);
void SwapEndian(matrix4& V);

//=========================================================================
//=========================================================================
//=========================================================================
// EVENT_DATA_FORMAT
//=========================================================================
//=========================================================================
//=========================================================================

RVA(0x13eac0, 0x9)
event_data_format::event_data_format(void) {
    m_Flags = 0;
}

//=========================================================================

RVA(0x13ead0, 0x2e)
s32 event_data_format::CountSetBits(s32 NumBitsBefore, s32 NumBitsPossible) const {
    s32 i;
    s32 Count = 0;
    u32 Mask = 1 << NumBitsBefore;
    for (i = 0; i < NumBitsPossible; ++i) {
        if (m_Flags & Mask) {
            ++Count;
        }

        Mask <<= 1;
    }

    return Count;
}

//=========================================================================

RVA(0x13eb00, 0x12)
void event_data_format::SetInt(s32 Idx) {
    u32 Mask = 1;
    Mask <<= Idx;
    m_Flags |= Mask;
}

//=========================================================================

RVA(0x13eb20, 0x15)
void event_data_format::SetFloat(s32 Idx) {
    u32 Mask = 1;
    Mask <<= EVENT_MAX_INTS + Idx;
    m_Flags |= Mask;
}

//=========================================================================

RVA(0x13eb40, 0x15)
void event_data_format::SetPoint(s32 Idx) {
    u32 Mask = 1;
    Mask <<= EVENT_MAX_INTS + EVENT_MAX_FLOATS + Idx;
    m_Flags |= Mask;
}

//=========================================================================

RVA(0x13eb60, 0x15)
void event_data_format::SetBool(s32 Idx) {
    u32 Mask = 1;
    Mask <<= EVENT_MAX_INTS + EVENT_MAX_FLOATS + EVENT_MAX_POINTS + Idx;
    m_Flags |= Mask;
}

//=========================================================================

RVA(0x13eb80, 0x15)
void event_data_format::SetString(s32 Idx) {
    u32 Mask = 1;
    Mask <<= EVENT_MAX_INTS + EVENT_MAX_FLOATS + EVENT_MAX_POINTS + EVENT_MAX_BOOLS + Idx;
    m_Flags |= Mask;
}

//=========================================================================

//=========================================================================
//=========================================================================
//=========================================================================
// EVENT_DATA
//=========================================================================
//=========================================================================
//=========================================================================

RVA(0x13eba0, 0x10)
event_data::event_data(void) {
    m_Type[0] = '\0';
    m_Name[0] = '\0';
}

//=========================================================================

RVA(0x13ebb0, 0x16)
void event_data::SetType(const char* Type) {
    x_strncpy(m_Type, Type, EVENT_MAX_STRING_LENGTH);
}

//=========================================================================

RVA(0x13ebd0, 0x16)
void event_data::SetName(const char* Name) {
    x_strncpy(m_Name, Name, EVENT_MAX_STRING_LENGTH);
}

//=========================================================================

RVA(0x13ebf0, 0x15)
void event_data::StoreInt(s32 Idx, s32 Value) {
    m_Ints[Idx] = Value;
    m_DataFormat.SetInt(Idx);
}

//=========================================================================

RVA(0x13ec10, 0x15)
void event_data::StoreFloat(s32 Idx, f32 Value) {
    m_Floats[Idx] = Value;
    m_DataFormat.SetFloat(Idx);
}

//=========================================================================

RVA(0x13ec30, 0x2c)
void event_data::StorePoint(s32 Idx, const vector3& Value) {
    m_Points[Idx] = Value;
    m_DataFormat.SetPoint(Idx);
}

//=========================================================================

RVA(0x13ec60, 0x18)
void event_data::StoreBool(s32 Idx, xbool Value) {
    m_Bools[Idx] = Value;
    m_DataFormat.SetBool(Idx);
}

//=========================================================================

RVA(0x13ec80, 0x35)
void event_data::StoreString(s32 Idx, const char* String) {
    if (NULL == String) {
        return;
    }
    x_strncpy(m_Strings[Idx], String, EVENT_MAX_STRING_LENGTH);
    m_DataFormat.SetString(Idx);
}

//=========================================================================

RVA(0x13ecc0, 0x74)
void event_data::SwitchEndian(void) {
    s32 i;

    SwapEndian(m_DataFormat.m_Flags);

    for (i = 0; i < EVENT_MAX_INTS; ++i) {
        SwapEndian(m_Ints[i]);
    }

    for (i = 0; i < EVENT_MAX_FLOATS; ++i) {
        SwapEndian(m_Floats[i]);
    }

    for (i = 0; i < EVENT_MAX_POINTS; ++i) {
        SwapEndian(m_Points[i]);
    }

    for (i = 0; i < EVENT_MAX_BOOLS; ++i) {
        SwapEndian(m_Bools[i]);
    }
}

//=========================================================================
//=========================================================================
//=========================================================================
// ANIM_EVENT
//=========================================================================
//=========================================================================
//=========================================================================

s32 PaddedStringLength(const char* String);
s32 StringPadding(const char* String);

RVA(0x13ed40, 0x6e)
anim_event::anim_event() : m_ByteStreamDataOffset(0), m_TypeOffset(0), m_NameOffset(0) {
    s_nEvents++;
}

//=========================================================================
RVA(0x13edb0, 0x58)
anim_event::~anim_event() {
    s_nEvents--;
}

//=========================================================================

//=========================================================================
RVA(0x13ee10, 0x1bb)
void anim_event::SetData(const event_data& Data) {
    x_mem_owner Owner("anim_event::SetData");
    s32 i;
    u32 Mask = 1;
    m_DataFormat.m_Flags = 0;

    if (Data.GetType()) {
        m_TypeOffset = SaveTypeNameString(Data.GetType());
    }

    if (Data.GetName()) {
        m_NameOffset = SaveTypeNameString(Data.GetName());
    }

    //
    // Set flags
    //
    for (i = 0; i < EVENT_MAX_INTS; ++i) {
        if (i < Data.nInts()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }
    for (i = 0; i < EVENT_MAX_FLOATS; ++i) {
        if (i < Data.nFloats()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }
    for (i = 0; i < EVENT_MAX_POINTS; ++i) {
        if (i < Data.nPoints()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }
    for (i = 0; i < EVENT_MAX_BOOLS; ++i) {
        if (i < Data.nBools()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }
    for (i = 0; i < EVENT_MAX_STRINGS; ++i) {
        if (i < Data.nStrings()) {
            m_DataFormat.m_Flags |= Mask;
        }

        Mask <<= 1;
    }

    //
    // Store all the data
    //
    m_ByteStreamDataOffset = m_EventByteStream.GetLength();

    m_EventByteStream.Append((byte*)(Data.Ints()), sizeof(s32) * Data.nInts());
    m_EventByteStream.Append((byte*)(Data.Floats()), sizeof(f32) * Data.nFloats());
    m_EventByteStream.Append((byte*)(Data.Points()), sizeof(vector3) * Data.nPoints());
    m_EventByteStream.Append((byte*)(Data.Bools()), sizeof(xbool) * Data.nBools());

    for (i = 0; i < Data.nStrings(); ++i) {
        s32 Offset = SaveTypeNameString(Data.String(i));
        m_EventByteStream.Append(&Offset, sizeof(s32));
    }

#ifdef X_ASSERT
    //
    // Self test
    //

    // Check DataFlags
    ASSERT(GetNInts() == Data.nInts());
    ASSERT(GetNFloats() == Data.nFloats());
    ASSERT(GetNPoints() == Data.nPoints());
    ASSERT(GetNBools() == Data.nBools());
    ASSERT(GetNStrings() == Data.nStrings());

    // Check Type and Name
    if (Data.GetType()) {
        ASSERT(!x_strcmp(GetType(), Data.GetType()));
    }

    if (Data.GetName()) {
        ASSERT(!x_strcmp(GetName(), Data.GetName()));
    }

    // Check for data...
    for (i = 0; i < Data.nInts(); ++i) {
        ASSERT(GetInt(i) == (Data.Ints())[i]);
    }

    for (i = 0; i < Data.nFloats(); ++i) {
        ASSERT(GetFloat(i) == (Data.Floats())[i]);
    }

    for (i = 0; i < Data.nPoints(); ++i) {
        const vector3 Point1 = GetPoint(i);
        const vector3 Point2 = (Data.Points())[i];
        ASSERT((Point1 - Point2).LengthSquared() < 0.0001f);
    }

    for (i = 0; i < Data.nBools(); ++i) {
        ASSERT(GetBool(i) == (Data.Bools())[i]);
    }

    for (i = 0; i < Data.nStrings(); ++i) {
        ASSERT(!x_strcmp(GetString(i), Data.String(i)));
    }

    // Now just check that GetData() works
    event_data NewData = GetData();
    ASSERT(x_strlen(Data.GetType()) <= EVENT_MAX_STRING_LENGTH);
    ASSERT(x_strlen(Data.GetName()) <= EVENT_MAX_STRING_LENGTH);
    ASSERT(x_strlen(NewData.GetType()) <= EVENT_MAX_STRING_LENGTH);
    ASSERT(x_strlen(NewData.GetName()) <= EVENT_MAX_STRING_LENGTH);
    ASSERT(!x_strcmp(NewData.GetType(), Data.GetType()));
    ASSERT(!x_strcmp(NewData.GetName(), Data.GetName()));

    ASSERT(NewData.nInts() == Data.nInts());
    for (i = 0; i < NewData.nInts(); ++i) {
        ASSERT((NewData.Ints())[i] == (Data.Ints())[i]);
    }

    ASSERT(NewData.nFloats() == Data.nFloats());
    for (i = 0; i < NewData.nFloats(); ++i) {
        ASSERT((NewData.Floats())[i] == (Data.Floats())[i]);
    }

    ASSERT(NewData.nPoints() == Data.nPoints());
    for (i = 0; i < NewData.nPoints(); ++i) {
        ASSERT((NewData.Points())[i] == (Data.Points())[i]);
    }

    ASSERT(NewData.nBools() == Data.nBools());
    for (i = 0; i < NewData.nBools(); ++i) {
        ASSERT((NewData.Bools())[i] == (Data.Bools())[i]);
    }

    ASSERT(NewData.nStrings() == Data.nStrings());
    for (i = 0; i < NewData.nStrings(); ++i) {
        ASSERT(!x_strcmp(NewData.String(i), Data.String(i)));
    }
#endif // #ifdef X_ASSERT
}

//=========================================================================

RVA(0x13efd0, 0x1c4)
event_data anim_event::GetData(void) const {
    event_data Data;
    s32 i;

    Data.SetType(GetType());
    Data.SetName(GetName());

    for (i = 0; i < GetNInts(); ++i) {
        Data.StoreInt(i, GetInt(i));
    }

    for (i = 0; i < GetNFloats(); ++i) {
        Data.StoreFloat(i, GetFloat(i));
    }

    for (i = 0; i < GetNPoints(); ++i) {
        Data.StorePoint(i, GetPoint(i));
    }

    for (i = 0; i < GetNBools(); ++i) {
        Data.StoreBool(i, GetBool(i));
    }

    for (i = 0; i < GetNStrings(); ++i) {
        Data.StoreString(i, GetString(i));
    }

    return Data;
}

//=========================================================================

RVA(0x13f1a0, 0xa)
const char* anim_event::GetType(void) const {
    return GetTypeNameString(m_TypeOffset);
}

//=========================================================================

RVA(0x13f1b0, 0xa)
const char* anim_event::GetName(void) const {
    return GetTypeNameString(m_NameOffset);
}

//=========================================================================

inline const byte* anim_event::GetDataBuffer(void) const {
    return (byte*)(m_EventByteStream.GetBuffer() + m_ByteStreamDataOffset);
}

//=========================================================================

RVA(0x13f1c0, 0xa)
s32 event_data_format::GetNInts(void) const {
    return CountSetBits(0, EVENT_MAX_INTS);
}

//=========================================================================

RVA(0x13f1d0, 0xa)
s32 event_data_format::GetNFloats(void) const {
    return CountSetBits(EVENT_MAX_INTS, EVENT_MAX_FLOATS);
}

//=========================================================================

RVA(0x13f1e0, 0xa)
s32 event_data_format::GetNPoints(void) const {
    return CountSetBits((EVENT_MAX_INTS + EVENT_MAX_FLOATS), EVENT_MAX_POINTS);
}

//=========================================================================

RVA(0x13f1f0, 0xa)
s32 event_data_format::GetNBools(void) const {
    return CountSetBits((EVENT_MAX_INTS + EVENT_MAX_FLOATS + EVENT_MAX_POINTS), EVENT_MAX_BOOLS);
}

//=========================================================================

RVA(0x13f200, 0xa)
s32 event_data_format::GetNStrings(void) const {
    return CountSetBits(
        (EVENT_MAX_INTS + EVENT_MAX_FLOATS + EVENT_MAX_POINTS + EVENT_MAX_BOOLS),
        EVENT_MAX_STRINGS
    );
}

//=========================================================================

RVA(0x13f210, 0x1b)
s32 anim_event::GetInt(s32 Idx) const {
    const s32 Offset = Idx * sizeof(s32);

    return *(s32*)(GetDataBuffer() + Offset);
}

//=========================================================================

RVA(0x13f230, 0x27)
f32 anim_event::GetFloat(s32 Idx) const {
    const s32 Offset = (GetNInts() * sizeof(s32)) + (Idx * sizeof(f32));

    return *(f32*)(GetDataBuffer() + Offset);
}

//=========================================================================

RVA(0x13f260, 0x51)
vector3 anim_event::GetPoint(s32 Idx) const {
    const s32 Offset =
        (GetNInts() * sizeof(s32)) + (GetNFloats() * sizeof(f32)) + (Idx * sizeof(vector3));

    vector3* pVec = (vector3*)(GetDataBuffer() + Offset);
    return *pVec;
}

//=========================================================================

RVA(0x13f2c0, 0x48)
xbool anim_event::GetBool(s32 Idx) const {
    const s32 Offset = (GetNInts() * sizeof(s32)) + (GetNFloats() * sizeof(f32))
                       + (GetNPoints() * sizeof(vector3)) + (Idx * sizeof(xbool));

    return *(xbool*)(GetDataBuffer() + Offset);
}

//=========================================================================
RVA(0x13f310, 0x57)
const char* anim_event::GetString(s32 Idx) const {
    const s32 Offset = (GetNInts() * sizeof(s32)) + (GetNFloats() * sizeof(f32))
                       + (GetNPoints() * sizeof(vector3)) + (GetNBools() * sizeof(xbool))
                       + (Idx * sizeof(s32));
    return GetTypeNameString(*(s32*)(GetDataBuffer() + Offset));
}

//=========================================================================

RVA(0x13f370, 0xf)
const char* anim_event::GetTypeNameString(s32 Offset) const {
    ASSERT(Offset < m_EventTypeNameStrings.GetLength());
    return ((char*)(m_EventTypeNameStrings.GetBuffer() + Offset));
}

//=========================================================================

RVA(0x13f380, 0x126)
s32 anim_event::SaveTypeNameString(const char* String) {
    x_mem_owner Owner("anim_event::SaveTypeNameString");
    if (NULL == String) {
        return NULL;
    }

    // First, see if we have the string saved yet
    char* CurString = (char*)m_EventTypeNameStrings.GetBuffer();
    s32 Offset = 0;

    while (Offset < m_EventTypeNameStrings.GetLength()) {
        if (NULL == CurString) {
            // problem -- m_EventStrings is corrupted
            return -1;
        }

        if (x_strcmp(CurString, String) == 0) {
            // found it
            return Offset;
        }

        const s32 PaddedLength = PaddedStringLength(CurString);
        Offset += PaddedLength;
        CurString += PaddedLength * sizeof(char);
    }

    // We didn't find it, add it
    Offset = m_EventTypeNameStrings.GetLength();
    const s32 Length = x_strlen(String);
    m_EventTypeNameStrings.Append((byte*)String, Length);
    m_EventTypeNameStrings.Append((byte)'\0');

    // dword align...
    m_EventTypeNameStrings.Append((byte*)"@@@@", StringPadding(String));

    return Offset;
}

//=========================================================================

//=========================================================================
// returns strlen() + null terminator + padding ===> which is total mem used
RVA(0x13f4b0, 0x1e)
s32 PaddedStringLength(const char* String) {
    ASSERT(String);
    return x_strlen(String) + 1 + StringPadding(String);
}

//=========================================================================

RVA(0x13f4d0, 0x24)
s32 StringPadding(const char* String) {
    return (4 - ((x_strlen(String) + 1) % 4));
}
#endif
