//==============================================================================
//  Fog.hpp
//==============================================================================

#ifndef FOG_HPP
#define FOG_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/x_files/x_bitmap.hpp>
#include <xCore/x_files/x_math.hpp>
#include "Tokenizer/Tokenizer.hpp"

//==============================================================================
//  TYPES
//==============================================================================

class fog
{

public:

                    fog                 ( void );
                   ~fog                 ( void );
                    
        void        Load                ( token_stream& TOK );
        void        Load                ( const char* );
        void        Unload              ( void );
                    
        void        SaveSettings        ( const char* pFileName );
        void        LoadSettings        ( const char* pFileName );
                    
        void        SetEyePos           ( const vector3& Pos );
                    
        void        SetMinMaxY          ( f32 MinY, f32 MaxY );
                    
        xcolor      ComputeFog          ( f32 Dist, f32 DeltaY ) const;
        xcolor      ComputeFog          ( const vector3& Pos ) const;
        xcolor      ComputeParticleFog  ( const vector3& Pos ) const;
                    
        void        ComputeUVs          ( f32 Dist, f32 DeltaY, f32& U, f32& V ) const;
        void        ComputeUVs          ( const vector3& Pos, f32& U, f32& V ) const;
                    
        void        RenderFogMap        ( void ) const;
                    
        void        GetFogConst         ( f32& Dist0, f32& Dist1, f32& DeltaY0, f32& DeltaY1 ) const;
                    
        f32         GetVisDistance      ( void ) const;
        
const   xbitmap&    GetBMP              ( void ) const;
        
        xcolor      GetHazeColor        ( void ) const;
        void        GetHazeAngles       ( radian& StartAngle, radian& FinishAngle ) const;
};

//==============================================================================
#endif
//==============================================================================
