#ifndef __SVR_REQUESTS_H
#define __SVR_REQUESTS_H

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_array.hpp>
#include "netlib/netlib.hpp"
#include <xCore/x_files/x_threads.hpp>

#include "masterentity.hpp"

enum DirGetFlags
{
	// Bits 0-15 are for decomposition and common flags

	// Decomposition Flags - apply these decompositions for Directories
	DIR_GF_DECOMPROOT      = 0x00000001,  // Add the dir itself 
	DIR_GF_DECOMPSERVICES  = 0x00000002,  // Add dir services
	DIR_GF_DECOMPSUBDIRS   = 0x00000004,  // Add dir subdirs
	DIR_GF_DECOMPRECURSIVE = 0x00000008,  // Recursive into dir subdirs

	// Common flags - include these attributes for all entities
	DIR_GF_ADDTYPE         = 0x00000010,  // Add entity types
	DIR_GF_ADDDISPLAYNAME  = 0x00000020,  // Add display names
	DIR_GF_ADDCREATED      = 0x00000040,  // Add creation date/time
	DIR_GF_ADDTOUCHED      = 0x00000080,  // Add touched date/time
	DIR_GF_ADDLIFESPAN     = 0x00000100,  // Add lifespan
	DIR_GF_ADDDOTYPE       = 0x00000200,  // Add DataObject types
	DIR_GF_ADDDODATA       = 0x00000400,  // Add DataObject data
	DIR_GF_ADDDATAOBJECTS  = 0x00000800,  // Add all DataObjects
	DIR_GF_ADDACLS         = 0x00001000,  // Add ACLs
	DIR_GF_ADDCRC          = 0x00002000,  // Add entity CRC
	DIR_GF_ADDUIDS         = 0x00004000,  // Add create and touch user ids
	DIR_GF_ADDORIGIP       = 0x00008000,  // Add originating IP address (admins only)

	// Bits 16-23 are for Directory only fields

	// Directory Flags - include these attributes for directories
	DIR_GF_DIRADDPATH      = 0x00010000,  // Add dir paths (from root)
	DIR_GF_DIRADDNAME      = 0x00020000,  // Add service names
	DIR_GF_DIRADDREQUNIQUE = 0x00040000,  // Add directory unqiue display name flag

	// Bits 24-31 are for Service only fields

	// Service Flags - include these attributes for services
	DIR_GF_SERVADDPATH     = 0x01000000,  // Add dir paths (from root)
	DIR_GF_SERVADDNAME     = 0x02000000,  // Add service names
	DIR_GF_SERVADDNETADDR  = 0x04000000,  // Add service net addresses
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Entity Flags (byte) - Control attributes in add/change requests
enum DirUpdateFlags
{
	DIR_UF_UNIQUEDISPLAYNAME = 0x01,  // Display name must be unique
	DIR_UF_DIRNOTUNIQUE      = 0x02,  // Directory allows duplicate display names
	DIR_UF_DIRREQUNIQUE      = 0x04,  // Directory requires unique display names
	DIR_UF_OVERWRITE         = 0x08,  // Overwrite existing entities

	// These 2 flags SHARE the 0x10 value (by design)
	DIR_UF_SERVRETURNADDR    = 0x10,  // Return service net address in reply
	DIR_UF_DIRNOACLINHERIT   = 0x10,  // Do not inherit parent ACLs for AddDirs

