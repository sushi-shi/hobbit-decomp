//=========================================================================
//
//  MoveManager.cpp
//
//=========================================================================

#include "x_files.hpp"
#include "movemanager.hpp"
#include "connmanager.hpp"
#include "globals.hpp"
#include "objects/player/playerobject.hpp"



//=========================================================================
// Recieved move queing data + functions
//=========================================================================

#define MAX_RECIEVED_MOVES  10
#define MAX_PLAYERS         32

static s32 s_MoveQueueMax = 0 ;

// Simple class to hold and process a fifo queue list of player moves
class move_queue
{
public:
    // Structures
    struct move
    {
        player_object::move Move ;          // Player move
        s32                 Seq ;           // Sequence number
    } ;
   
private:
    // Data
    move    m_Moves[MAX_RECIEVED_MOVES] ;   // List of items
    s32     m_Front ;                       // Front of queue
    s32     m_Back ;                        // Back of queue
    s32     m_NItems ;                      // # of items currently in queue

public:
    // Constructor
    move_queue()
    {
        // Init to empty
        Clear() ;
    }

    // Make the queue empty
    void Clear( void )
    {
        m_Front = m_Back = m_NItems = 0 ;
    }

    // Adds move to back of queue. If full, pops the last move before hand
    s32 PushMove( const s32 PlayerIndex, const player_object::move& Move, const s32 Seq )
    {
        // Must be a player slot!
        ASSERT(PlayerIndex >= 0) ;
        ASSERT(PlayerIndex < MAX_PLAYERS) ;

        // If full, pop the last move to make room for this one
        if (m_NItems == MAX_RECIEVED_MOVES)
        {
            ASSERT(m_Back == m_Front) ;
            PopMove(PlayerIndex) ;
        }

        // Should never be full here...
        ASSERT(m_NItems != MAX_RECIEVED_MOVES) ;

        // Add to back of queue
        m_Moves[m_Back].Move = Move ;
        m_Moves[m_Back].Seq  = Seq ;

        // Update queue info
        m_Back = (m_Back + 1) % MAX_RECIEVED_MOVES ;
        m_NItems++ ;

        // Update global max stat
        s_MoveQueueMax = MAX(s_MoveQueueMax, m_NItems) ;

        return m_NItems ;
    }

    // Gets current move from front of queue (if any) and applies to player if they exist in the world
    s32 PopMove( const s32 PlayerIndex )
    {
        // Must be a player slot!
        ASSERT(PlayerIndex >= 0) ;
        ASSERT(PlayerIndex < MAX_PLAYERS) ;

        // Empty?
        if (m_NItems == 0)
        {
            ASSERT(m_Back == m_Front) ;
            return m_NItems ;
        }

        // Lookup the player and apply the current move at the front of the queue if the player exists
        object::id     ObjID( PlayerIndex, -1 );
        player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID(ObjID) ;
        if ((pPlayer) && (pPlayer->GetType() == object::TYPE_PLAYER))
        {
            pPlayer->ReceiveMove(m_Moves[m_Front].Move, m_Moves[m_Front].Seq) ;
            pPlayer->SetMovesRemaining(m_NItems-1) ;
        }
        else
        {
            // Player does not exists, so clear the queue
            Clear() ;
        }

        // Update queue info
        m_Front = (m_Front + 1) % MAX_RECIEVED_MOVES ;
        m_NItems-- ;
        return m_NItems ;
    }

    // Updates # of player moves remaining
    s32 UpdatePlayerMovesRemaing( const s32 PlayerIndex )
    {
        // Must be a player slot!
        ASSERT(PlayerIndex >= 0) ;
        ASSERT(PlayerIndex < MAX_PLAYERS) ;

        // Lookup the player and apply the current move at the front of the queue if the player exists
        object::id     ObjID( PlayerIndex, -1 );
        player_object* pPlayer = (player_object*)ObjMgr.GetObjectFromID(ObjID) ;
        if ((pPlayer) && (pPlayer->GetType() == object::TYPE_PLAYER))
            pPlayer->SetMovesRemaining(m_NItems) ;
        else
        {
            // Player does not exists, so clear the queue
            Clear() ;
        }

        return m_NItems ;
    }
} ;

