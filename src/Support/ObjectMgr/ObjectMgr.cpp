// Imported genuine tribes-aa pristine4aab7137 engine reference: aa/support/objectmgr/ObjectMgr.cpp
// Provisional Hobbit API/layout adaptation; no fabricated PC labels. See IMPORT-NOTES.md.
//==============================================================================
//
//  ObjectMgr.cpp
//
//==============================================================================

//==============================================================================
//
//  Assumptions:
//   - The unit of measure is a meter.
//   - One "rep" of the terrain is composed of 256x256 squares.
//   - Each terrain square is 8 meters on a side.
//   - Thus, the terrain is 2048m, or over 2km, on a side.
//   - A terrain block is 8x8 squares.
//   - Thus, the terrain has 32x32 blocks, each 64m on a side.
//
//   - The object manager's grid is based on terrain blocks.
//
//
//      0------+------+------+-...-+------+  +X
//      |   0  |   1  |   2  |     |  31  |
//      +------+------+------+-   -+------+
//      |  32  |  33  |  34  |     |  63  |
//      +------+------+------+-   -+------+
//      |   :
//      |   :
//     +Z
//
//==============================================================================

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/ObjectMgr.hpp>
#include <xCore/Entropy/Entropy.hpp>
#include <Support/GameMgr/GameMgr.hpp>
#include <Support/Objects/Projectiles/Waypoint.hpp>
#include <Support/Objects/bot/BotObject.hpp>
#include <Support/Objects/Player/PlayerObject.hpp>
#include <Support/Objects/Vehicles/Vehicle.hpp>
#include <Support/Objects/Vehicles/gndeffect.hpp>

// HACK HACK
// HACK HACK
#include <Support/Demo1/Globals.hpp>
// HACK HACK
// HACK HACK

//==============================================================================
//  DEFINES
//==============================================================================

#define CLASS_BASE (1024)
#define CLASS_END (CLASS_BASE + object::TYPE_END_OF_LIST)

#define OUT_OF_GRID (CLASS_END + 0) // "Outside of grid" grid cell.
#define ALL_OF_GRID (CLASS_END + 1) // "All of grid" grid cell.

#define ZONE_BASE (CLASS_END + 2)        // Start of zone cells.
#define ZONE_END (ZONE_BASE + MAX_ZONES) // End   of zone cells.

#define CELL_END (ZONE_END)

#define OPEN_BASE (ZONE_END) // Start of open nodes.

//------------------------------------------------------------------------------

//#define OBJ_MGR_LOG
//#define OBJ_MGR_STATS

//==============================================================================
//  MACROS TO ACCESS THE REFERENCE NODES
//==============================================================================

#define CHAIN_NEXT(Ref) m_RefNode[(Ref)].ChainNext
#define CELL_NEXT(Ref) m_RefNode[(Ref)].CellNext
#define CELL_PREV(Ref) m_RefNode[(Ref)].CellPrev
#define OBJ_SLOT(Ref) m_RefNode[(Ref)].Slot
#define CELL(Ref) m_RefNode[(Ref)].Cell

//==============================================================================
//  TYPES
//==============================================================================

#ifdef OBJ_MGR_LOG
struct log_entry {
    s32 Sequence;
    char Opcode[4];
    char Type[4];
    u32 Address;
    s32 ObjID;
    s32 Extra;
};
#endif

//==============================================================================
//  STORAGE
//==============================================================================

obj_mgr ObjMgr; // Looks rather innocent, eh?  Don't be fooled!
s32 ObjSequence = 0;

//------------------------------------------------------------------------------

const object::id obj_mgr::NullID;
xbool obj_mgr::m_Initialized = FALSE;
obj_type_info* obj_mgr::m_pObjTypeInfo[object::TYPE_END_OF_LIST] = {0};

//------------------------------------------------------------------------------

#ifdef OBJ_MGR_LOG
log_entry LogData[256];
s32 LogCursor = 0;
s32 LogSequence = 0;
#endif

#ifdef OBJMGR_STATS
static s32 ColliderRayCount;
static xtimer ColliderRayTime;
static s32 ColliderExtCount;
static xtimer ColliderExtTime;
#endif

//static xbool SHOW_LONGEST_LOGIC = TRUE;
static xbool SHOW_COLLIDER_TIME = FALSE;

void AdvanceAllBotLogic(f32 DeltaTime);

extern xbool g_TweakGame;

//==============================================================================
//  UTILITY FUNCTIONS
//==============================================================================

static object::type GetTypeForPain(object* pObject) {
    object::type ObjType = pObject->GetType();

    if (pObject->GetAttrBits() & object::ATTR_PLAYER) {
        player_object* pPlayer = (player_object*)pObject;
        player_object::armor_type Armor = pPlayer->GetArmorType();
        ASSERT(IN_RANGE(player_object::ARMOR_TYPE_LIGHT, Armor, player_object::ARMOR_TYPE_HEAVY));
        ObjType = (object::type)(object::TYPE_PLAYER + (s32)Armor);
    }

    return (ObjType);
}

//==============================================================================
//  OBJECT TYPE INFO FUNCTIONS
//==============================================================================

obj_type_info::obj_type_info(
    object::type aType,
    char* apTypeName,
    obj_create_fn* apCreateFn,
    u32 aDefaultAttrBits
) {
    Type = aType;
    pTypeName = apTypeName;
    pCreateFn = apCreateFn;
    DefaultAttrBits = aDefaultAttrBits;

    InstanceCount = 0;

    obj_mgr::m_pObjTypeInfo[aType] = this;

    NetInfo.Clear();
    //AverageLogicTime = 0.0f;
}

//==============================================================================
//  OBJECT MANAGER FUNCTIONS
//==============================================================================

inline void obj_mgr::Log(const char* pOpcode, object* pObject, s32 Extra) {
#ifndef OBJ_MGR_LOG
    (void)pOpcode;
    (void)pObject;
    (void)Extra;
#else
    LogData[LogCursor].Sequence = ENDIAN_SWAP_32(LogSequence);
    LogData[LogCursor].Opcode[0] = pOpcode[0];
    LogData[LogCursor].Opcode[1] = pOpcode[1];
    LogData[LogCursor].Opcode[2] = pOpcode[2];
    LogData[LogCursor].Opcode[3] = pOpcode[3];
    if (pObject) {
        LogData[LogCursor].Type[0] = pObject->m_pTypeInfo->pTypeName[0];
        LogData[LogCursor].Type[1] = pObject->m_pTypeInfo->pTypeName[1];
        LogData[LogCursor].Type[2] = pObject->m_pTypeInfo->pTypeName[2];
        LogData[LogCursor].Type[3] = pObject->m_pTypeInfo->pTypeName[3];
        LogData[LogCursor].Address = ENDIAN_SWAP_32((u32)pObject);
        LogData[LogCursor].ObjID = ENDIAN_SWAP_32((s32)pObject->m_ObjectID);
    } else {
        LogData[LogCursor].Type[0] = pOpcode[0];
        LogData[LogCursor].Type[1] = pOpcode[1];
        LogData[LogCursor].Type[2] = pOpcode[2];
        LogData[LogCursor].Type[3] = pOpcode[3];
        LogData[LogCursor].Address = 0;
        LogData[LogCursor].ObjID = 0;
    }
    LogData[LogCursor].Extra = ENDIAN_SWAP_32((s32)Extra);

    LogSequence++;
    LogCursor = (LogCursor + 1) & 0xFF;
#endif
}

//==============================================================================

obj_mgr::obj_mgr(void) {
    ASSERT(!m_Initialized);
    m_Initialized = TRUE;

    Initialize();
}

//==============================================================================

obj_mgr::~obj_mgr(void) {
    ASSERT(m_Initialized);
    m_Initialized = FALSE;

    // Assert that there are no objects left?
}

//==============================================================================

void obj_mgr::Initialize(void) {
    s32 i;

    // Prepare the cell lists which have a pre-reserved "header" node.
    for (i = 0; i < CELL_END; i++) {
        OBJ_SLOT(i) = -1;            // Indicates ref list header.
        CHAIN_NEXT(i) = (obj_ref)-1; // Indicates ref list header.
        CELL_NEXT(i) = (obj_ref)i;
        CELL_PREV(i) = (obj_ref)i;
        CELL(i) = (obj_ref)-1;
    }

    // Prepare the free nodes.
    for (i = OPEN_BASE; i < MAX_REF_NODES; i++) {
        OBJ_SLOT(i) = -1;
        CHAIN_NEXT(i) = (obj_ref)(i + 1);
    }
    CHAIN_NEXT(MAX_REF_NODES - 1) = -1;
    m_FreeChainHead = OPEN_BASE;
    m_FreeChainTail = MAX_REF_NODES - 1;
    ASSERT((m_FreeChainTail == -1) || (CHAIN_NEXT(m_FreeChainTail) == -1));

    // Initialize master object list and the first ref list.
    for (i = 0; i < MAX_OBJECTS; i++) {
        m_pObject[i] = NULL;
        m_GridChain[i] = -1;
        m_LastSelect[i] = -1;
    }

    // Clear the selection information.
    m_SelectSeq = 0;
    m_ObjectSeq = 0;
    m_SelectCursorID = -1;
    for (i = 0; i < MAX_SELECTIONS; i++) {
        m_SelectChain[i] = -1;
        m_SelectCursor[i] = -1;
    }

    // Clear the type cursor information.
    m_TypeCursorID = -1;
    for (i = 0; i < MAX_TYPE_CURSORS; i++) {
        m_TypeCursor[i] = -1;
    }
}

//==============================================================================

object* obj_mgr::CreateObject(object::type Type) {
    object* pObject;
    ASSERT(m_Initialized);
    ASSERT(IN_RANGE(0, Type, object::TYPE_END_OF_LIST - 1));
    ASSERT(m_pObjTypeInfo[Type]->pCreateFn);

    pObject = m_pObjTypeInfo[Type]->pCreateFn();

    pObject->m_pTypeInfo = m_pObjTypeInfo[Type];
    pObject->m_ObjectID.Slot = -1;
    pObject->m_ObjectID.Seq = -1;
    pObject->m_DirtyBits = 0;
    pObject->m_AttrBits = m_pObjTypeInfo[Type]->DefaultAttrBits;

#ifdef OBJ_MGR_LOG
    Log("CREA", pObject);
#endif

    return (pObject);
}

//==============================================================================

