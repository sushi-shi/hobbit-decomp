//=========================================================================
//
// ps2_dmaman.hpp
//
//=========================================================================

#ifndef PS2_DMAMAN_HPP
#define PS2_DMAMAN_HPP

#include <xCore/x_files/x_types.hpp>
#include <xCore/x_files/x_time.hpp>

//=========================================================================

void    dmaman_Kill             ( void );
void    dmaman_Init             ( void );

void    dmaman_NewTask          ( void* DMAAddr, 
                                  char* pTaskName
                                );

void    dmaman_EndTask          ( s32 VRAMBytesLoaded, 
                                  s32 DListBytesUsed );

void    dmaman_ExecuteTasks     ( void );
xbool   dmaman_AreTasksFinished ( void );
void    dmaman_HandlersOn       ( void );
void    dmaman_HandlersOff      ( void );
void	dmaman_WaitForTasks		( void );
f32     dmaman_GetRunTime       ( void );
void    dmaman_PrintTaskStats   ( void );


struct dmaman_task_stats
{
    u32     DMAAddr;
    xtick   StartTime;
    xtick   EndTime;
    xtick   CPUTime;
    char    Name[32];
    s32     VRAMBytesLoaded;
    s32     DListBytesUsed;
};

s32     dmaman_GetNTasks        ( void );
void    dmaman_GetTaskStats     ( s32 ID, dmaman_task_stats& Stats );

//=========================================================================
#endif
