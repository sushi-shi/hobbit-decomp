//==============================================================================
//
//  GravCycle.hpp
//  Class for the GravCycle vehicle
//
//==============================================================================

#ifndef GRAVCYCLE_HPP
#define GRAVCYCLE_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "gndeffect.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

//==============================================================================
//  FUNCTIONS
//==============================================================================

void        GravCycle_InitFX( void );
object*     GravCycleCreator( void );

//==============================================================================
//  TYPES
//==============================================================================

class gravcycle: public gnd_effect
{

public:
    update_code Impact          ( vector3& Pos, vector3& Norm );

    void        OnAdd           ( void );
    void        ApplyMove       ( player_object* pPlayer, s32 SeatIndex, player_object::move& Move ) ;

    void        Initialize      ( const vector3& Pos, radian InitHdg, u32 TeamBits );
    void        SetupSeat       ( s32 Index, seat& Seat ) ;
    void        OnAcceptInit    ( const bitstream& BitStream );
    void        Render          ( void );
    void        RenderParticles ( void );
    
    update_code OnAdvanceLogic  ( f32 DeltaTime );
    void        OnRemove        ( void );
    f32         GetPainRadius   ( void ) const;
    xbool       IsShielded      ( void ) const { return( FALSE ); }

private:
    s32                         m_AudioEng;
    s32                         m_AudioAfterBurn;
};

//==============================================================================
#endif
//==============================================================================
