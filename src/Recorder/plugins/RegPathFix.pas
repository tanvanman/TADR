unit RegPathFix;

// set main registry path of game settings to path that is based on game modification
// RegName field property (check mods.ini), so every mod have its own settings

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_RegPathFix : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallRegPathFix;
Procedure OnUninstallRegPathFix;

// -----------------------------------------------------------------------------

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  ModsList,
  textdata,
  Sysutils,
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

    Result:= RegPathFixPlugin;
  end else
    Result := nil;
end;

end.

