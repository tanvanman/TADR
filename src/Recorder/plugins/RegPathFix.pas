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
  TA_MemoryLocations;

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
                          $0041D5F3, 0);

    Result:= RegPathFixPlugin;
  end else
    Result := nil;
end;

procedure LoadSharedMaps;
var
  FindHandle: Integer;
  SearchRec: TAFindData;
begin
  if IniSettings.SharedMapsPath <> '' then
  begin
    SetCurrentDir(IniSettings.SharedMapsPath);
    FindHandle := findfirst_HPI('*.UFO', @SearchRec, -1, 1);
    if FindHandle >= 0 then
    begin
      repeat
        InsertToHPIAry(PAnsiChar(IniSettings.SharedMapsPath + SearchRec.FileName), 0);
      until
        (findnext_HPI(FindHandle, @SearchRec) < 0);
    end;
    SetCurrentDirectoryToTAPath;
  end;
end;

procedure LoadSharedMapsHook;
asm
    pushAD
    call LoadSharedMaps
    popAD
    mov     [esp+268h-$25C], 0
    push $0041D5F8;
    call PatchNJump;
end;

end.

