unit IniOptions;

{$DEFINE GameStatsLogging}

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
    RanksURL             : String;
    UnitLimit            : Integer;
    Read                 : Boolean;

    ScriptorPath         : String;
    SharedMapsPath       : String;

    Plugin_Builders      : Boolean;
    Plugin_AiNukes       : Boolean;
    Plugin_AiBuildList   : Boolean;

    CreateStatsFile      : Boolean;

    { GUI Plugins }
    Plugin_Colors        : Boolean;
    Colors               : array [0..30] of Cardinal;
    Plugin_HBWidth       : Integer;
    Plugin_HBDynamicSize : Boolean;
    Plugin_HBCategory1   : Cardinal;
    Plugin_HBCategory2   : Cardinal;
    Plugin_HBCategory3   : Cardinal;
    Plugin_HBCategory4   : Cardinal;
    Plugin_HBCategory5   : Cardinal;

    Plugin_TrueIncome    : Boolean;
    Plugin_ClockPosition : Byte;

    Plugin_MinWeaponReload  : Integer;
    Plugin_Transporters  : Boolean;
    Plugin_Stockpile     : Boolean;

    Plugin_CircleUnitSelect : Boolean;
    Plugin_ForceDrawBuildSpotNano : Boolean;
    Plugin_BuildSpotNanoShimmer : Boolean;
    Plugin_DrawBuildSpotQueueNano : Boolean;

    
    Plugin_MinReclaimTime : Integer;
    Plugin_Gaf : Boolean;
  end;
var IniSettings: TIniSettings;

type
  TIniColors = (UNITSELECTIONBOX = 0,
                UNITHEALTHBARGOOD,
                UNITHEALTHBARMEDIUM,
                UNITHEALTHBARLOW,
                WEAPONRELOADBAR,
                RECLAIMBAR,
                STOCKPILEBAR,
                BUILDQUEUEBOXSELECTED1,
                BUILDQUEUEBOXSELECTED2,
                BUILDQUEUEBOXNONSELECTED1,
                BUILDQUEUEBOXNONSELECTED2,
                LOADBARSTEXTURESREADY,
                LOADBARSTEXTURESLOADING,
                LOADBARSTERRAINREADY,
                LOADBARSTERRAINLOADING,
                LOADBARSUNITSREADY,
                LOADBARSUNITSLOADING,
                LOADBARSANIMATIONSREADY,
                LOADBARSANIMATIONSLOADING,
                LOADBARS3DDATAREADY,
                LOADBARS3DDATALOADING,
                LOADBARSEXPLOSIONSREADY,
                LOADBARSEXPLOSIONSLOADING,
                MAINMENUDOTS,
                NANOLATHEPARTICLEBASE,
                NANOLATHEPARTICLECOLORS,
                UNDERCONSTRUCTSURFACELO,
                UNDERCONSTRUCTSURFACEHI,
                UNDERCONSTRUCTOUTLINELO,
                UNDERCONSTRUCTOUTLINEHI,
                MAINMENUDOTSDISABLED);

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
  TA_MemoryConstants,
  ModsList,
  logging,
  strUtils,
  TypInfo;
const
  INI_MEM_OFFSET = $5098A3;

