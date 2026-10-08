// Imported genuine tribes-aa pristine4aab7137 engine reference: aa/support/objectmgr/ColliderCannon.cpp
// Provisional Hobbit API/layout adaptation; no fabricated PC labels. See IMPORT-NOTES.md.
//==============================================================================
//
//  ColliderCannon.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ColliderCannon.hpp>
#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <xCore/Entropy/Entropy.hpp>
#include <Support/Objects/Terrain/Terrain.hpp>
#include <Support/Building/BuildingOBJ.hpp>

//==============================================================================
//  STORAGE
//==============================================================================

static vector3 s_Start, s_Dir;

static vector3 s_RayStart, s_RayEnd;
static bbox s_ExtStart, s_ExtEnd;

static vector3 s_RayNormal, s_ExtNormal;
static vector3 s_RayImpact, s_ExtImpact;
static xbool s_RayHit, s_ExtHit;
static f32 s_ExtHigh;

static object::id s_pRayObject;
static object::id s_pExtObject;

//==============================================================================
//  FUNCTIONS
//==============================================================================

void cc_Aim(const vector3& Start, const vector3& Direction) {
    s_Start = Start;
    s_Dir = Direction;
}

//==============================================================================

void cc_Fire(void) {
    collider Collider;
    collider::collision Collision;
    vector3 Movement;

    // Ray.

    s_RayStart = s_Start;
    s_RayEnd = s_RayStart + s_Dir * 100.0f;
    Collider.RaySetup(NULL, s_RayStart, s_RayEnd);
    ObjMgr.Collide(object::ATTR_ALL, Collider);
    s_RayHit = Collider.GetCollision(Collision);
    if (s_RayHit) {
        s_RayImpact = Collision.Point;
        s_RayNormal = s_RayImpact + Collision.Plane.Normal * 10.0f;
        s_pRayObject = ((object*)Collision.pObjectHit)->GetObjectID();
    }

    // Extrusion.

    s_ExtStart.Set(s_Start, 1.0f);
    Movement = s_Dir * 100.0f;
    Collider.ExtSetup(NULL, s_ExtStart, Movement);
    ObjMgr.Collide(object::ATTR_ALL, Collider);
    s_ExtHit = Collider.GetCollision(Collision);
    if (s_ExtHit) {
        s_ExtEnd = s_ExtStart;
        s_ExtEnd.Translate(Movement * Collision.T);
        s_ExtImpact = Collision.Point;
        s_ExtNormal = s_ExtImpact + Collision.Plane.Normal * 10.0f;
        s_ExtHigh = Collider.GetHighPoint();
        s_pExtObject = ((object*)Collision.pObjectHit)->GetObjectID();
    }
}

//==============================================================================

void cc_Render(void) {
    draw_Line(s_RayStart, s_RayEnd, XCOLOR_WHITE);
    draw_Point(s_RayStart, XCOLOR_BLUE);
    draw_Point(s_RayEnd, XCOLOR_BLUE);

    if (s_RayHit) {
        object* pObj = ObjMgr.GetObjectFromID(s_pRayObject);
        draw_Point(s_RayImpact, XCOLOR_RED);
        draw_Line(s_RayImpact, s_RayNormal, XCOLOR_RED);
        if (pObj) {
            x_printfxy(
                0,
                0,
                "Ray -- Type:%s  Slot:%d  Seq:%d",
                pObj->GetTypeName(),
                s_pRayObject.Slot,
                s_pRayObject.Seq
            );
            pObj->DebugRender();
        }
    }

    if (s_ExtHit) {
        vector3 Extra = s_ExtImpact;
        object* pObj = ObjMgr.GetObjectFromID(s_pExtObject);
        Extra.Y = s_ExtHigh;
        draw_Point(s_ExtImpact, XCOLOR_GREEN);
        draw_BBox(s_ExtEnd, XCOLOR_GREEN);
        draw_Line(s_ExtImpact, s_ExtNormal, XCOLOR_GREEN);
        draw_Line(Extra, s_ExtImpact, XCOLOR_BLUE);
        draw_Line(Extra, s_ExtEnd.GetCenter(), XCOLOR_BLUE);
        draw_Line(s_ExtImpact, Extra, XCOLOR_YELLOW);
        if (pObj) {
            x_printfxy(
                0,
                1,
                "Ext -- Type:%s  Slot:%d  Seq:%d",
                pObj->GetTypeName(),
                s_pExtObject.Slot,
                s_pExtObject.Seq
            );
            pObj->DebugRender();
        }
    }
}

