unit ModsList;

interface
uses SysUtils, Classes, INIFiles;

function FixModsINI(replayerDir: string): boolean;
function ReadModsIniField(replayerDir: string; ident: string): string;

const
  MODSINI = 'mods.ini';

implementation
uses idplay, INI_Options;

//var ReplayerPath: string;

function FixModsINI(replayerDir: string): boolean;
var
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
        if iniSettings.name <> '' then
          WriteString('MOD' + readedModID, 'Name', iniSettings.name);
        if iniSettings.version <> '' then
          WriteString('MOD' + readedModID, 'Version', iniSettings.version);
        if iniSettings.RegName <> '' then
          WriteString('MOD' + readedModID, 'RegName', iniSettings.RegName);
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

function ReadModsIniField(replayerDir: string; ident: string): string;
var
  readedModID: string;
begin
if replayerDir <> '' then
begin
  with TIniFile.Create(replayerDir + MODSINI) do
  try
      if iniSettings.modId <> - 1 then
      begin
        readedModID:= IntToStr(iniSettings.modId);
        Result:= ReadString('MOD' + readedModID, ident, #0);
      end;
    finally
      Free;
    end;
  end;
end;

end.
