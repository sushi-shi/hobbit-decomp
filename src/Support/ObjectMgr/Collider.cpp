// Imported genuine tribes-aa pristine4aab7137 engine reference: aa/support/objectmgr/Collider.cpp
// Provisional Hobbit API/layout adaptation; no fabricated PC labels. See IMPORT-NOTES.md.
//==============================================================================
//
//  Collider.cpp
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_stdio.hpp>
#include <Support/ObjectMgr/Collider.hpp>

#include <xCore/Entropy/Entropy.hpp>

//==============================================================================
//  DEBUG DEFINES
//==============================================================================

//#define DEBUG_COLLISION   // Sets up ngon points

#ifdef DEBUG_COLLISION

vector3 CollPoints[64];
s32 NCollPoints = 0;

#endif //#ifdef DEBUG_COLLISION

//==============================================================================
//  STATIC CONSTANTS STORAGE
//==============================================================================

f32 collider::MIN_EXT_DIST = (0.0001f);               // Min dist of movement allowed
f32 collider::MIN_EXT_DIST_SQR = (0.0001f * 0.0001f); // Min squared dist of movement allowed
f32 collider::MIN_EXT_AXIS_DIST = (0.0001f);          // Min dist of movement along an axis allowed

f32 collider::MIN_RAY_DIST = (0.001f);              // Min dist of raycast allowed
f32 collider::MIN_RAY_DIST_SQR = (0.001f * 0.001f); // Min squared dist of raycast allowed

//==============================================================================
//  LOCAL STORAGE
//==============================================================================

s32 COLL_STAT_NRaySetups = 0;
s32 COLL_STAT_NExtSetups = 0;
s32 COLL_STAT_NExtApplyNGons = 0;
s32 COLL_STAT_NExtApplyNGonsSolve = 0;
s32 COLL_STAT_NRayApplyNGons = 0;
s32 COLL_STAT_NRayApplyNGonsSolve = 0;

static s16 FaceIndex[] = {5, 4, 6, 7, 1, 3, 2, 0, 2, 3, 7, 6, 5, 1, 0, 4, 1, 5, 7, 3, 2, 6, 4, 0};

//==============================================================================
//  FUNCTIONS
//==============================================================================

static void BuildBBoxPoints(const bbox& BBox, vector3* P) {
    P[0].X = BBox.Min.X;
    P[0].Y = BBox.Min.Y;
    P[0].Z = BBox.Min.Z;
    P[1].X = BBox.Min.X;
    P[1].Y = BBox.Min.Y;
    P[1].Z = BBox.Max.Z;
    P[2].X = BBox.Min.X;
    P[2].Y = BBox.Max.Y;
    P[2].Z = BBox.Min.Z;
    P[3].X = BBox.Min.X;
    P[3].Y = BBox.Max.Y;
    P[3].Z = BBox.Max.Z;
    P[4].X = BBox.Max.X;
    P[4].Y = BBox.Min.Y;
    P[4].Z = BBox.Min.Z;
    P[5].X = BBox.Max.X;
    P[5].Y = BBox.Min.Y;
    P[5].Z = BBox.Max.Z;
    P[6].X = BBox.Max.X;
    P[6].Y = BBox.Max.Y;
    P[6].Z = BBox.Min.Z;
    P[7].X = BBox.Max.X;
    P[7].Y = BBox.Max.Y;
    P[7].Z = BBox.Max.Z;
}

//==============================================================================

collider::collider(void) {
    m_Type = NONE;
    m_NExcluded = 0;
}

//==============================================================================

void collider::RaySetup(
    const void* pMovingObject,
    const vector3& WorldStart,
    const vector3& WorldEnd,
    s32 aExclude,
    xbool LineOfSightTest
) {
    COLL_STAT_NRaySetups++;

    ASSERT(WorldStart.InRange(-20000.0f, 20000.0f));
    ASSERT(WorldEnd.InRange(-20000.0f, 20000.0f));

    m_Type = RAY;
    m_Context = 0;
    m_pContextObj = NULL;
    m_ID = 0;
    m_NodeIndex = -1;
    m_NodeType = -1;
    m_ImpactIsLocal = FALSE;
    m_pMovingObj = (void*)pMovingObject;

    if (aExclude != -1) {
        ClearExcludeList();
        Exclude(aExclude);
    }

    m_RecordContacts = FALSE;
    m_NContacts = 0;

    m_LineOfSightTest = LineOfSightTest;
    m_EarlyOut = FALSE;

    m_Collision.Collided = FALSE;
    m_Collision.pObjectHit = NULL;
    m_Collision.T = 2.0f;

    m_RayInfo[0].Start = WorldStart;
    m_RayInfo[0].End = WorldEnd;
    m_RayInfo[0].Dir = WorldEnd - WorldStart;
    m_RayLengthSquared = m_RayInfo[0].Dir.LengthSquared();
    m_RayLength = x_sqrt(m_RayLengthSquared);
    m_RayInfo[0].Dir /= m_RayLength;
    m_RayInfo[0].BBox(WorldStart, WorldEnd);

    // Make sure we don't have a degenerately short ray.
    ASSERT(m_RayLengthSquared >= MIN_RAY_DIST_SQR);

    // Do not attempt to collect permeables.
    m_NPermeables = -1;
}

//==============================================================================

const vector3& collider::RayGetStart(void) {
    return (m_RayInfo[m_Context].Start);
}

//==============================================================================

const vector3& collider::RayGetEnd(void) {
    return (m_RayInfo[m_Context].End);
}

//==============================================================================

const vector3& collider::RayGetDir(void) {
    return (m_RayInfo[m_Context].Dir);
}

//==============================================================================

f32 collider::RayGetLen(void) {
    return (m_RayLength);
}

//==============================================================================

f32 collider::RayGetLenSquared(void) {
    return (m_RayLengthSquared);
}

//==============================================================================