void obj_mgr::AddObject(object* pObject, s32 Slot, const char* pTag) {
    s32 i;

    ASSERT(pObject);
    ASSERT(pObject->m_ObjectID.Slot == -1);
    ASSERT(pObject->m_pTypeInfo);
    ASSERT(pObject->m_pTypeInfo == m_pObjTypeInfo[pObject->m_pTypeInfo->Type]);

    // Count the instance in the type info.
    pObject->m_pTypeInfo->InstanceCount++;

    // Make sure we have a valid slot.
    if (Slot == AVAILABLE_SERVER_SLOT) {
        for (i = 32; i < MAX_SERVER_OBJECTS; i++) {
            if (m_pObject[i] == NULL) {
                Slot = i;
                break;
            }
        }
    } else if (Slot == AVAILABLE_CLIENT_SLOT) {
        //ASSERT This is not a server-only.
        for (i = MAX_SERVER_OBJECTS; i < MAX_OBJECTS; i++) {
            if (m_pObject[i] == NULL) {
                Slot = i;
                break;
            }
        }
    }

    ASSERT(Slot >= 0);
    ASSERT(Slot < MAX_OBJECTS);
    ASSERT(m_pObject[Slot] == NULL); // Already an object there!

    // Add the object to the master list.
    m_pObject[Slot] = pObject;

    // Give the object its ID number.
    m_ObjectSeq++;
    m_ObjectSeq = MAX(m_ObjectSeq, 0);
    pObject->m_ObjectID.Slot = Slot;
    pObject->m_ObjectID.Seq = m_ObjectSeq;

    // Insert object into grid.
    InsertObjIntoGrid(pObject);

    // Add the object to its "type cell".
    // Do NOT add this ref to the object's singly linked list for the grid.
    CreateCellRef(Slot, CLASS_BASE + pObject->m_pTypeInfo->Type, FALSE);

#ifdef OBJ_MGR_LOG
    Log("ADD ", pObject, pObject->m_pTypeInfo->InstanceCount);
#endif

    // Call the object's OnAdd function.
    pObject->OnAdd();

    // If there is a tag, register the object with the GameMgr.
    if (pTag && pTag[0]) {
        ASSERT(pGameLogic);
        pGameLogic->RegisterItem(pObject->m_ObjectID, pTag);

        if (pObject->GetType() == object::TYPE_WAYPOINT) {
            waypoint* pWayPt = (waypoint*)pObject;

            pWayPt->SetHidden(TRUE);
            /*
            #ifdef TARGET_PC            
            const xwchar* pLabel = pWayPt->GetLabel();
            if( pLabel[0] == '\0' )
                pWayPt->SetLabel( pTag );
            #endif
*/
        }
    }
}

//==============================================================================

void obj_mgr::CreateInitObject(object::type ObjectType, s32 Slot, bitstream& BitStream) {
    object* pObject = CreateObject(ObjectType);
    pObject->OnAcceptInit(BitStream);
    AddObject(pObject, Slot);
}

//==============================================================================

void obj_mgr::DestroyObject(object::id ObjectID) {
    object* pObject = GetObjectFromID(ObjectID);
    if (pObject) {
        RemoveObject(pObject);
        DestroyObject(pObject);
    }
}

//==============================================================================

void obj_mgr::AcceptUpdate(object::id ObjectID, bitstream& BitStream, f32 SecInPast) {
    object::update_code UpdateCode;
    object* pObject = GetObjectFromID(ObjectID);
    ASSERT(pObject);
    UpdateCode = pObject->OnAcceptUpdate(BitStream, SecInPast);
    switch (UpdateCode) {
        case object::UPDATE:
            UpdateObject(pObject);
            break;
        case object::DESTROY:
            if (tgl.ServerPresent) {
                RemoveObject(pObject);
                DestroyObject(pObject);
            }
            break;
        default:
            break;
    }
}

//==============================================================================

object::id obj_mgr::GetIDFromSlot(s32 Slot) {
    if (Slot == -1) {
        return (NullID);
    }

    ASSERT(Slot >= 0);
    ASSERT(Slot < MAX_OBJECTS);

    object::id Result(Slot, -1);

    if (m_pObject[Slot]) {
        Result.Seq = m_pObject[Slot]->m_ObjectID.Seq;
    }

    return (Result);
}

//==============================================================================

object* obj_mgr::GetObjectFromSlot(s32 Slot) {
    if (Slot == -1) {
        return (NULL);
    }

    ASSERT(Slot >= 0);
    ASSERT(Slot < MAX_OBJECTS);

    return (m_pObject[Slot]);
}

//==============================================================================

object* obj_mgr::GetObjectFromID(object::id& ObjectID) {
    // If slot is -1, that signals NULL object.
    if (ObjectID.Slot == -1) {
        return (NULL);
    }

    // The Slot is not -1.
    // Make sure it is valid otherwise.
    ASSERT(ObjectID.Slot >= 0);
    ASSERT(ObjectID.Slot < MAX_OBJECTS);

    // If there is nothing in the ObjMgr at given slot, then return NULL.
    if (m_pObject[ObjectID.Slot] == NULL) {
        return (NULL);
    }

    // There is an object in the ObjMgr at the given slot.
    // Make sure the Seq numbers are compatible.
    if ((ObjectID.Seq != -1) && (ObjectID.Seq != m_pObject[ObjectID.Slot]->m_ObjectID.Seq)) {
        return (NULL);
    }

    // If the Seq number was a wild card, patch it up now.
    if (ObjectID.Seq == -1) {
        ObjectID.Seq = m_pObject[ObjectID.Slot]->m_ObjectID.Seq;
    }

    // There is an object, and the Seq numbers were compatible.
    // Return it!
    return (m_pObject[ObjectID.Slot]);
}

//==============================================================================

object* obj_mgr::GetObjectFromID(const object::id& ObjectID) {
    // If slot is -1, that signals NULL object.
    if (ObjectID.Slot == -1) {
        return (NULL);
    }

    // The Slot is not -1.
    // Make sure it is valid otherwise.
    ASSERT(ObjectID.Slot >= 0);
    ASSERT(ObjectID.Slot < MAX_OBJECTS);

    // If there is nothing in the ObjMgr at given slot, then return NULL.
    if (m_pObject[ObjectID.Slot] == NULL) {
        return (NULL);
    }

    // There is an object in the ObjMgr at the given slot.
    // Make sure the Seq numbers are compatible.
    if ((ObjectID.Seq != -1) && (ObjectID.Seq != m_pObject[ObjectID.Slot]->m_ObjectID.Seq)) {
        return (NULL);
    }

    // There is an object, and the Seq numbers were compatible.
    // Return it!
    return (m_pObject[ObjectID.Slot]);
}

//==============================================================================

void obj_mgr::CreateCellRef(s32 Slot, obj_ref Cell, xbool AddToChain) {
    obj_ref NewRef = GetNewRef();

    OBJ_SLOT(NewRef) = Slot;
    CELL(NewRef) = Cell;
    CELL_NEXT(NewRef) = Cell;
    CELL_PREV(NewRef) = CELL_PREV(Cell);

    CELL_PREV(Cell) = NewRef;
    CELL_NEXT(CELL_PREV(NewRef)) = NewRef;

    if (AddToChain) {
        CHAIN_NEXT(NewRef) = m_GridChain[Slot];
        m_GridChain[Slot] = NewRef;
    } else {
        CHAIN_NEXT(NewRef) = -1;
    }
}

//==============================================================================

void obj_mgr::InsertObjIntoGrid(object* pObject) {
    if (pObject->m_AttrBits & object::ATTR_GLOBAL) {
        CreateCellRef(pObject->m_ObjectID.Slot, ALL_OF_GRID);
    } else {
        // The grid squares are 64 units (meters) on a side.

        s32 XLo, XHi, ZLo, ZHi, i, j;
        xbool Out = FALSE;

        XLo = (s32)x_floor(pObject->m_WorldBBox.Min.X / 64.0f);
        ZLo = (s32)x_floor(pObject->m_WorldBBox.Min.Z / 64.0f);
        XHi = (s32)x_floor(pObject->m_WorldBBox.Max.X / 64.0f);
        ZHi = (s32)x_floor(pObject->m_WorldBBox.Max.Z / 64.0f);

        if (XLo < 0) {
            XLo = 0;
            Out = TRUE;
        }
        if (ZLo < 0) {
            ZLo = 0;
            Out = TRUE;
        }
        if (XHi > 31) {
            XHi = 31;
            Out = TRUE;
        }
        if (ZHi > 31) {
            ZHi = 31;
            Out = TRUE;
        }

        for (j = ZLo; j <= ZHi; j++) {
            for (i = XLo; i <= XHi; i++) {
                obj_ref GridID = (j << 5) + i;
                CreateCellRef(pObject->m_ObjectID.Slot, GridID);
            }
        }

        if (Out) {
            CreateCellRef(pObject->m_ObjectID.Slot, OUT_OF_GRID);
        }
    }
}

//==============================================================================

void obj_mgr::ReleaseChain(obj_ref& ChainHead) {
    if (ChainHead == -1) {
        return;
    }

    obj_ref Ref = ChainHead;

    while (TRUE) {
        ASSERT(CELL(Ref) != -1);
        OBJ_SLOT(Ref) = -1;
        CELL(Ref) = -1;
        if (CELL_NEXT(Ref) != -1) {
            CELL_NEXT(CELL_PREV(Ref)) = CELL_NEXT(Ref);
            CELL_PREV(CELL_NEXT(Ref)) = CELL_PREV(Ref);
            CELL_NEXT(Ref) = -1;
            CELL_PREV(Ref) = -1;
        }

        if (CHAIN_NEXT(Ref) == -1) {
            break;
        }

        Ref = CHAIN_NEXT(Ref);
    }

    ASSERT((m_FreeChainTail == -1) || (CHAIN_NEXT(m_FreeChainTail) == -1));
    if (m_FreeChainTail == -1) {
        ASSERT(m_FreeChainHead == -1);
        m_FreeChainHead = Ref;
        m_FreeChainTail = Ref;
    } else {
        CHAIN_NEXT(m_FreeChainTail) = ChainHead;
        m_FreeChainTail = Ref;
    }

    ChainHead = -1;
}

//==============================================================================

obj_ref obj_mgr::GetNewRef(void) {
    ASSERT(m_FreeChainHead != -1); // Out of ref nodes!
    obj_ref NewRef = m_FreeChainHead;
    m_FreeChainHead = CHAIN_NEXT(m_FreeChainHead);
    if (NewRef == m_FreeChainTail) {
        m_FreeChainTail = -1;
        ASSERT(m_FreeChainHead == -1);
    }
    return (NewRef);
}

//==============================================================================

