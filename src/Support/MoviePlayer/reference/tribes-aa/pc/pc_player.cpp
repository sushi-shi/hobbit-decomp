#include "movieplayer/movieplayer.hpp"
//-----------------------------------------------------------------------------
xbool   movie::Open        (const char *pFilename,s32 width,s32 height,xbool LoadIntoMemory)
{
    (void)pFilename;
    (void)width;
    (void)height;
    (void)LoadIntoMemory;
    m_Finished = TRUE;
    return FALSE;
}

//-----------------------------------------------------------------------------
void    movie::Close       (void)
{
    m_Finished = TRUE;
}

//-----------------------------------------------------------------------------
xbitmap *movie::GetFrame   (void)
{
    m_Finished = TRUE;
    return NULL;
}

//-----------------------------------------------------------------------------
void    movie::SetPosition (s32 Index)
{
    m_Finished = TRUE;
    (void)Index;
}

//-----------------------------------------------------------------------------
xbool   movie::Advance     (void)
{
    m_Finished = TRUE;
    return FALSE;
}

//-----------------------------------------------------------------------------
xbool   movie::Decode      (void)
{
    m_Finished = TRUE;
    return FALSE;
}

