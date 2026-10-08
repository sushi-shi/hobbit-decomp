// Reconstructed from pinned PC bytes and Parsing:BinOut.obj identities.
// Serialized record names and unresolved fields are documented in BinFormat.hpp.

#include <rva.h>

#include <xCore/Auxiliary/Parsing/BinOut.hpp>

#include <xCore/x_files/x_plus.hpp>

#include <stdarg.h>

// Naturally emitted special members and template instances; shared x_array.hpp
// supplies their bodies. These anchors do not force an instantiation.
RVA_COMPGEN(0x00007f70, 0x54, ??_Exstring@@QAEPAXI@Z)
RVA_COMPGEN(0x000375f0, 0x2d, ??1?$xarray@Vxstring@@@@QAE@XZ)
RVA_COMPGEN(0x001c8440, 0x11, ??1?$xarray@Tfield_types@@@@QAE@XZ)

RVA(0x00240630, 0x34)
bin_out::bin_out() {
    LittleEndianFile = 0;
    BigEndianFile = 0;
}
RVA(0x00240670, 0x9d)
bin_out::~bin_out() {
    CloseFile();
}
RVA(0x00240710, 0x13a)
void bin_out::CloseFile() {
    if (LittleEndianFile) {
        WriteFile(0, LittleEndianFile);
        x_fclose(LittleEndianFile);
        LittleEndianFile = 0;
    }
    if (BigEndianFile) {
        WriteFile(1, BigEndianFile);
        x_fclose(BigEndianFile);
        BigEndianFile = 0;
    }
    for (int i = 0; i < Blocks.GetCount(); i++) {
        for (int j = 0; j < Blocks[i].Rows.GetCount(); j++) {
            for (int k = 0; k < Blocks[i].Rows[j].Fields.GetCount(); k++) {
                delete[] Blocks[i].Rows[j].Fields[k].Data;
            }
        }
    }
    Names.Clear();
    Blocks.Clear();
}
RVA(0x00240850, 0x88)
int bin_out::OpenFile(const char* LittleName, const char* BigName) {
    CloseFile();
    if (LittleName) {
        LittleEndianFile = x_fopen(LittleName, "wb");
    }
    if (BigName) {
        BigEndianFile = x_fopen(BigName, "wb");
    }
    if ((LittleName && !LittleEndianFile) || (BigName && !BigEndianFile)) {
        x_fclose(LittleEndianFile);
        x_fclose(BigEndianFile);
        LittleEndianFile = 0;
        BigEndianFile = 0;
        return 0;
    }
    return 1;
}
RVA(0x002408e0, 0x2b0)
void bin_out::AddHeader(const char* pName, int Count) {
    i_block& Block = Blocks.Append();
    int i;
    for (i = 0; i < Names.GetCount(); i++) {
        if (x_stricmp(pName, Names[i]) == 0) {
            break;
        }
    }
    Block.Header.NameIndex = i;
    if (i == Names.GetCount()) {
        Names.Append(pName);
    }
    Block.Header.Count = Count;
    Block.Rows.Append();
}
RVA(0x00240b90, 0x6a7)
void bin_out::AddField(const char* pName, ...) {
    int iBlock = Blocks.GetCount() - 1;
    int iRow = Blocks[iBlock].Rows.GetCount() - 1;
    i_field& Field = Blocks[iBlock].Rows[iRow].Fields.Append();
    int i;
    for (i = 0; i < Names.GetCount(); i++) {
        if (x_stricmp(pName, Names[i]) == 0) {
            break;
        }
    }
    Field.NameIndex = i;
    if (i == Names.GetCount()) {
        Names.Append(pName);
    }
    // Preserve the PC no-effect duplicate-field scan; no missing assertion is invented.
    for (i = 0; i < Blocks[iBlock].Rows[iRow].Fields.GetCount() - 1; i++) {
    }
    Field.ByteSize = 0;
    const char* Types = x_strchr(pName, ':') + 1;
    xarray<field_types> Values;
    va_list Args;
    va_start(Args, pName);
    for (int j = 0; Types[j]; j++) {
        switch (Types[j]) {
            case 'F':
            case 'f':
                Values.Append().F = va_arg(Args, double);
                Field.ByteSize += 4;
                break;
            case 'D':
            case 'd':
                Values.Append().I = va_arg(Args, int);
                Field.ByteSize += 4;
                break;
            case 'S':
            case 's':
                Values.Append().S = va_arg(Args, const char*);
                Field.ByteSize += (x_strlen(Values[Values.GetCount() - 1].S) + 36) & ~31;
                break;
            case 'G':
            case 'g':
                Values.Append().G = va_arg(Args, unsigned __int64);
                Field.ByteSize += 8;
                break;
        }
    }
    va_end(Args);
    Field.Data = new unsigned char[Field.ByteSize];
    unsigned char* pData = Field.Data;
    for (i = 0; Types[i]; i++) {
        switch (Types[i]) {
            case 'F':
            case 'f':
                // Proven 4-byte wire scalar overlay.
                *reinterpret_cast<float*>(pData) = Values[i].F;
                pData += 4;
                break;
            case 'D':
            case 'd':
                *reinterpret_cast<int*>(pData) = Values[i].I; // Proven 4-byte wire scalar overlay.
                pData += 4;
                break;
            case 'G':
            case 'g':
                // Proven GUID wire overlay stores the high 32-bit word before the low word.
                *reinterpret_cast<unsigned*>(pData) = static_cast<unsigned>(Values[i].G >> 32);
                pData += 4;
                *reinterpret_cast<unsigned*>(pData) = static_cast<unsigned>(Values[i].G);
                pData += 4;
                break;
            case 'S':
            case 's': {
                int Len = (x_strlen(Values[i].S) + 36) & ~31;
                x_memcpy(pData, Values[i].S, x_strlen(Values[i].S) + 1);
                pData += Len;
            } break;
        }
    }
}