void obj_mgr::RemoveObject(object* pObject) {
    ASSERT(pObject);
    ASSERT(pObject->m_ObjectID.Slot >= 0);
    ASSERT(pObject->m_ObjectID.Slot < MAX_OBJECTS);

#ifdef OBJ_MGR_LOG
    Log("REMV", pObject, pObject->m_pTypeInfo->InstanceCount);
#endif

    // Call the object's OnRemove function.
    pObject->OnRemove();

    // Update the the instance count in the type info.
    pObject->m_pTypeInfo->InstanceCount--;

    // We need to find this object's reference in its "type cell".  We have to
    // do a linear search.
    obj_ref TypeCell = CLASS_BASE + pObject->m_pTypeInfo->Type;
    obj_ref Ref = CELL_NEXT(TypeCell);
    while (TRUE) {
        if (OBJ_SLOT(Ref) == pObject->m_ObjectID.Slot) {
            // Found it!

            // If the object we are removing is under one of the type cursors,
            // then we have to back the cursor off before we remove the object.
            for (s32 i = 0; i <= m_TypeCursorID; i++) {
                if (m_TypeCursor[i] == Ref) {
                    m_TypeCursor[i] = CELL_PREV(m_TypeCursor[i]);
                }
            }

            // Now remove it.
            CHAIN_NEXT(Ref) = -1;
            ReleaseChain(Ref);
            break;
        } else {
            Ref = CELL_NEXT(Ref);
            ASSERT(Ref != TypeCell);
        }
    }

    // Release the object's chain of references.  This will clear it from all
    // grid cells.
    ReleaseChain(m_GridChain[pObject->m_ObjectID.Slot]);

    // And finalize.
    m_pObject[pObject->m_ObjectID.Slot] = NULL;
    pObject->m_ObjectID.Slot = -1;
}

//==============================================================================

void obj_mgr::DestroyObject(object* pObject) {
    ASSERT(pObject);
    ASSERT(pObject->m_ObjectID.Slot == -1);

    delete pObject;
}

//==============================================================================

void obj_mgr::Clear(void) {
    s32 i;

    for (i = 0; i < MAX_OBJECTS; i++) {
        //SanityCheck();
        if (m_pObject[i]) {
            object* pObject = m_pObject[i];
            RemoveObject(pObject);
            DestroyObject(pObject);
        }
        //SanityCheck();
    }

    Initialize();
    //SanityCheck();
}

//==============================================================================

void obj_mgr::UpdateObject(object* pObject) {
    ASSERT(pObject);
    ASSERT(pObject->m_ObjectID.Slot >= 0);
    ASSERT(pObject->m_ObjectID.Slot < MAX_OBJECTS);

#ifdef OBJ_MGR_LOG
    Log("UPDT", pObject, pObject->m_pTypeInfo->InstanceCount);
#endif

    ReleaseChain(m_GridChain[pObject->m_ObjectID.Slot]);
    InsertObjIntoGrid(pObject);
}

//==============================================================================

void obj_mgr::AdvanceAllLogic(f32 DeltaTime) {
    s32 i;
    object* pObject;

#ifdef OBJMGR_STATS
    ColliderRayCount = 0;
    ColliderExtCount = 0;
    ColliderRayTime.Reset();
    ColliderExtTime.Reset();
#endif

    // Clear all object "ATTR_LOGIC_APPLIED" attribute moving object (players/bots/vehicles)
    static object::type ClearLogicAppliedList[] = {
        object::TYPE_PLAYER,
        object::TYPE_BOT,
        object::TYPE_VEHICLE_WILDCAT,
        object::TYPE_VEHICLE_BEOWULF,
        object::TYPE_VEHICLE_JERICHO2,
        object::TYPE_VEHICLE_SHRIKE,
        object::TYPE_VEHICLE_THUNDERSWORD,
        object::TYPE_VEHICLE_HAVOC,
    };
    for (i = 0; i < (s32)(sizeof(ClearLogicAppliedList) / sizeof(object::type)); i++) {
        StartTypeLoop(ClearLogicAppliedList[i]);
        while ((pObject = ObjMgr.GetNextInType())) {
            pObject->m_AttrBits &= ~object::ATTR_LOGIC_APPLIED;
        }
        ObjMgr.EndTypeLoop();
    }

    // Loop through all object types.  Skip satchel, though, and run it last.
    for (i = 0; i < object::TYPE_END_OF_LIST; i++) {
        if (i != object::TYPE_SATCHEL_CHARGE) {
            AdvanceLogicForType((object::type)i, DeltaTime);
        }
    }
    AdvanceLogicForType(object::TYPE_SATCHEL_CHARGE, DeltaTime);

    // Add to netinfo stats.
    {
        for (i = 0; i < object::TYPE_END_OF_LIST; i++) {
            if (m_pObjTypeInfo[i]) {
                // Move to next samples
                m_pObjTypeInfo[i]->NetInfo.NextSample();
            }
        }
    }

    if (SHOW_COLLIDER_TIME) {
#ifdef OBJMGR_STATS
        x_printfxy(25, 2, "RayC %2d %1.2f", ColliderRayCount, ColliderRayTime.ReadMs());
        x_printfxy(25, 3, "ExtC %2d %1.2f", ColliderExtCount, ColliderExtTime.ReadMs());
#endif
    }
}

//==============================================================================

xtimer BotTimer;

void obj_mgr::AdvanceLogicForType(object::type Type, f32 DeltaTime) {
    object* pObject;

    ASSERT(IN_RANGE(object::TYPE_START_OF_LIST, Type, object::TYPE_END_OF_LIST));

    // Lookup type info
    obj_type_info* pObjTypeInfo = m_pObjTypeInfo[Type];

    // Process?
    if (pObjTypeInfo) {
        s32 NInstancesProcessed = 0;
        //pObjTypeInfo->LogicTime.Reset();
        //pObjTypeInfo->LogicTime.Start();

        if (Type == object::TYPE_BOT) {
            BotTimer.Start();
            AdvanceAllBotLogic(DeltaTime);
        } else if (pObjTypeInfo->InstanceCount) {
            ASSERT(m_TypeCursorID == -1);
            StartTypeLoop(Type);

#ifdef OBJ_MGR_LOG
            s32 Count = 0;
            Log("Curs", NULL, m_TypeCursor[m_TypeCursorID]);
            Log("Next", NULL, CELL_NEXT(m_TypeCursor[m_TypeCursorID]));
            Log("Prev", NULL, CELL_PREV(m_TypeCursor[m_TypeCursorID]));
#endif

            while ((pObject = GetNextInType())) {
                object::update_code UpdateCode;

#ifdef OBJ_MGR_LOG
                Log("Curs", NULL, m_TypeCursor[m_TypeCursorID]);
                Log("Next", NULL, CELL_NEXT(m_TypeCursor[m_TypeCursorID]));
                Log("Prev", NULL, CELL_PREV(m_TypeCursor[m_TypeCursorID]));
                Log("LOGC", pObject, ++Count);
#endif

                UpdateCode = pObject->OnAdvanceLogic(DeltaTime);
                NInstancesProcessed++;

#ifdef OBJ_MGR_LOG
                Log("Curs", NULL, m_TypeCursor[m_TypeCursorID]);
                Log("Next", NULL, CELL_NEXT(m_TypeCursor[m_TypeCursorID]));
                Log("Prev", NULL, CELL_PREV(m_TypeCursor[m_TypeCursorID]));
#endif

                switch (UpdateCode) {
                    case object::UPDATE:
                        UpdateObject(pObject);
                        break;
                    case object::DESTROY:
                        if ((tgl.ServerPresent)
                            || (pObject->m_ObjectID.Slot >= MAX_SERVER_OBJECTS)) {
                            RemoveObject(pObject);
                            DestroyObject(pObject);
                        }
                        break;
                    default:
                        break;
                }
            }

            EndTypeLoop();
            ASSERT(m_TypeCursorID == -1);
        }

        BotTimer.Stop();
        //pObjTypeInfo->LogicTime.Stop();
        //pObjTypeInfo->AverageLogicTime = (pObjTypeInfo->AverageLogicTime + pObjTypeInfo->LogicTime.ReadMs())*0.5f;

        /*
        // Check that none of the objects took too long.
        if( NInstancesProcessed > 0 )
        {
            f32 AvgMs = pObjTypeInfo->LogicTime.ReadMs() / (f32)NInstancesProcessed;
            if( AvgMs > 30.0f )
            {
                x_DebugMsg("============================================\n");
                x_DebugMsg("Object type <%1s> logic time is too high!!!\n",pObjTypeInfo->pTypeName);
                x_DebugMsg("There were %1d instances.\n", NInstancesProcessed);
                x_DebugMsg("Total time: %1.2fms\n",pObjTypeInfo->LogicTime.ReadMs());
                x_DebugMsg("Average time: %1.2fms\n",AvgMs);
                ASSERT(FALSE);
            }
        }
*/
    }
}

//==============================================================================

void obj_mgr::AddToChain(obj_ref& ChainHead, s32 Slot, obj_ref Cell) {
    obj_ref NewRef = GetNewRef();

    OBJ_SLOT(NewRef) = Slot;
    CELL(NewRef) = Cell;
    CHAIN_NEXT(NewRef) = ChainHead;

    ChainHead = NewRef;
}

//==============================================================================

void obj_mgr::Select(u32 AttrMask, const vector3& Center, f32 Radius) {
    s32 XLo, XHi, ZLo, ZHi, i, j;
    xbool Out = FALSE;

    // FIX: We are testing objects in all grids which are within a square around
    //      the sphere, not the sphere itself.

    m_SelectSeq++;
    m_SelectCursorID++;
    ASSERT(m_SelectCursorID < MAX_SELECTIONS);

    XLo = (s32)x_floor((Center.X - Radius) / 64.0f);
    ZLo = (s32)x_floor((Center.Z - Radius) / 64.0f);
    XHi = (s32)x_floor((Center.X + Radius) / 64.0f);
    ZHi = (s32)x_floor((Center.Z + Radius) / 64.0f);

    if (XLo < 0) {
        XLo = 0;
        Out = TRUE;
    }
    if (ZLo < 0) {
        ZLo = 0;
        Out = TRUE;
    }
    if (XHi > 31) {
        XHi = 31;
        Out = TRUE;
    }
    if (ZHi > 31) {
        ZHi = 31;
        Out = TRUE;
    }

    for (j = ZLo; j <= ZHi; j++) {
        for (i = XLo; i <= XHi; i++) {
            obj_ref GridID = (j << 5) + i;
            SelectCell(GridID, Center, Radius, AttrMask);
        }
    }

    if (Out) {
        SelectCell(OUT_OF_GRID, Center, Radius, AttrMask);
    }

    // Always.
    {
        SelectCell(ALL_OF_GRID, AttrMask);
    }

    m_SelectCursor[m_SelectCursorID] = m_SelectChain[m_SelectCursorID];
}

//==============================================================================

void obj_mgr::Select(u32 AttrMask, const bbox& BBox) {
    s32 XLo, XHi, ZLo, ZHi, i, j;
    xbool Out = FALSE;

    // FIX: We are not testing objects bbox to the selection volume.

    m_SelectSeq++;
    m_SelectCursorID++;
    ASSERT(m_SelectCursorID < MAX_SELECTIONS);

    XLo = (s32)x_floor(BBox.Min.X / 64.0f);
    ZLo = (s32)x_floor(BBox.Min.Z / 64.0f);
    XHi = (s32)x_floor(BBox.Max.X / 64.0f);
    ZHi = (s32)x_floor(BBox.Max.Z / 64.0f);

    if (XLo < 0) {
        XLo = 0;
        Out = TRUE;
    }
    if (ZLo < 0) {
        ZLo = 0;
        Out = TRUE;
    }
    if (XHi > 31) {
        XHi = 31;
        Out = TRUE;
    }
    if (ZHi > 31) {
        ZHi = 31;
        Out = TRUE;
    }

    for (j = ZLo; j <= ZHi; j++) {
        for (i = XLo; i <= XHi; i++) {
            obj_ref GridID = (j << 5) + i;
            SelectCell(GridID, BBox, AttrMask);
        }
    }

    if (Out) {
        SelectCell(OUT_OF_GRID, BBox, AttrMask);
    }

    // Always.
    {
        SelectCell(ALL_OF_GRID, AttrMask);
    }

    m_SelectCursor[m_SelectCursorID] = m_SelectChain[m_SelectCursorID];
}

