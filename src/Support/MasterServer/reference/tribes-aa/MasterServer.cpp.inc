#include <xCore/x_files/x_files.hpp>
#include <xCore/Entropy/Entropy.hpp>
#include "masterserver.hpp"
#include "netlib/netlib.hpp"
#include "bytestream.hpp"
#include "../../AADS/serverman.hpp"

extern server_manager* pSvrMgr;
//-----------------------------------------------------------------------------
master_server::master_server(void)
{
    m_Timeout           = 0.0f;
    m_ScanTimeout       = 0.0f;
    m_Initialized       = FALSE;        // Set to true once Init() is called, false when Kill() is called
    m_UpdateState       = US_IDLE;
    m_IsServer          = FALSE;
    m_AcquireDirectory  = FALSE;
    m_ServerEntity.SetAddress(net_address());
    m_CurrentMasterServer.Clear();
    m_MasterServers.Clear();
    m_FilterEnabled     = FALSE;
	m_ExternalAddress.Clear();
	m_ServerCount = 0;
}

//-----------------------------------------------------------------------------
master_server::~master_server(void)
{
    if (m_Initialized)             // Kill() must be called before we can deconstruct
        Kill();
}

//-----------------------------------------------------------------------------
void master_server::Init(void)
{
    ASSERT(!m_Initialized);
    m_Initialized = TRUE;
    Reset();
    m_RegistrationRetryTime = 0.0f;
    m_AcquireDirectory = FALSE;
    m_FilterEnabled = FALSE;
    m_ServerEntity.ClearAddress();
    m_CurrentMasterServer.Clear();
    m_MasterServers.Clear();
    m_pqPendingRequests = new xmesgq(MAX_DEFAULT_MESSAGES);
    m_ServerIndex = 0;
}

//-----------------------------------------------------------------------------
void master_server::Kill(void)
{
    ASSERT(m_Initialized);
    m_Initialized = FALSE;

    Reset();
    m_ServerEntity.ClearAddress();
    m_CurrentMasterServer.Clear();
    m_MasterServers.Clear();
    delete m_pqPendingRequests;
}

//-----------------------------------------------------------------------------
single_entity *master_server::GetEntity(s32 Index)
{
    ASSERT(m_Initialized);
    ASSERT (Index < m_Entities.GetCount());
    return m_Entities[Index];
}

//-----------------------------------------------------------------------------
const net_address& master_server::GetMasterServerAddr(void)
{
	return m_ServerEntity.GetAddress();
}

server_info* master_server::GetEntityInfo       ( s32 Index )
{
    return GetEntity(Index)->GetInfo();
}


//-----------------------------------------------------------------------------
void master_server::SetServer(xbool isServer)
{
    ASSERT(m_Initialized);
    if (isServer)
	{
        m_pqPendingRequests->Send((void *)US_START_REGISTRATION,MQ_NOBLOCK);
	}
    else
        m_pqPendingRequests->Send((void *)US_START_UNREGISTER,MQ_NOBLOCK);
	AcquireDirectory(FALSE);
    m_RegistrationRetry = 4;
    m_IsServer = isServer;
}

//-----------------------------------------------------------------------------
void master_server::AcquireDirectory ( xbool isEnabled )
{
    m_AcquireDirectory = isEnabled;
    Reset();
    if (isEnabled)
    {
		s32 i;
		s32 limit;

		limit=x_rand()%4;
		for (i=0;i<limit;i++)
		{
			CycleMasterServer();
		}
        m_pqPendingRequests->Send((void *)US_START_READ_DIRECTORY,MQ_NOBLOCK);
    }
}

//-----------------------------------------------------------------------------
update_state master_server::GetCurrentState(void)
{
    return m_UpdateState;
}

//-----------------------------------------------------------------------------
void master_server::ProcessPacket   ( const byte *pBuffer,s32 Length,const net_address &Local, const net_address &Remote )
{
    xbool   WasLastPacket;


    WasLastPacket = ParsePacket((byte *)pBuffer,Length,Local,Remote);
    if (WasLastPacket)
    {
        m_DirectoryReadComplete = TRUE;
    }
    else
    {
        m_Timeout       = 0.0f;                 // Force a request send immediately (we're acquiring more packets)
   }
}

