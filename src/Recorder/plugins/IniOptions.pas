unit IniOptions;

interface
uses PluginEngine, Windows, SysUtils, IniFiles;

type
  TIniSettings = record
    ModId                : Integer;
    DemosPrefix          : String;
    Name                 : String;
    RegName              : String;
    Version              : String;

    UnitType             : Integer;
    WeaponType           : Integer;
    WeaponIdPatch        : Boolean;
//    RanksURL             : String;
    UnitLimit            : Integer;
    Read                 : Boolean;

    ScriptorPath           : String;
    CommonMapsPath         : String;
    CommonGameDataPath     : String;
    UseCommonMaps          : Boolean;
    UseCommonGameData      : Boolean;

    AiNukes                : Boolean;
    AiBuildList            : Boolean;
    ExpandMinimap          : Boolean;

    CreateStatsFile        : Boolean;

    { GUI Plugins }
    Colors                 : Boolean;
    CustomColors           : array[0..3] of array[0..28] of Byte;
    Colors_MenuDots        : Byte;
    Colors_DisableMenuDots : Boolean;
    HBWidth                : Integer;
    HBDynamicSize          : Boolean;
    HBCategory1            : Cardinal;
    HBCategory2            : Cardinal;
    HBCategory3            : Cardinal;
    HBCategory4            : Cardinal;
    HBCategory5            : Cardinal;

    BroadcastNanolathe     : Boolean;
    ClockPosition          : Byte;

    MinWeaponReload        : Integer;
    Transporters           : Boolean;
    Stockpile              : Boolean;
    BattleRoomEnh          : Boolean;
    ScriptSlotsLimit       : Boolean;
    InterceptsOnlyList     : Boolean;

    ForceDrawBuildSpotNano : Boolean;
    BuildSpotNanoShimmer   : Boolean;
    DrawBuildSpotQueueNano : Boolean;

    StopButton             : Boolean;
    ResurrectPatrol        : Boolean;
    
    MinReclaimTime         : Integer;
  end;
  
var
  IniSettings: TIniSettings;

// -----------------------------------------------------------------------------

const
  State_INI_Options : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallINI_Options;
Procedure OnUninstallINI_Options;
function GetINIFileName: string;

// -----------------------------------------------------------------------------

implementation
uses
  TADemoConsts,
  TA_MemoryLocations,
  TA_MemoryStructures,
  TA_MemoryConstants,
  ModsList,
  logging,
  Colors,
  strUtils,
  TypInfo;

type
  PIniFileName = ^TIniFileName;
  TIniFileName = array[0..12] of AnsiChar;

function TaFileExists(name: string; out path: string): boolean;
var
  taDir: string;
begin
  Result:= False;
  path:= '';
  if Length(name) > 0 then
  begin
    taDir:= IncludeTrailingPathDelimiter(ExtractFilePath(SelfLocation));
    if name[1] = '\' then
      name:= Copy(name, 2, Length(name) - 1);
    if FileExists(taDir + name) then
    begin
      path:= taDir + name;
      Result:= True;
    end;
  end;
end;

// read INI file name from TA's memory
var
  iniPath_cache : string;
function GetINIFileName: string;
var
 iniName: string;
 tempPath: string;
begin
if iniPath_cache = '' then
begin
  Result:= #0;
  try
    iniName:= PIniFileName(Totala_ini)^;
    Trim(iniName);
    if TaFileExists(iniName, tempPath) then
      iniPath_cache:= tempPath
    else
      iniPath_cache:= #0;
    result:= iniPath_cache;
  except
    //shouldn't fail, however ...
    if TaFileExists('totala.ini', tempPath) then
      iniPath_cache:= tempPath
    else
      iniPath_cache:= #0;
    result:= iniPath_cache;
  end;
end else
  result:= iniPath_cache;
end;

