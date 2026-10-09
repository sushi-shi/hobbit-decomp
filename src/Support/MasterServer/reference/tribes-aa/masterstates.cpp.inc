#include <xCore/x_files/x_files.hpp>
#include <xCore/Entropy/Entropy.hpp>
#include "masterserver.hpp"
#include "netlib/netlib.hpp"
#include "bytestream.hpp"
#include "../../AADS/serverman.hpp"


s32 s_EntityCount;
u8	s_PacketReceived[512];
s32	s_NaksSent;
s32 s_TopPacket;
s32 s_ReceiveLoop;
s32 s_LastPacketIndex;


#include "telnetconsole/telnetmgr.hpp"
//-----------------------------------------------------------------------------
void master_server::UpdateIdle(f32 DeltaTime)
{
    (void)DeltaTime;

    if ( (m_RegistrationRetryTime <=0.0f) && (m_IsServer) )
    {
        m_pqPendingRequests->Send((void *)US_START_REGISTRATION,MQ_NOBLOCK);
        m_RegistrationRetryTime = 1.0f;
        m_RegistrationRetry = 4;
    }
    else
    {
        m_UpdateState = (update_state)(s32)m_pqPendingRequests->Recv(MQ_NOBLOCK);
    }
}

//-----------------------------------------------------------------------------
void master_server::StartRegister(void)
{
    s32 i;
    s32 count;
    bytestream Bytestream;
#ifdef bwatson
    x_DebugMsg("Start registration\n");
#endif
    Bytestream.Init(m_RequestBuffer,sizeof(m_RequestBuffer));
    //
    // Send a registration request to the master server
    //
    Bytestream.PutByte(MC_SMALL_MESSAGE);               // Class
    Bytestream.PutWord(ST_DIRECTORY_SERVER);            // ServiceType
    Bytestream.PutWord(MT_ADD_SERVICE_EX);              // MessageType
    Bytestream.PutByte( DIR_UF_OVERWRITE|
                        DIR_UF_SERVRETURNADDR|
                        DIR_UF_SERVGENNETADDR|
                        DIR_UF_DIRNOTUNIQUE );          // Entity Flags
    Bytestream.PutWideString(m_ServerEntity.GetPath()); // Path
    Bytestream.PutWideString(m_ServerEntity.GetName()); // Name

#if 0
    Bytestream.PutByte(0);                          // Address length (0 forces server to fill it for us)
#else
    Bytestream.PutByte(2);                          // Address length (0 forces server to fill it for us)

    Bytestream.PutNetWord(m_ServerEntity.GetAddress().Port);
    //Bytestream.PutLong(m_ServerEntity.GetAddress().IP);
#endif
    Bytestream.PutWideString(m_ServerEntity.GetDisplayName());               // Display name
    Bytestream.PutLong((s32)MS_LIFESPAN);           // Life span
    //
    // Dump the attributes for this server to the master
    //
    count = m_ServerEntity.GetAttributeCount();
    Bytestream.PutWord(count);

    for (i=0;i<count;i++)
    {
        entity_attrib attribute;
        m_ServerEntity.GetAttribute(i,attribute.Name,attribute.Value);
        Bytestream.PutByte(x_strlen(attribute.Name));
        Bytestream.PutBytes((byte*)attribute.Name,x_strlen(attribute.Name));
        Bytestream.PutWord(x_strlen(attribute.Value));
        Bytestream.PutBytes((byte*)attribute.Value,x_strlen(attribute.Value));
    }

    Bytestream.PutWord(0);

    ASSERTS(m_CurrentMasterServer.IP,"Must have set a master server address before any updates can occur");
    net_Send(m_ServerEntity.GetAddress(),m_CurrentMasterServer,Bytestream.GetDataPtr(),Bytestream.GetBytesUsed());

    m_Timeout               = MS_RECEIVE_TIME;
    m_StatusReceived        = FALSE;
    m_UpdateState           = US_WAIT_REGISTRATION;
}

