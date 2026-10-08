#ifndef __GCNAUD_CONTAINER
#define __GCNAUD_CONTAINER

#include "x_types.hpp"
#include "gcnaud_cfx.hpp"

#define CONTAINER_VERSION_NUMBER 5


typedef struct s_container
{
    struct s_container *m_pNext;
    s32             m_Id;
    s32             m_Count;            // Number of samples within this container
    s32             m_MediaFile;        // File ptr used for streaming
    s32             m_MemoryBase;       // Base address of spu memory used by this container
    s32             m_MemoryTop;        // End address of spu memory used by this container
    f32             m_Volume;           // Base volume for this container.
    s32             *m_ContainerHeader; // Contains a list of data to be passed back to the EE
    cfx_attrib_list *m_pAttributes;
    cfx_element     m_Elements[1];      // This MUST be the last field within the struct since it's dynamically sized
} container;



typedef struct s_container_header
{
    char    m_Signature[7];             // 'inevaud'
    u8      m_Version;                  // See above CONTAINER_VERSION_NUMBER
    u8      m_ContainerID;
    s32     m_Count;                    // # samples
    s32     m_Length;                   // Length of container data
    s32     m_Alignment;                // # byte alignment (16)
    s32     m_OffsetToSamples;          // Actual base offset within file of the sample data
} container_header;

void            gcncontainer_Init(void);
s32             gcncontainer_Load(char *pName,s32 *pCount);
s32             gcncontainer_Unload(s32 ContainerId);
cfx_element*    gcncontainer_Find(s32 ElementId);
cfx_state*      gcncontainer_Header(s32 ContainerId,cfx_info *pInfo);
void            gcncontainer_SetVolume(s32 ContainerId,f32 Volume);
f32             gcncontainer_GetVolume(s32 identifier);

#endif