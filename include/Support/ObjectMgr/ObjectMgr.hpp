// Imported genuine tribes-aa pristine4aab7137 engine reference: aa/support/objectmgr/ObjectMgr.hpp
// Provisional Hobbit API/layout adaptation; no fabricated PC labels. See IMPORT-NOTES.md.
//==============================================================================
//
//  ObjectMgr.hpp
//
//==============================================================================

#ifndef OBJECTMGR_HPP
#define OBJECTMGR_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/Pain.hpp>
#include <Support/ObjectMgr/Object.hpp>
#include <xCore/Entropy/e_View.hpp>
#include <xCore/x_files/x_time.hpp>
#include <xCore/x_files/x_stdio.hpp>

//==============================================================================
//  TYPES
//==============================================================================

#define OBJ_NET_INFO_SAMPLES 32

struct obj_net_info {
    s32 CreateBitsPacked[OBJ_NET_INFO_SAMPLES];
    s32 UpdateBitsPacked[OBJ_NET_INFO_SAMPLES];
    s32 CreatesPacked[OBJ_NET_INFO_SAMPLES];
    s32 UpdatesPacked[OBJ_NET_INFO_SAMPLES];
    s32 SampleIndex;

    void Clear(void);
    void
    GetStats(s32& CreateBitsPacked, s32& UpdateBitsPacked, s32& CreatesPacked, s32& UpdatesPacked);
    void AddCreate(s32 NBits);
    void AddUpdate(s32 NBits);
    void NextSample(void);
};

//==============================================================================

struct obj_type_info {
public:
    object::type Type;
    char* pTypeName;
    obj_create_fn* pCreateFn;
    u32 DefaultAttrBits;

    s32 InstanceCount;
    //xtimer              LogicTime;
    //f32                 AverageLogicTime;

    obj_net_info NetInfo;

    obj_type_info(
        object::type Type,
        char* pTypeName,
        obj_create_fn* pCreateFn,
        u32 DefaultAttrBits
    );

private:
    obj_type_info(void);
};

//------------------------------------------------------------------------------

typedef s16 obj_ref;

//==============================================================================
//  CLASS OBJ_MGR
//==============================================================================

class obj_mgr {
    //------------------------------------------------------------------------------
    //  Public Values and Types
    //------------------------------------------------------------------------------

public:
    enum {
        AVAILABLE_SERVER_SLOT = -1,
        AVAILABLE_CLIENT_SLOT = -2,
        MAX_TYPE_CURSORS = 5,
        MAX_SELECTIONS = 5,
        MAX_SERVER_OBJECTS_BITS = 10,
        MAX_CLIENT_OBJECTS_BITS = 9,
        MAX_SERVER_OBJECTS = (1 << MAX_SERVER_OBJECTS_BITS),
        MAX_CLIENT_OBJECTS = (1 << MAX_CLIENT_OBJECTS_BITS),
        MAX_ZONES = 512,
        MAX_OBJECTS = MAX_SERVER_OBJECTS + MAX_CLIENT_OBJECTS,
        MAX_REF_NODES = 2048 + MAX_OBJECTS + MAX_ZONES + (32 * 32) + 2,
    };

    static const object::id NullID;

    //------------------------------------------------------------------------------
    //  Public Functions
    //------------------------------------------------------------------------------

public:
    obj_mgr(void);
    ~obj_mgr(void);

    void Clear(void);

    object* CreateObject(object::type ObjectType);
    void AddObject(object* pObject, s32 Slot, const char* pTag = NULL);

    object::id GetIDFromSlot(s32 Slot);
    object* GetObjectFromSlot(s32 Slot);

    object* GetObjectFromID(const object::id& ObjectID);
    object* GetObjectFromID(object::id& ObjectID);

    s32 GetInstanceCount(object::type ObjectType);

    void AdvanceAllLogic(f32 DeltaTime);

    void UpdateObject(object* pObject);
    void RemoveObject(object* pObject);
    void DestroyObject(object* pObject);

    void CreateInitObject(object::type ObjectType, s32 Slot, bitstream& BitStream);
    void DestroyObject(object::id ObjectID);
    void AcceptUpdate(object::id ObjectID, bitstream& BitStream, f32 SecInPast);

    void Select(u32 AttrMask, const vector3& Center, f32 Radius);