void collider::ExtSetup(
    const void* pMovingObject,
    const bbox& WorldBBox,
    vector3& WorldMovement,
    s32 aExclude,
    xbool LineOfSightTest,
    xbool RecordContacts
) {
    vector3 Corner[8];

    COLL_STAT_NExtSetups++;

    // Is movement valid?
    ASSERT(WorldMovement.LengthSquared() >= MIN_EXT_DIST_SQR);

    // Make axis movement valid so that plane normals will be valid
    if (x_abs(WorldMovement.X) < MIN_EXT_AXIS_DIST) {
        WorldMovement.X = 0;
    }
    if (x_abs(WorldMovement.Y) < MIN_EXT_AXIS_DIST) {
        WorldMovement.Y = 0;
    }
    if (x_abs(WorldMovement.Z) < MIN_EXT_AXIS_DIST) {
        WorldMovement.Z = 0;
    }

    // Prepare for extrusion
    m_Type = EXTRUSION;
    m_Context = 0;
    m_pContextObj = NULL;
    m_ID = 0;
    m_NodeIndex = -1;
    m_NodeType = -1;
    m_ImpactIsLocal = FALSE;
    m_pMovingObj = (void*)pMovingObject;
    m_NPermeables = -1;

    if (aExclude != -1) {
        ClearExcludeList();
        Exclude(aExclude);
    }

    m_RecordContacts = RecordContacts;
    m_NContacts = 0;

    m_LineOfSightTest = LineOfSightTest;
    m_EarlyOut = FALSE;

    m_Collision.Collided = FALSE;
    m_Collision.pObjectHit = NULL;
    m_Collision.T = 2.0f;

    m_BestImpactDot = 1.0f;
    m_ExtInfo[0].Movement = WorldMovement;
    m_ExtInfo[0].Up(0, 1, 0);
    m_ExtHigh = (WorldMovement.Y < 0.0f) ? WorldBBox.Min.Y + WorldMovement.Y : WorldBBox.Min.Y;

    // Build the corners of the starting bounding box.
    BuildBBoxPoints(WorldBBox, Corner);

    // Depending on the direction of movement, extrude certain faces into
    // volumes.
    m_ExtNVolumes = 0;
    if (WorldMovement.X > 0.0f) {
        ExtAddVolume(0, Corner);
    }
    if (WorldMovement.X < 0.0f) {
        ExtAddVolume(1, Corner);
    }
    if (WorldMovement.Y > 0.0f) {
        ExtAddVolume(2, Corner);
    }
    if (WorldMovement.Y < 0.0f) {
        ExtAddVolume(3, Corner);
    }
    if (WorldMovement.Z > 0.0f) {
        ExtAddVolume(4, Corner);
    }
    if (WorldMovement.Z < 0.0f) {
        ExtAddVolume(5, Corner);
    }
    ASSERT(m_ExtNVolumes <= 3);

    // Build pill around extrusion.
    ExtComputePill(m_ExtInfo[0]);

    // Build a single bbox to contain all of the extrusion volumes.  This bbox
    // does NOT contain the original world bbox of the object before it moves.
    m_ExtInfo[0].VolumeBBox.Clear();
    for (s32 i = 0; i < m_ExtNVolumes; i++) {
        m_ExtInfo[0].VolumeBBox.AddVerts(m_ExtInfo[0].Volume[i].Point, 8);
    }

    // Save the starting bounding box position.
    m_ExtInfo[0].StartBBox = WorldBBox;

    // Save the ending bounding box position.
    m_ExtInfo[0].EndBBox = WorldBBox;
    m_ExtInfo[0].EndBBox.Min += WorldMovement;
    m_ExtInfo[0].EndBBox.Max += WorldMovement;

    // Build movement bbox.
    m_ExtInfo[0].MovementBBox.Clear();
    m_ExtInfo[0].MovementBBox += m_ExtInfo[0].StartBBox;
    m_ExtInfo[0].MovementBBox += m_ExtInfo[0].EndBBox;
}

//==============================================================================

void collider::ExtSetup(
    const void* pMovingObject,
    const bbox& WorldBBox,
    const bbox& PermeableWorldBBox,
    vector3& WorldMovement,
    s32 aExclude,
    xbool LineOfSightTest,
    xbool RecordContacts
) {
    ExtSetup(pMovingObject, WorldBBox, WorldMovement, aExclude, LineOfSightTest, RecordContacts);
    m_NPermeables = 0;
    m_PermeableBBox = PermeableWorldBBox;
}

//==============================================================================

s32 collider::GetNPermeables(void) {
    if (m_NPermeables == -1) {
        return (0);
    }
    return (m_NPermeables);
}

//==============================================================================

void collider::GetPermeable(s32 Index, void*& pObject, f32& T) {
    ASSERT(Index >= 0);
    ASSERT(Index < m_NPermeables);
    pObject = m_Permeable[Index].pObject;
    T = m_Permeable[Index].T;
}

//==============================================================================

void collider::AddContact(f32 T, const plane& Plane, const vector3& CollisionPt) {
    s32 i;

    // Only keep track of a set number of contacts
    if (m_NContacts == MAX_CONTACTS) {
        return;
    }

    // In local space?
    if (m_Context == 1) {
        // Convert plane to world space ready for comparing
        plane WorldPlane = Plane;
        WorldPlane.Transform(m_ContextL2W);

        // Check to see if a contact with the same plane already exists
        for (i = 0; i < m_NContacts; i++) {
            // Get contact to check
            contact& Contact = m_Contacts[i];

            // Hit the same plane?
            if ((WorldPlane.Normal == Contact.Plane.Normal) && (WorldPlane.D == Contact.Plane.D)) {
                // Keep if the closest contact on this plane
                if (T < Contact.T) {
                    Contact.T = T;
                    Contact.Point = m_ContextL2W * CollisionPt;
                }

                // We are now done
                return;
            }
        }

        // Same plane contact does not exist, so add it to the list
        contact& Contact = m_Contacts[m_NContacts++];
        Contact.T = T;
        Contact.Plane = WorldPlane;
        Contact.Point = m_ContextL2W * CollisionPt;
    } else {
        // World space

        // Check to see if a contact with the same plane already exists
        for (i = 0; i < m_NContacts; i++) {
            // Get contact to check
            contact& Contact = m_Contacts[i];

            // Hit the same plane?
            if ((Plane.Normal == Contact.Plane.Normal) && (Plane.D == Contact.Plane.D)) {
                // Keep if the closest contact on this plane
                if (T < Contact.T) {
                    Contact.T = T;
                    Contact.Point = CollisionPt;
                }

                // We are now done
                return;
            }
        }

        // Same plane contact does not exist, so add it to the list
        contact& Contact = m_Contacts[m_NContacts++];
        Contact.T = T;
        Contact.Plane = Plane;
        Contact.Point = CollisionPt;
    }

    // Make sure array wasn't overrun
    ASSERT(m_NContacts <= MAX_CONTACTS);
}

