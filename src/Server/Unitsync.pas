unit unitsync;

interface

uses
  Classes, lobby, dialogs, SysUtils, textdata, Logging, packet;

type
  PUnitPkt = ^TUnitPkt;
  TUnitPkt = packed record
    pktid :byte;
    sub :byte;
    fill :longword;
    id :longword;
    case integer of
      0 : (status :word; limit :word; );
      1 : (crc :longword);
  end;

type
  PRawUnit = ^TRawUnit;
  TRawUnit = record
    Id :longword;
    Crc :longword;
    limit :word;
    inuse :boolean;
  end;

type
  TUnitSync = class
    Units :TList;
    crc :longword;

    constructor Create (name :string);
    destructor Destroy; override;
    procedure Compact;
    procedure Add (u :TUnitPkt);
    function Get (i :integer; use :boolean) :TUnitPkt;
    function Find (id :longword) :PRawUnit;
  end;

implementation

uses
  replay, main;

constructor TUnitSync.Create (name :string);
type
  plong = ^longword;
var
  s :string;
  u :TUnitPkt;
  i :integer;
  cur :string;
  p :PRawUnit;
begin
  Units := TList.Create;

  i := 1;
  while i < Length (name) do
  begin
    cur := Copy (name, i, 14);
    Move (cur[1], u, Sizeof (u));
    Add (u);
    Inc (i, 14);
  end;

{  u.pktid := $1a;
  u.pktid := $3;
  u.fill := 0;
  u.id := $ebc53551;
  u.status := $0101;
  u.limit := 5;}

  New (p);

  p.Id := SY_UNIT;
  p.Crc := 0;
  p.limit := 100;
  p.inuse := true;

  Units.Add (p);
end;

destructor TUnitSync.Destroy;
var
  i :integer;
begin
  for i := 0 to units.count - 1 do
    Dispose(units.Items [i]);
  units.Free;

  inherited Destroy;
end;

procedure TUnitsync.Add (u :TUnitPkt);
var
  i :integer;
  p :PRawUnit;
begin
  if (u.pktid = $14) or (u.pktid = 36) then
    exit;

  if (u.id = $ffffffff) then
  begin
    crc := u.fill;
//    showmessage ('the crc is ' + inttostr (crc));
    exit;

  end;

  if (u.sub = $3) then
  begin
    for i := 0 to units.count - 1 do
    begin
      if PRawUnit (units.Items [i])^.id = u.id then
      begin
        PRawUnit (units.Items [i])^.inuse := u.status = $0101;
        PRawUnit (units.Items [i])^.limit := u.limit;

        exit;
      end;
    end;

    New (p);

{    if u.id = SY_Unit then
      showmessage ('hoho');}

    p.Id := u.id;
    p.Crc := 0;
    p.limit := u.limit;
    p.inuse := u.status = $0101;

    Units.Add (p);
  end;

  if (u.sub = $2) then
  begin
    for i := 0 to units.count - 1 do
    begin
      if PRawUnit (units.Items [i])^.id = u.id then
      begin
        PRawUnit (units.Items [i])^.Crc := u.crc;

        exit;
      end;
    end;

    New (p);

{        if u.id = $371d264a then
          showmessage (inttostr (u.crc)); }

    p.Id := u.id;
    p.Crc := u.crc;;
    p.limit := 0;
    p.inuse := false;

    Units.Add (p);


//    ShowMessage ('Impossible. Damn u man');
  end;
end;

function TUnitSync.Get (i :integer; use :boolean) :TUnitPkt;
var
  p :TUnitPkt;
  raw :PRawUnit;
begin
  raw := units.items[i];

  p.pktid := $1a;
  p.sub := $3;
  p.fill := 0;
  p.id := raw^.id;
  if use then
  begin
    p.status := $0101;
    p.limit := raw^.limit;
  end else
  begin
    p.status := $0001;
    p.limit := $ffff;
  end;

  Result := p;
end;

function TUnitSync.Find (id :longword) :PRawUnit;
var
  raw :PRawUnit;
  i :integer;
begin
  Result := nil;
  for i := 0 to units.count - 1 do
  begin
    raw := units.items [i];
    if raw.Id = id then
    begin
      Result := raw;
      exit;
    end;
  end;
end;

//Tar bort alla units som inte är inuse..
procedure TUnitSync.Compact;
var
  i :integer;
  raw :PRawUnit;
begin
  i := 0;
  while i < units.Count do
  begin
    raw := units.items [i];
    if not raw.inuse then
      units.Delete (i)
    else
      inc (i);
  end;
end;

end.
