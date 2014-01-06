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

  TAdynmemStruct_WeaponTypedefArray = $2CF3;

  TAdynmemStruct_MaxUnitLimit = $37EEC; // Word
  TAdynmemStruct_ActualUnitLimit = $37EEA;  // Word

  TAdynmemStruct_IsAlteredUnitLimit = $589;    // Byte;

  TAdynmemStruct_GameState = $3923B;

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
  UnitStruct_UnitStateMask = $110; // dWord

  UnitInfoStructSize = $249;
  UnitInfoStruct_movementclass = $1B6;

  WeaponTypedefStructSize = $115;
  WeaponTypedefStruct_WeaponName = $0; // array of Byte 32
  WeaponTypedefStruct_ID = $10A; // Byte

  TAMovementClassArray = $512358;
  TAMovementClassStruct_Size = 32;

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
  PUnitfInfo = ^TGameUnitInfo;
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
    class Function getViewPLayer : Byte;
    class Function getGameSpeed : Byte;
    
    class Function getMaxUnitLimit : Word;
    class Function getActualUnitLimit : Word;
    class Function getIsAlteredUnitLimit : Boolean;
    class function getUnitsPtr : LongWord;
    class function getUnits_EndMarkerPtr : LongWord;
    class function getUnitInfoPtr : LongWord;
    class function getUnitInfoCount : LongWord;

    class Function getPausedState : Boolean;
    class Procedure SetPausedState( value : Boolean);
  public
    Property Paused : Boolean read getPausedState write setPausedState;
    
    // will return nil for units without movement class (!)
    class Function getUnitPtr(unitIndex : LongWord) : Pointer;
    class Function getUnitStruct(unitIndex : LongWord) : LongWord;

    property ViewPlayer : Byte read getViewPlayer;
    Property GameSpeed : Byte read getGameSpeed;
    Property MaxUnitLimit : Word read getMaxUnitLimit;
    Property ActualUnitLimit : Word read getActualUnitLimit;
    Property IsAlteredUnitLimit : Boolean read getIsAlteredUnitLimit;
    Property UnitsPtr : LongWord read getUnitsPtr;
    Property Units_EndMarkerPtr : LongWord read getUnits_EndMarkerPtr;
    Property UnitInfoPtr : LongWord read getUnitInfoPtr;
    Property UnitInfoCount : LongWord read getUnitInfoCount;

    class Function getPlayerByIndex(playerIndex : LongWord) : Pointer; 
    class Function getPlayerByDPID(playerPID : TDPID) : Pointer;
    // zero based player index
    class Function getPlayerIndex(playerPID : TDPID) : LongWord;

    class function getTemplatePtr(templateid: Word): LongWord;
    class function getMovementClassPtr(classid: Word): LongWord;
    class function getWeapon(weaponid: Word): LongWord;

    class function IsTAVersion31 : Boolean;
  end;

  TAPlayer = class
  protected
    class function getShareEnergyVal : single;
    class function getShareMetalVal : single;
    class function getShareEnergy : Boolean;
    class function getShareMetal : Boolean;
    class function getShootAll : Boolean;
  public
    property ShareEnergyVal : single read getShareEnergyVal;
    property ShareMetalVal : single read getShareMetalVal;
    property ShareEnergy : Boolean read getShareEnergy;
    property ShareMetal : Boolean read getShareMetal;
    property ShootAll : Boolean read getShootAll;

    class Function getDPID(player : Pointer) : TDPID;
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
    class Function getKills(unitptr : Pointer) : Word;
    class procedure setKills(unitptr : Pointer; kills : Word);

    class Function getHealth(unitptr : Pointer) : Word;
    class procedure setHealth(unitptr : Pointer; health : LongWord);

    class Function getCloak(unitptr : Pointer) : LongWord;
    class procedure setCloak(unitptr : Pointer; cloak : Word);

    class function getUnitX(unitptr : Pointer): LongWord;
    class function getUnitY(unitptr : Pointer): LongWord;
    class function getUnitZ(unitptr : Pointer): LongWord;
    class procedure setUnitX(unitptr : Pointer; X : LongWord);
    class procedure setUnitY(unitptr : Pointer; Y : LongWord);
    class procedure setUnitZ(unitptr : Pointer; Z : LongWord);

    class procedure Kill(unitptr : Pointer; deathtype : Byte);
    class Function getBuildTimeLeft(unitptr : Pointer) : single;

    class Function GetOwnerPtr(unitptr : Pointer) : Pointer;
    class Function GetOwnerIndex(unitptr : Pointer) : Integer;
    class Function IsOnThisComp(unitptr : Pointer) : LongWord;
    class Function IsLocal(unitptr: Pointer; remoteUnitId: PLongWord; out local: Boolean): LongWord;
    class Function GetId(unitptr : Pointer) : Word;
    class Function GetLongId(unitptr : Pointer) : LongWord;

    class function SearchCustomUnitInfos(unitId: LongWord; remoteUnitId: PLongWord; local: Boolean; out index: Integer ): Boolean;
    class function setUpgradeable(unitptr: Pointer; State: Byte; remoteUnitId: PLongWord): Byte;
    class function getUnitTemplateId(unitptr: Pointer): Word;
    class function getUnitInfoField(unitptr: Pointer; fieldType: LongWord; remoteUnitId: PLongWord): LongWord;
    class function setUnitInfoField(unitptr: Pointer; fieldType: LongWord; value: LongWord; remoteUnitId: PLongWord): LongWord;

    class function getMovementClass(unitptr : Pointer): LongWord;
    class function setWeapon(unitptr : Pointer; index: LongWord; weaponid: Word; requirespatch: Boolean): Boolean;
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

