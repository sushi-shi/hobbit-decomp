//==============================================================================
//
//  UpdateManager.cpp
//
//==============================================================================

#include "x_files.hpp"
#include "specialversion.hpp"
#include "updatemanager.hpp"
#include "connmanager.hpp"
#include "globals.hpp"
#include "objects/player/playerobject.hpp"                
#include "GameServer.hpp"


#define LIFE_SEQ_BITS_SENT  8
#define LIFE_TYPE_BITS_SENT 6
#define LIFE_SLOT_BITS_SENT (obj_mgr::MAX_SERVER_OBJECTS_BITS)

//==============================================================================

//#define ENABLE_UPDATE_LOG

#ifdef ENABLE_UPDATE_LOG
xbool   VUM         = TRUE;
#define LIFE_LOG    if(VUM) x_DebugMsg
#define LIFE_REC    if( !(++Records %= 4) )  LIFE_LOG( "\n" );  LIFE_LOG
#else
#define LIFE_LOG    if (0) NULL_LOG
#define LIFE_REC    if (0) NULL_LOG
#endif

extern s32 g_PacketSeq;
s32        Records;

//==============================================================================

struct player_packet
{
    u32 Data[256];
    bitstream BitStream;
    s32 nUpdates;
    s32 nUpdatesProcessed;
    u32 UpdateSlot[32];
    s32 UpdateBitOffset[32];
    s32 UpdatePostCursor[32];
    f32 SecInPast;
    xtick ArrivalTime;
};

#define MAX_PLAYER_PACKETS 4
player_packet   s_PlayerPacket[ MAX_PLAYER_PACKETS ];
s32             s_PlayerPacketIndex=0;

static s32 PLAYER_NUM_UPDATE_PACKETS = 1;
static s32 PLAYER_UPDATES_PER_PACKET = 8;
static s32 PLAYER_UPDATES_PER_FRAME = 3;
static s32 PLAYER_UPDATE_CURSOR = 0;

//==============================================================================

inline static
void NULL_LOG( const char* pFormatStr, ... )
{
    (void)pFormatStr;
}

//==============================================================================

update_manager::update_manager( void )
{
}

//==============================================================================

update_manager::~update_manager( void )
{
}

//==============================================================================

void update_manager::Reset( void )
{
    s32 i;

    // Clear all update entries
    for( i = 0; i < obj_mgr::MAX_SERVER_OBJECTS; i++ )
    {
        m_Object[i].LostUpdateBits = 0;
        m_Object[i].DirtyBits = 0;
        m_Object[i].LifeStage = -1;
        m_Object[i].ObjSeq    = -1;
    }

    // Hook up all life entries into free list
    m_FirstLife = -1;
    m_LastLife  = -1;
    m_FirstFreeLife = 0;
    for( i=0; i<MAX_LIFE_ENTRIES; i++ )
    {
        m_Life[i].Seq    = -1;
        m_Life[i].Create = -1;
        m_Life[i].Next   = i+1;
        m_Life[i].Prev   = i-1;
        m_Life[i].ObjID.Slot = -1;
        m_Life[i].ObjID.Seq  = -1;
        m_Life[i].PacketSeq  = -1;
        m_Life[i].ClassType  = -1;
        m_Life[i].NTimesPacked = -1;
    }
    m_Life[MAX_LIFE_ENTRIES-1].Next = -1;

    m_NLivesAllocated = 0;
    m_NLifeAllocs = 0;
    m_NLifeDeallocs = 0;
    m_LifeSeq = 0;
    m_LifeGroupSeq = 0;

    m_WorstLifeDelay   = 0.0f;
    m_RecentLifeDelay  = 0.0f;
    m_LifeDelayCounter = 1;

    // Hook up all update entries into free list
    m_FirstUpdate = -1;
    m_LastUpdate  = -1;
    m_FirstFreeUpdate = 0;
    for( i=0; i<MAX_UPDATE_ENTRIES; i++ )
    {
        m_Update[i].Next = i+1;
        m_Update[i].Prev = i-1;
        m_Update[i].ObjID.Slot = -1;
        m_Update[i].ObjID.Seq  = -1;
        m_Update[i].PacketNext = -1;
    }
    m_Update[MAX_LIFE_ENTRIES-1].Next = -1;

    m_NUpdatesAllocated = 0;
    m_UpdateCursor = 32;

    // Clear player packets.
    for( i = 0; i < MAX_PLAYER_PACKETS; i++ )
    {
        s_PlayerPacket[i].nUpdates = 0;
    }
}

//==============================================================================

void update_manager::Init( const s32 ClientIndex )
{
    Reset();
    
    m_ClientControlledIndex[0] = -1;
    m_ClientControlledIndex[1] = -1;

    // Consider connection open for now
    m_Connected = TRUE;
	m_ClientIndex		= ClientIndex;

    m_WorstLifeDelay   = 0.0f;
    m_RecentLifeDelay  = 0.0f;
    m_LifeDelayCounter = 1;

    x_memset( s_PlayerPacket, 0, sizeof(s_PlayerPacket) );
}

//==============================================================================

void update_manager::Kill( void )
{
}

//==============================================================================

s32 update_manager::AllocLifeOnEnd( void )
{
    s32 I;

    ASSERT( (m_FirstFreeLife>=-1) && (m_FirstFreeLife<MAX_LIFE_ENTRIES) );

    // Get free life node from free list
    if( m_FirstFreeLife == -1 )
        return -1;
    
    I = m_FirstFreeLife;
    m_FirstFreeLife = m_Life[m_FirstFreeLife].Next;
    if( m_FirstFreeLife != -1 )
        m_Life[m_FirstFreeLife].Prev = -1;
    m_Life[I].Next = -1;
    m_Life[I].Prev = -1;

    ASSERT( m_Life[I].Seq == -1 );

    // Initialize this node
    m_Life[I].Create     = -1;
    m_Life[I].Next       = -1;
    m_Life[I].Prev       = -1;
    m_Life[I].ObjID.Slot = -1;
    m_Life[I].ObjID.Seq  = -1;
    m_Life[I].PacketSeq  = -1;
    m_Life[I].Seq        = m_LifeSeq;

    m_Life[I].ClassType     = -1;
    m_Life[I].NTimesPacked  = -1;
    m_Life[I].TimeEnqueued  = x_GetTime();

    m_LifeSeq++;

    // Add node to end of used list
    if( m_FirstLife == -1 )
    {
        m_FirstLife = m_LastLife = I;
    }
    else
    {
        m_Life[m_LastLife].Next = I;
        m_Life[I].Prev = m_LastLife;
        m_LastLife = I;
    }

    ASSERT( (m_LastLife>=-1) && (m_LastLife<MAX_LIFE_ENTRIES) );
    m_NLifeAllocs++;
    m_NLivesAllocated++;

    ASSERT( (I>=0) && (I<MAX_LIFE_ENTRIES) );
    return I;
}

//==============================================================================

void update_manager::DeallocLife( s32 I )
{
    // Remove from main list
    ASSERT( (I>=0) && (I<MAX_LIFE_ENTRIES) );

    if( m_FirstLife == I )
        m_FirstLife = m_Life[I].Next;

    if( m_LastLife == I )
        m_LastLife = m_Life[I].Prev;

    if( m_Life[I].Prev != -1 )
        m_Life[ m_Life[I].Prev ].Next = m_Life[I].Next;
    
    if( m_Life[I].Next != -1 )
        m_Life[ m_Life[I].Next ].Prev = m_Life[I].Prev;

    ASSERT( (m_LastLife>=-1) && (m_LastLife<MAX_LIFE_ENTRIES) );

    // Add to free list

    m_Life[I].Seq = -1;
    m_Life[I].ObjID.Slot = -1;
    m_Life[I].ObjID.Seq  = -1;
    m_Life[I].PacketSeq = -1;
    m_Life[I].Prev = -1;
    m_Life[I].Next = m_FirstFreeLife;
    if( m_FirstFreeLife != -1 )
        m_Life[m_FirstFreeLife].Prev = I;
    m_FirstFreeLife = I;

    m_NLifeDeallocs++;
    m_NLivesAllocated--;
}

