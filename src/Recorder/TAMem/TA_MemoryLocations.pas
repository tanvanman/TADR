unit TA_MemoryLocations;

interface
uses
  dplay, Classes, TA_MemoryStructures;

type
  TAMem = class
  protected
    class Function GetLocalPlayerID : Byte;
    class function GetActivePlayersCount : Byte;
    class function GetGameTime : Integer;
    class Function GetGameSpeed : Byte;
    class function GetDevMode : Boolean;
    class function GetIsNetworkLayerEnabled : Boolean;

    class Function GetMaxUnitLimit : Word;
    class Function GetActualUnitLimit : Word;
    class Function GetIsAlteredUnitLimit : Boolean;
    class function GetUnitsPtr : Pointer;
    class function GetUnits_EndMarkerPtr : Pointer;
    class function GetMaxUnitId: LongWord;
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
    class function GetGameingType : TGameingType;
    class function GetAIDifficulty : TAIDifficulty;
    class procedure SetCameraToUnit(UnitPtr : Pointer);
    class procedure SetCameraFadeLevel(FadePercent : Integer);
    class function GetViewPlayerRaceSide: TTAPlayerSide;

    class Function GetPausedState : Boolean;
    class Procedure SetPausedState( value : Boolean);
  public
    Property Paused : Boolean read GetPausedState write SetPausedState;

    property LocalPlayerID : Byte read GetLocalPlayerId;
    property ActivePlayersCount : Byte read GetActivePlayersCount;
    property GameTime : Integer read GetGameTime;
    Property GameSpeed : Byte read GetGameSpeed;
    Property DevMode : Boolean read GetDevMode;
    Property NetworkLayerEnabled : Boolean read GetIsNetworkLayerEnabled;
    Property MaxUnitLimit : Word read GetMaxUnitLimit;
    Property ActualUnitLimit : Word read GetActualUnitLimit;
    Property IsAlteredUnitLimit : Boolean read GetIsAlteredUnitLimit;
    Property UnitsPtr : Pointer read GetUnitsPtr;
    Property Units_EndMarkerPtr : Pointer read GetUnits_EndMarkerPtr;
    Property MaxUnitsID : Cardinal read GetMaxUnitId;
    Property MainStructPtr : Pointer read GetMainStructPtr;
    Property ProgramStructPtr : Pointer read GetProgramStructPtr;
    Property PlayersStructPtr : Pointer read GetPlayersStructPtr;
    Property ModelsArrayPtr : Pointer read GetModelsArrayPtr;
    Property WeaponTypeDefArrayPtr : Pointer read GetWeaponTypeDefArrayPtr;
    Property FeatureTypeDefArrayPtr : Pointer read GetFeatureTypeDefArrayPtr;
    Property UnitInfosPtr : Pointer read GetUnitInfosPtr;
    Property UnitInfosCount : LongWord read GetUnitInfosCount;
    Property SwitchesMask : Word read GetSwitchesMask write SetSwitchesMask;
    Property GameingType : TGameingType read GetGameingType;
    Property AIDifficulty : TAIDifficulty read GetAIDifficulty;
    Property RaceSide : TTAPlayerSide read GetViewPlayerRaceSide;
    Property CameraToUnit : Pointer write SetCameraToUnit;
    class procedure ShakeCam(X, Y, Duration : Cardinal);

    class function ScriptActionName2Index(ActionName: PAnsiChar): Integer;
    class function GetModelPtr(index: Word): Pointer;
    class function UnitInfoId2Ptr(index: Word): Pointer;
    class function UnitInfoCrc2Ptr(CRC: Cardinal): Pointer;
    class function Crc32ToCrc24(CRC: Cardinal): Cardinal;
    class function MovementClassId2Ptr(index: Word): Pointer;
    class function WeaponId2Ptr(ID: Cardinal) : Pointer;
    class function FeatureDefId2Ptr(ID : Word) : Pointer;
    class function PlaceFeatureOnMap(FeatureName: String; Position: TPosition; Turn: TTurn): Boolean;
    class function RemoveMapFeature(X, Z : Integer; Method: Boolean): Boolean;
    class function ProtectMemoryRegion(Address : Cardinal; Writable: Boolean): Integer;
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
    class Function GetPlayerByDPID(playerPID : TDPID) : LongWord;

    class function PlayerIndex(player : PPlayerStruct) : Byte;
    class Function PlayerController(player : PPlayerStruct) : TTAPlayerController;
    class Function PlayerSide(player : PPlayerStruct) : TTAPlayerSide;
    class function PlayerLogoIndex(player : PPlayerStruct) : Byte;

    class function GetShareRadar(player : Pointer) : Boolean;
    class Procedure SetShareRadar(player : Pointer; value : Boolean);
    class function PositionInLOS(Player: PPlayerStruct; Position: PPosition): Boolean;

    class function IsKilled(player : Pointer) : Boolean;
    class function IsActive(player : Pointer) : Boolean;

    class function GetAlliedState(Player1 : Pointer; Player2 : Integer) : Boolean;
    class Procedure SetAlliedState(Player1 : Pointer; Player2 : Integer; value : Boolean);
  end;

  TAUnit = class
  public
    class Function GetKills(UnitPtr: Pointer): Word;
    class procedure SetKills(UnitPtr: Pointer; kills: Word);
    class Function GetHealth(UnitPtr: Pointer): Word;
    class function MakeDamage(MakerUnitPtr: Pointer; TakerUnitPtr: Pointer; DamageType: TDmgType; Amount: Cardinal): Word;
    class procedure SetHealth(UnitPtr: Pointer; health: LongWord);
    class Function GetCloak(UnitPtr: Pointer): LongWord;
    class procedure SetCloak(UnitPtr: Pointer; cloak: Word);

    class function GetUnitX(UnitPtr: Pointer): Word;
    class procedure SetUnitX(UnitPtr: Pointer; X: Word);
    class function GetUnitZ(UnitPtr: Pointer): Word;
    class procedure SetUnitZ(UnitPtr: Pointer; Z: Word);
    class function GetUnitY(UnitPtr: Pointer): Word;
    class procedure SetUnitY(UnitPtr: Pointer; Y: Word);

    class function GetTurnX(UnitPtr: Pointer): Word;
    class procedure SetTurnX(UnitPtr: Pointer; X: Word);
    class function GetTurnZ(UnitPtr: Pointer): Word;
    class procedure SetTurnZ(UnitPtr: Pointer; Z: Word);
    class function GetTurnY(UnitPtr: Pointer): Word;
    class procedure SetTurnY(UnitPtr: Pointer; Y: Word);

    class function GetMovementClass(UnitPtr: Pointer): Pointer;
    class function SetTemplate(UnitPtr: Pointer; newUnitInfo: Pointer): Boolean;

    class function GetWeapon(UnitPtr: Pointer; index: Cardinal): Cardinal;
    class function SetWeapon(UnitPtr: Pointer; index: Cardinal; NewWeaponID: Cardinal): Boolean;
    class function GetAttackerID(UnitPtr: Pointer): LongWord;
    class function GetTransporterUnit(UnitPtr: Pointer): Pointer;
    class function GetTransportingUnit(UnitPtr: Pointer): Pointer;
    class function GetPriorUnit(UnitPtr: Pointer): Pointer;
    class function GetRandomFreePiece(UnitPtr: Pointer; piecemin, piecemax : Integer) : Cardinal;
    class procedure AttachDetachUnit(Transported, Transporter: Pointer; Piece: Byte; Attach: Boolean);
    class function GetUnitAttachedTo(UnitPtr: Pointer; Piece: Byte): Pointer;
    class function GetLoadWeight(UnitPtr: Pointer): Integer;
    class function GetLoadCurAmount(UnitPtr: Pointer): Integer;

    class function UpdateLos(UnitPtr: Pointer): LongWord;
    class function FireWeapon(AttackerPtr : Pointer; WhichWeap : Byte; TargetUnitPtr : Pointer; TargetShortPosition : TShortPosition) : Integer;

    { position stuff }
    class function AtMouse: Pointer;
    class function AtPosition(Position: PPosition): Cardinal;
    class function Position2Grid(Position: TPosition; UnitInfo: Pointer; out GridPosX, GridPosZ: Word ): Boolean;
    class function GetCurrentSpeedPercent(UnitPtr: Pointer): Cardinal;
    class function GetCurrentSpeedVal(UnitPtr: Pointer): Cardinal;
    class procedure SetCurrentSpeed(UnitPtr: Pointer; NewSpeed: Cardinal);

    { creating and killing unit }
    class function TestBuildSpot(PlayerIndex: Byte; UnitInfo: Pointer; nPosX, nPosZ: Word ): Boolean;
    class function IsPlantYardOccupied(BuilderPtr: PUnitStruct; State: Integer): Boolean;
    class function TestAttachAtGridSpot(UnitInfo : Pointer; nPosX, nPosZ : Word): Boolean;
    class function CreateUnit(OwnerIndex: LongWord; UnitInfo: PUnitInfo; Position: TPosition; Turn: PTurn; TurnZOnly, RandomTurnZ: Boolean; UnitState: LongWord): Pointer;
    class Function GetBuildPercentLeft(UnitPtr : Pointer) : Cardinal;
    class Function GetMissingHealth(UnitPtr : Pointer) : Cardinal;
    class procedure Kill(UnitPtr : Pointer; deathtype: byte);
    class procedure SwapByKill(UnitPtr: Pointer; newUnitInfo: Pointer);

    { actions (orders, unit state) }
    class function GetCurrentOrderType(UnitPtr: Pointer): TTAActionType;
    class function GetCurrentOrderParams(UnitPtr: Pointer; Par: Byte): Cardinal;
    class function GetCurrentOrderState(UnitPtr: Pointer): Cardinal;
    class function GetCurrentOrderPos(UnitPtr: Pointer): Cardinal;
    class function GetCurrentOrderTargetUnit(UnitPtr: Pointer): Pointer;

    class function EditCurrentOrderParams(UnitPtr: Pointer; Par: Byte; NewValue: LongWord): Boolean;
    class function CreateMainOrder(UnitPtr: Pointer; TargetUnitPtr: Pointer; ActionType: TTAActionType; Position: PPosition; ShiftKey: Byte; Par1: LongWord; Par2: LongWord): LongInt;

    { COB }
    class Function GetCOBDataPtr(UnitPtr : Pointer) : Pointer;
    class function CallCobProcedure(UnitPtr: Pointer; ProcName: String; Par1, Par2, Par3, Par4: PLongWord): Cardinal;
    class function CallCobWithCallback(UnitPtr: Pointer; ProcName: String; Par1, Par2, Par3, Par4: LongWord): Cardinal;
    class function GetCobString(UnitPtr: Pointer): String;

    { id, owner, unit type etc. }
    class Function GetOwnerPtr(UnitPtr : Pointer) : Pointer;
    class Function GetOwnerIndex(UnitPtr : Pointer) : Integer;
    class Function GetId(UnitPtr : Pointer) : Word;
    class Function GetLongId(UnitPtr : Pointer) : LongWord;
    class Function Id2Ptr(LongUnitId : LongWord) : PUnitStruct;
    class Function Id2LongId(UnitId: Word) : LongWord;
    class function GetUnitInfoId(UnitPtr: Pointer): Word;
    class function GetUnitInfoCrc(UnitPtr: Pointer): Cardinal;
    class function GetUnitInfoPtr(UnitPtr: Pointer): Pointer;
    class Function IsOnThisComp(UnitPtr : Pointer; IncludeAI: Boolean) : Boolean;
    class function IsAllied(UnitPtr: Pointer; UnitId: LongWord): Byte;
    class Function IsRemoteIdLocal(UnitPtr: Pointer; remoteUnitId: PLongWord; out local: Boolean): LongWord;

    class Function IsUnitTypeInCategory(CategoryType: TUnitCategories; UnitInfo: PUnitInfo; TargetUnitInfo: PUnitInfo): Boolean;

    { cloning global template and setting its fields }
    class function GrantUnitInfo(UnitPtr: Pointer; State: Byte; remoteUnitId: PLongWord): Boolean;
    class function SearchCustomUnitInfos(unitId: LongWord; remoteUnitId: PLongWord; local: Boolean; out index: Integer ): Boolean;
    class function GetUnitInfoField(UnitPtr: Pointer; fieldType: TUnitInfoExtensions; remoteUnitId: PLongWord): LongWord;
    class function SetUnitInfoField(UnitPtr: Pointer; fieldType: TUnitInfoExtensions; value: Integer; remoteUnitId: PLongWord): Boolean;
  end;

  TAUnits = class
  public
    class Function CreateMinions(UnitPtr: Pointer; Amount: Byte; UnitInfo: Pointer; Action: TTAActionType; ArrayId: Cardinal): Integer;
    class procedure GiveUnit(ExistingUnitPtr: Pointer; PlayerIdx: Byte);
    { searching units in game }
    class function CreateSearchFilter(Mask: Integer): TUnitSearchFilterSet;
    class function GetRandomArrayId(ArrayType: Byte): Word;
    class function SearchUnits(UnitPtr: Pointer; SearchId: LongWord; SearchType: Byte; MaxDistance: Integer; Filter: TUnitSearchFilterSet; UnitTypeFilter: Pointer ): LongWord;
    class function UnitsIntoGetterArray(UnitPtr: Pointer; ArrayType: Byte; Id: LongWord; const UnitsArray: TFoundUnits): Boolean;
    class procedure ClearSearchRec(Id: LongWord; ArrayType: Byte);
    class procedure RandomizeSearchRec(Id: LongWord; ArrayType: Byte);
    class function Distance(Pos1, Pos2 : PPosition): Cardinal;
    class function CircleCoords(CenterPosition: TPosition; Radius: Integer; Angle: Integer; out x, z: Integer): Boolean;
  end;

  TASfx = class
  public
    class procedure Speech(UnitPtr: Pointer; SpeechType: Cardinal; Text: PAnsiChar);
    class function Play3DSound(EffectID: Cardinal; Position: TPosition; NetBroadcast: Boolean): Integer;
    class function PlayGafAnim(BmpType: Byte; X, Z: Word; Glow, Smoke: Byte): Integer;
    class function EmitSfxFromPiece(UnitPtr: PUnitStruct; TargetUnitPtr: PUnitStruct; PieceIdx: Integer; SfxType: Byte): Cardinal; 
    class function NanoParticles(StartPos: TPosition; TargetPos : TNanolathePos): Cardinal;
    class function NanoReverseParticles(StartPos: TPosition; TargetPos : TNanolathePos): Cardinal;
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

