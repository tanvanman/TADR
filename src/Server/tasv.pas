unit tasv;

interface

uses
  DPlay, ActiveX, Classes, Windows, SysUtils, packet, Forms, Logging, lobby,
  replay, savefile,  unitsync, extctrls, dialogs;

//const
//  SYNC_SPEED :integer = 100;

const
  NUM_FAKES = 20;
  KEEP_LIMIT = 200;     //Should be like 20 sec? Oh well, at this time the dude must have units in any case.
  KILL_SPEED = 20;
  GIVE_SPEED = 20;

type
  TPlayer = class
    Id  :TDPID;
    Data :TIdent;
    Synced :integer;          //Like, in what stage of syncing you're in... 0 = not 
                              // (could also be: In what stage of syncing you're type) 
    Sent :integer;            //Number of $1a sent to this player.
    curunit :integer;         //What unit we should send.
    errors :integer;          //Number of faulty units
    acked  :integer;          //Number of units the client responded to with crc. 
                              //(alternatively: responded with crc on) 
    received :integer;        //Number of $1a it claims to have recieved 
    hastaken :boolean;        //True if this player has taken control over someone. 
    warned :boolean;          //True if this player was been warned.
  end;

type
  TPlayers = class
    Players :TList;
    constructor Create;
    destructor Destroy; override;
    procedure Add (id :TDPID);
    procedure Remove (id :TDPID);
    function Get (id :TDPID) :TPlayer;
    function Size :integer;
    function GetAt (nr :integer) :TPlayer;
    function GetPos (id :TDPID) :integer;
    procedure Sort;
  end;

{--------------------------------------------------------------------}

type
  TAServer = class
    watcher1 :tdpid;
    dp :IDirectPlay3;
//    serviceproviders :TStringList;
    curconn :TDPLConnection;
//    player :TDPID;
    sess :TDPSessionDesc2;
    curserial :longword;
    NumWatchers :integer;
    players :TPlayers;
    nocheats:boolean;
    sonarcount:integer;
    idletimes:integer;
    numdrones :integer;
    Drones :array[1..10] of TDPID;
    DroneInfo :array[1..10] of TDemoPlayer;
    rejected :array[1..10] of boolean;
    Lastseen :array[1..10] of integer;
    //Starting base for a certain player... E.g. 0,250, etc. based on maxunits...
    unitrange :array[1..10] of integer;
    taken :array[1..10] of boolean;
    health :array[1..10] of TSaveHealth;
    lastchat :array[1..10] of string;
    incomem      : array [1..10] of single;
    incomee      : array [1..10] of single;
    sharedm      : array [1..10] of single;
    sharede      : array [1..10] of single;
    lastsharedm  : array [1..10] of single;
    lastsharede  : array [1..10] of single;
    lasttotalm   : array [1..10] of single;
    lasttotale   : array [1..10] of single;
    totalsharedm : array [1..10] of single;
    totalsharede : array [1..10] of single;
    lastserial   : array[1..10] of longword;
    laststat     : array[1..10] of longword;

    lastplayerpack :cardinal; //Latests 2c serial number for watcher.
    lastcompack :cardinal; //Lowest 2c serial number for all drones.
    lastdronepack : array[1..10] of cardinal; //Latests 2c for drone.
    possynccomplete : array[1..10] of cardinal; //When to start sending after a pos.
    recentpos       : array[1..10] of boolean;
    kill :TList;    //List of units that shall meet an untimely death.
    take :TList;    //List of units that shall have a new owner. 

    keepplayers :TSavePlayers;
//    orgpid    :array [1..10] of integer;
    save :TSavefile;
    mm :TTimer;
    data :pointer;
    loop      :integer;
    quit :boolean;
    done      :boolean;
    curtime, nextpulse :integer;
    units :TUnitSync;

    inited :boolean;
    speed :integer;
    provider :TGuid;
    inpos :boolean;

    paused :boolean;
    cont :TContLoad;

    maxunits :word;
    intake :boolean;
    donetake :boolean;

    keffunits :TList;
    oldnewtimer :boolean;

    constructor Create (dp :IDirectPlay3);
    destructor Destroy; override;
    procedure CreateSession (p :pointer; save :TSaveFile);
    procedure Test (r :HResult);
    procedure Leave;

    function IsDrone (id :TDPID) :boolean;
    procedure Send (from :integer; dest :TDPID; packet :TPacket);
    procedure SendUdp (from :integer; dest :TDPID; packet :TPacket);

    procedure Delay (l :integer);
    procedure DoLobby (Sender :TObject);
    function Launch :boolean;
    procedure Load;
    procedure DoLoad (Sender :TObject);
    procedure DoPlay (Sender :TObject);

    procedure Filter (var s :string;a:integer);

    procedure SetPos (i :integer);
    procedure Say (from :integer; st :String);

    function GetDroneNr (pid :cardinal) :integer;
  end;

type
  PService = ^TService;
  TService = record
    guid :TGuid;
    conn :PDPLConnection;
  end;

implementation

uses
  main, textdata, loading;

{99797420-F5F5-11CF-9827-00A0241496C8}
const
  TOTALA_GUID: TGUID = (D1:$99797420;D2:$F5F5;D3:$11CF;D4:($98,$27,$00,$a0,$24,$14,$96,$c8));

type
  TTake = class
    source :integer;
    dest   :TDPid;
    unitnr :integer;
  end;

{--------------------------------------------------------------------}

constructor TPlayers.Create;
begin
  inherited Create;
  Players := TList.Create;
end;

destructor TPlayers.Destroy;
var
  i :integer;
begin
  for i := 0 to Players.Count - 1 do
    TPlayer(Players.Items[i]).Free;

  Players.Free;
  inherited Destroy;
end;

procedure Tplayers.Add (id :TDPID);
var
  p :TPlayer;
begin
  p := TPlayer.Create;
  p.Id := id;
  p.Data := nil;
  p.curunit := 0;
  p.errors := 0;
  p.acked := 0;
  p.hastaken := false;
  p.warned := false;
  p.sent := 1;
  p.Synced := -1;
  Players.Add (p);
end;

procedure TPlayers.Remove (id :TDPID);
var
  p :TPlayer;
  i :integer;
begin
  for i := 0 to Players.Count - 1 do
  begin
    p := Players.Items [i];
    if p.id = id then
    begin
      Players.Remove (p);
      p.Free;
      exit;
    end;
  end;
end;

function TPlayers.Get (id :TDPID) :TPlayer;
var
  p :TPlayer;
  i :integer;
begin
  for i := 0 to Players.Count - 1 do
  begin
    p := Players.Items [i];
    if p.id = id then
    begin
      Result := p;
      exit;
    end;
  end;
end;

function TPlayers.Size :integer;
begin
  Result := players.Count;
end;

function TPlayers.GetAt (nr :integer) :TPlayer;
begin
  Result := players.items [nr];
end;

function compare (Item1, Item2: Pointer): Integer;
begin
  if TPlayer(item1).id = TPlayer(item2).id then Result := 0;
  if TPlayer(item1).id < TPlayer(item2).id then Result := -1;
  if TPlayer(item1).id > TPlayer(item2).id then Result := 1;
end;

procedure TPlayers.Sort;
begin
  players.Sort (compare);
end;

function TPlayers.GetPos (id :TDPID) :integer;
var
  p :TPlayer;
  i :integer;
begin
  for i := 0 to Players.Count - 1 do
  begin
    p := Players.Items [i];
    if p.id = id then
    begin
      Result := i;
      exit;
    end;
  end;
end;

{--------------------------------------------------------------------}

function EnumConns (const lpguidSP: TGUID; lpConnection: Pointer;
  dwConnectionSize: DWORD; const lpName: TDPName; dwFlags: DWORD; lpContext: Pointer) : BOOL; stdcall;