//==============================================================================

s32  update_manager::AllocUpdateOnEnd( void )
{
    s32 I;

    // Get free life node from free list
    if( m_FirstFreeUpdate == -1 )
        return -1;
    
    I = m_FirstFreeUpdate;
    m_FirstFreeUpdate = m_Update[m_FirstFreeUpdate].Next;
    if( m_FirstFreeUpdate != -1 )
    m_Update[m_FirstFreeUpdate].Prev = -1;
    m_Update[I].Next = -1;
    m_Update[I].Prev = -1;

    // Initialize this node
    m_Update[I].Next       = -1;
    m_Update[I].Prev       = -1;
    m_Update[I].ObjID.Slot = -1;
    m_Update[I].ObjID.Seq  = -1;
    m_Update[I].Bits       =  0;
    m_Update[I].PacketNext = -1;

    // Add node to end of used list
    if( m_FirstUpdate == -1 )
    {
        m_FirstUpdate = m_LastUpdate = I;
    }
    else
    {
        m_Update[m_LastUpdate].Next = I;
        m_Update[I].Prev = m_LastUpdate;
        m_LastUpdate = I;
    }

    m_NUpdatesAllocated++;

    //x_printfxy(0,21,"Updates Allocated %1d",m_NLivesAllocated);
    ASSERT( (I>=0) && (I<MAX_UPDATE_ENTRIES) );
    return I;
}

//==============================================================================

void update_manager::DeallocUpdate( s32 I )
{
    // Remove from main list
    ASSERT( (I>=0) && (I<MAX_UPDATE_ENTRIES) );

    if( m_FirstUpdate == I )
        m_FirstUpdate = m_Update[I].Next;

    if( m_LastUpdate == I )
        m_LastUpdate = m_Update[I].Prev;

    if( m_Update[I].Prev != -1 )
        m_Update[ m_Update[I].Prev ].Next = m_Update[I].Next;
    
    if( m_Update[I].Next != -1 )
        m_Update[ m_Update[I].Next ].Prev = m_Update[I].Prev;

    // Add to free list

    m_Update[I].Prev = -1;
    m_Update[I].Next = m_FirstFreeUpdate;
    if( m_FirstFreeUpdate != -1 )
        m_Update[m_FirstFreeUpdate].Prev = I;
    m_FirstFreeUpdate = I;

    m_NUpdatesAllocated--;
}

//==============================================================================

void update_manager::SendCreate( s32 ObjSlot )
{
    if( !m_Connected ) return;

SanityCheck();

    s32 I = AllocLifeOnEnd();
    if( I==-1 )
    {
        m_Connected = FALSE;
        x_DebugMsg("*************************************\n");
        x_DebugMsg("UpdateManger: No life entries available\n");
        x_DebugMsg("*************************************\n");
        return;
    }
    m_Life[I].Create = 1;   // CREATE
    m_Life[I].ObjID.Slot = ObjSlot;
    m_Life[I].ObjID.Seq  = m_Object[ObjSlot].ObjSeq;

    object* pObj = ObjMgr.GetObjectFromID( m_Life[I].ObjID );
    ASSERT( pObj );
    ASSERT( pObj->GetObjectID().Seq == m_Life[I].ObjID.Seq );

    m_Life[I].ClassType = pObj->GetType();
    m_Life[I].NTimesPacked = 0;

SanityCheck();

    LIFE_LOG( "{%d} SendCreate[%03d], ObjSeq %d, LifeSlot %03d, LifeStage %d, Type %s\n", m_ClientIndex,
              ObjSlot, m_Life[I].ObjID.Seq, I, m_Object[ObjSlot].LifeStage,pObj->GetTypeName() );
}

//==============================================================================

void update_manager::SendDestroy( s32 ObjSlot )
{
    s32 I;

    if( !m_Connected ) return;

SanityCheck();
/*
    // Remove all updates refering to obj
    I = m_FirstUpdate;
    while( I != -1 )
    {
        s32 Next = m_Life[I].Next;

        if( (m_Update[I].ObjID.Slot == ObjSlot) &&
            (m_Update[I].ObjID.Seq  == m_Object[ObjSlot].ObjSeq) )
        {
            DeallocUpdate(I);
            LIFE_LOG( "Removed updates when object %d was destroyed\n", ObjSlot );
        }

        I = Next;
    }
*/
SanityCheck();

    // Search for the corresponding Create
    ASSERT( (m_LastLife>=-1) && (m_LastLife<MAX_LIFE_ENTRIES) );
    I = m_LastLife;
    while( I != -1 )
    {
        if( (m_Life[I].PacketSeq == -1) && 
            (m_Life[I].Create == 1) && 
            (m_Life[I].ObjID.Seq == m_Object[ObjSlot].ObjSeq) )
            break;

        I = m_Life[I].Prev;
    }

    if( I != -1 )
    {
        DeallocLife(I);
SanityCheck();
        return;
    }

    // Add a destroy life entry
    I = AllocLifeOnEnd();
    if( I==-1 )
    {
        m_Connected = FALSE;
        x_DebugMsg("*************************************\n");
        x_DebugMsg("UpdateManger: No destroy entries available\n");
        x_DebugMsg("*************************************\n");

SanityCheck();
        return;
    }
    m_Life[I].Create = 0;   // DESTROY
    m_Life[I].ObjID.Slot = ObjSlot;
    m_Life[I].ObjID.Seq  = m_Object[ObjSlot].ObjSeq;

    m_Life[I].ClassType = -1;
    m_Life[I].NTimesPacked = 0;

SanityCheck();

    LIFE_LOG( "{%d} SendDestroy[%03d], ObjSeq %d, LifeSlot %03d\n", m_ClientIndex,
              ObjSlot, m_Life[I].ObjID.Seq, I );
}

//==============================================================================

s32 update_manager::GetNumLifeEntries( void )
{
    return m_NLivesAllocated;
}

//==============================================================================

