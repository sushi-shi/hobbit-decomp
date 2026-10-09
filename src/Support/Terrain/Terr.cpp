//==============================================================================
//
// TERR.CPP
//
//==============================================================================
 
#include <stdio.h>
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#include <xCore/x_files/x_stdio.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_time.hpp>
#include <xCore/x_files/x_memory.hpp>
#include <Support/Terrain/Terr.hpp>
#include <xCore/Entropy/e_Engine.hpp>
#include <xCore/Entropy/e_Draw.hpp>
#include <xCore/Entropy/e_VRAM.hpp>
#include "Tokenizer/Tokenizer.hpp"
#include <Support/ObjectMgr/ObjectMgr.hpp>

//#define DEBUG_RAYCAST
//#define DEBUG_EXTRUSION
//#define SHOW_BBOX
//#define TERRAIN_TIMERS

#ifdef TARGET_PS2
extern u32 VUM_TERR_CODE_START __attribute__((section(".vudata")));
extern u32 VUM_TERR_CODE_END   __attribute__((section(".vudata")));
static s32 s_MCodeHandle = -1;
#endif

#define HEIGHT_PREC (16.0f)

f32 TERRAIN_BASE_WORLD_PIXEL_SIZE = 0.25f;

#define DETAIL_WORLD_PIZEL_SIZE     0.008f

s32 STAT_TotalPassesRendered=0;

#ifdef TERRAIN_TIMERS
xtimer EmitTime;
xtimer EmitTimeNonClipped;
xtimer EmitTimeClipped;
#endif

#ifdef TARGET_PS2
static giftag s_NoClipFogGiftag;
#endif

volatile f32 s_DetailDistTweak = 0.1f;

volatile xbool PS2TERRAIN_USEFRAGMENTS = FALSE;
volatile xbool PS2TERRAIN_RENDERPASSES = TRUE;
volatile xbool PS2TERRAIN_RENDERBLOCKS = TRUE;
volatile xbool PS2TERRAIN_FOG = TRUE;
volatile xbool PS2TERRAIN_FOGPASS = TRUE;
volatile xbool PS2TERRAIN_DETAILPASS = TRUE;
volatile xbool PS2TERRAIN_EMITNONCLIPPED = TRUE;
volatile xbool PS2TERRAIN_EMITCLIPPED = TRUE;
volatile xbool PS2TERRAIN_FORCEALLCLIPPED = FALSE;
volatile xbool PS2TERRAIN_SHOWSTATS = FALSE;
volatile f32   PS2TERRAIN_VISDISTFUDGE = 0.0f;
volatile xbool PS2TERRAIN_APPLYLIGHTING = TRUE;
volatile s32   PS2TERRAIN_BLOCKZ = -1;
volatile s32   PS2TERRAIN_BLOCKX = -1;
volatile xbool PS2TERRAIN_USEOCCLUSION = TRUE;
volatile xbool PS2TERRAIN_LOCKOCCLUSION = FALSE;
volatile xbool PS2TERRAIN_NOOCCLUSION = FALSE;

//==============================================================================

struct terr_block_ref
{
    s16 BZ;
    s16 BX;
    u16 ClipMask;
    s16 PAD;
};

#define MAX_BLOCK_REFS 700
static terr_block_ref s_NCBlockRef[MAX_BLOCK_REFS];
static s32            s_NNCBlockRefs;
static terr_block_ref s_CBlockRef[MAX_BLOCK_REFS];
static s32            s_NCBlockRefs;

//==============================================================================
/*
void terr::BuildLighting( char* pFileName )
{
    s32     x,y,i,j,k;
    char LightPath[256];
    
    ASSERT( m_Fog != NULL );

    printf("*************************\n");
    printf("*** BUILDING LIGHTING ***\n");
    printf("*************************\n");

    // Allocate temporary room for lighting
    byte* pLight = (byte*)x_malloc(256*256);
    ASSERT(pLight);

    // Build lighting file path
    {
        char DRIVE[64];
        char DIR[256];
        char FNAME[64];
        char EXT[16];
        x_splitpath( pFileName,DRIVE,DIR,FNAME,EXT);
        x_makepath( LightPath,DRIVE,DIR,FNAME,".lit");
    }

    // Build path and determine if file is already saved
    xbool LightingLoaded=FALSE;
    if( !m_Fog->LightingForceRebuild )
    {
        X_FILE* fp = x_fopen(LightPath,"rb");
        if( fp )
        {
            LightingLoaded = TRUE;
            x_fread( pLight, 256*256, 1, fp );
            x_fclose(fp);
        }
    }

    // Do tracing in light direction
    s32 RayStepSum=0;
    s32 RayHitTotals=0;

    if( LightingLoaded == FALSE )
    {
        printf("Computing lighting and shadows\n");

        f32 RayDist = (120);
        s32 Steps = (s32)(RayDist/6.0f);
        f32 TInc = (1.0f/(f32)Steps);
        vector3 PInc;
        PInc = (m_Fog->LightingDir*RayDist) / (f32)Steps;

        for( y=0; y<256; y++ )
        {
            for( x=0; x<256; x++ )
            {
                f32 IShad;
                f32 IDiff;

                //
                //  Compute direction intensity (shadow)
                //
                {
                    vector3 P;
                    f32 I;

                    P.X = x*8.0f;
                    P.Y = y*8.0f;
                    P.Z = GetVertHeight(x,y);
                    P.Z += 0.1f;

                    // Trace for a certain length in light direction
                    for( i=0; i<Steps; i++ )
                    {
                        f32 H = GetHeight(P.X,P.Y);
                        if( P.Z < H )
                        {
                            RayStepSum += i;
                            RayHitTotals++;
                            break;
                        }
                        P += PInc;
                    }

                    // Did we succeed
                    if( i!=Steps ) IShad = m_Fog->LightingShadowFactor;
                    else           IShad = 1.0f;
                }

                //
                //  Compute diffuse
                //
                if( 1 )
                {
                    vector3 P[9];
                    vector3 N;

                    // Collect points
                    i=0;
                    for( s32 dy=-1; dy<=1; dy++ )
                    for( s32 dx=-1; dx<=1; dx++ )
                    {
                        P[i].X = (x+dx)*8.0f;
                        P[i].Y = (y+dy)*8.0f;
                        P[i].Z = GetVertHeight(x+dx,y+dy);
                        i++;
                    }

                    // Build normals and sum
                    N.Zero();
                    N += Cross( P[3]-P[4], P[0]-P[4] );
                    N += Cross( P[6]-P[4], P[3]-P[4] );
                    N += Cross( P[7]-P[4], P[6]-P[4] );
                    N += Cross( P[8]-P[4], P[7]-P[4] );
                    N += Cross( P[5]-P[4], P[8]-P[4] );
                    N += Cross( P[2]-P[4], P[5]-P[4] );
                    N += Cross( P[1]-P[4], P[2]-P[4] );
                    N += Cross( P[0]-P[4], P[1]-P[4] );
                    N.Normalize();

                    IDiff = Dot(N,m_Fog->LightingDir);
                    if( IDiff < 0 ) IDiff = 0;
                    if( IDiff > 1 ) IDiff = 1;
                    IDiff = m_Fog->LightingMinDiffuse + IDiff*(m_Fog->LightingMaxDiffuse-m_Fog->LightingMinDiffuse);
                }
                else
                {
                    IDiff=m_Fog->LightingMaxDiffuse;
                }

                //
                //  Combine lighting
                //
                f32 I = IDiff*IShad;
                if( I < m_Fog->LightingGlobalMin ) I = m_Fog->LightingGlobalMin;
                if( I > m_Fog->LightingGlobalMax ) I = m_Fog->LightingGlobalMax;
                pLight[x+y*256] = (s32)(128*I);
            }
        }
    }

    // Filter lightmap
    if( LightingLoaded == FALSE )
    {
        printf("Filtering light intensities\n");

        byte* pLM = (byte*)x_malloc(256*256);
        ASSERT(pLM);

        f32 Filter[9] = { 0.7, 1.0, 0.7,
                          1.0, 2.5, 1.0,
                          0.7, 1.0, 0.7 };
        f32 FilterSum;

        FilterSum=0;
        for( i=0; i<9; i++ ) FilterSum += Filter[i];
                         

        for( y=0; y<256; y++ )
        for( x=0; x<256; x++ )
        {
            f32 TLI=0;

            // Collect intensities
            for( s32 dy=-1; dy<=1; dy++ )
            for( s32 dx=-1; dx<=1; dx++ )
            {
                f32 LI = pLight[ (((dx+x)+256)%256) + ((((dy+y)+256)%256)*256) ];
                TLI += LI*Filter[(dx+1)+(dy+1)*3];
            }

            pLM[x+y*256] = (byte)(TLI/FilterSum);
        }

        x_memcpy(pLight,pLM,256*256);
        x_free(pLM);
    }

    // Apply lighting
    if( 1 )
    {
        printf("Applying lighting to material alphas\n");
        // Read into blocks
        for( i=0; i<m_NPasses;  i++ )
        for( k=0; k<32; k++ )
        for( j=0; j<32; j++ )
        for( y=0; y<9;  y++ )
        for( x=0; x<9;  x++ )
        {
            s32 LI;
            s32 AI;
            s32 I;
            LI = pLight[((j*8+x)%256)+((k*8+y)%256)*256];
            AI = m_Block[j+(k*32)].Alpha[i].Alpha[x+y*9];
            I = (AI*LI)/128;
            m_Block[j+(k*32)].Alpha[i].Alpha[x+y*9] = (byte)I;
        }
    }

    // Save out lighting
    if( 1 )
    {
        printf("Saving lighting file %1s\n",LightPath);
        X_FILE* fp = x_fopen(LightPath,"wb");
        ASSERT(fp);
        x_fwrite(pLight,256*256,1,fp);
        x_fclose(fp);
    }

    // Save out targa of lightmap
    {
        printf("Saving light map lightmap.tga\n");

        byte* pPixel = (byte*)x_malloc(256*256*4);
        ASSERT(pPixel);

        i=0;
        for( j=0; j<256*256; j++ )
        {
            pPixel[i++] = pLight[j];
            pPixel[i++] = pLight[j];
            pPixel[i++] = pLight[j];
            pPixel[i++] = pLight[j];
        }

        // Build tga header
        byte Header[18];
        x_memset(&Header,0,18);
        Header[16] = 32;
        Header[12] = (256>>0)&0xFF;
        Header[13] = (256>>8)&0xFF;
        Header[14] = (256>>0)&0xFF;
        Header[15] = (256>>8)&0xFF;
        Header[2]  = 2;
        Header[17] = 8;

        // Write out data
        X_FILE* fp;
        fp = x_fopen("lightmap.tga","wb");
        if( fp )
        {
            x_fwrite( Header, 18, 1, fp );
            x_fwrite( pPixel, 256*256*4, 1, fp );
            x_fclose( fp );
        }

        x_free(pPixel);
    }

    x_free(pLight);
    printf("*************************\n");
    printf("*** LIGHTING FINISHED ***\n");
    printf("*************************\n");
}
*/
//==============================================================================

static void LoadTexture( xbitmap& BMP, char* pFileName )
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
#ifdef TARGET_PS2
    if( !BMP.Load(XBMPath) )
#endif
    {
        xbool Result = auxbmp_LoadNative( BMP, pFileName );
        ASSERT( Result );
        if( !Result )
        {
            //x_DebugMsg("Using default for bitmap %1s\n",pFileName);
        }

        //printf("BUILDING MIPS\n");
        BMP.BuildMips();
#ifdef TARGET_PS2
        BMP.Save( XBMPath );
#endif
    }

    vram_Register( BMP );
    */
}

//==============================================================================

f32     terr::GetVertHeight   ( s32 PZ, s32 PX )
{
    s32 BX,BZ;
    PX = ((PX+65536)&255);
    PZ = ((PZ+65536)&255);
    BX = (PX>>3);
    BZ = (PZ>>3);
    PX = PX-(BX<<3);
    PZ = PZ-(BZ<<3);

    return m_Block[BZ+(BX<<5)].Height[ PZ+(PX<<3)+PX ] * (1.0f/HEIGHT_PREC);
}

//==============================================================================

void terr::GetQuadHeights   ( s32 PZ, s32 PX, f32* pH )
{
    PX = ((PX+65536)&255);
    PZ = ((PZ+65536)&255);

    if( (PZ==255) || (PX==255) )
    {
        pH[0] = GetVertHeight(PZ  ,PX  );
        pH[1] = GetVertHeight(PZ+1,PX  );
        pH[2] = GetVertHeight(PZ  ,PX+1);
        pH[3] = GetVertHeight(PZ+1,PX+1);
    }
    else
    {
        s32 BX,BZ;
        BX = (PX>>3);
        BZ = (PZ>>3);
        PX = PX-(BX<<3);
        PZ = PZ-(BZ<<3);
        s16* pHeight = &m_Block[BZ+(BX<<5)].Height[PZ+(PX<<3)+PX];

        pH[0] = pHeight[  0 ] * (1.0f/HEIGHT_PREC); 
        pH[1] = pHeight[  1 ] * (1.0f/HEIGHT_PREC); 
        pH[2] = pHeight[  9 ] * (1.0f/HEIGHT_PREC); 
        pH[3] = pHeight[ 10 ] * (1.0f/HEIGHT_PREC); 
    }
}

//==============================================================================

f32     terr::GetVertLight   ( s32 PZ, s32 PX )
{
    /*
    s32 BX,BZ;
    PX = ((PX+65536)&255);
    PZ = ((PZ+65536)&255);
    BX = (PX>>3);
    BZ = (PZ>>3);
    PX = PX-(BX<<3);
    PZ = PZ-(BZ<<3);

    f32 Alpha=0.0f;

    for( s32 i=0; i<m_NPasses; i++ )
        Alpha += (f32)m_Block[BZ+(BX<<5)].Alpha[i].Alpha[ PZ+(PX<<3)+PX ];

    return (Alpha * (1/128.0f));
    */
    return( 1.0f );
}

//==============================================================================

terr::terr          ( void )
{
    m_Fog = NULL;
}

//==============================================================================

void terr::GetQuadBounds   ( s32 PZ, s32 PX, s32 Level, f32& Min, f32& Max )
{
    PX = (PX+65536) & 255;
    PZ = (PZ+65536) & 255;

    if( Level > 0 )
    {
        PX >>= Level;
        PZ >>= Level;
        s32 I = m_MinMaxOffset[Level] + PZ + PX*(256>>Level);
        Min = m_MinY[I] * (1/HEIGHT_PREC);
        Max = m_MaxY[I] * (1/HEIGHT_PREC);
    }
    else
    {
        // Check verts directly so we don't have to store data for lowest level
        // This saves about 256*256*2*2 = 256k
        f32 H[4];
        GetQuadHeights( PZ, PX, H );
        
        Min = H[0];
        Min = MIN( Min, H[1] );
        Min = MIN( Min, H[2] );
        Min = MIN( Min, H[3] );
        Max = H[0];
        Max = MAX( Max, H[1] );
        Max = MAX( Max, H[2] );
        Max = MAX( Max, H[3] );
    }
}

//==============================================================================

void terr::LoadDetailMap(void)
{
    /*
#ifdef TARGET_PS2
    ps2color*  pClut;
    byte*   pData;
    s32     AvgIntensity=0;

    LoadTexture( m_DetailMat.BMP, m_DetailMat.Name );

    pClut = (ps2color*)m_DetailMat.BMP.GetClutData();
    pData = (byte*)m_DetailMat.BMP.GetPixelData();

    // Build alpha of detail clut
    for( s32 i=0; i<256; i++ )
    {
        ps2color C = pClut[i];
        C.A = (byte)((C.R+C.G+C.B)/3);
        pClut[i] = C;
    }

    // Compute average intensity
    s32 NPixels = m_DetailMat.BMP.GetWidth() * m_DetailMat.BMP.GetHeight();
    for( s32 i=0; i<NPixels; i++ )
    {
        AvgIntensity += pData[i];
    }
    AvgIntensity /= NPixels;

    m_DetailMipColor.R = (byte)AvgIntensity;
    m_DetailMipColor.G = (byte)AvgIntensity;
    m_DetailMipColor.B = (byte)AvgIntensity;
    m_DetailMipColor.A = 128;

    //x_DebugMsg("NMips %1d\n",m_DetailMat.BMP.GetNMips() );
    //x_DebugMsg("DETAIL MAP AVERAGE INTENSITY: %1d\n",AvgIntensity);

    // Apply morphing to mipmaps
    ASSERT( m_DetailMat.BMP.GetNMips() == 4 );
    for( s32 i=0; i<=m_DetailMat.BMP.GetNMips(); i++ )
    {
        // Get ptr to pixel data
        pData = (byte*)m_DetailMat.BMP.GetPixelData(i);

        // Compute number of pixels to process
        s32 NPixels = (m_DetailMat.BMP.GetWidth(i)*m_DetailMat.BMP.GetHeight(i));

        // fade pixels
        for( s32 j=0; j<NPixels; j++ )
        {
            s32 P = pData[j];
            P = (s32)(AvgIntensity + m_DetailMipFade[i]*(f32)(P-AvgIntensity));
            ASSERT( (P>=0) && (P<=255) );
            pData[j] = (byte)P;
        }
    }

    // Save out textures for review
    //m_DetailMat.BMP.SaveMipsTGA("detailmip.tga");
#endif
    */
}


//==============================================================================

void terr::Kill( void )
{
    /*
    //x_DebugMsg("FAKE Terrain Kill\n");

    #ifdef TARGET_PS2
    vram_Unregister( m_DetailMat.BMP );
    #endif

    s32 i;
    // Destroy materials
    for( i=0; i<m_NPasses; i++ )
    {
        vram_Unregister( m_Material[i].BMP );
        m_Material[i].BMP.Kill();
    }

    //VRAM_UnRegister( m_DetailMat.BMP );
    //m_DetailMat.BMP.KillBitmap();
    */
}

//==============================================================================

s32 ReadDynamixString( char* Buff, byte* pData )
{
    s32 i;
    s32 Len = pData[0];
    for( i=0; i<Len; i++ )
        Buff[i] = pData[i+1];
    Buff[i] = 0;
    return Len+1;
}

//==============================================================================

