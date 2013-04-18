program CallstackParser;
{$APPTYPE CONSOLE}
uses
  windows,
  SysUtils,
  uDebug in '..\uDebug.pas';

function GetMapAddressFromAddress(const Address: Longword): Longword;
const
  CodeBase = $1000;
  OffSet: Longword = $700000 + CodeBase;
begin
if OffSet <= Address then
  // Map file address = Access violation address - Offset
  Result := Address - OffSet
else
  Result := Address;
end;

var f : text;
procedure DoConvert( const s : string );
var
  ErrorLocation : Longword;
begin
try
  ErrorLocation := StrToInt( s );
except
  on e : EConvertError do
    Exit;
end;
Writeln( f, 'Addr  :$', IntToHex( ErrorLocation, 8) );
ErrorLocation := GetMapAddressFromAddress( ErrorLocation );
Writeln( f, 'file  :', GetModuleNameFromAddress( ErrorLocation ) );
Writeln( f, 'Method:',GetProcNameFromAddress( ErrorLocation ) );
Writeln( f, 'Line  :',GetLineNumberFromAddress( ErrorLocation ) );
Writeln( f );
end; {DoConvert}

var
  kernellib : THandle;
type
  TGetVolumePathNameAHandler = function( lpszFileName : pAnsiChar;
                                        lpszVolumePathName: pAnsiChar;
                                        cchBufferLength : longword) : longbool; stdcall;// External  name 'GetVolumePathName';
var
  proc : TGetVolumePathNameAHandler;
function GetVolumePathName( lpszFileName : pAnsiChar;
                            lpszVolumePathName: pAnsiChar;
                            cchBufferLength : longword ) : longbool; stdcall; //External kernel32 name 'GetVolumePathName';
begin
Result := False;
if kernellib = 0 then
  kernellib := LoadLibrary( kernel32 );
if kernellib <> 0 then
  begin
  proc := GetProcAddress( kernellib, 'GetVolumePathNameA' );
  if Assigned(proc) then
    Result := proc( lpszFileName, lpszVolumePathName, cchBufferLength );
  end;
end;

var
  i : Integer;
  s,s2 : string;
begin
MapFileName := 'Dplayx.map';
LoadAndParseMapFile;
try
  assignfile(f,'callstack.txt');
  rewrite(f);
  if ParamCount >= 1 then
    for i := 1 to paramcount do
      DoConvert( '$'+ParamStr(i) )
  else
    while True do
      begin
      Writeln('Enter callstack to trace');
      Readln(s);
      if s = 'exit' then
        break;
      while s <> '' do
        begin
        i := Pos(' ', s);
        if i <> 0 then
          begin
          s2 := Copy( s, 1, i-1);
          Delete( s, 1, i );
          end
        else
          begin
          s2 := s;
          s := '';
          end;
        DoConvert('$'+s2);
        end;
      end
finally
  CloseFile(f);
  CleanUpMapFile;
  readln;
end;
end.