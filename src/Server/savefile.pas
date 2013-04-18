unit savefile;

interface

uses
  Classes, Textdata, packet,logging, cstream, sysutils, dialogs;

const
  MEM_SIZE = 50000;

type
  TDemoPlayer = record
    Color :byte;
    Side :byte;
    Number :byte;
    Name :string;
    Status :string;
    orgpid :integer;
  end;

type
  TSavePlayers = record
    TimetoDie :array[1..10] of integer;   //Contains index on/over/of their last packet
    Killed    :array[1..10] of boolean;   //Used by the server. Really nice. Not.
  end;

type
  PMove = ^TMove;
  TMove = record
    Time :integer;
    Sender :byte;
    Data :string;
  end;

type
  TError = (sfNone, sfUnknown, sfOldVersion, sfNoMagic, sfNewVersion);

type
  PMark = ^TMark;
  TMark = record
    fileofs :integer;       //Var i filen..
    line    :integer;       //..man hittar denna rad
  end;

type
  PSaveHealth = ^TSaveHealth;
  TSaveHealth = record
    Maxunits :integer;
    health :array[0..5000] of integer;
  end;


type
  TContLoad = class
  protected
    inf :TFileStream;
    fname :string;
    Marks :TList;
    Moves :TList;
    curmark :integer;
    startline :integer;
    procedure InitReplay (mark :integer); overload;
  public
    ver :integer;
    totalmoves :integer;
    constructor Create (fn :string; version :integer);
    destructor Destroy; override;
    procedure InitReplay; overload;             //Kör innan första getmove
    function GetMove (index :integer) :PMove;   //Index ska vara globalt..
    procedure AddMark (fileofs, line :integer);
  end;

type
  TSaveFile = class
    Numplayers :byte;
    Map :string;
    Maxunits  :word;
    Players :array[1..10] of TDemoPlayer;
    orgpid  :array [1..10] of integer;
    version :integer;
    comments :string;
    chat :string;
//    Moves :TList;
    Units :string;
    VerName :string;
    Datum   :string;
    Playeradress :array[1..10] of string;
    RecFrom :string;
    fsize :string;

    Error :TError;
    cont :TContLoad;
    saveplayers :TSavePlayers;
    crc :longword;

    constructor Create (name :string; justinfo :boolean);
    destructor Destroy; override;
    procedure AddCommentSector (kommentar :string; name :string);
    class function MakeString (const base; const ofs; len :integer) :string;
    class function unsmartpak( c:string; version :integer; health :PSaveHealth;var last2c:cardinal;incnon2c:boolean):string;
   end;

implementation

uses
  tasv, lobby, selip;

type
  PHeader = ^RHeader;
  RHeader = packed record
    Length :word;
    Magic  :array [0..7] of char;
    Version :word;
    Numplayers :byte;
//    maxunits   :word;
    MapName :array[0..63] of char;
  end;

type
  PHeader2 = ^RHeader2;
  RHeader2 = packed record
    Length :word;
    Magic  :array [0..7] of char;
    Version :word;
    Numplayers :byte;
    maxunits   :word;
    MapName :array[0..63] of char;
  end;

type
  PExtraHeader = ^RExtraHeader;
  RExtraHeader = packed record
    Length :word;
    numsectors  : integer;
  end;

type
  PExtraSector = ^RExtraSector;
  RExtraSector = packed record
    Length     : word;
    sectortype : integer;
    data       : array[0..20000] of char;
  end;

type
  PPlayer = ^RPlayer;
  RPlayer = packed record
    Length :word;
    Color   :byte;
    Side    :byte;
    Number  :byte;
    Name    :array[0..63] of char;
  end;


type
  PStatus = ^RStatus;
  RStatus = packed record
    Length :word;
    Number  :byte;
    Status :array[0..511] of char;
  end;

type
  PUnitdata = ^RUnitdata;
  RUnitdata = packed record
    Length :word;
    Data    :array[0..10000] of char;
  end;

