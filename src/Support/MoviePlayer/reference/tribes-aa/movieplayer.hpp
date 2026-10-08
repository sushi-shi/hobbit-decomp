#ifndef __MOVIEPLAYER_HPP
#define __MOVIEPLAYER_HPP

#include "x_files.hpp"
#include "x_bitmap.hpp"
#include "x_threads.hpp"

struct mpeg_av_stream
{
    xmesgq*       pqReady;
    xmesgq*       pqAvail;
    s32           Index;
    s32           Remain;
    u8            *pBuffer;
};

class movie
{
public:
            movie       (void);
            ~movie      (void);
    xbool   Open        (const char *pFilename,s32 width,s32 height,xbool LoadIntoMemory=FALSE);
    void    Close       (void);
    xbitmap *GetFrame    (void);
    void    SetPosition (s32 Index);
    void    UpdateStream (void);

    // 
    // Functions that perform all the grunt work
    //
    xbool   Advance     (void);     // Advance by 1 frame
    xbool   Decode      (void);     // Decode current frame to internal buffer
    //
    // Simple access functions
    //
    s32     GetWidth    (void) {return m_Width;}
    s32     GetHeight   (void) {return m_Height;}
    s32     GetDepth    (void) {return m_BitDepth;}
    s32     GetPosition (void) {return m_Position;}
    xbool   End         (void) {return m_Finished;}
    s32     GetBitmapCount(void) { return m_nBitmaps;}

private:

    s32             m_Width;                // Width in pixels
    s32             m_Height;               // Height in pixels
    s32             m_BitDepth;             // Depth in bits
    s32             m_Position;             // Position in frame #
    xbool           m_Finished;             // Flag set when there are no more frames
    xbool           m_InMemory;             // Set if the movie is contained entirely within memory
    s32             m_FrameCount;           // Number of frames within movie
    s32             m_CurrentFrame;         // Current frame #
    s32             m_Length;               // Length of the input mpeg stream
    xbitmap         *m_pBitmaps;            // Bitmaps containing decompressed data
    s32             m_nBitmaps;
    void            *m_pBuffer;             // Ptr to mpeg decode buffer
    void            *m_pWorkspace;          // Workspace for mpeg decode hardware
    byte            *m_pResidentStream;     // Ptr to resident stream block for a movie played entirely in RAM

public:
    void            *m_pPrivate;            // implementation dependant private buffer
    xbool           m_AudioFirst[2];
    mpeg_av_stream  m_VideoStream;
    mpeg_av_stream  m_AudioStream[2];
    xmesgq*         m_pqStreamBuffers;       // Message queue holding the streaming buffers from cd
    void            *m_pReadBuffer;         // Ptr to block of memory to be used for read requests
};
//
// Stand alone movie player routine. Will return when movie
// is complete or start is pressed
//
class ui_manager;

void movie_Play     (const char *pFilename,s32 width,s32 height,ui_manager *pUIManager=NULL);
// Starts a movie playing asynchronously. Movie MUST be small enough to fit in memory
// in it's entirety.
void movie_PlayAsync(const char *pFilename,s32 width,s32 height);
// This will wait until the movie playing is complete. It will force the movie to abort if ForceQuit is
// true.
void movie_WaitAsync(xbool ForceQuit);

#endif