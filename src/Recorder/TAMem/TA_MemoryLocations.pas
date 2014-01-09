unit TA_MemoryLocations;

interface
uses
  dplay,
  Classes, strutils;
const
  MAXPLAYERCOUNT = 10;

  TADynmemStructPtr  = $511de8;
  TAdynmemStruct_SharedBits = $2a42;
  TAdynmemStruct_LOS_Sight = $2a43; // Byte;
  TAdynmemStruct_LOS_Type = $14281;
  TAdynmemStruct_Players = $1b63;  // 10 * PlayerStructSize
  TAdynmemStruct_GameSpeed = $38A4B; // Word
  TAdynmemStruct_IsPaused = $38A51;  //

  //1439B - unit info array
  //1438F - unit info count
  TAdynmemStruct_Units = $14357;// Pointer
  TAdynmemStruct_Units_EndMarker = $1435B; // Pointer
  TAdynmemStruct_UnitCount_Unk = $14353; // Pointer

  TAdynmemStruct_UnitInfoCount = $1438F;
  TAdynmemStruct_UnitInfoArray = $1439B; // Pointer

  TAdynmemStruct_ModelsArray = $14377; // array of pointers (4 bytes * model idx)
  TAdynmemStruct_WeaponTypedefArray = $2CF3;

  TAdynmemStruct_MaxUnitLimit = $37EEC; // Word
  TAdynmemStruct_ActualUnitLimit = $37EEA;  // Word

  TAdynmemStruct_IsAlteredUnitLimit = $589;    // Byte;

  TAdynmemStruct_GameState = $3923B;

  TAMovementClassArray = $512358;
  
  PlayerStructSize = $14b;
  PlayerStruct_Active = $0;
  PlayerStruct_DPID = $4;
  PlayerStruct_PlayerType = $73;
  PlayerStruct_PlayerInfo = $27;
  PlayerStruct_Index = $146;
  PlayerStruct_AlliedPlayers = $108;
  PlayerStruct_Units = $67;
  PlayerStruct_Units_End = $6b;

  PlayerInfoStruct_SharedBits = $97;
  PlayerInfoStruct_IsWatching = $9B;

  UnitStructSize = $118;
  UnitStruct_MoveClass = $0;
  UnitStruct_OwnerIndex = $FF;

  UnitStruct_Weapon1_p = $10;
  UnitStruct_Weapon2_p = $2C;
  UnitStruct_Weapon3_p = $48;

  UnitStruct_Pos = $6A;
  UnitStruct_Posx_ = $0;
  UnitStruct_PosX = $2;
  UnitStruct_Posy_ = $4;
  UnitStruct_PosY = $6;
  UnitStruct_PosZ_ = $8;
  UnitStruct_PosZ = $A;

  UnitStruct_UNITINFO_p = $92;
  UnitStruct_UnitINFOID = $A6;

  UnitStruct_OwnerPtr = $96;
  UnitStruct_UnitInGameIndex = $A8;
  UnitStruct_Kills = $B8; // Word
  UnitStruct_BuildTimeLeft = $104; // dWord
  UnitStruct_HealthVal = $108;  // Word
  UnitStruct_cIsCloaked = $10E;  // Word, get is correct, set can be false positive, use unit state mask
  UnitStruct_UnitStateMask = $110; // DWord
  UnitStruct_COBDataPtr = $18E;

  UnitInfoStructSize = $249;
  UnitInfoStruct_movementclass = $1B6;

  WeaponTypedefStructSize = $115;
  WeaponTypedefStruct_WeaponName = $0; // array of Byte 32
  WeaponTypedefStruct_ID = $10A; // Byte

  SPEED_CONSTANT : double = 0.0000152587890625;

const
  ShiftBiuldClick_Add : PShortInt = PShortInt($41AC14);
  ShiftBiuldClick_Sub : PShortInt = PShortInt($41AC18);


// PLongWord(0x4BF8C0)^ := 0xcc2 // disable TA buildrectangel, note: use WriteProcessMemory
// PLongWord(0x4BF8C0)^ := 0x5368EC83 // enable TA buildrectangel, note: use WriteProcessMemory

const
  BoolValues : array [Boolean] of Byte = (0,1);
