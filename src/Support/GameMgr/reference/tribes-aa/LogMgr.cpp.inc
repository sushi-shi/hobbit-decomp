//==============================================================================
//
//  LogMgr.cpp
//
//==============================================================================

#include "LogMgr.hpp"

#include "GameMgr.hpp"

#include "TelnetConsole\TelnetMgr.hpp"
#include <Support/StringMgr/StringMgr.hpp>
#include "AADS\Data\UI\Messages.h"

#ifdef TARGET_PC
#include <time.h>
#include <io.h>
#endif

#ifdef TARGET_LINUX
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#endif

xbool TagPlayers = FALSE;

//==============================================================================
//  STORAGE
//==============================================================================

log_info    LogTable[] = 
{
/* MSG_DEATH_TEAMKILLED                */  { "<K> TEAMKILLED <V>."                               , ARG_VICTIM    , ARG_KILLER     , STYLE_NEGATIVE      },
/* MSG_DEATH_SUICIDE                   */  { "<V> suicided."                                     , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_OWN_MINE                  */  { "<V> tripped <1> own mine."                         , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_LANDED_TOO_HARD           */  { "<V> landed too hard."                              , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_KILLED_SELF               */  { "<V> killed <2>."                                   , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_NEW_ARMOR                 */  { "<V> needs new armor."                              , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_SPARE_PARTS               */  { "<V> became spare parts."                           , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_RESPAWN                   */  { "<V> will respawn shortly."                         , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_CAUGHT_BLAST              */  { "<V> caught the blast."                             , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_GOT_BOMBED                */  { "<V> got bombed."                                   , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_TOO_FAR                   */  { "<V> just went too far."                            , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_FORCE_FIELD               */  { "<V> died in a force field."                        , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_A_MINE                    */  { "<V> tripped a mine."                               , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_MISSILE                   */  { "<V> received a missile."                           , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_GOT_SHOT_DOWN             */  { "<V> got shot down."                                , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_AA_TURRET                 */  { "An AA turret shot <V> down."                       , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_REMOTE_TURRET             */  { "A remote turret got <V>."                          , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_SENTRY_TURRET             */  { "A sentry turret nailed <V>."                       , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_CAUGHT_MORTAR             */  { "<V> caught a mortar shell."                        , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_MORTAR_TURRET             */  { "A mortar turret got <V>."                          , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_PLASMA_TURRET             */  { "A plasma turret fried <V>."                        , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_RECYCLED                  */  { "<V> was recycled."                                 , ARG_VICTIM    , ARG_NONE       , STYLE_NORMAL        },
/* MSG_DEATH_TOOK_OUT                  */  { "<K> took out <V>."                                 , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_ELIMINATED                */  { "<K> eliminated <V>."                               , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_DEFEATED                  */  { "<K> defeated <V>."                                 , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_SMOKED                    */  { "<K> smoked <V>."                                   , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_DEMOLISHED                */  { "<K> demolished <V>."                               , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_NAILED                    */  { "<K> nailed <V>."                                   , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_FINISHED_OFF              */  { "<K> finished off <V>."                             , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_BLASTED                   */  { "<K> blasted <V>."                                  , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_BOMBED                    */  { "<K> bombed <V>."                                   , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_GUNNED_DOWN               */  { "<K> gunned down <V>."                              , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_OTHERS_MINE               */  { "<V> tripped <K>'s mine."                           , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_SHOT_DOWN                 */  { "<K> shot <V> down."                                , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_ATE_PLASMA                */  { "<V> ate <K>'s plasma."                             , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_FRIED                     */  { "<K> fried <V>."                                    , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_TURRET_STOPPED            */  { "<K>'s turret stopped <V>."                         , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_DETONATED                 */  { "<K> detonated <V>."                                , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_PICKED_OFF                */  { "<K> picked off <V>."                               , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_DEATH_MOWED_DOWN                */  { "<K> mowed down <V>."                               , ARG_VICTIM    , ARG_KILLER     , STYLE_NORMAL        },
/* MSG_POSITIVE                        */  { "<W>:  \"Awesome!\""                                , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_POSITIVE_1                      */  { "<W>:  \"Woohoo!\""                                 , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_NEGATIVE                        */  { "<W>:  \"Oops...\""                                 , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_NEGATIVE_1                      */  { "<W>:  \"Shazbot!\""                                , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_DEATH_TAUNT                     */  { NULL                                                , ARG_WARRIOR   , ARG_NONE       , STYLE_NONE          },
/* MSG_DEATH_TAUNT_1                   */  { NULL                                                , ARG_WARRIOR   , ARG_NONE       , STYLE_NONE          },
/* MSG_DEATH_TAUNT_2                   */  { NULL                                                , ARG_WARRIOR   , ARG_NONE       , STYLE_NONE          },
/* MSG_DEATH_TAUNT_3                   */  { NULL                                                , ARG_WARRIOR   , ARG_NONE       , STYLE_NONE          },
/* MSG_AUTO_RECOVER_FLAG               */  { "<W>:  \"Recover our flag.\""                       , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_GET_ENEMY_FLAG             */  { "<W>:  \"Get the enemy flag.\""                     , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_DESTROY_TURRETS            */  { "<W>:  \"Destroy enemy turrets.\""                  , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_ENEMY_IN_BASE              */  { "<W>:  \"The enemy is in our base.\""               , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_COVER_FLAG_CARRIER         */  { "<W>:  \"Cover our flag carrier.\""                 , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_DEFEND_OUR_FLAG            */  { "<W>:  \"Defend our flag.\""                        , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_COVER_ME                   */  { "<W>:  \"Cover me!\""                               , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_ENEMY_BASE_DISABLED        */  { "<W>:  \"The enemy base is disabled.\""             , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_GENERATOR_DESTROYED        */  { "<W>:  \"Enemy generator destroyed.\""              , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_REMOTE_EQUIPMENT_DESTROYED */  { "<W>:  \"Enemy remote equipment destroyed.\""       , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SENSORS_DESTROYED          */  { "<W>:  \"Enemy sensor destroyed.\""                 , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_TURRETS_DESTROYED          */  { "<W>:  \"Enemy turret destroyed.\""                 , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_VEHICLE_STATION_DESTROYED  */  { "<W>:  \"Enemy vehicle station destroyed.\""        , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_HAVE_FLAG                  */  { "<W>:  \"I have the enemy flag.\""                  , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_RETRIEVE_FLAG              */  { "<W>:  \"Retrieve our flag!\""                      , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_FLAG_SECURE                */  { "<W>:  \"Our flag is secure.\""                     , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_AWESOME                    */  { "<W>:  \"Awesome.\""                                , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_BYE                        */  { "<W>:  \"Bye!\""                                    , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_GOOD_GAME                  */  { "<W>:  \"Good game.\""                              , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_HI                         */  { "<W>:  \"Hi!\""                                     , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_OOPS                       */  { "<W>:  \"Oops...\""                                 , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_YOU_ROCK                   */  { "<W>:  \"You rock!\""                               , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SHAZBOT                    */  { "<W>:  \"Shazbot!\""                                , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_WOOHOO                     */  { "<W>:  \"Woohoo!\""                                 , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_GUNSHIP_READY              */  { "<W>:  \"Gunship ready.  Need a ride?\""            , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_WHERE_TO                   */  { "<W>:  \"Where to?\""                               , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_REPAIR_BASE                */  { "<W>:  \"Repair our base.\""                        , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_REPAIR_GENERATOR           */  { "<W>:  \"Repair our generator.\""                   , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_REPAIR_ME                  */  { "<W>:  \"Repair me!\""                              , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_REPAIR_SENSORS             */  { "<W>:  \"Repair our sensors.\""                     , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_REPAIR_TURRETS             */  { "<W>:  \"Repair our turrets.\""                     , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_REPAIR_VEHICLE_STATION     */  { "<W>:  \"Repair our vehicle station.\""             , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_ATTACK                */  { "<W>:  \"I'll attack the enemy.\""                  , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_ATTACK_BASE           */  { "<W>:  \"I'll attack the enemy base.\""             , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_ATTACK_FLAG           */  { "<W>:  \"I'll go for the enemy flag.\""             , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_ATTACK_GENERATOR      */  { "<W>:  \"I'll attack the enemy generator.\""        , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_ATTACK_SENSORS        */  { "<W>:  \"I'll attack the enemy sensors.\""          , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_ATTACK_TURRETS        */  { "<W>:  \"I'll attack the enemy turrets.\""          , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_ATTACK_VEHICLE_STATIO */  { "<W>:  \"I'll attack the enemy vehicle station.\""  , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_DEFEND_BASE           */  { "<W>:  \"I'll defend our base.\""                   , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_DEFEND                */  { "<W>:  \"I'm on defense.\""                         , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_DEFEND_FLAG           */  { "<W>:  \"I'll defend our flag.\""                   , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_REPAIR_BASE           */  { "<W>:  \"I'll repair our base.\""                   , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_REPAIR_EQUIPMENT      */  { "<W>:  \"I'll repair our equipment.\""              , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_REPAIR_GENERATOR      */  { "<W>:  \"I'll repair our generator.\""              , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_REPAIR                */  { "<W>:  \"I'm on repairs.\""                         , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_REPAIR_SENSORS        */  { "<W>:  \"I'll repair our sensors.\""                , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_REPAIR_TURRETS        */  { "<W>:  \"I'll repair our turrets.\""                , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_REPAIR_VEHICLE_STATIO */  { "<W>:  \"I'll repair our vehicle station.\""        , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_COVER_YOU             */  { "<W>:  \"I'll cover you.\""                         , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_SET_UP_DEFENSE        */  { "<W>:  \"I'll set up defenses.\""                   , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_DEPLOY_REMOTES        */  { "<W>:  \"I'll deploy remote equipment.\""           , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_DEPLOY_SENSORS        */  { "<W>:  \"I'll deploy sensors.\""                    , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SELF_DEPLOY_TURRETS        */  { "<W>:  \"I'll deploy turrets.\""                    , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_TARGET_ACQUIRED            */  { "<W>:  \"Target acquired.\""                        , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_TARGET_DESTROYED           */  { "<W>:  \"Target destroyed.\""                       , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_HELP                       */  { "<W>:  \"Help!\""                                   , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_MOVE                       */  { "<W>:  \"Move!\""                                   , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_SORRY                      */  { "<W>:  \"Sorry.\""                                  , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_WARN_BOMBER                */  { "<W>:  \"Incoming bomber!\""                        , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_WARN_HOSTILES              */  { "<W>:  \"Incoming hostiles!\""                      , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_WARN_VEHICLES              */  { "<W>:  \"Incoming vehicle!\""                       , ARG_WARRIOR   , ARG_NONE       , STYLE_CHAT          },
/* MSG_AUTO_WARN_FRIENDLY_FIRE         */  { "<W>:  \"Watch where you're shooting!\""            , ARG_N         , ARG_NONE       , STYLE_CHAT          },
/* MSG_SCORE_DESTROY_ENEMY_GEN         */  { "<W> destroyed an enemy generator."                 , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DESTROY_ENEMY_VSTATION    */  { "<W> destroyed an enemy vehicle station."           , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DESTROY_ENEMY_FSTATION    */  { "<W> destroyed an enemy inventory station."         , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DESTROY_ENEMY_FTURRET     */  { "<W> destroyed an enemy turret."                    , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DESTROY_ENEMY_FSENSOR     */  { "<W> destroyed an enemy sensor."                    , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DESTROY_ENEMY_RSTATION    */  { "<W> destroyed an enemy remote station."            , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DESTROY_ENEMY_RTURRET     */  { "<W> destroyed an enemy remote turret."             , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DESTROY_ENEMY_RSENSOR     */  { "<W> destroyed an enemy remote sensor."             , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DESTROY_ENEMY_GRAVCYCLE   */  { "<W> destroyed an enemy grav cycle."                , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DESTROY_ENEMY_FIGHTER     */  { "<W> destroyed an enemy fighter."                   , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DESTROY_ENEMY_BOMBER      */  { "<W> destroyed an enemy bomber."                    , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DESTROY_ENEMY_TRANSPORT   */  { "<W> destroyed an enemy transport."                 , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DISABLE_TEAM_GEN          */  { "<W> disabled a team generator."                    , ARG_N         , ARG_NONE       , STYLE_NEGATIVE      },
/* MSG_SCORE_DISABLE_TEAM_VSTATION     */  { "<W> disabled a team vehicle station."              , ARG_N         , ARG_NONE       , STYLE_NEGATIVE      },
/* MSG_SCORE_DISABLE_TEAM_FSTATION     */  { "<W> disabled a team inventory station."            , ARG_N         , ARG_NONE       , STYLE_NEGATIVE      },
/* MSG_SCORE_DISABLE_TEAM_FTURRET      */  { "<W> disabled a team turret."                       , ARG_N         , ARG_NONE       , STYLE_NEGATIVE      },
/* MSG_SCORE_DISABLE_TEAM_FSENSOR      */  { "<W> disabled a team sensor."                       , ARG_N         , ARG_NONE       , STYLE_NEGATIVE      },
/* MSG_SCORE_DISABLE_TEAM_RSTATION     */  { "<W> disabled a team remote station."               , ARG_N         , ARG_NONE       , STYLE_NEGATIVE      },
/* MSG_SCORE_DISABLE_TEAM_RTURRET      */  { "<W> disabled a team remote turret."                , ARG_N         , ARG_NONE       , STYLE_NEGATIVE      },
/* MSG_SCORE_DISABLE_TEAM_RSENSOR      */  { "<W> disabled a team remote sensor."                , ARG_N         , ARG_NONE       , STYLE_NEGATIVE      },
/* MSG_SCORE_DEFEND_TEAM_GEN           */  { "<W> defended a generator."                         , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_DEFEND_TEAM_SWITCH        */  { "<W> defended an objective."                        , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_REPAIR_TEAM_GEN           */  { "<W> repaired a generator."                         , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_REPAIR_TEAM_VSTATION      */  { "<W> repaired a vehicle station."                   , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_REPAIR_TEAM_FSTATION      */  { "<W> repaired an inventory station."                , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_REPAIR_TEAM_FTURRET       */  { "<W> repaired a turret."                            , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_REPAIR_TEAM_FSENSOR       */  { "<W> repaired a sensor."                            , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_REPAIR_TEAM_RSTATION      */  { "<W> repaired a remote station."                    , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_REPAIR_TEAM_RTURRET       */  { "<W> repaired a remote turret."                     , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_REPAIR_TEAM_RSENSOR       */  { "<W> repaired a remote sensor."                     , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_SUICIDE                   */  { NULL                                                , ARG_N         , ARG_NONE       , STYLE_NONE          },
/* MSG_SCORE_TEAMKILL                  */  { NULL                                                , ARG_N         , ARG_NONE       , STYLE_NONE          },
/* MSG_SCORE_TAKE_OBJECTIVE           **/  { "<W> captured an objective for <A>."                , ARG_N         , ARG_NONE       , STYLE_GAME_NORMAL   },
/* MSG_SCORE_FLARE_MISSILE             */  { "<W> provided a flare assist."                      , ARG_N         , ARG_NONE       , STYLE_NORMAL        },
/* MSG_SCORE_CTF_CAP_FLAG             **/  { "<W> captured the <E> flag."                        , ARG_N         , ARG_NONE       , STYLE_GAME_URGENT   },
/* MSG_SCORE_CTF_TAKE_FLAG            **/  { "<W> took the <E> flag."                            , ARG_N         , ARG_NONE       , STYLE_GAME_URGENT   },
/* MSG_SCORE_CTF_DEFEND_FLAG          **/  { "<W> defended the <A> flag."                        , ARG_N         , ARG_NONE       , STYLE_GAME_NORMAL   },
/* MSG_SCORE_CTF_RETURN_FLAG          **/  { "<W> returned the <A> flag."                        , ARG_N         , ARG_NONE       , STYLE_GAME_POSITIVE },
/* MSG_SCORE_CTF_DEFEND_CARRIER       **/  { "<W> defended the <A> flag carrier."                , ARG_N         , ARG_NONE       , STYLE_GAME_NORMAL   },
/* MSG_SCORE_CTF_KILL_CARRIER         **/  { "<W> killed the <E> flag carrier."                  , ARG_N         , ARG_NONE       , STYLE_GAME_NORMAL   },
/* MSG_SCORE_CNH_HOLD_OBJECTIVE      ***/  { "Team <T> has held their new objective."            , ARG_N         , ARG_NONE       , STYLE_GAME_NORMAL   },
/* MSG_PICKUP_TAKEN                    */  { NULL                                                , ARG_PICKUP    , ARG_NONE       , STYLE_NONE          },
/* MSG_PICKUP_EXCHANGE_AVAILABLE       */  { NULL                                                , ARG_PICKUP    , ARG_NONE       , STYLE_NONE          },
/* MSG_VEHICLE_DESTROYED_TEAM          */  { NULL                                                , ARG_KILLER    , ARG_VEHICLE    , STYLE_NONE          },
/* MSG_VEHICLE_DESTROYED_ENEMY         */  { NULL                                                , ARG_KILLER    , ARG_VEHICLE    , STYLE_NONE          },
/* MSG_PLAYER_CONNECTED                */  { NULL /*"<I> has connected."*/                       , ARG_INLINE    , ARG_NONE       , STYLE_ADMIN_NORMAL  },
/* MSG_PLAYER_JOINED_GAME              */  { "<I> has joined the game."                          , ARG_INLINE    , ARG_NONE       , STYLE_ADMIN_NORMAL  },
/* MSG_PLAYER_JOINED_TEAM              */  { "<I> joined team <T>."                              , ARG_INLINE    , ARG_TEAM       , STYLE_ADMIN_NORMAL  },
/* MSG_PLAYER_DISCONNECTED             */  { NULL /*"<I> has disconnected."*/                    , ARG_INLINE    , ARG_NONE       , STYLE_ADMIN_NORMAL  },
/* MSG_ADMIN_PLAYER_KICKED_BY_ADMIN    */  { "<I> was kicked by the SERVER."                     , ARG_INLINE    , ARG_NONE       , STYLE_ADMIN_NORMAL  },
/* MSG_ADMIN_CHANGED_TEAM              */  { "SERVER moved <W> to <T>."                          , ARG_WARRIOR   , ARG_TEAM       , STYLE_ADMIN_NORMAL  },
/* MSG_VOTE_KICK_WARNING               */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_VOTE_KICK                       */  { "<K> initiated a vote to kick <W>."                 , ARG_WARRIOR   , ARG_KILLER     , STYLE_ADMIN_NORMAL  },
/* MSG_VOTE_CHANGE_MAP            <L>?**/  { "<K> initiated a map vote."                         , ARG_KILLER    , ARG_NONE       , STYLE_ADMIN_NORMAL  },
/* MSG_VOTE_KICK_FAILS                 */  { "Vote to kick <W> fails."                           , ARG_WARRIOR   , ARG_NONE       , STYLE_ADMIN_NORMAL  },
/* MSG_VOTE_MAP_FAILS                  */  { "Vote to change map fails."                         , ARG_NONE      , ARG_NONE       , STYLE_ADMIN_NORMAL  },
/* MSG_VOTE_KICK_PASSES                */  { "Vote to kick <W> passes."                          , ARG_WARRIOR   , ARG_NONE       , STYLE_ADMIN_NORMAL  },
/* MSG_VOTE_MAP_PASSES                 */  { "Vote to change map passes."                        , ARG_NONE      , ARG_NONE       , STYLE_ADMIN_NORMAL  },
/* MSG_OUT_OF_BOUNDS                   */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_IN_BOUNDS                       */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_PLAYER_SWITCHED_TEAM            */  { "<W> switched to team <T>."                         , ARG_WARRIOR   , ARG_TEAM       , STYLE_ADMIN_NORMAL  },
/* MSG_CTF_TEAM_SCORES                 */  { "Team <T> scores!"                                  , ARG_TEAM      , ARG_NONE       , STYLE_GAME_URGENT   },
/* MSG_CTF_TEAM_CAP                    */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_CTF_ENEMY_CAP                   */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_CTF_YOU_HAVE_FLAG               */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_CTF_YOU_DROPPED_FLAG            */  { "<W> dropped the <E> flag."                         , ARG_WARRIOR   , ARG_NONE       , STYLE_GAME_URGENT   },
/* MSG_CTF_PLAYER_TOOK_FLAG            */  { NULL                                                , ARG_WARRIOR   , ARG_NONE       , STYLE_NONE          },
/* MSG_CTF_FLAG_MINED                  */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_CNH_TEAM_GOT_OBJECTIVE          */  { "Team <T> has captured the <O>."                    , ARG_TEAM      , ARG_OBJECTIVE  , STYLE_NONE          },
/* MSG_HUNT_FLAGS_TO_NEXUS             */  { "<W> returned <N> flags to the Nexus."              , ARG_WARRIOR   , ARG_N          , STYLE_ADMIN_NORMAL  },
/* MSG_HUNT_POINTS                     */  { "<W> scored <N> points."                            , ARG_WARRIOR   , ARG_N          , STYLE_ADMIN_NORMAL  },
/* MSG_HUNT_FLAGS_DROPPED              */  { "<W> dropped <N> flags."                            , ARG_WARRIOR   , ARG_N          , STYLE_ADMIN_NORMAL  },
/* MSG_HUNT_FLAGS_LOST                 */  { "<W> lost <N> flags."                               , ARG_WARRIOR   , ARG_N          , STYLE_ADMIN_NORMAL  },
/* MSG_HUNT_FLAGS_FUMBLED              */  { "<W> fumbled <N> flags."                            , ARG_WARRIOR   , ARG_N          , STYLE_ADMIN_NORMAL  },
/* MSG_HUNT_FLAGS_MISPLACED            */  { "<W> misplaced <N> flags."                          , ARG_WARRIOR   , ARG_N          , STYLE_ADMIN_NORMAL  },
/* MSG_HUNT_YOUR_FLAGS                 */  { "<W> now has <N> flags."                            , ARG_N         , ARG_NONE       , STYLE_GAME_NORMAL   },
/* MSG_HUNT_YOU_DROPPED_FLAGS          */  { NULL                                                , ARG_N         , ARG_NONE       , STYLE_NONE          },
/* MSG_HUNT_YOUR_SCORE                 */  { "<W> brought in <N> flags for <M> points."          , ARG_N         , ARG_M          , STYLE_GAME_NORMAL   },
/* MSG_TIME_60                         */  { "Time remaining: 60 seconds."                       , ARG_NONE      , ARG_NONE       , STYLE_ADMIN_NORMAL  },
/* MSG_TIME_30                         */  { "Time remaining: 30 seconds."                       , ARG_NONE      , ARG_NONE       , STYLE_ADMIN_NORMAL  },
/* MSG_TIME_10                         */  { "Time remaining: 10 seconds."                       , ARG_NONE      , ARG_NONE       , STYLE_ADMIN_NORMAL  },
/* MSG_TURRET_DEPLOYED                 */  { "<W> deployed a remote turret."                     , ARG_WARRIOR   , ARG_M          , STYLE_NORMAL        },
/* MSG_STATION_DEPLOYED                */  { "<W> deployed a remote station."                    , ARG_WARRIOR   , ARG_M          , STYLE_NORMAL        },
/* MSG_SENSOR_DEPLOYED                 */  { "<W> deployed a remote sensor."                     , ARG_WARRIOR   , ARG_M          , STYLE_NORMAL        },
/* MSG_N_OF_M_TURRETS_DEPLOYED         */  { NULL                                                , ARG_N         , ARG_M          , STYLE_NONE          },
/* MSG_N_OF_M_STATIONS_DEPLOYED        */  { NULL                                                , ARG_N         , ARG_M          , STYLE_NONE          },
/* MSG_N_OF_M_SENSORS_DEPLOYED         */  { NULL                                                , ARG_N         , ARG_M          , STYLE_NONE          },
/* MSG_BARREL_DEPLOYED                 */  { "<W> changed a base turret barrel."                 , ARG_NONE      , ARG_NONE       , STYLE_NORMAL        },
/* MSG_TURRET_DEPLOY_TOO_CLOSE         */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_TURRET_DEPLOY_TOO_MANY_NEAR     */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_TURRET_DEPLOY_LIMIT_REACHED     */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_TURRET_DEPLOY_NO_ROOM           */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_TURRET_DEPLOY_BAD_SURFACE       */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_STATION_DEPLOY_BAD_SURFACE      */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_STATION_DEPLOY_TOO_STEEP        */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_STATION_DEPLOY_LIMIT_REACHED    */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_STATION_DEPLOY_TOO_CLOSE        */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_STATION_DEPLOY_NO_ROOM          */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_SENSOR_DEPLOY_BAD_SURFACE       */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_SENSOR_DEPLOY_TOO_STEEP         */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_SENSOR_DEPLOY_LIMIT_REACHED     */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_SENSOR_DEPLOY_TOO_CLOSE         */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_SENSOR_DEPLOY_NO_ROOM           */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_BARREL_DEPLOY_NO_TURRET         */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_BARREL_DEPLOY_TURRET_DOWN       */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_BARREL_DEPLOY_WRONG_TEAM        */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_DEPLOY_NEAR_REMOTE_STATION      */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_ACCESS_DENIED                   */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_STATION_DISABLED                */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_STATION_NOT_POWERED             */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_CANNOT_PILOT_ARMOR              */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_CANNOT_PILOT_PACK               */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_VEHICLE_LIMIT                   */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
/* MSG_VEHICLE_BLOCKED                 */  { NULL                                                , ARG_NONE      , ARG_NONE       , STYLE_NONE          },
};

//==============================================================================

log_mgr LogMgr;

static const char* GameTypeString[] =
{
    "#ERROR#", "CTF", "CnH", "TDM", "DM", "HUNT", 
};

static char s_TimeStamp[10];

//==============================================================================
//  FUNCTIONS
//==============================================================================

log_mgr::log_mgr( void )
{
    m_DoIRC    = FALSE;
    m_DoLog    = FALSE;
    m_pLogFile = NULL;
    m_pUBBFile = NULL;
    m_pTagFile = NULL;

    m_LogDir[0] = '.';
    m_LogDir[1] = '\0';
}

//==============================================================================

log_mgr::~log_mgr( void )
{
}

//==============================================================================

void log_mgr::SetIRCStatus( xbool Enabled )
{
    m_DoIRC = Enabled;
}

//==============================================================================

void log_mgr::SetLogStatus( xbool Enabled )
{
    if( !m_DoLog && Enabled )
    {
        // Fire it up.
        m_DoLog = Enabled;
        OpenLogFiles();
    }

    if( m_DoLog && !Enabled )
    {
        // Shut 'er down.
        m_DoLog = Enabled;
        CloseLogFiles();
    }
}

//==============================================================================

xbool log_mgr::GetIRCStatus( void )
{
    return( m_DoIRC );
}

//==============================================================================

xbool log_mgr::GetLogStatus( void )
{
    return( m_DoLog );
}

//==============================================================================

void log_mgr::ResetStrings( void )
{
    m_pIRCWrite = m_IRCBuffer;
    m_pTTYWrite = m_TTYBuffer;
    m_pLogWrite = m_LogBuffer;
    m_pUBBWrite = m_UBBBuffer;
}

//==============================================================================

xbool log_mgr::FetchArgValue( const msg& Msg, s16 Arg, s16& Value )
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
    if( Arg == ARG_WARRIOR )
    {
        Value = Msg.Target;
        return( TRUE );
    }
    /*
    else
    if( Arg == ARG_TEAM )
    {
        Value = Msg.Target;
        return( TRUE );
    }
    */
    else
    {
        return( FALSE );
    }
}

//==============================================================================

void log_mgr::AppendString( const char* pString )
{
    if( m_DoIRC )
    {
        AppendStringIRC( pString );
    }

    if( m_DoLog )
    {
        AppendStringUBB( pString );
        AppendStringLog( pString );
    }
}

//==============================================================================

void log_mgr::AppendStringIRC( const char* pString )
{
    const char* pRead = pString;

    while( *pRead )
        *m_pIRCWrite++ = *pRead++;
    *m_pIRCWrite = '\0';
}

//==============================================================================

void log_mgr::AppendStringUBB( const char* pString )
{
    const char* pRead = pString;

    while( *pRead )
        *m_pUBBWrite++ = *pRead++;
    *m_pUBBWrite = '\0';
}

//==============================================================================

void log_mgr::AppendStringLog( const char* pString )
{
    const char* pRead = pString;

    while( *pRead )
        *m_pLogWrite++ = *pRead++;
    *m_pLogWrite = '\0';
}

//==============================================================================

void log_mgr::UpdateTimeStamp( void )
{
    if( GameMgr.GameInProgress() )
    {
        s32 Seconds = (s32)(GameMgr.GetTimeRemaining() + 0.5f);
        s32 Minutes = Seconds / 60;
        Seconds    -= Minutes * 60;

        x_sprintf( s_TimeStamp, "[%02d:%02d] ", Minutes, Seconds );
    }
    else
    {
        x_strcpy( s_TimeStamp, "        " );
    }
}

//==============================================================================

static char* s_UBBColorSet[] =
{
    "",                 // 0 (normal) DEFAULT / COMMENTERY       PREFIX GAME
    "[color=#00FFFF]",  // 1 (cyan  ) STORM / WARRIOR 
    "[color=#FF80FF]",  // 2 (violet) INFERNO
    "[color=#00FF00]",  // 3 (green ) KILLER
    "[color=#FF8080]",  // 4 (l.red ) VICTIM / NEGATIVE          PREFIX GAME
    "[color=#FFFF00]",  // 5 (yellow) URGENT                     PREFIX COMMENT
    "[color=#A0A0A0]",  // 6 (l.grey) CHAT                       PREFIX CHAT
    "[color=#8080FF]",  // 7 (l.blue)                            PREFIX ADMIN
};

//------------------------------------------------------------------------------

static char* s_UBBCloseSet[] =
{
    "",
    "[/color]",
};

//------------------------------------------------------------------------------

static char* s_UBBOpenPrefix[] =
{
    s_UBBColorSet[ 0 ],          // STYLE_NORMAL        =  0,
    s_UBBColorSet[ 0 ],          // STYLE_NEGATIVE,
    s_UBBColorSet[ 4 ],          // STYLE_GAME_NORMAL,
    s_UBBColorSet[ 4 ],          // STYLE_GAME_URGENT,
    s_UBBColorSet[ 4 ],          // STYLE_GAME_POSITIVE,
    s_UBBColorSet[ 4 ],          // STYLE_GAME_NEGATIVE,
    s_UBBColorSet[ 7 ],          // STYLE_ADMIN_NORMAL,
    s_UBBColorSet[ 7 ],          // STYLE_ADMIN_URGENT,
    s_UBBColorSet[ 5 ],          // STYLE_COMMENTERY,
    s_UBBColorSet[ 6 ],          // STYLE_CHAT,
    s_UBBColorSet[ 6 ],          // STYLE_CHAT_STORM,
    s_UBBColorSet[ 6 ],          // STYLE_CHAT_INFERNO,
};

//------------------------------------------------------------------------------

static char* s_UBBClosePrefix[] =
{
    s_UBBCloseSet[ 0 ],          // STYLE_NORMAL        =  0,
    s_UBBCloseSet[ 0 ],          // STYLE_NEGATIVE,
    s_UBBCloseSet[ 1 ],          // STYLE_GAME_NORMAL,
    s_UBBCloseSet[ 1 ],          // STYLE_GAME_URGENT,
    s_UBBCloseSet[ 1 ],          // STYLE_GAME_POSITIVE,
    s_UBBCloseSet[ 1 ],          // STYLE_GAME_NEGATIVE,
    s_UBBCloseSet[ 1 ],          // STYLE_ADMIN_NORMAL,
    s_UBBCloseSet[ 1 ],          // STYLE_ADMIN_URGENT,
    s_UBBCloseSet[ 1 ],          // STYLE_COMMENTERY,
    s_UBBCloseSet[ 1 ],          // STYLE_CHAT,
    s_UBBCloseSet[ 1 ],          // STYLE_CHAT_STORM,
    s_UBBCloseSet[ 1 ],          // STYLE_CHAT_INFERNO,
};

//------------------------------------------------------------------------------

static char* s_UBBOpenGeneral[] =
{
    s_UBBColorSet[ 0 ],          // STYLE_NORMAL        =  0,
    s_UBBColorSet[ 4 ],          // STYLE_NEGATIVE,
    s_UBBColorSet[ 0 ],          // STYLE_GAME_NORMAL,
    s_UBBColorSet[ 5 ],          // STYLE_GAME_URGENT,
    s_UBBColorSet[ 0 ],          // STYLE_GAME_POSITIVE,
    s_UBBColorSet[ 0 ],          // STYLE_GAME_NEGATIVE,
    s_UBBColorSet[ 0 ],          // STYLE_ADMIN_NORMAL,
    s_UBBColorSet[ 5 ],          // STYLE_ADMIN_URGENT,
    s_UBBColorSet[ 0 ],          // STYLE_COMMENTERY,
    s_UBBColorSet[ 6 ],          // STYLE_CHAT,
    s_UBBColorSet[ 6 ],          // STYLE_CHAT_STORM,
    s_UBBColorSet[ 6 ],          // STYLE_CHAT_INFERNO,

    s_UBBColorSet[ 1 ],          // STYLE_WARRIOR,        
    s_UBBColorSet[ 1 ],          // STYLE_WARRIOR_STORM,  
    s_UBBColorSet[ 2 ],          // STYLE_WARRIOR_INFERNO,
    s_UBBColorSet[ 4 ],          // STYLE_WARRIOR_VICTIM, 
    s_UBBColorSet[ 3 ],          // STYLE_WARRIOR_KILLER, 
    s_UBBColorSet[ 1 ],          // STYLE_TEAM_STORM,  
    s_UBBColorSet[ 2 ],          // STYLE_TEAM_INFERNO,
};

//------------------------------------------------------------------------------

static char* s_UBBCloseGeneral[] = 
{
    s_UBBCloseSet[ 0 ],          // STYLE_NORMAL        =  0,
    s_UBBCloseSet[ 1 ],          // STYLE_NEGATIVE,
    s_UBBCloseSet[ 0 ],          // STYLE_GAME_NORMAL,
    s_UBBCloseSet[ 1 ],          // STYLE_GAME_URGENT,
    s_UBBCloseSet[ 0 ],          // STYLE_GAME_POSITIVE,
    s_UBBCloseSet[ 0 ],          // STYLE_GAME_NEGATIVE,
    s_UBBCloseSet[ 0 ],          // STYLE_ADMIN_NORMAL,
    s_UBBCloseSet[ 1 ],          // STYLE_ADMIN_URGENT,
    s_UBBCloseSet[ 0 ],          // STYLE_COMMENTERY,
    s_UBBCloseSet[ 1 ],          // STYLE_CHAT,
    s_UBBCloseSet[ 1 ],          // STYLE_CHAT_STORM,
    s_UBBCloseSet[ 1 ],          // STYLE_CHAT_INFERNO,
                    
    s_UBBCloseSet[ 1 ],          // STYLE_WARRIOR,        
    s_UBBCloseSet[ 1 ],          // STYLE_WARRIOR_STORM,  
    s_UBBCloseSet[ 1 ],          // STYLE_WARRIOR_INFERNO,
    s_UBBCloseSet[ 1 ],          // STYLE_WARRIOR_VICTIM, 
    s_UBBCloseSet[ 1 ],          // STYLE_WARRIOR_KILLER, 
    s_UBBCloseSet[ 1 ],          // STYLE_TEAM_STORM,  
    s_UBBCloseSet[ 1 ],          // STYLE_TEAM_INFERNO,
};                  

//------------------------------------------------------------------------------

static s32 s_UBBDepth = 0;

//------------------------------------------------------------------------------

void log_mgr::AppendFormatUBB( s32 Style, xbool Enter )
{
    if( Enter )
    {
        s_UBBDepth++;

        if( s_UBBDepth == 1 )
        {
            // Entering base style for first time.  Add time stamp.
            m_BaseStyle = Style;
            AppendStringUBB( s_UBBOpenPrefix [Style] );
            AppendStringUBB( s_TimeStamp );
            AppendStringUBB( s_UBBClosePrefix[Style] );
            AppendStringUBB( s_UBBOpenGeneral[Style] );
        }
        else
        {
            // Entering a local style.  Close base style, then open local.
            AppendStringUBB( s_UBBCloseGeneral[m_BaseStyle] );

            // Optimization.  Back out empty color code spans.  
            // [color=#8080FF][/color][color=#FFFF00]Blah Blah
            // -----------------------X
            if( (x_strncmp( (m_pUBBWrite - 23), "[color=#", 8 ) == 0) &&
                (x_strncmp( (m_pUBBWrite -  8), "[/color]", 8 ) == 0) )
            {
                m_pUBBWrite -= 23;
            }

            // Open local style.
            AppendStringUBB( s_UBBOpenGeneral[Style] );
        }
    }
    else
    {
        s_UBBDepth--;

        AppendStringUBB( s_UBBCloseGeneral[Style] );

        if( s_UBBDepth == 0 )
        {
            // That's the end.
            AppendString( "\n" );
        }
        else
        {
            // Re-enter the base style.
            AppendStringUBB( s_UBBOpenGeneral[m_BaseStyle] );
        }
    }
}

//==============================================================================

static char* s_IRCColorCode     = "\x03";
static char* s_IRCUnderlineCode = "\x1F";
static xbool s_IRCUnderline     = FALSE;
static s32   s_IRCDepth         = 0;

//------------------------------------------------------------------------------

static char* s_IRCBackground[] =
{
    ",01",                      // STYLE_NORMAL        =  0,
    ",01",                      // STYLE_NEGATIVE,
    ",05",                      // STYLE_GAME_NORMAL,
    ",05",                      // STYLE_GAME_URGENT,
    ",05",                      // STYLE_GAME_POSITIVE,
    ",05",                      // STYLE_GAME_NEGATIVE,
    ",02",                      // STYLE_ADMIN_NORMAL,
    ",02",                      // STYLE_ADMIN_URGENT,
    ",14",                      // STYLE_COMMENTERY,
    ",01",                      // STYLE_CHAT,
    ",01",                      // STYLE_CHAT_STORM,
    ",01",                      // STYLE_CHAT_INFERNO,
    "",                         // STYLE_WARRIOR,
    "",                         // STYLE_WARRIOR_STORM,
    "",                         // STYLE_WARRIOR_INFERNO,
    "",                         // STYLE_WARRIOR_VICTIM,
    "",                         // STYLE_WARRIOR_KILLER, 
    "",                         // STYLE_TEAM_STORM,
    "",                         // STYLE_TEAM_INFERNO,
    ",01",                      // STYLE_TABLE_LABEL,
    ",01",                      // STYLE_TABLE_SCORE,
};

//------------------------------------------------------------------------------

static char* s_IRCForeground[] =
{
    "15",                       // STYLE_NORMAL        =  0,
    "04",                       // STYLE_NEGATIVE,
    "15",                       // STYLE_GAME_NORMAL,
    "08",                       // STYLE_GAME_URGENT,
    "00",                       // STYLE_GAME_POSITIVE,
    "04",                       // STYLE_GAME_NEGATIVE,
    "08",                       // STYLE_ADMIN_NORMAL,
    "04",                       // STYLE_ADMIN_URGENT,
    "00",                       // STYLE_COMMENTERY,
    "11",                       // STYLE_CHAT,
    "11",                       // STYLE_CHAT_STORM,
    "09",                       // STYLE_CHAT_INFERNO,
    "11",                       // STYLE_WARRIOR,
    "11",                       // STYLE_WARRIOR_STORM,
    "09",                       // STYLE_WARRIOR_INFERNO,
    "04",                       // STYLE_WARRIOR_VICTIM,
    "11",                       // STYLE_WARRIOR_KILLER, 
    "11",                       // STYLE_TEAM_STORM,
    "09",                       // STYLE_TEAM_INFERNO,
    "00",                       // STYLE_TABLE_LABEL,
    "15",                       // STYLE_TABLE_SCORE,
};

//------------------------------------------------------------------------------

void log_mgr::AppendFormatIRC( s32 Style, xbool Enter )
{
    if( Enter )
    {
        s_IRCDepth++;

        if( s_IRCDepth == 1 )
        {
            m_BaseStyle = Style;
            AppendStringIRC( s_TimeStamp );
        }

        AppendStringIRC( s_IRCColorCode );
        AppendStringIRC( s_IRCForeground[    Style    ] );
        AppendStringIRC( s_IRCBackground[ m_BaseStyle ] );

        if( IN_RANGE( STYLE_WARRIOR, Style, STYLE_WARRIOR_KILLER ) )
        {
            s_IRCUnderline = TRUE;
            AppendStringIRC( s_IRCUnderlineCode );
        }

        if( s_IRCDepth == 1 )
        {
            AppendStringIRC( " " );
        }
    }
    else
    {
        s_IRCDepth--;

        if( s_IRCUnderline )
        {
            s_IRCUnderline = FALSE;
            AppendStringIRC( s_IRCUnderlineCode );
        }

        if( s_IRCDepth == 0 )
        {
            AppendStringIRC( " \n" );
        }
        else
        {
            AppendStringIRC( s_IRCColorCode );
            AppendStringIRC( s_IRCForeground[ m_BaseStyle ] );
            AppendStringIRC( s_IRCBackground[ m_BaseStyle ] );
        }
    }
}

//==============================================================================

static s32 s_LogDepth = 0;

//------------------------------------------------------------------------------

void log_mgr::AppendFormatLog( s32 Style, xbool Enter )
{
    if( Enter )
    {
        s_LogDepth++;

        if( s_LogDepth == 1 )
        {
            AppendStringLog( s_TimeStamp );
        }
    }
    else
    {
        s_LogDepth--;
    }
}

//==============================================================================

void log_mgr::AppendFormat( s32 Style, xbool Enter )
{
    if( m_DoIRC )
    {
        AppendFormatIRC( Style, Enter );
    }

    if( m_DoLog )
    {
        AppendFormatUBB( Style, Enter );
        AppendFormatLog( Style, Enter );
    }
}

//==============================================================================

void log_mgr::AppendField( s32 Style, const char* pRead )
{
    AppendFormat( Style, TRUE );
    AppendString( pRead );
    AppendFormat( Style, FALSE );
}

//==============================================================================

void log_mgr::LogMsg( const msg& OriginalMsg )
{   
    if( (LogTable[ OriginalMsg.Index ].String == NULL) ||
        (LogTable[ OriginalMsg.Index ].Style  == STYLE_NONE) )
    {
        return;
    }

          msg           Msg   = OriginalMsg;
    const game_score&   Score = GameMgr.GetScore();
    const char*         pRead;
          char*         pWrite;
          char          Buffer[256];
          char          Key;
          xbool         Error = FALSE;
          s16           Value;
          s32           Team = -1;
          s32           Style;

    pRead = LogTable[Msg.Index].String; 

    ResetStrings();
    UpdateTimeStamp();

    // Determine the "style".

    Style = LogTable[Msg.Index].Style;

    if( (Style == STYLE_CHAT) && (GameMgr.IsTeamBasedGame()) )
    {
        // Find a team based on available data.
        if( FetchArgValue( Msg, ARG_WARRIOR, Value ) )
        {
            Team = Score.Player[Value].Team;
        }
        else
        {
            ASSERT( FALSE );
        }

        Style = STYLE_CHAT_STORM + Team;
    }

    // Inject formatting for base style.
    AppendFormat( Style, TRUE );

    // Process rest of string.

    pWrite = Buffer;

    while( *pRead && !Error )
    {
        if( *pRead != '<' )
        {
            *pWrite++ = *pRead++;
            continue;
        }

        *pWrite = '\0';
        AppendString( Buffer );

        pRead++;            // Skip '<'.
        Key = *pRead++;     // Read key.
        pRead++;            // Skip '<'.

        switch( Key )
        {
        //--KILLER--------------------------------------------------------------
        case 'K':
            {
                if( !FetchArgValue( Msg, ARG_KILLER, Value ) )  goto BAIL_OUT;
                if( !(Score.Player[Value].IsInGame) )           goto BAIL_OUT;

                if( Score.IsTeamBased )
                    AppendFormat( STYLE_WARRIOR_STORM + Score.Player[Value].Team, TRUE );
                else
                    AppendFormat( STYLE_WARRIOR_KILLER, TRUE );

                AppendString( Score.Player[Value].NName );
                AppendFormat( STYLE_WARRIOR, FALSE );
                break;
            }

        //--VICTIM--------------------------------------------------------------
        case 'V':
            {
                if( !FetchArgValue( Msg, ARG_VICTIM, Value ) )  goto BAIL_OUT;
                if( !Score.Player[Value].IsInGame )             goto BAIL_OUT;

                if( Score.IsTeamBased )
                    AppendFormat( STYLE_WARRIOR_STORM + Score.Player[Value].Team, TRUE );
                else
                    AppendFormat( STYLE_WARRIOR_VICTIM, TRUE );

                AppendString( Score.Player[Value].NName );
                AppendFormat( STYLE_WARRIOR, FALSE );
                break;
            }

        //--WARRIOR-------------------------------------------------------------
        case 'W':
            {
                if( !FetchArgValue( Msg, ARG_WARRIOR, Value ) )  goto BAIL_OUT;
                if( !Score.Player[Value].IsConnected )           goto BAIL_OUT;

                if( Score.IsTeamBased )
                    AppendFormat( STYLE_WARRIOR_STORM + Score.Player[Value].Team, TRUE );
                else
                    AppendFormat( STYLE_WARRIOR, TRUE );

                AppendString( Score.Player[Value].NName );
                AppendFormat( STYLE_WARRIOR, FALSE );
                break;
            }

        //--INLINE--------------------------------------------------------------
        case 'I':
            {
                char NName[20];

                if( !FetchArgValue( Msg, ARG_INLINE, Value ) )  goto BAIL_OUT;
                MakeNarrowString( NName, MsgMgr.GetInlineName( Value ), 20 );

                AppendFormat( STYLE_WARRIOR, TRUE );
                AppendString( NName );
                AppendFormat( STYLE_WARRIOR, FALSE );
                break;
            }

        //--PICKUP--------------------------------------------------------------
        case 'P':
            {
                ASSERT( FALSE );
                break;
            }

        //--N and M-------------------------------------------------------------
        case 'N':
        case 'M':
            {
                if( !FetchArgValue( Msg, (Key == 'N') ? ARG_N : ARG_M, Value ) )
                    goto BAIL_OUT;

                AppendString( (const char*)xfs( "%d", Value ) );
                break;
            }

        //--TEAM----------------------------------------------------------------
        case 'T':
            {
                if( !Score.IsTeamBased )                        goto BAIL_OUT;
                if( !FetchArgValue( Msg, ARG_TEAM, Value ) )
                {
                    // The message wants a team, but its not explicitly there.
                    // Try to use the player's (if any) team.
                    if( FetchArgValue( Msg, ARG_WARRIOR, Value ) )
                    {
                        Value = Score.Player[Value].Team;
                    }
                }
                if( !IN_RANGE( 0, Value, 1 ) )  goto BAIL_OUT;

                AppendFormat( STYLE_TEAM_STORM + Value, TRUE );
                AppendString( Score.Team[Value].NName );
                AppendFormat( STYLE_TEAM_STORM, FALSE );
                break;
            }

        //--ALLIED TEAM---------------------------------------------------------
        case 'A':
            {
                if( !Score.IsTeamBased )                         goto BAIL_OUT;
                if( !FetchArgValue( Msg, ARG_WARRIOR, Value ) )  goto BAIL_OUT;
                if( !Score.Player[Value].IsInGame )              goto BAIL_OUT;
                Team = Score.Player[Value].Team;

                AppendFormat( STYLE_TEAM_STORM + Team, TRUE );
                AppendString( Score.Team[Team].NName );
                AppendFormat( STYLE_TEAM_STORM, FALSE );
                break;
            }

        //--ENEMY TEAM----------------------------------------------------------
        case 'E':
            {
                if( !Score.IsTeamBased )                         goto BAIL_OUT;
                if( !FetchArgValue( Msg, ARG_WARRIOR, Value ) )  goto BAIL_OUT;
                if( !Score.Player[Value].IsInGame )              goto BAIL_OUT;
                Team = 1 - Score.Player[Value].Team;

                AppendFormat( STYLE_TEAM_STORM + Team, TRUE );
                AppendString( Score.Team[Team].NName );
                AppendFormat( STYLE_TEAM_STORM, FALSE );
                break;
            }

        //--OBJECTIVE-----------------------------------------------------------
        case 'O':
            {
                ASSERT( FALSE );
                break;
            }

        //--SHIP (vehicle)------------------------------------------------------
        case 'S':
            {
                ASSERT( FALSE );
                break;
            }

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

                char* pText[] = { "his", "her", "himself", "herself" };
                s32   Index   = 0;

                if( !Score.Player[Value].IsMale )   Index += 1;
                if( Key == '2' )                    Index += 2;

                AppendString( pText[Index] );
                break;
            }
        }

        pWrite = Buffer;
    }

    *pWrite = '\0';
    AppendString( Buffer );
    AppendFormat( Style, FALSE );

    // Distribute.
    Distribute();

    // HACK HACK HACK
    /*
    if( Msg.Index == MSG_CTF_TEAM_SCORES )
        DumpScoreTable();
    */
    return;

BAIL_OUT:;

    ASSERT( FALSE );
}

//==============================================================================

void log_mgr::Distribute( void )
{
    if( m_DoIRC )
    {
        g_ChatManager.Message( m_IRCBuffer );
    }

    if( m_DoLog )
    {
        if( m_pLogFile )
        {
            x_fprintf( m_pLogFile, m_LogBuffer );
        }

        if( m_pUBBFile )
        {
            x_fprintf( m_pUBBFile, m_UBBBuffer );
        }
    }
}

//==============================================================================

void log_mgr::LogEndOfGame( s32 Cause )
{
    LogFullScore();

    if( m_DoLog )
        CloseLogFiles();
}

//==============================================================================

void log_mgr::LogStartOfGame( void )
{
    {
        tm*     pTime;
        time_t  Time;

        time( &Time );
        pTime = localtime( &Time );

        m_StartYear = pTime->tm_year+1900;
        m_StartMon  = pTime->tm_mon + 1;
        m_StartDay  = pTime->tm_mday;
        m_StartHour = ((pTime->tm_hour + 11) % 12) + 1;
        m_StartMin  = pTime->tm_min;
        m_StartSec  = pTime->tm_sec;
    }

    if( m_DoLog )
        OpenLogFiles();

    // Reset the timers.
    m_TimeToFullScore  = x_frand( 60.0f, 120.0f );
    m_TimeToSmallScore = x_frand( 30.0f,  90.0f );
    m_TimeToStatus     = x_frand( 30.0f, 120.0f );
    m_TimeToComment    = x_frand( 30.0f, 120.0f );

    // Tag all players in game at the moment?
    if( TagPlayers && m_pTagFile )
    {
        const game_score& Score = GameMgr.GetScore();

        for( s32 i = 0; i < 32; i++ )
        {
            if( Score.Player[i].IsConnected )
                TagPlayer( i, m_pTagFile );
        }
    }
}

//==============================================================================

void log_mgr::OpenLogFiles( void )
{
    if( (m_pLogFile && m_pUBBFile && (m_pTagFile || !TagPlayers)) || 
        (!GameMgr.GameInProgress()) )
        return;

    //
    // Make sure there are no more than 250 log files.  
    // Delete oldest files if needed.
    //

    s32 Count = 0;

#ifdef TARGET_PC
    do
    {
        _finddata_t File;
        long        Handle;
        time_t      Oldest;
        char        Name[ 256 ];

        xfs FileSpec( "%s\\Log*.txt", m_LogDir );
        Handle = _findfirst( FileSpec, &File );

        if( Handle != -1L )
        {
            Count  = 0;
            Oldest = File.time_write;
            x_strcpy( Name, File.name );

            // At least one file out there.
            do
            {
                Count += 1;
                if( File.time_write < Oldest )
                {
                    Oldest = File.time_write;
                    x_strcpy( Name, File.name );
                }
            } while( _findnext( Handle, &File ) == 0 );
            _findclose( Handle );

            if( Count >= 250 )
            {
                // Delete the oldest file.
                xfs Delete( "del \"%s\\%s\" > nul", m_LogDir, Name );
                system( (const char*)Delete );
            }
        }
    }
    while( Count > 2500 );
#endif

#ifdef TARGET_LINUX
    do
    {
        DIR*    pHandle;
        time_t  Oldest;
        char    OldestFileName[256] = "";

        pHandle = opendir( m_LogDir );

        if( pHandle )
        {
            Count = 0;

            while( 1 )
            {
                dirent*     pFile;
                struct stat filestat;

                pFile = readdir( pHandle );

                if( !pFile )
                    break;

                if( (x_strncmp( pFile->d_name, "Log",3 ) == 0) &&
                    (x_strstr ( pFile->d_name, ".txt" ) ) )
                {
                    if( stat( xfs( "%s/%s", m_LogDir, pFile->d_name ), &filestat ) )
                    {
                        if( filestat.st_mtime > Oldest )
                        {
                            Oldest = filestat.st_mtime;
                            x_strcpy( OldestFileName, pFile->d_name );
                        }
                    }
                    Count++;
                }
            }
            closedir( pHandle );

            if( Count >= 250 )
            {
                unlink((const char*)xfs("%s/%s",m_LogDir,OldestFileName));
            }
        }
    }
    while( Count > 250 );
#endif

    //
    // Build the file names.
    //

    xstring ServerName;
    xstring FileName;
    xstring Mission    = xstring(StringMgr( "MissionName", fegl.pMissionDef->DisplayNameString ));
    char    BadChars[] = "\\/:*?\"<>";
    s32     i;
    char    Buffer[ 32 ];

    const game_score& Score = GameMgr.GetScore();

    // Start with the server name.
    MakeNarrowString( Buffer, fegl.ServerSettings.ServerName );
    ServerName = Buffer;

    // Remove any bad characters from the file name.
    if( (ServerName[0] == '-') && (ServerName[1] == '>') )
        ServerName.Delete( 0, 2 );
    for( i = 0; i < ServerName.GetLength(); i++ )
    {
        if( (x_strchr( BadChars, ServerName[i] )) || 
            (ServerName[i] <  0x20) || 
            (ServerName[i] >= 0x80) )
        {
            ServerName.Delete( i );
            i -= 1;
        }
    }

    // Remove leading and trailing spaces from server name.
    while( ServerName[0] == ' ' )
        ServerName.Delete(0);
    while( ServerName[ ServerName.GetLength()-1 ] == ' ' )
        ServerName.Delete( ServerName.GetLength()-1 );

    // Assemble the file name.
    FileName.Format( "%s/Log -- %s -- %4d-%02d-%02d (%02d.%02d.%02d) -- %s-%s",
                     m_LogDir, (const char*)ServerName,
                     m_StartYear, m_StartMon, m_StartDay,
                     m_StartHour, m_StartMin, m_StartSec,
                     GameTypeString[ Score.GameType ],
                     (const char*)Mission );

    if( m_pLogFile == NULL )
    {
        m_pLogFile = x_fopen( xfs( "%s.log.txt", (const char*)FileName ), "wt" );
    }

    if( m_pUBBFile == NULL )
    {
        m_pUBBFile = x_fopen( xfs( "%s.ubb.txt", (const char*)FileName ), "wt" );
    }

    if( (m_pTagFile == NULL) && TagPlayers )
    {
        m_pTagFile = x_fopen( xfs( "%s.tag.txt", (const char*)FileName ), "wt" );
    }

    m_DoLog = (m_pLogFile && m_pUBBFile && (m_pTagFile || !TagPlayers) );
}

//==============================================================================

void log_mgr::CloseLogFiles( void )
{
    if( m_pLogFile )
    {
        x_fclose( m_pLogFile );
        m_pLogFile = NULL;
    }

    if( m_pUBBFile )
    {
        x_fclose( m_pUBBFile );
        m_pUBBFile = NULL;
    }

    if( m_pTagFile )
    {
        x_fclose( m_pTagFile );
        m_pTagFile = NULL;
    }
}

//==============================================================================

void log_mgr::SetLogDir( const char* pDir )
{
    if( pDir == NULL )
        return;

    X_FILE* pFile;
    s32     i;
    s32     Len = x_strlen( pDir );
    char    Scratch[ X_MAX_DIR ];

    x_strcpy( Scratch, pDir );


    // Attempt to open a test file using the directory.
#if defined(TARGET_LINUX)
    // Convert all '\' to '/'.
    for( i = 0; i < Len; i++ )
        if( Scratch[i] == '\\' )
            Scratch[i] =  '/';

    // Remove any trailing '\'.
    if( Scratch[Len-1] == '/' )
    {
        Len          -= 1;
        Scratch[Len]  = '\0';
    }
    xfs  TestFile( "%s/TestFile.txt", Scratch );
#else
    // Convert all '/' to '\'.
    for( i = 0; i < Len; i++ )
        if( Scratch[i] == '/' )
            Scratch[i] =  '\\';

    // Remove any trailing '\'.
    if( Scratch[Len-1] == '\\' )
    {
        Len          -= 1;
        Scratch[Len]  = '\0';
    }

    xfs  TestFile( "%s\\TestFile.txt", Scratch );
#endif

    pFile = x_fopen( TestFile, "wt" );
    if( pFile )
    {
        x_fclose( pFile );
#if defined(TARGET_LINUX)
        unlink((const char*)TestFile);
#else
        xfs  Delete  ( "del \"%s\"", (const char*)TestFile );
        system( (const char*)Delete );
#endif
        x_strcpy( m_LogDir, Scratch );
    }    
}

//==============================================================================

const char* log_mgr::GetLogDir( void )
{
    return( m_LogDir );
}

//==============================================================================

void log_mgr::AdvanceLogic( f32 DeltaTime )
{
    m_TimeToFullScore  -= DeltaTime;
    m_TimeToSmallScore -= DeltaTime;
    m_TimeToStatus     -= DeltaTime;
    m_TimeToComment    -= DeltaTime;

    if( m_TimeToFullScore  < 0.0f )     LogFullScore();
    if( m_TimeToSmallScore < 0.0f )     LogSmallScore();
    if( m_TimeToStatus     < 0.0f )     LogStatus();    
    if( m_TimeToComment    < 0.0f )     LogComment();
}

//==============================================================================

struct score
{
    char    Name[16];
    s32     Score;
    s32     Kills;
    s32     TKs;
};

s32 SortFn( const void* p1, const void* p2 )
{
    score* pNode1 = (score*)p1;
    score* pNode2 = (score*)p2;

    if( pNode1->Score > pNode2->Score )        return( -1 );
    if( pNode1->Score < pNode2->Score )        return(  1 );
    if( pNode1->Kills > pNode2->Kills )        return( -1 );
    if( pNode1->Kills < pNode2->Kills )        return(  1 );
    if( pNode1->TKs   > pNode2->TKs   )        return(  1 ); // REVERSE
    if( pNode1->TKs   < pNode2->TKs   )        return( -1 ); // REVERSE

    return( x_strcmp( pNode1->Name, pNode2->Name ) );
}

void AddDots( char* pText )
{
    if( !pText )      return;
    if( !pText[0] )   return;

    for( s32 i = 1; pText[i]; i++ )
    {
        if( (pText[i-1] == ' ') && 
            (pText[i  ] == ' ') && 
            (pText[i+1] == ' ') )
        {
            pText[i] = '.';
        }

        if( (pText[i-1] == '.') && 
            (pText[i  ] == ' ') && 
            (pText[i+1] == ' ') )
        {
            pText[i] = '.';
        }
    }
}

//==============================================================================

void log_mgr::DumpTeamScore( void )
{
    const game_score& Score = GameMgr.GetScore();
    s32               i, j, m, T;
    s32               Size       [2]     = { 0, 0 };
    score             List       [2][32];  // ... for each player.
    score             Team       [2];      // ... for team score.
    score             Summary    [2];      // ... for team summary.
    char              PlayerLine [2][32][36];
    char              ScoreLine  [2][36];
    char              SummaryLine[2][36];
    char              Separator[] = "--------------------------------   --------------------------------";
    char              Header[]    = "Warrior Name     Score Kills TKs   Warrior Name     Score Kills TKs";

    //
    // PlayerLine[0-1][0-31] is for the players on the teams.
    // Looks like this: <W> " .......... <Score> <Kills> <TKs>"
    //
    // ScoreLine[0-1] is for the team score on the top.
    // Looks like this: <T> " .......... <Score>              "
    //
    // SummaryLine[0-1] is for the team summary line.
    // Looks like this: "Team summary .. <Score> <Kills> <TKs>"
    //

    Summary[0].Score = 0;    Summary[1].Score = 0;
    Summary[0].Kills = 0;    Summary[1].Kills = 0;
    Summary[0].TKs   = 0;    Summary[1].TKs   = 0;

    // Make the lists.
    for( i = 0; i < 32; i++ )
    {
        if( Score.Player[i].IsInGame )
        {
            T = Score.Player[i].Team;
            j = Size[T];
            List[T][j].Score = Score.Player[i].Score;
            List[T][j].Kills = Score.Player[i].Kills;
            List[T][j].TKs   = Score.Player[i].TKs;
            x_strncpy( List[T][j].Name, Score.Player[i].NName, 15 );
            List[T][j].Name[15] = '\0';
            Size[T]++;

            Summary[T].Score += List[T][j].Score;
            Summary[T].Kills += List[T][j].Kills;
            Summary[T].TKs   += List[T][j].TKs;
        }
    }

    // Max team size.
    m = MAX( Size[0], Size[1] );
    if( m == 0 )
        return;

    // Sort the lists.
    if( Size[0] )    x_qsort( List[0], Size[0], sizeof(score), SortFn );
    if( Size[1] )    x_qsort( List[1], Size[1], sizeof(score), SortFn );

    // For each player, build his "line".
    for( T = 0; T < 2;       T++ )
    for( i = 0; i < Size[T]; i++ )
    {
        x_sprintf( PlayerLine[T][i], "%*d %5d %3d", 
                                     22 - x_strlen( List[T][i].Name ),
                                     List[T][i].Score,
                                     List[T][i].Kills,
                                     List[T][i].TKs );
        AddDots( PlayerLine[T][i] );
    }

    // Build the team score lines.
    for( T = 0; T < 2; T++ )
    {
        x_strncpy( Team[T].Name, Score.Team[T].NName, 15 );
        Team[T].Name[15] = '\0';
        Team[T].Score    = Score.Team[T].Score;

        x_sprintf( ScoreLine[T], "%*d", 
                                 17 - x_strlen( Team[T].Name ),
                                 Team[T].Score );
        AddDots( ScoreLine[T] );
    }

    // Build the team summary lines.
    for( T = 0; T < 2; T++ )
    {
        x_sprintf( SummaryLine[T], "Team totals .....%5d %5d %3d",
                                   Summary[T].Score, 
                                   Summary[T].Kills, 
                                   Summary[T].TKs );
        AddDots( SummaryLine[T] );
    }

    // Time to dump the data.

    //
    // IRC
    //
    if( m_DoIRC )
    {
        // Build the "horizontal rule" line.
        m_pIRCWrite = m_IRCBuffer;
        AppendFormatIRC( STYLE_TABLE_LABEL, TRUE );
        AppendStringIRC( Separator );
        AppendFormatIRC( STYLE_TABLE_LABEL, FALSE );

        // Save the horizontal rule.
        xstring HR( m_IRCBuffer );

        // Print HR.
        g_ChatManager.Message( (const char*)HR );

        // Print team's name and score.
        {
            m_pIRCWrite = m_IRCBuffer;
            AppendFormatIRC( STYLE_TABLE_SCORE, TRUE );

            for( T = 0; T < 2; T++ )
            {
                AppendFormatIRC( STYLE_TEAM_STORM+T, TRUE );
                AppendStringIRC( "Team " );
                AppendStringIRC( Team[T].Name );
                AppendFormatIRC( STYLE_TEAM_STORM+T, FALSE );
                AppendStringIRC( ScoreLine[T] );
                AppendStringIRC( "          " );

                if( T == 0 )
                    AppendStringIRC( "   " );
            }

            AppendFormatIRC( STYLE_TABLE_SCORE, FALSE );
            g_ChatManager.Message( m_IRCBuffer );
        }

        // Print HR.
        g_ChatManager.Message( (const char*)HR );

        // Print the column header line.
        m_pIRCWrite = m_IRCBuffer;
        AppendFormatIRC( STYLE_TABLE_LABEL, TRUE );
        AppendStringIRC( Header );
        AppendFormatIRC( STYLE_TABLE_LABEL, FALSE );
        g_ChatManager.Message( m_IRCBuffer );    

        // Print each warrior.
        for( i = 0; i < m; i++ )
        {
            m_pIRCWrite = m_IRCBuffer;
            AppendFormatIRC( STYLE_TABLE_SCORE, TRUE );

            for( T = 0; T < 2; T++ )
            {
                if( i < Size[T] )
                {
                    AppendFormatIRC( STYLE_WARRIOR_STORM+T, TRUE );
                    AppendStringIRC( List[T][i].Name );
                    AppendFormatIRC( STYLE_WARRIOR_STORM+T, FALSE );
                    AppendStringIRC( PlayerLine[T][i] );
                }
                else
                {
                    AppendStringIRC( "                                " );
                }

                if( T == 0 )
                    AppendStringIRC( "   " );
            }

            AppendFormatIRC( STYLE_TABLE_SCORE, FALSE );
            g_ChatManager.Message( m_IRCBuffer );    
        }

        // Print HR.
        g_ChatManager.Message( (const char*)HR );

        // Print summary line.
        {
            m_pIRCWrite = m_IRCBuffer;
            AppendFormatIRC( STYLE_TABLE_SCORE, TRUE );

            AppendStringIRC( SummaryLine[0] );
            AppendStringIRC( "   " );
            AppendStringIRC( SummaryLine[1] );

            AppendFormatIRC( STYLE_TABLE_SCORE, FALSE );
            g_ChatManager.Message( m_IRCBuffer );
        }

        // Print HR.
        g_ChatManager.Message( (const char*)HR );
    }

    //
    // LOG
    //
    if( m_DoLog && m_pLogFile )
    {
        x_fprintf( m_pLogFile, "\n%s\n", Separator );
        x_fprintf( m_pLogFile, "Team %s%s             Team %s%s\n",
                               Team[0].Name, ScoreLine[0], 
                               Team[1].Name, ScoreLine[1] );
        x_fprintf( m_pLogFile, "%s\n", Separator );
        x_fprintf( m_pLogFile, "%s\n", Header );

        for( i = 0; i < m; i++ )
        {
            for( T = 0; T < 2; T++ )
            {
                if( i < Size[T] )
                {
                    x_fprintf( m_pLogFile, "%s%s",
                                           List[T][i].Name,
                                           PlayerLine[T][i] );
                }
                else
                {
                    x_fprintf( m_pLogFile, "                                " );
                }

                if( T == 0 )
                    x_fprintf( m_pLogFile, "   " );
            }

            x_fprintf( m_pLogFile, "\n" );
        }

        x_fprintf( m_pLogFile, "%s\n", Separator );
        x_fprintf( m_pLogFile, "%s   %s\n",
                               SummaryLine[0], 
                               SummaryLine[1] );
        x_fprintf( m_pLogFile, "%s\n\n", Separator );
    }

    //
    // UBB
    //
    if( m_DoLog && m_pUBBFile )
    {
        Separator[33] = '|';
        x_fprintf( m_pUBBFile, "[code]Team %s%s           | Team %s%s\n",
                               Team[0].Name, ScoreLine[0], 
                               Team[1].Name, ScoreLine[1] );

        x_fprintf( m_pUBBFile, "[/code][code]%s\n[/code][code]", Header );

        for( i = 0; i < m; i++ )
        {
            for( T = 0; T < 2; T++ )
            {
                if( i < Size[T] )
                {
                    x_fprintf( m_pUBBFile, "%s%s",
                                           List[T][i].Name,
                                           PlayerLine[T][i] );
                }
                else
                {
                    x_fprintf( m_pUBBFile, "                                " );
                }

                if( T == 0 )
                    x_fprintf( m_pUBBFile, " | " );
            }

            x_fprintf( m_pUBBFile, "\n" );
        }

        x_fprintf( m_pUBBFile, "[/code][code]%s | %s\n",
                               SummaryLine[0], 
                               SummaryLine[1] );
        x_fprintf( m_pUBBFile, "[/code]\n", Separator );
    }
}

//==============================================================================

void log_mgr::DumpNonTeamScore( void )
{
}

//==============================================================================

void log_mgr::LogFullScore( void )
{
    const game_score& Score = GameMgr.GetScore();

    m_TimeToFullScore = x_frand( 60.0f, 120.0f );
    if( (GameMgr.GetTimeRemaining() < 30.0f) && (GameMgr.GameInProgress()) )
        return;

    if( !m_DoIRC && !m_DoLog )
        return;

    UpdateTimeStamp();

    if( Score.IsTeamBased )
        DumpTeamScore();
    else
        DumpNonTeamScore();
}

//==============================================================================

void log_mgr::LogSmallScore( void )
{
    const game_score& Score = GameMgr.GetScore();

    m_TimeToSmallScore = x_frand( 30.0f, 90.0f );
    if( GameMgr.GetTimeRemaining() < 15.0f )
        return;

    if( !Score.IsTeamBased )
        return;

    ResetStrings();
    UpdateTimeStamp();

    // Scores -- .POW:123 -- <PGP>:123 -- Avalon

    char MapName[64];
    MakeNarrowString( MapName, StringMgr( "MissionName", fegl.pMissionDef->DisplayNameString ), 64 );
    
    AppendFormat( STYLE_COMMENTERY, TRUE );
    AppendString( "Scores -- " );
    AppendField ( STYLE_TEAM_STORM, Score.Team[0].NName );
    AppendString( xfs( ":%d -- ", Score.Team[0].Score ) );
    AppendField ( STYLE_TEAM_INFERNO, Score.Team[1].NName );
    AppendString( xfs( ":%d -- %s", Score.Team[1].Score, MapName ) );
    AppendFormat( STYLE_COMMENTERY, FALSE );

    Distribute();
}

//==============================================================================

void log_mgr::LogStatus( void )
{
    const game_score& Score = GameMgr.GetScore();

    m_TimeToStatus = x_frand( 30.0f, 120.0f );
    if( GameMgr.GetTimeRemaining() < 15.0f )
        return;

    ResetStrings();
    UpdateTimeStamp();

    if( Score.GameType == GAME_CTF )
    {
        s32 StormFlag;
        s32 InfernoFlag;
        vector3 Dummy;

        pGameLogic->GetFlagStatus( 0x01, StormFlag,   Dummy );
        pGameLogic->GetFlagStatus( 0x02, InfernoFlag, Dummy );

        if( (StormFlag >= 0) && (InfernoFlag >= 0) )
        {
            AppendFormat( STYLE_COMMENTERY, TRUE );
            if( x_rand() & 0x01 )
            {
                AppendField ( STYLE_WARRIOR_STORM, Score.Player[InfernoFlag].NName );
                AppendString( " and " );
                AppendField ( STYLE_WARRIOR_INFERNO, Score.Player[StormFlag].NName );
                AppendString( " have the flags." );
            }
            else
            {
                AppendField ( STYLE_WARRIOR_STORM, Score.Player[InfernoFlag].NName );
                AppendString( " has the " );
                AppendField ( STYLE_TEAM_INFERNO, Score.Team[1].NName );
                AppendString( " flag, and " );
                AppendField ( STYLE_WARRIOR_INFERNO, Score.Player[StormFlag].NName );
                AppendString( " has the " );
                AppendField ( STYLE_TEAM_STORM, Score.Team[0].NName );
                AppendString( " flag." );
            }
            AppendFormat( STYLE_COMMENTERY, FALSE );
            goto shortcut;
        }

        if( (StormFlag == -2) && (InfernoFlag == -2) )
        {
            AppendFormat( STYLE_COMMENTERY, TRUE );
            AppendString( "Both team's flags are secure." );
            AppendFormat( STYLE_COMMENTERY, FALSE );
            goto shortcut;
        }

        if( (StormFlag == -1) && (InfernoFlag == -1) )
        {
            AppendFormat( STYLE_COMMENTERY, TRUE );
            AppendString( "Both team's flags are loose in the field!" );
            AppendFormat( STYLE_COMMENTERY, FALSE );
            goto shortcut;
        }

        if( StormFlag >= 0 )
        {
            AppendFormat( STYLE_COMMENTERY, TRUE );
            AppendField ( STYLE_WARRIOR_INFERNO, Score.Player[StormFlag].NName );
            AppendString( " has the " );
            AppendField ( STYLE_TEAM_STORM, Score.Team[0].NName );
            AppendString( " flag." );

            if( InfernoFlag == -1 )
            {
                AppendString( "  The " );
                AppendField ( STYLE_TEAM_INFERNO, Score.Team[1].NName );
                AppendString( " flag is afield." );
            }

            AppendFormat( STYLE_COMMENTERY, FALSE );
            goto shortcut;
        }

        if( InfernoFlag >= 0 )
        {
            AppendFormat( STYLE_COMMENTERY, TRUE );
            AppendField ( STYLE_WARRIOR_STORM, Score.Player[InfernoFlag].NName );
            AppendString( " has the " );
            AppendField ( STYLE_TEAM_INFERNO, Score.Team[1].NName );
            AppendString( " flag." );

            if( StormFlag == -1 )
            {
                AppendString( "  The " );
                AppendField ( STYLE_TEAM_STORM, Score.Team[0].NName );
                AppendString( " flag is afield." );
            }

            AppendFormat( STYLE_COMMENTERY, FALSE );
            goto shortcut;
        }
    }

shortcut:
    Distribute();
}

//==============================================================================

void log_mgr::LogComment( void )
{
    m_TimeToComment = x_frand( 30.0f, 120.0f );
    if( GameMgr.GetTimeRemaining() < 15.0f )
        return;

    ResetStrings();
    UpdateTimeStamp();
}

//==============================================================================

void log_mgr::LogDisconnect( s32 PlayerIndex )
{
    const game_score& Score = GameMgr.GetScore();
    
    xfs XFS( " has disconnected.  Score:%d  Kills:%d", 
             Score.Player[PlayerIndex].Score,
             Score.Player[PlayerIndex].Kills );

    ResetStrings();
    UpdateTimeStamp();
    AppendFormat( STYLE_ADMIN_NORMAL, TRUE );
    AppendField ( STYLE_WARRIOR, Score.Player[PlayerIndex].NName );
    AppendString( (const char*)XFS );
    AppendFormat( STYLE_ADMIN_NORMAL, FALSE );
    Distribute();
}

//==============================================================================

void log_mgr::LogConnect( s32 PlayerIndex, xbool Reconnect )
{
    const game_score& Score = GameMgr.GetScore();
 
    ResetStrings();
    UpdateTimeStamp();
    AppendFormat( STYLE_ADMIN_NORMAL, TRUE );
    AppendField ( STYLE_WARRIOR, Score.Player[PlayerIndex].NName );

    if( Reconnect )
    {   
        xfs XFS( " has RECONNECTED.  Score:%d  Kills:%d", 
                 Score.Player[PlayerIndex].Score,
                 Score.Player[PlayerIndex].Kills );
        AppendString( (const char*)XFS );
    }
    else
    {
        AppendString( " has connected." );
    }

    AppendFormat( STYLE_ADMIN_NORMAL, FALSE );
    Distribute();

    // Tag this player and his machine?
    if( TagPlayers )
    {
        X_FILE* pFile = x_fopen( xfs( "%s/TagLog.txt", m_LogDir ), "a+t" );
        if( pFile )
        {
            TagPlayer( PlayerIndex, pFile );
            x_fclose( pFile );
        }
    }

    if( m_pTagFile )
    {
        TagPlayer( PlayerIndex, m_pTagFile );
    }
}

//==============================================================================

void log_mgr::TagPlayer( s32 PlayerIndex, X_FILE* pFile )
{
    const game_score& Score = GameMgr.GetScore();

    if( Score.Player[PlayerIndex].Name[0] == 0x90 )
        return;

    if( (Score.Player[PlayerIndex].NName[0] == 'A' ) && 
        (Score.Player[PlayerIndex].NName[1] == 'C' ) && 
        (Score.Player[PlayerIndex].NName[2] == 'I' ) && 
        (Score.Player[PlayerIndex].NName[3] == 'D' ) && 
        (Score.Player[PlayerIndex].NName[4] == '\0') )
        return;

    u32 IP = ENDIAN_SWAP_32( Score.Player[PlayerIndex].IPAddr );
    xfs XFS( "%12d\t\"%03d.%03d.%03d.%03d\"\t%d\t\"'%s\"\n",
             Score.Player[PlayerIndex].NetID, 
             (IP & 0xFF000000) >> 24,
             (IP & 0x00FF0000) >> 16,
             (IP & 0x0000FF00) >>  8,
             (IP & 0x000000FF) >>  0,
             Score.Player[PlayerIndex].IsSplit,
             Score.Player[PlayerIndex].NName );
    x_fprintf( pFile, "%s", (const char*)XFS );
    x_fflush ( pFile );
}

//==============================================================================