//-----------------------------------------------------------------------------
void master_server::WaitRegister(f32 DeltaTime)
{
    bytestream Bytestream;
    // If we received a status packet after a wait registration had been issued, then
    // it really should have been a registration packet returned.
    if ( m_StatusReceived  )
    {
        if (m_LastStatus == 0)
        {
            m_UpdateState = US_IDLE;
            m_RegistrationRetryTime = MS_REREGISTER_TIME;             // Re-register in 4 minutes
            return;
        }
        else
        {
            m_Timeout = 10.0f;
        }
        m_StatusReceived = FALSE;
    }

    m_Timeout -= DeltaTime;
    if (m_Timeout < 0.0f)
    {
#ifdef bwatson
        x_DebugMsg("Timed out during registration\n");
#endif
        char textbuff[64];

        net_IPToStr(m_ExternalAddress.IP,textbuff);
		g_TelnetManager.AddDebugText("master_server: Unable to register with %s:%d.\n",textbuff,m_ExternalAddress.Port );
        // Now select another master server in the list to connect to
        // just in case the current one is offline
        CycleMasterServer();
        m_Timeout = MS_RECEIVE_TIME;
        m_UpdateState = US_IDLE;
        m_RegistrationRetry --;
        if (m_RegistrationRetry > 0)
        {
#ifdef bwatson
            x_DebugMsg("Queued retry %d\n",m_RegistrationRetry);
#endif
            net_IPToStr(m_ExternalAddress.IP,textbuff);
		    g_TelnetManager.AddDebugText("master_server: Retrying registration to %s:%d.\n",textbuff,m_ExternalAddress.Port );
            m_pqPendingRequests->Send((void *)US_START_REGISTRATION,MQ_NOBLOCK);
        }
    }
}

//-----------------------------------------------------------------------------
void master_server::StartUnregister(void)
{
    bytestream Bytestream;

#ifdef bwatson
    x_DebugMsg("Start unregister\n");
#endif
    Bytestream.Init(m_RequestBuffer,sizeof(m_RequestBuffer));
    //
    // Send a registration request to the master server
    //
    Bytestream.PutByte(MC_SMALL_MESSAGE);           // Class
    Bytestream.PutWord(ST_DIRECTORY_SERVER);        // ServiceType
    Bytestream.PutWord(MT_REMOVE_SERVICE);          // MessageType
    Bytestream.PutWideString(m_ServerEntity.GetPath());               // Path
    Bytestream.PutWideString(m_ServerEntity.GetName());               // Name
    Bytestream.PutByte(6);                          // 6 bytes in network address

    Bytestream.PutNetWord(m_ExternalAddress.Port);
    Bytestream.PutLong(m_ExternalAddress.IP);

    net_Send(m_ServerEntity.GetAddress(),m_CurrentMasterServer,Bytestream.GetDataPtr(),Bytestream.GetBytesUsed());

    m_UpdateState           = US_WAIT_UNREGISTER;
    m_StatusReceived        = FALSE;
    m_Timeout               = MS_RECEIVE_TIME;
}

//-----------------------------------------------------------------------------
void master_server::WaitUnregister(f32 DeltaTime)
{
    // If we received a status packet after a wait registration had been issued, then
    // it really should have been a registration packet returned.

    if (m_StatusReceived)
    {
        m_UpdateState = US_IDLE;
        m_RegistrationRetryTime = MS_REREGISTER_TIME;             // Re-register in 4 minutes
        m_ExternalAddress.Clear();
        return;
    }
    m_Timeout -= DeltaTime;
    if (m_Timeout < 0.0f)
    {
        m_RegistrationRetry--;
        if (m_RegistrationRetry > 0)
        {
            m_pqPendingRequests->Send((void *)US_START_UNREGISTER,MQ_NOBLOCK);
        }
        m_Timeout = MS_RECEIVE_TIME;
        m_UpdateState = US_IDLE;
    }
}

