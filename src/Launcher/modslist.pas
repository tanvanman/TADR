{

    The TA Launcher 1.x - Modslist unit
    Copyright (C) 2013 Rime, N72

    e-mail: plobex@o2.pl

    Licensed under the terms stored in launcher-license.txt

}

unit modslist;

interface
uses SysUtils, Classes, IniFiles, Dialogs;

type
  TModsIniSettings = record
    ID          : word;
    Name        : string;
    Version     : string;
    Path        : string;
    RegName     : string;
    UnitsLimit  : string;
    SFXLock     : string;
    IconIndex   : array[0..1] of integer;
    InfoPath    : string;
    UpdaterPath : string;
    Readme      : string;
    ChangeLog   : string;
    Website     : string;
    Forum       : string;
  end;
var LoadedModsList: array of TModsIniSettings;

var
 zeromod: integer;

const
  MODSINI = 'mods.ini';

function LoadModsList(out errornumber: integer): boolean;
function FindModId(id: integer): integer;
function FindModName(id: integer): string;

implementation
uses settings;

procedure DeleteX(const Index: Cardinal);
var
  ALength: Cardinal;
  i: Cardinal;
begin
  ALength := Length(LoadedModsList);
  Assert(ALength > 0);
  Assert(Index < ALength);
  for i := Index + 1 to ALength - 1 do
    LoadedModsList[i - 1] := LoadedModsList[i];
  SetLength(LoadedModsList, ALength - 1);
end;

function LoadModsList(out errornumber: integer): boolean;
var
  Sections: TStringList;
  i: word;
  incorrect: word;
begin
Result:= False;
incorrect:= 0;
zeromod:= -1;
if FileExists(IncludeTrailingPathDelimiter(LauncherSettings.AppDataDir)+MODSINI) then
begin
  with TIniFile.Create(IncludeTrailingPathDelimiter(LauncherSettings.AppDataDir)+MODSINI) do
    try
      Sections := TStringList.Create;
      try
        ReadSections(Sections);
        if Sections.Count > 0 then
        begin
        SetLength(LoadedModsList, Sections.Count);
        for i := 0 to Sections.Count - 1 do
        begin
          if (ReadString(Sections[i], 'ID', '') <> '') and
              (ReadString(Sections[i], 'Path', '') <> '') then
          begin
            if (ReadString(Sections[i], 'ID', '') <> '0') then
            begin
            LoadedModsList[i].ID := ReadInteger(Sections[i], 'ID', 0);
            LoadedModsList[i].Name := ReadString(Sections[i], 'Name', 'Name not specified');
            LoadedModsList[i].Version := ReadString(Sections[i], 'Version', '');
            LoadedModsList[i].Path := ReadString(Sections[i], 'Path', '');
            LoadedModsList[i].UnitsLimit := ReadString(Sections[i], 'UnitLimitLock', '0');
            LoadedModsList[i].SFXLock := ReadString(Sections[i], 'SFXLock', '0');
            LoadedModsList[i].RegName := ReadString(Sections[i], 'RegName', 'Cavedog Entertainment');
            end else
              // dont fill backward compatibility
              zeromod:= i;
          end else
            Inc(incorrect);
            Continue;
          end;
          // delete backward compatibility from loadedmodslist to prevent it
          // from showing up in main window
          if zeromod <> - 1 then DeleteX(zeromod);
        end else { sections count < 1 }
        begin
          errornumber:= 3;
          Exit;
        end;
      finally
        Sections.Free;
      end;
    finally
      Free;
    end;
  if incorrect > 0 then
    showmessage('Warning ! Found '+intToStr(incorrect)+' incorrect entry(ies) in '+MODSINI);
  //if High(LoadedModsList) > 0 then
  if High(LoadedModsList) > -1 then
    Result:= True
  else
    errornumber:= 2;
end else
  begin
     errornumber:= 1;
     Exit;
  end;
end;

function FindModName(id: integer): string;
var
  i: word;
begin
  Result:= #0;
  if id <> - 1 then
  begin
    for i := Low(LoadedModsList) to High(LoadedModsList) do
    begin
      if LoadedModsList[i].ID = id then
      begin
        Result:= LoadedModsList[i].Name;
        Break;
      end;
    end;
  end;
end;

function FindModId(id: integer): integer;
var
  i: word;
begin
  Result:= -1;
  for i := Low(LoadedModsList) to High(LoadedModsList) do
  begin
    if LoadedModsList[i].ID = id then
    begin
      Result:= i;
      Break;
    end;
  end;
end;

end.
