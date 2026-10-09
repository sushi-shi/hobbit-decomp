//=========================================================================
//
// ps2_dmaman.cpp
//
//=========================================================================
#include <xCore/x_files/x_debug.hpp>
#include <xCore/x_files/x_time.hpp>
#include <xCore/x_files/x_plus.hpp>
#include <xCore/Entropy/PS2/ps2_misc.hpp>
#include "iop/iop.hpp"
#include <xCore/x_files/x_threads.hpp>


//=========================================================================

#define MAX_TASKS   128

#define DMA_CHCR (0x00001C5)
// chcr.DIR = 1; // From memory
// chcr.MOD = 1; // Chain
// chcr.ASP = 0; // No tag on stack
// chcr.TTE = 1; // Transfer tag
// chcr.TIE = 1; // Interrupt enabled
// chcr.STR = 1; // Start

//=========================================================================

struct dma_task
{
    u32     DMAAddr;
    xtick   StartTime;
    xtick   EndTime;
    xtick   CPUTime;
    char    Name[32];
    s32     VRAMBytesLoaded;
    s32     DListBytesUsed;
};

struct task_list
{
    xtick       StartTime;
    xtick       EndTime;

    dma_task        Task[ MAX_TASKS ];

    s32         NTasks;
    s32         PrevNTasks;
    s32         CurrentTaskID;
    xbool       IsFinished;

};

//=========================================================================

       task_list        s_List[2];
volatile       task_list*       s_RunList;
volatile       task_list*       s_BuildList;
static sceDmaChan*      s_DMAChannel;
static xbool            s_DMAHandlersOn;
       s32              s_HandlerId;

volatile s32 NInt=0;
volatile s64 NITicks1=0;
volatile s64 NITicks2=0;
volatile s32 NInterrupts=0;
volatile s32 NInterruptsOn=0;

// These structs allow us to look at the hardware registers easily.

struct t_gifregs
{
    tGIF_CTRL ctrl PS2_ALIGNMENT(16);
    tGIF_MODE mode PS2_ALIGNMENT(16);
    tGIF_STAT stat PS2_ALIGNMENT(16);
    u32       Dummy PS2_ALIGNMENT(16);
    tGIF_TAG0 tag0 PS2_ALIGNMENT(16);
    tGIF_TAG1 tag1 PS2_ALIGNMENT(16);
    tGIF_TAG2 tag2 PS2_ALIGNMENT(16);
    tGIF_TAG3 tag3 PS2_ALIGNMENT(16);
    tGIF_CNT  cnt  PS2_ALIGNMENT(16);
    tGIF_P3CNT p3cnt PS2_ALIGNMENT(16);
    tGIF_P3TAG p3tag PS2_ALIGNMENT(16);
};

struct t_vif1regs
{
    tVIF1_STAT  stat    PS2_ALIGNMENT(16);
    tVIF1_FBRST fbrst   PS2_ALIGNMENT(16);
    tVIF1_ERR   err     PS2_ALIGNMENT(16);
    tVIF_MARK   mark    PS2_ALIGNMENT(16);
    tVIF_CYCLE  cycle   PS2_ALIGNMENT(16);   
    tVIF_MODE   mode    PS2_ALIGNMENT(16);
    tVIF1_NUM   num     PS2_ALIGNMENT(16);
    tVIF_MASK   mask    PS2_ALIGNMENT(16);
    tVIF_CODE   code    PS2_ALIGNMENT(16);
    tVIF_ITOPS  itops   PS2_ALIGNMENT(16);
    tVIF1_BASE  base    PS2_ALIGNMENT(16);
    tVIF1_OFST  ofst    PS2_ALIGNMENT(16);
    tVIF1_TOPS  tops    PS2_ALIGNMENT(16);
    tVIF_ITOP   itop    PS2_ALIGNMENT(16);
    tVIF1_TOP   top     PS2_ALIGNMENT(16);
};