void update_manager::PackLife( conn_packet& Packet, bitstream& BitStream, s32 NLivesAllowed )
{    
    s32 I;

    // Clear packet of life info
    for( s32 i=0; i<CONN_LIVES_PER_PACKET; i++ )
        Packet.LifeID[i] = -1;

    Packet.NLife        = 0;
    Packet.LifeGroupSeq = 256;

    // Write down the packet cursor so we can come back and tweak it up later.
    s32 HeaderCursor = BitStream.GetCursor();

    // Write out number of life entries.  Zero, so far.
    BitStream.WriteU32( 0, 4 );

    // Not connected?  Outta here!
    if( !m_Connected ) 
        return;

    // If there is nothing in the queue, then we are done.
    if( m_FirstLife == -1 )
        return;

    // If there is nothing pending in the queue, then we are also done.
    I = m_FirstLife;
	// Find the first life packet with a sequence number of -1 which denotes
	// it hasn't been sent yet
    while( I != -1 )
    {
        if( m_Life[I].PacketSeq == -1 )
            break;
        I = m_Life[I].Next;
    }
	// If we reached the end of the life list, return now. We have nothing to do.
    if( I == -1 )
        return;

    // Verbosity.
    {
        LIFE_LOG( ".\n{%d}-----Begin PackLife( PacketSeq %d, LifeGroup 0x%02X )-----", m_ClientIndex,Packet.Seq, m_LifeGroupSeq );
        Records = -1;
    }

    // Write out the number of bits used in all this data.  Zero, so far.
    BitStream.WriteU32( 0, 13 );

    // Write out the group sequence number.
    BitStream.WriteU32( m_LifeGroupSeq, 8 );        // Write to stream
    Packet.LifeGroupSeq = m_LifeGroupSeq;           // Write into packet
    m_LifeGroupSeq = (m_LifeGroupSeq + 1) & 255;    // Advance value    

    ASSERT( (m_FirstLife >= -1) && (m_FirstLife< MAX_LIFE_ENTRIES) );
    ASSERT( (m_LastLife  >= -1) && (m_LastLife < MAX_LIFE_ENTRIES) );

SanityCheck();

    // Write out as many items as we can
    while( NLivesAllowed-- )
    {
        // Remember where this item started
        if( BitStream.GetNBitsFree() < 32 )
            break;
        s32 ItemCursor = BitStream.GetCursor();

		// Find item with a sequence # of -1, which means it hasn't been sent.
        while( I != -1 )
        {
            if( m_Life[I].PacketSeq == -1 )
                break;
            I = m_Life[I].Next;
        }
		// If we reach the end of the list, we're done.
        if( I == -1 ) 
            break;

        #ifdef SAFE_NETWORK
        BitStream.WriteMarker();
        #endif

        // Life delay debugging [TO DO: Get rid of this.]
        /*
        {
            xtick DelayTick = x_GetTime() - m_Life[I].TimeEnqueued;
            f32   DelayMS   = x_TicksToMs( DelayTick );
            m_WorstLifeDelay   = MAX( DelayMS, m_WorstLifeDelay );
            m_RecentLifeDelay += DelayMS;
            m_LifeDelayCounter++;

            if( m_LifeDelayCounter > 100 )
            {
                m_RecentLifeDelay  = DelayMS;
                m_LifeDelayCounter = 1;
            }
        }
        */

        // Write out basic info
        //BitStream.WriteU32( m_Life[I].Seq, LIFE_SEQ_BITS_SENT );
        BitStream.WriteU32( m_Life[I].ObjID.Slot, LIFE_SLOT_BITS_SENT );
        BitStream.WriteRangedS32( m_Life[I].Create, 0, 2 );

        // Pack up init info from object
        object* pObj = NULL;
        if( m_Life[I].Create == 0 ) // DESTROY
        {
            LIFE_REC( "D[%03d]  ", m_Life[I].ObjID.Slot );
        }
        if( m_Life[I].Create == 1 ) // CREATE
        {
            // Get object from manager
            pObj = ObjMgr.GetObjectFromID( m_Life[I].ObjID );

#ifdef X_DEBUG
            if( pObj == NULL )
            {
                SanityCheck();
                ASSERT( pObj );
            }
#endif

            // Confirm this is the actual object
            ASSERT( m_Life[I].ObjID.Seq == pObj->GetObjectID().Seq );

            // We need to pack up what class the object is
            BitStream.WriteU32( pObj->GetType(), LIFE_TYPE_BITS_SENT );

            // Pack up init info the object needs
            pObj->OnProvideInit( BitStream );

            LIFE_REC( "C[%03d](%6.6s)  ", m_Life[I].ObjID.Slot, pObj->GetTypeName() );
        }
        if( m_Life[I].Create == 2 ) // MESSAGE
        {
            LIFE_LOG( "M  " );
            MsgMgr.PackMsg( m_Life[I].Message, BitStream );
        }

        // Check if we over-ran bitstream
        if( BitStream.Overwrite() )
        {
            BitStream.SetCursor(ItemCursor);
            BitStream.ClearOverwrite();
            LIFE_LOG( "* OVERRUN: Ignore last item *" );
            break;
        }

        // Update netinfo for type
        if( pObj )
            pObj->GetTypeInfoPtr()->NetInfo.AddCreate( BitStream.GetCursor() - ItemCursor );

        // We didn't overwrite buffer so keep this one
        Packet.LifeID[Packet.NLife] = I;
		// Set the sequence # of the packet since we've now packaged it up to send
        m_Life[I].PacketSeq = Packet.Seq;
        m_Life[I].NTimesPacked = 0;
        Packet.NLife++;
    }

    // Fill in header and return
    s32 EndCursor = BitStream.GetCursor(); 
    s32 Bits      = EndCursor - HeaderCursor - 25;  // 25 = 4 + 13 + 8
    BitStream.SetCursor( HeaderCursor );
    BitStream.WriteU32 ( Packet.NLife, 4 );
    BitStream.WriteU32 ( Bits, 13 );
    BitStream.SetCursor( EndCursor );

    ASSERT( (m_LastLife>=-1) && (m_LastLife<MAX_LIFE_ENTRIES) );

    LIFE_LOG( "\n-----End PackLife( Items %d )-----\n.\n", Packet.NLife );
    DumpLifeState();
}

//==============================================================================

void update_manager::UnpackLife( bitstream& BitStream )
{
    if( !m_Connected )
        return;

//SanityCheck();
    
    // Read number of records.
    u32 NLives;
    BitStream.ReadU32( NLives, 4 );

    // If there are no records, then we are done.
    if( NLives == 0 )
        return;

    // Read number of bits of data and group seq number.
    xbool Reject   = FALSE;
    u32   Expected = m_LifeGroupSeq;
    u32   GroupSeq;
    u32   Bits;
    BitStream.ReadU32( Bits,     13 );
    BitStream.ReadU32( GroupSeq,  8 );

    // How are we doing?
    if( GroupSeq != m_LifeGroupSeq )
        Reject = TRUE;

    // A little verbosity.
    if( (NLives > 0) || Reject )
    {
        (void)Expected;
        LIFE_LOG( ".\n{%d}-----Begin UnpackLife( PacketSeq %d, LifeGroup 0x%02X )-----\n", m_ClientIndex,g_PacketSeq, GroupSeq );
        LIFE_LOG( "Records %d, ExpectedGroup 0x%02X", NLives, Expected );
        Records = -1;
    }

    // Skip all of the life entries.
    if( Reject )
    {
        s32 Cursor = BitStream.GetCursor();
        BitStream.SetCursor( Cursor + Bits );

        LIFE_LOG( "\n*** LIFE REJECTED ***" );
        LIFE_LOG( "\n-----End UnpackLife-----\n.\n" );

        return;
    }

    // Advance the expect life group sequence number.
    m_LifeGroupSeq = (m_LifeGroupSeq + 1) & 255;

    // Process all the items
    ASSERT( NLives < 100 );
    for( u32 i=0; i<NLives; i++ )
    {
        u32 Slot;
        xbool Create;

        #ifdef SAFE_NETWORK
        BitStream.ReadMarker();
        #endif

        BitStream.ReadU32(Slot,LIFE_SLOT_BITS_SENT);
        BitStream.ReadRangedS32( Create, 0, 2 );

        if( Create == 0 ) // DESTROY
        {     
            LIFE_REC( "D[%03d]", Slot );
            {
                object* pObj = ObjMgr.GetObjectFromSlot( Slot );
                if( !pObj )
                {
                    LIFE_LOG( "@@@@@@ OBJECT DOES NOT EXIST @@@@@@\n" );
                    LIFE_LOG( "@@@@@@ RECOVERING... @@@@@@\n" );
                    continue;
                }
                else
                {
                    LIFE_LOG( "(%6.6s)  ", pObj->GetTypeName() );
                }
            }
            object::id ObjID = ObjMgr.GetIDFromSlot( Slot );
            ObjMgr.DestroyObject( ObjID );
			// Search and destroy updates in the pending packet list
			s32 index,update;

			for (index=0;index < PLAYER_NUM_UPDATE_PACKETS;index++)
			{
				player_packet& PP = s_PlayerPacket[index];
				for (update=0;update<PP.nUpdates;update++)
				{
					if (PP.UpdateSlot[update]==Slot)
					{
						ASSERT(PP.UpdateBitOffset[update]!=-1);
						PP.UpdateBitOffset[update]=-1;
					}
				}
				
			}
        }
        if( Create == 1 ) // CREATE
        {
            LIFE_REC( "C[%03d]", Slot );

            // Read what class the object is
            u32 Type;
            BitStream.ReadU32( Type, LIFE_TYPE_BITS_SENT );
            ObjMgr.CreateInitObject( (object::type)Type, Slot, BitStream );
            {
                object* pObj = ObjMgr.GetObjectFromSlot( Slot );
                LIFE_LOG( "(%6.6s)  ", pObj->GetTypeName() );
            }
        }
        if( Create == 2 ) // MESSAGE
        {
            LIFE_LOG( "M  " );
            MsgMgr.AcceptMsg( BitStream );
        }
    }

    if( NLives > 0 )
    {
        LIFE_LOG( "\n-----End UnpackLife-----\n.\n" );
    }
}

