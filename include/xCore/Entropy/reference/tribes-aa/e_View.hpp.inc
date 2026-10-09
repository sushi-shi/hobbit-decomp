//==============================================================================
//  
//  e_View.hpp
//
//==============================================================================

#ifndef E_VIEW_HPP
#define E_VIEW_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/x_files/x_math.hpp>

//==============================================================================
//  TYPES
//==============================================================================

class view
{   

//------------------------------------------------------------------------------
//  Local Types
//------------------------------------------------------------------------------

public:
    // Coordinate systems
    enum system { WORLD, VIEW };

    // Visiblity results
    enum visible
    {
        VISIBLE_NONE,     // Totally outside of volume
        VISIBLE_FULL,     // Totally inside of volume
        VISIBLE_PARTIAL   // Partially visible in volume (clipping maybe needed)
    } ;


//------------------------------------------------------------------------------
//  Public Functions
//------------------------------------------------------------------------------

public:
                                
                view            ( void );
                view            ( const view& View );
               ~view            ( void );
                
void            SetViewport     ( s32 X0, s32 Y0, s32 X1, s32 Y1 );
void            SetXFOV         ( radian XFOV );
void            SetYFOV         ( radian YFOV );
void            SetZLimits      ( f32 ZNear, f32 ZFar );
                
void            SetPosition     ( const vector3& Position );
void            SetRotation     ( const radian3& Rotation );
void            SetV2W          ( const matrix4& V2W      );
                
void            Translate       ( const vector3& Translation, system System = WORLD );
void            RotateX         ( radian Angle, system System = WORLD );
void            RotateY         ( radian Angle, system System = WORLD );
void            RotateZ         ( radian Angle, system System = WORLD );

//------------------------------------------------------------------------------

void            GetViewport     ( s32& X0, s32& Y0, s32& X1, s32& Y1 ) const;
void            GetViewport     ( rect& Rect ) const;

void            GetPixel        ( f32 ParamX, f32 ParamY, s32& X, s32& Y ) const;
void            GetZLimits      ( f32& ZNear, f32& ZFar ) const;
void            GetPitchYaw     ( radian& Pitch, radian& Yaw ) const;
                
radian          GetXFOV         ( void ) const;
radian          GetYFOV         ( void ) const;
                
vector3         GetPosition     ( void ) const;

const matrix4&  GetW2V          ( void ) const;
const matrix4&  GetV2W          ( void ) const;
const matrix4&  GetV2C          ( void ) const;
const matrix4&  GetC2S          ( void ) const;
const matrix4&  GetV2S          ( void ) const;
const matrix4&  GetW2C          ( void ) const;
const matrix4&  GetW2S          ( void ) const;

vector3         GetViewX        ( void ) const;     // "Camera Left"
vector3         GetViewY        ( void ) const;     // "Camera Up"
vector3         GetViewZ        ( void ) const;     // "Camera Line of Sight"

f32             GetScreenDist   ( void ) const;

//------------------------------------------------------------------------------

void            LookAtPoint     ( const vector3& Point, 
                                        system   System = WORLD );
                
void            LookAtPoint     ( const vector3& FromPoint, 
                                  const vector3& ToPoint, 
                                        system   System = WORLD );

void            OrbitPoint      ( const vector3& Point, 
                                        f32      Distance,
                                        radian   Pitch,
                                        radian   Yaw );

//------------------------------------------------------------------------------
//  These functions build planes from the frustum taking into account the 
//  viewport size and field of view.  All the planes face inward.  A point in 
//  the view frustum will be inside all the planes.
//------------------------------------------------------------------------------

void            GetViewPlanes   ( plane& Top,
                                  plane& Bottom,
                                  plane& Left,
                                  plane& Right,
                                  system System = WORLD ) const;
                
void            GetViewPlanes   ( plane& Top,
                                  plane& Bottom,
                                  plane& Left,
                                  plane& Right,
                                  plane& Near,
                                  plane& Far,
                                  system System = WORLD ) const;

//------------------------------------------------------------------------------
//  This version of the plane function returns the internally cached planes,
//  so it is very quick.  The order of the planes is rigged to improve culling
//  operations.  Essentially, the planes are ordered so that, statistically, 
//  "stuff" can be culled by one of the first few planes thus avoiding the need
//  to test every plane.
//
//  0=Left  1=Right  2=Bottom  3=Top  4=Near  5=Far
//------------------------------------------------------------------------------

const plane*    GetViewPlanes   ( system System = WORLD ) const;

const s32*      GetViewPlaneMinBBoxIndices( system System = WORLD ) const;
const s32*      GetViewPlaneMaxBBoxIndices( system System = WORLD ) const;

void            GetMinMaxZ  ( const bbox& BBox, f32& MinZ, f32& MaxZ ) const;

//------------------------------------------------------------------------------
//  This version of the plane function allows you to get the planes which pass
//  through a region of the viewport.  The X/Y coordinates specify a rectangle
//  with the familiar SCREEN coordinates where +X=Right and +Y=Down.  The (0,0)
//  point is NOT the top left.  Rather, it is where the line of sight passes
//  through.  Note that the region you request can be smaller OR larger than the
//  view's viewport or even larger than the entire screen.
//
//  For example, say you want to get the planes of a region which is W pixels 
//  wide and H pixels tall centered about the line of sight.  (Note that the 
//  position of the line of sight on the screen and the current viewport 
//  settings are not relevant.)  You would use values (-W/2, -H/2, W/2, H/2).
//------------------------------------------------------------------------------

void            GetViewPlanes   ( f32 X0, f32 Y0, f32 X1, f32 Y1,
                                  plane& Top,
                                  plane& Bottom,
                                  plane& Left,
                                  plane& Right,
                                  plane& Near,
                                  plane& Far,
                                  system System = WORLD ) const;
                
/*              
void            GetViewEdges    ( vector3& TLEdge, 
                                  vector3& TREdge, 
                                  vector3& BLEdge, 
                                  vector3& BREdge, 
                                  system   System = WORLD ) const;
                
void            GetProjection   ( f32& ProjectXC0, 
                                  f32& ProjectXC1,
                                  f32& ProjectYC0, 
                                  f32& ProjectYC1 ) const;
*/
//------------------------------------------------------------------------------

void            GetProjection   ( f32& XP0, f32& XP1, f32& YP0, f32& YP1 ) const;
//------------------------------------------------------------------------------

xbool           PointInView     ( const vector3& Point, 
                                        system   System = WORLD ) const;

//------------------------------------------------------------------------------
//  These are quick implementations, and can give false positives in certain
//  situations.  The return values are 0="outside of view", 1="completely within
//  view", and 2="partially within view".  Note that you can treat the return
//  value as a boolean for the question "Visible?".
//------------------------------------------------------------------------------

s32             SphereInView    ( const vector3& Center,
                                        f32      Radius,
                                        system   System = WORLD ) const;
                
s32             BBoxInView      ( const bbox&    BBox,
                                        system   System = WORLD ) const;

xbool           SphereInCone    ( const vector3& Center,
                                        f32      Radius ) const;

xbool           SphereInConeAngle( const vector3& Center,
                                         f32      Radius,
                                         f32      tanAngle ) const;

//------------------------------------------------------------------------------

vector3         ConvertW2V      ( const vector3& Point ) const;
vector3         ConvertV2W      ( const vector3& Point ) const;
                
vector3         PointToScreen   ( const vector3& Point,
                                        system   System = WORLD ) const;
                
vector3         RayFromScreen   (       f32      ScreenX,
                                        f32      ScreenY,
                                        system   System = WORLD ) const;
              
                
f32             CalcScreenSize  ( const vector3& Position,
                                        f32      WorldRadius,
                                        system   System = WORLD ) const;
/*              
f32             CalcScreenPercentage
                                ( const vector3& Position,
                                        f32      WorldSize,
                                        system   System = WORLD ) const;
*/


// Use to take a sub shot of the screen! (useful for multi-part screen shots)
void            SetSubShot      ( s32 SubShotX, s32 SubShotY, s32 ShotSize ) ;


//------------------------------------------------------------------------------
//  Fields
//------------------------------------------------------------------------------

protected:

