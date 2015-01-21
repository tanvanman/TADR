unit TA_MemoryLocations;

interface
uses
  dplay, Classes, TA_MemoryStructures;

type
  TAMem = class
  protected
    class function GetShareEnergyVal : single;
    class function GetShareMetalVal : single;
    class function GetShareEnergy : Boolean;
    class function GetShareMetal : Boolean;
    class function GetShootAll : Boolean;
    class function GetSwitchesMask : Word;
    class procedure SetSwitchesMask(Mask: Word);
    class procedure SetShootAll(ANewState: Boolean);

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
    class function GetMainStructPtr : PTADynMemStruct;
    class function GetProgramStructPtr : Pointer;
    class function GetPlayersStructPtr : Pointer;
    class function GetModelsArrayPtr : Pointer;
    class function GetWeaponTypeDefArrayPtr : Pointer;
    class function GetFeatureTypeDefArrayPtr : Pointer;
    class function GetUnitInfosPtr : Pointer;
    class function GetUnitInfosCount : LongWord;

    class function GetGameingType : TGameingType;
    class function GetGUICallbackState : TGUICallbackState;
    class function GetAIDifficulty : TAIDifficulty;
    class procedure SetCameraFadeLevel(FadePercent : Integer);
    class function GetViewPlayerRaceSide: TTAPlayerSide;

    class Function GetPausedState : Boolean;
    class Procedure SetPausedState( value : Boolean);
  public
    property ShareEnergyVal : single read GetShareEnergyVal;
    property ShareMetalVal : single read GetShareMetalVal;
    property ShareEnergy : Boolean read GetShareEnergy;
    property ShareMetal : Boolean read GetShareMetal;
    property ShootAll : Boolean read GetShootAll write SetShootAll;
    Property SwitchesMask : Word read GetSwitchesMask write SetSwitchesMask;
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
    Property UnitsArray_p : Pointer read GetUnitsPtr;
    Property EndOfUnitsArray_p : Pointer read GetUnits_EndMarkerPtr;
    Property MaxUnitsID : Cardinal read GetMaxUnitId;
    Property MainStruct : PTADynMemStruct read GetMainStructPtr;
    Property ProgramStructPtr : Pointer read GetProgramStructPtr;
    Property PlayersStructPtr : Pointer read GetPlayersStructPtr;
    Property ModelsArrayPtr : Pointer read GetModelsArrayPtr;
    Property WeaponTypeDefArrayPtr : Pointer read GetWeaponTypeDefArrayPtr;
    Property FeatureTypeDefArrayPtr : Pointer read GetFeatureTypeDefArrayPtr;
    Property UnitInfosPtr : Pointer read GetUnitInfosPtr;
    Property UnitInfosCount : LongWord read GetUnitInfosCount;

    Property GameingType : TGameingType read GetGameingType;
    Property GUICallbackState : TGUICallbackState read GetGUICallbackState;
    Property AIDifficulty : TAIDifficulty read GetAIDifficulty;
    Property RaceSide : TTAPlayerSide read GetViewPlayerRaceSide;
    class procedure ShakeCam(X, Y, Duration : Cardinal);

    class function ScriptActionName2Index(ActionName: PAnsiChar) : Integer;
    class function GetModelPtr(index: Word) : Pointer;
    class function UnitInfoId2Ptr(index: Word) : PUnitInfo;
    class function UnitInfoCrc2Ptr(CRC: Cardinal) : PUnitInfo;
    class function Crc32ToCrc24(CRC: Cardinal) : Cardinal;
    class function MovementClassId2Ptr(index: Word) : Pointer;
    class function FeatureDefId2Ptr(ID : Word) : Pointer;
    class procedure Position2Grid(Position: TPosition; out GridPosX, GridPosZ: Word);
    class function PositionInMapArea(Position: TPosition): Boolean;
    class function ProtectMemoryRegion(Address : Cardinal; Writable: Boolean) : Integer;
    class function IsTAVersion31 : Boolean;
  end;

  TASfx = class
  public
    class procedure Speech(p_Unit: Pointer; SpeechType: Cardinal; Text: PAnsiChar);
    class function Play3DSound(EffectID: Cardinal; Position: TPosition; NetBroadcast: Boolean) : Integer;
    class function PlayGafAnim(BmpType: Byte; X, Z: Word; Glow, Smoke: Byte) : Integer;
    class function EmitSfxFromPiece(p_Unit: PUnitStruct; Targetp_Unit: PUnitStruct;
      PieceIdx: Integer; SfxType: Byte; Broadcast: Boolean) : Cardinal;
    class function NanoParticles(StartPos: TPosition; TargetPos : TNanolathePos) : Cardinal;
    class function NanoReverseParticles(StartPos: TPosition; TargetPos : TNanolathePos) : Cardinal;
  end;

  TAWeapon = class
  public
    class function WeaponId2Ptr(ID: Cardinal) : Pointer;
    class function GetWeaponID(WeaponPtr: PWeaponDef) : Cardinal;
    class function FireMap_Weapon(WeaponPtr: PWeaponDef;
      TargetX, TargetZ: Cardinal) : Boolean;
  end;