//==============================================================================

s32 collider::GetNContacts(void) const {
    return m_NContacts;
}

//==============================================================================

const collider::contact& collider::GetContact(s32 Index) const {
    ASSERT(Index >= 0);
    ASSERT(Index < m_NContacts);
    ASSERT(Index < MAX_CONTACTS);
    return m_Contacts[Index];
}

//==============================================================================

f32 collider::GetHighPoint(void) const {
    return (m_ExtHigh);
}

//==============================================================================

void collider::SetContext(void* pObject) {
    m_Context = 0;
    m_pContextObj = pObject;
}

//==============================================================================

void collider::SetContext(void* pObject, const matrix4& L2W, const matrix4& W2L) {
    m_Context = 1;
    m_pContextObj = pObject;
    m_ContextL2W = L2W;
    m_ContextW2L = W2L;

    //
    // This process is different depending on the type of collider...
    //

    if (m_Type == RAY) {
        m_RayInfo[1].Start = W2L * m_RayInfo[0].Start;
        m_RayInfo[1].End = W2L * m_RayInfo[0].End;
        m_RayInfo[1].Dir = m_RayInfo[1].End - m_RayInfo[1].Start;
        m_RayLengthSquared = m_RayInfo[1].Dir.LengthSquared();
        m_RayLength = x_sqrt(m_RayLengthSquared);
        m_RayInfo[1].Dir /= m_RayLength;
        m_RayInfo[1].BBox(m_RayInfo[1].Start, m_RayInfo[1].End);
    }

    else if (m_Type == EXTRUSION) {
        m_ExtInfo[1].VolumeBBox.Clear();

        m_ExtInfo[1].StartBBox = m_ExtInfo[0].StartBBox;
        m_ExtInfo[1].StartBBox.Transform(W2L);

        m_ExtInfo[1].EndBBox = m_ExtInfo[0].EndBBox;
        m_ExtInfo[1].EndBBox.Transform(W2L);

        // Build movement bbox
        m_ExtInfo[1].MovementBBox.Clear();
        m_ExtInfo[1].MovementBBox += m_ExtInfo[1].StartBBox;
        m_ExtInfo[1].MovementBBox += m_ExtInfo[1].EndBBox;

        m_ExtInfo[1].Movement = m_ContextW2L.RotateVector(m_ExtInfo[0].Movement);
        m_ExtInfo[1].Up = m_ContextW2L.RotateVector(m_ExtInfo[0].Up);

        for (s32 i = 0; i < m_ExtNVolumes; i++) {
            m_ContextW2L.Transform(m_ExtInfo[1].Volume[i].Point, m_ExtInfo[0].Volume[i].Point, 8);
            ExtComputeVolumePlanes(m_ExtInfo[1].Volume[i], m_ExtInfo[1].Movement);
            m_ExtInfo[1].VolumeBBox.AddVerts(m_ExtInfo[1].Volume[i].Point, 8);
        }

        ExtComputePill(m_ExtInfo[1]);
    }

    else {
        ASSERT(FALSE);
    }
}

//==============================================================================

void collider::SetID(s32 ID) {
    m_ID = ID;
}

//==============================================================================

s32 collider::GetID(void) {
    return m_ID;
}

//==============================================================================

void collider::SetNodeIndex(s32 NodeIndex) {
    m_NodeIndex = NodeIndex;
}

//==============================================================================

s32 collider::GetNodeIndex(void) {
    return m_NodeIndex;
}

//==============================================================================

void collider::SetNodeType(s32 NodeType) {
    m_NodeType = NodeType;
}

//==============================================================================

s32 collider::GetNodeType(void) {
    return m_NodeType;
}

//==============================================================================

void collider::SetEarlyOut(xbool EarlyOut) {
    m_EarlyOut = EarlyOut;
}

//==============================================================================

xbool collider::GetEarlyOut(void) {
    return m_EarlyOut;
}

//==============================================================================

void collider::ClearExcludeList(void) {
    m_NExcluded = 0;
}

//==============================================================================

s32 collider::GetExcludedCount(void) const {
    return (m_NExcluded);
}

//==============================================================================

s32 collider::GetExcluded(s32 Index) const {
    ASSERT(Index < m_NExcluded);
    ASSERT(Index >= 0);
    return (m_ExcludeList[Index]);
}

//==============================================================================

xbool collider::Exclude(s32 aExclude) {
    if (aExclude != -1) {
        if (m_NExcluded == 32) {
            return (FALSE);
        }

        m_ExcludeList[m_NExcluded] = aExclude;
        m_NExcluded++;
    }

    return (TRUE);
}

//==============================================================================

void* collider::GetMovingObject(void) const {
    return (m_pMovingObj);
}

//==============================================================================

xbool collider::HasCollided(void) const {
    return (m_Collision.Collided);
}

//==============================================================================

xbool collider::GetCollision(collider::collision& Collision) {
    // If the impact took place in a local context, we must convert its data
    // into world space.
    if (m_ImpactIsLocal && m_Collision.Collided) {
        // Convert the plane.
        m_Collision.Plane.Transform(m_ImpactL2W);

        // Transform the collision point back to world space.
        m_Collision.Point = m_ImpactL2W * m_Collision.Point;

        // The impact is no longer local!
        m_ImpactIsLocal = FALSE;
    }

    Collision = m_Collision;
    return (m_Collision.Collided);
}

//==============================================================================

f32 collider::GetCollisionT(void) const {
    return (m_Collision.T);
}

//==============================================================================

collider::type collider::GetType(void) const {
    return (m_Type);
}

//==============================================================================

const bbox& collider::GetVolumeBBox(void) const {
    if (m_Type == RAY) {
        return (m_RayInfo[m_Context].BBox);
    }

    else if (m_Type == EXTRUSION) {
        return (m_ExtInfo[m_Context].VolumeBBox);
    }

    else {
        ASSERT(FALSE); // Collider not set up yet!
        return (m_ExtInfo[m_Context].VolumeBBox);
    }
}

//==============================================================================

