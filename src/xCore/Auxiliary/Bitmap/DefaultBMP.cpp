#include <rva.h>

#include <xCore/Auxiliary/Bitmap/aux_Bitmap.hpp>

// PC-owned embedded asset: 128x128 P8_URGB_8888, unlike the sibling 64x64 image.
DATA(0x00338328)
unsigned int AuxBitmapDefaultClut[256] = {
#include "../../../../build/gen/retail-assets/DefaultBMP_clut.inc"
};
DATA(0x00338728)
unsigned char AuxBitmapDefaultData[16384] = {
#include "../../../../build/gen/retail-assets/DefaultBMP_pixels.inc"
};

RVA(0x00147120, 0x28)
void auxbmp_SetupDefault(xbitmap& Bitmap) {
    Bitmap.Setup(
        xbitmap::FMT_P8_URGB_8888,
        128,
        128,
        0,
        AuxBitmapDefaultData,
        0,
        reinterpret_cast<unsigned char*>(AuxBitmapDefaultClut),
        -1,
        0
    );
}