const
  BoolValues : array [Boolean] of Byte = (0,1);

var
  TAData : TAMem;

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
  GAFSequences,
  UnitInfoExpand,
  COB_Extensions,
  TA_MemoryConstants,
  idplay,
  TA_NetworkingMessages,
  TA_MemPlayers,
  TA_MemUnits;

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

Class function TAMem.GetShareEnergyVal : Single;
begin
  Result := PPlayerStruct(TAData.PlayersStructPtr).fShareLimitEnergy;
end;

Class function TAMem.GetShareMetalVal : Single;
begin
  Result := PPlayerStruct(TAData.PlayersStructPtr).fShareLimitMetal;
end;

Class function TAMem.GetShareEnergy : Boolean;
begin
  Result := SharedState_SharedEnergy in PPlayerStruct(TAData.PlayersStructPtr).PlayerInfo.SharedBits;
end;

Class function TAMem.GetShareMetal : Boolean;
begin
  Result := SharedState_SharedMetal in PPlayerStruct(TAData.PlayersStructPtr).PlayerInfo.SharedBits;
end;

Class function TAMem.GetShootAll : Boolean;
begin
  Result := ((TAData.SwitchesMask and SwitchesMasks[SwitchesMask_ShootAll]) =
    SwitchesMasks[SwitchesMask_ShootAll]);
end;

Class procedure TAMem.SetShootAll(ANewState: Boolean);
begin
  if ANewState then
    TAData.SwitchesMask := TAData.SwitchesMask or SwitchesMasks[SwitchesMask_ShootAll]
  else
    TAData.SwitchesMask := TAData.SwitchesMask and not SwitchesMasks[SwitchesMask_ShootAll];
end;

class Function TAMem.GetLocalPlayerId : Byte;
begin
result := TAData.MainStruct.cControlPlayerID;
end;

class Function TAMem.GetActivePlayersCount : Byte;
begin
result := Byte(TAData.MainStruct.nActivePlayersCount);
end;

class Function TAMem.GetGameTime : Integer;
begin
result := TAData.MainStruct.lGameTime;
end;

class Function TAMem.GetGameSpeed : Byte;
begin
result := TAData.MainStruct.nTAGameSpeed;
end;

class Function TAMem.GetDevMode : Boolean;
begin
result := (TAData.MainStruct.nGameState and 2) = 2;
end;

class function TAMem.GetIsNetworkLayerEnabled: Boolean;
begin
result := (GlobalDPlay <> nil) and ((TAData.MainStruct.cNetworkLayerEnabled and 1) = 1);
end;

class function TAMem.GetMaxUnitLimit : Word;
begin
result := TAData.MainStruct.nMaxUnitLimitPerPlayer;
end;

