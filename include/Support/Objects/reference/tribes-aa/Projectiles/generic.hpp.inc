//==============================================================================
//
//  GenericShot.hpp
//
//==============================================================================

#ifndef GENERIC_SHOT_HPP
#define GENERIC_SHOT_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include "Linear.hpp"

//==============================================================================
//  FUNCTIONS
//==============================================================================

object*     GenericShotCreator( void );

//==============================================================================
//  TYPES
//==============================================================================

class generic_shot : public linear
{

public:

        enum kind
        {
            AA_TURRET,
            PLASMA_TURRET,
            CLAMP_TURRET,
            LAST
        };

        void        Setup               ( void );
                                        
        void        OnAcceptInit        ( const bitstream& BitStream );
        void        OnProvideInit       (       bitstream& BitStream );

        void        OnAdd               ( void );

        kind        GetKind             ( void ) { return m_Kind; }
                                        
        void        Initialize          ( const matrix4&   L2W, 
                                                u32        TeamBits,
                                                kind       Kind,
                                                object::id OriginID,
                                                object::id ExcludeID = obj_mgr::NullID );

        void        Render              ( void );
        update_code Impact              ( vector3& Point, vector3& Normal );
        update_code OnAdvanceLogic      ( f32 DeltaTime );

        object::id  m_TargetID;
                                        
static  void        BeginRender         ( kind Kind );
static  void        EndRender           ( void );
        void        RenderParticles     ( void );
    
        f32         GetMuzzleSpeed      ( void );
        f32         GetMaxAge           ( void );

protected:

        kind        m_Kind;
        radian      m_Rot;
        pfx_effect  m_SmokeEffect;
        xbool       m_HasExpired;
        
        s32         m_Texture1;         // VRAM texture ID
};

//==============================================================================
#endif // GENERIC_SHOT_HPP
//==============================================================================