class Function TAMem.GetLocalPlayerId : Byte;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).cControlPlayerID;
end;

class Function TAMem.GetActivePlayersCount : Byte;
begin
result := Byte(PTAdynmemStruct(TAData.MainStructPtr).nActivePlayersCount);
end;

class Function TAMem.GetGameTime : Integer;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).lGameTime;
end;

class Function TAMem.GetGameSpeed : Byte;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).nTAGameSpeed;
end;

class Function TAMem.GetDevMode : Boolean;
begin
result := (PTAdynmemStruct(TAData.MainStructPtr).nGameState and 2) = 2;
end;

class function TAMem.GetIsNetworkLayerEnabled: Boolean;
begin
result := (PTAdynmemStruct(TAData.MainStructPtr).cNetworkLayerEnabled and 1) = 1;
end;

class function TAMem.GetMaxUnitLimit : Word;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).nMaxUnitLimitPerPlayer;
end;

class function TAMem.GetActualUnitLimit : Word;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).nPerMissionUnitLimit;
end;

class function TAMem.GetIsAlteredUnitLimit : Boolean;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).cAlteredUnitLimit <> 0;
end;

class function TAMem.GetUnitsPtr : Pointer;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).p_Units;
end;

class function TAMem.GetUnits_EndMarkerPtr : Pointer;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).p_LastUnitInArray;
end;

class function TAMem.GetMainStructPtr : Pointer;
begin
result := Pointer(PLongWord(TADynmemStructPtr)^);
end;

class function TAMem.GetProgramStructPtr : Pointer;
begin

result := PTAdynmemStruct(TAData.MainStructPtr).p_TAProgram;
end;

class function TAMem.GetPlayersStructPtr: Pointer;
begin
result := @PTAdynmemStruct(TAData.MainStructPtr).Players[0];
end;

class function TAMem.GetModelsArrayPtr : Pointer;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).p_MODEL_PTRS;
end;

class function TAMem.GetWeaponTypeDefArrayPtr : Pointer;
begin
  if IniSettings.WeaponType <= 256 then
    result := @PTAdynmemStruct(TAData.MainStructPtr).Weapons[0]
  else
    result := @PTAdynmemStruct(WeaponLimitPatchArr)^.Weapons[0];
end;

class function TAMem.GetFeatureTypeDefArrayPtr : Pointer;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.p_FeatureDefs;
end;

class function TAMem.GetUnitInfosPtr : Pointer;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).p_UnitDefs;
end;

class function TAMem.GetUnitInfosCount : LongWord;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).lNumUnitTypeDefs;
end;

class function TAMem.GetSwitchesMask : Word;
begin
Result:= PTAdynmemStruct(TAData.MainStructPtr).nSwitchesMask;
end;

class procedure TAMem.SetSwitchesMask(Mask: Word);
begin
PTAdynmemStruct(TAData.MainStructPtr).nSwitchesMask:= Mask;
end;

class Function TAMem.GetPausedState : Boolean;
begin
result := PTAdynmemStruct(TAData.MainStructPtr).cIsGamePaused <> 0;
end;

class Procedure TAMem.SetPausedState( value : Boolean);
begin
PTAdynmemStruct(TAData.MainStructPtr).cIsGamePaused := BoolValues[value];
end;

class function TAMem.GetModelPtr(index: Word): Pointer;
begin
result := PLongWord(LongWord(GetModelsArrayPtr) + index * 4);
end;

class function TAMem.UnitInfoId2Ptr(index: Word): Pointer;
begin
result := Pointer(LongWord(TAData.UnitInfosPtr) + index * SizeOf(TUnitInfo));
end;

class function TAMem.UnitInfoCrc2Ptr(CRC: Cardinal): Pointer;
var
  i, Max : Integer;
  CheckedUnitInfo : PUnitInfo;
begin
  Result := nil;
  if CRC = 0 then
    Exit;
  Max := TAData.UnitInfosCount;
  for i := 0 to Max do
  begin
    CheckedUnitInfo := TAMem.UnitInfoId2Ptr(i);
    if CheckedUnitInfo <> nil then
    begin
      if TAMem.Crc32ToCrc24(CheckedUnitInfo.CRC_FBI) = CRC then
      begin
        Result := CheckedUnitInfo;
        Break;
      end;
    end;
  end;
end;

class function TAMem.Crc32ToCrc24(CRC: Cardinal): Cardinal;
begin
  Result := CRC and not $FF000000;
end;

class function TAMem.MovementClassId2Ptr(index: Word): Pointer;
begin
result := Pointer(TAMovementClassArray + SizeOf(TMoveInfoClassStruct) * index);
end;

class function TAMem.WeaponId2Ptr(ID: Cardinal): Pointer;
begin
  if IniSettings.WeaponType <= 256 then
    result:= @PTAdynmemStruct(TAData.MainStructPtr).Weapons[ID]
  else
    result:= Pointer(Cardinal(TAData.WeaponTypeDefArrayPtr) + SizeOf(TWeaponDef) * ID);
end;

class function TAMem.FeatureDefId2Ptr(ID: Word): Pointer;
begin
  result := Pointer(LongWord(PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.p_FeatureDefs) + SizeOf(TFeatureDefStruct) * ID);
end;

class function TAMem.PlaceFeatureOnMap(FeatureName: String; Position: TPosition; Turn: TTurn): Boolean;
var
  FeatureId : SmallInt;
  GridPlot : PPlotGrid;
  x, z : Integer;
  //FeatureDef : PFeatureDefStruct;
begin
  Result := False;
  FeatureId := FeatureName2ID(PAnsiChar(FeatureName));

  if FeatureId = -1 then
    FeatureId := LoadFeature(PAnsiChar(FeatureName));

  if FeatureId <> -1 then
  begin
    //FeatureDef := TAmem.FeatureDefId2Ptr(FeatureId);
    z := Position.Z div 16;
    x := Position.X div 16;
    GridPlot := GetGridPosPLOT(x, z);
    Result := (SpawnFeatureOnMap(GridPlot, FeatureId, @Position, @Turn, 10) <> nil);
  end;
end;

class function TAMem.RemoveMapFeature(X, Z: Integer; Method: Boolean): Boolean;
var
  GridPlot : PPlotGrid;
begin
  GridPlot := GetGridPosPLOT(X div 16, Z div 16);
  Result := FEATURES_Destroy(GridPlot, Method);
end;

class function TAMem.GetMaxUnitId: LongWord;
begin
  if TAData.GameingType = gtMenu then
    result := TAData.MaxUnitLimit * MAXPLAYERCOUNT
  else
    result := TAData.ActualUnitLimit * MAXPLAYERCOUNT;
end;

class function TAMem.ScriptActionName2Index(ActionName: PAnsiChar): Integer;
var
  CurrCobScript : Cardinal;
  CurrCobScript_Int : Cardinal;
  CurrCobScript_Int2 : Cardinal;
begin
  CurrCobScript := PCardinal(COBScriptHandler_Begin)^;
  CurrCobScript_Int := (PCardinal(COBScriptHandler_End)^ - PCardinal(COBScriptHandler_Begin)^) div 25;
  if ( CurrCobScript_Int > 0 ) then
  begin
    repeat
      CurrCobScript_Int2 := CurrCobScript + 20 * (CurrCobScript_Int div 2) + 5 * (CurrCobScript_Int div 2);
      if ( AnsiCompareStr(UpperCase(PAnsiChar(PCardinal(CurrCobScript_Int2 + 21)^)), ActionName) < 0 ) then
      begin
        CurrCobScript := CurrCobScript_Int2 + 25;
        CurrCobScript_Int := CurrCobScript_Int - 1 - CurrCobScript_Int div 2;
      end else
        CurrCobScript_Int := CurrCobScript_Int div 2;

    until ( CurrCobScript_Int = 0 );
  end;
  if ( CurrCobScript = COBScriptHandler_End ) or
     ( AnsiCompareStr(UpperCase(PAnsiChar(PCardinal(CurrCobScript + 21)^)), ActionName) > 0 ) then
  begin
    Result := 0;
  end else
    Result := (CurrCobScript - PCardinal(COBScriptHandler_Begin)^) div 25;
end;

class function TAMem.GetGameingType: TGameingType;
begin
  Result := gtMenu;
  if PTAdynmemStruct(TAData.MainStructPtr).p_MapOTAFile <> nil then
    Result := TGameingType(PMapOTAFile(PTAdynmemStruct(TAData.MainStructPtr).p_MapOTAFile).MissionType);
end;

