//==============================================================================
//
//  ServerVersion.hpp
//
//==============================================================================

#ifndef SERVERVERSION_HPP
#define SERVERVERSION_HPP

//==============================================================================
//  COMMENTS:
//==============================================================================
//
//  VERSION USER    WHAT CHANGED
//  1001    SB      move manager move structure now includes taunt button
//  1002    SB      added character and armor type to player update info
//  1003    AT      added Monkeymode to move structure
//  1004    DMT     added object::id structure (removed stand alone Seq)
//  1005    SB      added NextWeapon to move structure
//  1006    AT      shrunk update/life packet overhead
//  1007    JN      Removed Matrix4's from linear projectiles
//  1008    AT      Increased num players to 24, modified partobject create
//  1009    SB      Added skin texture to player/corpse network data
//  1010    AT      Added priorities to OnProvideUpdate
//  1011    SB      Added health zero bit check on player net data
//  1012    SB      Added sending of anim index for certain player states
//  1013    AT      Added WorldPosCM and UnitVector to bitstream.  Changed
//                  linear class creation
//  1014    AT      Server man is created after mission load
//  1015    SB      Tuned player net data.
//  1016    AT      Removed seq from create/destroy data
//  1017    SB      Hooked up suicide
//  1018    JN      Needed extra bit for grenade launcher (thrown vs. launched)
//  1019    AT      Changed text/audio being sent to clients from server
//  1020    SB      Changed way player health is being sent across net
//  1021    DMT     Added bit for "player glow" when carrying flag
//  1022    AT      Changed speed range on weapons
//  1023    JN      Needed more info sent for explosion effects and such
//  1024    SB      Tweaked player netdata to be a tad smaller
//  1025    DMT     Added support for pop up message over the net
//  1026    SB      More player net data bit tweaks!
//  1027    AT      Added blowup wiggle to player
//  1028    AT      Server chooses names and new invenstation states
//  1029    AT      Added energy to updates
//  1030    SB      Added player free look yaw to net data
//  1031    JN      Blaster no longer provides or receives any data updates
//  1032    SB      Added phase1 of vehicle network data...
//  1033    DMT     Changed data for arcing class
//  1034    SB      Phase2 of vehicle network data...
//  1035    AT      Changed login packet for 2-player
//  1036    SB      Phase3 of vehicle network data - got client/server sync working.
//  1037    SB      Fixed vehicle network data crash (had to add sending of vehicle object ID)
//  1038    SB      Finally finished vehicle/player network sync data
//  1039    SB      Added sending of inventory loadout across the net
//  1040    AT      Changed login code for warrior select
//  1041    SB      Added heat signature to player net data
//  1042    SB      Added voice type to player init net data
//  1043    SB      Added new corpse init info and new pickup types
//  1044    DMT     Restructured the GameMgr.
//  1045    SB      Tightened up player ammo net data. Also added screen flash and sound net data
//  1046    SB      Added new keys to player move struc
//  1047    SB      Added new WEAPON_TYPE_NONE which increase range of weapon types sent across net
//  1048    SB      New inventory data
//  1049    SB      New player shield and pack net data
//  1050    SB      Added vehicle type to loadout
//  1051    SB      Satchel charge now has states that are sent across the net
//  1052    SB      Players weapon target id now goes to ghost players also
//  1053    AT      Changed Type in life pack to U32 / 6bits 0-63
//  1054    DMT     Changed ATTR bits; added ELFABLE, REPAIRABLE, LABELED, HOT.
//  1055    SB      Added new player initialization data + TargetingLaserKey to move
//  1056    DMT     Changed startup and shutdown sequence.
//  1057    AT/DMT  Change client server synchronization stuff.  A lot.
//  1058    DMT     Added game type to mission request.
//  1059    DMT     Added glow radius and flag count to player object.
//  1060    JN      Added "type" parameter to grenade initialization (on Aug. 1)
//  1061    SB      Added vehicle weapon data
//  1062    SB      Added inventory loadout to player init data
//  1063    DMT     Added turrets.
//  1064    SB      Changed player health back to a f32 so repair gun works
//  1065    DMT     Reduced bits send across wire by GameMgr.  Added Time, too.
//  1066    SB      Added armed bit to player net data
//  1067    JN      Added bits in player object for missile lock states
//  1068    SB      Made more player dirty bits available
//  1069    SB      Added new satchel charge state
//  1070    AT      Changes to disconnect, connect, and sync
//  1071    JN      Needed extra bit on player for missile lock, and changed missile dirty bits
//  1072    SB      Added jump + land time info to player net. Server+Client are now perfectly in sync (for now..)
//  1073    SB      Scrunched down size of player_object::move net data that is sent from client to the server
//  1074    SB      Server now only sends player loadout changes (as opposed to all of it). eg. firing
//  1075    SB      Fixed up player missile/shield pack sounds + added write/read object id to bitstream class
//  1076    AT      New client/server state machine
//  1077    AT      Changed WriteRangedS32
//  1078    AT      Added ExcludeID to arcing
//  1079    DMT     Added flag to GameMgr for time limit.
//  1080    DMT     Fixed/optimized arcing a bit. 
//  1081    SB      Fixed/optimized player net a bit. New satchel net data.
//  1082    JN      Added initialization field to beacon.
//  1083    DMT     Tightened up assets and other projectiles a little.
//  1084    DMT     Added normal to beacon.
//  1085    SB      Tweaked vehicle and player health bits
//  1086    SB      Optimized player net data more...
//  1087    SB      Add player optimization to bitstream read/write ObjectID
//  1088    SB      Shrunk vehicle net data
//  1089    SB      Added vehicle data so that fx's happen on client
//  1090    SB      Added player spawn bit
//  1091    SB      Added extra player bit for "packet too big" fail. Also optimized data sending when in a vehicle
//  1092    SB      Added change team bit to player move read/write. Added team bits to respawn net data
//  1093    SB      Compressed corspe pos + vels
//  1094    AT      Player update distributed over time now...
//  1095    BW      New release for Alpha 10
//  1096    SB      Added anim index and anim frame to player net init data - fixes dead players!
//  1097    DMT     Added bits for disabled and destroyed assets.
//  1098    CJG     Added move data to player for new controls
//  1099    SB      Added exchange key + info to player net data
//  1100    DMT     Using wide string for object labels and player names.
//  1101    BW      USB Ethernet Adapter code re-included
//  1102    DMT     Support for additional messages
//  1103    DMT     Demo 2a
//  1104    DMT     Demo 2b
//  1105    DMT     Demo 2a with new SLUS
//  1106    BW      Transition from pre-alpha to Alpha 1 build
//  1107    DMT     Second attempt at a DEMO VERION
//  1108    SB      Added vote data to player move and net data
//  1109    SB      Added player create corpse dirty bit + made corpse a client side object
//  1110    DMT     Added voting support to GameMgr.
//  1111    DMT     Added support for server password.
//  1112    BW      Alpha 4
//  1113    DMT     New system for sending Life data.
//  1114    AT      Login SessionID Added
//  1115    BW      New version for wide beta test
//  1116    DMT     Fixed bugs in update manager
//  1117    BW      New version for E3
//  1118    DMT     Changes to asset data over wire
//  1119    BW      Changed format of saved data
//  1120    BW      Alpha 12 and data formats changed
//  1121    SB      Added attribute bits, health, and energy to player net init data
//  1122    SB      Sent death anim index when creating corpses
//  1123    SB      Reduces move manager object slot bits from 16 -> 5
//  1124    SB      Send explosion type for satchel charge to know if "beep beep" should be played
//  1125    DMT     Added data to missile to track down a bug.  Will remove it later.
//  1126    BW      Added checksumming to the client server packets to try and catch some corruption
//  1127    DMT     Added support for banning kicked players.
//  1128    DMT     Now with even MORE bits!  128 bits in ack history.
//  1129    AT      Added SessionID stepping & ConnManager reset at mission end
//  1130    DMT     Increased bits used by players for map votes.
//  1131    SB      Increased medium armor grenade launcher ammo to 12 - can change loadout net data
//  1132    SB      Removed anim frame on death
//  1133    SB      Increased medium+heavy armor mine counts - can change loadout net data
//  1134    DMT     Added type to stream to debug networking bug.
//  1135    DMT     Tweaked sensor bits.
//  1136    DMT     Compressed wide strings.
//  1137    DMT     Compressed data and added "inline" names to MsgMgr.
//  1138    DMT     Changed some messages.
//  1139    DMT     Added 1 more bit to GameMgr.m_ChangeMap.
//  1140    DMT     Changed resolution of acring velocity bits.
//  1141    DMT     Removed uneeded data from asset initializations.
//  1142    SB      Fixed player jet stick input
//  1143    SB      Fixed player death pos send (version 99!)
//  1144    MS      Tweaked initial audio container volumes.
//  1145    DMT     Fixed a few projectile blending bugs.
//  1146    DMT     Deferring creation of players until synchronized.
//  1147    SB      Added TVRefreshRate to network/player logic for NTSC/PAL playing each other
//  1148    DMT     Added ViewChanged to move structure for proper aiming in 3rd.
//  1149    BW      Removed all support for additional debug info in network packets
//  1150    CJG     Changed buddy string on server lookups
//  1151    DMT     Tightened up data on pickups
//  1152    SB      Fixed "statue player" when joining a game if your player is already dead
//  1153    CJG     Optimized buddy string on server lookups
//  1154    DMT     Fixed problem in move manager network stuff.
//  1155    SB      Server now applies one recieved move per frame. Helps cpu spikes and auto aiming.
//  1156    BW      Alpha 6
//  1157    SB      Tweaked player physics (can run on any slope terrain + fixed lofting)
//  1158    SB      Fixed vehicle + player sync issues
//  1159    DMT     Fixed shield bubble issues.
//  1160    DMT     Added a couple of messages.
//  1161    SB      Fixed jet thrust when moving slightly on analog stick
//  1162    DMT     Changed to ensure incompatibility with all previous versions.
//  1163    SB      Fixed client jerky vehicle target lock
//  1164    SB      Fixed client jerky death camera
//  1165    DMT     Added mechanism to deliver final score to clients
//  1166    DMT     Next release candidate
//  1167	BW		Made sure the packet checksumming was being performed on a bitstream
//	1168	BW		Increment for RC13.

//==============================================================================

#define SERVER_VERSION  1168
#define ENCRYPTION_KEY  0x22

//==============================================================================
#include "SpecialVersion.hpp"
#ifdef DEMO_DISK_HARNESS
#undef  SERVER_VERSION
#define SERVER_VERSION  9999
#endif

//==============================================================================
#endif // SERVERVERSION_HPP
//==============================================================================
