unit TA_MemoryLocations;

interface
uses
  dplay, TA_MemoryStructures;

type
  PCustomUnitInfo = ^TCustomUnitInfo;
  TCustomUnitInfo = packed record
    unitId        : LongWord;
    unitIdRemote  : LongWord;  // owner's (unit upgrade packet sender) local unit id
    //OwnerPlayer  : Longint; // Buffer.EventPlayer_DirectID = PlayerAryIndex2ID(UnitInfo.ThisPlayer_ID   )
    InfoPtrOld   : LongWord;  // local old global template Pointer
    InfoStruct   : TGameUnitInfo;
  end;
  TUnitInfos = array of TCustomUnitInfo;

  TStoreUnitsRec = packed record
    Id : LongWord;
    UnitIds  : array of LongWord;
  end;
  TUnitSearchArr = array of TStoreUnitsRec;
  TSpawnedMinionsArr = array of TStoreUnitsRec;

  TAMem = class
  protected
    class Function GetViewPLayer : Byte;
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
    Property UnitInfosPtr : Pointer read GetUnitInfosPtr;
    Property UnitInfosCount : LongWord read GetUnitInfosCount;
    Property SwitchesMask : Word read GetSwitchesMask write SetSwitchesMask;

    class function GetModelPtr(index: Word): Pointer;
    class function UnitInfoId2Ptr(index: Word): Pointer;
    class function GetMovementClassPtr(index: Word): Pointer;
    class function GetWeapon(weaponid: Word): Pointer;
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

    class Procedure SetShareRadar(player : Pointer; value : Boolean);
    class function GetShareRadar(player : Pointer) : Boolean;

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

    class function GetUnitX(UnitPtr : Pointer): LongWord;
    class procedure SetUnitX(UnitPtr : Pointer; X : LongWord);
    class function GetUnitY(UnitPtr : Pointer): LongWord;
    class procedure SetUnitY(UnitPtr : Pointer; Y : LongWord);
    class function GetUnitZ(UnitPtr : Pointer): LongWord;
    class procedure SetUnitZ(UnitPtr : Pointer; Z : LongWord);

    class function GetTurnX(UnitPtr : Pointer): SmallInt;
    class procedure SetTurnX(UnitPtr : Pointer; X : SmallInt);
    class function GetTurnY(UnitPtr : Pointer): SmallInt;
    class procedure SetTurnY(UnitPtr : Pointer; Y : SmallInt);
    class function GetTurnZ(UnitPtr : Pointer): SmallInt;
    class procedure SetTurnZ(UnitPtr : Pointer; Z : SmallInt);

    class function GetMovementClassPtr(UnitPtr : Pointer): Pointer;
    class function SetTemplate(UnitPtr: Pointer; NewUnitInfoId: Word): Boolean;
    class function SetWeapon(UnitPtr : Pointer; index: LongWord; weaponid: Word): Boolean;

    class function UpdateLos(UnitPtr: Pointer): LongWord;
    class procedure Speech(UnitPtr : longword; speechtype: longword; speechtext: PChar);
    class procedure SoundEffectId(UnitPtr : longword; voiceid: longword);

    class function AtMouse: Pointer;

    { creating and killing unit }
    class function TestBuildSpot(PlayerIndex: Byte; UnitInfoId: Word; Position: TPosition; out GridPosX, GridPosY: LongInt): Boolean;
    class function CreateUnit(OwnerIndex: LongWord; UnitInfoId: LongWord; Position: TPosition; Turn: PTurn; TurnZOnly, RandomTurnZ: Boolean; FullHp: LongWord): Pointer;
    class Function GetBuildTimeLeft(UnitPtr : Pointer) : single;
    class procedure Kill(UnitPtr : Pointer; deathtype: byte);
    class procedure SwapByKill(UnitPtr: Pointer; newUnitInfoId: Word);

    { actions (orders, unit state) }
    class function GetCurrentOrder(UnitPtr: Pointer): TTAActionType;
    class function CreateOrder(UnitPtr: Pointer; TargetUnitPtr: Pointer; ActionType: TTAActionType; Position: PPosition; ShiftKey: Byte; Par1: LongWord): LongInt;

    { COB }
    class Function GetCOBDataPtr(UnitPtr : Pointer) : Pointer;
    class function CallCobProcedure(UnitPtr: Pointer; ProcName: String; Par1, Par2, Par3, Par4: PLongWord): Integer;
    class function GetCobString(UnitPtr: Pointer): String;

    { id, owner etc. }
    class Function Id2Ptr(LongUnitId : LongWord) : Pointer;
    class Function GetOwnerPtr(UnitPtr : Pointer) : Pointer;
    class Function GetOwnerIndex(UnitPtr : Pointer) : Integer;
    class Function GetId(UnitPtr : Pointer) : Word;
    class Function GetLongId(UnitPtr : Pointer) : LongWord;
    class Function Id2LongId(UnitId: Word) : LongWord;
    class function GetUnitInfoId(UnitPtr: Pointer): Word;
    class function GetUnitInfoPtr(UnitPtr: Pointer): Pointer;
    class Function IsOnThisComp(UnitPtr : Pointer; IncludeAI: Boolean) : LongWord;
    class Function IsRemoteIdLocal(UnitPtr: Pointer; remoteUnitId: PLongWord; out local: Boolean): LongWord;
    class function IsAllied(UnitPtr: Pointer; UnitId: LongWord): Byte;
    class Function Team(UnitId: LongWord): Integer;

    { cloning global template and setting its fields }
    class function SetUpgradeable(UnitPtr: Pointer; State: Byte; remoteUnitId: PLongWord): Boolean;
    class function SearchCustomUnitInfos(unitId: LongWord; remoteUnitId: PLongWord; local: Boolean; out index: Integer ): Boolean;
    class function GetUnitInfoField(UnitPtr: Pointer; fieldType: LongWord; remoteUnitId: PLongWord): LongWord;
    class function SetUnitInfoField(UnitPtr: Pointer; fieldType: LongWord; value: LongWord; remoteUnitId: PLongWord): Boolean;
  end;

  TAUnits = class
  public
    class Function CreateMinions(UnitPtr: Pointer; Amount: Byte; Pattern: TMinionsPattern; UnitType: Word; Action: TTAActionType; ArrayId: LongWord): Integer;

    { searching units in game }
    class function CreateSearchFilter(Mask: Integer): TUnitSearchFilterSet;
    class function GetRandomArrayId(ArrayType: Byte): Word;
    class function SearchUnits(UnitPtr: Pointer; SearchId: LongWord; SearchType: Byte; MaxDistance: Integer; Filter: TUnitSearchFilterSet; UnitTypeFilter: Word ): LongWord;
    class function UnitsIntoGetterArray(UnitPtr: Pointer; ArrayType: Byte; Id: LongWord; const UnitsArray: TFoundUnits): Boolean;
    class procedure ClearArrayElem(Id: LongWord; ArrayType: Byte);
  //  class function Distance(UnitPtr: Pointer; UnitId2: LongWord): LongWord; overload;
    class function Distance(UnitId1, UnitId2: LongWord): Integer;
    class function Teleport(CallerTelePtr: Pointer; DestinationTeleId, TeleportedUnitId: Cardinal): Cardinal;
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
  COB_extensions,
  IniOptions,
  TAMemManipulations,
  TA_MemoryConstants;