//==============================================================================

void obj_mgr::Select(u32 AttrMask) {
    s32 i;

    m_SelectSeq++;
    m_SelectCursorID++;
    ASSERT(m_SelectCursorID < MAX_SELECTIONS);

    for (i = 0; i < 1024; i++) {
        SelectCell(i, AttrMask);
    }

    SelectCell(OUT_OF_GRID, AttrMask);
    SelectCell(ALL_OF_GRID, AttrMask);

    m_SelectCursor[m_SelectCursorID] = m_SelectChain[m_SelectCursorID];
}

//==============================================================================

void obj_mgr::SelectOutOfRep0(u32 AttrMask) {
    m_SelectSeq++;
    m_SelectCursorID++;
    ASSERT(m_SelectCursorID < MAX_SELECTIONS);

    SelectCell(OUT_OF_GRID, AttrMask);

    m_SelectCursor[m_SelectCursorID] = m_SelectChain[m_SelectCursorID];
}

//==============================================================================

void obj_mgr::SelectCell(obj_ref Cell, const bbox& BBox, u32 AttrMask) {
    obj_ref Ref = CELL_NEXT(Cell);

    while (Ref != Cell) {
        object* pObj = m_pObject[OBJ_SLOT(Ref)];

        if ((m_pObject[OBJ_SLOT(Ref)]->m_AttrBits & AttrMask)
            && (m_LastSelect[OBJ_SLOT(Ref)] != m_SelectSeq)) {
            m_LastSelect[OBJ_SLOT(Ref)] = m_SelectSeq;

            if (pObj->m_WorldBBox.Intersect(BBox)) {
                AddToChain(m_SelectChain[m_SelectCursorID], pObj->m_ObjectID.Slot, Ref);
            }
        }

        Ref = CELL_NEXT(Ref);
    }
}

//==============================================================================

void obj_mgr::SelectCell(obj_ref Cell, const vector3& Center, f32 Radius, u32 AttrMask) {
    obj_ref Ref = CELL_NEXT(Cell);

    while (Ref != Cell) {
        object* pObj = m_pObject[OBJ_SLOT(Ref)];

        if ((m_pObject[OBJ_SLOT(Ref)]->m_AttrBits & AttrMask)
            && (m_LastSelect[OBJ_SLOT(Ref)] != m_SelectSeq)) {
            m_LastSelect[OBJ_SLOT(Ref)] = m_SelectSeq;

            f32 RadiusSquared = SQR(Radius);
            RadiusSquared += pObj->m_WorldBBox.GetRadiusSquared();

            vector3 Span = Center;
            Span -= pObj->m_WorldBBox.GetCenter();

            if (Span.LengthSquared() < RadiusSquared) {
                AddToChain(m_SelectChain[m_SelectCursorID], pObj->m_ObjectID.Slot, Ref);
            }
        }

        Ref = CELL_NEXT(Ref);
    }
}

//==============================================================================

void obj_mgr::SelectCell(obj_ref Cell, u32 AttrMask) {
    obj_ref Ref = CELL_NEXT(Cell);

    while (Ref != Cell) {
        object* pObj = m_pObject[OBJ_SLOT(Ref)];

        if ((m_pObject[OBJ_SLOT(Ref)]->m_AttrBits & AttrMask)
            && (m_LastSelect[OBJ_SLOT(Ref)] != m_SelectSeq)) {
            m_LastSelect[OBJ_SLOT(Ref)] = m_SelectSeq;

            AddToChain(m_SelectChain[m_SelectCursorID], pObj->m_ObjectID.Slot, Ref);
        }

        Ref = CELL_NEXT(Ref);
    }
}

//==============================================================================

void obj_mgr::ClearSelection(void) {
    ASSERT(m_SelectCursorID >= 0);
    ReleaseChain(m_SelectChain[m_SelectCursorID]);
    m_SelectCursorID--;
}

//==============================================================================

void obj_mgr::RestartSelection(void) {
    m_SelectCursor[m_SelectCursorID] = m_SelectChain[m_SelectCursorID];
}

//==============================================================================

object* obj_mgr::GetNextSelected(void) {
    object* pObject = NULL;

    if (m_SelectCursor[m_SelectCursorID] != -1) {
        ASSERT(OBJ_SLOT(m_SelectCursor[m_SelectCursorID]) != -1);
        pObject = m_pObject[OBJ_SLOT(m_SelectCursor[m_SelectCursorID])];
        m_SelectCursor[m_SelectCursorID] = CHAIN_NEXT(m_SelectCursor[m_SelectCursorID]);
    }

    return (pObject);
}

//==============================================================================

void obj_mgr::UnbindOriginID(object::id OriginID) {
    // Need to unbind the OriginID for all applicable objects.
    for (s32 i = 0; i < MAX_SERVER_OBJECTS; i++) {
        if (m_pObject[i]) {
            m_pObject[i]->UnbindOriginID(OriginID);
        }
    }
}

//==============================================================================

void obj_mgr::UnbindTargetID(object::id TargetID) {
    // Need to unbind the TargetID for all applicable objects.
    for (s32 i = 0; i < MAX_SERVER_OBJECTS; i++) {
        if (m_pObject[i]) {
            m_pObject[i]->UnbindTargetID(TargetID);
        }
    }
}

//==============================================================================
xtimer ObjMgrCollideTimer[2];

void obj_mgr::Collide(u32 AttrMask, collider& Collider) {
    // Start timers
    if (Collider.GetType() == collider::RAY) {
        ObjMgrCollideTimer[0].Start();
    }
    if (Collider.GetType() == collider::EXTRUSION) {
        ObjMgrCollideTimer[1].Start();
    }

    // Use the SelectSeq to prevent double collision testing.
    m_SelectSeq++;

    // First, collide with "everywhere" things.
    CollideWithCell(ALL_OF_GRID, AttrMask, Collider);

    // Early out?
    if (Collider.GetEarlyOut()) {
        return;
    }

    // Now, deal with each type of collider.
    if (Collider.GetType() == collider::RAY) {
#ifdef OBJMGR_STATS
        ColliderRayCount++;
        ColliderRayTime.Start();
#endif
        CollideRay(AttrMask, Collider);
#ifdef OBJMGR_STATS
        ColliderRayTime.Stop();
#endif
    } else if (Collider.GetType() == collider::EXTRUSION) {
#ifdef OBJMGR_STATS
        ColliderExtCount++;
        ColliderExtTime.Start();
#endif
        CollideExt(AttrMask, Collider);
#ifdef OBJMGR_STATS
        ColliderExtTime.Stop();
#endif
    }

    // End timers
    if (Collider.GetType() == collider::RAY) {
        ObjMgrCollideTimer[0].Stop();
    }
    if (Collider.GetType() == collider::EXTRUSION) {
        ObjMgrCollideTimer[1].Stop();
    }
}

//==============================================================================

