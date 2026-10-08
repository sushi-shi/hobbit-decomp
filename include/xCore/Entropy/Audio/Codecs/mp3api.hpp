// Source import: Area51 original revision 431f72b9; sibling engine version.
// Provisional Hobbit correspondence; no PC address or layout claim.
// Provenance: docs/imports/entropy-audio-io.json.
#ifndef MP3API_HPP
#define MP3API_HPP

#include <xCore/Entropy/Audio/Codecs/MP3DEC.H>

ASIRESULT AILEXPORT ASI_startup     (void);
ASIRESULT AILEXPORT ASI_shutdown    (void);

HASISTREAM   AILEXPORT ASI_stream_open (U32           user,  //)
                                        AILASIFETCHCB fetch_CB,  
                                        U32           total_size);

S32 AILEXPORT ASI_stream_process (HASISTREAM  stream, //)
                                  void        *buffer,
                                  S32         request_size);


ASIRESULT AILEXPORT ASI_stream_close(HASISTREAM stream);

#endif // MP3API_HPP