// -----------------------------------------------------------------------------

function IsTAVersion31 : Boolean;
begin
result := TAMem.IsTAVersion31();
end; {IsTAVersion31}

var
  CacheUsed : Boolean;
  IsTAVersion31_Cache : Boolean;
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

// -----------------------------------------------------------------------------

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
//?
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

class Function TAPlayer.GetPlayerByIndex(playerIndex : LongWord) : Pointer;
begin
result:= @PTAdynmemStruct(TAData.MainStructPtr)^.Players[playerIndex];
//result := Pointer(TAData.PlayersStructPtr+(playerIndex*SizeOf(TPlayerStruct)) );
end;

class Function TAPlayer.GetPlayerByDPID(playerPID : TDPID) : LongWord;
var
  aplayerPID : PDPID;
  i : Integer;
begin
result := LongWord(-1);
aplayerPID := Pointer(PPlayerStruct(TAData.PlayersStructPtr).lDirectPlayID);
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

class Function TAMem.GetPausedState : Boolean;
begin
result := PTAdynmemStruct(TAData.MainStructPtr)^.cIsGamePaused <> 0;
end;

class Procedure TAMem.SetPausedState( value : Boolean);
begin
PTAdynmemStruct(TAData.MainStructPtr)^.cIsGamePaused := BoolValues[value];
end;

class Function TAUnit.Id2Ptr(LongUnitId : LongWord) : Pointer;
begin
Result:= nil;
if (Word(LongUnitId) > TAData.MaxUnitLimit * MAXPLAYERCOUNT) then exit;
result := Pointer(LongWord(TAData.UnitsPtr) + SizeOf(TUnitStruct)*Word(LongUnitId));
end;

class function TAUnit.GetUnitInfoPtr(UnitPtr: Pointer) : Pointer;
begin
result:= PUnitStruct(UnitPtr).p_UnitDef;
end;

// -----------------------------------------------------------------------------

class Function TAPlayer.GetPlayerPtrByDPID(playerPID : TDPID) : Pointer;
var
  i : Integer;
  PlayerSt: PPlayerStruct;
begin
result := nil;
i := 0;
PlayerSt:= GetPlayerByIndex(i);
while i < MAXPLAYERCOUNT do
  begin
  if PDPID(PlayerSt.lDirectPlayID)^ = playerPID then
    begin
    result := PlayerSt;
    break;
    end;
  inc(i);
  PlayerSt:= GetPlayerByIndex(i);
  end;
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

class Function TAPlayer.GetDPID(player : Pointer) : TDPID;
begin
result := PPlayerStruct(player).lDirectPlayID;
end;

class Function TAPlayer.PlayerType(player : Pointer) : TTAPlayerType;
begin
result := PPlayerStruct(player).en_cPlayerType;
end;

Class function TAPlayer.GetIsActive(player : Pointer) : Boolean;
begin
result := PPlayerStruct(Player).lPlayerActive <> $0;
end;

Class function TAPlayer.GetIsWatcher(player : Pointer) : Boolean;
begin
result := PropertyMask_Watcher in PPlayerStruct(Player).PlayerInfo.PropertyMask;
end;

Class Procedure TAPlayer.SetShareRadar(player : Pointer; value : Boolean);
begin
if value then
  Include(PPlayerStruct(Player).PlayerInfo.SharedBits, SharedState_SharedRadar)
else
  Exclude(PPlayerStruct(Player).PlayerInfo.SharedBits, SharedState_SharedRadar);
end;

Class function TAPlayer.GetShareRadar(player : Pointer) : Boolean;
begin
result := SharedState_SharedRadar in PPlayerStruct(Player).PlayerInfo.SharedBits;
end;

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

// -----------------------------------------------------------------------------

class Function TAUnit.GetKills(UnitPtr : Pointer) : Word;
begin
result := PUnitStruct(UnitPtr).nKills;
end;

class procedure TAUnit.SetKills(UnitPtr : Pointer; Kills : Word);
begin
PUnitStruct(UnitPtr).nKills := Kills;
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

class function TAUnit.TestBuildSpot(PlayerIndex: Byte; UnitInfoId: Word; Position: TPosition; out GridPosX, GridPosY: LongInt): Boolean;
var
  PosX, PosY: Integer;
  TestPos: LongWord;
  LoSpot, HiSpot: Smallint;
  UnitInfoSt: PGameUnitfInfo;
  PlayerSt: PPlayerStruct;
begin
  Result:= False;
  UnitInfoSt:= Pointer(TAMem.UnitInfoId2Ptr(UnitInfoId));
  PlayerSt:= TAPlayer.GetPlayerByIndex(PlayerIndex);
  if (UnitInfoSt <> nil) and (PlayerSt <> nil) then
  begin
    PosX:= MakeLong(Position.x_, Position.X);
    LoSpot:= (PosX - (SmallInt(UnitInfoSt.nFootPrintX shl 19)) + $80000) shr 20;
    PosY:= MakeLong(0, Position.Y);
    HiSpot:= (PosY - (SmallInt(UnitInfoSt.nFootPrintZ shl 19)) + $80000) shr 20;
    If (LoSpot < 0) or (HiSpot < 0) then Exit;
    TestPos:= MakeLong(LoSpot, HiSpot);
    Result:= (TestGridSpot(UnitInfoSt, TestPos, 0, PlayerIndex) = 1);
    if Result then
    begin
      GridPosX:= LoSpot;
      GridPosY:= HiSpot;
    end;
  end;
end;