const bbox& collider::GetMovementBBox(void) const {
    if (m_Type == RAY) {
        return (m_RayInfo[m_Context].BBox);
    }

    else if (m_Type == EXTRUSION) {
        return (m_ExtInfo[m_Context].MovementBBox);
    }

    else {
        ASSERT(FALSE); // Collider not set up yet!
        return (m_ExtInfo[m_Context].MovementBBox);
    }
}

//==============================================================================

xbool collider::TrivialRejectBBox(const bbox& BBox) const {
    if (m_Type == RAY) {
        // First compare overlapping boxes.
        if (!m_RayInfo[m_Context].BBox.Intersect(BBox)) {
            return (TRUE);
        }

        // Do ray to sphere test.
        vector3 Center = BBox.GetCenter();
        f32 R2 = BBox.GetRadiusSquared();
        vector3 StoC = Center - m_RayInfo[m_Context].Start;
        f32 Dot = m_RayInfo[m_Context].Dir.Dot(StoC);
        f32 Dist2 = StoC.LengthSquared() - (Dot * Dot);
        if (Dist2 > R2) {
            return (TRUE);
        }

        // Can't think of any more quick tests, so rejection is over.
        return (FALSE);
    } else if (m_Type == EXTRUSION) {
        const ext_info& Info = m_ExtInfo[m_Context];
        //f32 HoldMaxT[3];
        //f32 MinT, MaxT, LargestMinT;

        // First compare overlapping boxes.
        if (!Info.VolumeBBox.Intersect(BBox)) {
            return (TRUE);
        }

        // Can't think of any more quick tests, so rejection is over.
        return (FALSE);
    }

    ASSERT(FALSE); // Collider not set up yet!
    return (FALSE);
}

//==============================================================================

void collider::ApplyTri(const vector3& P0, const vector3& P1, const vector3& P2) {
    // Early out?
    if (m_EarlyOut) {
        return;
    }

    vector3 P[3] = {P0, P1, P2};
    ApplyNGon(P, 3);
}

//==============================================================================

void collider::ApplyTri(
    const vector3& P0,
    const vector3& P1,
    const vector3& P2,
    const plane& Plane
) {
    // Early out?
    if (m_EarlyOut) {
        return;
    }

    vector3 P[3] = {P0, P1, P2};
    ApplyNGon(P, 3, Plane);
}

//==============================================================================

void collider::ApplyNGon(const vector3* pPoint, s32 NPoints) {
    // Early out?
    if (m_EarlyOut) {
        return;
    }

    // Get plane of ngon.
    plane Plane;
    Plane.Setup(pPoint[0], pPoint[1], pPoint[2]);

    // fragment too small
    if (Plane.Normal.LengthSquared() < 0.99f) {
        return;
    }

    // Call other function.
    ApplyNGon(pPoint, NPoints, Plane);
}

//==============================================================================

void collider::ApplyNGon(const vector3* pPoint, s32 NPoints, const plane& Plane) {
    // Early out?
    if (m_EarlyOut) {
        return;
    }

    if (m_Type == RAY) {
        RayApplyNGon(pPoint, NPoints, Plane);
    } else if (m_Type == EXTRUSION) {
        ExtApplyNGon(pPoint, NPoints, Plane);

        // Eliminate any permeables beyond current T.
        while ((m_NPermeables > 0) && (m_Permeable[m_NPermeables - 1].T > m_Collision.T)) {
            m_NPermeables--;
        }
    } else {
        ASSERT(FALSE); // Collider not set up yet!
    }
}

//==============================================================================

void collider::ApplyBBox(const bbox& BBox) {
    // Early out?
    if (m_EarlyOut) {
        return;
    }

    if (m_pContextObj == m_pMovingObj) {
        return;
    }

    if (m_Type == RAY) {
        RayApplyBBox(BBox);
    } else if (m_Type == EXTRUSION) {
        ExtApplyBBox(BBox);

        // Eliminate any permeables beyond current T.
        while ((m_NPermeables > 0) && (m_Permeable[m_NPermeables - 1].T > m_Collision.T)) {
            m_NPermeables--;
        }
    } else {
        ASSERT(FALSE); // Collider not set up yet!
    }
}

//==============================================================================

void collider::ApplyCollision(const collider::collision& Collision) {
    ASSERT(m_Type == RAY); // This only works for ray colliders.

    // Early out?
    if (m_EarlyOut) {
        return;
    }

    if ((Collision.Collided) && (Collision.T < m_Collision.T) && (Collision.T >= 0)
        && (Collision.T <= 1)) {
        m_Collision = Collision;
        m_Collision.pObjectHit = m_pContextObj;

        m_ImpactIsLocal = (m_Context == 1);

        if (m_Context == 1) {
            m_ImpactL2W = m_ContextL2W;
        }

        // Eliminate any permeables beyond current T.
        while ((m_NPermeables > 0) && (m_Permeable[m_NPermeables - 1].T > m_Collision.T)) {
            m_NPermeables--;
        }
    }
}

//==============================================================================