type TIniFileName = array [0..12] of AnsiChar;
PIniFileName = ^TIniFileName;

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
    iniName:= PIniFileName(INI_MEM_OFFSET)^;
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

  function ReadIniBool(var iniFile: TIniFile; sect: string; ident: string; default: boolean): Boolean;
  var
    temp: string;
  begin
    Result:= Default;
    if iniFile.ValueExists(sect, ident) then
    begin
      temp:= iniFile.ReadString(sect, ident, '');
      if Length(temp) > 1 then
      begin
        if Pos('FALSE', temp) <> 0 then
          Result:= False;
        if Pos('TRUE', temp) <> 0 then
          Result:= True;
      end else
      begin
        TLog.Add(0, 'Error reading setting [' + ident + ']');
        Exit;
      end;
    end;
  end;

  function ReadIniValue(var iniFile: TIniFile; sect: string; ident: string; default: integer): Integer;
  var
    temp: string;
  begin
    Result:= default;
    if iniFile.ValueExists(sect, ident) then
    begin
      temp:= iniFile.ReadString(sect, ident, '');
      if Length(temp) >= 1 then
      begin
        try
          if Pos(';', temp) <> 0 then
            temp:= LeftStr(temp, Length(temp) - 1);
          Trim(temp);
          Result:= StrToInt(temp);
        except
          TLog.Add(0, 'Error reading setting [' + ident + ']');
          Exit;
        end;
      end else
        Result:= Default;
    end;
  end;

  function ReadIniString(var iniFile: TIniFile; sect: string; ident: string; default: string): String;
  var
    temp: string;
  begin
    Result:= default;
    if iniFile.ValueExists(sect, ident) then
    begin
      temp:= iniFile.ReadString(sect, ident, '');
      if Length(temp) > 1 then
      begin
        try
          if Pos(';', temp) <> 0 then
            temp:= LeftStr(temp, Length(temp) - 1);
          Trim(temp);
          Result:= temp;
        except
          TLog.Add(0, 'Error reading setting [' + ident + ']');
          Exit;
        end;
      end else
        Result:= Default;
    end;
  end;

function ReadIniPath(var iniFile: TIniFile; sect: string; ident: string; out path: string): boolean;
  var
    temp: string;
  begin
    Result:= false;
    if iniFile.ValueExists(sect, ident) then
    begin
      temp:= iniFile.ReadString(sect, ident, '');
      if Length(temp) > 1 then
      begin
        try
          if Pos(';', temp) <> 0 then
            temp:= LeftStr(temp, Length(temp) - 1);

          Result:= true;
        except
          TLog.Add(0, 'Error reading setting [' + ident + ']');
          Exit;
        end;
      end else
        Result:= false;
    end;
  end;

  function ReadIniDword(var iniFile: TIniFile; sect: string; ident: string; default: dword): Integer;
  var
    temp: string;
  begin
    Result:= default;
    if iniFile.ValueExists(sect, ident) then
    begin
      temp:= iniFile.ReadString(sect, ident, '');
      if Length(temp) > 1 then
      begin
        try
          Trim(temp);
          temp:= Copy(temp, Pos(':', temp) + 1, Length(temp) - Pos(':', temp));
          Result:= StrToInt(temp);
        except
          TLog.Add(0, 'Error reading setting [' + ident + ']');
          Exit;
        end;
      end else
        Result:= Default;
    end;
  end;

var
  iniFile: TIniFile;
  MultiGameWeapon: Boolean;
  i: integer;
  currcolor: string;