class function TAUnit.CreateUnit( OwnerIndex: LongWord;
                              UnitInfoId: LongWord;
                              Position: TPosition;
                              Turn: PTurn;
                              TurnZOnly: Boolean;
                              RandomTurnZ: Boolean;
                              FullHp: LongWord): Pointer;
var
  UnitSt: PUnitStruct;
  PosX, PosZ, PosY: LongWord;
begin
  PosX:= MakeLong(Position.x_, Position.X);
  PosZ:= MakeLong(Position.z_, Position.Z);
  PosY:= MakeLong(Position.y_, Position.Y);

  Result:= Pointer(Unit_Create(OwnerIndex, UnitInfoId, PosX, PosZ, PosY, FullHp, 0, 0));

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

{
Rapidly kills a unit.
0 - dgun (makes 30000 dmg on unit, then kills it)
1 - explode as
2 - self destruct
3 - no blast
4 - self destruct countdown
In all cases kill packet is broadcasted.
}
class procedure TAUnit.Kill(UnitPtr : Pointer; deathtype: byte);
var
  UnitSt: PUnitStruct;
begin
  if UnitPtr <> nil then
  begin
    case deathtype of
      0 : Unit_MakeDamage_(UnitPtr, UnitPtr, 30000, 3, 0);
      1 : Unit_KillMakeDamage(UnitPtr, 0);
      2 : Unit_KillMakeDamage(UnitPtr, 1);
      4 : TAUnit.CreateOrder(UnitPtr, nil, Action_SelfDestruct, nil, 1, 0);
    end;
    if (deathtype >= 1) and (deathtype <= 3) then
    begin
      UnitSt:= UnitPtr;
      UnitSt.lUnitStateMask:= UnitSt.lUnitStateMask or $4000;
      Unit_Kill(UnitPtr, 3);
    end;
  end;
end;

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

function CircleCoords(CenterPosition: TPosition; Radius: Integer; Angle: integer; out x, y: LongInt): Boolean;
begin
  x:= Round(CenterPosition.X + cos(Angle) * Radius);
  y:= Round(CenterPosition.Y + sin(Angle) * Radius);
  if (x > 0) and (y > 0) then Result:= True else Result:= False;
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

{
Finds units from all players, owner or allied
SearchType
1 - units on yardmap of caller unit
2 - near by max distance
3 - single nearest unit
4 - all in game for certain type
}
class function TAUnits.SearchUnits ( UnitPtr: Pointer;
                                  SearchId: LongWord;
                                  SearchType: Byte;
                                  MaxDistance: Integer;
                                  Filter: TUnitSearchFilterSet;
                                  UnitTypeFilter: Word ): LongWord;
var
  FoundCount: Integer;
  MaxUnitId: LongWord;
  UnitSt,                      // caller unit (structure)
  CheckedUnitSt: PUnitStruct;  // array unit (structure)
  UnitInfoSt,                         // caller unitinfo (structure)
  CheckedUnitInfoSt: PGameUnitfInfo;  // array unitinfo (structure)
  Px, Py,               // array unit base coords (grid style)
  Rx, Ry,               // caller yardmap coords (grid style)
  dx, dy: SmallInt;     // dimensions of caller yardmap (grid style)
  PosX, PosY: Integer;  // array unit base coords (pixels)

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
          if (TAUnit.IsOnThisComp(CheckedUnitSt, (usfAI in Filter)) <> 1) then
            Continue;
        end else
        begin
          if usfEnemy in Filter then
          begin
            if (not (TAUnit.IsAllied(UnitPtr, UnitId) <> 1)) and (not (TAUnit.IsOnThisComp(CheckedUnitSt, (usfAI in Filter)) <> 1)) then
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
              Ry:= UnitSt.nGridPosY;
              dx:= UnitInfoSt.nFootPrintX;
              dy:= UnitInfoSt.nFootPrintZ;
              PosX:= MakeLong(CheckedUnitSt.Position.x_, CheckedUnitSt.Position.X);
              Px:= (PosX - (SmallInt(CheckedUnitInfoSt.nFootPrintX shl 19)) + $80000) shr 20;
              PosY:= MakeLong(CheckedUnitSt.Position.y_, CheckedUnitSt.Position.Y);
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
  end;

  // Single unit search
  if (SearchType = 3) and (FoundCount > 0) then
  begin
    LastNearestUnitDistance:= 0;
    if High(FoundArray) + 1 > 0 then
      for i:= Low(FoundArray) to High(FoundArray) do
      begin
       {  if I = Low(MyArray) then
           LastNearestUnitDistance:= NearestUnitDistance
         else
           if NearestUnitDistance < LastNearestUnitDistance then LastNearestUnitDistance:= NearestUnitDistance;    }
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

  if not Assigned(UnitSearchArr[SearchId].UnitIds) then
  begin
    if TAUnits.UnitsIntoGetterArray(UnitPtr, 1, SearchId, FoundArray) then
      Result:= FoundCount;
  end else
    SendTextLocal('Wow, a fkn search conflict !');
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

class procedure TAUnits.ClearArrayElem(Id: LongWord; ArrayType: Byte);
begin
  case ArrayType of
  1 : begin
      if Assigned(UnitSearchArr[Id].UnitIds) then
        UnitSearchArr[Id].UnitIds:= nil;
      end;
  2 : begin
      if Assigned(SpawnedMinionsArr[Id].UnitIds) then
        SpawnedMinionsArr[Id].UnitIds:= nil;
      end;
  end;
end;

class function TAUnits.Distance(UnitId1, UnitId2: LongWord): Integer;
var
  Unit1St, Unit2St: PUnitStruct;
  Px, Py, Rx, Ry: SmallInt;
begin
  Result:= -1;
  Unit1St:= TAUnit.Id2Ptr(UnitId1);
  Unit2St:= TAUnit.Id2Ptr(UnitId2);
  if (Unit1St <> nil) and (Unit2St <> nil) then
  begin
    Rx:= Unit1St.Position.X;
    Ry:= Unit1St.Position.Y;
    Px:= Unit2St.Position.X;
    Py:= Unit2St.Position.Y;
    Result:= Round(Sqrt(Abs((Px-Rx)*(Px-Rx) + (Py-Ry)*(Py-Ry))));
  end;
end;

class function TAUnits.Teleport(CallerTelePtr: Pointer; DestinationTeleId, TeleportedUnitId: LongWord): LongWord;
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
      if TAUnit.CreateOrder(CallerTelePtr, TAUnit.Id2Ptr(TeleportedUnitId), Action_Teleport, @Position, 1, 100) <> 0 then
        Result:= 1;
      TAUnit.CreateOrder(TeleportedUnitPtr, nil, Action_Park, nil, 0, 0);
    end;
