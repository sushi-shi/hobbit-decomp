// Imported engine source reference: Area51 pristine431f72b9, Support/Animation/AnimKeyData.cpp
// Provisional Hobbit adaptation unless an explicit PC RVA is recorded. See IMPORT-NOTES.md.
#include <rva.h>
#include <xCore/x_files/x_context.hpp>
#include <xCore/x_files/x_memory.hpp>
//=========================================================================
//
//  ANIMKEYDATA.CPP
//
//=========================================================================

//=========================================================================
// INCLUDES
//=========================================================================

#include <Support/Animation/AnimData.hpp>
//#include <xCore/Auxiliary/Parsing/bitstream.hpp>
#include <xCore/Auxiliary/Parsing/bitstream.hpp>

//
// Based on anim_key_formats
//
//=========================================================================
//                                            C    S  16   32
DATA(0x337a98)
static s32 s_ScaleFormatOverhead[] = {0, 12, 24, 0};
DATA(0x337aa8)
static s32 s_ScaleFormatSize[] = {0, 0, 2, 12};
DATA(0x337ab8)
static s32 s_RotationFormatOverhead[] = {0, 16, 0, 0};
DATA(0x337ac8)
static s32 s_RotationFormatSize[] = {0, 0, 8, 16};

//=========================================================================

DATA(0x003670b8)
static anim_key_block* s_pMRUBlock = NULL;
DATA(0x003670bc)
static anim_key_block* s_pLRUBlock = NULL;
DATA(0x003670c8)
static s32 s_nBlocksDecompressedTotal = 0;
DATA(0x003670c0)
static s32 s_nBlocksDecompressed = 0;
DATA(0x003670c4)
static s32 s_nBlockBytesDecompressed = 0;
DATA(0x00337ad8)
static s32 s_MaxAllowedDecompressedBytes = 300 * 1024;

//=========================================================================

DATA(0x3670ac)
s32 anim_key_stream::s_SF;
DATA(0x3670a0)
s32 anim_key_stream::s_RF;
DATA(0x3670b0)
s32 anim_key_stream::s_TF;
DATA(0x3670b4)
s32 anim_key_stream::s_SO;
DATA(0x36709c)
s32 anim_key_stream::s_RO;
DATA(0x3670a8)
s32 anim_key_stream::s_TO;
DATA(0x3670a4)
byte* anim_key_stream::s_pData;

//=========================================================================

extern void AnimationDecompress(
    const anim_group& AG,
    const byte* pCompressedData,
    anim_key_stream* pStream,
    s32 DecompressedSize
);

//=========================================================================

RVA(0x0013f530, 0xa)
void anim_SetMaxAllowedDecompressedBytes(int NBytes) {
    s_MaxAllowedDecompressedBytes = NBytes;
}

//=========================================================================
//=========================================================================
//=========================================================================
// ANIM_KEY
//=========================================================================
//=========================================================================
//=========================================================================

/*
void anim_key::Interpolate( const anim_key& K0, const anim_key& K1, f32 T )
{
#if USE_SCALE_KEYS    
    Scale       = K0.Scale       + T*(K1.Scale       - K0.Scale);
#endif

    Rotation    = Blend( K0.Rotation, K1.Rotation, T );
    Translation = K0.Translation + T*(K1.Translation - K0.Translation);
}
*/

//=========================================================================

void anim_key::Identity(void) {
#if USE_SCALE_KEYS
    Scale.Set(1, 1, 1);
#endif
    Translation.Zero();
    Rotation.Identity();
}

//=========================================================================

void anim_key::Setup(matrix4& M) {
#if USE_SCALE_KEYS
    M.Setup(Scale, Rotation, Translation);
#else
    // Fill out 3x3 rotations.
    f32 tx = 2.0f * Rotation.X; // 2x
    f32 ty = 2.0f * Rotation.Y; // 2y
    f32 tz = 2.0f * Rotation.Z; // 2z
    f32 txw = tx * Rotation.W;  // 2x * w
    f32 tyw = ty * Rotation.W;  // 2y * w
    f32 tzw = tz * Rotation.W;  // 2z * w
    f32 txx = tx * Rotation.X;  // 2x * x
    f32 tyx = ty * Rotation.X;  // 2y * x
    f32 tzx = tz * Rotation.X;  // 2z * x
    f32 tyy = ty * Rotation.Y;  // 2y * y
    f32 tzy = tz * Rotation.Y;  // 2z * y
    f32 tzz = tz * Rotation.Z;  // 2z * z
    M(0, 0) = 1.0f - (tyy + tzz);
    M(0, 1) = tyx + tzw;
    M(0, 2) = tzx - tyw;
    M(1, 0) = tyx - tzw;
    M(1, 1) = 1.0f - (txx + tzz);
    M(1, 2) = tzy + txw;
    M(2, 0) = tzx + tyw;
    M(2, 1) = tzy - txw;
    M(2, 2) = 1.0f - (txx + tyy);

    // Fill out translation
    M.SetTranslation(Translation);

    // Fill out last column
    M(0, 3) = 0.0f;
    M(1, 3) = 0.0f;
    M(2, 3) = 0.0f;
    M(3, 3) = 1.0f;
#endif
}