struct t_dmactrlregs
{
    tD_CTRL     d_ctrl  PS2_ALIGNMENT(16);
    tD_STAT     d_stat  PS2_ALIGNMENT(16);
    tD_PCR      d_pcr   PS2_ALIGNMENT(16);
    tD_SQWC     d_sqwc  PS2_ALIGNMENT(16);
    tD_RBSR     d_rbsr  PS2_ALIGNMENT(16);
    tD_RBOR     d_rbor  PS2_ALIGNMENT(16);
    tD_STADR    d_stadr PS2_ALIGNMENT(16);
};

struct t_dmachanregs
{
    tD_CHCR     d_chcr  PS2_ALIGNMENT(16);
    tD_MADR     d_madr  PS2_ALIGNMENT(16);
    tD_QWC      d_qwc   PS2_ALIGNMENT(16);
    tD_TADR     d_tadr  PS2_ALIGNMENT(16);
    tD_ASR0     d_asr0  PS2_ALIGNMENT(16);
    tD_ASR1     d_asr1  PS2_ALIGNMENT(16);
};

struct t_gsregs
{
    tGS_PMODE       gs_pmode    PS2_ALIGNMENT(0x20);
    u32             gs_pad0[4]  PS2_ALIGNMENT(0x10);
    tGS_SMODE2      gs_smode2   PS2_ALIGNMENT(0x10);
    tGS_DISPFB1     gs_dispfb1  PS2_ALIGNMENT(0x10);
    tGS_DISPLAY1    gs_display1 PS2_ALIGNMENT(0x10);
    tGS_DISPFB2     gs_dispfb2  PS2_ALIGNMENT(0x10);
    tGS_DISPLAY2    gs_display2 PS2_ALIGNMENT(0x10);
    tGS_EXTBUF      gs_extbuf   PS2_ALIGNMENT(0x10);
    tGS_EXTDATA     gs_extdata  PS2_ALIGNMENT(0x10);
    tGS_EXTWRITE    gs_extwrite PS2_ALIGNMENT(0x10);
    tGS_BGCOLOR     gs_bgcolor  PS2_ALIGNMENT(0x10);
    u32             gs_pad1[0x3d4] PS2_ALIGNMENT(0x10);
    tGS_CSR         gs_csr      PS2_ALIGNMENT(0x10);
    tGS_IMR         gs_imr      PS2_ALIGNMENT(0x10);
};

volatile t_gifregs*      gifregs     = (t_gifregs*)GIF_CTRL;
volatile t_gsregs*       gsregs      = (t_gsregs*)GS_PMODE;
volatile t_vif1regs*     vif1regs    = (t_vif1regs*)VIF1_STAT;
volatile t_dmactrlregs*  dmactrlregs = (t_dmactrlregs*)D_CTRL;
volatile t_dmachanregs*  dma1regs    = (t_dmachanregs*)D1_CHCR;

static xthread* s_DmaThread;
static s32 s_DmaSemaphore;
static s32 s_StartDmaSemaphore;

//=========================================================================
//=========================================================================
//=========================================================================
// INTERRUPT HANDLER
//=========================================================================
//=========================================================================
//=========================================================================

//=========================================================================

static 
int DMAInterruptHandler( int )
{
    if (s_DMAHandlersOn == FALSE)
    {
        return 1;
    }
    // We can't use sema_Signal since it's not an interrupt context semaphore
    NInterrupts++;
	iSignalSema(s_DmaSemaphore);
    ExitHandler();
    return -1;
}

//=========================================================================
static void s_DmaPeriodicUpdate(void)
{
    volatile dma_task* pTask;    

    while (1)
    {
		WaitSema(s_StartDmaSemaphore);
        ASSERT(s_DMAHandlersOn);
        s_RunList->StartTime = x_GetTime();

        while (s_RunList->CurrentTaskID != s_RunList->NTasks)
        {
            // Launch task
            ASSERT(s_RunList->CurrentTaskID < s_RunList->NTasks);
            pTask = &s_RunList->Task[ s_RunList->CurrentTaskID ];
            pTask->StartTime = x_GetTime();
            // Let path3 be intermittent    
            // continuous = 0
            // intermittent = 4
            (*GIF_MODE) |= 0x04;

	        s_DMAChannel->chcr.TTE = 1;
	        s_DMAChannel->chcr.TIE = 1;
            u32 C = *D_STAT;
            if( ((C>>(16+1))&0x01) == 0 )
                *D_STAT = (u32)((1<<(16+1)) | (1<<(1)));

            sceDmaSend( s_DMAChannel, (void*)pTask->DMAAddr);

            // Wait until task is complete
			WaitSema(s_DmaSemaphore);
			if (NInterrupts != 1)
			{
				x_DebugMsg("s_DmaPeriodicUpdate: Got a bad interrupt condition\n");
			}
			NInterrupts = 0;
            pTask->EndTime = x_GetTime();
            s_RunList->CurrentTaskID++;
        }
        s_RunList->EndTime = x_GetTime();
        s_RunList->IsFinished = TRUE;
    }
}

