unit TA_MemoryLocations;

interface
uses
  dplay,
  Classes, SynCommons, strutils;
const
  MAXPLAYERCOUNT = 10;

  TADynmemStructPtr  = $511de8;
  TAdynmemStruct_SharedBits = $2a42;
  TAdynmemStruct_LOS_Sight = $2a43; // byte;
  TAdynmemStruct_LOS_Type = $14281;
  TAdynmemStruct_Players = $1b63;  // 10 * PlayerStructSize
  TAdynmemStruct_GameSpeed = $38A4B; // word
  TAdynmemStruct_IsPaused = $38A51;  //

  //1439B - unit info array
  //1438F - unit info count
  TAdynmemStruct_Units = $14357;// pointer
  TAdynmemStruct_Units_EndMarker = $1435B; // pointer
  TAdynmemStruct_UnitCount_Unk = $14353; // pointer

  TAdynmemStruct_UnitInfoCount = $1438F;
  TAdynmemStruct_UnitInfoArray = $1439B; // pointer

  TAdynmemStruct_WeaponTypedefArray = $2CF3;

  TAdynmemStruct_MaxUnitLimit = $37EEC; // word
  TAdynmemStruct_ActualUnitLimit = $37EEA;  // word


  TAdynmemStruct_IsAlteredUnitLimit = $589;    // byte;

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
  UnitStruct_Kills = $B8; // word
  UnitStruct_RecentDamage = $FA; // byte, it's a count down after being hit by sth, starts from $F0
  UnitStruct_BuildTimeLeft = $104; // dword
  UnitStruct_HealthVal = $108;  // word
  UnitStruct_cIsCloaked = $10E;  // word, get is correct, set can be false positive, use unit state mask
  UnitStruct_UnitStateMask = $110; // dword

  UnitInfoStructSize = $249;
  UnitInfoStruct_movementclass = $1B6;

  WeaponTypedefStructSize = $115;
  WeaponTypedefStruct_WeaponName = $0; // array of byte 32
  WeaponTypedefStruct_ID = $10A; // byte

const
  ShiftBiuldClick_Add : PShortInt = PShortInt($41AC14);
  ShiftBiuldClick_Sub : PShortInt = PShortInt($41AC18);


// PLongword(0x4BF8C0)^ := 0xcc2 // disable TA buildrectangel, note: use WriteProcessMemory
// PLongword(0x4BF8C0)^ := 0x5368EC83 // enable TA buildrectangel, note: use WriteProcessMemory

const
  BoolValues : array [boolean] of byte = (0,1);