//=========================================================================
//=========================================================================
//=========================================================================
// ANIM_KEY_STREAM
//=========================================================================
//=========================================================================
//=========================================================================

void anim_key_stream::GetOffsetsAndFormats(
    s32 nFrames,
    s32& SO,
    s32& RO,
    s32& TO,
    anim_key_format& SF,
    anim_key_format& RF,
    anim_key_format& TF
) const {
    SF = (anim_key_format)((Offset >> STREAM_SCL_SHIFT) & STREAM_SCL_MASK);
    RF = (anim_key_format)((Offset >> STREAM_ROT_SHIFT) & STREAM_ROT_MASK);
    TF = (anim_key_format)((Offset >> STREAM_TRS_SHIFT) & STREAM_TRS_MASK);
    SO = (Offset >> STREAM_OFT_SHIFT) & STREAM_OFT_MASK;
    RO = SO + s_ScaleFormatOverhead[SF] + s_ScaleFormatSize[SF] * nFrames;
    TO = RO + s_RotationFormatOverhead[RF] + s_RotationFormatSize[RF] * nFrames;
}

//=========================================================================

#if defined(HOBBIT_ANIMATION_LATER_KEY_ACCESS)
inline void anim_key_stream::GrabKey(s32 iFrame, anim_key& Key) {
#if USE_SCALE_KEYS

    // Decompress scale
    {
        if (s_SF == CONSTANT_VALUE) {
            Key.Scale.Set(1.0f, 1.0f, 1.0f);
        } else if (s_SF == SINGLE_VALUE) {
            Key.Scale = ((vector3p*)(s_pData + s_SO))[0];
        } else if (s_SF == PRECISION_32) {
            Key.Scale = ((vector3p*)(s_pData + s_SO))[iFrame];
        } else {
            ASSERT(FALSE);
        }
    }
#endif

    // Decompress rotation
    {
        if (s_RF == PRECISION_16) {
            // I'm using temp variables to tell the compiler that pR doesn't
            // point to Key.Rotation so it can do the math out of order.
            u16* pR = &((u16*)(s_pData + s_RO))[iFrame << 2];
            f32 TempX = ((f32)pR[0] * (2.0f / 65535.0f)) - 1.0f;
            f32 TempY = ((f32)pR[1] * (2.0f / 65535.0f)) - 1.0f;
            f32 TempZ = ((f32)pR[2] * (2.0f / 65535.0f)) - 1.0f;
            f32 TempW = ((f32)pR[3] * (2.0f / 65535.0f)) - 1.0f;
            Key.Rotation.X = TempX;
            Key.Rotation.Y = TempY;
            Key.Rotation.Z = TempZ;
            Key.Rotation.W = TempW;
        } else if (s_RF == CONSTANT_VALUE) {
            Key.Rotation.Identity();
        } else if (s_RF == SINGLE_VALUE) {
            Key.Rotation = ((quaternion*)(s_pData + s_RO))[0];
        } else if (s_RF == PRECISION_32) {
            Key.Rotation = ((quaternion*)(s_pData + s_RO))[iFrame];
        } else {
            ASSERT(FALSE);
        }
    }

    // Decompress translation
    {
        if (s_TF == CONSTANT_VALUE) {
            Key.Translation.Set(0.0f, 0.0f, 0.0f); // = vector3(0,0,0);
        } else if (s_TF == SINGLE_VALUE) {
            Key.Translation = ((vector3p*)(s_pData + s_TO))[0];
        } else if (s_TF == PRECISION_32) {
            Key.Translation = ((vector3p*)(s_pData + s_TO))[iFrame];
        } else {
            ASSERT(FALSE);
        }
    }
}

