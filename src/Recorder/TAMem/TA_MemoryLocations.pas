unit TA_MemoryLocations;

interface
uses
  dplay;
const
  MAXPLAYERCOUNT = 10;

  TADynmemStructPtr  = $511de8;
  TAdynmemStruct_SharedBits = $2a42;
  TAdynmemStruct_LOS_Sight = $2a43; // byte;
  TAdynmemStruct_LOS_Type = $14281;
  TAdynmemStruct_Players = $1b63;  // 10 * PlayerStructSize
  TAdynmemStruct_GameSpeed = $38A4B; // word
  TAdynmemStruct_IsPaused = $38A51;  //

  TAdynmemStruct_Units = $14357;// pointer
  TAdynmemStruct_Units_EndMarker = $1435B; // pointer
  TAdynmemStruct_UnitCount_Unk = $14353; // pointer
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
  UnitStruct_OwnerIndex = $ff;
  UnitStruct_OwnerPtr = $96;  
  UnitStruct_Kills = $b8;
  UnitStruct_BuildTimeLeft = $104;
const
  ShiftBiuldClick_Add : PShortInt = PShortInt($41ac14);
  ShiftBiuldClick_Sub : PShortInt = PShortInt($41ac18);


// PLongword(0x4BF8C0)^ := 0xcc2 // disable TA buildrectangel, note: use WriteProcessMemory
// PLongword(0x4BF8C0)^ := 0x5368EC83 // enable TA buildrectangel, note: use WriteProcessMemory

const
  BoolValues : array [boolean] of byte = (0,1);
type
  PAlliedState = ^TAlliedState;
  TAlliedState = array [ 0..9 ] of byte;

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

    class Function getPausedState : boolean;
    class Procedure SetPausedState( value : boolean);
  public
    Property Paused : boolean read getPausedState write setPausedState;

    class Function GetUnitPtr(unitIndex : longword) : pointer;
  
    property ViewPlayer : byte read getViewPlayer;
    Property GameSpeed : byte read getGameSpeed;
    Property MaxUnitLimit : word read getMaxUnitLimit;
    Property ActualUnitLimit : word read getActualUnitLimit;
    Property IsAlteredUnitLimit : boolean read getIsAlteredUnitLimit;
    Property UnitsPtr : longword read getUnitsPtr;
    Property Units_EndMarkerPtr : longword read getUnits_EndMarkerPtr;

    class Function getPlayerByIndex(playerIndex : longword) : pointer; 
    class Function getPlayerByDPID(playerPID : TDPID) : pointer;
    // zero based player index
    class Function getPlayerIndex(playerPID : TDPID) : longword;
        
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
    class Function getBuildTimeLeft(unitptr : pointer) : single;

    class Function GetOwnerPtr(unitptr : pointer) : pointer;
    class Function GetOwnerIndex(unitptr : pointer) : integer;
  end;

var
  TAData : TAMem;
  PlayerInfo: TAPlayer;

function IsTAVersion31 : Boolean;

implementation
uses
  sysutils,
  logging,
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
  if (PByte(Plongword($511DE8)^+$37F30)^ and (1 shl 2)) = 4 then
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
result := PWord( Longword(unitptr)+UnitStruct_Kills)^
end;

class procedure TAUnit.setKills(unitptr : pointer; Kills : word);
begin
PWord( Longword(unitptr)+UnitStruct_Kills)^ := Kills;
end;

class Function TAUnit.getBuildTimeLeft(unitptr : pointer) : single;
begin
result := PSingle( Longword(unitptr)+UnitStruct_BuildTimeLeft)^
end;

class Function TAUnit.GetOwnerPtr(unitptr : pointer) : pointer;
begin
result := PPointer( Longword(unitptr)+UnitStruct_OwnerPtr)^
end;

class Function TAUnit.GetOwnerIndex(unitptr : pointer) : integer;
begin
result := PByte( Longword(unitptr)+UnitStruct_OwnerIndex)^
end;

end.