//==============================================================================

static xbool PRIORITY_OFF = TRUE;
//static f32 PriorityDistTable[] = { 65*65, 100*100, 300*300, 450*450, 600*600 };
//static f32 PriorityTable[]     = { 1.0,   0.50f,   0.30f,   0.15f,   0.00f };

f32 update_manager::ComputePriority( const vector3& ObjPos )
{
    (void)ObjPos;
    if( PRIORITY_OFF )
        return 1.0f;
    return 1.0f;
/*
    // Get distance squared to object
    f32 D = (ObjPos - m_ClientPos).LengthSquared();
    s32 NEntries = sizeof(PriorityDistTable) / sizeof(f32);

    // Handle extremes
    if( D < PriorityDistTable[0] ) return 1.0f;
    if( D > PriorityDistTable[NEntries-1] ) return 0.0f;

    // Find entries we are between
    s32 i;
    for( i=0; i<NEntries-1; i++ )
    if( D < PriorityDistTable[i+1] ) break;
    ASSERT( i < (NEntries-1) );

    // Compute parametric value between entries
    f32 F = (D - PriorityDistTable[i]) / (PriorityDistTable[i+1]-PriorityDistTable[i]);

    // Compute priority
    f32 P = PriorityTable[i] + F*(PriorityTable[i+1]-PriorityTable[i]);
    
    return P;
    */
}

//==============================================================================

xbool update_manager::PackObjUpdate( bitstream& BitStream, s32 Slot, s32& Count, conn_packet& Packet )
{
    object* pObj = ObjMgr.GetObjectFromSlot( Slot );
    ASSERT( pObj );

    s32 UpdateSkipCursor = 0;

    // Decide what needs to be written
    u32 DirtyBits = m_Object[Slot].DirtyBits | m_Object[Slot].LostUpdateBits;
    u32 BitsNotPacked;
    if( DirtyBits )
    {
        // Verbosity.
        if( Packet.NUpdates == 0 )
        {
            LIFE_LOG( ".\n{%d}-----Begin PackUpdates( PacketSeq %d )-----", m_ClientIndex,Packet.Seq );
            Records = -1;
        }  
        LIFE_REC( "U[%03d](%6.6s)  ", Slot, pObj->GetTypeName() );

        // Try and add data to stream
        s32 BitCursor = BitStream.GetCursor();

        #ifdef SAFE_NETWORK
        BitStream.WriteMarker();
        #endif

        // Write out info
        BitStream.WriteU32( Slot, LIFE_SLOT_BITS_SENT );

        #ifdef SAFE_NETWORK
		// Extra debug info to make sure object being updated is the correct type
        BitStream.WriteRangedS32( pObj->GetType(), object::TYPE_START_OF_LIST, object::TYPE_END_OF_LIST );
		#endif

        // Extra data for players/bots
        if( Slot < 32 )
        {
            // Reserve room for nbits
            UpdateSkipCursor = BitStream.GetCursor();
            BitStream.WriteU32( 0, 16 );
        }

        s32 SizeCursor = BitStream.GetCursor();

        // Check to see if we're packing data for controlling client
        if( (m_ClientControlledIndex[0] == Slot) || 
            (m_ClientControlledIndex[1] == Slot) )
        {
            BitsNotPacked = DirtyBits;
            ((player_object*)pObj)->OnProvideMoveUpdate( BitStream, BitsNotPacked );
        }
        else
        {
            // Compute priority for the object
            f32 P = ComputePriority( pObj->GetPosition() );

            // Do generic update
            BitsNotPacked = DirtyBits;
            pObj->OnProvideUpdate( BitStream, BitsNotPacked, P );
        }

        // Check if we over-ran bitstream
        if( BitStream.Overwrite() )
        {
            BitStream.SetCursor(BitCursor);
            BitStream.ClearOverwrite();
            LIFE_LOG( "*OVERRUN*  " );
            return( FALSE );
        }

        if( BitStream.GetCursor() > SizeCursor )
        {
            // Update fit in packet alloc a new update and add to packet
            s32 UI = AllocUpdateOnEnd();

            // If no update packets available consider us finished
            if( UI == -1 )
            {
				// BW
				BitStream.SetCursor(BitCursor);
				BitStream.ClearOverwrite();
				// BW
                LIFE_LOG( "*NO UPDATE ENTRY AVAILABLE*  " );
                return( FALSE );
            }
            //ASSERT( UI != -1 );

            m_Update[UI].Bits       = DirtyBits ^ BitsNotPacked;
            m_Update[UI].ObjID      = pObj->GetObjectID();
            m_Update[UI].PacketNext = Packet.FirstUpdate;
            Packet.FirstUpdate      = UI;
            Packet.NUpdates++;

            Count--;

            // Update netinfo for type
            pObj->GetTypeInfoPtr()->NetInfo.AddUpdate( BitStream.GetCursor() - BitCursor );

            if( Slot < 32 )
            {
                // Patch number of bits
                s32 TempC = BitStream.GetCursor();
                BitStream.SetCursor( UpdateSkipCursor );
                BitStream.WriteU32( TempC, 16 );
                BitStream.SetCursor( TempC );
            }
        }
        else
        {
            BitStream.SetCursor(BitCursor);
            BitStream.ClearOverwrite();
        }

        // Clear bits
        m_Object[Slot].DirtyBits      &= BitsNotPacked;
        m_Object[Slot].LostUpdateBits &= BitsNotPacked;
    }

    return( TRUE );
}

//==============================================================================

