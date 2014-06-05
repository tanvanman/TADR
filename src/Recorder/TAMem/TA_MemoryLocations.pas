unit TA_MemoryLocations;

interface
uses
  dplay, Classes, TA_MemoryStructures;

type
  PCustomUnitInfo = ^TCustomUnitInfo;
  TCustomUnitInfo = packed record
    unitId        : Cardinal;
    unitIdRemote  : Cardinal;  // owner's (unit upgrade packet sender) local unit id
    //OwnerPlayer  : Longint; // Buffer.EventPlayer_DirectID = PlayerAryIndex2ID(UnitInfo.ThisPlayer_ID   )
    InfoPtrOld   : Cardinal;  // local old global template Pointer
    InfoStruct   : TGameUnitInfo;
  end;
  TUnitInfos = array of TCustomUnitInfo;

  TStoreUnitsRec = packed record
    Id : Cardinal;
    UnitIds  : array of LongWord;
  end;
  TUnitSearchArr = array of TStoreUnitsRec;
  TSpawnedMinionsArr = array of TStoreUnitsRec;

  TAMem = class
  protected
    class Function GetViewPlayer : Byte;
    class Function GetGameSpeed : Byte;

    class Function GetMaxUnitLimit : Word;
    class Function GetActualUnitLimit : Word;
    class Function GetIsAlteredUnitLimit : Boolean;
    class function GetUnitsPtr : Pointer;
    class function GetUnits_EndMarkerPtr : Pointer;
    class function GetMainStructPtr : Pointer;
    class function GetProgramStructPtr : Pointer;
    class function GetPlayersStructPtr : Pointer;
    class function GetModelsArrayPtr : Pointer;
    class function GetWeaponTypeDefArrayPtr : Pointer;
    class function GetFeatureTypeDefArrayPtr : Pointer;
    class function GetUnitInfosPtr : Pointer;
    class function GetUnitInfosCount : LongWord;
    class function GetSwitchesMask : Word;
    class procedure SetSwitchesMask(Mask: Word);

    class Function GetPausedState : Boolean;
    class Procedure SetPausedState( value : Boolean);
  public
    Property Paused : Boolean read GetPausedState write SetPausedState;

    property ViewPlayer : Byte read GetViewPlayer;
    Property GameSpeed : Byte read GetGameSpeed;
    Property MaxUnitLimit : Word read GetMaxUnitLimit;
    Property ActualUnitLimit : Word read GetActualUnitLimit;
    Property IsAlteredUnitLimit : Boolean read GetIsAlteredUnitLimit;
    Property UnitsPtr : Pointer read GetUnitsPtr;
    Property Units_EndMarkerPtr : Pointer read GetUnits_EndMarkerPtr;
    Property MainStructPtr : Pointer read GetMainStructPtr;
    Property ProgramStructPtr : Pointer read GetProgramStructPtr;
    Property PlayersStructPtr : Pointer read GetPlayersStructPtr;
    Property ModelsArrayPtr : Pointer read GetModelsArrayPtr;
    Property WeaponTypeDefArrayPtr : Pointer read GetWeaponTypeDefArrayPtr;
    Property FeatureTypeDefArrayPtr : Pointer read GetFeatureTypeDefArrayPtr;
    Property UnitInfosPtr : Pointer read GetUnitInfosPtr;
    Property UnitInfosCount : LongWord read GetUnitInfosCount;
    Property SwitchesMask : Word read GetSwitchesMask write SetSwitchesMask;

    class function GetModelPtr(index: Word): Pointer;
    class function UnitInfoId2Ptr(index: Word): Pointer;
    class function GetMovementClassPtr(index: Word): Pointer;
    class function GetWeapon(ID: Word) : Pointer;
    class function GetFeatureDef(ID : Word) : Pointer;
    class function GetMaxUnitId: LongWord;
    class function IsTAVersion31 : Boolean;
  end;

  TAPlayer = class
  protected
    class function GetShareEnergyVal : single;
    class function GetShareMetalVal : single;
    class function GetShareEnergy : Boolean;
    class function GetShareMetal : Boolean;
    class function GetShootAll : Boolean;
    class procedure SetShootAll(value: Boolean);
  public
    property ShareEnergyVal : single read GetShareEnergyVal;
    property ShareMetalVal : single read GetShareMetalVal;
    property ShareEnergy : Boolean read GetShareEnergy;
    property ShareMetal : Boolean read GetShareMetal;
    property ShootAll : Boolean read GetShootAll write SetShootAll;

    class Function GetDPID(player : Pointer) : TDPID;
    class Function GetPlayerByIndex(playerIndex : LongWord) : Pointer;
    class Function GetPlayerPtrByDPID(playerPID : TDPID) : Pointer;
    // zero based player index
    class Function GetPlayerByDPID(playerPID : TDPID) : LongWord;

    class Function PlayerType(player : Pointer) : TTAPlayerType;

    class function GetShareRadar(player : Pointer) : Boolean;
    class Procedure SetShareRadar(player : Pointer; value : Boolean);

    class function GetIsWatcher(player : Pointer) : Boolean;
    class function GetIsActive(player : Pointer) : Boolean;

    class function GetAlliedState(Player1 : Pointer; Player2 : Integer) : Boolean;
    class Procedure SetAlliedState(Player1 : Pointer; Player2 : Integer; value : Boolean);
  end;

  TMinionsPattern = ( MinionsPattern_Random, MinionsPattern_Square, MinionsPattern_Circle );

  TUnitSearchFilter = ( usfNone, usfOwner, usfAllied, usfEnemy, usfAI, usfExcludeAir );
  TUnitSearchFilterSet = set of TUnitSearchFilter;

  TAUnit = class
  public
    class Function GetKills(UnitPtr : Pointer) : Word;
    class procedure SetKills(UnitPtr : Pointer; kills : Word);
    class Function GetHealth(UnitPtr : Pointer) : Word;
    class procedure SetHealth(UnitPtr : Pointer; health : LongWord);
    class Function GetCloak(UnitPtr : Pointer) : LongWord;
    class procedure SetCloak(UnitPtr : Pointer; cloak : Word);

    class function GetUnitX(UnitPtr : Pointer): Word;
    class procedure SetUnitX(UnitPtr : Pointer; X : Word);
    class function GetUnitZ(UnitPtr : Pointer): Word;
    class procedure SetUnitZ(UnitPtr : Pointer; Z : Word);
    class function GetUnitY(UnitPtr : Pointer): Word;
    class procedure SetUnitY(UnitPtr : Pointer; Y : Word);

    class function GetTurnX(UnitPtr : Pointer): Word;
    class procedure SetTurnX(UnitPtr : Pointer; X : Word);
    class function GetTurnZ(UnitPtr : Pointer): Word;
    class procedure SetTurnZ(UnitPtr : Pointer; Z : Word);
    class function GetTurnY(UnitPtr : Pointer): Word;
    class procedure SetTurnY(UnitPtr : Pointer; Y : Word);

    class function GetMovementClass(UnitPtr : Pointer): Pointer;
    class function SetTemplate(UnitPtr: Pointer; NewUnitInfoId: Word): Boolean;

    class function GetWeapon(UnitPtr : Pointer; index: LongWord): Word;
    class function SetWeapon(UnitPtr : Pointer; index: LongWord; weaponid: Word): Boolean;
    class function GetAttackerID(UnitPtr : Pointer): LongWord;

    class function UpdateLos(UnitPtr: Pointer): LongWord;

    { sfx }
    class procedure Speech(UnitPtr : longword; speechtype: longword; speechtext: PChar);
    class procedure SoundEffectId(UnitPtr : longword; voiceid: longword);
    class function PlayGafAnim(BmpType: Byte; X, Z: Word; Glow, Smoke: Byte): Integer;

    { position stuff }
    class function AtMouse: Pointer;
    class function CreatePositionOfCoords(X, Z, Y: Word): TPosition;
    class function Position2Grid(Position: TPosition; UnitInfoId: Word; out GridPosX, GridPosZ: SmallInt ): Boolean;
    class function GetCurrentSpeed(UnitPtr: Pointer): Cardinal; // percentage

    { creating and killing unit }
    class function TestBuildSpot(PlayerIndex: Byte; UnitInfoId: Word; nPosX, nPosZ: Word ): Boolean;
    class function CreateUnit(OwnerIndex: LongWord; UnitInfoId: LongWord; Position: TPosition; Turn: PTurn; TurnZOnly, RandomTurnZ: Boolean; UnitState: LongWord; FullHp: LongWord): Pointer;
    class Function GetBuildTimeLeft(UnitPtr : Pointer) : single;
    class procedure Kill(UnitPtr : Pointer; deathtype: byte);
    class procedure SwapByKill(UnitPtr: Pointer; newUnitInfoId: Word);

    { actions (orders, unit state) }
    class function GetCurrentOrder(UnitPtr: Pointer): TTAActionType;
    class function GetCurrentOrderParams(UnitPtr: Pointer; Par: Byte): Cardinal;
    class function GetCurrentOrderState(UnitPtr: Pointer): Cardinal;
    class function EditCurrentOrderParams(UnitPtr: Pointer; Par: Byte; NewValue: LongWord): Boolean;
    class function CreateOrder(UnitPtr: Pointer; TargetUnitPtr: Pointer; ActionType: TTAActionType; Position: PPosition; ShiftKey: Byte; Par1: LongWord; Par2: LongWord): LongInt;

    { COB }
    class Function GetCOBDataPtr(UnitPtr : Pointer) : Pointer;
    class function CallCobProcedure(UnitPtr: Pointer; ProcName: String; Par1, Par2, Par3, Par4: PLongWord): Cardinal;
    class function CallCobWithCallback(UnitPtr: Pointer; ProcName: String; Par1, Par2, Par3, Par4: LongWord): Cardinal;
    class function GetCobString(UnitPtr: Pointer): String;

    { id, owner etc. }
    class Function GetOwnerPtr(UnitPtr : Pointer) : Pointer;
    class Function GetOwnerIndex(UnitPtr : Pointer) : Integer;
    class Function GetOwnerIndexBuildspotSafe(UnitPtr : Pointer): Integer;
    class Function GetId(UnitPtr : Pointer) : Word;
    class Function GetLongId(UnitPtr : Pointer) : LongWord;
    class Function Id2Ptr(LongUnitId : LongWord) : Pointer;
    class Function Id2LongId(UnitId: Word) : LongWord;
    class function GetUnitInfoId(UnitPtr: Pointer): Word;
    class function GetUnitInfoPtr(UnitPtr: Pointer): Pointer;
    class Function IsOnThisComp(UnitPtr : Pointer; IncludeAI: Boolean) : Boolean;
    class function IsAllied(UnitPtr: Pointer; UnitId: LongWord): Byte;
    class Function IsFromAlly(UnitPtr: Pointer): Byte;
    class Function Team(UnitId: LongWord): Integer;
    class Function IsRemoteIdLocal(UnitPtr: Pointer; remoteUnitId: PLongWord; out local: Boolean): LongWord;

    { cloning global template and setting its fields }
    class function GrantUnitInfo(UnitPtr: Pointer; State: Byte; remoteUnitId: PLongWord): Boolean;
    class function SearchCustomUnitInfos(unitId: LongWord; remoteUnitId: PLongWord; local: Boolean; out index: Integer ): Boolean;
    class function GetUnitInfoField(UnitPtr: Pointer; fieldType: TUnitInfoExtensions; remoteUnitId: PLongWord): LongWord;
    class function SetUnitInfoField(UnitPtr: Pointer; fieldType: TUnitInfoExtensions; value: LongWord; remoteUnitId: PLongWord): Boolean;
  end;

  TAUnits = class
  public
    class Function CreateMinions(UnitPtr: Pointer; Amount: Byte; Pattern: TMinionsPattern; UnitType: Word; Action: TTAActionType; ArrayId: LongWord): Integer;

    { searching units in game }
    class function CreateSearchFilter(Mask: Integer): TUnitSearchFilterSet;
    class function GetRandomArrayId(ArrayType: Byte): Word;
    class function SearchUnits(UnitPtr: Pointer; SearchId: LongWord; SearchType: Byte; MaxDistance: Integer; Filter: TUnitSearchFilterSet; UnitTypeFilter: Word ): LongWord;
    class function UnitsIntoGetterArray(UnitPtr: Pointer; ArrayType: Byte; Id: LongWord; const UnitsArray: TFoundUnits): Boolean;
    class procedure ClearSearchRec(Id: LongWord; ArrayType: Byte);
    class procedure RandomizeSearchRec(Id: LongWord; ArrayType: Byte);
  //  class function Distance(UnitPtr: Pointer; UnitId2: LongWord): LongWord; overload;
    class function Distance(UnitId1, UnitId2: LongWord): Integer;
    class function CircleCoords(CenterPosition: TPosition; Radius: Integer; Angle: integer; out x, z: LongInt): Boolean;
   // class function Teleport(CallerTelePtr: Pointer; DestinationTeleId, TeleportedUnitId: Cardinal): Cardinal;
  end;

