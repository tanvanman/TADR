unit INI_Options;

interface
uses PluginEngine, Windows, SysUtils, IniFiles;

type
  TIniSettings = record
    modid: integer;
    demosprefix: string;
    name: string;
    regname: string;
    version: string;
    weaponidpatch: boolean;
    RanksURL: string;
    unitlimit: integer;
    Colors: array [0..27] of Integer;
    read: boolean;
    docolors: boolean;
  end;
var iniSettings: TIniSettings;

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
  ModsList,
  logging,
  strUtils;
const
  INIFILENAME_MAXLENGTH = 12;
  INI_MEM_OFFSET = $5098A3;

type TIniFileName = array [0..INIFILENAME_MAXLENGTH] of AnsiChar;
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
      if Length(temp) > 1 then
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
  WeaponType: Integer;
  MultiGameWeapon: Boolean;
begin
Result:= False;

iniSettings.modid:= 0;
iniSettings.demosprefix:= '';
iniSettings.weaponidpatch:= False;

if GetINIFileName <> #0 then
  begin
    iniFile:= TIniFile.Create(GetINIFileName);
    try
      iniSettings.modid:= ReadIniValue(iniFile, 'MOD','ID', -1);
      iniSettings.name:= ReadIniString(iniFile, 'MOD','Name', '');
      iniSettings.version:= ReadIniString(iniFile, 'MOD','Version', '');
      iniSettings.demosprefix:= ReadIniString(iniFile, 'MOD','DemosFileNamePrefix', '');

      iniSettings.RanksURL:= ReadIniString(iniFile, 'Preferences','RanksURL', '');

      weaponType:= ReadIniValue(iniFile, 'Preferences','WeaponType', 256);
      multiGameWeapon:= ReadIniBool(iniFile, 'Preferences','MultiGameWeapon', False);
      iniSettings.weaponidpatch:= (weaponType > 256) and multiGameWeapon;

      iniSettings.unitlimit:= ReadIniValue(iniFile, 'Preferences','UnitLimit', 1500);

      if iniFile.SectionExists('Colors') then
      begin
      iniSettings.docolors:= true;
      iniSettings.Colors[0]:= ReadIniValue(iniFile, 'Colors','UnitSelectionBox', -1);
      iniSettings.Colors[1]:= ReadIniValue(iniFile, 'Colors','UnitHealthBarGood', -1);
      iniSettings.Colors[2]:= ReadIniValue(iniFile, 'Colors','UnitHealthBarMedium', -1);
      iniSettings.Colors[3]:= ReadIniValue(iniFile, 'Colors','UnitHealthBarLow', -1);
      iniSettings.Colors[4]:= ReadIniValue(iniFile, 'Colors','BuildQueueBoxSelected1', -1);
      iniSettings.Colors[5]:= ReadIniValue(iniFile, 'Colors','BuildQueueBoxSelected2', -1);
      iniSettings.Colors[6]:= ReadIniValue(iniFile, 'Colors','BuildQueueBoxNonSelected1', -1);
      iniSettings.Colors[7]:= ReadIniValue(iniFile, 'Colors','BuildQueueBoxNonSelected2', -1);
      iniSettings.Colors[8]:= ReadIniValue(iniFile, 'Colors','LoadBarsTexturesReady', -1);
      iniSettings.Colors[9]:= ReadIniValue(iniFile, 'Colors','LoadBarsTexturesLoading', -1);
      iniSettings.Colors[10]:= ReadIniValue(iniFile, 'Colors','LoadBarsTerrainReady', -1);
      iniSettings.Colors[11]:= ReadIniValue(iniFile, 'Colors','LoadBarsTerrainLoading', -1);
      iniSettings.Colors[12]:= ReadIniValue(iniFile, 'Colors','LoadBarsUnitsReady', -1);
      iniSettings.Colors[13]:= ReadIniValue(iniFile, 'Colors','LoadBarsUnitsLoading', -1);
      iniSettings.Colors[14]:= ReadIniValue(iniFile, 'Colors','LoadBarsAnimationsReady', -1);
      iniSettings.Colors[15]:= ReadIniValue(iniFile, 'Colors','LoadBarsAnimationsLoading', -1);
      iniSettings.Colors[16]:= ReadIniValue(iniFile, 'Colors','LoadBars3DDataReady', -1);
      iniSettings.Colors[17]:= ReadIniValue(iniFile, 'Colors','LoadBars3DDataLoading', -1);
      iniSettings.Colors[18]:= ReadIniValue(iniFile, 'Colors','LoadBarsExplosionsReady', -1);
      iniSettings.Colors[19]:= ReadIniValue(iniFile, 'Colors','LoadBarsExplosionsLoading', -1);
      iniSettings.Colors[20]:= ReadIniValue(iniFile, 'Colors','MainMenuDots', -1);
      iniSettings.Colors[21]:= ReadIniValue(iniFile, 'Colors','NanolatheParticleBase', -1);
      iniSettings.Colors[22]:= ReadIniValue(iniFile, 'Colors','NanolatheParticleColors', -1);
      iniSettings.Colors[23]:= ReadIniValue(iniFile, 'Colors','UnderConstructSurfaceLo', -1);
      iniSettings.Colors[24]:= ReadIniValue(iniFile, 'Colors','UnderConstructSurfaceHi', -1);
      iniSettings.Colors[25]:= ReadIniValue(iniFile, 'Colors','UnderConstructOutlineLo', -1);
      iniSettings.Colors[26]:= ReadIniValue(iniFile, 'Colors','UnderConstructOutlineHi', -1);
      if ReadIniBool(iniFile, 'Colors','MainMenuDotsDisabled', False) then
        iniSettings.Colors[27]:= 1
      else
        iniSettings.Colors[27]:= -1;
      end;

    finally
      Result:= True;
      iniFile.Free;
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

    iniSettings.read:= ReadINISettings;
    
  end else
    result := nil;
end;

end.
