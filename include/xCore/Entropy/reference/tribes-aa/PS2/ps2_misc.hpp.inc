//=========================================================================
//
// PS2_MISC.HPP
//
//=========================================================================

#ifndef PS2_MISC_HPP
#define PS2_MISC_HPP

//=========================================================================

#include "eekernel.h"
#include "stdlib.h"
#include "stdio.h"
#include "eeregs.h"
#include "libgraph.h"
#include "libdma.h"
#include "libpkt.h"
#include "sifdev.h"
#include "libdev.h"
#include "sifrpc.h"
#include "libpad.h"
#include "libcdvd.h"
#include "libvu0.h"

#include <xCore/Entropy/PS2/ps2_dlist.hpp>
#include <xCore/Entropy/PS2/ps2_dma.hpp>
#include <xCore/Entropy/PS2/ps2_gsreg.hpp>
#include <xCore/Entropy/PS2/ps2_vifgif.hpp>
#include <xCore/Entropy/PS2/ps2_framebuf.hpp>
#include "ps2_dmaman.hpp"
#include <xCore/Entropy/PS2/ps2_text.hpp>
#include <xCore/Entropy/PS2/ps2_font.hpp>
#include <xCore/Entropy/PS2/ps2_vram.hpp>




//=========================================================================
// GS CONTEXT SWITCHING
//=========================================================================

void    eng_PushGSContext           ( s32 Context );
void    eng_PopGSContext            ( void );
s32     eng_GetGSContext            ( void );
s32     eng_GetFrameIndex           ( void );
u64     eng_GetFRAMEReg             ( void );
u64     eng_GetFRAMEReg             ( s32 Buffer ) ; // 0=back buffer, 1=front buffer
u32     eng_GetFrameBufferAddr      ( s32 Buffer ) ;

//=========================================================================
// MICROCODE
//=========================================================================

s32     eng_RegisterMicrocode   ( char* Name, void* CodeDMAAddr, s32 NBytes );
void    eng_ActivateMicrocode   ( s32 ID );
s32     eng_GetActiveMicrocode  ( void );

//=========================================================================
// MAIN DISPLAY LIST
//=========================================================================

extern dlist DLIST;
#define DLStruct(x)     ( (x*)DLIST.Alloc(sizeof(x)) )
#define DLAlloc(x)      DLIST.Alloc(x)
#define DLGetAddr()     DLIST.GetAddr()
#define DLSetAddr(x)    DLIST.SetAddr(x)
#define DLAlign16()     DLIST.Align16()

void    eng_PushDList   ( const dlist& DList );
void    eng_PopDList    ( void );

//=========================================================================
// PS2 COLOR 
//=========================================================================
struct ps2color
{

    u8  R, G, B, A;     // Values are in [ 0, 255 ].

                ps2color            ( void )                            { }
                ps2color            ( const xcolor& C )                 { R=C.R; G=C.G; B=C.B; A=C.A; }
                ps2color            ( const ps2color& C )               { R=C.R; G=C.G; B=C.B; A=C.A; }
                ps2color            ( u8 aR, u8 aG, u8 aB, u8 aA = 255 ){ R=aR; B=aB; G=aG; A=aA; }
                ps2color            ( u32 ABGR )                        { Set(ABGR); }
        void    Set                 ( u8 aR, u8 aG, u8 aB, u8 aA = 255 ){ R=aR; B=aB; G=aG; A=aA; }
const   ps2color& operator =        ( const ps2color& C )               { R=C.R; G=C.G; B=C.B; A=C.A; return(*this); }
const   ps2color& operator =        ( u32 ABGR )                        { Set(ABGR); return(*this); }

                operator const u32  ( void ) const                      { return( *((u32*)this) ); }

        void    Set                 ( u32 ABGR )                        {
                                                                          A = (u8)((ABGR & 0xFF000000) >> 24);
                                                                          B = (u8)((ABGR & 0x00FF0000) >> 16);
                                                                          G = (u8)((ABGR & 0x0000FF00) >>  8);
                                                                          R = (u8)((ABGR & 0x000000FF) >>  0);
                                                                        }
};                                                                      

//=========================================================================
// Magical macro which defines the entry point of the app. Make sure that 
// the use has the entry point: void AppMain( s32 argc, char* argv[] ) 
// define somewhere.
//=========================================================================
void ps2eng_Begin( s32 argc, char* argv[] );
void ps2eng_End( void );

#define AppMain AppMain( s32 argc, char* argv[] );                          \
int main( s32 argc, char* argv[] )                                          \
{                                                                           \
    ps2eng_Begin( argc, argv );                                             \
    AppMain( argc, argv );                                                  \
    ps2eng_End();                                                           \
    return 0;                                                               \
}                                                                           \
void AppMain                                                                \


#endif
