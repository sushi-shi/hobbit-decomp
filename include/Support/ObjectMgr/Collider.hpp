// Imported genuine tribes-aa pristine4aab7137 engine reference: aa/support/objectmgr/Collider.hpp
// Provisional Hobbit API/layout adaptation; no fabricated PC labels. See IMPORT-NOTES.md.
//==============================================================================
//
//  Collider.hpp
//
//==============================================================================

#ifndef COLLIDER_HPP
#define COLLIDER_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/x_files/x_math.hpp>

//==============================================================================
//  TYPES
//==============================================================================

class collider {
    //------------------------------------------------------------------------------
    //  Public constants
public:
    // Extrusion constants
    static f32 MIN_EXT_DIST;      // Min dist of movement allowed
    static f32 MIN_EXT_DIST_SQR;  // Min squared dist of movement allowed
    static f32 MIN_EXT_AXIS_DIST; // Min dist of movement along an axis allowed

    // Ray constants
    static f32 MIN_RAY_DIST;     // Min dist of raycase allowed
    static f32 MIN_RAY_DIST_SQR; // Min squared dist of raycase allowed

    //------------------------------------------------------------------------------
    //  Public Types

public:
    enum type {
        NONE,
        RAY,
        EXTRUSION,
    };

    enum defines {
        MAX_CONTACTS = 8 // Max # of contacts to record
    };

    // Info about closest collision
    struct collision {
        xbool Collided;   // TRUE if collision occurred, else FALSE
        f32 T;            // Intersection 0->1 if a collision occurred
        vector3 Point;    // Point of collision
        plane Plane;      // Plane of collision
        void* pObjectHit; // Object that was hit
        s32 ID;           // ID (can be used to identify sub-objects in object etc)
        s32 NodeIndex;    // Index of node that was hit
        s32 NodeType;     // Type of node that was hit
    };

    // Info about any contacts that occured in movement bounds
    struct contact {
        f32 T;         // Intersection 0->1 of contact
        plane Plane;   // Plane of contact
        vector3 Point; // Point of contact
    };

    //------------------------------------------------------------------------------
    //  Public Functions

public:
    //------------------------------
    // Ray specific functions
    //------------------------------

    void RaySetup(
        const void* pMovingObject, // Object that is moving (or NULL)
        const vector3& WorldStart, // Start of ray
        const vector3& WorldEnd,   // End of ray
        s32 aExclude = -1,         // Exclude object id
        xbool LineOfSightTest = FALSE
    ); // Line of sight test (can early out if TRUE)

    const vector3& RayGetStart(void);
    const vector3& RayGetEnd(void);
    const vector3& RayGetDir(void);
    f32 RayGetLen(void);
    f32 RayGetLenSquared(void);

    //------------------------------
    // Extrusion specific functions
    //------------------------------

    void ExtSetup(
        const void* pMovingObject,     // Object that is moving (or NULL)
        const bbox& WorldBBox,         // Start world bbox
        vector3& WorldMovement,        // Movement to attempt
        s32 aExclude = -1,             // Exclude object id (or obj_mgr::NullId)
        xbool LineOfSightTest = FALSE, // Line of sight test (can early out if TRUE)
        xbool RecordContacts = FALSE
    ); // Records all contacts within movement bounds

    void ExtSetup(
        const void* pMovingObject,      // Object that is moving (or NULL)
        const bbox& WorldBBox,          // Start world bbox
        const bbox& PermeableWorldBBox, // Start world bbox for collision with permeables
        vector3& WorldMovement,         // Movement to attempt
        s32 aExclude = -1,              // Exclude object id (or obj_mgr::NullId)
        xbool LineOfSightTest = FALSE,  // Line of sight test (can early out if TRUE)
        xbool RecordContacts = FALSE
    ); // Records all contacts within movement bounds

    f32 GetHighPoint(void) const;

    s32 GetNPermeables(void);
    void GetPermeable(s32 Index, void*& pObject, f32& T);

    s32 GetNContacts(void) const;
    const contact& GetContact(s32 Index) const;

    //------------------------------
    // Generic functions
    //------------------------------

    collider(void);
    ~collider(void) {};

    void SetContext(void* pObject);
    void SetContext(void* pObject, const matrix4& L2W, const matrix4& W2L);

    void SetID(s32 ID);
    s32 GetID(void);

    void SetNodeIndex(s32 NodeIndex);
    s32 GetNodeIndex(void);

    void SetNodeType(s32 NodeType);
    s32 GetNodeType(void);

    void SetEarlyOut(xbool EarlyOut);
    xbool GetEarlyOut(void);

    void ClearExcludeList(void);
    xbool Exclude(s32 aExclude);
    s32 GetExcluded(s32 Index) const;
    s32 GetExcludedCount(void) const;

    void* GetMovingObject(void) const;