class function TAMem.GetAIDifficulty: TAIDifficulty;
begin
  Result := TAIDifficulty(PTAdynmemStruct(TAData.MainStructPtr).lCurrenTAIProfile);
end;

class function TAMem.GetViewPlayerRaceSide: TTAPlayerSide;
begin
  Result := TTAPlayerSide(PPlayerInfoStruct(PPlayerStruct(TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID)).PlayerInfo).Raceside);
end;

class procedure TAMem.SetCameraToUnit(UnitPtr: Pointer);
begin
  PTAdynmemStruct(TAData.MainStructPtr).pCameraToUnit := UnitPtr;
end;

class procedure TAMem.SetCameraFadeLevel(FadePercent : Integer);
begin

end;

class procedure TAMem.ShakeCam(X, Y, Duration: Cardinal);
begin
  PTAdynmemStruct(TAData.MainStructPtr).field_1432F :=
    (PTAdynmemStruct(TAData.MainStructPtr).field_1432F + Duration) div 2;
  PTAdynmemStruct(TAData.MainStructPtr).field_14333 := Duration;
  if ( X <> 0 ) then
    PTAdynmemStruct(TAData.MainStructPtr).ShakeMagnitude_1 :=
      PTAdynmemStruct(TAData.MainStructPtr).ShakeMagnitude_1 + X;
  if ( Y <> 0 ) then
    PTAdynmemStruct(TAData.MainStructPtr).ShakeMagnitude_2 :=
      PTAdynmemStruct(TAData.MainStructPtr).ShakeMagnitude_2 + Y;
  if ( PTAdynmemStruct(TAData.MainStructPtr).field_1432F > 0 ) then
    PTAdynmemStruct(TAData.MainStructPtr).cShake := PTAdynmemStruct(TAData.MainStructPtr).cShake or 1;
end;

class function TAMem.ProtectMemoryRegion(Address : Cardinal; Writable: Boolean): Integer;
begin
  if Writable then
    Result := AllowMemReadWrite(Address)
  else
    Result := SetMemReadOnly(Address);
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
result := nil;
if playerindex < MAXPLAYERCOUNT then
  result:= @PTAdynmemStruct(TAData.MainStructPtr).Players[playerIndex];
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
    result := i + 1;
    break;
    end;
  aplayerPID := Pointer(LongWord(aplayerPID)+SizeOf(TPlayerStruct));
  inc(i);
  end;
end;

class Function TAPlayer.PlayerIndex(player : PPlayerStruct) : Byte;
begin
  Result := Player.cPlayerIndex;
end;

class Function TAPlayer.PlayerController(player : PPlayerStruct) : TTAPlayerController;
begin
  Result := Player.cPlayerController;
end;

class Function TAPlayer.PlayerSide(player : PPlayerStruct) : TTAPlayerSide;
begin
  Result := TTAPlayerSide(Player.PlayerInfo.Raceside);
end;

class Function TAPlayer.PlayerLogoIndex(player : PPlayerStruct) : Byte;
begin
  Result := Player.PlayerInfo.PlayerLogoColor;
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

class function TAPlayer.PositionInLOS(Player: PPlayerStruct;
  Position: PPosition): Boolean;
var
  X, Z : Integer;
  Pos : Cardinal;
begin
  Result := False;
  if PositionInPlayerMapped(Player, Position) then
  begin
    X := Position.X div 32;
    Z := (Position.Z - (Position.Y div 2)) div 32;
    if (X < Player.lLOS_Width) and
       (Z < Player.lLOS_Height) then
    begin
      Pos := Z * Player.lLOS_Width + X;
      Result := PByte(Cardinal(Player.LOS_MEMORY) + Pos)^ <> 0;
    end;
  end;
end;

Class function TAPlayer.IsKilled(player : Pointer) : Boolean;
begin
result := PPlayerStruct(Player).PlayerInfo.PropertyMask and $40 = $40;
end;

Class function TAPlayer.IsActive(player : Pointer) : Boolean;
begin
result := PPlayerStruct(Player).lPlayerActive <> 0;
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

class function TAUnit.MakeDamage(MakerUnitPtr: Pointer; TakerUnitPtr: Pointer; DamageType: TDmgType; Amount: Cardinal) : Word;
var
  UnitInfoSt : PUnitInfo;
  Angle : Word;
  AtanX, AtanY : Integer;
begin
  UnitInfoSt := PUnitStruct(TakerUnitPtr).p_UnitDef;
  case DamageType of
    dtWeapon..dtParalyze :
      begin
        AtanY := PUnitStruct(TakerUnitPtr).Position.z - PUnitStruct(MakerUnitPtr).Position.z;
        AtanX := PUnitStruct(TakerUnitPtr).Position.x - PUnitStruct(MakerUnitPtr).Position.x;
        Angle := Word(TA_Atan2(AtanY, AtanX));
        UNITS_MakeDamage(MakerUnitPtr, TakerUnitPtr, Amount, Ord(DamageType), Angle);
      end;
    dtHeal :
      begin
        if PUnitStruct(TakerUnitPtr).nHealth < UnitInfoSt.lMaxHP then
          UNITS_MakeDamage(MakerUnitPtr, TakerUnitPtr, Amount, 10, 0);
      end;
    else
      UNITS_MakeDamage(MakerUnitPtr, TakerUnitPtr, Amount, Ord(DamageType), 0);
  end;
  Result := TAUnit.GetHealth(TakerUnitPtr);
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
PUnitStruct(UnitPtr).nGridPosX := (MakeLong(PUnitStruct(UnitPtr).Position.x_, X) - (SmallInt(PUnitInfo(TAMem.UnitInfoId2Ptr(PUnitStruct(UnitPtr).nUnitInfoID)).nFootPrintX shl 19)) + $80000) shr 20;
PUnitStruct(UnitPtr).nLargeGridPosX := PUnitStruct(UnitPtr).nGridPosX div 2;
end;

class function TAUnit.GetUnitZ(UnitPtr : Pointer): Word;
begin
result := PUnitStruct(UnitPtr).Position.Z;
end;

class procedure TAUnit.SetUnitZ(UnitPtr : Pointer; Z : Word);
begin
PUnitStruct(UnitPtr).Position.Z := Z;
PUnitStruct(UnitPtr).nGridPosZ := (MakeLong(PUnitStruct(UnitPtr).Position.z_, Z) - (SmallInt(PUnitInfo(TAMem.UnitInfoId2Ptr(PUnitStruct(UnitPtr).nUnitInfoID)).nFootPrintZ shl 19)) + $80000) shr 20;
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

class function TAUnit.SetTemplate(UnitPtr: Pointer; NewUnitInfo: Pointer): Boolean;
var
  NewTemplatePtr: PUnitInfo;
  UnitSt: PUnitStruct;
begin
  Result:= False;
  if (UnitPtr <> nil) and
     (NewUnitInfo <> nil) then
  begin
    NewTemplatePtr := NewUnitInfo;
    UnitSt := UnitPtr;
    UnitSt.nUnitInfoID := NewTemplatePtr.nCategory;
    UnitSt.p_UnitDef := NewTemplatePtr;
    Result := True;
  end;
end;

class function TAUnit.GetWeapon(UnitPtr : Pointer; index: Cardinal): Cardinal;
var
  Weapon: Pointer;
begin
  Result:= 0;
  if UnitPtr <> nil then
  begin
    case index of
      WEAPON_PRIMARY   : Weapon := PUnitStruct(UnitPtr).UnitWeapons[0].p_Weapon;
      WEAPON_SECONDARY : Weapon := PUnitStruct(UnitPtr).UnitWeapons[1].p_Weapon;
      WEAPON_TERTIARY  : Weapon := PUnitStruct(UnitPtr).UnitWeapons[2].p_Weapon;
      else Weapon := nil;
    end;
    if Weapon <> nil then
      if IniSettings.WeaponType <= 256 then
        Result := PWeaponDef(Weapon)^.ucID
      else
        Result := PWeaponDef(Weapon)^.lWeaponIDCrack;
  end;
end;

class function TAUnit.SetWeapon(UnitPtr : Pointer; index: Cardinal; NewWeaponID: Cardinal): Boolean;
var
  Weapon: Pointer;
begin
  Result:= False;
  if UnitPtr <> nil then
  begin
    Weapon:= TAMem.WeaponId2Ptr(NewWeaponID);
    case index of
      WEAPON_PRIMARY   : PUnitStruct(UnitPtr).UnitWeapons[0].p_Weapon := Weapon;
      WEAPON_SECONDARY : PUnitStruct(UnitPtr).UnitWeapons[1].p_Weapon := Weapon;
      WEAPON_TERTIARY  : PUnitStruct(UnitPtr).UnitWeapons[2].p_Weapon := Weapon;
    end;
    Result:= True;
  end;
end;

class function TAUnit.GetAttackerID(UnitPtr : Pointer): LongWord;
begin
result:= 0;
if PUnitStruct(UnitPtr).p_Attacker <> nil then
  result:= TAUnit.GetId(PUnitStruct(UnitPtr).p_Attacker);
end;

class function TAUnit.GetTransporterUnit(UnitPtr: Pointer): Pointer;
begin
result:= nil;
if PUnitStruct(UnitPtr).p_TransporterUnit <> nil then
  result:= PUnitStruct(UnitPtr).p_TransporterUnit;
end;

class function TAUnit.GetTransportingUnit(UnitPtr: Pointer): Pointer;
begin
result:= nil;
if PUnitStruct(UnitPtr).p_TransportedUnit <> nil then
  result:= PUnitStruct(UnitPtr).p_TransportedUnit;
end;

class function TAUnit.GetPriorUnit(UnitPtr: Pointer): Pointer;
begin
  result:= PUnitStruct(UnitPtr).p_PriorUnit;
end;

class function TAUnit.GetRandomFreePiece(UnitPtr: Pointer; piecemin, piecemax : Integer) : Cardinal;
var
 i, j : word;
 MaxElem : Integer;
 X : Cardinal;
 Pieces : array of Cardinal;
 PiecesLength : Integer;
 TransportedUnit : Pointer;
 AttachedToPiece : Byte;
begin
  Result := 0;
  Randomize;

  SetLength(Pieces, piecemax - piecemin + 1);
  for i := 0 to High(Pieces) do
  begin
    Pieces[i] := piecemin + i;
  end;

  TransportedUnit := TAUnit.GetTransportingUnit(UnitPtr);
  if TransportedUnit = nil then
  begin
    result := Random(piecemax - piecemin + 1) + piecemin;
    Exit;
  end;
  while (TransportedUnit <> nil) do
  begin
    if TransportedUnit <> nil then
    begin
      AttachedToPiece := PUnitStruct(TransportedUnit).ucAttachedToPiece;
      if AttachedToPiece <> 0 then
      begin
        for i := 0 to High(Pieces) do
        begin
          if Pieces[i] = AttachedToPiece then
          begin
            PiecesLength := Length(Pieces);
            for j := i + 1 to PiecesLength - 1 do
              Pieces[j - 1] := Pieces[j];
            SetLength(Pieces, PiecesLength - 1);
            Break;
          end;
        end;
      end;
    end;
    TransportedUnit := TAUnit.GetPriorUnit(TransportedUnit);
  end;

  MaxElem := High(Pieces);
  if High(Pieces) > 0 then
  begin
    for i := MaxElem downto 0 do
    begin
      j := Random(i) + 1;
      if not (i = j) then
      begin
        X := Pieces[i];
        Pieces[i] := Pieces[j];
        Pieces[j] := X;
      end;
    end;
    Result := Pieces[High(Pieces)];
  end else
    if High(Pieces) <> -1 then
      Result := Pieces[High(Pieces)];