begin
  Result:= False;

  IniSettings.ModId := -1;
  IniSettings.DemosPrefix := '';
  IniSettings.WeaponIdPatch := False;
  IniSettings.UnitLimit := 1500;

  if GetINIFileName <> #0 then
  begin
    iniFile:= TIniFile.Create(GetINIFileName);
    try
      IniSettings.ModId := ReadIniValue(iniFile, 'MOD','ID', -1);
      IniSettings.Name := ReadIniString(iniFile, 'MOD','Name', '');
      IniSettings.Version := ReadIniString(iniFile, 'MOD','Version', '');
      IniSettings.RegName := ReadIniString(iniFile, 'MOD','RegName', '');

      IniSettings.DemosPrefix := ReadIniString(iniFile, 'MOD','DemosFileNamePrefix', '');

      IniSettings.RanksURL := ReadIniString(iniFile, 'Preferences','RanksURL', '');

      IniSettings.UnitType := ReadIniValue(iniFile, 'Preferences', 'UnitType', 512);
      IniSettings.WeaponType := ReadIniValue(iniFile, 'Preferences', 'WeaponType', 256);
      multiGameWeapon := ReadIniBool(iniFile, 'Preferences','MultiGameWeapon', False);
      IniSettings.WeaponIdPatch := (IniSettings.WeaponType > 256) and multiGameWeapon;

      IniSettings.UnitLimit := ReadIniValue(iniFile, 'Preferences', 'UnitLimit', 0);

      IniSettings.ScriptorPath := ReadIniString(iniFile, 'Preferences', 'ScriptorIncludePath', '');
      if IniSettings.ScriptorPath <> '' then
        IniSettings.ScriptorPath := IncludeTrailingPathDelimiter(IniSettings.ScriptorPath);

      IniSettings.SharedMapsPath := ReadIniString(iniFile, 'Preferences', 'SharedMapsPath', '');
      if IniSettings.SharedMapsPath <> '' then
        IniSettings.SharedMapsPath := IncludeTrailingPathDelimiter(IniSettings.SharedMapsPath);

      IniSettings.Plugin_Builders := ReadIniBool(iniFile, 'Preferences', 'Builders', False);
      IniSettings.Plugin_AiNukes := ReadIniBool(iniFile, 'Preferences', 'AiNukes', False);
      IniSettings.Plugin_AiBuildList := ReadIniBool(iniFile, 'Preferences', 'AiBuildListExpand', False);

      IniSettings.CreateStatsFile := ReadIniBool(iniFile, 'Preferences', 'CreateStats', False);

      if iniFile.SectionExists('Colors') then
      begin
        IniSettings.Plugin_Colors := True;
        for i:= 0 to 29 do
        begin
          currcolor := GetEnumName(TypeInfo(TIniColors), i);
          IniSettings.Colors[i] := ReadIniValue(iniFile, 'Colors', UpperCase(currcolor), 0);
        end;
        if ReadIniBool(iniFile, 'Colors','MainMenuDotsDisabled', False) then
          IniSettings.Colors[30]:= 1
        else
          IniSettings.Colors[30]:= 0;
      end;

      IniSettings.Plugin_HBWidth := ReadIniValue(iniFile, 'Preferences', 'HealthBarWidth', 0);
      IniSettings.Plugin_HBDynamicSize := ReadIniBool(iniFile, 'Preferences', 'HealthBarDynamicSize', False);
      IniSettings.Plugin_HBCategory1 := ReadIniValue(iniFile, 'Preferences', 'HealthBarDynamicCat1', 0);
      IniSettings.Plugin_HBCategory2 := ReadIniValue(iniFile, 'Preferences', 'HealthBarDynamicCat2', 0);
      IniSettings.Plugin_HBCategory3 := ReadIniValue(iniFile, 'Preferences', 'HealthBarDynamicCat3', 0);
      IniSettings.Plugin_HBCategory4 := ReadIniValue(iniFile, 'Preferences', 'HealthBarDynamicCat4', 0);
      IniSettings.Plugin_HBCategory5 := ReadIniValue(iniFile, 'Preferences', 'HealthBarDynamicCat5', 0);
      IniSettings.Plugin_MinWeaponReload := ReadIniValue(iniFile, 'Preferences', 'MinWeaponReloadTime', 0);
      IniSettings.Plugin_Transporters := ReadIniBool(iniFile, 'Preferences', 'TransportersCount', False);
      IniSettings.Plugin_Stockpile := ReadIniBool(iniFile, 'Preferences', 'StockpileCount', False);
      IniSettings.Plugin_CircleUnitSelect := ReadIniBool(iniFile, 'Preferences', 'CircleUnitSelect', False);
      IniSettings.Plugin_ForceDrawBuildSpotNano := ReadIniBool(iniFile, 'Preferences', 'ForceDrawBuildSpotNano', False);
      IniSettings.Plugin_DrawBuildSpotQueueNano := ReadIniBool(iniFile, 'Preferences', 'DrawBuildSpotQueueNano', False);
      IniSettings.Plugin_BuildSpotNanoShimmer := ReadIniBool(iniFile, 'Preferences', 'BuildSpotNanoShimmer', False);

      IniSettings.Plugin_TrueIncome := ReadIniBool(iniFile, 'Preferences', 'TrueIncome', False);

      IniSettings.Plugin_ClockPosition := ReadIniValue(iniFile, 'Preferences', 'ClockPosition', 0);
      IniSettings.Plugin_MinReclaimTime := ReadIniValue(iniFile, 'Preferences', 'MinReclaimTime', 0);

      IniSettings.Plugin_Gaf := ReadIniBool(iniFile, 'Preferences', 'GAFExt', False);
    finally
      Result:= True;
      iniFile.Free;
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