var
  buf :PDPLConnection;
  service :PService;
begin
  New (service);
  GetMem (buf, dwConnectionSize);
  Move (lpConnection^, buf^, dwConnectionSize);

  Move (lpguidsp, service.guid, sizeof (lpguidsp));
  service.conn := buf;

  fmMain.lbProviders.Items.AddObject (lpName.lpszShortName, TObject (service));
  Result := true;
end;

{kol 2 har inte med metall / energy att göra iafcol 2 has nothing to do with metal/energy to do anyway. }

{ bit 1 = the one to the right.}
{col 3 : loword * 100 = energy, hiword * 100 = metal}
{col 1 bit 32 = game closed}
{loword col 4 = number of units.}

{ 1325465696 - 4 - 655370 - 16974074   = Yes, 0, No, Blk, 1000 1000 1/10 Open }
{ 1325465696 - 0 - 720906 - 16974074   = Yes, 0, No, Blk, 1100 1000 1/10 Open }
{ 1325465696 - 0 - 786442 - 16974074   = Yes, 0, No, Blk, 1200 1000 1/10 Open }

{ 1325465696 - 0 - 655371 - 16974074   = Yes, 0, No, Blk, 1000 1100 1/10 Open }

function EnumSessions (const lpThisSD: TDPSessionDesc2; var lpdwTimeOut: DWORD;
  dwFlags: DWORD; lpContext: Pointer) : BOOL; stdcall;
var
  guid :PGuid;
  s     :string;
begin
  Result := false;
  if (dwFlags and DPESC_TIMEDOUT) <> 0 then
    exit;

  New (Guid);
  Move (lpThisSD.guidInstance, guid^, sizeof (TGuid));

  s := Format ('Session: %s   - %d - %d - %d - %d', [lpThisSD.lpszSessionName, lpThisSD.dwUser1,lpThisSD.dwUser2,lpThisSD.dwUser3,lpThisSD.dwUser4]);
//  fmMain.lbProviders.Items.AddObject (s, TObject (guid));
  Result := true;
end;

function EnumPlayers (TDPID: TDPID; dwPlayerType: DWORD; const lpName: TDPName; dwFlags: DWORD;
  lpContext: Pointer) : BOOL; stdcall;
var
  s :string;
begin
  s := lpName.lpszShortName;
  s := s + ' - ' + inttostr (tdpid);
  fmMain.lbPlayers.Items.AddObject (s, pointer(tdpid));
  Result := true;
end;

{--------------------------------------------------------------------}

constructor TAServer.Create (dp :IDirectPlay3);
//var
//  dp1 :IDirectPlay;
//  x    :integer;
begin
  inherited Create;

//  DirectPlayCreate (@GUID_NULL, dp1, nil);
//  dp1.QueryInterface (IID_IDirectPlay3, dp);
  Self.dp := dp;

//  CoCreateInstance( CLSID_DirectPlay, nil, CLSCTX_INPROC_SERVER,
//    IID_IDirectPlay3A, dp);

//  TPacket.Create (byte(','), '$0B$00$11$00$00$00ÿÿ$01$00,$0B$00$12$00$00$00ÿÿ$01$00' + ',$0B$00$13$00$00$00ÿÿ$01$00,$0B$00$14$00$00$00ÿÿ$01$00,$0B$00$15$00$00$00ÿÿ$01$00,$0B$00$16$00$00$00ÿÿ$01$00,$0B$00$17$00$00$00ÿÿ$01$00');

//  serviceproviders := TStringList.Create;
  dp.EnumConnections (TOTALA_GUID, EnumConns, Self, DPCONNECTION_DIRECTPLAY);

  inited := false;
  speed := 120;
  inpos := false;
//  quit := false;
end;

destructor TAServer.Destroy;
begin
  Leave;
  FreeMem (data, 10000);

  inherited Destroy;
end;

procedure TAServer.Test (r :HResult);
begin
  if r <> DP_OK then
  begin
    raise Exception.CreateFmt ('DPLAY : %s', [ErrorString (r)]);
  end;
end;

procedure TAServer.CreateSession (p :pointer; save :TSavefile);
var
  name :TDPName;
  id   :TIdent;
  pd   :string;
  i,j  :integer;
  tmp  :array[0..63] of char;
  x    :TPlayers;
  y    :TDPID;
  fail :boolean;
  cp   :integer;
  sn   :PChar;

  s    :string;
  pa   :TPacket;
  ano  :TPlayers;

  service :PService;
  dp1 :IDirectPlay;
begin
  service := PService (p);
  if dp.InitializeConnection (service.conn, 0) <> DP_OK then
  begin
    DirectPlayCreate (@GUID_NULL, dp1, nil);
    dp1.QueryInterface (IID_IDirectPlay3, dp);
    Test (dp.InitializeConnection (service.conn, 0));
    fmmain.dp := dp; //Ew
  end;

  provider := service.guid;

  FillChar (sess, Sizeof (sess), 0);
  sess.dwSize := sizeof (sess);
  sess.dwFlags := 0;
  sess.guidApplication := TOTALA_GUID;
  sess.dwCurrentPlayers := 0;
  sess.dwMaxPlayers := 100;
  pd := 'TA DEMO 0.99ß    ' + save.map;

  sn := stralloc (100);
  StrPCopy (sn, pd);
  sess.lpszSessionName := sn;

{ 1325465696 - 0 - 786442 - 16974074   = Yes, 0, No, Blk, 1200 1000 1/10 Open }

  sess.dwUser1 := 1753284736;
  sess.dwUser2 := 4;
  sess.dwUser3 := 655370;
  sess.dwUser4 := 16974324;

  dp.Open (sess, DPOPEN_CREATE);

  Self.save := save;

  numdrones := save.Numplayers;
  for i := 1 to 10 do
    DroneInfo [i] := save.Players [i];

  x := TPlayers.Create;

  cp := 0;
  repeat
    fail := false;
    for i := 1 to NUM_FAKES do
    begin
      FillChar (name, Sizeof (name), 0);
      name.dwSize := sizeof (name);

      StrPCopy (tmp, 'Pingvin ' + inttostr (i));
      name.lpszShortName := tmp;
      name.lpszLongName := tmp;

      pd := '$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00P$00';
      dp.CreatePlayer (y, @name, 0, StrToData (pd), DataSize (pd), 0);
      x.Add (y);

      fmMain.lbPlayers.Items.Add (inttostr (i) + ' - ' + inttostr (y));
    end;

    if x.getat (0).id > x.getat (NUM_FAKES - 1).id then
    begin
      fail := true;
      for i := 1 to NUM_FAKES do
      begin
        dp.DestroyPlayer (x.getat (i-1).id);
      end;
      dp.Close;
      dp.Open (sess, DPOPEN_CREATE);

      x.Free;
      x := TPlayers.Create;
    end;

    inc (cp);
    if cp > 100 then
    begin
      MessageDlg ('Whee, you have to restart the replayer..', mtError, [mbok], 0);
      Application.Terminate;
    end;
  until not fail;

  x.Sort;
//  showmessage (inttostr (cp));


