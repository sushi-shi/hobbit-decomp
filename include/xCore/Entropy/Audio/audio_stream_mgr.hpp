// Source import: Area51 original revision 431f72b9; sibling engine version.
// Provisional Hobbit correspondence; no PC address or layout claim.
// Provenance: docs/imports/entropy-audio-io.json.
#ifndef AUDIO_STREAM_MGR_HPP
#define AUDIO_STREAM_MGR_HPP

#include <xCore/Entropy/IOManager/io_mgr.hpp>
#include <xCore/Entropy/Audio/audio_private.hpp>
#include <xCore/Entropy/Audio/audio_channel_mgr.hpp>
#include <xCore/Entropy/IOManager/io_request.hpp>

// Descriptive reconstructed name; complete real PC copy body, original spelling unknown.
extern void pc_CopyCompletedStreamBuffer(io_request* pRequest, audio_stream* pStream, s32 WriteBufferIndex);

extern void audio_stream_read_callback( io_request* pRequest, audio_stream* pStream, s32 ReadBufferIndex );

class audio_stream_mgr
{

//------------------------------------------------------------------------------
// Public functions.

public:

                            audio_stream_mgr        ( void );
                           ~audio_stream_mgr        ( void );
                                                    
            void            Init                    ( void );
            void            Kill                    ( void );

            void            Update                  ( void );
            void            SetRequest              ( audio_stream* pStream, io_request::callback_fn* pCallback );

            audio_stream*   AcquireStream           ( u32           WaveformOffset,
                                                      u32           WaveformLength,
                                                      channel*      pLeft,
                                                      channel*      pRight );
            void            ReleaseStream           ( audio_stream* pStream );
            xbool           WarmStream              ( audio_stream* pStream, io_request::callback_fn* pCallback = NULL );
            xbool           ReadStream              ( audio_stream* pStream, io_request::callback_fn* pCallback = NULL );
            xbool           ReserveStreams          ( s32           nStreams );
            xbool           UnReserveStreams        ( s32           nStreams );

//------------------------------------------------------------------------------

#if defined(TARGET_PC) && !defined(HOBBIT_AUDIO_LATER_STREAMS)
            // PC constructor independently proves seven 280-byte elements.
            audio_stream    m_AudioStreams[ 7 ];
#else
            audio_stream    m_AudioStreams[ MAX_AUDIO_STREAMS ];
#endif
            u32             m_ARAM;
            u32             m_MainRam;
            u32             m_ReadBuffers[2];
            u32             m_ActiveReadBuffer;
#if !defined(TARGET_PC) || defined(HOBBIT_AUDIO_LATER_STREAMS)
            s32             m_nReservedStreams;
#endif
};

//------------------------------------------------------------------------------

extern audio_stream_mgr g_AudioStreamMgr;

#endif // AUDIO_STREAM_MGR_HPP