//-----------------------------------------------------------------------------
void master_server::SetAddress      ( const net_address Address )
{
    m_ServerEntity.SetAddress(Address);
}

//-----------------------------------------------------------------------------
void master_server::ClearAddress      ( void )
{
    m_ServerEntity.ClearAddress();
}

//-----------------------------------------------------------------------------
s32 master_server::GetEntityCount(void)
{
    return m_Entities.GetCount();
}

//-----------------------------------------------------------------------------
void master_server::SetPath(const xwchar* pPath)
{
    m_ServerEntity.SetPath(pPath);
}

//-----------------------------------------------------------------------------
void master_server::SetName(const xwchar* pName)
{
    m_ServerEntity.SetName(pName);
    m_ServerEntity.SetDisplayName(pName);
}
//-----------------------------------------------------------------------------
void master_server::CycleMasterServer(void)
{
    s32 previndex;

    // We cycle through the list of master servers. If we find another one which
    // has a valid resolved address then we set the server entity address to that
    // address and port.
    previndex = m_ServerIndex;
    while (1)
    {
        m_ServerIndex++;
        if (m_ServerIndex >= m_MasterServers.GetCount())
        {
            m_ServerIndex=0;
        }
        if ( (m_MasterServers[m_ServerIndex].Address.IP != 0) ||
             (m_ServerIndex == previndex) )
            break;
    }
#ifdef bwatson
    char str[128];

    net_IPToStr(m_MasterServers[m_ServerIndex].Address.IP,str);
    x_DebugMsg("master_server::CycleMasterServer() - reset address of master server to %s:%d\n",str,
                m_MasterServers[m_ServerIndex].Address.Port);
#endif
    m_CurrentMasterServer = m_MasterServers[m_ServerIndex].Address;
}

//-----------------------------------------------------------------------------
void master_server::Reset(void)
{
    s32 i;

    for (i=0;i<m_Entities.GetCount();i++)
    {
        delete m_Entities[i];
    }
    m_Entities.Clear();
    m_Timeout = 0.0f;
}

//-----------------------------------------------------------------------------
void master_server::DeleteEntity(s32 Index)
{
    if (Index >= m_Entities.GetCount() )
        return;
    m_Entities.Delete(Index);
}

//-----------------------------------------------------------------------------
void master_server::Update(f32 DeltaTime)
{
    s32             i,count;

    ASSERT(m_Initialized);

    if (m_ServerEntity.GetAddress().IP == 0)
	{
		Reset();
		m_UpdateState = US_IDLE;
        return;
	}

    count = m_MasterServers.GetCount();
    for (i=0;i<count;i++)
    {
        // We only try to resolve one name at a time since this can take quite a while
        if (m_MasterServers[i].Resolved == FALSE)
        {   
            m_MasterServers[i].Address.IP = net_ResolveName(m_MasterServers[i].Name);
            m_MasterServers[i].Resolved = TRUE;

            if ( (m_MasterServers[i].Address.IP) && (m_CurrentMasterServer.IP == 0) )
            {
                m_CurrentMasterServer = m_MasterServers[i].Address;
            }
            break;
        }
    }

    if (m_CurrentMasterServer.IP == 0)
        return;

    m_RegistrationRetryTime -= DeltaTime;
    ///--------------------------------------------
    // Finite state machine for update requests being polled or sent to the server
    ///--------------------------------------------
    switch (m_UpdateState)
    {
    case US_IDLE:                   UpdateIdle(DeltaTime);          break;
    case US_START_REGISTRATION:     StartRegister();                break;
    case US_WAIT_REGISTRATION:      WaitRegister(DeltaTime);        break;
    case US_START_UNREGISTER:       StartUnregister();              break;
    case US_WAIT_UNREGISTER:        WaitUnregister(DeltaTime);      break;
    case US_START_READ_DIRECTORY:   StartReadDirectory();           break;
    case US_WAIT_DIRECTORY:         WaitReadDirectory(DeltaTime);   break;
    default:
        ASSERT(FALSE);
    }
    //
    // Now go through entity list and expire those that have
    // timed out
    //
    count = m_Entities.GetCount();
    for (i=0;i<count;i++)
    {
        server_info *pInfo;

        pInfo = GetEntityInfo(i);
        ASSERT(pInfo);
        if (pInfo->ServerInfo.GameType == -1)
        {
            pInfo->RefreshTimeout -= DeltaTime;
            if (pInfo->RefreshTimeout <= 0.0f)
            {
				pInfo->Retries--;
				if (pInfo->Retries>0)
				{
					pInfo->RefreshTimeout = 3.0f;
					pSvrMgr->SendLookup(PKT_NET_LOOKUP,this->m_ServerEntity.GetAddress(),m_Entities[i]->GetAddress());
				}
				else
				{
					pInfo->RefreshTimeout = 0.0f;
					pInfo->ServerInfo.GameType = -2;
				}
            }
        }
    }
}