type
  PPacket = ^RPacket;
  RPacket = packed record
    Length :word;
    Time    :word;
    Sender  :byte;
    Data    :array[0..2048] of char;
  end;

class function Tsavefile.unsmartpak(c:string; version :integer; health :PSaveHealth;var last2c:cardinal;incnon2c:boolean):string;
var
   packnum      :cardinal;
   cpoint       :^cardinal;
   s,ut,tmp     :string;
   point        :pointer;
   w            :word;
   wp           :^word;
begin
  ut:='';
  c:=c[1]+'xx'+copy(c,2,10000);
  if c[1]=#$4 then
    c:=TPacket.decompress(c);
  c:=copy(c,4,10000);
  if version=3 then
    c:=copy(c,5,10000);
  repeat
    s:=TPacket.split2(c,true);
    case s[1] of
      #$fe    : begin
                  cpoint:=@s[2];
                  packnum:=cpoint^;
                  last2c:=packnum;
                end;
      #$ff    : begin
                  tmp:=','+#$0b+#$00+'xxxx'+#$ff#$ff#$01#$00;
                  cpoint:=@tmp[4];
                  cpoint^:=packnum;
                  inc(packnum);
                  last2c:=packnum;
                  ut:=ut+tmp;
                end;
      #$fd     : begin
                  tmp:=copy(s,1,3)+'zzzz'+copy(s,4,10000);
                  cpoint:=@tmp[4];
                  cpoint^:=packnum;

                  wp := @tmp[8];
                  if wp^ = $ffff then
                  begin
                    health.health[packnum mod health.Maxunits] := BinToInt (Copy (tmp, 11, 4), 2, 16);
                  end;

                  inc(packnum);
                  last2c:=packnum;
                  tmp[1]:=#$2c;
                  ut:=ut+tmp;
                 end;
      #$14     : begin end;
      #$2c     :begin
                  cpoint := @s[4];
                  last2c := cpoint^;
                end;

      else       begin
                   if incnon2c then
                     ut:=ut+s;
                 end;
      end;
   until c='';
   result:=#3+ut;
end;

constructor TSaveFile.Create (name :String; justinfo :boolean);
var
  inf    :TFileStream;
  s      :string;
  i,a      :integer;
  h      :PHeader2;
  player :PPlayer;
  stat   :PStatus;
  units  :PUnitdata;
  move   :PPacket;
  move2  :PMove;
  eh     :PExtraHeader;
  es     :PExtraSector;
  sectors:integer;
  ano :TPlayers;
  p :TPacket;
  id :TIdent;
  curmem :integer;
  fileofs :integer;
  curcrc :longword;

  //Läser först längden från filen. som är ett word..
  function ReadRec (var p) :boolean;
  type
    PBuf = ^TBuf;
    TBuf = packed record
      len :word;
      data :byte;
    end;
  var
    len :word;
    pt  :pointer;
  begin
    Result := inf.Read (len, Sizeof (len)) = Sizeof (len);
    if not Result then exit;
    GetMem (pt, len);
    Result := inf.Read (PBuf(pt)^.data, Len - sizeof (len)) = len - sizeof (len);
    PBuf(pt)^.Len := Len;

    curcrc := CalcCRC (curcrc, pt, len);
    System.Move (pt, p, Sizeof (pt));
  end;

begin
  Error := sfNone;
  curcrc := 0;

