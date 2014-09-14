unit RegPathFix;

// set main registry path of game settings to path that is based on game modification
// RegName field property (check mods.ini), so every mod have its own settings

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_RegPathFix : boolean = true;

type
  TAFindData = record
    unk1 : Cardinal;
    unk2 : Cardinal;
    unk3 : Cardinal;
    unk4 : Cardinal;
    unk5 : Cardinal;
    FileName : array[0..63] of AnsiChar;
  end;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallRegPathFix;
Procedure OnUninstallRegPathFix;

// -----------------------------------------------------------------------------

procedure LoadSharedMapsHook;

implementation
uses
  Windows,
  IniOptions,
  TA_MemoryConstants,
  ModsList,
  textdata,
  Sysutils,
  TA_FunctionsU,
  TA_MemoryLocations,
  TADemoConsts;

var
  RegPathFixPlugin: TPluginData;

Procedure OnInstallRegPathFix;
begin
end;

Procedure OnUninstallRegPathFix;
begin
end;

function GetPlugin : TPluginData;
var
  sRegName : AnsiString;
  baRegName: TByteArray;
begin
  if IsTAVersion31 and State_RegPathFix then
  begin
    RegPathFixPlugin := TPluginData.create( True,
                            'regfix',
                            State_RegPathFix,
                            @OnInstallRegPathFix,
                            @OnUninstallRegPathFix );

    if IniSettings.RegName <> '' then
    begin
      sRegName := Copy(IniSettings.RegName, 1, 21);
      sRegName := LeftPad(sRegName, #0, 21);

      Move(sRegName[1], baRegName, Length(sRegName));

      RegPathFixPlugin.MakeReplacement(State_RegPathFix,
                          'TA Settings Registry Path',
                          $0050DDFD,
                          baRegName, Length(sRegName));

      RegPathFixPlugin.MakeReplacement(State_RegPathFix,
                          'TA Settings Registry Path 2',
                          $00509EB8,
                          baRegName, Length(sRegName));
    end;

    RegPathFixPlugin.MakeRelativeJmp( State_RegPathFix,
                          'Load shared maps files',
                          @LoadSharedMapsHook,
                          $0041D4DB, 0);

    Result:= RegPathFixPlugin;
  end else
    Result := nil;
end;

procedure LoadSharedMaps;
var
  FindHandle: Integer;
  SearchRec: TAFindData;
  SharedMaps, SharedBasicGameData : Boolean;
  TAPath : String;
begin
  SharedMaps := IniSettings.SharedMapsPath <> '';
  SharedBasicGameData := IniSettings.SharedBasicGameData <> '';

  SetCurrentDirectoryToTAPath;
  TAPath := IncludeTrailingPathDelimiter(ExtractFilePath(SelfLocation));

  FindHandle := findfirst_HPI('rev31.GP3', @SearchRec, -1, 1);
  if FindHandle >= 0 then
  begin
    repeat
      InsertToHPIAry(PAnsiChar(TAPath + SearchRec.FileName), 1);
    until
      (findnext_HPI(FindHandle, @SearchRec) < 0);
    findclose_HPI(FindHandle);
  end;

  FindHandle := findfirst_HPI('*.CCX', @SearchRec, -1, 1);
  if FindHandle >= 0 then
  begin
    repeat
      InsertToHPIAry(PAnsiChar(TAPath + SearchRec.FileName), 1);
    until
      (findnext_HPI(FindHandle, @SearchRec) < 0);
    findclose_HPI(FindHandle);
  end;

  if SharedBasicGameData then
  begin
    SetCurrentDir(IniSettings.SharedBasicGameData);
    FindHandle := findfirst_HPI('*.CCX', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        InsertToHPIAry(PAnsiChar(IniSettings.SharedBasicGameData + SearchRec.FileName), 1);
      until
        (findnext_HPI(FindHandle, @SearchRec) < 0);
      findclose_HPI(FindHandle);
    end;
    SetCurrentDirectoryToTAPath;
  end;

  if SharedMaps then
  begin
    SetCurrentDir(IniSettings.SharedMapsPath);
    FindHandle := findfirst_HPI('*.CCX', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        InsertToHPIAry(PAnsiChar(IniSettings.SharedMapsPath + SearchRec.FileName), 1);
      until
        (findnext_HPI(FindHandle, @SearchRec) < 0);
      findclose_HPI(FindHandle);
    end;
    SetCurrentDirectoryToTAPath;
  end;

  FindHandle := findfirst_HPI('*.UFO', @SearchRec, -1, 1);
  if FindHandle >= 0 then
  begin
    repeat
      InsertToHPIAry(PAnsiChar(TAPath + SearchRec.FileName), 0);
    until
      (findnext_HPI(FindHandle, @SearchRec) < 0);
    findclose_HPI(FindHandle);
  end;

  if SharedMaps then
  begin
    SetCurrentDir(IniSettings.SharedMapsPath);
    FindHandle := findfirst_HPI('*.UFO', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        InsertToHPIAry(PAnsiChar(IniSettings.SharedMapsPath + SearchRec.FileName), 0);
      until
        (findnext_HPI(FindHandle, @SearchRec) < 0);
      findclose_HPI(FindHandle);
    end;
    SetCurrentDirectoryToTAPath;
  end;

  FindHandle := findfirst_HPI('*.HPI', @SearchRec, -1, 1);
  if FindHandle >= 0 then
  begin
    repeat
      InsertToHPIAry(PAnsiChar(TAPath + SearchRec.FileName), 0);
    until
      (findnext_HPI(FindHandle, @SearchRec) < 0);
    findclose_HPI(FindHandle);
  end;

  if SharedBasicGameData then
  begin
    SetCurrentDir(IniSettings.SharedBasicGameData);
    FindHandle := findfirst_HPI('*.HPI', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        InsertToHPIAry(PAnsiChar(IniSettings.SharedBasicGameData + SearchRec.FileName), 0);
      until
        (findnext_HPI(FindHandle, @SearchRec) < 0);
      findclose_HPI(FindHandle);
    end;
  end;

  if SharedMaps then
  begin
    SetCurrentDir(IniSettings.SharedMapsPath);
    FindHandle := findfirst_HPI('*.HPI', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        InsertToHPIAry(PAnsiChar(IniSettings.SharedMapsPath + SearchRec.FileName), 0);
      until
        (findnext_HPI(FindHandle, @SearchRec) < 0);
      findclose_HPI(FindHandle);
    end;
  end;

  SetCurrentDirectoryToTAPath;
end;

procedure LoadSharedMapsHook;
asm
    pushAD
    call LoadSharedMaps
    popAD
    // before rev31
    //push $0041D4E0;
    // after ta hpi readings
    push $0041D5F3;
    call PatchNJump;
end;

end.