class Function TAMem.getViewPLayer : Byte;
begin
result := PByte(PLongWord(TADynmemStructPtr)^ + TAdynmemStruct_LOS_Sight)^;
end;

class Function TAMem.getGameSpeed : Byte;
begin
result := PByte(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_GameSpeed)^;
end;

class function TAMem.getMaxUnitLimit : Word;
begin
result := PWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_MaxUnitLimit)^;
end;

class function TAMem.getActualUnitLimit : Word;
begin
result := PWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_ActualUnitLimit)^;
end;

class function TAMem.getIsAlteredUnitLimit : Boolean;
begin
result := PByte(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_IsAlteredUnitLimit)^ <> 0;
end;

class function TAMem.getUnitsPtr : LongWord;
begin
result := PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Units)^;
end;

class function TAMem.getUnits_EndMarkerPtr : LongWord;
begin
result := PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Units_EndMarker)^;
end;

class function TAMem.getUnitInfoPtr : LongWord;
begin
result := PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_UnitInfoArray)^;
end;

class function TAMem.getUnitInfoCount : LongWord;
begin
result := PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_UnitInfoCount)^;
end;

class Function TAMem.getPlayerByIndex(playerIndex : LongWord) : Pointer;
begin
result := Pointer( PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Players+(playerIndex*PlayerStructSize) );
end;

class Function TAMem.getPlayerIndex(playerPID : TDPID) : LongWord;
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

class Function TAMem.getPausedState : Boolean;
begin
result := PByte(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_IsPaused)^ <> 0;
end;

class Procedure TAMem.SetPausedState( value : Boolean);
begin
PByte(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_IsPaused)^ := BoolValues[value]
end;

class Function TAMem.getUnitPtr(unitIndex : LongWord) : Pointer;
begin
if (unitIndex > getMaxUnitLimit * 10) then exit;
result := Pointer( PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Units)^+UnitStructSize*unitIndex );
end;

class Function TAMem.getUnitStruct(unitIndex : LongWord) : LongWord;
begin
Result:= 0;
if (unitIndex > getMaxUnitLimit * 10) then exit;
result := LongWord(PLongWord(PLongWord(TADynmemStructPtr)^+TAdynmemStruct_Units)^+UnitStructSize*unitIndex );
end;