// Queues (1 for each player)
static move_queue  s_MoveQueue[MAX_PLAYERS] ;


// Loops through all players and processes 1 move at a time, until all moves are processed
void ProcessReceivedMoves( void )
{
    s32 i ;
    s32 MovesRemaining = 0 ;

    // Update player moves remaining for better target lock prediction
    for (i = 0 ; i < MAX_PLAYERS ; i++)
        MovesRemaining += s_MoveQueue[i].UpdatePlayerMovesRemaing(i) ;

    // Loop until all moves have been processed
    while(MovesRemaining)
    {
        // Clear count
        MovesRemaining = 0 ;

        // Loop through all queues and apply a move per player
        for (i = 0 ; i < MAX_PLAYERS ; i++)
            MovesRemaining += s_MoveQueue[i].PopMove(i) ;
    }
}

// Adds a move to the corresponding players queue
void ReceiveMove( s32 PlayerIndex, const player_object::move& Move, const s32 Seq )
{
    // Must be a player slot!
    ASSERT(PlayerIndex >= 0) ;
    ASSERT(PlayerIndex < MAX_PLAYERS) ;

    // Add to player queue
    s_MoveQueue[PlayerIndex].PushMove(PlayerIndex, Move, Seq) ;
}


//=========================================================================
// Functions
//=========================================================================

move_manager::move_manager( void )
{
}

//=========================================================================

move_manager::~move_manager( void )
{
}

//=========================================================================

void move_manager::Reset( void )
{
    s32 i;

    // Hook up all moves entries into free list
    for( i=0; i<MAX_MOVE_ENTRIES; i++ )
    {
        m_Move[i].Info.Clear() ;
        m_Move[i].MoveSeq = -1;
        m_Move[i].ObjSlot = -1;
        m_Move[i].NTimesSent = 0;
    }
    m_MoveSeq = 0;
    m_LastReceivedSeq = -1;

    m_NMovesAllocated = 0;
    m_ReadPos = -1;
    m_WritePos = 0;
    m_Cursor = -1;
}

//=========================================================================

void move_manager::Init( void )
{
    Reset();

    // Consider connection open for now
    m_Connected = TRUE;
}

//=========================================================================

void move_manager::Kill( void )
{
}

//=========================================================================

void move_manager::SendMove( s32 ObjSlot, player_object::move& Move )
{
    //x_printf("SendMove\n");
    if( !m_Connected ) return;
    if( m_NMovesAllocated == MAX_MOVE_ENTRIES )
    {
        // Allow overwriting of moves
        //m_Connected = FALSE;
        //return;
        x_DebugMsg("*************************************\n");
        x_DebugMsg("MoveManager: Ran out of moves!!!\n");
        x_DebugMsg("*************************************\n");
    }
    //ASSERT( m_WritePos != m_ReadPos );
    
    s32 I = m_WritePos;
    m_WritePos = (m_WritePos+1)%MAX_MOVE_ENTRIES;

    m_NMovesAllocated++;

    // Move read position if write position has caught up
    if( m_ReadPos==-1 )
        m_ReadPos = 0;

    if( m_ReadPos==m_WritePos )
    {
        m_NMovesAllocated--;
        m_ReadPos = (m_ReadPos+1)%MAX_MOVE_ENTRIES;
    }

    #ifdef MOVE_MGR_DEBUG_NET
        Move.SeqID = m_MoveSeq ;
    #endif

    // Fill out move info
    m_Move[I].Info       = Move;
    m_Move[I].ObjSlot    = ObjSlot;
    m_Move[I].MoveSeq    = m_MoveSeq;
    m_Move[I].NTimesSent = 0;
    m_MoveSeq++;
    //x_printf("SendMove %1d\n",m_MoveSeq);
}

