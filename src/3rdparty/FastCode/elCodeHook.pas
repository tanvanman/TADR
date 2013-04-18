unit elCodeHook;

interface
uses
  Windows,
  PSAPI,
  SysUtils;

type
  PJump = ^TJump;
  TJump = packed record
    OpCode: Byte;
    Distance: Pointer;
  end;

  TCodeHook = class
  private
    FOldJump: TJump;
    FOldAddr: Pointer;
    FNewAddr: Pointer;
  public
    procedure Install;
    procedure UnInstall;
    constructor Create(OldAddr: Pointer; NewAddr: Pointer);
    destructor Destroy; override;
    property OldJump: TJump read FOldJump;
    property OldAddr: Pointer read FOldAddr;
    property NewAddr: Pointer read FNewAddr;
  end;

// searches 50 bytes for 5 0xCC(int3)'s in a row followed by a $E8
// This locates the call instruction after some known markers
Function FindFuncAddress(SearchAddress : pointer; ScanRange : integer = 50) : pointer;

function CreateCodeHook(ModuleHandle: THandle; OldProcName: String; NewProcAddr: Pointer): TCodeHook;

implementation

Function FindFuncAddress(SearchAddress : pointer; ScanRange : integer = 50) : pointer;
const
  signature = $CC;
  SigMatches = 5;
var
  i : integer;
  matchCount : integer;

  Procedure ExtractAddress;
  begin
  result := SearchAddress;
  Inc(Integer(result));
  result := Pointer(Integer(result) + SizeOf(Pointer) + PInteger(result)^);
  end;
begin
result := nil;
matchCount := 0;
assert( SearchAddress <> nil );
if (PBYTE(SearchAddress)^ <> $E8) then
  begin
  for i := 0 to ScanRange do
    begin
    if PBYTE(SearchAddress)^ = signature then
      inc( matchCount )
    else if (matchCount >= SigMatches) and (PBYTE(SearchAddress)^ = $E8) then
      begin
      ExtractAddress;
      break;
      end
    else
      matchCount := 0;
    inc( Integer(SearchAddress) );
    end;  
  end
else
  ExtractAddress;
end;

procedure TCodeHook.Install;
var
  NewJump: PJump;
  OldProtect: DWORD;
  temp: DWORD;
begin
  if not VirtualProtect(OldAddr, SizeOf(TJump), PAGE_EXECUTE_READWRITE, OldProtect) then
  begin
    RaiseLastOSError;
  end;
  try
    NewJump := PJump(OldAddr);
    FOldJump := NewJump^;

    NewJump.OpCode := $E9;
    NewJump.Distance := Pointer(Integer(NewAddr) - Integer(OldAddr) - 5);

    if not FlushInstructionCache(GetCurrentProcess, OldAddr, SizeOf(TJump)) then
    begin
      RaiseLastOSError;
    end;
  finally
    if not VirtualProtect(OldAddr, SizeOf(TJump), OldProtect, temp) then
    begin
      RaiseLastOSError;
    end;
  end;
end;

constructor TCodeHook.Create(OldAddr: Pointer; NewAddr: Pointer);
begin
  inherited Create;
  FOldAddr := OldAddr;
  FNewAddr := NewAddr;
  Install;
end;

procedure TCodeHook.UnInstall;
var
  NewJump: PJump;
  OldProtect: DWORD;
  temp: DWORD;
begin
  if not VirtualProtect(OldAddr, SizeOf(TJump), PAGE_READWRITE, @OldProtect) then
  begin
    RaiseLastOSError;
  end;
try  
  NewJump := PJump(OldAddr);
  NewJump^ := OldJump;
  if not FlushInstructionCache(GetCurrentProcess, OldAddr, SizeOf(TJump)) then
  begin
    RaiseLastOSError;
  end;
finally
  if not VirtualProtect(OldAddr, SizeOf(TJump), OldProtect, temp) then
  begin
    RaiseLastOSError;
  end;
end;
end;

destructor TCodeHook.Destroy;
begin
  UnInstall;
  inherited;
end;

function CreateCodeHook(ModuleHandle: THandle; OldProcName: String; NewProcAddr: Pointer): TCodeHook;
var
  OldProcAddr: Pointer;
begin
  OldProcAddr := GetProcAddress(ModuleHandle, PChar(OldProcName));
  Assert(Assigned(OldProcAddr), OldProcName);

  Result := TCodeHook.Create(OldProcAddr, NewProcAddr);
end;

end.