//  moves := nil;

  inf := TFileStream.Create (name, fmOpenRead);

  if inf.Size < 1000000 then
    fsize := IntToStr (inf.size div 1000) + ' kb'
  else
  begin
    fsize := IntToStr (inf.size div 100000) + ' mb';
    insert ('.', fsize, length (fsize) - 3);
  end;

  //Läs in headern
  if not ReadRec (h) then
  begin
    Error := sfUnknown;
    inf.Free;
    exit;
  end;

  s := 'TA Demo';
  for i := 1 to 7 do
    if s[i] <> h.Magic[i-1] then
    begin
      Error := sfNoMagic;
      inf.Free;
      exit;
    end;
  version:=h.version;
  if h.Version > 5 then
  begin
    Error := sfNewVersion;
    inf.Free;
    exit;
  end;

  if h.Version < 3 then
  begin
    Error := sfOldVersion;
    inf.Free;
    exit;
  end;

  case h.Version of
    3 :VerName := '0.80ß';
    4 :VerName := '0.81a';
    5 :VerName := '0.90ß';
  else
    VerName := 'Unknown (' + inttostr (h.version) + ')';
  end;

  comments:='';
  chat := '';
  Datum:='';
  for a:=1 to 10 do
    playeradress[a]:='';
  a:=1;
  if h.Version > 4 then begin
    ReadRec (eh);
    sectors:=eh.numsectors;
    freemem(eh);
    if sectors>0 then
      for i:=1 to sectors do begin
        ReadRec (es);
        if es.sectortype=1 then
        begin
          if comments <> '' then
            comments := comments + #13#10'*'#13#10;
          comments := comments+MakeString (es^, es.data, es.length);
        end;

        if es.sectortype=2 then
        begin
          chat := MakeString (es^, es.data, es.length);
        end;

        if es.sectortype=3 then
        begin
          VerName := MakeString (es^, es.data, es.length);
        end;

        if es.sectortype=4 then
        begin
          Datum := MakeString (es^, es.data, es.length);
        end;

        if es.sectortype=5 then
        begin
          RecFrom := MakeString (es^, es.data, es.length);
        end;

        if es.sectortype=6 then
        begin
          playeradress[a] := SimpleCrypt (MakeString (es^, es.data, es.length));

          playeradress [a] := fmSelIP.GetAddress (playeradress [a]);
          inc(a);
        end;
      end;
  end;
  NumPlayers := h.Numplayers;
  MaxUnits := h.MaxUnits;
  Map := MakeString (h^, h.MapName, h.Length);
  FreeMem (h, h.Length);

  //Läs in spelardata
  for i := 1 to Numplayers do
  begin
    ReadRec (player);

    Players [i].Color := player.color;
    Players [i].Side := player.side;
    Players [i].Number := player.number;
    Players [i].Name := MakeString (player^, player.name, player.length);

    FreeMem (player, player.length);
  end;

  //Läs in spelarstatus
  for i := 1 to NumPlayers do
  begin
    ReadRec (stat);
    Players [i].Status := MakeString (stat^, stat.status, stat.length);

    s := players[i].status;
    p := TPacket.Create (@s[1], Length (s));
    id := TIdent.Create (p);
    players [i].orgpid := id.pid;
    id.Free;
    p.Free;

    Freemem (stat, stat.length);
  end;

  //Så här långt är crcn beräknad
  crc := curcrc;

  //Läs in units
  ReadRec (units);
  Self.Units := MakeString (units^, units.data, units.length);
  FreeMem (units, units.length);

  //Nu har vi nödvändig info
  if justinfo = true then
  begin
    Error := sfNone;
    inf.Free;
    exit;
  end;


  //Läs in själva spelet
//  Moves := Tlist.Create;

  i := 0;
  curmem := 0;
  fillchar (saveplayers, sizeof (saveplayers), 0);
  fileofs := inf.Position;
  cont := TContLoad.Create (name, version);

  cont.AddMark (fileofs, 0);

  while ReadRec (move) do
  begin
    inc (curmem, move.length);
    Inc (fileofs, move.length);

    if curmem > MEM_SIZE then //Dags för en ny stolpe
    begin
      cont.AddMark (fileofs, i);
      curmem := 0;
    end;

    if (move.sender > 10) or (move.sender < 1) then