#else
inline void anim_key_stream::GrabKey(s32 iFrame, anim_key& Key) {
#if USE_SCALE_KEYS

    // Decompress scale
    {
        if (s_SF == CONSTANT_VALUE) {
            Key.Scale.Set(1.0f, 1.0f, 1.0f);
        } else if (s_SF == SINGLE_VALUE) {
            const vector3p& V = ((vector3p*)(s_pData + s_SO))[0];
            Key.Scale.X = V.X;
            Key.Scale.Y = V.Y;
            Key.Scale.Z = V.Z;
        } else if (s_SF == PRECISION_32) {
            const vector3p& V = ((vector3p*)(s_pData + s_SO))[iFrame];
            Key.Scale.X = V.X;
            Key.Scale.Y = V.Y;
            Key.Scale.Z = V.Z;
        }
    }
#endif

    // Decompress rotation
    {
        if (s_RF == CONSTANT_VALUE) {
            Key.Rotation.X = 0.0f;
            Key.Rotation.Y = 0.0f;
            Key.Rotation.Z = 0.0f;
            Key.Rotation.W = 1.0f;
        } else if (s_RF == SINGLE_VALUE) {
            Key.Rotation = ((quaternion*)(s_pData + s_RO))[0];
        } else if (s_RF == PRECISION_16) {
            u16* pR = &((u16*)(s_pData + s_RO))[iFrame << 2];
            Key.Rotation.X = ((f32)pR[0] * (2.0f / 65535.0f)) - 1.0f;
            Key.Rotation.Y = ((f32)pR[1] * (2.0f / 65535.0f)) - 1.0f;
            Key.Rotation.Z = ((f32)pR[2] * (2.0f / 65535.0f)) - 1.0f;
            Key.Rotation.W = ((f32)pR[3] * (2.0f / 65535.0f)) - 1.0f;
        } else if (s_RF == PRECISION_32) {
            Key.Rotation = ((quaternion*)(s_pData + s_RO))[iFrame];
        }
    }

    // Decompress translation
    {
        if (s_TF == CONSTANT_VALUE) {
            Key.Translation = vector3(0.0f, 0.0f, 0.0f);
        } else if (s_TF == SINGLE_VALUE) {
            Key.Translation = ((vector3*)(s_pData + s_TO))[0];
        } else if (s_TF == PRECISION_32) {
            Key.Translation = ((vector3*)(s_pData + s_TO))[iFrame];
        }
    }
}

#endif

//=========================================================================

inline void anim_key_stream::GetRawKey(byte* pData, s32 nFrames, s32 iFrame, anim_key& Key) {
    s_SF = (Offset >> STREAM_SCL_SHIFT) & STREAM_SCL_MASK;
    s_RF = (Offset >> STREAM_ROT_SHIFT) & STREAM_ROT_MASK;
    s_TF = (Offset >> STREAM_TRS_SHIFT) & STREAM_TRS_MASK;
    s_SO = (Offset >> STREAM_OFT_SHIFT) & STREAM_OFT_MASK;
    s_RO = s_SO + s_ScaleFormatOverhead[s_SF] + s_ScaleFormatSize[s_SF] * nFrames;
    s_TO = s_RO + s_RotationFormatOverhead[s_RF] + s_RotationFormatSize[s_RF] * nFrames;
    s_pData = pData;

    GrabKey(iFrame, Key);
}

//=========================================================================

#if defined(HOBBIT_ANIMATION_LATER_KEY_ACCESS)
void anim_key_stream::GetInterpKey(byte* pData, s32 nFrames, s32 iFrame, f32 T, anim_key& Key) {
    ASSERT(iFrame < nFrames - 1);

    s_pData = pData;
    s_SF = (Offset >> STREAM_SCL_SHIFT) & STREAM_SCL_MASK;
    s_RF = (Offset >> STREAM_ROT_SHIFT) & STREAM_ROT_MASK;
    s_TF = (Offset >> STREAM_TRS_SHIFT) & STREAM_TRS_MASK;
    s_SO = (Offset >> STREAM_OFT_SHIFT) & STREAM_OFT_MASK;
    s_RO = s_SO + s_ScaleFormatOverhead[s_SF] + s_ScaleFormatSize[s_SF] * nFrames;
    s_TO = s_RO + s_RotationFormatOverhead[s_RF] + s_RotationFormatSize[s_RF] * nFrames;
    anim_key K0;
    anim_key K1;

    GrabKey(iFrame + 0, K0);
    GrabKey(iFrame + 1, K1);

    Key.Interpolate(K0, K1, T);
}

#else
inline void anim_key_stream::GetInterpKey(byte* pData, s32 nFrames, s32 iFrame, f32 T, anim_key& Key) {
    ASSERT(iFrame < nFrames - 1);
    s_pData = pData;
    s_SF = (Offset >> STREAM_SCL_SHIFT) & STREAM_SCL_MASK;
    s_RF = (Offset >> STREAM_ROT_SHIFT) & STREAM_ROT_MASK;
    s_TF = (Offset >> STREAM_TRS_SHIFT) & STREAM_TRS_MASK;
    s_SO = (Offset >> STREAM_OFT_SHIFT) & STREAM_OFT_MASK;
    s_RO = s_SO + s_ScaleFormatOverhead[s_SF] + s_ScaleFormatSize[s_SF] * nFrames;
    s_TO = s_RO + s_RotationFormatOverhead[s_RF] + s_RotationFormatSize[s_RF] * nFrames;

    GrabAndInterpKeys(iFrame, T, Key);
}