void terr::Load( const char* pFileName )
{
    s32 i,j,k,x,z;

    // Confirm block data is 16byte aligned
#ifdef TARGET_PS2
    ASSERT( (((u32)m_Block)&0x0F) == 0 );

    // Register microcode
    if( s_MCodeHandle == -1 )
    {
        u32 Start = (u32)(&VUM_TERR_CODE_START);
        u32 End   = (u32)(&VUM_TERR_CODE_END);
        s_MCodeHandle = eng_RegisterMicrocode("TERRAIN",&VUM_TERR_CODE_START,End-Start );
    }
#endif

    // Read mip level
    m_BaseWorldPixelSize = 0.25f;

    // Read detail mip level
    m_DetailWorldPixelSize = 0.007f;

    // Read detail mip-fade values
    m_DetailMipFade[0] = 1.0f;
    m_DetailMipFade[1] = 0.9f;
    m_DetailMipFade[2] = 0.6f;
    m_DetailMipFade[3] = 0.3f;
    m_DetailMipFade[4] = 0.0f;

    // Read .ter file name
    char TerrFileName[256];
    x_strcpy(TerrFileName,pFileName);
    //x_DebugMsg("Using terrain: %1s\n",TerrFileName);

    // Try loading .ter file
    s32 TDSize;
    byte* TD;
    {
        X_FILE* terrfp;
        terrfp = x_fopen(TerrFileName,"rb");
        ASSERT( terrfp );
        TDSize = x_flength(terrfp);
        TD = (byte*)x_malloc(sizeof(byte)*TDSize);
        ASSERT(TD);
        x_fread( TD, TDSize, 1, terrfp );
        x_fclose(terrfp);
    }


    // READ IN HEIGHT DATA
    {
        // Seek to data section
        //VERIFY(TOK.Find("TerrainData",TRUE));

        // Read heights
        m_MaxHeight = -1000000.0f;
        m_MinHeight =  1000000.0f;
        s16* pHeight = (s16*)x_malloc(sizeof(s16)*256*256);
        ASSERT(pHeight);
        byte* pHD = &TD[1];
        for( x=0; x<256; x++ )
        for( z=0; z<256; z++ )
        {
            s32 H;
            //H = TOK.ReadInt();
            s32 B0 = pHD[(z+x*256)*2+0];
            s32 B1 = pHD[(z+x*256)*2+1];
            H = B0 | (B1<<8);
            H = (s32)(HEIGHT_PREC*((f32)H/(32.0f)));

            pHeight[z+x*256] = (s16)H;

            f32 FH = H / HEIGHT_PREC;
            if( FH > m_MaxHeight ) m_MaxHeight = FH;
            if( FH < m_MinHeight ) m_MinHeight = FH;
        }

        // Copy into blocks
        x_memset(m_Block,0,sizeof(m_Block));
        for( i=0; i<32; i++ )
        for( j=0; j<32; j++ )
        for( x=0; x<9;  x++ )
        for( z=0; z<9;  z++ )
        {
            m_Block[j+(i<<5)].Height[z+(x<<3)+x] = pHeight[(((j<<3)+z)&255)+((((i<<3)+x)&255)<<8)];
        }

        x_free(pHeight);
        pHeight=NULL;
    }


    // Read and load materials
    //x_DebugMsg("******************************\n");
    //x_DebugMsg("*** LOADING BASE MATERIALS ***\n");
    //x_DebugMsg("******************************\n");
    m_NPasses = 0;
    byte* pAD;
    {
        byte* pMD = &TD[65536*3+1];
        for( i=0; i<8; i++ )
        {
            char MatName[256];
            char* pName;
            pMD += ReadDynamixString( MatName, pMD );

            pName = MatName + x_strlen(MatName);
            while( pName > MatName ) 
                if( (*pName) == '.' ) break;
                else pName--;
            pName--;

            while( pName > MatName ) 
                if( (*pName) == '.' ) break;
                else pName--;
            if( (*pName) == '.' )
                pName++;

            if( (MatName[0]) && (i < TERR_MAX_BASE_PASSES) )
            {
                // See if pass exists
                x_strcpy(m_Material[i].Name,xfs("Data/Terrain/%s.bmp",pName));
                x_strtoupper(m_Material[i].Name);

                if( m_Material[i].Name[0] != 0 )
                {
//                    LoadTexture( m_Material[i].BMP, m_Material[i].Name );
                    m_NPasses++;
                }
            }
        }
        pAD = pMD;
    }

    // Read detail material
    {
        s32 i;
        char* Env[5] = {"LUSH","ICE","BADLANDS","DESERT","LAVA"};
        for( i=0; i<5; i++ )
        {
            if( x_strstr( m_Material[0].Name, Env[i] ) )
            {
                x_strcpy( m_DetailMat.Name,xfs("Data/Terrain/Detail/%s.bmp",Env[i]) );
                break;
            }
        }
        ASSERT( i != 5 );

        LoadDetailMap();
    }


    // Clear block pass-masks
    for( i=0; i<32*32; i++ )
    {
        m_Block[i].PassMask = 0;
        m_Block[i].NPasses  = 0;
    }

    // Read material alphas
    {
        byte* pAlpha = (byte*)x_malloc(sizeof(byte)*256*256);
        ASSERT(pAlpha);

        for( i=0; i<m_NPasses; i++ )
        {
            // Read alpha values
            for( x=0; x<256; x++ )
            for( z=0; z<256; z++ )
            {
                s32 A = *pAD;//TOK.ReadInt();
                pAD++;
                A>>=1;
                if( TERR_MAX_BASE_PASSES==1 ) A = 128;
                pAlpha[z+(x<<8)] = (byte)(A);
            }

            // Read into blocks
            for( k=0; k<32; k++ )
            for( j=0; j<32; j++ )
            {
                s32 BlockTotalAlpha=0;

                for( x=0; x<9; x++ )
                for( z=0; z<9; z++ )
                {
                    byte A = (byte)pAlpha[(((j<<3)+z)&255)+((((k<<3)+x)&255)<<8)];
                    m_Block[j+(k<<5)].Alpha[i].Alpha[z+(x<<3)+x] = A;
                    BlockTotalAlpha += A;
                }

                // Turn on pass if there are alphas non-zero
                if( BlockTotalAlpha > 0 )
                {
                    m_Block[j+(k<<5)].PassMask |= (1<<i);
                    m_Block[j+(k<<5)].NPasses++;
                }
            }
        }

        if( 0 )
        {
            for( i=0; i<32*32; i++ )
            {
                m_Block[i].PassMask = 0xFFFFFFFF;
                m_Block[i].NPasses  = m_NPasses;
            }
        }

        x_free(pAlpha);
    }

    // Report number of passes needed
    {
        s32 TotalPasses=0;
        s32 MaxPasses=0;
        s32 MinPasses=10000;
        for( i=0; i<32*32; i++ )
        {
            s32 NPasses=0;
            for( j=0; j<m_NPasses; j++ )
            if( (m_Block[i].PassMask & (1<<j)) )
                NPasses++;

            MaxPasses = MAX(MaxPasses,NPasses);
            MinPasses = MIN(MinPasses,NPasses);
            TotalPasses += NPasses;
        }

        //x_DebugMsg("PASS INFO PASS INFO PASS INFO PASS INFO PASS INFO\n");
        //x_DebugMsg("Num Base textures   %1d\n",m_NPasses);
        //x_DebugMsg("Total Passes        %1d\n",TotalPasses);
        //x_DebugMsg("Max Passes          %1d\n",MaxPasses);
        //x_DebugMsg("Min Passes          %1d\n",MinPasses);
        //x_DebugMsg("Avg Passes          %1.1f\n",(f32)TotalPasses / 1024.0f );
        //x_DebugMsg("PASS INFO PASS INFO PASS INFO PASS INFO PASS INFO\n");
    }

    // Build offsets for the height levels
    {
        s32 Size=4;
        m_MinMaxOffset[8] = 0;
        m_MinMaxOffset[7] = 1;
        for( i=6; i>=0; i-- )
        {
            m_MinMaxOffset[i] = m_MinMaxOffset[i+1] + Size;
            Size <<= 2;
        }
        m_MinMaxOffset[0] = 0;
    }

    {
        s32 z,x,k;
        s16* pLowestMin;
        s16* pLowestMax;

        pLowestMin = (s16*)x_malloc(sizeof(s16)*256*256);
        pLowestMax = (s16*)x_malloc(sizeof(s16)*256*256);

        // Populate lowest level of height data
        k=0;
        for( x=0; x<256; x++ )
        for( z=0; z<256; z++ )
        {
            f32 MinH= 100000;
            f32 MaxH=-100000;
            f32 H;
            H = GetVertHeight((z+0),(x+0)); MinH = MIN(H,MinH);  MaxH = MAX(H,MaxH);
            H = GetVertHeight((z+0),(x+1)); MinH = MIN(H,MinH);  MaxH = MAX(H,MaxH);
            H = GetVertHeight((z+1),(x+1)); MinH = MIN(H,MinH);  MaxH = MAX(H,MaxH);
            H = GetVertHeight((z+1),(x+0)); MinH = MIN(H,MinH);  MaxH = MAX(H,MaxH);
            pLowestMin[k] = (s16)(MinH*HEIGHT_PREC);
            pLowestMax[k] = (s16)(MaxH*HEIGHT_PREC);
            k++;
        }

        // Populate parent levels
        for( s32 L=1; L<=8; L++ )
        {
            s16* pMinSrc;
            s16* pMaxSrc;
            s32 Num = 256 >> L;

            if( L==1 )
            {
                pMinSrc = pLowestMin;
                pMaxSrc = pLowestMax;
            }
            else
            {
                pMinSrc = m_MinY + m_MinMaxOffset[L-1];
                pMaxSrc = m_MaxY + m_MinMaxOffset[L-1];
            }

            k=m_MinMaxOffset[L];
            for( x=0; x<Num; x++ )
            for( z=0; z<Num; z++ )
            {
                s16 MinH= 32767;
                s16 MaxH=-32767;
                s16 H;

                for( s32 dx=0; dx<=1; dx++ )
                for( s32 dz=0; dz<=1; dz++ )
                {
                    s32 I = (((z*2)+dz)%(Num*2))+(((x*2)+dx)%(Num*2))*(Num*2);
                    H = pMinSrc[I];
                    MinH = MIN(H,MinH);
                    H = pMaxSrc[I];
                    MaxH = MAX(H,MaxH);
                }
                
                m_MinY[k] = MinH;
                m_MaxY[k] = MaxH;
                k++;
            }
        }

        // free lowest values
        x_free(pLowestMin);
        x_free(pLowestMax);
    }

    // Build lighting
    //x_DebugMsg("FAKE Build lighting\n");
    //BuildLighting( m_FileName );

    x_free(TD);

    //x_DebugMsg("**********************\n");
    //x_DebugMsg("*** TERRAIN LOADED ***\n");
    //x_DebugMsg("**********************\n");
    //printf("Load time: %1.1f secs\n",TIME_PopMs()/1000.0f );
}

//==============================================================================

f32 terr::NearestDistSquared( const vector3& Min, const vector3& Max )
{
    vector3 P = m_EyePos;

    if( m_EyePos.X > Max.X ) P.X = Max.X;
    else
    if( m_EyePos.X < Min.X ) P.X = Min.X;

    if( m_EyePos.Y > Max.Y ) P.Y = Max.Y;
    else
    if( m_EyePos.Y < Min.Y ) P.Y = Min.Y;

    if( m_EyePos.Z > Max.Z ) P.Z = Max.Z;
    else
    if( m_EyePos.Z < Min.Z ) P.Z = Min.Z;

    P -= m_EyePos;
    return (P.X*P.X + P.Y*P.Y + P.Z*P.Z);
}
/*
//==============================================================================

static void RenderBBox( const vector3& Min, const vector3& Max, const vector4& Color )
{
    static s32 Edge[12*2] = {1,5,5,7,7,3,3,1,0,4,4,6,6,2,2,0,3,2,7,6,5,4,1,0};
    s32 i,j;
    vector3 P[8];

    P[0].X = Min.X;    P[0].Y = Min.Y;    P[0].Z = Min.Z;
    P[1].X = Min.X;    P[1].Y = Min.Y;    P[1].Z = Max.Z;
    P[2].X = Min.X;    P[2].Y = Max.Y;    P[2].Z = Min.Z;
    P[3].X = Min.X;    P[3].Y = Max.Y;    P[3].Z = Max.Z;
    P[4].X = Max.X;    P[4].Y = Min.Y;    P[4].Z = Min.Z;
    P[5].X = Max.X;    P[5].Y = Min.Y;    P[5].Z = Max.Z;
    P[6].X = Max.X;    P[6].Y = Max.Y;    P[6].Z = Min.Z;
    P[7].X = Max.X;    P[7].Y = Max.Y;    P[7].Z = Max.Z;

    // Render the cube wireframe
    PRIM_Begin( PRIM_3D_LINES );
        
        PRIM_Color((s32)(Color.X*255),(s32)(Color.Y*255),(s32)(Color.Z*255),(s32)(Color.W*255));
        
        for( s32 i=0; i<12; i++ )
        {
            PRIM_Vertex( P[Edge[i*2+0]].X, P[Edge[i*2+0]].Y, P[Edge[i*2+0]].Z );
            PRIM_Vertex( P[Edge[i*2+1]].X, P[Edge[i*2+1]].Y, P[Edge[i*2+1]].Z );
        }

    PRIM_End();
}

//==============================================================================

struct block_c_pack
{
    dmatag      DMA;
    u32         CMD[4];
    vector4     XYZBase;
    vector4     XYZDelta;
    vector4     UVBase;
    vector4     UVDelta;
    giftag      GifTagNC;
    giftag      GifTagC;
    s32         DetailColor[4];
    dmatag      DMARef;
    dmatag      DMAKick;    // Builds verts/uvs/fog and blocks until done
};

struct alpha_c_pack
{
    dmatag      DMA;
    u32         CMD[4];  // Alpha opcode
    dmatag      DMARef;  // Reference to the alpha data
    dmatag      DMAKick; // Kick microcode and flush
};

struct detail_c_pack
{
    dmatag      DMA;
    u32         CMD[4];
    u32         VIF[4];
};

struct fog_c_pack
{
    dmatag      DMA;
    u32         CMD[4];
    giftag      GifTag1;
    giftag      GifTag2;
    u32         VIF[4];
};

//==============================================================================

void terr::ShipBlockClip( s32 BX, s32 BY, const vector3& Trans, f32 NDistSquared )
{
    //
    // Build block pack to create/transform/fog verts
    //
    ASSERT( (BX>=0) && (BX<32) && (BY>=0) && (BY<32) );
    block* pBlock = m_Block + BX+BY*32;

    block_c_pack* pPack = DLStruct(block_c_pack);

    pPack->DMA.SetCont( 8*16 );
    pPack->DMA.PAD[1] = VIF_Unpack(0,8,VIF_V4_32,FALSE,FALSE,TRUE);

    pPack->CMD[0] = 8; // Opcode for clipped block processing
    pPack->CMD[1] = 0;
    pPack->CMD[2] = 0;
    pPack->CMD[3] = 0;

    pPack->XYZBase.X  = Trans.X;
    pPack->XYZBase.Y  = Trans.Y;
    pPack->XYZBase.Z  = Trans.Z;
    pPack->XYZBase.W  = 1.0f;

    pPack->XYZDelta.X = 8.0f;
    pPack->XYZDelta.Y = 8.0f;
    pPack->XYZDelta.Z = 0.0f;
    pPack->XYZDelta.W = 0.0f;

    pPack->UVBase.X  = 0;
    pPack->UVBase.Y  = 0;
    pPack->UVBase.Z  = 1;
    pPack->UVBase.W  = 1;

    pPack->UVDelta.X = 0.125f;
    pPack->UVDelta.Y = 0.125f;
    pPack->UVDelta.Z = 0;
    pPack->UVDelta.W = 0;

    pPack->GifTagNC.Build2(3,0,GIF_PRIM_TRIANGLE,GIF_FLAG_TEXTURE|GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
    pPack->GifTagNC.Reg(GIF_REG_ST,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;

    pPack->GifTagC.Build2(3,0,GIF_PRIM_TRIANGLEFAN,GIF_FLAG_TEXTURE|GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
    pPack->GifTagC.Reg(GIF_REG_ST,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;

    if( m_DoDetail && (NDistSquared < (m_DetailDist*m_DetailDist)) )
    {
        pPack->DetailColor[0] = 128;
        pPack->DetailColor[1] = 128;
        pPack->DetailColor[2] = 128;
        pPack->DetailColor[3] = 128;
    }
    else
    {
        pPack->DetailColor[0] = m_DetailMipColor.R;
        pPack->DetailColor[1] = m_DetailMipColor.G;
        pPack->DetailColor[2] = m_DetailMipColor.B;
        pPack->DetailColor[3] = m_DetailMipColor.A;
    }

    pPack->DMARef.SetRef(176,(u32)pBlock->Height);
    pPack->DMARef.PAD[1] = VIF_Unpack(8,81,VIF_S_16,TRUE,FALSE,TRUE);

    pPack->DMAKick.SetCont(0);
    pPack->DMAKick.PAD[0] = SCE_VIF1_SET_MSCNT(0);
    pPack->DMAKick.PAD[1] = SCE_VIF1_SET_FLUSH(0);

    //
    // Loop through materials and build alpha packs
    //
    VRAM_SetMipEquation(m_BaseMipK);
    s32 NPassesRendered=0;
    for( s32 M=0; M<m_NPasses; M++ )
    if( pBlock->PassMask & (1<<M) )
    {
        VRAM_Activate( m_Material[M].BMP );

        if( NPassesRendered==0 ) 
        {
            GSREG_Begin();
            GSREG_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_ZERO) );
            GSREG_End();
        }
        else       
        if( NPassesRendered==1 ) 
        {
            GSREG_Begin();
            GSREG_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
            GSREG_End();
        }

        alpha_c_pack* pPack = DLStruct(alpha_c_pack);

        pPack->DMA.SetCont(16*1);
        pPack->DMA.PAD[1] = VIF_Unpack(0,1,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 10; // Opcode for block_c_alpha processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        pPack->DMARef.SetRef( sizeof(block_alpha), (u32)&pBlock->Alpha[M] );
        pPack->DMARef.PAD[1] = VIF_Unpack(1,81,VIF_S_8,FALSE,FALSE,TRUE);

        pPack->DMAKick.SetCont(0);
        pPack->DMAKick.PAD[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->DMAKick.PAD[1] = SCE_VIF1_SET_FLUSH(0);

        NPassesRendered++;
    }
    STAT_TotalPassesRendered += NPassesRendered;

    if( m_DoDetail && (NDistSquared < (m_DetailDist*m_DetailDist)) )
    {
        //VRAM_SetMipEquation( m_DetailMipK );
        VRAM_Activate( m_DetailMat.BMP );

        GSREG_Begin();
        GSREG_SetMipEquation( FALSE, m_DetailMipK, 5, MIP_MAG_BILINEAR, MIP_MIN_INTERP_MIPMAP_BILINEAR );
        GSREG_SetAlphaBlend( ALPHA_BLEND_MODE(C_DST,C_ZERO,A_SRC,C_ZERO) );
        GSREG_End();

        detail_c_pack* pPack = DLStruct(detail_c_pack);

        pPack->DMA.SetCont(sizeof(detail_c_pack)-sizeof(dmatag));
        pPack->DMA.PAD[1] = VIF_Unpack(0,1,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 16; // Opcode for detail pass processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        pPack->VIF[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->VIF[1] = SCE_VIF1_SET_FLUSH(0);
        pPack->VIF[2] = 0;
        pPack->VIF[3] = 0;
    }

    GSREG_Begin();
    GSREG_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_DST,A_SRC,C_DST) );
    GSREG_End();

    // Build fog pass
    if( m_DoFog )
    {
        fog_c_pack* pPack = DLStruct(fog_c_pack);

        pPack->DMA.SetCont(sizeof(fog_c_pack)-sizeof(dmatag));
        pPack->DMA.PAD[1] = VIF_Unpack(0,3,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 14; // Opcode for fog pass processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        pPack->GifTag1.Build2(3,10,GIF_PRIM_TRIANGLE,GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
        pPack->GifTag1.Reg(GIF_REG_NOP,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;

        pPack->GifTag2.Build2(3,10,GIF_PRIM_TRIANGLEFAN,GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
        pPack->GifTag2.Reg(GIF_REG_NOP,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;

        pPack->VIF[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->VIF[1] = SCE_VIF1_SET_FLUSH(0);
        pPack->VIF[2] = 0;
        pPack->VIF[3] = 0;
    }
}

//==============================================================================

struct block_nc_pack
{
    dmatag      DMA;
    u32         CMD[4];
    vector4     XYZBase;
    vector4     XYZDelta;
    vector4     UVBase;
    vector4     UVDelta;
    giftag      GifTag;
    s32         DetailColor[4];
    dmatag      DMARef;
    dmatag      DMAKick;    // Builds verts/uvs/fog and blocks until done
} __attribute__((aligned (16)));

struct alpha_nc_pack
{
    dmatag      DMA;
    u32         CMD[4];  // Alpha opcode
    u32         FOGRange[4];    // Interlaced fog range
    dmatag      DMARef;  // Reference to the alpha data
    dmatag      DMAKick; // Kick microcode and flush
};

struct fog_nc_pack
{
    dmatag      DMA;
    u32         CMD[4];
    giftag      GifTag;
    u32         VIF[4];
};

//==============================================================================

static xbool         s_NCPacksInited=FALSE;
static block_nc_pack s_BlockNCPack;

void InitNCPacks( void )
{
    s_NCPacksInited = TRUE;

    block_nc_pack* pPack = &s_BlockNCPack;

    pPack->DMA.SetCont( 7*16 );
    pPack->DMA.PAD[1] = VIF_Unpack(0,7,VIF_V4_32,FALSE,FALSE,TRUE);

    pPack->CMD[0] = 4; // Opcode for block processing
    pPack->CMD[1] = 0;
    pPack->CMD[2] = 0;
    pPack->CMD[3] = 0;

    pPack->XYZBase.X  = 0;
    pPack->XYZBase.Y  = 0;
    pPack->XYZBase.Z  = 0;
    pPack->XYZBase.W  = 1.0f;

    pPack->XYZDelta.X = 8.0f;
    pPack->XYZDelta.Y = 8.0f;
    pPack->XYZDelta.Z = 0.0f;
    pPack->XYZDelta.W = 0.0f;

    pPack->UVBase.X  = 0;
    pPack->UVBase.Y  = 0;
    pPack->UVBase.Z  = 1;
    pPack->UVBase.W  = 1;

    pPack->UVDelta.X = 0.125f;
    pPack->UVDelta.Y = 0.125f;
    pPack->UVDelta.Z = 0;
    pPack->UVDelta.W = 0;

    pPack->GifTag.Build2(3,10,GIF_PRIM_TRIANGLEFAN,GIF_FLAG_TEXTURE|GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
    //pPack->GifTag.Build2(3,10,GIF_PRIM_TRIANGLEFAN,GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
    //pPack->GifTag.Build2(3,10,GIF_PRIM_LINESTRIP,GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
    pPack->GifTag.Reg(GIF_REG_ST,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;
    pPack->GifTag.EOP = 0;

    //pPack->DMARef.SetRef(176,(u32)pBlock->Height);
    //pPack->DMARef.PAD[1] = VIF_Unpack(6,81,VIF_S_16,TRUE,FALSE,TRUE);

    pPack->DMAKick.SetCont(0);
    pPack->DMAKick.PAD[0] = SCE_VIF1_SET_MSCNT(0);
    pPack->DMAKick.PAD[1] = SCE_VIF1_SET_FLUSH(0);
    
}

void terr::ShipBlockNoClip( s32 BX, s32 BY, const vector3& Trans, f32 NDistSquared )
{
    u32 StartDListAddress = (u32)DLGetAddr();

    if( s_NCPacksInited==FALSE )
        InitNCPacks();
    //
    // Build block pack to create/transform/fog verts
    //
    ASSERT( (BX>=0) && (BX<32) && (BY>=0) && (BY<32) );
    block* pBlock = m_Block + BX+BY*32;

    block_nc_pack* pPack = DLStruct(block_nc_pack);
    memcpy(pPack,&s_BlockNCPack,sizeof(block_nc_pack));
    pPack->XYZBase.X  = Trans.X;
    pPack->XYZBase.Y  = Trans.Y;
    pPack->XYZBase.Z  = Trans.Z;
    pPack->DMARef.SetRef(176,(u32)pBlock->Height);
    pPack->DMARef.PAD[1] = VIF_Unpack(7,81,VIF_S_16,TRUE,FALSE,TRUE);

    if( m_DoDetail && (NDistSquared < (m_DetailDist*m_DetailDist)) )
    {
        pPack->DetailColor[0] = 128;
        pPack->DetailColor[1] = 128;
        pPack->DetailColor[2] = 128;
        pPack->DetailColor[3] = 128;
    }
    else
    {
        pPack->DetailColor[0] = m_DetailMipColor.R;
        pPack->DetailColor[1] = m_DetailMipColor.G;
        pPack->DetailColor[2] = m_DetailMipColor.B;
        pPack->DetailColor[3] = m_DetailMipColor.A;
    }

    // Loop through materials and build alpha packs
    //
    s32 FogOffset=0;
    s32 FogLeft=81;
    VRAM_SetMipEquation( m_BaseMipK );
    s32 NPassesRendered=0;
    for( s32 M=0; M<m_NPasses; M++ )
    if( pBlock->PassMask & (1<<M) )
    {
        VRAM_Activate( m_Material[M].BMP );

        if( NPassesRendered==0 ) 
        {
            GSREG_Begin();
            GSREG_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_ZERO) );
            GSREG_End();
        }
        else       
        if( NPassesRendered==1 ) 
        {
            GSREG_Begin();
            GSREG_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
            GSREG_End();
        }

        alpha_nc_pack* pPack = DLStruct(alpha_nc_pack);

        pPack->DMA.SetCont(16*2);
        pPack->DMA.PAD[1] = VIF_Unpack(0,2,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 6; // Opcode for block_alpha processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        
        s32 NFog;
        if( NPassesRendered == (pBlock->NPasses-1) )
            NFog = FogLeft;
        else
            NFog = MIN(FogLeft,(((81/pBlock->NPasses)/3)*3));
        pPack->FOGRange[0] = FogOffset; // Offset into fog vectors
        pPack->FOGRange[1] = NFog; // Number of fog values to compute
        pPack->FOGRange[2] = 0;
        pPack->FOGRange[3] = 0;
        FogOffset += NFog;

        pPack->DMARef.SetRef( sizeof(block_alpha), (u32)&pBlock->Alpha[M] );
        pPack->DMARef.PAD[1] = VIF_Unpack(2,81,VIF_S_8,FALSE,FALSE,TRUE);

        pPack->DMAKick.SetCont(0);
        pPack->DMAKick.PAD[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->DMAKick.PAD[1] = SCE_VIF1_SET_FLUSH(0);
        NPassesRendered++;
    }
    STAT_TotalPassesRendered += NPassesRendered;

    if( m_DoDetail && (NDistSquared < (m_DetailDist*m_DetailDist)) )
    {
        //VRAM_SetMipEquation( m_DetailMipK );
        VRAM_Activate( m_DetailMat.BMP );

        GSREG_Begin();
        GSREG_SetMipEquation( FALSE, m_DetailMipK, 5, MIP_MAG_BILINEAR, MIP_MIN_INTERP_MIPMAP_BILINEAR );
        GSREG_SetAlphaBlend( ALPHA_BLEND_MODE(C_DST,C_ZERO,A_SRC,C_ZERO) );
        GSREG_End();

        detail_c_pack* pPack = DLStruct(detail_c_pack);

        pPack->DMA.SetCont(sizeof(detail_c_pack)-sizeof(dmatag));
        pPack->DMA.PAD[1] = VIF_Unpack(0,1,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 18; // Opcode for detail pass processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        pPack->VIF[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->VIF[1] = SCE_VIF1_SET_FLUSH(0);
        pPack->VIF[2] = 0;
        pPack->VIF[3] = 0;
    }

    GSREG_Begin();
    GSREG_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_DST,A_SRC,C_DST) );
    GSREG_End();

    // Build fog pass
    if( m_DoFog )
    {
        fog_nc_pack* pPack = DLStruct(fog_nc_pack);

        pPack->DMA.SetCont(16*3);
        pPack->DMA.PAD[1] = VIF_Unpack(0,2,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 12; // Opcode for fog pass processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        pPack->GifTag.Build2(3,10,GIF_PRIM_TRIANGLEFAN,GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
        pPack->GifTag.Reg(GIF_REG_NOP,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;
        pPack->GifTag.EOP = 0;

        pPack->VIF[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->VIF[1] = SCE_VIF1_SET_FLUSH(0);
        pPack->VIF[2] = 0;
        pPack->VIF[3] = 0;
    }

    s32 DListUsed = ((u32)DLGetAddr() - StartDListAddress);

}
*/