end;

class function TAUnit.CallCobProcedure(UnitPtr: Pointer; ProcName: String; Par1, Par2, Par3, Par4: PLongWord): Integer;
var
  ParamsAmount: LongWord;
  UnitSt: PUnitStruct;
  lPar1, lPar2, lPar3, lPar4: LongWord;
begin
  Result:= -1;
  if UnitPtr <> nil then
  begin
    ParamsAmount:= 0;
    lPar1:= 0;
    lPar2:= 0;
    lPar3:= 0;
    lPar4:= 0;
    if Par1 <> nil then
    begin
      lPar1:= PLongWord(Par1)^;
      Inc(ParamsAmount);
    end;

    if Par2 <> nil then
    begin
      lPar2:= PLongWord(Par2)^;
      Inc(ParamsAmount);
    end;

    if Par3 <> nil then
    begin
      lPar3:= PLongWord(Par3)^;
      Inc(ParamsAmount);
    end;

    if Par4 <> nil then
    begin
      lPar4:= PLongWord(Par4)^;
      Inc(ParamsAmount);
    end;

    UnitSt:= UnitPtr;
    Result:= Script_RunCallBack ( 0,
                                  0,
                                  LongWord(UnitSt.p_AlmostCOBStruct),
                                  lPar4,
                                  lPar3,
                                  lPar2,
                                  lPar1,
                                  ParamsAmount,
                                  0,
                                  0, // out var ?
                                  PAnsiChar(ProcName) );
  end;
end;

class function TAUnit.GetCobString(UnitPtr: Pointer): String;
begin
end;

class Function TAUnits.CreateMinions(UnitPtr: Pointer; Amount: Byte; Pattern: TMinionsPattern; UnitType: Word; Action: TTAActionType; ArrayId: LongWord): Integer;
const
  MAXRETRIES = 25;
var
  UnitSt: PUnitStruct;
  CallerUnitInfoSt, UnitInfoSt: PGameUnitfInfo;
  PlayerSt: PPlayerStruct;
  PlayerIndex: Byte;
  UnitsArray: TFoundUnits;
  CallerPosition, TestPosition: TPosition;
  CallerTurn: TTurn;
  DestIsAir: Boolean;
  SpotTestFree: Boolean;
  NewPos: array [0..1] of LongInt;
  GridPos: array [0..1] of LongInt;
  ToBeSpawned: array of TPosition;
  ResultUnit: Pointer;
//  Retries: Integer;
  i, j: Integer;
  r, angle, jiggle: Integer;
  ModelDiagonal: array [0..1] of Cardinal;
  MinSpawnRange: Cardinal;      // minimum range from caller unit where spawn checks will be done
begin
  Result:= -1;
  if UnitPtr <> nil then
  begin
    UnitSt:= UnitPtr;
  //  PlayerSt:= TAMem.GetPlayerByIndex(UnitSt.cOwnerIndex);
    PlayerSt:= Pointer(UnitSt.p_Owner);
    if PlayerSt.en_cPlayerType = Player_LocalAI then
      PlayerIndex:= PlayerSt.cPlayerOwnerIndexOne
    else
      PlayerIndex:= PlayerSt.cPlayerIndexZero;

    UnitInfoSt:= Pointer(TAMem.UnitInfoId2Ptr(UnitType));
    CallerUnitInfoSt:= Pointer(TAMem.UnitInfoId2Ptr(UnitSt.nUnitCategoryID));

    CallerPosition:= UnitSt.Position;
    CallerTurn:= UnitSt.Turn;

    ModelDiagonal[0]:= Round(Sqrt(ReverseBits(CallerUnitInfoSt.lWidthX) + ReverseBits(CallerUnitInfoSt.lWidthY)) / 2);
    ModelDiagonal[1]:= Round(Sqrt(ReverseBits(UnitInfoSt.lWidthX) + ReverseBits(UnitInfoSt.lWidthY)) / 2);
    MinSpawnRange:= Round((ModelDiagonal[0] + ModelDiagonal[1])*1.4);

    //Retries:= 0;
    Randomize;
    case Pattern of
    MinionsPattern_Random :
      begin
        for i:= 1 to Amount do
        begin
          r:= MinSpawnRange;
          for j:= MAXRETRIES downto 0 do
          begin
            jiggle:= Round(Random(High(Byte))/3);
            angle:= Random(360)+1;
            if CircleCoords(CallerPosition, r + jiggle, angle, NewPos[0], NewPos[1]) then
            begin
              TestPosition:= CallerPosition;
              TestPosition.X:= NewPos[0];
              TestPosition.Y:= NewPos[1];

              DestIsAir:= (UnitInfoSt.UnitTypeMask and 2048 = 2048);
              SpotTestFree:= TAUnit.TestBuildSpot(PlayerIndex, UnitType, TestPosition, GridPos[0], GridPos[1]);

              if SpotTestFree or DestIsAir then
              begin
                if DestIsAir then
                begin
                  GridPos[0] := Round(TestPosition.X / 16);
                  GridPos[1] := Round(TestPosition.Y / 16);
                end;
                SetLength(ToBeSpawned, High(ToBeSpawned) + 2);
                ToBeSpawned[High(ToBeSpawned)].x_ := TestPosition.x_;
                ToBeSpawned[High(ToBeSpawned)].X := Round(GridPos[0] * 16 + (UnitInfoSt.nFootPrintX / 2) * 16);
                ToBeSpawned[High(ToBeSpawned)].z_ := CallerPosition.z_;
                if DestIsAir then
                  ToBeSpawned[High(ToBeSpawned)].Z := UnitInfoSt.nCruiseAlt
                else
                  ToBeSpawned[High(ToBeSpawned)].Z := CallerPosition.Z;
                ToBeSpawned[High(ToBeSpawned)].y_ := TestPosition.y_;
                ToBeSpawned[High(ToBeSpawned)].Y := Round(GridPos[1] * 16 + (UnitInfoSt.nFootPrintZ / 2) * 16);

                ResultUnit:= TAUnit.CreateUnit(PlayerIndex, UnitType, ToBeSpawned[High(ToBeSpawned)], nil, False, False, 1);
                if ResultUnit <> nil then
                begin
                  SetLength(UnitsArray, High(UnitsArray) + 2);
                  UnitsArray[High(UnitsArray)]:= PUnitStruct(ResultUnit).lUnitInGameIndex;
                  TAUnit.CreateOrder(ResultUnit, UnitPtr, Action, nil, 1, 0);
                  Break;
                end;
              end;
            end;
            //Inc(Retries);
          end;
        end;
      end; { MinionsPattern_Random }
    end; { case }
    Result:= High(ToBeSpawned) + 1;
   { SendTextLocal('Caller model diag: ' + IntToStr(ModelDiagonal[0]));
    SendTextLocal('Spawn model diag: ' + IntToStr(ModelDiagonal[1]));
    SendTextLocal('Min range: ' + IntToStr(MinSpawnRange));
    SendTextLocal('Will spawn: ' + IntToStr(High(ToBeSpawned)+1));
    SendTextLocal('Retries: ' + IntToStr(Retries));   }

    if (Result > 0) and (ArrayId <> 0) then
    begin
      if not Assigned(SpawnedMinionsArr[ArrayId].UnitIds) then
        if not TAUnits.UnitsIntoGetterArray(UnitPtr, 2, ArrayId, UnitsArray) then
          Result:= 0;
    end;
  end;