RVA(0x002412c0, 0x12)
void bin_out::AddEndLine() {
    Blocks[Blocks.GetCount() - 1].Rows.Append();
}

RVA(0x002412e0, 0x82)
void bin_out::AddVector3(const char* pName, const vector3& V) {
    AddField(xfs("%s:fff", pName), V.X, V.Y, V.Z);
}

//=========================================================================

RVA(0x00241370, 0x87)
void bin_out::AddColor(const char* pName, xcolor C) {
    AddField(xfs("%s:dddd", pName), C.R, C.G, C.B, C.A);
}

//=========================================================================

RVA(0x00241400, 0x72)
void bin_out::AddF32(const char* pName, float F) {
    AddField(xfs("%s:f", pName), F);
}

//=========================================================================

RVA(0x00241480, 0x70)
void bin_out::AddS32(const char* pName, int I) {
    AddField(xfs("%s:d", pName), I);
}

//=========================================================================

RVA(0x002414f0, 0x70)
void bin_out::AddString(const char* pName, const char* pStr) {
    AddField(xfs("%s:s", pName), pStr);
}

//=========================================================================

RVA(0x00241560, 0x9e)
void bin_out::AddBBox(const char* pName, const bbox& BBox) {
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

RVA(0x00241600, 0xc1)
void bin_out::AddRadian3(const char* pName, const radian3& Orient) {
    float P = (static_cast<float>(((Orient.Pitch) * 180.0f) / 3.1415926535897932384626433832795f));
    float Y = (static_cast<float>(((Orient.Yaw) * 180.0f) / 3.1415926535897932384626433832795f));
    float R = (static_cast<float>(((Orient.Roll) * 180.0f) / 3.1415926535897932384626433832795f));
    AddField(xfs("%s:fff", pName), P, Y, R);
}

//=========================================================================

RVA(0x002416d0, 0x89)
void bin_out::AddQuaternion(const char* pName, const quaternion& Q) {
    AddField(xfs("%s:ffff", pName), Q.X, Q.Y, Q.Z, Q.W);
}

//=========================================================================

RVA(0x00241760, 0x77)
void bin_out::AddBool(const char* pName, int Bool) {
    AddField(xfs("%s:d", pName), (Bool) ? (1) : (0));
}

//=========================================================================

RVA(0x002417e0, 0x75)
void bin_out::AddGuid(const char* pName, unsigned __int64 Guid) {
    AddField(xfs("%s:g", pName), Guid);
}

//=========================================================================

#define BINOUT_SWAP64(A)                                                                           \
    (((unsigned __int64)(A) >> 56) | ((unsigned __int64)(A) << 56)                                 \
     | (((unsigned __int64)(A) & 0x00FF000000000000) >> 40)                                        \
     | (((unsigned __int64)(A) & 0x000000000000FF00) << 40)                                        \
     | (((unsigned __int64)(A) & 0x0000FF0000000000) >> 24)                                        \
     | (((unsigned __int64)(A) & 0x0000000000FF0000) << 24)                                        \
     | (((unsigned __int64)(A) & 0x000000FF00000000) >> 8)                                         \
     | (((unsigned __int64)(A) & 0x00000000FF000000) << 8))
// Complete PC writer reconstruction; matching is unfinished. No bytes are masked.
RVA(0x00241860, 0x611)
void bin_out::WriteFile(int BigEndian, X_FILE* File) {
    int i, j, k;
    if (Blocks.GetCount() == 0) {
        return;
    }
    unsigned Version = 666;
    if (BigEndian) {
        Version = ENDIAN_SWAP_32(Version);
    }
    x_fwrite(&Version, 4, 1, File);
    Version = 0;
    x_fwrite(&Version, 4, 1, File);
    int StringBytes = 0;
    for (i = 0; i < Names.GetCount(); i++) {
        StringBytes += Names[i].GetLength() + 1;
    }
    if (BigEndian) {
        i = ENDIAN_SWAP_32(i);
        StringBytes = ENDIAN_SWAP_32(StringBytes);
    }
    x_fwrite(&i, 4, 1, File);
    x_fwrite(&StringBytes, 4, 1, File);
    for (i = 0; i < Names.GetCount(); i++) {
        x_fwrite((const char*)Names[i], Names[i].GetLength() + 1, 1, File);
    }
    for (i = 0; i < Blocks.GetCount(); i++) {
        i_block& Block = Blocks[i];
        Block.Header.NextBlockOffset = sizeof(block);
        for (j = 0; j < Block.Rows.GetCount(); j++) {
            i_row& Row = Block.Rows[j];
            Row.Header.ByteSize = 0;
            for (k = 0; k < Row.Fields.GetCount(); k++) {
                Row.Header.ByteSize += 8 + Row.Fields[k].ByteSize;
            }
            Block.Header.NextBlockOffset += sizeof(row) + Row.Header.ByteSize;
        }
    }
    for (i = 0; i < Blocks.GetCount(); i++) {
        i_block& Block = Blocks[i];
        Block.Header.NextBlockOffset += x_ftell(File);
        if (i == Blocks.GetCount() - 1) {
            Block.Header.NextBlockOffset = 0;
        }
        block Header;
        // Take a value snapshot before endian conversion; original spelling is unresolved.
        Header = block(Block.Header);
        if (BigEndian) {
            Header.NextBlockOffset = ENDIAN_SWAP_32(Block.Header.NextBlockOffset);
            Header.NameIndex = ENDIAN_SWAP_32(Block.Header.NameIndex);
            Header.Count = ENDIAN_SWAP_32(Block.Header.Count);
        }
        x_fwrite(&Header, sizeof(Header), 1, File);
        for (j = 0; j < Block.Rows.GetCount(); j++) {
            i_row& Row = Block.Rows[j];
            row RowHeader;
            RowHeader = Row.Header;
            if (BigEndian) {
                RowHeader.ByteSize = ENDIAN_SWAP_32(Row.Header.ByteSize);
            }
            x_fwrite(&RowHeader, sizeof(RowHeader), 1, File);
            for (k = 0; k < Row.Fields.GetCount(); k++) {
                i_field& Field = Row.Fields[k];
                bin_format::field FieldHeader;
                FieldHeader.NameIndex = Field.NameIndex;
                FieldHeader.ByteSize = Field.ByteSize;
                if (BigEndian) {
                    FieldHeader.NameIndex = ENDIAN_SWAP_32(Field.NameIndex);
                    FieldHeader.ByteSize = ENDIAN_SWAP_32(Field.ByteSize);
                }
                x_fwrite(&FieldHeader, sizeof(FieldHeader), 1, File);
                if (Field.ByteSize) {
                    if (!BigEndian) {
                        x_fwrite(Field.Data, Field.ByteSize, 1, File);
                    } else {
                        const char* Types = x_strchr(Names[Field.NameIndex], ':') + 1;
                        unsigned char* Buffer = new unsigned char[Field.ByteSize];
                        unsigned char* Out = Buffer;
                        unsigned char* In = Field.Data;
                        for (int t = 0; Types[t]; t++) {
                            switch (Types[t]) {
                                case 'D':
                                case 'd':
                                case 'F':
                                case 'f':
                                    *(unsigned*)Out = ENDIAN_SWAP_32(*(unsigned*)In);
                                    In += 4;
                                    Out += 4;
                                    break;
                                case 'G':
                                case 'g': {
                                    unsigned __int64 Guid = (unsigned __int64)*(unsigned*)In << 32;
                                    In += 4;
                                    Guid |= *(unsigned*)In;
                                    In += 4;
                                    Guid = BINOUT_SWAP64(Guid);
                                    *(unsigned*)Out = (unsigned)Guid;
                                    Out += 4;
                                    *(unsigned*)Out = (unsigned)(Guid >> 32);
                                    Out += 4;
                                } break;
                                case 'S':
                                case 's':
                                    x_strcpy((char*)Out, (char*)In);
                                    Out += (x_strlen((char*)In) + 36) & ~31;
                                    In += (x_strlen((char*)In) + 36) & ~31;
                                    break;
                            }
                        }
                        x_fwrite(Buffer, Field.ByteSize, 1, File);
                        delete[] Buffer;
                    }
                }
            }
        }
    }
}
#undef BINOUT_SWAP64

// Naturally emitted container specializations from the shared definitions.
RVA_COMPGEN(0x00241ec0, 0x1df, ?Append@?$xarray@Ui_row@bin_out@@@@QAEAAUi_row@bin_out@@XZ)
RVA_COMPGEN(0x002420a0, 0x2d, ??1?$xarray@Ui_block@bin_out@@@@QAE@XZ)
RVA_COMPGEN(0x002420d0, 0xad, ?Append@?$xarray@Tfield_types@@@@QAEAATfield_types@@XZ)
RVA_COMPGEN(0x00242180, 0x1a, ??0i_row@bin_out@@QAE@XZ)
RVA_COMPGEN(0x002421a0, 0x11, ??1i_row@bin_out@@QAE@XZ)
RVA_COMPGEN(0x002421c0, 0x54, ??_Ei_block@bin_out@@QAEPAXI@Z)
RVA_COMPGEN(0x00242220, 0x1a, ??0i_block@bin_out@@QAE@XZ)
RVA_COMPGEN(0x00242240, 0x2d, ??1i_block@bin_out@@QAE@XZ)
RVA_COMPGEN(0x00242270, 0x18f, ??4?$xarray@Ui_row@bin_out@@@@QAEABV0@ABV0@@Z)
