unit INI_Options;

interface

uses Windows, SysUtils, IniFiles, dialogs;

type
 TiOptions = record
  COBExtType: boolean;
 end;
var iOptions: TiOptions;

function ReadININame: string;
function ReadINIOptions: boolean;

implementation
uses COB_extensions;

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
 showmessage('INI name buffer set');
 try
  for i := 0 to SizeOfBuffer - 1 do
    begin
     Buffer[i]:= Pbyte(Address)^;
     Inc(Address);
    end;
 showmessage('Buffer filled');
  SetString(CheckIsINI, PAnsiChar(@Buffer[0]), SizeOfBuffer);
 showmessage('Set string done');
 except
 showmessage('read failure, using totala.ini');
  CheckIsIni:= 'totala.ini';
 end;

 Trim(CheckIsINI);
 showmessage('trim');
 GetModuleFileName(hInstance, DllPathBuffer, Length(DllPathBuffer));
 showmessage('working dir set');
 INIPath:= IncludeTrailingPathDelimiter(ExtractFileDir(DllPathBuffer));
  if FileExists(INIPath + CheckIsINI) then
  begin
    Result:= INIPath + CheckIsINI;
 showmessage('INI found and exist');
    end;
end;

function ReadINIOptions: boolean;
var
INIFile: TINIFile;
begin
Result:= False;
ioptions.COBextType:= True;
 showmessage('read ini options begin');
  if ReadININame <> #0 then
    begin
      INIFile:= TIniFile.Create(ReadININame);
      try
        if INIFile.ReadInteger('Recorder','COBExt', 1) = 1 then ioptions.COBextType:= True else ioptions.COBextType:= False;
        SetCOBType;
 showmessage('set cob type');
        Result:= True;
      finally
        INIFIle.Free;
      end;
    end;
end;

end.
 