const
  BoolValues : array [Boolean] of Byte = (0,1);

var
  TAData : TAMem;
  PlayerInfo: TAPlayer;

function IsTAVersion31 : Boolean;

implementation
uses
  Windows,
  sysutils,
  logging,
  Math,
  TADemoConsts,
  TA_FunctionsU,
  IniOptions,
  TAMemManipulations,
  ExplodeBitmaps,
  COB_Extensions,
  TA_MemoryConstants;

// -----------------------------------------------------------------------------

function ReverseBits(const Value: LongWord): LongWord; register; assembler;
asm
      BSWAP   EAX
      MOV     EDX, EAX
      AND     EAX, 0AAAAAAAAh
      SHR     EAX, 1
      AND     EDX, 055555555h
      SHL     EDX, 1
      OR      EAX, EDX
      MOV     EDX, EAX
      AND     EAX, 0CCCCCCCCh
      SHR     EAX, 2
      AND     EDX, 033333333h
      SHL     EDX, 2
      OR      EAX, EDX
      MOV     EDX, EAX
      AND     EAX, 0F0F0F0F0h
      SHR     EAX, 4
      AND     EDX, 00F0F0F0Fh
      SHL     EDX, 4
      OR      EAX, EDX
end;

function IsTAVersion31 : Boolean;
begin
result := TAMem.IsTAVersion31();
end; {IsTAVersion31}

var
  CacheUsed : Boolean;
  IsTAVersion31_Cache : Boolean;

// -----------------------------------------------------------------------------
// TAMem
// -----------------------------------------------------------------------------

class function TAMem.IsTAVersion31 : Boolean;
const
  Address = $4ad494;
  ExpectedData : array [0..2] of Byte = (0,$55,$e8);
var
  FailIndex : Integer;
  FailValue : Byte;
  Procedure DoReport;
  begin
  Tlog.Add(0, 'At 0x'+IntToHex(Address,8)+' index '+IntToStr(FailIndex)+
              ' expecting 0x'+IntToHex(ExpectedData[FailIndex],2)+
              ' but found 0x'+IntToHex(FailValue,2));
  end;
begin
if not CacheUsed then
  begin
  try
    IsTAVersion31_Cache := TestBytes(Address, @ExpectedData[0], length(ExpectedData), FailIndex, FailValue );
    if not IsTAVersion31_Cache then
      DoReport;
  except
    on e : EAccessViolation do
      IsTAVersion31_Cache := false;
  end;
  CacheUsed := true;
  end;
result := IsTAVersion31_Cache;
end;

class Function TAMem.GetViewPLayer : Byte;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.cLOS_Sight_PlayerID;
end;

class Function TAMem.GetGameSpeed : Byte;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.nTAGameSpeed;
end;

class function TAMem.GetMaxUnitLimit : Word;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.nMaxUnitLimitPerPlayer;
end;

class function TAMem.GetActualUnitLimit : Word;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.nActualUnitLimit;
end;

class function TAMem.GetIsAlteredUnitLimit : Boolean;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.cAlteredUnitLimit <> 0;
end;

class function TAMem.GetUnitsPtr : Pointer;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.p_Units;
end;

class function TAMem.GetUnits_EndMarkerPtr : Pointer;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.p_LastUnitInArray;
end;

class function TAMem.GetMainStructPtr : Pointer;
begin
result := Pointer(PLongWord(TADynmemStructPtr)^);
end;

class function TAMem.GetProgramStructPtr : Pointer;
begin

result := PTAdynmemStruct(TAData.MainStructPtr)^.p_TAProgram;
end;

class function TAMem.GetPlayersStructPtr: Pointer;
begin
result := @PTAdynmemStruct(TAData.MainStructPtr)^.Players[0];
end;

class function TAMem.GetModelsArrayPtr : Pointer;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.p_MODEL_PTRS;
end;

class function TAMem.GetWeaponTypeDefArrayPtr : Pointer;
begin
result := @PTAdynmemStruct(TAData.MainStructPtr)^.Weapons[0];
end;

class function TAMem.GetFeatureTypeDefArrayPtr : Pointer;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.p_FeatureDefs;
end;

class function TAMem.GetUnitInfosPtr : Pointer;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.p_UnitDefs;
end;

class function TAMem.GetUnitInfosCount : LongWord;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.lNumUnitTypeDefs_Sqrt;
end;

class function TAMem.GetSwitchesMask : Word;
begin
Result:= PTAdynmemStruct(TAData.MainStructPtr)^.nSwitchesMask;
end;

class procedure TAMem.SetSwitchesMask(Mask: Word);
begin
PTAdynmemStruct(TAData.MainStructPtr)^.nSwitchesMask:= Mask;
end;

class Function TAMem.GetPausedState : Boolean;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.cIsGamePaused <> 0;
end;

class Procedure TAMem.SetPausedState( value : Boolean);
begin
PTAdynmemStruct(TAData.MainStructPtr)^.cIsGamePaused := BoolValues[value];
end;

class function TAMem.GetModelPtr(index: Word): Pointer;
begin
result := PLongWord(LongWord(GetModelsArrayPtr) + index * 4);
end;

class function TAMem.UnitInfoId2Ptr(index: Word): Pointer;
begin
result := Pointer(LongWord(TAData.UnitInfosPtr) + index * SizeOf(TGameUnitInfo));
end;

class function TAMem.GetMovementClassPtr(index: Word): Pointer;
begin
result := Pointer(TAMovementClassArray + SizeOf(TMoveInfoClassStruct) * index);
end;

class function TAMem.GetWeapon(ID: Word): Pointer;
begin
// fix me to be compatible with xpoy id patch
  if IniSettings.WeaponIdPatch then
    result:= @PTAdynmemStruct(TAData.MainStructPtr).Weapons[ID]
  else
    result:= @PTAdynmemStruct(TAData.MainStructPtr).Weapons[ID];
end;

class function TAMem.GetFeatureDef(ID: Word): Pointer;
begin
  result := Pointer(LongWord(PTAdynmemStruct(TAData.MainStructPtr).p_FeatureDefs) + SizeOf(TFeatureDefStruct) * ID);
end;

class function TAMem.GetMaxUnitId: LongWord;
begin
  if TAData.IsAlteredUnitLimit then
    result := TAData.ActualUnitLimit * MAXPLAYERCOUNT
  else
    result := TAData.MaxUnitLimit * MAXPLAYERCOUNT;
end;

// -----------------------------------------------------------------------------
// TAPlayer
// -----------------------------------------------------------------------------

Class function TAPlayer.GetShareEnergyVal : single;
begin
result := PPlayerStruct(TAData.PlayersStructPtr).fShareLimitEnergy;
end;

Class function TAPlayer.GetShareMetalVal : single;
begin
result := PPlayerStruct(TAData.PlayersStructPtr).fShareLimitMetal;
end;

Class function TAPlayer.GetShareEnergy : Boolean;
begin
result := SharedState_SharedEnergy in PPlayerStruct(TAData.PlayersStructPtr).PlayerInfo.SharedBits;
end;

Class function TAPlayer.GetShareMetal : Boolean;
begin
result := SharedState_SharedMetal in PPlayerStruct(TAData.PlayersStructPtr).PlayerInfo.SharedBits;
end;

Class function TAPlayer.GetShootAll : Boolean;
begin
Result:= ((TAData.SwitchesMask and SwitchesMasks[SwitchesMask_ShootAll]) = SwitchesMasks[SwitchesMask_ShootAll]);
end;

Class procedure TAPlayer.SetShootAll(value: Boolean);
begin
if value then
  TAData.SwitchesMask := TAData.SwitchesMask or SwitchesMasks[SwitchesMask_ShootAll]
else
  TAData.SwitchesMask := TAData.SwitchesMask and not SwitchesMasks[SwitchesMask_ShootAll];
end;

class Function TAPlayer.GetDPID(player : Pointer) : TDPID;
begin
result := PPlayerStruct(player).lDirectPlayID;
end;

class Function TAPlayer.GetPlayerByIndex(playerIndex : LongWord) : Pointer;
begin
result:= @PTAdynmemStruct(TAData.MainStructPtr)^.Players[playerIndex];
//result := Pointer(TAData.PlayersStructPtr+(playerIndex*SizeOf(TPlayerStruct)) );
end;

class Function TAPlayer.GetPlayerPtrByDPID(playerPID : TDPID) : Pointer;
var
  i : Integer;
  PlayerSt: PPlayerStruct;
begin
result := nil;
i := 0;
while i < MAXPLAYERCOUNT do
  begin
  PlayerSt:= GetPlayerByIndex(i);
  if PDPID(PlayerSt.lDirectPlayID)^ = playerPID then
    begin
    result := PlayerSt;
    break;
    end;
  inc(i);
  end;
end;

class Function TAPlayer.GetPlayerByDPID(playerPID : TDPID) : LongWord;
var
  aplayerPID : PDPID;
  i : Integer;
begin
result := LongWord(-1);
aplayerPID := @(PPlayerStruct(TAData.PlayersStructPtr).lDirectPlayID);
i := 0;
while i < MAXPLAYERCOUNT do
  begin
  if aplayerPID^ = playerPID then
    begin
    result := i;
    break;
    end;
  aplayerPID := Pointer(LongWord(aplayerPID)+SizeOf(TPlayerStruct));
  inc(i);
  end;
end;

class Function TAPlayer.PlayerType(player : Pointer) : TTAPlayerType;
begin
result := PPlayerStruct(player).en_cPlayerType;
end;