void update_manager::PackUpdates( conn_packet& Packet, bitstream& BitStream, s32 NUpdatesAllowed )
{
    ASSERT( BitStream.Overwrite() == FALSE );

    // Clear packet of updates
    Packet.NUpdates = 0;
    Packet.FirstUpdate = -1;

    // Reserve room for num items
    s32 HeaderCursor = BitStream.GetCursor();
    BitStream.WriteRangedS32( 0, 0, 100 );

    if( !m_Connected ) 
        return;

    //
    // Add LOCAL player updates
    //
	//****BISCUIT**** What if this fails? Do we need to bail out of the updates below?
    {
        s32 Dummy;
		s32 Slot;

		Slot = m_ClientControlledIndex[0];
        if ( ( Slot != -1) && (m_Object[Slot].ObjSeq!=-1) && (m_Object[Slot].LifeStage>=1) )
            PackObjUpdate( BitStream, Slot, Dummy, Packet );

		Slot = m_ClientControlledIndex[1];
        if ( ( Slot != -1) && (m_Object[Slot].ObjSeq!=-1) && (m_Object[Slot].LifeStage>=1) )
            PackObjUpdate( BitStream, Slot, Dummy, Packet );
    }

    //
    // Add player updates
    //
    {
        // Loop through players and add a safe amount
        s32 NPlayerUpdatesAllowed = PLAYER_UPDATES_PER_PACKET - Packet.NUpdates;
        s32 RoundTrip = PLAYER_UPDATE_CURSOR;
        while( NPlayerUpdatesAllowed )
        {
            s32 Slot = PLAYER_UPDATE_CURSOR;
        
            // Check if object exists on server AND client
            if( (m_Object[Slot].ObjSeq!=-1) && (m_Object[Slot].LifeStage>=1) )
            {
                if( !PackObjUpdate( BitStream, Slot, NPlayerUpdatesAllowed, Packet ) )
                    break;
            }

            // Move to next object and watch if we visited all objects
            PLAYER_UPDATE_CURSOR = (PLAYER_UPDATE_CURSOR+1) % 32;
            if( PLAYER_UPDATE_CURSOR == RoundTrip )
                break;
        }
    }

    //
    // Add object updates
    //
    if( !BitStream.Overwrite() )
    {
        // Loop through objects and add as many updates as we can
        s32 RoundTrip = m_UpdateCursor;
        while( NUpdatesAllowed )
        {
            s32 Slot = m_UpdateCursor;
        
            // Check if object exists on server AND client
            if( (m_Object[Slot].ObjSeq!=-1) && (m_Object[Slot].LifeStage>=1) )
            {
                if( !PackObjUpdate( BitStream, Slot, NUpdatesAllowed, Packet ) )
                    break;
            }

            // Move to next object and watch if we visited all objects
            m_UpdateCursor += 1;
            if( m_UpdateCursor >= obj_mgr::MAX_SERVER_OBJECTS )
                m_UpdateCursor = 32;
            if( m_UpdateCursor == RoundTrip )
                break;
        }
    }

    //ASSERT( Packet.NUpdates <= 100 );

    // Fill in header and return
    s32 EndCursor = BitStream.GetCursor();
    BitStream.SetCursor(HeaderCursor);
    BitStream.WriteRangedS32( Packet.NUpdates, 0, 100 );
    BitStream.SetCursor(EndCursor);

    // Verbosity.
    if( Packet.NUpdates )
    {
        LIFE_LOG( "\n-----End PackUpdates( Updates %d )-----\n.\n", Packet.NUpdates );
    }  
}

//==============================================================================

void FreePlayerPacket( void )
{
    s_PlayerPacket[s_PlayerPacketIndex].nUpdates = 0;
    s_PlayerPacketIndex = (s_PlayerPacketIndex+1)%PLAYER_NUM_UPDATE_PACKETS;
}

//==============================================================================

void ProcessPlayerPackets( xbool Flush )
{
    s32 i;
    s32 nUpdates = 0;

    while( 1 )
    {
        player_packet& PP = s_PlayerPacket[s_PlayerPacketIndex];
        if( PP.nUpdates == 0 )
            break;

        // Get time packet has waited
        f32 SecWaited = (f32)x_TicksToSec( x_GetTime() - PP.ArrivalTime );

        s32 Start = PP.nUpdatesProcessed;
        s32 NToUpdate = PP.nUpdates - PP.nUpdatesProcessed;
        ASSERT( (NToUpdate>0) && (NToUpdate<=PP.nUpdates) );

        for(i=Start; i<Start+NToUpdate; i++)
        {
			if (PP.UpdateBitOffset[i] != -1)
			{
				object::id ObjID( PP.UpdateSlot[i], -1 );
				PP.BitStream.SetCursor( PP.UpdateBitOffset[i] );

				ObjMgr.AcceptUpdate( ObjID, PP.BitStream, PP.SecInPast + SecWaited );

				ASSERT( PP.BitStream.GetCursor() == PP.UpdatePostCursor[i] );
			}

            PP.nUpdatesProcessed++;
            nUpdates++;

            if( (!Flush) && (nUpdates==PLAYER_UPDATES_PER_FRAME) )
                break;
        }

        if( PP.nUpdatesProcessed == PP.nUpdates )
        {
            FreePlayerPacket();
        }

        if( (!Flush) && (nUpdates==PLAYER_UPDATES_PER_FRAME) )
            break;
    }
}

//==============================================================================

s32 AllocPlayerPacket( bitstream& BitStream, f32 SecInPast )
{
    s32 i = s_PlayerPacketIndex;
    while( 1 )
    {
        if( s_PlayerPacket[i].nUpdates == 0 )
        {
            s32 Size = BitStream.GetNBytes();
            ASSERT( Size < 1024 );
            x_memcpy( s_PlayerPacket[i].Data, BitStream.GetDataPtr(), Size );
            s_PlayerPacket[i].BitStream.Init( s_PlayerPacket[i].Data, Size );
            s_PlayerPacket[i].nUpdates = 0;
            s_PlayerPacket[i].nUpdatesProcessed = 0;
            s_PlayerPacket[i].SecInPast = SecInPast;
            s_PlayerPacket[i].ArrivalTime = x_GetTime();
            return i;
        }

        i = (i+1)%PLAYER_NUM_UPDATE_PACKETS;

        if( i==s_PlayerPacketIndex )
        {
            // Flush all player updates since we've overflowed
            ProcessPlayerPackets(TRUE);

            return AllocPlayerPacket( BitStream, SecInPast );
        }
    }

    return s_PlayerPacketIndex;
}

//==============================================================================