end;

class procedure TAUnit.AttachDetachUnit(Transported, Transporter: Pointer; Piece: Byte; Attach: Boolean);
begin
  if Transported <> nil then
  begin
    if PUnitStruct(Transported).p_TransporterUnit <> nil then
      TA_AttachDetachUnit(Transported, nil, -1, 2);               // pop in transported units array
    if Attach then
      TA_AttachDetachUnit(Transported, Transporter, Piece, 0)
    else
      TA_AttachDetachUnit(Transported, nil, -1, 1);
  end;
end;

class function TAUnit.GetUnitAttachedTo(UnitPtr: Pointer; Piece: Byte): Pointer;
var
  MaxUnitId, UnitId : Cardinal;
  TestedUnit : PUnitStruct;
begin
  Result := nil;

  MaxUnitId := TAData.MaxUnitsID;
  for UnitId := 1 to MaxUnitId do
  begin
    TestedUnit := TAUnit.Id2Ptr(UnitId);
    if TAUnit.GetTransporterUnit(TestedUnit) = UnitPtr then
    begin
      if TestedUnit.ucAttachedToPiece = Piece then
      begin
        Result := TestedUnit;
        Exit;
      end;
    end;
  end;
end;

class function TAUnit.GetLoadWeight(UnitPtr: Pointer): Integer;
var
  TransportedIterr : Pointer;
  CurLoadWeight : Integer;
begin
  CurLoadWeight := 0;
  TransportedIterr := TAUnit.GetTransportingUnit(UnitPtr);
  while TransportedIterr <> nil do
  begin
    if TAUnit.GetTransporterUnit(TransportedIterr) = UnitPtr then
    begin
      CurLoadWeight := CurLoadWeight +
                       Round(PUnitInfo(TAUnit.GetUnitInfoPtr(TransportedIterr)).lBuildCostMetal);
    end;
    TransportedIterr := TAUnit.GetPriorUnit(TransportedIterr);
  end;
  Result := CurLoadWeight;
end;

class function TAUnit.GetLoadCurAmount(UnitPtr: Pointer): Integer;
var
  TransportedIterr : Pointer;
  i : Integer;
begin
  i := 0;
  TransportedIterr := TAUnit.GetTransportingUnit(UnitPtr);
  while TransportedIterr <> nil do
  begin
    if TAUnit.GetTransporterUnit(TransportedIterr) = UnitPtr then
    begin
      Inc(i);
    end;
    TransportedIterr := TAUnit.GetPriorUnit(TransportedIterr);
  end;
  Result := i;
end;

class function TAUnit.UpdateLos(UnitPtr: Pointer): LongWord;
begin
  UNITS_RebuildLOS(UnitPtr);
  //Result := TA_UpdateLOS(TAUnit.GetOwnerIndex(UnitPtr), 0);
  Result := TA_UpdateLOS(0);
end;

class function TAUnit.FireWeapon(AttackerPtr : Pointer; WhichWeap : Byte; TargetUnitPtr : Pointer; TargetShortPosition : TShortPosition) : Integer;
var
  WeapPtr : PWeaponDef;
  UnitSt : PUnitStruct;
  WeapTypeMask : Cardinal;
  WeapType : Integer;
  WeapStatePtr : Pointer;
  WeapTargetIdPtr : Pointer;
  TargetPosition : TPosition;
begin
  Result := 0;
  WeapType := -1;
  UnitSt := AttackerPtr;
  case WhichWeap of
    WEAPON_PRIMARY : WeapPtr := UnitSt.UnitWeapons[0].p_Weapon;
    WEAPON_SECONDARY : WeapPtr := UnitSt.UnitWeapons[1].p_Weapon;
    WEAPON_TERTIARY : WeapPtr := UnitSt.UnitWeapons[2].p_Weapon;
    else WeapPtr := nil;
  end;

  if (WeapPtr <> nil) and (AttackerPtr <> nil) then
  begin
    WeapTypeMask := WeapPtr.lWeaponTypeMask;
    if ((WeapTypeMask shr 19) and 1) = 1 then
    begin
      WeapType := 0;
    end else
    begin
      if ((WeapTypeMask shr 4) and 1) = 1 then
      begin
        WeapType := 1;
      end else
      begin
        if  ((WeapTypeMask and 1) = 1) or ((WeapTypeMask and $100000) = $100000) then
        begin
          WeapType := 3;
        end else
        begin
          WeapTypeMask := WeapTypeMask shr 8;
          if (WeapTypeMask and 1) = 1 then
            WeapType := 2;
        end;
      end;
    end;

    if WeapType <> -1 then
    begin
      if WeapPtr.p_FireCallback <> nil then
      begin
        case WhichWeap of
          WEAPON_PRIMARY :
            begin
              WeapStatePtr := @UnitSt.UnitWeapons[0].lState;
              WeapTargetIDPtr := @UnitSt.UnitWeapons[0].nTargetID;
            end;
          WEAPON_SECONDARY :
            begin
              WeapStatePtr := @UnitSt.UnitWeapons[1].lState;
              WeapTargetIDPtr := @UnitSt.UnitWeapons[1].nTargetID;
            end;
          WEAPON_TERTIARY :
            begin
              WeapStatePtr := @UnitSt.UnitWeapons[2].lState;
              WeapTargetIDPtr := @UnitSt.UnitWeapons[2].nTargetID;
            end;
          else begin
            WeapStatePtr := nil;
            WeapTargetIDPtr := nil;
          end;
        end;
        PLongWord(WeapStatePtr)^ := PLongWord(WeapStatePtr)^ or $1;
        if (TargetUnitPtr <> nil) then
          TargetPosition := PUnitStruct(TargetUnitPtr).Position
        else
          GetTPosition(TargetShortPosition.X, TargetShortPosition.Z, TargetPosition);

        case WeapType of
          0 : Result := fire_callback0(AttackerPtr, WeapTargetIDPtr, TargetUnitPtr, @TargetPosition);
          1 : Result := fire_callback1(AttackerPtr, WeapTargetIDPtr, TargetUnitPtr, @TargetPosition);
          2 : Result := fire_callback2(AttackerPtr, WeapTargetIDPtr, TargetUnitPtr, @TargetPosition);
          3 : Result := fire_callback3(AttackerPtr, WeapTargetIDPtr, TargetUnitPtr, @TargetPosition);
        end;
        PLongWord(WeapStatePtr)^ := PLongWord(WeapStatePtr)^ and not $1;
      end;
    end;
  end;
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

class function TAUnit.AtPosition(Position: PPosition): Cardinal;
var
  UnkStruct : Pointer;
begin
  Result := 0;
  if Position <> nil then
  begin
    UnkStruct := UnitAtPosition(Position);
    Result := Word(PCardinal(UnkStruct)^);
  end;
end;

class function TAUnit.Position2Grid(Position: TPosition; UnitInfo: Pointer; out GridPosX, GridPosZ: Word ): Boolean;
var
  PosX, PosZ: Integer;
begin
 // Result := False;
  try
    PosX := MakeLong(Position.x_, Position.X);
    GridPosX := (PosX - (SmallInt(PUnitInfo(UnitInfo).nFootPrintX shl 19)) + $80000) shr 20;
    //GridPosX := (PosX - (PUnitInfo(UnitInfo).nFootPrintX shl 19) + $80000) shr 20;
    PosZ := MakeLong(0, Position.Z);
    GridPosZ := (PosZ - (SmallInt(PUnitInfo(UnitInfo).nFootPrintZ shl 19)) + $80000) shr 20;
    //GridPosZ := (PosZ - (PUnitInfo(UnitInfo).nFootPrintZ shl 19) + $80000) shr 20;
  finally
    Result := True;
  end;
end;

class function TAUnit.GetCurrentSpeedPercent(UnitPtr: Pointer): Cardinal;
begin
  result := 0;
  if PUnitStruct(UnitPtr).p_MovementClass <> nil then
  begin
    if PUnitStruct(UnitPtr).lSfxOccupy = 4 then
      result := Trunc(((PMoveClass(PUnitStruct(UnitPtr).p_MovementClass).lCurrentSpeed) /
                        (PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).lMaxSpeedRaw)) * 100)
    else
      result := Trunc(((PMoveClass(PUnitStruct(UnitPtr).p_MovementClass).lCurrentSpeed) /
                        Trunc((PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).lMaxSpeedRaw) / 2)) * 100);
    if result > 100 then
      result := 100;
  end;
end;

class function TAUnit.GetCurrentSpeedVal(UnitPtr: Pointer): Cardinal;
begin
  result := 0;
  if PUnitStruct(UnitPtr).p_MovementClass <> nil then
  begin
    result := (PMoveClass(PUnitStruct(UnitPtr).p_MovementClass).lCurrentSpeed);
  end;
end;

class procedure TAUnit.SetCurrentSpeed(UnitPtr: Pointer; NewSpeed: Cardinal); // percentage
begin
  if PUnitStruct(UnitPtr).p_MovementClass <> nil then
    PMoveClass(PUnitStruct(UnitPtr).p_MovementClass).lCurrentSpeed := NewSpeed;
end;

class function TAUnit.TestBuildSpot(PlayerIndex: Byte; UnitInfo: Pointer; nPosX, nPosZ: Word ): Boolean;
var
  TestPos: LongWord;
  UnitInfoSt: PUnitInfo;
  PlayerSt: PPlayerStruct;
  GridPosX, GridPosZ: Word;
  Pos : TPosition;
begin
  Result:= False;
  UnitInfoSt := UnitInfo;
  PlayerSt := TAPlayer.GetPlayerByIndex(PlayerIndex);

  Pos.X := nPosX;
  Pos.Z := nPosZ;
  if (UnitInfoSt <> nil) and (PlayerSt <> nil) then
    if TAUnit.Position2Grid(Pos, UnitInfoSt, GridPosX, GridPosZ ) then
    begin
      TestPos := MakeLong(GridPosX, GridPosZ);
      Result := (TestGridSpot(UnitInfoSt, TestPos, 0, PlayerSt) = 1);
    end;
end;

class function TAUnit.IsPlantYardOccupied(BuilderPtr: PUnitStruct; State: Integer): Boolean;
begin
  Result := CanCloseOrOpenYard(BuilderPtr, State);
end;

class function TAUnit.TestAttachAtGridSpot(UnitInfo : Pointer; nPosX, nPosZ : Word): Boolean;
var
  GridPos : Cardinal;
begin
  GridPos := MakeLong(nPosX, nPosZ);
  //SendTextLocal(IntToStr(nPosX) + ' : ' +  IntToStr(nPosZ));
  Result := CanAttachAtGridSpot(UnitInfo, 0, GridPos, 1);
end;

class function TAUnit.CreateUnit( OwnerIndex: LongWord;
                                  UnitInfo: PUnitInfo;
                                  Position: TPosition;
                                  Turn: PTurn;
                                  TurnZOnly: Boolean;
                                  RandomTurnZ: Boolean;
                                  UnitState: LongWord ): Pointer;
var
  UnitSt: PUnitStruct;
