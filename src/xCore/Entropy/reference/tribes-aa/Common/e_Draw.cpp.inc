//==============================================================================
//  
//  e_Draw.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/Entropy/Entropy.hpp>
#include <xCore/Entropy/e_Draw.hpp>

//==============================================================================
//  FUNCTIONS
//==============================================================================

void draw_Line( const vector3& P0,
                const vector3& P1,
                      xcolor   Color )
{
    draw_Begin( DRAW_LINES );
        draw_Color( Color );
        draw_Vertex( P0 );
        draw_Vertex( P1 );
    draw_End();
}

//==============================================================================

void draw_BBox( const bbox&    BBox,
                      xcolor   Color )
{
    static s16 Index[] = {1,5,5,7,7,3,3,1,0,4,4,6,6,2,2,0,3,2,7,6,5,4,1,0};
    vector3 P[8];

    P[0].X = BBox.Min.X;    P[0].Y = BBox.Min.Y;    P[0].Z = BBox.Min.Z;
    P[1].X = BBox.Min.X;    P[1].Y = BBox.Min.Y;    P[1].Z = BBox.Max.Z;
    P[2].X = BBox.Min.X;    P[2].Y = BBox.Max.Y;    P[2].Z = BBox.Min.Z;
    P[3].X = BBox.Min.X;    P[3].Y = BBox.Max.Y;    P[3].Z = BBox.Max.Z;
    P[4].X = BBox.Max.X;    P[4].Y = BBox.Min.Y;    P[4].Z = BBox.Min.Z;
    P[5].X = BBox.Max.X;    P[5].Y = BBox.Min.Y;    P[5].Z = BBox.Max.Z;
    P[6].X = BBox.Max.X;    P[6].Y = BBox.Max.Y;    P[6].Z = BBox.Min.Z;
    P[7].X = BBox.Max.X;    P[7].Y = BBox.Max.Y;    P[7].Z = BBox.Max.Z;

    draw_Begin( DRAW_LINES, DRAW_USE_ALPHA );
        draw_Color( Color );
        draw_Verts( P, 8 );
        draw_Execute( Index, sizeof(Index)/sizeof(s16) );
    draw_End();
}

//==============================================================================

void draw_NGon( const vector3* pPoint,
                      s32      NPoints,
                      xcolor   Color,
                      xbool    DoWire )
{
    if( DoWire )
    {
        s32 P = NPoints-1;
        draw_Begin(DRAW_LINES, DRAW_NO_ZBUFFER);
            draw_Color(Color);
            for( s32 i=0; i<NPoints; i++ )
            {
                draw_Vertex(pPoint[P]);
                draw_Vertex(pPoint[i]);
                P = i;
            }
        draw_End();
    }
    else
    {
        draw_Begin(DRAW_TRIANGLES, DRAW_NO_ZBUFFER);
            draw_Color(Color);
            for( s32 i=1; i<NPoints-1; i++ )
            {
                draw_Vertex(pPoint[0]);
                draw_Vertex(pPoint[i]);
                draw_Vertex(pPoint[i+1]);
            }
        draw_End();
    }
}

//==============================================================================

void draw_Sphere( const vector3& Pos,
                        f32      Radius,
                        xcolor   Color)
{
    #define A  0.8506f
    #define B  0.5257f
    static vector3  SphereV[12] = 
    { 
      vector3(+A, 0,+B), vector3(+A, 0,-B), vector3(-A, 0,+B), vector3(-A, 0,-B),
      vector3(+B,+A, 0), vector3(-B,+A, 0), vector3(+B,-A, 0), vector3(-B,-A, 0),
      vector3( 0,+B,+A), vector3( 0,-B,+A), vector3( 0,+B,-A), vector3( 0,-B,-A),
    };
    #undef A
    #undef B
    
    static s16 Index[] = 
    { 0,1,1,4,0,4,0,6,1,6,2,3,2,5,3,5,3,7,2,7,4,5,5,8,4,8,4,10,5,10,6,
      7,6,9,7,9,7,11,6,11,0,8,0,9,8,9,2,8,2,9,1,10,10,11,1,11,3,10,3,11 };
        
    vector3 P[12];
    for( s32 i=0; i<12; i++ )
        P[i] = Pos + SphereV[i] * Radius;

    draw_Begin( DRAW_LINES, DRAW_USE_ALPHA );
        draw_Color( Color );
        draw_Verts( P, 12 );
        draw_Execute( Index, sizeof(Index)/sizeof(s16) );
    draw_End();
}

//==============================================================================