RVA(0x140000, 0x42a)
void anim_key_stream::GrabAndInterpKeys(s32 iFrame, f32 T, anim_key& Key) {
    // The earlier PC decoder completes constant streams once, and interpolates
    // only varying streams. Packed vector records have three genuine floats.
    s32 nHandled = 0;
    if (s_SF == CONSTANT_VALUE) {
        Key.Scale.Set(1.0f, 1.0f, 1.0f);
        ++nHandled;
    } else if (s_SF == SINGLE_VALUE) {
        const vector3p& V = ((vector3p*)(s_pData + s_SO))[0];
        Key.Scale.X = V.X;
        Key.Scale.Y = V.Y;
        Key.Scale.Z = V.Z;
        ++nHandled;
    }

    if (s_RF == PRECISION_16) {
        const u16* pR = &((u16*)(s_pData + s_RO))[iFrame << 2];
        quaternion Q0;
        quaternion Q1;
        // Packed quaternion factor: 2.0f / 65535.0f rounded to f32.
        Q0.X = ((f32)pR[0] * DATA_COMPGEN(0x002f3bf0, 0.00003051804378628731f)) - 1.0f;
        Q0.Y = ((f32)pR[1] * (2.0f / 65535.0f)) - 1.0f;
        Q0.Z = ((f32)pR[2] * (2.0f / 65535.0f)) - 1.0f;
        Q0.W = ((f32)pR[3] * (2.0f / 65535.0f)) - 1.0f;
        Q1.X = ((f32)pR[4] * (2.0f / 65535.0f)) - 1.0f;
        Q1.Y = ((f32)pR[5] * (2.0f / 65535.0f)) - 1.0f;
        Q1.Z = ((f32)pR[6] * (2.0f / 65535.0f)) - 1.0f;
        Q1.W = ((f32)pR[7] * (2.0f / 65535.0f)) - 1.0f;
        f32 Dot = Q0.X*Q1.X + Q0.Y*Q1.Y + Q0.Z*Q1.Z + Q0.W*Q1.W;
        if ((Dot > 2.0f) || (Dot < -2.0f)) {
            Key.Rotation = Q0;
        } else {
            // Genuine original Blend polynomial ancestry, with the earlier PC
            // decoder's precomputed dot and direct result fields.
            f32 x0,y0,z0,w0;
            if (Dot < 0.0f) {
                x0 = -Q0.X; y0 = -Q0.Y; z0 = -Q0.Z; w0 = -Q0.W;
            } else {
                x0 = Q0.X; y0 = Q0.Y; z0 = Q0.Z; w0 = Q0.W;
            }
            x0 = x0 + T*(Q1.X-x0);
            y0 = y0 + T*(Q1.Y-y0);
            z0 = z0 + T*(Q1.Z-z0);
            w0 = w0 + T*(Q1.W-w0);
            f32 LenSquared = x0*x0 + y0*y0 + z0*z0 + w0*w0;
            f32 OneOverL;
            if (LenSquared < 0.857f)
                OneOverL = (0.699368f*LenSquared - 1.819985f)*LenSquared + 2.126369f;
            else
                OneOverL = (0.454012f*LenSquared - 1.403517f)*LenSquared + 1.949542f;
            Key.Rotation.X = x0*OneOverL;
            Key.Rotation.Y = y0*OneOverL;
            Key.Rotation.Z = z0*OneOverL;
            Key.Rotation.W = w0*OneOverL;
        }
        ++nHandled;
    } else if (s_RF == SINGLE_VALUE) {
        Key.Rotation = ((quaternion*)(s_pData + s_RO))[0];
        ++nHandled;
    }

    if (s_TF == CONSTANT_VALUE) {
        Key.Translation.X = 0.0f;
        Key.Translation.Y = 0.0f;
        Key.Translation.Z = 0.0f;
        ++nHandled;
    } else if (s_TF == SINGLE_VALUE) {
        const vector3p& V = ((vector3p*)(s_pData + s_TO))[0];
        Key.Translation.X = V.X;
        Key.Translation.Y = V.Y;
        Key.Translation.Z = V.Z;
        ++nHandled;
    }
    if (nHandled == 3)
        return;

    if (s_SF == PRECISION_32) {
        const vector3p& V0 = ((vector3p*)(s_pData + s_SO))[iFrame];
        const vector3p& V1 = ((vector3p*)(s_pData + s_SO))[iFrame+1];
        Key.Scale.X = V0.X + T*(V1.X-V0.X);
        Key.Scale.Y = V0.Y + T*(V1.Y-V0.Y);
        Key.Scale.Z = V0.Z + T*(V1.Z-V0.Z);
    }
    if (s_RF == CONSTANT_VALUE) {
        Key.Rotation.X = 0.0f;
        Key.Rotation.Y = 0.0f;
        Key.Rotation.Z = 0.0f;
        Key.Rotation.W = 1.0f;
    } else if (s_RF == PRECISION_32) {
        const quaternion& Q0 = ((quaternion*)(s_pData + s_RO))[iFrame];
        const quaternion& Q1 = ((quaternion*)(s_pData + s_RO))[iFrame+1];
        Key.Rotation = Blend(Q0, Q1, T);
    }
    if (s_TF == PRECISION_32) {
        const vector3p& V0 = ((vector3p*)(s_pData + s_TO))[iFrame];
        const vector3p& V1 = ((vector3p*)(s_pData + s_TO))[iFrame+1];
        Key.Translation.X = V0.X + T*(V1.X-V0.X);
        Key.Translation.Y = V0.Y + T*(V1.Y-V0.Y);
        Key.Translation.Z = V0.Z + T*(V1.Z-V0.Z);
    }
}
#endif

