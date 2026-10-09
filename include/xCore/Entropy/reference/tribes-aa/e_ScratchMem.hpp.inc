//==============================================================================
//  
//  e_ScratchMem.hpp
//
//==============================================================================

#ifndef E_SCRATCHMEM_HPP
#define E_SCRATCHMEM_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/x_files/x_types.hpp>

//==============================================================================
//  FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
//  Public functions
//------------------------------------------------------------------------------

void    smem_ChangeSize         ( s32 NBytes );
byte*   smem_BufferAlloc        ( s32 NBytes ); // Can return NULL
byte*   smem_StackAlloc         ( s32 NBytes ); // Can return NULL
void    smem_StackPushMarker    ( void );
void    smem_StackPopToMarker   ( void );

//------------------------------------------------------------------------------
//  Private functions
//------------------------------------------------------------------------------

void    smem_Init               ( s32 NBytes );
void    smem_Kill               ( void );
void    smem_Toggle             ( void );


//==============================================================================
//==============================================================================
#ifdef X_DEBUG
#define smem_BufferAlloc(S)     smem_dfunc_BufferAlloc( (S), __FILE__, __LINE__ )
#define smem_StackAlloc(S)      smem_dfunc_StackAlloc( (S), __FILE__, __LINE__ )
#define smem_StackPushMarker()  smem_dfunc_StackPushMarker( __FILE__, __LINE__ )
#define smem_StackPopToMarker() smem_dfunc_StackPopToMarker( __FILE__, __LINE__ )
byte*   smem_dfunc_BufferAlloc        ( s32 NBytes, const char* pFileName, s32 Line );
byte*   smem_dfunc_StackAlloc         ( s32 NBytes, const char* pFileName, s32 Line );
void    smem_dfunc_StackPushMarker    ( const char* pFileName, s32 Line );
void    smem_dfunc_StackPopToMarker   ( const char* pFileName, s32 Line );
#else
#define smem_BufferAlloc(S)     smem_rfunc_BufferAlloc( (S) )
#define smem_StackAlloc(S)      smem_rfunc_StackAlloc( (S) )
#define smem_StackPushMarker()  smem_rfunc_StackPushMarker()
#define smem_StackPopToMarker() smem_rfunc_StackPopToMarker()
byte*   smem_rfunc_BufferAlloc        ( s32 NBytes );
byte*   smem_rfunc_StackAlloc         ( s32 NBytes );
void    smem_rfunc_StackPushMarker    ( void );
void    smem_rfunc_StackPopToMarker   ( void );
#endif

// These functions can be used to make sure you have a valid scratch allocation
s32     smem_GetActiveID        ( void ) ;
byte*   smem_GetActiveStartAddr ( void ) ;
byte*   smem_GetActiveEndAddr   ( void ) ;
void    smem_Validate           ( void ) ;

//==============================================================================
#endif // E_SCRATCHMEM_HPP
//==============================================================================
