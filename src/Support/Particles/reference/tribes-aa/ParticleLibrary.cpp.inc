#include <xCore/Entropy/Entropy.hpp>
#include "ParticleLibrary.hpp"
#include "tokenizer/tokenizer.hpp"
#include <xCore/Auxiliary/Bitmap/aux_Bitmap.hpp>
#include "labelsets/labelparse.hpp"
#include <xCore/Entropy/e_ScratchMem.hpp>

//=============================================================================
//  Binary File Format
//=============================================================================

/*
#define     VERSION     1

struct pfx_lib_header
{
    s32     Version;
    s32     nTextures;
    s32     nParticleDefs;
    s32     nEmitterDefs;
    s32     nEffectDefs;
    s32     nEffectEmitters;
};

struct pfx_lib_texture
{
    s32     StringIndex;
    s32     nImages;
};
*/

//=============================================================================
//  Symbol Table Helpers
//=============================================================================

struct texture_record
{
    xstring PathName;
    s32     nImages;
};

struct symbol
{
    xstring Name;
    s32     ID;
};

//typedef xarray<symbol*> symbol_table;

class symbol_table : public xarray<symbol*>
{
public:
    ~symbol_table( void ) 
    {   
        while( m_Count > 0 )
        {
            m_Count--;
            delete m_pData[ m_Count ];
        }
    };
};

void SymTabClear( symbol_table& Table )
{
    s32 i;

    for( i=0 ; i<Table.GetCount() ; i++ )
    {
        delete Table[i];
        Table.Delete( i );
    }
}

s32 SymTabFind( symbol_table& Table, xstring& String )
{
    s32 Index = -1;
    s32 i;

    for( i=0 ; i<Table.GetCount() ; i++ )
    {
        symbol* pSymbol = Table[i];
        ASSERT( pSymbol );
        if( pSymbol->Name == String )
        {
            Index = pSymbol->ID;
            break;
        }
    }

    return Index;
}

s32 SymTabAdd( symbol_table& Table, xstring& String, s32 ID )
{
    s32 Index = -1;

    Index = SymTabFind( Table, String );

    x_MemSanity();

    ASSERT( Index == -1 );
    {
        symbol* pSymbol = new symbol;
        ASSERT( pSymbol );
        pSymbol->Name = String;
        pSymbol->ID    = ID;
        Index          = Table.GetCount();
        Table.Append() = pSymbol;
    }

    return Index;
}

//=============================================================================
//  Load Texture
//=============================================================================

static void LoadTexture( xbitmap& BMP, const char* pFileName )
{
/*
    char DRIVE[64];
    char DIR[256];
    char FNAME[64];
    char EXT[16];
    char XBMPath[256];

    //x_DebugMsg("TEXTURE LOAD: %s\n",pFileName);
    x_splitpath( pFileName,DRIVE,DIR,FNAME,EXT);
    x_makepath( XBMPath,DRIVE,DIR,FNAME,".xbmp");

//    if( !BMP.Load(XBMPath) )
    {
        xbool Result = auxbmp_LoadNative( BMP, pFileName );

        if( !Result )
        {
            //x_DebugMsg("Using default for bitmap %1s\n",pFileName);
        }

        //printf("BUILDING MIPS\n");
//        BMP.BuildMips();
//        BMP.Save( XBMPath );
    }

    vram_Register( BMP );
*/
}

//=============================================================================
//  Particle library
//=============================================================================

pfx_library::pfx_library( void )
{
    m_pMemory = NULL;
    m_Created = FALSE;
}

//=============================================================================

pfx_library::~pfx_library( void )
{
}

//=============================================================================

xbool pfx_library::Create( const char* pPathName )
{
    ASSERT( !m_Created );

    // Try loading binary first
    if( !CreateFromBinary( pPathName ) )
    {
        CreateFromASCII( pPathName );
    }

    // Return sucess code
    return m_Created;
}

