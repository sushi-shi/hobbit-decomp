// Imported genuine tribes-aa pristine4aab7137 engine reference: aa/support/EventMgr/EventMgr.hpp
// Provisional Hobbit API/layout adaptation; no fabricated PC labels. See IMPORT-NOTES.md.
//==============================================================================
//
//  EventMgr.hpp
//
//==============================================================================

#ifndef EVENTMGR_HPP
#define EVENTMGR_HPP

//==============================================================================
//  INCLUDES
//==============================================================================

#include <xCore/x_files/x_files.hpp>
#include <Support/ObjectMgr/Object.hpp>

//==============================================================================
//  TYPES
//==============================================================================

class event {
    //------------------------------------------------------------------------------
    //  Public Types
    //------------------------------------------------------------------------------

public:
    enum code {
        // "Outer" events.
        RECEIVE_PACKET,
        RECEIVE_INPUT,
        UPDATE_TIME,
        RENDER,

        // "Inner" events.
        FLAG_TOUCHED,
        FLAG_TIME_OUT,
        PLAYER_DIED,

        // List terminator.
        END_OF_LIST,
    };

    //------------------------------------------------------------------------------
    //  Public Functions
    //------------------------------------------------------------------------------

public:
    void EncodeFlagTouched(object::id FlagID, object::id PlayerID);
    void DecodeFlagTouched(object::id& FlagID, object::id& PlayerID);

    void EncodeFlagTimeOut(object::id FlagID);
    void DecodeFlagTimeOut(object::id& FlagID);

    void EncodePlayerDied(object::id PlayerID);
    void DecodePlayerDied(object::id& PlayerID);

    //------------------------------------------------------------------------------
    //  Private Storage
    //------------------------------------------------------------------------------

protected:
    code m_Code;
    object::id m_ID[2];

    //------------------------------------------------------------------------------
    //  Internal Stuff
    //------------------------------------------------------------------------------

    friend class event_mgr;
    friend class game_mgr;
};

//==============================================================================

class event_mgr {

    //------------------------------------------------------------------------------
    //  Public Functions
    //------------------------------------------------------------------------------

public:
    event_mgr(void);
    ~event_mgr(void);

    void FlagTouched(object::id FlagID, object::id PlayerID);
    void FlagTimeOut(object::id FlagID);
    void PlayerDied(object::id PlayerID);

    //------------------------------------------------------------------------------
    //  Private Storage
    //------------------------------------------------------------------------------

protected:
    // The Initialized flag is static to ensure that only one event manager is
    // ever instantiated.

    static xbool m_Initialized;
};

//==============================================================================
//  GLOBAL VARIABLES
//==============================================================================

extern event_mgr EventMgr;

//==============================================================================
#endif // EVENTMGR_HPP
//==============================================================================
