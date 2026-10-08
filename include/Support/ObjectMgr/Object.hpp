// Imported genuine tribes-aa pristine4aab7137 engine reference: aa/support/objectmgr/Object.hpp
// Provisional Hobbit API/layout adaptation; no fabricated PC labels. See IMPORT-NOTES.md.
//==============================================================================
//
//  Object.hpp
//
//==============================================================================

#ifndef OBJECT_HPP
#define OBJECT_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <Support/ObjectMgr/Collider.hpp>

//==============================================================================
//  TYPES
//==============================================================================

class object;
class bitstream;

struct obj_type_info;
struct pain;

typedef object* obj_create_fn(void);

//==============================================================================
//  CLASS OBJECT
//==============================================================================

class object {

    //------------------------------------------------------------------------------
    //  Public Types
    //------------------------------------------------------------------------------

public:
    struct id {
        s16 Slot; // If -1, then object not in ObjMgr.
        s16 Seq;

        id(void) {
            Slot = -1;
            Seq = -1;
        }
        id(s16 aSlot, s16 aSeq) {
            Slot = aSlot;
            Seq = aSeq;
        }
        xbool operator==(id ID) const {
            return ((ID.Slot == Slot) && (ID.Seq == Seq));
        }
        xbool operator!=(id ID) const {
            return ((ID.Slot != Slot) || (ID.Seq != Seq));
        }
        s32 GetAsS32(void) const {
            return (((((s32)Slot) << 16) | (((s32)Seq) & 0xFFFF)));
        }
        void SetAsS32(s32 a) {
            Slot = (s16)(a >> 16);
            Seq = (s16)(a & 0xFFFF);
        }
    };

    //------------------------------------------------------------------------------

public:
    enum type {
        TYPE_NULL = -1,
        TYPE_START_OF_LIST = 0,

        //
        // Begin DAMAGEABLE types.
        //
        TYPE_BEGIN_DAMAGEABLE = 0,

        TYPE_FLAG = 0, // Flag isn't damageable, but can be moved by pain force.

        // Static, though possibly animated, types.

        TYPE_STATION_FIXED,
        TYPE_STATION_VEHICLE,
        TYPE_STATION_DEPLOYED,

        TYPE_TURRET_FIXED,
        TYPE_TURRET_SENTRY,
        TYPE_TURRET_CLAMP,

        TYPE_SENSOR_REMOTE,
        TYPE_SENSOR_MEDIUM,
        TYPE_SENSOR_LARGE,

        TYPE_GENERATOR,
        TYPE_BEACON,

        // Moving solid types that can have other objects land/stick to them.
        // NOTE: These must be above other moving objects in order for the
        // update logic to work properly.

        // ALSO - If you update these types, make sure you update the useful vehicle defines below!!
        TYPE_VEHICLE_WILDCAT,
        TYPE_VEHICLE_BEOWULF,
        TYPE_VEHICLE_JERICHO2,
        TYPE_VEHICLE_SHRIKE,
        TYPE_VEHICLE_THUNDERSWORD,
        TYPE_VEHICLE_HAVOC,

        // Other moving types.  (Other objects can not land on them.)

        TYPE_MINE,
        TYPE_SATCHEL_CHARGE,

        TYPE_CORPSE,
        TYPE_PLAYER, // Used for damage to LIGHT armor.
        TYPE_BOT,    // Used for damage to MEDIUM armor.

        //
        // End DAMAGEABLE types.
        //
        TYPE_END_DAMAGEABLE, // Used for damage to HEAVY armor.

        TYPE_VPAD,

        TYPE_DISK,
        TYPE_PLASMA,
        TYPE_BULLET,
        TYPE_GRENADE,
        TYPE_MORTAR,
        TYPE_LASER,
        TYPE_BLASTER,
        TYPE_MISSILE,
        TYPE_GENERICSHOT,
        TYPE_SHRIKESHOT,
        TYPE_BOMBERSHOT,
        TYPE_BOMB,

        TYPE_PICKUP,

        TYPE_NEXUS,
        TYPE_FLIPFLOP,
        TYPE_PROJECTOR,
        TYPE_FORCE_FIELD,

        TYPE_PARTICLE_EFFECT,
        TYPE_DEBRIS,
        TYPE_BUBBLE,

        // Static types.

        TYPE_TERRAIN,
        TYPE_BUILDING,

        TYPE_WAYPOINT,
        TYPE_SCENIC,

        TYPE_PLACE_HOLDER,

        // End.
        TYPE_END_OF_LIST,

        // Special values used by pain system.  (Yes, it's a bit of a HACK.)
        TYPE_ARMOR_LIGHT = TYPE_PLAYER,
        TYPE_ARMOR_MEDIUM = TYPE_PLAYER + 1,
        TYPE_ARMOR_HEAVY = TYPE_PLAYER + 2,

        // Useful vehicle defines
        TYPE_VEHICLE_START = TYPE_VEHICLE_WILDCAT,
        TYPE_VEHICLE_END = TYPE_VEHICLE_HAVOC,
    };

    //------------------------------------------------------------------------------

public:
    enum attr {
#define bit(x) (1 << (x))

        // Notes on the attribute bits:
        //
        // Since the bits are OR'ed together, we need to have bit catagories
        // which are specific enough for any collision or object manager
        // query.
        //
        // ATTR_SOLID_STATIC
        //   Nothing can pass thru this object.  And it NEVER moves, NEVER
        //   changes shape, NEVER goes away.
        //      - terrain
        //      - buildings
        //      - scenery (rocks, trees)
        //
        // ATTR_SOLID_DYNAMIC
        //   Nothing can pass thru this object.  But it can move, or change
        //   shape, go away, and be created mid-game.
        //      - players
        //      - turrets (fixed and deployed)
        //      - inventory stations (fixed and deployed)
        //      - sensors (fixed and deployed)
        //      - generators
        //      - beacons
        //
        // ATTR_PERMEABLE
        //   Solid objects can pass THROUGH permeable objects.  However, the
        //   contact between solid and permeable objects may cause reactions.
        //      - pickups (packs, ammo, weapons)
        //      - corpses
        //      - flags
        //
        // ATTR_PERMANENT
        //   Permanent objects never go away or get created mid-game.  They can
        //   change state, though.
        //      - generators
        //      - fixed turrets
        //      - fixed sensors
        //      - fixed invens
        //