// -----------------------------------------------------------------------------

class Function TAMem.getPlayerByDPID(playerPID : TDPID) : Pointer;
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

class Function TAPlayer.getDPID(player : Pointer) : TDPID;
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

class Function TAUnit.getKills(unitptr : Pointer) : Word;
begin
result := PWord( LongWord(unitptr)+UnitStruct_Kills)^;
end;

class procedure TAUnit.setKills(unitptr : Pointer; Kills : Word);
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

class Function TAUnit.getHealth(unitptr : Pointer) : Word;
begin
result := PWord( LongWord(unitptr)+UnitStruct_HealthVal)^;
end;

class procedure TAUnit.setHealth(unitptr : Pointer; Health : LongWord);
begin
PLongWord( LongWord(unitptr)+UnitStruct_HealthVal)^ := Health;
end;

class procedure TAUnit.setUnitX(unitptr : Pointer; X : LongWord);
begin
PLongWord( LongWord(unitptr)+UnitStruct_Pos+UnitStruct_Posx_)^ := LongWord(X*163840);
end;

class procedure TAUnit.setUnitY(unitptr : Pointer; Y : LongWord);
begin
PLongWord( LongWord(unitptr)+UnitStruct_Pos+UnitStruct_Posy_)^ := LongWord(Y*163840);
end;

class procedure TAUnit.setUnitZ(unitptr : Pointer; Z : LongWord);
begin
PLongWord( LongWord(unitptr)+UnitStruct_Pos+UnitStruct_Posz_)^ := LongWord(Z*163840);
end;

class function TAUnit.getUnitX(unitptr : Pointer): LongWord;
begin
result := PLongWord( LongWord(unitptr)+UnitStruct_Pos+UnitStruct_Posx_)^;
end;

class function TAUnit.getUnitY(unitptr : Pointer): LongWord;
begin
result := PLongWord( LongWord(unitptr)+UnitStruct_Pos+UnitStruct_Posy_)^;
end;

class function TAUnit.getUnitZ(unitptr : Pointer): LongWord;
begin
result := PLongWord( LongWord(unitptr)+UnitStruct_Pos+UnitStruct_Posz_)^;
end;

class function TAUnit.getMovementClass(unitptr : Pointer): LongWord;
begin
result := PLongWord( LongWord(unitptr)+UnitStruct_MoveClass)^;
end;

const
  Cloak_BitMask = $4;
  CloakUnitStateMask_BitMask = $8;
class procedure TAUnit.setCloak(unitptr : Pointer; Cloak : Word);
var
  Bitfield : PWord;
begin
  Bitfield := PWord(LongWord(unitptr)+UnitStruct_UnitStateMask+$1);
  if Cloak = 1 then
    Bitfield^ := Bitfield^ or CloakUnitStateMask_BitMask
  else
    Bitfield^ := Bitfield^ and not CloakUnitStateMask_BitMask;
end;

Class function TAUnit.getCloak(unitptr : Pointer) : LongWord;
var
  Bitfield : PWord;
begin
  Bitfield := PWord(LongWord(unitptr)+UnitStruct_cIsCloaked);
  if (Bitfield^ and Cloak_BitMask) = Cloak_BitMask then
    Result:= 1
  else
    Result:= 0;
end;

class Function TAUnit.getBuildTimeLeft(unitptr : Pointer) : single;
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

class function TAMem.getTemplatePtr(templateid: Word): LongWord;
begin
result := getUnitInfoPtr + templateid * UnitInfoStructSize;
end;

class function TAUnit.getUnitTemplateId(unitptr: Pointer): Word;
begin
result := PWord( LongWord(unitptr)+UnitStruct_UnitINFOID)^;
end;

class function TAMem.getMovementClassPtr(classid: Word): LongWord;
begin
result := TAMovementClassArray + TAMovementClassStruct_Size * classid;
end;