void obj_mgr::CollideRay(u32 AttrMask, collider& Collider) {
    f32 t0, t1;
    vector3 P0(Collider.RayGetStart());
    vector3 P1(Collider.RayGetEnd());

    //
    // The vast majority of rays are going to be "short" and "in the grid".
    // That is, the entire ray will be within a single grid cell and NOT out of
    // the grid.  So, expend some effort to process this case fast.
    //
    {
        s32 i0, i1;
        s32 j0, j1;

        // These are the same as (s32)( x_floor( Q / 64.0f ) ).
        i0 = (s32)((P0.X / 64.0f) + 16384) - 16384;
        j0 = (s32)((P0.Z / 64.0f) + 16384) - 16384;
        i1 = (s32)((P1.X / 64.0f) + 16384) - 16384;
        j1 = (s32)((P1.Z / 64.0f) + 16384) - 16384;

        if ((i0 == i1) && (j0 == j1)) {
            s32 GridID;

            // Its a short ray!  See if it is out of the grid.
            if ((i0 < 0) || (j0 < 0) || (i0 > 31) || (j0 > 31)) {
                GridID = OUT_OF_GRID;
            } else {
                GridID = (j0 << 5) + i0;
            }

            // Process the grid cell.
            CollideWithCell(GridID, AttrMask, Collider);
            return;
        }
    }

    //
    // Well, its not a short ray.  So, we are going to have to walk this ray
    // through the grid and process cells which are touched.
    //

    t0 = 0.0f;
    t1 = 1.0f;

    //
    // We need to clip the ray, or rather, its t values, to the grid bounds.
    //
    {
        xbool Out = FALSE;
        xbool Reject = FALSE;
        f32 t;
        u8 P0Mask = 0;
        u8 P1Mask = 0;

        if (P0.X < 0.0f) {
            P0Mask |= 0x01;
        }
        if (P0.Z < 0.0f) {
            P0Mask |= 0x02;
        }
        if (P0.X > 2048.0f) {
            P0Mask |= 0x04;
        }
        if (P0.Z > 2048.0f) {
            P0Mask |= 0x08;
        }

        if (P1.X < 0.0f) {
            P1Mask |= 0x01;
        }
        if (P1.Z < 0.0f) {
            P1Mask |= 0x02;
        }
        if (P1.X > 2048.0f) {
            P1Mask |= 0x04;
        }
        if (P1.Z > 2048.0f) {
            P1Mask |= 0x08;
        }

        // Do we need to clip?
        if (P0Mask & P1Mask) {
            // The line segment is completely out of the grid.  Reject it, but
            // signal that we need to process the OUT_OF_GRID cell.
            Reject = TRUE;
            Out = TRUE;
        } else if (P0Mask | P1Mask) {
            // The line segment is partially out of the grid.  Clip it, and
            // signal that we need to process the OUT_OF_GRID cell.
            Out = TRUE;

            // Clip entry on X = 0.
            if ((P0Mask & 0x01) && !(P1Mask & 0x01)) {
                t = (0.0f - P0.X) / (P1.X - P0.X);
                if (t > t0) {
                    t0 = t;
                }
            }

            // Clip exit on X = 0.
            if (!(P0Mask & 0x01) && (P1Mask & 0x01)) {
                t = (0.0f - P0.X) / (P1.X - P0.X);
                if (t < t1) {
                    t1 = t;
                }
            }

            // Clip entry on Z = 0.
            if ((P0Mask & 0x02) && !(P1Mask & 0x02)) {
                t = (0.0f - P0.Z) / (P1.Z - P0.Z);
                if (t > t0) {
                    t0 = t;
                }
            }

            // Clip exit on Z = 0.
            if (!(P0Mask & 0x02) && (P1Mask & 0x02)) {
                t = (0.0f - P0.Z) / (P1.Z - P0.Z);
                if (t < t1) {
                    t1 = t;
                }
            }

            // Clip entry on X = 2048.
            if ((P0Mask & 0x04) && !(P1Mask & 0x04)) {
                t = (2048.0f - P0.X) / (P1.X - P0.X);
                if (t > t0) {
                    t0 = t;
                }
            }

            // Clip exit on X = 2048.
            if (!(P0Mask & 0x04) && (P1Mask & 0x04)) {
                t = (2048.0f - P0.X) / (P1.X - P0.X);
                if (t < t1) {
                    t1 = t;
                }
            }

            // Clip entry on Z = 2048.
            if ((P0Mask & 0x08) && !(P1Mask & 0x08)) {
                t = (2048.0f - P0.Z) / (P1.Z - P0.Z);
                if (t > t0) {
                    t0 = t;
                }
            }

            // Clip exit on Z = 2048.
            if (!(P0Mask & 0x08) && (P1Mask & 0x08)) {
                t = (2048.0f - P0.Z) / (P1.Z - P0.Z);
                if (t < t1) {
                    t1 = t;
                }
            }
        }

        // Did we clip the whole line segment away?
        if (t0 > t1) {
            Reject = TRUE;
        }

        // Was any portion of the segment out of the grid?
        if (Out) {
            CollideWithCell(OUT_OF_GRID, AttrMask, Collider);

            // Early out?
            if (Collider.GetEarlyOut()) {
                return;
            }
        }

        // If all of the segment was out, then we are done.
        if (Reject) {
            return;
        }
    }

    //
    // We now have parametric values t0 and t1 which are clipped to the grid.
    // Walk t0 towards t1 until we get to the cell containing the end point.
    // Also, bail out if we hit at some t value less than our stepping t0.
    //
    {
        s32 GridX0, GridZ0;
        s32 GridX1, GridZ1;
        s32 StepGX, StepGZ;
        f32 StepTX, StepTZ;
        f32 NextTX, NextTZ;
        s32 GridCell, StopCell;

        vector3 Delta = P1 - P0;
        vector3 Clip0;
        vector3 Clip1;

        // Clip P0 and P1 into Clip0 and Clip1.
        if ((t0 > 0.0f) || (t1 < 1.0f)) {
            Clip0 = P0 + Delta * t0;
            Clip1 = P0 + Delta * t1;
        } else {
            Clip0 = P0;
            Clip1 = P1;
        }

        // Determine starting and ending grid sections.

        GridX0 = (s32)x_floor(Clip0.X / 64.0f);
        GridZ0 = (s32)x_floor(Clip0.Z / 64.0f);
        GridX1 = (s32)x_floor(Clip1.X / 64.0f);
        GridZ1 = (s32)x_floor(Clip1.Z / 64.0f);

        GridX0 = MINMAX(0, GridX0, 31);
        GridZ0 = MINMAX(0, GridZ0, 31);
        GridX1 = MINMAX(0, GridX1, 31);
        GridZ1 = MINMAX(0, GridZ1, 31);

        if (GridX0 != GridX1) {
            StepTX = x_abs(64.0f / Delta.X);
            if (Delta.X > 0.0f) {
                NextTX = (((GridX0 + 1) * 64.0f) - P0.X) / (Delta.X);
                StepGX = 1;
            } else {
                NextTX = (((GridX0) * 64.0f) - P0.X) / (Delta.X);
                StepGX = -1;
            }
        } else {
            NextTX = 2.0f;
            StepTX = 0.0f;
            StepGX = 0;
        }

        if (GridZ0 != GridZ1) {
            StepTZ = x_abs(64.0f / Delta.Z);
            if (Delta.Z > 0.0f) {
                NextTZ = (((GridZ0 + 1) * 64.0f) - P0.Z) / (Delta.Z);
                StepGZ = 32;
            } else {
                NextTZ = (((GridZ0) * 64.0f) - P0.Z) / (Delta.Z);
                StepGZ = -32;
            }
        } else {
            NextTZ = 2.0f;
            StepTZ = 0.0f;
            StepGZ = 0;
        }

        GridCell = (GridZ0 << 5) + GridX0;
        StopCell = (GridZ1 << 5) + GridX1;

        // Here's the big show.  Remember, we are walking t0 towards t1 until we
        // get to the stop cell, or we have a collision at some t value less
        // than t0.

        while (t0 < Collider.GetCollisionT()) {
            // Process the current GridCell
            CollideWithCell(GridCell, AttrMask, Collider);

            // Early out?
            if (Collider.GetEarlyOut()) {
                return;
            }

            // If we are in the final cell, bail out.
            if (GridCell == StopCell) {
                break;
            }

            // Now take a step.
            if (NextTX < NextTZ) {
                // Step in X.
                t0 = NextTX;
                NextTX += StepTX;
                GridCell += StepGX;
            } else {
                // Step in Z.
                t0 = NextTZ;
                NextTZ += StepTZ;
                GridCell += StepGZ;
            }
        }
    }
}

//==============================================================================

void obj_mgr::CollideExt(u32 AttrMask, collider& Collider) {
    // Extrusion colliders are always relatively short.  (At least, they are
    // supposed to be!)  So, its pretty safe to just consider the entire
    // extrusion volume like a box and process touched grid cells.

    s32 XLo, XHi, ZLo, ZHi, i, j;
    xbool Out = FALSE;
    bbox BBox = Collider.GetMovementBBox();

    XLo = (s32)x_floor(BBox.Min.X / 64.0f);
    ZLo = (s32)x_floor(BBox.Min.Z / 64.0f);
    XHi = (s32)x_floor(BBox.Max.X / 64.0f);
    ZHi = (s32)x_floor(BBox.Max.Z / 64.0f);

    if (XLo < 0) {
        XLo = 0;
        Out = TRUE;
    }
    if (ZLo < 0) {
        ZLo = 0;
        Out = TRUE;
    }
    if (XHi > 31) {
        XHi = 31;
        Out = TRUE;
    }
    if (ZHi > 31) {
        ZHi = 31;
        Out = TRUE;
    }

    for (j = ZLo; j <= ZHi; j++) {
        for (i = XLo; i <= XHi; i++) {
            obj_ref GridID = (j << 5) + i;
            CollideWithCell(GridID, AttrMask, Collider);

            // Early out?
            if (Collider.GetEarlyOut()) {
                return;
            }
        }
    }

    if (Out) {
        CollideWithCell(OUT_OF_GRID, AttrMask, Collider);
    }
}

//==============================================================================

struct record {
    u16 Ref;
    u16 Slot;
    object* pObject;
    s32 Progress;
    s32 Sequence;
};

static record Record[8];
static s32 RIndex;

void DumpRecord(s32 Cell) {
    x_DebugLog(
        "Problem in obj_mgr::CollideWithCell detected.\n"
        "Total objects processed in cell %d thus far is %d.\n"
        "Records to follow.\n",
        Cell,
        RIndex
    );

    for (s32 i = 0; i < MIN(RIndex, 8); i++) {
        x_DebugLog(
            "Record[%d]\n"
            "Ref      = %d\n"
            "Slot     = %d\n"
            "pObject  = 0x%08X\n"
            "ObjType  = %s\n"
            "Progress = %d\n"
            "Sequece  = %d\n",
            i,
            Record[i].Ref,
            Record[i].Slot,
            Record[i].pObject,
            Record[i].pObject ? Record[i].pObject->GetTypeName() : "NULL",
            Record[i].Progress,
            Record[i].Sequence
        );
    }
}

//==============================================================================

void obj_mgr::CollideWithCell(obj_ref Cell, u32 AttrMask, collider& Collider) {
    obj_ref Ref = CELL_NEXT(Cell);
    object::id Exclude[32];
    s32 i, j;

    /**/ volatile s32 MajikNumber = 0x4AFB0001;

    j = Collider.GetExcludedCount();
    ASSERT(j < 32);
    for (i = 0; i < j; i++) {
        Exclude[i].SetAsS32(Collider.GetExcluded(i));
    }

    RIndex = 0;

    while (Ref != Cell) {
        /**/ Record[RIndex & 7].Progress = 0;
        /**/ Record[RIndex & 7].Ref = Ref;
        /**/ Record[RIndex & 7].Sequence = RIndex;
        /**/ if (MajikNumber != 0x4AFB0001) {
            DumpRecord(Cell);
            BREAK;
            ASSERT(FALSE);
        }

        // Lookup object slot at current reference
        s32 ObjectSlot = OBJ_SLOT(Ref);

        /**/ Record[RIndex & 7].Slot = ObjectSlot;

        // See if the Ref'd object:
        //  (a) is NOT a "client" object, and
        if (ObjectSlot < MAX_SERVER_OBJECTS) {
            /**/ if (ObjectSlot < 0) {
                DumpRecord(Cell);
                BREAK;
                ASSERT(FALSE);
            }

            // Lookup object
            object* pObject = m_pObject[ObjectSlot];

            /**/ if (!pObject) {
                DumpRecord(Cell);
                BREAK;
                ASSERT(FALSE);
            }

            /**/ Record[RIndex & 7].pObject = pObject;
            /**/ Record[RIndex & 7].Progress++;
            /**/ if (MajikNumber != 0x4AFB0001) {
                DumpRecord(Cell);
                BREAK;
                ASSERT(FALSE);
            }

            // See if the Ref'd object:
            //  (b) matches the attribute mask,
            //  (c) hasn't already been processed for this operation, and
            //  (d) is NOT the moving object.
            if ((pObject->m_AttrBits & AttrMask) && (m_LastSelect[ObjectSlot] != m_SelectSeq)
                && (pObject != (object*)Collider.GetMovingObject())) {
                /**/ Record[RIndex & 7].Progress++;
                /**/ if (MajikNumber != 0x4AFB0001) {
                    DumpRecord(Cell);
                    BREAK;
                    ASSERT(FALSE);
                }

                // See if it is in the exclude list.
                for (i = 0; i < j; i++) {
                    if (Exclude[i].Slot == ObjectSlot) {
                        break;
                    }
                }

                // Did we get to the end of the list without bailing?  If so, the
                // the object was not in the exclude list, so process it.
                if (i == j) {
                    /**/ Record[RIndex & 7].Progress++;
                    /**/ if (MajikNumber != 0x4AFB0001) {
                        DumpRecord(Cell);
                        BREAK;
                        ASSERT(FALSE);
                    }

                    m_LastSelect[ObjectSlot] = m_SelectSeq;
                    Collider.SetContext(pObject);
                    if ((pObject->m_AttrBits & object::ATTR_GLOBAL)
                        || (!Collider.TrivialRejectBBox(pObject->m_WorldBBox))) {
                        /**/ Record[RIndex & 7].Progress++;
                        /**/ if (MajikNumber != 0x4AFB0001) {
                            DumpRecord(Cell);
                            BREAK;
                            ASSERT(FALSE);
                        }

                        // Do collision
                        pObject->OnCollide(AttrMask, Collider);

                        /**/ Record[RIndex & 7].Progress++;
                        /**/ if (MajikNumber != 0x4AFB0001) {
                            DumpRecord(Cell);
                            BREAK;
                            ASSERT(FALSE);
                        }

                        // Early out?
                        if (Collider.GetEarlyOut()) {
                            return;
                        }
                    }
                }
            }
        }

        /**/ RIndex++;
        /**/ if (MajikNumber != 0x4AFB0001) {
            DumpRecord(Cell);
            BREAK;
            ASSERT(FALSE);
        }

        // On to the next ref.
        Ref = CELL_NEXT(Ref);

        /**/ if (MajikNumber != 0x4AFB0001) {
            DumpRecord(Cell);
            BREAK;
            ASSERT(FALSE);
        }
    }
}

