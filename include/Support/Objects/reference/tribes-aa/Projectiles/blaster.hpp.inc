//==============================================================================
//
//  Blaster.hpp
//
//==============================================================================

#ifndef BLASTER_HPP
#define BLASTER_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Linear.hpp"

//==============================================================================
//  DEFINES
//==============================================================================

#define MAX_LEGS            10          // max # of legs to draw during ricochet
#define GLOWLIFE            0.1f        // how long the afterimage lasts

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     BlasterCreator( void );

//==============================================================================
//  TYPES
//==============================================================================

class blaster : public linear
{

private:

    struct blast_leg
    {
        vector3     m_Pos;
        f32         m_Life;
        xbool       m_Bounce;
    };

    blast_leg   m_Legs[ MAX_LEGS ];
    s32         m_CurLeg;
    s32         m_Bounces;
    s32         m_LastStartLeg;
    xbool       m_HasImpacted;
    xbool       m_OriginalStart;

    void        AddLeg          ( const vector3& Vert, xbool Bounce = FALSE );
        
public:

    // allows us to age our trail
    object::update_code         OnAdvanceLogic( f32 DeltaTime );

    void        Setup           ( void );

    void        OnAcceptInit    ( const bitstream& BitStream );
    void        OnProvideInit   (       bitstream& BitStream );
    void        OnAdd           ( void );

    void        Initialize      ( const matrix4&   L2W, 
                                        u32        TeamBits,
                                        object::id OriginID );

    void        Render          ( void );
    update_code Impact          ( vector3& Point, vector3& Normal );
    void        StopMoving      ( vector3& Point );

static void     Init            ( void );
static void     Kill            ( void );
static void     BeginRender     ( void );
static void     EndRender       ( void );

protected:

    xbool       m_HasExploded;
};

//==============================================================================
#endif // BLASTER_HPP
//==============================================================================