void collider::RayApplyBBox(const bbox& BBox) {
    ray_info& Info = m_RayInfo[m_Context];
    vector3 P0 = Info.Start;
    vector3 P1 = Info.End;
    vector3 Direction = P1 - P0;
    xbool PlaneUsed[3] = {TRUE, TRUE, TRUE};
    f32 T[3] = {-1.0f, -1.0f, -1.0f};
    f32 PlaneD[3];
    s32 MaxPlane;
    s32 i;

    // Early out?
    if (m_EarlyOut) {
        return;
    }

    // Attempt to reject based on bounding boxes.
    if (Info.BBox.Min.X >= BBox.Max.X) {
        return;
    }
    if (Info.BBox.Min.Y >= BBox.Max.Y) {
        return;
    }
    if (Info.BBox.Min.Z >= BBox.Max.Z) {
        return;
    }
    if (Info.BBox.Max.X <= BBox.Min.X) {
        return;
    }
    if (Info.BBox.Max.Y <= BBox.Min.Y) {
        return;
    }
    if (Info.BBox.Max.Z <= BBox.Min.Z) {
        return;
    }

    // Consider relationship of each component of P0 to the box.
    for (i = 0; i < 3; i++) {
        if (P0[i] >= BBox.Max[i]) {
            PlaneD[i] = BBox.Max[i];
        } else if (P0[i] <= BBox.Min[i]) {
            PlaneD[i] = BBox.Min[i];
        } else {
            PlaneUsed[i] = FALSE;
        }
    }

    // Is the starting point in the box?
    if (!PlaneUsed[0] && !PlaneUsed[1] && !PlaneUsed[2]) {
        m_ImpactIsLocal = (m_Context == 1);
        m_Collision.Collided = TRUE;
        m_Collision.pObjectHit = m_pContextObj;
        m_Collision.ID = m_ID;
        m_Collision.NodeIndex = m_NodeIndex;
        m_Collision.NodeType = m_NodeType;
        m_Collision.T = 0.0f;
        m_Collision.Point = P0;
        m_Collision.Plane.Setup(P0, -Direction);

        if (m_Context == 1) {
            m_ImpactL2W = m_ContextL2W;
        }

        // Can end now if this is just a line of sight test
        m_EarlyOut |= m_LineOfSightTest;

        return;
    }

    // For each plane to be used, compute the distance to the plane.
    for (i = 0; i < 3; i++) {
        if (PlaneUsed[i] && (Direction[i] != 0.0f)) {
            T[i] = (PlaneD[i] - P0[i]) / Direction[i];
        }
    }

    // We need to know which plane had the largest distance.
    if (T[0] > T[1]) {
        MaxPlane = ((T[0] > T[2]) ? 0 : 2);
    } else {
        MaxPlane = ((T[1] > T[2]) ? 1 : 2);
    }

    // If the largest plane distance is less than zero, then there is no hit.
    if (T[MaxPlane] <= 0.0f) {
        return;
    }

    // See if the point we think is the hit point is a real hit.
    for (i = 0; i < 3; i++) {
        // See if component 'i' of the hit point is on the box.
        if (i != MaxPlane) {
            f32 Component = P0[i] + T[MaxPlane] * Direction[i];
            if ((Component < BBox.Min[i]) || (Component > BBox.Max[i])) {
                // We missed!  Hit point was not on the box.
                return;
            }
        }
    }

    // We have a verified hit.
    f32 t = T[MaxPlane];
    if ((t < m_Collision.T) && (t >= 0.0f) && (t <= 1.0f)) {
        m_ImpactIsLocal = (m_Context == 1);
        m_Collision.Collided = TRUE;
        m_Collision.pObjectHit = m_pContextObj;
        m_Collision.ID = m_ID;
        m_Collision.NodeIndex = m_NodeIndex;
        m_Collision.NodeType = m_NodeType;
        m_Collision.T = t;
        m_Collision.Point = P0 + t * (P1 - P0);
        m_Collision.Plane.Normal.Zero();

        if (PlaneD[MaxPlane] == BBox.Max[MaxPlane]) {
            m_Collision.Plane.Normal[MaxPlane] = 1;
            m_Collision.Plane.D = -BBox.Max[MaxPlane];
        } else {
            m_Collision.Plane.Normal[MaxPlane] = -1;
            m_Collision.Plane.D = BBox.Min[MaxPlane];
        }

        if (m_Context == 1) {
            m_ImpactL2W = m_ContextL2W;
        }

        // Can end now if this is just a line of sight test
        m_EarlyOut |= m_LineOfSightTest;
    }
}

//==============================================================================

void collider::ExtApplyBBox(const bbox& BBox) {
    // Early out?
    if (m_EarlyOut) {
        return;
    }

    // Do quick check.
    ext_info& Info = m_ExtInfo[m_Context];
    if (!Info.MovementBBox.Intersect(BBox)) {
        return;
    }

    s32 Axis = -1;
    f32 T = 2.0f;
    vector3 Normal(0, 0, 0);
    s32 i;

    // Quick test - collision T of zero? That is, are boxes overlapping from
    // the start?
    if (Info.StartBBox.Intersect(BBox)) {
        T = 0;
    } else {
        // Check all axes for a collision.
        for (i = 0; i < 3; i++) {
            // Moving on this axis?
            if (Info.Movement[i] != 0) {
                f32 FirstT = 2;
                xbool Overlap = FALSE;

                // Moving which way?
                if (Info.Movement[i] > 0) {
                    // Does movement overlap on this axis?
                    if ((Info.StartBBox.Max[i] <= BBox.Min[i])
                        && (Info.EndBBox.Max[i] >= BBox.Min[i])) {
                        // Calculate ratio.
                        FirstT = (BBox.Min[i] - Info.StartBBox.Max[i]) / Info.Movement[i];
                        Overlap = TRUE;
                    }
                } else {
                    // Does movement overlap on this axis?
                    if ((Info.StartBBox.Min[i] >= BBox.Max[i])
                        && (Info.EndBBox.Min[i] <= BBox.Max[i])) {
                        // Calculate ratio.
                        FirstT = (BBox.Max[i] - Info.StartBBox.Min[i]) / Info.Movement[i];
                        Overlap = TRUE;
                    }
                }

                // Possible collision?
                if (Overlap) {
                    //x_printf("Axis:%d CollT%f\n", i, FirstT);

                    // Clamp the computed T value.
                    if (FirstT < 0) {
                        FirstT = 0;
                    } else if (FirstT > 1) {
                        FirstT = 1;
                    }

                    // Possible valid collision?
                    if ((FirstT < T) && (FirstT >= 0) && (FirstT <= 1)) {
                        // Check if a collision occurred at this T.
                        bbox TBBox;
                        TBBox.Min = Info.StartBBox.Min + Info.Movement * FirstT;
                        TBBox.Max = Info.StartBBox.Max + Info.Movement * FirstT;

                        // Fix floating point inaccuracy.  After all, we already
                        // know the bbox overlaps on this axis!
                        TBBox.Min[i] = BBox.Min[i];
                        TBBox.Max[i] = BBox.Max[i];

                        // Is box overlapping on other axis at this T?
                        if (TBBox.Intersect(BBox)) {
                            // Keep it!
                            T = FirstT;
                            Axis = i;
                        }
                    }
                }
            }
        }
    }

    // No collision?
    if (T > 1.0f) {
        // Make sure start position does not overlap.
        ASSERT(!(Info.StartBBox.Intersect(BBox)));

        // Make sure end position does not overlap.
        ASSERT(!(Info.EndBBox.Intersect(BBox)));

        //x_printf("NO Coll\n");
        return;
    }

    //x_printf("Coll %f\n",T);

    // Did a collision occur which is the closest so far?
    if (T < m_Collision.T) {
        // Setup collision point.
        for (i = 0; i < 3; i++) {
            f32 Movement = Info.Movement[i] * T;
            if (Info.Movement[i] > 0.0f) {
                m_Collision.Point[i] = (BBox.Min[i] + Info.StartBBox.Max[i] + Movement) * 0.5f;
            } else {
                m_Collision.Point[i] = (BBox.Max[i] + Info.StartBBox.Min[i] + Movement) * 0.5f;
            }
        }

        // Setup plane normal.
        if (Axis == -1) {
            ASSERT(T == 0);
            // This means we collided at time 0.  Make normal of collision be
            // reverse of movement.
            Normal = -Info.Movement;
        } else {
            // Opposite of movement.
            Normal[Axis] = (Info.Movement[Axis] > 0.0f) ? -1.0f : 1.0f;
        }

        // Record in collision structure.
        m_ImpactIsLocal = (m_Context == 1);
        m_Collision.Collided = TRUE;
        m_Collision.pObjectHit = m_pContextObj;
        m_Collision.ID = m_ID;
        m_Collision.NodeIndex = m_NodeIndex;
        m_Collision.NodeType = m_NodeType;
        m_Collision.T = T;
        m_Collision.Plane.Setup(m_Collision.Point, Normal);

        if (m_Context == 1) {
            m_ImpactL2W = m_ContextL2W;
        }

        // Can end now if this is just a line of sight test
        m_EarlyOut |= m_LineOfSightTest;
    }
}