//==============================================================================

void obj_mgr::InflictPain(
    pain::type PainType,
    const vector3& Position,
    object::id OriginID,
    object::id VictimID,
    f32 PainScalar
) {
    // Only apply pain on the server side.
    if (!tgl.ServerPresent) {
        return;
    }

    object* pObject;
    object::id ObjectID;

    ASSERT(IN_RANGE(0, PainType, pain::PAIN_END_OF_LIST));

    // Dish out the pain.

    if ((g_PainBase[PainType].DamageR0 > 0.0f) || (g_PainBase[PainType].ForceR0 > 0.0f)) {
        // We are doing a sphere of pain.

        f32 Range = MAX(g_PainBase[PainType].DamageR0, g_PainBase[PainType].ForceR0);

        Select(object::ATTR_DAMAGEABLE, Position, Range);

        while ((pObject = GetNextSelected())) {
            ObjectID = pObject->GetObjectID();
            InflictPain(
                PainType,
                pObject,
                Position,
                OriginID,
                ObjectID,
                (ObjectID == VictimID),
                PainScalar
            );
        }

        ClearSelection();
    } else {
        // Apply pain directly to the victim.

        pObject = GetObjectFromID(VictimID);
        if (pObject && (pObject->GetAttrBits() & object::ATTR_DAMAGEABLE)) {
            InflictPain(PainType, pObject, Position, OriginID, VictimID, TRUE, PainScalar);
        }
    }
}

//==============================================================================

void obj_mgr::InflictPain(
    pain::type PainType,
    object* pObject,
    const vector3& Position,
    object::id OriginID,
    object::id VictimID,
    xbool DirectHit,
    f32 PainScalar
) {
    // Only apply pain on the server side.
    if (!tgl.ServerPresent) {
        return;
    }

    ASSERT(IN_RANGE(0, PainType, pain::PAIN_END_OF_LIST));

    pain Pain;
    Pain = g_PainBase[PainType];
    ASSERT(Pain.PainType == PainType);

    // Start with maximum values for damage and force.
    f32 DamageScalar = PainScalar;
    f32 ForceScalar = PainScalar;

    // Determine the distance to the pain.

    if (DirectHit) {
        Pain.Distance = 0.0f;
        Pain.LineOfSight = TRUE;
    } else {
        f32 ObjRadius = pObject->GetPainRadius();
        vector3 ObjPosition = pObject->GetPainPoint();

        f32 Safe = MAX(Pain.DamageR0, Pain.ForceR0);
        Pain.Distance = MAX(0, (ObjPosition - Position).Length() - ObjRadius);

        if (Pain.Distance >= Safe) {
            // Not a direct hit, and object is too far away.  No pain.
            return;
        }

        // Check for "really close" or "direct hit".
        if ((Pain.Distance < 0.01f) || (DirectHit)) {
            Pain.LineOfSight = TRUE;
        } else {
            // Not too far, and not too close.
            // Perform a line of sight test.
            {
                collider Ray;
                vector3 Vector;
                Vector = ObjPosition - Position;
                Vector.NormalizeAndScale(0.05f);
                vector3 Start = Position - Vector;
                Ray.RaySetup(NULL, Start, ObjPosition);
                ObjMgr.Collide(object::ATTR_SOLID_STATIC | object::ATTR_FORCE_FIELD, Ray);
                Pain.LineOfSight = !Ray.HasCollided();
            }

            // Scale the damage based on distance.
            if (Pain.Distance > Pain.DamageR1) {
                if (Pain.Distance > Pain.DamageR0) {
                    DamageScalar = 0.0f;
                } else {
                    DamageScalar *=
                        ((Pain.DamageR0 - Pain.Distance) / (Pain.DamageR0 - Pain.DamageR1));
                }
            }

            // Scale the force based on distance.
            if (Pain.Distance > Pain.ForceR1) {
                if (Pain.Distance > Pain.ForceR0) {
                    ForceScalar = 0.0f;
                } else {
                    ForceScalar *= ((Pain.ForceR0 - Pain.Distance) / (Pain.ForceR0 - Pain.ForceR1));
                }
            }
        }
    }

    // The damage scalar now accounts for the overall pain scalar and the
    // distance attenuation.  Determine a team damage scalar.

    f32 TeamDamageScalar = 1.0f;
    object* pOrigin = GetObjectFromID(OriginID);

    // Set up some values while we're in the area.
    Pain.Center = Position;
    Pain.OriginID = OriginID;
    Pain.VictimID = VictimID;
    Pain.TeamBits = 0x00;
    Pain.DirectHit = DirectHit;

    if (pOrigin) {
        Pain.TeamBits = pOrigin->GetTeamBits();
    }

    // For players/bots, we have to tweak the ObjType a little.

    object::type ObjType = GetTypeForPain(pObject);
    ASSERT(IN_RANGE(object::TYPE_BEGIN_DAMAGEABLE, ObjType, object::TYPE_END_DAMAGEABLE));

    // If the source of the pain is a player (direct or indirect), AND
    // the recipient of the pain is not the same player, AND the recipient
    // is on the same team as the source of the pain, AND it is not "healing
    // pain", then consider the team damage setting.

    if ((Pain.OriginID.Slot < 32) &&                         // From a player
        (Pain.OriginID != pObject->m_ObjectID) &&            // Not self inflicted
        (Pain.TeamBits & pObject->m_TeamBits) &&             // Same team
        (g_MetalDamageTable[Pain.PainType][ObjType] > 0.0f)) // Not healing
    {
        // Pain to another player on the same team?
        if (pObject->m_AttrBits & object::ATTR_PLAYER) {
            // Use team damage.
            TeamDamageScalar = GameMgr.GetTeamDamage();
        }

        // Pain to an asset or vehicle?
        if ((pObject->m_AttrBits & object::ATTR_ASSET)
            || (pObject->m_AttrBits & object::ATTR_VEHICLE)) {
            // Use team damage, but no lower than 50%.
            TeamDamageScalar = MAX(GameMgr.GetTeamDamage(), 0.5f);
        }
    }

    // Decide how much damage goes to the energy/shield.

    f32 EnergyScalar;
    f32 MetalScalar;
    f32 ObjEnergy = pObject->GetEnergy();
    xbool ObjShield = pObject->IsShielded();

    if (!ObjShield && !Pain.SapEnergy) {
        // No shield, and this pain does not sap energy direct.  Easy.
        MetalScalar = DamageScalar;
        Pain.EnergyDamage = 0.0f;
    } else {
        // Use shield leak table to route some damage to energy/shields and
        // some to the metal.
        EnergyScalar = DamageScalar * (1.0f - g_ShieldLeakTable[Pain.PainType][ObjType]);
        MetalScalar = DamageScalar * (g_ShieldLeakTable[Pain.PainType][ObjType]);

        Pain.EnergyDamage = EnergyScalar * g_ShieldDamageTable[Pain.PainType][ObjType];

        if (ObjEnergy < Pain.EnergyDamage) {
            // Damage overwhelmed the shields.  Damage must spill to the metal.
            if (Pain.EnergyDamage > 0.001f) {
                f32 SpillRatio = 1.0f - (ObjEnergy / Pain.EnergyDamage);
                MetalScalar += SpillRatio * EnergyScalar;
            } else {
                MetalScalar = DamageScalar;
            }

            Pain.EnergyDamage = ObjEnergy;
        }
    }

    // At last, the final computation for metal damage!
    Pain.MetalDamage = MetalScalar * TeamDamageScalar * g_MetalDamageTable[Pain.PainType][ObjType];

    // Ah, but no more than 100% damage.
    Pain.MetalDamage = MIN(Pain.MetalDamage, 1.0f);

    // Use the force.
    Pain.Force = Pain.MaxForce * ForceScalar;

    // Hurt me, baby!
    pObject->OnPain(Pain);
}

//==============================================================================

// NOTE: the order of these tables depend on the order of the ObjectTypes[] table

//                       Light   Medium  Heavy   GrCycl  Shrike  Bomber  XPort   Dummy
f32 PainMinSpeed[8] = {35.0f, 35.0f, 35.0f, 50.0f, 50.0f, 10.0f, 10.0f, 99.0f};
f32 PainMaxSpeed[8] = {75.0f, 65.0f, 60.0f, 100.0f, 100.0f, 40.0f, 30.0f, 999.0f};

f32 VehicleMinSpeed = 30.0f;
f32 GroundScalar = 0.15f;

pain::type HitPainTypes[] = {
    pain::HIT_BY_ARMOR_LIGHT,
    pain::HIT_BY_ARMOR_MEDIUM,
    pain::HIT_BY_ARMOR_HEAVY,
    pain::HIT_BY_GRAV_CYCLE,
    pain::HIT_BY_SHRIKE,
    pain::HIT_BY_BOMBER,
    pain::HIT_BY_TRANSPORT,
    pain::HIT_IMMOVEABLE, // this MUST be at end of list for GetIndex() to work
};

object::type ObjectTypes[] = {
    object::TYPE_PLAYER,         // Light armor
    object::TYPE_BOT,            // Medium armor
    object::TYPE_END_DAMAGEABLE, // Heavy armor
    object::TYPE_VEHICLE_WILDCAT,
    object::TYPE_VEHICLE_SHRIKE,
    object::TYPE_VEHICLE_THUNDERSWORD,
    object::TYPE_VEHICLE_HAVOC,
};

//==============================================================================

static s32 GetIndex(object::type Type) {
    s32 NumEntries = sizeof(ObjectTypes) / sizeof(ObjectTypes[0]);
    s32 Index = -1;

    for (s32 i = 0; i < NumEntries; i++) {
        if (ObjectTypes[i] == Type) {
            Index = i;
            break;
        }
    }

    // If object type was not found in list then it must be an immovable object.
    if (Index == -1) {
        Index = NumEntries;
    }

    return (Index);
}