void update_manager::UnpackUpdates( bitstream& BitStream, f32 SecInPast )
{
    if( !m_Connected ) return;

//SanityCheck();

    ASSERT( PLAYER_NUM_UPDATE_PACKETS <= MAX_PLAYER_PACKETS );

    // Read number of items
    s32 NUpdates;
    BitStream.ReadRangedS32( NUpdates, 0, 100 );
    ASSERT( NUpdates < 100 );

    s32 PlayerPacketI = -1;

    // Verbosity.
    if( NUpdates > 0 )
    {
        LIFE_LOG( ".\n{%d}-----Begin UnpackUpdates( PacketSeq %d, Updates %d )-----", m_ClientIndex,g_PacketSeq, NUpdates );
        Records = -1;
    }

#ifdef SAFE_NETWORK
    s32 Recent[3] = {-1,-1,-1};
    s32 RType [3] = { 0, 0, 0};
#endif

    // Process all the updates
    for( s32 i=0; i<NUpdates; i++ )
    {
        u32 Slot;

        #ifdef SAFE_NETWORK
        BitStream.ReadMarker();
        #endif

        BitStream.ReadU32( Slot, LIFE_SLOT_BITS_SENT );
        object::id ObjID( Slot, -1 );

#ifdef SAFE_NETWORK
        object::type ReadType;
        object::type ObjType;
        object*      pObject;
        BitStream.ReadRangedS32( (s32&)ReadType, object::TYPE_START_OF_LIST, object::TYPE_END_OF_LIST );
        pObject = ObjMgr.GetObjectFromID( ObjID );
        ASSERT( pObject );
        ObjType = pObject->GetType();
        ASSERT( ObjType == ReadType );

        Recent[0] = Recent[1];
        Recent[1] = Recent[2];
        Recent[2] = (s32)Slot;
        RType[0]  = RType[1];
        RType[1]  = RType[2];
        RType[2]  = ReadType;
#endif

        LIFE_REC( "U[%03d]", Slot );
        {
            object* pObj = ObjMgr.GetObjectFromSlot( Slot );
            if( pObj )
            {
                LIFE_LOG( "(%6.6s)  ", pObj->GetTypeName() );
#ifdef SAFE_NETWORK
                RType[2] = (s32)pObj->GetType();
#endif
            }
            else
            {
                LIFE_LOG( "---------------------------\n" );
                LIFE_LOG( "---------------------------\n" );
                LIFE_LOG( "---------------------------\n" );
                LIFE_LOG( "---------------------------\n" );
                LIFE_LOG( "---------------------------\n" );
                LIFE_LOG( "---------------------------\n" );
                LIFE_LOG( "---------------------------\n" );
                LIFE_LOG( "---------------------------\n" );
#ifdef SAFE_NETWORK
                x_DebugLog( "Received update for empty object slot!\n"
                            "PacketSeq %d, Updates %d\n"
                            "Last three updates in this packet:\n"
                            "(Slot %d, Type %d)\n"
                            "(Slot %d, Type %d)\n"
                            "(Slot %d, Type %d)\n",
                            g_PacketSeq, NUpdates,
                            Recent[0], RType[0],
                            Recent[1], RType[1],
                            Recent[2], RType[2] );
#endif
                ASSERT( pObj );
            }
        }

        if( Slot < 32 )
        {
            // Player or bot

#ifdef SAFE_NETWORK
            if( BitStream.GetCursorRemaining() < 16 )
            {
                x_DebugLog( "Reading past end of packet!\n"
                            "Assertion failure should follow.\n"
                            "PacketSeq %d, Updates %d\n"
                            "Last three updates in this packet:\n"
                            "(Slot %d, Type %d)\n"
                            "(Slot %d, Type %d)\n"
                            "(Slot %d, Type %d)\n",
                            g_PacketSeq, NUpdates,
                            Recent[0], RType[0],
                            Recent[1], RType[1],
                            Recent[2], RType[2] );
                ASSERT( FALSE );
            }
#endif
            // Get size of update data
            u32 Cursor;
            BitStream.ReadU32( Cursor, 16 );

            if( PlayerPacketI == -1 )
                PlayerPacketI = AllocPlayerPacket( BitStream, SecInPast );

            ASSERT( PlayerPacketI != -1 );

            player_packet& PP = s_PlayerPacket[PlayerPacketI];
            PP.UpdateSlot[PP.nUpdates] = Slot;
            PP.UpdateBitOffset[PP.nUpdates] = BitStream.GetCursor();
            PP.UpdatePostCursor[PP.nUpdates] = Cursor;
            PP.nUpdates++;

            // Skip past update data
            BitStream.SetCursor( Cursor );

            //ObjMgr.AcceptUpdate( ObjID, BitStream, SecInPast );
            //ASSERT( BitStream.GetCursor() == Cursor );
        }
        else
        {
            ObjMgr.AcceptUpdate( ObjID, BitStream, SecInPast );
        }
    }

    // Verbosity.
    if( NUpdates > 0 )
    {
        LIFE_LOG( "\n-----End UnpackUpdates( Updates %d )-----\n.\n", NUpdates );
    }
}

//==============================================================================

