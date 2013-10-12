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
      if iniSettings.modId <> - 1 then
      begin
        readedModID:= IntToStr(iniSettings.modId);
        WriteInteger('MOD' + readedModID, 'ID', iniSettings.modid);
        WriteString('MOD' + readedModID, 'Name', iniSettings.name);
        WriteString('MOD' + readedModID, 'Path', ParamStr(0));
        WriteBool('MOD' + readedModID, 'UseWeaponIdPatch', iniSettings.weaponidpatch);
      end;
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