//==============================================================================

static void ApplyPain(object* pVictim, object* pOrigin, f32 DeltaSpeed) {
    if (!(pVictim->GetAttrBits() & object::ATTR_DAMAGEABLE)) {
        return;
    }

    object::id OriginID = pOrigin->GetObjectID();
    s32 Victim = GetIndex(GetTypeForPain(pVictim));
    s32 Origin = GetIndex(GetTypeForPain(pOrigin));
    pain::type PainType = HitPainTypes[Origin];

    f32 MinSpeed, MaxSpeed;

    if (HitPainTypes[Victim] == pain::HIT_IMMOVEABLE) {
        MinSpeed = PainMinSpeed[Origin];
        MaxSpeed = PainMaxSpeed[Origin];
    } else {
        MinSpeed = PainMinSpeed[Victim];
        MaxSpeed = PainMaxSpeed[Victim];
    }

    if (pOrigin->GetAttrBits() & object::ATTR_VEHICLE) {
        vehicle* pVehicle = (vehicle*)pOrigin;
        player_object* pPlayer = pVehicle->GetSeatPlayer(0);

        if (pPlayer) {
            OriginID = pPlayer->GetObjectID();
        }

        // Special case: if something hits a vehicle and the vehicle is moving slowly, then treat it as solid static
        if (pOrigin->GetVelocity().Length() < VehicleMinSpeed) {
            PainType = pain::HIT_IMMOVEABLE;
            DeltaSpeed = pVictim->GetVelocity().Length();
        }
    }

    f32 PainScalar = (DeltaSpeed - MinSpeed) / (MaxSpeed - MinSpeed);
    PainScalar = MINMAX(0.0f, PainScalar, 1.0f);

    ObjMgr.InflictPain(
        PainType,
        pVictim,
        pVictim->GetPosition(),
        OriginID,
        pVictim->GetObjectID(),
        TRUE,
        PainScalar
    );

    if (g_TweakGame) {
        u8 C1 = 'S';
        u8 C2 = 'S';

        if (pOrigin->GetAttrBits() & object::ATTR_PLAYER) {
            C1 = 'P';
        }
        if (pVictim->GetAttrBits() & object::ATTR_PLAYER) {
            C2 = 'P';
        }
        if (pOrigin->GetAttrBits() & object::ATTR_VEHICLE) {
            C1 = 'V';
        }
        if (pVictim->GetAttrBits() & object::ATTR_VEHICLE) {
            C2 = 'V';
        }

        x_DebugMsg(
            "%c -> %c : Min = %3.0f : Max = %3.0f : Speed = %3.2f : PainScalar = %1.2f\n",
            C1,
            C2,
            MinSpeed,
            MaxSpeed,
            DeltaSpeed,
            PainScalar
        );

        x_printf("%c -> %c : Speed=%1.2f PainScalar=%1.2f\n", C1, C2, DeltaSpeed, PainScalar);
    }
}

//==============================================================================

f32 ImpactScalars[4][2] = // Coefficient of Restitution scalars
    {
        {0.1f, 0.1f}, // Player  -> Player
        {0.1f, 0.1f}, // Player  -> Vehicle
        {0.1f, 0.7f}, // Vehicle -> Player
        {0.7f, 0.7f}, // Vehicle -> Vehicle
};

void GetImpactScalars(const object* pObj1, const object* pObj2, f32& Scale1, f32& Scale2) {
    s32 Index = 0;

    if (pObj1->GetAttrBits() & object::ATTR_VEHICLE) {
        Index |= 0x02;
    }
    if (pObj2->GetAttrBits() & object::ATTR_VEHICLE) {
        Index |= 0x01;
    }

    Scale1 = ImpactScalars[Index][0];
    Scale2 = ImpactScalars[Index][1];

    // Special case: if player is ejecting then dont bounce him as much!
    if (pObj2->GetAttrBits() & object::ATTR_PLAYER) {
        player_object* pPlayer = (player_object*)pObj2;

        if (pPlayer->GetVehicleDismountTime() > 0.0f) {
            Scale2 = 0.1f;
        }
    }
}

//==============================================================================

void obj_mgr::ProcessImpact(object* pObj1, const collider::collision& Collision) {
    object* pObj2 = (object*)Collision.pObjectHit;

    ASSERT(pObj1);
    ASSERT(pObj2);

    vector3 Axis;

    f32 Mass1, InVel1, OutVel1;
    f32 Mass2, InVel2, OutVel2;
    vector3 Pos1, OldVel1, NewVel1, ParaVel1, PerpVel1, DeltaVel1;
    vector3 Pos2, OldVel2, NewVel2, ParaVel2, PerpVel2, DeltaVel2;

    // Is the "still" object a player or a vehicle?
    if (pObj2->GetAttrBits() & (object::ATTR_VEHICLE | object::ATTR_PLAYER)) {
        //
        // The moving object hit a moveable object.
        //

        Mass1 = pObj1->GetMass();
        Mass2 = pObj2->GetMass();

        Pos1 = pObj1->GetPosition();
        Pos2 = pObj2->GetPosition();

        OldVel1 = pObj1->GetVelocity();
        OldVel2 = pObj2->GetVelocity();

        Axis = (Pos2 - Pos1);
        Axis.Normalize();

        ParaVel1 = Axis * Axis.Dot(OldVel1);
        ParaVel2 = Axis * Axis.Dot(OldVel2);

        PerpVel1 = OldVel1 - ParaVel1;
        PerpVel2 = OldVel2 - ParaVel2;

        InVel1 = ParaVel1.Length();
        InVel2 = ParaVel2.Length();

        OutVel1 = (((Mass1 - Mass2) / (Mass1 + Mass2)) * InVel1)
                  + (((Mass2 + Mass2) / (Mass1 + Mass2)) * InVel2);

        OutVel2 = (((Mass1 + Mass1) / (Mass1 + Mass2)) * InVel1)
                  + (((Mass2 - Mass1) / (Mass1 + Mass2)) * InVel2);

        NewVel1 = PerpVel1 + (OutVel1 * Axis);
        NewVel2 = PerpVel2 + (OutVel2 * Axis);

        DeltaVel1 = NewVel1 - OldVel1;
        DeltaVel2 = NewVel2 - OldVel2;

        f32 DeltaSpeed1 = DeltaVel1.Length();
        f32 DeltaSpeed2 = DeltaVel2.Length();

        f32 MinImpactVel1 = PainMinSpeed[GetIndex(GetTypeForPain(pObj1))];
        f32 MinImpactVel2 = PainMinSpeed[GetIndex(GetTypeForPain(pObj2))];

        if (DeltaSpeed1 > MinImpactVel1) {
            ApplyPain(pObj1, pObj2, DeltaSpeed1);
        }
        if (DeltaSpeed2 > MinImpactVel2) {
            ApplyPain(pObj2, pObj1, DeltaSpeed2);
        }

        f32 Scale1, Scale2;

        GetImpactScalars(pObj1, pObj2, Scale1, Scale2);

        DeltaVel1 *= Scale1;
        DeltaVel2 *= Scale2;

        pObj1->OnAddVelocity(DeltaVel1);

        xbool ApplyVelocity = TRUE;

        // Dont allow a player to move a settled vehicle
        if ((pObj1->GetAttrBits() & object::ATTR_PLAYER)
            && (pObj2->GetAttrBits() & object::ATTR_VEHICLE)) {
            gnd_effect* pVehicle = (gnd_effect*)pObj2;

            if (pVehicle->IsSettled() == TRUE) {
                ApplyVelocity = FALSE;
            }
        }

        if (ApplyVelocity == TRUE) {
            pObj2->OnAddVelocity(DeltaVel2);
        }
    } else {
        //
        // The moving object hit an immoveable object.
        //

        OldVel1 = pObj1->GetVelocity();
        Collision.Plane.GetComponents(OldVel1, ParaVel1, PerpVel1);

        NewVel1 = (PerpVel1 * -GroundScalar) + (ParaVel1 * 0.99f);
        DeltaVel1 = NewVel1 - OldVel1;

        // Player objects take care of their own collision response
        if ((pObj1->GetType() != object::TYPE_PLAYER) && (pObj1->GetType() != object::TYPE_BOT)) {
            pObj1->OnAddVelocity(DeltaVel1);
        }

        f32 DeltaSpeed = DeltaVel1.Length();
        f32 MinImpactVel = PainMinSpeed[GetIndex(GetTypeForPain(pObj1))];

        if (DeltaSpeed > MinImpactVel) {
            // Apply pain to each object according to change in velocity of the moving object.
            ApplyPain(pObj1, pObj2, DeltaSpeed);
            ApplyPain(pObj2, pObj1, DeltaSpeed);
        }

        // Destroy the vehicle if its upside down
        if (pObj1->GetAttrBits() & object::ATTR_VEHICLE) {
            vector3 LocalY(0.0f, 1.0f, 0.0f);
            LocalY.Rotate(pObj1->GetRotation());

            f32 Dot = Collision.Plane.Normal.Dot(LocalY);

            if ((Dot < 0.0f) && (LocalY.Y < 0.0f)) {
                ApplyPain(pObj1, pObj2, 10000.0f);
            }
        }
    }
}

//==============================================================================

f32 obj_mgr::GetDamageRadius(pain::type PainType) const {
    return (MAX(g_PainBase[PainType].DamageR0, g_PainBase[PainType].DamageR1));
}

//==============================================================================