type
  PAlliedState = ^TAlliedState;
  TAlliedState = array [ 0..9 ] of Byte;

  // 0x20
  PMoveInfoClassStruct = ^TMoveInfoClassStruct;
  TMoveInfoClassStruct = packed record
    pName          : Pointer;
    nFootPrintX    : Word; // default: 0
    nFootPrintZ    : Word; // default: 0
    nMaxWaterDepth : SmallInt; // default: 1027 = 10000
    nMinWaterDepth : SmallInt; // default: F0D8 = -10000
    cMaxSlope      : ShortInt; // default: FF = -1
    cBadSlope      : ShortInt; // default: 7F (FF shr 1)
    cMaxWaterSlope : ShortInt; // default: FF = -1
    cBadWaterSlope : ShortInt; // default: 7F (FF shr 1)
    lUnknown1      : LongWord; // always C0 00 00 00
    lUnknown2      : LongWord; // always C4 00 00 00
    lUnknown3      : LongWord; // pointer ?
    lUnknown4      : LongWord; // always 00 00 00 00
  end;

  // 0x249
  PGameUnitfInfo = ^TGameUnitInfo;
  TGameUnitInfo = packed record
    szName               : array [0..31] of AnsiChar;
    szUnitName           : array [0..31] of AnsiChar;
    szUnitDescription    : array [0..63] of AnsiChar;
    szObjectName         : array [0..31] of AnsiChar;
    szSide               : array [0..7] of AnsiChar;
    nUID                 : Word;
    Unknown1             : array [0..19] of Byte;
    AIWeight             : array [0..63] of Byte;
    AILimit              : array [0..63] of Byte;
    Unknown2             : array [0..11] of Byte;
    nFootPrintX          : Word;
    nFootPrintZ          : Word; 
    pYardMap             : PLongWord;
    Unknown3             : array [0..11] of Byte;
    lWidthX              : LongWord;
    lUnknown4            : LongWord;
    lWidthY              : LongWord;
    lUnknown5            : LongWord;
    lWidthZ              : LongWord;
    lUnknown6            : LongWord;
    Unknown7             : array [0..15] of Byte;
    fBuildCostEnergy     : single;
    fBuildCostMetal      : single;
    pCOBScript           : PLongWord;
    lMaxSpeedRaw         : LongWord;
		lMaxSpeedSlope       : LongWord;    // (lMaxSpeedRaw shl 16) / ((cMaxSlope + 1) shl 16)
		lBrakeRate           : LongWord;
    lAcceleration        : LongWord;
    lBankScale           : LongWord;
    lPitchScale          : LongWord;
    lDamageModifier      : LongWord;
    lMoveRate1           : LongWord;
    lMoveRate2           : LongWord;
    lMovementClass       : LongWord;
	  nTurnRate            : Word;
    nCorpseIndex         : Word;
    nMaxWaterDepth       : SmallInt;
    nMinWaterDepth       : SmallInt;
		fEnergyMake          : single;
		fEnergyUse           : single;
		fMetalMake           : single;
		fMetalUse            : single;
		fWindGenerator       : single;
		fTidalGenerator      : single;
		fCloakCost           : single;
		fCloakCostMoving     : single;
    lEnergyStorage       : LongWord;
    lMetalStorage        : LongWord;
    lBuildTime           : LongWord;
    p_WeaponPrimary      : PLongWord;
    p_WeaponSecondary    : PLongWord;
    p_WeaponTertiary     : PLongWord;
		nMaxHP               : Word;
    Unknown8             : array [0..1] of Byte;
    nWorkerTime          : Word;
    nHealTime            : Word;
    nSightDistance       : Word;
    nRadarDistance       : Word;
    nSonarDistance       : Word;
    nMinCloakDistance    : Word;
    nRadarDistanceJam    : Word;
    nSonarDistanceJam    : Word;
    nSoundCategory       : Word;
    nBuildAngle          : Word;
    nBuildDistance       : Word;
    nManeuverLeashLength : Word;
    nAttackRunLength     : Word;
    nKamikazeDistance    : Word;
    nSortBias            : Word;
    nCruiseAlt           : Word;
    nCategory            : Word;
		p_ExplodeAs          : PLongWord;
		p_SelfDestructAsAs   : PLongWord;
    cMaxSlope            : ShortInt;
    cMaxWaterSlope       : ShortInt;
    cTransportSize       : Byte;
    cTransportCapacity   : Byte;
    cWaterLine           : Byte;
    cMakesMetal          : Byte;
    cGUINum              : Byte;
    cBMCode              : Byte;
    cDefaultMissionType  : Byte;
    p_WeaponPrimaryBadTargetCategoryArray: LongWord;
    p_WeaponSecondaryBadTargetCategoryArray: LongWord;
    p_WeaponSpecialBadTargetCategoryArray: LongWord;
    p_NoChaseCategoryMaskArray: LongWord;
    UnitTypeMask    : LongWord;
    lUnitBitFields2 : LongWord;
  end;

  PCustomUnitInfo = ^TCustomUnitInfo;
  TCustomUnitInfo = packed record
    unitId        : LongWord;
    unitIdRemote  : LongWord;  // owner's (unit upgrade packet sender) local unit id
    //OwnerPlayer  : Longint; // Buffer.EventPlayer_DirectID = PlayerAryIndex2ID(UnitInfo.ThisPlayer_ID   )
    InfoPtrOld   : LongWord;  // local old Pointer
    InfoStruct   : TGameUnitInfo;
  end;

  TUnitInfos = array of TCustomUnitInfo;

  TTAPlayerType = (Player_LocalHuman = 1, Player_LocalAI = 2, Player_RemotePlayer = 3);
  TAMem = class
  protected
    class Function GetViewPLayer : Byte;
    class Function GetGameSpeed : Byte;
    
    class Function GetMaxUnitLimit : Word;
    class Function GetActualUnitLimit : Word;
    class Function GetIsAlteredUnitLimit : Boolean;
    class function GetUnitsPtr : LongWord;
    class function GetUnits_EndMarkerPtr : LongWord;
    class function GetModelsArrayPtr : LongWord;
    class function GetUnitInfoPtr : LongWord;
    class function GetUnitInfoCount : LongWord;

    class Function GetPausedState : Boolean;
    class Procedure SetPausedState( value : Boolean);
  public
    Property Paused : Boolean read GetPausedState write SetPausedState;
    
    // will return nil for units without movement class (!)
    class Function GetUnitPtr(unitIndex : LongWord) : Pointer;
    class Function GetUnitStruct(unitIndex : LongWord) : LongWord;

    property ViewPlayer : Byte read GetViewPlayer;
    Property GameSpeed : Byte read GetGameSpeed;
    Property MaxUnitLimit : Word read GetMaxUnitLimit;
    Property ActualUnitLimit : Word read GetActualUnitLimit;
    Property IsAlteredUnitLimit : Boolean read GetIsAlteredUnitLimit;
    Property UnitsPtr : LongWord read GetUnitsPtr;
    Property Units_EndMarkerPtr : LongWord read GetUnits_EndMarkerPtr;
    Property UnitInfoPtr : LongWord read GetUnitInfoPtr;
    Property UnitInfoCount : LongWord read GetUnitInfoCount;

    class Function GetPlayerByIndex(playerIndex : LongWord) : Pointer;
    class Function GetPlayerByDPID(playerPID : TDPID) : Pointer;
    // zero based player index
    class Function GetPlayerIndex(playerPID : TDPID) : LongWord;

    class function GetModelPtr(index: Word): Pointer;
    class function GetTemplatePtr(index: Word): LongWord;
    class function GetMovementClassPtr(index: Word): LongWord;
    class function GetWeapon(weaponid: Word): LongWord;

    class function IsTAVersion31 : Boolean;
  end;

  TAPlayer = class
  protected
    class function GetShareEnergyVal : single;
    class function GetShareMetalVal : single;
    class function GetShareEnergy : Boolean;
    class function GetShareMetal : Boolean;
    class function GetShootAll : Boolean;
  public
    property ShareEnergyVal : single read GetShareEnergyVal;
    property ShareMetalVal : single read GetShareMetalVal;
    property ShareEnergy : Boolean read GetShareEnergy;
    property ShareMetal : Boolean read GetShareMetal;
    property ShootAll : Boolean read GetShootAll;

    class Function GetDPID(player : Pointer) : TDPID;
    class Function PlayerType(player : Pointer) : TTAPlayerType;

    class Procedure SetShareRadar(player : Pointer; value : Boolean);
    class function GetShareRadar(player : Pointer) : Boolean;

    class function GetIsWatcher(player : Pointer) : Boolean;
    class function GetIsActive(player : Pointer) : Boolean;

    class function GetAlliedState(Player1 : Pointer; Player2 : Integer) : Boolean;
    class Procedure SetAlliedState(Player1 : Pointer; Player2 : Integer; value : Boolean);
  end;

  TAUnit = class
  public
    class Function GetKills(unitptr : Pointer) : Word;
    class procedure SetKills(unitptr : Pointer; kills : Word);

    class Function GetHealth(unitptr : Pointer) : Word;
    class procedure SetHealth(unitptr : Pointer; health : LongWord);

    class Function GetCloak(unitptr : Pointer) : LongWord;
    class procedure SetCloak(unitptr : Pointer; cloak : Word);

    class function GetUnitX(unitptr : Pointer): LongWord;
    class function GetUnitY(unitptr : Pointer): LongWord;
    class function GetUnitZ(unitptr : Pointer): LongWord;
    class procedure SetUnitX(unitptr : Pointer; X : LongWord);
    class procedure SetUnitY(unitptr : Pointer; Y : LongWord);
    class procedure SetUnitZ(unitptr : Pointer; Z : LongWord);

    class procedure Kill(unitptr : Pointer; deathtype : Byte);
    class Function GetBuildTimeLeft(unitptr : Pointer) : single;

    class Function GetOwnerPtr(unitptr : Pointer) : Pointer;
    class Function GetOwnerIndex(unitptr : Pointer) : Integer;
    class Function IsOnThisComp(unitptr : Pointer) : LongWord;
    class Function IsLocal(unitptr: Pointer; remoteUnitId: PLongWord; out local: Boolean): LongWord;
    class Function GetId(unitptr : Pointer) : Word;
    class Function GetLongId(unitptr : Pointer) : LongWord;
    class Function GetCOBDataPtr(unitptr : Pointer) : LongWord;

    class function SearchCustomUnitInfos(unitId: LongWord; remoteUnitId: PLongWord; local: Boolean; out index: Integer ): Boolean;
    class function SetUpgradeable(unitptr: Pointer; State: Byte; remoteUnitId: PLongWord): Boolean;
    class function GetUnitTemplateId(unitptr: Pointer): Word;
    class function SetTemplate(unitptr: Pointer; newTemplateId: Word; recreate: boolean): Boolean;
    class function GetUnitInfoField(unitptr: Pointer; fieldType: LongWord; remoteUnitId: PLongWord): LongWord;
    class function SetUnitInfoField(unitptr: Pointer; fieldType: LongWord; value: LongWord; remoteUnitId: PLongWord): Boolean;

    class function GetMovementClass(unitptr : Pointer): LongWord;
    class function SetWeapon(unitptr : Pointer; index: LongWord; weaponid: Word; requirespatch: Boolean): Boolean;
    class function UpdateLos(unitptr: Pointer): LongWord;
    class procedure Speech(unitptr : longword; speechtype: longword; speechtext: PChar);
    class procedure SoundEffectId(unitptr : longword; voiceid: longword);
  end;

