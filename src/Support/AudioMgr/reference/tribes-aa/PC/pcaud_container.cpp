#include "pcaud_cfx.hpp"
#include "pcaud_container.hpp"
#include "pcaud_audio.hpp"
#include "x_files.hpp"

#define ELEMENT_BUFFER_SIZE 128
//-----------------------------------------------------------------------------
void pccontainer_Init(void)
{
    g_pcAudio.m_pContainers = NULL;
}


cfx_element_attributes *FindDuplicateAttribute(container    *pContainer,cfx_stored_element *pStoredElement)
{
    cfx_element_attributes  Attributes;
    cfx_attrib_list         *pList;
    cfx_element_attributes  *pEntry;
    s32                     i;

    Attributes.m_Type       = pStoredElement->m_Type;
    Attributes.m_Priority   = pStoredElement->m_Priority;
    Attributes.m_Volume     = pStoredElement->m_Volume;
    Attributes.m_Pan        = pStoredElement->m_Pan;
    Attributes.m_Pitch      = pStoredElement->m_Pitch;
    Attributes.m_Count      = pStoredElement->m_Count;
    Attributes.m_Falloff    = pStoredElement->m_Falloff;
    Attributes.m_ADSR1      = pStoredElement->m_ADSR1;
    Attributes.m_ADSR2      = pStoredElement->m_ADSR2;
    Attributes.m_SampleRate = pStoredElement->m_SampleRate;
    Attributes.m_OwnerId    = pStoredElement->m_OwnerId;
    Attributes.m_MediaFile  = pContainer->m_MediaFile;

    // We scan through our current list of attributes to see if we have any duplicates of the same data
    // First, we manicure what we have right now in to the form stored in the duplicates table.

    pList = pContainer->m_pAttributes;
    while (pList)
    {
        pEntry = pList->m_Attributes;
        for (i=0;i<pList->m_InUse;i++)
        {
            // If we do find an identical attrib entry, just return a pointer to that
            if (x_memcmp(pEntry,&Attributes,sizeof(Attributes))==0)
            {
                return pEntry;
            }
            pEntry++;
        }
        pList = pList->m_pNext;
    }
    // Here we did not find any dups, so let's create a new entry
    pList = pContainer->m_pAttributes;
    while (pList)
    {
        if (pList->m_InUse != ATTRIBUTES_PER_BLOCK)
        {
            pEntry = &pList->m_Attributes[pList->m_InUse];
            pList->m_InUse++;
            x_memcpy(pEntry,&Attributes,sizeof(Attributes));
            return pEntry;
        }
        pList = pList->m_pNext;
    }
    // Here, we need to create a new block for duplicates
    pList = (cfx_attrib_list*)x_malloc(sizeof(cfx_attrib_list));
    ASSERT(pList);
    pList->m_pNext = pContainer->m_pAttributes;
    pList->m_InUse = 1;
    pList->m_pNext = NULL;
    pContainer->m_pAttributes = pList;
    x_memcpy(&pList->m_Attributes[0],&Attributes,sizeof(Attributes));
    return &pList->m_Attributes[0];
}