function ReadINISettings: boolean;

  function ReadIniBool(var IniFile: TIniFile; sect: string; ident: string; default: boolean): Boolean;
  var
    temp: string;
  begin
    Result:= Default;
    if IniFile.ValueExists(sect, ident) then
    begin
      temp:= IniFile.ReadString(sect, ident, '');
      if Length(temp) > 1 then
      begin
        if Pos('FALSE', temp) <> 0 then
          Result:= False;
        if Pos('TRUE', temp) <> 0 then
          Result:= True;
      end else
      begin
        //TLog.Add(0, 'Error reading setting [' + ident + ']');
        Exit;
      end;
    end;
  end;

  function ReadIniValue(var IniFile: TIniFile; sect: string; ident: string; default: integer): Integer;
  var
    temp: string;
  begin
    Result:= default;
    if IniFile.ValueExists(sect, ident) then
    begin
      temp:= IniFile.ReadString(sect, ident, '');
      if Length(temp) >= 1 then
      begin
        try
          if Pos(';', temp) <> 0 then
            temp:= LeftStr(temp, Length(temp) - 1);
          Trim(temp);
          Result:= StrToInt(temp);
        except
          Exit;
        end;
      end else
        Result:= Default;
    end;
  end;

  function ReadIniString(var IniFile: TIniFile; sect: string; ident: string; default: string): String;
  var
    temp: string;
  begin
    Result:= default;
    if IniFile.ValueExists(sect, ident) then
    begin
      temp:= IniFile.ReadString(sect, ident, '');
      if Length(temp) > 1 then
      begin
        try
          if Pos(';', temp) <> 0 then
            temp:= LeftStr(temp, Length(temp) - 1);
          Trim(temp);
          Result:= temp;
        except
          //TLog.Add(0, 'Error reading setting [' + ident + ']');
          Exit;
        end;
      end else
        Result:= Default;
    end;
  end;

function ReadIniPath(var IniFile: TIniFile; sect: string; ident: string; out path: string): boolean;
  var
    temp: string;
  begin
    Result:= false;
    if IniFile.ValueExists(sect, ident) then
    begin
      temp:= IniFile.ReadString(sect, ident, '');
      if Length(temp) > 1 then
      begin
        try
          if Pos(';', temp) <> 0 then
            temp:= LeftStr(temp, Length(temp) - 1);

          Result:= true;
        except
          //TLog.Add(0, 'Error reading setting [' + ident + ']');
          Exit;
        end;
      end else
        Result:= false;
    end;
  end;

  function ReadIniDword(var IniFile: TIniFile; sect: string; ident: string; default: dword): Integer;
  var
    temp: string;
  begin
    Result:= default;
    if IniFile.ValueExists(sect, ident) then
    begin
      temp:= IniFile.ReadString(sect, ident, '');
      if Length(temp) > 1 then
      begin
        try
          Trim(temp);
          temp:= Copy(temp, Pos(':', temp) + 1, Length(temp) - Pos(':', temp));
          Result:= StrToInt(temp);
        except
          //TLog.Add(0, 'Error reading setting [' + ident + ']');
          Exit;
        end;
      end else
        Result:= Default;
    end;
  end;

var
  IniFile: TIniFile;
  MultiGameWeapon: Boolean;
  i: integer;
begin
  Result:= False;

  IniSettings.ModId := -1;
  IniSettings.DemosPrefix := '';
  IniSettings.WeaponIdPatch := False;
  IniSettings.UnitLimit := 1500;

  if GetINIFileName <> #0 then
  begin
    IniFile:= TIniFile.Create(GetINIFileName);
    try
      IniSettings.ModId := ReadIniValue(IniFile, 'MOD','ID', -1);
      IniSettings.Name := ReadIniString(IniFile, 'MOD','Name', '');
      IniSettings.Version := ReadIniString(IniFile, 'MOD','Version', '');
      IniSettings.RegName := ReadIniString(IniFile, 'Preferences','RegistryName', '');

      IniSettings.DemosPrefix := ReadIniString(IniFile, 'MOD','DemosFileNamePrefix', '');

