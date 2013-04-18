unit unitlist;

interface

uses
  classes, sysutils, dialogs, cstream;

type
  PUnit = ^TUnit;
  TUnit = record
    id :longword;
    limit :longword;
  end;

type
  TUnitList = class
    units :TList;
    constructor Create (name :string);
    procedure Save (name :string);
  end;

implementation

constructor TUnitList.Create (name :string);
var
  f :TBufStream;
  tot, i :integer;
  u :PUnit;
begin
  f := TBufStream.Create (name, fmOpenRead);
  units := TList.Create;

  f.Read (tot, sizeof (tot));
  for i := 1 to tot do
  begin
    New (u);
    if f.Read (u^, sizeof (tunit)) < sizeof (tunit) then
    begin
//      Showmessage ('only ' + inttostr (i) + ' units');
      f.Free;
      exit;
    end;

    units.Add (u);
  end;

  f.Free;
end;

procedure TUnitList.Save (name :string);
var
  f :TBufStream;
  i :integer;
  p :PUnit;
begin
  f := TBufStream.Create (name, fmCreate);

  i := 275;
  f.Write (i, sizeof (i));
  for i := 0 to units.count - 1 do
  begin

    p := units.items [i];
    p^.limit := 15;

    f.Write (p^, sizeof (tunit));
  end;
  f.Free;
end;

end.
