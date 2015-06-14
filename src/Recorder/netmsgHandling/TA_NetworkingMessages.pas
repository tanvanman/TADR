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
  TANM_Rejection = $1B;
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

const
  TANM_Team = $24;
type
  PTeamMessage = ^TTeamMessage;
  TTeamMessage = packed record
    Marker     : Byte;
    PlayerDPID : TDPID;
    TeamId     : Byte; // max 5 teams
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
  TANM_UnitStatAndMove = $2C;
type
  PUnitStatAndMoveMessage = ^TUnitStatAndMoveMessage;
  TUnitStatAndMoveMessage = packed record
    Marker   : Byte; // TANM_UnitStatAndMove_Message
    Size     : Word; // the size of the bit encoded structure
    Timstamp : Longword; // the timestamp for the player
  end;

Const
  TANM_ChatMessage = $05;
  TANM_LoadingStarted = $08;
  TANM_UnitBuildStarted = $09;
  TANM_UnitTakeDamage = $0B;
  TANM_UnitKilled = $0C;
  TANM_UnitStartScript = $10;
  TANM_UnitState = $11;
  TANM_UnitBuildFinished = $12;
  TANM_PlaySound = $13;
  TANM_GiveUnit = $14;
  // TANM_ = $15; // loading state (?)
  TANM_ShareResources = $16;
  TANM_PlayerResourcesInfo = $28;
  // TANM_ = $29; // field_12A pplayerstruct
  TANM_UnitTypesSync = $1A;
  // TANM_ = $1D; // obsolete, calls nullsub
  // TANM_ = $21; // response with $22
  TANM_HostMigration = $18;
  TANM_PlayerInfo = $20;

Const
  TANM_WeaponFired = $0D;
type
  PWeaponFiredMessage = ^TWeaponFiredMessage;
  TWeaponFiredMessage = packed record
    Marker          : Byte;
    Position_Start  : TPosition;
    Position_Target : TPosition;
    WeaponID        : Byte;
    Interceptor     : Byte;
    Angle           : Word;
    Trajectory      : Word;
    TargetUnitId    : Word;
    AttackerUnitId  : Word;
    WeapIdx         : Byte;
  end;

  PWeaponFiredMessagePatched = ^TWeaponFiredMessagePatched;
  TWeaponFiredMessagePatched = packed record
    Marker          : Byte;
    Position_Start  : TPosition;
    Position_Target : TPosition;
    WeaponID        : Cardinal;
    Interceptor     : Byte;
    Angle           : Word;
    Trajectory      : Word;
    TargetUnitId    : Word;
    AttackerUnitId  : Word;
    WeapIdx         : Byte;
  end;

Const
  TANM_AreaOfEffect = $0E;
type
  PAreaOfEffectMessage = ^TAreaOfEffectMessage;
  TAreaOfEffectMessage = packed record
    Marker     : Byte;
    Position   : TPosition;
    WeaponID   : Byte;
  end;

  PAreaOfEffectMessagePatched = ^TAreaOfEffectMessagePatched;
  TAreaOfEffectMessagePatched = packed record
    Marker     : Byte;
    Position   : TPosition;
    WeaponID   : Cardinal;
  end;

Const
  TANM_FeatureAction = $0F;
type
  PFeatureActionMessage = ^TFeatureActionMessage;
  TFeatureActionMessage = packed record
    Marker     : Byte;
    WeaponID   : Byte;
    X          : Word;
    Y          : Word;
  end;

  PFeatureActionMessagePatched = ^TFeatureActionMessagePatched;
  TFeatureActionMessagePatched = packed record
    Marker     : Byte;
    WeaponID   : Cardinal;
    X          : Word;
    Y          : Word;
  end;

{
Recorder sends to everyone else when the player private chats to someone

Note: old versions can send more than the expected size!!! Upto 100 characters
}
const
  TANM_EnemyChat = $F9;
type
  // size = 73
  PEnemyChatMessage = ^TEnemyChatMessage;
  TEnemyChatMessage = packed record
    Marker     : Byte;  // TANM_EnemyChat
    FromPlayer : TDPID;
    ToPlayer   : TDPID;
    Text       : array [0..64-1] of char;
  end;

{
Replayer message to anounce that this is a replayer mode game (single byte)
}
const
  TANM_ReplayerServer = $FA;
