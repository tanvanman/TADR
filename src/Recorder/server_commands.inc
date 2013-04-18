function TDPlay.OnAutoPause( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if not AutoPauseAtStart then
  begin
  AutoPauseAtStart := true;
  if ImServer or
  	( (ServerPlayer <> nil) and
  	  (ServerPlayer.InternalVersion < TADemoVersion_99b3_beta1) and
      (Players[1].IsFirstPlayerWithb3) ) then
    Sendchat('Autopause enabled - At the start only the host can unpause');
  end
else
  begin
  AutoPauseAtStart := false;
  if ImServer or
  	( (ServerPlayer <> nil) and
  	  (ServerPlayer.InternalVersion < TADemoVersion_99b3_beta1) and
      (Players[1].IsFirstPlayerWithb3) ) then
    Sendchat('Autopause disabled');
  end;
end; {OnAutoPause}

function TDPlay.Onf1off( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
F1Disable := 1;
SendChat('*** '+Sender.Name+' F1 key disabled');
end;

function TDPlay.OnCmdwarp( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if (CommanderWarp = 0)  then
  begin
  CommanderWarp := 1;
  if ImServer then
    Sendchat('Cmd warping enabled');
  end
else
  begin
  CommanderWarp := 0;
  if ImServer then
    Sendchat('Cmd warping disabled');
  end;
end;

function TDPlay.OnSyncon( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  aLowerLimit : integer;
  aUpperlimit : integer;
  aTAGameSpeed : byte;
begin
Result := True;
notime := false;
Assert( params.Count >= 2 );

try
  aLowerLimit := StrToInt(params[0]) + 10;
except
  on E :  EConvertError do
    begin
    sendchat (e.message);
    exit;    
    end;
end;
try
  aUpperlimit := StrToInt(params[1]) + 10;
except
  on E :  EConvertError do
    begin
    sendchat (e.message);    
    exit;
    end;
end;
// cap the lower limit
if aLowerLimit < Game_MinSpeed then
  aLowerLimit := Game_MinSpeed
else if aLowerLimit > Game_MaxSpeed then
  aLowerLimit := Game_MaxSpeed;
// cap the upper limit
if aUpperlimit < Game_MinSpeed then
  aUpperlimit := Game_MinSpeed
else if aUpperlimit > Game_MaxSpeed then
  aUpperlimit := Game_MaxSpeed;
// final cap test
if (aUpperlimit < aLowerLimit) then
  aUpperlimit := aLowerLimit;  
SpeedHack.LowerLimit := aLowerLimit;
SpeedHack.Upperlimit := aUpperlimit;

if (SpeedHack.LowerLimit <> TA_MinSpeed) or
   (SpeedHack.Upperlimit <> TA_MaxSpeed) then
  begin
  SpeedLock := true;
  FastSpeed := SpeedHack.LowerLimit;
  SlowSpeed := SpeedHack.Upperlimit;
  end;

// if the current gamespeed is outside the caps, correct it
aTAGameSpeed := TAData.GameSpeed;
if aTAGameSpeed < SpeedHack.LowerLimit then
  SendLocal( #$19#$01+char(aTAGameSpeed), 0, true, true)
else if aTAGameSpeed > SpeedHack.Upperlimit then
  SendLocal( #$19#$01+char(aTAGameSpeed), 0, true, true);

sendchat ('Speed locked between ' + IntToStr(SpeedHack.LowerLimit-10)+ ' and '+
                                    IntToStr(SpeedHack.Upperlimit-10));
end;

function TDPlay.OnSyncOff( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
sendchat ('Sync correction disabled');
notime := True;
end;

function TDPlay.OnBase( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  FileName : String;
begin
Result := True;
if params.Count = 0 then
  initfastbase('')
else
  begin
  FileName := params[0];
  if FileExists( FileName )then
     initfastbase(FileName)
  else
     initfastbase(serverdir + FileName);
  end;
end;

function TDPlay.OnDoBase( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if ImServer and QuickBaseEnabled and Sender.givebase then
  begin
  if Sender.StartInfo.X = 0 then
    sendchat('Please build a building to mark the centre of your base')
  else
    begin
    if (Players[1].StartInfo.ID<>High(longword)) and (Players[2].StartInfo.ID<>High(Longword)) then
      begin
      Sender.givebase := false;
      dobase(Sender.ID);
      end
    else
      sendchat(Sender.name+ ' please wait for sync');
    end;
  end;
end;
