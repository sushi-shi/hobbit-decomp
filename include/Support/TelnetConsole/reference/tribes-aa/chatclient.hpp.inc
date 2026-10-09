#ifndef __CHAT_CLIENT_HPP
#define __CHAT_CLIENT_HPP

#include <xCore/x_files/x_types.hpp>
#include "netlib/netlib.hpp"

#define CHAT_BUFFER_HISTORY      64
#define CHAT_LINE_WIDTH         192

enum chat_state
{
    CHAT_IDLE,
    CHAT_BEGIN_LOGIN,
    CHAT_SEND_NICK,
    CHAT_SEND_JOIN,
    CHAT_WAIT_JOIN,
    CHAT_ACTIVE,
    CHAT_DISCONNECT,
};

class chat_client
{
public:
                    chat_client     (void);
    void            Init(void);
    void            Kill(void);

    xbool           OpenSession     (void);
    void            CloseSession    (void);
    void            Update          (const f32 DeltaTime);
    void            SetHost         (const net_address& Address)        { m_Host     = Address;                     };
    void            SetChannel      (const char* pChannel)              { m_Channel  = pChannel; MakeSafe(m_Channel);};
    void            SetNick         (const char* pNickname)             { m_Nick     = pNickname; MakeSafe(m_Nick); };
    void            Print           (const char* pFormatStr,...);
    void            Message         (const char* pMessage);
    xbool           IsConnected     (void)                              { return m_ChatState != CHAT_IDLE; };

    const char*     GetChannel      (void)                              { return (const char*)m_Channel;            };
    const net_address& GetHost          (void)                          { return m_Host;                            };
    const char*     GetNick         (void)                              { return (const char*)m_Nick;               };
    
private:
    void            PrintBare       (const char* pFormatStr,...);
    const char*     GetLine         (void);
    void            SetState        (chat_state State)                  { m_ChatState = State; m_StateTimer = 0.0f; };
    void            MakeSafe        (xstring& String);

    net_address     m_Local;
    net_address     m_Host;
    xstring         m_Channel;
    xstring         m_Nick;
    chat_state      m_ChatState;

    f32             m_GameStateTimer;
    f32             m_GameScoreTimer;
    f32             m_SendInterval;
    f32             m_StateTimer;
    char            m_PostFix;
    char            m_OutBuffer[CHAT_BUFFER_HISTORY][CHAT_LINE_WIDTH];
    // If we, for some reason, overrun the end of the 'out buffer' with a very long body of text
    // then we will over-run the in buffer which will be, for the most part, harmless.
    char            m_InBuffer[256];
    s32             m_InBufferIndex;
    s32             m_InBufferSize;
    char            m_LineBuffer[CHAT_LINE_WIDTH];
    xbool           m_LineIndex;
    s32             m_WriteIndex;
    s32             m_ReadIndex;
};

extern chat_client g_ChatManager;

#endif