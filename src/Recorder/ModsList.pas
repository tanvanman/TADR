unit ModsList;

interface
uses SysUtils, Classes, INIFiles;

function FixModsINI: boolean;
function ReadModsIniField(ident: string): string;

const
  MODSINI = 'mods.ini';

implementation
uses idplay, IniOptions, TADemoConsts;

function FixModsINI: boolean;
var
  readedModID: string;
begin
if GetAppData <> '' then
begin
  with TIniFile.Create(GetAppData + MODSINI) do
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
        // in case user moved mod directory, alwasys fix the path in mods.ini
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

function ReadModsIniField(ident: string): string;
var
  readedModID: string;
  IniFile: TIniFile;
begin
Result := #0;
if GetAppData <> '' then
begin
  IniFile := TIniFile.Create(GetAppData + MODSINI);
  with IniFile do
  try
      if iniSettings.modId <> - 1 then
      begin
        readedModID:= IntToStr(iniSettings.modId);
        Result := ReadString('MOD' + readedModID, ident, #0);
      end;
    finally
      Free;
    end;
  end;
end;

end.
