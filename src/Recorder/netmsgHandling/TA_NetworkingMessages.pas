unit TA_NetworkingMessages;

interface
uses
  Dplay, TA_MemoryStructures, TA_MemoryLocations;
  
{
Sent whenever a pause or speed change occurs
}
const
  TANM_SimulationSpeedChange = $19;
  SpeedChange = $01;
  PauseChange = $00;
type
  PSimulationSpeedChangeMessage = ^TSimulationSpeedChangeMessage;
  TSimulationSpeedChangeMessage = packed record
    Marker  : byte;  // TANM_SimulationSpeedChange
    case SimSpeedChangeType : Byte of
      PauseChange : ( PauseState  : Byte ); // 0 = pause off, 1 = pause on
      SpeedChange : ( NewSimSpeed : Byte ); // 1-20 scale. GUI reports -9..10 scale.
  end;

{
This message is triggered when ever a player is kicked/drops/leaves from the battleroom.
}

Const
  TANM_Rejection = $1b;
// Rejection reasons
  PlayerDidProperlyClose = $01;
  PlayerDidnotProperlyClose = $06;
type
  PRejectionMessage = ^TRejectionMessage;
  TRejectionMessage = packed record
    Marker  : byte; // TANM_Rejection
    DPlayPlayerID : TDPID; // Directplay Player ID for player being rejected
    Reason : byte; // allways $06
  end;

{
Ally Packet, signals a change in allied state
}
const
  TANM_Ally = $23;
type
  PAllyMessage = ^TAllyMessage;
  TAllyMessage = packed record
    Marker     : byte;  // TANM_Ally
    PlayerID_1 : TDPID; // Directplay Player ID for the player who initiated the ally
    PlayerID_2 : TDPID; // Directplay Player ID for player who is on the receiving end of the ally attempt
    Allied     : Byte;     // The new allied state
    Unknown    : Longword; // unknown
  end;


{
todo : document
}
const
  TANM_Ping = $02;
type
  PPingMessage = ^TPingMessage;
  TPingMessage = packed record
    Marker     : byte;      // TANM_Ping
    Unknown1   : longword;  // ping slot?
    Unknown2   : Longword;  // player ID?
    Unknown3   : Longword;  // timestamp?   
  end;
  
{
unitstat + move.

Bit packed structure
}
Const
  TANM_UnitStatAndMove_Message = $2c;
type
  PUnitStatAndMoveMessage = ^TUnitStatAndMoveMessage;
  TUnitStatAndMoveMessage = packed record
    Marker   : byte; // TANM_UnitStatAndMove_Message
    Size     : Word; // the size of the bit encoded structure
    Timstamp : Longword; // the timestamp for the player
  end;

Const
  TANM_WeaponProjectile_Message = $0D;
type
  PWeaponProjectileMessage = ^TWeaponProjectileMessage;
  TWeaponProjectileMessage = packed record
    PacketType : Byte;
    Position_Start : TPosition;
    Position_Target : TPosition;
    WeaponID : Byte;
    field_1A : Byte;
    field_1B : Word;
    field_1D : Word;
    Attacker_ID : Word;
    Victim_ID : Word;
    field_23 : Byte;
  end;

  PWeaponProjectileMessagePatched = ^TWeaponProjectileMessagePatched;
  TWeaponProjectileMessagePatched = packed record
    PacketType : Byte;
    Position_Start : TPosition;
    Position_Target : TPosition;
    WeaponID : Cardinal;
    field_1A : Byte;
    field_1B : Word;
    field_1D : Word;
    Attacker_ID : Word;
    Victim_ID : Word;
    field_23 : Byte;
  end;

{
Recorder sends to everyone else when the player private chats to someone

Note: old versions can send more than the expected size!!! Upto 100 characters
}
const
  TANM_EnemyChat = $f9;
type
  // size = 73
  PEnemyChatMessage = ^TEnemyChatMessage;
  TEnemyChatMessage = packed record
    Marker  : byte;  // TANM_EnemyChat
    FromPlayer : TDPID;
    ToPlayer : TDPID;
    Text : array [0..64-1] of char; 
  end;

{
Replayer message to anounce that this is a replayer mode game (single byte)
}
const
  TANM_ReplayerServer = $fa;
type
  PReplayerServerMessage = ^TReplayerServerMessage;
  TReplayerServerMessage = packed record
    Marker  : byte;  // TANM_ReplayerServer
  end;
{
Used to transfer data between recorders.
Unknown message subtypes are ignored by versions which dont understand them (this feature is required!)
}
// recorder to recorder packet
const
  TANM_RecorderToRecorder = $fb;
type
  PRecorderToRecorderMessage = ^TRecorderToRecorderMessage;
  TRecorderToRecorderMessage = packed record
    Marker  : byte;  // TANM_RecorderToRecorder
    MsgSize : Byte;  // size of the data message
    MsgSubType : Byte; // The Recorder to Recorder message subtype
  end;

// recorder to recorder subpacket type
const
  // text data (MsgSize)
  TANM_Rec2Rec_MarkerData = $0;
type
  PRec2Rec_MarkerData_Message = ^TRec2Rec_MarkerData_Message;
  TRec2Rec_MarkerData_Message = packed record
    PacketCount : byte;
    // variable data structure
  end;  

const
  // no extra data
  TANM_Rec2Rec_CmdWarp = $1;

{
Use to notify other recorders that the cheat detection for well known trainers has been tripped.
Generates false positives
}
const
  TANM_Rec2Rec_CheatDetection = $2;
type
  PRec2Rec_CheatsDetected_Message = ^TRec2Rec_CheatsDetected_Message;
  TRec2Rec_CheatsDetected_Message = packed record
    CheatsDetected : Longword;
  end;