var
  TAData : TAMem;
  PlayerInfo: TAPlayer;
  
function IsTAVersion31 : Boolean;

implementation
uses
  sysutils,
  logging,
  TADemoConsts,
  TA_FunctionsU,
  COB_extensions,
  INI_Options,
  TAMemManipulations;

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
result := PByte(PLongWord(TADynmemStructPtr)^ + TAdynmemStruct_LOS_Sight)^;
end;

class Function TAMem.GetGameSpeed : Byte;
begin
result := PByte(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_GameSpeed)^;
end;

class function TAMem.GetMaxUnitLimit : Word;
begin
result := PWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_MaxUnitLimit)^;
end;

class function TAMem.GetActualUnitLimit : Word;
begin
result := PWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_ActualUnitLimit)^;
end;

class function TAMem.GetIsAlteredUnitLimit : Boolean;
begin
result := PByte(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_IsAlteredUnitLimit)^ <> 0;
end;

class function TAMem.GetUnitsPtr : LongWord;
begin
result := PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Units)^;
end;

class function TAMem.GetUnits_EndMarkerPtr : LongWord;
begin
result := PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Units_EndMarker)^;
end;

class function TAMem.GetModelsArrayPtr : LongWord;
begin
result := PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_ModelsArray)^;
end;

class function TAMem.GetUnitInfoPtr : LongWord;
begin
result := PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_UnitInfoArray)^;
end;

