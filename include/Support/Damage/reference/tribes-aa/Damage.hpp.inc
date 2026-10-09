//=========================================================================
//
// DAMAGE.HPP
//
//=========================================================================

#ifndef DAMAGE_HPP
#define DAMAGE_HPP

//==============================================================================
// INCLUDES
//==============================================================================

#include <xCore/x_files/x_files.hpp>


//==============================================================================
// FORWARD REFERENCES
//==============================================================================
class texture ;



//==============================================================================
// CLASSES
//==============================================================================

class damage
{
// Defines
private:
    enum defines
    {
        PALETTE_COUNT = 4        // Number of palettes
    } ;

// Data
private:

    // Damage texture + cluts
    texture*        m_Texture ;                     // Damage texture
    s32             m_ClutHandle[PALETTE_COUNT] ;   // Palette clut handles
    byte*           m_Clut ;                        // Palettes of 256 colors- one for each damage level


// Functions
public:
                    damage              ( void ) ;
                   ~damage              ( void ) ;

    s32             GetPaletteIndex     ( xcolor Color ) ;
    void            PrepareTexture      ( texture* DamageTexture ) ;
                   
    void            Init                ( texture* DamageTexture ) ;
    void            Kill                ( void ) ;
                    
    void            GetTexture          ( f32 Health, texture*& Texture, s32& ClutHandle ) ;

};


//=========================================================================
#endif
//=========================================================================