begin
  if OwnerIndex = 10 then
    OwnerIndex := TAData.LocalPlayerID;
    
  Result := UNITS_Create( OwnerIndex,
                          UnitInfo.nCategory,
                          MakeLong(0, Position.X),
                          MakeLong(0, Position.Y),
                          MakeLong(0, Position.Z),
                          1,
                          UnitState,
                          0 );



  if (Result <> nil) then
  begin
    UnitSt := Pointer(Result);
    if (Turn <> nil) then
    begin
      UnitSt.Turn.Z := Turn.Z;
      if not TurnZOnly then
      begin
        UnitSt.Turn.X := Turn.X;
        UnitSt.Turn.Y := Turn.Y;
      end;
    end else
    if RandomTurnZ then
    begin
      UnitSt.Turn.Z := Random(High(SmallInt));
    end;
  end;
end;

class Function TAUnit.GetBuildPercentLeft(UnitPtr : Pointer) : Cardinal;
begin
  if ( PUnitStruct(UnitPtr).lBuildTimeLeft = 0.0 ) then
    Result := 0
  else
    Result := Trunc(1 - (PUnitStruct(UnitPtr).lBuildTimeLeft * -99.0));
end;

class Function TAUnit.GetMissingHealth(UnitPtr : Pointer) : Cardinal;
var
  ShouldBe : Cardinal;
  IsCurr : Cardinal;
  UnitInfo : PUnitInfo;
begin
  Result := 0;
  if (UnitPtr <> nil) then
    if PUnitStruct(UnitPtr).nUnitInfoID <> 0 then
    begin
      UnitInfo := TAMem.UnitInfoId2Ptr(PUnitStruct(UnitPtr).nUnitInfoID);
      if ( PUnitStruct(UnitPtr).lBuildTimeLeft = 0.0 ) then
        Result := 0
      else begin
        ShouldBe := Round((1 - PUnitStruct(UnitPtr).lBuildTimeLeft) * UnitInfo.lMaxHP);
        IsCurr := PUnitStruct(UnitPtr).nHealth;
        if IsCurr < ShouldBe then
          Result := ShouldBe - IsCurr;
      end;
    end;
end;

class procedure TAUnit.Kill(UnitPtr : Pointer; deathtype: byte);
begin
  if UnitPtr <> nil then
  begin
    case deathtype of
      0 : UNITS_MakeDamage(nil, UnitPtr, 30000, 3, 0);
      1 : UnitExplosion(UnitPtr, 0);
      2 : UnitExplosion(UnitPtr, 1);
      3 : UNITS_MakeDamage(nil, UnitPtr, 30000, 4, 0);
      4 : TAUnit.CreateMainOrder(UnitPtr, nil, Action_SelfDestruct, nil, 1, 0, 0);
      5 : PUnitStruct(UnitPtr).lUnitStateMask:= PUnitStruct(UnitPtr).lUnitStateMask or $4000;
    end;
    if (deathtype = 1) or (deathtype = 2) then
      UNITS_MakeDamage(nil, UnitPtr, 30000, 4, 0);
  end;
end;

class procedure TAUnit.SwapByKill(UnitPtr: Pointer; newUnitInfo: Pointer);
var
  UnitSt: PUnitStruct;
  Position: TPosition;
  Turn: TTurn;
begin
  if (UnitPtr <> nil) and
     (newUnitInfo <> nil) then
  begin
    UnitSt := UnitPtr;
    Position := UnitSt.Position;
    Turn := UnitSt.Turn;
    if TAUnit.CreateUnit(TAUnit.GetOwnerIndex(UnitPtr), newUnitInfo, Position, @Turn, True, False, 1) <> nil then
      TAUnit.Kill(UnitPtr, 3);
  end;
end;

class function TAUnit.GetCurrentOrderType(UnitPtr: Pointer): TTAActionType;
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
    if Par = 1 then
      Result := PUnitOrder(PUnitStruct(UnitPtr).p_UnitOrders).lPar1
    else
      Result := PUnitOrder(PUnitStruct(UnitPtr).p_UnitOrders).lPar2;
  end;
end;

class function TAUnit.GetCurrentOrderState(UnitPtr: Pointer): Cardinal;
begin
  Result := 0;
  if PUnitStruct(UnitPtr).p_UnitOrders <> nil then
    Result := PUnitOrder(PUnitStruct(UnitPtr).p_UnitOrders).lOrder_State;
end;

class function TAUnit.GetCurrentOrderPos(UnitPtr: Pointer): Cardinal;
var
  tempx, tempz : Word;
begin
  Result := 0;
  if PUnitStruct(UnitPtr).p_UnitOrders <> nil then
  begin
    tempx := PUnitOrder(PUnitStruct(UnitPtr).p_UnitOrders).Pos.X;
    tempz := PUnitOrder(PUnitStruct(UnitPtr).p_UnitOrders).Pos.Z;
    Result := MakeLong(tempz, tempx);
  end;
end;

class function TAUnit.GetCurrentOrderTargetUnit(UnitPtr: Pointer): Pointer;
begin
  Result := nil;
  if PUnitStruct(UnitPtr).p_UnitOrders <> nil then
  begin
    Result := PUnitOrder(PUnitStruct(UnitPtr).p_UnitOrders).p_UnitTarget;
  end;
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

class function TAUnit.CreateMainOrder(UnitPtr: Pointer; TargetUnitPtr: Pointer; ActionType: TTAActionType; Position: PPosition; ShiftKey: Byte; Par1: LongWord; Par2: LongWord): LongInt;
begin
  Result:= Order2Unit(Ord(ActionType), ShiftKey, UnitPtr, TargetUnitPtr, Position, Par1, Par2);
end;

class Function TAUnit.GetCOBDataPtr(UnitPtr : Pointer) : Pointer;
begin
result := PUnitStruct(UnitPtr).p_UnitScriptsData;
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
    Result := Script_RunScript ( 0, 0, LongWord(UnitSt.p_UnitScriptsData),
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
    Result := Script_ProcessCallback( nil, nil, LongWord(UnitSt.p_UnitScriptsData),
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
Result := 0;
if UnitPtr <> nil then
//Result := PPlayerStruct(PUnitStruct(UnitPtr).p_Owner).cPlayerIndexZero;
  Result := PUnitStruct(UnitPtr).cMyLOSPlayerID;
end;

class Function TAUnit.GetId(UnitPtr : Pointer) : Word;
begin
Result := 0;
if UnitPtr <> nil then
  result := Word(PUnitStruct(UnitPtr).lUnitInGameIndex);
end;

class Function TAUnit.GetLongId(UnitPtr : Pointer) : LongWord;
begin
result := PUnitStruct(UnitPtr).lUnitInGameIndex;
end;

class Function TAUnit.Id2Ptr(LongUnitId : LongWord) : PUnitStruct;
begin
  Result := nil;
  if LongUnitId <> 0 then
  begin
    if (Word(LongUnitId) <= TAData.MaxUnitsID) then
      result := Pointer(LongWord(TAData.UnitsPtr) + SizeOf(TUnitStruct)*Word(LongUnitId));
    if Cardinal(result) > Cardinal(TAData.Units_EndMarkerPtr) then
    begin
      TLog.Add(0, 'Error : Tried to access unit ID too high');
      Result := nil;
    end;
  end;
end;

class Function TAUnit.Id2LongId(UnitId: Word) : LongWord;
var
  UnitPtr: Pointer;
begin
  result:= 0;
  if TAData.UnitsPtr <> nil then
  begin
    UnitPtr:= TAUnit.Id2Ptr(UnitId);
    if PUnitStruct(UnitPtr).nUnitInfoID <> 0 then
      result:= PUnitStruct(UnitPtr).lUnitInGameIndex;
  end;
end;

class function TAUnit.GetUnitInfoId(UnitPtr: Pointer): Word;
begin
result:= PUnitStruct(UnitPtr).nUnitInfoID;
end;

class function TAUnit.GetUnitInfoCrc(UnitPtr: Pointer): Cardinal;
begin
result := 0;
if TAUnit.GetUnitInfoPtr(UnitPtr) <> nil then
  result := PUnitInfo(TAUnit.GetUnitInfoPtr(UnitPtr)).CRC_FBI;
end;

class function TAUnit.GetUnitInfoPtr(UnitPtr: Pointer) : Pointer;
begin
result:= PUnitStruct(UnitPtr).p_UnitDef;
end;

class Function TAUnit.IsOnThisComp(UnitPtr : Pointer; IncludeAI: Boolean) : Boolean;
var
  playerPtr : Pointer;
  TAPlayerType : TTAPlayerController;
begin
result:= False;
try
  playerPtr := TAUnit.GetOwnerPtr(Pointer(UnitPtr));
  //playerPtr := TAPlayer.GetPlayerByIndex(TAunit.GetOwnerIndex(UnitPtr));
  if playerPtr <> nil then
  begin
    TAPlayerType := TAPlayer.PlayerController(playerPtr);
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
  playerindex: Integer;
begin
  playerPtr := TAUnit.GetOwnerPtr(unitptr);
  Unit2Ptr := TAUnit.Id2Ptr(Word(UnitId));
  playerIndex := TAUnit.GetOwnerIndex(Unit2Ptr);
  result := BoolValues[TAPlayer.GetAlliedState(playerPtr,playerIndex)];
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

class Function TAUnit.IsUnitTypeInCategory(CategoryType: TUnitCategories; UnitInfo: PUnitInfo; TargetUnitInfo: PUnitInfo): Boolean;
var
  pCategory : Pointer;
  UnitInfoIdMask : Cardinal;
  CategoryMask : Cardinal;
begin
  Result := False;
  pCategory := nil;
  if (UnitInfo <> nil) and
     (TargetUnitInfo <> nil) then
  begin
    case CategoryType of
      ucsNoChase : pCategory := UnitInfo.p_NoChaseCategoryMaskArray;
      ucsPriBadTarget : pCategory := UnitInfo.p_WeaponPrimaryBadTargetCategoryArray;
      ucsSecBadTarget : pCategory := UnitInfo.p_WeaponSecondaryBadTargetCategoryArray;
      ucsTerBadTarget : pCategory := UnitInfo.p_WeaponSpecialBadTargetCategoryArray;
    end;
    if pCategory <> nil then
    begin
      UnitInfoIdMask := 1 shl (TargetUnitInfo.nCategory and $1F);
      CategoryMask := PCardinal(Cardinal(pCategory) + 4 * (TargetUnitInfo.nCategory shr 5))^;
      if (UnitInfoIdMask and CategoryMask) <> 0 then
        Result := True;
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
    TmpUnitInfo.InfoStruct:= PUnitInfo(TmpUnitInfo.InfoPtrOld)^;
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
        CustomUnitInfosArray[arrIdx].InfoStruct:= PUnitInfo(CustomUnitInfosArray[arrIdx].InfoPtrOld)^;
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
  engineTmpl, UseTemplate: PUnitInfo;
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
    UNITINFO_MAXHEALTH       : Result := UseTemplate.lMaxHP;
    UNITINFO_HEALTIME        : Result := UseTemplate.nHealTime;

    UNITINFO_MAXSPEED        : Result := Trunc(UseTemplate.lMaxSpeedRaw * 100);
    UNITINFO_ACCELERATION    : Result := Trunc(UseTemplate.lAcceleration * 100);
    UNITINFO_BRAKERATE       : Result := Trunc(UseTemplate.lBrakeRate * 100);
    UNITINFO_TURNRATE        : Result := UseTemplate.nTurnRate;
    UNITINFO_CRUISEALT       : Result := UseTemplate.nCruiseAlt;
    UNITINFO_MANEUVERLEASH   : Result := UseTemplate.nManeuverLeashLen;
    UNITINFO_ATTACKRUNLEN    : Result := UseTemplate.nAttackRunLength;
    UNITINFO_MAXWATERDEPTH   : Result := UseTemplate.nMaxWaterDepth;
    UNITINFO_MINWATERDEPTH   : Result := UseTemplate.nMinWaterDepth;
    UNITINFO_MAXSLOPE        : Result := UseTemplate.cMaxSlope;
    UNITINFO_MAXWATERSLOPE   : Result := UseTemplate.cMaxWaterSlope;
    UNITINFO_WATERLINE       : Result := UseTemplate.cWaterLine;

    UNITINFO_TRANSPORTSIZE   : Result := UseTemplate.cTransportSize;
    UNITINFO_TRANSPORTCAP    : Result := UseTemplate.cTransportCap;

    UNITINFO_BANKSCALE       : Result := UseTemplate.lBankScale;
    UNITINFO_KAMIKAZEDIST    : Result := UseTemplate.nKamikazeDistance;
    UNITINFO_DAMAGEMODIFIER  : Result := UseTemplate.lDamageModifier;

    UNITINFO_WORKERTIME      : Result := UseTemplate.nWorkerTime;
    UNITINFO_BUILDDIST       : Result := UseTemplate.nBuildDistance;

    UNITINFO_SIGHTDIST       : Result := UseTemplate.nSightDistance;
    UNITINFO_RADARDIST       : Result := UseTemplate.nRadarDistance;
    UNITINFO_SONARDIST       : Result := UseTemplate.nSonarDistance;
    UNITINFO_MINCLOAKDIST    : Result := UseTemplate.nMinCloakDistance;
    UNITINFO_RADARDISTJAM    : Result := UseTemplate.nRadarDistanceJam;
    UNITINFO_SONARDISTJAM    : Result := UseTemplate.nSonarDistanceJam;

    UNITINFO_MAKESMETAL      : Result := UseTemplate.cMakesMetal;
    UNITINFO_FENERGYMAKE     : Result := Trunc(UseTemplate.fEnergyMake * 100);
    UNITINFO_FMETALMAKE      : Result := Trunc(UseTemplate.fMetalMake * 100);
    UNITINFO_FENERGYUSE      : Result := Abs(Trunc(UseTemplate.fEnergyUse * 100));
    UNITINFO_FMETALUSE       : Result := Abs(Trunc(UseTemplate.fMetalUse * 100));
    UNITINFO_FENERGYSTOR     : Result := UseTemplate.lEnergyStorage;
    UNITINFO_FMETALSTOR      : Result := UseTemplate.lMetalStorage;
    UNITINFO_FWINDGENERATOR  : Result := Trunc(UseTemplate.fWindGenerator * 100);
    UNITINFO_FTIDALGENERATOR : Result := Trunc(UseTemplate.fTidalGenerator * 100);
    UNITINFO_FCLOAKCOST      : Result := Trunc(UseTemplate.fCloakCost * 100);
    UNITINFO_FCLOAKCOSTMOVE  : Result := Trunc(UseTemplate.fCloakCostMoving * 100);

    UNITINFO_BUILDCOSTMETAL  : Result := Trunc(UseTemplate.lBuildCostMetal);
    UNITINFO_BUILDCOSTENERGY : Result := Trunc(UseTemplate.lBuildCostEnergy);

    UNITINFO_BUILDTIME       : Result := UseTemplate.lBuildTime;

// 1 ?
// 2 standingfireorder
// 3 ?
// 4 init_cloaked
// 5 downloadable
    UNITINFO_BUILDER         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 6)) > 0 );