	DIR_UF_SERVGENNETADDR    = 0x20,  // Generate NetAddr from connection (AddService(Ex) only)
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// DataObjectSetMode (byte) - Mode for setting data objects on a DirEntity
enum DirDataObjectSetMode
{
	DIR_DO_ADDREPLACE    = 0,  // Add on not exist, replace on exist
	DIR_DO_ADDIGNORE     = 1,  // Add on not exist, ignore on exist
	DIR_DO_ADDONLY       = 2,  // Add on not exist, error on exist
	DIR_DO_REPLACEIGNORE = 3,  // Replace on exist, ignore on not exist
	DIR_DO_REPLACEONLY   = 4,  // Replace on exist, error on not exist
	DIR_DO_RESETDELETE   = 5,  // Clear existing set first, then add all.
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FindMatchMode (byte) - Mode for find queries
enum DirFindMatchMode
{
	DIR_FMM_EXACT   = 0,  // Compared value must equal search value
	DIR_FMM_BEGIN   = 1,  // Compared value must begin with search value
	DIR_FMM_END     = 2,  // Compared value must end with search value
	DIR_FMM_CONTAIN = 3   // Compared value must contain search value
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FindFlags (byte) - Control flags for find queries
enum DirFindFlags
{
	DIR_FF_MATCHALL  = 0x01,  // Return all valid matches
	DIR_FF_FULLKEY   = 0x02,  // Match only if all search field match
	DIR_FF_RECURSIVE = 0x04   // Search directories recursively for matches
};


enum message_class
{
    MC_SMALL_MESSAGE = 5,

};

enum service_type
{
    ST_DIRECTORY_SERVER = 2,
};

enum message_type
{
    MT_STATUS_REPLY                         = 1,
    MT_SINGLE_ENTITY_REPLY                  = 2,
    MT_MULTI_ENTITY_REPLY                   = 3,
    MT_GET_DIRECTORY_CONTENTS               = 100,
    MT_GET_DIRECTORY_CONTENTS_REPLY         = 101,
    MT_GET_DIRECTORY                        = 102,
    MT_GET_DIRECTORY_EX                     = 103,
    MT_GET_SERVICE                          = 104,
    MT_GET_SERVICE_EX                       = 105,
    MT_ADD_DIRECTORY                        = 200,
    MT_ADD_SERVICE                          = 202,
    MT_RENEW_SERVICE                        = 205,
    MT_NAME_SERVICE                         = 207,
    MT_REMOVE_DIRECTORY                     = 208,
    MT_REMOVE_SERVICE                       = 209,
    MT_MODIFY_DIRECTORY                     = 210,
    MT_MODIFY_SERVICE                       = 212,
    MT_ADD_DIRECTORY_EX                     = 214,
    MT_ADD_SERVICE_EX                       = 215,
    MT_MODIFY_DIRECTORY_EX                  = 216,
    MT_MODIFY_SERVICE_EX                    = 217,

    MT_DIRECTORY_SET_DATA_OBJECTS           = 300,
    MT_SERVICE_SET_DATA_OBJECTS             = 301,
    MT_DIRECTORY_CLEAR_DATA_OBJECTS         = 302,
    MT_SERVICE_CLEAR_DATA_OBJECTS           = 303,
    MT_DIRECTORY_MODIFY_DATA_OBJECTS        = 304,
    MT_SERVICE_MODIFY_DATA_OBJECTS          = 305,
    MT_DIRECTORY_EXPLICIT_SET_DATA_OBJECTS  = 306,
    MT_SERVICE_EXPLICIT_SET_DATA_OBJECTS    = 307,

	MT_RESPONSES_RECEIVED					= 115,
};

#define MS_MAX_NAME_LENGTH  64
#define MS_RECEIVE_TIME     (20.0f)         // Timeout for receive
#define MS_DIRECTORY_TIMEOUT (5.0f)
#define MS_REREGISTER_TIME  (4.0f*60.0f)    // Re-register time
#define MS_LIFESPAN         (5.0f*60.0f)    // Lifespan of this entity. Should be > than MS_REREGISTER_TIME

enum update_state
{
    US_IDLE = 0,
    US_START_REGISTRATION,
    US_WAIT_REGISTRATION,
    US_START_UNREGISTER,
    US_WAIT_UNREGISTER,
    US_START_READ_DIRECTORY,
    US_WAIT_DIRECTORY,
    US_START_ROUTING,
    US_WAIT_ROUTING,
};

struct master_info
{
    master_info         (void)   { Resolved = FALSE;Address.Clear();*Name=0x0;};
    ~master_info        (void)  { };

