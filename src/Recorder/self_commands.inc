function TDPlay.OnHelp( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;

  procedure OutputInfo( const Command : string; HelpString : boolean );
  var
    CommandHandler : TCommandHandler;
  begin
  CommandHandler := Commands.LookUpCommand( Command );
  if CommandHandler <> nil then
    begin
    if HelpString then
      SendChatLocal2( CommandHandler.HelpInfo )
    else
      SendChatLocal2( 'Syntax is .'+Command +' '+CommandHandler.Syntax);
    end
  else
    SendChatLocal2( 'Command .'+command+' is not registered' )
  end; {OutputInfo}

var
  KnownCommands : string;
  i : Integer;
begin {OnHelp}
Result := True;
if params.Count <> 0 then
  begin
  if params[0] = 'info' then
    begin
    if params.Count = 1 then
      SendChatLocal('Returns the help information about a command')
    else
      OutputInfo( params[1], True );
    end
  else if params[0] = 'syntax' then
    begin
    if params.Count = 1 then
      SendChatLocal('Returns information about the syntax of a command')
    else
      OutputInfo( params[1], false );
    end
  else if params[0] = 'commands' then
    begin
    KnownCommands := 'Known commands: ';
    for i := 0 to Commands.Count-1 do
      begin
      KnownCommands := KnownCommands + Commands[i].Name;
      if i <> Commands.Count-1 then
        KnownCommands := KnownCommands +', ';
      end;
    SendChatLocal2( KnownCommands );
    end
  else
    OutputInfo( Command, True );
  end
else
  OutputInfo( Command, True );
end; {OnHelp}

function TDPlay.OnStatus( const Command : string;  Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
sendchatlocal('TA demo recorder ' + GetTADemoVersion + ' ('+getRecorderStatusString+')');
datachanged := true;
{
if imserver then
  sendchatlocal ('imserver ' + inttostr(servernumber))
else
  sendchatlocal ('imnotserver ' + inttostr (servernumber) + ' ' + mapname + ' ' + Players[servernumber].Name);
}
end;

function TDPlay.OnYankSpank( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
SendChat('One free krogoth for every Swede');
end;

function TDPlay.OnCrashMe( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
raise Exception.Create('Crash Me');
end;

function TDPlay.OnLoggingVerbosity( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if params.Count >= 1 then
  begin
  Log_.VerboseLoggingLevel := StrToInt(params[0]);
  SendChat('Setting logging verbosity level to '+IntToStr(Log_.VerboseLoggingLevel));
  end
else
  SendChat('Current logging verbosity level:'+IntToStr(Log_.VerboseLoggingLevel));
end; {OnLoggingVerbosity}

function TDPlay.OnCrash( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if not crash then
  begin
  SendChat ('Recorder will not catch exceptions');
  SendChat ('If TA crashes, please send your errorlog.txt to us');
//  raise Exception.Create ('You suck man');
  crash := true;
  end
else
  begin
  SendChat ('Recorder will catch exceptions');
  crash := false;
  end;
end;

function TDPlay.OnPanic( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
SendChat('Recorder for '+Players[1].name+' off');
TADemoRecorderOff := True;
end;

function TDPlay.OnAbout( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
SendChat('TA demo recorder ' + GetTADemoVersion);
SendChat('Written by SY_SJ and SY_Fnordia');
SendChat('Updated by Xon');
SendChat('Visit our site at http://www.clan-sy for more info');
end;

function TDPlay.OnFixall( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if fixon then
  begin
  protectdt := true;
  fixfacexps := true;
{   f TAStatus=0 then
   logpl:=true;}
  SendChat('All fixes turned on');
  end;
end;

function TDPlay.OnFixFactoryExplosions( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if not fixon then
  begin
  SendChat('Lag fixes disabled');
  Exit;
  end;
if fixfacexps then
  begin
  fixfacexps := false;
  SendChat('Factory explosion fix disabled');
  end
else
  begin
  fixfacexps := true;
  SendChat('Factory explosion fix enabled');
  end;
end;

function TDPlay.OnProtectdt( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if not fixon then
  begin
  SendChat('Lag fixes disabled');
  Exit;
  end;
if protectdt then
  begin
  protectdt:=false;
  SendChat('Missing DT fix disabled');
  end
else
  begin
  protectdt:=true;
  SendChat('Missing DT fix enabled');
  end;
end;

function TDPlay.OnSharemappos( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var i : Integer;
begin
Result := True;
if chatview = nil then Exit;
OldMapX := high(word);
OldMapY := high(word);
if shareMapPos or (chatview^.ehaOff>0) then
  begin
  shareMapPos:=false;
  for i :=1 to 10 do
    begin
    chatview^.otherMapX[i]:=-1;
    chatview^.otherMapY[i]:=-1;
    end;
  SendRecorderToRecorderMsg2( #$fc#$ff#$ff#$ff#$ff );  
  SendChatLocal('Toggled ShareMapPos to: OFF');
  end
else
  begin
  ShareMapPos := true;
  SendChatLocal('Toggled ShareMapPos to: ON');
  end;
end;

function TDPlay.OnTime( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  i : Integer;
  tmp : String;
begin
Result := True;
SendChat ('Current position of the players:');
for i := 1 to Players.Count do
  begin
  tmp := IntToStr ((Players[i].LastTimeStamp div 30) mod 60);
  if length (tmp) = 1 then
    tmp := '0' + tmp;
  SendChat (' (' + IntToStr (Players[i].LastTimeStamp div 1800)+':'+ tmp + ') - ' +Players[i].name);
//{$IFNDEF release}IntToStr (Players[i].LastTimeStamp) +{$ENDIF}
  end;
//      raise Exception.create ('u suk man');
end;

function TDPlay.OnReportFilter( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  I : Integer;
  s : string;
begin
Result := True;
if PacketsToFilter <> nil then
  begin
  SendChat('Packet filtering enabled');
  s := '';
  for i :=  0 to High(PacketsToFilter) do
    s := s+'$'+InttoHex(PacketsToFilter[i],2)+' ';
  SendChat('Filtering out '+s);
  end
else
  SendChat('Packet filtering disabled');
end; {OnReportFilter}

function TDPlay.OnDisableFilter( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if PacketsToFilter <> nil then
  begin
  PacketsToFilter := nil;
  SendChat('Packet filtering disabled');
  end;
end;

function TDPlay.OnAddFilter( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  I, i2, count : Integer;
  s : string;
  Filter : Byte;
  DuplicateFilter : Boolean;
  PacketsToFilter2 : TArrayOfByte;
begin
Result := True;
if params.Count <> 0 then
  begin
  if PacketsToFilter = nil then
    SendChat('Packet filtering enabled');
  s := '';
  setlength( PacketsToFilter2, Length(PacketsToFilter)+params.Count );
  for i := 0 to High(PacketsToFilter) do
    PacketsToFilter2[i] := PacketsToFilter[i];
  Count := Length(PacketsToFilter);
  for i := 0 to params.Count-1 do
    begin
    if not IsStrByte(params[i],Filter) then
      continue;
    DuplicateFilter := False;
    for i2 := 0 to High(PacketsToFilter2) do
      if Filter = PacketsToFilter2[i2] then
        begin
        DuplicateFilter := True;
        break;
        end;
    if not DuplicateFilter then
      begin
      PacketsToFilter2[count] := Filter;
      s := s+'$'+InttoHex(Filter,2)+' ';
      inc(count);
      end;
    end;
  if s <> '' then
    begin
    setlength( PacketsToFilter2, Count );
    PacketsToFilter := PacketsToFilter2;
    SendChat('Filtering out '+s);
    end;
  end;
end; {OnAddFilter}

function TDPlay.OnRemoveFilter( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  i, i2,i3, Count : Integer;
  s : string;
  PacketsToFilter2 : TArrayOfByte;
  Filter : Byte;
begin
Result := True;
if params.Count <> 0 then
  begin
  s := '';
  Count := Length(PacketsToFilter);
  setlength( PacketsToFilter2, Length(PacketsToFilter) );
  for i := 0 to High(PacketsToFilter) do
    PacketsToFilter2[i] := PacketsToFilter[i];
  for i := 0 to params.Count-1 do
    begin
    Filter := StrToInt(params[i]);
    for i2 := 0 to High(PacketsToFilter2) do
      if Filter = PacketsToFilter2[i2] then
        begin
        s := s+'$'+InttoHex(Filter,2)+' ';
        Dec(Count);
        for i3 := i2 to Count-1 do
          PacketsToFilter2[i3] := PacketsToFilter2[i3+1];
        break;
        end;
    end;
  if s <> '' then
    begin
    setlength( PacketsToFilter2, Count );
    PacketsToFilter := PacketsToFilter2;
    SendChat('Removed filtering of '+s);
    end;
  if PacketsToFilter = nil then
    SendChat('Packet filtering enabled');
  end;
end; {OnRemoveFilter}


function TDPlay.OnRecord( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if TAStatus = InBattleRoom then
  begin
  if params.Count <> 0 then
    FileName := params[0]
  else
    FileName := '';

  if (ExtractFileExt(FileName)='') and (FileName<>'') and (FileName<>'none') then
    FileName := FileName+'.tad';
  if FileName<>'none' then
    SendChat('Recording to '+FileName)
  else
    SendChat('Recording disabled');

  if ai <> '' then
    SendChat ('Warning: A custom AI (' + ai + ') is enabled');
  end
else
  begin
  if FileName='' then
    begin
    if params.Count <> 0 then
      FileName := params[0]
    else
      FileName := '';

    if (ExtractFileExt(FileName)='') and (FileName<>'') then
      FileName:=FileName+'.tad';
    if FileName <> '' then
      begin
      FileName := removeinvalid (FileName);
      createlogfile();
      end;
    end;
  if FileName<>'' then
    begin
    IsRecording := true;
    prevtime := GetTickCount;
    SendChat('Recording to '+FileName);
    end;
  end;
end;

function TDPlay.OnRecordStatus( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  value : boolean;
begin
Result := True;
if params.Count = 0 then
  begin
  if NoRecording then
    SendChatLocal('Recording of games disabled')
  else
    SendChatLocal('Recording of games enabled');
  end
else if IsStrBool( params[0], Value ) then
  begin
  if Value then
    begin
    NoRecording := true;
    SendChatLocal('Ok, you will not be able to record');
    end
  else
    begin
    NoRecording := false;
    SendChatLocal('Ok, you will be able to record');
    end
  end
else
  SendChatLocal( 'Paramater not understood, true/false or any numerical integer evaluated to 0/1' );
end;

function TDPlay.On3dta( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  Value : Boolean;
begin
Result := True;
if (params.Count = 0) or (TAStatus = InGame) then
  begin
  if use3d then
    SendChat ('3D TA is enabled')
  else
    SendChat ('3D TA is disabled');
  end
else if IsStrBool( params[0], Value ) then
  begin
  if Value then
    begin
    use3d := true;
    SendChat ('3D TA now enabled');
    end
  else
    begin
    use3d := false;
    SendChat ('3D TA now disabled');
    end
  end
else
  SendChatLocal( 'Paramater not understood, true/false or any numerical integer evaluated to 0/1' );
end;

function TDPlay.OnStopLog( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
IsRecording := False;
//      FileName:='';
SendChat('logging halted');
end;

function TDPlay.OnOnlyUnits( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if onlyunits then
  begin
  onlyunits := false;
  SendChat('All packets recorded');
  end
else
  begin
  onlyunits := true;
  SendChat('Only the units will be recorded');
  end;
end;

function TDPlay.OnCreatetxt( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if createtxtfile then
  begin
  createTxtFile:=false;
  SendChat('Not creating a txt file');
  end
else
  begin
  createTxtFile:=true;
  SendChat('Will create a txt file with game info');
  end;
end;

function TDPlay.OnLockOn( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  i : Integer;
  s : string;
begin
Result := True;
if chatview = nil then Exit;
chatview^.lockviewon := 0;
if not NotViewingRecording and (params.Count <> 0) then
  begin
  s := uppercase(params[0]);
  for i:=2 to Players.Count do
    if s = uppercase(Players[i].Name) then
      chatview^.lockviewon := i-1;
  end;
end;

function TDPlay.OnRandMap( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  mapname : string;
begin
Result := True;
if params.Count = 0 then
  mapname := 'maps.txt'
else
  mapname := params[0];

if FileExists( mapname )then
   GetRandomMap(mapname)
else
   GetRandomMap(serverdir + mapname);
end;

function TDPlay.OnFakeWatch( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
if fakewatch then
  begin
  fakewatch := false;
  SendChat('Normal player mode');
  end
else
  begin
  fakewatch := true;
  SendChat('Fake watcher mode');
  end;
end;

function TDPlay.OnForceCd( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := true;
datachanged := true;
fakecd := not fakecd;
if fakecd then
  SendChat( 'Fake CD enabled for '+Players[1].name)
else
  SendChat( 'Fake CD disabled for '+Players[1].Name)
end;

function TDPlay.OnLookUpAddr( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
var
  errorAddress : Longword;
begin
Result := True;
errorAddress := StrToInt(Params[0]);
if not MapFileSetup then
  LoadAndParseMapFile;
errorAddress := GetMapAddressFromAddress(errorAddress);
SendChatLocal('Module:'+GetModuleNameFromAddress(errorAddress));
SendChatLocal('function:'+ GetProcNameFromAddress(errorAddress));
SendChatLocal('line:'+ GetLineNumberFromAddress(errorAddress));
end; {OnLookUpAddr}
    (*
function TDPlay.On( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
begin
Result := True;
end;

    if (Command = 'plfrom')  then begin
      tmp:='Use .pltest/.plresults in battle room';
      if (TAStatus=InGame) then begin
        tmp:='Not logging pl';
        if logpl then begin
          tmp:='No name';
          for c:=1 to Players.Count do
            if uppercase(copy(s2,9,length(Players[c].Name)))=uppercase(Players[c].Name) then begin
              a:=Players[c].lostpackets+Players[c].goodpackets;
              SendChat('PL statistics from '+Players[c].Name);
              SendChat('Good '+inttostr(Players[c].goodpackets)+' Lost '+inttostr(Players[c].lostpackets)+ ' Average/Current/Peak PL '+inttostr((Players[c].lostpackets*100) div a)+'% '+inttostr(Players[c].losscount)+'% '+inttostr(Players[c].MaxLoss)+'% ');
              tmp:='Ranges ';
              for i := 1 to 10 do
                tmp := tmp + inttostr((Players[c].PacketLostHistory[i]*100) div a)+'%';
            end;
        end;
      end;
      SendChat(tmp);
    end;

{    if (Command = '.income')then
    begin
      for a:=1 to Players.Count do begin
        SendChat(Players[a].Name+' Metal: '+floattostrf(incomem[a]-lastsharedm[a],ffFixed,7,0)+' Energy: '+floattostrf(incomee[a]-lastsharede[a],ffFixed,7,0));
      end;
    end;
 }



 if (TAStatus = InBattleRoom ) then
   begin
    if (Command = 'forcecd')  then begin
      datachanged:=true;
      if fakecd then begin
        fakecd:=false;
      end else begin
        fakecd:=true;
      end;
    end;
    if (Command = 'staton') {and (TAStatus = 0)} then
    begin
      statdll := LoadLibrary ('netstat.dll');
      procstart := GetProcAddress (statdll, 'Start');
      procnewunit := GetProcAddress (statdll, 'NewUnit');
      procunitfinished := GetProcAddress (statdll, 'UnitFinished');
      procdamage := GetProcAddress (statdll, 'Damage');
      prockill := GetProcAddress (statdll, 'Kill');
      procstat := GetProcAddress (statdll, 'Income');
      staton := true;
      SendChat ('Ok, recording statistics');
    end;
    {    if (copy (s2, 1, 4) = '.jam') and (TAStatus = 0) then
    begin
      datachanged := true;
      SendLocal (#$1a + #$3 + '    ' + #$00#$00#$00#$42 + #$01#$00#$00#$00, 0, false, true);
    end;}

    if (Command = 'logpl')   then begin
      if logpl or (not fixon) then begin
        logpl:=false;
        SendChat('Packet loss logging disabled');
      end else begin
        logpl:=true;
        SendChat('Packet loss logging enabled');
      end;
    end;



    if (Command = 'pltest')  then
    begin
      crcattempts:=crcattempts+1;
      b:=1;
      if crcattempts<8 then begin
        for c:=2 to Players.Count do
          if uppercase(copy(s2,9,length(Players[c].Name)))=uppercase(Players[c].Name) then
          begin
            SendChat('Sending test packets to '+Players[c].Name);
            b:=0;
            for a:=1 to 101 do begin
              tmp:='1234'+#0#0#0#0+'1234';
              ip:=@tmp[1];
              ip^:=a;
              ip:=@tmp[9];
              ip^:=Players[1].Id;
              tp:=TPacket.Create($02,@tmp[1],length(tmp));
              tmp:=tp.TaData;
              tp.free;
              dp3.Send (Players[1].Id, Players[c].id, 0, tmp[1], length(tmp));
              sentpings[a]:=0;
            end;
            pingtimer:=GetTickCount;
          end;
        end;
      if b=1 then
        SendChat('No name or too many attempts');
    end;
    if (Command = 'plresults') then
    begin
      b:=0;
      c:=0;
      for a:=1 to 101 do
      begin
        if sentpings[a]<>0 then
        begin
          inc(b);
          c:=c+sentpings[a]-pingtimer;
        end;
      end;
      if b>0 then
        tmp:='PL: '+inttostr(100-b)+'% Ping: '+floattostr(c/b)+' ms'
      else
        tmp:='Its all PL man';
      SendChat(tmp);
    end;


    end;

*)