// 7 zbuffer
    UNITINFO_STEALTH         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 8)) > 0 );
    UNITINFO_ISAIRBASE       : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 9)) > 0 );
    UNITINFO_TARGETTINGUPGRADE   : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 10)) > 0 );
    UNITINFO_CANFLY          : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 11)) > 0 );
    UNITINFO_CANHOVER        : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 12)) > 0 );
    UNITINFO_TELEPORTER      : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 13)) > 0 );
    UNITINFO_HIDEDAMAGE      : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 14)) > 0 );
    UNITINFO_SHOOTME         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 15)) > 0 );
    UNITINFO_HASWEAPONS      : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 16)) > 0 );
// 17 armoredstate
// 18 activatewhenbuilt
    UNITINFO_FLOATER         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 19)) > 0 );
    UNITINFO_UPRIGHT         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 20)) > 0 );
    UNITINFO_BMCODE          : Result := UseTemplate.cBMCode;
    UNITINFO_AMPHIBIOUS      : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 21)) > 0 );
// 22 ?
// 23 internal command reload -> sub_42D1F0. probably reloads cob script
    UNITINFO_ISFEATURE       : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 24)) > 0 );
// 25 noshadow
    UNITINFO_IMMUNETOPARALYZER  : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 26)) > 0 );
    UNITINFO_HOVERATTACK     : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 27)) > 0 );
    UNITINFO_KAMIKAZE        : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 28)) > 0 );
    UNITINFO_ANTIWEAPONS     : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 29)) > 0 );
    UNITINFO_DIGGER          : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 30)) > 0 );
// 31 has GUI (does gui file in guis folder exists with name of tested unit) ? sub_42d2e0

    UNITINFO_ONOFFABLE       : Result := Byte((UseTemplate.UnitTypeMask2 and 4) > 0 );
    UNITINFO_CANSTOP         : Result := Byte((UseTemplate.UnitTypeMask2 and 8) > 0 );
    UNITINFO_CANATTACK       : Result := Byte((UseTemplate.UnitTypeMask2 and 16) > 0 );
    UNITINFO_CANGUARD        : Result := Byte((UseTemplate.UnitTypeMask2 and 32) > 0 );
    UNITINFO_CANPATROL       : Result := Byte((UseTemplate.UnitTypeMask2 and 64) > 0 );
    UNITINFO_CANMOVE         : Result := Byte((UseTemplate.UnitTypeMask2 and 128) > 0 );
    UNITINFO_CANLOAD         : Result := Byte((UseTemplate.UnitTypeMask2 and 256) > 0 );
    UNITINFO_CANRECLAMATE    : Result := Byte((UseTemplate.UnitTypeMask2 and 1024) > 0 );
    UNITINFO_CANRESURRECT    : Result := Byte((UseTemplate.UnitTypeMask2 and 2048) > 0 );
    UNITINFO_CANCAPTURE      : Result := Byte((UseTemplate.UnitTypeMask2 and 4096) > 0 );
    UNITINFO_CANDGUN         : Result := Byte((UseTemplate.UnitTypeMask2 and 16384) > 0 );
    UNITINFO_SHOWPLAYERNAME  : Result := Byte((UseTemplate.UnitTypeMask2 and 131072) > 0 );
    UNITINFO_COMMANDER       : Result := Byte((UseTemplate.UnitTypeMask2 and 262144) > 0 );
    UNITINFO_CANTBERANSPORTED: Result := Byte((UseTemplate.UnitTypeMask2 and 524288) > 0 );
  end;
end;

class function TAUnit.SetUnitInfoField(UnitPtr: Pointer; fieldType: TUnitInfoExtensions; value: Integer; remoteUnitId: PLongWord): Boolean;
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
      UNITINFO_SOUNDCTGR : CustomUnitInfosArray[arrIdx].InfoStruct.nSoundCategory := Word(value);
      UNITINFO_MOVEMENTCLASS_SAFE..UNITINFO_MOVEMENTCLASS :
        begin
        // movement class information is stored in both - specific and global template
        // we fix both of them to make TA engine happy
        CustomUnitInfosArray[arrIdx].InfoStruct.p_MovementClass:= TAMem.MovementClassId2Ptr(Word(value));
        MovementClassStruct:= CustomUnitInfosArray[arrIdx].InfoStruct.p_MovementClass;
        if MovementClassStruct.pName <> nil then
        begin
          CustomUnitInfosArray[arrIdx].InfoStruct.nMaxWaterDepth:= MovementClassStruct.nMaxWaterDepth;
          CustomUnitInfosArray[arrIdx].InfoStruct.nMinWaterDepth:= MovementClassStruct.nMinWaterDepth;
          CustomUnitInfosArray[arrIdx].InfoStruct.cMaxSlope:= MovementClassStruct.cMaxSlope;
          CustomUnitInfosArray[arrIdx].InfoStruct.cMaxWaterSlope:= MovementClassStruct.cMaxWaterSlope;
          if FieldType = UNITINFO_MOVEMENTCLASS_SAFE then
            CreateMovementClass(LongWord(TAUnit.Id2Ptr(Word(unitId))));
        end else
            Exit;
        end;
      UNITINFO_MAXHEALTH       : CustomUnitInfosArray[arrIdx].InfoStruct.lMaxHP := Word(value);
      UNITINFO_HEALTIME        : CustomUnitInfosArray[arrIdx].InfoStruct.nHealTime := Word(value);

      UNITINFO_MAXSPEED        : CustomUnitInfosArray[arrIdx].InfoStruct.lMaxSpeedRaw := value;
      UNITINFO_ACCELERATION    : CustomUnitInfosArray[arrIdx].InfoStruct.lAcceleration := value / 100;
      UNITINFO_BRAKERATE       : CustomUnitInfosArray[arrIdx].InfoStruct.lBrakeRate := value / 100;
      UNITINFO_TURNRATE        : CustomUnitInfosArray[arrIdx].InfoStruct.nTurnRate := Word(value);
      UNITINFO_CRUISEALT       : CustomUnitInfosArray[arrIdx].InfoStruct.nCruiseAlt := Word(value);
      UNITINFO_MANEUVERLEASH   : CustomUnitInfosArray[arrIdx].InfoStruct.nManeuverLeashLen := Word(value);
      UNITINFO_ATTACKRUNLEN    : CustomUnitInfosArray[arrIdx].InfoStruct.nAttackRunLength := Word(value);
      UNITINFO_MAXWATERDEPTH   : CustomUnitInfosArray[arrIdx].InfoStruct.nMaxWaterDepth := SmallInt(value);
      UNITINFO_MINWATERDEPTH   : CustomUnitInfosArray[arrIdx].InfoStruct.nMinWaterDepth := SmallInt(value);
      UNITINFO_MAXSLOPE        : CustomUnitInfosArray[arrIdx].InfoStruct.cMaxSlope := ShortInt(value);
      UNITINFO_MAXWATERSLOPE   : CustomUnitInfosArray[arrIdx].InfoStruct.cMaxWaterSlope := ShortInt(value);
      UNITINFO_WATERLINE       : CustomUnitInfosArray[arrIdx].InfoStruct.cWaterLine := Byte(value);

      UNITINFO_TRANSPORTSIZE   : CustomUnitInfosArray[arrIdx].InfoStruct.cTransportSize := Byte(value);
      UNITINFO_TRANSPORTCAP    : CustomUnitInfosArray[arrIdx].InfoStruct.cTransportCap := Byte(value);

      UNITINFO_BANKSCALE       : CustomUnitInfosArray[arrIdx].InfoStruct.lBankScale := value;
      UNITINFO_KAMIKAZEDIST    : CustomUnitInfosArray[arrIdx].InfoStruct.nKamikazeDistance := Word(value);
      UNITINFO_DAMAGEMODIFIER  : CustomUnitInfosArray[arrIdx].InfoStruct.lDamageModifier := value;

      UNITINFO_WORKERTIME      : CustomUnitInfosArray[arrIdx].InfoStruct.nWorkerTime := Word(value);
      UNITINFO_BUILDDIST       : CustomUnitInfosArray[arrIdx].InfoStruct.nBuildDistance := Word(value);

      UNITINFO_SIGHTDIST :
        begin
        CustomUnitInfosArray[arrIdx].InfoStruct.nSightDistance := Word(value);
        TAUnit.UpdateLos(TAUnit.Id2Ptr(Word(unitId)));
        end;
      UNITINFO_RADARDIST :
        begin
        CustomUnitInfosArray[arrIdx].InfoStruct.nRadarDistance := Word(value);
        TAUnit.UpdateLos(TAUnit.Id2Ptr(Word(unitId)));
        end;
      UNITINFO_SONARDIST       : CustomUnitInfosArray[arrIdx].InfoStruct.nSonarDistance := Word(value);
      UNITINFO_MINCLOAKDIST    : CustomUnitInfosArray[arrIdx].InfoStruct.nMinCloakDistance := Word(value);
      UNITINFO_RADARDISTJAM    : CustomUnitInfosArray[arrIdx].InfoStruct.nRadarDistanceJam := Word(value);
      UNITINFO_SONARDISTJAM    : CustomUnitInfosArray[arrIdx].InfoStruct.nSonarDistanceJam := Word(value);

      UNITINFO_MAKESMETAL      : CustomUnitInfosArray[arrIdx].InfoStruct.cMakesMetal := Byte(value);
      UNITINFO_FENERGYMAKE     : CustomUnitInfosArray[arrIdx].InfoStruct.fEnergyMake := value / 100;
      UNITINFO_FMETALMAKE      : CustomUnitInfosArray[arrIdx].InfoStruct.fMetalMake := value / 100;
      UNITINFO_FENERGYUSE      : CustomUnitInfosArray[arrIdx].InfoStruct.fEnergyUse := value / 100;
      UNITINFO_FMETALUSE       : CustomUnitInfosArray[arrIdx].InfoStruct.fMetalUse := value / 100;
      UNITINFO_FENERGYSTOR     : CustomUnitInfosArray[arrIdx].InfoStruct.lEnergyStorage := value;
      UNITINFO_FMETALSTOR      : CustomUnitInfosArray[arrIdx].InfoStruct.lEnergyStorage := value;
      UNITINFO_FWINDGENERATOR  : CustomUnitInfosArray[arrIdx].InfoStruct.fWindGenerator := value / 100;
      UNITINFO_FTIDALGENERATOR : CustomUnitInfosArray[arrIdx].InfoStruct.fTidalGenerator := value / 100;
      UNITINFO_FCLOAKCOST      : CustomUnitInfosArray[arrIdx].InfoStruct.fCloakCost := value / 100;
      UNITINFO_FCLOAKCOSTMOVE  : CustomUnitInfosArray[arrIdx].InfoStruct.fCloakCostMoving := value / 100;

      UNITINFO_BMCODE          : CustomUnitInfosArray[arrIdx].InfoStruct.cBMCode := value;

      UNITINFO_EXPLODEAS       : CustomUnitInfosArray[arrIdx].InfoStruct.p_ExplodeAs := PLongWord(TAMem.WeaponId2Ptr(value));
      UNITINFO_SELFDSTRAS      : CustomUnitInfosArray[arrIdx].InfoStruct.p_SelfDestructAsAs := PLongWord(TAMem.WeaponId2Ptr(value));