//==============================================================================

inline
s32 terr::IsBoxInView(const bbox& BBox, s32 Mask)
{
    f32*     pF = ((f32*)&BBox);
    s32*     pMinI = m_ClipMinIndex;
    s32*     pMaxI = m_ClipMaxIndex;
    plane*   pPlane = m_ClipPlane;

    for(s32 i=0; i<TERR_NUM_PLANES; i++)
    {
        // Are we checking this plane?
        if( Mask & (1<<i))
        {
            // Compute min and max dist along normal
            f32 MinDist = pPlane->Normal.X * pF[pMinI[0]] +
                          pPlane->Normal.Y * pF[pMinI[1]] +
                          pPlane->Normal.Z * pF[pMinI[2]] +
                          pPlane->D;

            f32 MaxDist = pPlane->Normal.X * pF[pMaxI[0]] +
                          pPlane->Normal.Y * pF[pMaxI[1]] +
                          pPlane->Normal.Z * pF[pMaxI[2]] +
                          pPlane->D;

            // If outside plane, we are culled
            if(MaxDist < 0)
                return -1;

            // If inside plane turn off this clipping bit
            if(MinDist >= 0)
                Mask ^= (1<<i);
        }

        // Move to next plane
        pMinI += 3;
        pMaxI += 3;
        pPlane++;
    }

    return Mask;
} 

//==============================================================================
//==============================================================================
//==============================================================================
// START OF D3D RENDERING
//==============================================================================
//==============================================================================
//==============================================================================
#ifdef TARGET_PC
struct d3dvert
{
    vector3 P;
    xcolor  C;
    vector2 T;
};

#define NUM_D3D_BLOCKS  64
#define NUM_PASS_BUFFERS 8

struct d3dblock
{
    d3dvert V[81];
};

struct d3dpass
{
    IDirect3DVertexBuffer8* pVertexBuffer[TERR_MAX_BASE_PASSES];
    d3dblock*               pBlock[TERR_MAX_BASE_PASSES];
    s32                     Count;
};

xbool s_D3DSetup = FALSE;
d3dpass                 s_pPass[NUM_PASS_BUFFERS];
s32                     s_PassBufferIndex;
IDirect3DIndexBuffer8*  s_pD3DIndexBuffer=NULL;
s16* s_pD3DIndex=NULL;

//==============================================================================

void SetupD3D( void )
{
    s32 i,j,x,y;
    dxerr Err;

    if( s_D3DSetup ) return;
    s_D3DSetup = TRUE;
    
    //
    // Get access to index buffer and fill out the grid indices
    //

    Err = g_pd3dDevice->CreateIndexBuffer(  (sizeof(s16)*3*128)*NUM_D3D_BLOCKS, 
                                            D3DUSAGE_WRITEONLY, 
                                            D3DFMT_INDEX16, 
                                            D3DPOOL_MANAGED, 
                                            &s_pD3DIndexBuffer );
    ASSERT( Err==0 );

    Err = s_pD3DIndexBuffer->Lock( 0, 0, (byte**)&s_pD3DIndex, D3DLOCK_NOSYSLOCK  );
    ASSERT( Err == 0 );

    i=0;
    for( j=0; j<NUM_D3D_BLOCKS; j++ )
    {
        for( y=0; y<8; y++ )
        for( x=0; x<8; x++ )
        {
            if( (x^y)&0x01 )
            {
                // +---+
                // |\  |
                // |  \|
                // +---+
                s_pD3DIndex[i++] = (s16)(((x+0)+(y+0)*9)+(j*81));
                s_pD3DIndex[i++] = (s16)(((x+0)+(y+1)*9)+(j*81));
                s_pD3DIndex[i++] = (s16)(((x+1)+(y+0)*9)+(j*81));
                s_pD3DIndex[i++] = (s16)(((x+1)+(y+0)*9)+(j*81));
                s_pD3DIndex[i++] = (s16)(((x+0)+(y+1)*9)+(j*81));
                s_pD3DIndex[i++] = (s16)(((x+1)+(y+1)*9)+(j*81));
            }
            else
            {
                // +---+
                // |  /|
                // |/  |
                // +---+
                s_pD3DIndex[i++] = (s16)(((x+0)+(y+0)*9)+(j*81));
                s_pD3DIndex[i++] = (s16)(((x+0)+(y+1)*9)+(j*81));
                s_pD3DIndex[i++] = (s16)(((x+1)+(y+1)*9)+(j*81));
                s_pD3DIndex[i++] = (s16)(((x+0)+(y+0)*9)+(j*81));
                s_pD3DIndex[i++] = (s16)(((x+1)+(y+1)*9)+(j*81));
                s_pD3DIndex[i++] = (s16)(((x+1)+(y+0)*9)+(j*81));
            }
        }
    }

    s_pD3DIndexBuffer->Unlock();


    //
    // Create and get access to the vertex buffer and fill out the grid
    //
    for( i=0; i<NUM_PASS_BUFFERS; i++ )
    {
        for( s32 j=0; j<TERR_MAX_BASE_PASSES; j++ )
        {
            // SB - D3DPOOL_SYSTEMMEM so we can recover from losing the d3d device
            Err = g_pd3dDevice->CreateVertexBuffer( sizeof(d3dblock)*NUM_D3D_BLOCKS, 
                                                    D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
                                                    D3DFVF_DIFFUSE|D3DFVF_XYZ|D3DFVF_TEX1, 
                                                    D3DPOOL_SYSTEMMEM,
                                                    &s_pPass[i].pVertexBuffer[j] );


            ASSERT( s_pPass[i].pVertexBuffer[j] );
            ASSERT( Err==0 );
        }
    }

    s_PassBufferIndex = 0;
}

//==============================================================================

void ShutdownD3D( void )
{
    if( !s_D3DSetup ) return;
    s_D3DSetup = FALSE;
    s_pD3DIndexBuffer->Release();
    for( s32 i=0; i<NUM_PASS_BUFFERS; i++ )
    for( s32 j=0; j<TERR_MAX_BASE_PASSES; j++ )
        s_pPass[i].pVertexBuffer[j]->Release();
}

#ifdef TERRAIN_TIMERS
xtimer OpenTime;
xtimer CloseTime;
xtimer VertBuildTime;
xtimer DrawPrimTime;
#endif

void terr::OpenD3DPass(void)
{
#ifdef TERRAIN_TIMERS
    OpenTime.Start();
#endif

    d3dpass* pP = &s_pPass[s_PassBufferIndex];
    dxerr Err;

    // Lock all the vertex buffers
    for( s32 i=0; i<m_NPasses; i++ )
    {
        Err = pP->pVertexBuffer[i]->Lock( 0, 0, (byte**)&pP->pBlock[i], D3DLOCK_DISCARD  );
        ASSERT( Err == 0 );
    }

    pP->Count = 0;
#ifdef TERRAIN_TIMERS
    OpenTime.Stop();
#endif
}

//==============================================================================

void terr::CloseD3DPass(void)
{
#ifdef TERRAIN_TIMERS
    CloseTime.Start();
#endif
    d3dpass* pP = &s_pPass[s_PassBufferIndex];

    // Render buffers
    for( s32 i=0; i<m_NPasses; i++ )
    {
        // Setup material and blend mode
        if(1)
        {
//            vram_Activate( m_Material[i].BMP );
            if( i==0 ) g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
            else   
            if( i==1 ) g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
        }

        // Unlock the vertex buffer
        pP->pVertexBuffer[i]->Unlock();

        if( pP->Count > 0 )
        {
            // Set as active and render
            g_pd3dDevice->SetStreamSource( 0, pP->pVertexBuffer[i], sizeof(d3dvert) );

        
#ifdef TERRAIN_TIMERS
            DrawPrimTime.Start();
#endif
            //g_pd3dDevice->DrawPrimitive  ( D3DPT_POINTLIST, 0, 81*pP->Count );
            //g_pd3dDevice->DrawIndexedPrimitive  ( D3DPT_TRIANGLELIST, 0, 81*pP->Count, 0, 128*pP->Count );

            // Without this check, D3D can crash when the device is lost (due to alt+tab)
            if (g_pd3dDevice->TestCooperativeLevel() == D3D_OK)
                g_pd3dDevice->DrawIndexedPrimitive  ( D3DPT_TRIANGLELIST, 0, 81*pP->Count, 0, 128*pP->Count );

#ifdef TERRAIN_TIMERS
            DrawPrimTime.Stop();
#endif
        }
    }

    // Move to next pass
    s_PassBufferIndex = (s_PassBufferIndex+1)%NUM_PASS_BUFFERS;

#ifdef TERRAIN_TIMERS
    CloseTime.Stop();
#endif
}

//==============================================================================

void terr::EmitBlocks( void )
{
    s32 i;
    s32 MinZ,MinX,ClipMask;
    vector3 Min;
    vector3 Max;

#ifdef TERRAIN_TIMERS
EmitTime.Start();
#endif

    // Do non-clipped blocks first
    OpenD3DPass();
    for( i=0; i<s_NNCBlockRefs; i++ )
    {
        MinZ     = s_NCBlockRef[i].BZ;
        MinX     = s_NCBlockRef[i].BX;
        ClipMask = s_NCBlockRef[i].ClipMask;

        // Get BBOX bounds
        Min.Z = MinZ * 8.0f;
        Min.X = MinX * 8.0f;
        Min.Y = 0;
        
        Max.Z = (MinZ+8) * 8.0f;
        Max.X = (MinX+8) * 8.0f;
        GetQuadBounds( MinZ, MinX, 3, Min.Y, Max.Y );

        // Get distance to block
        f32 NDistSquared = NearestDistSquared( Min, Max );
        
        s32 BZ = (((MinZ+65536)&255)>>3);
        s32 BX = (((MinX+65536)&255)>>3);

        ShipBlockNoClip(BZ,BX,Min,Max,NDistSquared);
    }
    CloseD3DPass();


    // Do clipped blocks
    OpenD3DPass();
    for( i=0; i<s_NCBlockRefs; i++ )
    {
        MinZ     = s_CBlockRef[i].BZ;
        MinX     = s_CBlockRef[i].BX;
        ClipMask = s_CBlockRef[i].ClipMask;

        // Get BBOX bounds
        Min.Z = MinZ * 8.0f;
        Min.X = MinX * 8.0f;
        Min.Y = 0;
        
        Max.Z = (MinZ+8) * 8.0f;
        Max.X = (MinX+8) * 8.0f;
        GetQuadBounds( MinZ, MinX, 3, Min.Y, Max.Y );

        // Get distance to block
        f32 NDistSquared = NearestDistSquared( Min, Max );
        
        s32 BZ = (((MinZ+65536)&255)>>3);
        s32 BX = (((MinX+65536)&255)>>3);

        ShipBlockNoClip(BZ,BX,Min,Max,NDistSquared);
    }
    CloseD3DPass();

#ifdef TERRAIN_TIMERS
EmitTime.Stop();
#endif

}

//==============================================================================

void terr::ShipBlockNoClip( s32 BZ, s32 BX,
                          const vector3& Min,
                          const vector3& Max,
                          f32 NDistSquared )
{
    s32 z,x;

    // If this pass buffer is full, close and open a new one
    if( s_pPass[s_PassBufferIndex].Count == NUM_D3D_BLOCKS )
    {
        CloseD3DPass();
        OpenD3DPass();
    }

#ifdef TERRAIN_TIMERS
    VertBuildTime.Start();
#endif
    
    // Get which block we are supposed to render
    ASSERT( (BZ>=0) && (BZ<32) && (BX>=0) && (BX<32) );
    block* pBlock = &m_Block[BZ+BX*32];
    d3dpass* pPass = &s_pPass[s_PassBufferIndex];

    // Loop through material passes
    for( s32 P=0; P<m_NPasses; P++ )
    {
        s16* pHeight = pBlock->Height; 
        d3dvert* pV  = ((d3dvert*)pPass->pBlock[P]) + (pPass->Count*81);
        byte*    pA  = pBlock->Alpha[P].Alpha;

        for( x=0; x<9; x++ )
        for( z=0; z<9; z++ )
        {
            byte A = (*pA)<<1;
            pV->P.Z = Min.Z + z*8.0f;
            pV->P.X = Min.X + x*8.0f;
            pV->P.Y = (*pHeight) * (1.0f/HEIGHT_PREC);
            pV->C.R = A;
            pV->C.G = A;
            pV->C.B = A;
            pV->C.A = 255;
            pV->T.X = 0.125f*z;
            pV->T.Y = 0.125f*x;
            pV++;
            pA++;
            pHeight++;
        }
    }

#ifdef TERRAIN_TIMERS
    VertBuildTime.Stop();
#endif

    pPass->Count++;
}

#endif

//==============================================================================
//==============================================================================
//==============================================================================
// END OF D3D RENDERING
//==============================================================================
//==============================================================================
//==============================================================================

//==============================================================================
//==============================================================================
//==============================================================================
// START OF LINUX RENDERING
//==============================================================================
//==============================================================================
//==============================================================================
#ifdef TARGET_LINUX
//==============================================================================

void terr::EmitBlocks( void )
{
}

//==============================================================================

void terr::ShipBlockNoClip( s32 BZ, s32 BX,
                          const vector3& Min,
                          const vector3& Max,
                          f32 NDistSquared )
{
}

#endif

//==============================================================================
//==============================================================================
//==============================================================================
// END OF LINUX DUMMY RENDERING
//==============================================================================
//==============================================================================
//==============================================================================
terr::~terr         ( void )
{
#ifdef TARGET_PC
    ShutdownD3D();
#endif
}

//==============================================================================

void terr::Render( xbool DoFog, xbool DoDetail )
{
    //TIME_Push();
/*
    vram_Activate( m_DetailMat.BMP );
    draw_Begin(DRAW_SPRITES, DRAW_2D|DRAW_TEXTURED);
    draw_SpriteUV(vector3(0,0,0),vector2(128,128),vector2(0,0),vector2(1,1),XCOLOR_WHITE);
    draw_End();
*/

    s32                 i,j;
    terr_stack_node     StackNode[32];
    s32                 NStackNodes;
    terr_stack_node*    TopNode;
    s32                 NodesRendered=0;
    s32                 NodesProcessed=0;
#ifdef TARGET_LINUX
    f32                 VisDistance = 400;
#endif
#ifdef TARGET_PC
    f32                 VisDistance = 400;
#endif
#ifdef TARGET_PS2
    f32                 VisDistance = m_Fog->GetVisDistance()+PS2TERRAIN_VISDISTFUDGE;
#endif

#ifdef TERRAIN_TIMERS
    xtimer VisTime;
    xtimer TotalTime;
#endif

#ifdef TARGET_PC
    SetupD3D();
#ifdef TERRAIN_TIMERS
    TotalTime.Start();
    OpenTime.Reset();
    CloseTime.Reset();
    VisTime.Reset();
    DrawPrimTime.Reset();
    VertBuildTime.Reset();
#endif
#endif
#ifdef TERRAIN_TIMERS
    TotalTime.Reset();
    TotalTime.Start();
    EmitTime.Reset();
    EmitTimeNonClipped.Reset();
    EmitTimeClipped.Reset();
#endif


    STAT_TotalPassesRendered = 0;
    s_NNCBlockRefs=0;
    s_NCBlockRefs=0;
    m_DoFog = DoFog;
    m_DoDetail = DoDetail;

    // Upload microcode
#ifdef TARGET_PS2
    eng_ActivateMicrocode(s_MCodeHandle);
    vram_Flush();
#endif

    // Render terrain for all active views
    s32 NViews = eng_GetNActiveViews();
    for( s32 V=0; V<NViews; V++ )
    {
#ifdef TERRAIN_TIMERS
        VisTime.Start();
#endif
        const view* pView = eng_GetActiveView(V);

#ifdef TARGET_PC
        // Setup D3D settings
        matrix4 W2V = pView->GetW2V();
        matrix4 V2C = pView->GetV2C();
        matrix4 L2W;
        L2W.Identity();
        eng_SetViewport( *pView );
        //g_pd3dDevice->SetRenderState    ( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
        g_pd3dDevice->SetRenderState    ( D3DRS_CULLMODE, D3DCULL_NONE );
        g_pd3dDevice->SetTransform      ( D3DTS_VIEW,             (D3DMATRIX*)&W2V );
        g_pd3dDevice->SetTransform      ( D3DTS_PROJECTION,       (D3DMATRIX*)&V2C );
        g_pd3dDevice->SetTransform      ( D3DTS_WORLDMATRIX(0),   (D3DMATRIX*)&L2W );
        g_pd3dDevice->SetRenderState    ( D3DRS_ALPHABLENDENABLE, TRUE );
        g_pd3dDevice->SetRenderState    ( D3DRS_SRCBLEND,         D3DBLEND_SRCALPHA );
        g_pd3dDevice->SetRenderState    ( D3DRS_DESTBLEND,        D3DBLEND_ZERO );
        g_pd3dDevice->SetVertexShader   ( D3DFVF_DIFFUSE|D3DFVF_XYZ|D3DFVF_TEX1 );

        // Setup texture pixel blending
		g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR  );
		g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR  );
		g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR  );
		g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
		g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );

        // Setup indices
        g_pd3dDevice->SetIndices        ( s_pD3DIndexBuffer, 0 );
//        vram_Activate( m_Material[0].BMP );
        g_pd3dDevice->SetTexture( 0, 0 );
#endif

        // Get view information
        m_EyePos = pView->GetPosition();

        // Compute the mipks
        //m_BaseMipK = VRAM_ComputeMipK( BASE_WORLD_PIXEL_SIZE );
        //m_DetailMipK = VRAM_ComputeMipK( DETAIL_WORLD_PIZEL_SIZE );
        // Upload microcode, matrices, and fog params
#ifdef TARGET_PS2
        m_BaseMipK = vram_ComputeMipK( TERRAIN_BASE_WORLD_PIXEL_SIZE, *pView );
        m_DetailMipK = vram_ComputeMipK( m_DetailWorldPixelSize, *pView );
        BootAndLoadMatrices( pView );