//-----------------------------------------------------------------------------
void master_server::StartReadDirectory(void)
{
    bytestream Bytestream;

	if (!m_AcquireDirectory)
	{
		m_UpdateState   = US_IDLE;
		return;
	}
#ifdef bwatson
    x_DebugMsg("START_READ_DIRECTORY\n");
#endif
    Reset();
    Bytestream.Init(m_RequestBuffer,sizeof(m_RequestBuffer));
    Bytestream.PutByte(MC_SMALL_MESSAGE);           // Class
    Bytestream.PutWord(ST_DIRECTORY_SERVER);        // ServiceType
    Bytestream.PutWord(MT_GET_DIRECTORY_EX);        // MessageType
    Bytestream.PutLong(
#if 0
        // Add these if we want to get the list of subdirectories. Do we use subdirectories for filtering?
            DIR_GF_DECOMPROOT    |
            DIR_GF_DECOMPSUBDIRS |
            DIR_GF_SERVADDPATH   |
            DIR_GF_SERVADDNAME   |
	        DIR_GF_ADDDISPLAYNAME|
            DIR_GF_DIRADDPATH    |
            DIR_GF_DIRADDNAME    |
#endif
	        DIR_GF_ADDDOTYPE	 |
            DIR_GF_ADDTYPE       |
	        DIR_GF_DECOMPSERVICES|
	        DIR_GF_SERVADDNETADDR|
	        DIR_GF_ADDDODATA	 |
	        DIR_GF_ADDDATAOBJECTS);                 // GetFlags
    Bytestream.PutWord(20);                         // MaxEntitiesPerReply
    Bytestream.PutWideString(m_ServerEntity.GetPath());               // DirectoryPath
    Bytestream.PutWord(0);                          // NumDataObjectTypes
    net_Send(m_ServerEntity.GetAddress(),m_CurrentMasterServer,Bytestream.GetDataPtr(),Bytestream.GetBytesUsed());
	s_EntityCount = 0;
    if (m_UpdateState != US_WAIT_DIRECTORY)
    {
        m_UpdateState           = US_WAIT_DIRECTORY;
        m_DirectoryReadRetries  = 3;
    }
    m_DirectoryReadTimeout      = MS_DIRECTORY_TIMEOUT;
    m_DirectoryReadComplete     = FALSE;
	x_memset(s_PacketReceived,0,sizeof(s_PacketReceived));
	s_NaksSent = 0;
	s_TopPacket = 0;
	s_ReceiveLoop = 0;
	if (m_MasterServerRetries == -1)
	{
		m_MasterServerRetries = 3;
	}
	m_ServerCount = 0;
}

//-----------------------------------------------------------------------------
void master_server::WaitReadDirectory(f32 DeltaTime)
{
    if (m_DirectoryReadComplete)
    {
        m_UpdateState = US_IDLE;
    }

    m_DirectoryReadTimeout-=DeltaTime;
    if (m_DirectoryReadTimeout < 0.0f)
    {
        // If we timed out, go idle for a very short time so we can issue the directory
        // read again fairly quickly.
#ifdef bwatson
        x_DebugMsg("Timed out waiting for directory contents\n");
#endif
		SendNakStream(-1);

        m_DirectoryReadRetries--;
        if ( (m_DirectoryReadRetries >0) && (m_MasterServerRetries >= 0) )
        {
	        CycleMasterServer();
            m_UpdateState           = US_START_READ_DIRECTORY;
            m_DirectoryReadTimeout  = MS_DIRECTORY_TIMEOUT;
			m_MasterServerRetries --;
        }
        else
        {
            m_UpdateState   = US_IDLE;
        }
    }
}

//-----------------------------------------------------------------------------
void master_server::AckPacket(s32 Sequence)
{
	s32 index;

	if ( (Sequence & 0x7f) < s_LastPacketIndex)
	{
		s_ReceiveLoop++;
	}
	s_LastPacketIndex = (Sequence & 0x7f);
	index = (Sequence & 0x7f)+s_ReceiveLoop;

	if ((u32)index > sizeof(s_PacketReceived))
		return;

	s_PacketReceived[index]++;
	if (index > s_TopPacket)
	{
		s_TopPacket = index;
	}
}

extern server_manager *pSvrMgr;