#if 0
void DumpAttributes(container *pContainer)
{
    cfx_attrib_list *pList;
    s32 i,index;

    cfx_element_attributes *pAttr;

    Kprintf("Attribute dump for %d\n",pContainer->m_Id);
    pList = pContainer->m_pAttributes;
    index = 0;
    while (pList)
    {
        pAttr = pList->m_Attributes;
        for (i=0;i<pList->m_InUse;i++)
        {
            Kprintf("Entry %d, Type:%d, Pri:%d, Vol:%d, Pitch:%d, Pan:%d, ADSR:%04x:%04x, Rate:%d\n",
                    index,pAttr->m_Type, pAttr->m_Priority,pAttr->m_Volume,pAttr->m_Pitch,pAttr->m_Pan,
                    pAttr->m_ADSR1,pAttr->m_ADSR2,pAttr->m_SampleRate);
            index++;
            pAttr++;
        }
        pList = pList->m_pNext;
    }
}
#endif
//-----------------------------------------------------------------------------
s32 pccontainer_Load( char* pName, s32* pCfxCount )
{
    container*          pContainer;
    container*          pLast;
    container_header    Header;

    cfx_stored_element* pStoredBuffer;
    cfx_stored_element* pStoredElement;
    cfx_element*        pElement;
    X_FILE*             Handle;
    s32                 Length;
    s32                 spufree;
    s32                 i;
//    char                NameBuffer[64];
    s32                 CurrentMediaLocation;
    s32                 ContainerAllocations;
    cfx_element         *pOther;
    s32                 j;
    s32                 ElementsRemainingInBuffer;
    s32                 TotalElementCount;
    s32                 ElementFileIndex;

    x_DebugMsg("AudioLoadContainer(%s)\n",pName);

    Handle = x_fopen(pName,"rb");
    ASSERT(Handle);
    x_fseek(Handle,0,X_SEEK_END);
    Length = x_ftell(Handle);
    x_fseek(Handle,0,X_SEEK_SET);
    x_fread(&Header,sizeof(container_header),1,Handle);
    ASSERT(x_memcmp(&Header.m_Signature,"inevaud",7)==0);
    ASSERT(Header.m_Version == CONTAINER_VERSION_NUMBER);
    ContainerAllocations = 0;
	
    //
    // Make sure we haven't loaded a container with the same ID yet. This would be BAD!
    //
    pLast = g_pcAudio.m_pContainers;
    while (pLast)
    {
        ASSERT(pLast->m_Id != Header.m_ContainerID);
        pLast = pLast->m_pNext;
    }

    Length = sizeof(container)+Header.m_Count*sizeof(cfx_element);
    pContainer = (container*)x_malloc(Length);
    ASSERT(pContainer);

    ContainerAllocations+=Length;

    pStoredBuffer = (cfx_stored_element*)x_malloc(sizeof(cfx_stored_element) * ELEMENT_BUFFER_SIZE);
    ASSERT(pStoredBuffer);

    pContainer->m_MediaFile     = (s32)Handle;
    pContainer->m_Id            = Header.m_ContainerID<<24;
    pContainer->m_Count         = Header.m_Count;
    pContainer->m_MemoryBase    = spu_Memtop();
    pContainer->m_pAttributes   = NULL;
    pContainer->m_Volume        = 1.0f;
    
	
	// Make sure that this is the element package.
    // This is hack for now.
	if (Header.m_ContainerID == 4)
		g_Miles.AllocSampleData(469);
    
    // Debug, Make me stop when loading the music package.
//    if (Header.m_ContainerID == 3)
//        Header.m_ContainerID = Header.m_ContainerID;

    pElement = pContainer->m_Elements;
    pStoredElement = pStoredBuffer;

    spufree = spu_MemFree();
    CurrentMediaLocation = 0;
    // Although all of this stuff can be done in the loop below, this is being done
    // first so we can discard the pStoredBuffer block of memory to help alleviate
    // the problems caused by memory fragmentation. It *should* mean that all memory
    // allocations for the resident IOP Hybrid streams should be contiguous.
    TotalElementCount = Header.m_Count;
    ElementsRemainingInBuffer = 0;
    ElementFileIndex = x_ftell(Handle);
    
    s32 ParentIndex = 0;

    for (i=0;i<Header.m_Count;i++)
    {
        if (ElementsRemainingInBuffer==0)
        {
            s32 Count;
            
            Count = TotalElementCount;
            if (Count > 128)
                Count = 128;
            x_fseek(Handle,ElementFileIndex,X_SEEK_SET);
            x_fread(pStoredBuffer,ELEMENT_BUFFER_SIZE * sizeof(cfx_stored_element),1,Handle );
            pStoredElement = pStoredBuffer;
            TotalElementCount-= Count;
            ElementFileIndex = x_ftell(Handle);
            ElementsRemainingInBuffer = Count;
        }
        // All of the items loaded in the package need to be split into 2 sections. One, read only
        // and the other read-write. The read-only section should contain data that varies as little
        // as possible from sample to sample. This allows us to save significant amounts of space by
        // eliminating duplicates. All other either modified or unique fields should be stored in the
        // actual element structure.
        pElement->m_pAttributes     = FindDuplicateAttribute(pContainer,pStoredElement);

        pElement->m_MediaLocation   = pStoredElement->m_MediaLocation;
        pElement->m_HybridLength    = 0;
        pElement->m_HybridPosition  = 0;
        pElement->m_Status          = CFXSTAT_IDLE;
        pElement->m_pVoice          = NULL;
        pElement->m_pOwner          = NULL;
        
        // If the elements count is 0 then its a child, since the child doesn't have the corrent attributes
        // we need to know where its parent is to extract the correct data.
        if (pElement->m_pAttributes->m_Count == 0)
        {
            pElement->m_pParent     = &pContainer->m_Elements[ParentIndex];
        }
        else
        {
            pElement->m_pParent     = NULL;
            ParentIndex             = i;
        }

        pElement->m_Length          = pStoredElement->m_Length;
        pElement->m_Flags           = pStoredElement->m_Flags;
        pElement->m_Delay           = pStoredElement->m_Delay;
        pElement++;
        pStoredElement++;
        ElementsRemainingInBuffer--;
    }
    x_free(pStoredBuffer);
	
    // Parse elements, allocate memory as required (if hybrid mainly)
    pElement = pContainer->m_Elements;

    s32 length = 0;
	char name[256] = {0};

    pElement->m_MediaLocation += Header.m_OffsetToSamples;
    for (i=0;i<Header.m_Count;i++)
    {
//        byte *pBuffer;
		
        s32 AlignedLength;

        switch (pElement->m_pAttributes->m_Type)
        {
        case CFXTYPE_ELEMENT:
			pElement->m_MediaLocation += Header.m_OffsetToSamples;

            AlignedLength = (pElement->m_Length + Header.m_Alignment - 1) & ~(Header.m_Alignment-1);
            //
            // Has this media point already been loaded? If so, find it
            //
            pOther = pContainer->m_Elements;

            if (pElement->m_MediaLocation < CurrentMediaLocation)
            {
                for (j=0;j<i;j++)
                {
                    if (pOther->m_MediaLocation == pElement->m_MediaLocation)
                    {
                        ASSERT(pElement->m_pAttributes->m_Type == pOther->m_pAttributes->m_Type);
                        break;
                    }
                    pOther++;
                }
                ASSERT(j!=i);
                pElement->m_SpuLocation = pContainer->m_Elements[j].m_SpuLocation;
            }
            else
            {
// Brian's stuff
//                pBuffer = (byte*)x_malloc(AlignedLength);
//                ASSERT(pBuffer);
//                x_fseek(Handle,pElement->m_MediaLocation,X_SEEK_SET);
//                x_sprintf(NameBuffer,"CFXTYPE_ELEMENT(media=%d)",pElement->m_MediaLocation);
//                pElement->m_SpuLocation = spu_Alloc(AlignedLength,NameBuffer);
//                ASSERT(pElement->m_SpuLocation);
//                x_fread(&pElement->m_AdpcmHeader,sizeof(adpcm_hdr),1,Handle);
//                x_fread(pBuffer,AlignedLength,1,Handle);
				
                // Since effect package is the only package that has any looping sample that are wavs, do special case.
                if (Header.m_ContainerID == 2)
                {
                    s32 loopStart, loopEnd;
                    memset(name,0, sizeof(name));
                    x_fseek(Handle,pElement->m_MediaLocation,X_SEEK_SET);
                    
                    // Get then name of the file, except the last now loop points.
                    x_fread(name, (pElement->m_Length-(2*sizeof(s32))), 1,Handle);
                    x_memcpy(pElement->m_pAttributes->m_Name, name, 60);
                    pElement->m_SpuLocation = g_Miles.LoadSampleData(name, pElement->m_pAttributes->m_OwnerId);
                    
                    // Get the looping data.
                    x_fread(&loopStart, sizeof(s32), 1, Handle);
                    x_fread(&loopEnd, sizeof(s32), 1, Handle);

                    // Do this wether the effect is looped or not.
                    g_Miles.SetLoop(pElement->m_SpuLocation, loopStart, loopEnd);
                }
                else
                {
                    // Get the name and location and let miles load it.
                    memset(name,0, sizeof(name));
                    x_fseek(Handle,pElement->m_MediaLocation,X_SEEK_SET);
                    x_fread(name, pElement->m_Length, 1,Handle);
                    x_memcpy(pElement->m_pAttributes->m_Name, name, 60);
                    pElement->m_SpuLocation = g_Miles.LoadSampleData(name, pElement->m_pAttributes->m_OwnerId);
                }

// Brian's stuff
//                spu_Transfer(pBuffer,pElement->m_SpuLocation,pElement->m_Length,SPUTRANS_WRITE);
//                x_memset(pBuffer,0,AlignedLength);
//                spu_Transfer(pBuffer,pElement->m_SpuLocation,pElement->m_Length,SPUTRANS_READ);
//                x_free(pBuffer);
                CurrentMediaLocation = pElement->m_MediaLocation + pElement->m_Length;
            }
            break;
        case CFXTYPE_ELEMENT_STREAM:
            // Get the name and location of the file.
            pElement->m_MediaLocation += Header.m_OffsetToSamples;
            x_fseek(Handle,pElement->m_MediaLocation,X_SEEK_SET);
            x_fread(pElement->m_pAttributes->m_Name, 60, 1,Handle);

            break;
        case CFXTYPE_ELEMENT_HYBRID:
            // Get the name and location of the file.
            pElement->m_MediaLocation += Header.m_OffsetToSamples;
            x_fseek(Handle,pElement->m_MediaLocation,X_SEEK_SET);
            x_fread(pElement->m_pAttributes->m_Name, 60, 1,Handle);

#if 0
            pOther = pContainer->m_Elements;
            for (j=0;j<i;j++)
            {
                if (pElement->m_MediaLocation == pOther->m_MediaLocation)
                {
                    ASSERT(pElement->m_pAttributes->m_Type == pOther->m_pAttributes->m_Type);
                    break;
                }
                pOther++;
            }
            pElement->m_HybridLength = (pElement->m_Length+4095)&~4095;

            if (j != i)
            {
                pElement->m_pHybridBuffer = pOther->m_pHybridBuffer;
            }
            else
            {
                pElement->m_pHybridBuffer = x_malloc(pElement->m_HybridLength);
                ContainerAllocations += pElement->m_HybridLength;
                ASSERT(pElement->m_pHybridBuffer);
                x_memset(pElement->m_pHybridBuffer,0,pElement->m_HybridLength);
                x_fseek(Handle,pElement->m_MediaLocation,X_SEEK_SET);
                x_fread(pElement->m_pHybridBuffer,pElement->m_Length,1,Handle);
            }
#endif
            break;
        case CFXTYPE_COMPLEX:
            break;
        }

        pElement++;
    }

    pContainer->m_MemoryTop = spu_Memtop();

    pContainer->m_pNext = g_pcAudio.m_pContainers;
    g_pcAudio.m_pContainers = pContainer;
    {
        cfx_attrib_list *pAttribs;
        pAttribs = pContainer->m_pAttributes;
        while (pAttribs)
        {
            ContainerAllocations += sizeof(cfx_attrib_list);
            pAttribs = pAttribs->m_pNext;
        }
    }

    x_fseek(Handle,0,X_SEEK_END);
    Length = x_ftell(Handle);
    x_DebugMsg("Container '%s' id #%d loaded. \n",pName,Header.m_ContainerID);
    x_DebugMsg("     %d bytes long, %d samples\n",Length,Header.m_Count);
    x_DebugMsg("     %d bytes used in ARAM, %d free\n",spufree-spu_MemFree(),spu_MemFree());

//    DumpAttributes(pContainer);
                
    // We don't bother closing the file since it contains hybrid and streamed samples
    //    close(Handle);
    *pCfxCount = pContainer->m_Count;
    return pContainer->m_Id;
}

