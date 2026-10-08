#include <rva.h>
#include <Support/Geometry/RenderInterface.hpp>

// PC RenderInterface.cpp settings storage. Original identifier/linkage spelling
// is unrecovered; four-byte mutable float storage and initial value are direct
// PC evidence. See PROVENANCE.json for source-owner and comparison evidence.
DATA(0x003441ec)
float g_RenderFarClipDistance = 10000.0f;

RVA(0x001a3d10, 0x42)
void SetRenderFarClipDistance(float Distance)
{
    // The negated strict comparisons preserve the PC unordered-value behavior.
    if (!(Distance > DATA_COMPGEN(0x002e99c0, 1000.0f))) g_RenderFarClipDistance = 1000.0f;
    else if (!(Distance < DATA_COMPGEN(0x002e9974, 20000.0f))) g_RenderFarClipDistance = 20000.0f;
    else g_RenderFarClipDistance = Distance;
}

RVA(0x001a3d60, 0x7)
float GetViewFarDistance()
{
    return g_RenderFarClipDistance;
}