type
  PAlliedState = ^TAlliedState;
  TAlliedState = array [ 0..9 ] of byte;

  // 0x249
  PUnitfInfo = ^TGameUnitInfo;
  TGameUnitInfo = packed record
    szName: array [0..31] of AnsiChar;
    szUnitName: array [0..31] of AnsiChar;
    szUnitDescription: array [0..63] of AnsiChar;
    szObjectName: array [0..31] of AnsiChar;
    szSide: array [0..7] of AnsiChar;
    nUID: Word;
    Unknown1: array [0..19] of Byte;
    AIWeight: array [0..63] of Byte;
    AILimit: array [0..63] of Byte;
    Unknown2: array [0..11] of Byte;
    nFootPrintX: Word;
    nFootPrintZ: Word; 
    pYardMap: PLongWord;
    Unknown3: array [0..11] of Byte;
    lWidthX: LongWord;
    lUnknown4: LongWord;
    lWidthY: LongWord;
    lUnknown5: LongWord;
    lWidthZ: LongWord;
    lUnknown6: LongWord;
    Unknown7: array [0..15] of Byte;
    fBuildCostEnergy: single;
    fBuildCostMetal: single;
    pCOBScript: PLongWord;
    lMaxSpeedRaw: LongWord;
		lMaxSpeedSlope: LongWord;
		lBrakeRate: LongWord;
    lAcceleration: LongWord;
    lBankScale: LongWord;
    lPitchScale: LongWord;
    lDamageModifier: LongWord;
    lMoveRate1: LongWord;
    lMoveRate2: LongWord;
    lMovementClass: LongWord;
	  TurnRate: Word;
    nCorpseIndex: Word;
    nMaxWaterDepth: Word;
    nMinWaterDepth: Word;
		fEnergyMake: single;
		fEnergyUse: single;
		fMetalMake: single;
		fMetalUse: single;
		fWindGenerator: single;
		fTidalGenerator: single;
		fCloakCost: single;
		fCloakCostMoving: single;
    lEnergyStorage: LongWord;
    lMetalStorage: LongWord;
    lBuildTime: LongWord;
    p_WeaponPrimary: PLongWord;
    p_WeaponSecondary: PLongWord;
    p_WeaponTertiary: PLongWord;
		nMaxHP: Word;
    Unknown8: array [0..1] of Byte;
    nWorkerTime: Word;
    nHealTime: Word;
    nSightDistance: Word;
    nRadarDistance: Word;
    nSonarDistance: Word;
    nMinCloakDistance: Word;
    nRadarDistanceJam: Word;
    nSonarDistanceJam: Word;
    nSoundCategory: Word;
    nBuildAngle: Word;
    nBuildDistance: Word;
    nManeuverLeashLength: Word;
    nAttackRunLength: Word;
    nKamikazeDistance: Word;
    nSortBias: Word;
    nCruiseAlt: Word;
    nCategory: Word;
		p_ExplodeAs: PLongWord;
		p_SelfDestructAsAs: PLongWord;
    cMaxSlope: Byte;
    cBadSlope: Byte; 
    cTransportSize: Byte;
    cTransportCapacity: Byte;
    cWaterLine: Byte;
    cMakesMetal: Byte;
    cGUINum: Byte;
    cBMCode: Byte;
    cDefaultMissionType: Byte;
    p_WeaponPrimaryBadTargetCategoryArray: LongWord;
    p_WeaponSecondaryBadTargetCategoryArray: LongWord;
    p_WeaponSpecialBadTargetCategoryArray: LongWord;
    p_NoChaseCategoryMaskArray: LongWord;
    UnitTypeMask: LongWord;
    lUnitBitFields2: LongWord;
  end;

  PCustomUnitInfo = ^TCustomUnitInfo;
  TCustomUnitInfo = packed record
    UnitId        : LongWord;
    UnitIdRemote  : LongWord;  // remote unit long id (sender's local)
    //OwnerPlayer  : Longint; // Buffer.EventPlayer_DirectID = PlayerAryIndex2ID(UnitInfo.ThisPlayer_ID   )
    InfoPtrOld   : LongWord;  // local old pointer
    InfoStruct   : TGameUnitInfo;
  end;

  TUnitInfos = array of TCustomUnitInfo;

  TTAPlayerType = (Player_LocalHuman = 1, Player_LocalAI = 2, Player_RemotePlayer = 3);
  TAMem = class
  protected
    class Function getViewPLayer : byte;
    class Function getGameSpeed : byte;
    
    class Function getMaxUnitLimit : word;
    class Function getActualUnitLimit : word;
    class Function getIsAlteredUnitLimit : boolean;
    class function getUnitsPtr : longword;
    class function getUnits_EndMarkerPtr : longword;
    class function getUnitInfoPtr : longword;
    class function getUnitInfoCount : longword;

    class Function getPausedState : boolean;
    class Procedure SetPausedState( value : boolean);
  public
    Property Paused : boolean read getPausedState write setPausedState;

    // will return nil for units without movement class (!)
    class Function GetUnitPtr(unitIndex : longword) : pointer;
    class Function GetUnitStruct(unitIndex : longword) : LongWord;

    property ViewPlayer : byte read getViewPlayer;
    Property GameSpeed : byte read getGameSpeed;
    Property MaxUnitLimit : word read getMaxUnitLimit;
    Property ActualUnitLimit : word read getActualUnitLimit;
    Property IsAlteredUnitLimit : boolean read getIsAlteredUnitLimit;
    Property UnitsPtr : longword read getUnitsPtr;
    Property Units_EndMarkerPtr : longword read getUnits_EndMarkerPtr;
    Property UnitInfoPtr : longword read getUnitInfoPtr;
    Property UnitInfoCount : longword read getUnitInfoCount;

    class Function getPlayerByIndex(playerIndex : longword) : pointer; 
    class Function getPlayerByDPID(playerPID : TDPID) : pointer;
    // zero based player index
    class Function getPlayerIndex(playerPID : TDPID) : longword;

    class function getTemplatePtr(templateid: word): Longword;
    class function getUnitTemplateId(unitptr: pointer): Word;

    class function getMovementClassPtr(classid: word): LongWord;
    class function getWeapon(weaponid: word): Longword;

    class function IsTAVersion31 : Boolean;
  end;

  TAPlayer = class
  protected
    class function getShareEnergyVal : single;
    class function getShareMetalVal : single;
    class function getShareEnergy : boolean;
    class function getShareMetal : boolean;
    class function getShootAll : boolean;
  public
    property ShareEnergyVal : single read getShareEnergyVal;
    property ShareMetalVal : single read getShareMetalVal;
    property ShareEnergy : boolean read getShareEnergy;
    property ShareMetal : boolean read getShareMetal;
    property ShootAll : boolean read getShootAll;

    class Function getDPID(player : pointer) : TDPID;
    class Function PlayerType(player : pointer) : TTAPlayerType;

    class Procedure SetShareRadar(player : pointer; value : boolean);
    class function GetShareRadar(player : pointer) : boolean;

    class function GetIsWatcher(player : pointer) : boolean;
    class function GetIsActive(player : pointer) : boolean;

    class function GetAlliedState(Player1 : pointer; Player2 : integer) : boolean;
    class Procedure SetAlliedState(Player1 : pointer; Player2 : integer; value : boolean);
  end;

  TAUnit = class
  public
    class Function getKills(unitptr : pointer) : word;
    class procedure setKills(unitptr : pointer; Kills : word);

    class Function getHealth(unitptr : pointer) : word;
    class procedure setHealth(unitptr : pointer; Health : longword);

    class Function getCloak(unitptr : pointer) : LongWord;
    class procedure setCloak(unitptr : pointer; Cloak : word);
    class procedure setForcedCloak(unitptr : pointer; Cloak : word);

    class procedure setUnitX(unitptr : pointer; X : LongWord);
    class procedure setUnitY(unitptr : pointer; Y : LongWord);
    class procedure setUnitZ(unitptr : pointer; Z : LongWord);
    class function getUnitX(unitptr : pointer): longword;
    class function getUnitY(unitptr : pointer): longword;
    class function getUnitZ(unitptr : pointer): longword;

    class procedure Kill(unitptr : pointer; deathtype : byte);

    class function getMovementClass(unitptr : pointer): longword;

    class Function getRecentDamage(unitptr : pointer) : byte;
    class procedure setRecentDamage(unitptr : pointer);

    class Function getBuildTimeLeft(unitptr : pointer) : single;

    class Function GetOwnerPtr(unitptr : pointer) : pointer;
    class Function GetOwnerIndex(unitptr : pointer) : integer;
    class Function IsOnThisComp(unitptr : pointer) : longword;
    class Function IsLocal(UnitPtr: Pointer; RemoteUnitId: PLongWord; out Local: boolean): LongWord;

    class Function GetId(unitptr : pointer) : Word;
    class Function GetLongId(unitptr : pointer) : LongWord;
    class function SearchCustomUnitInfos(UnitId: LongWord; RemoteUnitId: PLongWord; Local: Boolean; out Index: integer ): boolean;

    class procedure setTemplate(unitptr : pointer; newTemplateID: Word);
    class procedure setMovementClass(unitptr : pointer; newmclass: Word);
    class procedure setWeapon(unitptr : pointer; index: LongWord; weaponid: Word);
    class function setUpgradeable(UnitPtr: Pointer; State: Byte; RemoteUnitId: PLongWord): ShortInt;
    class function editTemplate(UnitPtr: Pointer; fieldType: LongWord; value: LongWord; RemoteUnitId: PLongWord): ShortInt;
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
  COB_Extensions,
  INI_Options,
  TAMemManipulations;

