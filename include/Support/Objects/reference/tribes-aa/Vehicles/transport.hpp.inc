//==============================================================================
//
//  Transport.hpp
//  Class for the Transport vehicle
//
//==============================================================================

#ifndef TRANSPORT_HPP
#define TRANSPORT_HPP

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

void        Transport_InitFX( void );
object*     TransportCreator( void );

//==============================================================================
//  TYPES
//==============================================================================

class transport: public gnd_effect
{
    update_code Impact          ( vector3& Pos, vector3& Norm );

public:

    void        OnAdd           ( void );
    void        ApplyMove       ( player_object* pPlayer, s32 SeatIndex, player_object::move& Move ) ;

    void        Initialize      ( const vector3& Pos, radian InitHdg, u32 TeamBits );
    void        SetupSeat       ( s32 Index, seat& Seat ) ;
    void        OnAcceptInit    ( const bitstream& BitStream );
    void        OnProvideInit   (       bitstream& BitStream );
    void        Render          ( void );
    void        RenderParticles ( void );
    
    update_code OnAdvanceLogic  ( f32 DeltaTime );
    void        UpdateInstances ( void ) ;
    void        OnRemove        ( void );
    f32         GetPainRadius   ( void ) const;
    void        UpdateAssets    ( void );

    void        SetTeamBits     ( u32 TeamBits );

private:
    object::id                  m_TurretID;

    s32                         m_AudioEng;
    s32                         m_AudioAfterBurn;
};

//==============================================================================
#endif
//==============================================================================
