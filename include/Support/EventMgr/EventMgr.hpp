// Imported genuine area51 pristine431f72b9 engine reference: Support/EventMgr/EventMgr.hpp
// Provisional Hobbit API/layout adaptation; no fabricated PC labels. See IMPORT-NOTES.md.
//==============================================================================
//
//  EventMgr.hpp
//
//==============================================================================

#ifndef EVENT_MGR_HPP
#define EVENT_MGR_HPP

#include <Support/Animation/AnimData.hpp>
#include <Support/Animation/CharAnimPlayer.hpp>
#include <Support/Animation/AnimPlayer.hpp>
#include <Support/Loco/Loco.hpp>
#include <Support/Obj_mgr/obj_mgr.hpp>
#include <Support/AudioMgr/AudioMgr.hpp>
#include <Support/Animation/BasePlayer.hpp>

//==============================================================================
//  INCLUDES
//==============================================================================

//==============================================================================
//  TYPES
//==============================================================================
class event_mgr {
public:
    event_mgr(void);
    ~event_mgr(void);

    void HandleSuperEvents(char_anim_player& CharAnimPlayer, object* pObj);
    void HandleSuperEvents(loco_char_anim_player& CharAnimPlayer, object* pObj);
    void HandleSuperEvents(simple_anim_player& SimpleAnimPlayer, object* pObj);
    void HandleSuperEvents(
        loco_char_anim_player& CharAnimPlayer,
        loco_anim_controller& LocoAnimController,
        object* pObj
    );
    f32 ClosestPointToAABox(const vector3& Point, const bbox& Box, vector3& ClosestPoint);
#if !defined(X_RETAIL)
    xbool m_bLogAudio;
    xbool m_bLogParticle;
#endif

protected:
    void HandleAnimEvents(
        const anim_event& AnimEvent,
        object* pObj,
        s32 EventIndex,
        base_player& BasePlayer
    );

    void HandleAudioEvent(const event& Event, object* pParentObj, xbool UsePosition);
    void HandleParticleEvent(const event& Event, object* pParentObj);
    void HandleHotPointEvent(const event& Event, object* pParentObj);
    void HandleGenericEvent(const event& Event, object* pParentObj);
    void HandleIntensityEvent(const event& Event, object* pParentObj);
    void HandleDebrisEvent(const event& Event, object* pParentObj);
    void HandlePainEvent(event& Event, object* pParentObj);
    void HandleSetMeshEvent(const event& Event, object* pParentObj);
    void HandleSwapMeshEvent(const event& event, object* pParentObj);
    void HandleFadeGeometryEvent(const event& Event, object* pParentObj);
    void HandleSetVirtualTextureEvent(const event& event, object* pParentObj);
    void HandleCameraFOVEvent(const event& Event, object* pParentObj);

    matrix4 m_Tranform;

protected:
};

extern event_mgr g_EventMgr;

//==============================================================================
#endif // EVENT_MGR_HPP
//==============================================================================