void update_manager::PacketAck( conn_packet& Packet, xbool Arrived )
{
    s32 i;
    s32 I;

    if( !m_Connected )
        return;

    /*
    if( (Packet.NLife > 0) || (!Arrived) )
        DumpLifeState();
    */

SanityCheck();

    // Loop through life attached to packet
    if( Arrived )
    {
        if( Packet.NLife > 0 )
        {
            LIFE_LOG( ".\n-----Begin Packet ACK( PacketSeq %d )-----", Packet.Seq );
            Records = -1;
        }
        else
        if( Packet.NUpdates > 0 )
        {
            LIFE_LOG( ".\n-----Packet ACK( PacketSeq %d )-----\n.\n", Packet.Seq );
        }
        /*
        if( Packet.NLife > 0 )
        {
            LIFE_LOG( ".\n-----PacketAck TRUE-----\n" );
            LIFE_LOG( "PacketAck( PacketSeq %d, TRUE ):\n"                  // line continues...
                      " (%03d)[%04d]\n (%03d)[%04d]\n (%03d)[%04d]\n (%03d)[%04d]\n" // line continues...
                      " (%03d)[%04d]\n (%03d)[%04d]\n (%03d)[%04d]\n (%03d)[%04d]\n",
                      Packet.Seq, 
                      Packet.LifeID[0], Packet.LifeID[0] == -1 ? -1 : m_Life[Packet.LifeID[0]].ObjID.Slot,
                      Packet.LifeID[1], Packet.LifeID[1] == -1 ? -1 : m_Life[Packet.LifeID[1]].ObjID.Slot,
                      Packet.LifeID[2], Packet.LifeID[2] == -1 ? -1 : m_Life[Packet.LifeID[2]].ObjID.Slot,
                      Packet.LifeID[3], Packet.LifeID[3] == -1 ? -1 : m_Life[Packet.LifeID[3]].ObjID.Slot,
                      Packet.LifeID[4], Packet.LifeID[4] == -1 ? -1 : m_Life[Packet.LifeID[4]].ObjID.Slot,
                      Packet.LifeID[5], Packet.LifeID[5] == -1 ? -1 : m_Life[Packet.LifeID[5]].ObjID.Slot,
                      Packet.LifeID[6], Packet.LifeID[6] == -1 ? -1 : m_Life[Packet.LifeID[6]].ObjID.Slot,
                      Packet.LifeID[7], Packet.LifeID[7] == -1 ? -1 : m_Life[Packet.LifeID[7]].ObjID.Slot );
        }
        */
        for( i=0; i<Packet.NLife; i++ )
        if( Packet.LifeID[i] != -1 )
        {
            I = Packet.LifeID[i];
            if( m_Life[I].PacketSeq == Packet.Seq )
            {
                if( m_Life[I].Create == 0 )
                {
                    LIFE_REC( "+D[%03d]  ", m_Life[I].ObjID.Slot );
                }

                // If this was a create
                if( m_Life[I].Create == 1 )
                {
                    // Notify object tracker that object exists on client now
                    if( m_Object[ m_Life[I].ObjID.Slot ].ObjSeq == m_Life[I].ObjID.Seq )
                    {
                        m_Object[ m_Life[I].ObjID.Slot ].LifeStage = 1;
                        //m_Object[ m_Life[I].ObjID ].DirtyBits = 0xFFFFFFFF;
                        LIFE_REC( "+C[%03d]  ", m_Life[I].ObjID.Slot );
                    }
                    else
                    {
                        LIFE_REC( "+d[%03d]  ", m_Life[I].ObjID.Slot );
                    }
                }

                DeallocLife(I);
            }
            else
            {
                LIFE_REC( "?2[***]  " );
            }
        }

        if( Packet.NLife > 0 )
        {
            LIFE_LOG( "\n-----End Packet ACK -----\n.\n" );
        }
    }
    else if( Packet.LifeGroupSeq != 256 )
    {
        LIFE_LOG( ".\n{%d}*****Begin Packet NACK( PacketSeq %d )*****", m_ClientIndex, Packet.Seq );
        Records = -1;
        /*
        LIFE_LOG( ".\n**** PacketAck FALSE ****\n" );
        LIFE_LOG( "PacketAck( PacketSeq %d, TRUE ):\n"                  // line continues...
                  " (%03d)[%04d]\n (%03d)[%04d]\n (%03d)[%04d]\n (%03d)[%04d]\n" // line continues...
                  " (%03d)[%04d]\n (%03d)[%04d]\n (%03d)[%04d]\n (%03d)[%04d]\n",
                  Packet.Seq, 
                  Packet.LifeID[0], Packet.LifeID[0] == -1 ? -1 : m_Life[Packet.LifeID[0]].ObjID.Slot,
                  Packet.LifeID[1], Packet.LifeID[1] == -1 ? -1 : m_Life[Packet.LifeID[1]].ObjID.Slot,
                  Packet.LifeID[2], Packet.LifeID[2] == -1 ? -1 : m_Life[Packet.LifeID[2]].ObjID.Slot,
                  Packet.LifeID[3], Packet.LifeID[3] == -1 ? -1 : m_Life[Packet.LifeID[3]].ObjID.Slot,
                  Packet.LifeID[4], Packet.LifeID[4] == -1 ? -1 : m_Life[Packet.LifeID[4]].ObjID.Slot,
                  Packet.LifeID[5], Packet.LifeID[5] == -1 ? -1 : m_Life[Packet.LifeID[5]].ObjID.Slot,
                  Packet.LifeID[6], Packet.LifeID[6] == -1 ? -1 : m_Life[Packet.LifeID[6]].ObjID.Slot,
                  Packet.LifeID[7], Packet.LifeID[7] == -1 ? -1 : m_Life[Packet.LifeID[7]].ObjID.Slot );
        LIFE_LOG( "*** Resetting LifeGroup to 0x%02X\n", Packet.LifeGroupSeq );

        DumpLifeState();
        */

        SanityCheck();

        // We need to resend the life records which were lost or sent out of 
        // order.  Simply set the PacketSeq for each life entry to be -1, and 
        // it will get sent again.  Note that this should be applied to all 
        // life entries which are in AND AFTER the lost packet IF at least one
        // of the entries was waiting for confirmation of THAT packet.

        if( m_Life[ Packet.LifeID[0] ].PacketSeq == Packet.Seq )
        {
            // Reset the life group sequence number.
            m_LifeGroupSeq = Packet.LifeGroupSeq;

            // Clobber 'em all from this point on.
            s32 I = Packet.LifeID[0];

            while( I != -1 )
            {
                s32 NI = m_Life[I].Next;

                // You are no longer in a packet, resend yerself when you can.
                m_Life[I].PacketSeq = -1;

                // If it is a create, look to see if the destroy is pending
                // in the list.  If so, nuke 'em both completely.
                if( m_Life[I].Create == 1 )
                {
                    LIFE_REC( "-C[%03d]  ", m_Life[I].ObjID.Slot );

                    // Look for create/destroy match and remove
                    s32 DI = m_Life[I].Next;
                    while( DI != -1 )
                    {
                        if( (m_Life[DI].Create     == 0) &&
                            (m_Life[DI].ObjID.Slot == m_Life[I].ObjID.Slot) &&
                            (m_Life[DI].ObjID.Seq  == m_Life[I].ObjID.Seq) )
                        {
                            LIFE_REC( "~C[%03d]  ", DI );
                            LIFE_REC( "~D[%03d]  ", I );
                            if( NI == DI )
                                NI = m_Life[DI].Next;
                            DeallocLife(DI);
                            DeallocLife(I);
                            break;
                        }

                        DI = m_Life[DI].Next;
                    }
                }

                if( m_Life[I].Create == 0 )
                {
                    LIFE_REC( "-D[%03d]  ", m_Life[I].ObjID.Slot );
                }

                I = NI;
            }
        }
        /*
        for( s32 i=0; i<Packet.NLife; i++ )
        if( Packet.LifeID[i] != -1 )
        {
            I = Packet.LifeID[i];
            ASSERT( m_Life[I].PacketSeq == Packet.Seq );

            // Tell this entry it is no longer in packet
            m_Life[I].PacketSeq = -1;

            if( m_Life[I].Create )
            {
                // Look for create/destroy match and remove
                s32 DI = m_Life[I].Next;
                while( DI != -1 )
                {
                    if( (m_Life[DI].Create     == FALSE) &&
                        (m_Life[DI].ObjID.Slot == m_Life[I].ObjID.Slot) &&
                        (m_Life[DI].ObjID.Seq  == m_Life[I].ObjID.Seq) )
                    {
                        if( m_Life[DI].PacketSeq==-1 )
                        {
                            LIFE_LOG("      removing %1d and %1d due to drop/create-destroy\n",I,DI);
                            DeallocLife(DI);
                            DeallocLife(I);
                            break;
                        }
                        else
                        {
                            LIFE_LOG("      can't remove %1d and %1d due to drop/create-destroy\n",I,DI);
                            break;
                        }
                    }

                    DI = m_Life[DI].Next;
                }
            }
        }
        */
        SanityCheck();
        if( Packet.NLife || Packet.NUpdates )
        {
            LIFE_LOG( "\n*****End Packet NACK*****\n.\n" );
        }
    }

    Packet.NLife = 0;

    // Loop through updates attached to packet
    I = Packet.FirstUpdate;
    while( I != -1 )
    {
        s32 Next = m_Update[I].PacketNext;

        if( Arrived == FALSE )
        {
            s32 ObjSlot = m_Update[I].ObjID.Slot;

            // Could not find replacement so update lost bits
            if( m_Object[ObjSlot].ObjSeq == m_Update[I].ObjID.Seq )
                m_Object[ObjSlot].LostUpdateBits |= m_Update[I].Bits;
        }

        DeallocUpdate(I);
        I = Next;
    }

    Packet.FirstUpdate = -1;
    Packet.NUpdates    = 0;

SanityCheck();

    /*
    if( (Packet.NLife > 0) || (!Arrived) )
    {
        LIFE_LOG( ".\n" );
        DumpLifeState();
    }
    */
}

//==============================================================================

