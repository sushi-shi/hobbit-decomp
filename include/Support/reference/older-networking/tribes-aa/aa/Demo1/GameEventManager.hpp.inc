//=========================================================================
//
//  GameEventManager.hpp
//
//=========================================================================

#ifndef GAMEEVENTMANAGER_HPP
#define GAMEEVENTMANAGER_HPP

#include "x_files.hpp"
#include "netlib/bitstream.hpp"
#include "ObjectMgr/ObjectMgr.hpp"

//=========================================================================

struct conn_packet;

class game_event_manager
{
private:

    xbool           m_Connected;
    u32             m_GMDirtyBits;
    u32             m_GMPlayerBits;
    u32             m_GMScoreBits;

public:

    game_event_manager      ( void );
    ~game_event_manager     ( void );

    void Init               ( void );
    void Kill               ( void );

    xbool IsConnected       ( void );
    void PacketAck          ( conn_packet& Packet, xbool Arrived );

    void PackGameManager    ( conn_packet& Packet, bitstream& BitStream );
    void UnpackGameManager  ( bitstream& BitStream, f32 AvgPing );

    void UpdateGMDirtyBits  ( u32 DirtyBits, u32 PlayerBits, u32 ScoreBits );
};

//=========================================================================
#endif
//=========================================================================

