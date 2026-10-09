#include "masterserver.hpp"
#include "masterentity.hpp"

//-----------------------------------------------------------------------------
single_entity::single_entity(void)
{
    ClearPath();
    ClearName();
    ClearDisplayName();
    ClearAddress();
    ClearInfo();
}

//-----------------------------------------------------------------------------
single_entity::~single_entity(void)
{
}

//-----------------------------------------------------------------------------
single_entity::single_entity(bytestream& Bytestream,s32 Flags)
{
    s32 Count;
    s32 i;
	xwchar buffer[128];
    // Empty all the incoming data items just in case they are not contained within
    // this packet.
    ClearPath();
    ClearName();
    ClearDisplayName();
    ClearAddress();
    ClearInfo();

    if (Flags & DIR_GF_ADDTYPE)
    {
        m_Type = Bytestream.GetByte();
    }

    if (m_Type =='S' )
    {
        if (Flags & DIR_GF_SERVADDPATH)
        {
            Bytestream.GetWideString(buffer);
        }
        if (Flags & DIR_GF_SERVADDNAME)
        {
            Bytestream.GetWideString(buffer);
        }

        if (Flags & DIR_GF_SERVADDNETADDR)
        {
            s32 length;
            length = Bytestream.GetByte();
            if (length == 6)
            {
                m_Address.Port = Bytestream.GetNetWord();
                m_Address.IP   = Bytestream.GetLong();
            }
            else
            {
                ASSERT(length==0);
                while (length)
                {
                    Bytestream.GetByte();
                    length--;
                }
            }
        }
    }

	m_Path[0]=0x0;
	m_Name[0]=0x0;

    if (m_Type == 'D' )
    {
        if (Flags & DIR_GF_DIRADDPATH)
        {
            Bytestream.GetWideString(buffer);
        }
        if (Flags & DIR_GF_DIRADDNAME)      
        {
            Bytestream.GetWideString(buffer);
        }
        if (Flags & DIR_GF_DIRADDREQUNIQUE) m_RequestUnique = Bytestream.GetByte();
    }

    if (Flags & DIR_GF_ADDDISPLAYNAME)
    {
        Bytestream.GetWideString(buffer);
    }
    if (Flags & DIR_GF_ADDLIFESPAN)         m_Lifespan = Bytestream.GetLong();
    if (Flags & DIR_GF_ADDCREATED)          m_Created = Bytestream.GetLong();
    if (Flags & DIR_GF_ADDTOUCHED)          m_Touched = Bytestream.GetLong();
    if (Flags & DIR_GF_ADDCRC)              m_CRC = Bytestream.GetLong();

    if (Flags & DIR_GF_ADDDATAOBJECTS)
    {
        Count = Bytestream.GetWord();               // Number of data objects
        for (i=0;i<Count;i++)
        {
            entity_attrib Attr;
            s32 length;
            s32 modified;

            x_memset(&Attr,0,sizeof(Attr));

            modified = FALSE;
            if (Flags & DIR_GF_ADDDOTYPE)
            {
                length = Bytestream.GetByte();              // Length of data object type, byte
                Bytestream.GetBytes((byte *)Attr.Name,length);      // Name of data object type
                modified = TRUE;
            }

            if (Flags & DIR_GF_ADDDODATA)
            {
                length = Bytestream.GetWord();              // Length of data value
                Bytestream.GetBytes((byte *)Attr.Value,length);     // Name of data value
                modified = TRUE;
            }
            if (modified)
            {
                SetAttribute(Attr);
            }

        }
	}
    if (Flags & DIR_GF_ADDACLS)             ASSERT(FALSE);
    // Dup some information to the server info struct
    // since we want to display the dest server name and IP
    m_ServerInfo.Info.IP    = m_Address.IP;
    m_ServerInfo.Info.Port  = m_Address.Port;
	m_ServerInfo.ServerInfo.Name[0] = 0x0;
    x_wstrcpy( m_ServerInfo.ServerInfo.Name, m_DisplayName );
    // Give the server 10 seconds to respond
    m_ServerInfo.RefreshTimeout = x_frand(0.1f,1.0f);
	m_ServerInfo.Retries = 4;
}

//-----------------------------------------------------------------------------
void single_entity::ClearAttributes(void)
{
    m_Attributes.Clear();
}

//-----------------------------------------------------------------------------
s32 single_entity::GetAttributeCount(void)
{
    return m_Attributes.GetCount();
}

//-----------------------------------------------------------------------------
const char* single_entity::GetAttribute(const char* pName)
{
    s32 count,i;

    count = GetAttributeCount();

    for (i=0;i<count;i++)
    {
       if (x_stricmp(pName,m_Attributes[i].Name)==0)
        {
            return m_Attributes[i].Value;
        }
    }
    return NULL;
}

//-----------------------------------------------------------------------------
void single_entity::GetAttribute(s32 index, char* pName, char* pValue)
{
    ASSERT( index < GetAttributeCount() );

    x_strcpy( pName, m_Attributes[index].Name  );
    x_strcpy( pValue,m_Attributes[index].Value );

}

//-----------------------------------------------------------------------------
void single_entity::SetAttribute(const char* pName, const char* pValue)
{
    s32 count,i;
    entity_attrib Attr;

    // Check to see if this attribute is already in our attribute list.
    // if so, let's overwrite it otherwise append a new attribute to
    // the list.
    count = GetAttributeCount();

    for (i=0;i<count;i++)
    {
        if (x_stricmp(pName,m_Attributes[i].Name)==0)
        {
            x_strcpy(m_Attributes[i].Value,pValue);
            return;
        }
    }

    x_strcpy(Attr.Name,pName);
    x_strcpy(Attr.Value,pValue);
    m_Attributes.Append(Attr);
}

//-----------------------------------------------------------------------------
void    single_entity::SetAttribute(const entity_attrib &Attr)
{
    m_Attributes.Append(Attr);
}

//-----------------------------------------------------------------------------
void single_entity::SetAddress          (const net_address Address)
{
    m_Address = Address;
}

//-----------------------------------------------------------------------------
void single_entity::SetDisplayName      (const xwchar* pDisplayName)
{
    x_wstrcpy(m_DisplayName,pDisplayName);
}

//-----------------------------------------------------------------------------
void single_entity::SetName             (const xwchar* pName)
{
    x_wstrcpy(m_Name,pName);
}

//-----------------------------------------------------------------------------
void single_entity::SetPath             (const xwchar* pPath)
{
    x_wstrcpy(m_Path,pPath);
}

void single_entity::ClearInfo           (void)
{
    // We clear the entity server information and set the gametype
    // to -1 so that we know we haven't yet received a response
    x_memset(&m_ServerInfo,0,sizeof(m_ServerInfo));
    m_ServerInfo.ServerInfo.GameType = -1;
    x_wstrcpy(m_ServerInfo.ServerInfo.Name,m_DisplayName);

}


