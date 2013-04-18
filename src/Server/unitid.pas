unit unitid;

interface

uses
  classes, sysutils, dialogs;

type
  TSection = class
    name :string;
    ids  :tlist;
    constructor Create;
    destructor Destroy; override;
  end;

type
  TUnitIds = class
    defunits :TList;
    groups :TList;
    notloaded :boolean;
//    units :Tlist;

    constructor Create (fname :string);
    destructor Destroy; override;
    function GetName (id :longword) :string;
    function GetUnits (ids :TList) :TStringList;
  end;

implementation

constructor TSection.Create;
begin
  ids := Tlist.Create;
end;

destructor TSection.Destroy;
begin
  ids.free;
  inherited Destroy;
end;

constructor TUnitIds.Create (fname :string);
var
  f :TextFile;
  s :string;
  cursect :TSection;
  curunits :TList;
  i :longword;
  code :integer;
  units :TList;
begin
  groups := nil;
  units := nil;
  notloaded := true;

  AssignFile (f, fname);
  {$I-}
  Reset (f);
  {$I+}
  if ioresult <> 0 then
  begin
    MessageDlg ('No unitid.txt found', mtError, [mbok], 0);
    exit;
  end;

  groups := tlist.create;
//  units := tlist.Create;
  cursect := nil;
  defunits := TList.Create;

  curunits := defunits;

  while not eof (f) do
  begin
    Readln (f, s);
    s := Trim (s);
    if Length (s) > 0 then
    begin
      case s[1] of
        ';'  :;
        '='  :begin
                cursect := TSection.Create;
                cursect.name := Copy (s, 2, 10000);
                curunits.Add (cursect);
              end;
        '+'  :begin
                curunits := TList.Create;
                groups.add (curunits);
              end;
      else
        if not assigned (cursect) then
        begin
          ShowMessage('Unit id before section found.. Not good');
          exit;
        end;

        Val ('$' + s, i, code);
        if code <> 0 then
        begin
          showmessage('Non-hex unit id found. (' + s + ')');
          exit;
        end;

        cursect.ids.Add (pointer (i));
      end;

    end;
  end;

  notloaded := false;
  CloseFile(f);
end;

destructor TUnitIds.Destroy;
var
  i, j :integer;
  units :tlist;
begin
  if assigned (groups) then
  begin
    for j := 0 to groups.count - 1 do
    begin
      units := tlist(groups.items[j]);
      for i := 0 to units.count - 1 do
      begin
        TSection(units.items[i]).free;
      end;
      units.free;
    end;
    groups.free;
  end;

  inherited Destroy;
end;

function TUnitIds.GetName (id :longword) :string;
var
  i, j :integer;
  s :TSection;
begin
  Result := '';
{  if not assigned (groups) then
    exit;

  for i := 0 to units.count - 1 do
  begin
    s := units.items [i];
    for j := 0 to s.ids.Count - 1 do
    begin
      if longword(s.ids.items[j]) = id then
      begin
        Result := s.name;
        exit;
      end;
    end;
  end;}
end;

function TUnitIds.GetUnits (ids :TList) :TStringList;
var
  i, j, k :integer;
  units   :tlist;
  sect    :Tsection;
  tmp     :TStringlist;
  id      :longword;
begin
  tmp := TStringlist.Create;
  Result := tmp;

  if notloaded then
    exit;

  for i := 0 to groups.count - 1 do
  begin
    tmp.Clear;

    units := groups.items [i];
    for j := 0 to units.count - 1 do
    begin
      sect := units.items [j];
      for k := 0 to sect.ids.count - 1 do
      begin
        id := longword(sect.ids.items[k]);
        if ids.indexof (pointer (id)) <> -1 then
        begin
          tmp.Add (sect.name);
          break;
        end;
      end;
    end;

    if tmp.count > 0 then
    begin
      break;
    end;
  end;

  for i := 0 to defunits.count - 1 do
  begin
    sect := defunits.items [i];
    for k := 0 to sect.ids.count - 1 do
    begin
      id := longword(sect.ids.items[k]);
      if ids.indexof (pointer (id)) <> -1 then
      begin
        tmp.Add (sect.name);
        break;
      end;
    end;
  end;
end;

end.