//==============================================================================

void collider::ApplyPermeableBBox(const bbox& BBox) {
    // Early out?
    if (m_EarlyOut) {
        return;
    }

    // Don't bother if collider is not interested
    if (m_NPermeables == -1) {
        return;
    }

    ASSERT(m_Context == 0);
    ASSERT(m_Type == EXTRUSION);

    // Do quick check.
    ext_info& Info = m_ExtInfo[0];
    if (!Info.MovementBBox.Intersect(BBox)) {
        return;
    }

    s32 Axis = -1;
    f32 T = 2.0f;
    s32 i;

    bbox Start = m_PermeableBBox;
    bbox End = Start;
    End.Translate(Info.Movement);

    // Quick test - collision T of zero? That is, are boxes overlapping from
    // the start?
    if (m_PermeableBBox.Intersect(BBox)) {
        T = 0.0f;
    } else {
        // Check all axes for a collision.
        for (i = 0; i < 3; i++) {
            // Moving on this axis?
            if (Info.Movement[i] != 0) {
                f32 FirstT = 2;
                xbool Overlap = FALSE;

                // Moving which way?
                if (Info.Movement[i] > 0) {
                    // Does movement overlap on this axis?
                    if ((Start.Max[i] <= BBox.Min[i]) && (End.Max[i] >= BBox.Min[i])) {
                        // Calculate ratio.
                        FirstT = (BBox.Min[i] - Start.Max[i]) / Info.Movement[i];
                        Overlap = TRUE;
                    }
                } else {
                    // Does movement overlap on this axis?
                    if ((Start.Min[i] >= BBox.Max[i]) && (End.Min[i] <= BBox.Max[i])) {
                        // Calculate ratio.
                        FirstT = (BBox.Max[i] - Start.Min[i]) / Info.Movement[i];
                        Overlap = TRUE;
                    }
                }

                // Possible collision?
                if (Overlap) {
                    // Clamp the computed T value.
                    if (FirstT < 0) {
                        FirstT = 0;
                    } else if (FirstT > 1) {
                        FirstT = 1;
                    }

                    // Possible valid collision?
                    if ((FirstT < T) && (FirstT >= 0) && (FirstT <= 1)) {
                        // Check if a collision occurred at this T.
                        bbox TBBox;
                        TBBox.Min = Start.Min + Info.Movement * FirstT;
                        TBBox.Max = Start.Max + Info.Movement * FirstT;

                        // Fix floating point inaccuracy.  After all, we already
                        // know the bbox overlaps on this axis!
                        TBBox.Min[i] = BBox.Min[i];
                        TBBox.Max[i] = BBox.Max[i];

                        // Is box overlapping on other axis at this T?
                        if (TBBox.Intersect(BBox)) {
                            // Keep it!
                            T = FirstT;
                            Axis = i;
                        }
                    }
                }
            }
        }
    }

    // No collision?
    if (T > 1.0f) {
        // Make sure start position does not overlap.
        ASSERT(!(Start.Intersect(BBox)));

        // Make sure end position does not overlap.
        ASSERT(!(End.Intersect(BBox)));

        return;
    }

    // Collision!  We must add this object to our list of hit permeables.
    {
        xarray<ext_pinfo> List(m_Permeable, 32, m_NPermeables);
        s32 i;
        ext_pinfo PInfo;

        PInfo.pObject = m_pContextObj;
        PInfo.T = T;

        if ((List.GetCount() == 32) && (List[31].T < T)) {
            // List is full and the thing we hit was hit AFTER the last item
            // in the list, so forget about it!
            return;
        }

        // If the list is full, blow away the last item.
        if (List.GetCount() == 32) {
            List.Delete(31);
        }

        // Find place to insert this hit.
        i = 0;
        while ((i < List.GetCount()) && (T < List[i].T)) {
            i++;
        }
        List.Insert(i, PInfo);

        m_NPermeables = List.GetCount();
    }
}

//==============================================================================

