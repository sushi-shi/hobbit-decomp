#ifndef __MASTERENTITY_HPP
#define __MASTERENTITY_HPP

#include "netlib/netlib.hpp"
#include "bytestream.hpp"
#include "../AADS/serverman.hpp"

#define ME_MAX_ATTRIB_LENGTH    16
#define ME_MAX_NAME_LENGTH      16

struct entity_attrib
{
    s32         Mode;
    char        Name [ME_MAX_ATTRIB_LENGTH];
    char        Value[ME_MAX_ATTRIB_LENGTH];
};

// A single entity can have a list of attributes associated with it. These attributes will be forwarded
// to the master server if the entity is just being created otherwise it will be generally acquired from
// the master server on a server grab.
class single_entity
{
public:
    single_entity           (void);
    single_entity           (bytestream& Bytestream,s32 Flags);
   ~single_entity           (void);

    void                    ClearAttributes     (void);

    s32                     GetAttributeCount   (void);
    const char*             GetAttribute        (const char* pName);
    void                    GetAttribute        (s32 index, char* pName,char* pValue);
    void                    SetAttribute        (const char* pName, const char* pValue);
    void                    SetAttribute        (const entity_attrib &Attr);

    const net_address&      GetAddress          (void) { return m_Address;              };
    const xwchar*           GetDisplayName      (void) { return m_DisplayName;          };
    const xwchar*           GetName             (void) { return m_Name;                 };
    const xwchar*           GetPath             (void) { return m_Path;                 };
    server_info*            GetInfo             (void) { return &m_ServerInfo;           };

    void                    SetAddress          (const net_address Address);
    void                    SetDisplayName      (const xwchar* pDisplayName);
    void                    SetName             (const xwchar* pName);
    void                    SetPath             (const xwchar* pPath);

    void                    ClearAddress        (void) { m_Address.Clear();             };
    void                    ClearName           (void) { m_Name[0]=0x0;                 };
    void                    ClearDisplayName    (void) { m_DisplayName[0]=0x0;          };
    void                    ClearPath           (void) { m_Path[0]=0x0;                 };
    void                    ClearInfo           (void);

private:

    xwchar                  m_Type;
    xwchar                  m_Path[ME_MAX_NAME_LENGTH];
    xwchar                  m_Name[ME_MAX_NAME_LENGTH];
    xwchar                  m_DisplayName[ME_MAX_NAME_LENGTH];
    xbool                   m_UniqueDisplayNames;
    xbool                   m_RequestUnique;
    server_info             m_ServerInfo;
    net_address             m_Address;
    s32                     m_Lifespan;
    s32                     m_Created;
    s32                     m_Touched;
    s32                     m_CRC;
    s32                     m_NumDataObjects;
    xarray<entity_attrib>   m_Attributes;
};


#endif // __MASTERENTITY_HPP