// -----------------------------------------------------------------------------

function IsTAVersion31 : Boolean;
begin
result := TAMem.IsTAVersion31();
end; {IsTAVersion31}

var
  CacheUsed : boolean;
  IsTAVersion31_Cache : boolean;
class function TAMem.IsTAVersion31 : Boolean;
const
  Address = $4ad494;
  ExpectedData : array [0..2] of byte = (0,$55,$e8);
var
  FailIndex : integer;
  FailValue : byte;
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

class Function TAMem.getViewPLayer : byte;
begin
result := PByte(PLongword(TADynmemStructPtr)^ + TAdynmemStruct_LOS_Sight)^;
end;

class Function TAMem.getGameSpeed : byte;
begin
result := PByte(Plongword(TADynmemStructPtr)^+TAdynmemStruct_GameSpeed)^;
end;

class function TAMem.getMaxUnitLimit : word;
begin
result := PWord(Plongword(TADynmemStructPtr)^+TAdynmemStruct_MaxUnitLimit)^;
end;

class function TAMem.getActualUnitLimit : word;
begin
result := PWord(Plongword(TADynmemStructPtr)^+TAdynmemStruct_ActualUnitLimit)^;
end;

class function TAMem.getIsAlteredUnitLimit : boolean;
begin
result := PByte(Plongword(TADynmemStructPtr)^+TAdynmemStruct_IsAlteredUnitLimit)^ <> 0;
end;

