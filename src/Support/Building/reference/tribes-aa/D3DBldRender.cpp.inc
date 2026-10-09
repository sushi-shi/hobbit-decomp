#include "BldRender.hpp"

//=========================================================================

void DebugRender( building& Render )
{
    eng_Begin(); 


    for( s32 i=0; i<Render.m_nZones; i++ )
    {
        for( int j=0; j<Render.m_pZone[i].nDLists; j++ )
        {
            struct temp_uv
            {
                s16 BaseU, BaseV;
                s16 LMU, LMV;
            };

            building::dlist& DList   = Render.m_pDList[ j + Render.m_pZone[i].iDList ];
            vector4*                    pVertex = (vector4*) ( ((u32*)DList.pData) + 8 );
            temp_uv*                    pUV     = (temp_uv*) ( ((u32*)&pVertex[ DList.nVertices ]) + 4 );

           // VRAM_Activate( Handle );

            int c=0;
            ASSERT(DList.nVertices > 2);

            draw_Begin( DRAW_TRIANGLES, DRAW_TEXTURED );
            //prim_Begin( PRIM_3D_TRIANGLES, TRUE, FALSE );
            draw_Color( xcolor(128,128,128,128) );
            for( int k=0; k<DList.nVertices; k++,c++ )
            {
                //
                // Start the strip
                //
                if( *((u32*)&pVertex[k].W) & (1<<15)  )
                {
                    ASSERT( *((u32*)&pVertex[k+1].W) & (1<<15) );
                    k += 2;
                    c  = 2;
                }
                ASSERT( k<DList.nVertices );

                //
                // Render poly
                //
                {
                    for( int t=0; t<3; t++)
                    {
                        float u,v;
                        static const int off[2][3]={ {2,1,0}, {1,2,0} };
                        int m = k - off[c&1][t];

                        if( 1 )
                        {
                            u = pUV[m].BaseU;
                            v = pUV[m].BaseV;

                            u *= (1.0f/(1<<10));
                            v *= (1.0f/(1<<10));
                        }
                        else
                        {
                            u = pUV[m].LMU;
                            v = pUV[m].LMV;

                            u *= (1.0f/(1<<14));
                            v *= (1.0f/(1<<14));
                        }

                        draw_UV( u,v );                

                        draw_Vertex( pVertex[m].X, 
                                     pVertex[m].Y, 
                                     pVertex[m].Z ); 

                    }
                }
            }
            draw_End();
        }
    }

    eng_End();
}

//=========================================================================

void BLDRD_Initialize( void )
{

}

//=========================================================================

void BLDRD_Begin( void )
{
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,      D3DTOP_MODULATE    );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1,    D3DTA_TEXTURE      );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2,    D3DTA_TFACTOR      );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,      D3DTOP_DISABLE     );

    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,      D3DTOP_DISABLE      );
    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,      D3DTOP_DISABLE      );
    //g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,      D3DTOP_MODULATE    );
    //g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG1,    D3DTA_TEXTURE      );
    //g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLORARG2,    D3DTA_CURRENT      );
    //g_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,      D3DTOP_DISABLE     );

    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR  );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR  );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR  );

    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR  );
    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR  );
    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_MIPFILTER, D3DTEXF_LINEAR  );

    g_pd3dDevice->SetTextureStageState( 2, D3DTSS_ALPHAOP,      D3DTOP_DISABLE      );
    g_pd3dDevice->SetTextureStageState( 2, D3DTSS_COLOROP,      D3DTOP_DISABLE      );

    g_pd3dDevice->SetVertexShader( (D3DFVF_XYZ|D3DFVF_TEX2) );

//    g_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );
    g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
}

//=========================================================================

void BLDRD_End( void )
{
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,      D3DTOP_MODULATE    );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1,    D3DTA_TEXTURE      );
    g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2,    D3DTA_DIFFUSE      );

    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,      D3DTOP_DISABLE      );
    g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,      D3DTOP_DISABLE      );

    g_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
}

//=========================================================================

void BLDRD_Delay(void)
{
}

//=========================================================================