// 1 ?
// 2 standingfireorder
// 3 ?
// 4 init_cloaked
// 5 downloadable
      UNITINFO_BUILDER         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 6);
// 7 zbuffer
      UNITINFO_STEALTH         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 8);
      UNITINFO_ISAIRBASE       : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 9);
      UNITINFO_TARGETTINGUPGRADE   : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 10);
      UNITINFO_CANFLY          : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 11);
      UNITINFO_CANHOVER        : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 12);
      UNITINFO_TELEPORTER      : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 13);
      UNITINFO_HIDEDAMAGE      : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 14);
      UNITINFO_SHOOTME         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 15);
      UNITINFO_HASWEAPONS      : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 16);
// 17 armoredstate
// 18 activatewhenbuilt
      UNITINFO_FLOATER         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 19);
      UNITINFO_UPRIGHT         : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 20);
      UNITINFO_AMPHIBIOUS      : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 21);
// 22 ?
// 23 internal command reload -> sub_42D1F0. probably reloads cob script 
// 24 isfeature
// 25 noshadow
      UNITINFO_IMMUNETOPARALYZER  : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 26);
      UNITINFO_HOVERATTACK     : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 27);
      UNITINFO_KAMIKAZE        : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 28);
      UNITINFO_ANTIWEAPONS     : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 29);
      UNITINFO_DIGGER          : SetUnitTypeMask(arrIdx, 0, (value = 1), 1 shl 30);
// 31 has GUI (does gui file in guis folder exists with name of tested unit) ? sub_42d2e0

      UNITINFO_ONOFFABLE       : SetUnitTypeMask(arrIdx, 1, (value = 1), 4);
      UNITINFO_CANSTOP         : SetUnitTypeMask(arrIdx, 1, (value = 1), 8);
      UNITINFO_CANATTACK       : SetUnitTypeMask(arrIdx, 1, (value = 1), 16);
      UNITINFO_CANGUARD        : SetUnitTypeMask(arrIdx, 1, (value = 1), 32);
      UNITINFO_CANPATROL       : SetUnitTypeMask(arrIdx, 1, (value = 1), 64);
      UNITINFO_CANMOVE         : SetUnitTypeMask(arrIdx, 1, (value = 1), 128);
      UNITINFO_CANLOAD         : SetUnitTypeMask(arrIdx, 1, (value = 1), 256);
      UNITINFO_CANRECLAMATE    : SetUnitTypeMask(arrIdx, 1, (value = 1), 1024);
      UNITINFO_CANRESURRECT    : SetUnitTypeMask(arrIdx, 1, (value = 1), 2048);
      UNITINFO_CANCAPTURE      : begin SetUnitTypeMask(arrIdx, 1, (value = 1), $1000); SetUnitTypeMask(arrIdx, 1, (value = 1), $2000); end;
      UNITINFO_CANDGUN         : SetUnitTypeMask(arrIdx, 1, (value = 1), $4000);
      UNITINFO_SHOWPLAYERNAME  : SetUnitTypeMask(arrIdx, 1, (value = 1), $20000);
      UNITINFO_COMMANDER       : SetUnitTypeMask(arrIdx, 1, (value = 1), $40000);
      UNITINFO_CANTBERANSPORTED: SetUnitTypeMask(arrIdx, 1, (value = 1), $80000);
    end;
    Result:= True;
  end;
end;

class Function TAUnits.CreateMinions(UnitPtr: Pointer; Amount: Byte; UnitInfo: Pointer; Action: TTAActionType; ArrayId: Cardinal): Integer;
const
  MAXRETRIES = 25 - 1;
var
  UnitSt: PUnitStruct;
  CallerUnitInfoSt, UnitInfoSt: PUnitInfo;
  PlayerIndex: Byte;
  UnitsArray: TFoundUnits;
  CallerPosition, TestPosition: TPosition;
  CallerTurn: TTurn;
  DestIsAir: Boolean;
  SpotTestFree: Boolean;
  NewPos: array [0..1] of LongInt;
  GridPos: array [0..1] of Word;
  ToBeSpawned: array of TPosition;
  UnitState : LongWord;
  ResultUnit: Pointer;
  //Retries: Integer;
  i, j: Integer;
  r, angle, jiggle: Integer;
  ModelDiagonal: array [0..1] of Cardinal;
  SpawnRange: Cardinal;
begin
  Result := 0;
  if UnitPtr <> nil then
  begin
    UnitSt := UnitPtr;

    PlayerIndex := TAUnit.GetOwnerIndex(pointer(UnitPtr));

    UnitInfoSt := UnitInfo;
    CallerUnitInfoSt := Pointer(TAMem.UnitInfoId2Ptr(UnitSt.nUnitInfoID));

    CallerPosition := UnitSt.Position;
    CallerTurn := UnitSt.Turn;

    ModelDiagonal[0] := Round(Hypot(CallerUnitInfoSt.nFootPrintX, CallerUnitInfoSt.nFootPrintZ) * 14);
    ModelDiagonal[1] := Round(Hypot(UnitInfoSt.nFootPrintX, UnitInfoSt.nFootPrintZ) * 14);
    SpawnRange := Round((ModelDiagonal[0] + ModelDiagonal[1]) *1.4);

    //Retries:= 0;
    Randomize;
    ResultUnit := nil;

    for i := 1 to Amount do
    begin
      r := SpawnRange;
      for j := MAXRETRIES downto 0 do
      begin
        jiggle := Round(Random(High(Byte))/2);
        angle := Random(360)+1;
        if CircleCoords(CallerPosition, r + jiggle, angle, NewPos[0], NewPos[1]) then
        begin
          if GetTPosition(NewPos[0], NewPos[1], TestPosition) <> nil then
          begin
            DestIsAir := (UnitInfoSt.UnitTypeMask and 2048 = 2048);
            SpotTestFree := TAUnit.TestBuildSpot(PlayerIndex, UnitInfoSt, TestPosition.X, TestPosition.Z);
            if SpotTestFree or DestIsAir then
            begin
              TAUnit.Position2Grid(TestPosition, UnitInfoSt, GridPos[0], GridPos[1]);
              SetLength(ToBeSpawned, High(ToBeSpawned) + 2);
              ToBeSpawned[High(ToBeSpawned)] := TestPosition;
              ToBeSpawned[High(ToBeSpawned)].X := Round(TestPosition.X + (UnitInfoSt.nFootPrintX / 2) * 16);
              ToBeSpawned[High(ToBeSpawned)].Z := Round(TestPosition.Z + (UnitInfoSt.nFootPrintZ / 2) * 16);
              if DestIsAir then
              begin
                if (UnitInfoSt.nCruiseAlt > 0) then
                  ToBeSpawned[High(ToBeSpawned)].Y := UnitInfoSt.nCruiseAlt;
                UnitState := 6;
              end else
              begin
                if GetPosHeight(@ToBeSpawned[High(ToBeSpawned)]) <> - 1 then
                  ToBeSpawned[High(ToBeSpawned)].Y := GetPosHeight(@ToBeSpawned[High(ToBeSpawned)]);
                UnitState := 1;
              end;

              ResultUnit := TAUnit.CreateUnit(PlayerIndex, UnitInfo, ToBeSpawned[High(ToBeSpawned)], nil, False, False, UnitState);
              if ResultUnit <> nil then
              begin
                if TAData.NetworkLayerEnabled then
                  Send_UnitBuildFinished(UnitPtr, ResultUnit);
                SetLength(UnitsArray, High(UnitsArray) + 2);
                UnitsArray[High(UnitsArray)] := PUnitStruct(ResultUnit).lUnitInGameIndex;
                if Action <> Action_Ready then
                  TAUnit.CreateMainOrder(ResultUnit, UnitPtr, Action, nil, 1, 0, 0);
                Break;
              end;
            end;
          end;
        end;
        //Inc(Retries);
      end;
    end;
    Result:= High(ToBeSpawned) + 1;
{    SendTextLocal('Caller model diag: ' + IntToStr(ModelDiagonal[0]));
    SendTextLocal('Spawn model diag: ' + IntToStr(ModelDiagonal[1]));
    SendTextLocal('Min range: ' + IntToStr(SpawnRange));
    SendTextLocal('Will spawn: ' + IntToStr(High(ToBeSpawned)+1));
    SendTextLocal('Retries: ' + IntToStr(Retries));  }

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

class procedure TAUnits.GiveUnit(ExistingUnitPtr: Pointer; PlayerIdx: Byte);
var
  PlayerStruct : PPlayerStruct;