#endif


        // Get planes to cull against
        pView->GetViewPlanes( m_ClipPlane[3],//T
                              m_ClipPlane[2],//B
                              m_ClipPlane[0],//L
                              m_ClipPlane[1],//R
                              m_ClipPlane[4],//N
                              m_ClipPlane[5],//F
                              view::WORLD);

        // Modify near and far culling planes so near is on the eye
        // and far is the visibility dist away, not the view's far plane
        m_ClipPlane[4].D = -m_ClipPlane[4].Dot(m_EyePos);
        vector3 FarPt = m_EyePos + m_ClipPlane[4].Normal*VisDistance;
        m_ClipPlane[5].D = -m_ClipPlane[5].Dot(FarPt);

        // Get planes to clip against
        plane DummyPlane;
        pView->GetViewPlanes(-1000.0f,
                             -1000.0f,
                              1000.0f,
                              1000.0f,
                              m_ClipPlane[9],   //T
                              m_ClipPlane[8],   //B
                              m_ClipPlane[6],   //L
                              m_ClipPlane[7],   //R
                              m_ClipPlane[10],  //N
                              DummyPlane,       //F
                              view::WORLD);

        // Build bbox indices
        j=0;
        for( i=0; i<TERR_NUM_PLANES; i++ )
        {
            m_ClipPlane[i].GetBBoxIndices( m_ClipMinIndex+j, m_ClipMaxIndex+j );
            j+=3;
        }

        //
        // Compute DetailDist
        //
        {
            f32 ScreenDist = pView->GetScreenDist();
            m_DetailDist = s_DetailDistTweak*ScreenDist;
        }

        // Loop through the 9 instances of the terrain and place on stack
        s32 EyeBlockZ,EyeBlockX;
        if( m_EyePos.Z >= 0 ) EyeBlockZ = (s32)((m_EyePos.Z/2048.0f)+0.5f);
        else                  EyeBlockZ = (s32)((m_EyePos.Z/2048.0f)-1.5f);
        if( m_EyePos.X >= 0 ) EyeBlockX = (s32)((m_EyePos.X/2048.0f)+0.5f);
        else                  EyeBlockX = (s32)((m_EyePos.X/2048.0f)-1.5f);

        //ENG_PrintfXY(20,0,"%1d %1d  %1f %f",EyeBlockX,EyeBlockY,(EyePos.X/2048.0f),(EyePos.Y/2048.0f));

        NStackNodes=0;
        TopNode = StackNode;
        for( s32 dx=-1; dx<=1; dx++)
        for( s32 dz=-1; dz<=1; dz++)
        {
            TopNode->Offset.Z = (EyeBlockZ + dz) * 2048.0f;
            TopNode->Offset.X = (EyeBlockX + dx) * 2048.0f;
            TopNode->Offset.Y = 0;
            TopNode->MinZ     = (EyeBlockZ+dz)*256;
            TopNode->MaxZ     = TopNode->MinZ + 256;
            TopNode->MinX     = (EyeBlockX+dx)*256;
            TopNode->MaxX     = TopNode->MinX + 256;
            GetQuadBounds(0,0,8,TopNode->MinY,TopNode->MaxY);
            TopNode->Level    = 8;
            TopNode->ClipMask = (1<<TERR_NUM_PLANES)-1;

            NStackNodes++;
            TopNode++;
        }

        if( !PS2TERRAIN_LOCKOCCLUSION )
            m_OccPos = m_EyePos;

        // Process and emit blocks from stack
        while( NStackNodes )
        {
            NodesProcessed++;
            TopNode = StackNode + NStackNodes - 1;

            // Build bbox for node
            bbox BBox;
            BBox.Min.Z = TopNode->MinZ*8.0f;
            BBox.Min.X = TopNode->MinX*8.0f;
            BBox.Min.Y = TopNode->MinY;
            BBox.Max.Z = TopNode->MaxZ*8.0f;
            BBox.Max.X = TopNode->MaxX*8.0f;
            BBox.Max.Y = TopNode->MaxY;

            // Check if node is in view
            s32 NewClipMask = IsBoxInView( BBox, TopNode->ClipMask );
            if( NewClipMask==-1 )
            {
                NStackNodes--;
                continue;
            }

            // If no more planes to clip to we know we are inside view
            // Loop through and emit blocks
            if( NewClipMask == 0 )
            {
                for( s32 x=TopNode->MinX; x<TopNode->MaxX; x+=8 )
                for( s32 z=TopNode->MinZ; z<TopNode->MaxZ; z+=8 )
                {
                    if( PS2TERRAIN_USEOCCLUSION )
                    {
                        bbox BBox;
                        BBox.Min.Z = z*8.0f;
                        BBox.Min.X = x*8.0f;
                        BBox.Max.Z = BBox.Min.Z + 64.0f;
                        BBox.Max.X = BBox.Min.X + 64.0f;
                        GetQuadBounds( z, x, 3, BBox.Min.Y, BBox.Max.Y );

                        if( IsOccluded( BBox, m_OccPos ) )
                        {
                            continue;
                        }
                    }

                    NodesRendered++;
                    s32 BI = ((z>>3)&31) + (((x>>3)&31)<<5);
                    if( m_Block[BI].SquareMask || PS2TERRAIN_FORCEALLCLIPPED )
                    {
                        s_CBlockRef[s_NCBlockRefs].BZ = (s16)z;
                        s_CBlockRef[s_NCBlockRefs].BX = (s16)x;
                        s_CBlockRef[s_NCBlockRefs].ClipMask = (u16)NewClipMask;
                        s_NCBlockRefs++;
                    }
                    else
                    {
                        s_NCBlockRef[s_NNCBlockRefs].BZ = (s16)z;
                        s_NCBlockRef[s_NNCBlockRefs].BX = (s16)x;
                        s_NCBlockRef[s_NNCBlockRefs].ClipMask = (u16)NewClipMask;
                        s_NNCBlockRefs++;
                    }
                }

                NStackNodes--;
                continue;
            }

            // Check if we have recursed enough
            if( TopNode->Level == 3 )
            {
                NodesRendered++;

                s32 BI = ((TopNode->MinZ>>3)&31) + (((TopNode->MinX>>3)&31)<<5);
                xbool Occluded = FALSE;

                if( PS2TERRAIN_USEOCCLUSION )
                {
                    bbox BBox;
                    BBox.Min.Z = TopNode->MinZ*8.0f;
                    BBox.Min.X = TopNode->MinX*8.0f;
                    BBox.Min.Y = TopNode->MinY;
                    BBox.Max.Z = BBox.Min.Z + 64.0f;
                    BBox.Max.X = BBox.Min.X + 64.0f;
                    BBox.Max.Y = TopNode->MaxY;

                    Occluded = IsOccluded( BBox, m_OccPos );
                }

                if( !Occluded )
                {
                    if( (NewClipMask & ((1<<6)|(1<<7)|(1<<8)|(1<<9)|(1<<10))) ||
                        (m_Block[BI].SquareMask) ||
                        PS2TERRAIN_FORCEALLCLIPPED )
                    {
                        s_CBlockRef[s_NCBlockRefs].BZ = (s16)TopNode->MinZ;
                        s_CBlockRef[s_NCBlockRefs].BX = (s16)TopNode->MinX;
                        s_CBlockRef[s_NCBlockRefs].ClipMask = (u16)NewClipMask;
                        s_NCBlockRefs++;
                    }
                    else
                    {
                        s_NCBlockRef[s_NNCBlockRefs].BZ = (s16)TopNode->MinZ;
                        s_NCBlockRef[s_NNCBlockRefs].BX = (s16)TopNode->MinX;
                        s_NCBlockRef[s_NNCBlockRefs].ClipMask = (u16)NewClipMask;
                        s_NNCBlockRefs++;
                    }
                }

                NStackNodes--;
                continue;
            }

            // Subdivide and continue
            vector3 Offset;
            s32 MinX,MidX,MaxX;
            s32 MinZ,MidZ,MaxZ;
            s32 Level;

            Offset = TopNode->Offset;
            Level = TopNode->Level-1;
            MinZ = TopNode->MinZ;
            MaxZ = TopNode->MaxZ;
            MinX = TopNode->MinX;
            MaxX = TopNode->MaxX;
            MidZ = (MinZ + MaxZ)>>1;
            MidX = (MinX + MaxX)>>1;

            TopNode->Offset   = Offset;
            TopNode->MinZ     = MinZ;
            TopNode->MaxZ     = MidZ;
            TopNode->MinX     = MinX;
            TopNode->MaxX     = MidX;
            TopNode->Level    = Level;
            TopNode->ClipMask = NewClipMask;
            GetQuadBounds(TopNode->MinZ,TopNode->MinX,Level,TopNode->MinY,TopNode->MaxY);
            TopNode++;

            TopNode->Offset   = Offset;
            TopNode->MinZ     = MidZ;
            TopNode->MaxZ     = MaxZ;
            TopNode->MinX     = MinX;
            TopNode->MaxX     = MidX;
            TopNode->Level    = Level;
            TopNode->ClipMask = NewClipMask;
            GetQuadBounds(TopNode->MinZ,TopNode->MinX,Level,TopNode->MinY,TopNode->MaxY);
            TopNode++;

            TopNode->Offset   = Offset;
            TopNode->MinZ     = MinZ;
            TopNode->MaxZ     = MidZ;
            TopNode->MinX     = MidX;
            TopNode->MaxX     = MaxX;
            TopNode->Level    = Level;
            TopNode->ClipMask = NewClipMask;
            GetQuadBounds(TopNode->MinZ,TopNode->MinX,Level,TopNode->MinY,TopNode->MaxY);
            TopNode++;

            TopNode->Offset   = Offset;
            TopNode->MinZ     = MidZ;
            TopNode->MaxZ     = MaxZ;
            TopNode->MinX     = MidX;
            TopNode->MaxX     = MaxX;
            TopNode->Level    = Level;
            TopNode->ClipMask = NewClipMask;
            GetQuadBounds(TopNode->MinZ,TopNode->MinX,Level,TopNode->MinY,TopNode->MaxY);

            NStackNodes += 3;
        }

        DEMAND( s_NNCBlockRefs < MAX_BLOCK_REFS );
        DEMAND( s_NCBlockRefs  < MAX_BLOCK_REFS );

#ifdef TERRAIN_TIMERS
        VisTime.Stop();
        //x_printfxy(0,10,"TerrDecideBlocks: %f",TIME_PopMs());
#endif

        //TIME_Push();
        EmitBlocks();
        //x_printfxy(0,2+V,"NCBlocks %1d  CBlocks %1d",s_NNCBlockRefs,s_NCBlockRefs);
    }
#ifdef TERRAIN_TIMERS
    TotalTime.Stop();
#endif

#ifdef TARGET_PC
    if( 0 )
    {
        s32 L=5;
#ifdef TERRAIN_TIMERS
        x_printfxy(0,L++,"OpenTime        %5.2f",OpenTime.ReadMs());
        x_printfxy(0,L++,"CloseTime       %5.2f",CloseTime.ReadMs());
        x_printfxy(0,L++,"EmitTime        %5.2f",EmitTime.ReadMs());
        x_printfxy(0,L++,"VertBuildTime   %5.2f",VertBuildTime.ReadMs());
        x_printfxy(0,L++,"VisTime         %5.2f",VisTime.ReadMs());
        x_printfxy(0,L++,"DrawPrimTime    %5.2f",DrawPrimTime.ReadMs());
        x_printfxy(0,L++,"TotalTime       %5.2f",TotalTime.ReadMs());
#endif
    }
#endif
#ifdef TARGET_PS2
#ifdef athyssen
    if( PS2TERRAIN_SHOWSTATS )
    {
        s32 L=2;
#ifdef TERRAIN_TIMERS
        x_printfxy(0,L++,"VisTime          %5.2f",VisTime.ReadMs());
        x_printfxy(0,L++,"EmitTime         %5.2f",EmitTime.ReadMs());
        x_printfxy(0,L++,"EmitTimeNClipped %5.2f",EmitTimeNonClipped.ReadMs());
        x_printfxy(0,L++,"EmitTimeClipped  %5.2f",EmitTimeClipped.ReadMs());
        x_printfxy(0,L++,"TotalTime        %5.2f",TotalTime.ReadMs());
        x_printfxy(0,L++,"Blocks           %1d %1d %1d",s_NCBlockRefs,s_NNCBlockRefs,s_NCBlockRefs+s_NNCBlockRefs);
#endif
    }
#endif
#endif

}

//==============================================================================
//==============================================================================
//==============================================================================
// BEGIN PS2 RENDERING
//==============================================================================
//==============================================================================
//==============================================================================
//==============================================================================
xbool g_FlickerTerrainHoles = FALSE;

#ifdef TARGET_PS2

void terr::BootAndLoadMatrices( const view* pView )
{
    struct fogdata
    {
        vector4 EyePos;
        vector4 Const0;
        vector4 Const1;
    };
    struct boot
    {
        dmatag          DMA;
        u32             VIF[4];
        u32             CMD[4];     // Matrix load command
        matrix4         L2S;        // Local to screen
        matrix4         L2C;        // Local to clip
        matrix4         C2S;        // Clip to screen
        u32             VIF2[4];    // Kick and upack for fog values
        fogdata         Fog;
    };

//;       0 - Eye position
//;       1 - Fog color
//;       2 - HazeStartDist HazeEndDist
//;       3 - HC0, HC1, HC2

    boot* pBoot = DLStruct(boot);
    pBoot->DMA.SetCont( sizeof(boot) - sizeof(dmatag) );
    pBoot->DMA.PAD[0] = SCE_VIF1_SET_BASE    (0,   0);
    pBoot->DMA.PAD[1] = SCE_VIF1_SET_OFFSET  (0,   0);

    // Setup double buffering and matrix unpack command
    pBoot->VIF[0] = VIF_SkipWrite(4,0);
    pBoot->VIF[1] = SCE_VIF1_SET_MSCAL  (0,   0);
    pBoot->VIF[2] = SCE_VIF1_SET_FLUSH  (0);
    pBoot->VIF[3] = VIF_Unpack( 0, 13, VIF_V4_32, 0, 0, 0 );

    // Setup CMD data
    pBoot->CMD[0] = 2;  // opcode load matrices
    pBoot->CMD[1] = 0;  // offset
    pBoot->CMD[2] = 0;  // param1
    pBoot->CMD[3] = 0;  // param2

    // Get matrices from engine
    pBoot->L2S = pView->GetW2S();
    pBoot->L2C = pView->GetW2C();
    pBoot->C2S = pView->GetC2S();

    // Setup kick
    pBoot->VIF2[0] = SCE_VIF1_SET_MSCNT(0);
    pBoot->VIF2[1] = 0;
    pBoot->VIF2[2] = 0;
    pBoot->VIF2[3] = VIF_Unpack( 1007, sizeof(fogdata)/16, VIF_V4_32, 0, 0, 1 );

    f32 Dist0,Dist1;
    f32 DeltaY0,DeltaY1;
    m_Fog->GetFogConst( Dist0, Dist1, DeltaY0, DeltaY1 );

    pBoot->Fog.EyePos  = vector4(m_EyePos.X,m_EyePos.Y,m_EyePos.Z,1);
    pBoot->Fog.Const0  = vector4(Dist0,DeltaY0,0,0);
    pBoot->Fog.Const1  = vector4(Dist1,DeltaY1,1,1);

}


//==============================================================================

struct block_c_pack
{
    dmatag      SquareMaskDMA;
    u32         SquareMask[8];
    dmatag      DMA;
    u32         CMD[4];
    vector4     XYZBase;
    vector4     XYZDelta;
    vector4     UVBase;
    vector4     UVDelta;
    giftag      GifTagNC;
    giftag      GifTagC;
    s32         DetailColor[4];
    dmatag      DMARef;
    dmatag      DMAKick;    // Builds verts/uvs/fog and blocks until done
};

struct alpha_c_pack
{
    dmatag      DMA;
    u32         CMD[4];  // Alpha opcode
    dmatag      DMARef;  // Reference to the alpha data
    dmatag      DMAKick; // Kick microcode and flush
};

struct detail_c_pack
{
    dmatag      DMA;
    u32         CMD[4];
    u32         VIF[4];
};

struct fog_c_pack
{
    dmatag      DMA;
    u32         CMD[4];
    giftag      GifTag1;
    giftag      GifTag2;
    u32         VIF[4];
};

//==============================================================================