class function TAMem.GetUnitInfoCount : LongWord;
begin
result := PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_UnitInfoCount)^;
end;

class Function TAMem.GetPlayerByIndex(playerIndex : LongWord) : Pointer;
begin
result := Pointer( PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Players+(playerIndex*PlayerStructSize) );
end;

class Function TAMem.GetPlayerIndex(playerPID : TDPID) : LongWord;
var
  aplayerPID : PDPID;
  i : Integer;
begin
result := LongWord(-1);
aplayerPID := Pointer( PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Players+PlayerStruct_DPID );
i := 0;
while i < MAXPLAYERCOUNT do
  begin
  if aplayerPID^ = playerPID then
    begin
    result := i;
    break;
    end;
  aplayerPID := Pointer(LongWord(aplayerPID)+PlayerStructSize);
  inc(i);
  end;
end;

class Function TAMem.GetPausedState : Boolean;
begin
result := PByte(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_IsPaused)^ <> 0;
end;

class Procedure TAMem.SetPausedState( value : Boolean);
begin
PByte(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_IsPaused)^ := BoolValues[value]
end;

class Function TAMem.GetUnitPtr(unitIndex : LongWord) : Pointer;
begin
if (unitIndex > GetMaxUnitLimit * 10) then exit;
result := Pointer( PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Units)^+UnitStructSize*unitIndex );
end;

class Function TAMem.GetUnitStruct(unitIndex : LongWord) : LongWord;
begin
Result:= 0;
if (unitIndex > GetMaxUnitLimit * 10) then exit;
result := LongWord(PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Units)^+UnitStructSize*unitIndex );
end;

// -----------------------------------------------------------------------------

class Function TAMem.GetPlayerByDPID(playerPID : TDPID) : Pointer;
var
  aplayerPID : PDPID;
  i : Integer;
begin
result := nil;
aplayerPID := Pointer( PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Players+PlayerStruct_DPID );
i := 0;
while i < MAXPLAYERCOUNT do
  begin
  if aplayerPID^ = playerPID then
    begin
    result := Pointer(LongWord(aplayerPID) - PlayerStruct_DPID);
    break;
    end;
  aplayerPID := Pointer(LongWord(aplayerPID)+PlayerStructSize);
  inc(i);
  end;
end;

class function TAPlayer.GetAlliedState(Player1 : Pointer; Player2 : Integer) : Boolean;
begin
if (Player1 = nil) or (LongWord(Player2) >= MAXPLAYERCOUNT) then exit;
result := PAlliedState(LongWord(Player1)+PlayerStruct_AlliedPlayers)[Player2] <> 0;
end;

class Procedure TAPlayer.SetAlliedState(Player1 : Pointer; Player2 : Integer; value : Boolean);
begin
if (Player1 = nil) or (LongWord(Player2) >= MAXPLAYERCOUNT) then exit;
PAlliedState(LongWord(Player1)+PlayerStruct_AlliedPlayers)[Player2] := BoolValues[value]
end;

class Function TAPlayer.GetDPID(player : Pointer) : TDPID;
begin
result := PDPID(LongWord(player)+ PlayerStruct_DPID )^;
end;