void draw_Marker( const vector3& Pos,
                        xcolor   Color )
{
//    s32 i;
/*
    u32 ActiveViewMask;

    // Get currently active views and shut them all off
    ActiveViewMask = eng_GetActiveViewMask();
    for( i=0; i<ENG_MAX_VIEWS; i++ )
        eng_DeactivateView(i);

    // Loop through active views and render
    for( i=0; i<ENG_MAX_VIEWS; i++ )
    if( ActiveViewMask & (1<<i) )
*/
    {
        const view* pView = eng_GetActiveView(0);

        // Get screen coordinates of projected point
        vector3 ScreenPos;
        ScreenPos = pView->PointToScreen( Pos );

        // Get screen viewport bounds
        s32 X0,Y0,X1,Y1;
        pView->GetViewport(X0,Y0,X1,Y1);

        // Peg screen coordinates to viewport bounds
        ScreenPos.X = MAX( X0, ScreenPos.X );
        ScreenPos.X = MIN( X1, ScreenPos.X );
        ScreenPos.Y = MAX( Y0, ScreenPos.Y );
        ScreenPos.Y = MIN( Y1, ScreenPos.Y );

        // Make screen pos relative to viewport
        ScreenPos.X -= X0;
        ScreenPos.Y -= Y0;

        // For view 'i'...
        //eng_ActivateView(i);
        
        // Draw away.
        draw_Begin( DRAW_QUADS, DRAW_2D );
        {
            draw_Color( Color );
            draw_Vertex( ScreenPos.X,    ScreenPos.Y-8, 0 );
            draw_Vertex( ScreenPos.X-8,  ScreenPos.Y,   0 );
            draw_Vertex( ScreenPos.X,    ScreenPos.Y+8, 0 );
            draw_Vertex( ScreenPos.X+8,  ScreenPos.Y,   0 );

            // If behind the screen, add a black center.
            if( ScreenPos.Z < 0 )
            {
                draw_Color( XCOLOR_BLACK );
                draw_Vertex( ScreenPos.X,    ScreenPos.Y-4, 0 );
                draw_Vertex( ScreenPos.X-4,  ScreenPos.Y,   0 );
                draw_Vertex( ScreenPos.X,    ScreenPos.Y+4, 0 );
                draw_Vertex( ScreenPos.X+4,  ScreenPos.Y,   0 );
            }
        }
        draw_End();
        //eng_DeactivateView(i);
    }
/*
    // Reactivate old views
    for( i=0; i<ENG_MAX_VIEWS; i++ )
    if( ActiveViewMask & (1<<i) )
        eng_ActivateView(i);
*/
}

//==============================================================================

void draw_Point( const vector3& Pos,
                       xcolor   Color )
{
/*
    s32 i;
    //u32 ActiveViewMask;

    // Get currently active views and shut them all off

    ActiveViewMask = eng_GetActiveViewMask();
    for( i=0; i<ENG_MAX_VIEWS; i++ )
        eng_DeactivateView(i);

     Loop through active views and render
    for( i=0; i<ENG_MAX_VIEWS; i++ )
    if( ActiveViewMask & (1<<i) )
*/
    {
        const view* pView = eng_GetActiveView(0);

        // Get screen coordinates of projected point
        vector3 ScreenPos;
        ScreenPos = pView->PointToScreen( Pos );

        // If behind camera, just forget about it.
        if( ScreenPos.Z < 0 )
            return;

        // Get screen viewport bounds
        s32 X0,Y0,X1,Y1;
        pView->GetViewport(X0,Y0,X1,Y1);

        // Make screen pos relative to viewport
        //ScreenPos.X -= X0;
        //ScreenPos.Y -= Y0;

        // For view 'i'...
        //eng_ActivateView(i);
        
        // Draw away.
        draw_Begin( DRAW_QUADS, DRAW_2D );
        {
            draw_Color( Color );
            draw_Vertex( ScreenPos.X-2,  ScreenPos.Y-2, 0 );
            draw_Vertex( ScreenPos.X-2,  ScreenPos.Y+2, 0 );
            draw_Vertex( ScreenPos.X+2,  ScreenPos.Y+2, 0 );
            draw_Vertex( ScreenPos.X+2,  ScreenPos.Y-2, 0 );
        }
        draw_End();
        //eng_DeactivateView(i);
    }
/*
    // Reactivate old views
    for( i=0; i<ENG_MAX_VIEWS; i++ )
    if( ActiveViewMask & (1<<i) )
        eng_ActivateView(i);
*/
}

//==============================================================================