        //
        // Required fields.
        //

        vector3     m_WorldPos;         // Position    of camera in World (V2W)
        matrix4     m_WorldOrient;      // Orientation of camera in World (V2W)
                                        
        s32         m_ViewportX0;       // Corners of 2D viewpoint on screen
        s32         m_ViewportY0;       
        s32         m_ViewportX1;       
        s32         m_ViewportY1;       
                                        
        radian      m_XFOV;             // Field of view in X
                                        
        f32         m_ZNear;            // Near plane in Z
        f32         m_ZFar;             // Far  plane in Z

        //
        // Sub shot fields for multi-part screen shots
        //
        s32         m_SubShotX ;        // Current X shot
        s32         m_SubShotY ;        // Current Y shot
        s32         m_ShotSize ;        // Total shots is m_ShotSize*m_ShotSize

        //
        // Derived fields with cached data.
        //
        
mutable u32         m_Dirty;            // Dirty flags for derived fields.
                                        
mutable matrix4     m_W2V;              // Transforms World -> View
mutable matrix4     m_V2W;              // Transforms View -> World
mutable matrix4     m_V2C;
mutable matrix4     m_C2S;
mutable matrix4     m_V2S;
mutable matrix4     m_W2C;
mutable matrix4     m_W2S;

mutable f32         m_ScreenDist;       // Dist where 3D units = screen units
mutable radian      m_YFOV;             // Field of view in Y

// The planes are ordered for faster culling: L0,R1,B2,T3,N4,F5
mutable plane       m_WorldSpacePlane[6];
mutable plane       m_ViewSpacePlane[6];
mutable s32         m_WorldPlaneMinIndex[6*3];
mutable s32         m_WorldPlaneMaxIndex[6*3];
mutable s32         m_ViewPlaneMinIndex[6*3];
mutable s32         m_ViewPlaneMaxIndex[6*3];
mutable plane       m_ZPlane;
mutable s32         m_ZPlaneMinI[3];
mutable s32         m_ZPlaneMaxI[3];
mutable vector3     m_ConeAxis;
mutable f32         m_ConeSlope;
        
mutable f32         m_ProjectX[2];      // ScreenX = ProjectX[0] + ProjectX[1]*(ViewX/ViewZ)
mutable f32         m_ProjectY[2];      // ScreenY = ProjectY[0] + ProjectY[1]*(ViewY/ViewZ)     

/*
mutable vector3     TLEdge;             // Normalized frustum edges in World space
mutable vector3     TREdge;
mutable vector3     BLEdge;
mutable vector3     BREdge;
*/

//------------------------------------------------------------------------------
//  Internal Functions
//------------------------------------------------------------------------------

protected:

    void    UpdateW2V       ( void ) const;
    void    UpdateV2W       ( void ) const;
                            
    void    UpdateV2C       ( void ) const;
    void    UpdateC2S       ( void ) const;
    void    UpdateV2S       ( void ) const;
    void    UpdateW2C       ( void ) const;
    void    UpdateW2S       ( void ) const;
                            
    void    UpdateYFOV      ( void ) const;
    void    UpdatePlanes    ( void ) const;
    void    UpdateEdges     ( void ) const;
    void    UpdateScreenDist( void ) const;
    void    UpdateProjection( void ) const;
};

#include "Common\e_View_inline.hpp"

//==============================================================================
#endif // E_VIEW_HPP
//==============================================================================