Class function TAPlayer.GetShareRadar(player : Pointer) : Boolean;
begin
result := SharedState_SharedRadar in PPlayerStruct(Player).PlayerInfo.SharedBits;
end;

Class Procedure TAPlayer.SetShareRadar(player : Pointer; value : Boolean);
begin
if value then
  Include(PPlayerStruct(Player).PlayerInfo.SharedBits, SharedState_SharedRadar)
else
  Exclude(PPlayerStruct(Player).PlayerInfo.SharedBits, SharedState_SharedRadar);
end;

Class function TAPlayer.GetIsWatcher(player : Pointer) : Boolean;
begin
result := PropertyMask_Watcher in PPlayerStruct(Player).PlayerInfo.PropertyMask;
end;

Class function TAPlayer.GetIsActive(player : Pointer) : Boolean;
begin
result := PPlayerStruct(Player).lPlayerActive <> $0;
end;

class function TAPlayer.GetAlliedState(Player1 : Pointer; Player2 : Integer) : Boolean;
begin
Result:= False;
if (Player1 = nil) or (LongWord(Player2) >= MAXPLAYERCOUNT) then exit;
result := PPlayerStruct(Player1).cAllyFlagArray[Player2] <> 0;
end;

class Procedure TAPlayer.SetAlliedState(Player1 : Pointer; Player2 : Integer; value : Boolean);
begin
if (Player1 = nil) or (LongWord(Player2) >= MAXPLAYERCOUNT) then exit;
PPlayerStruct(Player1).cAllyFlagArray[Player2] := BoolValues[value]
end;

// -----------------------------------------------------------------------------
// TAUnit
// -----------------------------------------------------------------------------

class Function TAUnit.GetKills(UnitPtr : Pointer) : Word;
begin
result := PUnitStruct(UnitPtr).nKills;
end;

class procedure TAUnit.SetKills(UnitPtr : Pointer; Kills : Word);
begin
PUnitStruct(UnitPtr).nKills := Kills;
end;

class Function TAUnit.GetHealth(UnitPtr : Pointer) : Word;
begin
result := PUnitStruct(UnitPtr).nHealth;
end;

class procedure TAUnit.SetHealth(UnitPtr : Pointer; Health : LongWord);
begin
PUnitStruct(UnitPtr).nHealth := Health;
end;

const
  Cloak_BitMask = $4;
  CloakUnitStateMask_BitMask = $800;
Class function TAUnit.GetCloak(UnitPtr : Pointer) : LongWord;
var
  Bitfield : Word;
begin
  Bitfield := PUnitStruct(UnitPtr).nUnitStateMaskBas;
  if (Bitfield and Cloak_BitMask) = Cloak_BitMask then
    Result:= 1
  else
    Result:= 0;
end;

class procedure TAUnit.SetCloak(UnitPtr : Pointer; Cloak : Word);
begin
  if Cloak = 1 then
    PUnitStruct(UnitPtr).lUnitStateMask := PUnitStruct(UnitPtr).lUnitStateMask or CloakUnitStateMask_BitMask
  else
    PUnitStruct(UnitPtr).lUnitStateMask := PUnitStruct(UnitPtr).lUnitStateMask and not CloakUnitStateMask_BitMask;
end;

class function TAUnit.GetUnitX(UnitPtr : Pointer): Word;
begin
result := PUnitStruct(UnitPtr).Position.X;
end;

class procedure TAUnit.SetUnitX(UnitPtr : Pointer; X : Word);
begin
PUnitStruct(UnitPtr).Position.X := X;
PUnitStruct(UnitPtr).nGridPosX := (MakeLong(PUnitStruct(UnitPtr).Position.x_, X) - (SmallInt(PGameUnitfInfo(TAMem.UnitInfoId2Ptr(PUnitStruct(UnitPtr).nUnitCategoryID)).nFootPrintX shl 19)) + $80000) shr 20;
PUnitStruct(UnitPtr).nLargeGridPosX := PUnitStruct(UnitPtr).nGridPosX div 2;
end;

class function TAUnit.GetUnitZ(UnitPtr : Pointer): Word;
begin
result := PUnitStruct(UnitPtr).Position.Z;
end;

class procedure TAUnit.SetUnitZ(UnitPtr : Pointer; Z : Word);
begin
PUnitStruct(UnitPtr).Position.Z := Z;
PUnitStruct(UnitPtr).nGridPosZ := (MakeLong(PUnitStruct(UnitPtr).Position.z_, Z) - (SmallInt(PGameUnitfInfo(TAMem.UnitInfoId2Ptr(PUnitStruct(UnitPtr).nUnitCategoryID)).nFootPrintZ shl 19)) + $80000) shr 20;
PUnitStruct(UnitPtr).nLargeGridPosZ := PUnitStruct(UnitPtr).nGridPosZ div 2;
end;

class function TAUnit.GetUnitY(UnitPtr : Pointer): Word;
begin
result := PUnitStruct(UnitPtr).Position.Y;
end;

class procedure TAUnit.SetUnitY(UnitPtr : Pointer; Y : Word);
begin
PUnitStruct(UnitPtr).Position.Y := Y;
end;

class function TAUnit.GetTurnX(UnitPtr : Pointer): Word;
begin
result := PUnitStruct(UnitPtr).Turn.X;
end;

class procedure TAUnit.SetTurnX(UnitPtr : Pointer; X : Word);
begin
PUnitStruct(UnitPtr).Turn.X:= X;
end;

class function TAUnit.GetTurnZ(UnitPtr : Pointer): Word;
begin
result := PUnitStruct(UnitPtr).Turn.Z;
end;

class procedure TAUnit.SetTurnZ(UnitPtr : Pointer; Z : Word);
begin
PUnitStruct(UnitPtr).Turn.Z:= Z;
end;

class function TAUnit.GetTurnY(UnitPtr : Pointer): Word;
begin
result := PUnitStruct(UnitPtr).Turn.Y;
end;

class procedure TAUnit.SetTurnY(UnitPtr : Pointer; Y : Word);
begin
PUnitStruct(UnitPtr).Turn.Y:= Y;
end;

class function TAUnit.GetMovementClass(UnitPtr : Pointer): Pointer;
begin
result := PUnitStruct(UnitPtr).p_MovementClass;
end;

class function TAUnit.SetTemplate(UnitPtr: Pointer; NewUnitInfoId: Word): Boolean;
var
  NewTemplatePtr: Pointer;
  UnitSt: PUnitStruct;
begin
  Result:= False;
  if UnitPtr <> nil then
  begin
    NewTemplatePtr:= Pointer(TAMem.UnitInfoId2Ptr(NewUnitInfoId));
    UnitSt:= UnitPtr;
    UnitSt.nUnitCategoryID:= NewUnitInfoId;
    UnitSt.p_UnitDef:= NewTemplatePtr;
    Result:= True;
  end;
end;

class function TAUnit.GetWeapon(UnitPtr : Pointer; index: LongWord): Word;
var
  Weapon: Pointer;
begin
  Result:= 0;
  if UnitPtr <> nil then
  begin
    case index of
      WEAPON_PRIMARY: Weapon := PUnitStruct(UnitPtr)^.p_Weapon1;
      WEAPON_SECONDARY: Weapon := PUnitStruct(UnitPtr)^.p_Weapon2;
      WEAPON_TERTIARY: Weapon := PUnitStruct(UnitPtr)^.p_Weapon3;
    end;
    if Weapon <> nil then
      Result:= PWeaponDef(Weapon)^.ucID;
  end;
end;

class function TAUnit.SetWeapon(UnitPtr : Pointer; index: LongWord; weaponId: Word): Boolean;
var
  Weapon: Pointer;
begin
  Result:= False;
  if UnitPtr <> nil then
  begin
    Weapon:= TAMem.GetWeapon(weaponId);
    case index of
      WEAPON_PRIMARY: PUnitStruct(UnitPtr)^.p_Weapon1 := Weapon;
      WEAPON_SECONDARY: PUnitStruct(UnitPtr)^.p_Weapon2 := Weapon;
      WEAPON_TERTIARY: PUnitStruct(UnitPtr)^.p_Weapon3 := Weapon;
    end;
    Result:= True;
  end;
end;

class function TAUnit.GetAttackerID(UnitPtr : Pointer): LongWord;
begin
result:= 0;
if PUnitStruct(UnitPtr).p_Attacker <> nil then
  result:= TAUnit.GetLongId(PUnitStruct(UnitPtr).p_Attacker);
end;

class function TAUnit.UpdateLos(UnitPtr : pointer): LongWord;
begin
  TA_UpdateUnitLOS(LongWord(UnitPtr));
  Result:= TA_UpdateLOS(LongWord(TAUnit.GetOwnerIndexBuildspotSafe(UnitPtr)), 0);
end;

class procedure TAUnit.Speech(UnitPtr : longword; speechtype: longword; speechtext: PChar);
begin
  PlaySound_UnitSpeech(UnitPtr, speechtype, speechtext);
end;

class procedure TAUnit.SoundEffectId(UnitPtr : longword; voiceid: longword);
begin
  PlaySound_EffectId(voiceid, UnitPtr);
end;

class function TAUnit.PlayGafAnim(BmpType: Byte; X, Z: Word; Glow, Smoke: Byte): Integer;
var
  Position : TPosition;
  GlowInt, SmokeInt : Integer;
begin
  Position := TAUnit.CreatePositionOfCoords(X, Z, 0);
  Position.Y := GetPosHeight(@Position);
  if Glow = 0 then
    GlowInt := -1
  else
    GlowInt := Glow - 1;

  if Smoke = 0 then
    SmokeInt := -1
  else
    SmokeInt := Smoke - 1;

  result := ShowExplodeGaf(@Position, ExtraAnimations[BmpType], GlowInt, SmokeInt);
end;

class function TAUnit.AtMouse: Pointer;
var
  Id: LongWord;
begin
  Result:= nil;
  Id:= GetUnitAtMouse;
  if (Id <> 0) and (TAUnit.Id2Ptr(Id) <> TAUnit.Id2Ptr(0)) then
    Result:= TAUnit.Id2Ptr(Id);
end;

class function TAUnit.CreatePositionOfCoords(X, Z, Y: Word): TPosition;
begin
  FillChar(Result, SizeOf(Result), 0);
  Result.X := X;
  Result.Z := Z;
  Result.Y := Y;
end;

class function TAUnit.Position2Grid(Position: TPosition; UnitInfoId: Word; out GridPosX, GridPosZ: SmallInt ): Boolean;
var
  PosX, PosZ: Integer;
begin
  Result := False;
  PosX := MakeLong(Position.x_, Position.X);
  GridPosX := (PosX - (SmallInt(PGameUnitfInfo(TAMem.UnitInfoId2Ptr(UnitInfoId)).nFootPrintX shl 19)) + $80000) shr 20;
  PosZ := MakeLong(0, Position.Z);
  GridPosZ := (PosZ - (SmallInt(PGameUnitfInfo(TAMem.UnitInfoId2Ptr(UnitInfoId)).nFootPrintZ shl 19)) + $80000) shr 20;
  if (GridPosX < 0) or (GridPosZ < 0) then
    Exit
  else
    Result:= True;
end;

