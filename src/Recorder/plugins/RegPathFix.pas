unit RegPathFix;

// set main registry path of game settings to path that is based on game modification
// RegName field property (check mods.ini), so every mod have its own settings

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_RegPathFix : boolean = true;
  KEY_WOW64_64KEY = $0100;

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
  Registry,
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
  Registry : TRegistry;
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

    Registry := TRegistry.Create(KEY_READ or KEY_WOW64_64KEY);
    try
      Registry.RootKey := HKEY_CURRENT_USER;
      if Registry.KeyExists('Software\TA Patch\') then
        if Registry.OpenKey('Software\TA Patch\', False) then
        begin
          IniSettings.CommonMapsPath := Registry.ReadString('CommonMapsPath');
          if IniSettings.CommonMapsPath <> '' then
            IniSettings.CommonMapsPath := IncludeTrailingPathDelimiter(IniSettings.CommonMapsPath);

          IniSettings.CommonGameDataPath := Registry.ReadString('CommonGameDataPath');
          if IniSettings.CommonGameDataPath <> '' then
            IniSettings.CommonGameDataPath := IncludeTrailingPathDelimiter(IniSettings.CommonGameDataPath);
        end;
    finally
      Registry.CloseKey;
      Registry.Free;
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
  sRev31Name : String;
begin
  SharedMaps := IniSettings.CommonMapsPath <> '';
  SharedBasicGameData := IniSettings.CommonGameDataPath <> '';

  SetCurrentDirectoryToTAPath;
  TAPath := IncludeTrailingPathDelimiter(ExtractFilePath(SelfLocation));
  sRev31Name := Format(PAnsiChar($005028CC), [PAnsiChar($005028D8)]);
  FindHandle := findfirst_HPI(PAnsiChar(sRev31Name), @SearchRec, -1, 1);
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
    SetCurrentDir(IniSettings.CommonGameDataPath);
    FindHandle := findfirst_HPI('*.CCX', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        InsertToHPIAry(PAnsiChar(IniSettings.CommonGameDataPath + SearchRec.FileName), 1);
      until
        (findnext_HPI(FindHandle, @SearchRec) < 0);
      findclose_HPI(FindHandle);
    end;
    SetCurrentDirectoryToTAPath;
  end;

  if SharedMaps then
  begin
    SetCurrentDir(IniSettings.CommonMapsPath);
    FindHandle := findfirst_HPI('*.CCX', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        InsertToHPIAry(PAnsiChar(IniSettings.CommonMapsPath + SearchRec.FileName), 1);
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
    SetCurrentDir(IniSettings.CommonMapsPath);
    FindHandle := findfirst_HPI('*.UFO', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        InsertToHPIAry(PAnsiChar(IniSettings.CommonMapsPath + SearchRec.FileName), 0);
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
    SetCurrentDir(IniSettings.CommonGameDataPath);
    FindHandle := findfirst_HPI('*.HPI', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        InsertToHPIAry(PAnsiChar(IniSettings.CommonGameDataPath + SearchRec.FileName), 0);
      until
        (findnext_HPI(FindHandle, @SearchRec) < 0);
      findclose_HPI(FindHandle);
    end;
  end;

  if SharedMaps then
  begin
    SetCurrentDir(IniSettings.CommonMapsPath);
    FindHandle := findfirst_HPI('*.HPI', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        InsertToHPIAry(PAnsiChar(IniSettings.CommonMapsPath + SearchRec.FileName), 0);
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

