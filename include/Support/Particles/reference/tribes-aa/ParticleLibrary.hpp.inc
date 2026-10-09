//==============================================================================
//
//  ParticleLibrary.hpp
//
//==============================================================================

#ifndef PARTICLELIBRARY_HPP
#define PARTICLELIBRARY_HPP

//=============================================================================
//  Includes
//=============================================================================

#include <xCore/x_files/x_files.hpp>
#include "ParticleDefs.hpp"

//=============================================================================
//  Particle library
//=============================================================================

class pfx_library
{
public:
                            pfx_library         ( void );
                           ~pfx_library         ( void );

    xbool                   Create              ( const char* pPathName );
    xbool                   CreateFromASCII     ( const char* pPathName );
    xbool                   CreateFromBinary    ( const char* pPathName );

    void                    Kill                ( void );

    pfx_effect_def*         GetEffectDef        ( s32 Index ) const;
    pfx_emitter_def*        GetEmitterDef       ( s32 Index ) const;
    pfx_particle_def*       GetParticleDef      ( s32 Index ) const;
    s16                     GetIndex            ( s32 Index ) const;
    pfx_key*                GetKey              ( s32 Index ) const;

protected:

    xbool                   m_Created;                  // TRUE when created

    byte*                   m_pMemory;                  // Block of memory holding all data

    s32                     m_nTextures;                // Number of Bitmaps
    s32                     m_nParticleDefs;            // Number of particle defs
    s32                     m_nEmitterDefs;             // Number of emitter defs
    s32                     m_nEffectDefs;              // Number of effect defs
    s32                     m_nIndices;                 // Number of indices
    s32                     m_nKeys;                    // Number of keys

    xbitmap*                m_pTextures;                // Array of Bitmaps
    pfx_particle_def*       m_pParticleDefs;            // Array of Particle defs
    pfx_emitter_def*        m_pEmitterDefs;             // Array of Emitter defs
    pfx_effect_def*         m_pEffectDefs;              // Array of Effect defs
    s16*                    m_pIndices;                 // Array of Indices
    pfx_key*                m_pKeys;                    // Array of Keys
};

//=============================================================================
#endif // PARTICLELIBRARY_HPP
//=============================================================================