//=========================================================================

u32 anim_key_stream::GetFlags(void) const {
    return (Offset >> STREAM_FLG_SHIFT) & STREAM_FLG_MASK;
}

//=========================================================================

void anim_key_stream::SetFlags(u32 Flags) {
    // Clear current flags
    Offset &= ~(STREAM_FLG_MASK << STREAM_FLG_SHIFT);

    // Write new flags
    Offset |= (Flags << STREAM_FLG_SHIFT);
}

//=========================================================================

void anim_key_stream::SetFormats(anim_key_format SF, anim_key_format RF, anim_key_format TF) {
    // Clear current formats
    Offset &= ~(STREAM_SCL_MASK << STREAM_SCL_SHIFT);
    Offset &= ~(STREAM_ROT_MASK << STREAM_ROT_SHIFT);
    Offset &= ~(STREAM_TRS_MASK << STREAM_TRS_SHIFT);

    // Write new formats
    Offset |= ((u32)SF << STREAM_SCL_SHIFT);
    Offset |= ((u32)RF << STREAM_ROT_SHIFT);
    Offset |= ((u32)TF << STREAM_TRS_SHIFT);
}

//=========================================================================

//=========================================================================
//=========================================================================
//=========================================================================
// ANIM_KEY_BLOCK
//=========================================================================
//=========================================================================
//=========================================================================

inline void anim_key_block::AttachToList() {
    pNext = s_pMRUBlock;
    pPrev = 0;
    if (s_pMRUBlock) {
        s_pMRUBlock->pPrev = this;
    }
    if (s_pLRUBlock == 0) {
        s_pLRUBlock = this;
    }
    s_pMRUBlock = this;
}

//=========================================================================

inline void anim_key_block::DetachFromList() {
    if (pNext) {
        pNext->pPrev = pPrev;
    }
    if (pPrev) {
        pPrev->pNext = pNext;
    }
    if (s_pMRUBlock == this) {
        s_pMRUBlock = pNext;
    }
    if (s_pLRUBlock == this) {
        s_pLRUBlock = pPrev;
    }
    pNext = 0;
    pPrev = 0;
}

//=========================================================================

RVA(0x0013f540, 0x1aa)
anim_key_stream* anim_key_block::AcquireStreams(const anim_group& AG) {
    xcontext Context("anim_key_block::AcquireStreams");
    DetachFromList();
    AttachToList();
    if (pStream) {
        return pStream;
    }
    while (1) {
        while ((s_nBlockBytesDecompressed + DecompressedDataSize) > s_MaxAllowedDecompressedBytes) {
            s_pLRUBlock->ReleaseStreams();
            if (s_pMRUBlock == 0) {
                break;
            }
        }
        pStream = static_cast<anim_key_stream*>(x_malloc_fn(
            DecompressedDataSize,
            "C:\\projects\\meridian\\Support\\Animation\\AnimKeyData.cpp",
            121
        ));
        if (pStream) {
            break;
        }
        s_pLRUBlock->ReleaseStreams();
    }
    s_nBlockBytesDecompressed += DecompressedDataSize;
    s_nBlocksDecompressed++;
    s_nBlocksDecompressedTotal++;
    AnimationDecompress(
        AG,
        AG.GetCompressedDataPtr() + CompressedDataOffset,
        pStream,
        DecompressedDataSize
    );
    return pStream;
}

//=========================================================================

RVA(0x0013f6f0, 0x89)
void anim_key_block::ReleaseStreams() {
    if (pStream == 0) {
        return;
    }
    DetachFromList();
    x_free_fn(pStream, "C:\\projects\\meridian\\Support\\Animation\\AnimKeyData.cpp", 159);
    pStream = 0;
    s_nBlockBytesDecompressed -= DecompressedDataSize;
    s_nBlocksDecompressed--;
}

//=========================================================================
//=========================================================================
//=========================================================================
// ANIM_KEYS
//=========================================================================
//=========================================================================
//=========================================================================

anim_keys::anim_keys(void) {}

//=========================================================================

RVA(0x0013f790, 0x1)
anim_keys::~anim_keys(void) {}

//=========================================================================

xbool anim_keys::IsBoneMasked(const anim_group& AnimGroup, s32 iBone) const {
    ASSERT((iBone >= 0) && (iBone < m_nBones));
    anim_key_block& KeyBlock = AnimGroup.m_pKeyBlock[m_iKeyBlock];
    anim_key_stream* pStream = KeyBlock.AcquireStreams(AnimGroup);
    return (pStream[iBone].GetFlags() & STREAM_FLAG_MASKED) ? (TRUE) : (FALSE);
}