{  save.orgpid [1] := 3;
  save.orgpid [2] := 1;
  save.orgpid [3] := 2;}

  for i := 0 to numdrones - 1 do
  begin

    FillChar (name, Sizeof (name), 0);
    name.dwSize := sizeof (name);

    StrPCopy (tmp, save.players[save.orgpid [i+1]].name);
    name.lpszShortName := tmp;
    name.lpszLongName := tmp;

    dp.SetPlayerName (x.getat(i).id, @name, DPSET_GUARANTEED);
    drones [save.orgpid [i+1]] := x.Getat (i).id;
    droneinfo [save.orgpid [i+1]] := save.players [save.orgpid [i+1]];

    unitrange [save.orgpid [i+1]] := i * save.maxunits;
    taken[i+1] := false;
    rejected[i+1] := false;

    health[i+1].maxunits := save.maxunits;
    for j := 0 to save.maxunits do
      health[i+1].health[j] := 42;
  end;

  for i := numdrones to NUM_FAKES - 1 do
  begin
    dp.DestroyPlayer (x.getat (i).id);
  end;

  x.Free;

  dp.EnumPlayers (@sess.GuidInstance, EnumPlayers, Self, 0);

  curserial := $fffffffe;
  loop := 0;

  NumWatchers := 0;
  players := TPlayers.Create;
  kill := TList.Create;
  take := TList.Create;

  keffunits := TList.Create;

  GetMem (data, 10000);

  for i:=1 to 10 do
    possynccomplete[i]:=lastdronepack[i];

  units := TUnitSync.Create (save.Units);
  units.Compact;
  nocheats:=false;
  sonarcount:=0;
  mm := TTimer.Create (fmMain);
  mm.Enabled := false;
  mm.Interval := options.interval+10;
//  mm.SetInterval (100);
  mm.OnTimer := DoLobby;
  mm.Enabled := true;

  inited := true;
//  mm.Start;
end;

procedure TAServer.Leave;
var
  dp1 :IDirectPlay;
  i   :integer;
begin
  if inited then
  begin
    for i := 1 to numdrones do
      dp.DestroyPlayer (drones[i]);
    dp.Close;

    mm.enabled := false;

    kill.Free;
    players.Free;
    units.Free;
    mm.Free;

    inited := false;
  end;



//  quit := true;
//  DirectPlayCreate (@GUID_NULL, dp1, nil);
//  dp1.QueryInterface (IID_IDirectPlay3, dp);
end;

procedure TAServer.SendUdp (from :integer; dest :TDPID; packet :TPacket);
var
  data :string;
begin
//  if Dest = 0 then
//    packet.Serial := curserial;
  packet.Serial := 0;

  data := packet.TAData;
//  Test(

//  Test(
  dp.Send (drones[from], dest, 0, data[1], Length (data));

  if Dest = 0 then
    curserial := curserial - 1;
end;

procedure TAServer.Send (from :integer; dest :TDPID; packet :TPacket);
var
  data :string;
begin
  if Dest = 0 then
    packet.Serial := curserial;

  data := packet.TAData;
//  Test(

//  Test(
  dp.Send (drones[from], dest, 1, data[1], Length (data));

  if Dest = 0 then
    curserial := curserial - 1;
end;

procedure TAServer.Say (from :integer; st :String);
var
  t :TChat;
begin
  t := TChat.Create (st);
  SendUdp (from, 0, t);
  t.Free;
end;

function TAServer.IsDrone (id :TDPID) :boolean;
var
  i :integer;
begin
  Result := true;
  for i := 1 to numdrones do
    if id = drones[i] then exit;
  Result := false;
end;

procedure TAServer.Delay (l :integer);
var
  t :TDateTime;
begin
  t := now;
  repeat
    Application.ProcessMessages;
  until now > t + l / 86400000;
end;

procedure TAServer.DoLobby (Sender :TObject);
var
  from, pto :TDPID;
  dsize     :cardinal;
  err       :HResult;
  p         :TPacket;
  ping      :TPing;
  svar      :TPing;
  id        :TIdent;
  id2       :TIdent2;
  chat      :TChat;
  i         :integer;
  playr     :TPlayer;
  r         :TReplay;
  s,tmp,cur :string;
  pkt       :TUnitPkt;
  j         :integer;
  a,b       :integer;
  point     :pointer;
  upkt      :TUnit;
  runit     :PRawUnit;

  tonr    :integer;
  allgo :integer;
  tmpu    :^TUnitPkt;
begin

//  loop := 0;
//  id := TIdent.Create (player);

  id2 := TIdent2.Create ([]);
  for i := 1 to numdrones do
    id2.Add (drones[i]);

//  r := TReplay.Create;

//  repeat
  repeat
    dsize := 10000;
    err := dp.Receive (from, pto, 1, data, dsize);
    if err = DP_OK then
    begin
      tonr := 0;
      for i := 1 to numdrones do
        if drones[i] = pto then
          tonr := i;

      if (from = DPID_SYSMSG) and (tonr = 1) then
      begin
        case PDPMsg_Generic (data)^.dwType of
            DPSYS_CREATEPLAYERORGROUP  :begin
                                          if not IsDrone (PDPMsg_CreatePlayerGroup (Data)^.tdpid) then
                                          begin
                                            Inc (NumWatchers);
                                            watcher1:=(PDPMsg_CreatePlayerGroup (Data)^.tdpid);
                                            id2.Add (PDPMsg_CreatePlayerGroup (Data)^.tdpid);
                                            players.Add (PDPMsg_CreatePlayerGroup (Data)^.tdpid);
                                            fmMain.lbPlayers.Items.Clear;
                                            dp.EnumPlayers (@sess.GuidInstance, EnumPlayers, Self, 0);

//                   fmMain.lbChat.Items.Add ('new player');
                                          end;
                                        end;
            DPSYS_DESTROYPLAYERORGROUP :begin
                                          if not IsDrone (PDPMsg_DestroyPlayerGroup (Data)^.tdpid) then
                                          begin
                                            Dec (NumWatchers);
                                            fmMain.lbPlayers.Items.Clear;
                                            players.Remove (PDPMsg_DestroyPlayerGroup (Data)^.tdpid);
                                            id2.Remove (PDPMsg_DestroyPlayerGroup (Data)^.tdpid);
                                            dp.EnumPlayers (@sess.GuidInstance, EnumPlayers, Self, 0);
                                          end;
                                        end;
            DPSYS_SETSESSIONDESC       :begin
                                        end;
        end;

        exit;
      end;

      s := PtrToStr (data, dsize);
      repeat
        cur := TPacket.Split (s);
        p := TPacket.Create (@cur[1], Length (cur));
        case p.Kind of
          $02 :begin
                 ping := TPing.Create (p);
                 svar := TPing.Create (ping.From, ping.id, 1000000 + Random (1000000));
                 fmMain.lbCom.Items.Add ('Ping');
                 Send (tonr, from, svar);
                 ping.free;
                 svar.Free;

                 playr := players.Get (from);
                 fmMain.lbChat.Items.Add ('got stuff from player');
                 if playr.Synced = -1 then
                 begin
                   fmMain.lbChat.Items.Add ('starting sync');
                   playr.Synced := 0;
                 end;

               end;
          $05 :begin
                 if tonr = 1 then
                 begin
                   chat := TChat.Create (p);
                   fmMain.lbChat.Items.Add (chat.Msg);

                   if Pos ('.LAUNCH', Uppercase (chat.msg)) > 0 then
                     Say (1, '*** .launch is now obsolete. Just click in!');

                   if Pos ('.RESYNC', Uppercase (chat.msg)) > 0 then
                   begin
                     for i := 0 to keffunits.count - 1 do
                     begin
                       units.add (PUnitPkt (keffunits.items [i])^);
                       units.add (PUnitPkt (keffunits.items [i])^);
                     end;

                     for i := 0 to players.size - 1 do
                     begin
                       players.GetAt (i).synced := 0;
                       players.getAT (i).sent := players.getat (i).sent + 1;
                     end;
                   end;
                   if Pos ('.NOCHEAT', Uppercase (chat.msg)) > 0 then begin
                     if nocheats then nocheats:=false
                     else nocheats:=true;
                   end;
                   chat.Free;
                 end;
               end;
          $1a : begin
                  playr := players.Get (from);
                  upkt := TUnit.Create (p);
                  case upkt.subtype of
                    $2 :begin
                          runit := units.Find (upkt.id);
                          // If not then uninteresting, it's a unit we don't know/recognise
                          if Assigned (runit) then
                          begin
                            if (runit.crc <> upkt.crc) and (upkt.id <> SY_UNIT) then
                              Inc (playr.errors)
                            else
                              inc (playr.acked);
                          end else
                          begin
                            pkt.pktid := $1a;
                            pkt.sub := $3;
                            pkt.fill := 0;
                            pkt.id := upkt.id;
                            pkt.status := $0101;