class function TAMem.getWeapon(weaponid: Word): LongWord;
begin
// fix me to be compatible with xpoy id patch
  if iniSettings.weaponidpatch then
  //
  else
    result:= PLongWord(TADynmemStructPtr)^+TAdynmemStruct_WeaponTypedefArray + weaponid * WeaponTypedefStructSize;
end;

{class procedure TAUnit.setTemplate(unitptr : Pointer; newTemplateID: Word);
var
  newTemplate: LongWord;
  ResultPtr: LongWord;
begin
  if unitptr <> nil then
  begin
    newTemplate:= TAMem.getTemplatePtr(newTemplateID);
    PWord(LongWord(unitptr)+UnitStruct_UNITINFOID)^ := newTemplateID;
    PLongWord(LongWord(unitptr)+UnitStruct_UNITINFO_p)^ := newTemplate;
    ResultPtr:= CreateMovementClass(LongWord(unitptr));
  end;
end;  }

class function TAUnit.setWeapon(unitptr : Pointer; index: LongWord; weaponid: Word; requirespatch: Boolean): Boolean;
var
  Weapon: LongWord;
begin
  Result:= False;
  if unitptr <> nil then
  begin
    if requirespatch and iniSettings.weaponidpatch then
      Weapon:= TAMem.getWeapon(weaponid)
    else
      Weapon:= TAMem.getWeapon(Byte(weaponid));
    case index of
      WEAPON1: PLongWord(LongWord(unitptr)+UnitStruct_Weapon1_p)^ := Weapon;
      WEAPON2: PLongWord(LongWord(unitptr)+UnitStruct_Weapon2_p)^ := Weapon;
      WEAPON3: PLongWord(LongWord(unitptr)+UnitStruct_Weapon3_p)^ := Weapon;
    end;
    Result:= True;
  end;
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
class function TAUnit.setUpgradeable(unitptr: Pointer; State: Byte; remoteUnitId: PLongWord): Byte;
var
  tmpUnitInfo: TCustomUnitInfo;
  arrId: Integer;
  unitId: LongWord;
  local: Boolean;
  templateFound: Boolean;
begin
  Result:= 0;
  unitId:= IsLocal(unitptr,remoteUnitId, Local);
  TemplateFound:= TAUnit.SearchCustomUnitInfos(unitId, remoteUnitId, Local, ArrId);

  if (not TemplateFound) and (ArrId = -1) then
  begin
    TmpUnitInfo.unitId:= unitId;
    if Local then
      TmpUnitInfo.InfoPtrOld:= PLongWord(LongWord(Pointer(unitptr))+UnitStruct_UNITINFO_p)^
    else
      begin
        TmpUnitInfo.unitIdRemote:= LongWord(remoteUnitId);
        TmpUnitInfo.InfoPtrOld:= PLongWord(LongWord(TAMem.getUnitStruct(unitId))+UnitStruct_UNITINFO_p)^;
      end;
    TmpUnitInfo.InfoStruct:= PUnitfInfo(TmpUnitInfo.InfoPtrOld)^;
    ArrId:= CustomUnitInfos.Add(TmpUnitInfo);
    TemplateFound:= True;
  end else
    if (not Local) then
    begin
     // if state = 1 then
    //  begin
        // received remote setupgradeable but unit with the same short id already exists in array,
        // let's overwrite array element with new data, so we preserve pointers to other elements, hopefully not crashing TA
        CustomUnitInfosArray[ArrId].unitIdRemote:= LongWord(remoteUnitId);
        CustomUnitInfosArray[ArrId].InfoPtrOld:= TAMem.getTemplatePtr(PWord(LongWord(TAMem.getUnitStruct(Word(remoteUnitId)))+UnitStruct_UnitINFOID)^);
        CustomUnitInfosArray[ArrId].InfoStruct:= PUnitfInfo(CustomUnitInfosArray[ArrId].InfoPtrOld)^;
        TemplateFound:= True;
    //  end;
    end;

    if TemplateFound then
    begin
      Result:= 1;
      if state = 0 then
      begin
        if Local then
          PLongWord(LongWord(Pointer(unitptr))+UnitStruct_UNITINFO_p)^ := CustomUnitInfosArray[ArrId].InfoPtrOld
        else
          PLongWord(LongWord(TAMem.getUnitStruct(unitId))+UnitStruct_UNITINFO_p)^ := CustomUnitInfosArray[ArrId].InfoPtrOld;
      end else
      begin
        if Local then
          PLongWord(LongWord(Pointer(unitptr))+UnitStruct_UNITINFO_p)^ := LongWord(@CustomUnitInfosArray[ArrId].InfoStruct)
        else
          PLongWord(LongWord(TAMem.getUnitStruct(unitId))+UnitStruct_UNITINFO_p)^ := LongWord(@CustomUnitInfosArray[ArrId].InfoStruct);
      end;
    end;