void terr::ShipBlockClip( s32 BZ, s32 BX, const vector3& Min, const vector3& Max, f32 NDistSquared )
{
    (void)Max;
#ifdef SHOW_BBOX
    draw_BBox( bbox(Min,Max), XCOLOR_RED );
#endif

    //
    // Build block pack to create/transform/fog verts
    //
    ASSERT( (BZ>=0) && (BZ<32) && (BX>=0) && (BX<32) );
    block* pBlock = m_Block + BZ+BX*32;

    block_c_pack* pPack = DLStruct(block_c_pack);

    pPack->SquareMaskDMA.SetCont( 8*4 );
    pPack->SquareMaskDMA.PAD[1] = VIF_Unpack(1010,8,VIF_S_32,FALSE,FALSE,TRUE);

    // Be sure it is in the first iteration of the terrain
    if( (Min.X <    0.0f) || 
        (Min.Z <    0.0f) || 
        (Max.X > 2048.0f) || 
        (Max.Z > 2048.0f) || 
        (g_FlickerTerrainHoles && (x_rand() & 0x01)) )
    {
        for( s32 i=0; i<8; i++ )
            pPack->SquareMask[i] = 0;
    }
    else
    {
        for( s32 i=0; i<8; i++ )
            pPack->SquareMask[i] = ((byte*)(&pBlock->SquareMask))[7-i];
    }


    pPack->DMA.SetCont( 8*16 );
    pPack->DMA.PAD[1] = VIF_Unpack(0,8,VIF_V4_32,FALSE,FALSE,TRUE);

    pPack->CMD[0] = 8; // Opcode for clipped block processing
    pPack->CMD[1] = 0;
    pPack->CMD[2] = 0;
    pPack->CMD[3] = 0;

    pPack->XYZBase.X  = Min.X;
    pPack->XYZBase.Y  = 0;//Min.Y;
    pPack->XYZBase.Z  = Min.Z;
    pPack->XYZBase.W  = 1.0f;

    pPack->XYZDelta.X = 8.0f;
    pPack->XYZDelta.Y = 0.0f;
    pPack->XYZDelta.Z = 8.0f;
    pPack->XYZDelta.W = 0.0f;

    pPack->UVBase.X  = 0;
    pPack->UVBase.Y  = 0;
    pPack->UVBase.Z  = 1;
    pPack->UVBase.W  = 1;

    pPack->UVDelta.X = 0.125f;
    pPack->UVDelta.Y = 0.125f;
    pPack->UVDelta.Z = 0;
    pPack->UVDelta.W = 0;

    pPack->GifTagNC.Build2(3,0,GIF_PRIM_TRIANGLE,GIF_FLAG_TEXTURE|GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
    pPack->GifTagNC.Reg(GIF_REG_ST,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;

    pPack->GifTagC.Build2(3,0,GIF_PRIM_TRIANGLEFAN,GIF_FLAG_TEXTURE|GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
    pPack->GifTagC.Reg(GIF_REG_ST,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;

    if( PS2TERRAIN_DETAILPASS && m_DoDetail && (NDistSquared < (m_DetailDist*m_DetailDist)) )
    {
        pPack->DetailColor[0] = 128;
        pPack->DetailColor[1] = 128;
        pPack->DetailColor[2] = 128;
        pPack->DetailColor[3] = 128;
    }
    else
    {
        pPack->DetailColor[0] = m_DetailMipColor.R;
        pPack->DetailColor[1] = m_DetailMipColor.G;
        pPack->DetailColor[2] = m_DetailMipColor.B;
        pPack->DetailColor[3] = m_DetailMipColor.A;
    }

    pPack->DMARef.SetRef(176,(u32)pBlock->Height);
    pPack->DMARef.PAD[1] = VIF_Unpack(8,81,VIF_S_16,TRUE,FALSE,TRUE);

    pPack->DMAKick.SetCont(0);
    pPack->DMAKick.PAD[0] = SCE_VIF1_SET_MSCNT(0);
    pPack->DMAKick.PAD[1] = SCE_VIF1_SET_FLUSH(0);

    //
    // Loop through materials and build alpha packs
    //
    vram_SetMipEquation(m_BaseMipK);
    s32 NPassesRendered=0;
    for( s32 M=0; M<m_NPasses; M++ )
    if( pBlock->PassMask & (1<<M) )
    {
        vram_Activate( m_Material[M].BMP );

        if( NPassesRendered==0 ) 
        {
            gsreg_Begin();
            gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_ZERO) );
            gsreg_End();
        }
        else       
        if( NPassesRendered==1 ) 
        {
            gsreg_Begin();
            gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
            gsreg_End();
        }

        alpha_c_pack* pPack = DLStruct(alpha_c_pack);

        pPack->DMA.SetCont(16*1);
        pPack->DMA.PAD[1] = VIF_Unpack(0,1,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 10; // Opcode for block_c_alpha processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        pPack->DMARef.SetRef( sizeof(block_alpha), (u32)&pBlock->Alpha[M] );
        pPack->DMARef.PAD[1] = VIF_Unpack(1,81,VIF_S_8,FALSE,FALSE,TRUE);

        pPack->DMAKick.SetCont(0);
        pPack->DMAKick.PAD[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->DMAKick.PAD[1] = SCE_VIF1_SET_FLUSH(0);

        NPassesRendered++;
    }
    STAT_TotalPassesRendered += NPassesRendered;

    if( PS2TERRAIN_DETAILPASS && m_DoDetail && (NDistSquared < (m_DetailDist*m_DetailDist)) )
    {
        vram_SetMipEquation( m_DetailMipK );
        vram_Activate( m_DetailMat.BMP );

        gsreg_Begin();
        gsreg_SetMipEquation( FALSE, m_DetailMipK, m_DetailMat.BMP.GetNMips(), MIP_MAG_BILINEAR, MIP_MIN_INTERP_MIPMAP_BILINEAR );
        gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_DST,C_ZERO,A_SRC,C_ZERO) );
        gsreg_End();

        detail_c_pack* pPack = DLStruct(detail_c_pack);

        pPack->DMA.SetCont(sizeof(detail_c_pack)-sizeof(dmatag));
        pPack->DMA.PAD[1] = VIF_Unpack(0,1,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 16; // Opcode for detail pass processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        pPack->VIF[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->VIF[1] = SCE_VIF1_SET_FLUSH(0);
        pPack->VIF[2] = 0;
        pPack->VIF[3] = 0;
    }

    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_DST,A_SRC,C_DST) );
    gsreg_End();
/*
    // Build fog pass
    if( m_DoFog )
    {
        fog_c_pack* pPack = DLStruct(fog_c_pack);

        pPack->DMA.SetCont(sizeof(fog_c_pack)-sizeof(dmatag));
        pPack->DMA.PAD[1] = VIF_Unpack(0,3,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 14; // Opcode for fog pass processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        pPack->GifTag1.Build2(3,10,GIF_PRIM_TRIANGLE,GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
        pPack->GifTag1.Reg(GIF_REG_NOP,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;

        pPack->GifTag2.Build2(3,10,GIF_PRIM_TRIANGLEFAN,GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
        pPack->GifTag2.Reg(GIF_REG_NOP,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;

        pPack->VIF[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->VIF[1] = SCE_VIF1_SET_FLUSH(0);
        pPack->VIF[2] = 0;
        pPack->VIF[3] = 0;
    }
*/

    // Build fog pass
    if( m_DoFog && PS2TERRAIN_FOG && PS2TERRAIN_FOGPASS )
    {
/*
        vram_Activate( m_Fog->GetBMP() );
        gsreg_Begin();
        gsreg_SetClamping(TRUE);
        gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_DST,A_SRC,C_DST) );
        gsreg_End();
*/

        // SB - Clut activation is shared by context!
        if (m_Fog->GetBMP().IsClutBased())
        {
            eng_PushGSContext(1);
            vram_Activate( m_Fog->GetBMP() );
            eng_PopGSContext() ;
        }

        fog_c_pack* pPack = DLStruct(fog_c_pack);

        pPack->DMA.SetCont(sizeof(fog_c_pack)-sizeof(dmatag));
        pPack->DMA.PAD[1] = VIF_Unpack(0,3,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 14; // Opcode for fog pass processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        pPack->GifTag1.Build2(3,10,GIF_PRIM_TRIANGLE,GIF_FLAG_TEXTURE|GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA|GIF_FLAG_CONTEXT);
        pPack->GifTag1.Reg(GIF_REG_ST,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;

        pPack->GifTag2.Build2(3,10,GIF_PRIM_TRIANGLEFAN,GIF_FLAG_TEXTURE|GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA|GIF_FLAG_CONTEXT);
        pPack->GifTag2.Reg(GIF_REG_ST,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;

        pPack->VIF[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->VIF[1] = SCE_VIF1_SET_FLUSH(0);
        pPack->VIF[2] = 0;
        pPack->VIF[3] = 0;
/*
        gsreg_Begin();
        gsreg_SetClamping(FALSE);
        gsreg_End();
*/
    }

}

//==============================================================================

struct block_nc_pack
{
    dmatag      DMA;
    u32         CMD[4];
    vector4     XYZBase;
    vector4     XYZDelta;
    vector4     UVBase;
    vector4     UVDelta;
    giftag      GifTag;
    s32         DetailColor[4];
    dmatag      DMARef;
    dmatag      DMAKick;    // Builds verts/uvs/fog and blocks until done
} __attribute__((aligned (16)));

struct alpha_nc_pack
{
    dmatag      DMA;
    u32         CMD[4];  // Alpha opcode
    u32         FOGRange[4];    // Interlaced fog range
    dmatag      DMARef;  // Reference to the alpha data
    dmatag      DMAKick; // Kick microcode and flush
};

struct fog_nc_pack
{
    dmatag      DMA;
    u32         CMD[4];
    giftag      GifTag;
    u32         VIF[4];
};

//==============================================================================

static xbool         s_NCPacksInited=FALSE;
static block_nc_pack s_BlockNCPack;

void InitNCPacks( void )
{
    s_NCPacksInited = TRUE;

    block_nc_pack* pPack = &s_BlockNCPack;

    pPack->DMA.SetCont( 7*16 );
    pPack->DMA.PAD[1] = VIF_Unpack(0,7,VIF_V4_32,FALSE,FALSE,TRUE);

    pPack->CMD[0] = 4; // Opcode for block processing
    pPack->CMD[1] = 0;
    pPack->CMD[2] = 0;
    pPack->CMD[3] = 0;

    pPack->XYZBase.X  = 0;
    pPack->XYZBase.Y  = 0;
    pPack->XYZBase.Z  = 0;
    pPack->XYZBase.W  = 1.0f;
/*
    pPack->XYZDelta.X = 8.0f;
    pPack->XYZDelta.Y = 0.0f;
    pPack->XYZDelta.Z = 8.0f;
    pPack->XYZDelta.W = 0.0f;
*/
    pPack->XYZDelta.X = 64.0f;
    pPack->XYZDelta.Y = 0.0f;
    pPack->XYZDelta.Z = 64.0f;
    pPack->XYZDelta.W = 0.0f;

    pPack->UVBase.X  = 0;
    pPack->UVBase.Y  = 0;
    pPack->UVBase.Z  = 1;
    pPack->UVBase.W  = 1;

    pPack->UVDelta.X = 0.125f;
    pPack->UVDelta.Y = 0.125f;
    pPack->UVDelta.Z = 0;
    pPack->UVDelta.W = 0;

    pPack->GifTag.Build2(3,10,GIF_PRIM_TRIANGLEFAN,GIF_FLAG_TEXTURE|GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
    //pPack->GifTag.Build2(3,10,GIF_PRIM_TRIANGLEFAN,GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
    //pPack->GifTag.Build2(3,10,GIF_PRIM_LINESTRIP,GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA);
    pPack->GifTag.Reg(GIF_REG_ST,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;
    pPack->GifTag.EOP = 0;

    //pPack->DMARef.SetRef(176,(u32)pBlock->Height);
    //pPack->DMARef.PAD[1] = VIF_Unpack(6,81,VIF_S_16,TRUE,FALSE,TRUE);

    pPack->DMAKick.SetCont(0);
    pPack->DMAKick.PAD[0] = SCE_VIF1_SET_MSCNT(0);
    pPack->DMAKick.PAD[1] = SCE_VIF1_SET_FLUSH(0);
    
}

//==============================================================================

void terr::EmitBlocks( void )
{
    s32 i;
    s32 MinZ,MinX,ClipMask;
    vector3 Min;
    vector3 Max;
    
#ifdef TERRAIN_TIMERS
    EmitTime.Start();
#endif

    // Build dlist fragments
    if( PS2TERRAIN_USEFRAGMENTS )
    {
        s32 M;

        vram_SetMipEquation( m_BaseMipK );

        // Get textures loaded
        for( M=0; M<m_NPasses; M++ )
        {
            vram_SetMipEquation( m_BaseMipK );
            vram_Activate( m_Material[M].BMP );
        }
        vram_SetMipEquation( m_DetailMipK );
        vram_Activate( m_DetailMat.BMP );

        // Build activation fragments
        vram_SetMipEquation( m_BaseMipK );
        for( M=0; M<m_NPasses; M++ )
        {
            dlist DList( m_DListActivate[M].DList, sizeof(dlist_fragment) );
            eng_PushDList( DList );

            vram_Activate( m_Material[M].BMP );

            if( M==0 ) 
            {
                gsreg_Begin();
                gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_ZERO) );
                gsreg_End();
            }
            else       
            if( M==1 ) 
            {
                gsreg_Begin();
                gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
                gsreg_End();
            }

            dmatag* pDMA = DLStruct(dmatag);
            pDMA->SetRet(0);

            eng_PopDList();
        }
    }

    // Setup context2 for fogging
    {
        eng_PushGSContext(1);

        // Fog bmp is activated if it's a palette
        if (!m_Fog->GetBMP().IsClutBased())
            vram_Activate( m_Fog->GetBMP() );

        gsreg_Begin();
        gsreg_SetClamping(TRUE);
        gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_DST,A_SRC,C_DST) );
        gsreg_End();

        eng_PopGSContext();

        s_NoClipFogGiftag.Build2(3,10,GIF_PRIM_TRIANGLEFAN,GIF_FLAG_TEXTURE|GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA|GIF_FLAG_CONTEXT);
        s_NoClipFogGiftag.Reg(GIF_REG_ST,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;
        s_NoClipFogGiftag.EOP = 0;
    }
        
    // Setup UV upload
    {
        dmatag* pDMA = DLStruct(dmatag);
        pDMA->SetCont(16*81);
        pDMA->PAD[1] = VIF_Unpack(872,81,VIF_V4_32,FALSE,FALSE,TRUE);

        f32 U,V=0.0f;
        for( s32 i=0; i<9; i++ )
        {
            U=0.0f;
            for( s32 j=0; j<9; j++ )
            {
                vector4* pUV = DLStruct(vector4);
                pUV->X = U;
                pUV->Y = V;
                pUV->Z = 1.0f;
                pUV->W = 1.0f;
                U += 0.125f;
            }
            V += 0.125f;
        }
    }

    //
    // Do non-clipped blocks first
    //
    if( PS2TERRAIN_EMITNONCLIPPED )
    {
#ifdef TERRAIN_TIMERS
        EmitTimeNonClipped.Start();
#endif
        for( i=0; i<s_NNCBlockRefs; i++ )
        {
            MinZ     = s_NCBlockRef[i].BZ;
            MinX     = s_NCBlockRef[i].BX;
            ClipMask = s_NCBlockRef[i].ClipMask;

            // Get BBOX bounds
            Min.Z = MinZ * 8.0f;
            Min.X = MinX * 8.0f;
            Min.Y = 0;
        
            Max.Z = (MinZ+8) * 8.0f;
            Max.X = (MinX+8) * 8.0f;
            GetQuadBounds( MinZ, MinX, 3, Min.Y, Max.Y );

            // Get distance to block
            f32 NDistSquared = NearestDistSquared( Min, Max );
                
            s32 BZ = (((MinZ+65536)&255)>>3);
            s32 BX = (((MinX+65536)&255)>>3);

            if( (PS2TERRAIN_BLOCKZ == -1) ||
                ((BZ == PS2TERRAIN_BLOCKZ) &&
                 (BX == PS2TERRAIN_BLOCKX)) )
            ShipBlockNoClip(BZ,BX,Min,Max,NDistSquared);

        }
#ifdef TERRAIN_TIMERS
        EmitTimeNonClipped.Stop();
#endif
    }

    //
    // Do clipped blocks
    //
    if( PS2TERRAIN_EMITCLIPPED )
    {
        // boot clipper

        {
            struct boot_clipper
            {
                dmatag      DMA;
                u32         CMD[4];
                u32         VIF[4];
            };
            boot_clipper* pPack = DLStruct(boot_clipper);
            pPack->DMA.SetCont(16*2);
            pPack->DMA.PAD[1] = VIF_Unpack(0,1,VIF_V4_32,FALSE,FALSE,TRUE);
            pPack->CMD[0] = 20;
            pPack->CMD[1] = 0;
            pPack->CMD[2] = 0;
            pPack->CMD[3] = 0;
            pPack->VIF[0] = SCE_VIF1_SET_MSCNT(0);
            pPack->VIF[1] = SCE_VIF1_SET_FLUSH(0);
            pPack->VIF[2] = 0;
            pPack->VIF[3] = 0;
        }

#ifdef TERRAIN_TIMERS
        EmitTimeClipped.Start();
#endif
        for( i=0; i<s_NCBlockRefs; i++ )
        {
            MinZ     = s_CBlockRef[i].BZ;
            MinX     = s_CBlockRef[i].BX;
            ClipMask = s_CBlockRef[i].ClipMask;

            // Get BBOX bounds
            Min.Z = MinZ * 8.0f;
            Min.X = MinX * 8.0f;
            Min.Y = 0;
        
            Max.Z = (MinZ+8) * 8.0f;
            Max.X = (MinX+8) * 8.0f;
            GetQuadBounds( MinZ, MinX, 3, Min.Y, Max.Y );

            // Get distance to block
            f32 NDistSquared = NearestDistSquared( Min, Max );
        
            s32 BZ = (((MinZ+65536)&255)>>3);
            s32 BX = (((MinX+65536)&255)>>3);

            if( (PS2TERRAIN_BLOCKZ == -1) ||
                ((BZ == PS2TERRAIN_BLOCKZ) &&
                 (BX == PS2TERRAIN_BLOCKX)) )
            ShipBlockClip(BZ,BX,Min,Max,NDistSquared);
        }
#ifdef TERRAIN_TIMERS
        EmitTimeClipped.Stop();
#endif
    }

#ifdef TERRAIN_TIMERS
    EmitTime.Stop();
#endif

    gsreg_Begin();
    gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_DST,A_SRC,C_DST) );
    gsreg_End();
}

//==============================================================================

void terr::ShipBlockNoClip( s32 BZ, s32 BX,
                          const vector3& Min,
                          const vector3& Max,
                          f32 NDistSquared )
{
    (void)Max;
if( !PS2TERRAIN_RENDERBLOCKS )
    return;

#ifdef SHOW_BBOX
    draw_BBox( bbox(Min,Max), XCOLOR_BLUE );
#endif

//    u32 StartDListAddress = (u32)DLGetAddr();


    if( s_NCPacksInited==FALSE )
        InitNCPacks();
    //
    // Build block pack to create/transform/fog verts
    //
    ASSERT( (BZ>=0) && (BZ<32) && (BX>=0) && (BX<32) );
    block* pBlock = m_Block + BZ+BX*32;

    block_nc_pack* pPack = DLStruct(block_nc_pack);
    memcpy(pPack,&s_BlockNCPack,sizeof(block_nc_pack));
    pPack->XYZBase.X  = Min.X;
    pPack->XYZBase.Y  = 0;//Trans.Y;
    pPack->XYZBase.Z  = Min.Z;
    pPack->DMARef.SetRef(176,(u32)pBlock->Height);
    pPack->DMARef.PAD[1] = VIF_Unpack(7,81,VIF_S_16,TRUE,FALSE,TRUE);

    if( PS2TERRAIN_DETAILPASS && m_DoDetail && (NDistSquared < (m_DetailDist*m_DetailDist)) )
    {
        pPack->DetailColor[0] = 128;
        pPack->DetailColor[1] = 128;
        pPack->DetailColor[2] = 128;
        pPack->DetailColor[3] = 128;
    }
    else
    {
        pPack->DetailColor[0] = m_DetailMipColor.R;
        pPack->DetailColor[1] = m_DetailMipColor.G;
        pPack->DetailColor[2] = m_DetailMipColor.B;
        pPack->DetailColor[3] = m_DetailMipColor.A;
    }

if( !PS2TERRAIN_RENDERPASSES )
    return;

    //ASSERT(m_NPasses==1);
    // Loop through materials and build alpha packs
    //
    s32 FogOffset=0;
    s32 FogLeft=81;
    vram_SetMipEquation( m_BaseMipK );
    s32 NPassesRendered=0;
    //vram_Activate( m_Material[0].BMP );
    for( s32 M=0; M<m_NPasses; M++ )
    {
        if( !PS2TERRAIN_USEFRAGMENTS )
        {
            vram_Activate( m_Material[M].BMP );

            if( NPassesRendered==0 ) 
            {
                gsreg_Begin();
                gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_ZERO) );
                gsreg_End();
            }
            else       
            if( NPassesRendered==1 ) 
            {
                gsreg_Begin();
                gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_ZERO,A_SRC,C_DST) );
                gsreg_End();
            }

        }
        else
        {
            dmatag* pDMA = DLStruct(dmatag);
            pDMA->SetCall(0,(u32)&m_DListActivate[M]);
        }

        alpha_nc_pack* pPack = DLStruct(alpha_nc_pack);

        pPack->DMA.SetCont(16*2);
        pPack->DMA.PAD[1] = VIF_Unpack(0,2,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 6; // Opcode for block_alpha processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        if( PS2TERRAIN_FOG )
        {
            s32 NFog;
            if( NPassesRendered == (m_NPasses-1) )
                NFog = FogLeft;
            else
                NFog = MIN(FogLeft,(((81/m_NPasses)/3)*3));
            pPack->FOGRange[0] = FogOffset; // Offset into fog vectors
            pPack->FOGRange[1] = NFog; // Number of fog values to compute
            pPack->FOGRange[2] = 0;
            pPack->FOGRange[3] = 0;
            FogOffset += NFog;
        }
        else
        {
            pPack->FOGRange[0] = 0; // Offset into fog vectors
            pPack->FOGRange[1] = 0; // Number of fog values to compute
            pPack->FOGRange[2] = 0;
            pPack->FOGRange[3] = 0;
        }

        pPack->DMARef.SetRef( sizeof(block_alpha), (u32)&pBlock->Alpha[M] );
        pPack->DMARef.PAD[1] = VIF_Unpack(2,81,VIF_S_8,FALSE,FALSE,TRUE);
        pPack->DMAKick.SetCont(0);
        pPack->DMAKick.PAD[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->DMAKick.PAD[1] = SCE_VIF1_SET_FLUSH(0);
        NPassesRendered++;
    }
    STAT_TotalPassesRendered += NPassesRendered;

    if( PS2TERRAIN_DETAILPASS && m_DoDetail && (NDistSquared < (m_DetailDist*m_DetailDist)) )
    {
        vram_SetMipEquation( m_DetailMipK );
        vram_Activate( m_DetailMat.BMP );

        gsreg_Begin();
        gsreg_SetMipEquation( FALSE, m_DetailMipK, m_DetailMat.BMP.GetNMips(), MIP_MAG_BILINEAR, MIP_MIN_INTERP_MIPMAP_BILINEAR );
        gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_DST,C_ZERO,A_SRC,C_ZERO) );
        gsreg_End();

        detail_c_pack* pPack = DLStruct(detail_c_pack);

        pPack->DMA.SetCont(sizeof(detail_c_pack)-sizeof(dmatag));
        pPack->DMA.PAD[1] = VIF_Unpack(0,1,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 18; // Opcode for detail pass processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        pPack->VIF[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->VIF[1] = SCE_VIF1_SET_FLUSH(0);
        pPack->VIF[2] = 0;
        pPack->VIF[3] = 0;
    }


    // Build fog pass
    if( m_DoFog && PS2TERRAIN_FOG && PS2TERRAIN_FOGPASS )
    {
/*
        vram_Activate( m_Fog->GetBMP() );
        gsreg_Begin();
        gsreg_SetClamping(TRUE);
        gsreg_SetAlphaBlend( ALPHA_BLEND_MODE(C_SRC,C_DST,A_SRC,C_DST) );
        gsreg_End();
*/

        // SB - Clut activation is shared by context!
        if (m_Fog->GetBMP().IsClutBased())
        {
            eng_PushGSContext(1);
            vram_Activate( m_Fog->GetBMP() );
            eng_PopGSContext() ;
        }

        fog_nc_pack* pPack = DLStruct(fog_nc_pack);

        pPack->DMA.SetCont(16*3);
        pPack->DMA.PAD[1] = VIF_Unpack(0,2,VIF_V4_32,FALSE,FALSE,TRUE);

        pPack->CMD[0] = 12; // Opcode for fog pass processing
        pPack->CMD[1] = 0;
        pPack->CMD[2] = 0;
        pPack->CMD[3] = 0;

        //pPack->GifTag.Build2(3,10,GIF_PRIM_TRIANGLEFAN,GIF_FLAG_TEXTURE|GIF_FLAG_SMOOTHSHADE|GIF_FLAG_ALPHA|GIF_FLAG_CONTEXT);
        //pPack->GifTag.Reg(GIF_REG_ST,GIF_REG_RGBAQ,GIF_REG_XYZ2) ;
        //pPack->GifTag.EOP = 0;
        pPack->GifTag = s_NoClipFogGiftag;

        pPack->VIF[0] = SCE_VIF1_SET_MSCNT(0);
        pPack->VIF[1] = SCE_VIF1_SET_FLUSH(0);
        pPack->VIF[2] = 0;
        pPack->VIF[3] = 0;
/*
        gsreg_Begin();
        gsreg_SetClamping(FALSE);
        gsreg_End();
*/
    }

    //s32 DListUsed = ((u32)DLGetAddr() - StartDListAddress);
}


#endif
//==============================================================================
//==============================================================================
//==============================================================================
// END PS2 RENDERING
//==============================================================================
//==============================================================================
//==============================================================================

f32 terr::GetHeight( f32 PZ, f32 PX )
{
    f32 H;
    f32 zp = PZ * (1/8.0f);
    f32 xp = PX * (1/8.0f);
    s32 z  = (s32)x_floor(zp);
    s32 x  = (s32)x_floor(xp);

    zp -= (f32)z;
    xp -= (f32)x;
    z  &= 255;
    x  &= 255;

    f32 yBottomLeft  = GetVertHeight( z,   x   );
    f32 yBottomRight = GetVertHeight( z+1, x   );
    f32 yTopLeft     = GetVertHeight( z,   x+1 );
    f32 yTopRight    = GetVertHeight( z+1, x+1 );

    if( (z^x) & 0x01 )
    {
        // +---+
        // |\  |
        // |  \|
        // +---+

        if( (1.0f-zp) > xp )
            // bottom half
            H = yBottomLeft + (     zp) * (yBottomRight - yBottomLeft) 
                            + (     xp) * (yTopLeft     - yBottomLeft);
        else
            // top half
            H = yTopRight   + (1.0f-zp) * (yTopLeft     - yTopRight) 
                            + (1.0f-xp) * (yBottomRight - yTopRight);
    }
    else
    {
        // +---+
        // |  /|
        // |/  |
        // +---+

        if( zp > xp )
            // bottom half
            H = yBottomRight + (1.0f-zp) * (yBottomLeft - yBottomRight) 
                             + (     xp) * (yTopRight   - yBottomRight);
        else
            // top half
            H = yTopLeft     + (     zp) * (yTopRight   - yTopLeft)
                             + (1.0f-xp) * (yBottomLeft - yTopLeft);
    }

    return H;
}

