// Original Area51 sibling source import; no Hobbit PC address claim. See docs/imports/entropy-network-memcard.md.
//=============================================================================
//
//  BitStream.hpp
//
//=============================================================================

#ifndef NETSTREAM_HPP
#define NETSTREAM_HPP

//=============================================================================
//  INCLUDES
//=============================================================================

#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_math.hpp>
#include <xCore/Entropy/e_Network.hpp>
#include <xCore/x_files/x_bitstream.hpp>

//=============================================================================
//  TYPES
//=============================================================================

class netstream : public bitstream
{
public:
                        netstream(void);
                       ~netstream(void);

            void        Init                ( void );
            void        Kill                ( void );
            void        Open                ( s32 HeaderId, s32 Type ); 
            void        Close               ( void );
            void        Send                ( net_socket& Socket, const net_address& Remote );
            xbool       Receive             ( net_socket& Socket, net_address& Remote );
            xbool       Validate            ( void );

private:
            byte        m_Buffer[896];
};

//=============================================================================
#endif
//=============================================================================