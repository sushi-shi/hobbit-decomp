#ifndef HOBBIT_X_BITMAP_PRIVATE_HPP
#define HOBBIT_X_BITMAP_PRIVATE_HPP

#include <xCore/x_files/x_color.hpp>

// Shared declarations for the original bitmap conversion, quantizer and
// color-map translation units; gathered from the sibling conversion prelude.
void quant_Begin();
void quant_SetPixels(const xcolor* colors, int count);
void quant_End(xcolor* palette, int count, int useAlpha);
void cmap_Begin(const xcolor* palette, int count, int useAlpha);
int cmap_GetIndex(xcolor color);
void cmap_End();

#endif