    char                Name[128];
    xbool               Resolved;
    net_address         Address;
};

class master_server
{
public:
    master_server(void);
    ~master_server(void);

    void                Init                ( void );
    void                Kill                ( void );
    void                Update              ( f32 DeltaTime );
    void                Reset               ( void );
                                    
    single_entity*      GetEntity           ( s32 Index );
    server_info*        GetEntityInfo       ( s32 Index );
    void                DeleteEntity        ( s32 Index );
    s32                 GetEntityCount      ( void );
    void                SetPath             ( const xwchar* pPath );
    void                SetName             ( const xwchar* pName );
    void                SetServer           ( xbool isServer );
    void                AcquireDirectory    ( xbool isEnabled );
    void                ProcessPacket       ( const byte *pBuffer,s32 Length, const net_address &Local, const net_address &Remote );
    void                SetAddress          ( const net_address Address );
    void                ClearAddress        ( void );
    void                AddMasterServer     ( const char* pMasterName, const s32 Port );
	const net_address&	GetMasterServerAddr	( void );
#if 0
    const xwchar*       GetPath             ( void ) {return m_ServerEntity.GetPath();};
    const xwchar*       GetName             ( void ) {return m_ServerEntity.GetName();};
#endif
    update_state        GetCurrentState     ( void );
                                    
    void                ClearAttributes     ( void );
    void                SetAttribute        ( const char* pName, const char* pValue);
    const char*         GetAttribute        ( const char* pName );
                                    
    void                ClearFilters        ( void );
    void                GetFilter           ( const  char* pName,       char* pValue, s32& Mode );
    void                GetFilter           ( s32 i, char* pName,       char* pValue, s32& Mode );
    void                SetFilter           ( const  char* pName, const char* pValue, s32  Mode );
    void                EnableFilter        ( xbool IsOn ) { m_FilterEnabled = IsOn; };
	xbool				IsRegistered		( void ) { return m_ExternalAddress.IP != 0;};
	s32					GetServerCount		(void) { return m_ServerCount;};


private:
    void                StartRegister       (void);
    void                StartUnregister     (void);
    void                StartReadDirectory  (void);
    void                CycleMasterServer   (void);

    void                WaitRegister        (f32 DeltaTime);
    void                WaitUnregister      (f32 DeltaTime);
    void                WaitReadDirectory   (f32 DeltaTime);
    void                UpdateIdle          (f32 DeltaTime);
    xbool               ParsePacket         (byte* pBuffer,s32 Length,const net_address &Local, const net_address &Remote);

private:
    xbool                           m_Initialized;
    xbool                           m_StatusReceived;
    xbool                           m_DirectoryReadComplete;
    xbool                           m_AcquireDirectory;
    xbool                           m_IsServer;
                                    
    xarray<single_entity*>          m_Entities;
    xarray<master_info>             m_MasterServers;
                                    
    // Polling times                
    f32                             m_Timeout;
    f32                             m_ScanTimeout;
    f32                             m_RegistrationRetryTime;
    f32                             m_DirectoryReadTimeout;
    s32                             m_DirectoryReadRetries;
	s32								m_MasterServerRetries;
                              
    // Network stuff                
    net_address                     m_CurrentMasterServer;
    net_address                     m_ExternalAddress;
    xmesgq*                         m_pqPendingRequests;
    //
    // Server registration information
    //
    s32                             m_RegistrationRetry;
    s32                             m_LastStatus;
    byte                            m_RequestBuffer[MAX_PACKET_SIZE];
    update_state                    m_UpdateState;
    single_entity                   m_ServerEntity;
    xarray<entity_attrib>           m_Filters;
    xbool                           m_FilterEnabled;
    s32                             m_ServerIndex;
	s32								m_ServerCount;	
	void							SendNakStream(s32 Sequence);
	void							AckPacket(s32 Sequence);
    

};

#endif