end;

class function TAUnit.CreateOrder(UnitPtr: Pointer; TargetUnitPtr: Pointer; ActionType: TTAActionType; Position: PPosition; ShiftKey: Byte; Par1: LongWord): LongInt;
begin
  Result:= Order2Unit(Ord(ActionType), ShiftKey, UnitPtr, TargetUnitPtr, Position, 0, 0);
end;

class function TAUnit.GetCurrentOrder(UnitPtr: Pointer): TTAActionType;
var
  UnitSt: PUnitStruct;
begin
  UnitSt:= UnitPtr;
  if UnitSt.p_UnitOrders <> nil then
    Result:= TTAActionType(PByte(LongWord(UnitSt.p_UnitOrders)+4)^)
  else
    Result:= Action_NoResult;
end;

class Function TAUnit.GetHealth(UnitPtr : Pointer) : Word;
begin
result := PUnitStruct(UnitPtr).nHealth;
end;

class procedure TAUnit.SetHealth(UnitPtr : Pointer; Health : LongWord);
begin
PUnitStruct(UnitPtr).nHealth := Health;
end;

class procedure TAUnit.SetUnitX(UnitPtr : Pointer; X : LongWord);
begin
PUnitStruct(UnitPtr).Position.x_:= LoWord(LongWord(X*163840));
PUnitStruct(UnitPtr).Position.X:= HiWord(LongWord(X*163840));
end;

class procedure TAUnit.SetUnitY(UnitPtr : Pointer; Y : LongWord);
begin
PUnitStruct(UnitPtr).Position.y_:= LoWord(LongWord(y*163840));
PUnitStruct(UnitPtr).Position.Y:= HiWord(LongWord(Y*163840));
end;

class procedure TAUnit.SetUnitZ(UnitPtr : Pointer; Z : LongWord);
begin
PUnitStruct(UnitPtr).Position.z_:= LoWord(LongWord(Z*163840));
PUnitStruct(UnitPtr).Position.Z:= HiWord(LongWord(Z*163840));
end;

class procedure TAUnit.SetTurnX(UnitPtr : Pointer; X : SmallInt);
begin
PUnitStruct(UnitPtr).Turn.X:= X;
end;

class procedure TAUnit.SetTurnZ(UnitPtr : Pointer; Z : SmallInt);
begin
PUnitStruct(UnitPtr).Turn.Z:= Z;
end;

class procedure TAUnit.SetTurnY(UnitPtr : Pointer; Y : SmallInt);
begin
PUnitStruct(UnitPtr).Turn.Y:= Y;
end;

class function TAUnit.GetUnitX(UnitPtr : Pointer): LongWord;
begin
result := MakeLong(PUnitStruct(UnitPtr).Position.x_, PUnitStruct(UnitPtr).Position.X);
end;

class function TAUnit.GetUnitY(UnitPtr : Pointer): LongWord;
begin
result := MakeLong(PUnitStruct(UnitPtr).Position.y_, PUnitStruct(UnitPtr).Position.y);
end;

class function TAUnit.GetUnitZ(UnitPtr : Pointer): LongWord;
begin
result := MakeLong(PUnitStruct(UnitPtr).Position.z_, PUnitStruct(UnitPtr).Position.z);
end;

class function TAUnit.GetTurnX(UnitPtr : Pointer): SmallInt;
begin
result := PUnitStruct(UnitPtr).Turn.X;
end;

class function TAUnit.GetTurnZ(UnitPtr : Pointer): SmallInt;
begin
result := PUnitStruct(UnitPtr).Turn.Z;
end;

class function TAUnit.GetTurnY(UnitPtr : Pointer): SmallInt;
begin
result := PUnitStruct(UnitPtr).Turn.Y;
end;

class function TAUnit.GetUnitInfoId(UnitPtr: Pointer): Word;
begin
result:= PUnitStruct(UnitPtr).nUnitCategoryID;
end;

class function TAUnit.GetMovementClassPtr(UnitPtr : Pointer): Pointer;
begin
result := PUnitStruct(UnitPtr).p_MovementClass;
end;

const
  Cloak_BitMask = $4;
  CloakUnitStateMask_BitMask = $8;
class procedure TAUnit.SetCloak(UnitPtr : Pointer; Cloak : Word);
begin
  if Cloak = 1 then
    PUnitStruct(UnitPtr).lUnitStateMask := PUnitStruct(UnitPtr).lUnitStateMask or CloakUnitStateMask_BitMask
  else
    PUnitStruct(UnitPtr).lUnitStateMask := PUnitStruct(UnitPtr).lUnitStateMask and not CloakUnitStateMask_BitMask;
end;

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

class Function TAUnit.GetBuildTimeLeft(UnitPtr : Pointer) : single;
begin
result := PUnitStruct(UnitPtr).lBuildTimeLeft;
end;

class Function TAUnit.GetOwnerPtr(UnitPtr : Pointer) : Pointer;
begin
result := PUnitStruct(UnitPtr).p_Owner;
end;

class Function TAUnit.GetOwnerIndex(UnitPtr : Pointer) : Integer;
begin
result := PUnitStruct(UnitPtr).cMyLOSPlayerID;
end;

class Function TAUnit.IsOnThisComp(UnitPtr : Pointer; IncludeAI: Boolean) : LongWord;
var
  playerPtr : Pointer;
  TAPlayerType : TTAPlayerType;
