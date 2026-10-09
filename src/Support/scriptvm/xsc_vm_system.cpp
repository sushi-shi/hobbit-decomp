//==============================================================================
//
//  xsc_vm_system.cpp
//
//==============================================================================

#include <Support/scriptvm/xsc_vm_core.hpp>
#include <rva.h>

//==============================================================================
//  Defines
//==============================================================================

//==============================================================================
//  Adapter functions
//==============================================================================

RVA(0x2463f0, 0x18) static void xsc_fabs( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_abs( v );
}

RVA(0x246410, 0xf) static void xsc_iabs( u8* pArgs )
{
    s32* pRet = (s32*)pArgs;
    s32  v    = vmarg_s32( pArgs );
    *pRet = x_abs( v );
}

RVA(0x246420, 0xf) static void xsc_sqr( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_sqr( v );
}

RVA(0x246430, 0xb) static void xsc_sqrt( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_sqrt( v );
}

RVA(0x246440, 0x11) static void xsc_1sqrt( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_1sqrt( v );
}

RVA(0x246460, 0x1c) static void xsc_floor( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_floor( v );
}

RVA(0x246480, 0x1c) static void xsc_ceil( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_ceil( v );
}

RVA(0x2464a0, 0xf) static void xsc_log( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_log( v );
}

RVA(0x2464b0, 0x15) static void xsc_log2( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_log2( v );
}

RVA(0x2464d0, 0xf) static void xsc_log10( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_log10( v );
}

RVA(0x2464e0, 0x1f) static void xsc_exp( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_exp( v );
}

RVA(0x246500, 0x13) static void xsc_pow( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v1   = vmarg_f32( pArgs );
    f32  v2   = vmarg_f32( pArgs );
    *pRet = x_pow( v1, v2 );
}

RVA(0x246520, 0x2a) static void xsc_fmod( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v1   = vmarg_f32( pArgs );
    f32  v2   = vmarg_f32( pArgs );
    *pRet = x_fmod( v1, v2 );
}

RVA(0x246550, 0x3b) static void xsc_lpr( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v1   = vmarg_f32( pArgs );
    f32  v2   = vmarg_f32( pArgs );
    *pRet = x_lpr( v1, v2 );
}

RVA(0x246590, 0x52) static void xsc_round( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v1   = vmarg_f32( pArgs );
    f32  v2   = vmarg_f32( pArgs );
    *pRet = x_round( v1, v2 );
}

RVA(0x2465f0, 0x20) static void xsc_ldexp( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v1   = vmarg_f32( pArgs );
    s32  v2   = vmarg_s32( pArgs );
    *pRet = x_ldexp( v1, v2 );
}

RVA(0x246610, 0x1c) static void xsc_sin( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_sin( v );
}

RVA(0x246630, 0x1c) static void xsc_cos( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_cos( v );
}

RVA(0x246650, 0xd) static void xsc_tan( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_tan( v );
}

RVA(0x246660, 0x10) static void xsc_asin( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_asin( v );
}

RVA(0x246670, 0x10) static void xsc_acos( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_acos( v );
}

RVA(0x246680, 0xd) static void xsc_atan( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_atan( v );
}

RVA(0x246690, 0x2a) static void xsc_atan2( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v1   = vmarg_f32( pArgs );
    f32  v2   = vmarg_f32( pArgs );
    *pRet = x_atan2( v1, v2 );
}

RVA(0x2466c0, 0x1e) static void xsc_sincos( u8* pArgs )
{
    f32  v1   = vmarg_f32( pArgs );
    f32* v2   = (f32*)vmarg_ptr( pArgs );
    f32* v3   = (f32*)vmarg_ptr( pArgs );
    x_sincos( v1, *v2, *v3 );
}

RVA(0x2466e0, 0x7d) static void xsc_ModAngle( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_ModAngle( v );
}

RVA(0x246760, 0x8d) static void xsc_ModAngle2( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v    = vmarg_f32( pArgs );
    *pRet = x_ModAngle2( v );
}

RVA(0x2467f0, 0x8d) static void xsc_MinAngleDiff( u8* pArgs )
{
    f32* pRet = (f32*)pArgs;
    f32  v1   = vmarg_f32( pArgs );
    f32  v2   = vmarg_f32( pArgs );
    *pRet = x_MinAngleDiff( v1, v2 );
}


//==============================================================================
//  RegisterSystemMethods
//==============================================================================

void xsc_vm_core::RegisterSystemMethods( void )
{
    RegisterNativeMethod( "system", "x_abs",          "FF",    xsc_fabs         );
    RegisterNativeMethod( "system", "x_abs",          "II",    xsc_iabs         );
    RegisterNativeMethod( "system", "x_sqr",          "FF",    xsc_sqr          );
    RegisterNativeMethod( "system", "x_sqrt",         "FF",    xsc_sqrt         );
    RegisterNativeMethod( "system", "x_1sqrt",        "FF",    xsc_1sqrt        );
    RegisterNativeMethod( "system", "x_floor",        "FF",    xsc_floor        );
    RegisterNativeMethod( "system", "x_ceil",         "FF",    xsc_ceil         );
    RegisterNativeMethod( "system", "x_log",          "FF",    xsc_log          );
    RegisterNativeMethod( "system", "x_log2",         "FF",    xsc_log2         );
    RegisterNativeMethod( "system", "x_log10",        "FF",    xsc_log10        );
    RegisterNativeMethod( "system", "x_exp",          "FF",    xsc_exp          );
    RegisterNativeMethod( "system", "x_pow",          "FFF",   xsc_pow          );
    RegisterNativeMethod( "system", "x_fmod",         "FFF",   xsc_fmod         );
    RegisterNativeMethod( "system", "x_lpr",          "FFF",   xsc_lpr          );
    RegisterNativeMethod( "system", "x_round",        "FFF",   xsc_round        );
    RegisterNativeMethod( "system", "x_ldexp",        "FFI",   xsc_ldexp        );

    RegisterNativeMethod( "system", "x_sin",          "FF",    xsc_sin          );
    RegisterNativeMethod( "system", "x_cos",          "FF",    xsc_cos          );
    RegisterNativeMethod( "system", "x_tan",          "FF",    xsc_tan          );
    RegisterNativeMethod( "system", "x_asin",         "FF",    xsc_asin         );
    RegisterNativeMethod( "system", "x_acos",         "FF",    xsc_acos         );
    RegisterNativeMethod( "system", "x_atan",         "FF",    xsc_atan         );
    RegisterNativeMethod( "system", "x_atan2",        "FFF",   xsc_atan2        );

    RegisterNativeMethod( "system", "x_sincos",       "V&F&F", xsc_sincos       );
    RegisterNativeMethod( "system", "x_ModAngle",     "FF",    xsc_ModAngle     );
    RegisterNativeMethod( "system", "x_ModAngle2",    "FF",    xsc_ModAngle2    );
    RegisterNativeMethod( "system", "x_MinAngleDiff", "FFF",   xsc_MinAngleDiff );
}

//==============================================================================
