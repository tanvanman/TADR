function TDPlay.OnShareLos( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if not Sender.Uses_Rec2Rec_Notification then
  begin
  Sender.SharingLos := not Sender.SharingLos;
  end
else if Sender.IsSelf then
  begin
  Sender.SharingLos := not Sender.SharingLos;
  // emit the sharelos change to other players
  if Sender.SharingLos then
    begin
    SendChatLocal( 'Toggled ShareLos to: ON');
    SendRecorderToRecorderMsg( TANM_Rec2Rec_Sharelos, #$01 );
    end
  else
    begin
    SendChatLocal( 'Toggled ShareLos to: OFF');
    SendRecorderToRecorderMsg( TANM_Rec2Rec_Sharelos, #$00 );
    end;
  end;
end;

function TDPlay.OnFixDisabled( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
FixOn := false;
SendChat('*** '+Players[1].Name+' All fixes disabled');
Result := True;
end;

function TDPlay.OnFixEnabled( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
FixOn := true;
SendChat('*** '+Players[1].Name+' All fixes enabled');
Result := True;
end;

function TDPlay.OnEhaoff( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
if chatview <> nil then
  chatview^.ehaoff := 1;
SendChat('*** '+Players[1].Name+' eHa stuff disabled');
Result := True;
end;

function TDPlay.OnEhaOn( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
if chatview <> nil then
  chatview^.ehaoff := 0;
SendChat('*** '+Players[1].Name+' eHa stuff enabled');
Result := True;
end;

function TDPlay.OnTaHookOff( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
if chatview <> nil then
  chatview^.ehaoff:=2;
SendChat('*** '+Players[1].Name+' tahook disabled');
Result := True;
end;

function TDPlay.OnColors( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  s : string;
  I : Integer;
begin
Result := True;
if chatview = nil then Exit;
s:=Players[1].Name+' ';
for i := 1 to Players.Count do
  s:=s+char(chatview^.playercolors[i]+integer('0'))+' ';
SendChat(s);
end;

function TDPlay.Onehareport( const Command : string;  Sender : TPlayerData; params : TStringList ) : Boolean;
var i : Integer;
begin
Result := True;
if (params.Count > 0) and not (params.find(Players[1].Name,i)) then
  Exit;
if(assigned(chatview)) and (chatview^.ehaWarning <> 0) then
  SendChat('*** '+Players[1].Name + ' Eha status '+inttostr(chatview^.ehaWarning))
else
  SendChat('*** '+Players[1].Name + ' no Eha programs detected')
end;

function TDPlay.OnPlayers( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  i : Integer;
  playerlist : TList;
  PlayerNames : string;
  player: TPlayerData;
begin
Result := True;
if (params.Count > 0) and not (params.find( lowercase( Players[1].Name ),i)) then
  Exit;
PlayerNames := '';
playerlist := TList.Create;
try
  for i := 1 to Players.Count do
    playerlist.Add (Players[i]);
  playerlist.Sort(compare);
  for i := 1 to Players.Count do
    begin
    player := TPlayerData(playerlist.Items[i - 1]);
    PlayerNames := PlayerNames + player.Name + '('+inttostr(TAMem.getPlayerIndex(player.ID) )+')';
    if i <> Players.Count then
      PlayerNames := PlayerNames + ', ';
    end;
finally
  playerlist.Free;
end;
SendChat('*** '+Players[1].Name+' sees ' + PlayerNames);
end; {OnPlayers}

function  DirectPlay_PlayerPoll( dpId: TDPID;
                                 dwPlayerType: DWORD;
                                 const lpName: TDPName;
                                 dwFlags: DWORD;
                                 lpContext: Pointer) : BOOL; stdcall;
var
  playerlist : TStringList;
begin
Result := True;
if dwPlayerType = DPPLAYERTYPE_PLAYER then
  begin
  assert( lpContext <> nil );
  playerlist := lpContext;
  playerlist.add( lpName.lpszLongName );
  end;
end; {DirectPlay_PlayerPoll}

function TDPlay.Onpolldp( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  res : HRESULT;
  playerlist : TStringList;
  PlayerNames : string;
  i : Integer;
begin
Result := True;
if (params.Count > 0) and not (params.find(Players[1].Name,i)) then
  Exit;
playerlist := TStringList.Create;
try
  res := dp3.EnumPlayers( nil, @DirectPlay_PlayerPoll, playerlist, DPENUMPLAYERS_GROUP );
  if (res = DP_OK) then
    begin
    PlayerNames := '';
    for i := 0 to playerlist.Count-1 do
      begin
      PlayerNames := PlayerNames + playerlist[ i ];
      if i <> playerlist.Count-1 then
        PlayerNames := PlayerNames + ', ';
      end;
    SendChat('*** '+Players[1].Name+' Directplay layer sees ' + PlayerNames)
    end
  else
    SendChat('*** '+Players[1].Name+', ' + ErrorString(res) );
finally
  playerlist.Free;
end;
end; {OnDPPlayers}

function TDPlay.OnReport( const Command : string;  Sender : TPlayerData; params : TStringList ) : Boolean;
var i : Integer;
begin
Result := True;
if (params.Count > 0) and not (params.find(Players[1].Name,i)) then
  Exit;
SendChat('*** '+Players[1].Name+' uses TA Unofficial Patch ' + GetTADemoVersion + ' ('+getRecorderStatusString+')');
{
if imserver then
   SendChat ('imserver ' + inttostr(servernumber))
else
  SendChat ('imnotserver ' + inttostr (servernumber) + ' ' + mapname + ' ' + Players[servernumber].Name);
}
end;

function TDPlay.OnDate( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var i : Integer;
begin
Result := True;
if (params.Count > 0) and not (params.find(Players[1].Name,i)) then
  Exit;
SendChat('*** Current time/date for '+Players[1].Name +' is '+ DateTimeToStr(Now));
end;

function TDPlay.OnVoteReady( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  i, NotVotedGoCount : Integer;
begin
Result := True;
if Sender.UnpauseReady then
  begin
  Sender.UnpauseReady := False;
  if Sender.IsSelf then
    SendChat( '*** '+Sender.Name +' not ready' );
  end
else
  begin
  Sender.UnpauseReady := true;
  if Sender.IsSelf then
    SendChat( '*** '+Sender.Name +' ready' );

  NotVotedGoCount := 0;
  for i :=1 to Players.Count do
    if (not Players[i].UnpauseReady) and
       ( Players[i].UnitsAliveCount > 0) and
       ((Players[i].Side =0) or (Players[i].Side=1)) then
    inc( NotVotedGoCount );
  // unpause if everyone has voted to go
  if NotVotedGoCount = 0 then
    begin
    AutoPauseAtStart := False;
    SendLocal( #$19#$00#$00, 0, True, True);
    end;
  end;
end; {OnVoteReady}

{function TDPlay.OnVoteMap( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  i : Integer;
begin
Result := True;
// check if map name is provided and Sender is a player (not watcher)
if (params.Count = 0) or not
   ((Sender.Side = 0) or (Sender.Side = 1)) then
 Exit;

// only host can change map anyway so, only host will handle map voting commands
 If ImServer then
   begin
     // todo : check anti flood
     // check if other vote is in progress
     if VoteInProgress then
       begin
         SendChat('Other vote is in progress.');
         Exit;
       end;

     // todo : check params if map actually exists

     VoteInProgress:= True;
     
     // create 'can participate in vote' list
     for i := 1 to Players.Count do
      begin
       // watchers will be not allowed to participate
       if ((Players[i].Side = 0) or (Players[i].Side = 1)) then
        Players[i].CanParticipateInVote:= True else Players[i].CanParticipateInVote:= False;
      end;

     // todo : vote accepted, send start packet.
     // todo : Enable GUI elemnts: vote results for watchers, results + yes/no for CanParticipate players
     // todo : start expire timer
     VoteTimer.SetExpire(10);
     SendChat('Voting started');

   end;
end; }

function TDPlay.OnVoteGo( const Command : string;  Sender : TPlayerData; params : TStringList ) : Boolean;
var
  NotVotedGoCount : Integer;
  i : Integer;
begin
Result := True;
Sender.VotedGo := true;
NotVotedGoCount := 0;
for i :=1 to Players.Count do
  if (not Players[i].VotedGo or not not Players[i].ClickedIn ) and
     ((Players[i].Side =0) or (Players[i].Side=1)) then
    inc( NotVotedGoCount );
ForceGo := NotVotedGoCount = 0;
end;

function TDPlay.OnGive( const Command : string;  Sender : TPlayerData; params : TStringList ) : Boolean;
var
  i : Integer;
begin
Result := True;
if params.find( Players[1].Name, i ) then
  begin
  Sender.cantake := True;
  SendChat(Players[1].Name+' ready to take units from '+ Sender.Name);
  if Sender.IsAllied then
    SendChat ('You don''t really need to type .give.. allying is enough');
  end;
end;

function TDPlay.OnStopGive( const Command : string;  Sender : TPlayerData; params : TStringList ) : Boolean;
var
  i : Integer;
begin
Result := True;
if params.find( Players[1].Name, i ) then
  begin
  Sender.cantake := false;
  SendChat(Players[1].Name+' barred from taking units from '+ Sender.Name);
  end;
end;

function TDPlay.OnUnits( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  i : longword;
  deity : string;
  tmp : string;
begin
Result := True;
if Sender.IsSelf then
  begin
  if NotViewingRecording then
    begin
    SendChat('Sorry ' + Players[1].Name + ' -  .units is considered cheating');
    datachanged := true;
    end
  else
    for i:=1 to Players.Count do
      SendChat(Players[i].Name+' '+IntToStr(Players[i].UnitsAliveCount));
  end
else if (Sender.InternalVersion <= 2) then
  begin //Punish people who try to take advantage of .units
  deity := Deities[random (numdeities) + 1];
  SendChat (Sender.Name + ' will now feel the wrath of ' + deity + '!');
  killunits := '';
  assert(maxunits >= 10);
  for i := 2 to maxunits - 10 do
    begin
    if random(2) = 0 then
      begin
      tmp := #$0B#$F6#$01#$00#$00#$18#$79#$92#$01;
      setword (@tmp [2], i + 1 + Sender.StartInfo.ID);
  //          tmp := #$0C#$F6#$01#$00#$00#$18#$79#$92#$01#$01#$01;
  //          setword (@tmp [2], a + 1 + Sender.StartInfo.ID);
       killunits := killunits + tmp;
       end;
    end;

  SendLocal (killunits, 0, false, true);
  end;
end;

function TDPlay.OnTake( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  i  : integer;
  TimeStamp : Longword;
  tmp : string;
begin
Result := True;
if Sender.IsSelf then
  begin
  TimeStamp := timeGetTime-30000;    
  for i :=1 to Players.Count do
    begin
    if Players[i].CanTake and
       (Players[i].StartInfo.ID <> High(Longword)) and
       (Players[i].LastMsgTimeStamp < TimeStamp) and
       (TakeStatus = NoOneTaking) then
      begin
      TakeStatus := SelfTaking;
      Players[i].CanTake := False;
      TakePlayer := Players[i].Name;
      inc( TakeRef );
      if (Command = 'takecmd') or (Sender.UnitsAliveCount <= 0) then
        Players[i].TakeUnit := 1
      else
        Players[i].TakeUnit := 2;
      SendChat( Players[1].Name +' taking '+Players[i].Name+'s units');
      end;
    end
  end
else if not Sender.IsSelf then
  begin
  if TakeStatus = SelfTaking then
    begin
    TakeStatus := OtherPlayerTaking;
    SendChat(Players[1].Name +' aborting take claim');
    end;
  TakePlayer := Sender.Name;
  TakeRef := 0;
  if Sender.InternalVersion >= TADemoVersion_99b3_beta2 then
    TakeStatus := OtherPlayerTaking
  else if Sender.InternalVersion >= TADemoVersion_99b2 then
    TakeStatus := OtherPlayerTaking_OldVersion_99b2
  else
    TakeStatus := OtherPlayerTaking_OldVersion_Pre99b2;
  if not Sender.EnemyChat then
    begin
    // new recorders knows not to give away units that isn't ready
    // related to the .take bug were unfinished units were given and become finished instantly
    killunits := '';
    //  assert( maxunits >= 0);
    for i := 0 to maxunits do
      begin
      //#$0B#$F6#$01#$DD#$05#$3C#$00#$92#$01# commander firing
      if UnitStatus[Longword(i) + Sender.StartInfo.ID].donestatus > 255 then
        begin
        tmp := #$0B#$F6#$01#$00#$00#$18#$79#$92#$01;
        setword (@tmp [2], Longword(i) + 1 + Sender.StartInfo.ID);
        killunits := killunits + tmp;
        end;
      end;
    //  sendlocal (killunits, 0, true, false);
    end;
  end;
end;

function TDPlay.OnKill( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
sendlocal (killunits, 0, true, false);
end;

function TDPlay.OnHookreport( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var i : integer;
begin
Result := True;
if (params.Count > 0) and not (params.find(Players[1].Name,i)) then
  Exit;
if IsTahookRunning then
  SendChat ('*** '+Players[1].Name + ' uses TAHook');
end;

function TDPlay.OnBaseOff( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  i : Integer;
begin
Result := True;
QuickBaseEnabled := false;
for i := 1 to 10 do
  Players[i].GiveBase := false;
SendChat('Quick base disabled');
end;

