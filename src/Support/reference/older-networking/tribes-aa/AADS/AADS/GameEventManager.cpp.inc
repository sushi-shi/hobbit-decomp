//=========================================================================
//
//  MoveManager.cpp
//
//=========================================================================

#include "x_files.hpp"
#include "gameeventmanager.hpp"
#include "connmanager.hpp"
#include "globals.hpp"

#include "GameMgr\GameMgr.hpp"

//=========================================================================

game_event_manager::game_event_manager( void )
{
}

//=========================================================================

game_event_manager::~game_event_manager( void )
{
}

//=========================================================================

void game_event_manager::Init( void )
{
    m_Connected    = TRUE;

    // DAT - 
    m_GMDirtyBits  = 0xFFFFFFFF;
    m_GMPlayerBits = 0xFFFFFFFF;
    m_GMScoreBits  = 0xFFFFFFFF;
}

//=========================================================================

void game_event_manager::Kill( void )
{
}

//=========================================================================

void game_event_manager::PackGameManager( conn_packet& Packet, bitstream& BitStream )
{
    (void)Packet;
    (void)BitStream;
    if( !m_Connected ) return;

    // Make backup copies of the dirty bits.
    u32 DirtyBits  = m_GMDirtyBits;
    u32 PlayerBits = m_GMPlayerBits;
    u32 ScoreBits  = m_GMScoreBits;

    // Let the GameMgr send whatever data it can given the current
    // dirty bits.  This function call will clear bits which are sent.
    GameMgr.ProvideUpdate( BitStream, m_GMDirtyBits, m_GMPlayerBits, m_GMScoreBits );

    // Now, save into the packet the bits which were written.  Simply XOR
    // the backup copies of the bit masks with the updated versions.
    Packet.GMDirtyBits   =  m_GMDirtyBits   ^  DirtyBits;
    Packet.GMPlayerBits  =  m_GMPlayerBits  ^  PlayerBits;
    Packet.GMScoreBits   =  m_GMScoreBits   ^  ScoreBits;
}

//=========================================================================

void game_event_manager::UnpackGameManager( bitstream& BitStream, f32 AvgPing )
{
    (void)BitStream;
    if( !m_Connected ) return;

    // DAT - Request unpacking from game manager
    GameMgr.AcceptUpdate( BitStream, AvgPing );
}

//=========================================================================

void game_event_manager::PacketAck( conn_packet& Packet, xbool Arrived )
{
    if( !m_Connected ) return;

    if( !Arrived )
    {
        m_GMDirtyBits  |= Packet.GMDirtyBits; 
        m_GMPlayerBits |= Packet.GMPlayerBits;
        m_GMScoreBits  |= Packet.GMScoreBits;
    }
}

//=========================================================================

xbool game_event_manager::IsConnected( void )
{
    return m_Connected;
}

//=========================================================================

void game_event_manager::UpdateGMDirtyBits( u32 DirtyBits, u32 PlayerBits, u32 ScoreBits )
{
    m_GMDirtyBits  |= DirtyBits;
    m_GMPlayerBits |= PlayerBits;
    m_GMScoreBits  |= ScoreBits;
}

//=========================================================================