begin
  if ExistingUnitPtr <> nil then
  begin
    PlayerStruct := TAPlayer.GetPlayerByIndex(PlayerIdx);
    UNITS_GiveUnit(ExistingUnitPtr, PlayerStruct, nil);
  end;
end;

class function TAUnits.CreateSearchFilter(Mask: Integer): TUnitSearchFilterSet;
var
  a, b: word;
begin
  if Mask <> 0 then
    Result:= []
  else
  begin
    Result:= [usfNone];
    Exit;
  end;

  for a:= Ord(usfNone) to Ord(usfIncludeInBuildState) do
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
  MaxDistance: Integer; Filter: TUnitSearchFilterSet; UnitTypeFilter: Pointer ): LongWord;
var
  FoundCount: Integer;
  MaxUnitId: LongWord;
  UnitSt, CheckedUnitSt: PUnitStruct;
  UnitInfoSt, CheckedUnitInfoSt: PUnitInfo;
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
  MaxUnitId:= TAData.MaxUnitsID;

  for UnitId := 1 to MaxUnitId do
  begin
    CheckedUnitSt:= TAUnit.Id2Ptr(UnitId);
    if (CheckedUnitSt = nil) or
       (CheckedUnitSt = UnitPtr) then
      Continue;
    if CheckedUnitSt.nUnitInfoID = 0 then
      Continue;
    if (UnitTypeFilter <> nil) then
      if (TAUnit.GetUnitInfoPtr(CheckedUnitSt) <> UnitTypeFilter) then
        Continue;
    if usfAllied in Filter then
    begin
      if (TAUnit.IsAllied(UnitPtr, UnitId) <> 1) then
        Continue;
    end else
    begin
      if usfOwner in Filter then
      begin
        if UnitSt.p_Owner <> CheckedUnitSt.p_Owner then
          Continue;
      end else
      begin
        if usfEnemy in Filter then
        begin
          if (not (TAUnit.IsAllied(UnitPtr, UnitId) <> 1)) and TAUnit.IsOnThisComp(CheckedUnitSt, (usfAI in Filter)) then
            Continue;
        end;
      end;

      if not (usfAI in Filter) then
      begin
        if TAPlayer.PlayerController(CheckedUnitSt.p_Owner) = Player_LocalAI then
          Continue;
      end;
    end;

    CheckedUnitInfoSt:= Pointer(CheckedUnitSt.p_UnitDef);
    if usfExcludeAir in Filter then
    begin
      if TAUnit.GetUnitInfoField(CheckedUnitSt, UNITINFO_CANFLY, nil) = 1 then
        Continue;
    end;
    if usfExcludeSea in Filter then
    begin
      if TAUnit.GetUnitInfoField(CheckedUnitSt, UNITINFO_FLOATER, nil) = 1 then
        Continue
      else
        //subs or pels/hovers currently in sea
        if not (GetPosHeight(@CheckedUnitSt.Position) > PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.SeaLevel) then
        begin
          Continue;
        end;
    end;
    if usfExcludeBuildings in Filter then
    begin
      if TAUnit.GetUnitInfoField(CheckedUnitSt, UNITINFO_BMCODE, nil) = 0 then
        Continue;
    end;
    if usfExcludeNonWeaponed in Filter then
    begin
      if TAUnit.GetUnitInfoField(CheckedUnitSt, UNITINFO_HASWEAPONS, nil) = 0 then
        Continue;
    end;

    if not (usfIncludeInBuildState in Filter) and
       (CheckedUnitSt.lBuildTimeLeft <> 0) then
      Continue;

    if UnitId <> Word(UnitSt.lUnitInGameIndex) then
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
                Distance:= TAUnits.Distance(@UnitSt.Position, @CheckedUnitSt.Position);
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
        NearestUnitDistance := TAUnits.Distance(@UnitSt.Position, @PUnitStruct(TAUnit.Id2Ptr(FoundArray[i])).Position);
        if i = Low(FoundArray) then
        begin
          if NearestUnitDistance <> -1 then
            LastNearestUnitDistance := NearestUnitDistance;
          Result:= FoundArray[i];
        end else
          if (NearestUnitDistance < LastNearestUnitDistance) and (NearestUnitDistance <> -1) then
          begin
            LastNearestUnitDistance := NearestUnitDistance;
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

class function TAUnits.Distance(Pos1, Pos2 : PPosition): Cardinal;
var
  Distance : Extended;
begin
  Result := 0;
  if (Pos1 <> nil) and (Pos2 <> nil) then
  begin
    if ((PPosition(Pos1)^.X - PPosition(Pos2)^.X) <> 0) and
       ((PPosition(Pos1)^.Z - PPosition(Pos2)^.Z) <> 0) then
    begin
      Distance := Hypot(PPosition(Pos1)^.X - PPosition(Pos2)^.X,
                        PPosition(Pos1)^.Z - PPosition(Pos2)^.Z);
      Result := Round(Distance);
    end else
      Result := 0;
  end;
end;

class function TAUnits.CircleCoords(CenterPosition: TPosition; Radius: Integer; Angle: Integer; out x, z: Integer): Boolean;
var
  MapWidth : Integer;
  MapHeight : Integer;
begin
  x := Round(CenterPosition.X + cos(Angle) * Radius);
  z := Round(CenterPosition.Z + sin(Angle) * Radius);
  MapWidth := PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.lMapWidth;
  MapHeight := PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.lMapHeight;
  if (x > 0) and (z > 0) and (x < MapWidth) and (z < MapHeight) then
    Result:= True
  else
    Result:= False;
end;

class procedure TASfx.Speech(UnitPtr: Pointer; SpeechType: Cardinal; Text: PAnsiChar);
begin
  PlaySound_UnitSpeech(UnitPtr, SpeechType, Text);
end;

class function TASfx.Play3DSound(EffectID: Cardinal; Position: TPosition; NetBroadcast: Boolean): Integer;
begin
  Result := PlaySound_3D_ID(EffectID, @Position, BoolValues[NetBroadcast]);
end;

class function TASfx.PlayGafAnim(BmpType: Byte; X, Z: Word; Glow, Smoke: Byte): Integer;
var
  Position : TPosition;
  GlowInt, SmokeInt : Integer;
begin
  result := 0;
  GetTPosition(X, Z, Position);

  if Glow = 0 then
    GlowInt := -1
  else
    GlowInt := Glow - 1;

  if Smoke = 0 then
    SmokeInt := -1
  else
    SmokeInt := Smoke - 1;

  case BmpType of
    0 : result := ShowExplodeGaf(@Position, PLongWord(LongWord(TAData.MainStructPtr) + $147F7)^, GlowInt, SmokeInt); //explosion
    1 : result := ShowExplodeGaf(@Position, PLongWord(LongWord(TAData.MainStructPtr) + $147FB)^, GlowInt, SmokeInt); //explode2
    2 : result := ShowExplodeGaf(@Position, PLongWord(LongWord(TAData.MainStructPtr) + $147FF)^, GlowInt, SmokeInt); //explode3
    3 : result := ShowExplodeGaf(@Position, PLongWord(LongWord(TAData.MainStructPtr) + $14803)^, GlowInt, SmokeInt); //explode4
    4 : result := ShowExplodeGaf(@Position, PLongWord(LongWord(TAData.MainStructPtr) + $14807)^, GlowInt, SmokeInt); //explode5
    5 : result := ShowExplodeGaf(@Position, PLongWord(LongWord(TAData.MainStructPtr) + $1480B)^, GlowInt, SmokeInt); //nuke1
    6..20 : result := ShowExplodeGaf(@Position, LongWord(ExtraAnimations[BmpType - 6]), GlowInt, SmokeInt); // explode6,7,8... custanim1,2,3...
  end;
end;

procedure UnitPosition2NanoTarget(UnitPtr: PUnitStruct; var NanolatPos: TNanolathePos);
asm
    mov     eax, UnitPtr
    mov     ecx, [eax+TUnitStruct.p_UnitDef]
    mov     edx, [eax+TUnitStruct.Position.X_]
    mov     ebx, [eax+TUnitStruct.Position.Y_]
    add     ecx, 15Eh
    mov     esi, [ecx]
    mov     edi, [ecx+4]
    mov     ecx, [ecx+8]
    add     edx, esi
    mov     esi, [eax+TUnitStruct.Position.Z_]
    add     edi, ebx
    add     ecx, esi
    mov     NanolatPos.Pos1.X, edx
    mov     NanolatPos.Pos1.Y, edi
    mov     NanolatPos.Pos1.Z, ecx
    mov     ecx, [eax+TUnitStruct.p_UnitDef]
    mov     edx, [eax+TUnitStruct.Position.X_]
    mov     ebx, [eax+TUnitStruct.Position.Z_]
    add     ecx, 16Ah
    mov     esi, [eax+TUnitStruct.Position.Z_]
    mov     edi, [ecx]
    add     edx, edi
    mov     edi, [ecx+4]
    mov     ecx, [ecx+8]
    mov     NanolatPos.Pos2.X, edx
    add     ecx, ebx
    mov     NanolatPos.Pos2.Z, ecx
    add     edi, esi
    mov     NanolatPos.Pos2.Y, edi
end;

class function TASfx.EmitSfxFromPiece(UnitPtr: PUnitStruct; TargetUnitPtr: PUnitStruct;
  PieceIdx: Integer; SfxType: Byte): Cardinal;
var
  PiecePos : TPosition;
  TargetBase : TPosition;
  TargetNanoBase : TNanolathePos;
  UnitPos : TPositionCard;
  BuildingUnitInfo : PUnitInfo;
begin
  Result := 0;
  BuildingUnitInfo := TargetUnitPtr.p_UnitDef;
  GetPiecePosition(PiecePos, UnitPtr, PieceIdx);
  GetPiecePosition(TargetBase, TargetUnitPtr, 0);

  UnitPos.X := MakeLong(0, TargetUnitPtr.Position.X);
  UnitPos.Y := MakeLong(0, TargetUnitPtr.Position.Y);
  UnitPos.Z := MakeLong(0, TargetUnitPtr.Position.Z);

  TargetNanoBase.Pos1.X := UnitPos.X + BuildingUnitInfo.lWidthX_;
  TargetNanoBase.Pos1.Y := UnitPos.Y + BuildingUnitInfo.lWidthY_;
  TargetNanoBase.Pos1.Z := UnitPos.Z + BuildingUnitInfo.lWidthZ_;

  TargetNanoBase.Pos2.X := UnitPos.X + BuildingUnitInfo.lFootPrintX_;
  TargetNanoBase.Pos2.Y := UnitPos.Y + BuildingUnitInfo.lFootPrintY_;
  TargetNanoBase.Pos2.Z := UnitPos.Z + BuildingUnitInfo.lFootPrintZ_;

  case SfxType of
    6 : Result := EmitSfx_NanoParticles(@PiecePos, @TargetNanoBase, 6);
    7 : Result := EmitSfx_NanoParticlesReverse(@TargetNanoBase, @PiecePos, 6);
    8 : Result := EmitSfx_Teleport(@PiecePos, @TargetBase, 30, 5);
  end;
end;

class function TASfx.NanoParticles(StartPos: TPosition; TargetPos : TNanolathePos): Cardinal;
begin
  Result := EmitSfx_NanoParticles(@StartPos, @TargetPos, 6);
end;

class function TASfx.NanoReverseParticles(StartPos: TPosition;
  TargetPos: TNanolathePos): Cardinal;
begin
  Result := EmitSfx_NanoParticlesReverse(@TargetPos, @StartPos, 6);
end;

end.
