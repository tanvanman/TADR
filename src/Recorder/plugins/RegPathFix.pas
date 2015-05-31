unit RegPathFix;

// set main registry path of game settings to path that is based on game modification
// RegName field property (check mods.ini), so every mod have its own settings

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_RegPathFix: Boolean = True;

const
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

procedure LoadCommonMaps;
var
  FindHandle: Integer;
  SearchRec: TAFindData;
  CommonMaps, CommonGameData : Boolean;
  TAPath : String;
  sRev31Name : String;
begin
  CommonMaps := (IniSettings.CommonMapsPath <> '') and
    IniSettings.UseCommonMaps;
  CommonGameData := (IniSettings.CommonGameDataPath <> '') and
    IniSettings.UseCommonGameData;

  SetCurrentDirectoryToTAPath;
  TAPath := IncludeTrailingPathDelimiter(ExtractFilePath(SelfLocation));
  sRev31Name := Format(PAnsiChar(Rev31GP3_Name), [PAnsiChar(Rev31GP3_31)]);
  FindHandle := HAPIFILE_FindFirst(PAnsiChar(sRev31Name), @SearchRec, -1, 1);
  if FindHandle >= 0 then
  begin
    repeat
      HAPIFILE_InsertToArray(PAnsiChar(TAPath + SearchRec.FileName), 1);
    until
      (HAPIFILE_FindNext(FindHandle, @SearchRec) < 0);
    HAPIFILE_FindClose(FindHandle);
  end;

  FindHandle := HAPIFILE_FindFirst('*.CCX', @SearchRec, -1, 1);
  if FindHandle >= 0 then
  begin
    repeat
      HAPIFILE_InsertToArray(PAnsiChar(TAPath + SearchRec.FileName), 1);
    until
      (HAPIFILE_FindNext(FindHandle, @SearchRec) < 0);
    HAPIFILE_FindClose(FindHandle);
  end;

  if CommonGameData then
  begin
    SetCurrentDir(IniSettings.CommonGameDataPath);
    FindHandle := HAPIFILE_FindFirst('*.CCX', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        HAPIFILE_InsertToArray(PAnsiChar(IniSettings.CommonGameDataPath + SearchRec.FileName), 1);
      until
        (HAPIFILE_FindNext(FindHandle, @SearchRec) < 0);
      HAPIFILE_FindClose(FindHandle);
    end;
    SetCurrentDirectoryToTAPath;
  end;

  if CommonMaps then
  begin
    SetCurrentDir(IniSettings.CommonMapsPath);
    FindHandle := HAPIFILE_FindFirst('*.CCX', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        HAPIFILE_InsertToArray(PAnsiChar(IniSettings.CommonMapsPath + SearchRec.FileName), 1);
      until
        (HAPIFILE_FindNext(FindHandle, @SearchRec) < 0);
      HAPIFILE_FindClose(FindHandle);
    end;
    SetCurrentDirectoryToTAPath;
  end;

  FindHandle := HAPIFILE_FindFirst('*.UFO', @SearchRec, -1, 1);
  if FindHandle >= 0 then
  begin
    repeat
      HAPIFILE_InsertToArray(PAnsiChar(TAPath + SearchRec.FileName), 0);
    until
      (HAPIFILE_FindNext(FindHandle, @SearchRec) < 0);
    HAPIFILE_FindClose(FindHandle);
  end;

  if CommonMaps then
  begin
    SetCurrentDir(IniSettings.CommonMapsPath);
    FindHandle := HAPIFILE_FindFirst('*.UFO', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        HAPIFILE_InsertToArray(PAnsiChar(IniSettings.CommonMapsPath + SearchRec.FileName), 0);
      until
        (HAPIFILE_FindNext(FindHandle, @SearchRec) < 0);
      HAPIFILE_FindClose(FindHandle);
    end;
    SetCurrentDirectoryToTAPath;
  end;

  FindHandle := HAPIFILE_FindFirst('*.HPI', @SearchRec, -1, 1);
  if FindHandle >= 0 then
  begin
    repeat
      HAPIFILE_InsertToArray(PAnsiChar(TAPath + SearchRec.FileName), 0);
    until
      (HAPIFILE_FindNext(FindHandle, @SearchRec) < 0);
    HAPIFILE_FindClose(FindHandle);
  end;

  if CommonGameData then
  begin
    SetCurrentDir(IniSettings.CommonGameDataPath);
    FindHandle := HAPIFILE_FindFirst('*.HPI', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        HAPIFILE_InsertToArray(PAnsiChar(IniSettings.CommonGameDataPath + SearchRec.FileName), 0);
      until
        (HAPIFILE_FindNext(FindHandle, @SearchRec) < 0);
      HAPIFILE_FindClose(FindHandle);
    end;
  end;

  if CommonMaps then
  begin
    SetCurrentDir(IniSettings.CommonMapsPath);
    FindHandle := HAPIFILE_FindFirst('*.HPI', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        HAPIFILE_InsertToArray(PAnsiChar(IniSettings.CommonMapsPath + SearchRec.FileName), 0);
      until
        (HAPIFILE_FindNext(FindHandle, @SearchRec) < 0);
      HAPIFILE_FindClose(FindHandle);
    end;
  end;

  SetCurrentDirectoryToTAPath;
end;

procedure LoadCommonDataHook;
asm
  pushAD
  call LoadCommonMaps
  popAD
  // before rev31
  //push $0041D4E0;
  // after ta hpi readings
  push $0041D5F3;
  call PatchNJump;
end;

function LocateMovieInCommonDir(FilePath: PAnsiChar): Integer; stdcall;
var
  CommonGameData: Boolean;
begin
  CommonGameData := (IniSettings.CommonGameDataPath <> '') and
    IniSettings.UseCommonGameData;
  if CommonGameData then
    SetCurrentDir(IniSettings.CommonGameDataPath);
  Result := HAPIFILE_GetFileLength(FilePath);
end;

procedure LocateMovieInCommonDirHook;
asm
  push edx
  call LocateMovieInCommonDir;
  push $004267B1;
  call PatchNJump;
end;

procedure SetDirAfterMovie;
asm
  call SetCurrentDirectoryToTAPath
  mov  eax, [TAdynmemStructPtr]
  push $0042687D;
  call PatchNJump;
end;

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
  CurrentProcessHandle : THandle;
  CommittedBytes : Longword;
  OldProtect, tmpOldProtect : longword;

begin
  if IsTAVersion31 and State_RegPathFix then
  begin
    Result := TPluginData.create( True, '',
                                  State_RegPathFix,
                                  @OnInstallRegPathFix,
                                  @OnUninstallRegPathFix );

    if IniSettings.RegName <> '' then
    begin
      sRegName := Copy(IniSettings.RegName, 1, 21);
      sRegName := LeftPad(sRegName, #0, 21);

      Move(sRegName[1], baRegName, Length(sRegName));
      CurrentProcessHandle := GetCurrentProcess;

      Win32Check(VirtualProtect(Pointer($0050DDFD), Length(sRegName), PAGE_READWRITE, OldProtect));
      Win32Check(WriteProcessMemory(CurrentProcessHandle, Pointer($0050DDFD),
        @baRegName[0], Length(sRegName), CommittedBytes) );
      FlushInstructionCache(CurrentProcessHandle, Pointer($0050DDFD), Length(sRegName));
      Win32Check(VirtualProtect(Pointer($0050DDFD), Length(sRegName), OldProtect, tmpOldProtect));

      Win32Check(VirtualProtect(Pointer($00509EB8), Length(sRegName), PAGE_READWRITE, OldProtect));
      Win32Check(WriteProcessMemory(CurrentProcessHandle, Pointer($00509EB8),
        @baRegName[0], Length(sRegName), CommittedBytes) );
      FlushInstructionCache(CurrentProcessHandle, Pointer($00509EB8), Length(sRegName));
      Win32Check(VirtualProtect(Pointer($00509EB8), Length(sRegName), OldProtect, tmpOldProtect));
    end;

    Registry := TRegistry.Create(KEY_READ or KEY_WOW64_64KEY);
    try
      Registry.RootKey := HKEY_LOCAL_MACHINE;
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
    
    Result.MakeRelativeJmp( State_RegPathFix,
                            'Load common maps and game data files',
                            @LoadCommonDataHook,
                            $0041D4DB, 0 );

    Result.MakeRelativeJmp( State_RegPathFix,
                            'Load movies from common data dir',
                            @LocateMovieInCommonDirHook,
                            $004267AB, 1 );

    Result.MakeRelativeJmp( State_RegPathFix,
                            'Sets dir to TA after movie has finished',
                            @SetDirAfterMovie,
                            $00426878, 0 );
  end else
    Result := nil;
end;

end.