//=========================================================================

static
void InitDMAHandlers( void )
{
    // Create thread to handle DMA interrupts

	struct SemaParam sema;
	sema.initCount = 0;
	sema.maxCount = 2;
	sema.option = 0;

	s_DmaSemaphore  = CreateSema(&sema);
    s_StartDmaSemaphore = CreateSema(&sema);

    // This mutex needs to be blocked to start with.
    s_DmaThread = new xthread(s_DmaPeriodicUpdate,"DMA Handler",4096,5);

    // Install Interrupt Handler for end of dma transfer
#if PATH
    s_HandlerId = AddDmacHandler( DMAC_GIF, DMAInterruptHandler, 0 );
    EnableDmac( DMAC_GIF );
#else
    s_HandlerId = AddDmacHandler( DMAC_VIF1, DMAInterruptHandler, 0 );
    EnableDmac( DMAC_VIF1 );
#endif
}

//=========================================================================
static
void KillDMAHandlers( void )
{
    // Remove Interrupt Handler and Disable Interrupt
#if PATH
    DisableDmac( DMAC_GIF );
    RemoveDmacHandler( DMAC_GIF, s_HandlerId );
#else
    DisableDmac( DMAC_VIF1 );
    RemoveDmacHandler( DMAC_VIF1, s_HandlerId );
#endif
    // Destroy threads and semaphores used by dma handler
    delete s_DmaThread;
    DeleteSema(s_DmaSemaphore);
    DeleteSema(s_StartDmaSemaphore);
}

//=========================================================================

void dmaman_Kill( void )
{
    KillDMAHandlers();
}

//=========================================================================

void dmaman_Init( void )
{
    // Get dma channel
    s_DMAChannel = sceDmaGetChan(SCE_DMA_VIF1);
	
	// Install interrupts
	s_DMAHandlersOn = FALSE;
	InitDMAHandlers();
	
	// Setup initial chain
    x_memset( s_List, 0, sizeof(s_List) );
    s_BuildList = &s_List[0];
    s_RunList = &s_List[1];
    s_RunList->IsFinished = TRUE;
}

//=========================================================================

void    dmaman_NewTask          ( void* DMAAddr, 
                                  char* pTaskName )
{
    volatile dma_task* pTask;

    // Add dma_task to BuildList1
    ASSERT( s_BuildList->NTasks < MAX_TASKS );
    pTask = &s_BuildList->Task[ s_BuildList->NTasks ];
    
    // Build dma_task info
    pTask->DMAAddr          = (u32)DMAAddr;
    pTask->VRAMBytesLoaded  = 0;
    pTask->DListBytesUsed   = 0;
    pTask->CPUTime          = x_GetTime();
    x_strcpy((char*)pTask->Name,pTaskName);
    //
    // Create the initial mark point for this display list
    //
    dmatag* pTag = DLStruct(dmatag);
    pTag->SetCont(0);
    pTag->PAD[0] = SCE_VIF1_SET_FLUSHA( 0 );
    pTag->PAD[1] = SCE_VIF1_SET_MARK( s_BuildList->NTasks, 0 );


}

//=========================================================================
s32 NExecs=0;
s64 NTicks=0;

