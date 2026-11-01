unit RecorderRegistry;

// Typed, bounded configuration reads and best-effort persistence. TAG_Venom.
// Missing, inaccessible or malformed optional settings keep their defaults;
// they must not leave the recorder half reset. Reads never create registry keys.
interface

uses Windows, SysUtils;

type
  TRecorderRegistry = class
  private
    FRoot: HKEY;
    FBasePath: string;
    FView: Longword;
    FLastError: Longint;
    function OpenSection(const Section: string; WriteAccess: Boolean;
      var Key: HKEY): Boolean;
    function ReadScalar(const Section, Name: string; var Value: Longword;
      AllowByteBoolean: Boolean): Boolean;
  public
    constructor Create(Root: HKEY; const BasePath: string; View: Longword = 0);
    function ReadInteger(const Section, Name: string; Default: Integer): Integer;
    function ReadBool(const Section, Name: string; Default: Boolean): Boolean;
    function ReadString(const Section, Name, Default: string): string;
    function WriteInteger(const Section, Name: string; Value: Integer): Boolean;
    function WriteString(const Section, Name, Value: string): Boolean;
    property LastError: Longint read FLastError;
  end;

function EnsureRecorderDirectory(const Path: string): Boolean;

implementation

constructor TRecorderRegistry.Create(Root: HKEY; const BasePath: string;
  View: Longword);
begin
  inherited Create;
  FRoot := Root;
  FBasePath := BasePath;
  FView := View;
end;

function TRecorderRegistry.OpenSection(const Section: string;
  WriteAccess: Boolean; var Key: HKEY): Boolean;
var
  Path: string;
  Disposition: Longword;
begin
  Key := 0;
  Path := FBasePath;
  if Section <> '' then
  begin
    if (Path <> '') and (Path[Length(Path)] <> '\') then
      Path := Path + '\';
    Path := Path + Section;
  end;
  if WriteAccess then
    FLastError := RegCreateKeyExA(FRoot, PAnsiChar(Path), 0, nil,
      REG_OPTION_NON_VOLATILE, KEY_SET_VALUE or FView, nil, Key, @Disposition)
  else
    FLastError := RegOpenKeyExA(FRoot, PAnsiChar(Path), 0,
      KEY_QUERY_VALUE or FView, Key);
  Result := FLastError = ERROR_SUCCESS;
end;

function TRecorderRegistry.ReadScalar(const Section, Name: string;
  var Value: Longword; AllowByteBoolean: Boolean): Boolean;
var
  Key: HKEY;
  Kind, Size: Longword;
begin
  Result := False;
  Value := 0;
  if not OpenSection(Section, False, Key) then Exit;
  try
    Size := SizeOf(Value);
    Kind := REG_NONE;
    FLastError := RegQueryValueExA(Key, PAnsiChar(Name), nil, @Kind,
      @Value, @Size);
    if FLastError <> ERROR_SUCCESS then Exit;
    Result := ((Kind = REG_DWORD) and (Size = SizeOf(Value))) or
      (AllowByteBoolean and (Kind = REG_BINARY) and (Size = 1));
    if not Result then FLastError := ERROR_INVALID_DATA;
  finally
    RegCloseKey(Key);
  end;
end;

function TRecorderRegistry.ReadInteger(const Section, Name: string;
  Default: Integer): Integer;
var
  Value: Longword;
begin
  Result := Default;
  if ReadScalar(Section, Name, Value, False) then
    Result := Integer(Value); // Preserve the original REG_DWORD bit pattern.
end;

function TRecorderRegistry.ReadBool(const Section, Name: string;
  Default: Boolean): Boolean;
var
  Value: Longword;
begin
  Result := Default;
  // Accept both historical one-byte Boolean and DWORD representations.
  if ReadScalar(Section, Name, Value, True) then Result := Value <> 0;
end;

function TRecorderRegistry.ReadString(const Section, Name, Default: string): string;
const
  // A Win32 path has at most 32767 UTF-16 characters. This also accommodates
  // their legacy double-byte ANSI representation; it is not a replay size cap.
  MaxPathBytes = 65536;
var
  Key: HKEY;
  Kind, Size: Longword;
  Buffer: string;
  I: Integer;
begin
  Result := Default;
  if not OpenSection(Section, False, Key) then Exit;
  try
    Size := 0;
    Kind := REG_NONE;
    FLastError := RegQueryValueExA(Key, PAnsiChar(Name), nil, @Kind, nil, @Size);
    if FLastError <> ERROR_SUCCESS then Exit;
    if ((Kind <> REG_SZ) and (Kind <> REG_EXPAND_SZ)) or
       (Size > MaxPathBytes) then
    begin
      FLastError := ERROR_INVALID_DATA;
      Exit;
    end;
    SetLength(Buffer, Size + 1);
    Buffer[Length(Buffer)] := #0;
    FLastError := RegQueryValueExA(Key, PAnsiChar(Name), nil, @Kind,
      PByte(PAnsiChar(Buffer)), @Size);
    if FLastError <> ERROR_SUCCESS then Exit;
    if (Kind <> REG_SZ) and (Kind <> REG_EXPAND_SZ) then
    begin
      FLastError := ERROR_INVALID_DATA;
      Exit;
    end;
    SetLength(Buffer, Size);
    for I := 1 to Length(Buffer) do
      if Buffer[I] = #0 then
      begin
        SetLength(Buffer, I - 1);
        Break;
      end;
    Result := Buffer;
  finally
    RegCloseKey(Key);
  end;
end;

function TRecorderRegistry.WriteInteger(const Section, Name: string;
  Value: Integer): Boolean;
var
  Key: HKEY;
begin
  Result := False;
  if not OpenSection(Section, True, Key) then Exit;
  try
    FLastError := RegSetValueExA(Key, PAnsiChar(Name), 0, REG_DWORD,
      @Value, SizeOf(Value));
    Result := FLastError = ERROR_SUCCESS;
  finally
    RegCloseKey(Key);
  end;
end;

function TRecorderRegistry.WriteString(const Section, Name, Value: string): Boolean;
var
  Key: HKEY;
  Data: string;
begin
  Result := False;
  if not OpenSection(Section, True, Key) then Exit;
  try
    Data := Value + #0;
    FLastError := RegSetValueExA(Key, PAnsiChar(Name), 0, REG_SZ,
      PByte(PAnsiChar(Data)), Length(Data));
    Result := FLastError = ERROR_SUCCESS;
  finally
    RegCloseKey(Key);
  end;
end;

function EnsureRecorderDirectory(const Path: string): Boolean;
begin
  Result := False;
  if Path = '' then Exit;
  try
    Result := ForceDirectories(Path);
  except
    on E: EInOutError do Result := False;
  end;
end;

end.