begin
result:= 0;
try
  playerPtr := TAUnit.GetOwnerPtr(Pointer(UnitPtr));
  TAPlayerType := TAPlayer.PlayerType(playerPtr);
  case TAPlayerType of
    Player_LocalHuman:
      result := 1;
    Player_LocalAI:
      if IncludeAI then result := 1;
    else
      result := 0;
  end;
except
  result := 0;
end;
end;

{
TA stores unit ID as a LongWord where one word is actual ID, and a second one is random(65535).
The random part is being made at unit create by every player in game.
This procedure is making comparision of random word, and it can tell us is unit #x,
still that one we asked before, because unit could die in packet receive time and new one with same id could be created already.
F.e. unit #3 :
- ABCD0003 = ABCD0003
- PC1: we are changing some property of unit and sending packet
- PC1: unit died
- PC1: new unit with #3 was created FFDDAA0003
- PC2: due to some random lag, lately received ABCD0003 property change packet
- but ABCD0003 <> FFDDAA0003 so we are not going to make changes on "new" #3 unit
}
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
      Local:= (IsOnThisComp(Pointer(UnitPtr), True) = 1);
      Result:= TAUnit.GetLongId(Pointer(UnitPtr));
    end;
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

class Function TAUnit.Team(UnitId: LongWord): Integer;
var
  Unit2Ptr: Pointer;
begin
  Unit2Ptr := TAUnit.Id2Ptr(Word(UnitId));
  result := TAUnit.GetOwnerIndex(Unit2Ptr);
end;

class Function TAUnit.GetId(UnitPtr : Pointer) : Word;
begin
result := Word(PUnitStruct(UnitPtr).lUnitInGameIndex);
end;

class Function TAUnit.GetLongId(UnitPtr : Pointer) : LongWord;
begin
result := PUnitStruct(UnitPtr).lUnitInGameIndex;
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

class Function TAUnit.GetCOBDataPtr(UnitPtr : Pointer) : Pointer;
begin
result := PUnitStruct(UnitPtr).p_AlmostCOBStruct;
end;

class function TAMem.GetMovementClassPtr(index: Word): Pointer;
begin
result := Pointer(TAMovementClassArray + SizeOf(TMoveInfoClassStruct) * index);
end;

class function TAMem.GetWeapon(weaponid: Word): Pointer;
begin
// fix me to be compatible with xpoy id patch
  if IniSettings.WeaponIdPatch then
    result:= @PTAdynmemStruct(TAData.MainStructPtr).Weapons[weaponid]
  else
    result:= @PTAdynmemStruct(TAData.MainStructPtr).Weapons[weaponid];
end;

class function TAMem.GetMaxUnitId: LongWord;
begin
  if TAData.IsAlteredUnitLimit then
    result := TAData.ActualUnitLimit * MAXPLAYERCOUNT
  else
    result := TAData.MaxUnitLimit * MAXPLAYERCOUNT;
end;

class function TAMem.GetModelPtr(index: Word): Pointer;
begin
result := PLongWord(LongWord(GetModelsArrayPtr) + index * 4);
end;

class function TAMem.UnitInfoId2Ptr(index: Word): Pointer;
begin
result := Pointer(LongWord(TAData.UnitInfosPtr) + index * SizeOf(TGameUnitInfo));
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
    if TAUnit.CreateUnit(UnitSt.cMyLOSPlayerID, newUnitInfoId, Position, @Turn, True, False, 1) <> nil then
      TAUnit.Kill(UnitPtr, 3);
  end;
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

class function TAUnit.SetWeapon(UnitPtr : Pointer; index: LongWord; weaponId: Word): Boolean;
var
  Weapon: Pointer;
begin
  Result:= False;
  if UnitPtr <> nil then
  begin
    Weapon:= TAMem.GetWeapon(weaponId);
    case index of
      WEAPON1: PUnitStruct(UnitPtr)^.p_Weapon1 := Weapon;
      WEAPON2: PUnitStruct(UnitPtr)^.p_Weapon2 := Weapon;
      WEAPON3: PUnitStruct(UnitPtr)^.p_Weapon3 := Weapon;
    end;
    Result:= True;
  end;
end;

class function TAUnit.UpdateLos(UnitPtr : pointer): LongWord;
begin
  TA_UpdateUnitLOS(LongWord(UnitPtr));
  Result:= TA_UpdateLOS(LongWord(TAUnit.GetOwnerIndex(UnitPtr)), 0);
end;

class procedure TAUnit.Speech(UnitPtr : longword; speechtype: longword; speechtext: PChar);
begin
  PlaySound_UnitSpeech(UnitPtr, speechtype, speechtext);
end;

class procedure TAUnit.SoundEffectId(UnitPtr : longword; voiceid: longword);
begin
  PlaySound_EffectId(voiceid, UnitPtr);
end;

{
Searches non-sorted array of custom unit info structures.
we can't delete any elements of array (would move pointers and TA would crash),
}
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

{
Clone current unit template to a new memory location that is writable,
making it fully customizable. Or restore the orginal TA's UNITINFO_p pointer
}
class function TAUnit.SetUpgradeable(UnitPtr: Pointer; State: Byte; remoteUnitId: PLongWord): Boolean;
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