//==============================================================================

volatile f32 AMBIENT_HUE = 1.0f;
volatile f32 AMBIENT_INT = 0.65f;

xcolor GetAmbientLighting(
    const vector3& Point,
    const vector3& RayDir,
    f32 AmbientHue,
    f32 AmbientIntensity
) {
    f32 BestY = -1000000.0f;
    xcolor Color = XCOLOR_WHITE;
    xcolor C;
    vector3 P;
    vector3 N = RayDir;
    N.Normalize();
    vector3 Pos = Point - N;
    object* pObj = NULL;

    // Get lighting from terrain and buildings.
    ObjMgr.Select(object::ATTR_SOLID_STATIC, Pos, 10.0f);
    while ((pObj = ObjMgr.GetNextSelected())) {
        if (pObj->GetType() == object::TYPE_TERRAIN) {
            terrain* pTerrain = (terrain*)pObj;
            pTerrain->GetLighting(Pos, C, P);
            if (P.Y > BestY) {
                BestY = P.Y;
                Color = C;
            }
        }

        if (pObj->GetType() == object::TYPE_BUILDING) {
            building_obj* pBuilding = (building_obj*)pObj;
            pBuilding->GetLighting(Pos, RayDir, C, P);
            if (P.Y > BestY) {
                BestY = P.Y;
                Color = C;
            }
        }
    }
    ObjMgr.ClearSelection();

    // Bust color into intensity and hue
    f32 FR = (f32)Color.R * (1.0f / 255.0f);
    f32 FG = (f32)Color.G * (1.0f / 255.0f);
    f32 FB = (f32)Color.B * (1.0f / 255.0f);

    f32 FI = (FR + FG + FB) / 3.0f;
    f32 FM = MAX(MAX(FR, FG), FB);
    if (FM < 0.0001f) {
        FM = 0.0001f;
    }
    FR *= 1.0f / FM;
    FG *= 1.0f / FM;
    FB *= 1.0f / FM;

    // Interpolate intensity towards white
    FI = FI + (1.0f - AmbientIntensity) * (1.0f - FI);
    if (FI > 1.0f) {
        FI = 1.0f;
    }
    if (FI < 0.0f) {
        FI = 0.0f;
    }

    // Interpolate hue towards white
    FR = FR + (1.0f - AmbientHue) * (1.0f - FR);
    FG = FG + (1.0f - AmbientHue) * (1.0f - FG);
    FB = FB + (1.0f - AmbientHue) * (1.0f - FB);
    if (FR > 1.0f) {
        FR = 1.0f;
    }
    if (FR < 0.0f) {
        FR = 0.0f;
    }
    if (FG > 1.0f) {
        FG = 1.0f;
    }
    if (FG < 0.0f) {
        FG = 0.0f;
    }
    if (FB > 1.0f) {
        FB = 1.0f;
    }
    if (FB < 0.0f) {
        FB = 0.0f;
    }

    // Recombine intensity and hue
    FR *= FI;
    FG *= FI;
    FB *= FI;

    Color.R = (u8)(FR * 255.0f);
    Color.G = (u8)(FG * 255.0f);
    Color.B = (u8)(FB * 255.0f);

    return (Color);
}

//==============================================================================