class function TAMem.getUnitsPtr : longword;
begin
result := PLongword(PLongword(TADynmemStructPtr)^+TAdynmemStruct_Units)^;
end;

class function TAMem.getUnits_EndMarkerPtr : longword;
begin
result := PLongword(PLongword(TADynmemStructPtr)^+TAdynmemStruct_Units_EndMarker)^;
end;

class function TAMem.getUnitInfoPtr : longword;
begin
result := PLongword(PLongword(TADynmemStructPtr)^+TAdynmemStruct_UnitInfoArray)^;
end;

class function TAMem.getUnitInfoCount : longword;
begin
result := PLongword(PLongword(TADynmemStructPtr)^+TAdynmemStruct_UnitInfoCount)^;
end;

class Function TAMem.getPlayerByIndex(playerIndex : longword) : pointer;
begin
result := pointer( PLongword(TADynmemStructPtr)^+TAdynmemStruct_Players+(playerIndex*PlayerStructSize) );
end;

class Function TAMem.getPlayerIndex(playerPID : TDPID) : longword;
var
  aplayerPID : PDPID;
  i : integer;
begin
result := longword(-1);
aplayerPID := pointer( PLongword(TADynmemStructPtr)^+TAdynmemStruct_Players+PlayerStruct_DPID );
i := 0;
while i < MAXPLAYERCOUNT do
  begin
  if aplayerPID^ = playerPID then
    begin
    result := i;
    break;
    end;
  aplayerPID := pointer(longword(aplayerPID)+PlayerStructSize);
  inc(i);
  end;
end;

class Function TAMem.getPausedState : boolean;
begin
result := PByte(Plongword(TADynmemStructPtr)^+TAdynmemStruct_IsPaused)^ <> 0;
end;

class Procedure TAMem.SetPausedState( value : boolean);
begin
PByte(Plongword(TADynmemStructPtr)^+TAdynmemStruct_IsPaused)^ := BoolValues[value]
end;

class Function TAMem.GetUnitPtr(unitIndex : longword) : pointer;
begin
if (unitIndex > getMaxUnitLimit * 10) then exit;
result := pointer( Plongword(Plongword(TADynmemStructPtr)^+TAdynmemStruct_Units)^+UnitStructSize*unitIndex );
end;

class Function TAMem.GetUnitStruct(unitIndex : longword) : LongWord;
begin
Result:= 0;
if (unitIndex > getMaxUnitLimit * 10) then exit;
result := longword(Plongword(Plongword(TADynmemStructPtr)^+TAdynmemStruct_Units)^+UnitStructSize*unitIndex );
end;

// -----------------------------------------------------------------------------

class Function TAMem.getPlayerByDPID(playerPID : TDPID) : pointer;
var
  aplayerPID : PDPID;
  i : integer;
begin
result := nil;
aplayerPID := pointer( PLongword(TADynmemStructPtr)^+TAdynmemStruct_Players+PlayerStruct_DPID );
i := 0;
while i < MAXPLAYERCOUNT do
  begin
  if aplayerPID^ = playerPID then
    begin
    result := pointer(longword(aplayerPID) - PlayerStruct_DPID);
    break;
    end;
  aplayerPID := pointer(longword(aplayerPID)+PlayerStructSize);
  inc(i);
  end;
end;

class function TAPlayer.GetAlliedState(Player1 : pointer; Player2 : integer) : boolean;
begin
if (Player1 = nil) or (longword(Player2) >= MAXPLAYERCOUNT) then exit;
result := PAlliedState(longword(Player1)+PlayerStruct_AlliedPlayers)[Player2] <> 0;
end;

class Procedure TAPlayer.SetAlliedState(Player1 : pointer; Player2 : integer; value : boolean);
begin
if (Player1 = nil) or (longword(Player2) >= MAXPLAYERCOUNT) then exit;
PAlliedState(longword(Player1)+PlayerStruct_AlliedPlayers)[Player2] := BoolValues[value]
end;

class Function TAPlayer.getDPID(player : pointer) : TDPID;
begin
result := PDPID(Longword(player)+ PlayerStruct_DPID )^;
end;

class Function TAPlayer.PlayerType(player : pointer) : TTAPlayerType;
begin
result := TTAPlayerType(PByte(Longword(player)+ PlayerStruct_PlayerType )^);
end;