//-----------------------------------------------------------------------------
void  master_server::SendNakStream(s32 Sequence)
{
	bytestream AckStream;
	byte Buffer[384];
	s32 count;
	s32 i;

	// If we've sent too many nak packets OR we haven't any to send,
	// just bail out
	if ( (s_NaksSent > 10) || (s_TopPacket==0) )
		return;
	s_NaksSent++;

	// Count the number to send
	count = 0;
	for (i=0;i<(s32)sizeof(s_PacketReceived);i++)
	{
		if (i>s_TopPacket)
			break;
		if (!s_PacketReceived[i]) count++;
	}

	if (count!=0)
	{

		AckStream.Init(Buffer,sizeof(Buffer));
		AckStream.PutByte(MC_SMALL_MESSAGE);
		AckStream.PutWord(ST_DIRECTORY_SERVER);
		AckStream.PutWord(MT_RESPONSES_RECEIVED);
		AckStream.PutLong(	DIR_GF_ADDDOTYPE	 |
							DIR_GF_ADDTYPE       |
							DIR_GF_DECOMPSERVICES|
							DIR_GF_SERVADDNETADDR|
							DIR_GF_ADDDODATA	 |
							DIR_GF_ADDDATAOBJECTS);                 // GetFlags

		AckStream.PutWord(32);										// EntitiesPerReply
		AckStream.PutWideString(m_ServerEntity.GetPath());          // DirectoryPath
		AckStream.PutWord(0);										// NumDataObjects
		AckStream.PutByte(count);									// NumOfPacketsToSend

		i=0;
		while(count)
		{
			s32 index;

			// Find next packet in list that needs to be NAKed
			while(s_PacketReceived[i])
			{
				i++;
			}

			if (i>s_TopPacket)
				break;
			index = i;
			while (index > 254)
			{
				AckStream.PutByte(0xff);
				index-=254;
			}
			AckStream.PutByte(index);
			// We only have to check once since PutByte will stop putting data in when
			// the buffer is full
			if (AckStream.GetBytesUsed() >= (s32)sizeof(Buffer))
				break;
			count--;
			i++;
		}
		net_Send(m_ServerEntity.GetAddress(),m_CurrentMasterServer,AckStream.GetDataPtr(),AckStream.GetBytesUsed());
	}
    
	x_memset(s_PacketReceived,0x00,sizeof(s_PacketReceived));
	s_TopPacket = Sequence & 0x7f;
	s_ReceiveLoop = 0;
	s_LastPacketIndex = 0;
}
//-----------------------------------------------------------------------------
xbool master_server::ParsePacket(byte *pBuffer, s32 Length,const net_address &Local, const net_address &Remote)
{
    bytestream      Bytestream;
    s32             SequenceNumber;
    s32             MessageClass;
    s32             ServiceType;
    s32             MessageType;
    s32             Status;
    s32             Flags;
    xbool           IsFinished;
    s32             EntityCount;
    s32             i;
    single_entity*  pEntity;

    Bytestream.Init(pBuffer,Length);
    IsFinished = TRUE;

    char ipstr[64];
    char ipstr2[64];
    net_IPToStr(Remote.IP,ipstr);
    net_IPToStr(Local.IP,ipstr2);
    if (Length < 3)
    {
        if (pSvrMgr)
            pSvrMgr->ParsePacket(pBuffer,Local,Remote);
        return FALSE;
    }

    MessageClass = Bytestream.GetByte();
    ServiceType  = Bytestream.GetWord();
    //
    // If it wasn't a packet for us, let's just bail now.
    //
    if ( (MessageClass != MC_SMALL_MESSAGE) ||
         (ServiceType != ST_DIRECTORY_SERVER) )
    {
        if (pSvrMgr)
            pSvrMgr->ParsePacket(pBuffer,Local,Remote);
        return FALSE;
    }

    MessageType  = Bytestream.GetWord();
    Status       = Bytestream.GetWord();

    if (Status >=0)
    {
        switch (MessageType)
        {
        case MT_MULTI_ENTITY_REPLY:
            m_DirectoryReadTimeout  = MS_DIRECTORY_TIMEOUT;
			m_DirectoryReadRetries  = 0;
			m_MasterServerRetries	= -1;
            SequenceNumber = (u8)Bytestream.GetByte();
            Flags = Bytestream.GetLong();
            EntityCount = Bytestream.GetWord();
			if (!m_AcquireDirectory) return TRUE;
//			x_DebugMsg("MT_MULTI_ENTITY_REPLY: Count=%4d, Sequence = 0x%02x, flags=0x%08x\n",EntityCount,SequenceNumber,Flags);

			// If we receive a packet number that we have already seen, or the number of packets
			// received exceeds the MS_RESPONSES_PER_PACKET threshold, then we send back a response
			// to notify the master server that we got some (but not necessarily all) data

			AckPacket( (SequenceNumber & 0x7f)+(s_ReceiveLoop<<7));
			if ( SequenceNumber & 0x80 )
			{
				SendNakStream(SequenceNumber);
			}
			m_ServerCount += EntityCount;

            for (i=0;i<EntityCount;i++)
            {
                xbool found;
				if (m_Entities.GetCount() > 2000)
				{
					x_DebugMsg("master_server::ParsePacket - Too many servers in master server list (exceeded 2000)");
					break;
				}
                pEntity = new single_entity(Bytestream,Flags);
                
				if (Bytestream.WasExhauted())
				{
					x_DebugMsg("master_server::ParsePacket - bytestream unexpectantly ran out of data. Dropped\n");
					delete pEntity;
					break;
				}

                if ( m_AcquireDirectory && pEntity->GetAddress().IP )
                {
                    found = FALSE;

                    if (pSvrMgr)
                    {
                        s32 count,index;
                        const server_info *pInfo;

                        count = pSvrMgr->GetCount();

                        for (index=0;index<count;index++)
                        {
                            pInfo = pSvrMgr->GetServer(index);

                            if ( ((u32)pInfo->Info.IP == pEntity->GetAddress().IP) &&
					             ((u32)pInfo->Info.Port == pEntity->GetAddress().Port) &&
                                 (x_wstrcmp(pInfo->ServerInfo.Name,pEntity->GetName())==0)
                               )
                            {
                                found = TRUE;
                                break;
                            }
                        }
                    }

#if 1
					const char* value;
					char version[64];
					xstring name(pEntity->GetName());

					x_sprintf(version,"%d",SERVER_VERSION);


					value = pEntity->GetAttribute("V");
					if (!value)
					{
						x_DebugMsg("master_server::ParsePacket - No version attribute found on this entity %s\n",(const char*)name);
						found = TRUE;
					}
					else
					{
						if (x_strcmp(version,value) != 0)
						{
							x_DebugMsg("master_server::ParsePacket - Wrong version id %s, expected %s, for this entity %s\n",value,version,(const char*)name);
							found=TRUE;
						}
					}
#endif
					for (s32 index=0;index<m_Entities.GetCount();index++)
					{
						if (m_Entities[index]->GetAddress() == pEntity->GetAddress() )
						{
							found = TRUE;
							break;
						}
					}
                    // If the server we're about to add to the list is within the server manager list (for local net)
                    // list, let's just not bother adding it
                    if (found)
                    {
                        delete pEntity;
                    }
                    else
                    {
                        m_Entities.Append(pEntity);
                        // Since we got a server name, we now go and send that server a lookup packet 
                        // to see whether or not it is alive and get it's stats if it is. The stats
                        // will be returned to "serverman" code so that should do checks to see whether
                        // or not it is a local or net response.

#if 0
                        if (pSvrMgr)
                        {
                            pSvrMgr->SendLookup(PKT_NET_LOOKUP,
                                                m_ServerEntity.GetAddress(),
                                                pEntity->GetAddress());
                        }
#endif
                    }
                }
				else
				{
					delete pEntity;
				}
            }

            IsFinished = (SequenceNumber & 0x80) != 0;
            break;
        case MT_STATUS_REPLY:
#ifdef bwatson
            x_DebugMsg("Reply Class       : %d\n",MessageClass);
            x_DebugMsg("Reply ServiceType : %d\n",ServiceType);
            x_DebugMsg("Reply MessageType : %d\n",MessageType);
            x_DebugMsg("Reply Status      : %d\n",Status);
#endif
            if (m_UpdateState == US_WAIT_REGISTRATION)
            {
                i = Bytestream.GetByte();
//              ASSERT(i==6);
                m_ExternalAddress.Port = Bytestream.GetNetWord();
                m_ExternalAddress.IP   = Bytestream.GetLong();
				if (m_ExternalAddress.IP == 0)
				{
					Status = -1;
				}
            }
            m_StatusReceived = TRUE;
            m_LastStatus     = Status;
            break;
        default:
            ASSERTS(FALSE,xfs("Unhandled Message type %d\n",MessageType));
        }
    }
    else
    {
        m_LastStatus = Status;
        m_StatusReceived = TRUE;
#ifdef bwatson
        x_DebugMsg("Got a status of %d\n",Status);
#endif
    }
    return IsFinished;
}