//      showmessage ('Very odd')
    else
      saveplayers.timetodie [move.sender] := i;
    FreeMem (move, move^.length);
    Inc (i);
  end;

  cont.totalmoves := i;
  cont.curmark := cont.Marks.count - 1;
  cont.addmark (fileofs, i - 1);
  cont.addmark (fileofs, i);      //eof-marker typ

  //Fixar till ORG-PID
  ano := TPlayers.Create;
  for i := 1 to numplayers do
  begin
    s := players[i].status;
    p := TPacket.Create (@s[1], Length (s));
    id := TIdent.Create (p);
    ano.add (id.pid);
    ano.getat (i-1).synced := i;
  end;
  ano.Sort;

  for i := 1 to numplayers do
  begin
    orgpid [i] := ano.getat (i-1).synced;
//    players[i].orgpid := ano.getat (i-1).id;
  end;

  //Stäng filen och markera success
  inf.Free;
  Error := sfNone;
end;

destructor TSaveFile.Destroy;
var
  i :integer;
  p :PMove;
begin
{  if assigned (moves) then
  begin
    for i := 0 to moves.count - 1 do
    begin
      p := moves.items[i];
      Dispose (p);
    end;

    moves.Free;
  end;}

  inherited Destroy;
end;

procedure TSaveFile.AddCommentSector (kommentar :string; name :string);
var
  inf    :TFileStream;
  unf    :TFileStream;
  s      :string;
  i      :integer;
  h      :PHeader2;
  move   :PPacket;
  eh     :PExtraHeader;
  es     :PExtraSector;
  sectors:integer;
  id     :TIdent;

  function ReadRec (var p) :boolean;
  type
    PBuf = ^TBuf;
    TBuf = packed record
      len :word;
      data :byte;
    end;
  var
    len :word;
    pt  :pointer;
  begin
    Result := inf.Read (len, Sizeof (len)) = Sizeof (len);
    if not Result then exit;
    GetMem (pt, len);
    Result := inf.Read (PBuf(pt)^.data, Len - sizeof (len)) = len - sizeof (len);
    PBuf(pt)^.Len := Len;

    System.Move (pt, p, Sizeof (pt));
  end;
begin
  inf := TFileStream.Create (name, fmOpenRead);
  unf := TFileStream.Create (name+'.tmp',fmCreate);

  //Läs in headern
  ReadRec (h);

  s := 'TA Demo';
  for i := 1 to 7 do
    if s[i] <> h.Magic[i-1] then
    begin
      Error := sfNoMagic;
      inf.Free;
      unf.Free;
      DeleteFile(name+'.tmp');
      exit;
    end;

  version:=h.version;
  if h.Version > 5 then
  begin
    Error := sfNewVersion;
    inf.Free;
    unf.Free;
    DeleteFile(name+'.tmp');
    exit;
  end;

  if h.Version < 4 then
  begin
    Error := sfOldVersion;
    inf.Free;
    unf.Free;
    DeleteFile(name+'.tmp');
    exit;
  end;
  if h.version=4 then
    h.version:=5;
  unf.write(h.length,h.length);

  if version=4 then begin
    s:=#6#0#1#0#0#0;
    unf.write(s[1],length(s));
  end else begin
    readrec(eh);
    eh.numsectors:=eh.numsectors+1;
    unf.write(eh.length,eh.length);
    if eh.numsectors>1 then begin
      for i:=2 to eh.numsectors do begin
        ReadRec (move);
        unf.Write(move.length,move.length);
        FreeMem (move, move^.length);
      end;
    end;
    freemem(eh);
  end;

  s:=#0#0;
  s:=s+#1#0#0#0;
  s:=s+kommentar;
  s[1]:=char(length(s) and $ff);     //fyll i storlek
  s[2]:=char(length(s) shr 8);
  unf.write(s[1],length(s));

  while ReadRec (move) do
  begin
    unf.Write(move.length,move.length);
    FreeMem (move, move^.length);
  end;

  //Stäng filen och markera success
  inf.Free;
  unf.free;
  DeleteFile(name);
  RenameFile(name+'.tmp',name);
end;