//=========================================================================

void move_manager::PackMoves( conn_packet& Packet, bitstream& BitStream, s32 NMovesAllowed )
{
    s32 NMI = 0;
    s32 MI[CONN_MOVES_PER_PACKET];
    s32 NMovesWritten = 0;

    // Clear packet of move info
    for( s32 i=0; i<CONN_MOVES_PER_PACKET; i++ )
    {
        Packet.MoveID[i] = -1;
        Packet.MoveSeq[i] = -1;
    }
    Packet.NMoves = 0;

    // Find moves to send
    s32 I = m_ReadPos;
    if( I==-1 ) I = 0;
    while( NMI < NMovesAllowed )
    {
        // Find a move
        while( I != m_WritePos )
        {
            if( m_Move[I].NTimesSent < 2 )
                break;
            I = (I+1)%MAX_MOVE_ENTRIES;
        }
        if( I==m_WritePos ) break;

        // Keep this move
        ASSERT( m_Move[I].ObjSlot != -1 );
        MI[NMI] = I;
        I = (I+1)%MAX_MOVE_ENTRIES;
        NMI++;
    }

    // Reserve room for num moves
    s32 HeaderCursor = BitStream.GetCursor();
    BitStream.WriteRangedS32(0,0,31);

    if( NMI==0 ) return;

    BitStream.WriteS32( m_Move[MI[0]].MoveSeq );

    ASSERT( BitStream.Overwrite() == FALSE );

    if( !m_Connected ) 
    {
        x_printf("MoveManager: buffer filled...shut down\n");
        return;
    }
    if( m_NMovesAllocated==0 ) return;

    // Write out as many moves as we can
    I=0;
    while( NMI-- )
    {
        // Remember where this item started
        s32 ItemCursor = BitStream.GetCursor();

        // SB - Only players go through this move manager and they use slots 0-31
        ASSERT((m_Move[MI[I]].ObjSlot >= 0) && (m_Move[MI[I]].ObjSlot <= 31)) ;
        BitStream.WriteRangedS32( m_Move[MI[I]].ObjSlot, 0, MAX_PLAYERS-1 );
        m_Move[MI[I]].Info.Write(BitStream) ;

        // Check if we over-ran bitstream
        if( BitStream.Overwrite() )
        {
            BitStream.SetCursor(ItemCursor);
            break;
        }

        //x_DebugMsg("Packing move (%1d,%1d) for %1d\n",I,m_Move[I].Seq,m_Move[I].ObjID);

        // We didn't overwrite buffer so keep this one
        Packet.MoveID[NMovesWritten]  = MI[I];
        Packet.MoveSeq[NMovesWritten] = m_Move[MI[I]].MoveSeq;
        NMovesWritten++;
        Packet.NMoves++;
        m_Move[MI[I]].NTimesSent++;
        I++;
    }

    //x_DebugMsg("NMovesPacked: %1d\n",Packet.NMoves);

    //x_printfxy(10,1,"Pack %1d moves of %1d (%1d,%1d)",NMovesWritten,m_NMovesAllocated,m_ReadPos,m_WritePos);
    //x_DebugMsg("Packed %1d moves of size %1d\n",NMovesWritten,sizeof(move));

    // Fill in header and return
    s32 EndCursor = BitStream.GetCursor();
    BitStream.SetCursor(HeaderCursor);
    BitStream.WriteRangedS32( NMovesWritten, 0, 31 );
    BitStream.SetCursor(EndCursor);

    //x_DebugMsg("%1d %1d\n",BitStream.GetNBytesUsed(),BitStream.GetNBitsUsed());
}

//=========================================================================