Class function TAPlayer.GetIsActive(player : pointer) : boolean;
begin
result := PLongword(player)^ <> $0;
end;

Class function TAPlayer.GetIsWatcher(player : pointer) : boolean;
var
  Bitfield : PWord;
begin
Bitfield := PWord(PLongword(Longword(player)+PlayerStruct_PlayerInfo)^+PlayerInfoStruct_IsWatching);
result := Bitfield^ <> $40;
end;

const
  ShareRadar_BitMask = $40;
Class Procedure TAPlayer.SetShareRadar(player : pointer; value : boolean);
var
  Bitfield : PWord;
begin
Bitfield := PWord(PLongword(Longword(player)+PlayerStruct_PlayerInfo)^+PlayerInfoStruct_SharedBits);
if value then
  Bitfield^ := Bitfield^ or ShareRadar_BitMask
else
  Bitfield^ := Bitfield^ and not ShareRadar_BitMask
end;

Class function TAPlayer.GetShareRadar(player : pointer) : boolean;
var
  Bitfield : PByte;
begin
Bitfield := PByte(PLongword(Longword(player)+PlayerStruct_PlayerInfo)^+PlayerInfoStruct_IsWatching);
result := (Bitfield^ and $40) = $40;
end;

Class function TAPlayer.GetShareEnergyVal : single;
begin
result := PSingle( PLongword(TADynmemStructPtr)^+TAdynmemStruct_Players+$E8 )^;
end;

Class function TAPlayer.GetShareMetalVal : single;
begin
result := PSingle( PLongword(TADynmemStructPtr)^+TAdynmemStruct_Players+$E4 )^;
end;

Class function TAPlayer.GetShareEnergy : boolean;
var
  Bitfield : PWord;
begin
Bitfield := PWord(PLongword(PLongword(TADynmemStructPtr)^+TAdynmemStruct_Players+PlayerStruct_PlayerInfo)^+PlayerInfoStruct_SharedBits);
result := (Bitfield^ and $4) = $4;
end;

Class function TAPlayer.GetShareMetal : boolean;
var
  Bitfield : PWord;
begin
Bitfield := PWord(PLongword(PLongword(TADynmemStructPtr)^+TAdynmemStruct_Players+PlayerStruct_PlayerInfo)^+PlayerInfoStruct_SharedBits);
result := (Bitfield^ and $2) = $2;
end;

Class function TAPlayer.GetShootAll : boolean;
begin
  if (PByte(Plongword(TADynmemStructPtr)^+$37F30)^ and (1 shl 2)) = 4 then
    Result:= True
  else
    Result:= False;
end;

{
  edi - player

  xor eax,eax
  mov ecx, [edi+PlayerStruct_PlayerInfo]
  mov ax, word ptr [ecx+PlayerInfoStruct_SharedBits]
  mov edx, eax
  xor dl, al
  and edx, ShareRadar_BitMask
  xor edx, eax
  mov word ptr [ecx+PlayerInfoStruct_SharedBits], dx
}

// -----------------------------------------------------------------------------

class Function TAUnit.getKills(unitptr : pointer) : word;
begin
result := PWord( Longword(unitptr)+UnitStruct_Kills)^;
end;

class procedure TAUnit.setKills(unitptr : pointer; Kills : word);
begin
PWord( Longword(unitptr)+UnitStruct_Kills)^ := Kills;
end;

const
  Kill_BitMask = $40;
class procedure TAUnit.Kill(unitptr : pointer; deathtype : byte);
var
  Bitfield : PByte;
begin
Bitfield := PByte(Longword(unitptr)+UnitStruct_UnitStateMask+$1);
if deathtype > 0 then
  Bitfield^ := Bitfield^ or Kill_BitMask
else
  Bitfield^ := Bitfield^ and not Kill_BitMask
end;

class Function TAUnit.getHealth(unitptr : pointer) : word;
begin
result := PWord( Longword(unitptr)+UnitStruct_HealthVal)^;
end;

class procedure TAUnit.setHealth(unitptr : pointer; Health : longword);
begin
PLongWord( Longword(unitptr)+UnitStruct_HealthVal)^ := Health;
end;

class procedure TAUnit.setUnitX(unitptr : pointer; X : LongWord);
begin
PLongWord( Longword(unitptr)+UnitStruct_Pos+UnitStruct_Posx_)^ := LongWord(X*163840);
end;