class function TAUnit.GetCurrentSpeed(UnitPtr: Pointer): Cardinal;
begin
  result := 0;
  if PUnitStruct(UnitPtr).p_MovementClass <> nil then
  begin
    if PUnitStruct(UnitPtr).lTerrainLevel = 4 then
      result := Trunc(((PMoveClass(PUnitStruct(UnitPtr).p_MovementClass).lCurrentSpeed) /
                        (PGameUnitfInfo(PUnitStruct(UnitPtr).p_UnitDef).lMaxSpeedRaw)) * 100)
    else
      result := Trunc(((PMoveClass(PUnitStruct(UnitPtr).p_MovementClass).lCurrentSpeed) /
                        Trunc((PGameUnitfInfo(PUnitStruct(UnitPtr).p_UnitDef).lMaxSpeedRaw) / 2)) * 100);
    if result > 100 then
      result := 100;
  end;
end;

class function TAUnit.TestBuildSpot(PlayerIndex: Byte; UnitInfoId: Word; nPosX, nPosZ: Word ): Boolean;
var
  TestPos: LongWord;
  UnitInfoSt: PGameUnitfInfo;
  PlayerSt: PPlayerStruct;
  GridPosX, GridPosZ: SmallInt;
  Position: TPosition;
begin
  Result:= False;
  UnitInfoSt:= Pointer(TAMem.UnitInfoId2Ptr(UnitInfoId));
  PlayerSt:= TAPlayer.GetPlayerByIndex(PlayerIndex);
  Position.X := nPosX;
  Position.Y := nPosZ;
  if (UnitInfoSt <> nil) and (PlayerSt <> nil) then
    if TAUnit.Position2Grid(Position, UnitInfoId, GridPosX, GridPosZ ) then
    begin
      TestPos:= MakeLong(GridPosX, GridPosZ);
      Result:= (TestGridSpot(UnitInfoSt, TestPos, 0, PlayerIndex) = 1);
    end;
end;

class function TAUnit.CreateUnit( OwnerIndex: LongWord;
                              UnitInfoId: LongWord;
                              Position: TPosition;
                              Turn: PTurn;
                              TurnZOnly: Boolean;
                              RandomTurnZ: Boolean;
                              UnitState: LongWord;
                              FullHp: LongWord): Pointer;
var
  UnitSt: PUnitStruct;
  PosX, PosZ, PosY: LongWord;
begin
  PosX := MakeLong(Position.x_, Position.X);
  PosZ := MakeLong(Position.z_, Position.Z);
  PosY := MakeLong(Position.y_, Position.Y);

  Result:= Pointer(Unit_Create(OwnerIndex, UnitInfoId, PosX, PosY, PosZ, FullHp, UnitState, 0));

  if (Result <> nil) then
  begin
    UnitSt:= Pointer(Result);
    if (Turn <> nil) then
    begin
      UnitSt.Turn.Z:= Turn.Z;
      if not TurnZOnly then
      begin
        UnitSt.Turn.X:= Turn.X;
        UnitSt.Turn.Y:= Turn.Y;
      end;
    end else
    if RandomTurnZ then
    begin
      UnitSt.Turn.Z:= Random(High(SmallInt));
    end;
  end;
end;

class Function TAUnit.GetBuildTimeLeft(UnitPtr : Pointer) : single;
begin
result := PUnitStruct(UnitPtr).lBuildTimeLeft;
end;

class procedure TAUnit.Kill(UnitPtr : Pointer; deathtype: byte);
begin
  if UnitPtr <> nil then
  begin
    case deathtype of
      0 : Unit_MakeDamage_(UnitPtr, UnitPtr, 30000, 3, 0);
      1 : Unit_KillMakeDamage(UnitPtr, 0);
      2 : Unit_KillMakeDamage(UnitPtr, 1);
      4 : TAUnit.CreateOrder(UnitPtr, nil, Action_SelfDestruct, nil, 1, 0, 0);
      5 : PUnitStruct(UnitPtr).lUnitStateMask:= PUnitStruct(UnitPtr).lUnitStateMask or $4000;
    end;
    if (deathtype >= 1) and (deathtype <= 3) then
    begin
      PUnitStruct(UnitPtr).lUnitStateMask:= PUnitStruct(UnitPtr).lUnitStateMask or $4000;
      Unit_Kill(UnitPtr, 3);
    end;
  end;
end;

class procedure TAUnit.SwapByKill(UnitPtr: Pointer; newUnitInfoId: Word);
var
  UnitSt: PUnitStruct;
  Position: TPosition;
  Turn: TTurn;
begin
  if UnitPtr <> nil then
  begin
    UnitSt:= UnitPtr;
    Position:= UnitSt.Position;
    Turn:= UnitSt.Turn;
    if TAUnit.CreateUnit(UnitSt.cMyLOSPlayerID, newUnitInfoId, Position, @Turn, True, False, 1, 1) <> nil then
      TAUnit.Kill(UnitPtr, 5);
  end;
end;

class function TAUnit.GetCurrentOrder(UnitPtr: Pointer): TTAActionType;
var
  UnitSt: PUnitStruct;
begin
  UnitSt:= UnitPtr;
  if UnitSt.p_UnitOrders <> nil then
    Result := TTAActionType(PUnitOrder(UnitSt.p_UnitOrders).cOrderType)
  else
    Result := Action_NoResult;
end;

class function TAUnit.GetCurrentOrderParams(UnitPtr: Pointer; Par: Byte): Cardinal;
begin
  Result := 0;
  if PUnitStruct(UnitPtr).p_UnitOrders <> nil then
  begin
    if Par = 0 then
      Result := PUnitOrder(PUnitStruct(UnitPtr).p_UnitOrders).lPar1
    else
      Result := PUnitOrder(PUnitStruct(UnitPtr).p_UnitOrders).lPar2;
  end;
end;

class function TAUnit.GetCurrentOrderState(UnitPtr: Pointer): Cardinal;
begin
  Result := 0;
  if PUnitStruct(UnitPtr).p_UnitOrders <> nil then
    Result := PUnitOrder(PUnitStruct(UnitPtr).p_UnitOrders).Order_State;
end;

class function TAUnit.EditCurrentOrderParams(UnitPtr: Pointer; Par: Byte; NewValue: LongWord): Boolean;
begin
  Result := False;
  if PUnitStruct(UnitPtr).p_UnitOrders <> nil then
  begin
    if Par = 1 then
      PUnitOrder(PUnitStruct(UnitPtr).p_UnitOrders).lPar1 := NewValue
    else
      PUnitOrder(PUnitStruct(UnitPtr).p_UnitOrders).lPar2 := NewValue;
    Result := True;
  end;
end;

class function TAUnit.CreateOrder(UnitPtr: Pointer; TargetUnitPtr: Pointer; ActionType: TTAActionType; Position: PPosition; ShiftKey: Byte; Par1: LongWord; Par2: LongWord): LongInt;
begin
  Result:= Order2Unit(Ord(ActionType), ShiftKey, UnitPtr, TargetUnitPtr, Position, Par1, Par2);
end;

class Function TAUnit.GetCOBDataPtr(UnitPtr : Pointer) : Pointer;
begin
result := PUnitStruct(UnitPtr).p_AlmostCOBStruct;
end;

class function TAUnit.CallCobProcedure(UnitPtr: Pointer; ProcName: String; Par1, Par2, Par3, Par4: PLongWord): Cardinal;
var
  ParamsCount: LongWord;
  UnitSt: PUnitStruct;
  lPar1, lPar2, lPar3, lPar4: LongWord;
begin
  Result := 0;
  if UnitPtr <> nil then
  begin
    ParamsCount:= 0;
    if Par1 <> nil then
    begin
      lPar1 := PLongWord(Par1)^;
      Inc(ParamsCount);
    end else
      lPar1 := 0;
    if Par2 <> nil then
    begin
      lPar2 := PLongWord(Par2)^;
      Inc(ParamsCount);
    end else
      lPar2 := 0;
    if Par3 <> nil then
    begin
      lPar3 := PLongWord(Par3)^;
      Inc(ParamsCount);
    end else
      lPar3 := 0;
    if Par4 <> nil then
    begin
      lPar4 := PLongWord(Par4)^;
      Inc(ParamsCount);
    end else
      lPar4 := 0;

    UnitSt:= UnitPtr;
    Result := Script_RunScript ( 0, 0, LongWord(UnitSt.p_AlmostCOBStruct),
                                 lPar4, lPar3, lPar2, lPar1,
                                 ParamsCount,
                                 0, 0,
                                 PAnsiChar(ProcName) );
  end;
end;

class function TAUnit.CallCobWithCallback(UnitPtr: Pointer; ProcName: String; Par1, Par2, Par3, Par4: LongWord): Cardinal;
var
  UnitSt: PUnitStruct;
begin
  Result := 0;
  if UnitPtr <> nil then
  begin
    UnitSt := UnitPtr;
    Result := Script_ProcessCallback( nil, nil, LongWord(UnitSt.p_AlmostCOBStruct),
                                      @Par4, @Par3, @Par2, @Par1,
                                      PAnsiChar(ProcName) );
    if Result = 1 then
      Result := Par1;
  end;
end;

class function TAUnit.GetCobString(UnitPtr: Pointer): String;
begin
end;

class Function TAUnit.GetOwnerPtr(UnitPtr : Pointer) : Pointer;
begin
result := PUnitStruct(UnitPtr).p_Owner;
end;

class Function TAUnit.GetOwnerIndex(UnitPtr : Pointer) : Integer;
begin
result := PUnitStruct(UnitPtr).cMyLOSPlayerID;
end;

class Function TAUnit.GetOwnerIndexBuildspotSafe(UnitPtr : Pointer): Integer;
begin
  if PPlayerStruct(PUnitStruct(UnitPtr).p_Owner).en_cPlayerType = Player_LocalAI then
    Result := PUnitStruct(UnitPtr).cOwningPlayerID
  else
    Result := PPlayerStruct(PUnitStruct(UnitPtr).p_Owner).cPlayerIndexZero;
end;

class Function TAUnit.GetId(UnitPtr : Pointer) : Word;
begin
result := Word(PUnitStruct(UnitPtr).lUnitInGameIndex);
end;

class Function TAUnit.GetLongId(UnitPtr : Pointer) : LongWord;
begin
result := PUnitStruct(UnitPtr).lUnitInGameIndex;
end;

class Function TAUnit.Id2Ptr(LongUnitId : LongWord) : Pointer;
begin
Result:= nil;
if (Word(LongUnitId) > TAData.MaxUnitLimit * MAXPLAYERCOUNT) then exit;
result := Pointer(LongWord(TAData.UnitsPtr) + SizeOf(TUnitStruct)*Word(LongUnitId));
end;

class Function TAUnit.Id2LongId(UnitId: Word) : LongWord;
var
  UnitPtr: Pointer;
begin
  result:= 0;
  UnitPtr:= TAUnit.Id2Ptr(UnitId);
  if PUnitStruct(UnitPtr).nUnitCategoryID <> 0 then
    result:= PUnitStruct(UnitPtr).lUnitInGameIndex;
end;

class function TAUnit.GetUnitInfoId(UnitPtr: Pointer): Word;
begin
result:= PUnitStruct(UnitPtr).nUnitCategoryID;
end;