extern void pcaudio_Unlock(void);
extern void pcaudio_Lock(void);
//-----------------------------------------------------------------------------
s32 pccontainer_Unload(s32 containerid)
{
    container *pContainer,*pLastContainer;
//    s32 i;
    cfx_element *pElement;
    cfx_attrib_list         *pList,*pNext;

    pContainer = g_pcAudio.m_pContainers;

    pLastContainer = NULL;

    // Stop all the sounds
    pcaudio_Lock();


    //audio_StopAll();

    // Wait for all the sounds to have been killed before
    // attempting to unload the package. This will be done
    // on the audio_PeriodicUpdate thread.
    cfx *pCfx;


    pCfx = g_pcAudio.m_pRootCfx;

    // Only cfx's at the top level will have anything useful to
    // be returned to the EE side. The identifiers used below this
    // level are just for internal tracking and debugging.
    while (pCfx)
    {
        pCfx->m_Status = CFXSTAT_STOPPING;
        pCfx = pCfx->m_pNext;
    }

    pcaudio_Unlock();

    while (g_pcAudio.m_pRootCfx)
    {
        x_DelayThread(10);
    }

    while (pContainer)
    {
        if (pContainer->m_Id == containerid)
        {
            break;
        }
        pLastContainer = pContainer;
        pContainer = pContainer->m_pNext;
    }

    if (!pContainer)
        return 0;
    //
    // Check to make sure this was the last container to use any memory within the
    // spu. Containers must be unloaded in reverse order.
    //

    if (pContainer->m_MemoryTop != spu_Memtop() )
    {
        x_DebugMsg("container_Unload: Attempt to unload a container that has data after it.\n");
        return 0;
    }
    // 
    // Remove the container from the list
    //
    if (pLastContainer)
    {
        pLastContainer->m_pNext = pContainer->m_pNext;
    }
    else
    {
        g_pcAudio.m_pContainers = pContainer->m_pNext;
    }
    //
    // Free any spu memory used by this container
    //
    spu_Free(pContainer->m_MemoryBase);

    //
    // Now release all other memory used by this container (iop buffers)
    //
    pElement = pContainer->m_Elements;

/*    for (s32 i=0;i<pContainer->m_Count;i++)
    {
 
        if (pElement->m_pAttributes->m_Type == CFXTYPE_ELEMENT_HYBRID)
        {
            x_free(pElement->m_pHybridBuffer);
        }
        
        pElement++;
    }
*/
    
        
    //
    // Free up any other open resources on this container
    //
    if ( (pContainer->m_MediaFile & (1<<31))==0 )
    {
//        close(pContainer->m_MediaFile);
    }

    pList = pContainer->m_pAttributes;
    while (pList)
    {
        pNext = pList->m_pNext;
        x_free(pList);
        pList = pNext;
    }
    x_free(pContainer);
    return TRUE;
}