class function TSaveFile.MakeString (const base; const ofs; len :integer) :string;
var
  pb, po :pointer;
  olen  :integer;
begin
  pb := Addr (base);
  po := Addr (ofs);

  olen := longword(po) - longword(pb);
  Result := PtrToStr (po, len - olen);
end;

{--------------------------------------------------------------------}

constructor TContLoad.Create (fn :string; version :integer);
begin
  inherited Create;

  Marks := TList.Create;
  Moves := TList.Create;
  ver := version;

//  inf := TFileStream.Create (fn, fmOpenRead);
  fname := fn;
end;

destructor TContLoad.Destroy;
var
  i :integer;
begin
  for i := 0 to marks.count - 1 do
    Dispose (marks.items[i]);
  for i := 0 to moves.count - 1 do
    Dispose (moves.items[i]);

  marks.free;
  moves.free;
  if Assigned (inf) then
    inf.Free;

  inherited Destroy;
end;

procedure TContLoad.AddMark (fileofs, line :integer);
var
  mark :PMark;
begin
  New (mark);

  mark.fileofs := fileofs;
  mark.line := line;

  Marks.Add (mark);
end;

procedure TContLoad.InitReplay;
begin
  inf := TFileStream.Create (fname, fmOpenRead);
  InitReplay (0);
end;

procedure TContLoad.InitReplay (mark :integer);
var
  i :integer;
  m :PMark;
  move   :PPacket;
  move2  :PMove;


  function ReadRec (var p) :boolean;
  type
    PBuf = ^TBuf;
    TBuf = packed record
      len :word;
      data :byte;
    end;
  var
    len :word;
    pt  :pointer;
  begin
    Result := inf.Read (len, Sizeof (len)) = Sizeof (len);
    if not Result then exit;
    GetMem (pt, len);
    Result := inf.Read (PBuf(pt)^.data, Len - sizeof (len)) = len - sizeof (len);
    PBuf(pt)^.Len := Len;

    System.Move (pt, p, Sizeof (pt));
  end;


begin
  for i := moves.count - 1 downto 0 do
  begin
    move2 := moves.items[i];
    moves.Delete (i);
    Dispose (move2);
  end;
//  getheapstatus.totalallocated

  if moves.count <> 0 then
    raise Exception.Create ('cp');

{  moves.Free;
  moves := TList.Create;}

  m := marks.items[mark];

  inf.Seek (m.fileofs, soFromBeginning);
  for i := m.line to PMark (marks.items[mark+1]).line - 1 do
  begin
    if not ReadRec (move) then
      raise Exception.Create ('Error reading from demo file?');

    New (move2);

    move2.time := move.time;
    move2.Sender := move.sender;
    move2.data := TSaveFile.MakeString (move^, move.data, move.length);

{    move2.data:=TSaveFile.unsmartpak(move2.data, ver);

    //Tryck in crc-plats
    move2.Data := move2.data[1] + 'cc'+#$ff#$ff#$ff#$ff + Copy(move2.Data, 2, 30000);
    move2.Data := TPacket.Encrypt (move2.Data);}

    Moves.Add (move2);
//    Dispose (move2);
    FreeMem (move, move^.length);
  end;

  curmark := mark;
  startline := m.line;
end;

function TContLoad.GetMove (index :integer) :PMove;
var
  i :integer;
  found :boolean;
begin
  Result := nil;
  if index >= totalmoves then
    exit;

  if not ((index >= PMark(Marks.items[curmark]).line) and (index < PMark(Marks.items[curmark+1]).line)) then //Aha vi är utanför
  begin
    found := false;
    for i := 0 to marks.count - 3 do
    begin
      if (pmark(marks.items[i]).line <= index) and (pmark(marks.items[i+1]).line > index) then
      begin
        InitReplay (i);
        found :=true;
        break;
      end;
    end;

    if not found then
      exit;
  end;

  Result := Moves.Items[index - startline];
end;

end.