class function TAUnit.GetUnitInfoPtr(UnitPtr: Pointer) : Pointer;
begin
result:= PUnitStruct(UnitPtr).p_UnitDef;
end;

class Function TAUnit.IsOnThisComp(UnitPtr : Pointer; IncludeAI: Boolean) : Boolean;
var
  playerPtr : Pointer;
  TAPlayerType : TTAPlayerType;
begin
result:= False;
try
  playerPtr := TAUnit.GetOwnerPtr(Pointer(UnitPtr));
  //playerPtr := TAPlayer.GetPlayerByIndex(TAunit.GetOwnerIndex(UnitPtr));
  if playerPtr <> nil then
  begin
    TAPlayerType := TAPlayer.PlayerType(playerPtr);
    case TAPlayerType of
      Player_LocalHuman:
        result := True;
      Player_LocalAI:
        if IncludeAI then
          result := True;
    end;
  end;
except
  result := False;
end;
end;

class Function TAUnit.IsAllied(UnitPtr: Pointer; UnitId: LongWord): Byte;
var
  Unit2Ptr, playerPtr: Pointer;
  playerindex: LongWord;
begin
  playerPtr := TAUnit.GetOwnerPtr(unitptr);
  Unit2Ptr := TAUnit.Id2Ptr(Word(UnitId));
  playerIndex := TAUnit.GetOwnerIndex(Unit2Ptr);
  result := BoolValues[TAPlayer.GetAlliedState(playerPtr,playerIndex)];
end;

class Function TAUnit.IsFromAlly(UnitPtr: Pointer): Byte;
var
  playerPtr: Pointer;
  thisPlayer: Pointer;
begin
  // prawdziwy owner
  playerPtr := TAUnit.GetOwnerPtr(unitptr);
  thisPlayer := TAData.PlayersStructPtr;
  result := BoolValues[TAPlayer.GetAlliedState(playerPtr, 0)];
end;

class Function TAUnit.Team(UnitId: LongWord): Integer;
var
  Unit2Ptr: Pointer;
begin
  Unit2Ptr := TAUnit.Id2Ptr(Word(UnitId));
  result := TAUnit.GetOwnerIndex(Unit2Ptr);
end;

class Function TAUnit.IsRemoteIdLocal(UnitPtr: Pointer; remoteUnitId: PLongWord; out Local: Boolean): LongWord;
begin
  Local:= False;
  Result:= 0;
  if remoteUnitId <> nil then
  begin
    Local:= False;
    Result:= Word(remoteUnitId);
  end else
  begin
    if UnitPtr <> nil then
    begin
      Local:= IsOnThisComp(Pointer(UnitPtr), True);
      Result:= TAUnit.GetLongId(Pointer(UnitPtr));
    end;
  end;
end;

class function TAUnit.GrantUnitInfo(UnitPtr: Pointer; State: Byte; remoteUnitId: PLongWord): Boolean;
var
  tmpUnitInfo: TCustomUnitInfo;
  arrIdx: Integer;
  unitId: LongWord;
  local: Boolean;
  templateFound: Boolean;
begin
  Result:= False;
  unitId:= IsRemoteIdLocal(UnitPtr, remoteUnitId, Local);
  TemplateFound:= TAUnit.SearchCustomUnitInfos(unitId, remoteUnitId, Local, arrIdx);

  if (not TemplateFound) and (arrIdx = -1) then
  begin
    TmpUnitInfo.unitId:= unitId;
    if Local then
      TmpUnitInfo.InfoPtrOld:= LongWord(PUnitStruct(UnitPtr).p_UnitDef)
    else
      begin
        TmpUnitInfo.unitIdRemote:= LongWord(remoteUnitId);
       // TmpUnitInfo.InfoPtrOld:= PLongWord(LongWord(TAMem.GetUnitStruct(unitId))+UnitStruct_UNITINFO_p)^;
        TmpUnitInfo.InfoPtrOld:= LongWord(TAUnit.GetUnitInfoPtr(TAUnit.Id2Ptr(unitId)));
      end;
    TmpUnitInfo.InfoStruct:= PGameUnitfInfo(TmpUnitInfo.InfoPtrOld)^;
    arrIdx:= CustomUnitInfos.Add(TmpUnitInfo);
    TemplateFound:= True;
  end else
    if (not Local) then
    begin
     // if state = 1 then
    //  begin
        // received remote setupgradeable, but unit with the same short id already exists in array,
        // let's overwrite array element with new data, so we preserve pointers to other elements, hopefully not crashing TA
        CustomUnitInfosArray[arrIdx].unitIdRemote:= LongWord(remoteUnitId);
        //CustomUnitInfosArray[arrIdx].InfoPtrOld:= TAMem.GetTemplatePtr(PWord(LongWord(TAMem.GetUnitStruct(Word(remoteUnitId)))+UnitStruct_UnitINFOID)^);
        CustomUnitInfosArray[arrIdx].InfoPtrOld:= LongWord(TAMem.UnitInfoId2Ptr(TAUnit.GetUnitInfoId(TAUnit.Id2Ptr(Word(remoteUnitId)))));
        CustomUnitInfosArray[arrIdx].InfoStruct:= PGameUnitfInfo(CustomUnitInfosArray[arrIdx].InfoPtrOld)^;
        TemplateFound:= True;
    //  end;
    end;

    if TemplateFound then
    begin
      Result:= True;
      if state = 0 then
      begin
        if Local then
        //  PLongWord(LongWord(Pointer(UnitPtr))+UnitStruct_UNITINFO_p)^ := CustomUnitInfosArray[arrIdx].InfoPtrOld
          PUnitStruct(UnitPtr).p_UnitDef := @CustomUnitInfosArray[arrIdx].InfoPtrOld
        else
        //  PLongWord(LongWord(TAMem.GetUnitPtr(unitId))+UnitStruct_UNITINFO_p)^ := CustomUnitInfosArray[arrIdx].InfoPtrOld;
          PUnitStruct(TAUnit.Id2Ptr(unitId)).p_UnitDef := @CustomUnitInfosArray[arrIdx].InfoPtrOld;
      end else
      begin
        if Local then
          PUnitStruct(UnitPtr).p_UnitDef := @CustomUnitInfosArray[arrIdx].InfoStruct
        else
          PUnitStruct(TAUnit.Id2Ptr(unitId)).p_UnitDef := @CustomUnitInfosArray[arrIdx].InfoStruct;
      end;
    end;
end;

class function TAUnit.SearchCustomUnitInfos(unitId: LongWord; remoteUnitId: PLongWord; Local: Boolean; out Index: Integer ): Boolean;
var
  i: Integer;
  TmpUnitInfo: PCustomUnitInfo;
begin
  Result:= False;
  Index:= -1;
  for i := CustomUnitInfos.Count-1 downto 0 do
  begin
    TmpUnitInfo:= @CustomUnitInfosArray[i];
    if TmpUnitInfo^.unitId = unitId then
    begin
      if Local then
      begin
        Index:= i;
        Result:= True;
        Exit;
      end else
        if LongWord(TmpUnitInfo^.unitIdRemote) = LongWord(remoteUnitId) then
        begin
          Index:= i;
          Result:= True;
          Exit;
        end else
          Continue;
      end;
    end;
end;

class function TAUnit.GetUnitInfoField(UnitPtr: Pointer; fieldType: TUnitInfoExtensions; remoteUnitId: PLongWord): LongWord;
var
  arrIdx: Integer;
  unitId: LongWord;
  local: Boolean;
  custTmplFound: boolean;
  engineTmpl, UseTemplate: PGameUnitfInfo;
begin
  Result:= 0;
  unitId:= IsRemoteIdLocal(UnitPtr, remoteUnitId, local);
  custTmplFound:= TAUnit.SearchCustomUnitInfos(unitId, remoteUnitId, local, arrIdx);

  // if getter was done for a unit that has no custom unitinfo template
  // we are going to read field value from TA's GameUnitInfo array
  if not custTmplFound then
  begin
//    engineTmpl:= Pointer(TAMem.GetTemplatePtr(GetUnitInfoId(TAMem.GetUnitPtr(Word(unitId)))));
    engineTmpl:= TAMem.UnitInfoId2Ptr(GetUnitInfoId(TAUnit.Id2Ptr(Word(unitId))));
    if (engineTmpl <> nil) then
      UseTemplate := engineTmpl
    else
      Exit;
  end else
    UseTemplate := @CustomUnitInfosArray[arrIdx].InfoStruct;

  case fieldType of
    Ext_MAXHEALTH       : Result := UseTemplate.nMaxHP;
    Ext_HEALTIME        : Result := UseTemplate.nHealTime;

    Ext_MAXSPEED        : Result := Trunc(UseTemplate.lMaxSpeedRaw * 100);
    Ext_ACCELERATION    : Result := Trunc(UseTemplate.lAcceleration * 100);
    Ext_BRAKERATE       : Result := Trunc(UseTemplate.lBrakeRate * 100);
    Ext_TURNRATE        : Result := UseTemplate.nTurnRate;
    Ext_CRUISEALT       : Result := UseTemplate.nCruiseAlt;
    Ext_MANEUVERLEASH   : Result := UseTemplate.nManeuverLeashLen;
    Ext_ATTACKRUNLEN    : Result := UseTemplate.nAttackRunLength;
    Ext_MAXWATERDEPTH   : Result := UseTemplate.nMaxWaterDepth;
    Ext_MINWATERDEPTH   : Result := UseTemplate.nMinWaterDepth;
    Ext_MAXSLOPE        : Result := UseTemplate.cMaxSlope;
    Ext_MAXWATERSLOPE   : Result := UseTemplate.cMaxWaterSlope;
    Ext_WATERLINE       : Result := UseTemplate.cWaterLine;

    Ext_TRANSPORTSIZE   : Result := UseTemplate.cTransportSize;
    Ext_TRANSPORTCAP    : Result := UseTemplate.cTransportCap;

    Ext_BANKSCALE       : Result := UseTemplate.lBankScale;
    Ext_KAMIKAZEDIST    : Result := UseTemplate.nKamikazeDistance;
    Ext_DAMAGEMODIFIER  : Result := UseTemplate.lDamageModifier;

    Ext_WORKERTIME      : Result := UseTemplate.nWorkerTime;
    Ext_BUILDDIST       : Result := UseTemplate.nBuildDistance;

    Ext_SIGHTDIST       : Result := UseTemplate.nSightDistance;
    Ext_RADARDIST       : Result := UseTemplate.nRadarDistance;
    Ext_SONARDIST       : Result := UseTemplate.nSonarDistance;
    Ext_MINCLOAKDIST    : Result := UseTemplate.nMinCloakDistance;
    Ext_RADARDISTJAM    : Result := UseTemplate.nRadarDistanceJam;
    Ext_SONARDISTJAM    : Result := UseTemplate.nSonarDistanceJam;

    Ext_MAKESMETAL      : Result := UseTemplate.cMakesMetal;
    Ext_FENERGYMAKE     : Result := Trunc(UseTemplate.fEnergyMake * 100);
    Ext_FMETALMAKE      : Result := Trunc(UseTemplate.fMetalMake * 100);
    Ext_FENERGYUSE      : Result := Trunc(UseTemplate.fEnergyUse * 100);
    Ext_FMETALUSE       : Result := Trunc(UseTemplate.fMetalUse * 100);
    Ext_FENERGYSTOR     : Result := UseTemplate.lEnergyStorage;
    Ext_FMETALSTOR      : Result := UseTemplate.lMetalStorage;
    Ext_FWINDGENERATOR  : Result := Trunc(UseTemplate.fWindGenerator * 100);
    Ext_FTIDALGENERATOR : Result := Trunc(UseTemplate.fTidalGenerator * 100);
    Ext_FCLOAKCOST      : Result := Trunc(UseTemplate.fCloakCost * 100);
    Ext_FCLOAKCOSTMOVE  : Result := Trunc(UseTemplate.fCloakCostMoving * 100);

    Ext_BUILDCOSTMETAL  : Result := Trunc(UseTemplate.lBuildCostMetal);
    Ext_BUILDCOSTENERGY : Result := Trunc(UseTemplate.lBuildCostEnergy);

