unit IniOptions;

interface
uses PluginEngine, Windows, SysUtils, IniFiles;

type
  TIniSettings = record
    ModId          : Integer;
    DemosPrefix    : String;
    Name           : String;
    RegName        : String;
    Version        : String;
    WeaponIdPatch  : Boolean;
    RanksURL       : String;
    UnitLimit      : Integer;
    Colors         : array [0..27] of Integer;
    Read           : Boolean;
    DoColors       : Boolean;
    MobileToMobile : Boolean;
  end;
var IniSettings: TIniSettings;

type
  TIniColors = ( UNITSELECTIONBOX = 0,
  UNITHEALTHBARGOOD,
  UNITHEALTHBARMEDIUM,
  UNITHEALTHBARLOW,
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
  i: integer;
  currcolor: string;
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
      iniSettings.MobileToMobile:= ReadIniBool(iniFile, 'Preferences','MobileToMobile', False);

      if iniFile.SectionExists('Colors') then
      begin
        iniSettings.docolors:= true;
        for i:= 0 to 26 do
        begin
          currcolor:= GetEnumName(TypeInfo(TIniColors), i);
          iniSettings.Colors[i] := ReadIniValue(iniFile, 'Colors', currcolor , -1);
        end;
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
