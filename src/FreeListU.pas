unit FreeListU;

interface
uses
  classes,
  Contnrs;
  
var
  ObjectsToFree : TObjectList;

procedure OnInitialize;
procedure OnFinalize;

implementation
uses
  sysutils;
  
procedure OnInitialize;
begin
ObjectsToFree := TObjectList.create(true);
end;

procedure OnFinalize;
begin
FreeAndNil( ObjectsToFree );
end;

end.