class function TAMem.GetActualUnitLimit : Word;
begin
result := TAData.MainStruct.nPerMissionUnitLimit;
end;

class function TAMem.GetIsAlteredUnitLimit : Boolean;
begin
result := TAData.MainStruct.cAlteredUnitLimit <> 0;
end;

class function TAMem.GetUnitsPtr : Pointer;
begin
result := TAData.MainStruct.p_Units;
end;

class function TAMem.GetUnits_EndMarkerPtr : Pointer;
begin
result := TAData.MainStruct.p_LastUnitInArray;
end;

class function TAMem.GetMainStructPtr : PTADynMemStruct;
begin
result := Pointer(PCardinal(TADynmemStructPtr)^);
end;

class function TAMem.GetProgramStructPtr : Pointer;
begin

result := TAData.MainStruct.p_TAProgram;
end;

class function TAMem.GetPlayersStructPtr: Pointer;
begin
result := @TAData.MainStruct.Players[0];
end;

class function TAMem.GetModelsArrayPtr : Pointer;
begin
result := TAData.MainStruct.p_MODEL_PTRS;
end;

class function TAMem.GetWeaponTypeDefArrayPtr : Pointer;
begin
  if IniSettings.WeaponType <= 256 then
    result := @TAData.MainStruct.Weapons[0]
  else
    result := @PTAdynmemStruct(WeaponLimitPatchArr)^.Weapons[0];
end;

class function TAMem.GetFeatureTypeDefArrayPtr : Pointer;
begin
result := TAData.MainStruct.TNTMemStruct.p_FeatureDefs;
end;

class function TAMem.GetUnitInfosPtr : Pointer;
begin
result := TAData.MainStruct.p_UNITINFOs;
end;

class function TAMem.GetUnitInfosCount : LongWord;
begin
result := TAData.MainStruct.lNumUnitTypeDefs;
end;

class function TAMem.GetSwitchesMask : Word;
begin
Result:= TAData.MainStruct.nSwitchesMask;
end;

class procedure TAMem.SetSwitchesMask(Mask: Word);
begin
TAData.MainStruct.nSwitchesMask:= Mask;
end;

class Function TAMem.GetPausedState : Boolean;
begin
result := TAData.MainStruct.cIsGamePaused <> 0;
end;

class Procedure TAMem.SetPausedState( value : Boolean);
begin
TAData.MainStruct.cIsGamePaused := BoolValues[value];
end;

class function TAMem.GetModelPtr(index: Word) : Pointer;
begin
result := PLongWord(LongWord(GetModelsArrayPtr) + index * 4);
end;

class function TAMem.UnitInfoId2Ptr(index: Word) : PUnitInfo;
begin
result := Pointer(LongWord(TAData.UnitInfosPtr) + index * SizeOf(TUnitInfo));
end;

class function TAMem.UnitInfoCrc2Ptr(CRC: Cardinal) : PUnitInfo;
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

class function TAMem.Crc32ToCrc24(CRC: Cardinal) : Cardinal;
begin
  Result := CRC and not $FF000000;
end;

class function TAMem.MovementClassId2Ptr(index: Word) : Pointer;
begin
result := Pointer(TAMovementClassArray + SizeOf(TMoveInfoClassStruct) * index);
end;

class function TAMem.FeatureDefId2Ptr(ID: Word) : Pointer;
begin
  result := Pointer(LongWord(TAData.MainStruct.TNTMemStruct.p_FeatureDefs) + SizeOf(TFeatureDefStruct) * ID);
end;

class function TAMem.GetMaxUnitId: LongWord;
begin
  if TAData.GameingType = gtMenu then
    result := TAData.MaxUnitLimit * MAXPLAYERCOUNT
  else
    result := TAData.ActualUnitLimit * MAXPLAYERCOUNT;
end;