void update_manager::UpdateObject( object::id ObjID, xbool MissionLoaded, object* pObj, xbool Syncing )
{
    if( !m_Connected ) 
		return;

    // Check if object already exists
    if( m_Object[ObjID.Slot].ObjSeq == ObjID.Seq )
    {
        if( pObj )
        {
            // Accumulate dirty bits
            m_Object[ObjID.Slot].DirtyBits |= pObj->GetDirtyBits();
          //m_Object[ObjID.Slot].DirtyBits |= 0xFFFFFFFF;
        }

        return;
    }

    // Object needs to be created or destroyed
    if( pObj == NULL )
    {
        ASSERT( ObjID.Seq == -1 );

        // Destroy current object
        if( m_Object[ObjID.Slot].ObjSeq != -1 )
        {
            SendDestroy( ObjID.Slot );
            m_Object[ObjID.Slot].ObjSeq    = -1;
            m_Object[ObjID.Slot].DirtyBits =  0;
            m_Object[ObjID.Slot].LifeStage = -1;
        }
    }
    else
    {
        ASSERT( ObjID.Seq != -1 );

        // Destroy current object
        if( m_Object[ObjID.Slot].ObjSeq != -1 )
        {
            SendDestroy( ObjID.Slot );
            m_Object[ObjID.Slot].ObjSeq    = -1;
            m_Object[ObjID.Slot].DirtyBits =  0;
            m_Object[ObjID.Slot].LifeStage = -1;
        }

        // Check if object is already on client side due to mission load
        if( MissionLoaded )
        {
            m_Object[ObjID.Slot].ObjSeq = ObjID.Seq;
            m_Object[ObjID.Slot].DirtyBits = 0xFFFFFFFF;
          //m_Object[ObjID.Slot].DirtyBits = 0x00000000;
            m_Object[ObjID.Slot].LifeStage = 1;
        }
        else
        {
			// Here we are going to send over a create.  If the client is 
			// currently "syncing", then do NOT send over "volatile" objects.
            s32 Type = pObj->GetType();
            switch( Type )
            {
                case object::TYPE_DISK:
                case object::TYPE_PLASMA:
                case object::TYPE_BULLET:
                case object::TYPE_GRENADE:
                case object::TYPE_MORTAR:
                case object::TYPE_LASER:
                case object::TYPE_BLASTER:
                case object::TYPE_MISSILE:
                case object::TYPE_GENERICSHOT:
                case object::TYPE_SHRIKESHOT:
                case object::TYPE_BOMBERSHOT:
                case object::TYPE_BOMB:
                case object::TYPE_CORPSE:
                case object::TYPE_PARTICLE_EFFECT:
                case object::TYPE_DEBRIS:
                case object::TYPE_BUBBLE:
                    break;

				default:
					Syncing = FALSE;
					break;
			}

			if (!Syncing)
			{
				// Create new object
				m_Object[ObjID.Slot].ObjSeq = ObjID.Seq;
				m_Object[ObjID.Slot].LifeStage = 0; // waiting for create on client
				SendCreate( ObjID.Slot );
				m_Object[ObjID.Slot].DirtyBits = 0;
			}
        }
    }
}

//==============================================================================

xbool update_manager::IsConnected( void )
{
    return m_Connected;
}

//==============================================================================

void update_manager::SanityCheck( void )
{
    return;

    // Search main life lists
    s32 I;
    s32 Loops;

    // Search life free list
    I = m_FirstFreeLife;
    Loops = 0;
    while( I != -1 )
    {
        VERIFY( Loops++ < 1000 );
        VERIFY( m_Life[I].Next != I );
        VERIFY( m_Life[I].Prev != I );
        if( m_Life[I].Next != -1 ) 
            VERIFY( m_Life[ m_Life[I].Next ].Prev == I );
        if( m_Life[I].Prev != -1 ) 
            VERIFY( m_Life[ m_Life[I].Prev ].Next == I );
        I = m_Life[I].Next;
    }

    // Search life main list
    I = m_FirstLife;
    Loops = 0;
    while( I != -1 )
    {
        VERIFY( Loops++ < 1000 );
        VERIFY( m_Life[I].Next != I );
        VERIFY( m_Life[I].Prev != I );
        
        if( m_Life[I].Next != -1 ) 
            VERIFY( m_Life[ m_Life[I].Next ].Prev == I );
        
        if( m_Life[I].Prev != -1 ) 
            VERIFY( m_Life[ m_Life[I].Prev ].Next == I );

        I = m_Life[I].Next;
    }

    // If first life is not in packet then no lives should be in packets
    I = m_FirstLife;
    if( I != -1 )
    {
        if( m_Life[I].PacketSeq == -1 )
        {
            while( I != -1 )
            {
                VERIFY( m_Life[I].PacketSeq == -1 );
                I = m_Life[I].Next;
            }
        }
    }

    // If create exists be sure destroy does not
    I = m_FirstLife;
    while( I != -1 )
    {
        if( m_Life[I].Create && (m_Life[I].PacketSeq==-1))
        {
            s32 IP = m_Life[I].Next;
            while( IP != -1 )
            {
                if( (!m_Life[IP].Create) &&
                    (m_Life[IP].ObjID.Seq == m_Life[I].ObjID.Seq) )
                {
                    VERIFY(FALSE);
                }

                IP = m_Life[IP].Next;
            }
        }

        I = m_Life[I].Next;
    }
}

//==============================================================================

void update_manager::SetClientControlled( s32 PlayerIndex0, s32 PlayerIndex1 )
{
    m_ClientControlledIndex[0] = PlayerIndex0;
    m_ClientControlledIndex[1] = PlayerIndex1;
}

//==============================================================================

void update_manager::SetClientPos( s32 ID, const vector3& Pos )
{
    m_ClientPos[ID] = Pos;
}

//==============================================================================

void update_manager::SendMsg( const msg& Msg )
{
    if( !m_Connected )
        return;

SanityCheck();

    s32 I = AllocLifeOnEnd();
    if( I == -1 )
    {
        m_Connected = FALSE;
        x_DebugMsg("*************************************\n");
        x_DebugMsg("UpdateManger: No life entries available\n");
        x_DebugMsg("*************************************\n");
        return;
    }

    m_Life[I].Create  = 2;   // MESSAGE
    m_Life[I].Message = Msg;

    m_Life[I].NTimesPacked = 0;

SanityCheck();

    //LIFE_LOG( "%4d SendMsg(%d) %d\n", m_Life[I].Seq, I, Msg.Index );
}

//==============================================================================

void update_manager::ShowLifeDelay( s32 L )
{
    x_printfxy( 12, L, "R:%5.1f C %d",
                       m_RecentLifeDelay / m_LifeDelayCounter,
                       GetNumLifeEntries() );
}

//==============================================================================

void update_manager::DumpLifeState( void )
{
    char* Word[] = { "Destroy", "Create ", "Message", };
    LIFE_LOG( "{%d} Current life queue:\n", m_ClientIndex );
    s32 i = m_FirstLife;
    while( i != -1 )
    {
        LIFE_LOG( "  [%03d] (%03d)  Op:%s  Packet %d  Seq %d LifeStage %d", 
                  m_Life[i].ObjID.Slot,
                  i, Word[m_Life[i].Create],
                  m_Life[i].PacketSeq,
				  m_Life[i].Seq,
				  m_Object[m_Life[i].ObjID.Slot].LifeStage);
        if( m_Life[i].Create == 1 )
        {
            LIFE_LOG( "  Type:%s\n",
                ObjMgr.GetTypeInfo( (object::type)m_Life[i].ClassType )->pTypeName );
        }
        else
        {
            LIFE_LOG( "\n" );
        }
        
        i = m_Life[i].Next;
    }
}

//==============================================================================

void update_manager::DumpObjectState( void )
{
    /*
    s32 i;

    LIFE_LOG( "-----Begin Current Object List-----\n" );

    if( tgl.ServerPresent )
    {
        for( i = tgl.pServer->m_MissionLoadedIndex; i < obj_mgr::MAX_SERVER_OBJECTS; i++ )
        {
            LIFE_LOG( " [%04d]  LifeStage:%02d", m_Object[i].LifeStage ); 

            object* pObject = ObjMgr.GetObjectFromSlot( i );
            if( pObject )
            {
                LIFE_LOG( "  Type:%s", pObject->GetTypeName() );
            }
            LIFE_LOG( "\n" );
        }
    }
    else
    {
        for( i = 32; i < obj_mgr::MAX_SERVER_OBJECTS; i++ )
        {
            object* pObject = ObjMgr.GetObjectFromSlot( i );
            if( pObject )
            {
                if( pObject->GetType() <= object::TYPE_BEACON )
                    continue;
                if( pObject->GetAttrBits() & object::ATTR_SOLID_STATIC )
                    continue;
                if( pObject->GetType() != object::TYPE_PICKUP )
                    continue;  

                LIFE_LOG( " [%04d]  Type:%s\n", pObject->GetTypeName() );
            }
        }
    }

    LIFE_LOG( "-----End Current Object List-----\n.\n" );
    */
}

//==============================================================================