//=========================================================================

RVA(0x13f7a0, 0x28f)
void anim_keys::GetRawKey(
    const anim_group& AnimGroup,
    s32 iFrame,
    s32 iStream,
    anim_key& Key
) const {
    ASSERT((iStream >= 0) && (iStream < (m_nBones + m_nProps)));

    s32 iBlock = iFrame >> MAX_KEYS_PER_BLOCK_SHIFT;
    s32 iBlockFrame = iFrame & MAX_KEYS_PER_BLOCK_MASK;
    if ((iBlock == m_nKeyBlocks) && (iBlockFrame == 0)) {
        iBlock--;
        iBlockFrame = MAX_KEYS_PER_BLOCK;
    }
    ASSERT((iBlock >= 0) && (iBlock < m_nKeyBlocks));

    anim_key_block& KeyBlock = AnimGroup.m_pKeyBlock[m_iKeyBlock + iBlock];
    anim_key_stream* pStream = KeyBlock.AcquireStreams(AnimGroup);

    pStream[iStream].GetRawKey((byte*)pStream, KeyBlock.nFrames, iBlockFrame, Key);
}

//=========================================================================

RVA(0x13fa30, 0xe5)
void anim_keys::GetInterpKey(
    const anim_group& AnimGroup,
    f32 Frame,
    s32 iStream,
    anim_key& Key
) const {
    ASSERT((iStream >= 0) && (iStream < (m_nBones + m_nProps)));

    s32 iFrame = (s32)Frame;
    f32 fFrac = Frame - (f32)iFrame;
    s32 iBlock = iFrame >> MAX_KEYS_PER_BLOCK_SHIFT;
    s32 iBlockFrame = iFrame & MAX_KEYS_PER_BLOCK_MASK;
    if ((iBlock == m_nKeyBlocks) && (iBlockFrame == 0)) {
        iBlock--;
        iBlockFrame = MAX_KEYS_PER_BLOCK;
    }
    ASSERT((iBlock >= 0) && (iBlock < m_nKeyBlocks));

    anim_key_block& KeyBlock = AnimGroup.m_pKeyBlock[m_iKeyBlock + iBlock];
    anim_key_stream* pStream = KeyBlock.AcquireStreams(AnimGroup);

    pStream[iStream].GetInterpKey((byte*)pStream, KeyBlock.nFrames, iBlockFrame, fFrac, Key);
}

//=========================================================================

RVA(0x13fb20, 0x329)
void anim_keys::GetRawKeys(const anim_group& AnimGroup, s32 iFrame, anim_key* pKey) const {
    xcontext Context("anim_keys::GetRawKeys");
    s32 iBlock = iFrame >> MAX_KEYS_PER_BLOCK_SHIFT;
    s32 iBlockFrame = iFrame & MAX_KEYS_PER_BLOCK_MASK;
    if ((iBlock == m_nKeyBlocks) && (iBlockFrame == 0)) {
        iBlock--;
        iBlockFrame = MAX_KEYS_PER_BLOCK;
    }
    ASSERT((iBlock >= 0) && (iBlock < m_nKeyBlocks));

    anim_key_block& KeyBlock = AnimGroup.m_pKeyBlock[m_iKeyBlock + iBlock];
    anim_key_stream* pStream = KeyBlock.AcquireStreams(AnimGroup);

    for (s32 i = 0; i < m_nBones; i++) {
        pStream[i].GetRawKey((byte*)pStream, KeyBlock.nFrames, iBlockFrame, pKey[i]);
    }
}

//=========================================================================

RVA(0x13fe50, 0x1ac)
void anim_keys::GetInterpKeys(const anim_group& AnimGroup, f32 Frame, anim_key* pKey) const {
    xcontext Context("anim_keys::GetInterpKeys");
    s32 iFrame = (s32)Frame;
    f32 fFrac = Frame - (f32)iFrame;
    s32 iBlock = iFrame >> MAX_KEYS_PER_BLOCK_SHIFT;
    s32 iBlockFrame = iFrame & MAX_KEYS_PER_BLOCK_MASK;
    if ((iBlock == m_nKeyBlocks) && (iBlockFrame == 0)) {
        iBlock--;
        iBlockFrame = MAX_KEYS_PER_BLOCK;
    }
    ASSERT((iBlock >= 0) && (iBlock < m_nKeyBlocks));

    anim_key_block& KeyBlock = AnimGroup.m_pKeyBlock[m_iKeyBlock + iBlock];
    anim_key_stream* pStream = KeyBlock.AcquireStreams(AnimGroup);

    for (s32 i = 0; i < m_nBones; i++) {
        if (pStream[i].Offset & (STREAM_FLAG_MASKED << STREAM_FLG_SHIFT)) {
            pKey[i].Scale.Set(1.0f, 1.0f, 1.0f);
            pKey[i].Rotation = quaternion(0.0f, 0.0f, 0.0f, 1.0f);
            pKey[i].Translation.X = 0.0f;
            pKey[i].Translation.Y = 0.0f;
            pKey[i].Translation.Z = 0.0f;
        } else {
            pStream[i].GetInterpKey((byte*)pStream, KeyBlock.nFrames, iBlockFrame, fFrac, pKey[i]);
        }
    }
}