class Function TAPlayer.PlayerType(player : Pointer) : TTAPlayerType;
begin
result := TTAPlayerType(PByte(LongWord(player)+ PlayerStruct_PlayerType )^);
end;

Class function TAPlayer.GetIsActive(player : Pointer) : Boolean;
begin
result := PLongWord(player)^ <> $0;
end;

Class function TAPlayer.GetIsWatcher(player : Pointer) : Boolean;
var
  Bitfield : PWord;
begin
Bitfield := PWord(PLongWord(LongWord(player)+PlayerStruct_PlayerInfo)^+PlayerInfoStruct_IsWatching);
result := Bitfield^ <> $40;
end;

const
  ShareRadar_BitMask = $40;
Class Procedure TAPlayer.SetShareRadar(player : Pointer; value : Boolean);
var
  Bitfield : PWord;
begin
Bitfield := PWord(PLongWord(LongWord(player)+PlayerStruct_PlayerInfo)^+PlayerInfoStruct_SharedBits);
if value then
  Bitfield^ := Bitfield^ or ShareRadar_BitMask
else
  Bitfield^ := Bitfield^ and not ShareRadar_BitMask
end;

Class function TAPlayer.GetShareRadar(player : Pointer) : Boolean;
var
  Bitfield : PByte;
begin
Bitfield := PByte(PLongWord(LongWord(player)+PlayerStruct_PlayerInfo)^+PlayerInfoStruct_IsWatching);
result := (Bitfield^ and $40) = $40;
end;

Class function TAPlayer.GetShareEnergyVal : single;
begin
result := PSingle( PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Players+$E8 )^;
end;

Class function TAPlayer.GetShareMetalVal : single;
begin
result := PSingle( PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Players+$E4 )^;
end;

Class function TAPlayer.GetShareEnergy : Boolean;
var
  Bitfield : PWord;
begin
Bitfield := PWord(PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Players+PlayerStruct_PlayerInfo)^+PlayerInfoStruct_SharedBits);
result := (Bitfield^ and $4) = $4;
end;

Class function TAPlayer.GetShareMetal : Boolean;
var
  Bitfield : PWord;
begin
Bitfield := PWord(PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Players+PlayerStruct_PlayerInfo)^+PlayerInfoStruct_SharedBits);
result := (Bitfield^ and $2) = $2;
end;

Class function TAPlayer.GetShootAll : Boolean;
begin
  if (PByte(PLongWord(TADynmemStructPtr)^+$37F30)^ and (1 shl 2)) = 4 then
    Result:= True
  else
    Result:= False;
end;

{
  edi - player

  xor eax,eax
  mov ecx, [edi+PlayerStruct_PlayerInfo]
  mov ax, Word ptr [ecx+PlayerInfoStruct_SharedBits]
  mov edx, eax
  xor dl, al
  and edx, ShareRadar_BitMask
  xor edx, eax
  mov Word ptr [ecx+PlayerInfoStruct_SharedBits], dx
}

// -----------------------------------------------------------------------------

class Function TAUnit.GetKills(unitptr : Pointer) : Word;
begin
result := PWord( LongWord(unitptr)+UnitStruct_Kills)^;
end;

class procedure TAUnit.SetKills(unitptr : Pointer; Kills : Word);
begin
PWord( LongWord(unitptr)+UnitStruct_Kills)^ := Kills;
end;

const
  Kill_BitMask = $40;
class procedure TAUnit.Kill(unitptr : Pointer; deathtype : Byte);
var
  Bitfield : PByte;
begin
Bitfield := PByte(LongWord(unitptr)+UnitStruct_UnitStateMask+$1);
if deathtype > 0 then
  Bitfield^ := Bitfield^ or Kill_BitMask
else
  Bitfield^ := Bitfield^ and not Kill_BitMask
end;

class Function TAUnit.GetHealth(unitptr : Pointer) : Word;
begin
result := PWord( LongWord(unitptr)+UnitStruct_HealthVal)^;
end;

class procedure TAUnit.SetHealth(unitptr : Pointer; Health : LongWord);
begin
PLongWord( LongWord(unitptr)+UnitStruct_HealthVal)^ := Health;
end;

class procedure TAUnit.SetUnitX(unitptr : Pointer; X : LongWord);
begin
PLongWord( LongWord(unitptr)+UnitStruct_Pos+UnitStruct_Posx_)^ := LongWord(X*163840);
end;

class procedure TAUnit.SetUnitY(unitptr : Pointer; Y : LongWord);
begin
PLongWord( LongWord(unitptr)+UnitStruct_Pos+UnitStruct_Posy_)^ := LongWord(Y*163840);
end;

class procedure TAUnit.SetUnitZ(unitptr : Pointer; Z : LongWord);
begin
PLongWord( LongWord(unitptr)+UnitStruct_Pos+UnitStruct_Posz_)^ := LongWord(Z*163840);
end;

class function TAUnit.GetUnitX(unitptr : Pointer): LongWord;
begin
result := PLongWord( LongWord(unitptr)+UnitStruct_Pos+UnitStruct_Posx_)^;
end;

