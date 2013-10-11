unit ModsList;

interface
uses SysUtils, Classes, INIFiles;

function CheckModsList(replayerDir: string): boolean;

const
  MODSINI = 'mods.ini';

implementation
uses idplay, INI_Options;

function CheckModsList(replayerDir: string): boolean;
var
  modsIniSettings: TINIFile;
  readedModID: string;
begin
Result:= False;
if replayerDir <> '' then
begin
  with TIniFile.Create(replayerDir + MODSINI) do
  try
    try
      readedModID:= IntToStr(iniSettings.modId);
      if SectionExists('MOD' + readedModID) then
        EraseSection('MOD' + readedModID);
      WriteInteger('MOD' + readedModID, 'ID', iniSettings.modid);
      WriteString('MOD' + readedModID, 'Name', iniSettings.name);
      WriteString('MOD' + readedModID, 'Path', ParamStr(0));
    finally
      Result:= True;
      Free;
    end;
  except
    Result:= False;
  end;
end else
  begin
    Result:= False;
  end;
end;

end.