// 1 ?
// 2 standingfireorder
// 3 ?
// 4 init_cloaked
// 5 downloadable
    Ext_BUILDER         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 6)) > 0 );
// 7 zbuffer
    Ext_STEALTH         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 8)) > 0 );
    Ext_ISAIRBASE       : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 9)) > 0 );
    Ext_TARGETTINGUPGRADE   : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 10)) > 0 );
    Ext_CANFLY          : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 11)) > 0 );
    Ext_CANHOVER        : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 12)) > 0 );
    Ext_TELEPORTER      : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 13)) > 0 );
    Ext_HIDEDAMAGE      : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 14)) > 0 );
    Ext_SHOOTME         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 15)) > 0 );
// 16 UNIT HAS ANY WEAPON
// 17 armoredstate
// 18 activatewhenbuilt
    Ext_FLOATER         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 19)) > 0 );
    Ext_UPRIGHT         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 20)) > 0 );
    Ext_AMPHIBIOUS      : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 21)) > 0 );
// 22 ?
// 23 internal command reload -> sub_42D1F0. probably reloads cob script 
// 24 isfeature
// 25 noshadow
    Ext_IMMUNETOPARALYZER  : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 26)) > 0 );
    Ext_HOVERATTACK     : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 27)) > 0 );
    Ext_KAMIKAZE        : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 28)) > 0 );
    Ext_ANTIWEAPONS     : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 29)) > 0 );
    Ext_DIGGER          : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 30)) > 0 );
// 31 has GUI (does gui file in guis folder exists with name of tested unit) ? sub_42d2e0

    Ext_ONOFFABLE       : Result := Byte((UseTemplate.UnitTypeMask2 and 4) > 0 );
    Ext_CANSTOP         : Result := Byte((UseTemplate.UnitTypeMask2 and 8) > 0 );
    Ext_CANATTACK       : Result := Byte((UseTemplate.UnitTypeMask2 and 16) > 0 );
    Ext_CANGUARD        : Result := Byte((UseTemplate.UnitTypeMask2 and 32) > 0 );
    Ext_CANPATROL       : Result := Byte((UseTemplate.UnitTypeMask2 and 64) > 0 );
    Ext_CANMOVE         : Result := Byte((UseTemplate.UnitTypeMask2 and 128) > 0 );
    Ext_CANLOAD         : Result := Byte((UseTemplate.UnitTypeMask2 and 256) > 0 );
    Ext_CANRECLAMATE    : Result := Byte((UseTemplate.UnitTypeMask2 and 1024) > 0 );
    Ext_CANRESURRECT    : Result := Byte((UseTemplate.UnitTypeMask2 and 2048) > 0 );
    Ext_CANCAPTURE      : Result := Byte((UseTemplate.UnitTypeMask2 and 4096) > 0 );
    Ext_CANDGUN         : Result := Byte((UseTemplate.UnitTypeMask2 and 16384) > 0 );
    Ext_SHOWPLAYERNAME  : Result := Byte((UseTemplate.UnitTypeMask2 and 131072) > 0 );
    Ext_COMMANDER       : Result := Byte((UseTemplate.UnitTypeMask2 and 262144) > 0 );
    Ext_CANTBERANSPORTED: Result := Byte((UseTemplate.UnitTypeMask2 and 524288) > 0 );
  end;
end;

class function TAUnit.SetUnitInfoField(UnitPtr: Pointer; fieldType: TUnitInfoExtensions; value: LongWord; remoteUnitId: PLongWord): Boolean;
procedure SetUnitTypeMask(arrIdx: Integer; unittypemask: byte; onoff: boolean; mask: LongWord);
begin
  case onoff of
  False :
    begin
    if unittypemask = 0 then
      CustomUnitInfosArray[arrIdx].InfoStruct.UnitTypeMask := CustomUnitInfosArray[arrIdx].InfoStruct.UnitTypeMask and not mask
    else
      CustomUnitInfosArray[arrIdx].InfoStruct.UnitTypeMask2 := CustomUnitInfosArray[arrIdx].InfoStruct.UnitTypeMask2 and not mask;
    end;
  True :
    begin
    if unittypemask = 0 then
      CustomUnitInfosArray[arrIdx].InfoStruct.UnitTypeMask := CustomUnitInfosArray[arrIdx].InfoStruct.UnitTypeMask or mask
    else
      CustomUnitInfosArray[arrIdx].InfoStruct.UnitTypeMask2 := CustomUnitInfosArray[arrIdx].InfoStruct.UnitTypeMask2 or mask;
    end;
  end;
end;

var
  arrIdx: Integer;
  unitId: LongWord;
  local: Boolean;
  MovementClassStruct: PMoveInfoClassStruct;
begin
  Result:= False;
  unitId:= IsRemoteIdLocal(UnitPtr, remoteUnitId, local);
  if TAUnit.SearchCustomUnitInfos(unitId, remoteUnitId, local, arrIdx) then
  begin
    case fieldType of
      Ext_SOUNDCTGR : CustomUnitInfosArray[arrIdx].InfoStruct.nSoundCategory := Word(value);
      Ext_MOVEMENTCLASS_SAFE..Ext_MOVEMENTCLASS :
        begin
        // movement class information is stored in both - specific and global template
        // we fix both of them to make TA engine happy
        CustomUnitInfosArray[arrIdx].InfoStruct.p_MovementClass:= TAMem.GetMovementClassPtr(Word(value));
        MovementClassStruct:= CustomUnitInfosArray[arrIdx].InfoStruct.p_MovementClass;
        if MovementClassStruct.pName <> nil then
        begin
          CustomUnitInfosArray[arrIdx].InfoStruct.nMaxWaterDepth:= MovementClassStruct.nMaxWaterDepth;
          CustomUnitInfosArray[arrIdx].InfoStruct.nMinWaterDepth:= MovementClassStruct.nMinWaterDepth;
          CustomUnitInfosArray[arrIdx].InfoStruct.cMaxSlope:= MovementClassStruct.cMaxSlope;
          CustomUnitInfosArray[arrIdx].InfoStruct.cMaxWaterSlope:= MovementClassStruct.cMaxWaterSlope;
          if FieldType = Ext_MOVEMENTCLASS_SAFE then
            CreateMovementClass(LongWord(TAUnit.Id2Ptr(Word(unitId))));
        end else
            Exit;
        end;
      Ext_MAXHEALTH       : CustomUnitInfosArray[arrIdx].InfoStruct.nMaxHP := Word(value);
      Ext_HEALTIME        : CustomUnitInfosArray[arrIdx].InfoStruct.nHealTime := Word(value);

      Ext_MAXSPEED        : CustomUnitInfosArray[arrIdx].InfoStruct.lMaxSpeedRaw := value;
      Ext_ACCELERATION    : CustomUnitInfosArray[arrIdx].InfoStruct.lAcceleration := value / 100;
      Ext_BRAKERATE       : CustomUnitInfosArray[arrIdx].InfoStruct.lBrakeRate := value / 100;
      Ext_TURNRATE        : CustomUnitInfosArray[arrIdx].InfoStruct.nTurnRate := Word(value);
      Ext_CRUISEALT       : CustomUnitInfosArray[arrIdx].InfoStruct.nCruiseAlt := Word(value);
      Ext_MANEUVERLEASH   : CustomUnitInfosArray[arrIdx].InfoStruct.nManeuverLeashLen := Word(value);
      Ext_ATTACKRUNLEN    : CustomUnitInfosArray[arrIdx].InfoStruct.nAttackRunLength := Word(value);
      Ext_MAXWATERDEPTH   : CustomUnitInfosArray[arrIdx].InfoStruct.nMaxWaterDepth := SmallInt(value);
      Ext_MINWATERDEPTH   : CustomUnitInfosArray[arrIdx].InfoStruct.nMinWaterDepth := SmallInt(value);
      Ext_MAXSLOPE        : CustomUnitInfosArray[arrIdx].InfoStruct.cMaxSlope := ShortInt(value);
      Ext_MAXWATERSLOPE   : CustomUnitInfosArray[arrIdx].InfoStruct.cMaxWaterSlope := ShortInt(value);
      Ext_WATERLINE       : CustomUnitInfosArray[arrIdx].InfoStruct.cWaterLine := Byte(value);

      Ext_TRANSPORTSIZE   : CustomUnitInfosArray[arrIdx].InfoStruct.cTransportSize := Byte(value);
      Ext_TRANSPORTCAP    : CustomUnitInfosArray[arrIdx].InfoStruct.cTransportCap := Byte(value);

      Ext_BANKSCALE       : CustomUnitInfosArray[arrIdx].InfoStruct.lBankScale := value;
      Ext_KAMIKAZEDIST    : CustomUnitInfosArray[arrIdx].InfoStruct.nKamikazeDistance := Word(value);
      Ext_DAMAGEMODIFIER  : CustomUnitInfosArray[arrIdx].InfoStruct.lDamageModifier := value;

      Ext_WORKERTIME      : CustomUnitInfosArray[arrIdx].InfoStruct.nWorkerTime := Word(value);
      Ext_BUILDDIST       : CustomUnitInfosArray[arrIdx].InfoStruct.nBuildDistance := Word(value);

      Ext_SIGHTDIST :
        begin
        CustomUnitInfosArray[arrIdx].InfoStruct.nSightDistance := Word(value);
        TAUnit.UpdateLos(TAUnit.Id2Ptr(Word(unitId)));
        end;
      Ext_RADARDIST :
        begin
        CustomUnitInfosArray[arrIdx].InfoStruct.nRadarDistance := Word(value);
        TAUnit.UpdateLos(TAUnit.Id2Ptr(Word(unitId)));
        end;
      Ext_SONARDIST       : CustomUnitInfosArray[arrIdx].InfoStruct.nSonarDistance := Word(value);
      Ext_MINCLOAKDIST    : CustomUnitInfosArray[arrIdx].InfoStruct.nMinCloakDistance := Word(value);
      Ext_RADARDISTJAM    : CustomUnitInfosArray[arrIdx].InfoStruct.nRadarDistanceJam := Word(value);
      Ext_SONARDISTJAM    : CustomUnitInfosArray[arrIdx].InfoStruct.nSonarDistanceJam := Word(value);

      Ext_MAKESMETAL      : CustomUnitInfosArray[arrIdx].InfoStruct.cMakesMetal := Byte(value);
      Ext_FENERGYMAKE     : CustomUnitInfosArray[arrIdx].InfoStruct.fEnergyMake := value / 100;
      Ext_FMETALMAKE      : CustomUnitInfosArray[arrIdx].InfoStruct.fMetalMake := value / 100;
      Ext_FENERGYUSE      : CustomUnitInfosArray[arrIdx].InfoStruct.fEnergyUse := value / 100;
      Ext_FMETALUSE       : CustomUnitInfosArray[arrIdx].InfoStruct.fMetalUse := value / 100;
      Ext_FENERGYSTOR     : CustomUnitInfosArray[arrIdx].InfoStruct.lEnergyStorage := value;
      Ext_FMETALSTOR      : CustomUnitInfosArray[arrIdx].InfoStruct.lEnergyStorage := value;
      Ext_FWINDGENERATOR  : CustomUnitInfosArray[arrIdx].InfoStruct.fWindGenerator := value / 100;
      Ext_FTIDALGENERATOR : CustomUnitInfosArray[arrIdx].InfoStruct.fTidalGenerator := value / 100;
      Ext_FCLOAKCOST      : CustomUnitInfosArray[arrIdx].InfoStruct.fCloakCost := value / 100;
      Ext_FCLOAKCOSTMOVE  : CustomUnitInfosArray[arrIdx].InfoStruct.fCloakCostMoving := value / 100;

      Ext_BMCODE          : CustomUnitInfosArray[arrIdx].InfoStruct.cBMCode := value;

      Ext_EXPLODEAS       : CustomUnitInfosArray[arrIdx].InfoStruct.p_ExplodeAs := PLongWord(TAMem.GetWeapon(value));
      Ext_SELFDSTRAS      : CustomUnitInfosArray[arrIdx].InfoStruct.p_SelfDestructAsAs := PLongWord(TAMem.GetWeapon(value));

