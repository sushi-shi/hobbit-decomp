//==============================================================================
//
//  DebugUtils.hpp
//
//  A set of simple to use macros, used to time pieces of code
//
//==============================================================================


// To use these timer macros, define "DEBUG_TURN_STATS_ON", and then 
// include this file in your .cpp file,



//=========================================================================
// DEBUG TIMER MACROS
//=========================================================================

#ifdef DEBUG_TURN_STATS_ON

// Timer stat structure
struct stat
{
    const char* Name ;
    s32         Indent ;
    f32         Time ;
} ;

// Timer stat variables
static f32  s_StatsTotalTime=0 ;
static s32  s_StatsXPos=1;
static s32  s_StatsYPos=0;
static stat s_StatsList[50] ;
static s32  s_StatsLevel=0;
static s32  s_StatsCount=0 ;

static void BeginStats(s32 x, s32 y)
{
    s_StatsTotalTime=0 ;
    s_StatsXPos=x ;
    s_StatsYPos=y ;
    s_StatsLevel=0 ;
    s_StatsCount=0 ;
}

static void BeginTimer()
{
    s_StatsLevel++ ;
}

static void EndTimer(const char* Name, f32 Time)
{
    s_StatsLevel-- ;
    if (s_StatsLevel == 0)
        s_StatsTotalTime += Time ;
    s_StatsList[s_StatsCount].Name   = Name ;
    s_StatsList[s_StatsCount].Indent = s_StatsLevel ;           
    s_StatsList[s_StatsCount++].Time = Time ;
}

static void EndStats()
{
    // print backwards for correct indenting
    for (s32 i = s_StatsCount-1 ; i >=0 ; i--)
        x_printfxy(s_StatsXPos + s_StatsList[i].Indent,
                   s_StatsYPos++,
                   "%s:%04.3fms",
                    s_StatsList[i].Name, s_StatsList[i].Time) ;

    x_printfxy(s_StatsXPos, s_StatsYPos++, "TotalTime:%04.3f", s_StatsTotalTime) ;
}


// Simple macros to time player stats and print them
#define DEBUG_BEGIN_STATS(__x__, __y__)     BeginStats(__x__, __y__) ;
#define DEBUG_BEGIN_TIMER(__name__)         xtimer __name__ ; __name__.Start() ; BeginTimer() ;
#define DEBUG_END_TIMER(__name__)           __name__.Stop() ; EndTimer(#__name__, __name__.ReadMs()) ;
#define DEBUG_END_STATS                     EndStats() ;
#define DEBUG_DEFINE_TIMER(__name__)        xtimer __name__ ;
#define DEBUG_START_TIMER(__name__)         __name__.Start() ;
#define DEBUG_STOP_TIMER(__name__)          __name__.Stop() ;

#else

// Dummy macros    
#define DEBUG_BEGIN_STATS(__x__, __y__)
#define DEBUG_BEGIN_TIMER(__name__)                            
#define DEBUG_END_TIMER(__name__)
#define DEBUG_END_STATS
#define DEBUG_DEFINE_TIMER(__name__)
#define DEBUG_START_TIMER(__name__)
#define DEBUG_STOP_TIMER(__name__)

#endif

//=========================================================================