void obj_mgr::PrintStats(X_FILE* pFile) {
    (void)pFile;
#if 0

    s32 L = 0;

    if( SHOW_LONGEST_LOGIC )
    {
        s32 LN=0;
        f32 LargestAvgTime=F32_MAX;
        f32 TotalMs=0.0f;
        s32 i;

        for( i=0; i<5; i++ )
        {
            f32 BestT = 0.0f;
            s32 BestID = 0;
            for( s32 j=0; j<object::TYPE_END_OF_LIST; j++ )
            if( m_pObjTypeInfo[j] )
            {
                if( m_pObjTypeInfo[j]->AverageLogicTime < LargestAvgTime )
                {
                    if( m_pObjTypeInfo[j]->AverageLogicTime > BestT )
                    {
                        BestID = j;
                        BestT = m_pObjTypeInfo[j]->AverageLogicTime;
                    }
                }
            }

            //if( BestT < 0.1f )
            //    break;
/*
            // Display this object's stats
            x_printfxy( 0, LN++, "%3d %4.1f %s", 
                m_pObjTypeInfo[BestID]->InstanceCount,
                m_pObjTypeInfo[BestID]->LogicTime.ReadMs(),
                m_pObjTypeInfo[BestID]->pTypeName );
*/

            s32 CreateBitsPacked;
            s32 UpdateBitsPacked;
            s32 CreatesPacked;
            s32 UpdatesPacked;

            m_pObjTypeInfo[BestID]->NetInfo.GetStats( CreateBitsPacked,
                                                      UpdateBitsPacked,
                                                      CreatesPacked,
                                                      UpdatesPacked );

            x_printfxy( 0, LN++, "%3d %4.1f %4d %3d %4d %3d %s", 
                m_pObjTypeInfo[BestID]->InstanceCount,
                m_pObjTypeInfo[BestID]->LogicTime.ReadMs(),
                CreateBitsPacked/8,
                CreatesPacked,
                UpdateBitsPacked/8,
                UpdatesPacked,
                m_pObjTypeInfo[BestID]->pTypeName );

            LargestAvgTime = BestT;
        }

        for( i = 0; i < object::TYPE_END_OF_LIST; i++ )
        if( m_pObjTypeInfo[i] )
            TotalMs += m_pObjTypeInfo[i]->LogicTime.ReadMs();

        x_printfxy( 0, LN++, "    %4.1f   ",TotalMs);

        if( m_pObjTypeInfo[object::TYPE_BOT] )
        {
            s32 i;

            for( i = 0; i < OBJ_NET_INFO_SAMPLES; i++ )
            {
                s32 X = (i%8)*3;
                s32 Y = 10+(i/8);
                x_printfxy( X, Y, "%1d",m_pObjTypeInfo[object::TYPE_BOT]->NetInfo.UpdatesPacked[i] );
            }
        }

        if( (tgl.NRenders % 6) == 0 ) 
            x_printf("\n");
        x_printf("%4d ",m_pObjTypeInfo[object::TYPE_BOT]->NetInfo.UpdateBitsPacked[(m_pObjTypeInfo[object::TYPE_BOT]->NetInfo.SampleIndex+31)%32]);

        return;
    }


    s32 TotalCreateBitsPacked=0;
    s32 TotalUpdateBitsPacked=0;
    s32 TotalCreatesPacked=0;
    s32 TotalUpdatesPacked=0;
    s32 TotalObjects=0;
    f32 TotalMs=0;

    for( s32 i = 0; i < object::TYPE_END_OF_LIST; i++ )
    {
        if( m_pObjTypeInfo[i] )
        {
            s32 CreateBitsPacked;
            s32 UpdateBitsPacked;
            s32 CreatesPacked;
            s32 UpdatesPacked;

            m_pObjTypeInfo[i]->NetInfo.GetStats( CreateBitsPacked,
                                                 UpdateBitsPacked,
                                                 CreatesPacked,
                                                 UpdatesPacked );

            if( pFile == NULL )
            {
                x_printfxy( 0, L++, "%3d %4.1f %4d %3d %4d %3d %s", 
                    m_pObjTypeInfo[i]->InstanceCount,
                    m_pObjTypeInfo[i]->LogicTime.ReadMs(),
                    CreateBitsPacked/8,
                    CreatesPacked,
                    UpdateBitsPacked/8,
                    UpdatesPacked,
                    m_pObjTypeInfo[i]->pTypeName );
            }
            else
            {
                x_fprintf( pFile, "%3d %4.1f %4d %3d %4d %3d %s\n", 
                    m_pObjTypeInfo[i]->InstanceCount,
                    m_pObjTypeInfo[i]->LogicTime.ReadMs(),
                    CreateBitsPacked/8,
                    CreatesPacked,
                    UpdateBitsPacked/8,
                    UpdatesPacked,
                    m_pObjTypeInfo[i]->pTypeName );
            }

            TotalObjects += m_pObjTypeInfo[i]->InstanceCount;
            TotalMs += m_pObjTypeInfo[i]->LogicTime.ReadMs();
            TotalCreateBitsPacked += CreateBitsPacked;
            TotalUpdateBitsPacked += UpdateBitsPacked;
            TotalCreatesPacked    += CreatesPacked;
            TotalUpdatesPacked    += UpdatesPacked;   
        }
    }

    if( pFile == NULL )
    {
        x_printfxy( 0, L++, "%3d %4.1f %4d %3d %4d %3d", 
            TotalObjects,
            TotalMs,
            TotalCreateBitsPacked/8,
            TotalCreatesPacked,
            TotalUpdateBitsPacked/8,
            TotalUpdatesPacked );
    }
    else
    {
        x_fprintf( pFile, "%3d %4.1f %4d %3d %4d %3d\n", 
            TotalObjects,
            TotalMs,
            TotalCreateBitsPacked/8,
            TotalCreatesPacked,
            TotalUpdateBitsPacked/8,
            TotalUpdatesPacked );
    }

#endif

    /* Version DMT was using at one time. */
    //s32 Line = 0;
    s32 i;
    s32 CreateBitsPacked;
    s32 UpdateBitsPacked;
    s32 CreatesPacked;
    s32 UpdatesPacked;

    for (i = 0; i < object::TYPE_END_OF_LIST; i++) {
        if (!m_pObjTypeInfo[i]) {
            continue;
        }

        if (m_pObjTypeInfo[i]->InstanceCount == 0) {
            continue;
        }

        if (m_pObjTypeInfo[i]->DefaultAttrBits & object::ATTR_SOLID_STATIC) {
            continue;
        }

        m_pObjTypeInfo[i]
            ->NetInfo.GetStats(CreateBitsPacked, UpdateBitsPacked, CreatesPacked, UpdatesPacked);

        /*
        x_printfxy( 0, Line++, "%3d %4.1f %4d %3d %4d %3d %s", 
                m_pObjTypeInfo[i]->InstanceCount,
                m_pObjTypeInfo[i]->LogicTime.ReadMs(),
                CreateBitsPacked/8,
                CreatesPacked,
                UpdateBitsPacked/8,
                UpdatesPacked,
                m_pObjTypeInfo[i]->pTypeName );
*/
    }
}

//==============================================================================

const obj_type_info* obj_mgr::GetTypeInfo(object::type Type) {
    ASSERT(m_pObjTypeInfo[Type]);
    return (m_pObjTypeInfo[Type]);
}

//==============================================================================

void obj_mgr::SanityCheck(void) {
    s32 i;

    // Make sure the number of objects of each type in the data base is the
    // same as the number of objects in the type cell.

    for (i = 1; i < object::TYPE_END_OF_LIST; i++) {
        if (m_pObjTypeInfo[i]) {
            s32 Count = 0;

            obj_ref TypeCell = CLASS_BASE + i;
            obj_ref Ref = CELL_NEXT(TypeCell);
            while (Ref != TypeCell) {
                Ref = CELL_NEXT(Ref);
                Count++;
            }

            ASSERT(Count == m_pObjTypeInfo[i]->InstanceCount);
        }
    }
}

//==============================================================================

#ifdef X_DEBUG
static char* om_pFile[10];
static s32 om_Line[10];
static object::type om_Type[10];
static f32 om_Time[10];
#undef StartTypeLoop
#endif

//==============================================================================

void obj_mgr::StartTypeLoop(object::type Type) {
    m_TypeCursorID++;
    ASSERT(m_TypeCursorID < MAX_TYPE_CURSORS);
    m_TypeCursor[m_TypeCursorID] = CELL_NEXT(CLASS_BASE + Type);

#ifdef X_DEBUG
    om_pFile[m_TypeCursorID] = NULL;
    om_Line[m_TypeCursorID] = -1;
    om_Type[m_TypeCursorID] = Type;
    om_Time[m_TypeCursorID] = (f32)x_GetTimeSec();
#endif
}

//==============================================================================
#ifdef X_DEBUG

void obj_mgr::DbgStartTypeLoop(object::type Type, char* pFile, s32 Line) {
    m_TypeCursorID++;
    ASSERT(m_TypeCursorID < MAX_TYPE_CURSORS);
    m_TypeCursor[m_TypeCursorID] = CELL_NEXT(CLASS_BASE + Type);

    om_pFile[m_TypeCursorID] = pFile;
    om_Line[m_TypeCursorID] = Line;
    om_Type[m_TypeCursorID] = Type;
    om_Time[m_TypeCursorID] = (f32)x_GetTimeSec();
}

#endif
//==============================================================================

object* obj_mgr::GetNextInType(void) {
    object* pObject = NULL;

    if (OBJ_SLOT(m_TypeCursor[m_TypeCursorID]) != -1) {
        pObject = m_pObject[OBJ_SLOT(m_TypeCursor[m_TypeCursorID])];
        m_TypeCursor[m_TypeCursorID] = CELL_NEXT(m_TypeCursor[m_TypeCursorID]);
    }

    return (pObject);
}

//==============================================================================

void obj_mgr::EndTypeLoop(void) {
    ASSERT(m_TypeCursorID >= 0);
    m_TypeCursor[m_TypeCursorID] = -1;
    m_TypeCursorID--;
}

//==============================================================================

s32 obj_mgr::GetInstanceCount(object::type ObjectType) {
    ASSERT(IN_RANGE(0, ObjectType, object::TYPE_END_OF_LIST));

    if (m_pObjTypeInfo[ObjectType]) {
        return (m_pObjTypeInfo[ObjectType]->InstanceCount);
    } else {
        return (0);
    }
}

//==============================================================================
//  OBJECT CLASS NET INFO
//==============================================================================

void obj_net_info::Clear(void) {
    SampleIndex = 0;
    for (s32 i = 0; i < OBJ_NET_INFO_SAMPLES; i++) {
        CreateBitsPacked[i] = 0;
        UpdateBitsPacked[i] = 0;
        CreatesPacked[i] = 0;
        UpdatesPacked[i] = 0;
    }
}

//==============================================================================

void obj_net_info::GetStats(
    s32& aCreateBitsPacked,
    s32& aUpdateBitsPacked,
    s32& aCreatesPacked,
    s32& aUpdatesPacked
) {
    aCreateBitsPacked = 0;
    aUpdateBitsPacked = 0;
    aCreatesPacked = 0;
    aUpdatesPacked = 0;

    for (s32 i = 0; i < OBJ_NET_INFO_SAMPLES; i++) {
        aCreateBitsPacked += CreateBitsPacked[i];
        aUpdateBitsPacked += UpdateBitsPacked[i];
        aCreatesPacked += CreatesPacked[i];
        aUpdatesPacked += UpdatesPacked[i];
    }
}

//==============================================================================

void obj_net_info::AddCreate(s32 NBits) {
    CreateBitsPacked[SampleIndex] += NBits;
    CreatesPacked[SampleIndex] += 1;
}

//==============================================================================

void obj_net_info::AddUpdate(s32 NBits) {
    UpdateBitsPacked[SampleIndex] += NBits;
    UpdatesPacked[SampleIndex] += 1;
}

//==============================================================================

void obj_net_info::NextSample(void) {
    SampleIndex = (SampleIndex + 1) % OBJ_NET_INFO_SAMPLES;

    // Clear sample counts.
    CreateBitsPacked[SampleIndex] = 0;
    UpdateBitsPacked[SampleIndex] = 0;
    CreatesPacked[SampleIndex] = 0;
    UpdatesPacked[SampleIndex] = 0;
}

//==============================================================================