//==============================================================================

void terr::GetNormal( f32 PZ, f32 PX, vector3& Normal )
{
    f32 zp = PZ * (1/8.0f);
    f32 xp = PX * (1/8.0f);
    s32 z  = (s32)x_floor(zp);
    s32 x  = (s32)x_floor(xp);

    zp -= (f32)z;
    xp -= (f32)x;
    z  &= 255;
    x  &= 255;

    f32 yBottomLeft  = GetVertHeight( z,   x   );
    f32 yBottomRight = GetVertHeight( z+1, x   );
    f32 yTopLeft     = GetVertHeight( z,   x+1 );
    f32 yTopRight    = GetVertHeight( z+1, x+1 );

    if( (z^x) & 0x01 )
    {
        // +---+
        // |\  |
        // |  \|
        // +---+

        if( (1.0f-zp) > xp )
            // bottom half
            Normal.Set( yBottomLeft-yBottomRight, yBottomLeft-yTopLeft, 8.0f );
        else
            // top half
            Normal.Set( yTopLeft-yTopRight, yBottomRight-yTopRight, 8.0f );
    }
    else
    {
        // +---+
        // |  /|
        // |/  |
        // +---+

        if( zp > xp )
            // bottom half
            Normal.Set( yBottomLeft-yBottomRight, yBottomRight-yTopRight, 8.0f );
        else
            // top half
            Normal.Set( yTopLeft-yTopRight, yBottomLeft-yTopLeft, 8.0f);
    }

    Normal.Normalize();
}

//==============================================================================

void terr::SetFog( const fog* pFog )
{
    m_Fog = (fog*)pFog;
}

//==============================================================================

void terr::GetMinMaxHeight ( f32& MinY, f32& MaxY )
{
    MinY = m_MinHeight;
    MaxY = m_MaxHeight;
}

//==============================================================================
#define SQUARE_SIZE (8)
#define BLOCK_SIZE (SQUARE_SIZE*8)

#define MAX_RAYTRACE_PTS    1024
vector3 s_RayTracePt[MAX_RAYTRACE_PTS];
xcolor  s_RayTraceColor[MAX_RAYTRACE_PTS];
s32     s_NRayTracePts;
vector3 s_RayTriPt[3];

//========================================================================

f32   RayTriIntersect( const vector3& P0, 
                       const vector3& P1,
                       const vector3& T0,
                       const vector3& T1,
                       const vector3& T2 )
{
    vector3 Normal  = v3_Cross( T1-T0, T2-T0 );
    f32     D       = -Normal.Dot( T0 );
    f32     D0      = P0.X*Normal.X + P0.Y*Normal.Y + P0.Z*Normal.Z + D;
    
    // Don't collide from behind.
    if( D0 <= 0.0f )  return( -1 );

    f32     D1      = P1.X*Normal.X + P1.Y*Normal.Y + P1.Z*Normal.Z + D;
    
    // Check if on same side
    if( (D0<0) && (D1<0) ) return -1;
    if( (D0>0) && (D1>0) ) return -1;
    
    // Get pt on plane
    f32 T = (0-D0)/(D1-D0);
    vector3 IP = P0 + T*(P1-P0);

    vector3 TP[3] = {T0,T1,T2};

    // Determine if point is inside tri
    for( s32 i=0; i<3; i++ )
    {
        vector3 EdgeNormal = Normal.Cross( TP[(i+1)%3]-TP[i] );
        if( EdgeNormal.Dot(IP-TP[i]) < -0.01f )
            return -1;
    }

    return T;
}

//==============================================================================

void terr::Collide( collider& Collider )
{
    if( Collider.GetType() == collider::RAY )
        RayCollide( Collider ); 
    else
    if( Collider.GetType() == collider::EXTRUSION )
        ExtCollide( Collider ); 
    else
        ASSERT( FALSE );
}

//==============================================================================

//xtimer RayBlockTimer;
xbool terr::RayBlockCollision(       collider& Collider,
                               const vector3& P0,
                               const vector3& P1)
{
//RayBlockTimer.Start();

    vector3     EntryPos;
    vector3     ExitPos;
    vector3     RayDelta;
    s32         NodeX,      NodeZ;
    s32         NewNodeX,   NewNodeZ;
    s32         ModNodeX,   ModNodeZ;
    s32         XDir;
    s32         ZDir;
    s32         NodeXUpdate;
    s32         NodeZUpdate;
    f32         OffsetX, OffsetZ;
    f32         OneOverDeltaX;
    f32         OneOverDeltaZ;
    f32         OffsetXUpdate;
    f32         OffsetZUpdate;
    f32         ExitOffsetX;
    f32         ExitOffsetZ;
    f32         NewOffsetX, NewOffsetZ;
    f32         T, NewT;
    f32         XExitT;
    f32         ZExitT;

    // Setup initial state
    NodeX           = (s32)x_floor( P0.X / SQUARE_SIZE );
    NodeZ           = (s32)x_floor( P0.Z / SQUARE_SIZE );
    OffsetX         = (f32)(NodeX*SQUARE_SIZE);
    OffsetZ         = (f32)(NodeZ*SQUARE_SIZE);
    ModNodeX        = (NodeX+65536) & 255;
    ModNodeZ        = (NodeZ+65536) & 255;

    RayDelta        = P1 - P0;
    EntryPos        = P0;
    T               = 0.0f;
    OneOverDeltaX   = 1.0f / RayDelta.X;
    OneOverDeltaZ   = 1.0f / RayDelta.Z;

    XDir            = (RayDelta.X < 0) ? (0):(1);
    ZDir            = (RayDelta.Z < 0) ? (0):(1);

    NodeXUpdate     = (RayDelta.X < 0) ? (-1):(1);
    NodeZUpdate     = (RayDelta.Z < 0) ? (-1):(1);
    OffsetXUpdate   = (f32)SQUARE_SIZE*NodeXUpdate;
    OffsetZUpdate   = (f32)SQUARE_SIZE*NodeZUpdate;
    ExitOffsetX     = (f32)XDir*SQUARE_SIZE;
    ExitOffsetZ     = (f32)ZDir*SQUARE_SIZE;

    // Loop until hit end of rope or intersect
    while( (T < 0.999f) )
    {
        // Compute X and Z exits from current node
        XExitT = ((OffsetX + ExitOffsetX)-P0.X)*OneOverDeltaX;
        ZExitT = ((OffsetZ + ExitOffsetZ)-P0.Z)*OneOverDeltaZ;
        ASSERT((XExitT>=0) && (ZExitT>=0));

        // Decide which exit is closer and update node we
        // are inside
        if( XExitT < ZExitT )
        {
            NewT        = XExitT;
            NewOffsetX  = OffsetX + OffsetXUpdate;
            NewOffsetZ  = OffsetZ;
            NewNodeX    = NodeX + NodeXUpdate;
            NewNodeZ    = NodeZ;
        }
        else
        {
            NewT        = ZExitT;
            NewOffsetX  = OffsetX;
            NewOffsetZ  = OffsetZ + OffsetZUpdate;
            NewNodeZ    = NodeZ + NodeZUpdate;
            NewNodeX    = NodeX;
        }

        // Peg T to 1.0f if past and compute exit position
        NewT = MIN(NewT,1.0f);
        ExitPos = P0 + NewT*RayDelta;

        // We have which node we are in (NodeX,NodeZ) and 
        // we have the entry and exit points

        if( (ModNodeX != NodeX) || (ModNodeZ != NodeZ) || !IsModSquareEmpty(ModNodeZ,ModNodeX) )
        {
            f32 H[4];

            //H[0] = GetVertHeight(ModNodeZ  ,ModNodeX  );
            //H[1] = GetVertHeight(ModNodeZ+1,ModNodeX  );
            //H[2] = GetVertHeight(ModNodeZ  ,ModNodeX+1);
            //H[3] = GetVertHeight(ModNodeZ+1,ModNodeX+1);
            GetQuadHeights( ModNodeZ, ModNodeX, H );

            f32 MaxH = MAX(H[0],H[1]);
            MaxH = MAX(MaxH,H[2]);
            MaxH = MAX(MaxH,H[3]);
            if( (EntryPos.Y<MaxH) || (ExitPos.Y<MaxH) )
            {
                vector3 TriPt[4];
                TriPt[0] = vector3( OffsetX,             H[0], OffsetZ ); // Bottom Left
                TriPt[1] = vector3( OffsetX,             H[1], OffsetZ+SQUARE_SIZE ); // Bottom Right
                TriPt[2] = vector3( OffsetX+SQUARE_SIZE, H[2], OffsetZ ); // Top Left
                TriPt[3] = vector3( OffsetX+SQUARE_SIZE, H[3], OffsetZ+SQUARE_SIZE ); // Top Right

                if( (ModNodeZ^ModNodeX) & 0x01 )
                {
                    // +---+
                    // |\  |
                    // |  \|
                    // +---+
                    f32 CT = Collider.GetCollisionT();
                    Collider.ApplyTri( TriPt[2], TriPt[0], TriPt[1] );
                    Collider.ApplyTri( TriPt[2], TriPt[1], TriPt[3] );
                    if( Collider.GetCollisionT() < CT )
                    {
//                        RayBlockTimer.Stop();
                        return TRUE;
                    }
                }
                else
                {
                    // +---+
                    // |  /|
                    // |/  |
                    // +---+
                    f32 CT = Collider.GetCollisionT();
                    Collider.ApplyTri( TriPt[2], TriPt[0], TriPt[3] );
                    Collider.ApplyTri( TriPt[3], TriPt[0], TriPt[1] );
                    if( Collider.GetCollisionT() < CT )
                    {
//                        RayBlockTimer.Stop();
                        return TRUE;
                    }
                }

                // Early out?
                if (Collider.GetEarlyOut())
                {
//                    RayBlockTimer.Stop();
                    return TRUE;
                }
            }
        }

        // Move on to next block
        T        = NewT;
        EntryPos = ExitPos;
        NodeX    = NewNodeX;
        NodeZ    = NewNodeZ;
        ModNodeX = (NodeX+65536) & 255;
        ModNodeZ = (NodeZ+65536) & 255;
        OffsetX  = NewOffsetX;
        OffsetZ  = NewOffsetZ;
    }

//RayBlockTimer.Stop();
    return FALSE;
}

//==============================================================================

void terr::RayCollide( collider& Collider )
{
    f32 HorizLen;
    vector3 P0 = Collider.RayGetStart();
    vector3 P1 = Collider.RayGetEnd();
    HorizLen = (P1.X-P0.X)*(P1.X-P0.X) + (P1.Z-P0.Z)*(P1.Z-P0.Z);

    if( HorizLen < 8.0f*8.0f )
    {
        bbox BBox = Collider.GetVolumeBBox();

        // Loop through all squares inside horiz bbox.
        s32 MinSquareZ = (s32)x_floor( BBox.Min.Z / SQUARE_SIZE );
        s32 MaxSquareZ = (s32)x_floor( BBox.Max.Z / SQUARE_SIZE );
        s32 MinSquareX = (s32)x_floor( BBox.Min.X / SQUARE_SIZE );
        s32 MaxSquareX = (s32)x_floor( BBox.Max.X / SQUARE_SIZE );
        for( s32 SX=MinSquareX; SX<=MaxSquareX; SX++ )
        for( s32 SZ=MinSquareZ; SZ<=MaxSquareZ; SZ++ )
        {
            vector3 TriPt[4];

            if( IsSquareEmpty(SZ,SX) )
                continue;

            // Get height info about square.
            f32 H[4];
            H[0] = GetVertHeight(SZ  ,SX  );
            H[1] = GetVertHeight(SZ+1,SX  );
            H[2] = GetVertHeight(SZ  ,SX+1);
            H[3] = GetVertHeight(SZ+1,SX+1);
            f32 MaxH = MAX(H[0],H[1]);
                MaxH = MAX(MaxH,H[2]);
                MaxH = MAX(MaxH,H[3]);
            f32 MinH = MIN(H[0],H[1]);
                MinH = MIN(MinH,H[2]);
                MinH = MIN(MinH,H[3]);

            // See if we missed the square vertically.
            if( (BBox.Min.Y > MaxH) || (BBox.Max.Y < MinH) )
                continue;

            // Build actual verts.
            f32 FSZ = (f32)SZ*SQUARE_SIZE;
            f32 FSX = (f32)SX*SQUARE_SIZE;
            TriPt[0] = vector3( FSX,             H[0], FSZ );               // Bottom Left
            TriPt[1] = vector3( FSX,             H[1], FSZ+SQUARE_SIZE );   // Bottom Right
            TriPt[2] = vector3( FSX+SQUARE_SIZE, H[2], FSZ );               // Top Left
            TriPt[3] = vector3( FSX+SQUARE_SIZE, H[3], FSZ+SQUARE_SIZE );   // Top Right

            if( (SZ^SX) & 0x01 )
            {
                // +---+
                // |\  |
                // |  \|
                // +---+
                Collider.ApplyTri( TriPt[2], TriPt[0], TriPt[1] );
                Collider.ApplyTri( TriPt[2], TriPt[1], TriPt[3] );
            }
            else
            {
                // +---+
                // |  /|
                // |/  |
                // +---+
                Collider.ApplyTri( TriPt[2], TriPt[0], TriPt[3] );
                Collider.ApplyTri( TriPt[3], TriPt[0], TriPt[1] );
            }

            // Early out?
            if (Collider.GetEarlyOut())
                return ;
        }
    }
    else
    {
        vector3     EntryPos;
        vector3     ExitPos;
        vector3     RayDelta;
        s32         NodeX,      NodeZ;
        s32         NewNodeX,   NewNodeZ;
        s32         XDir;
        s32         ZDir;
        s32         NodeXUpdate;
        s32         NodeZUpdate;
        f32         OffsetX, OffsetZ;
        f32         MinY, MaxY;
        f32         OneOverDeltaX;
        f32         OneOverDeltaZ;
        f32         OffsetXUpdate;
        f32         OffsetZUpdate;
        f32         ExitOffsetX;
        f32         ExitOffsetZ;
        f32         NewOffsetX, NewOffsetZ;
        f32         T, NewT;
        f32         XExitT;
        f32         ZExitT;

        s_NRayTracePts = 0;


        // Setup initial state
        NodeX           = (s32)x_floor( P0.X / BLOCK_SIZE );
        NodeZ           = (s32)x_floor( P0.Z / BLOCK_SIZE );
        OffsetX         = (f32)(NodeX*BLOCK_SIZE);
        OffsetZ         = (f32)(NodeZ*BLOCK_SIZE);
        NodeX           = (NodeX+65536) & 31;
        NodeZ           = (NodeZ+65536) & 31;

        RayDelta        = P1 - P0;
        EntryPos        = P0;
        T               = 0.0f;
        OneOverDeltaX   = 1.0f / RayDelta.X;
        OneOverDeltaZ   = 1.0f / RayDelta.Z;

        XDir            = (RayDelta.X < 0) ? (0):(1);
        ZDir            = (RayDelta.Z < 0) ? (0):(1);
    
        NodeXUpdate     = (RayDelta.X < 0) ? (-1):(1);
        NodeZUpdate     = (RayDelta.Z < 0) ? (-1):(1);
        OffsetXUpdate   = (f32)BLOCK_SIZE*NodeXUpdate;
        OffsetZUpdate   = (f32)BLOCK_SIZE*NodeZUpdate;
        ExitOffsetX     = (f32)XDir*BLOCK_SIZE;
        ExitOffsetZ     = (f32)ZDir*BLOCK_SIZE;

        // Loop until hit end of rope or intersect
        f32 CT = Collider.GetCollisionT();
        while( (T < 0.999f) && (T < CT) )
        {
            // Compute X and Z exits from current node
            XExitT = ((OffsetX + ExitOffsetX)-P0.X)*OneOverDeltaX;
            ZExitT = ((OffsetZ + ExitOffsetZ)-P0.Z)*OneOverDeltaZ;
            ASSERT((XExitT>=0) && (ZExitT>=0));

            // Decide which exit is closer and update node we
            // are inside
            if( XExitT < ZExitT )
            {
                NewT        = XExitT;
                NewOffsetX  = OffsetX + OffsetXUpdate;
                NewOffsetZ  = OffsetZ;
                NewNodeX    = (NodeX + NodeXUpdate + 65536) & 31;
                NewNodeZ    = NodeZ;
            }
            else
            {
                NewT        = ZExitT;
                NewOffsetX  = OffsetX;
                NewOffsetZ  = OffsetZ + OffsetZUpdate;
                NewNodeZ    = (NodeZ + NodeZUpdate + 65536) & 31;
                NewNodeX    = NodeX;
            }

            // Peg T to 1.0f if past and compute exit position
            NewT = MIN(NewT,1.0f);
            ExitPos = P0 + NewT*RayDelta;

            // We have which node we are in (NodeX,NodeZ) and 
            // we have the entry and exit points

            // Get bounds of block
            GetQuadBounds( NodeZ*8, NodeX*8, 3, MinY, MaxY );
    
            // Check if we are now traversing underground
            if( (EntryPos.Y < MinY) && (ExitPos.Y < MinY) )
                return;
        
            // Check if we need to check at the square level
            if( (EntryPos.Y < MaxY) || (ExitPos.Y < MaxY) )
            {
                if( RayBlockCollision( Collider, EntryPos, ExitPos ) )
                {
                    return;
                }
            }

            // Move on to next block
            T        = NewT;
            EntryPos = ExitPos;
            NodeX    = NewNodeX;
            NodeZ    = NewNodeZ;
            OffsetX  = NewOffsetX;
            OffsetZ  = NewOffsetZ;
        }
    }
}

//==============================================================================

void terr::ExtCollide( collider& Collider )
{
    bbox BBox = Collider.GetVolumeBBox();

    // Loop through all squares inside horiz bbox.
    s32 MinSquareZ = (s32)x_floor( BBox.Min.Z / SQUARE_SIZE );
    s32 MaxSquareZ = (s32)x_floor( BBox.Max.Z / SQUARE_SIZE );
    s32 MinSquareX = (s32)x_floor( BBox.Min.X / SQUARE_SIZE );
    s32 MaxSquareX = (s32)x_floor( BBox.Max.X / SQUARE_SIZE );
    for( s32 SX=MinSquareX; SX<=MaxSquareX; SX++ )
    for( s32 SZ=MinSquareZ; SZ<=MaxSquareZ; SZ++ )
    {
        if( IsSquareEmpty(SZ,SX) )
            continue;

        vector3 TriPt[4];

        // Get height info about square.
        f32 H[4];
        H[0] = GetVertHeight(SZ  ,SX  );
        H[1] = GetVertHeight(SZ+1,SX  );
        H[2] = GetVertHeight(SZ  ,SX+1);
        H[3] = GetVertHeight(SZ+1,SX+1);
        f32 MaxH = MAX(H[0],H[1]);
            MaxH = MAX(MaxH,H[2]);
            MaxH = MAX(MaxH,H[3]);
        f32 MinH = MIN(H[0],H[1]);
            MinH = MIN(MinH,H[2]);
            MinH = MIN(MinH,H[3]);

        // See if we missed the square vertically.
        if( (BBox.Min.Y > MaxH) || (BBox.Max.Y < MinH) )
            continue;

        // Build actual verts.
        f32 FSZ = (f32)SZ*SQUARE_SIZE;
        f32 FSX = (f32)SX*SQUARE_SIZE;
        TriPt[0] = vector3( FSX,             H[0], FSZ );               // Bottom Left
        TriPt[1] = vector3( FSX,             H[1], FSZ+SQUARE_SIZE );   // Bottom Right
        TriPt[2] = vector3( FSX+SQUARE_SIZE, H[2], FSZ );               // Top Left
        TriPt[3] = vector3( FSX+SQUARE_SIZE, H[3], FSZ+SQUARE_SIZE );   // Top Right

        if( (SZ^SX) & 0x01 )
        {
            // +---+
            // |\  |
            // |  \|
            // +---+
#ifdef DEBUG_EXTRUSION
            draw_Line( TriPt[2], TriPt[0], XCOLOR_BLACK );
            draw_Line( TriPt[0], TriPt[1], XCOLOR_BLACK );
            draw_Line( TriPt[1], TriPt[2], XCOLOR_BLACK );
            draw_Line( TriPt[2], TriPt[1], XCOLOR_BLACK );
            draw_Line( TriPt[1], TriPt[3], XCOLOR_BLACK );
            draw_Line( TriPt[3], TriPt[2], XCOLOR_BLACK );
#endif
            Collider.ApplyTri( TriPt[2], TriPt[0], TriPt[1] );
            Collider.ApplyTri( TriPt[2], TriPt[1], TriPt[3] );
        }
        else
        {
            // +---+
            // |  /|
            // |/  |
            // +---+

#ifdef DEBUG_EXTRUSION
            draw_Line( TriPt[2], TriPt[0], XCOLOR_BLACK );
            draw_Line( TriPt[0], TriPt[3], XCOLOR_BLACK );
            draw_Line( TriPt[3], TriPt[2], XCOLOR_BLACK );
            draw_Line( TriPt[3], TriPt[0], XCOLOR_BLACK );
            draw_Line( TriPt[0], TriPt[1], XCOLOR_BLACK );
            draw_Line( TriPt[1], TriPt[3], XCOLOR_BLACK );
#endif      
            Collider.ApplyTri( TriPt[2], TriPt[0], TriPt[3] );
            Collider.ApplyTri( TriPt[3], TriPt[0], TriPt[1] );
        }

        // Early out?
        if (Collider.GetEarlyOut())
            return ;
    }
}

//==============================================================================