//=============================================================================

xbool pfx_library::CreateFromBinary( const char* pPathName )
{
    (void)pPathName;
    return FALSE;
}

//=============================================================================

xbool pfx_library::CreateFromASCII( const char* pPathName )
{
    token_stream                ts;
    symbol_table                SymTabTextures;
    symbol_table                SymTabParticles;
    symbol_table                SymTabEmitters;
    s32                         i;
    s32                         nTextures;
    xarray<texture_record>      Textures;
    xarray<pfx_particle_def>    ParticleDefs;
    xarray<pfx_emitter_def>     EmitterDefs;
    xarray<pfx_effect_def>      EffectDefs;
    xarray<s16>                 Indices;
    xarray<pfx_key>             Keys;
    labels                      Labels;

    // Open token stream
    if( ts.OpenFile( xfs("%s.txt", pPathName )) )
    {
        // first, look for shape file
        if ( ts.Find( "#include", TRUE ) != FALSE )
        {
            ts.ReadString();
            Labels.LoadLabelSet( ts.String() );
        }
        else
            ts.Rewind();

        // Set capacity of symbol tables
        SymTabTextures.SetCapacity( 100 );
        SymTabParticles.SetCapacity( 100 );
        SymTabEmitters.SetCapacity( 100 );

        //=====================================
        //  Read Textures
        //=====================================

        ts.Read();
        ASSERT( ts.Delimiter() == '{' );
        ts.Read();
        nTextures = 0;
        while( ts.Delimiter() != '}' )
        {
            xstring ID       = ts.String();
            xstring PathName = ts.ReadString();
            s32     nImages  = ts.ReadInt();

            // Add texture to symbol table
            SymTabAdd( SymTabTextures, ID, nTextures );

            texture_record& t = Textures.Append();
            t.PathName = PathName;
            t.nImages  = nImages;
            nTextures += nImages;

            // Read next token
            ts.Read();
        }

        //=====================================
        //  Load Textures
        //=====================================
        /*
        m_nTextures = nTextures;                        
        m_pTextures = new xbitmap[m_nTextures];
        ASSERT( m_pTextures );
        Index = 0;
        for( i=0 ; i<Textures.GetCount() ; i++ )
        {
            for( s32 iImage = 0 ; iImage<Textures[i].nImages ; iImage++ )
            {
                if( Textures[i].nImages > 1 )
                    LoadTexture( m_pTextures[Index], xfs( "%s%02d.tga", (const char*)Textures[i].PathName, iImage ) );
                else
                    LoadTexture( m_pTextures[Index], xfs( "%s", (const char*)Textures[i].PathName ) );
                Index++;
            }
        }
        */

        //=====================================
        //  Read particle defs
        //=====================================

        ts.Read();
        ASSERT( ts.Delimiter() == '{' );
        ts.Read();
        while( ts.Delimiter() != '}' )
        {
            pfx_particle_def& pd    = ParticleDefs.Append();

            xstring ID               = ts.String();
            pd.Flags                 = ts.ReadHex();
            pd.DragCoefficient       = ts.ReadFloat();
            pd.GravityCoefficient    = ts.ReadFloat();
            pd.InheritVelCoefficient = ts.ReadFloat();
            pd.Life                  = ts.ReadFloat();
            pd.LifeVariance          = ts.ReadFloat();
            pd.Rotation              = RADIAN(ts.ReadFloat());
            pd.RotationVariance      = RADIAN(ts.ReadFloat());
            xstring TextureID        = ts.ReadSymbol();

            // Resolve Texture ID
            s32 iTexture = SymTabFind( SymTabTextures, TextureID );
            if( iTexture != -1 )
                pd.m_pTextures = &m_pTextures[iTexture];
            else
                pd.m_pTextures = &m_pTextures[0];

            // Add Particle to symbol table
            SymTabAdd( SymTabParticles, ID, ParticleDefs.GetCount()-1 );

            // Read Animation Keys
            pd.nKeys = 0;
            pd.iKey  = Keys.GetCount();
            ts.Read();
            ASSERT( ts.Delimiter() == '{' );
            ts.Read();
            while( ts.Delimiter() != '}' )
            {
                pfx_key& key = Keys.Append();

                key.S = ts.Float();
                key.I = ts.ReadFloat();
                key.R = ts.ReadInt()/2;
                key.G = ts.ReadInt()/2;
                key.B = ts.ReadInt()/2;
                key.A = ts.ReadInt()/2;

                // Increase count of keys
                pd.nKeys++;

                // Read next token
                ts.Read();
            }

            // Read next token
            ts.Read();
        }

        //=====================================
        //  Make permanent copy of ParticleDefs
        //=====================================

        m_nParticleDefs = ParticleDefs.GetCount();
        m_pParticleDefs = new pfx_particle_def[m_nParticleDefs];
        ASSERT( m_pParticleDefs );
        for( i=0 ; i<m_nParticleDefs ; i++ )
            m_pParticleDefs[i] = ParticleDefs[i];

        //=====================================
        //  Make permanent copy of Keys
        //=====================================

        m_nKeys = Keys.GetCount();
        m_pKeys = new pfx_key[m_nKeys];
        ASSERT( m_pKeys );
        for( i=0 ; i<m_nKeys ; i++ )
            m_pKeys[i] = Keys[i];

        //=====================================
        //  Read emitter defs
        //=====================================

        ts.Read();
        ASSERT( ts.Delimiter() == '{' );
        ts.Read();
        while( ts.Delimiter() != '}' )
        {
            pfx_emitter_def& ed = EmitterDefs.Append();

            xstring ID                  = ts.String();
            xstring Type                = ts.ReadSymbol();

            Type.MakeUpper();
            if ( Type == "SHAPE" )
            {
                // read the shape for this guy
                ts.ReadSymbol();
                // store the ID of the requested shape
                ed.ShapeID = Labels.FindLabel( ts.String() );
                ASSERT( ed.ShapeID != -1 );
            }

            ed.Flags                    = ts.ReadHex();
            ed.MaxParticles             = ts.ReadInt();
            ed.Life                     = ts.ReadFloat();
            ed.LifeVariance             = ts.ReadFloat();
            ed.EmitDelay                = ts.ReadFloat();
            ed.EmitDelayVariance        = ts.ReadFloat();
            ed.EmitInterval             = ts.ReadFloat();
            ed.EmitIntervalVariance     = ts.ReadFloat();
            ed.EmitOffset               = ts.ReadFloat();
            ed.EmitOffsetVariance       = ts.ReadFloat();
            ed.EmitVelocity             = ts.ReadFloat();
            ed.EmitVelocityVariance     = ts.ReadFloat();
            ed.EmitElevation            = RADIAN(ts.ReadFloat());
            ed.EmitElevationVariance    = RADIAN(ts.ReadFloat());
            ed.EmitAzimuth              = RADIAN(ts.ReadFloat());
            ed.EmitAzimuthVariance      = RADIAN(ts.ReadFloat());

            // Set Type
            ed.Type = pfx_emitter_def::UNDEFINED;
            if( Type == "POINT"          ) ed.Type = pfx_emitter_def::POINT;
            if( Type == "ORIENTED_POINT" ) ed.Type = pfx_emitter_def::ORIENTED_POINT;
            if( Type == "SHELL"          ) ed.Type = pfx_emitter_def::SHELL;
            if( Type == "SHOCKWAVE"      ) ed.Type = pfx_emitter_def::SHOCKWAVE;
            if( Type == "SHAPE"          ) ed.Type = pfx_emitter_def::SHAPE;
            ASSERT( ed.Type != pfx_emitter_def::UNDEFINED );

            // Add emitter to symbol table
            SymTabAdd( SymTabEmitters, ID, EmitterDefs.GetCount()-1 );

            // Read next token
            ts.Read();
        }

        //=====================================
        //  Make permanent copy of EmitterDefs
        //=====================================

        m_nEmitterDefs = EmitterDefs.GetCount();
        m_pEmitterDefs = new pfx_emitter_def[m_nEmitterDefs];
        ASSERT( m_pEmitterDefs );
        for( i=0 ; i<m_nEmitterDefs ; i++ )
            m_pEmitterDefs[i] = EmitterDefs[i];

        //=====================================
        //  Read effect defs
        //=====================================

        ts.Read();
        ASSERT( ts.Delimiter() == '{' );
        ts.Read();
        while( ts.Delimiter() != '}' )
        {
            pfx_effect_def& ed        = EffectDefs.Append();

            ed.Flags            = ts.ReadHex();
            ed.iEmitterDef      = Indices.GetCount();
            ed.nEmitters        = 0;

            ts.Read();
            ASSERT( ts.Delimiter() == '{' );
            ts.Read();
            while( ts.Delimiter() != '}' )
            {
                xstring EmitterID   = ts.String();
                xstring ParticleID  = ts.ReadSymbol();

                // Resolve EmitterDef ID
                s32 iEmitter = SymTabFind( SymTabEmitters, EmitterID );
                ASSERT( iEmitter != -1 );
                Indices.Append() = iEmitter;

                // Resolve ParticleDef ID
                s32 iParticle = SymTabFind( SymTabParticles, ParticleID );
                ASSERT( iParticle != -1 );
                Indices.Append() = iParticle;

                // Increase count of emitters
                ed.nEmitters++;

                // Read next token
                ts.Read();
            }

            // Read next token
            ts.Read();
        }

        //=====================================
        //  Make permanent copy of EffectDefs
        //=====================================

        m_nEffectDefs = EffectDefs.GetCount();
        m_pEffectDefs = new pfx_effect_def[m_nEffectDefs];
        ASSERT( m_pEffectDefs );
        for( i=0 ; i<m_nEffectDefs ; i++ )
            m_pEffectDefs[i] = EffectDefs[i];

        //=====================================
        //  Make permanent copy of Indices
        //=====================================

        m_nIndices = Indices.GetCount();
        m_pIndices = new s16[m_nIndices];
        ASSERT( m_pIndices );
        for( i=0 ; i<m_nIndices ; i++ )
            m_pIndices[i] = Indices[i];
    }

    //=====================================
    //  Write Binary Version
    //=====================================



    // Set created and active flags
    m_Created  = TRUE;

    // Return Success code
    return m_Created;
}

//=============================================================================

void pfx_library::Kill( void )
{
    s32 i;

    ASSERT( m_Created );
    m_Created = FALSE;

    for( i = 0; i < m_nTextures; i++ )
        vram_Unregister( m_pTextures[i] );

    delete [] m_pIndices;
    delete [] m_pEffectDefs;
    delete [] m_pEmitterDefs;
    delete [] m_pKeys;
    delete [] m_pParticleDefs;
    delete [] m_pTextures;
}

//=============================================================================

pfx_effect_def* pfx_library::GetEffectDef( s32 Index ) const
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nEffectDefs );

    return &m_pEffectDefs[Index];
}

pfx_emitter_def* pfx_library::GetEmitterDef( s32 Index ) const
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nEmitterDefs );

    return &m_pEmitterDefs[Index];
}

pfx_particle_def* pfx_library::GetParticleDef( s32 Index ) const
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nParticleDefs );

    return &m_pParticleDefs[Index];
}

s16 pfx_library::GetIndex( s32 Index ) const
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nIndices );

    return m_pIndices[Index];
}

pfx_key* pfx_library::GetKey( s32 Index ) const
{
    ASSERT( Index >= 0 );
    ASSERT( Index < m_nKeys );

    return &m_pKeys[Index];
}
