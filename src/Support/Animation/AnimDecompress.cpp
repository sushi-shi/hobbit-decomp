// Imported engine source reference: Area51 pristine431f72b9, Support/Animation/AnimDecompress.cpp
// Provisional Hobbit adaptation unless an explicit PC RVA is recorded. See IMPORT-NOTES.md.
#include <rva.h>
#include <Support/Animation/AnimData.hpp>
#include <xCore/Auxiliary/Parsing/bitstream.hpp>
#include <xCore/x_files/x_context.hpp>
#include <xCore/x_files/x_memory.hpp>

typedef void decomp_fn(bitstream&, const anim_group&, int, int, unsigned char*&);
void Decomp_Const(bitstream&, const anim_group&, int, int, unsigned char*&);
void DecompS_Single(bitstream&, const anim_group&, int, int, unsigned char*&);
void DecompS_Delta(bitstream&, const anim_group&, int, int, unsigned char*&);
void DecompV_32(bitstream&, const anim_group&, int, int, unsigned char*&);
void DecompQ_Single(bitstream&, const anim_group&, int, int, unsigned char*&);
void DecompQ_Delta(bitstream&, const anim_group&, int, int, unsigned char*&);
void DecompQ_32(bitstream&, const anim_group&, int, int, unsigned char*&);
void DecompQ_Delta2(bitstream&, const anim_group&, int, int, unsigned char*&);
void DecompQ_Delta3(bitstream&, const anim_group&, int, int, unsigned char*&);
void DecompQ_Delta4(bitstream&, const anim_group&, int, int, unsigned char*&);
void DecompT_LocalTrans(bitstream&, const anim_group&, int, int, unsigned char*&);
void DecompT_Single(bitstream&, const anim_group&, int, int, unsigned char*&);
void DecompT_Delta(bitstream&, const anim_group&, int, int, unsigned char*&);
DATA(0x00338068)
decomp_fn* s_ScaleDecompFnptr[] = {Decomp_Const, DecompS_Single, DecompS_Delta, DecompV_32};
DATA(0x00338078)
decomp_fn* s_RotationDecompFnptr[] = {
    Decomp_Const,
    DecompQ_Single,
    DecompQ_Delta,
    DecompQ_32,
    DecompQ_Delta2,
    DecompQ_Delta3,
    DecompQ_Delta4
};
DATA(0x00338094)
decomp_fn* s_TranslationDecompFnptr[] =
    {Decomp_Const, DecompT_LocalTrans, DecompT_Single, DecompT_Delta, DecompV_32};