//-----------------------------------------------------------------------------
void master_server::AddMasterServer ( const char* pMasterName, const s32 Port )
{
    master_info MasterAddress;
    s32         i;

    ASSERT(x_strlen(pMasterName)<128);
    MasterAddress.Address.Clear();
    MasterAddress.Address.Port = Port;
    MasterAddress.Resolved = FALSE;
    x_strcpy(MasterAddress.Name,pMasterName);

    for (i=0;i<m_MasterServers.GetCount();i++)
    {
        if ( (m_MasterServers[i].Address.Port == MasterAddress.Address.Port) && 
             (x_strcmp(pMasterName,m_MasterServers[i].Name)==0 ) )
        {
            return;
        }
    }

    m_MasterServers.Append(MasterAddress);
    m_CurrentMasterServer.Clear();
}

//-----------------------------------------------------------------------------
void master_server::ClearAttributes ( void )
{
    m_ServerEntity.ClearAttributes();
}

//-----------------------------------------------------------------------------
void master_server::SetAttribute    ( const char* pName,const char* pValue)
{
    m_ServerEntity.SetAttribute(pName,pValue);
}

//-----------------------------------------------------------------------------
const char* master_server::GetAttribute( const char* pName )
{
    return m_ServerEntity.GetAttribute(pName);
}

//-----------------------------------------------------------------------------
void master_server::ClearFilters        ( void )
{
    m_Filters.Clear();
}

//-----------------------------------------------------------------------------
void master_server::GetFilter           (const char* pName,char* pValue,s32& Mode)
{
    s32 count,i;

    count = m_Filters.GetCount();

    for (i=0;i<count;i++)
    {
        if (x_stricmp(pName,m_Filters[i].Name)==0)
        {
            x_strcpy(pValue,m_Filters[i].Value);
            Mode = m_Filters[i].Mode;
            return;
        }
    }
    ASSERT(FALSE);
}

//-----------------------------------------------------------------------------
void master_server::GetFilter           ( s32 index, char* pName, char* pValue, s32& Mode )
{
    ASSERT(index<m_Filters.GetCount());

    x_strcpy(pName,m_Filters[index].Name);
    x_strcpy(pValue,m_Filters[index].Value);
    Mode = m_Filters[index].Mode;
}

//-----------------------------------------------------------------------------
void master_server::SetFilter           (const char *pName,const char *pValue,s32 Mode)
{
    s32 i,count;

    count = m_Filters.GetCount();
    for (i=0;i<count;i++)
    {
        if (x_stricmp(pName,m_Filters[i].Name)==0)
        {
            x_strcpy(m_Filters[i].Value,pValue);
            m_Filters[i].Mode = Mode;
            return;
        }
    }

    {
        entity_attrib attr;

        x_strcpy(attr.Name,pName);
        x_strcpy(attr.Value,pValue);
        attr.Mode = Mode;
        m_Filters.Append(attr);
    }
}


