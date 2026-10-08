// Original Area51 sibling source import; no Hobbit PC address claim. See docs/imports/entropy-alternate-platforms.md.
//=========================================================================
//
// ps2_font.hpp
//
//=========================================================================

#ifndef PS2_FONT_HPP
#define PS2_FONT_HPP

//=========================================================================

#include <xCore/Entropy/Entropy.hpp>

//=========================================================================

void font_Init       ( void );
void font_Kill       ( void );
void font_BeginRender( void );
void font_Render     ( char* Str, s32 NChars, xcolor Color, s32 PixelX, s32 PixelY );
void font_EndRender  ( void );

//=========================================================================

#endif
