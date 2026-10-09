//=========================================================================
//
//  UpdateManager.hpp
//
//=========================================================================
#ifndef UPDATEMANAGER_HPP
#define UPDATEMANAGER_HPP

#include "x_files.hpp"
#include "netlib/bitstream.hpp"
#include "objectmgr/objectmgr.hpp"
#include "GameMgr/MsgMgr.hpp"

//=========================================================================

struct conn_packet;

#define MAX_LIFE_ENTRIES    1024   
#define MAX_UPDATE_ENTRIES  1024

class update_manager
{

private:

    struct life_entry
    {
        s32         Seq;
        s32         Create; // 0 - Destroy, 1 - Create, 2 - Message

        object::id  ObjID;
        msg         Message;

        s32         PacketSeq;
        s32         Next;
        s32         Prev;    

        // Extra debug info
        s32         ClassType;
        s32         NTimesPacked;
        xtick       TimeEnqueued;
    };

    struct update_entry
    {   
        object::id  ObjID;
        u32         Bits;
        s32         PacketNext;
        s32         Next;
        s32         Prev;
    };

    struct obj_entry
    {
        u32         LostUpdateBits;
        u32         DirtyBits;
        s32         LifeStage;
        s32         ObjSeq;
    };

    life_entry      m_Life[MAX_LIFE_ENTRIES];
    s32             m_LifeSeq;
    u32             m_LifeGroupSeq;
    s32             m_FirstLife;
    s32             m_LastLife;
    s32             m_FirstFreeLife;
    s32             m_NLivesAllocated;
    s32             m_NLifeAllocs;
    s32             m_NLifeDeallocs;

    update_entry    m_Update[MAX_UPDATE_ENTRIES];
    s32             m_FirstUpdate;
    s32             m_LastUpdate;
    s32             m_FirstFreeUpdate;
    s32             m_NUpdatesAllocated;
    s32             m_UpdateCursor;

    xbool           m_Connected;

    s32             m_ClientControlledIndex[2];

    vector3         m_ClientPos[2];

    obj_entry       m_Object[ obj_mgr::MAX_SERVER_OBJECTS ];

    s32  AllocLifeOnEnd ( void );
    void DeallocLife    ( s32 I );
    s32  AllocUpdateOnEnd( void );
    void DeallocUpdate  ( s32 I );

public:

    update_manager      ( void );
    ~update_manager     ( void );

    void Init           ( s32 ClientIndex );
    void Kill           ( void );

    void SetClientControlled( s32 PlayerIndex0, s32 PlayerIndex1 );

    void SendCreate     ( s32 ObjSlot );
    void SendDestroy    ( s32 ObjSlot );
//  void SendUpdate     ( object::id ObjID, u32 DirtyBits );

    void SendMsg        ( const msg& Msg );

    void PackLife       ( conn_packet& Packet, bitstream& BitStream, s32 NLivesAllowed );
    void UnpackLife     ( bitstream& BitStream );

    void PackUpdates    ( conn_packet& Packet, bitstream& BitStream, s32 NUpdatesAllowed );
    void UnpackUpdates  ( bitstream& BitStream, f32 SecInPast );

    xbool PackObjUpdate ( bitstream& BitStream, s32 Slot, s32& Count, conn_packet& Packet );

    void PacketAck      ( conn_packet& Packet, xbool Arrived );

    void UpdateObject   ( object::id ObjID, xbool MissionLoaded, object* pObj, xbool Syncing );

    xbool IsConnected   ( void );

    void SanityCheck    ( void );

    void SetClientPos   ( s32 ID, const vector3& Pos );

    f32  ComputePriority( const vector3& ObjPos );

    void Reset          ( void );

    s32  GetNumLifeEntries( void );

    // Begin debug stuff
    void ShowLifeDelay( s32 L );
    void DumpLifeState( void );
    void DumpObjectState( void );
    f32             m_WorstLifeDelay;
    f32             m_RecentLifeDelay;
    s32             m_LifeDelayCounter;
	s32				m_ClientIndex;
    // End debug stuff
};

//=========================================================================
#endif
//=========================================================================