type
  PReplayerServerMessage = ^TReplayerServerMessage;
  TReplayerServerMessage = packed record
    Marker  : Byte;  // TANM_ReplayerServer
  end;
{
Used to transfer data between recorders.
Unknown message subtypes are ignored by versions which dont understand them (this feature is required!)
}
// recorder to recorder packet
const
  TANM_RecorderToRecorder = $FB;
type
  PRecorderToRecorderMessage = ^TRecorderToRecorderMessage;
  TRecorderToRecorderMessage = packed record
    Marker     : Byte; // TANM_RecorderToRecorder
    MsgSize    : Byte; // size of the data message
    MsgSubType : Byte; // The Recorder to Recorder message subtype
  end;

// recorder to recorder subpacket type
const
  // text data (MsgSize)
  TANM_Rec2Rec_MarkerData = $0;
type
  PRec2Rec_MarkerData_Message = ^TRec2Rec_MarkerData_Message;
  TRec2Rec_MarkerData_Message = packed record
    PacketCount : Byte;
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
    CheatsDetected : Cardinal;
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
Used to transfer mod version to other players
}
const
  TANM_Rec2Rec_ModInfo = $5;
type
  PRec2Rec_ModInfo_Message = ^TRec2Rec_ModInfo_Message;
  TRec2Rec_ModInfo_Message = packed record
    PlayerID       : TDPID;
    ModID          : Integer;
    ModMajorVer    : AnsiChar;
    ModMinorVer    : AnsiChar;
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
    NewState      : Byte;
  end;

// unit changed weapon  
const
  TANM_Rec2Rec_UnitWeapon = $0B;
type
  PRec2Rec_UnitWeapon_Message = ^TRec2Rec_UnitWeapon_Message;
  TRec2Rec_UnitWeapon_Message = packed record
    UnitId        : Word;
    WeaponIdx     : Byte;
    NewWeaponID   : Cardinal;
  end;

// Used to signal that unit with custom template has been modified
const
  TANM_Rec2Rec_UnitInfoEdit = $0C;
type
  PRec2Rec_UnitInfoEdit_Message = ^TRec2Rec_UnitInfoEdit_Message;
  TRec2Rec_UnitInfoEdit_Message = packed record
    UnitId        : Word;
    FieldType     : Cardinal;
    NewValue      : Integer;
  end;

// unit type ID changed
const
  TANM_Rec2Rec_UnitInfoSwap = $0D;
type
  PRec2Rec_UnitInfoSwap_Message = ^TRec2Rec_UnitInfoSwap_Message;
  TRec2Rec_UnitInfoSwap_Message = packed record
    UnitID         : Word;
    UnitInfoCRC    : Cardinal;
  end;

const
  TANM_Rec2Rec_NewUnitLocation = $0E;
type
  PRec2Rec_NewUnitLocation_Message = ^TRec2Rec_NewUnitLocation_Message;
  TRec2Rec_NewUnitLocation_Message = packed record
    UnitID         : Word;
    NewX           : Integer;
    NewY           : Integer;
    NewZ           : Integer;
  end;

const
  TANM_Rec2Rec_EmitSFXToUnit = $0F;
type
  PRec2Rec_EmitSFXToUnit_Message = ^TRec2Rec_EmitSFXToUnit_Message;
  TRec2Rec_EmitSFXToUnit_Message = packed record
    FromUnitID     : Word;
    ToUnitID       : Word;
    FromPieceIdx   : SmallInt;
    SfxType        : Byte;
  end;

const
  TANM_Rec2Rec_SetNanolatheParticles = $10;
type
  PRec2Rec_SetNanolatheParticles_Message = ^TRec2Rec_SetNanolatheParticles_Message;
  TRec2Rec_SetNanolatheParticles_Message = packed record
    PosFrom        : TPosition;
    PosTo          : TNanolathePos;
    Reverse        : Byte;
  end;

const
  TANM_Rec2Rec_ExtraUnitState = $11;
type
  PRec2Rec_ExtraUnitState_Message = ^TRec2Rec_ExtraUnitState_Message;
  TRec2Rec_ExtraUnitState_Message = packed record
    UnitId        : Word;
    FieldType     : Cardinal;
    NewValue      : Integer;
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
  TANM_ShareMapPos = $FC;

type
  PShareMapPosMessage = ^TShareMapPosMessage;
  TShareMapPosMessage = packed record
    Marker  : byte;  // TANM_ShameMapPos
    MapX : Word;
    MapY : Word;
  end;

implementation

end.