class procedure TAUnit.setUnitY(unitptr : pointer; Y : LongWord);
begin
PLongWord( Longword(unitptr)+UnitStruct_Pos+UnitStruct_Posy_)^ := LongWord(Y*163840);
end;

class procedure TAUnit.setUnitZ(unitptr : pointer; Z : LongWord);
begin
PLongWord( Longword(unitptr)+UnitStruct_Pos+UnitStruct_Posz_)^ := LongWord(Z*163840);
end;

class function TAUnit.getUnitX(unitptr : pointer): longword;
begin
result := PLongWord( Longword(unitptr)+UnitStruct_Pos+UnitStruct_Posx_)^;
end;

class function TAUnit.getUnitY(unitptr : pointer): longword;
begin
result := PLongWord( Longword(unitptr)+UnitStruct_Pos+UnitStruct_Posy_)^;
end;

class function TAUnit.getUnitZ(unitptr : pointer): longword;
begin
result := PLongWord( Longword(unitptr)+UnitStruct_Pos+UnitStruct_Posz_)^;
end;

class function TAUnit.getMovementClass(unitptr : pointer): longword;
begin
result := PLongWord( Longword(unitptr)+UnitStruct_MoveClass)^;
end;

const
  Cloak_BitMask = $4;
  CloakUnitStateMask_BitMask = $8;
class procedure TAUnit.setCloak(unitptr : pointer; Cloak : word);
var
  Bitfield : PWord;
begin
  Bitfield := PWord(Longword(unitptr)+UnitStruct_UnitStateMask+$1);
  if Cloak = 1 then
    Bitfield^ := Bitfield^ or CloakUnitStateMask_BitMask
  else
    Bitfield^ := Bitfield^ and not CloakUnitStateMask_BitMask;
end;

class procedure TAUnit.setForcedCloak(unitptr : pointer; Cloak : word);
var
  Bitfield : PWord;
  Bitfield2 : PWord;
begin
  Bitfield := PWord(Longword(unitptr)+UnitStruct_UnitStateMask+$1);
  Bitfield2 := PWord(Longword(unitptr)+UnitStruct_cIsCloaked);
  if Cloak = 1 then
  begin
    Bitfield^ := Bitfield^ or CloakUnitStateMask_BitMask;
    Bitfield2^ := Bitfield2^ or Cloak_BitMask;
  end else
  begin
    Bitfield^ := Bitfield^ and not CloakUnitStateMask_BitMask;
    Bitfield2^ := Bitfield2^ and not Cloak_BitMask;
  end;
end;

Class function TAUnit.getCloak(unitptr : pointer) : LongWord;
var
  Bitfield : PWord;
begin
  Bitfield := PWord(Longword(unitptr)+UnitStruct_cIsCloaked);
  if (Bitfield^ and Cloak_BitMask) = Cloak_BitMask then
    Result:= 1
  else
    Result:= 0;
end;

class Function TAUnit.getRecentDamage(unitptr : pointer) : byte;
begin
result := PByte( Longword(unitptr)+UnitStruct_RecentDamage)^;
end;

class procedure TAUnit.setRecentDamage(unitptr : pointer);
begin
PByte( Longword(unitptr)+UnitStruct_RecentDamage)^ := $0;
end;

class Function TAUnit.getBuildTimeLeft(unitptr : pointer) : single;
begin
result := PSingle( Longword(unitptr)+UnitStruct_BuildTimeLeft)^;
end;

class Function TAUnit.GetOwnerPtr(unitptr : pointer) : pointer;
begin
result := PPointer( Longword(unitptr)+UnitStruct_OwnerPtr)^;
end;

class Function TAUnit.GetOwnerIndex(unitptr : pointer) : integer;
begin
result := PByte( Longword(unitptr)+UnitStruct_OwnerIndex)^;
end;

class Function TAUnit.IsOnThisComp(unitptr : pointer) : longword;
var
  playerPtr : pointer;
  TAPlayerType : TTAPlayerType;
begin
try
  playerPtr := TAUnit.GetOwnerPtr(pointer(UnitPtr));
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

class Function TAUnit.IsLocal(UnitPtr: Pointer; RemoteUnitId: PLongWord; out Local: boolean): LongWord;
begin
  if RemoteUnitId <> nil then
  begin
    Local:= False;
    Result:= Word(RemoteUnitId);
  end else
  begin
    if UnitPtr <> nil then
    begin
      Local:= (IsOnThisComp(Pointer(UnitPtr)) = 1);
      Result:= TAUnit.GetLongId(Pointer(UnitPtr));
    end;
  end;