RVA(0x00144cc0, 0x1)
void Decomp_Const(bitstream&, const anim_group&, int, int, unsigned char*&) {}
RVA(0x00144cd0, 0x2b)
void DecompV_32(bitstream& bs, const anim_group&, int, int frames, unsigned char*& data) {
    for (int i = 0; i < frames; i++) {
        bs.ReadVector(*(vector3*)data);
        data += sizeof(vector3);
    }
}
RVA(0x00144d00, 0x2b)
void DecompQ_32(bitstream& bs, const anim_group&, int, int frames, unsigned char*& data) {
    for (int i = 0; i < frames; i++) {
        bs.ReadQuaternion(*(quaternion*)data);
        data += sizeof(quaternion);
    }
}
RVA(0x00144d30, 0x6e)
void DecompS_Single(bitstream& bs, const anim_group&, int, int, unsigned char*& data) {
    int x, y, z;
    vector3 v;
    bs.ReadVariableLenS32(x);
    bs.ReadVariableLenS32(y);
    bs.ReadVariableLenS32(z);
    v.X = x / 128.0f;
    v.Y = y / 128.0f;
    v.Z = z / 128.0f;
    *(vector3*)data = v;
    data += sizeof(vector3);
}
RVA(0x00144da0, 0x42)
void DecompT_LocalTrans(
    bitstream&,
    const anim_group& group,
    int stream,
    int,
    unsigned char*& data
) {
    vector3 v = group.GetBone(stream).LocalTranslation;
    *(vector3*)data = v;
    data += sizeof(vector3);
}
RVA(0x00144df0, 0x6e)
void DecompT_Single(bitstream& bs, const anim_group&, int, int, unsigned char*& data) {
    int x, y, z;
    vector3 v;
    bs.ReadVariableLenS32(x);
    bs.ReadVariableLenS32(y);
    bs.ReadVariableLenS32(z);
    v.X = x / 128.0f;
    v.Y = y / 128.0f;
    v.Z = z / 128.0f;
    *(vector3*)data = v;
    data += sizeof(vector3);
}
RVA(0x00144e60, 0x8f)
void DecompQ_Single(bitstream& bs, const anim_group&, int, int, unsigned char*& data) {
    int x, y, z, w;
    quaternion q;
    bs.ReadVariableLenS32(x);
    bs.ReadVariableLenS32(y);
    bs.ReadVariableLenS32(z);
    bs.ReadVariableLenS32(w);
    q.X = x / 2048.0f;
    q.Y = y / 2048.0f;
    q.Z = z / 2048.0f;
    q.W = w / 2048.0f;
    *(quaternion*)data = q;
    data += sizeof(quaternion);
}
RVA(0x00144ef0, 0xa3)
void DeltaDecompress(bitstream& bs, float* sample, int stride, int samples, float precision) {
    int first, minimum;
    unsigned int bits;
    float reciprocal = 1.0f / precision;
    bs.ReadVariableLenS32(first);
    bs.ReadVariableLenS32(minimum);
    bs.ReadU32(bits, 5);
    sample[0] = first * reciprocal;
    for (int i = 1; i < samples; i++) {
        unsigned int d;
        bs.ReadU32(d, bits);
        sample[i * stride] = sample[(i - 1) * stride] + (((int)d + minimum) * reciprocal);
    }
}
RVA(0x00144fa0, 0xc8)
void DeltaDecompressQ(
    bitstream& bs,
    unsigned short* sample,
    int stride,
    int samples,
    float precision
) {
    int first, minimum;
    unsigned int bits;
    float reciprocal = 1.0f / precision;
    bs.ReadVariableLenS32(first);
    bs.ReadVariableLenS32(minimum);
    bs.ReadU32(bits, 5);
    float value = first * reciprocal;
    *sample = (unsigned short)((value + 1.0f) * 0.5f * 65535.0f);
    sample += stride;
    float previous = value;
    for (int i = 1; i < samples; i++) {
        unsigned int d;
        bs.ReadU32(d, bits);
        value = previous + (((int)d + minimum) * reciprocal);
        *sample = (unsigned short)((value + 1.0f) * 0.5f * 65535.0f);
        previous = value;
        sample += stride;
    }
}
RVA(0x00145070, 0x5b)
void DecompS_Delta(bitstream& bs, const anim_group&, int, int frames, unsigned char*& data) {
    DeltaDecompress(bs, &((vector3*)data)->X, 3, frames, 128.0f);
    DeltaDecompress(bs, &((vector3*)data)->Y, 3, frames, 128.0f);
    DeltaDecompress(bs, &((vector3*)data)->Z, 3, frames, 128.0f);
    data += sizeof(vector3) * frames;
}
RVA(0x001450d0, 0x5b)
void DecompT_Delta(bitstream& bs, const anim_group&, int, int frames, unsigned char*& data) {
    DeltaDecompress(bs, &((vector3*)data)->X, 3, frames, 16.0f);
    DeltaDecompress(bs, &((vector3*)data)->Y, 3, frames, 16.0f);
    DeltaDecompress(bs, &((vector3*)data)->Z, 3, frames, 16.0f);
    data += sizeof(vector3) * frames;
}
RVA(0x00145130, 0x70)
void DecompQ_Delta(bitstream& bs, const anim_group&, int, int frames, unsigned char*& data) {
    DeltaDecompressQ(bs, &((unsigned short*)data)[0], 4, frames, 2048.0f);
    DeltaDecompressQ(bs, &((unsigned short*)data)[1], 4, frames, 2048.0f);
    DeltaDecompressQ(bs, &((unsigned short*)data)[2], 4, frames, 2048.0f);
    DeltaDecompressQ(bs, &((unsigned short*)data)[3], 4, frames, 2048.0f);
    data += 8 * frames;
}
RVA(0x001451a0, 0x70)
void DecompQ_Delta2(bitstream& bs, const anim_group&, int, int frames, unsigned char*& data) {
    DeltaDecompressQ(bs, &((unsigned short*)data)[0], 4, frames, 16384.0f);
    DeltaDecompressQ(bs, &((unsigned short*)data)[1], 4, frames, 16384.0f);
    DeltaDecompressQ(bs, &((unsigned short*)data)[2], 4, frames, 16384.0f);
    DeltaDecompressQ(bs, &((unsigned short*)data)[3], 4, frames, 16384.0f);
    data += 8 * frames;
}
RVA(0x00145210, 0x70)
void DecompQ_Delta3(bitstream& bs, const anim_group&, int, int frames, unsigned char*& data) {
    DeltaDecompressQ(bs, &((unsigned short*)data)[0], 4, frames, 65536.0f);
    DeltaDecompressQ(bs, &((unsigned short*)data)[1], 4, frames, 65536.0f);
    DeltaDecompressQ(bs, &((unsigned short*)data)[2], 4, frames, 65536.0f);
    DeltaDecompressQ(bs, &((unsigned short*)data)[3], 4, frames, 65536.0f);
    data += 8 * frames;
}
RVA(0x00145280, 0x70)
void DecompQ_Delta4(bitstream& bs, const anim_group&, int, int frames, unsigned char*& data) {
    DeltaDecompressQ(bs, &((unsigned short*)data)[0], 4, frames, 512.0f);
    DeltaDecompressQ(bs, &((unsigned short*)data)[1], 4, frames, 512.0f);
    DeltaDecompressQ(bs, &((unsigned short*)data)[2], 4, frames, 512.0f);
    DeltaDecompressQ(bs, &((unsigned short*)data)[3], 4, frames, 512.0f);
    data += 8 * frames;
}
RVA(0x001452f0, 0xa3)
void RLEDecompress(bitstream& bs, unsigned int* data, int& samples) {
    unsigned int countBits, sampleBits, count, value;
    bs.ReadRangedU32(countBits, 3, 8);
    bs.ReadRangedU32(sampleBits, 0, 32);
    samples = 0;
    while (1) {
        bs.ReadU32(count, countBits);
        samples += count;
        if (count == 0) {
            break;
        }
        bs.ReadU32(value, sampleBits);
        while (count--) {
            *data++ = value;
        }
    }
}
RVA(0x001453a0, 0xa6)
void RLEDecompressOffsetInfo(
    bitstream& bs,
    anim_key_stream* stream,
    unsigned int mask,
    unsigned int shift
) {
    unsigned int countBits, sampleBits, count, value;
    bs.ReadRangedU32(countBits, 3, 8);
    bs.ReadRangedU32(sampleBits, 0, 32);
    while (1) {
        bs.ReadU32(count, countBits);
        if (count == 0) {
            break;
        }
        bs.ReadU32(value, sampleBits);
        while (count--) {
            stream->Offset &= ~(mask << shift);
            stream->Offset |= (value & mask) << shift;
            stream++;
        }
    }
}
RVA(0x00145450, 0x2c1)
void AnimationDecompress(
    const anim_group& group,
    const unsigned char* compressed,
    anim_key_stream* streams,
    int size
) {
    xcontext context("AnimationDecompress");
    int i;
    bitstream bs;
    bs.Init(compressed, 1024 * 1024);
    unsigned int keyCursor, headerCursor;
    bs.ReadU32(headerCursor);
    keyCursor = bs.GetCursor();
    bs.SetCursor(headerCursor);
    unsigned int totalStreams, frames;
    bs.ReadU32(totalStreams, 10);
    bs.ReadRangedU32(frames, 2, 33);
    RLEDecompressOffsetInfo(bs, streams, 3, 30);
    RLEDecompressOffsetInfo(bs, streams, 3, 28);
    RLEDecompressOffsetInfo(bs, streams, 3, 26);
    RLEDecompressOffsetInfo(bs, streams, 255, 18);
    int streamCount;
    unsigned int* scaleIndices = (unsigned int*)x_malloc_fn(
        sizeof(unsigned int) * totalStreams,
        "C:\\projects\\meridian\\Support\\Animation\\AnimDecompress.cpp",
        529
    );
    unsigned int* rotationIndices = (unsigned int*)x_malloc_fn(
        sizeof(unsigned int) * totalStreams,
        "C:\\projects\\meridian\\Support\\Animation\\AnimDecompress.cpp",
        530
    );
    unsigned int* translationIndices = (unsigned int*)x_malloc_fn(
        sizeof(unsigned int) * totalStreams,
        "C:\\projects\\meridian\\Support\\Animation\\AnimDecompress.cpp",
        531
    );
    RLEDecompress(bs, scaleIndices, streamCount);
    RLEDecompress(bs, rotationIndices, streamCount);
    RLEDecompress(bs, translationIndices, streamCount);
    bs.SetCursor(keyCursor);
    unsigned char* data = (unsigned char*)streams + sizeof(anim_key_stream) * totalStreams;
    for (i = 0; i < (int)totalStreams; i++) {
        streams[i].SetOffset((unsigned int)data - (unsigned int)streams);
        s_ScaleDecompFnptr[scaleIndices[i]](bs, group, i, frames, data);
        s_RotationDecompFnptr[rotationIndices[i]](bs, group, i, frames, data);
        s_TranslationDecompFnptr[translationIndices[i]](bs, group, i, frames, data);
    }
    x_free_fn(scaleIndices, "C:\\projects\\meridian\\Support\\Animation\\AnimDecompress.cpp", 574);
    x_free_fn(
        rotationIndices,
        "C:\\projects\\meridian\\Support\\Animation\\AnimDecompress.cpp",
        575
    );
    x_free_fn(
        translationIndices,
        "C:\\projects\\meridian\\Support\\Animation\\AnimDecompress.cpp",
        576
    );
}
