unit INI_Options;

interface

uses windows, sysutils, inifiles;

var
 COBextType: boolean;

function ReadININame: string;
function ReadINIOptions: boolean;

implementation

function ReadINIname: string;
const
 SizeOfBuffer: DWord = 13;
var
 CheckIsINI: string;
 DllPathBuffer: array[0..260] of Char;
 INIPath: string;
 Buffer: array of byte;
 Address: DWord;
 i: byte;
begin
 Result:= #0;
 Address:= $5098A3;
 SetLength(Buffer, SizeOfBuffer);
  for i := 0 to SizeOfBuffer - 1 do
    begin
     Buffer[i]:= Pbyte(Address)^;
     Inc(Address);
    end;
 SetString(CheckIsINI, PAnsiChar(@Buffer[0]), SizeOfBuffer);

 Trim(CheckIsINI);
 GetModuleFileName(hInstance, DllPathBuffer, Length(DllPathBuffer));
 INIPath:= IncludeTrailingPathDelimiter(ExtractFileDir(DllPathBuffer));
  if FileExists(INIPath + CheckIsINI) then
    Result:= INIPath + CheckIsINI;
end;

function ReadINIOptions: boolean;
var
INIFile: TINIFile;
begin
Result:= False;
COBextType:= True;
  if ReadININame <> #0 then
    begin
      INIFile:= TIniFile.Create(ReadININame);
      try
        if INIFile.ReadInteger('MOD','COBExtensionsStandard', 1) = 1 then COBextType:= True else COBextType:= False;
        Result:= True;
      finally
        INIFIle.Free;
      end;
    end;
end;

end.
 