    xbool HasCollided(void) const;
    xbool GetCollision(collider::collision& Collision);
    f32 GetCollisionT(void) const;

    type GetType(void) const;
    const bbox& GetVolumeBBox(void) const;
    const bbox& GetMovementBBox(void) const;

    xbool TrivialRejectBBox(const bbox& BBox) const;

    void ApplyTri(const vector3& P0, const vector3& P1, const vector3& P2);

    void ApplyTri(const vector3& P0, const vector3& P1, const vector3& P2, const plane& Plane);

    void ApplyNGon(const vector3* pPoint, s32 NPoints);

    void ApplyNGon(const vector3* pPoint, s32 NPoints, const plane& Plane);

    void ApplyBBox(const bbox& BBox);

    void ApplyCollision(const collider::collision& Collision);

    void ApplyPermeableBBox(const bbox& BBox);

    //------------------------------------------------------------------------------
    //  Private Storage

protected:
    //
    // Generic information needed by all types of colliders.
    //

    type m_Type;           // Type of collider.
    void* m_pMovingObj;    // Pointer to object which is moving.
    collision m_Collision; // Collision information.
    s32 m_Context;         // Which context? 0=World, 1=Local
    s32 m_ID;              // Current ID of collision
    s32 m_NodeIndex;       // Current index of node being tested (or -1)
    s32 m_NodeType;        // Current type of node being tested (or -1)
    void* m_pContextObj;   // Object of current context.
    matrix4 m_ContextL2W;  // If context = 1, L2W.
    matrix4 m_ContextW2L;  // If context = 1, W2L.

    s32 m_NExcluded;
    s32 m_ExcludeList[32]; // To be used by "driver" of collision.

    xbool m_LineOfSightTest; // If TRUE, collider can early out
    xbool m_EarlyOut;        // If TRUE, no other collision are processed (eg. LOS complete)

    matrix4 m_ImpactL2W;   // L2W for impact if local context.
    xbool m_ImpactIsLocal; // True if collision data in local context.

    xbool m_RecordContacts;           // TRUE if contacts should be recorded
    contact m_Contacts[MAX_CONTACTS]; // List of contacts that happened
    s32 m_NContacts;                  // Number of contacts that happened

    //
    // Information needed by ray colliders.
    //

    struct ray_info {
        vector3 Start;
        vector3 End;
        vector3 Dir;
        bbox BBox;
    };

    ray_info m_RayInfo[2];
    f32 m_RayLength;
    f32 m_RayLengthSquared;

    void RayApplyBBox(const bbox& BBox);

    void RayApplyNGon(const vector3* pPoint, s32 NPoints, const plane& Plane);

    //
    // Information needed by extrusion colliders.
    //

    struct ext_volume {
        f32 MinD[3];
        f32 MaxD[3];
        vector3 Normal[3];
        vector3 Point[8];
    };

    struct ext_info {
        ext_volume Volume[3];
        vector3 Movement;
        bbox VolumeBBox;
        bbox MovementBBox;
        bbox StartBBox;
        bbox EndBBox;
        vector3 Up;
        vector3 StartCenter;
        vector3 EndCenter;
        f32 RadiusSquared;
    };

    struct ext_pinfo {
        void* pObject;
        f32 T;
    };

    ext_info m_ExtInfo[2];
    s32 m_ExtNVolumes;
    f32 m_ExtHigh;
    f32 m_ExtHighL2WScale;
    f32 m_ExtHighL2WTrans;
    f32 m_BestImpactDot;

    s32 m_NPermeables; // -1 means "don't collect permeables"
    ext_pinfo m_Permeable[32];
    bbox m_PermeableBBox;

    void ExtAddPolyImpact(
        f32 T,
        const vector3& Normal,
        const plane& Plane,
        const vector3& CollisionPt,
        const vector3* pPoints,
        s32 NPoints
    );

    void ExtAddVolume(s32 Index, const vector3* pCorner);

    void ExtComputePill(ext_info& Info);
    void ExtComputeVolumePlanes(ext_volume& Volume, const vector3& SafeMove);

    void ExtApplyBBox(const bbox& BBox);
    void ExtApplyNGon(const vector3* pPoint, s32 NPoints, const plane& Plane);

    void AddContact(f32 T, const plane& Plane, const vector3& CollisionPt);
};

//==============================================================================
// These stats are available for public eyes
//==============================================================================

extern s32 COLL_STAT_NRaySetups;
extern s32 COLL_STAT_NExtSetups;
extern s32 COLL_STAT_NExtApplyNGons;
extern s32 COLL_STAT_NExtApplyNGonsSolve;
extern s32 COLL_STAT_NRayApplyNGons;
extern s32 COLL_STAT_NRayApplyNGonsSolve;

//==============================================================================
#endif // COLLIDER_HPP
//==============================================================================
