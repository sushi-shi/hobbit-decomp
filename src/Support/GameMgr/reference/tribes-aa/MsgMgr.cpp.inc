//==============================================================================
//
//  MsgMgr.cpp
//
//==============================================================================

#define MSG(i)      (const char*)xstring( StringMgr( "Messages", i ) )

//==============================================================================
//  INCLUDES
//==============================================================================

#include "MsgMgr.hpp"
#include "GameMgr.hpp"
#include <Support/StringMgr/StringMgr.hpp>
#include "AudioMgr\Audio.hpp"
#include "LabelSets/Tribes2Types.hpp"

#include "..\HUD\HUD_Manager.hpp"

#include "AADS\fe_globals.hpp"
#include "AADS\Globals.hpp"
#include "AADS\GameServer.hpp"

#include "AADS\Data\UI\Messages.h"
#include "AADS\Data\UI\UI_Strings.h"

#include "LogMgr.hpp"

//==============================================================================
//  TYPES
//==============================================================================

//==============================================================================
//  STORAGE
//==============================================================================

msg_mgr  MsgMgr;

static random Random;

//------------------------------------------------------------------------------

msg_info    MsgTable[] = 
{
//    message string id                              arg1              arg2       audience       where             color                          audio

    { MSG_DEATH_TEAMKILLED                         , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_SUICIDE                            , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_OWN_MINE                           , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_LANDED_TOO_HARD                    , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_KILLED_SELF                        , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_NEW_ARMOR                          , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_SPARE_PARTS                        , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_RESPAWN                            , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_CAUGHT_BLAST                       , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_GOT_BOMBED                         , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_TOO_FAR                            , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_FORCE_FIELD                        , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_A_MINE                             , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_MISSILE                            , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_GOT_SHOT_DOWN                      , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_AA_TURRET                          , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_REMOTE_TURRET                      , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_SENTRY_TURRET                      , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_CAUGHT_MORTAR                      , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_MORTAR_TURRET                      , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_PLASMA_TURRET                      , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_RECYCLED                           , ARG_VICTIM    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_TOOK_OUT                           , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_ELIMINATED                         , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_DEFEATED                           , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_SMOKED                             , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_DEMOLISHED                         , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_NAILED                             , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_FINISHED_OFF                       , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_BLASTED                            , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_BOMBED                             , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_GUNNED_DOWN                        , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_OTHERS_MINE                        , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_SHOT_DOWN                          , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_ATE_PLASMA                         , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_FRIED                              , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_TURRET_STOPPED                     , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_DETONATED                          , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_PICKED_OFF                         , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_DEATH_MOWED_DOWN                         , ARG_VICTIM    , ARG_KILLER     , HEAR_ALL       , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_POSITIVE                                 , ARG_WARRIOR   , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,       45  },
    { MSG_POSITIVE_1                               , ARG_WARRIOR   , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,       62  },
    { MSG_NEGATIVE                                 , ARG_WARRIOR   , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,       51  },
    { MSG_NEGATIVE_1                               , ARG_WARRIOR   , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,       54  },
    { MSG_DEATH_TAUNT                              , ARG_WARRIOR   , ARG_NONE       , HEAR_LOCAL     , WHERE_NONE        , COLOR_NONE           ,       56  },
    { MSG_DEATH_TAUNT_1                            , ARG_WARRIOR   , ARG_NONE       , HEAR_LOCAL     , WHERE_NONE        , COLOR_NONE           ,       57  },
    { MSG_DEATH_TAUNT_2                            , ARG_WARRIOR   , ARG_NONE       , HEAR_LOCAL     , WHERE_NONE        , COLOR_NONE           ,       58  },
    { MSG_DEATH_TAUNT_3                            , ARG_WARRIOR   , ARG_NONE       , HEAR_LOCAL     , WHERE_NONE        , COLOR_NONE           ,       59  },
    { MSG_AUTO_RECOVER_FLAG                        , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,        3  },
    { MSG_AUTO_GET_ENEMY_FLAG                      , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,        5  },
    { MSG_AUTO_DESTROY_TURRETS                     , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       10  },
    { MSG_AUTO_ENEMY_IN_BASE                       , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       14  },
    { MSG_AUTO_COVER_FLAG_CARRIER                  , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       19  },
    { MSG_AUTO_DEFEND_OUR_FLAG                     , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       21  },
    { MSG_AUTO_COVER_ME                            , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       23  },
    { MSG_AUTO_ENEMY_BASE_DISABLED                 , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       30  },
    { MSG_AUTO_GENERATOR_DESTROYED                 , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       32  },
    { MSG_AUTO_REMOTE_EQUIPMENT_DESTROYED          , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       33  },
    { MSG_AUTO_SENSORS_DESTROYED                   , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       34  },
    { MSG_AUTO_TURRETS_DESTROYED                   , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       35  },
    { MSG_AUTO_VEHICLE_STATION_DESTROYED           , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       36  },
    { MSG_AUTO_HAVE_FLAG                           , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       37  },
    { MSG_AUTO_RETRIEVE_FLAG                       , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       41  },
    { MSG_AUTO_FLAG_SECURE                         , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       42  },
    { MSG_AUTO_AWESOME                             , ARG_WARRIOR   , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,       45  },
    { MSG_AUTO_BYE                                 , ARG_WARRIOR   , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,       46  },
    { MSG_AUTO_GOOD_GAME                           , ARG_WARRIOR   , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,       48  },
    { MSG_AUTO_HI                                  , ARG_WARRIOR   , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,       49  },
    { MSG_AUTO_OOPS                                , ARG_WARRIOR   , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,       51  },
    { MSG_AUTO_YOU_ROCK                            , ARG_WARRIOR   , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,       53  },
    { MSG_AUTO_SHAZBOT                             , ARG_WARRIOR   , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,       54  },
    { MSG_AUTO_WOOHOO                              , ARG_WARRIOR   , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,       62  },
    { MSG_AUTO_GUNSHIP_READY                       , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       68  },
    { MSG_AUTO_WHERE_TO                            , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       74  },
    { MSG_AUTO_REPAIR_BASE                         , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       75  },
    { MSG_AUTO_REPAIR_GENERATOR                    , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       76  },
    { MSG_AUTO_REPAIR_ME                           , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       77  },
    { MSG_AUTO_REPAIR_SENSORS                      , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       78  },
    { MSG_AUTO_REPAIR_TURRETS                      , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       79  },
    { MSG_AUTO_REPAIR_VEHICLE_STATION              , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       80  },
    { MSG_AUTO_SELF_ATTACK                         , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       81  },
    { MSG_AUTO_SELF_ATTACK_BASE                    , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       82  },
    { MSG_AUTO_SELF_ATTACK_FLAG                    , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       83  },
    { MSG_AUTO_SELF_ATTACK_GENERATOR               , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       84  },
    { MSG_AUTO_SELF_ATTACK_SENSORS                 , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       85  },
    { MSG_AUTO_SELF_ATTACK_TURRETS                 , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       86  },
    { MSG_AUTO_SELF_ATTACK_VEHICLE_STATION         , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       87  },
    { MSG_AUTO_SELF_DEFEND_BASE                    , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       88  },
    { MSG_AUTO_SELF_DEFEND                         , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       89  },
    { MSG_AUTO_SELF_DEFEND_FLAG                    , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       90  },
    { MSG_AUTO_SELF_REPAIR_BASE                    , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       96  },
    { MSG_AUTO_SELF_REPAIR_EQUIPMENT               , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       97  },
    { MSG_AUTO_SELF_REPAIR_GENERATOR               , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       98  },
    { MSG_AUTO_SELF_REPAIR                         , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,       99  },
    { MSG_AUTO_SELF_REPAIR_SENSORS                 , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      100  },
    { MSG_AUTO_SELF_REPAIR_TURRETS                 , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      101  },
    { MSG_AUTO_SELF_REPAIR_VEHICLE_STATION         , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      102  },
    { MSG_AUTO_SELF_COVER_YOU                      , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      103  },
    { MSG_AUTO_SELF_SET_UP_DEFENSE                 , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      104  },
    { MSG_AUTO_SELF_DEPLOY_REMOTES                 , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      106  },
    { MSG_AUTO_SELF_DEPLOY_SENSORS                 , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      107  },
    { MSG_AUTO_SELF_DEPLOY_TURRETS                 , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      108  },
    { MSG_AUTO_TARGET_ACQUIRED                     , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      110  },
    { MSG_AUTO_TARGET_DESTROYED                    , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      112  },
    { MSG_AUTO_HELP                                , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      123  },
    { MSG_AUTO_MOVE                                , ARG_WARRIOR   , ARG_NONE       , HEAR_LOCAL     , WHERE_CHAT        , COLOR_NEUTRAL        ,      124  },
    { MSG_AUTO_SORRY                               , ARG_WARRIOR   , ARG_NONE       , HEAR_LOCAL     , WHERE_CHAT        , COLOR_NEUTRAL        ,      126  },
    { MSG_AUTO_WARN_BOMBER                         , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      130  },
    { MSG_AUTO_WARN_HOSTILES                       , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      131  },
    { MSG_AUTO_WARN_VEHICLES                       , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,      132  },
    { MSG_AUTO_WARN_FRIENDLY_FIRE                  , ARG_WARRIOR   , ARG_NONE       , HEAR_SINGLE    , WHERE_CHAT        , COLOR_NEUTRAL        ,      133  },
    { MSG_SCORE_DESTROY_ENEMY_GEN                  , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DESTROY_ENEMY_VSTATION             , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DESTROY_ENEMY_FSTATION             , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DESTROY_ENEMY_FTURRET              , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DESTROY_ENEMY_FSENSOR              , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DESTROY_ENEMY_RSTATION             , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DESTROY_ENEMY_RTURRET              , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DESTROY_ENEMY_RSENSOR              , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DESTROY_ENEMY_GRAVCYCLE            , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DESTROY_ENEMY_FIGHTER              , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DESTROY_ENEMY_BOMBER               , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DESTROY_ENEMY_TRANSPORT            , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DISABLE_TEAM_GEN                   , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SCORE_DISABLE_TEAM_VSTATION              , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SCORE_DISABLE_TEAM_FSTATION              , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SCORE_DISABLE_TEAM_FTURRET               , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SCORE_DISABLE_TEAM_FSENSOR               , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SCORE_DISABLE_TEAM_RSTATION              , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SCORE_DISABLE_TEAM_RTURRET               , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SCORE_DISABLE_TEAM_RSENSOR               , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SCORE_DEFEND_TEAM_GEN                    , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_DEFEND_TEAM_SWITCH                 , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_REPAIR_TEAM_GEN                    , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_REPAIR_TEAM_VSTATION               , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_REPAIR_TEAM_FSTATION               , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_REPAIR_TEAM_FTURRET                , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_REPAIR_TEAM_FSENSOR                , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_REPAIR_TEAM_RSTATION               , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_REPAIR_TEAM_RTURRET                , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_REPAIR_TEAM_RSENSOR                , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_SUICIDE                            , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SCORE_TEAMKILL                           , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SCORE_TAKE_OBJECTIVE                     , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_FLARE_MISSILE                      , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_CTF_CAP_FLAG                       , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_CTF_TAKE_FLAG                      , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_CTF_DEFEND_FLAG                    , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_CTF_RETURN_FLAG                    , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_CTF_DEFEND_CARRIER                 , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_CTF_KILL_CARRIER                   , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_SCORE_CNH_HOLD_OBJECTIVE                 , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_POSITIVE       ,        0  },
    { MSG_PICKUP_TAKEN                             , ARG_PICKUP    , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEUTRAL        ,        0  },
    { MSG_PICKUP_EXCHANGE_AVAILABLE                , ARG_PICKUP    , ARG_NONE       , HEAR_SINGLE    , WHERE_EXCHANGE    , COLOR_NEUTRAL        ,        0  },
    { MSG_VEHICLE_DESTROYED_TEAM                   , ARG_KILLER    , ARG_VEHICLE    , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEGATIVE_ARG   ,        0  },
    { MSG_VEHICLE_DESTROYED_ENEMY                  , ARG_KILLER    , ARG_VEHICLE    , HEAR_TEAM      , WHERE_CHAT        , COLOR_POSITIVE_ARG   ,        0  },
    { MSG_PLAYER_CONNECTED                         , ARG_INLINE    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,        0  },
    { MSG_PLAYER_JOINED_GAME                       , ARG_INLINE    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,        0  },
    { MSG_PLAYER_JOINED_TEAM                       , ARG_INLINE    , ARG_TEAM       , HEAR_TEAM      , WHERE_CHAT        , COLOR_URGENT         ,        0  },
    { MSG_PLAYER_DISCONNECTED                      , ARG_INLINE    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,        0  },
    { MSG_ADMIN_PLAYER_KICKED_BY_ADMIN             , ARG_INLINE    , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,   SFX_MISC_ADMIN_CHANGE_STATE  },
    { MSG_ADMIN_CHANGED_TEAM                       , ARG_WARRIOR   , ARG_TEAM       , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,   SFX_MISC_ADMIN_CHANGE_STATE  },
    { MSG_VOTE_KICK_WARNING                        , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_ABOVE       , COLOR_URGENT         ,   SFX_VOICE_ANN_VOTE_INITIATED  },
    { MSG_VOTE_KICK                                , ARG_WARRIOR   , ARG_NONE       , HEAR_SINGLE    , WHERE_CHAT        , COLOR_URGENT         ,   SFX_VOICE_ANN_VOTE_INITIATED  },
    { MSG_VOTE_CHANGE_MAP                          , ARG_NONE      , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,   SFX_VOICE_ANN_VOTE_INITIATED  },
    { MSG_VOTE_KICK_FAILS                          , ARG_WARRIOR   , ARG_NONE       , HEAR_SINGLE    , WHERE_CHAT        , COLOR_URGENT         ,   SFX_VOICE_ANN_VOTE_FAIL  },
    { MSG_VOTE_MAP_FAILS                           , ARG_NONE      , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,   SFX_VOICE_ANN_VOTE_FAIL  },
    { MSG_VOTE_KICK_PASSES                         , ARG_WARRIOR   , ARG_NONE       , HEAR_SINGLE    , WHERE_CHAT        , COLOR_URGENT         ,   SFX_VOICE_ANN_VOTE_PASSES  },
    { MSG_VOTE_MAP_PASSES                          , ARG_NONE      , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,   SFX_VOICE_ANN_VOTE_PASSES  },
    { MSG_OUT_OF_BOUNDS                            , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_ABOVE       , COLOR_URGENT         ,   SFX_MISC_OUT_OF_BOUNDS_WARNING  },
    { MSG_IN_BOUNDS                                , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_ABOVE       , COLOR_NEUTRAL        ,        0  },
    { MSG_PLAYER_SWITCHED_TEAM                     , ARG_WARRIOR   , ARG_TEAM       , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,        0  },
    { MSG_CTF_TEAM_SCORES                          , ARG_TEAM      , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_POSITIVE_ARG   ,        0  },
    { MSG_CTF_TEAM_CAP                             , ARG_NONE      , ARG_NONE       , HEAR_TEAM      , WHERE_ABOVE       , COLOR_POSITIVE       ,        0  },
    { MSG_CTF_ENEMY_CAP                            , ARG_NONE      , ARG_NONE       , HEAR_TEAM      , WHERE_ABOVE       , COLOR_NEGATIVE       ,        0  },
    { MSG_CTF_YOU_HAVE_FLAG                        , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_ABOVE       , COLOR_URGENT         ,        0  },
    { MSG_CTF_YOU_DROPPED_FLAG                     , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_ABOVE       , COLOR_NEGATIVE       ,        0  },
    { MSG_CTF_PLAYER_TOOK_FLAG                     , ARG_WARRIOR   , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEGATIVE       ,        0  },
    { MSG_CTF_FLAG_MINED                           , ARG_NONE      , ARG_NONE       , HEAR_TEAM      , WHERE_CHAT        , COLOR_NEUTRAL        ,   SFX_MISC_FLAG_MINED_FEMALE  },
    { MSG_CNH_TEAM_GOT_OBJECTIVE                   , ARG_TEAM      , ARG_OBJECTIVE  , HEAR_ALL       , WHERE_CHAT        , COLOR_POSITIVE_ARG   ,        0  },
    { MSG_HUNT_FLAGS_TO_NEXUS                      , ARG_WARRIOR   , ARG_N          , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,        0  },
    { MSG_HUNT_POINTS                              , ARG_WARRIOR   , ARG_N          , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,        0  },
    { MSG_HUNT_FLAGS_DROPPED                       , ARG_WARRIOR   , ARG_N          , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,        0  },
    { MSG_HUNT_FLAGS_LOST                          , ARG_WARRIOR   , ARG_N          , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,        0  },
    { MSG_HUNT_FLAGS_FUMBLED                       , ARG_WARRIOR   , ARG_N          , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,        0  },
    { MSG_HUNT_FLAGS_MISPLACED                     , ARG_WARRIOR   , ARG_N          , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,        0  },
    { MSG_HUNT_YOUR_FLAGS                          , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_ABOVE       , COLOR_NEUTRAL        ,        0  },
    { MSG_HUNT_YOU_DROPPED_FLAGS                   , ARG_N         , ARG_NONE       , HEAR_SINGLE    , WHERE_ABOVE       , COLOR_NEGATIVE       ,        0  },
    { MSG_HUNT_YOUR_SCORE                          , ARG_N         , ARG_M          , HEAR_SINGLE    , WHERE_ABOVE       , COLOR_NEUTRAL        ,        0  },
    { MSG_TIME_60                                  , ARG_NONE      , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,   SFX_VOICE_ANN_60SECONDS  },
    { MSG_TIME_30                                  , ARG_NONE      , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_NEUTRAL        ,   SFX_VOICE_ANN_30SECONDS  },
    { MSG_TIME_10                                  , ARG_NONE      , ARG_NONE       , HEAR_ALL       , WHERE_CHAT        , COLOR_URGENT         ,   SFX_VOICE_ANN_10SECONDS  },
    { MSG_TURRET_DEPLOYED                          , ARG_N         , ARG_M          , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEUTRAL        ,        0  },
    { MSG_STATION_DEPLOYED                         , ARG_N         , ARG_M          , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEUTRAL        ,        0  },
    { MSG_SENSOR_DEPLOYED                          , ARG_N         , ARG_M          , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEUTRAL        ,        0  },
    { MSG_N_OF_M_TURRETS_DEPLOYED                  , ARG_N         , ARG_M          , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEUTRAL        ,        0  },
    { MSG_N_OF_M_STATIONS_DEPLOYED                 , ARG_N         , ARG_M          , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEUTRAL        ,        0  },
    { MSG_N_OF_M_SENSORS_DEPLOYED                  , ARG_N         , ARG_M          , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEUTRAL        ,        0  },
    { MSG_BARREL_DEPLOYED                          , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEUTRAL        ,        0  },
    { MSG_TURRET_DEPLOY_TOO_CLOSE                  , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_TURRET_DEPLOY_TOO_MANY_NEAR              , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_TURRET_DEPLOY_LIMIT_REACHED              , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_TURRET_DEPLOY_NO_ROOM                    , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_TURRET_DEPLOY_BAD_SURFACE                , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_STATION_DEPLOY_BAD_SURFACE               , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_STATION_DEPLOY_TOO_STEEP                 , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_STATION_DEPLOY_LIMIT_REACHED             , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_STATION_DEPLOY_TOO_CLOSE                 , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_STATION_DEPLOY_NO_ROOM                   , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SENSOR_DEPLOY_BAD_SURFACE                , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SENSOR_DEPLOY_TOO_STEEP                  , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SENSOR_DEPLOY_LIMIT_REACHED              , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SENSOR_DEPLOY_TOO_CLOSE                  , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_SENSOR_DEPLOY_NO_ROOM                    , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_BARREL_DEPLOY_NO_TURRET                  , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_BARREL_DEPLOY_TURRET_DOWN                , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_BARREL_DEPLOY_WRONG_TEAM                 , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_DEPLOY_NEAR_REMOTE_STATION               , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,        0  },
    { MSG_ACCESS_DENIED                            , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,   SFX_POWER_INVENTORYSTATION_DENIED  },
    { MSG_STATION_DISABLED                         , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,   SFX_POWER_INVENTORYSTATION_DENIED  },
    { MSG_STATION_NOT_POWERED                      , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,   SFX_POWER_INVENTORYSTATION_DENIED  },
    { MSG_CANNOT_PILOT_ARMOR                       , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,   SFX_POWER_INVENTORYSTATION_DENIED  },
    { MSG_CANNOT_PILOT_PACK                        , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,   SFX_POWER_INVENTORYSTATION_DENIED  },
    { MSG_VEHICLE_LIMIT                            , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,   SFX_POWER_INVENTORYSTATION_DENIED  },
    { MSG_VEHICLE_BLOCKED                          , ARG_NONE      , ARG_NONE       , HEAR_SINGLE    , WHERE_BELOW       , COLOR_NEGATIVE       ,   SFX_POWER_INVENTORYSTATION_DENIED  },
};