//=========================================================================

void anim_keys::GetInterpKeys(
    const anim_group& AnimGroup,
    f32 Frame,
    anim_key* pKey,
    s32 nBones
) const {
    s32 iFrame = (s32)Frame;
    f32 fFrac = Frame - (f32)iFrame;
    s32 iBlock = iFrame >> MAX_KEYS_PER_BLOCK_SHIFT;
    s32 iBlockFrame = iFrame & MAX_KEYS_PER_BLOCK_MASK;
    if ((iBlock == m_nKeyBlocks) && (iBlockFrame == 0)) {
        iBlock--;
        iBlockFrame = MAX_KEYS_PER_BLOCK;
    }
    ASSERT((iBlock >= 0) && (iBlock < m_nKeyBlocks));

    anim_key_block& KeyBlock = AnimGroup.m_pKeyBlock[m_iKeyBlock + iBlock];
    anim_key_stream* pStream = KeyBlock.AcquireStreams(AnimGroup);

    ASSERT(nBones <= m_nBones);
    for (s32 i = 0; i < nBones; i++) {
        pStream[i].GetInterpKey((byte*)pStream, KeyBlock.nFrames, iBlockFrame, fFrac, pKey[i]);
    }
}

//=========================================================================

/*




//=========================================================================
//=========================================================================
//=========================================================================
// ANIM_KEY_SET
//=========================================================================
//=========================================================================
//=========================================================================




/*


//=========================================================================

void anim_key_set::GetRawKey( s32 iFrame, anim_key& Key ) const
{
    //
    // Get Scale
    //
    {

        if( m_ScaleFormat == KNOWN_CONSTANT )
        {
            Key.Scale.X = 1.0f;
            Key.Scale.Y = 1.0f;
            Key.Scale.Z = 1.0f;
        }
        else
        if( m_ScaleFormat == FULL_PRECISION )
        {
            vector3* pD = (vector3*)m_pScale;
            Key.Scale = pD[iFrame];
            #ifdef TARGET_GCN   
            SwapEndian( Key.Scale );  
            #endif
        }
        else
        if( m_ScaleFormat == SINGLE_VALUE )
        {
            vector3* pD = (vector3*)m_pScale;
            Key.Scale = pD[0];
            #ifdef TARGET_GCN   
            SwapEndian( Key.Scale );  
            #endif
        }
    }

    //
    // Get Rotation
    //
    {
        if( m_RotationFormat == QUAT_8_PACK )
        {
            static f32 ITable[16] = {0,0.142857f,0.28571f,0.428571f,0.571428f,0.714285f,0.857143f,1.0f,
                                    0.1f,0.2f,0.3f,0.5f,0.6f,0.7f,0.8f,0.9f};

            s32 iPack = iFrame >> 3;
            s32 iQ    = iFrame & 7;
            u64 PA = *((u64*)(m_pRotation + sizeof(u64)*(iPack<<1) + 0));
            u64 PB = *((u64*)(m_pRotation + sizeof(u64)*(iPack<<1) + sizeof(u64)));

            #ifdef TARGET_GCN
            SwapEndian(PA);
            SwapEndian(PB);
            #endif

            // unpack boundary quaternions
            quaternion QA,QB;
            QA.X = (((s64)PA<<(13*0))>>(64-(13*1))) * (1.0f/4095.0f);
            QA.Y = (((s64)PA<<(13*1))>>(64-(13*1))) * (1.0f/4095.0f);
            QA.Z = (((s64)PA<<(13*2))>>(64-(13*1))) * (1.0f/4095.0f);
            QA.W = (((s64)PA<<(13*3))>>(64-(13*1))) * (1.0f/4095.0f);
            QB.X = (((s64)PB<<(13*0))>>(64-(13*1))) * (1.0f/4095.0f);
            QB.Y = (((s64)PB<<(13*1))>>(64-(13*1))) * (1.0f/4095.0f);
            QB.Z = (((s64)PB<<(13*2))>>(64-(13*1))) * (1.0f/4095.0f);
            QB.W = (((s64)PB<<(13*3))>>(64-(13*1))) * (1.0f/4095.0f);

            // Blend between the boundary quaternions
            f32 T;
            if( iQ==0 ) T = 0.0f;
            else
            if( iQ==7 ) T = 1.0f;
            else
            if( iQ<4 ) T = ITable[((PA>>((3-iQ)<<2)) & 0xF)];
            else       T = ITable[((PB>>((6-iQ)<<2)) & 0xF)];

            Key.Rotation = Blend(QA,QB,T);

        }
        else
        if( m_RotationFormat == PRECISION_8 )
        {
            f32 Min[4];
            f32 Max[4];
            Min[0] = ((f32*)m_pRotation)[0];
            Min[1] = ((f32*)m_pRotation)[1];
            Min[2] = ((f32*)m_pRotation)[2];
            Min[3] = ((f32*)m_pRotation)[3];
            Max[0] = ((f32*)m_pRotation)[4];
            Max[1] = ((f32*)m_pRotation)[5];
            Max[2] = ((f32*)m_pRotation)[6];
            Max[3] = ((f32*)m_pRotation)[7];

            #ifdef TARGET_GCN
            SwapEndian( Min[0] );
            SwapEndian( Min[1] );
            SwapEndian( Min[2] );
            SwapEndian( Min[3] );
            SwapEndian( Max[0] );
            SwapEndian( Max[1] );
            SwapEndian( Max[2] );
            SwapEndian( Max[3] );
            #endif

            byte* pI = m_pRotation + (sizeof(f32)*8) + (iFrame<<2);

            Key.Rotation.X = Min[0] + (pI[0]/255.0f)*(Max[0]-Min[0]);
            Key.Rotation.Y = Min[1] + (pI[1]/255.0f)*(Max[1]-Min[1]);
            Key.Rotation.Z = Min[2] + (pI[2]/255.0f)*(Max[2]-Min[2]);
            Key.Rotation.W = Min[3] + (pI[3]/255.0f)*(Max[3]-Min[3]);
        }
        else
        if( m_RotationFormat == PRECISION_16 )
        {
            s16* pD = (s16*)m_pRotation;
            pD += (iFrame<<2);
            s16 D[4];
            D[0] = pD[0];
            D[1] = pD[1];
            D[2] = pD[2];
            D[3] = pD[3];

            #ifdef TARGET_GCN
            SwapEndian(D[0]);
            SwapEndian(D[1]);
            SwapEndian(D[2]);
            SwapEndian(D[3]);
            #endif

            Key.Rotation.X = (f32)D[0] * (1.0f/16384.0f);
            Key.Rotation.Y = (f32)D[1] * (1.0f/16384.0f);
            Key.Rotation.Z = (f32)D[2] * (1.0f/16384.0f);
            Key.Rotation.W = (f32)D[3] * (1.0f/16384.0f);
        }
        else
        if( m_RotationFormat == FULL_PRECISION )
        {
            quaternion* pD = (quaternion*)m_pRotation;
            Key.Rotation = pD[iFrame];
            #ifdef TARGET_GCN   
            SwapEndian( Key.Rotation );  
            #endif
        }
        else
        if( m_RotationFormat == SINGLE_VALUE )
        {
            quaternion* pD = (quaternion*)m_pRotation;
            Key.Rotation = pD[0];
            #ifdef TARGET_GCN
            SwapEndian( Key.Rotation );
            #endif
        }

    }

    //
    // Get Translation
    //
    {
        if( m_TranslationFormat == SINGLE_VALUE_16 )
        {
            s16* pD = (s16*)m_pTranslation;
            s16 D[3];
            D[0] = pD[0];
            D[1] = pD[1];
            D[2] = pD[2];

            #ifdef TARGET_GCN
            SwapEndian(D[0]);
            SwapEndian(D[1]);
            SwapEndian(D[2]);
            #endif

            Key.Translation.X = (f32)D[0]*(1.0f/16.0f);
            Key.Translation.Y = (f32)D[1]*(1.0f/16.0f);
            Key.Translation.Z = (f32)D[2]*(1.0f/16.0f);
        }
        else
        if( m_TranslationFormat == PRECISION_16 )
        {
            s16* pD = (s16*)m_pTranslation;
            pD += (iFrame*3);
            s16 D[3];
            D[0] = pD[0];
            D[1] = pD[1];
            D[2] = pD[2];

            #ifdef TARGET_GCN
            SwapEndian(D[0]);
            SwapEndian(D[1]);
            SwapEndian(D[2]);
            #endif

            Key.Translation.X = (f32)D[0]*(1.0f/16.0f);
            Key.Translation.Y = (f32)D[1]*(1.0f/16.0f);
            Key.Translation.Z = (f32)D[2]*(1.0f/16.0f);
        }
        else
        if( m_TranslationFormat == FULL_PRECISION )
        {
            vector3* pD = (vector3*)m_pTranslation;
            Key.Translation = pD[iFrame];
            #ifdef TARGET_GCN   
            SwapEndian( Key.Translation );  
            #endif
        }
        else
        if( m_TranslationFormat == SINGLE_VALUE )
        {
            vector3* pD = (vector3*)m_pTranslation;
            Key.Translation = pD[0];
            #ifdef TARGET_GCN   
            SwapEndian( Key.Translation );  
            #endif
        }
    }
}


*/

//=========================================================================