void draw_Frustum( const view&    View,
                         xcolor   Color,
                         f32      Dist )
{
    static s16 Index[8*2] = {0,1,0,2,0,3,0,4,1,2,2,3,3,4,4,1};
    vector3 P[6];
    s32     X0,X1,Y0,Y1;

    View.GetViewport(X0,Y0,X1,Y1);

    P[0] = View.GetPosition();
    P[1] = View.RayFromScreen( (f32)X0, (f32)Y0, view::VIEW );
    P[2] = View.RayFromScreen( (f32)X0, (f32)Y1, view::VIEW );
    P[3] = View.RayFromScreen( (f32)X1, (f32)Y1, view::VIEW );
    P[4] = View.RayFromScreen( (f32)X1, (f32)Y0, view::VIEW );

    // Normalize so that Z is Dist and move into world
    for( s32 i=1; i<=4; i++ )
    {
        P[i] *= Dist / P[i].Z;
        P[i]  = View.ConvertV2W( P[i] );
    }

    P[5] = (P[1] + P[2] + P[3] + P[4]) * 0.25f;

    draw_Begin( DRAW_LINES );
        draw_Color( Color );
        draw_Verts( P, 5 );
        draw_Execute( Index, 8*2 );
        draw_Color( XCOLOR_GREY );
        draw_Vertex(P[0]);
        draw_Vertex(P[5]);
    draw_End();
}

//==============================================================================

void draw_Axis( f32 Size )
{
    draw_Begin( DRAW_LINES );

        draw_Color(XCOLOR_WHITE);   draw_Vertex(0,0,0);
        draw_Color(XCOLOR_RED);     draw_Vertex(Size,0,0);
        draw_Color(XCOLOR_WHITE);   draw_Vertex(0,0,0);
        draw_Color(XCOLOR_GREEN);   draw_Vertex(0,Size,0);
        draw_Color(XCOLOR_WHITE);   draw_Vertex(0,0,0);
        draw_Color(XCOLOR_BLUE);    draw_Vertex(0,0,Size);

        /*
        draw_Color( xcolor(255,0,0,255) );
        draw_Vertex ( Size, 0, 0 );  draw_Vertex ( 0, W, 0 );
        draw_Vertex ( Size, 0, 0 );  draw_Vertex ( 0,-W, 0 );
        draw_Vertex ( Size, 0, 0 );  draw_Vertex ( 0, 0,-W );
        draw_Vertex ( Size, 0, 0 );  draw_Vertex ( 0, 0, W );

        draw_Color( xcolor(0,255,0,255) );
        draw_Vertex ( 0, Size, 0 );  draw_Vertex ( W, 0, 0 );
        draw_Vertex ( 0, Size, 0 );  draw_Vertex (-W, 0, 0 );
        draw_Vertex ( 0, Size, 0 );  draw_Vertex ( 0, 0,-W );
        draw_Vertex ( 0, Size, 0 );  draw_Vertex ( 0, 0, W );

        draw_Color( xcolor(0,0,255,255) );
        draw_Vertex ( 0, 0, Size );  draw_Vertex ( 0, W, 0 );
        draw_Vertex ( 0, 0, Size );  draw_Vertex ( 0,-W, 0 );
        draw_Vertex ( 0, 0, Size );  draw_Vertex (-W, 0, 0 );
        draw_Vertex ( 0, 0, Size );  draw_Vertex ( W, 0, 0 );
        */

    draw_End();
}

//==============================================================================

void draw_Grid( const vector3& Corner,
                const vector3& Edge1,
                const vector3& Edge2,
                      xcolor   Color, 
                      s32      NSubdivisions )
{
    s32 NWires = NSubdivisions+1;

    draw_Begin( DRAW_LINES );
    draw_Color( Color );

        for( s32 w=0; w<NWires; w++ )
        {
            f32 T = (f32)w/(f32)(NWires-1);
            draw_Vertex( Corner + Edge1*T );
            draw_Vertex( Corner + Edge1*T + Edge2 );
            draw_Vertex( Corner + Edge2*T );
            draw_Vertex( Corner + Edge2*T + Edge1 );
        }
    
    draw_End();
}

//==============================================================================