//------------------------------------------------------------------------------

xcolor s_ColorTable[] = 
{   //       R    G    B
    xcolor( 191, 255, 255 ),    // COLOR_NEUTRAL      blue
    xcolor( 255, 255,  63 ),    // COLOR_URGENT       yellow
    xcolor( 255, 191, 191 ),    // COLOR_NEGATIVE     red  
    xcolor( 127, 255, 127 ),    // COLOR_POSITIVE     green
    xcolor( 255, 210, 210 ),    // COLOR_ENEMY        very red  
    xcolor( 191, 255, 191 ),    // COLOR_ALLY         very green
};                                 
                                   
//==============================================================================
//  FUNCTIONS
//==============================================================================

msg_mgr::msg_mgr( void )
{
    Reset();
}

//==============================================================================

msg_mgr::~msg_mgr( void )
{
}

//==============================================================================

void msg_mgr::Reset( void )
{
    s32 i;

    for( i = 0; i < 16; i++ )
    {
        m_InlineName[i][0] = '\0';
    }

    m_QCount     = 0;
    m_QRead      = 0;
    m_QWrite     = 0;
    m_Playing    = 0;
    m_InlineLast = 0;
}

//==============================================================================

xbool msg_mgr::FetchArgValue( const msg& Msg, s16 Arg, s16& Value )
{
    if( Arg == Msg.Arg1 )
    {
        Value = Msg.Value1;
        return( TRUE );
    }
    else
    if( Arg == Msg.Arg2 )
    {
        Value = Msg.Value2;
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
}

//==============================================================================

void msg_mgr::PackArg( s16 Arg, s16 Value, bitstream& BitStream )
{
    BitStream.WriteRangedS32( Arg, ARG_KILLER, ARG_NONE );

    switch( Arg )
    {
    case ARG_KILLER:         // K
    case ARG_VICTIM:         // V
    case ARG_WARRIOR:        // W
        BitStream.WriteRangedS32( Value, 0, 31 );
        break;

    case ARG_INLINE:         // I
        BitStream.WriteRangedS32( Value, 0, 15 );
        BitStream.WriteWString( m_InlineName[Value] );
        break;

    case ARG_PICKUP:         // P
        BitStream.WriteRangedS32( Value, pickup::KIND_NONE, pickup::KIND_TOTAL );
        break;

    case ARG_N:              // N
    case ARG_M:              // M
        if( BitStream.WriteFlag( IN_RANGE( 0, Value, 63 ) ) )
            BitStream.WriteRangedS32( Value, 0, 63 );
        else
            BitStream.WriteS16( Value );
        break;

    case ARG_TEAM:           // T
        BitStream.WriteFlag( Value );
        break;

    case ARG_OBJECTIVE:      // O
    case ARG_VEHICLE:        // S (Ship)    
        ASSERT( FALSE ); // Currently not used.
        BitStream.WriteS16( Value );
        break;
    }
}

//==============================================================================

void msg_mgr::UnpackArg( s16& Arg, s16& Value, bitstream& BitStream )
{
    s32 arg;
    s32 value;

    BitStream.ReadRangedS32( arg, ARG_KILLER, ARG_NONE );

    switch( arg )
    {
    case ARG_KILLER:         // K
    case ARG_VICTIM:         // V
    case ARG_WARRIOR:        // W
        BitStream.ReadRangedS32( value, 0, 31 );
        break;

    case ARG_INLINE:         // I
        BitStream.ReadRangedS32( value, 0, 15 );
        BitStream.ReadWString( m_InlineName[value] );
        break;

    case ARG_PICKUP:         // P
        BitStream.ReadRangedS32( value, pickup::KIND_NONE, pickup::KIND_TOTAL );
        break;

    case ARG_N:              // N
    case ARG_M:              // M
        if( BitStream.ReadFlag() )
            BitStream.ReadRangedS32( value, 0, 63 );
        else
            BitStream.ReadS32( value, 16 );
        break;

    case ARG_TEAM:           // T
        value = BitStream.ReadFlag();
        break;

    case ARG_OBJECTIVE:      // O
    case ARG_VEHICLE:        // S (Ship)    
        ASSERT( FALSE ); // Currently not used.
        BitStream.ReadS32( value, 16 );
        break;
    }

    Arg   = arg;
    Value = value;
}

//==============================================================================

void msg_mgr::PackMsg( const msg& Msg, bitstream& BitStream )
{
    //x_DebugMsg( "MSGMGR: PackMsg (%d) %s\n", Msg.Index, MSG(Msg.Index) );

    BitStream.WriteS16( Msg.Index );
    BitStream.WriteS16( Msg.Target );

    if( BitStream.WriteFlag( Msg.Arg1 != ARG_NONE ) )
    {
        PackArg( Msg.Arg1, Msg.Value1, BitStream );
        if( BitStream.WriteFlag( Msg.Arg2 != ARG_NONE ) )
        {
            PackArg( Msg.Arg2, Msg.Value2, BitStream );
        }
    }
}

//==============================================================================

void msg_mgr::UnpackMsg( msg& Msg, bitstream& BitStream )
{
    BitStream.ReadS16( Msg.Index );
    BitStream.ReadS16( Msg.Target );

    if( BitStream.ReadFlag() )
    {
        UnpackArg( Msg.Arg1, Msg.Value1, BitStream );

        if( BitStream.ReadFlag() )
        {
            UnpackArg( Msg.Arg2, Msg.Value2, BitStream );
        }
        else
        {
            Msg.Arg2 = ARG_NONE;
        }
    }
    else
    {
        Msg.Arg1 = ARG_NONE;
        Msg.Arg2 = ARG_NONE;
    }

    //x_DebugMsg( "MSGMGR: UnpackMsg (%d) %s\n", Msg.Index, MSG(Msg.Index) );
}

//==============================================================================

void msg_mgr::AcceptMsg( bitstream& BitStream )
{
    msg Msg;
    UnpackMsg( Msg, BitStream );

    //x_DebugMsg( "MSGMGR: AcceptMsg (%d) %s\n", Msg.Index, MSG(Msg.Index) );

    ProcessMsg( Msg );
}

//==============================================================================

void msg_mgr::ProcessMsg( const msg& Msg )
{
    DisplayMsg( Msg );
/*
    //x_DebugMsg( "MSGMGR: ProcessMsg (%d) %s\n", Msg.Index, MSG(Msg.Index) );

    if( MsgTable[Msg.Index].Sound != 0 )
    {
        // This message has a sound.  
        // Sounds from players must (a) wait in line and (b) be "voice" correct. 
        // Sounds NOT from players are placed at the head of the line.

        if( MsgTable[Msg.Index].Sound & 0xFF000000 )
        {
            // This sound is NOT a player voice.  Force it at the head of the
            // queue... even if there isn't room.
            if( m_QCount == 4 )
            {
                // The queue is full.  We are going to yank the message at the
                // end of the list.  Display it's text, forget about its audio.
                // We will BACK the read index off and place the new message 
                // there.  This has the effect of getting rid of the last 
                // message in the list and placing the new message at the head
                // of the queue.
                m_QRead = (m_QRead+3) & 3;
                DisplayMsg( m_Queue[m_QRead] );
                m_Queue[m_QRead] = Msg;
            }
            else
            {
                // There is room in the queue.  Move the read index BACK one and
                // stick the new mesage there so it will go next.
                m_QRead = (m_QRead+3) & 3;
                m_Queue[m_QRead] = Msg;
                m_QCount++;
            }

            // We forced this new message to the head of the list because it had
            // "non-player voice" sound.  But, the other messages in the list may 
            // also have non-player sounds.  So, swap the new message back until
            // it isn't superceding a previous "high priority" message.

            for( s32 i = 1; i < m_QCount; i++ )
            {
                if( MsgTable[ m_Queue[ (m_QRead+i) & 3 ].Index ].Sound & 0xFF000000 )
                {
                    // Swap i and i-1.
                    msg M = m_Queue[ (m_QRead+i) & 3 ];
                            m_Queue[ (m_QRead+i) & 3 ] = m_Queue[ (m_QRead+i-1) & 3 ];
                                                         m_Queue[ (m_QRead+i-1) & 3 ] = M;
                }
                else
                    break;
            }            
        }
        else
        {
            // This sound is a player voice.
            // Is there space in the queue?
            if( m_QCount < 4 )
            {
                // Add message to queue.
                m_Queue[m_QWrite] = Msg;
                m_QWrite = (m_QWrite+1) & 3;
                m_QCount++;
            }
            else
            {
                // No space in queue.  Display message.  No audio.
                DisplayMsg( Msg );  
            }
        }
    }
    else
    {
        DisplayMsg( Msg );
    }
*/
}

//==============================================================================

void msg_mgr::AdvanceLogic( f32 DeltaSec )
{
    (void)DeltaSec;

/*
    // If there was a sound playing, see if it has finished.  If so, then there
    // is no longer a soung playing!

    if( m_Playing && (m_Timer == 0.0f) && !audio_IsPlaying( m_Playing ) )
    {
        m_Playing = 0;
        return;
    }

    // If there is currently no sound playing, and we have one in the queue, 
    // then, start 'er up!  But, set the pitch to 0 to give the streamed sound
    // time to spin up.  Set the timer to handle the delay.
    //
    // Also, we must use the correct voice for messages which originate from
    // a player in the game.

    if( !m_Playing && (m_QCount > 0) )
    {
        s32 Sound = MsgTable[ m_Queue[m_QRead].Index ].Sound;
        
        // Is this sound a player voice?
        if( (Sound & 0xFF000000) == 0 )
        {
            s16            Warrior;  
            player_object* pPlayer;

            // We need the player which originated this message so we can use
            // correct voice.  If we can't find the player, then the entire 
            // message is ignored.

            if( FetchArgValue( m_Queue[m_QRead], ARG_WARRIOR, Warrior ) &&
                (pPlayer = (player_object*)ObjMgr.GetObjectFromSlot( Warrior )) )
            {
                Sound += pPlayer->GetVoiceSfxBase();
            }
            else
            {
                // Couldn't correct the voice.  Ignore message.
                m_QRead = (m_QRead+1) & 3;
                m_QCount--;
                return;
            } 
        }

        m_Playing = audio_Play( Sound );
        audio_SetPitch( m_Playing, 0.0f );
        m_Timer  = 0.25f;
        m_OnDeck = m_Queue[m_QRead];

        m_QRead = (m_QRead+1) & 3;
        m_QCount--;
        return;
    }

    // If there is a sound "playing" but it is waiting for the timer to expire,
    // then advance the timer.  If the timer expires, then start the sound and
    // display the message.

    if( m_Playing && (m_Timer > 0.0f) )
    {
        m_Timer -= DeltaSec;
        if( m_Timer <= 0.0f )
        {
            m_Timer = 0.0f;
            audio_SetPitch( m_Playing, 1.0f );
            DisplayMsg( m_OnDeck );

            // Special Case Alert!  In split screen, the two players will often
            // receive the same messge.  Rather than play the sound twice, check
            // now to see if the new current message is the same as the PREVIOUS 
            // message (which is "OnDeck"), and if so, display it as well.
            
            if( (m_QCount > 0) &&
                (m_Queue[m_QRead].Index  == m_OnDeck.Index ) &&
                (m_Queue[m_QRead].Arg1   == m_OnDeck.Arg1  ) &&
                (m_Queue[m_QRead].Arg2   == m_OnDeck.Arg2  ) &&
                (m_Queue[m_QRead].Value1 == m_OnDeck.Value1) &&
                (m_Queue[m_QRead].Value2 == m_OnDeck.Value2) &&
                (m_Queue[m_QRead].Target != m_OnDeck.Target) )
            {
                DisplayMsg( m_Queue[m_QRead] );
                m_QRead = (m_QRead+1) & 3;
                m_QCount--;
            }

            // Special Case Alert!  In split screen, it is sometimes possible
            // for the two players to receive DIFFERENT messages with the SAME
            // ANNOUNCER audio.  Rather than play the sound twice, see if the 
            // previous message (which is "OnDeck") has the same ANNOUNCER sound 
            // buta different target.  If so, go ahead and just display the 
            // second message now.

            if( (m_QCount > 0) &&
                (MsgTable[ m_Queue[m_QRead].Index ].Sound & 0xFF000000) &&
                (MsgTable[ m_Queue[m_QRead].Index ].Sound == MsgTable[ m_OnDeck.Index ].Sound) &&
                (m_Queue[m_QRead].Target != m_OnDeck.Target) )
            {
                DisplayMsg( m_Queue[m_QRead] );
                m_QRead = (m_QRead+1) & 3;
                m_QCount--;
            }
        }
        return;
    }
*/
}

//==============================================================================

void msg_mgr::InjectColor( xwchar*& pString, xcolor Color )
{
    xwchar A, B;

    A   = 0xFF00;
    A  |= Color.R;

    B   = Color.G;
    B <<= 8;
    B  |= Color.B;

    if( B == 0 )    B = 1;

    *pString = A;   pString++;
    *pString = B;   pString++;
}

//==============================================================================

xcolor msg_mgr::GetColor( s32 Type )
{
    return( s_ColorTable[ Type ] );
}

//==============================================================================
// Given a message, convert it to a string and display it for recipient.
//
//    ARG_NONE,
//    ARG_KILLER,     // K
//    ARG_VICTIM,     // V
//    ARG_WARRIOR,    // W
//    ARG_INLINE,     // I
//    ARG_PICKUP,     // P
//    ARG_N,          // N
//    ARG_M,          // M
//    ARG_TEAM,       // T
//    ARG_OBJECTIVE,  // O
//    ARG_VEHICLE,    // S (Ship)    
//      <1> = his/her
//      <2> = himself/herself  

void msg_mgr::DisplayMsg( const msg& Msg )
{
    //x_DebugMsg( "MSGMGR: DisplayMsg (%d) %s\n", Msg.Index, MSG(Msg.Index) );

    if( MsgTable[Msg.Index].Where == WHERE_NONE )
        return;

    const game_score&   Score = GameMgr.GetScore();
    const xwchar*       pRead;
    const xwchar*       pInsert;
          xwchar        Template[256];
          xwchar        Message [256];
          xwchar*       pWrite;
          xwchar        Key;
          xbool         Error = FALSE;
          s16           Value;
          s32           Len;
          s32           BackColor;
          s32           Team = Score.Player[Msg.Target].Team;

    ASSERT( Msg.Index == MsgTable[Msg.Index].Index );

    x_wstrcpy( Template, StringMgr( "Messages", Msg.Index ) );
    pRead  = Template;
    pWrite = Message;

    // Determine "background" color.

    BackColor = MsgTable[Msg.Index].Color;

    ASSERT( BackColor != COLOR_ENEMY ); 
    ASSERT( BackColor != COLOR_ALLY  );

    if( (BackColor == COLOR_POSITIVE_ARG) ||
        (BackColor == COLOR_NEGATIVE_ARG) )
    {
        // Find a team based on available argument data.
        if( FetchArgValue( Msg, ARG_VICTIM, Value ) )
        {
            Value = Score.Player[Value].Team;
        }
        else
        if( FetchArgValue( Msg, ARG_KILLER, Value ) )
        {
            Value = Score.Player[Value].Team;
        }
        else
        if( FetchArgValue( Msg, ARG_TEAM, Value ) )
        {
            // All we need is: Value = Value;
        }
        else
            ASSERT( FALSE );

        // 'Value' now contains the team the message regards.
        // 'Msg.Target' is the player to receive the message.

        if( GameMgr.IsTeamBasedGame() )
        {
            if( MsgTable[Msg.Index].Color == COLOR_POSITIVE_ARG )
                BackColor = (Value == Team) ? COLOR_POSITIVE : COLOR_NEGATIVE;        
            else
                BackColor = (Value == Team) ? COLOR_NEGATIVE : COLOR_POSITIVE;        
        }
        else
        {
            if( MsgTable[Msg.Index].Color == COLOR_POSITIVE_ARG )
                BackColor = (Value == Team) ? COLOR_NEUTRAL  : COLOR_NEGATIVE;        
            else
                BackColor = (Value == Team) ? COLOR_NEGATIVE : COLOR_NEUTRAL;        
        }
    }

    // Inject background color into string.
    InjectColor( pWrite, s_ColorTable[BackColor] );

    // Process rest of string.

    while( *pRead && !Error )
    {
        if( *pRead != '<' )
        {
            *pWrite++ = *pRead++;
            continue;
        }

        pRead++;        // Skip '<'.
        Key = *pRead++; // Read key.
        pRead++;        // Skip '>'.

        switch( Key )
        {
        //--KILLER--------------------------------------------------------------
        case 'K':
            {
                if( !FetchArgValue( Msg, ARG_KILLER, Value ) )
                    goto BAIL_OUT;
                if( !(Score.Player[Value].IsInGame) )
                    goto BAIL_OUT;
                InjectColor( pWrite, (Score.Player[Value].Team == Team) 
                                     ? s_ColorTable[COLOR_ALLY ] 
                                     : s_ColorTable[COLOR_ENEMY] );
                Len = x_wstrlen( Score.Player[Value].Name );
                x_wstrcpy( pWrite, Score.Player[Value].Name );
                pWrite += Len;
                InjectColor( pWrite, s_ColorTable[BackColor] );
                break;
            }

        //--VICTIM--------------------------------------------------------------
        case 'V':
            {
                if( !FetchArgValue( Msg, ARG_VICTIM, Value ) )
                    goto BAIL_OUT;
                if( !Score.Player[Value].IsInGame )
                    goto BAIL_OUT;
                InjectColor( pWrite, (Score.Player[Value].Team == Team) 
                                     ? s_ColorTable[COLOR_ALLY ] 
                                     : s_ColorTable[COLOR_ENEMY] );
                Len = x_wstrlen( Score.Player[Value].Name );
                x_wstrcpy( pWrite, Score.Player[Value].Name );
                pWrite += Len;
                InjectColor( pWrite, s_ColorTable[BackColor] );
                break;
            }

        //--WARRIOR-------------------------------------------------------------
        case 'W':
            {
                if( !FetchArgValue( Msg, ARG_WARRIOR, Value ) )
                    goto BAIL_OUT;
                if( !Score.Player[Value].IsConnected )
                    goto BAIL_OUT;
                if( Score.Player[Value].IsInGame )
                    InjectColor( pWrite, (Score.Player[Value].Team == Team) 
                                         ? s_ColorTable[COLOR_ALLY ] 
                                         : s_ColorTable[COLOR_ENEMY] );
                else
                    InjectColor( pWrite, s_ColorTable[COLOR_NEUTRAL] );
                Len = x_wstrlen( Score.Player[Value].Name );
                x_wstrcpy( pWrite, Score.Player[Value].Name );
                pWrite += Len;
                InjectColor( pWrite, s_ColorTable[BackColor] );
                break;
            }

        //--INLINE--------------------------------------------------------------
        case 'I':
            {
                if( !FetchArgValue( Msg, ARG_INLINE, Value ) )
                    goto BAIL_OUT;
                InjectColor( pWrite, s_ColorTable[COLOR_NEUTRAL] );
                Len = x_wstrlen( m_InlineName[Value] );
                x_wstrcpy( pWrite, m_InlineName[Value] );
                pWrite += Len;
                InjectColor( pWrite, s_ColorTable[BackColor] );
                break;
            }

        //--PICKUP--------------------------------------------------------------
        case 'P':
            {
                if( !FetchArgValue( Msg, ARG_PICKUP, Value ) )
                    goto BAIL_OUT;
                ASSERT( IN_RANGE( pickup::KIND_WEAPON_DISK_LAUNCHER, Value, 
                                  pickup::KIND_DEPLOY_PACK_SATCHEL_CHARGE ) );
                pInsert = StringMgr( "Pickups", Value );
                InjectColor( pWrite, s_ColorTable[COLOR_URGENT] );
                ASSERT( pInsert );
                Len = x_wstrlen( pInsert );
                x_wstrcpy( pWrite, pInsert );
                pWrite += Len;
                InjectColor( pWrite, s_ColorTable[BackColor] );
                break;
            }

        //--N and M-------------------------------------------------------------
        case 'N':
        case 'M':
            {
                if( !FetchArgValue( Msg, (Key == 'N') ? ARG_N : ARG_M, Value ) )
                    goto BAIL_OUT;
                xfs XFS( "%d", Value );
                const char* pN = XFS;
                while( *pN )
                {
                    *pWrite++ = (xwchar)(*pN);
                    pN++;
                }
                break;
            }

        //--TEAM----------------------------------------------------------------
        case 'T':
            {
                if( !FetchArgValue( Msg, ARG_TEAM, Value ) )
                    goto BAIL_OUT;
                ASSERT( IN_RANGE( 0, Value, 1 ) );
                if( !Score.IsTeamBased )
                    goto BAIL_OUT;
                InjectColor( pWrite, (Score.Player[Value].Team == Team) 
                                     ? s_ColorTable[COLOR_ALLY ] 
                                     : s_ColorTable[COLOR_ENEMY] );
                Len = x_wstrlen( Score.Team[Value].Name );
                x_wstrcpy( pWrite, Score.Team[Value].Name );
                pWrite += Len;
                InjectColor( pWrite, s_ColorTable[BackColor] );
                break;
            }

        //--OBJECTIVE-----------------------------------------------------------
        case 'O':
            break;

        //--SHIP (vehicle)------------------------------------------------------
        case 'S':
            break;

        //--HIS/HER (of killer or victim)---------------------------------------
        //--HIMSELF/HERSELF (of killer or victim)-------------------------------
        case '1':
        case '2':
            {
                if( !FetchArgValue( Msg, ARG_KILLER, Value ) )
                    if( !FetchArgValue( Msg, ARG_VICTIM, Value ) )
                        goto BAIL_OUT;
                if( !Score.Player[Value].IsInGame )
                    goto BAIL_OUT;
                if( Score.Player[Value].IsMale )
                    pInsert = StringMgr( "ui", (Key == '1') ? IDS_HIS  : IDS_HIMSELF );
                else
                    pInsert = StringMgr( "ui", (Key == '1') ? IDS_HERS : IDS_HERSELF ); 
                ASSERT( pInsert );
                Len = x_wstrlen( pInsert );
                x_wstrcpy( pWrite, pInsert );
                pWrite += Len;
                break;
            }
        }
    }
    *pWrite++ = '\0';

    return;

BAIL_OUT: ;
    //x_DebugMsg( "MsgMgr -- Error processing message:  M:%d  P:%d  1:%d:%d  2:%d:%d\n",
    //            Msg.Index, Msg.Target,
    //            Msg.Arg1, Msg.Value1,
    //            Msg.Arg2, Msg.Value2 );
}

//==============================================================================

void msg_mgr::PlayerDied( const pain& Pain )
{
    msg   Msg;
    s32   Victim  = Pain.VictimID.Slot;
    s32   Killer  = Pain.OriginID.Slot;
    s32   Message = -1;
    xbool Generic = TRUE;

    const game_score& Score = GameMgr.GetScore();

    // No victim?  No message!  (Should probably assert, but we'll be gentle.)
    if( !IN_RANGE( 0, Victim, 31 ) )
        return;

    //
    // BEGIN - Series of criteria to choose message.
    //

    // TEAMKILL.
    if( (Killer != Victim) &&               // Not self kill.
        (IN_RANGE( 0, Killer, 31 )) &&      // Killer is a player.
        (Score.Player[Killer].Team == Score.Player[Victim].Team) )  // SAME TEAM
    {
        Message = MSG_DEATH_TEAMKILLED;
        Generic = FALSE;
    }

    // Manual suicide.
    else
    if( Pain.PainType == pain::MISC_PLAYER_SUICIDE )
    {
        Message = MSG_DEATH_SUICIDE;
        Generic = FALSE;
    }

    // Accidental suicide by impact with static world.
    else
    if( Pain.PainType == pain::HIT_IMMOVEABLE )
    {
        Message = MSG_DEATH_LANDED_TOO_HARD;
    }

    // Accidental suicide.
    else
    if( Killer == Victim )
    {
        if( Pain.PainType == pain::WEAPON_MINE )
            Message = MSG_DEATH_OWN_MINE;
        else 
            Message = MSG_DEATH_KILLED_SELF + Random.irand(0,3) % 3;  // %3 on purpose.
        Generic = FALSE;
    }

    // Special case: Explosion of any kind.
    else
    if( IN_RANGE( pain::EXPLOSION_GENERATOR, Pain.PainType, pain::EXPLOSION_TRANSPORT ) )
    {
        Message = MSG_DEATH_CAUGHT_BLAST;
    }

    // Special case: Hit by moving object controlled by another player.
    else
    if( (Killer != Victim) &&
        (IN_RANGE( 0, Killer, 31 )) && 
        (IN_RANGE( pain::HIT_BY_ARMOR_LIGHT, Pain.PainType, pain::HIT_BY_TRANSPORT )) )
    {
        Message = MSG_DEATH_MOWED_DOWN;
    }

    // Killer is another player.
    else 
    if( (Killer != Victim) &&
        (IN_RANGE( 0, Killer, 31 )) )
    {
        switch( Pain.PainType )
        {
        case pain::WEAPON_CHAINGUN:        Message = MSG_DEATH_GUNNED_DOWN;     break;
        case pain::WEAPON_BLASTER:         Message = MSG_DEATH_BLASTED;         break;
        case pain::WEAPON_PLASMA:          Message = MSG_DEATH_ATE_PLASMA + Random.irand(0,1);  break;
        case pain::WEAPON_MISSILE:         Message = MSG_DEATH_SHOT_DOWN;       break;
        case pain::WEAPON_LASER:           Message = MSG_DEATH_PICKED_OFF;      break;
        case pain::WEAPON_LASER_HEAD_SHOT: Message = MSG_DEATH_PICKED_OFF;      break;
        case pain::WEAPON_MINE:            Message = MSG_DEATH_OTHERS_MINE;     break;
        case pain::WEAPON_SATCHEL:         Message = MSG_DEATH_DETONATED;       break;
        case pain::WEAPON_SATCHEL_SMALL:   Message = MSG_DEATH_DETONATED;       break;
        case pain::WEAPON_BOMBER_BOMB:     Message = MSG_DEATH_BOMBED;          break;
        case pain::TURRET_CLAMP:           Message = MSG_DEATH_TURRET_STOPPED;  break;
        }
    }

    // Killer is not another player.
    else
    {
        switch( Pain.PainType )
        {
        case pain::WEAPON_BLASTER:         Message = MSG_DEATH_SENTRY_TURRET;   break; // HACK HACK
        case pain::WEAPON_MINE:            Message = MSG_DEATH_A_MINE;          break;
        case pain::WEAPON_BOMBER_BOMB:     Message = MSG_DEATH_GOT_BOMBED;      break;
        case pain::TURRET_AA:              Message = MSG_DEATH_GOT_SHOT_DOWN + Random.irand(0,1);  break;
        case pain::TURRET_PLASMA:          Message = MSG_DEATH_PLASMA_TURRET;   break;
        case pain::TURRET_MISSILE:         Message = MSG_DEATH_MISSILE       + Random.irand(0,1);  break;
        case pain::TURRET_MORTAR:          Message = MSG_DEATH_CAUGHT_MORTAR + Random.irand(0,1);  break;
        case pain::TURRET_SENTRY:          Message = MSG_DEATH_SENTRY_TURRET;   break;
        case pain::TURRET_CLAMP:           Message = MSG_DEATH_REMOTE_TURRET;   break;
        case pain::MISC_VEHICLE_SPAWN:     Message = MSG_DEATH_RECYCLED;        break;
        case pain::MISC_FORCE_FIELD:       Message = MSG_DEATH_FORCE_FIELD;     break;
        case pain::MISC_BOUNDS_DEATH:      Message = MSG_DEATH_TOO_FAR;         break;
        }
    }

    //
    // END - Series of criteria to choose message.
    //

    // Now, consider using a generic message just to add variety.
    if( Generic && ((Message == -1) || (Random.irand(0,3) == 0)) )
    {
        // Killer is another player.
        if( (Killer != Victim) &&
            (IN_RANGE( 0, Killer, 31 )) &&
            (Message != MSG_DEATH_TURRET_STOPPED) )
        {
            Message = Random.irand( MSG_DEATH_TOOK_OUT, MSG_DEATH_FINISHED_OFF );
        }
        else
        {
            if( GameMgr.GetGameType() == GAME_CAMPAIGN )
                Message = Random.irand( MSG_DEATH_NEW_ARMOR, MSG_DEATH_SPARE_PARTS );
            else
                Message = Random.irand( MSG_DEATH_NEW_ARMOR, MSG_DEATH_RESPAWN );
        }
    }

    //
    // That's it.  We should now have our message.
    //
    ASSERT( Message != -1 );

    Msg.Index = Message;
    Msg.Arg1  = MsgTable[Message].Arg1;
    Msg.Arg2  = MsgTable[Message].Arg2;

    Msg.Value1 = Victim;
    if( Msg.Arg2 == ARG_KILLER )
        Msg.Value2 = Killer;
    else
        Msg.Value2 = -1;

    ASSERT( Msg.Index == MsgTable[Msg.Index].Index );
    DistributeMsg( Msg );

    //
    // TO DO: More auto chatter here.
    //

    if( GameMgr.GetGameType() == GAME_CAMPAIGN )
        return;

    // Chatter: Death taunt.
    if( (Msg.Index != MSG_DEATH_TEAMKILLED) &&
        (Pain.OriginID.Slot != -1) && 
        (Pain.OriginID.Slot <  32) && 
        (Pain.OriginID.Slot != Pain.VictimID.Slot) &&
        (Pain.PainType < pain::WEAPON_MINE) )
    {
        Msg.Index  = Random.irand( 0, 3 ) + MSG_DEATH_TAUNT;
        Msg.Arg1   = ARG_WARRIOR;
        Msg.Arg2   = ARG_NONE;
        Msg.Value1 = Pain.OriginID.Slot;
        Msg.Value2 = -1;
        Msg.Target = Pain.VictimID.Slot;
        ASSERT( Msg.Index == MsgTable[Msg.Index].Index );
        DistributeMsg( Msg );
    }
}

//==============================================================================

void msg_mgr::DistributeMsg( msg& Msg )
{
    s32 i;
    s32 Target   = Msg.Target;
    s32 Audience = MsgTable[Msg.Index].Audience;

    if( (Msg.Index != MSG_VOTE_KICK) && 
        (Msg.Index != MSG_VOTE_KICK_FAILS) &&
        (Msg.Index != MSG_VOTE_KICK_PASSES) &&
        (Msg.Index != MSG_VOTE_CHANGE_MAP) )
    {
        LogMgr.LogMsg( Msg );
    }

    // Special case.  Under some circumstances, a client can send a message to
    // himself.  In this situation, do NOT attempt to route the message via the
    // server (since it isn't here).
    if( !tgl.ServerPresent )
    {
        if( Audience == HEAR_MACHINE )
        {
            Msg.Target = tgl.PC[0].PlayerIndex;
            ProcessMsg( Msg );
            if( tgl.NLocalPlayers == 2 )
            {
                Msg.Target = tgl.PC[1].PlayerIndex;
                ProcessMsg( Msg );
            }
        }
        else
        {
            ProcessMsg( Msg );
        }
        return;
    }

    const game_score& Score = GameMgr.GetScore();

    switch( Audience )
    {
    case HEAR_ALL:
        for( i = 0; i < 32; i++ )
        {
            if( (Score.Player[i].IsInGame) && 
                (Score.Player[i].IsBot != TRUE) )
            {
                Msg.Target = i;
                tgl.pServer->SendMsg( Msg );
            }
        }
        ProcessMsg( Msg );
        break;

    case HEAR_TEAM:
        for( i = 0; i < 32; i++ )
        {
            if( (Score.Player[i].IsInGame) && 
                (Score.Player[i].Team  == Target) && 
                (Score.Player[i].IsBot == FALSE) )
            {
                Msg.Target = i;
                tgl.pServer->SendMsg( Msg );
            }
        }
        break;

    case HEAR_LOCAL:
    {
        vector3 Pos1;
        vector3 Pos2;
        vector3 Delta;
        object* pObject;
        pObject = ObjMgr.GetObjectFromSlot( Target );
        if( !pObject )
            break;
        Pos1 = pObject->GetPosition();
        ObjMgr.StartTypeLoop( object::TYPE_PLAYER );
        while( (pObject = ObjMgr.GetNextInType()) )
        {
            Pos2  = pObject->GetPosition();
            Delta = Pos1 - Pos2;
            if( Delta.LengthSquared() < SQR(50) )
            {
                Msg.Target = pObject->GetObjectID().Slot;
                tgl.pServer->SendMsg( Msg );
            }
        }
        ObjMgr.EndTypeLoop();
        break;
    }

    case HEAR_MACHINE:
        Msg.Target = tgl.PC[0].PlayerIndex;
        ProcessMsg( Msg );
        if( tgl.NLocalPlayers == 2 )
        {
            Msg.Target = tgl.PC[1].PlayerIndex;
            ProcessMsg( Msg );
        }
        break;

    case HEAR_SINGLE:
        if( (Score.Player[Target].IsInGame) && 
            (Score.Player[Target].IsBot == FALSE) )
        {
            tgl.pServer->SendMsg( Msg );
        }
        break;
    }
}

//==============================================================================

void msg_mgr::VerifyArg( s32 MsgType, s32 Arg )
{
    if( Arg == ARG_NONE )
        return;

    xwchar Key[10] = { 'K', 'V', 'W', 'I', 'P', 'N', 'M', 'T', 'O', 'S' };
    xwchar Sub[ 4] = { '<', Key[Arg], '>', '\0' };

    ASSERT( x_wstrstr( StringMgr( "Messages", MsgType ), Sub ) );
}

//==============================================================================

void msg_mgr::Message( s32 MsgType, 
                       s32 Target,
                       s32 Arg1,    // = ARG_NONE,
                       s32 Value1,  // = -1,
                       s32 Arg2,    // = ARG_NONE,
                       s32 Value2 ) // = -1 );
{
    VerifyArg( MsgType, Arg1 );
    VerifyArg( MsgType, Arg2 );

    //x_DebugMsg( "MSGMGR: Message %d(%s)\n", MsgType, MSG(MsgType) );

    msg Msg;

    Msg.Index  = MsgType;
    Msg.Target = Target;
    Msg.Arg1   = Arg1;
    Msg.Value1 = Value1;
    Msg.Arg2   = Arg2;
    Msg.Value2 = Value2;

    // Special cases: MSG_POSITIVE, MSG_NEGATIVE
    if( (MsgType == MSG_POSITIVE) || (MsgType == MSG_NEGATIVE) )
    {
        Msg.Index += Random.irand( 0, 1 );
    }

    // Special case: Make sure messages with "voice" audio have ARG_WARRIOR.
    if( (MsgTable[Msg.Index].Sound != 0) &&
        ((MsgTable[Msg.Index].Sound & 0xFF000000) == 0) &&
        (Msg.Arg1 != ARG_WARRIOR) && 
        (Msg.Arg2 != ARG_WARRIOR) && 
        (Msg.Arg2 == ARG_NONE) )
    {
        Msg.Arg2 = Target;    
    }

    // Special case: Assign a position to any inline name.
    s32 Name = -1;
    if( Msg.Arg1 == ARG_INLINE )   Name = Msg.Value1;
    if( Msg.Arg2 == ARG_INLINE )   Name = Msg.Value2;
    if( Name != -1 )
    {
        const game_score& Score = GameMgr.GetScore();
        if( Score.Player[Name].IsConnected )
        {
            if( x_wstrcmp( m_InlineName[m_InlineLast], Score.Player[Name].Name ) )
            {
                m_InlineLast = (m_InlineLast + 1) & 0x0F;
                x_wstrcpy( m_InlineName[m_InlineLast], Score.Player[Name].Name );
            }
            if( Msg.Arg1 == ARG_INLINE )   Msg.Value1 = m_InlineLast;
            if( Msg.Arg2 == ARG_INLINE )   Msg.Value2 = m_InlineLast;
        }
        else
        {
            // Can't resolve the inline name.  Just bail.
            return;
        }
    }

    // Quick sanity check.
    ASSERT( Msg.Index == MsgTable[Msg.Index].Index );

    // Ship it!
    DistributeMsg( Msg );
}

//==============================================================================

xwchar* msg_mgr::GetInlineName( s32 Index )
{
    return( m_InlineName[Index] );
}

//==============================================================================