void collider::RayApplyNGon(const vector3* pPoint, s32 NPoints, const plane& Plane) {
    COLL_STAT_NRayApplyNGons++;

    // Early out?
    if (m_EarlyOut) {
        return;
    }

    // We do not collide with backfaces.  If the ray is moving "with" the normal
    // of the plane, then  there can be no collision.
    if (v3_Dot(Plane.Normal, m_RayInfo[m_Context].Dir) > 0.0f) {
        return;
    }

    // Get distance of points to plane.
    f32 D0 = Plane.Distance(m_RayInfo[m_Context].Start);
    f32 D1 = Plane.Distance(m_RayInfo[m_Context].End);

    // Check if on same side.
    if ((D0 < 0) && (D1 < 0)) {
        return;
    }
    if ((D0 > 0) && (D1 > 0)) {
        return;
    }

    COLL_STAT_NRayApplyNGonsSolve++;

    // Get point on plane.
    f32 T = (0 - D0) / (D1 - D0);
    vector3 IP =
        m_RayInfo[m_Context].Start + T * (m_RayInfo[m_Context].End - m_RayInfo[m_Context].Start);

    // Determine if point is inside tri.
    vector3 EdgeNormal;
    for (s32 i = 0; i < NPoints; i++) {
        EdgeNormal = Plane.Normal.Cross(pPoint[(i + 1) % NPoints] - pPoint[i]);
        if (EdgeNormal.Dot(IP - pPoint[i]) < -0.001f) {
            return;
        }
    }

    // Collision.
    if ((T < m_Collision.T) && (T >= 0.0f) && (T <= 1.0f)) {
        m_ImpactIsLocal = (m_Context == 1);
        m_Collision.Collided = TRUE;
        m_Collision.pObjectHit = m_pContextObj;
        m_Collision.ID = m_ID;
        m_Collision.NodeIndex = m_NodeIndex;
        m_Collision.NodeType = m_NodeType;
        m_Collision.T = T;
        m_Collision.Point = IP;
        m_Collision.Plane = Plane;

        if (m_Context == 1) {
            m_ImpactL2W = m_ContextL2W;
        }

        // Can end now if this is just a line of sight test
        m_EarlyOut |= m_LineOfSightTest;
    }
}

//==============================================================================

#define CLIP_BUFFER_VERTS 64

void collider::ExtApplyNGon(const vector3* pPoint, s32 NPoints, const plane& Plane) {
    COLL_STAT_NExtApplyNGons++;

    s32 i;

    // Early out?
    if (m_EarlyOut) {
        return;
    }

// So we can see the world space normal in the debugger...
#ifdef DEBUG_COLLISION
    plane WorldPlane = Plane;
    if (m_Context == 1) {
        WorldPlane.Transform(m_ContextL2W);
    }
#endif

    // We do not collide with backfaces.  If the direction of movement is "with"
    // the normal of the plane, then there can be no collision.
    f32 Dot = v3_Dot(Plane.Normal, m_ExtInfo[m_Context].Movement);
    if (Dot > 0) {
        return;
    }

    // Check if pill containing extrusion is completely on one side of ngon.
    f32 SD = Plane.Distance(m_ExtInfo[m_Context].StartCenter);
    f32 ED = Plane.Distance(m_ExtInfo[m_Context].EndCenter);
    if (((SD < 0) && (ED < 0)) || ((SD > 0) && (ED > 0))) {
        if (((SD * SD) > m_ExtInfo[m_Context].RadiusSquared)
            && ((ED * ED) > m_ExtInfo[m_Context].RadiusSquared)) {
            return;
        }
    }

    COLL_STAT_NExtApplyNGonsSolve++;

    // Loop through the volumes.
    for (s32 v = 0; v < m_ExtNVolumes; v++) {
        ext_volume* pVol = &m_ExtInfo[m_Context].Volume[v];

        // Allocate the clip buffer.
        static vector3 ClipBuffer[CLIP_BUFFER_VERTS * 2];

        // Clip points to volume planes.
        s32 NSrcPoints = NPoints;
        s32 NDstPoints;
        vector3* pSrc = (vector3*)pPoint;
        vector3* pDst = ClipBuffer;
        for (s32 p = 0; p < 3; p++) {
            const vector3& Normal = pVol->Normal[p];
            f32 MinD = pVol->MinD[p];
            f32 MaxD = pVol->MaxD[p];
            vector3* P0;
            vector3* P1;
            f32 D0;
            f32 D1;
            f32 OneOverDDiff;

            NDstPoints = 0;
            P1 = &pSrc[NSrcPoints - 1];
            D1 = Normal.X * P1->X + Normal.Y * P1->Y + Normal.Z * P1->Z;

            for (i = 0; i < NSrcPoints; i++) {
                P0 = P1;
                D0 = D1;
                P1 = &pSrc[i];
                D1 = Normal.X * P1->X + Normal.Y * P1->Y + Normal.Z * P1->Z;
                OneOverDDiff = 1.0f / (D1 - D0);

                if (D0 < MinD) {
                    if (D1 > MinD) {
                        f32 T = (MinD - D0) * OneOverDDiff;
                        pDst[NDstPoints] = (*P0) + T * ((*P1) - (*P0));
                        NDstPoints++;
                    }

                    if (D1 > MaxD) {
                        f32 T = (MaxD - D0) * OneOverDDiff;
                        pDst[NDstPoints] = (*P0) + T * ((*P1) - (*P0));
                        NDstPoints++;
                    }
                } else if (D0 > MaxD) {
                    if (D1 < MaxD) {
                        f32 T = (MaxD - D0) * OneOverDDiff;
                        pDst[NDstPoints] = (*P0) + T * ((*P1) - (*P0));
                        NDstPoints++;
                    }

                    if (D1 < MinD) {
                        f32 T = (MinD - D0) * OneOverDDiff;
                        pDst[NDstPoints] = (*P0) + T * ((*P1) - (*P0));
                        NDstPoints++;
                    }
                } else {
                    // Keep D0.
                    pDst[NDstPoints] = *P0;
                    NDstPoints++;

                    if (D1 < MinD) {
                        f32 T = (MinD - D0) * OneOverDDiff;
                        pDst[NDstPoints] = (*P0) + T * ((*P1) - (*P0));
                        NDstPoints++;
                    }

                    if (D1 > MaxD) {
                        f32 T = (MaxD - D0) * OneOverDDiff;
                        pDst[NDstPoints] = (*P0) + T * ((*P1) - (*P0));
                        NDstPoints++;
                    }
                }
            }

            // Flip buffers and continue.
            ASSERT(NDstPoints <= CLIP_BUFFER_VERTS);
            pSrc = pDst;
            pDst = (pDst == ClipBuffer) ? (ClipBuffer + CLIP_BUFFER_VERTS) : (ClipBuffer);
            NSrcPoints = NDstPoints;

            // Check if completely clipped out.
            if (NSrcPoints == 0) {
                break;
            }
        }

        // Check if we have some ngon in the volume.
        if (NSrcPoints > 0) {
            f32 MinD = F32_MAX;
            s32 MinPtID = 0;

            // Find MinD value.
            for (i = 0; i < NSrcPoints; i++) {
                f32 D = pVol->Normal[0].Dot(pSrc[i]);

                if (D < MinD) {
                    MinD = D;
                    MinPtID = i;
                }
            }

            // Compute T value and check if it beats current.
            f32 T = (MinD - pVol->MinD[0]) / (pVol->MaxD[0] - pVol->MinD[0]);

#ifdef DEBUG_COLLISION

            // Record n gon as best so far?
            if ((T >= 0.0f) && (T <= 1.0f)) {
                // Compare the plane we are moving to the plane we are hitting.
                f32 Dot = v3_Dot(pVol->Normal[0], Plane.Normal);

                if ((T < m_Collision.T) || ((T == m_Collision.T) && (Dot < m_BestImpactDot))) {
                    // Record NGon in world space
                    NCollPoints = NPoints;
                    if (m_Context == 0) {
                        for (i = 0; i < NPoints; i++) {
                            CollPoints[i] = pPoint[i];
                        }
                    } else {
                        for (i = 0; i < NPoints; i++) {
                            CollPoints[i] = m_ContextL2W * pPoint[i];
                        }
                    }
                }
            }

#endif // #ifdef DEBUG_COLLISION

            // See if this is the closest poly impact so far (keeping highest point in ngon)
            //ExtAddPolyImpact( T, pVol->Normal[0], Plane, pSrc[MinPtID], pPoint, NPoints );

            // See if this is the closest poly impact so far (keeping highest point in movement volume)
            ExtAddPolyImpact(T, pVol->Normal[0], Plane, pSrc[MinPtID], pSrc, NSrcPoints);

            // Early out?
            if (m_EarlyOut) {
                return;
            }
        }
    }
}