//                            pkt.limit := upkt.Limit;
                            pkt.limit := 65535;

                            New (tmpu);
                            tmpu^ := pkt;
                            keffunits.add (tmpu);

//                            units.Add (pkt);

                            pkt.sub := $2;
                            pkt.crc := upkt.crc;

                            new (tmpu);
                            tmpu^ := pkt;
                            keffunits.add (tmpu);
//                            units.add (pkt);
//                            ShowMessage ('othur');
                          end;
                        end;
                    $4 :begin
                          playr.received := upkt.status;
                        end;
                  end;
  //                units.Add (upkt);
  //                upkt.Free;
                end;
          $20 :begin
                 if not isdrone (from) then
                 begin
                   playr := players.Get (from);
                   if Assigned (playr.Data) then
                     playr.Data.Free;
                   playr.Data := TIdent.Create (p);
                 end;
               end;
        end;
        p.Free;
      until s = '';
    end;
  until err <> DP_OK;
{    if err <> DP_OK then
    Delay (100);}

  Inc (loop);
  if loop > 9 then
  begin
    Send (1, 0, id2);

    //Send $22.
    for i := 1 to numdrones do
    begin
      p := TIdent3.Create (drones [i], i);
      Send (1, 0, p);
      p.Free;
    end;

    for i := 1 to players.size do
    begin
      playr := players.getat (i - 1);
      p := TIdent3.Create (playr.id, i + numdrones);
      Send (1, 0, p);
      p.Free;
    end;

    for i := 1 to numdrones do
    begin
      s := save.players[i].status;
      p := TPacket.Create (@s[1], Length (s));
      id := TIdent.Create (p);
      id.pid := drones[i];

      // Unnecessary for the recorder to warn that the recorded is about to cheat :)
      id.InternVer := 0;  
      id.AllowWatch := true;
//      id.PermLos := true;
//      id.Cheat := players.size < 2;
      id.cheat := not nocheats;
//      if not nocheats then
//        id.cheat := true;
//      id.Comm := 0;

      Send (i, 0, id);
      id.Free;
      p.Free;
    end;

//      Send (0, id);
    loop := 0;

    allgo := 0;
    for i := 0 to players.Size - 1 do
    begin
      playr := players.GetAt (i);

{        if isdrone (playr.id) then
        continue;}

      case playr.synced of
        0  :begin
              Log.add ('ta-id : ', numdrones + i);
              p := TPacket.Create ($18, '$0' + HexToStr (numdrones + i, 1));
              Send (1, playr.id, p);
              p.Free;
              p := TPacket.Create ($1a, '$00$00$00$00$00$00$00$00$00$00$00$00$00');
              Send (1, playr.id, p);
              p.Free;
              playr.synced := 1;
//              playr.sent := 1;    //Total number of sent $1a.
            end;
        1  :begin
{              tmp := '';
              for j := 0 to units.units.count - 1 do
              begin
                pkt := units.Get (playr.curunit, false);
                tmp := tmp + PtrToStr (@pkt, Sizeof (pkt));
              end;

              p := TPacket.Create ($1a, @tmp[2], Length (tmp) - 1);
              Send (1, playr.id, p);
              p.Free;

              playr.curunit := 0;
              playr.sent := playr.sent + units.units.count;
              Say (1,'*** Sent all units - ' + inttostr (playr.sent));
              playr.synced := 2;}

              for j := 1 to options.SYNCSPEED*10 do
              begin
                pkt := units.Get (playr.curunit, false);
                p := TPacket.Create ($1a, @pkt.sub, sizeof (pkt) - 1);
                Send (1, playr.id, p);
                p.Free;
                Inc (playr.curunit);
                Inc (playr.sent);
                if playr.curunit = units.units.count then
                begin
                  playr.curunit := 0;
                  Say (1,'*** Sent all units - ' + inttostr (playr.sent));

                  playr.synced := 2;
                  break;
                end;
              end;
            end;
        2  :begin
              inc (playr.curunit);
              if playr.received = playr.sent then
                playr.Synced := 3;

              if playr.curunit > 10 then
              begin
                Say (1,'*** Current ack status: ' + inttostr (playr.received) + ' of ' + inttostr (playr.sent));
                say (1,'*** Attempting resync');
                playr.synced := 1;
                playr.sent := 1;
              end;
            end;
        3  :begin
              if playr.errors <> 0 then
              begin
                Say (1,'*** You have CRC errors on ' + inttostr (playr.errors) + ' units!');
              end;
              if playr.acked < units.units.count then
              begin
                Say(1,'*** You are missing ' + inttostr (units.units.count - playr.acked) + ' of ' + inttostr (units.units.count) + ' units!');
              end;

              playr.synced := 4;
              playr.curunit := 0;
            end;
        4  :begin
{              tmp := '';
              for j := 0 to units.units.count - 1 do
              begin
                pkt := units.Get (playr.curunit, true);
                tmp := tmp + PtrToStr (@pkt, Sizeof (pkt));
              end;

              p := TPacket.Create ($1a, @tmp[2], Length (tmp) - 1);
              Send (1, playr.id, p);
              p.Free;

              playr.curunit := 0;
              playr.sent := playr.sent + units.units.count;
              Say (1,'*** Sent ack on all units - ' + inttostr (playr.sent));
              playr.synced := 5;}

              for j := 1 to options.SYNCSPEED*10 do
              begin
                pkt := units.Get (playr.curunit, true);
                p := TPacket.Create ($1a, @pkt.sub, sizeof (pkt) - 1);
                Send (1, playr.id, p);
                p.Free;
                Inc (playr.curunit);
                Inc (playr.sent);
                if playr.curunit = units.units.count then
                begin
                  playr.curunit := 0;
                  Say(1,'*** Sent ack on all units - ' + inttostr (playr.sent));

                  playr.synced := 5;
                  playr.curunit := 0;
                  break;
                end;
              end; 
            end;
        5  :begin
              inc (playr.curunit);
              if playr.received = playr.sent then
                playr.Synced := 6;

              if playr.curunit > 10 then
              begin
                Say(1,'*** Current ack status: ' + inttostr (playr.received) + ' of ' + inttostr (playr.sent));
                Say (1,'*** Attempting resync 2');
                playr.synced := 4;
                playr.sent := units.units.count + 1;
              end;
            end;
        6  :begin
              Say(1,'*** Unit sync is complete');

              playr.synced := 7;
            end;
        7  :begin
              if playr.Data.Go then
                inc (allgo);
            end;
      end;

      if playr.Data.Watch and (not playr.warned) then
      begin
        Say (1,'Please consider joining as a regular player');
        Say (1,'instead of a watcher, since you are missing');
        Say (1,'many of the new features in this mode');
        playr.warned := true;
      end;
    end;


    if (allgo = players.size) and (players.size > 0) then
    begin
      for i := 1 to numdrones do
      begin
        s := save.players[i].status;
        p := TPacket.Create (@s[1], Length (s));
        id := TIdent.Create (p);
        id.pid := drones[i];

        id.AllowWatch := true;
        if not nocheats then begin
          id.PermLos := true;
          id.cheat := true;
        end;

  //      id.Cheat := players.size < 2;
//        id.cheat := true;
//        id.comm := 1;

        Send (i, 0, id);
        id.Free;
        p.Free;
      end;
      fmmain.button2click (self);
    end;

  end;

