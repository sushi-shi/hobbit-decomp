//==============================================================================
//
//  Shrike.hpp
//  Class for the Shrike vehicle
//
//==============================================================================

#ifndef SHRIKE_HPP
#define SHRIKE_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "gndeffect.hpp"

//==============================================================================
//  FUNCTIONS
//==============================================================================

void        Shrike_InitFX( void );
object*     ShrikeCreator( void );

//==============================================================================
//  TYPES
//==============================================================================

class shrike: public gnd_effect
{
    update_code Impact          ( vector3& Pos, vector3& Norm );

public:

    void        OnAdd           ( void );
    void        Initialize      ( const vector3& Pos, radian InitHdg, u32 TeamBits );
    void        SetupSeat       ( s32 Index, seat& Seat ) ;
    void        OnAcceptInit    ( const bitstream& BitStream );
    void        Render          ( void );
    void        RenderParticles ( void );
    
    update_code OnAdvanceLogic  ( f32 DeltaTime );
    void        OnRemove        ( void );
    void        ApplyMove       ( player_object* pPlayer, s32 SeatIndex, player_object::move& Move ) ;
    void        Get3rdPersonView( s32 Seat, vector3& Pos, radian3& Rot );
    f32         GetPainRadius   ( void ) const;
    vector3     GetBlendFirePos ( s32 Index = 0 );

private:
    s32                         m_AudioEng;
    s32                         m_AudioAfterBurn;
    s32                         m_FireSide;             // 0 = left, 1 = right, alternate
};


#endif