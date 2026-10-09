//==============================================================================
//
//  ClientServer.hpp
//
//==============================================================================

#ifndef CLIENTSERVER_HPP
#define CLIENTSERVER_HPP

#include "netlib\netlib.hpp"

#define STEP_SESSION_ID
//#define DEBUG_SHORT_GAMES

//==============================================================================

void    cs_Init             ( net_address& Address );
void    cs_Kill             ( void );
void    cs_OpenConnection   ( net_address& DstAddress );
void    cs_CloseConnection  ( net_address& DstAddress );
void    cs_HandleIncoming   ( const bitstream& aBS, net_address& Src );
void    cs_Update           ( f32 DeltaTime );
void    cs_Send             ( const bitstream& BS, net_address& Dst );
xbool   cs_Receive          ( bitstream& BS, net_address& Src );
void    cs_DisplayStats     ( void );

//==============================================================================

struct cs_login_req_data
{
    xwchar  Name[16];
    s32     CharacterType;
    s32     SkinType;
    s32     VoiceType;
};

struct cs_login_req
{
    s32                 ServerVersion;
    s32                 NPlayers;
    s32                 SystemId;
    xwchar              Password[16];
    cs_login_req_data   Data[2];
    f32                 TVRefreshRate ;
};

void cs_ReadLoginReq ( cs_login_req& Login, const bitstream& BS );
void cs_WriteLoginReq( const cs_login_req& Login, bitstream& BS );

//==============================================================================

struct cs_login_resp_data
{
    xwchar Name[16];
    s32    PlayerIndex;
};

struct cs_login_resp
{
    s32     RequestResp;
    s32     SessionID;
    xwchar  ServerName[32];
    s32     NPlayers;
    cs_login_resp_data Data[2];
};

void cs_ReadLoginResp ( cs_login_resp& Login, const bitstream& BS );
void cs_WriteLoginResp( const cs_login_resp& Login, bitstream& BS );

//==============================================================================

struct cs_end_mission
{
    s32 Round;
    s32 SessionID;
};

void cs_ReadEndMission (       cs_end_mission& EndMission, const bitstream& BS );
void cs_WriteEndMission( const cs_end_mission& EndMission,       bitstream& BS );

//==============================================================================

struct cs_mission_resp
{
    char    MissionName[32];
    s32     GameType;
    s32     Round;
};

void cs_ReadMissionResp (       cs_mission_resp& Mission, const bitstream& BS );
void cs_WriteMissionResp( const cs_mission_resp& Mission,       bitstream& BS );

//==============================================================================

struct cs_disconnect
{
    s32 Reason;
};

void cs_ReadDisconnect (       cs_disconnect& Disconnect, const bitstream& BS );
void cs_WriteDisconnect( const cs_disconnect& Disconnect,       bitstream& BS );

//==============================================================================

struct cs_insync
{
    s32 Reason;
};

void cs_ReadInSync (       cs_insync& InSync, const bitstream& BS );
void cs_WriteInSync( const cs_insync& InSync,       bitstream& BS );

//==============================================================================
#endif // CLIENTSERVER_HPP
//==============================================================================
