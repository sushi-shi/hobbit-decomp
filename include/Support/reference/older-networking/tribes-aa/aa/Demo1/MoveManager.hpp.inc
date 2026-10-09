//=========================================================================
//
//  MoveManager.hpp
//
//=========================================================================
#ifndef MOVEMANAGER_HPP
#define MOVEMANAGER_HPP

#include "x_files.hpp"
#include "netlib/bitstream.hpp"
#include "objectmgr/objectmgr.hpp"
#include "Objects/Player/PlayerObject.hpp"


//=========================================================================

struct conn_packet;

#define MAX_MOVE_ENTRIES  128

class move_manager
{
private:

    struct move_entry
    {
        player_object::move     Info;
        s32                     MoveSeq;
        s32                     ObjSlot;          // Object this move applies to
        s32                     NTimesSent;
    };

    move_entry      m_Move[MAX_MOVE_ENTRIES];
    s32             m_WritePos;
    s32             m_ReadPos;
    s32             m_NMovesAllocated;
    xbool           m_Connected;
    s32             m_MoveSeq;
    s32             m_LastReceivedSeq;
    s32             m_Cursor;

    s32  AllocMoveOnEnd ( void );

public:

    move_manager      ( void );
    ~move_manager     ( void );

    void Init           ( void );
    void Kill           ( void );

    void SendMove       ( s32 ObjSlot, player_object::move& Move );

    void PackMoves      ( conn_packet& Packet, bitstream& BitStream, s32 NMovesAllowed );
    void UnpackMoves    ( bitstream& BitStream );

    void PacketAck      ( conn_packet& Packet, xbool Arrived );

    xbool IsConnected   ( void );

    xbool FindMoveInPast  ( s32 Seq );
    xbool GetNextMove     ( player_object::move& Move, s32& ObjSlot );

    void Reset          ( void );
};

// Processes 1 move per call per player
void ProcessReceivedMoves( void ) ;



//=========================================================================
#endif
//=========================================================================