class function TAUnit.GetUnitY(unitptr : Pointer): LongWord;
begin
result := PLongWord( LongWord(unitptr)+UnitStruct_Pos+UnitStruct_Posy_)^;
end;

class function TAUnit.GetUnitZ(unitptr : Pointer): LongWord;
begin
result := PLongWord( LongWord(unitptr)+UnitStruct_Pos+UnitStruct_Posz_)^;
end;

class function TAUnit.GetMovementClass(unitptr : Pointer): LongWord;
begin
result := PLongWord( LongWord(unitptr)+UnitStruct_MoveClass)^;
end;

const
  Cloak_BitMask = $4;
  CloakUnitStateMask_BitMask = $8;
class procedure TAUnit.SetCloak(unitptr : Pointer; Cloak : Word);
var
  Bitfield : PWord;
begin
  Bitfield := PWord(LongWord(unitptr)+UnitStruct_UnitStateMask+$1);
  if Cloak = 1 then
    Bitfield^ := Bitfield^ or CloakUnitStateMask_BitMask
  else
    Bitfield^ := Bitfield^ and not CloakUnitStateMask_BitMask;
end;

Class function TAUnit.GetCloak(unitptr : Pointer) : LongWord;
var
  Bitfield : PWord;
begin
  Bitfield := PWord(LongWord(unitptr)+UnitStruct_cIsCloaked);
  if (Bitfield^ and Cloak_BitMask) = Cloak_BitMask then
    Result:= 1
  else
    Result:= 0;
end;

class Function TAUnit.GetBuildTimeLeft(unitptr : Pointer) : single;
begin
result := PSingle( LongWord(unitptr)+UnitStruct_BuildTimeLeft)^;
end;

class Function TAUnit.GetOwnerPtr(unitptr : Pointer) : Pointer;
begin
result := PPointer( LongWord(unitptr)+UnitStruct_OwnerPtr)^;
end;

class Function TAUnit.GetOwnerIndex(unitptr : Pointer) : Integer;
begin
result := PByte( LongWord(unitptr)+UnitStruct_OwnerIndex)^;
end;

class Function TAUnit.IsOnThisComp(unitptr : Pointer) : LongWord;
var
  playerPtr : Pointer;
  TAPlayerType : TTAPlayerType;
begin
try
  playerPtr := TAUnit.GetOwnerPtr(Pointer(unitptr));
  TAPlayerType := TAPlayer.PlayerType(playerPtr);
  case TAPlayerType of
    Player_LocalHuman:
      result := 1;
    Player_LocalAI:
      result := 1;
    else
      result := 0;
  end;
except
  result := 0;
end;
end;

class Function TAUnit.IsLocal(unitptr: Pointer; remoteUnitId: PLongWord; out Local: Boolean): LongWord;
begin
  Local:= False;
  Result:= 0;
  if remoteUnitId <> nil then
  begin
    Local:= False;
    Result:= Word(remoteUnitId);
  end else
  begin
    if unitptr <> nil then
    begin
      Local:= (IsOnThisComp(Pointer(unitptr)) = 1);
      Result:= TAUnit.GetLongId(Pointer(unitptr));
    end;
  end;
end;

class Function TAUnit.GetId(unitptr : Pointer) : Word;
begin
result := PWord( LongWord(unitptr)+UnitStruct_UnitInGameIndex)^;
end;

class Function TAUnit.GetLongId(unitptr : Pointer) : LongWord;
begin
result := PLongWord( LongWord(unitptr)+UnitStruct_UnitInGameIndex)^;
end;

class Function TAUnit.GetCOBDataPtr(unitptr : Pointer) : LongWord;
begin
result := PLongWord( LongWord(unitptr)+UnitStruct_COBDataPtr)^;
end;

class function TAMem.GetMovementClassPtr(index: Word): LongWord;
begin
result := TAMovementClassArray + SizeOf(TMoveInfoClassStruct) * index;
end;

class function TAMem.GetWeapon(weaponid: Word): LongWord;
begin
// fix me to be compatible with xpoy id patch
  if iniSettings.weaponidpatch then
  //
  else
    result:= PLongWord(TADynmemStructPtr)^+TAdynmemStruct_WeaponTypedefArray + weaponid * WeaponTypedefStructSize;
end;

class function TAMem.GetModelPtr(index: Word): Pointer;
begin
result := PLongWord(GetModelsArrayPtr + index * 4);
end;

class function TAMem.GetTemplatePtr(index: Word): LongWord;
begin
result := GetUnitInfoPtr + index * UnitInfoStructSize;
end;

class function TAUnit.GetUnitTemplateId(unitptr: Pointer): Word;
begin
result := PWord( LongWord(unitptr)+UnitStruct_UnitINFOID)^;
end;

class function TAUnit.SetTemplate(unitptr: Pointer; newTemplateId: Word; recreate: boolean): Boolean;
var
  newTemplate: LongWord;
  ResultPtr: LongWord;