        ATTR_SOLID_STATIC = bit(0),
        ATTR_SOLID_DYNAMIC = bit(1),
        ATTR_PERMEABLE = bit(2),
        ATTR_PERMANENT = bit(3),

        ATTR_DAMAGEABLE = bit(4), // Reacts to pain (damage and/or force)
        ATTR_REPAIRABLE = bit(5), // Can be repaired with repair "gun"
        ATTR_ELFABLE = bit(6),    // Can be the focus of an elf beam

        ATTR_HOT = bit(7),     // Attracts heat seekers
        ATTR_SENSED = bit(8),  // Shows up on sensors
        ATTR_LABELED = bit(9), // Has label/name for HUD display
        ATTR_ASSET = bit(10),  // Descends from class asset

        ATTR_VEHICLE = bit(11),
        ATTR_PLAYER = bit(12),
        ATTR_BUILDING = bit(13),
        ATTR_MINE = bit(14),
        ATTR_BEACON = bit(15),
        ATTR_FORCE_FIELD = bit(16),
        ATTR_TURRET = bit(17),
        ATTR_VPAD = bit(18),
        ATTR_PICKUP = bit(19),

        ATTR_LOGIC_APPLIED = bit(20), // Flags logic has been applied this frame

        ATTR_UNBIND_ORIGIN = bit(29), // Reacts to UnbindOriginID()
        ATTR_UNBIND_TARGET = bit(30), // Reacts to UnbindTargetID()

        ATTR_GLOBAL = bit(31),
        ATTR_SOLID = ATTR_SOLID_STATIC | ATTR_SOLID_DYNAMIC,
        ATTR_ALL = 0xFFFFFFFF,

#undef bit
    };

    //------------------------------------------------------------------------------

public:
    enum update_code {
        NO_UPDATE, // Object has not moved, do not update in ObjMgr.
        UPDATE,    // Object has moved, please update in ObjMgr.
        DESTROY,   // Object is dead, please remove from ObjMgr and delete!
    };

    //------------------------------------------------------------------------------
    //  Public Functions
    //------------------------------------------------------------------------------

public:
    virtual ~object(void);

    virtual void OnAdd(void);
    virtual void OnRemove(void);

    virtual update_code OnAdvanceLogic(f32 DeltaTime);
    virtual update_code OnAcceptUpdate(const bitstream& BitStream, f32 SecInPast);
    virtual void OnProvideUpdate(bitstream& BitStream, u32& DirtyBits, f32 Priority);
    virtual void OnAcceptInit(const bitstream& BitStream);
    virtual void OnProvideInit(bitstream& BitStream);

    virtual void OnCollide(u32 AttrBits, collider& Collider);
    virtual void OnPain(const pain& Pain);
    virtual void OnAddVelocity(const vector3& Velocity);

    virtual f32 GetHealth(void) const; // 0.0 thru 1.0
    virtual f32 GetEnergy(void) const; // 0.0 thru 1.0
    virtual xbool IsShielded(void) const;
    virtual const xwchar* GetLabel(void) const;

    const bbox& GetBBox(void) const;
    const vector3& GetPosition(void) const;

    virtual radian3 GetRotation(void) const;
    virtual vector3 GetVelocity(void) const;
    virtual vector3 GetAverageVelocity(void) const;
    virtual f32 GetMass(void) const;
    virtual vector3 GetBlendPos(void) const;
    virtual radian3 GetBlendRot(void) const;

    virtual f32 GetPainRadius(void) const;
    virtual vector3 GetPainPoint(void) const;
    vector3 GetBlendPainPoint(void) const;

    virtual vector3 GetBlendFirePos(s32 Index = 0);

    virtual f32 GetSenseRadius(void) const;
    virtual void ClearSensedBits(void);
    virtual void AddSensedBits(u32 SensedBits);
    virtual u32 GetSensedBits(void) const;

    virtual void MissileTrack(void);
    virtual void MissileLock(void);

    virtual void UnbindOriginID(object::id OriginID);
    virtual void UnbindTargetID(object::id TargetID);

    virtual matrix4* GetNodeL2W(s32 ID, s32 NodeIndex);

    virtual void Render(void);
    virtual void DebugRender(void);

    u32 GetTeamBits(void) const;
    u32 GetAttrBits(void) const;
    u32 GetDirtyBits(void) const;
    void ClearDirtyBits(void);

    id GetObjectID(void) const;
    type GetType(void) const;
    const char* GetTypeName(void) const;
    obj_type_info* GetTypeInfoPtr(void) const;

    virtual void SetTeamBits(u32 TeamBits);

    //------------------------------------------------------------------------------
    //  Internal Data
    //------------------------------------------------------------------------------

protected:
    obj_type_info* m_pTypeInfo;
    id m_ObjectID;
    u32 m_AttrBits;
    u32 m_DirtyBits;
    u32 m_TeamBits;

    vector3 m_WorldPos;
    bbox m_WorldBBox;

    //------------------------------------------------------------------------------
    //  Internal Stuff
    //------------------------------------------------------------------------------

protected:
    object(void);
    friend class obj_mgr;
};

//==============================================================================
#endif // OBJECT_HPP
//==============================================================================