void dmaman_ExecuteTasks( void )
{
    volatile task_list* pList;
    s64 StartTime;
    //s32 i;
    //s64 AndyStart;
    //s64 AndyEnd;
    static s32 NCalls=0;
    NCalls++;

    //printf("Executing dma_tasks: %1d\n",s_BuildList->NTasks);
    
    // Get Current time
    StartTime = x_GetTime();

    // Confirm previous list is finished
    ASSERT( s_RunList->IsFinished );

    // Finish building list
    s_BuildList->CurrentTaskID = 0;
    s_BuildList->IsFinished    = FALSE;    
    
    // Flip Run and Build
    pList       = s_RunList;
    s_RunList   = s_BuildList;
    s_BuildList = pList;
    
    // Kick off first dma_tasks
    if( s_RunList->NTasks > 0 )
    {
		SignalSema(s_StartDmaSemaphore);
    }
    else
    {
        s_RunList->StartTime  = StartTime;
        s_RunList->EndTime    = StartTime;
        s_RunList->IsFinished = TRUE;
    }
    
    // Prep next build chain
    s_BuildList->PrevNTasks = s_BuildList->NTasks;
    s_BuildList->NTasks = 0;

}

//=========================================================================
xbool s_DamageLists=FALSE;

void dmaman_EndTask( s32 VRAMBytesLoaded, s32 DListBytesUsed )
{
    dmatag* pTag;
    
    volatile dma_task* pTask;

    pTask = &s_BuildList->Task[s_BuildList->NTasks ];
    pTag = DLStruct(dmatag);
    
    // Make sure size is 16 byte multiple
    ASSERT( (((u32)pTag) & 0xF) == 0 );
    pTag->SetCont(0);
    pTag->PAD[0]= SCE_VIF1_SET_MARK( s_BuildList->NTasks|0x80, 0 );
    pTag->PAD[1]= SCE_VIF1_SET_FLUSHA( 0 );

    // Sony technote dated 3/22/02 - Erroneous VIF0/VIF1 INTC interrupts
    // states that multiple interrupts can occur due to the pipelined nature
    // of the VIF. To fix the problem, a NOP must be inserted after any sync
    // related instructions and another just before the flush with the i-bit
    // set.
    pTag = DLStruct(dmatag);
    
    // Make sure size is 16 byte multiple
    ASSERT( (((u32)pTag) & 0xF) == 0 );

    pTag->SetCont(0);
    pTag->PAD[0] = 0;
    pTag->PAD[1] = 0;

    pTag = DLStruct(dmatag);
    
    // Make sure size is 16 byte multiple
    ASSERT( (((u32)pTag) & 0xF) == 0 );

    pTag->SetCont(0);
    pTag->PAD[0] = 0;
    pTag->PAD[1] = 0;
    pTag->ID |= 0x80;           // Set IRQ bit

    pTag = DLStruct(dmatag);
    
    // Make sure size is 16 byte multiple
    ASSERT( (((u32)pTag) & 0xF) == 0 );

    pTag->SetEnd(0);
    pTag->PAD[0] = 0;
    pTag->PAD[1] = 0;

	if (s_DamageLists && DListBytesUsed)
	{
		u8* pList;
		pList = (u8*)pTask->DMAAddr;

		for (s32 i=0;i<200;i++)
		{
			pList[((u32)x_rand())%DListBytesUsed] = x_rand();
		}
		s_DamageLists = 0;
	}
    pTask->CPUTime = x_GetTime() - pTask->CPUTime;
    pTask->VRAMBytesLoaded = VRAMBytesLoaded;
    pTask->DListBytesUsed  = DListBytesUsed;
    s_BuildList->NTasks++;
}

//=========================================================================

xbool   dmaman_AreTasksFinished ( void )
{
    return s_RunList->IsFinished;
}

//=========================================================================