void terr::BuildLightingData( const vector3&    LightDir,
                                    f32         ShadowFalloffDist,
                                    f32         MinShadow,
                                    f32         MaxShadow,
                                    f32         MinDiffuse,
                                    f32         MaxDiffuse,
                                    f32         MinGlobal,
                                    f32         MaxGlobal,
                              const char*       pFileName )
{
    struct light_info
    {
        f32 IShad;
        f32 IDiff;
    };

    char LightPath[ X_MAX_PATH  ];
    
    ASSERT( m_Fog != NULL );

    //x_DebugMsg("*************************\n");
    //x_DebugMsg("*** BUILDING LIGHTING ***\n");
    //x_DebugMsg("*************************\n");

    // Build lighting file path
    {
        char Drive   [ X_MAX_DRIVE ];
        char Dir     [ X_MAX_DIR   ];
        char FName   [ X_MAX_FNAME ];
//      char Ext     [ X_MAX_EXT   ];

        x_splitpath( pFileName, Drive, Dir, FName, NULL     );
        x_makepath ( LightPath, Drive, Dir, FName, ".lit" );
    }

    // Allocate temporary room for lighting
    byte* pLightI = (byte*)x_malloc(sizeof(byte)*256*256);
    ASSERT(pLightI);

    
    X_FILE* fp = x_fopen(LightPath,"rb");
    xbool LightInfoLoaded = FALSE;
    if( fp )
    {
        LightInfoLoaded = TRUE;
        x_fread( pLightI, 256*256, 1, fp );
        x_fclose(fp);
    }

    x_free(pLightI);
    //x_DebugMsg("*************************\n");
    //x_DebugMsg("*** LIGHTING FINISHED ***\n");
    //x_DebugMsg("*************************\n");
}

//==============================================================================

void terr::SetEmptySquare( s32 SQ )
{
    s32 Z  = (SQ>>0)&0xFF;
    s32 X  = (SQ>>8)&0xFF;
    s32 BI = (Z>>3) + ((X>>3)<<5);
    
    Z -= (Z>>3)<<3;
    X -= (X>>3)<<3;

    u64 Bit = ((u64)1) << (Z+(X<<3));

    m_Block[BI].SquareMask |= Bit;
}

//==============================================================================

xbool terr::IsSquareEmpty( s32 aZ, s32 aX )
{
    s32 Z  = (aZ + 65536) & 0xFF;
    s32 X  = (aX + 65536) & 0xFF;

    if( (X != aX) || (Z != aZ) )
        return( FALSE );

    s32 BI = (Z>>3) + ((X>>3)<<5);
    
    Z -= (Z>>3)<<3;
    X -= (X>>3)<<3;

    u64 Bit = ((u64)1) << (Z+(X<<3));

    return (m_Block[BI].SquareMask & Bit)?(TRUE):(FALSE);
}

//==============================================================================

xbool terr::IsModSquareEmpty( s32 Z, s32 X )
{
    s32 BI = (Z>>3) + ((X>>3)<<5);
    
    Z -= (Z>>3)<<3;
    X -= (X>>3)<<3;

    u64 Bit = ((u64)1) << (Z+(X<<3));

    return (m_Block[BI].SquareMask & Bit)?(TRUE):(FALSE);
}

//==============================================================================

void terr::GetLighting( f32 PZ, f32 PX, xcolor& C, f32& Y )
{
    f32 zp = PZ * (1/8.0f);
    f32 xp = PX * (1/8.0f);
    s32 z  = (s32)x_floor(zp);
    s32 x  = (s32)x_floor(xp);
    f32 L;

    if( IsSquareEmpty(z,x) )
    {
        C = XCOLOR_WHITE;
        Y = -10000.0f;
        return;
    }

    zp -= (f32)z;
    xp -= (f32)x;
    z  &= 255;
    x  &= 255;

    f32 yBottomLeft  = GetVertHeight( z,   x   );
    f32 yBottomRight = GetVertHeight( z+1, x   );
    f32 yTopLeft     = GetVertHeight( z,   x+1 );
    f32 yTopRight    = GetVertHeight( z+1, x+1 );
    f32 lBottomLeft  = GetVertLight ( z,   x   );
    f32 lBottomRight = GetVertLight ( z+1, x   );
    f32 lTopLeft     = GetVertLight ( z,   x+1 );
    f32 lTopRight    = GetVertLight ( z+1, x+1 );

    if( (z^x) & 0x01 )
    {
        // +---+
        // |\  |
        // |  \|
        // +---+

        if( (1.0f-zp) > xp )
        {
            // bottom half
            Y = yBottomLeft + (     zp) * (yBottomRight - yBottomLeft) 
                            + (     xp) * (yTopLeft     - yBottomLeft);
            L = lBottomLeft + (     zp) * (lBottomRight - lBottomLeft) 
                            + (     xp) * (lTopLeft     - lBottomLeft);
        }
        else
        {
            // top half
            Y = yTopRight   + (1.0f-zp) * (yTopLeft     - yTopRight) 
                            + (1.0f-xp) * (yBottomRight - yTopRight);
            L = lTopRight   + (1.0f-zp) * (lTopLeft     - lTopRight) 
                            + (1.0f-xp) * (lBottomRight - lTopRight);
        }
    }
    else
    {
        // +---+
        // |  /|
        // |/  |
        // +---+

        if( zp > xp )
        {
            // bottom half
            Y = yBottomRight + (1.0f-zp) * (yBottomLeft - yBottomRight) 
                             + (     xp) * (yTopRight   - yBottomRight);
            L = lBottomRight + (1.0f-zp) * (lBottomLeft - lBottomRight) 
                             + (     xp) * (lTopRight   - lBottomRight);
        }
        else
        {
            // top half
            Y = yTopLeft     + (     zp) * (yTopRight   - yTopLeft)
                             + (1.0f-xp) * (yBottomLeft - yTopLeft);
            L = lTopLeft     + (     zp) * (lTopRight   - lTopLeft)
                             + (1.0f-xp) * (lBottomLeft - lTopLeft);
        }
    }

    C.R = (byte)(L*255.0f);
    C.G = (byte)(L*255.0f);
    C.B = (byte)(L*255.0f);
    C.A = 255;
}

//==============================================================================

void terr::GetSphereFaceKeys ( const vector3&           Pos,
                                        f32             Radius,
                                        s32             ObjectSlot,
                                        s32&            NKeys,
                                        fcache_key*     pKey )
{
    /*
    s32 MaxKeys = NKeys;

    NKeys = 0;

    // Loop through all squares inside horiz bbox.
    s32 MinSquareZ = (s32)x_floor( (Pos.Z-Radius) / SQUARE_SIZE );
    s32 MaxSquareZ = (s32)x_floor( (Pos.Z+Radius) / SQUARE_SIZE );
    s32 MinSquareX = (s32)x_floor( (Pos.X-Radius) / SQUARE_SIZE );
    s32 MaxSquareX = (s32)x_floor( (Pos.X+Radius) / SQUARE_SIZE );
    for( s32 SX=MinSquareX; SX<=MaxSquareX; SX++ )
    for( s32 SZ=MinSquareZ; SZ<=MaxSquareZ; SZ++ )
    {
        if( IsSquareEmpty(SZ,SX) )
            continue;

        // Get height info about square.
        f32 H[4];
        H[0] = GetVertHeight(SZ  ,SX  );
        H[1] = GetVertHeight(SZ+1,SX  );
        H[2] = GetVertHeight(SZ  ,SX+1);
        H[3] = GetVertHeight(SZ+1,SX+1);
        f32 MaxH = MAX(H[0],H[1]);
            MaxH = MAX(MaxH,H[2]);
            MaxH = MAX(MaxH,H[3]);
        f32 MinH = MIN(H[0],H[1]);
            MinH = MIN(MinH,H[2]);
            MinH = MIN(MinH,H[3]);

        // See if we missed the square vertically.
        if( ((Pos.Y-Radius) > MaxH) || ((Pos.Y+Radius) < MinH) )
            continue;

        // Add the triangle keys
        {
            s32 PrivX = SX + 256;
            s32 PrivZ = SZ + 256;
            if( (PrivX>=0) && (PrivZ>=0) && (PrivX<768) && (PrivZ<768) )
            {
                s32 PrivateInfo = PrivX + PrivZ*768;

                // Just stop adding keys if we begin to overflow
                ASSERT( NKeys+2 <= MaxKeys );
                if( (NKeys+2) > MaxKeys )
                    return;

                fcache_ConstructKey( pKey[NKeys+0], ObjectSlot, (PrivateInfo<<1)+0 );
                fcache_ConstructKey( pKey[NKeys+1], ObjectSlot, (PrivateInfo<<1)+1 );
                NKeys+=2;
            }
        }
    }
    */
}
                
//==============================================================================

void    terr::GetPillFaceKeys ( const vector3&    StartPos,
                                const vector3&    EndPos,
                                    f32         Radius,
                                    s32         ObjectSlot,
                                    s32&        NKeys,
                                    fcache_key* pKey )
{
    /*
    s32 MaxKeys = NKeys;
    vector3 RayDir = (EndPos - StartPos);
    f32 RayRadius2 = Radius*Radius;
    RayDir.Normalize();

    NKeys = 0;

    // Get bbox of pill
    bbox PillBBox;
    PillBBox.Min.X = MIN(StartPos.X,EndPos.X) - Radius;
    PillBBox.Min.Y = MIN(StartPos.Y,EndPos.Y) - Radius;
    PillBBox.Min.Z = MIN(StartPos.Z,EndPos.Z) - Radius;
    PillBBox.Max.X = MAX(StartPos.X,EndPos.X) + Radius;
    PillBBox.Max.Y = MAX(StartPos.Y,EndPos.Y) + Radius;
    PillBBox.Max.Z = MAX(StartPos.Z,EndPos.Z) + Radius;

    // Loop through all squares inside horiz bbox.
    s32 MinSquareZ = (s32)x_floor( PillBBox.Min.Z / SQUARE_SIZE );
    s32 MaxSquareZ = (s32)x_floor( PillBBox.Max.Z / SQUARE_SIZE );
    s32 MinSquareX = (s32)x_floor( PillBBox.Min.X / SQUARE_SIZE );
    s32 MaxSquareX = (s32)x_floor( PillBBox.Max.X / SQUARE_SIZE );
    for( s32 SX=MinSquareX; SX<=MaxSquareX; SX++ )
    for( s32 SZ=MinSquareZ; SZ<=MaxSquareZ; SZ++ )
    {
        if( IsSquareEmpty(SZ,SX) )
            continue;

        // Get height info about square.
        f32 H[4];
        H[0] = GetVertHeight(SZ  ,SX  );
        H[1] = GetVertHeight(SZ+1,SX  );
        H[2] = GetVertHeight(SZ  ,SX+1);
        H[3] = GetVertHeight(SZ+1,SX+1);
        f32 MaxH = MAX(H[0],H[1]);
            MaxH = MAX(MaxH,H[2]);
            MaxH = MAX(MaxH,H[3]);
        f32 MinH = MIN(H[0],H[1]);
            MinH = MIN(MinH,H[2]);
            MinH = MIN(MinH,H[3]);

        // See if we missed the square vertically.
        if( (PillBBox.Min.Y > MaxH) || (PillBBox.Max.Y < MinH) )
            continue;


        //Check if sphere comes in contact with pill
        f32     R2;
        vector3 Center;

        Center.X = (SX*SQUARE_SIZE) + (SQUARE_SIZE*0.5f);
        Center.Y = (MinH+MaxH)*0.5f;
        Center.Z = (SZ*SQUARE_SIZE) + (SQUARE_SIZE*0.5f);
        R2       = (SQUARE_SIZE*0.5f)*(SQUARE_SIZE*0.5f) +
                   (SQUARE_SIZE*0.5f)*(SQUARE_SIZE*0.5f) +
                   ((MaxH-MinH)*0.5f)*((MaxH-MinH)*0.5f);

        vector3 StoC   = Center - StartPos;
        f32     Dot    = RayDir.Dot(StoC);
        f32     Dist2  = StoC.LengthSquared() - (Dot*Dot);
        if( Dist2 < (R2*2.0f+RayRadius2) )
        {
            // Add the triangle keys
            s32 PrivX = SX + 256;
            s32 PrivZ = SZ + 256;
            if( (PrivX>=0) && (PrivZ>=0) && (PrivX<768) && (PrivZ<768) )
            {
                s32 PrivateInfo = PrivX + PrivZ*768;

                // Just stop adding keys if we begin to overflow
                ASSERT( NKeys+2 <= MaxKeys );
                if( (NKeys+2) > MaxKeys )
                    return;

                fcache_ConstructKey( pKey[NKeys+0], ObjectSlot, (PrivateInfo<<1)+0 );
                fcache_ConstructKey( pKey[NKeys+1], ObjectSlot, (PrivateInfo<<1)+1 );
                NKeys+=2;
            }
        }
    }
    */
}
                
//==============================================================================

void terr::ConstructFace   ( fcache_face& Face )
{
    /*
    s32 ObjectSlot;
    s32 PrivateInfo;
    fcache_DestructKey( Face.Key, ObjectSlot, PrivateInfo );

    s32 TriIndex = PrivateInfo & 0x01;
    s32 PrivZ = (PrivateInfo>>1) / 768;
    s32 PrivX = (PrivateInfo>>1) % 768;
    
    ASSERT( (PrivX>=0) && (PrivZ>=0) && (PrivX<768) && (PrivZ<768) );

    s32 SX = PrivX - 256;
    s32 SZ = PrivZ - 256;

    // Get height info about square.
    f32 H[4];
    H[0] = GetVertHeight(SZ  ,SX  );
    H[1] = GetVertHeight(SZ+1,SX  );
    H[2] = GetVertHeight(SZ  ,SX+1);
    H[3] = GetVertHeight(SZ+1,SX+1);

    // Build actual verts.
    f32 FSZ = (f32)SZ*SQUARE_SIZE;
    f32 FSX = (f32)SX*SQUARE_SIZE;
    vector3 TriPt[4];
    TriPt[0] = vector3( FSX,             H[0], FSZ );               // Bottom Left
    TriPt[1] = vector3( FSX,             H[1], FSZ+SQUARE_SIZE );   // Bottom Right
    TriPt[2] = vector3( FSX+SQUARE_SIZE, H[2], FSZ );               // Top Left
    TriPt[3] = vector3( FSX+SQUARE_SIZE, H[3], FSZ+SQUARE_SIZE );   // Top Right

    if( (SZ^SX) & 0x01 )
    {
        // +---+
        // |\  |
        // |  \|
        // +---+
        if( TriIndex )
        {
            Face.Pos[0] = TriPt[2];
            Face.Pos[1] = TriPt[0];
            Face.Pos[2] = TriPt[1];
        }
        else
        {
            Face.Pos[0] = TriPt[2];
            Face.Pos[1] = TriPt[1];
            Face.Pos[2] = TriPt[3];
        }
    }
    else
    {
        // +---+
        // |  /|
        // |/  |
        // +---+
        if( TriIndex )
        {
            Face.Pos[0] = TriPt[2];
            Face.Pos[1] = TriPt[0];
            Face.Pos[2] = TriPt[3];
        }
        else
        {
            Face.Pos[0] = TriPt[3];
            Face.Pos[1] = TriPt[0];
            Face.Pos[2] = TriPt[1];
        }
    }

    Face.NVerts = 3;
    Face.Plane.Setup( Face.Pos[0], Face.Pos[1], Face.Pos[2] );
    Face.BBox.Clear();
    Face.BBox.AddVerts( Face.Pos, 3 );
    Face.Plane.GetOrthoVectors( Face.OrthoS, Face.OrthoT );
    Face.OrthoST[0].X = Face.OrthoS.Dot( Face.Pos[0] );
    Face.OrthoST[0].Y = Face.OrthoT.Dot( Face.Pos[0] );
    Face.OrthoST[1].X = Face.OrthoS.Dot( Face.Pos[1] );
    Face.OrthoST[1].Y = Face.OrthoT.Dot( Face.Pos[1] );
    Face.OrthoST[2].X = Face.OrthoS.Dot( Face.Pos[2] );
    Face.OrthoST[2].Y = Face.OrthoT.Dot( Face.Pos[2] );
    */
}

//==============================================================================

#ifdef TERRAIN_TIMERS
xtimer TerrainOccTimer;
s32    TerrainOccNCalls;
s32    TerrainOccNOccluded;
s32    TerrainOccNRays;
s32    TerrainOccNRaysFound;
#endif

//==============================================================================

xbool terr::CrudeRay( s32 X0, s32 Y0, f32 Z0, s32 X1, s32 Y1, f32 Z1 )
{
    ASSERT( (Y0>=0) && (Y0<=255) );
    ASSERT( (X0>=0) && (X0<=255) );
    ASSERT( (Y1>=0) && (Y1<=255) );
    ASSERT( (X1>=0) && (X1<=255) );
    s32 deltax = x_abs(X1 - X0);        // The difference between the x's
    s32 deltay = x_abs(Y1 - Y0);        // The difference between the y's
    xbool longdist = ((deltax+deltay) > 32);

    if( (deltax<8) && (deltay<8) )
        return TRUE;

    s32 x = X0;                       // Start x off at the first pixel
    s32 y = Y0;                       // Start y off at the first pixel
    s32 z = (s32)(Z0*1024.0f);
    s32 xinc1,xinc2;
    s32 yinc1,yinc2;
    s32 zinc;
    s32 numpixels;
    s32 num,den,numadd;

    if( X1>=X0 )    { xinc1 =  1; xinc2 =  1; }
    else            { xinc1 = -1; xinc2 = -1; }

    if( Y1>=Y0 )    { yinc1 =  1; yinc2 =  1; }
    else            { yinc1 = -1; yinc2 = -1; }

    if( deltax >= deltay )         // There is at least one x-value for every y-value
    {
        xinc1 = 0;                  // Don't change the x when numerator >= denominator
        yinc2 = 0;                  // Don't change the y for every iteration
        den = deltax;
        num = deltax >> 1;
        numadd = deltay;
        numpixels = deltax;         // There are more x-values than y-values
    }
    else                          // There is at least one y-value for every x-value
    {
        xinc2 = 0;                  // Don't change the x for every iteration
        yinc1 = 0;                  // Don't change the y when numerator >= denominator
        den = deltay;
        num = deltay >> 1;
        numadd = deltax;
        numpixels = deltay;         // There are more y-values than x-values
    }

    zinc = (s32)(((Z1-Z0) / (f32)numpixels)*1024.0f);

    while( 1 )
    {
        // Check if we are under terrain
        block* pBlock = &m_Block[ (x>>3)+((y>>3)<<5) ];
        s32  H = pBlock->Height[ (x&7)+((y&7)<<3)+(y&7) ];

        if( !pBlock->SquareMask )
        {
            if( H > (z>>6) )
                return FALSE;   
        }

        // STEP 1
        num += numadd;              
        if (num >= den)             
        {
            num -= den;             
            x += xinc1;             
            y += yinc1;             
        }
        x += xinc2;                 
        y += yinc2;                 
        z += zinc;
        numpixels--;
        if( numpixels==0 ) break;

        if( longdist )
        {
            // STEP 2
            num += numadd;              
            if (num >= den)             
            {
                num -= den;             
                x += xinc1;             
                y += yinc1;             
            }
            x += xinc2;                 
            y += yinc2;                 
            z += zinc;
            numpixels--;
            if( numpixels==0 ) break;

            // STEP 3
            num += numadd;              
            if (num >= den)             
            {
                num -= den;             
                x += xinc1;             
                y += yinc1;             
            }
            x += xinc2;                 
            y += yinc2;                 
            z += zinc;
            numpixels--;
            if( numpixels==0 ) break;
        }
    }

    return TRUE;
}

//==============================================================================