void move_manager::UnpackMoves( bitstream& BitStream )
{
    if( !m_Connected ) return;

    // Read number of moves
    s32 NMoves;
    BitStream.ReadRangedS32( NMoves, 0, 31 );
    if( NMoves == 0 )
        return;

    s32 Slot;
    s32 Seq;
    BitStream.ReadS32( Seq );

    //x_printfxy(10,0,"Unpack moves %1d",NMoves);
    // Process all the items
    ASSERT( NMoves < 100 );
    for( s32 i=0; i<NMoves; i++ )
    {
        // SB - Only players go through this move manager and they use slots 0-31
        BitStream.ReadRangedS32( Slot, 0, MAX_PLAYERS-1 );

        // Lookup player
        object::id      ObjID( Slot, -1 );
        player_object*  pObj = (player_object*)ObjMgr.GetObjectFromID( ObjID );

        // Read in the move
        player_object::move Move ;
        if( pObj )
            Move.Read(BitStream, pObj->GetTVRefreshRate()) ;
        else
            Move.Read(BitStream, eng_GetTVRefreshRate()) ;

        //x_printfxy(10,1,"Unpacking move %1d for %1d\n",Seq,ObjID);

        if( (Seq > m_LastReceivedSeq) )
        {
            if( pObj )
            {
                // Adds a move to the corresponding players queue
                ReceiveMove(Slot, Move, Seq) ;
            }

            m_LastReceivedSeq = Seq;
        }
        Seq++;
    }
}

//=========================================================================

void move_manager::PacketAck( conn_packet& Packet, xbool Arrived )
{
    if( !m_Connected ) return;

    // Loop through life attached to packet
    if( Arrived )
    {
        for( s32 i=0; i<CONN_MOVES_PER_PACKET; i++ )
        {
            s32 I = Packet.MoveID[i];
            s32 S = Packet.MoveSeq[i];

            if( I!=-1 )
            {
                if( (m_ReadPos == I) && (m_Move[I].MoveSeq == S) )
                {
                    //x_DebugMsg("freeing move %1d\n",I);
                    m_Move[I].NTimesSent = 0;
                    m_ReadPos = (m_ReadPos+1)%MAX_MOVE_ENTRIES;
                    m_NMovesAllocated--;
                }
            }
        }
    }
    else
    {
        for( s32 i=0; i<CONN_MOVES_PER_PACKET; i++ )
        {
            s32 I = Packet.MoveID[i];
            s32 S = Packet.MoveSeq[i];

            if( (I!=-1) && (m_Move[I].MoveSeq == S) )
            {
                m_Move[I].NTimesSent--;
            }
        }
    }
}

//=========================================================================

xbool move_manager::IsConnected( void )
{
    return m_Connected;
}

//=========================================================================

xbool move_manager::FindMoveInPast( s32 Seq )
{
    s32 MinSeq;
    s32 MinSeqIndex = m_WritePos;
    m_Cursor = m_WritePos;

    // Move backwards in move list looking for entry
    MinSeq = -1;
    for( s32 i=0; i<MAX_MOVE_ENTRIES; i++ )
    {
        // Check if this is the actual one we are looking for
        if( m_Move[m_Cursor].MoveSeq == Seq )
        {
            m_Cursor = (m_Cursor+1)%MAX_MOVE_ENTRIES;
            return TRUE;
        }

        // Keep track of our best choice
        if( m_Move[m_Cursor].MoveSeq > Seq )
        {
            MinSeq = m_Move[m_Cursor].MoveSeq;
            MinSeqIndex = m_Cursor;
        }

        m_Cursor = (m_Cursor+(MAX_MOVE_ENTRIES-1))%MAX_MOVE_ENTRIES;
    }

    // Check if we found an alternative
    if( MinSeq == -1 )
        return FALSE;

    m_Cursor = MinSeqIndex;
    return TRUE;
}

//=========================================================================

xbool move_manager::GetNextMove( player_object::move& Move, s32& ObjSlot )
{
    if( m_Cursor == m_WritePos )
        return FALSE;

    Move = m_Move[m_Cursor].Info;
    ObjSlot = m_Move[m_Cursor].ObjSlot;
    m_Cursor = (m_Cursor+1)%MAX_MOVE_ENTRIES;
    return TRUE;
}

//=========================================================================