end;

class Function TAUnit.GetId(unitptr : pointer) : Word;
begin
result := PWord( Longword(unitptr)+UnitStruct_UnitInGameIndex)^;
end;

class Function TAUnit.GetLongId(unitptr : pointer) : LongWord;
begin
result := PLongWord( Longword(unitptr)+UnitStruct_UnitInGameIndex)^;
end;

class function TAMem.getTemplatePtr(templateid: word): LongWord;
begin
result := getUnitInfoPtr + templateid * UnitInfoStructSize;
end;

class function TAMem.getUnitTemplateId(unitptr: pointer): Word;
begin
result := PWord( Longword(unitptr)+UnitStruct_UnitINFOID)^;
end;

class function TAMem.getMovementClassPtr(classid: word): LongWord;
begin
result := $512358 + 32 * classid;
end;

class function TAMem.getWeapon(weaponid: word): LongWord;
begin
// fix me to be compatible with xpoy id patch
result:= PLongword(TADynmemStructPtr)^+TAdynmemStruct_WeaponTypedefArray + weaponid * WeaponTypedefStructSize;
end;

class procedure TAUnit.setTemplate(unitptr : pointer; newTemplateID: Word);
var
  newTemplate: LongWord;
  ResultPtr: LongWord;
begin
  if unitptr <> nil then
  begin
    newTemplate:= TAMem.getTemplatePtr(newTemplateID);
    PWord(Longword(unitptr)+UnitStruct_UNITINFOID)^ := newTemplateID;
    PLongWord(Longword(unitptr)+UnitStruct_UNITINFO_p)^ := newTemplate;
    ResultPtr:= CreateMovementClass(Longword(unitptr));
  end;
end;

{
  Rebuild movement class structure
}
class procedure TAUnit.setMovementClass(unitptr : pointer; newmclass: Word);
var
  newClassAddr: LongWord;
  orgGlobalTemplate: LongWord;
  ResultPtr: LongWord;
  newUnitInfoStruct: TGameUnitInfo;
begin
  if unitptr <> nil then
  begin
    // get new movement class address
    newClassAddr:= TAMem.getMovementClassPtr(newmclass);
    // get original template pointer
    orgGlobalTemplate:= PLongWord(Longword(unitptr)+UnitStruct_UNITINFO_p)^;
    // assign template to temp struct
    newUnitInfoStruct:= PUnitfInfo(orgGlobalTemplate)^;
    // assign new movement class to temp structure
    newUnitInfoStruct.lMovementClass:= newClassAddr;
    // temp fix, will have to parse movement class
    newUnitInfoStruct.cWaterLine:= 0;

    // write our UNITINFO_p pointer to temp structure
    PLongWord(Longword(unitptr)+UnitStruct_UNITINFO_p)^ := LongWord(Addr(newUnitInfoStruct));
    // tell TA to rebuild movement class structure based on new movement class and temp UnitInfo structure
    ResultPtr:= CreateMovementClass(Longword(unitptr));
    // bring back the orginal UNITINFO_p pointer
    PLongWord(Longword(unitptr)+UnitStruct_UNITINFO_p)^ := LongWord(orgGlobalTemplate);
  end;
end;

class procedure TAUnit.setWeapon(unitptr : pointer; index: LongWord; weaponid: Word);
var
  Weapon: Longword;
begin
  if unitptr <> nil then
  begin
    Weapon:= TAMem.getWeapon(weaponid);
      case index of
        WEAPON1: PLongWord(Longword(unitptr)+UnitStruct_Weapon1_p)^ := Weapon;
        WEAPON2: PLongWord(Longword(unitptr)+UnitStruct_Weapon2_p)^ := Weapon;
        WEAPON3: PLongWord(Longword(unitptr)+UnitStruct_Weapon3_p)^ := Weapon;
      end;
  end;
end;

{
  Searches non-sorted array of custom unit info structures.
  we can't delete any elements of array (would move pointers and TA would crash),
}
class function TAUnit.SearchCustomUnitInfos(UnitId: LongWord; RemoteUnitId: PLongWord; Local: Boolean; out Index: integer ): boolean;
var
  i: integer;
  TmpUnitInfo: PCustomUnitInfo;
