//=============================================================================
//
//  NETMAN.HPP
//
//=============================================================================
#ifndef NETMAN_HPP
#define NETMAN_HPP
#include <xCore/x_files/x_types.hpp>
#include "netlib.hpp"
#include "bitstream.hpp"

class net_connection
{

protected:

    net_connection* m_pNext;
    net_connection* m_pPrev;
    net_address m_SrcAddress;
    net_address m_DstAddress;


    friend void netman_AddConnection( net_connection& Connection );
    friend void netman_DelConnection( net_connection& Connection );
    friend void netman_Heartbeat    ( void );
    friend void netman_DispatchPacket( net_address&  SenderAddress,
                                       bitstream&    BitStream );


public:

    net_connection ( void );
    ~net_connection( void );

    net_address  GetSrcAddress          ( void ) { return m_SrcAddress; }
    net_address  GetDstAddress          ( void ) { return m_DstAddress; }
    void         SendPacket             ( bitstream& BitStream );

    virtual void HandleInit             ( net_address& Src, net_address& Dst );
    virtual void HandleKill             ( void );
    virtual void HandlePacket           ( bitstream& BitStream );
    virtual void HandleHeartbeat        ( void );
};

//=============================================================================

void netman_Init                ( void );

void netman_Kill                ( void );

net_address netman_GetAddress   ( void );

void netman_Send                ( net_address&  DstAddress, 
                                  bitstream&    BitStream );

void netman_DispatchAllPackets  ( void );

void netman_DispatchPacket      ( net_address&  SenderAddress,
                                  bitstream&    BitStream );

void netman_Heartbeat           ( void );

void netman_AddConnection       ( net_connection& Connection );
void netman_DelConnection       ( net_connection& Connection );

//=============================================================================

typedef void (*net_fnptr) ( net_address& SenderAddress, bitstream& BitStream);

void netman_RegisterHandler     ( net_fnptr pFunction );

//=============================================================================
#endif
//=============================================================================