xbool terr::IsOccluded( const bbox&     BBox,
                        const vector3&  EyePos )
{
#ifdef TERRAIN_TIMERS
    TerrainOccNCalls++;
#endif

    if( !PS2TERRAIN_LOCKOCCLUSION )
        m_OccPos = EyePos;


    if( PS2TERRAIN_NOOCCLUSION )
        return FALSE;

    // Check if we are within the XZ limits of the bbox
    if( (m_OccPos.X <= BBox.Max.X) && (m_OccPos.X >= BBox.Min.X) &&
        (m_OccPos.Z <= BBox.Max.Z) && (m_OccPos.Z >= BBox.Min.Z) )
    {
        return FALSE;
    }

    // Check if out of bounds
    if( (BBox.Min.X < 0) || (BBox.Max.X>2040.0f) ||
        (BBox.Min.Z < 0) || (BBox.Max.Z>2040.0f) ||
        (m_OccPos.X < 0) || (m_OccPos.X>2040.0f) ||
        (m_OccPos.Z < 0) || (m_OccPos.Z>2040.0f) )
        return FALSE;

#ifdef TERRAIN_TIMERS
    TerrainOccTimer.Start();
#endif

    // Check squared distance and do crude ray check
    if( 1 )
    {
        vector3 BBoxCenter = BBox.GetCenter();
        BBoxCenter.Y = BBox.Max.Y;

        //f32 Dist2 = (m_OccPos - BBoxCenter).LengthSquared();
        //if( Dist2 > 320.0f*320.0f )
        {
            //draw_Marker( BBoxCenter );

#ifdef TERRAIN_TIMERS
            TerrainOccNRays++;
#endif

            if( CrudeRay( (s32)((m_OccPos.Z/8.0f)+0.5f),
                          (s32)((m_OccPos.X/8.0f)+0.5f),
                          m_OccPos.Y,
                          (s32)((BBoxCenter.Z/8.0f)+0.5f),
                          (s32)((BBoxCenter.X/8.0f)+0.5f),
                          BBoxCenter.Y) )
            {
#ifdef TERRAIN_TIMERS
                TerrainOccNRaysFound++;
                TerrainOccTimer.Stop();
#endif
                return FALSE;
            }
        }
    }

    vector3 localCamPos;
    localCamPos.X = m_OccPos.Z;
    localCamPos.Y = m_OccPos.X;
    localCamPos.Z = m_OccPos.Y;
    vector3 ul(BBox.Min.Z, BBox.Max.X, BBox.Max.Y);
    vector3 ur(BBox.Max.Z, BBox.Max.X, BBox.Max.Y);
    vector3 ll(BBox.Min.Z, BBox.Min.X, BBox.Max.Y);
    vector3 lr(BBox.Max.Z, BBox.Min.X, BBox.Max.Y);

    vector3 edge0, edge1;
    f32 a, b, c;
    vector3 delta;
   
    if (localCamPos.Y > ul.Y)
    {
      if (localCamPos.X < ll.X)
      {
         // 4
         delta = localCamPos - ul;
         if (x_abs(delta.X) > x_abs(delta.Y))
         {
            // a
            if (localCamPos.Z > ul.Z)
            {
               // back edge
               edge0 = edge1 = ur;

               // project ll back
               a = ll.Y - localCamPos.Y;
               b = ul.X - localCamPos.X;
               c = ur.X - localCamPos.X;
               edge1.Y = localCamPos.Y + (a * (c / b));
            }
            else
            {
               // front edge
               edge0 = edge1 = ll;

               // project ur forward
               a = ur.Y - localCamPos.Y;
               b = ul.X - localCamPos.X;
               c = ur.X - localCamPos.X;
               edge0.Y = localCamPos.Y + (a * (b / c));
            }

            // Move edge0 and edge1 forward to ll.X
            a = edge0.X - localCamPos.X;
            b = ll.X - localCamPos.X;
            edge0 = localCamPos + (edge0 - localCamPos) * (b / a);
            edge1 = localCamPos + (edge1 - localCamPos) * (b / a);
         }
         else
         {
            // b
            if (localCamPos.Z > ul.Z)
            {
               // back edge
               edge0 = edge1 = ll;

               // Project ur back
               a = ur.X - localCamPos.X;
               b = ur.Y - localCamPos.Y;
               c = lr.Y - localCamPos.Y;
               edge0.X = localCamPos.X + (a * (c / b));
            }
            else
            {
               // front edge
               edge0 = edge1 = ur;

               // Project ll forward
               a = ll.X - localCamPos.X;
               b = ul.Y - localCamPos.Y;
               c = ll.Y - localCamPos.Y;
               edge1.X = localCamPos.X + (a * (b / c));
            }

            // Move edge0 and edge1 forward to ul.Y
            a = edge0.Y - localCamPos.Y;
            b = ul.Y - localCamPos.Y;
            edge0 = localCamPos + (edge0 - localCamPos) * (b / a);
            edge1 = localCamPos + (edge1 - localCamPos) * (b / a);
         }
      }
      else if (localCamPos.X > lr.X)
      {
         // 5
         delta = localCamPos - ur;
         if (x_abs(delta.X) > x_abs(delta.Y))
         {
            // a
            if (localCamPos.Z > ul.Z)
            {
               // back edge
               edge1 = edge0 = ul;

               // Project lr back
               a = lr.Y - localCamPos.Y;
               b = lr.X - localCamPos.X;
               c = ll.X - localCamPos.X;
               edge0.Y = localCamPos.Y + (a * (c / b));
            }
            else
            {
               // front edge
               edge0 = edge1 = lr;

               // Project ul forward
               a = ul.Y - localCamPos.Y;
               b = ur.X - localCamPos.X;
               c = ul.X - localCamPos.X;
               edge1.Y = localCamPos.Y + (a * (b / c));
            }

            // Move edge0 and edge1 forward to ur.X
            a = edge0.X - localCamPos.X;
            b = ur.X - localCamPos.X;
            edge0 = localCamPos + (edge0 - localCamPos) * (b / a);
            edge1 = localCamPos + (edge1 - localCamPos) * (b / a);
         }
         else
         {
            // b
            if (localCamPos.Z > ul.Z)
            {
               // back edge
               edge1 = edge0 = lr;

               // Project ul back
               a = ul.X - localCamPos.X;
               b = ul.Y - localCamPos.Y;
               c = ll.Y - localCamPos.Y;
               edge0.X = localCamPos.X + (a * (c / b));
            }
            else
            {
               // front edge
               edge1 = edge0 = ul;

               // Project lr forward
               a = lr.X - localCamPos.X;
               b = ur.Y - localCamPos.Y;
               c = lr.Y - localCamPos.Y;
               edge0.X = localCamPos.X + (a * (b / c));
            }

            // Move edge0 and edge1 forward to ur.Y
            a = edge0.Y - localCamPos.Y;
            b = ur.Y - localCamPos.Y;
            edge0 = localCamPos + (edge0 - localCamPos) * (b / a);
            edge1 = localCamPos + (edge1 - localCamPos) * (b / a);
         }
      }
      else
      {
         // 6
         if (localCamPos.Z > ul.Z)
         {
            edge0 = ur;
            edge1 = ul;

            a = ur.Y - localCamPos.Y;  // short
            b = lr.Y - localCamPos.Y;  // long
            c = lr.Z - localCamPos.Z;  // height
            edge0.Z = edge1.Z = localCamPos.Z + (c * (a / b));
         }
         else
         {
            // front edge
            edge0 = ur;
            edge1 = ul;
         }

         // Move edge0 and edge1 forward to ur.Y
         a = edge0.Y - localCamPos.Y;
         b = ur.Y - localCamPos.Y;
         edge0 = localCamPos + (edge0 - localCamPos) * (b / a);
         edge1 = localCamPos + (edge1 - localCamPos) * (b / a);
      }
   }
   else if (localCamPos.Y < ll.Y)
   {
      if (localCamPos.X < ll.X)
      {
         // 1
         delta = localCamPos - ll;
         if (x_abs(delta.X) > x_abs(delta.Y))
         {
            // a
            if (localCamPos.Z > ul.Z)
            {
               // back edge
               edge1 = edge0 = lr;

               // Project ul back
               a = ul.Y - localCamPos.Y;
               b = ul.X - localCamPos.X;
               c = ur.X - localCamPos.X;
               edge0.Y = localCamPos.Y + (a * (c / b));
            }
            else
            {
               // front edge
               edge0 = edge1 = ul;

               // Project lr forward
               a = lr.Y - localCamPos.Y;
               b = ll.X - localCamPos.X;
               c = lr.X - localCamPos.X;
               edge0.Y = localCamPos.Y + (a * (b / c));
            }

            // Move edge0 and edge1 forward to ll.X
            a = edge0.X - localCamPos.X;
            b = ll.X - localCamPos.X;
            edge0 = localCamPos + (edge0 - localCamPos) * (b / a);
            edge1 = localCamPos + (edge1 - localCamPos) * (b / a);
         }
         else
         {
            // b
            if (localCamPos.Z > ul.Z)
            {
               // back edge
               edge1 = edge0 = ul;

               // Project lr back
               a = lr.X - localCamPos.X;
               b = ll.Y - localCamPos.Y;
               c = ul.Y - localCamPos.Y;
               edge0.X = localCamPos.X + (a * (c / b));
            }
            else
            {
               // front edge
               edge1 = edge0 = lr;

               // Project ul forward
               a = ul.X - localCamPos.X;
               b = ll.Y - localCamPos.Y;
               c = ul.Y - localCamPos.Y;
               edge0.X = localCamPos.X + (a * (b / c));
            }

            // Move edge0 and edge1 forward to ll.Y
            a = edge0.Y - localCamPos.Y;
            b = ll.Y - localCamPos.Y;
            edge0 = localCamPos + (edge0 - localCamPos) * (b / a);
            edge1 = localCamPos + (edge1 - localCamPos) * (b / a);
         }
      }
      else if (localCamPos.X > lr.X)
      {
         // 2
         delta = localCamPos - lr;
         if (x_abs(delta.X) > x_abs(delta.Y))
         {
            // a
            if (localCamPos.Z > ul.Z)
            {
               // back edge
               edge0 = edge1 = ll;

               // Project ur back
               a = ur.Y - localCamPos.Y;
               b = lr.X - localCamPos.X;
               c = ll.X - localCamPos.X;
               edge0.Y = localCamPos.Y + (a * (c / b));
            }
            else
            {
               // front edge
               edge1 = edge0 = ur;

               // Project ll forward
               a = ll.Y - localCamPos.Y;
               b = lr.X - localCamPos.X;
               c = ll.X - localCamPos.X;
               edge1.Y = localCamPos.Y + (a * (b / c));
            }

            // Move edge0 and edge1 forward to lr.X
            a = edge0.X - localCamPos.X;
            b = lr.X - localCamPos.X;
            edge0 = localCamPos + (edge0 - localCamPos) * (b / a);
            edge1 = localCamPos + (edge1 - localCamPos) * (b / a);
         }
         else
         {
            // b
            if (localCamPos.Z > ul.Z)
            {
               // back edge
               edge1 = edge0 = ur;

               // Project ll back
               a = ll.X - localCamPos.X;
               b = lr.Y - localCamPos.Y;
               c = ur.Y - localCamPos.Y;
               edge0.X = localCamPos.X + (a * (c / b));
            }
            else
            {
               // front edge
               edge0 = edge1 = ll;

               // Project ur forward
               a = ur.X - localCamPos.X;
               b = lr.Y - localCamPos.Y;
               c = ur.Y - localCamPos.Y;
               edge1.X = localCamPos.X + (a * (b / c));
            }

            // Move edge0 and edge1 forward to lr.Y
            a = edge0.Y - localCamPos.Y;
            b = lr.Y - localCamPos.Y;
            edge0 = localCamPos + (edge0 - localCamPos) * (b / a);
            edge1 = localCamPos + (edge1 - localCamPos) * (b / a);
         }
      }
      else
      {
         // 3
         if (localCamPos.Z > ul.Z)
         {
            edge0 = ll;
            edge1 = lr;

            a = ll.Y - localCamPos.Y;  // short
            b = ul.Y - localCamPos.Y;  // long
            c = ul.Z - localCamPos.Z;  // height
            edge0.Z = edge1.Z = localCamPos.Z + (c * (a / b));
         }
         else
         {
            // front edge
            edge0 = ll;
            edge1 = lr;
         }

         // Move edge0 and edge1 forward to lr.Y
         a = edge0.Y - localCamPos.Y;
         b = lr.Y - localCamPos.Y;
         edge0 = localCamPos + (edge0 - localCamPos) * (b / a);
         edge1 = localCamPos + (edge1 - localCamPos) * (b / a);
      }
   }
   else
   {
      if (localCamPos.X < ll.X)
      {
         // 7
         if (localCamPos.Z > ul.Z)
         {
            edge0 = ul;
            edge1 = ll;

            a = ll.X - localCamPos.X;  // short
            b = lr.X - localCamPos.X;  // long
            c = lr.Z - localCamPos.Z;  // height
            edge0.Z = edge1.Z = localCamPos.Z + (c * (a / b));
         }
         else
         {
            // front edge
            edge0 = ul;
            edge1 = ll;
         }

         // Move edge0 and edge1 forward to ll.X
         a = edge0.X - localCamPos.X;
         b = ll.X - localCamPos.X;
         edge0 = localCamPos + (edge0 - localCamPos) * (b / a);
         edge1 = localCamPos + (edge1 - localCamPos) * (b / a);
      }
      else if (localCamPos.X > lr.X)
      {
         // 8
         if (localCamPos.Z > ul.Z)
         {
            edge0 = lr;
            edge1 = ur;

            a = lr.X - localCamPos.X;  // short
            b = ll.X - localCamPos.X;  // long
            c = ll.Z - localCamPos.Z;  // height
            edge0.Z = edge1.Z = localCamPos.Z + (c * (a / b));
         }
         else
         {
            // front edge
            edge0 = lr;
            edge1 = ur;
         }

         // Move edge0 and edge1 forward to lr.X
         a = edge0.X - localCamPos.X;
         b = lr.X - localCamPos.X;
         edge0 = localCamPos + (edge0 - localCamPos) * (b / a);
         edge1 = localCamPos + (edge1 - localCamPos) * (b / a);
      }
      else
      {
         ASSERT(FALSE);
      }
   }

   // Raise edges
   edge0.Z += 0.1f;
   edge1.Z += 0.1f;

   delta = edge1 - edge0;
   xbool occluded = FALSE;
   if (delta.X != 0.0f)
   {
//      ASSERT(delta.Y == 0.0f);
      occluded = OcclusionScanX(edge0, edge1, localCamPos);
   }
   else 
    if (delta.Y != 0.0f)
   {
//      ASSERT(delta.X == 0.0f);

      occluded = OcclusionScanY(edge0, edge1, localCamPos);
   }

#ifdef TERRAIN_TIMERS
    TerrainOccTimer.Stop();
    if( occluded == TRUE ) TerrainOccNOccluded++;
#endif
    return occluded;
}

//==============================================================================

xbool terr::OcclusionScanX(const vector3& edge0, const vector3& edge1, const vector3& localCamPos)
{
    //x_printfxy(0,10,"OccScanX");
   //ASSERT( (x_abs(edge0.Y-edge1.Y) < 0.01f) && (x_abs(edge0.Z-edge1.Z) < 0.01f) );

   f32 rightX = MAX(edge0.X, edge1.X);
   f32 leftX  = MIN(edge0.X, edge1.X);

   vector3 left  = edge0;
   vector3 right = edge1;
   left.X = leftX;
   right.X = rightX;
   
   vector3 leftDelta, rightDelta;

   f32 invSquareSize = 1.0f / f32(8.0f);
   u32 iters;
   if (localCamPos.Y < left.Y)
   {
      f32 starty = (x_floor(left.Y * invSquareSize) * f32(8.0f));

      leftDelta.X = (localCamPos.X - left.X) / (left.Y - localCamPos.Y);
      leftDelta.Y = -1;
      leftDelta.Z = (localCamPos.Z - left.Z) / (left.Y - localCamPos.Y);

      rightDelta.X = (localCamPos.X - right.X) / (right.Y - localCamPos.Y);
      rightDelta.Y = -1;
      rightDelta.Z = (localCamPos.Z - right.Z) / (right.Y - localCamPos.Y);

      f32 ydif = left.Y - starty;
      left += leftDelta * ydif;
      ydif = right.Y - starty;
      right += rightDelta * ydif;

      iters = u32((left.Y - localCamPos.Y) / f32(8.0f));
   }
   else if (localCamPos.Y > left.Y)
   {
      f32 starty = (x_ceil(left.Y * invSquareSize) * f32(8.0f));

      leftDelta.X = (localCamPos.X - left.X) / (localCamPos.Y - left.Y);
      leftDelta.Y = 1;
      leftDelta.Z = (localCamPos.Z - left.Z) / (localCamPos.Y - left.Y);

      rightDelta.X = (localCamPos.X - right.X) / (localCamPos.Y - right.Y);
      rightDelta.Y = 1;
      rightDelta.Z = (localCamPos.Z - right.Z) / (localCamPos.Y - right.Y);

      f32 ydif = starty - left.Y;
      left    += leftDelta * ydif;
      ydif     = starty - right.Y;
      right   += rightDelta * ydif;

      iters = u32((localCamPos.Y - left.Y) / f32(8.0f));
   }
   else
   {
      return FALSE;
   }

   left.X *= invSquareSize;
   right.X *= invSquareSize;
   left.Y *= invSquareSize;
   leftDelta *= 8.0f;
   rightDelta *= 8.0f;
   leftDelta.X *= invSquareSize;
   leftDelta.Y *= invSquareSize;
   rightDelta.X *= invSquareSize;
   xbool occluded = FALSE;
   for (u32 i = 0; i < iters; i++)
   {
      // Test the current position
      s32 startX = s32((left.X));
      s32 endX   = s32((right.X)+1.0f);
      s32 height = (s32)(left.Z*HEIGHT_PREC);
      s32 y      = s32(s32(left.Y) & 0xFF);
      s32 YOff0 = ((y>>3)<<5);
      s32 YOff1 = ((y&7)<<3)+(y&7);
      s32 j;

      for ( j = startX; j <= endX; j++)
      {
          s32 x = j&0xFF;
          block* pBlock = &m_Block[ (x>>3)+YOff0 ];
          s32  H = pBlock->Height[ (x&7)+YOff1 ];

          if( pBlock->SquareMask )
              break;

          if( H < height )
              break;
      }

      if( j>endX )
      {
         occluded = true;
         break;
      }

      left  += leftDelta;
      right += rightDelta;
   }
/*
   if (occluded)
   {
      // Have to double check for empty materials...
      left  = storeLeft;
      right = storeRight;

      for (u32 i = 0; i < iters; i++)
      {
         // Test the current position
         s32 startX = s32(mFloor(left.X * invSquareSize));
         s32 endX   = s32(mCeil(right.X * invSquareSize));
         u32 y      = u32(s32(left.Y * invSquareSize) & BlockMask);
         
         for (s32 i = startX; i <= endX; i++)
         {
            u32 x = u32(i & BlockMask);

            if ((getMaterial(x, y)->flags & Material::Empty) != 0)
            {
               occluded = false;
               break;
            }
         }

         if (!occluded)
            break;

         left  += leftDelta  * f32(8.0f);
         right += rightDelta * f32(8.0f);
      }
   }
*/
   return occluded;
}

//==============================================================================

xbool terr::OcclusionScanY(const vector3& edge0, const vector3& edge1, const vector3& localCamPos)
{
    //x_printfxy(0,10,"OccScanY");
   ASSERT( (x_abs(edge0.X-edge1.X) < 0.01f) && (x_abs(edge0.Z-edge1.Z) < 0.01f) );

   f32 topY     = MAX(edge0.Y, edge1.Y);
   f32 bottomY  = MIN(edge0.Y, edge1.Y);

   vector3 bottom  = edge0;
   vector3 top     = edge1;
   bottom.Y = bottomY;
   top.Y    = topY;
   
   vector3 bottomDelta, topDelta;
   f32 invSquareSize = 1.0f / f32(8.0f);

   u32 iters;
   if (localCamPos.X < bottom.X)
   {
      f32 startx = (x_floor(bottom.X * invSquareSize) * f32(8.0f));

      bottomDelta.X = -1;
      bottomDelta.Y = (localCamPos.Y - bottom.Y) / (bottom.X - localCamPos.X);
      bottomDelta.Z = (localCamPos.Z - bottom.Z) / (bottom.X - localCamPos.X);

      topDelta.X = -1;
      topDelta.Y = (localCamPos.Y - top.Y) / (bottom.X - localCamPos.X);
      topDelta.Z = (localCamPos.Z - top.Z) / (bottom.X - localCamPos.X);

      f32 xdif = bottom.X - startx;
      bottom  += bottomDelta * xdif;
      xdif     = top.X - startx;
      top     += topDelta * xdif;

      iters = (u32)((bottom.X - localCamPos.X) / f32(8.0f));
   }
   else if (localCamPos.X > bottom.X)
   {
      f32 startx = (x_ceil(bottom.X * invSquareSize) * f32(8.0f));

      bottomDelta.X = 1;
      bottomDelta.Y = (localCamPos.Y - bottom.Y) / (localCamPos.X - bottom.X);
      bottomDelta.Z = (localCamPos.Z - bottom.Z) / (localCamPos.X - bottom.X);

      topDelta.X = -1;
      topDelta.Y = (localCamPos.Y - top.Y) / (localCamPos.X - top.X);
      topDelta.Z = (localCamPos.Z - top.Z) / (localCamPos.X - top.X);

      f32 xdif = startx - bottom.X;
      bottom  += bottomDelta * xdif;
      xdif     = startx - top.X;
      top     += topDelta * xdif;

      iters = (u32)((localCamPos.X - bottom.X) / f32(8.0f));
   }
   else
   {
      return FALSE;
   }

   xbool occluded = FALSE;
   bottom.Y *= invSquareSize;
   top.Y *= invSquareSize;
   bottom.X *= invSquareSize;
   bottomDelta *= 8.0f;
   topDelta *= 8.0f;
   bottomDelta.Y *= invSquareSize;
   bottomDelta.X *= invSquareSize;
   topDelta.Y *= invSquareSize;
   for (u32 i = 0; i < iters; i++)
   {
      // Test the current position

      s32 j;
      s32 startY = s32((bottom.Y));
      s32 endY   = s32((top.Y)+1.0f);
      u32 x      = u32(s32(bottom.X) & 0xFF);
         
      s32 height = (s32)(bottom.Z*HEIGHT_PREC);
      for (j = startY; j <= endY; j++)
      {
          u32 y = j & 0xFF;
          block* pBlock = &m_Block[ (x>>3)+((y>>3)<<5) ];
          s32 H = pBlock->Height[ (x&7)+((y&7)<<3)+(y&7) ];

          if( pBlock->SquareMask )
              break;

          if( H < height )
              break;
      }

      if( j>endY )
      {
         occluded = true;
         break;
      }
      
      bottom += bottomDelta;
      top    += topDelta;
   }
/*
   if (occluded)
   {
      top = storeTop;
      bottom = storeBottom;
      
      for (u32 i = 0; i < iters; i++)
      {
         // Test the current position

         s32 startY = s32(mFloor(bottom.Y * invSquareSize));
         s32 endY   = s32(mCeil(top.Y * invSquareSize));
         u32 x      = u32(s32(bottom.X * invSquareSize) & BlockMask);
         
         xbool reallyOccluded = true;
         for (s32 i = startY; i <= endY; i++)
         {
            u32 y = u32(i & BlockMask);

            if ((getMaterial(x, y)->flags & Material::Empty) != 0)
            {
               occluded = false;
               break;
            }
         }

         if (!occluded)
            break;
      
         bottom += bottomDelta  * f32(8.0f);
         top    += topDelta     * f32(8.0f);
      }
   }
   */
   return occluded;
}

//==============================================================================