container *FindContainer(s32 ContainerId)
{
    container *pContainer;

    pContainer = g_pcAudio.m_pContainers;
    while (pContainer)
    {
        if (pContainer->m_Id == ContainerId)
            break;
        pContainer = pContainer->m_pNext;
    }
    return pContainer;
}
//-----------------------------------------------------------------------------
cfx_element *pccontainer_Find(s32 identifier)
{
    s32 ContainerId;
    container *pContainer;

    ContainerId = identifier & (255<<24);
    identifier = identifier & ((1<<24)-1);

    pContainer = FindContainer(ContainerId);
    ASSERT(pContainer);
    ASSERT(identifier < pContainer->m_Count);
    return &pContainer->m_Elements[identifier];

}

//-----------------------------------------------------------------------------
f32 pccontainer_GetVolume(s32 ContainerId)
{

    static s32 LastContainerId=-1;
    static container *pContainer=NULL;

    // ***SHORTCUT***, if we're looking for the same container as the
    // last time this routine was called, let's just pass back what we
    // had then.
    if (ContainerId != LastContainerId)
    {
        pContainer = FindContainer(ContainerId);
    }
    ASSERT(pContainer);
    LastContainerId = ContainerId;
    return pContainer->m_Volume;
}

//-----------------------------------------------------------------------------
void pccontainer_SetVolume(s32 ContainerId,f32 Volume)
{
    container *pContainer;

    pContainer = FindContainer(ContainerId);
    ASSERT(pContainer);
    pContainer->m_Volume = Volume;
    
}

//-----------------------------------------------------------------------------
cfx_state *pccontainer_Header(s32 ContainerId,cfx_info *pInfo)
{
    container *pContainer;
    s32 i;
    cfx_element *pElement;

    pContainer = g_pcAudio.m_pContainers;

    while (pContainer)
    {
        if (pContainer->m_Id == ContainerId)
            break;
        pContainer = pContainer->m_pNext;
    }
    ASSERT(pContainer);
    // We cannot use the pHeader block since it's just about to be
    // overwritten by the actual cfx_state structures for this
    // container.
    pElement = pContainer->m_Elements;
    for (i=0;i<pContainer->m_Count;i++)
    {
        ASSERT(pElement->m_pAttributes);
        pInfo->m_Pan     = pElement->m_pAttributes->m_Pan;
        pInfo->m_Pitch   = pElement->m_pAttributes->m_Pitch;
        pInfo->m_Volume  = pElement->m_pAttributes->m_Volume;
        pInfo->m_Falloff = pElement->m_pAttributes->m_Falloff;
        pInfo++;
        pElement++;
    }

    return NULL;
}