{
Used to signal that sharelos is enabled/disabled for a particular player.
Fixes the mess with detecting the use .sharelos command
}
const
  TANM_Rec2Rec_Sharelos = $3;
type
  PRec2Rec_Sharelos_Message = ^TRec2Rec_Sharelos_Message;
  TRec2Rec_Sharelos_Message = packed record
    ShareLosState : byte;
  end;

{
Used to transfer extra battleroom state info to other players
}
const
  TANM_Rec2Rec_GameStateInfo = $4;
type
{$MINENUMSIZE 1}
  PRec2Rec_GameStateInfo_Message = ^TRec2Rec_GameStateInfo_Message;
  TRec2Rec_GameStateInfo_Message = packed record
    AutopauseState : Byte;
    F1Disable      : Byte;
    Commanderwarp  : Byte;
    SpeedLock      : Byte;
    SpeedLockNative: Byte;
    SlowSpeed      : Byte;
    FastSpeed      : Byte;    
    AIDifficulty   : Byte;
  end;
  
{
Use to notify other players that unit wants to have its own "global" template
}
const
  TANM_Rec2Rec_UnitGrantUnitInfo = $0A;
type
  PRec2Rec_UnitGrantUnitInfo_Message = ^TRec2Rec_UnitGrantUnitInfo_Message;
  TRec2Rec_UnitGrantUnitInfo_Message = packed record
    UnitId        : Word;
    UnitIdRemote  : LongWord;
    NewState      : Byte;
  end;

// unit changed weapon  
const
  TANM_Rec2Rec_UnitWeapon = $0B;
type
  PRec2Rec_UnitWeapon_Message = ^TRec2Rec_UnitWeapon_Message;
  TRec2Rec_UnitWeapon_Message = packed record
    UnitId        : Word;
    WhichWeapon   : Byte;
    NewWeaponID   : Cardinal;
    RequiresPatch : Boolean;
  end;

// Used to signal that unit with custom template has been modified
const
  TANM_Rec2Rec_UnitInfoEdit = $0C;
type
  PRec2Rec_UnitInfoEdit_Message = ^TRec2Rec_UnitInfoEdit_Message;
  TRec2Rec_UnitInfoEdit_Message = packed record
    UnitId        : Word;
    UnitIdRemote  : LongWord;
    FieldType     : LongWord;
    NewValue      : LongWord;
  end;

// unit type ID changed
const
  TANM_Rec2Rec_UnitTemplate = $0D;
type
  PRec2Rec_UnitTemplate_Message = ^TRec2Rec_UnitTemplate_Message;
  TRec2Rec_UnitTemplate_Message = packed record
    UnitID         : Word;
    NewTemplateCrc : Cardinal;
    Recreate       : Byte;
  end;

const
  TANM_Rec2Rec_NewUnitLocation = $0E;
type
  PRec2Rec_NewUnitLocation_Message = ^TRec2Rec_NewUnitLocation_Message;
  TRec2Rec_NewUnitLocation_Message = packed record
    UnitID         : Word;
    NewX           : Cardinal;
    NewY           : Cardinal;
    NewZ           : Cardinal;
  end;

const
  TANM_Rec2Rec_SetNanolatheParticles = $0F;
type
  PRec2Rec_SetNanolatheParticles_Message = ^TRec2Rec_SetNanolatheParticles_Message;
  TRec2Rec_SetNanolatheParticles_Message = packed record
    PosFrom        : TPosition;
    PosTo          : TNanolathePos;
    Reverse        : Byte;
  end;

{
After interpreting vote command by host, launches voting. Host -> players
}
const
  TANM_Rec2Rec_VoteStart = $14;
type
  PRec2Rec_VoteStart_Message = ^TRec2Rec_VoteStart_Message;
  TRec2Rec_VoteStart_Message = packed record
    VoteType       : byte;  // map / kick player 
    VoteExpireTime : Word;
    VoteString     : string[64]; // map or player name
    // to przy przetwarzaniu komendy
    //VoteStartedBy  : string[32];
    //VoteTimeStamp  : Integer;
  end;

{
Vote answer. Player -> Host
From which player = FromPlayer.ID
}
const
  TANM_Rec2Rec_VoteAnswer = $15;
type
  PRec2Rec_VoteAnswer_Message = ^TRec2Rec_VoteAnswer_Message;
  TRec2Rec_VoteAnswer_Message = packed record
    Answer       : byte; // yes / no
  end;

{
Vote status. Host -> players
Only host counts votes so displayed numbers on players screen will be always correct
If player clicks yes/no button - bump count, disable buttons
}
const
  TANM_Rec2Rec_VoteStatus = $16;
type
  PRec2Rec_VoteStatus_Message = ^TRec2Rec_VoteStatus_Message;
  TRec2Rec_VoteStatus_Message = packed record
    YesCount       : byte;
    NoCount        : byte;
  end;

{
Used to end vote. F.e. 'yes' won or voting has expired. Host -> players
}
const
  TANM_Rec2Rec_VoteEnd = $17;
type
  PRec2Rec_VoteEnd_Message = ^TRec2Rec_VoteEnd_Message;
  TRec2Rec_VoteEnd_Message = packed record
    VoteResult       : byte; // yes / no / expired
  end;

{
Used by TA demo recorder to transmite the map view point of a player to other players
}

const
  TANM_ShameMapPos = $fc;

type
  PShareMapPosMessage = ^TShareMapPosMessage;
  TShareMapPosMessage = packed record
    Marker  : byte;  // TANM_ShameMapPos
    MapX : Word;
    MapY : Word;
  end;

implementation

end.
