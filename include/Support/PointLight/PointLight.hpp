//=========================================================================
//
//  POINTLIGHT.HPP
//
//=========================================================================

#ifndef POINTLIGHT_HPP
#define POINTLIGHT_HPP

//=========================================================================

#include <xCore/x_files/x_math.hpp>
#include <xCore/x_files/x_color.hpp>

//=========================================================================

typedef u32 fcache_key;

struct fcache_face
{
};

inline void fcache_ConstructKey ( fcache_key& Key, s32  ObjectSlot, s32  PrivateInfo ) { };
inline void fcache_DestructKey  ( fcache_key  Key, s32& ObjectSlot, s32& PrivateInfo ) { };

//=========================================================================

inline void ptlight_Init           ( void ) { };
inline void ptlight_Kill           ( void ) { };

// For fire and forget lights
inline void ptlight_Create         ( const vector3& Pos,
                                    f32      Radius,
                                    xcolor   Color,
                                    f32      RampUpTime,
                                    f32      HoldTime,
                                    f32      RampDownTime,
                                    xbool    IncludeBldgs = TRUE ) { };
                            
// For dynamically updated lights
inline s32  ptlight_Create         ( const vector3& Pos, f32 Radius, xcolor Color, xbool IncludeBldgs = TRUE ) { return( 1 ); };
inline void ptlight_Destroy        ( s32 Handle ) { };
inline void ptlight_SetPosition    ( s32 Handle, const vector3& Pos ) { };
inline void ptlight_SetRadius      ( s32 Handle, f32 Radius ) { };
inline void ptlight_SetColor       ( s32 Handle, xcolor Color ) { };

inline void ptlight_Render         ( void ) { };
inline void ptlight_AdvanceLogic   ( f32 DeltaSec ) { };

inline void AddBulletHole          ( const vector3& Pos, const vector3& Normal ) { };
inline void shadow_Render          ( const vector3& SunDir ) { };
inline void AddDummy               ( const vector3& Pos ) { };

//=========================================================================
#endif
//=========================================================================