//      IniSettings.RanksURL := ReadIniString(IniFile, 'Preferences','RanksURL', '');

      IniSettings.UnitType := ReadIniValue(IniFile, 'Preferences', 'UnitType', 512);
      IniSettings.WeaponType := ReadIniValue(IniFile, 'Preferences', 'WeaponType', 256);
      multiGameWeapon := ReadIniBool(IniFile, 'Preferences','MultiGameWeapon', False);
      IniSettings.WeaponIdPatch := (IniSettings.WeaponType > 256) and multiGameWeapon;

      IniSettings.UnitLimit := ReadIniValue(IniFile, 'Preferences', 'UnitLimit', 0);

      IniSettings.ScriptorPath := ReadIniString(IniFile, 'Preferences', 'ScriptorIncludePath', '');
      if IniSettings.ScriptorPath <> '' then
        IniSettings.ScriptorPath := IncludeTrailingPathDelimiter(IniSettings.ScriptorPath);

      IniSettings.AiNukes := ReadIniBool(IniFile, 'Preferences', 'AiNukes', False);
      IniSettings.AiBuildList := ReadIniBool(IniFile, 'Preferences', 'AiBuildListExpand', False);

      IniSettings.ExpandMinimap := ReadIniBool(IniFile, 'Preferences', 'ExpandMinimap', False);

      IniSettings.CreateStatsFile := ReadIniBool(IniFile, 'Preferences', 'CreateStats', False);

      if IniFile.SectionExists('Colors') or
         IniFile.SectionExists('ColorsSide1') or
         IniFile.SectionExists('ColorsSide2') or
         IniFile.SectionExists('ColorsSide3') then
      begin
        IniSettings.Colors := True;
        for i := Low(ColorsArray) to High(ColorsArray) - 2 do
        begin
          IniSettings.CustomColors[0][i] := ReadIniValue(IniFile, 'Colors', ColorsArray[i].sName, 0);
          IniSettings.CustomColors[1][i] := ReadIniValue(IniFile, 'ColorsSide1', ColorsArray[i].sName, 0);
          IniSettings.CustomColors[2][i] := ReadIniValue(IniFile, 'ColorsSide2', ColorsArray[i].sName, 0);
          IniSettings.CustomColors[3][i] := ReadIniValue(IniFile, 'ColorsSide3', ColorsArray[i].sName, 0);
        end;
        IniSettings.Colors_MenuDots := ReadIniValue(IniFile, 'Colors', ColorsArray[29].sName, 0);
        IniSettings.Colors_DisableMenuDots := ReadIniBool(IniFile, 'Colors', ColorsArray[30].sName, False);
      end;

      IniSettings.HBWidth := ReadIniValue(IniFile, 'Preferences', 'HealthBarWidth', 0);
      IniSettings.HBDynamicSize := ReadIniBool(IniFile, 'Preferences', 'HealthBarDynamicSize', False);
      IniSettings.HBCategory1 := ReadIniValue(IniFile, 'Preferences', 'HealthBarDynamicCat1', 0);
      IniSettings.HBCategory2 := ReadIniValue(IniFile, 'Preferences', 'HealthBarDynamicCat2', 0);
      IniSettings.HBCategory3 := ReadIniValue(IniFile, 'Preferences', 'HealthBarDynamicCat3', 0);
      IniSettings.HBCategory4 := ReadIniValue(IniFile, 'Preferences', 'HealthBarDynamicCat4', 0);
      IniSettings.HBCategory5 := ReadIniValue(IniFile, 'Preferences', 'HealthBarDynamicCat5', 0);
      IniSettings.MinWeaponReload := ReadIniValue(IniFile, 'Preferences', 'MinWeaponReloadTime', 0);
      IniSettings.MinReclaimTime := ReadIniValue(IniFile, 'Preferences', 'MinReclaimTime', 0);
      IniSettings.Transporters := ReadIniBool(IniFile, 'Preferences', 'TransportersCount', False);
      IniSettings.Stockpile := ReadIniBool(IniFile, 'Preferences', 'StockpileCount', False);
      IniSettings.ClockPosition := ReadIniValue(IniFile, 'Preferences', 'ClockPosition', 0);
      IniSettings.ForceDrawBuildSpotNano := ReadIniBool(IniFile, 'Preferences', 'ForceDrawBuildSpotNano', False);
      IniSettings.DrawBuildSpotQueueNano := ReadIniBool(IniFile, 'Preferences', 'DrawBuildSpotQueueNano', False);
      IniSettings.BuildSpotNanoShimmer := ReadIniBool(IniFile, 'Preferences', 'BuildSpotNanoShimmer', False);

      IniSettings.StopButton := ReadIniBool(IniFile, 'Preferences', 'StopButtonRemovesQueue', False);
      IniSettings.ResurrectPatrol := ReadIniBool(IniFile, 'Preferences', 'ResurrectionPatrol', False);

      IniSettings.BroadcastNanolathe := ReadIniBool(IniFile, 'Preferences', 'BroadcastNanolathe', False);
      IniSettings.BattleRoomEnh := ReadIniBool(IniFile, 'Preferences', 'BattleRoomEnhancements', False);

      IniSettings.ScriptSlotsLimit := ReadIniBool(IniFile, 'Preferences', 'IncScriptSlotsLimit', False);
      IniSettings.InterceptsOnlyList := ReadIniBool(IniFile, 'Preferences', 'UseInterceptsOnlyList', True);

      IniSettings.UseCommonMaps := ReadIniBool(IniFile, 'Preferences', 'UseCommonMaps', True);
      IniSettings.UseCommonGameData := ReadIniBool(IniFile, 'Preferences', 'UseCommonGameData', True);
    finally
      Result:= True;
      IniFile.Free;
    end;

    if FixModsINI then
    begin
      if IniSettings.Name = '' then
        if ReadModsIniField('Name') <> '' then
          IniSettings.Name:= ReadModsIniField('Name')
        else
          if IniSettings.ModId > 0 then
          begin
         //   TLog.Add( 0, 'Couldn''t read mod name !' );
            IniSettings.Name:= 'Unknown';
          end;
      if IniSettings.Version = '' then
        if ReadModsIniField('Version') <> '' then
          IniSettings.Version:= ReadModsIniField('Version');
      if IniSettings.RegName = '' then
        if ReadModsIniField('RegName') <> '' then
          IniSettings.RegName := ReadModsIniField('RegName')
        else
          IniSettings.RegName := 'TA Patch';
    end else
      if IniSettings.ModId > 0 then TLog.Add( 0, 'Couldn''t save mod id to mods.ini' );

    LocalModInfo.ModID := IniSettings.ModID;
    if IniSettings.Version <> '' then
    begin
      LocalModInfo.ModMajorVer := Copy(IniSettings.Version, 1, Pos('.', IniSettings.Version)-1 )[1];
      LocalModInfo.ModMinorVer := Copy(IniSettings.Version, Pos('.', IniSettings.Version) + 1, Length(IniSettings.Version))[1];
    end else
    begin
      LocalModInfo.ModMajorVer := '0';
      LocalModInfo.ModMinorVer := '0';
    end;
  end;
end;

Procedure OnInstallINI_Options;
begin
end;

Procedure OnUnInstallINI_Options;
begin
end;

function GetPlugin : TPluginData;
begin
if IsTAVersion31 and State_INI_Options then
  begin
	  result := TPluginData.create( false,
                                'totala.ini reader',
                                State_INI_Options,
                                @OnInstallINI_Options, @OnUnInstallINI_Options );

    IniSettings.read := ReadINISettings;     
    
  end else
    result := nil;
end;

end.