void dmaman_PrintTaskStats ( void )
{

    s32 i;

    f32 CPUTotal=0;
    f32 GSTotal=0;
    s32 VRAMTotal=0;
    s32 DListTotal=0;

    x_DebugMsg("---  -----  ------  --------  -------\n");
    x_DebugMsg("ID]  -CPU-  -GSVU-  --VRAM--  -DLIST-\n");
    for( i=0; i<s_BuildList->PrevNTasks; i++ )
    {
        f32 CPU = x_TicksToMs(s_BuildList->Task[i].CPUTime);
        f32 GS = x_TicksToMs(s_BuildList->Task[i].EndTime - s_BuildList->Task[i].StartTime);
        x_DebugMsg("%2d]  %5.2f  %6.2f  %8d  %7d  <%s>\n", 
            i, 
            CPU, 
            GS, 
            s_BuildList->Task[i].VRAMBytesLoaded,
            s_BuildList->Task[i].DListBytesUsed,
            s_BuildList->Task[i].Name);
        CPUTotal += CPU;
        GSTotal += GS;
        VRAMTotal += s_BuildList->Task[i].VRAMBytesLoaded;
        DListTotal += s_BuildList->Task[i].DListBytesUsed;
    }
    
    x_DebugMsg("---  -----  ------  --------  -------\n");
    x_DebugMsg("TTL  %5.2f  %6.2f  %8d  %7d\n", 
        CPUTotal, 
        GSTotal, 
        VRAMTotal,
        DListTotal);
}

//=========================================================================

void    dmaman_HandlersOn       ( void )
{
    s_DMAHandlersOn = TRUE;
}

//=========================================================================

void    dmaman_HandlersOff      ( void )
{
    s_DMAHandlersOn = FALSE;
}

//=========================================================================

volatile s32 TIMEOUTDELAY = 0;

extern fsAABuff*        s_pAAbuff;
extern s32              s_FrameCount;

extern void ps2_ResetHardware(void);

void	dmaman_WaitForTasks		( void )
{
    xtimer Time;
    Time.Start();

    while(1)
    {
        // If finished, jump out of loop
        if( s_RunList->IsFinished ) 
        {
            //TIMEOUTDELAY = MAX(0,TIMEOUTDELAY-1);
            TIMEOUTDELAY = 1000;
            break;
        }

        // 
        TIMEOUTDELAY--;
        if( TIMEOUTDELAY<=0 )
        {
            // Since not finished sleep for a while
            x_DelayThread( 2 );

            // Check if it's been too long
            f32 MS = Time.ReadMs();

            if ( ( MS > 250 ) && (!s_RunList->IsFinished) )
            {
			    scePrintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			    scePrintf("DMA Manager has timed out!\n");
			    scePrintf("Graphics task <%s> (%1d/%1d) did not return!\n",
                    s_RunList->Task[s_RunList->CurrentTaskID].Name,
                    1+s_RunList->CurrentTaskID,
                    s_RunList->NTasks);
                scePrintf("Timeout %f %f\n",MS,Time.ReadMs());
                scePrintf("Attempting to continue....\n");

                x_printf( "DMA TASK TIMEOUT <%s> %d/%d\n", 
                    s_RunList->Task[s_RunList->CurrentTaskID].Name,
                    1+s_RunList->CurrentTaskID,
                    s_RunList->NTasks );

                x_DebugLog( "DMA TASK TIMEOUT <%s> %d/%d\n", 
                    s_RunList->Task[s_RunList->CurrentTaskID].Name,
                    1+s_RunList->CurrentTaskID,
                    s_RunList->NTasks );
                // CJG - Force page flip to display what has been rendered so far
                //PutDispBuffer       ( s_pAAbuff, !(s_FrameCount&0x01), 1);

#ifdef TARGET_PS2_DEV
				BREAK;
#endif
				//*** NOTE *** this reset code is courtesy of a 'toptip' in the Sony newsgroups.
				// This *should* help us recover from a dma lockup. If that occurs, we reset it
				// all but we must go through the dma handler thread to restart a dma list.
				ps2_ResetHardware();
				x_DelayThread(10);			// Wait 10ms to allow gs time to reset
				PollSema(s_DmaSemaphore);
				SignalSema(s_DmaSemaphore);
				Time.Reset();
				TIMEOUTDELAY = 1000;
            }
        }
    }
}

//=========================================================================

f32     dmaman_GetRunTime       ( void )
{
    if( s_RunList->IsFinished )
        return x_TicksToMs(s_RunList->EndTime - s_RunList->StartTime);

    return 0;
}

//=========================================================================