end;

class function TAUnit.getUnitInfoField(unitptr: Pointer; fieldType: LongWord; remoteUnitId: PLongWord): LongWord;
var
  arrId: Integer;
  unitId: LongWord;
  local: Boolean;
begin
  unitId:= IsLocal(unitptr, remoteUnitId, local);
  if TAUnit.SearchCustomUnitInfos(unitId, remoteUnitId, local, arrId) then
  begin
    case fieldType of
      MAXHEALTH : Result:= LongWord(CustomUnitInfosArray[arrId].InfoStruct.nMaxHP);
      HEALTIME  : Result:= LongWord(CustomUnitInfosArray[arrId].InfoStruct.nHealTime);
      { ... }
    end;
  end;
end;

class function TAUnit.setUnitInfoField(unitptr: Pointer; fieldType: LongWord; value: LongWord; remoteUnitId: PLongWord): LongWord;
var
  arrId: Integer;
  unitId: LongWord;
  local: Boolean;
  MovementClassStruct: PMoveInfoClassStruct;
begin
  Result:= 0;
  unitId:= IsLocal(unitptr, remoteUnitId, local);
  if TAUnit.SearchCustomUnitInfos(unitId, remoteUnitId, local, arrId) then
  begin
    case fieldType of
      MOVEMENTCLASS :
        begin
        // movement class information is stored in both - specific and global template
        // we fix both of them to make TA engine happy
        CustomUnitInfosArray[arrId].InfoStruct.lMovementClass:= TAMem.getMovementClassPtr(Word(value));
        MovementClassStruct:= Pointer(CustomUnitInfosArray[arrId].InfoStruct.lMovementClass);
        if MovementClassStruct <> nil then
          begin
          CustomUnitInfosArray[arrId].InfoStruct.nMaxWaterDepth:= MovementClassStruct.nMaxWaterDepth;
          CustomUnitInfosArray[arrId].InfoStruct.nMinWaterDepth:= MovementClassStruct.nMinWaterDepth;
          CustomUnitInfosArray[arrId].InfoStruct.cMaxSlope:= MovementClassStruct.cMaxSlope;
          CustomUnitInfosArray[arrId].InfoStruct.cMaxWaterSlope:= MovementClassStruct.cMaxWaterSlope;
          CreateMovementClass(TAMem.GetUnitStruct(Word(unitId)));
          end else
            Exit;
        end;
      MAXHEALTH : CustomUnitInfosArray[arrId].InfoStruct.nMaxHP := Word(value);// else Result:= LongWord(CustomUnitInfosArray[arrId].InfoStruct.nMaxHP);
      HEALTIME  : CustomUnitInfosArray[arrId].InfoStruct.nHealTime := Word(value);// else Result:= LongWord(CustomUnitInfosArray[arrId].InfoStruct.nHealTime);

      MAXSPEED : CustomUnitInfosArray[arrId].InfoStruct.lMaxSpeedRaw := value;
      ACCELERATION : CustomUnitInfosArray[arrId].InfoStruct.lAcceleration := value;
      BRAKERATE : CustomUnitInfosArray[arrId].InfoStruct.lBrakeRate := value;
      TURNRATE : CustomUnitInfosArray[arrId].InfoStruct.nTurnRate := Word(value);
      CRUISEALT : CustomUnitInfosArray[arrId].InfoStruct.nCruiseAlt := Word(value);
      MANEUVERLEASH : CustomUnitInfosArray[arrId].InfoStruct.nManeuverLeashLength := Word(value);
      ATTACKRUNLEN : CustomUnitInfosArray[arrId].InfoStruct.nAttackRunLength := Word(value);
      MAXSLOPE : CustomUnitInfosArray[arrId].InfoStruct.cMaxSlope := Byte(value);
      MAXWATERSLOPE : CustomUnitInfosArray[arrId].InfoStruct.cMaxWaterSlope := Byte(value);
      WATERLINE : CustomUnitInfosArray[arrId].InfoStruct.cWaterLine := Byte(value);

      TRANSPORTSIZE : CustomUnitInfosArray[arrId].InfoStruct.cTransportSize := Byte(value);
      TRANSPORTCAP : CustomUnitInfosArray[arrId].InfoStruct.cTransportCapacity := Byte(value);

      BANKSCALE : CustomUnitInfosArray[arrId].InfoStruct.lBankScale := value;
      KAMIKAZEDIST : CustomUnitInfosArray[arrId].InfoStruct.nKamikazeDistance := Word(value);
      DAMAGEMODIFIER : CustomUnitInfosArray[arrId].InfoStruct.lDamageModifier := value;

      WORKERTIME : CustomUnitInfosArray[arrId].InfoStruct.nWorkerTime := Word(value);
      BUILDDIST : CustomUnitInfosArray[arrId].InfoStruct.nBuildDistance := Word(value);

      SIGHTDIST : begin CustomUnitInfosArray[arrId].InfoStruct.nSightDistance := Word(value); UpdateUnitLOS(TAMem.GetUnitStruct(Word(unitId))); end;
      RADARDIST : CustomUnitInfosArray[arrId].InfoStruct.nRadarDistance := Word(value); // find update radar proc
      SONARDIST : CustomUnitInfosArray[arrId].InfoStruct.nSonarDistance := Word(value);
      MINCLOAKDIST : CustomUnitInfosArray[arrId].InfoStruct.nMinCloakDistance := Word(value);
      RADARDISTJAM : CustomUnitInfosArray[arrId].InfoStruct.nRadarDistanceJam := Word(value);
      SONARDISTJAM : CustomUnitInfosArray[arrId].InfoStruct.nSonarDistanceJam := Word(value);

      MAKESMETAL : CustomUnitInfosArray[arrId].InfoStruct.cMakesMetal := Byte(value);
      FENERGYMAKE : CustomUnitInfosArray[arrId].InfoStruct.fEnergyMake := value / 100;
      FMETALMAKE : CustomUnitInfosArray[arrId].InfoStruct.fMetalMake := value / 100;
      FENERGYUSE : CustomUnitInfosArray[arrId].InfoStruct.fEnergyUse := value / 100;
      FMETALUSE : CustomUnitInfosArray[arrId].InfoStruct.fMetalUse := value / 100;
      FENERGYSTOR : CustomUnitInfosArray[arrId].InfoStruct.lEnergyStorage := value;
      FMETALSTOR : CustomUnitInfosArray[arrId].InfoStruct.lEnergyStorage := value;
      FWINDGENERATOR : CustomUnitInfosArray[arrId].InfoStruct.fWindGenerator := value / 100;
      FTIDALGENERATOR : CustomUnitInfosArray[arrId].InfoStruct.fTidalGenerator := value / 100;
      FCLOAKCOST : CustomUnitInfosArray[arrId].InfoStruct.fCloakCost := value / 100;
      FCLOAKCOSTMOVE : CustomUnitInfosArray[arrId].InfoStruct.fCloakCostMoving := value / 100;
    end;
    Result:= 1;
  end;
end;

end.