// 1 ?
// 2 standingfireorder
// 3 ?
// 4 init_cloaked
// 5 downloadable
      Ext_BUILDER         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 6);
// 7 zbuffer
      Ext_STEALTH         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 8);
      Ext_ISAIRBASE       : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 9);
      Ext_TARGETTINGUPGRADE   : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 10);
      Ext_CANFLY          : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 11);
      Ext_CANHOVER        : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 12);
      Ext_TELEPORTER      : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 13);
      Ext_HIDEDAMAGE      : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 14);
      Ext_SHOOTME         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 15);
// 16 UNIT HAS ANY WEAPON
// 17 armoredstate
// 18 activatewhenbuilt
      Ext_FLOATER         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 19);
      Ext_UPRIGHT         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 20);
      Ext_AMPHIBIOUS      : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 21);
// 22 ?
// 23 internal command reload -> sub_42D1F0. probably reloads cob script 
// 24 isfeature
// 25 noshadow
      Ext_IMMUNETOPARALYZER  : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 26);
      Ext_HOVERATTACK     : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 27);
      Ext_KAMIKAZE        : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 28);
      Ext_ANTIWEAPONS     : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 29);
      Ext_DIGGER          : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 30);
// 31 has GUI (does gui file in guis folder exists with name of tested unit) ? sub_42d2e0

      Ext_ONOFFABLE       : SetUnitTypeMask(arrIdx, 1, (value = 1), 4);
      Ext_CANSTOP         : SetUnitTypeMask(arrIdx, 1, (value = 1), 8);
      Ext_CANATTACK       : SetUnitTypeMask(arrIdx, 1, (value = 1), 16);
      Ext_CANGUARD        : SetUnitTypeMask(arrIdx, 1, (value = 1), 32);
      Ext_CANPATROL       : SetUnitTypeMask(arrIdx, 1, (value = 1), 64);
      Ext_CANMOVE         : SetUnitTypeMask(arrIdx, 1, (value = 1), 128);
      Ext_CANLOAD         : SetUnitTypeMask(arrIdx, 1, (value = 1), 256);
      Ext_CANRECLAMATE    : SetUnitTypeMask(arrIdx, 1, (value = 1), 1024);
      Ext_CANRESURRECT    : SetUnitTypeMask(arrIdx, 1, (value = 1), 2048);
      Ext_CANCAPTURE      : begin SetUnitTypeMask(arrIdx, 1, (value = 1), 4096); SetUnitTypeMask(arrIdx, 1, (value = 1), 8192); end;
      Ext_CANDGUN         : SetUnitTypeMask(arrIdx, 1, (value = 1), 16384);
      Ext_SHOWPLAYERNAME  : SetUnitTypeMask(arrIdx, 1, (value = 1), 131072);
      Ext_COMMANDER       : SetUnitTypeMask(arrIdx, 1, (value = 1), 262144);
      Ext_CANTBERANSPORTED: SetUnitTypeMask(arrIdx, 1, (value = 1), 524288);
    end;
    Result:= True;
  end;
end;

class Function TAUnits.CreateMinions(UnitPtr: Pointer; Amount: Byte; Pattern: TMinionsPattern; UnitType: Word; Action: TTAActionType; ArrayId: LongWord): Integer;
const
  MAXRETRIES = 25 - 1;
var
  UnitSt: PUnitStruct;
  CallerUnitInfoSt, UnitInfoSt: PGameUnitfInfo;
//PlayerSt: PPlayerStruct;
  PlayerIndex, PlayerIndexCreate: Byte;
  UnitsArray: TFoundUnits;
  CallerPosition, TestPosition: TPosition;
  CallerTurn: TTurn;
  DestIsAir: Boolean;
  SpotTestFree: Boolean;
  NewPos: array [0..1] of LongInt;
  GridPos: array [0..1] of SmallInt;
  ToBeSpawned: array of TPosition;
  UnitState : LongWord;
  ResultUnit: Pointer;
//  Retries: Integer;
  i, j: Integer;
  r, angle, jiggle: Integer;
  ModelDiagonal: array [0..1] of Cardinal;
  SpawnRange: Cardinal;
begin
  Result:= -1;
  if UnitPtr <> nil then
  begin
    UnitSt := UnitPtr;

    PlayerIndex := TAUnit.GetOwnerIndexBuildspotSafe(UnitPtr);
   { PlayerSt := Pointer(UnitSt.p_Owner);
    if PlayerSt.en_cPlayerType = Player_LocalAI then
      PlayerIndex := PlayerSt.cPlayerOwnerIndexOne
    else
      PlayerIndex := PlayerSt.cPlayerIndexZero; }

    PlayerIndexCreate := UnitSt.cMyLOSPlayerID;

    UnitInfoSt := Pointer(TAMem.UnitInfoId2Ptr(UnitType));
    CallerUnitInfoSt := Pointer(TAMem.UnitInfoId2Ptr(UnitSt.nUnitCategoryID));

    CallerPosition := UnitSt.Position;
    CallerTurn := UnitSt.Turn;

    ModelDiagonal[0] := Round(Sqrt(ReverseBits(CallerUnitInfoSt.lWidthX) + ReverseBits(CallerUnitInfoSt.lWidthY)) / 2);
    ModelDiagonal[1] := Round(Sqrt(ReverseBits(UnitInfoSt.lWidthX) + ReverseBits(UnitInfoSt.lWidthY)) / 2);
    SpawnRange := Round((ModelDiagonal[0] + ModelDiagonal[1])*1.4);

    //Retries:= 0;
    Randomize;
    case Pattern of
    MinionsPattern_Random :
      begin
        for i := 1 to Amount do
        begin
          r := SpawnRange;
          for j := MAXRETRIES downto 0 do
          begin
            jiggle := Round(Random(High(Byte))/2);
            angle := Random(360)+1;
            if CircleCoords(CallerPosition, r + jiggle, angle, NewPos[0], NewPos[1]) then
            begin
              TestPosition := CallerPosition;
              TestPosition.X := NewPos[0];
              TestPosition.Z := NewPos[1];

              DestIsAir := (UnitInfoSt.UnitTypeMask and 2048 = 2048);
              SpotTestFree := TAUnit.TestBuildSpot(PlayerIndex, UnitType, TestPosition.X, TestPosition.Z);

              if SpotTestFree or DestIsAir then
              begin
                //if DestIsAir then
                //begin
                //  GridPos[0] := TestPosition.X div 16;
                //   GridPos[1] := TestPosition.Z div 16;
                //end else
                //begin
                  TAUnit.Position2Grid(TestPosition, UnitType, GridPos[0], GridPos[1]);
                //end;
                SetLength(ToBeSpawned, High(ToBeSpawned) + 2);
                // X
                ToBeSpawned[High(ToBeSpawned)].x_ := TestPosition.x_;
                ToBeSpawned[High(ToBeSpawned)].X := Round(GridPos[0] * 16 + (UnitInfoSt.nFootPrintX / 2) * 16);
                // Z
                ToBeSpawned[High(ToBeSpawned)].z_ := TestPosition.z_;
                ToBeSpawned[High(ToBeSpawned)].Z := Round(GridPos[1] * 16 + (UnitInfoSt.nFootPrintZ / 2) * 16);
                // Y
                if DestIsAir then
                begin
                  ToBeSpawned[High(ToBeSpawned)].Y := UnitInfoSt.nCruiseAlt;
                  UnitState := 6;
                end else
                begin
                  ToBeSpawned[High(ToBeSpawned)].Y := GetPosHeight(@ToBeSpawned[High(ToBeSpawned)]);
                  UnitState := 1;
                end;
                ToBeSpawned[High(ToBeSpawned)].y_ := CallerPosition.y_;

                ResultUnit:= TAUnit.CreateUnit(PlayerIndexCreate, UnitType, ToBeSpawned[High(ToBeSpawned)], nil, False, False, UnitState, 1);
                if ResultUnit <> nil then
                begin
                  SetLength(UnitsArray, High(UnitsArray) + 2);
                  UnitsArray[High(UnitsArray)]:= PUnitStruct(ResultUnit).lUnitInGameIndex;
                  TAUnit.CreateOrder(ResultUnit, UnitPtr, Action, nil, 1, 0, 0);
                  Break;
                end;
              end;
            end;
           // Inc(Retries);
          end;
        end;
      end; // MinionsPattern_Random
    end; // case
    Result:= High(ToBeSpawned) + 1;
  {  SendTextLocal('Caller model diag: ' + IntToStr(ModelDiagonal[0]));
    SendTextLocal('Spawn model diag: ' + IntToStr(ModelDiagonal[1]));
    SendTextLocal('Min range: ' + IntToStr(SpawnRange));
    SendTextLocal('Will spawn: ' + IntToStr(High(ToBeSpawned)+1));   }
   // SendTextLocal('Retries: ' + IntToStr(Retries));

    if Result > 0 then
    begin
      if ArrayId = 65535 then
        Result:= TAunit.GetId(ResultUnit)
      else
        if ArrayId <> 0 then
        begin
          if not Assigned(SpawnedMinionsArr[ArrayId].UnitIds) then
            if not TAUnits.UnitsIntoGetterArray(UnitPtr, 2, ArrayId, UnitsArray) then
              Result:= 0;
        end;
    end;
  end;
end;

class function TAUnits.CreateSearchFilter(Mask: Integer): TUnitSearchFilterSet;
var
  a, b: byte;