//  until quit;
end;

function TAServer.Launch :boolean;
type
  pt = ^tt;
  tt = array[1..4] of byte;
var
  p :PDPSessionDesc2;
  size :longword;
  tmp :pt;
  pack :TPacket;
  tmp2 :byte;

  no  :TPlayerNo;
  i   :integer;
  pl  :TPlayer;
//$6C$FF$FF$FF$20$45$6E$74$61$20$44$61$20$53$74$61$67$65$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$80$04$60$03$00$D1$97$57$01$01$00$01$00$00$80$00$05$00$00$00$00$00$0A$00$0A$00$00$00$03$01$3A$4E$08$38$00$00$00$00$00$00$00$00$00$00$00$00$24$D1$97$57$01$05
//$5F$FF$FF$FF$20$45$6E$74$61$20$44$61$20$53$74$61$67$65$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$80$04$60$03$00$D1$97$57$01$01$00$01$00$00$80$00$45$00$00$00$00$00$64$00$64$00$F4$01$03$01$3A$4E$08$38$00$00$00$00$00$00$00$00$00$00$00$00$24$D1$97$57$01$05

begin
  Result := false;

  if players.size = 0 then
  begin
    MessageDlg ('You must join the server with TA _before_ you press launch!', mtError, [mbok], 0);
    exit;
  end;

  for i := 0 to players.size - 1 do
  begin
    if players.GetAt (i).synced < 7 {units.Units.Count} then
    begin
      Say (1,'*** Please wait for sync'); {(' + inttostr (players.getat(i).synced) + '/' + inttostr (units.units.count) + ')');}
      exit;
    end;
  end;

  for i := 0 to players.size - 1 do
  begin
    pl := players.getat (i);
{    if not pl.data.Watch then
    begin
      pack := TChat.Create ('*** Please turn to Watch everyone');
      Send (1, 0, pack);
      pack.Free;
      exit;
    end;
}  end;

  mm.Enabled := false;

  no := TPlayerNo.Create (drones[1], 1);
  Send (1, 0, no);
  no.Free;

  for i := 2 to numdrones do
  begin
    no := TPlayerNo.Create (drones[i], i);
    Send (1, 0, no);
    no.Free;
  end;

  for i := 0 to players.Size - 1 do
  begin
    pl := players.GetAt (i);
    no := TPlayerNo.Create (pl.Id, i + numdrones + 1);
    Send (1, 0, no);
    no.Free;
  end;
  no := TPlayerNo.Create (drones[1], 1);
  Send (1, 0, no);
  no.Free;

  dp.GetSessionDesc (nil, size);
  GetMem (p, size);
  dp.GetSessionDesc (p, size);

  tmp := @p.dwUser1;
  tmp[3] := byte ('2');

  dp.SetSessionDesc (p^, 0);
  FreeMem (p, size);

  //Skicka launchSend launch. 
  tmp2 := $6;
  pack := TPacket.Create ($fa, @tmp2, 1);
  Send (1, 0, pack);
  pack := TPacket.Create ($8, @tmp2, 1);
  Send (1, 0, pack);

  pack.Free;
  Result := true;
end;

procedure TAServer.Load;
var
  i :integer;
begin
  loop := 0;
  done := false;
  quit := false;

  for i := 0 to players.Size - 1 do
    players.getat (i).synced := 0;

  mm.OnTimer := DoLoad;
  mm.Enabled := true;

  maxunits := save.maxunits;
  cont := save.cont;
  keepplayers := save.saveplayers;
  save.Free;
  cont.InitReplay;
end;

procedure TAServer.DoLoad (Sender :TObject);
var
  from, pto :TDPID;
  dsize     :cardinal;
  err       :HResult;
  p         :TPacket;
  p2        :TProgress;
  i         :integer;
  allgo     :boolean;
  s, cur    :string;

begin
  repeat
    dsize := 10000;
    err := dp.Receive (from, pto, 1, data, dsize);

    if err = DP_OK then
    begin
      if not isdrone (from) then
      begin
        s := PtrToStr (data, dsize);
        repeat
          cur := TPacket.Split (s);
          p := TPacket.Create (@cur[1], length (cur));

          case p.Kind of
            byte('*') : begin
                          p2 := TProgress.Create (p);
                          fmMain.lbLoading.Items.Add (IntToStr (p2.Percent));
                          p2.Free;
                        end;
//            $15,
            $2c        :begin
                          players.get (from).synced := 1;
                        end;
          end;
          p.Free;
        until s = '';
      end;
    end;
  until err <> DP_OK;
//  Delay (100);
  Inc (loop);

  if not quit then
  begin
    fmMain.pbLoading.Position := fmMain.pbLoading.Position + 5;
    if fmMain.pbLoading.Position = 100 then
    begin
      quit := true;
//      done := true;
    end;
  end;

  if loop mod 10 = 0 then
  begin
    for i := 1 to numdrones do
    begin
      p := TProgress.Create (fmMain.pbLoading.Position);
      Send (i, 0, p);
      p.Free;
    end;

    if quit then
    begin
      for i := 1 to numdrones do
      begin
        Log.Add ('Starting');
        p := TPacket.Create ($15, nil, 0);
        Send (i, 0, p);
        p.Free;
      end;

      for i := 0 to players.size - 1 do
      begin
        p := TPacket.Create ($1e, '$0' + HexToStr (i + numdrones + 1, 1));
        Send (1, players.GetAt (i).id, p);
        p.Free;
      end;

      done := true;
    end;
  end;

{    if done then
  begin
  end;}


  allgo := true;
  for i := 0 to players.size - 1 do
    if players.getat (i).synced < 1 then
      allgo := false;

  if done and quit and allgo then
  begin
    nextpulse := DateTimeToTimeStamp(now).Time;
    curtime := 0;

    lastcompack:=0;
    options.mod2cstatus:=0;
    mm.OnTimer := DoPlay;
    mm.Interval  := options.interval+10;

    fmMain.nbMain.Pageindex := 3;

    fmMain.lbEvents.Items.Add ('Starting playback..');
    fmMain.pbGame.Position := 0;
    fmMain.pbGame.Enabled := true;
    fmMain.pbGame.Frequency := cont.totalmoves div 20;
    fmMain.pbGame.Max := cont.totalmoves - 1;
    loop := 0;
    speed:=100;
    fillchar (Lastseen, sizeof (lastseen), 0);
    mm.enabled := true;
    paused := false;
    intake := false;
    donetake := false;
    oldnewtimer := fmMain.timemode.Checked;

    quit := false;    //To be used in DoPlay later. 

    //So we know what unit numbers they have. 
    players.sort;

  end;
//  until done and quit;

end;

function TAServer.GetDroneNr (pid :cardinal) :integer;
var
  i :integer;
begin
  for i := 1 to 9 do
    if droneinfo[i].orgpid = pid then
      break;

  Result := i;
end;

// Takes a raw packet. 
procedure TAServer.Filter (var s :string;a :integer);
var
  tmp :string;
  i,b   :integer;
  len :integer;
  w   :word;
  point :^word;
begin
  tmp := TPacket.Decompress (TPacket.Decrypt (s));
  s := #3#0#0 + Copy (tmp, 4, 4);
  i := 8;

  repeat
    len := TPacket.PacketLength (tmp, i);
    if (len > 0) then
    begin
      case byte(tmp[i]) of
        $2c,
        $09,
        $05,
        $06,
        $07,
        $11,
        $10,
        $12,
        $0a,
        $28,
        $19,
        $0d,
        $0b,
        $0f,


{        $14,}        //EVIL PACKET!!!

        $1f,
        $23,
        $16,
{        $1b, }       //This is evil too I tells ya.. :)
        $29,

        $0c :begin
               s := s + Copy (tmp, i, len);
//               Log.Add ('    Allowed : ' + DataToStr (@tmp[i], len));
             end;
      else
        Log.Add ('Not allowed : ' +DataToStr (@tmp[i], len));
      end;

      Inc (i, len)
    end else
      Log.Add ('Unknown : ' + DataToStr (@tmp[i], Length (tmp) - i + 1));
  until (len = 0) or (i > Length (tmp));

  if Length (s) > 7 then
  begin
//    s := TPacket.Encrypt (TPacket.Compress (s));
  end else
    s := '';
end;

procedure setword(p:pointer;w:word);
var
  wp     :^word;
begin
  wp:=p;
  wp^:=w;
end;

procedure setdword(p:pointer;d:dword);
var
  dp     :^dword;
begin
  dp:=p;
  dp^:=d;
end;

procedure TAServer.DoPlay (Sender :TObject);
var
  p :TPacket;
  r :TReplay;
  i, j,a,b :integer;
  last2c:cardinal;
  s,s2 :string;
  loopsleft:integer;
  move :PMove;
  cur :String;
  cmd :string;
  tmp :string;
  tal :integer;
  fel :integer;
  chat :TChat;
  name :PDpName;
  packtogo:integer;
  prevpack:cardinal;
  w :word;
  ta :TTake;
  holdstring :string;
  wp     :^word;
  ip     :^cardinal;
  pf              :^single;
  f,f2,f3,f4      :single;
  id :TIdent;
var
  from, pto :TDPID;
  dsize     :cardinal;
  err       :HResult;
begin
  log.add ('entering doplay');

  repeat
    dsize := 10000;
    err := dp.Receive (from, pto, 1, data, dsize);
    if err = DP_OK then
    begin
      if (pto = drones[1]) and (not isdrone (from)) then   //Pausen kommer ju till alla så det räcker med att kika för en
      begin
        s := PtrToStr (data, dsize);
        s := TPacket.Decompress (TPacket.Decrypt (s));
        s := Copy (s, 8, 10000);

        Log.Add ('processing input packets');
        repeat
          cur := TPacket.Split2 (s, false);
          case cur[1] of
            #$19 :begin   //pause
                    if cur[2] = #0 then
                    begin
                      if cur[3] = #1 then
                      begin
                        paused := true;
                        fmmain.lbEvents.Items.Add ('The playback is paused by user');
                      end;
                      if cur[3] = #0 then
                      begin
                        paused := false;
                        fmmain.lbEvents.Items.Add ('The playback is unpaused by user');
                        nextpulse := DateTimeToTimeStamp(now).Time;   //Annars hoppar det fram..
                      end;
                    end else paused:=false;
                  end;
            #$2c :begin
//                    if players.getat(0).id=from then begin
                      ip:=@cur[4];
                      lastplayerpack:=ip^;
                      if speed>100 then       //fungerar bara i 100% ökningar
                        options.mod2cstatus:=options.mod2cstatus+(speed-100) div 100;
  //                  end;
                  end;
            #$05 :begin  //chat
                    cmd := Copy (cur, 2, 100);
                    cmd := Trim (cmd);
                    cmd := Copy (cmd, Pos ('>', cmd) + 2, 100);

                    tmp := Copy (cmd, Pos (' ', cmd), 100);
                    tmp := Trim (tmp);
                    Val (tmp, tal, fel);
                    if fel = 0 then
                    begin
                      if Uppercase(Copy (cmd, 1, 6)) = '.SPEED' then
                      begin
                        if (tal > 5) and (tal < 1001) then begin
                          fmmain.tbSpeed.Position := 1000 - tal;
                          speed := tal;
                          Say(1,'Setting speed to ' + inttostr (tal) + '% of normal');
                        end;
                      end;

                      if Uppercase(Copy (cmd, 1, 4)) = '.POS' then
                      begin
                        if (tal >= 0) and (tal < 100) then
                        begin
                          Say(1,'Setting position to ' + inttostr (tal) + '% into the game');

                          tal := Round ((tal / 100) * fmMain.pbgame.max);
                          setpos (tal);
                          fmMain.pbgame.Position := tal;
                        end;
                      end;
                    end;

                    if Uppercase (cmd) = '.POS' then begin
                      Say(1,'You are at position ' + inttostr (loop) + ' of ' + inttostr (fmmain.pbgame.max) + ' (' + inttostr (Round ((loop / fmmain.pbgame.max) * 100)) + '%)');
                    end;

                    if Uppercase (copy (cmd, 1, 7)) = '.INCOME' then begin
                      for a:=1 to numdrones do begin
                        s2:=DroneInfo[a].name;
                        while(length(s2)<15) do
                          s2:=s2+' ';
                        Say(1,s2+' Metal: '+floattostrf(incomem[a],ffFixed,7,0)+' Energy: '+floattostrf(incomee[a],ffFixed,7,0)+' Shared M: '+floattostrf(lastsharedm[a],ffFixed,7,0));
                      end;
                    end;

                    if Uppercase (copy (cmd, 1, 6)) = '.TOTAL' then begin
                      for a:=1 to numdrones do begin
                        s2:=DroneInfo[a].name;
                        while(length(s2)<15) do
                          s2:=s2+' ';
                        Say(1,s2+' Metal: '+floattostrf((lasttotalm[a]-totalsharedm[a])/1000,ffFixed,7,0)+'K Energy: '+floattostrf((lasttotale[a]-totalsharede[a])/1000,ffFixed,7,0)+'K Shared M: '+floattostrf(totalsharedm[a]/1000,ffFixed,7,0)+'K');
                      end;
                    end;

                    if Uppercase (copy (cmd, 1, 6)) = '.SONAR' then begin
                      s2:=#9+'F'+#$00+'12'+#0#0#$50#$00#$00#$00#$02#$00#$00#$00#$90#$00#$00#$00#$E9+'t'+#$00#$00;
                      setword(@s2[2],units.units.count);
                      setword(@s2[4],unitrange[1]+maxunits-1);
                      setword(@s2[8],sonarcount*$a0+$50);
                      inc(sonarcount);
                      p:=Tpacket.sjcreatenew(s2);
                      Send (1, 0, p);
                      p.free;
                      s2:=#$11#$05#$00#$01;
                      setword(@s2[2],unitrange[1]+maxunits-1);
                      p:=Tpacket.sjcreatenew(s2);
                      Send (1, 0, p);
                      p.free;
                      s2:=#$14+'123456'+#$00#$00#$00#$00+'F'+#$01#$00#$00#$00#$00+'d'+#$7E#$00#$00#$00#$00#$00;
                      setword(@s2[2],unitrange[1]+maxunits-1);
                      setdword(@s2[4],from);
                      p:=Tpacket.sjcreatenew(s2);
                      Send (1, 0, p);
                      p.free;
                    end;

                    if Uppercase (Copy (cmd, 1, 5)) = '.TAKE' then
                    begin
                      for i := 1 to numdrones do
                      begin
                        if (Uppercase(trim(DroneInfo[i].Name)) = Uppercase (tmp)) then
                        begin
                          if taken[i] then
                          begin
                            Say(i, 'Player ' + droneinfo[i].name + ' is already claimed');
                            exit;
                          end;

                          if players.get (from).hastaken then
                          begin
                            Say(i, 'You can only take control of one player');
                            exit;
                          end;

                          for j := 1 to maxunits do
                          begin
                            ta := TTake.Create;
                            ta.source := i;
                            ta.dest := from;
                            ta.unitnr := unitrange [i] + j;

                            take.Add (ta);
//                            kill.add (pointer(unitrange[i] + j));
                          end;

//                          Say (i, 'Commander health: ' + inttostr(health[i].health[0]));
//                          Say (i, 'Unitnr            ' + inttostr (unitrange [i]));

                          Getmem (name, 1000);
                          name.dwSize := sizeof (name);
                          dsize := 1000;
                          Test(dp.GetPlayerName (from, name, dsize));
                          Say (i, 'Giving ' + droneinfo[i].name + ' to ' + name.lpszShortName);
                          FreeMem (name, 1000);

                          if not intake then
                            Say (i, 'Issue a .donetake when everyone is done with .take');
                          intake := true;
                          players.Get (from).hastaken := true;
                          taken[i] := true;
                          exit;
                        end;
                      end;

                      Say (1,'Use like this: ".take playername"');
                    end;

                    if Uppercase (copy (cmd, 1, 9)) = '.DONETAKE' then
                    begin
                      donetake := true;

                      log.add (' numwatchers is ' + inttostr (numwatchers));
                      log.add (' players.count  ' + inttostr (players.size));
                      if nocheats then begin
                        for i := 0 to players.size - 1 do
                        begin
                          for j := 2 to maxunits do
                          begin
                            a := (numdrones * maxunits) + (i * maxunits) + j;
                            kill.add (pointer(a));
                          end;
                          log.add( 'just added ' + inttostr (maxunits) + ' units');
                          log.add('kill.count is now ' + inttostr (kill.count));
                        end;
                      end;
                      for i := 1 to numdrones do
                      begin
                        //Lägg in ett reject-paket som behandlas sist
                        ta := TTake.Create;
                        ta.unitnr := -1;
                        ta.source := i;
                        take.Insert (0, ta);
                      end;

                      Say (1, 'Handing out stuff. Good luck all. :)');
                    end;

                    if (Uppercase (copy (cmd, 1, 5)) = '.USUK') and (fel = 0) then
                    begin
                      tmp := '';
                      for i := 1 to tal do
                      begin
                        tmp := tmp + #$2a + 'd';
                      end;

                      p := TPacket.SJCreateNew (tmp);

                      Say (1, 'Sending a string of length ' + inttostr (length (p.fdata)));
                      SendUdp (1, 0, p);

                      p.Free;
                    end;

                    if (Uppercase (copy (cmd, 1, 6)) = '.CHEAT') and (fel = 0) then
                    begin
                      for i := 1 to numdrones do
                      begin
                        s := save.players[i].status;
                        p := TPacket.Create (@s[1], Length (s));
                        id := TIdent.Create (p);
                        id.pid := drones[i];

                        id.AllowWatch := true;
                        id.PermLos := true;
                  //      id.Cheat := players.size < 2;
                        id.cheat := false;
//                        id.comm := 0;

                        Send (i, 0, id);
                        id.Free;
                        p.Free;
                      end;
                    end;

                  end;
          end;
        until s = '';
      end;
    end;
  until err <> DP_OK;

  if paused {or (intake and (not donetake))} then
    exit;
  PackToGo:=(lastplayerpack+options.mod2cstatus+60-lastcompack) div options.smooth;
  if packtogo=0 then
    inc(idletimes)
  else
    idletimes:=0;
  if idletimes=10 then begin
    idletimes:=0;
    inc(packtogo);  //fixa så att den inte låser sig vid exit
  end;
  loopsleft:=(numdrones*(options.interval+20)*speed) div 4000+1;
  while (((DateTimeToTimeStamp(now).Time > nextpulse ) and not fmMain.timemode.Checked) or ((packtogo>0) and fmMain.timemode.Checked)) and (not quit) and (not donetake) and (loopsleft>0) do
  begin //lång rad ovanför nödvändig för att fläta in newtimermode
//    move := save.Moves.Items[loop];
    dec(loopsleft);
    move := cont.GetMove (loop);
    if Assigned (move) then
    begin
      s := move.Data;
      prevpack:=lastdronepack[move.sender];

      if recentpos[move.sender] then begin
        recentpos[move.sender]:=false;
        s := TSaveFile.unsmartpak (s, cont.ver, @health[move.sender],lastdronepack[move.sender],false);
        possynccomplete[move.sender]:=lastdronepack[move.sender]+maxunits;
      end;
      if lastdronepack[move.sender]<possynccomplete[move.sender] then
        s := TSaveFile.unsmartpak (s, cont.ver, @health[move.sender],lastdronepack[move.sender],false)
      else
        s := TSaveFile.unsmartpak (s, cont.ver, @health[move.sender],lastdronepack[move.sender],true);

      s := s[1] + 'cc'+#$ff#$ff#$ff#$ff + Copy(s, 2, 30000);
      packtogo:=packtogo+prevpack-lastdronepack[move.sender];
      if fmMain.timemode.Checked then begin
        lastcompack:=$ffffffff;
        for i:=1 to numdrones do begin
          if (lastdronepack[i]<lastcompack) and (not keepplayers.killed [i]) and (not recentpos[i]) then
            lastcompack:=lastdronepack[i];
        end;
      end;

      if Length (s) > 7 then
      begin
        //Filtrera informationen
        s2 := Copy (s, 8, 10000);
        cur := #3#0#0 + Copy (s, 4, 4);
        repeat
          tmp := TPacket.Split2 (s2, false);

          case tmp[1] of
            #$19 :begin
                    if tmp[2] <> #00 then
                    begin
                      case byte(tmp[3]) of
                        0..9   :cmd := '-' + inttostr (10 - byte(tmp[3]));
                        10     :cmd := 'Normal';
                        11..30 :cmd := '+' + inttostr (byte (tmp[3]) - 10);
                      end;

                      tmp := '';
                      Say (1, DroneInfo[move.sender].name + ' set game speed to ' + cmd);
                      log.add (DroneInfo[move.sender].name + ' set game speed to ' + cmd);
                    end else
                    begin
                      if tmp[3] = #01 then
                      begin
                        Say (1, DroneInfo[move.sender].name + ' paused the game');
                        if options.skippause then begin
                          if fmMain.timemode.Checked then
                            tmp := '';
                        end else
                          fmmain.timemode.checked := false;
                      end else
                      begin
                        Say (1, DroneInfo[move.sender].name + ' unpaused the game');
                        if options.skippause then begin
                          if fmMain.timemode.Checked then
                            tmp := '';
                        end else
                          fmmain.timemode.checked := oldnewtimer;
                      end;
                    end;
                  end;
            #$1b :begin
                    ip := @tmp[2];
                    cmd := 'someone (??)';

                    i := GetDroneNr (ip^);

                    if not rejected [i] then
                    begin
                      cmd := droneinfo[i].name;
                      Say (1, DroneInfo[move.sender].name + ' rejected ' + cmd);
                      rejected [i] := true;
                    end;
                  end;
            #$05 :begin
                    if lastchat[move.sender] = Trim (tmp) then
                      tmp := ''
                    else
                      lastchat[move.sender] := Trim (tmp);
                  end;
            #$F9 :begin
                    if Length (tmp) > 70 then //första versionen av enemychat är kortare
                    begin
                      ip := @tmp [2];
                      i := GetDroneNr (ip^); //nu har vi vem som sa detta

                      cmd := Trim (Copy (tmp, 10, 100));
                      if lastchat [i] <> cmd then
                      begin
                        lastchat [i] := cmd;
                        if Pos ('<', cmd) > 0 then
                        begin
                          Cmd [Pos ('<', cmd)] := '[';
                          Cmd [Pos ('> ', cmd)] := ']';

                          Say (i, cmd);
                        end;
                      end;
                      tmp := '';
                    end;
                  end;
            #$28 :begin
                    a:=lastserial[move.sender]-laststat[move.sender];
                    if a<180 then
                      a:=120;
                    pf:=@tmp[47];
                    f:=pf^;
                    f3:=(f-lasttotalm[move.sender]);
                    if f3>0 then begin
                      lastsharedm[move.sender]:=sharedm[move.sender];
                      sharedm[move.sender]:=0;
                      incomem[move.sender]:= (f3-lastsharedm[move.sender]) / a * 30;
                      lasttotalm[move.sender]:=f;
                    end;
                    pf:=@tmp[35];
                    f:=pf^;
                    f3:=(f-lasttotale[move.sender]);
                    if f3>0 then begin
                      lastsharede[move.sender]:=sharede[move.sender];
                      sharede[move.sender]:=0;
                      incomee[move.sender]:= (f3-lastsharede[move.sender]) / a * 30;
                      lasttotale[move.sender]:=f;
                    end;
                    laststat[move.sender]:=lastserial[move.sender];
                    pf:=@tmp[19];
                    f:=pf^;
                    pf:=@tmp[23];
                    f2:=pf^;
                    pf:=@tmp[27];
                    f3:=pf^;
                    pf:=@tmp[31];
                    f4:=pf^;
                  end;
            #$16 :begin
                      ip := @tmp [10];
                      i := GetDroneNr (ip^); //nu har vi vem som detta var till
                      pf:=@tmp[14];
                      f:=pf^;
                      if tmp[2]=#2 then begin
//                        Say(1,DroneInfo[i].name + ' got '+floattostrf(f,ffFixed,6,0)+' metal');
                        sharedm[i]:=sharedm[i]+f;
                        totalsharedm[i]:=totalsharedm[i]+f;
                      end else begin
 //                       Say(1,DroneInfo[i].name + ' got '+floattostrf(f,ffFixed,6,0)+' energy');
                        sharede[i]:=sharede[i]+f;
                        totalsharede[i]:=totalsharede[i]+f;
                      end;
                  end;
            #$2c :begin
                    ip := @tmp [4];
                    lastserial[move.sender]:=ip^;
                  end;
{            #$20 :begin
                    tmp := '';
                    byte(tmp[158]):=byte(tmp[158]) and (not $20);
                    Say (1, 'changed cheat');
                  end;}
          end;
          cur := cur + tmp;
        until s2 = '';

        if length (cur) > 7 then
        begin
          s := cur;
          if fmMain.cbCompress.Checked then
            s := TPacket.Encrypt (TPacket.Compress (s))
          else
            s := TPacket.Encrypt (s);

          dp.Send (drones[move.Sender], 0, 0, s[1], Length (s));
          Lastseen[move.Sender] := 0;
        end;
      end;
    end;

    Inc (loop);
    if (loop = cont.totalmoves) or (not assigned (move)) then
    begin
      inpos := true;
      fmMain.pbGame.Position := loop;
      fmMain.pbGame.Enabled := false;
      inpos := false;

      Say(1,'Game over man.. Game over..');

      fmmain.lbEvents.Items.Add ('Killing remaining units.');

      //Rejecta alla watchers.
      for i := 0 to players.Size - 1 do
      begin
        j := players.getat (i).id;
        p := TPacket.Create ($1b, DataToStr (@j, 4) + '$06');
        SendUdp (1, j, p);
        p.Free;
      end;

      quit := true;
      exit;
    end;
//        dp.DestroyPlayer (drones [i]);

    if loop mod 100 = 0 then
    begin
      inpos := true;
      fmMain.pbGame.Position := loop;
      inpos := false;
    end;

    Nextpulse := nextpulse + ((move.time*100) div speed);
//    Dispose (move); // hmm dumt att göra detta
  end;

  log.add ('done sendloop');

  for i := 1 to GIVE_SPEED do
  begin
    if (take.count = 0) or (not donetake) or (kill.count > 0) then
      break;

    ta := take.items [take.count - 1];
    take.delete (take.count - 1);

    if ta.unitnr = -1 then
    begin
      holdstring:='1234'+#$06;
      ip:=@holdstring[1];
//      ip^:=takefrom;
      ip^ := drones[ta.source];
      p:=TPacket.Create($1b,@holdstring[1],length(holdstring));
      Send (ta.source, 0, p);
      p.free;
      holdstring:=#$1b+holdstring;
      p := TPacket.sjcreatenew (holdstring);
      Send (ta.source, 0, p);
      p.Free;
    end else
    begin
      holdstring:='123456';
      wp:=@holdstring[1];
  //    w:=takeunit+startid[convertid(takefrom)];
      wp^:= ta.unitnr;
      ip:=@holdstring[3];
      ip^:=ta.dest;
      holdstring:=#$14+holdstring+#$00#$00#$00#$00+'F'+#$01#$00#$00#$00#$00+'d'+#$7E#$00#$00#$00#$00#$00;

      setword(@holdstring[12],health[ta.source].health[ta.unitnr-1 - unitrange [ta.source]]);
      p:=TPacket.sjcreatenew(holdstring);
      Send (ta.source, ta.dest, p);
      p.Free;
    end;
  end;

//  else
//    say (1, 'nothing to kill');
  for i := 1 to KILL_SPEED do
  begin
    if (Kill.count = 0) then
      break;

    log.add ('killing stuff nr ' + inttostr (kill.count));

    j := integer(kill.Items [kill.Count - 1]);
    kill.Delete (kill.count - 1);
    w := j;
    if w>numdrones*maxunits then begin
      p := TPacket.Create ($0B, DataToStr (@w, 2) + '$01$00$3C$70$C1$01');
      Send (1, 0, p);
      p.Free;
    end else begin
      p := TPacket.Create ($0C, DataToStr (@w, 2) + '$FF$FF$FF$FF$00$00$64$13');
      Send (1, 0, p);
      p.Free;
    end;
  end;


  if quit and (kill.count = 0) then
  begin
    fmmain.lbEvents.Items.Add ('Playback is complete.');

    for i := 1 to numdrones do
    begin
      p := TPacket.Create ($29, '$01$00');
      SendUdp (i, 0, p);
      p.Free;
      p := TPacket.Create ($29, '$01$01');
      SendUdp (i, 0, p);
      p.Free;
    end;

    mm.enabled := false;
    exit;
  end;

  for i := 1 to numdrones do
  begin
    if keepplayers.TimetoDie [i] < KEEP_LIMIT then
      continue;

    //Fixa keep-alive
    Inc (Lastseen [i], 1);
    if lastseen[i] > 10*5 then     //5 sekunder
    begin
      p := TPacket.Create ($6, '$06');
      SendUdp (i, 0, p);
      p.Free;
      Lastseen[i] := 0;
    end;

    //Döda döda döda
    if (not keepplayers.Killed [i]) and (loop > keepplayers.timetodie[i]) then
    begin
      Say (i, DroneInfo[i].Name + ' sends no more packets in this recording. Killing..');

      for j := unitrange [i] + 1 to unitrange [i] + maxunits do
      begin
        Kill.Add (pointer (j));
      end;

      keepplayers.killed [i] := true;
    end;
  end;

  log.add ('done with doplay');
//  Inc (curtime, speed);

end;

procedure TAServer.SetPos (i :integer);
var
  move :PMove;
  j :integer;
  a :cardinal;
  s:string;
begin
  if not inpos then
  begin
    loop := i;

    if fmMain.timemode.Checked then begin
      j:=1;
      a:=$fffffffe;
      while j=1 do begin
        move := cont.GetMove (loop);
        s:=move.Data;
        TSaveFile.unsmartpak (s, cont.ver, @health[move.sender],a,true);
        if a<>$fffffffe then begin
          options.mod2cstatus:=a-lastplayerpack;
          j:=0;
          break;
        end;
        inc(loop);
      end;
      lastcompack:=a;
    end;

    for j := 1 to numdrones do
    begin
      recentpos[j]:=true;
      if keepplayers.timetodie [j] > loop then
        keepplayers.killed [j] := false;      //Så att de dödas korrekt igen
    end;
  end;
end;
end.