begin
  Result:= False;
  Index:= -1;
  for i := CustomUnitInfos.Count-1 downto 0 do
  begin
    TmpUnitInfo:= @CustomUnitInfosArray[i];
    if TmpUnitInfo^.UnitId = UnitId then
    begin
      if Local then
      begin
        Index:= i;
        Result:= True;
        Exit;
      end else
        if LongWord(TmpUnitInfo^.UnitIdRemote) = LongWord(RemoteUnitId) then
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
class function TAUnit.setUpgradeable(UnitPtr: Pointer; State: Byte; RemoteUnitId: PLongWord): ShortInt;
var
  TmpUnitInfo: TCustomUnitInfo;
  ArrId: integer;
  UnitId: LongWord;
  Local: Boolean;
  TemplateFound: Boolean;
begin
  Result:= -1;
  UnitId:= IsLocal(UnitPtr,RemoteUnitId, Local);
  TemplateFound:= TAUnit.SearchCustomUnitInfos(UnitId, RemoteUnitId, Local, ArrId);

  if (not TemplateFound) and (ArrId = -1) then
  begin
    TmpUnitInfo.UnitId:= UnitId;
    if Local then
      TmpUnitInfo.InfoPtrOld:= PLongWord(Longword(Pointer(unitptr))+UnitStruct_UNITINFO_p)^
    else
      begin
        TmpUnitInfo.UnitIdRemote:= LongWord(RemoteUnitId);
        TmpUnitInfo.InfoPtrOld:= PLongWord(Longword(TAMem.getUnitStruct(UnitId))+UnitStruct_UNITINFO_p)^;
      end;
    TmpUnitInfo.InfoStruct:= PUnitfInfo(TmpUnitInfo.InfoPtrOld)^;
//testing
StrPLCopy(TmpUnitInfo.InfoStruct.szName, 'It''s me - Mario!', High(TmpUnitInfo.InfoStruct.szName));
TmpUnitInfo.InfoStruct.fEnergyMake:= 2000;
TmpUnitInfo.InfoStruct.cWaterLine:= 0;


    ArrId:= CustomUnitInfos.Add(TmpUnitInfo);
    TemplateFound:= True;
  end else
    if (not Local) then
    begin
     // if state = 1 then
    //  begin
        // received remote setupgradeable but unit with the same short id already exists in array,
        // let's overwrite array element with new data, so we preserve pointers to other elements hopefully not crashing TA
        CustomUnitInfosArray[ArrId].UnitIdRemote:= LongWord(RemoteUnitId);
        CustomUnitInfosArray[ArrId].InfoPtrOld:= TAMem.getTemplatePtr(PWord(Longword(TAMem.getUnitStruct(Word(RemoteUnitId)))+UnitStruct_UnitINFOID)^);
        CustomUnitInfosArray[ArrId].InfoStruct:= PUnitfInfo(CustomUnitInfosArray[ArrId].InfoPtrOld)^;
//testing
StrPLCopy(CustomUnitInfosArray[ArrId].InfoStruct.szName, 'It''s me - Mario!', High(CustomUnitInfosArray[ArrId].InfoStruct.szName));
CustomUnitInfosArray[ArrId].InfoStruct.fEnergyMake:= 2000;
//i and $FFF7FFFF or (1 shl 19))
        TemplateFound:= True;
    //  end;
    end;

    if TemplateFound then
    begin
      Result:= 1;
      if state = 0 then
      begin
        if Local then
          PLongWord(Longword(Pointer(unitptr))+UnitStruct_UNITINFO_p)^ := CustomUnitInfosArray[ArrId].InfoPtrOld
          //PLongWord(Longword(UnitPtr)+UnitStruct_UNITINFO_p)^ := CustomUnitInfosArray[ArrId].InfoPtrOld
        else
          PLongWord(Longword(TAMem.getUnitStruct(UnitId))+UnitStruct_UNITINFO_p)^ := CustomUnitInfosArray[ArrId].InfoPtrOld;
      end else
      begin
        if Local then
          PLongWord(Longword(Pointer(unitptr))+UnitStruct_UNITINFO_p)^ := LongWord(@CustomUnitInfosArray[ArrId].InfoStruct)
          //PLongWord(Longword(UnitPtr)+UnitStruct_UNITINFO_p)^ := LongWord(@CustomUnitInfosArray[ArrId].InfoStruct)
        else
          PLongWord(Longword(TAMem.getUnitStruct(UnitId))+UnitStruct_UNITINFO_p)^ := LongWord(@CustomUnitInfosArray[ArrId].InfoStruct);
      end;
    end;

end;

class function TAUnit.editTemplate(UnitPtr: Pointer; fieldType: LongWord; value: LongWord; RemoteUnitId: PLongWord): ShortInt;
begin

end;

end.
