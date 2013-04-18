unit lobby;

interface

uses
  packet, dplay, textdata, logging, math, dialogs, sysutils;

{ $ 2 0}
type
  TIdent = class (TPacket)
  protected
    function GetPid :TDPID;
    procedure SetPid (pid :TDPID);
    procedure SetAllowWatch (b :boolean);
    procedure SetCheat (b :boolean);
    procedure SetPermlos (b :boolean);
    procedure SetWatch (b :boolean);
    function GetWatch :Boolean;
    function GetGo :boolean;
    procedure SetComm (i :integer);
    procedure SetIntern (i :integer);
  public
    constructor Create (pid :TDPID); overload;

    property pid :TDPID read GetPid write SetPid;
    property AllowWatch :boolean write SetAllowWatch;
    property Cheat :boolean write SetCheat;
    property Permlos :boolean write SetPermLos;
    property Watch :boolean read GetWatch write SetWatch;
    property Go :boolean read GetGo;
    property Comm :integer write SetComm;
    property InternVer :integer write SetIntern;
  end;

{ (eller är det '&') jepp. $26 alltså}
type
  TIdent2 = class (TPacket)
    constructor Create (players :array of TDPID); overload;
    procedure Add (id :TDPID);
    procedure Remove (id :TDPID);
  end;

{ $22}
type
  TIdent3 = class (TPacket)
    constructor Create (player :TDPID; nr :integer); overload;
  end;

{ $5}
type
  TChat = class (TPacket)
  protected
    function GetMsg :string;
    procedure SetMsg (s :String);
  public
    constructor Create (Msg :string); overload;

    property Msg :string read GetMsg write SetMsg;
  end;

{ $2}
type
  TPing = class (TPacket)
  protected
    function GetId :longword;
    function GetFrom :TDPID;
    function GetPing :longword;
    procedure SetId (l :longword);
    procedure SetFrom (p :TDPID);
    procedure SetPing (l :longword);
  public
    constructor Create (from :TDPID; id, ping :longword); overload;

    property Id :longword read GetId write SetId;
    property From :TDPID read GetFrom write SetFrom;
    property Ping :longword read GetPing write SetPing;
  end;