//==============================================================================

void collider::ExtAddPolyImpact(
    f32 T,
    const vector3& Normal,
    const plane& Plane,
    const vector3& CollisionPt,
    const vector3* pPoints,
    s32 NPoints
) {
    f32 Dot;
    s32 i;

    if ((T >= 0.0f) && (T <= 1.0f)) {
        // Try add to contact list?
        if (m_RecordContacts) {
            AddContact(T, Plane, CollisionPt);
        }

        // Compare the plane we are moving to the plane we are hitting.
        Dot = v3_Dot(Normal, Plane.Normal);

        if ((T < m_Collision.T) || ((T == m_Collision.T) && (Dot < m_BestImpactDot))) {
            m_BestImpactDot = Dot;
            m_ImpactIsLocal = (m_Context == 1);
            m_Collision.Collided = TRUE;
            m_Collision.pObjectHit = m_pContextObj;
            m_Collision.ID = m_ID;
            m_Collision.NodeIndex = m_NodeIndex;
            m_Collision.NodeType = m_NodeType;
            m_Collision.T = T;
            m_Collision.Point = CollisionPt;
            m_Collision.Plane = Plane;

            if (m_Context == 1) {
                m_ImpactL2W = m_ContextL2W;
            }

            // Record highest point for player stepping code
            {
                // We need to record the high point.  Its pretty easy in
                // world space, so we'll have separate code for each
                // context.
                if (m_Context == 0) {
                    for (i = 0; i < NPoints; i++) {
                        if (m_ExtHigh < pPoints[i].Y) {
                            m_ExtHigh = pPoints[i].Y;
                        }
                    }
                } else {
                    // Go directly to world space (only consider y component)
                    for (i = 0; i < NPoints; i++) {
                        // Calc world y of point
                        f32 Y = (m_ContextL2W(0, 1) * pPoints[i].X)
                                + (m_ContextL2W(1, 1) * pPoints[i].Y)
                                + (m_ContextL2W(2, 1) * pPoints[i].Z) + m_ContextL2W(3, 1);

                        if (m_ExtHigh < Y) {
                            m_ExtHigh = Y;
                        }
                    }
                }
            }

            // Can end now if this is just a line of sight test
            m_EarlyOut |= m_LineOfSightTest;
        }
    }
}

//==============================================================================

inline void collider::ExtComputePill(ext_info& Info) {
    s32 i, j;
    vector3 C(0, 0, 0);

    for (i = 0; i < m_ExtNVolumes; i++) {
        C += Info.Volume[i].Point[0];
        C += Info.Volume[i].Point[1];
        C += Info.Volume[i].Point[2];
        C += Info.Volume[i].Point[3];
    }
    C *= (1.0f / (f32)(m_ExtNVolumes * 4));
    Info.StartCenter = C;
    Info.EndCenter = Info.StartCenter + Info.Movement;

    Info.RadiusSquared = 0;
    for (i = 0; i < m_ExtNVolumes; i++) {
        for (j = 0; j < 4; j++) {
            f32 R = (Info.Volume[i].Point[j] - C).LengthSquared();
            if (R > Info.RadiusSquared) {
                Info.RadiusSquared = R;
            }
        }
    }
}

//==============================================================================

inline void collider::ExtComputeVolumePlanes(ext_volume& Vol, const vector3& Move) {
    plane Plane;

    Plane.Setup(Vol.Point[0], Vol.Point[1], Vol.Point[2]);
    Vol.Normal[0] = Plane.Normal;
    Vol.MinD[0] = -Plane.D;
    Vol.MaxD[0] = Vol.Normal[0].Dot(Vol.Point[4]);

    Plane.Setup(Vol.Point[1], Vol.Point[0], Vol.Point[0] + Move);
    Vol.Normal[1] = Plane.Normal;
    Vol.MinD[1] = -Plane.D;
    Vol.MaxD[1] = Vol.Normal[1].Dot(Vol.Point[3]);

    Plane.Setup(Vol.Point[2], Vol.Point[1], Vol.Point[1] + Move);
    Vol.Normal[2] = Plane.Normal;
    Vol.MinD[2] = -Plane.D;
    Vol.MaxD[2] = Vol.Normal[2].Dot(Vol.Point[0]);
}

//==============================================================================

void collider::ExtAddVolume(s32 VID, const vector3* pCorners) {
    ext_volume& Vol = m_ExtInfo[m_Context].Volume[m_ExtNVolumes];

    VID *= 4;

    for (s32 i = 0; i < 4; i++) {
        Vol.Point[i] = pCorners[FaceIndex[VID + i]];
        Vol.Point[i + 4] = Vol.Point[i] + m_ExtInfo[0].Movement;
    }

    ExtComputeVolumePlanes(Vol, m_ExtInfo[m_Context].Movement);
    m_ExtNVolumes++;
}

//==============================================================================