begin
  if Mask <> 0 then
    Result:= []
  else
  begin
    Result:= [usfNone];
    Exit;
  end;

  for a:= Ord(usfNone) to Ord(usfExcludeAir) do
  begin
    b:= Round(Power(2, a));
    if (Mask and b) = b then
    begin
      Include(Result, TUnitSearchFilter(a + 1));
    end;
  end;
end;

class function TAUnits.GetRandomArrayId(ArrayType: Byte): Word;
var
  test: word;
  retry: integer;
  cond: boolean;
begin
  Result:= 0;
  retry:= 0;
  cond:= false;
  repeat
    inc(retry);
    test:= 1 + Random(High(Word) - 1);  //0..65534
    case ArrayType of
      1 : cond:= not Assigned(UnitSearchArr[test].UnitIds);
      2 : cond:= not Assigned(SpawnedMinionsArr[test].UnitIds);
      else
        exit;
    end;
    if (retry > 100) and (cond = false) then exit;
  until (cond = true);
    Result:= test;
end;

class function TAUnits.SearchUnits ( UnitPtr: Pointer; SearchId: LongWord; SearchType: Byte;
  MaxDistance: Integer; Filter: TUnitSearchFilterSet; UnitTypeFilter: Word ): LongWord;
var
  FoundCount: Integer;
  MaxUnitId: LongWord;
  UnitSt, CheckedUnitSt: PUnitStruct;
  UnitInfoSt, CheckedUnitInfoSt: PGameUnitfInfo;
  Px, Py, Rx, Ry, dx, dy: SmallInt;
  PosX, PosY: Integer;
  LastNearestUnitDistance, NearestUnitDistance, Distance: Integer;
  UnitId: LongWord;
  FoundArray: TFoundUnits;
  Condition: Boolean;
  i: Integer;
label
  AddFound;

begin
  FoundCount:= 0;
  Result:= 0;
  //if (SearchType = 2) and (MaxDistance = 0) then Exit;
  UnitSt:= UnitPtr;
  UnitInfoSt:= UnitSt.p_UnitDef;
  MaxUnitId:= TAMem.GetMaxUnitId;

  for UnitId := 1 to MaxUnitId do
  begin
    CheckedUnitSt:= TAUnit.Id2Ptr(UnitId);
    if CheckedUnitSt.nUnitCategoryID = 0 then
      Continue;
    if (UnitTypeFilter <> 0) then
      if (CheckedUnitSt.nUnitCategoryID <> UnitTypeFilter) then
        Continue;
    if usfAllied in Filter then
    begin
      if (TAUnit.IsAllied(UnitPtr, UnitId) <> 1) then
        Continue;
      end else
      begin
        if usfOwner in Filter then
        begin
          if not TAUnit.IsOnThisComp(CheckedUnitSt, (usfAI in Filter)) then
            Continue;
        end else
        begin
          if usfEnemy in Filter then
          begin
            if (not (TAUnit.IsAllied(UnitPtr, UnitId) <> 1)) and TAUnit.IsOnThisComp(CheckedUnitSt, (usfAI in Filter)) then
              Continue;
          end;
        end;
    end;
    CheckedUnitInfoSt:= Pointer(CheckedUnitSt.p_UnitDef);
    if usfExcludeAir in Filter then
    begin
      if (CheckedUnitInfoSt.UnitTypeMask = (CheckedUnitInfoSt.UnitTypeMask or 2048)) then
        Continue;
    end;

    if (CheckedUnitSt.lBuildTimeLeft = 0) and
       (UnitId <> Word(UnitSt.lUnitInGameIndex)) then
    begin
      case SearchType of
        1 : begin
              Rx:= UnitSt.nGridPosX;
              Ry:= UnitSt.nGridPosZ;
              dx:= UnitInfoSt.nFootPrintX;
              dy:= UnitInfoSt.nFootPrintZ;
              PosX:= MakeLong(CheckedUnitSt.Position.x_, CheckedUnitSt.Position.X);
              Px:= (PosX - (SmallInt(CheckedUnitInfoSt.nFootPrintX shl 19)) + $80000) shr 20;
              PosY:= MakeLong(CheckedUnitSt.Position.z_, CheckedUnitSt.Position.Z);
              Py:= (PosY - (SmallInt(CheckedUnitInfoSt.nFootPrintZ shl 19)) + $80000) shr 20;
              if MaxDistance = 0 then // here used as "sensitivity" switcher
                Condition:= (Rx < Px) and (Px < Rx + dx) and (Ry < Py) and (Py < Ry + dy)
              else
                Condition:= (Rx <= Px) and (Px <= Rx + dx) and (Ry <= Py) and (Py <= Ry + dy);
              if Condition then goto AddFound;
            end;
     2..3 : begin
              if MaxDistance > 0 then
              begin
                Distance:= TAUnits.Distance(UnitSt.lUnitInGameIndex, CheckedUnitSt.lUnitInGameIndex);
                if (Distance <> -1) and (Distance <= MaxDistance) then goto AddFound;
              end else
                goto AddFound;
            end;
        4 : goto AddFound;
      end;
    end;
    Continue;
    AddFound:
    Inc(FoundCount);
    SetLength(FoundArray, High(FoundArray) + 2);
    FoundArray[High(FoundArray)]:= CheckedUnitSt.lUnitInGameIndex;
    if SearchId = 65535 then
    begin
      Result := CheckedUnitSt.lUnitInGameIndex;
      Exit;
    end;
  end;

  // Single unit search
  if (SearchType = 3) and (FoundCount > 0) then
  begin
    LastNearestUnitDistance:= 0;
    if High(FoundArray) + 1 > 0 then
      for i:= Low(FoundArray) to High(FoundArray) do
      begin
        NearestUnitDistance:= TAUnits.Distance(UnitSt.lUnitInGameIndex, FoundArray[i]);
        if i = Low(FoundArray) then
        begin
          LastNearestUnitDistance:= NearestUnitDistance;
          Result:= FoundArray[i];
        end else
          if NearestUnitDistance < LastNearestUnitDistance then
          begin
            LastNearestUnitDistance:= NearestUnitDistance;
            Result:= FoundArray[i];
          end;
      end;
      Exit;
  end;

  if SearchId <> 65535 then
    if not Assigned(UnitSearchArr[SearchId].UnitIds) then
      if TAUnits.UnitsIntoGetterArray(UnitPtr, 1, SearchId, FoundArray) then
        Result:= FoundCount;
end;

class function TAUnits.UnitsIntoGetterArray(UnitPtr: Pointer; ArrayType: Byte; Id: LongWord; const UnitsArray: TFoundUnits): Boolean;
var
  i: integer;
  UnitRec: TStoreUnitsRec;
begin
  Result:= False;
  if High(UnitsArray) + 1 > 0 then
  begin
    UnitRec.Id:= Id;
    SetLength(UnitRec.UnitIds, High(UnitsArray)+1);
    for i:= Low(UnitsArray) to High(UnitsArray) do
      UnitRec.UnitIds[i]:= UnitsArray[i];
    case ArrayType of
      1 : UnitSearchResults.Insert(Id, UnitRec);
      2 : SpawnedMinions.Insert(Id, UnitRec);
    end;
    Result:= True;
  end;
end;

class procedure TAUnits.ClearSearchRec(Id: LongWord; ArrayType: Byte);
begin
  case ArrayType of
  1 : begin
      if Assigned(UnitSearchArr[Id].UnitIds) then
        UnitSearchArr[Id].UnitIds := nil;
      end;
  2 : begin
      if Assigned(SpawnedMinionsArr[Id].UnitIds) then
        SpawnedMinionsArr[Id].UnitIds := nil;
      end;
  end;
end;

class procedure TAUnits.RandomizeSearchRec(Id: LongWord; ArrayType: Byte);
var
 i, j : word;
 MaxElem : Integer;
 X : Cardinal;
begin
  Randomize;
  case ArrayType of
  1 : if Assigned(UnitSearchArr[Id].UnitIds) then
      begin
        MaxElem := High(UnitSearchArr[Id].UnitIds);
        if MaxElem > 0 then
        begin
          for i := MaxElem downto 0 do
          begin
            j := Random(i) + 1;
            if not (i = j) then
            begin
              X := UnitSearchArr[Id].UnitIds[i];
              UnitSearchArr[Id].UnitIds[i] := UnitSearchArr[Id].UnitIds[j];
              UnitSearchArr[Id].UnitIds[j] := X;
            end;
          end;
        end;
      end;
  2 : if not Assigned(SpawnedMinionsArr[Id].UnitIds) then
      begin
        MaxElem := High(SpawnedMinionsArr[Id].UnitIds);
        if MaxElem > 0 then
        begin
          for i := MaxElem downto 0 do
          begin
            j := Random(i) + 1;
            if not (i = j) then
            begin
              X := SpawnedMinionsArr[Id].UnitIds[i];
              SpawnedMinionsArr[Id].UnitIds[i] := SpawnedMinionsArr[Id].UnitIds[j];
              SpawnedMinionsArr[Id].UnitIds[j] := X;
            end;
          end;
        end;
      end;
  end;
end;

class function TAUnits.Distance(UnitId1, UnitId2: LongWord): Integer;
var
  Unit1St, Unit2St: PUnitStruct;
  Px, Pz, Rx, Rz: SmallInt;
begin
  Result:= -1;
  Unit1St:= TAUnit.Id2Ptr(UnitId1);
  Unit2St:= TAUnit.Id2Ptr(UnitId2);
  if (Unit1St <> nil) and (Unit2St <> nil) then
  begin
    Rx:= Unit1St.Position.X;
    Rz:= Unit1St.Position.Z;
    Px:= Unit2St.Position.X;
    Pz:= Unit2St.Position.Z;
    Result:= Round(Sqrt(Abs((Px-Rx)*(Px-Rx) + (Pz-Rz)*(Pz-Rz))));
  end;
end;

class function TAUnits.CircleCoords(CenterPosition: TPosition; Radius: Integer; Angle: Integer; out x, z: LongInt): Boolean;
begin
  x := Round(CenterPosition.X + cos(Angle) * Radius);
  z := Round(CenterPosition.Z + sin(Angle) * Radius);
  if (x > 0) and (z > 0) then
    Result:= True
  else
    Result:= False;
end;

{class function TAUnits.Teleport(CallerTelePtr: Pointer; DestinationTeleId, TeleportedUnitId: LongWord): LongWord;
var
  Position: TPosition;
  DestinationTelePtr: Pointer;
  TeleportedUnitPtr: Pointer;
begin
  Result:= 0;
  DestinationTelePtr:= TAUnit.Id2Ptr(DestinationTeleId);
  TeleportedUnitPtr:= TAUnit.Id2Ptr(TeleportedUnitId);
  if (DestinationTelePtr <> nil) and (TeleportedUnitPtr <> nil) then
    if (PUnitStruct(DestinationTelePtr).nUnitCategoryID <> 0) then
    begin
      Position:= PUnitStruct(DestinationTelePtr).Position;
      Inc(Position.Y, 16);
      if TAUnit.CreateOrder(CallerTelePtr, TAUnit.Id2Ptr(TeleportedUnitId), Action_Teleport, @Position, 1, 100, 0) <> 0 then
        Result:= 1;
      TAUnit.CreateOrder(TeleportedUnitPtr, nil, Action_Park, nil, 0, 0, 0);
    end;
end;   }

end.