class function TAUnit.GetUnitInfoField(UnitPtr: Pointer; fieldType: LongWord; remoteUnitId: PLongWord): LongWord;
var
  arrIdx: Integer;
  unitId: LongWord;
  local: Boolean;
  custTmplFound: boolean;
  engineTmpl: PGameUnitfInfo;
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
    if (engineTmpl = nil) then Exit;
  end;

  case fieldType of
    MAXHEALTH       : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nMaxHP else Result:= engineTmpl.nMaxHP;
    HEALTIME        : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nHealTime else Result:= engineTmpl.nHealTime;

    MAXSPEED        : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.lMaxSpeedRaw else Result:= engineTmpl.lMaxSpeedRaw;
    ACCELERATION    : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.lAcceleration else Result:= engineTmpl.lAcceleration;
    BRAKERATE       : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.lBrakeRate else Result:= engineTmpl.lBrakeRate;
    TURNRATE        : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nTurnRate else Result:= engineTmpl.nTurnRate;
    CRUISEALT       : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nCruiseAlt else Result:= engineTmpl.nCruiseAlt;
    MANEUVERLEASH   : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nManeuverLeashLen else Result:= engineTmpl.nManeuverLeashLen;
    ATTACKRUNLEN    : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nAttackRunLength else Result:= engineTmpl.nAttackRunLength;
    MAXWATERDEPTH   : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nMaxWaterDepth else Result:= engineTmpl.nMaxWaterDepth;
    MINWATERDEPTH   : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nMinWaterDepth else Result:= engineTmpl.nMinWaterDepth;
    MAXSLOPE        : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.cMaxSlope else Result:= engineTmpl.cMaxSlope;
    MAXWATERSLOPE   : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.cMaxWaterSlope else Result:= engineTmpl.cMaxWaterSlope;
    WATERLINE       : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.cWaterLine else Result:= engineTmpl.cWaterLine;

    TRANSPORTSIZE   : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.cTransportSize else Result:= engineTmpl.cTransportSize;
    TRANSPORTCAP    : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.cTransportCap else Result:= engineTmpl.cTransportCap;

    BANKSCALE       : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.lBankScale else Result:= engineTmpl.lBankScale;
    KAMIKAZEDIST    : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nKamikazeDistance else Result:= engineTmpl.nKamikazeDistance;
    DAMAGEMODIFIER  : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.lDamageModifier else Result:= engineTmpl.lDamageModifier;

    WORKERTIME      : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nWorkerTime else Result:= engineTmpl.nWorkerTime;
    BUILDDIST       : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nBuildDistance else Result:= engineTmpl.nBuildDistance;

    SIGHTDIST       : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nSightDistance else Result:= engineTmpl.nSightDistance;
    RADARDIST       : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nRadarDistance else Result:= engineTmpl.nRadarDistance;
    SONARDIST       : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nSonarDistance else Result:= engineTmpl.nSonarDistance;
    MINCLOAKDIST    : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nMinCloakDistance else Result:= engineTmpl.nMinCloakDistance;
    RADARDISTJAM    : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nRadarDistanceJam else Result:= engineTmpl.nRadarDistanceJam;
    SONARDISTJAM    : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nSonarDistanceJam else Result:= engineTmpl.nSonarDistanceJam;

    MAKESMETAL      : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.cMakesMetal else Result:= engineTmpl.cMakesMetal;
    FENERGYMAKE     : if custTmplFound then Result:= Round(CustomUnitInfosArray[arrIdx].InfoStruct.fEnergyMake * 100) else Result:= Round(engineTmpl.fEnergyMake * 100);
    FMETALMAKE      : if custTmplFound then Result:= Round(CustomUnitInfosArray[arrIdx].InfoStruct.fMetalMake * 100) else Result:= Round(engineTmpl.fMetalMake * 100);
    FENERGYUSE      : if custTmplFound then Result:= Round(CustomUnitInfosArray[arrIdx].InfoStruct.fEnergyUse * 100) else Result:= Round(engineTmpl.fEnergyUse * 100);
    FMETALUSE       : if custTmplFound then Result:= Round(CustomUnitInfosArray[arrIdx].InfoStruct.fMetalUse * 100) else Result:= Round(engineTmpl.fMetalUse * 100);
    FENERGYSTOR     : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.lEnergyStorage else Result:= engineTmpl.lEnergyStorage;
    FMETALSTOR      : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.lMetalStorage else Result:= engineTmpl.lMetalStorage;
    FWINDGENERATOR  : if custTmplFound then Result:= Round(CustomUnitInfosArray[arrIdx].InfoStruct.fWindGenerator * 100) else Result:= Round(engineTmpl.fWindGenerator * 100);
    FTIDALGENERATOR : if custTmplFound then Result:= Round(CustomUnitInfosArray[arrIdx].InfoStruct.fTidalGenerator * 100) else Result:= Round(engineTmpl.fTidalGenerator * 100);
    FCLOAKCOST      : if custTmplFound then Result:= Round(CustomUnitInfosArray[arrIdx].InfoStruct.fCloakCost * 100) else Result:= Round(engineTmpl.fCloakCost * 100);
    FCLOAKCOSTMOVE  : if custTmplFound then Result:= Round(CustomUnitInfosArray[arrIdx].InfoStruct.fCloakCostMoving * 100) else Result:= Round(engineTmpl.fCloakCostMoving * 100);
  end;
end;