{"}
type
  TPlayerNo = class (TPacket)
    constructor Create (id :TDPID; number :word); overload;
  end;

type
  TUnit = class (TPacket)
  protected
    function getsubtype :byte;
    function getid :longword;
    function getstatus :word;
    function GetLimit :word;
    procedure SetLimit (l :word);
    function GetCrc :longword;
  public
    constructor Create (subtype :byte; id, crc :longword); overload;
    property subtype :byte read GetSubtype;
    property id :longword read Getid;
    property crc :longword read GetCrc;
    property status :word read getstatus;
    property Limit :word read GetLimit write SetLimit;
  end;

implementation

{+ idFrom       : 759317 $0b $96 $15}
{ + lpData       : þÿÿÿ$20
$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00
140*0
$04$00$03$00
Skärmsize
$15–$0B$00
player-id
$01$00ÿ$00$00€$00
7*data
$02 - not clicked in (" = clicked in)
1*data
$00$00$00$00$00$00$00$00$00$00$00
9*0
d$00
2*data
$03$01
version
$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$$
17*data
$15–$0B$00
player-id
$05
1*data}

type
  PIdent = ^RIdent;
  RIdent = packed record
    Fill1   :array[1..139] of byte;        //0-138
    Width   :word;                         //139-140
    Height  :word;      //Skärmen          //141-142
    Fill3   :byte;                         //143
    Player1 :TDPID;                        //144-152
    Data2   :array[1..7] of byte;          //153-160
    Clicked :byte;                         //161
    Fill2   :array[1..9] of byte;          //162-171
    Data5   :word;                         //172-173
    Hiver   :byte;                         //174
    Lover   :byte;                         //175
    Data3   :array[1..17] of byte;         //176-192  7=182
    Player2 :TDPID;
    Data4   :byte;
  end;

const
  click_ut = $02 + 128 ;
//  click_in = byte('"') + 128;
  click_in = byte ('‚');

constructor TIdent.Create (pid :TDPID);
var
  r :RIdent;
  s :string;
  p :pointer;
begin
  FillChar (r, sizeof (r), 0);


{  s := 'Ror Shock$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00' +
                            '$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00' +
                            '$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$04$00$03$00lŽZ$00$01$00$00$01$00€$00"$0E$04$00$00$00$0A$00$0A$00ô$01$03$01—GD$06$00$00$00$00$00$00$00$00$00$00$00$00$$lŽZ$00$05';}

{  s := 'Ror Shock$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00' +
       '$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00' +
       '$00$00$04$00$03$00kµr$01$01$00$00$01$00€$00¢($04$00$00$00$0A$00$0A$00ô$01$03$01—GD$06$00$00$00$00$00$00$00$00$00$00$00$00$$kµr$01$05';}

   s := 'Ror Shock$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00' +
        '$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$04' +
        '$00$03$00yX+$00$01$00$00$01$00€$00²h$04$00$00$00$0A$00$0A$00ô$01$03$01—GD$06$00$00$00$00$00$00$00$00$00$00$00$00$$yX+$00$05';

//  s := 'Ashap Plateau$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00' + '$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00' + '$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00€$02à$01$00Ùþ$01$00$01$00$00$01$00`$00‚O$04$00$00$00$0A$00$0A$00ú$00$03$01Ñ³/É$00$00$00$00$00$00$00$00$00$00$00$00$$Ùþ$01$00$05';
  p := StrToData (s);

  Move (p^, r, Sizeof (r));
//  StrData ('$01$00ÿ$00$00€$00', r.Data2);
//  StrData ('d$00', r.Data5);
//  StrData ('$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$$', r.Data3);
//  r.Data4 := 5;

  r.Player1 := pid;
  r.Player2 := pid;
  r.Width := 500;
  r.Height := 500;
  r.Hiver := 3;
  r.Lover := 1;
//  r.Clicked := click_in;

  inherited Create ($20, @r, Sizeof (r));

  FreeMem (p, Datasize (s));
end;

function TIdent.GetPid :TDPID;
begin
  Result := PIdent (@Fdata [9])^.player1;
end;

procedure TIdent.SetPid (pid :TDPID);
begin
  PIdent (@Fdata [9])^.Player2 := pid;
  PIdent (@Fdata [9])^.Player1 := pid;
end;

procedure TIdent.SetAllowWatch (b :boolean);
begin
  if b then
    byte(FData[164]) := byte(FData[164]) or $80
  else
    byte(FData[164]) := byte(FData[164]) and (not $80);
end;

procedure TIdent.SetCheat (b :boolean);
begin
  if b then
    byte(FData[165]):=byte(FData[165]) or $20
  else
    byte(FData[165]):=byte(FData[165]) and (not $20);
end;

procedure TIdent.SetPermLos (b :boolean);
begin
  byte(FData[165]):=$08; //hmm lite fult
  {

  if b then
    byte(FData[165]):=byte(FData[165]) or $08
  else
    byte(FData[165]):=byte(FData[165]) and (not $08); }
end;

procedure TIdent.SetWatch (b :boolean);
begin
  byte(FData[164]) := byte(FData[164]) or $40;
end;

function TIdent.GetWatch :Boolean;
begin
  try
    Result := (byte(FData[164]) and $40) <> 0;
  except
    Result := false;
  end;

//  Log.Add (fdata[161] + fdata [162] + fdata[163] + fdata[164] + fdata[165]);
end;

procedure TIdent.SetComm (i :integer);
begin
  case i of
    0  :begin //ends
          byte(FData[165]) := byte(FData[165]) or 8;
          byte(FData[165]) := byte(FData[165]) and (not 16);
        end;
    1  :begin //cont
          byte(FData[165]) := byte(FData[165]) and (not 8);
          byte(FData[165]) := byte(FData[165]) and (not 16);
        end;
    2  :begin //death
          byte(FData[165]) := byte(FData[165]) or 8;
          byte(FData[165]) := byte(FData[165]) or 16;
        end;
  end;
//  byte (FData[164]) := $ff;
end;

procedure TIdent.SetIntern(i :integer);
var
  a :integer;
begin
  byte(fdata[189]) := 0;
end;

function TIdent.GetGo :boolean;
begin
  Result := (byte(FData[164]) and $20) <> 0;
//  Log.Add (fdata[155] + fdata [156] + fdata[157] + fdata[158] + fdata[159] + fdata[160]);
  Log.Add (fdata[161] + fdata [162] + fdata[163] + fdata[164] + fdata[165]);
end;

{--------------------------------------------------------------------}

type
  PIdent2 = ^RIdent2;
  RIdent2 = packed record
    players :array[0..9] of TDPID;
  end;

{s := '&
      lŽZ$00
      nŽZ$00
      $00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00}

constructor TIdent2.Create (players :array of TDPID);
var
  r :RIdent2;
  i :integer;
begin
  FillChar (r, sizeof (r), 0);

  for i := 0 to High (players) do
  begin
    r.players [i] := players[i];
  end;

  inherited Create ($26, @r, sizeof (r));
end;

procedure TIdent2.Add (id :TDPID);
var
  i :integer;
begin
  for i := 0 to 9 do
  begin
    if PIdent2 (@Fdata[9])^.players[i] = 0 then
    begin
      PIdent2 (@FData[9])^.players[i] := id;
      exit;
    end;
  end;
end;

procedure TIdent2.Remove (id :TDPID);
var
  i :integer;
begin
  for i := 0 to 9 do
  begin
    if PIdent2 (@Fdata[9])^.players[i] = id then
    begin
      PIdent2 (@FData[9])^.players[i] := 0;
      exit;
    end;
  end;
end;

{--------------------------------------------------------------------}

type
  PIdent3 = ^RIdent3;
  RIdent3 = packed record
    Pid :TDPID;
    nr  :byte;
  end;

constructor TIdent3.Create (player :TDPID; nr :integer);
var
  r :Rident3;
begin
  r.pid := player;
  r.nr := nr;
  inherited Create ($22, @r, sizeof (r));
end;

{--------------------------------------------------------------------}

{<Fnordia> sync$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00}

type
  PChat = ^RChat;
  RChat = packed record
    Msg :array [0..63] of char;
  end;

constructor TChat.Create (Msg :string);
var
  r :RChat;
  l :integer;
begin
  l := Min (Length (msg), 64);
  FillChar (r, sizeof (r), 0);
  Move (msg [1], r.Msg, l);
  inherited Create ($5, @r, Sizeof (r));
end;

function TChat.GetMsg :string;
var
  p :PChar;
begin
  p := PChat (@Fdata [9])^.Msg;
  Result := p;
end;

procedure TChat.SetMsg (s :String);
var
  r :RChat;
begin
  FillChar (r, sizeof (r), 0);
  Move (s[1], r.Msg, Length (msg));
  Move (r, FData [9], sizeof (r));
end;

{--------------------------------------------------------------------}

type
  PPing = ^RPing;
  RPing = packed record
    Id   :longword;
    Ping :longword;
    From :TDPID;
  end;

constructor TPing.Create (from :TDPID; id, ping :longword);
var
  r :RPing;
begin
  FillChar (r, sizeof (r), 0);
  r.from := from;
  r.id := id;
  r.ping := ping;

  inherited Create ($2, @r, Sizeof (r));
end;

function TPing.GetId :longword;
begin
  Result := PPing (@Fdata [9])^.id;
end;

function TPing.GetFrom :TDPID;
begin
  Result := PPing (@Fdata [9])^.From;
end;

function TPing.GetPing :longword;
begin
  Result := PPing (@Fdata [9])^.Ping;
end;

procedure TPing.SetId (l :longword);
begin
  PPing (@FData [9])^.Id := l;
end;

procedure TPing.SetFrom (p :TDPID);
begin
  PPing (@FData [9])^.Id := p;
end;

procedure TPing.SetPing (l :longword);
begin
  PPing (@FData [9])^.Id := l;
end;


{--------------------------------------------------------------------}

type
  RPlayerNo = packed record
    id  :TDPID;
    nr :byte;
  end;

constructor TPlayerNo.Create (id :TDPID; number :word);
var
  r :RPlayerNo;
begin
  r.id := id;
  r.nr := number;

  inherited Create (byte('"'), @r, sizeof (r));
end;

{--------------------------------------------------------------------}

type
  PUnit = ^RUnit;
  RUnit = packed record
    sub :byte;
    Fill :longword;
    Id   :longword;
    case integer of
      0 : (nr1 :word; nr2 :word);
      1 : (crc :longword);
  end;

constructor TUnit.Create (subtype :byte; id, crc :longword);
var
  r :RUnit;
begin
  r.sub := subtype;
  r.fill := 0;
  r.Id := id;
  r.nr1 := crc and $0000ffff;
  r.nr2 := (crc and $ffff0000) shr 16;

  inherited Create ($1a, @r, sizeof (r));
end;

function TUnit.GetSubtype :byte;
begin
  Result := PUnit (@Fdata [9])^.sub;
end;

function TUnit.getId :longword;
begin
  Result := PUnit (@Fdata [9])^.id;
end;

function TUnit.getstatus :word;
begin
  Result := PUnit (@Fdata [9])^.nr1;
end;

function TUnit.GetLimit :word;
begin
  Result := PUnit (@Fdata [9])^.nr2;
end;

procedure TUnit.SetLimit (l :word);
begin
  PUnit (@Fdata [9])^.nr2 := l;
end;

function TUnit.GetCrc :longword;
begin
  result := PUnit (@Fdata [9])^.crc;
end;

end.
