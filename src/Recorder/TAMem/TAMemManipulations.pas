unit TAMemManipulations;

interface
uses
  sysutils,
  MemMappedDataStructure;

Type
  PWord = ^Word;
  PLongword = ^Longword;

function TestByte( p : longword; b : byte) : Boolean; overload;
function TestByte( p : longword; b : byte; var failvalue : byte) : Boolean; overload;

function TestBytes( p : longword;
                    ExpectedData : PByteArray; len : integer;
                    var FailIndex : integer; var failvalue : byte) : Boolean;

procedure SetWord( p : pointer; data : Word ); overload;
procedure SetWord( p : longword; data : Word ); overload;
procedure SetLongword( p : pointer; data : longword); overload;
procedure SetLongword( p : longword; data : longword); overload;


function CheckForCheats : TTACheats;

implementation
uses
  windows;
  
function TestByte(p : cardinal;b : byte) : Boolean;
begin
result:= IsBadReadPtr(Pbyte(p),1) or (Pbyte(p)^ = b);
end;

function TestByte( p : longword; b : byte; var FailValue : byte) : Boolean; overload;
begin
if not IsBadReadPtr(Pbyte(p),1) then
  begin
  FailValue := Pbyte(p)^;
  result := (FailValue = b);
  end
else
  begin
  result := false;
  failvalue := not b;
  end;
end;

function TestBytes( p : longword;
                    ExpectedData : PByteArray; len : integer;
                    var FailIndex : integer; var failvalue : byte) : Boolean;
var
  i : integer;
begin
for i := 0 to len -1 do
  begin
  if not TestByte( p, ExpectedData[i], FailValue) then
    begin
    FailIndex := i;
    result := false;
    exit;
    end;
  inc(p);  
  end;
FailIndex := -1;
failvalue := 0;
result := true;
end;

procedure SetWord( p : pointer; data : Word );
begin
PWord(p)^ := data;
end;

procedure SetWord( p : longword; data : Word );
begin
PWord(p)^ := data;
end;

procedure SetLongword( p : pointer; data : longword);
begin
PLongword(p)^ := data;
end;

procedure SetLongword( p : longword; data : longword);
begin
PLongword(p)^ := data;
end;

function CheckForCheats : TTACheats;
begin
result := 0;
// here be magic numbers
if not TestByte($489CF5,4) then
  result:=result or Cheat_Invulnerability;

if not TestByte($401805,$67) or not TestByte($4017E7,$0f) or
   not TestByte($401808,$76) or not TestByte($40181E,$d9) then
  result:=result or Cheat_Invisible;

if not TestByte($402AD9,1) or not TestByte($402AD9,1) or
   not TestByte($403EFF,1) or not TestByte($4041F5,1) or
   not TestByte($4142D2,1) or not TestByte($4146F3,1) then
  result:=result or Cheat_FastBuild;
if not TestByte($4018BD,$d8) or not TestByte($4018D9,$d8) then
  result:=result or Cheat_FastBuild;

if not TestByte($4018BD,$d8) or not TestByte($4018D9,$d8) then
  result:=result or Cheat_InfiniteResources;

if not TestByte($484470,$7d) or not TestByte($4844A9,$0f) or
   not TestByte($466CCB,$0f) or not TestByte($466C38,$0f) or
   not TestByte($466D16,$75) or not TestByte($466E31,8) or
   not TestByte($48BC5E,$1e) then
  result:=result or Cheat_LosRadar;

if not TestByte($457ACF,$74) then
  result:=result or Cheat_ControlMenu;

if not TestByte($47D4C0,$0f) then
  result:=result or Cheat_BuildAnywhere;

if not TestByte($404298,$8a) then
  result:=result or Cheat_InstantCapture;

if not TestByte($43cf98,$91) then
  result:=result or Cheat_SpecialMove;

if not TestByte($46704A,$31) or not TestByte($467041,$88) then
  result:=result or Cheat_JamAll;

if not TestByte($499DE7,$c1) then
  result:=result or Cheat_ExtraDamage;

if not TestByte($41BACD,$db) or not TestByte($41BACE,$81) or
   not TestByte($41BB37,$81) or not TestByte($41BB38,$86) then
 result:= result or Cheat_InstantBuild;

if TestByte($401AC1,$90) and TestByte($401AC2,$90) and
   TestByte($401AC3,$90) and TestByte($401AC4,$90) and
   TestByte($401AC5,$90) and TestByte($401AC6,$90) then
 result:= result or Cheat_ResourcesFreeze;
if TestByte($401AFE,$90) and TestByte($401AFF,$90) and
   TestByte($401B00,$90) and TestByte($401B01,$90) and
   TestByte($401B02,$90) and TestByte($401B03,$90) then
 result:= result or Cheat_ResourcesFreeze;

if (PByte(Plongword($511de8)^+$37F2F)^ and (1 shl 1) ) <> 0 then
 result:= result or Cheat_DeveloperMode;

end; {CheckForCheats}

end.