class function TAMem.ScriptActionName2Index(ActionName: PAnsiChar) : Integer;
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
  if TAData.MainStruct.p_MapOTAFile <> nil then
    Result := TGameingType(PMapOTAFile(TAData.MainStruct.p_MapOTAFile).MissionType);
end;

class function TAMem.GetGUICallbackState: TGUICallbackState;
begin
  Result := TGUICallbackState(TAData.MainStruct.lGUICallbackState);
end;

class function TAMem.GetAIDifficulty: TAIDifficulty;
begin
  Result := TAIDifficulty(TAData.MainStruct.lCurrenTAIProfile);
end;

class function TAMem.GetViewPlayerRaceSide: TTAPlayerSide;
begin
  Result := TTAPlayerSide(PPlayerInfoStruct(PPlayerStruct(TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID)).PlayerInfo).Raceside);
end;

class procedure TAMem.SetCameraFadeLevel(FadePercent : Integer);
begin

end;

class procedure TAMem.ShakeCam(X, Y, Duration: Cardinal);
begin
  TAData.MainStruct.field_1432F :=
    (TAData.MainStruct.field_1432F + Duration) div 2;
  TAData.MainStruct.field_14333 := Duration;
  if ( X <> 0 ) then
    TAData.MainStruct.ShakeMagnitude_1 :=
      TAData.MainStruct.ShakeMagnitude_1 + X;
  if ( Y <> 0 ) then
    TAData.MainStruct.ShakeMagnitude_2 :=
      TAData.MainStruct.ShakeMagnitude_2 + Y;
  if ( TAData.MainStruct.field_1432F > 0 ) then
    TAData.MainStruct.cShake := TAData.MainStruct.cShake or 1;
end;

class function TAMem.ProtectMemoryRegion(Address : Cardinal; Writable: Boolean) : Integer;
begin
  if Writable then
    Result := MEM_AllowReadWrite(Address)
  else
    Result := MEM_SetReadOnly(Address);
end;

{ TASfx }

class procedure TASfx.Speech(p_Unit: Pointer; SpeechType: Cardinal; Text: PAnsiChar);
begin
  PlaySound_UnitSpeech(p_Unit, SpeechType, Text);
end;

class function TASfx.Play3DSound(EffectID: Cardinal; Position: TPosition; NetBroadcast: Boolean) : Integer;
begin
  Result := PlaySound_3D_ID(EffectID, @Position, BoolValues[NetBroadcast]);
end;

class function TASfx.PlayGafAnim(BmpType: Byte; X, Z: Word; Glow, Smoke: Byte) : Integer;
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
        0 : result := ShowExplodeGaf(@Position, PLongWord(LongWord(TAData.MainStruct) + $147F7)^, GlowInt, SmokeInt); //explosion
        1 : result := ShowExplodeGaf(@Position, PLongWord(LongWord(TAData.MainStruct) + $147FB)^, GlowInt, SmokeInt); //explode2
        2 : result := ShowExplodeGaf(@Position, PLongWord(LongWord(TAData.MainStruct) + $147FF)^, GlowInt, SmokeInt); //explode3
        3 : result := ShowExplodeGaf(@Position, PLongWord(LongWord(TAData.MainStruct) + $14803)^, GlowInt, SmokeInt); //explode4
        4 : result := ShowExplodeGaf(@Position, PLongWord(LongWord(TAData.MainStruct) + $14807)^, GlowInt, SmokeInt); //explode5
        5 : result := ShowExplodeGaf(@Position, PLongWord(LongWord(TAData.MainStruct) + $1480B)^, GlowInt, SmokeInt); //nuke1
    6..99 : result := ShowExplodeGaf(@Position, LongWord(ExtraGAFAnimations.Explode[BmpType - 6]), GlowInt, SmokeInt);
 100..199 : result := ShowExplodeGaf(@Position, LongWord(ExtraGAFAnimations.CustAnim[BmpType - 100]), GlowInt, SmokeInt);
  end;
end;