void draw_Label( const vector3& Pos,
                       xcolor   Color,
                 const char* pFormatStr, ... )
{
    s32 i;
    s32 CharsPerLine = 32;
    
    // Build message
    char Msg[128];
    x_va_list   Args;
    x_va_start( Args, pFormatStr );
    x_vsprintf( Msg, pFormatStr, Args );
    s32 MsgLen = x_strlen(Msg);
    s32 NLines = MsgLen/CharsPerLine;
    if( NLines*CharsPerLine < MsgLen ) NLines++;

    // Search for newlines
    for( i=0; i<MsgLen; i++ )
        if( Msg[i] == '\n' ) NLines++;

    // Get character sizes
    s32 Dummy;
    s32 CharWidth;
    s32 CharHeight;
    text_GetParams( Dummy, Dummy, Dummy, Dummy,
                    CharWidth, CharHeight, Dummy );

    // Compute pixel offset from center
    s32 OffsetY = -(CharHeight*NLines/2);

    // Push requested color
    text_PushColor(Color);

    // Loop through active views and render
    //u32 ActiveViewMask = eng_GetActiveViewMask();
    //for( i=0; i<ENG_MAX_VIEWS; i++ )
    //if( ActiveViewMask & (1<<i) )
    {
        const view* pView = eng_GetActiveView(0);

        // Get screen coordinates of projected point
        vector3 ScreenPos;
        ScreenPos = pView->PointToScreen( Pos );

        // Get screen viewport bounds
        s32 X0,Y0,X1,Y1;
        pView->GetViewport(X0,Y0,X1,Y1);

        xbool SkipDraw = FALSE;
        if( ScreenPos.X < X0 ) SkipDraw = TRUE;
        if( ScreenPos.X > X1 ) SkipDraw = TRUE;
        if( ScreenPos.Y < Y0 ) SkipDraw = TRUE;
        if( ScreenPos.Y > Y1 ) SkipDraw = TRUE;
        if( ScreenPos.Z < 0  ) SkipDraw = TRUE;

        // Compute 
        if( !SkipDraw )
        {
            s32 SX;
            s32 SY = (s32)ScreenPos.Y;
            s32 j = 0;
            s32 LenLeft = MsgLen;
            while( j<MsgLen )
            {
                xbool Newline = FALSE;
                s32 NChars = MIN(LenLeft,CharsPerLine);

                // Check for newline
                for( s32 k=j; k<j+NChars; k++ )
                if( Msg[k] == '\n' )
                {
                    NChars = k-j;
                    Newline = TRUE;
                }

                char C = Msg[j+NChars];
                Msg[j+NChars] = 0;

                SX = (s32)(ScreenPos.X -(CharWidth*NChars)/2);
                text_PrintPixelXY( &Msg[j], 
                                   (s32)(SX), 
                                   (s32)(SY + OffsetY) );

                Msg[j+NChars] = C;
                j+=NChars;
                LenLeft-=NChars;
                if( Newline )
                { 
                    j++;
                    LenLeft--;
                }
                SY += CharHeight;
            }
        }
    }

    // Return to original color
    text_PopColor();
}

//==============================================================================

void draw_Rect( const rect&    Rect,
                xcolor         Color,
                xbool          DoWire)
{
    f32 Near = 0.001f;

    if( DoWire )
    {
        if( (Rect.GetWidth() == 1) && (Rect.GetHeight() == 1) )
        {
            draw_Begin( DRAW_POINTS, DRAW_2D|DRAW_USE_ALPHA|DRAW_NO_ZBUFFER );
            draw_Color( Color );

            draw_Vertex( Rect.Min.X, Rect.Min.Y, Near );
        }
        else
        {
            draw_Begin( DRAW_LINES, DRAW_2D|DRAW_USE_ALPHA|DRAW_NO_ZBUFFER );
            draw_Color( Color );


            draw_Vertex( Rect.Min.X  , Rect.Min.Y  , Near );
            draw_Vertex( Rect.Min.X  , Rect.Max.Y-1, Near );

            draw_Vertex( Rect.Min.X  , Rect.Max.Y-1, Near );
            draw_Vertex( Rect.Max.X-1, Rect.Max.Y-1, Near );

            draw_Vertex( Rect.Max.X-1, Rect.Max.Y-1, Near );
            draw_Vertex( Rect.Max.X-1, Rect.Min.Y  , Near );

            draw_Vertex( Rect.Max.X-1, Rect.Min.Y  , Near );
            draw_Vertex( Rect.Min.X  , Rect.Min.Y  , Near );
        }
    }
    else
    {
        draw_Begin( DRAW_QUADS, DRAW_2D|DRAW_USE_ALPHA|DRAW_NO_ZBUFFER );
        draw_Color( Color );

        draw_Vertex( Rect.Min.X, Rect.Min.Y, Near );
        draw_Vertex( Rect.Min.X, Rect.Max.Y, Near );
        draw_Vertex( Rect.Max.X, Rect.Max.Y, Near );
        draw_Vertex( Rect.Max.X, Rect.Min.Y, Near );
    }

    draw_End();
}