begin
  Result:= False;
  if unitptr <> nil then
  begin
    newTemplate:= TAMem.GetTemplatePtr(newTemplateID);
    PWord(LongWord(unitptr)+UnitStruct_UNITINFOID)^ := newTemplateID;
    PLongWord(LongWord(unitptr)+UnitStruct_UNITINFO_p)^ := newTemplate;
    if recreate then
    begin
      ObjectCOBandSomething(LongWord(unitptr));
      ResultPtr:= CreateMovementClass(LongWord(unitptr));
    end;
    TAUnit.UpdateLos(unitptr);
    Result:= True;
  end;
end;

class function TAUnit.SetWeapon(unitptr : Pointer; index: LongWord; weaponid: Word; requirespatch: Boolean): Boolean;
var
  Weapon: LongWord;
begin
  Result:= False;
  if unitptr <> nil then
  begin
    if requirespatch and iniSettings.weaponidpatch then
      Weapon:= TAMem.GetWeapon(weaponid)
    else
      Weapon:= TAMem.GetWeapon(Byte(weaponid));
    case index of
      WEAPON1: PLongWord(LongWord(unitptr)+UnitStruct_Weapon1_p)^ := Weapon;
      WEAPON2: PLongWord(LongWord(unitptr)+UnitStruct_Weapon2_p)^ := Weapon;
      WEAPON3: PLongWord(LongWord(unitptr)+UnitStruct_Weapon3_p)^ := Weapon;
    end;
    Result:= True;
  end;
end;

class function TAUnit.UpdateLos(unitptr : pointer): LongWord;
begin
  TA_UpdateUnitLOS(LongWord(unitptr));
  Result:= TA_UpdateLOS(LongWord(TAUnit.GetOwnerIndex(unitptr)), 1);
end;

class procedure TAUnit.Speech(unitptr : longword; speechtype: longword; speechtext: PChar);
begin
  PlaySound_UnitSpeech(unitptr, speechtype, speechtext);
end;

class procedure TAUnit.SoundEffectId(unitptr : longword; voiceid: longword);
begin
  PlaySound_EffectId(voiceid, unitptr);
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
class function TAUnit.SetUpgradeable(unitptr: Pointer; State: Byte; remoteUnitId: PLongWord): Boolean;
var
  tmpUnitInfo: TCustomUnitInfo;
  arrIdx: Integer;
  unitId: LongWord;
  local: Boolean;
  templateFound: Boolean;
begin
  Result:= False;
  unitId:= IsLocal(unitptr,remoteUnitId, Local);
  TemplateFound:= TAUnit.SearchCustomUnitInfos(unitId, remoteUnitId, Local, arrIdx);

  if (not TemplateFound) and (arrIdx = -1) then
  begin
    TmpUnitInfo.unitId:= unitId;
    if Local then
      TmpUnitInfo.InfoPtrOld:= PLongWord(LongWord(Pointer(unitptr))+UnitStruct_UNITINFO_p)^
    else
      begin
        TmpUnitInfo.unitIdRemote:= LongWord(remoteUnitId);
        TmpUnitInfo.InfoPtrOld:= PLongWord(LongWord(TAMem.GetUnitStruct(unitId))+UnitStruct_UNITINFO_p)^;
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
        CustomUnitInfosArray[arrIdx].InfoPtrOld:= TAMem.GetTemplatePtr(PWord(LongWord(TAMem.GetUnitStruct(Word(remoteUnitId)))+UnitStruct_UnitINFOID)^);
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
          PLongWord(LongWord(Pointer(unitptr))+UnitStruct_UNITINFO_p)^ := CustomUnitInfosArray[arrIdx].InfoPtrOld
        else
          PLongWord(LongWord(TAMem.GetUnitStruct(unitId))+UnitStruct_UNITINFO_p)^ := CustomUnitInfosArray[arrIdx].InfoPtrOld;
      end else
      begin
        if Local then
          PLongWord(LongWord(Pointer(unitptr))+UnitStruct_UNITINFO_p)^ := LongWord(@CustomUnitInfosArray[arrIdx].InfoStruct)
        else
          PLongWord(LongWord(TAMem.GetUnitStruct(unitId))+UnitStruct_UNITINFO_p)^ := LongWord(@CustomUnitInfosArray[arrIdx].InfoStruct);
      end;
    end;
end;

class function TAUnit.GetUnitInfoField(unitptr: Pointer; fieldType: LongWord; remoteUnitId: PLongWord): LongWord;
var
  arrIdx: Integer;
  unitId: LongWord;
  local: Boolean;
  custTmplFound: boolean;
  engineTmpl: PGameUnitfInfo;