class function TAUnit.SetUnitInfoField(UnitPtr: Pointer; fieldType: LongWord; value: LongWord; remoteUnitId: PLongWord): Boolean;

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
      SOUNDCTGR : CustomUnitInfosArray[arrIdx].InfoStruct.nSoundCategory := Word(value);
      MOVEMENTCLASS_SAFE..MOVEMENTCLASS :
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
          if FieldType = MOVEMENTCLASS_SAFE then CreateMovementClass(LongWord(TAUnit.Id2Ptr(Word(unitId))));
          end else
            Exit;
        end;
      MAXHEALTH       : CustomUnitInfosArray[arrIdx].InfoStruct.nMaxHP := Word(value);
      HEALTIME        : CustomUnitInfosArray[arrIdx].InfoStruct.nHealTime := Word(value);

      MAXSPEED        : CustomUnitInfosArray[arrIdx].InfoStruct.lMaxSpeedRaw := value;
      ACCELERATION    : CustomUnitInfosArray[arrIdx].InfoStruct.lAcceleration := value;
      BRAKERATE       : CustomUnitInfosArray[arrIdx].InfoStruct.lBrakeRate := value;
      TURNRATE        : CustomUnitInfosArray[arrIdx].InfoStruct.nTurnRate := Word(value);
      CRUISEALT       : CustomUnitInfosArray[arrIdx].InfoStruct.nCruiseAlt := Word(value);
      MANEUVERLEASH   : CustomUnitInfosArray[arrIdx].InfoStruct.nManeuverLeashLen := Word(value);
      ATTACKRUNLEN    : CustomUnitInfosArray[arrIdx].InfoStruct.nAttackRunLength := Word(value);
      MAXWATERDEPTH   : CustomUnitInfosArray[arrIdx].InfoStruct.nMaxWaterDepth := SmallInt(value);
      MINWATERDEPTH   : CustomUnitInfosArray[arrIdx].InfoStruct.nMinWaterDepth := SmallInt(value);
      MAXSLOPE        : CustomUnitInfosArray[arrIdx].InfoStruct.cMaxSlope := ShortInt(value);
      MAXWATERSLOPE   : CustomUnitInfosArray[arrIdx].InfoStruct.cMaxWaterSlope := ShortInt(value);
      WATERLINE       : CustomUnitInfosArray[arrIdx].InfoStruct.cWaterLine := Byte(value);

      TRANSPORTSIZE   : CustomUnitInfosArray[arrIdx].InfoStruct.cTransportSize := Byte(value);
      TRANSPORTCAP    : CustomUnitInfosArray[arrIdx].InfoStruct.cTransportCap := Byte(value);

      BANKSCALE       : CustomUnitInfosArray[arrIdx].InfoStruct.lBankScale := value;
      KAMIKAZEDIST    : CustomUnitInfosArray[arrIdx].InfoStruct.nKamikazeDistance := Word(value);
      DAMAGEMODIFIER  : CustomUnitInfosArray[arrIdx].InfoStruct.lDamageModifier := value;

      WORKERTIME      : CustomUnitInfosArray[arrIdx].InfoStruct.nWorkerTime := Word(value);
      BUILDDIST       : CustomUnitInfosArray[arrIdx].InfoStruct.nBuildDistance := Word(value);

      SIGHTDIST :
        begin
        CustomUnitInfosArray[arrIdx].InfoStruct.nSightDistance := Word(value);
        TAUnit.UpdateLos(TAUnit.Id2Ptr(Word(unitId)));
        end;
      RADARDIST :
        begin
        CustomUnitInfosArray[arrIdx].InfoStruct.nRadarDistance := Word(value);
        TAUnit.UpdateLos(TAUnit.Id2Ptr(Word(unitId)));
        end;
      SONARDIST       : CustomUnitInfosArray[arrIdx].InfoStruct.nSonarDistance := Word(value);
      MINCLOAKDIST    : CustomUnitInfosArray[arrIdx].InfoStruct.nMinCloakDistance := Word(value);
      RADARDISTJAM    : CustomUnitInfosArray[arrIdx].InfoStruct.nRadarDistanceJam := Word(value);
      SONARDISTJAM    : CustomUnitInfosArray[arrIdx].InfoStruct.nSonarDistanceJam := Word(value);

      MAKESMETAL      : CustomUnitInfosArray[arrIdx].InfoStruct.cMakesMetal := Byte(value);
      FENERGYMAKE     : CustomUnitInfosArray[arrIdx].InfoStruct.fEnergyMake := value / 100;
      FMETALMAKE      : CustomUnitInfosArray[arrIdx].InfoStruct.fMetalMake := value / 100;
      FENERGYUSE      : CustomUnitInfosArray[arrIdx].InfoStruct.fEnergyUse := value / 100;
      FMETALUSE       : CustomUnitInfosArray[arrIdx].InfoStruct.fMetalUse := value / 100;
      FENERGYSTOR     : CustomUnitInfosArray[arrIdx].InfoStruct.lEnergyStorage := value;
      FMETALSTOR      : CustomUnitInfosArray[arrIdx].InfoStruct.lEnergyStorage := value;
      FWINDGENERATOR  : CustomUnitInfosArray[arrIdx].InfoStruct.fWindGenerator := value / 100;
      FTIDALGENERATOR : CustomUnitInfosArray[arrIdx].InfoStruct.fTidalGenerator := value / 100;
      FCLOAKCOST      : CustomUnitInfosArray[arrIdx].InfoStruct.fCloakCost := value / 100;
      FCLOAKCOSTMOVE  : CustomUnitInfosArray[arrIdx].InfoStruct.fCloakCostMoving := value / 100;

      EXPLODEAS       : CustomUnitInfosArray[arrIdx].InfoStruct.p_ExplodeAs := PLongWord(TAMem.GetWeapon(value));
      SELFDSTRAS      : CustomUnitInfosArray[arrIdx].InfoStruct.p_SelfDestructAsAs := PLongWord(TAMem.GetWeapon(value));

// 1 ?
// 2 standingfireorder
// 3 ?
// 4 init_cloaked
// 5 downloadable
      BUILDER         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 6);
// 7 zbuffer
      STEALTH         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 8);
      ISAIRBASE       : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 9);
      TARGETTINGUPGRADE   : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 10);
      CANFLY          : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 11);
      CANHOVER        : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 12);
      TELEPORTER      : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 13);
      HIDEDAMAGE      : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 14);
      SHOOTME         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 15);
// 16 UNIT HAS ANY WEAPON
// 17 armoredstate
// 18 activatewhenbuilt
      FLOATER         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 19);
// 20 upright
      AMPHIBIOUS      : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 21);
// 22 ?
// 23 internal command reload -> sub_42D1F0. probably reloads cob script 
// 24 isfeature
// 25 noshadow
      IMMUNETOPARALYZER  : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 26);
      HOVERATTACK     : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 27);
      KAMIKAZE        : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 28);
      ANTIWEAPONS     : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 29);
      DIGGER          : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 30);
// 31 has GUI (does gui file in guis folder exists with name of tested unit) ? sub_42d2e0

      ONOFFABLE       : SetUnitTypeMask(arrIdx, 1, (value = 1), 4);
      CANSTOP         : SetUnitTypeMask(arrIdx, 1, (value = 1), 8);
      CANATTACK       : SetUnitTypeMask(arrIdx, 1, (value = 1), 16);
      CANGUARD        : SetUnitTypeMask(arrIdx, 1, (value = 1), 32);
      CANPATROL       : SetUnitTypeMask(arrIdx, 1, (value = 1), 64);
      CANMOVE         : SetUnitTypeMask(arrIdx, 1, (value = 1), 128);
      CANLOAD         : SetUnitTypeMask(arrIdx, 1, (value = 1), 256);
      CANRECLAMATE    : SetUnitTypeMask(arrIdx, 1, (value = 1), 1024);
      CANRESURRECT    : SetUnitTypeMask(arrIdx, 1, (value = 1), 2048);
      CANCAPTURE      : begin SetUnitTypeMask(arrIdx, 1, (value = 1), 4096); SetUnitTypeMask(arrIdx, 1, (value = 1), 8192); end;
      CANDGUN         : SetUnitTypeMask(arrIdx, 1, (value = 1), 16384);
      SHOWPLAYERNAME  : SetUnitTypeMask(arrIdx, 1, (value = 1), 131072);
      COMMANDER       : SetUnitTypeMask(arrIdx, 1, (value = 1), 262144);
      CANTBERANSPORTED: SetUnitTypeMask(arrIdx, 1, (value = 1), 524288);
    end;
    Result:= True;
  end;
end;

end.
