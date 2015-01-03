unit TA_MemPlayers;

interface
uses
  dplay, TA_MemoryStructures;

type
  TAPlayer = class
  public
    class Function GetDPID(Player: PPlayerStruct) : TDPID;
    class Function GetPlayerByIndex(playerIndex: Byte) : PPlayerStruct;
    class Function GetPlayerPtrByDPID(playerPID: TDPID) : PPlayerStruct;
    class Function GetPlayerByDPID(playerPID: TDPID) : Byte;

    class function PlayerIndex(Player: PPlayerStruct) : Byte;
    class Function PlayerController(Player: PPlayerStruct) : TTAPlayerController;
    class Function PlayerSide(Player: PPlayerStruct) : TTAPlayerSide;
    class function PlayerLogoIndex(Player: PPlayerStruct) : Byte;

    class function GetShareRadar(Player: PPlayerStruct) : Boolean;
    class Procedure SetShareRadar(Player: PPlayerStruct; ANewState: Boolean);

    class function IsKilled(Player: PPlayerStruct) : Boolean;
    class function IsActive(Player: PPlayerStruct) : Boolean;

    class function GetAlliedState(Player1: PPlayerStruct; Player2: Byte) : Boolean;
    class Procedure SetAlliedState(Player1: PPlayerStruct; Player2: Byte; ANewState: Boolean);
  end;

implementation
uses
  TA_MemoryConstants,
  TA_MemoryLocations;

// -----------------------------------------------------------------------------
// TAPlayer
// -----------------------------------------------------------------------------

class Function TAPlayer.GetDPID(Player: PPlayerStruct) : TDPID;
begin
  Result := Player.lDirectPlayID;
end;

class Function TAPlayer.GetPlayerByIndex(playerIndex : Byte) : PPlayerStruct;
begin
  Result := nil;
  if playerindex <= MAXPLAYERCOUNT then
    Result := @TAData.MainStruct.Players[playerIndex];
end;

class Function TAPlayer.GetPlayerPtrByDPID(playerPID : TDPID) : PPlayerStruct;
var
  i : Integer;
begin
  Result := nil;
  for i := 0 to 9 do
  begin
    if TAData.MainStruct.Players[i].lDirectPlayID = playerPID then
    begin
      Result := @TAData.MainStruct.Players[i];
      Break;
    end;
  end;
end;

class Function TAPlayer.GetPlayerByDPID(playerPID : TDPID) : Byte;
var
  i : Integer;
begin
  Result := 10;
  for i := 0 to 9 do
  begin
    if TAData.MainStruct.Players[i].lDirectPlayID = playerPID then
    begin
      Result := i + 1;
      Break;
    end;
  end;
end;

class Function TAPlayer.PlayerIndex(Player: PPlayerStruct) : Byte;
begin
  Result := Player.cPlayerIndex;
end;

class Function TAPlayer.PlayerController(Player: PPlayerStruct) : TTAPlayerController;
begin
  Result := Player.cPlayerController;
end;

class Function TAPlayer.PlayerSide(Player: PPlayerStruct) : TTAPlayerSide;
begin
  Result := TTAPlayerSide(Player.PlayerInfo.Raceside);
end;

class Function TAPlayer.PlayerLogoIndex(Player: PPlayerStruct) : Byte;
begin
  Result := Player.PlayerInfo.PlayerLogoColor;
end;

Class function TAPlayer.GetShareRadar(Player: PPlayerStruct) : Boolean;
begin
  Result := SharedState_SharedRadar in Player.PlayerInfo.SharedBits;
end;

Class Procedure TAPlayer.SetShareRadar(Player: PPlayerStruct; ANewState: Boolean);
begin
  if ANewState then
    Include(PPlayerStruct(Player).PlayerInfo.SharedBits, SharedState_SharedRadar)
  else
    Exclude(PPlayerStruct(Player).PlayerInfo.SharedBits, SharedState_SharedRadar);
end;

Class function TAPlayer.IsKilled(Player: PPlayerStruct) : Boolean;
begin
  Result := Player.PlayerInfo.PropertyMask and $40 = $40;
end;

Class function TAPlayer.IsActive(Player: PPlayerStruct) : Boolean;
begin
  Result := Player.lPlayerActive <> 0;
end;

class function TAPlayer.GetAlliedState(Player1: PPlayerStruct; Player2: Byte) : Boolean;
begin
  Result := False;
  if (Player1 = nil) or
     (Player2 >= MAXPLAYERCOUNT) then
    Exit;
  Result := Player1.cAllyFlagArray[Player2] <> 0;
end;

class Procedure TAPlayer.SetAlliedState(Player1: PPlayerStruct; Player2: Byte; ANewState: Boolean);
begin
  if (Player1 = nil) or
     (Player2 >= MAXPLAYERCOUNT) then
    Exit;
  Player1.cAllyFlagArray[Player2] := BoolValues[ANewState];
end;

end.