begin
  Result:= 0;
  unitId:= IsLocal(unitptr, remoteUnitId, local);
  custTmplFound:= TAUnit.SearchCustomUnitInfos(unitId, remoteUnitId, local, arrIdx);

  // if getter was done for a unit that has no custom unitinfo template
  // we are going to read field value from TA's GameUnitInfo array
  if not custTmplFound then
  begin
    engineTmpl:= Pointer(TAMem.GetTemplatePtr(GetUnitTemplateId(Pointer(TAMem.GetUnitStruct(Word(unitId))))));
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
    MANEUVERLEASH   : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nManeuverLeashLength else Result:= engineTmpl.nManeuverLeashLength;
    ATTACKRUNLEN    : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nAttackRunLength else Result:= engineTmpl.nAttackRunLength;
    MAXWATERDEPTH   : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nMaxWaterDepth else Result:= engineTmpl.nMaxWaterDepth;
    MINWATERDEPTH   : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.nMinWaterDepth else Result:= engineTmpl.nMinWaterDepth;
    MAXSLOPE        : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.cMaxSlope else Result:= engineTmpl.cMaxSlope;
    MAXWATERSLOPE   : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.cMaxWaterSlope else Result:= engineTmpl.cMaxWaterSlope;
    WATERLINE       : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.cWaterLine else Result:= engineTmpl.cWaterLine;

    TRANSPORTSIZE   : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.cTransportSize else Result:= engineTmpl.cTransportSize;
    TRANSPORTCAP    : if custTmplFound then Result:= CustomUnitInfosArray[arrIdx].InfoStruct.cTransportCapacity else Result:= engineTmpl.cTransportCapacity;

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

class function TAUnit.SetUnitInfoField(unitptr: Pointer; fieldType: LongWord; value: LongWord; remoteUnitId: PLongWord): Boolean;
var
  arrIdx: Integer;
  unitId: LongWord;
  local: Boolean;
  MovementClassStruct: PMoveInfoClassStruct;
begin
  Result:= False;
  unitId:= IsLocal(unitptr, remoteUnitId, local);
  if TAUnit.SearchCustomUnitInfos(unitId, remoteUnitId, local, arrIdx) then
  begin
    case fieldType of
      SOUNDCTGR : CustomUnitInfosArray[arrIdx].InfoStruct.nSoundCategory := Word(value);
      MOVEMENTCLASS :
        begin
        // movement class information is stored in both - specific and global template
        // we fix both of them to make TA engine happy
        CustomUnitInfosArray[arrIdx].InfoStruct.lMovementClass:= TAMem.GetMovementClassPtr(Word(value));
        MovementClassStruct:= Pointer(CustomUnitInfosArray[arrIdx].InfoStruct.lMovementClass);
        if MovementClassStruct.pName <> nil then
          begin
          CustomUnitInfosArray[arrIdx].InfoStruct.nMaxWaterDepth:= MovementClassStruct.nMaxWaterDepth;
          CustomUnitInfosArray[arrIdx].InfoStruct.nMinWaterDepth:= MovementClassStruct.nMinWaterDepth;
          CustomUnitInfosArray[arrIdx].InfoStruct.cMaxSlope:= MovementClassStruct.cMaxSlope;
          CustomUnitInfosArray[arrIdx].InfoStruct.cMaxWaterSlope:= MovementClassStruct.cMaxWaterSlope;
          CreateMovementClass(TAMem.GetUnitStruct(Word(unitId)));
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
      MANEUVERLEASH   : CustomUnitInfosArray[arrIdx].InfoStruct.nManeuverLeashLength := Word(value);
      ATTACKRUNLEN    : CustomUnitInfosArray[arrIdx].InfoStruct.nAttackRunLength := Word(value);
      MAXWATERDEPTH   : CustomUnitInfosArray[arrIdx].InfoStruct.nMaxWaterDepth := SmallInt(value);
      MINWATERDEPTH   : CustomUnitInfosArray[arrIdx].InfoStruct.nMinWaterDepth := SmallInt(value);
      MAXSLOPE        : CustomUnitInfosArray[arrIdx].InfoStruct.cMaxSlope := ShortInt(value);
      MAXWATERSLOPE   : CustomUnitInfosArray[arrIdx].InfoStruct.cMaxWaterSlope := ShortInt(value);
      WATERLINE       : CustomUnitInfosArray[arrIdx].InfoStruct.cWaterLine := Byte(value);

      TRANSPORTSIZE   : CustomUnitInfosArray[arrIdx].InfoStruct.cTransportSize := Byte(value);
      TRANSPORTCAP    : CustomUnitInfosArray[arrIdx].InfoStruct.cTransportCapacity := Byte(value);

      BANKSCALE       : CustomUnitInfosArray[arrIdx].InfoStruct.lBankScale := value;
      KAMIKAZEDIST    : CustomUnitInfosArray[arrIdx].InfoStruct.nKamikazeDistance := Word(value);
      DAMAGEMODIFIER  : CustomUnitInfosArray[arrIdx].InfoStruct.lDamageModifier := value;

      WORKERTIME      : CustomUnitInfosArray[arrIdx].InfoStruct.nWorkerTime := Word(value);
      BUILDDIST       : CustomUnitInfosArray[arrIdx].InfoStruct.nBuildDistance := Word(value);

      SIGHTDIST       : begin
                        CustomUnitInfosArray[arrIdx].InfoStruct.nSightDistance := Word(value);
                        TAUnit.UpdateLos(Pointer(TAMem.GetUnitStruct(Word(unitId))));
                        end;
      RADARDIST       : begin
                        CustomUnitInfosArray[arrIdx].InfoStruct.nRadarDistance := Word(value);
                        TAUnit.UpdateLos(Pointer(TAMem.GetUnitStruct(Word(unitId))));
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
    end;
    Result:= True;
  end;
end;

end.