    void Select(u32 AttrMask, const bbox& BBox);
    /*
        void        Select          (       u32      AttrMask,
                                      const view&    View );
*/
    void Select(u32 AttrMask); // All!

    void SelectOutOfRep0(u32 AttrMask);

    void ClearSelection(void);
    object* GetNextSelected(void);
    void RestartSelection(void);

    void UnbindOriginID(object::id OriginID);
    void UnbindTargetID(object::id TargetID);

    void Collide(u32 AttrMask, collider& Collider);

    void InflictPain(
        pain::type PainType,
        const vector3& Position,
        object::id OriginID,
        object::id VictimID,
        f32 PainScalar = 1.0f
    );

    void InflictPain(
        pain::type PainType,
        object* pObject,
        const vector3& Position,
        object::id OriginID,
        object::id VictimID,
        xbool DirectHit = TRUE,
        f32 PainScalar = 1.0f
    );

    f32 GetDamageRadius(pain::type PainType) const;

    void ProcessImpact(object* pMovingObj, const collider::collision& Collision);

#ifdef X_DEBUG
    void DbgStartTypeLoop(object::type Type, char* pFile, s32 Line);
#endif
    void StartTypeLoop(object::type Type);
    object* GetNextInType(void);
    void EndTypeLoop(void);

    void PrintStats(X_FILE* pFile = NULL);

    const obj_type_info* GetTypeInfo(object::type Type);

    //------------------------------------------------------------------------------
    //  Internal Types
    //------------------------------------------------------------------------------

protected:
    struct obj_ref_node {
        s16 Slot;
        obj_ref ChainNext;
        obj_ref CellNext;
        obj_ref CellPrev;
        obj_ref Cell;
    };

    //------------------------------------------------------------------------------
    //  Internal Data
    //------------------------------------------------------------------------------

protected:
    // The Initialized flag is static to ensure that only one object manager is
    // ever instantiated.  The ObjTypeInfo is static since is statically
    // initialized.

    static xbool m_Initialized;
    static obj_type_info* m_pObjTypeInfo[object::TYPE_END_OF_LIST];

    object* m_pObject[MAX_OBJECTS];
    obj_ref m_GridChain[MAX_OBJECTS];
    s32 m_LastSelect[MAX_OBJECTS];
    obj_ref_node m_RefNode[MAX_REF_NODES];
    obj_ref m_FreeChainHead;
    obj_ref m_FreeChainTail;
    s16 m_ObjectSeq;

    obj_ref m_SelectChain[MAX_SELECTIONS];
    obj_ref m_SelectCursor[MAX_SELECTIONS];
    s32 m_SelectCursorID; // Stack index
    s32 m_SelectSeq;

    obj_ref m_TypeCursor[MAX_TYPE_CURSORS]; // Cursor stack
    s32 m_TypeCursorID;                     // Stack index

    //------------------------------------------------------------------------------
    //  Internal Functions
    //------------------------------------------------------------------------------

protected:
    void Initialize(void);

    void AdvanceLogicForType(object::type Type, f32 DeltaTime);

    void InsertObjIntoGrid(object* pObject);
    obj_ref GetNewRef(void);
    void CreateCellRef(s32 Slot, obj_ref Cell, xbool AddToChain = TRUE);
    void ReleaseChain(obj_ref& RefList);
    void AddToChain(obj_ref& ChainHead, s32 Slot, obj_ref Cell);
    void SelectCell(obj_ref Cell, const bbox& BBox, u32 AttrMask);
    void SelectCell(obj_ref Cell, const vector3& Center, f32 Radius, u32 AttrMask);
    void SelectCell(obj_ref Cell, u32 AttrMask);
    void CollideRay(u32 AttrMask, collider& Collider);
    void CollideExt(u32 AttrMask, collider& Collider);
    void CollideWithCell(obj_ref Cell, u32 AttrMask, collider& Collider);

    void Log(const char* pOpcode, object* pObject, s32 Extra = 0);

    void SanityCheck(void);

    //------------------------------------------------------------------------------

    friend obj_type_info;
};

#ifdef X_DEBUG
#define StartTypeLoop(t) DbgStartTypeLoop(t, __FILE__, __LINE__)
#endif

//==============================================================================
//  GLOBAL VARIABLES
//==============================================================================

extern obj_mgr ObjMgr;

//==============================================================================
#endif // OBJECTMGR_HPP
//==============================================================================