void BLDRD_RenderDList( const building::dlist& DList, xbool DoClip )
{
    building::dlist::pc_data* pData = (building::dlist::pc_data*)DList.pData;

    


    g_pd3dDevice->SetStreamSource   ( 0, pData->pVBuffer, sizeof(building::dlist::pc_vertex) );
    g_pd3dDevice->SetIndices        ( pData->pIBuffer, 0 );

    // Without this check, D3D can crash when the device is lost (due to alt+tab)
    if (g_pd3dDevice->TestCooperativeLevel() == D3D_OK)
        g_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 
                                            0, DList.nVertices, 
                                            0, pData->FacetCount );


    /*
    struct temp_uv
    {
        s16 BaseU, BaseV;
        s16 LMU, LMV;
    };    

    vector4*        pVertex = (vector4*) ( ((u32*)((building::dlist::pc_data*)DList.pData)->pDList) + 8 );
    temp_uv*        pUV     = (temp_uv*) ( ((u32*)&pVertex[ DList.nVertices ]) + 4 );
    int             c=0;

    for( int k=0; k<DList.nVertices; k++,c++ )
    {
        //
        // Start the strip
        //
        if( *((u32*)&pVertex[k].W) & (1<<15)  )
        {
       //     ASSERT( *((u32*)&pVertex[k+1].W) & (1<<15) );
            k += 2;
            c  = 2;
        }
        ASSERT( k<DList.nVertices );

        //
        // Render poly
        //
        {
            struct prim_vertex{ float x,y,z; xcolor C; float u,v; };
            prim_vertex     VertexBuff[128];
            int             Inc=0;

            for( int t=0; t<3; t++)
            {
                float u,v;
                static const int off[2][3]={ {2,1,0}, {1,2,0} };
                int m = k - off[c&1][t];

                if( 1 )
                {
                    u = pUV[m].BaseU;
                    v = pUV[m].BaseV;

                    u *= (1.0f/(1<<10));
                    v *= (1.0f/(1<<10));
                }
                else
                {
                    u = pUV[m].LMU;
                    v = pUV[m].LMV;

                    u *= (1.0f/(1<<14));
                    v *= (1.0f/(1<<14));
                }

                VertexBuff[ Inc ].x = pVertex[m].X;
                VertexBuff[ Inc ].y = pVertex[m].Y;
                VertexBuff[ Inc ].z = pVertex[m].Z;

                VertexBuff[ Inc ].u = u;
                VertexBuff[ Inc ].v = v;

                VertexBuff[ Inc ].C = xcolor(128,128,128,128);
                Inc++;
            }
            g_pd3dDevice->SetVertexShader( (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1) );
            g_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, Inc-2, VertexBuff, sizeof(prim_vertex) );
        }
    }
*/

}

//=========================================================================

//void BLDRD_UpdaloadMatrices ( const matrix4& L2W, const matrix4& W2V, const matrix4& V2C )
void BLDRD_UpdaloadMatrices( const matrix4& L2W, const matrix4& W2V, const matrix4& V2C,
                             const matrix4& L2W2,
                             const vector3& WorldEyePos,
                             const vector3& LocalEyePos,
                             const vector4& FogMul,
                             const vector4& FogAdd)

{
    g_pd3dDevice->SetTransform( D3DTS_WORLD,      (D3DMATRIX*)&L2W );
    g_pd3dDevice->SetTransform( D3DTS_VIEW,       (D3DMATRIX*)&W2V );
    g_pd3dDevice->SetTransform( D3DTS_PROJECTION, (D3DMATRIX*)&V2C );

    g_pd3dDevice->SetRenderState( D3DRS_TEXTUREFACTOR , xcolor( 255,255,255,255) );
}

//=========================================================================

void BLDRD_UpdaloadBase( const xbitmap& Bitmap, f32 MinLOD, f32 MaxLOD )
{
    g_pd3dDevice->SetTexture( 0, vram_GetSurface( Bitmap ) );
}

//=========================================================================

void BLDRD_UpdaloadLightMap ( const xbitmap& Bitmap, s32* pClutHandle )
{
    g_pd3dDevice->SetTexture( 1, vram_GetSurface( Bitmap ) );
}

//=========================================================================

void BLDRD_UpdaloadFog( const xbitmap& Bitmap )
{
}