//==============================================================================

void draw_Rect( const irect&   Rect,
                xcolor         Color,
                xbool          DoWire)
{
    f32 Near = 0.001f;

    if( DoWire )
    {
        if( (Rect.GetWidth() == 1) && (Rect.GetHeight() == 1) )
        {
            draw_Begin( DRAW_POINTS, DRAW_2D|DRAW_USE_ALPHA|DRAW_NO_ZBUFFER );
            draw_Color( Color );

            draw_Vertex( (f32)Rect.l, (f32)Rect.t, Near );
        }
        else
        {
            draw_Begin( DRAW_LINES, DRAW_2D|DRAW_USE_ALPHA|DRAW_NO_ZBUFFER );
            draw_Color( Color );


            draw_Vertex( (f32)Rect.l    , (f32)Rect.t    , Near );
            draw_Vertex( (f32)Rect.l    , (f32)(Rect.b-1), Near );

            draw_Vertex( (f32)Rect.l    , (f32)(Rect.b-1), Near );
            draw_Vertex( (f32)(Rect.r-1), (f32)(Rect.b-1), Near );

            draw_Vertex( (f32)(Rect.r-1), (f32)(Rect.b-1), Near );
            draw_Vertex( (f32)(Rect.r-1), (f32)Rect.t    , Near );

            draw_Vertex( (f32)(Rect.r-1), (f32)Rect.t    , Near );
            draw_Vertex( (f32)Rect.l    , (f32)Rect.t    , Near );
        }
    }
    else
    {
        draw_Begin( DRAW_QUADS, DRAW_2D|DRAW_USE_ALPHA|DRAW_NO_ZBUFFER );
        draw_Color( Color );

        draw_Vertex( (f32)Rect.l, (f32)Rect.t, Near );
        draw_Vertex( (f32)Rect.l, (f32)Rect.b, Near );
        draw_Vertex( (f32)Rect.r, (f32)Rect.b, Near );
        draw_Vertex( (f32)Rect.r, (f32)Rect.t, Near );
    }

    draw_End();
}

//==============================================================================

void draw_GouraudRect( const irect&   Rect,
                       const xcolor&  c1,
                       const xcolor&  c2,
                       const xcolor&  c3,
                       const xcolor&  c4,
                       xbool          DoWire )
{
    f32 Near = 0.001f;

    if( DoWire )
    {
        if( (Rect.GetWidth() == 1) && (Rect.GetHeight() == 1) )
        {
            draw_Begin( DRAW_POINTS, DRAW_2D|DRAW_USE_ALPHA|DRAW_NO_ZBUFFER );

            draw_Color( c1 );
            draw_Vertex( (f32)Rect.l, (f32)Rect.t, Near );
        }
        else
        {
            draw_Begin( DRAW_LINES, DRAW_2D|DRAW_USE_ALPHA|DRAW_NO_ZBUFFER );

            draw_Color( c1 );
            draw_Vertex( (f32)Rect.l    , (f32)Rect.t    , Near );
            draw_Color( c2 );
            draw_Vertex( (f32)Rect.l    , (f32)(Rect.b-1), Near );

            draw_Vertex( (f32)Rect.l    , (f32)(Rect.b-1), Near );
            draw_Color( c3 );
            draw_Vertex( (f32)(Rect.r-1), (f32)(Rect.b-1), Near );

            draw_Vertex( (f32)(Rect.r-1), (f32)(Rect.b-1), Near );
            draw_Color( c4 );
            draw_Vertex( (f32)(Rect.r-1), (f32)Rect.t    , Near );

            draw_Vertex( (f32)(Rect.r-1), (f32)Rect.t    , Near );
            draw_Color( c1 );
            draw_Vertex( (f32)Rect.l    , (f32)Rect.t    , Near );
        }
    }
    else
    {
        draw_Begin( DRAW_QUADS, DRAW_2D|DRAW_USE_ALPHA|DRAW_NO_ZBUFFER );

        draw_Color( c1 );
        draw_Vertex( (f32)Rect.l, (f32)Rect.t, Near );
        draw_Color( c2 );
        draw_Vertex( (f32)Rect.l, (f32)Rect.b, Near );
        draw_Color( c3 );
        draw_Vertex( (f32)Rect.r, (f32)Rect.b, Near );
        draw_Color( c4 );
        draw_Vertex( (f32)Rect.r, (f32)Rect.t, Near );
    }

    draw_End();
}