class function TASfx.EmitSfxFromPiece(p_Unit: PUnitStruct; Targetp_Unit: PUnitStruct;
  PieceIdx: Integer; SfxType: Byte; Broadcast: Boolean) : Cardinal;
var
  PiecePos : TPosition;
  TargetBase : TPosition;
  TargetNanoBase : TNanolathePos;
  UnitPos : TPositionLong;
  BuildingUnitInfo : PUnitInfo;
begin
  Result := 0;
  BuildingUnitInfo := Targetp_Unit.p_UNITINFO;
  GetPiecePosition(PiecePos, p_Unit, PieceIdx);
  GetPiecePosition(TargetBase, Targetp_Unit, 0);

  UnitPos.X := MakeLong(0, Targetp_Unit.Position.X);
  UnitPos.Y := MakeLong(0, Targetp_Unit.Position.Y);
  UnitPos.Z := MakeLong(0, Targetp_Unit.Position.Z);

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

  if IniSettings.BroadcastNanolathe then
  begin
    if Broadcast and TAData.NetworkLayerEnabled then
      GlobalDPlay.Broadcast_EmitSFXToUnit(TAUnit.GetID(p_Unit), TAUnit.GetID(Targetp_Unit), PieceIdx, SfxType);
  end;
end;

class function TASfx.NanoParticles(StartPos: TPosition; TargetPos : TNanolathePos) : Cardinal;
begin
  Result := EmitSfx_NanoParticles(@StartPos, @TargetPos, 6);
end;

class function TASfx.NanoReverseParticles(StartPos: TPosition;
  TargetPos: TNanolathePos) : Cardinal;
begin
  Result := EmitSfx_NanoParticlesReverse(@TargetPos, @StartPos, 6);
end;

{ TAWeapon }

class function TAWeapon.GetWeaponID(WeaponPtr: PWeaponDef) : Cardinal;
begin
  if IniSettings.WeaponType <= 256 then
    Result := PWeaponDef(WeaponPtr)^.ucID
  else
    Result := PWeaponDef(WeaponPtr)^.lWeaponIDCrack;
end;

class function TAWeapon.WeaponId2Ptr(ID: Cardinal) : Pointer;
begin
  if IniSettings.WeaponType <= 256 then
    result:= @TAData.MainStruct.Weapons[ID]
  else
    result:= Pointer(Cardinal(TAData.WeaponTypeDefArrayPtr) + SizeOf(TWeaponDef) * ID);
end;

class function TAWeapon.FireMap_Weapon(WeaponPtr: PWeaponDef;
  TargetX, TargetZ: Cardinal) : Boolean;
var
  StartPos, TargetPos: TPosition;
begin
  TargetPos.X := TargetX;
  TargetPos.Z := TargetZ;

  StartPos.x_ := 0;
  StartPos.X := 0;
  StartPos.z_ := 0;
  StartPos.Z := 0;

  TargetPos.Y := 1350;
  StartPos.Y := 65521;

  Result := PROJECTILES_FireMapWeap(WeaponPtr, @TargetPos, @StartPos, True);
end;

class procedure TAMem.Position2Grid(Position: TPosition; out GridPosX, GridPosZ: Word);
begin
  GridPosX := Word(PInteger(@Position.x_)^ shr 20);
  GridPosZ := Word(PInteger(@Position.z_)^ shr 20);
end;

class function TAMem.PositionInMapArea(Position: TPosition): Boolean;
var
  GridPosX, GridPosZ: Word;
  TNTMemStruct: PTNTMemStruct;
begin
  PInteger(@Position.x_)^ := -1;
  TAMem.Position2Grid(Position, GridPosX, GridPosZ);
  TNTMemStruct := @TAData.MainStruct.TNTMemStruct;
  Result := not ((GridPosX < 1) or (GridPosZ < 1) or (GridPosX >= TNTMemStruct.lTilesetMapSizeX) or
   (GridPosZ >= TNTMemStruct.lTilesetMapSizeY));
end;

end.
