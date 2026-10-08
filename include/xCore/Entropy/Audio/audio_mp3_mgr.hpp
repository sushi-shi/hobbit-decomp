// Source import: Area51 original revision 431f72b9; sibling engine version.
// Provisional Hobbit correspondence; no PC address or layout claim.
// Provenance: docs/imports/entropy-audio-io.json.

struct audio_stream;

class audio_mp3_mgr
{

public:

                    audio_mp3_mgr       ( void );
                   ~audio_mp3_mgr       ( void );
void                Init                ( void );
void                Kill                ( void );
void                Open                ( audio_stream* pStream );
void                Close               ( audio_stream* pStream );
void                Decode              ( audio_stream* pStream,
                                          s16*          pBufferL,
                                          s16*          pBufferR,
                                          s32           nSamples );
};

extern audio_mp3_mgr g_AudioMP3Mgr;

