// Source import: Area51 original revision 431f72b9; sibling engine version.
// Provisional Hobbit correspondence; no PC address or layout claim.
// Provenance: docs/imports/entropy-audio-io.json.
#include <rva.h>
#include <xCore/x_files/x_files.hpp>
#include <xCore/Entropy/Audio/audio_io_request.hpp>

//------------------------------------------------------------------------------

RVA(0x00282380, 0x19)
audio_io_request::audio_io_request( void )
{
    m_pPrev       = NULL;
    m_pNext       = NULL;
    m_Destination = NULL;
    m_Source      = NULL;
    m_pCallback   = NULL;
    m_Status      = NOT_QUEUED;
    m_Length      = 0;
}

//------------------------------------------------------------------------------

audio_io_request::~audio_io_request( void )
{
}

//------------------------------------------------------------------------------

RVA(0x002823b0, 0x34)
void audio_io_request::SetRequest( transfer_type    Type,
                                   void*            Destination, 
                                   void*            Source, 
                                   s32              Length, 
                                   request_priority Priority,
                                   callback_fn*     pCallback )
{
    // Must be unqueued!
    ASSERT( m_Status != PENDING );
    ASSERT( m_Status != IN_PROGRESS );

    // Set up the request.
    m_Type        = Type; 
    m_Destination = Destination;
    m_Source      = Source;
    m_Length      = Length;    
    m_Priority    = Priority;
    m_Status      = NOT_QUEUED;
    m_pCallback   = pCallback;
}
