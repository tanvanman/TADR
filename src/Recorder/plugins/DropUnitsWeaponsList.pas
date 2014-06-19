unit DropUnitsWeaponsList;

interface
uses
  Classes, SysUtils;

type
  TScriptorFile = class
  private
    fPath   : String;
    fStream : TStringList;
  public
    constructor Create(aPath : String);
    procedure WriteDefinition(sName : String; aIndex : Integer);
    procedure WriteComment(sText : String);
    procedure WriteNewLine;

    procedure Save;
  end;

procedure SaveUnitsWeaponsListToScriptorFile;

implementation
uses
  TA_MemoryLocations,
  TA_MemoryStructures,
  IniOptions,
  TADemoConsts;

constructor TScriptorFile.Create(aPath : String);
begin
  fPath := aPath;
  fStream := TStringList.Create;
end;

procedure TScriptorFile.WriteDefinition(sName : String; aIndex : Integer);
begin
  fStream.Add('#define ' + sName + #32 + IntToStr(aIndex));
end;

procedure TScriptorFile.WriteComment(sText : String);
begin
  fStream.Add('// ' + sText);
end;

procedure TScriptorFile.WriteNewLine;
begin
  fStream.Add('');
end;

procedure TScriptorFile.Save;
begin
  fStream.SaveToFile(fPath);
end;

procedure SaveUnitsWeaponsListToScriptorFile;
var
  ScriptorFile : TScriptorFile;
  sPath : String;

  UnitsMax : Integer;
  WeapsMax : Integer;

  UnitInfo : PUnitfInfo;
  WeapInfo : PWeaponDef;


  i : Integer;
begin
  sPath := IncludeTrailingPathDelimiter(ExtractFilePath(SelfLocation)) + 'unitsweaps.h';
  ScriptorFile := TScriptorFile.Create(sPath);
  try
    ScriptorFile.WriteComment('Units and weapons list of mod ' + IniSettings.Name + #32 + IniSettings.Version);
    ScriptorFile.WriteComment('Generated at : ' + DateTimeToStr(Now) + #13#10);

    ScriptorFile.WriteComment('Units :');
    UnitsMax := TAData.UnitInfosCount; //IniSettings.UnitType;
    for i := 0 to UnitsMax - 1 do
    begin
      UnitInfo := TAMem.UnitInfoId2Ptr(i);
      if UnitInfo <> nil then
      begin
        if UnitInfo.szUnitName <> 'None' then
        begin
          ScriptorFile.WriteComment(UnitInfo.szName + ' - ' + UnitInfo.szUnitDescription);
          ScriptorFile.WriteDefinition(UnitInfo.szUnitName, i);
        end;
      end;
    end;

    ScriptorFile.WriteNewLine;
    ScriptorFile.WriteComment('Weapons :');
    WeapsMax := IniSettings.WeaponType;
    for i := 0 to WeapsMax - 1 do
    begin
      WeapInfo := TAMem.WeaponId2Ptr(i);
      if WeapInfo <> nil then
      begin
        if WeapInfo.szWeaponName[0] <> #0 then
        begin
          ScriptorFile.WriteComment(WeapInfo.szWeaponDescription);
          if IniSettings.WeaponType <= 256 then
            ScriptorFile.WriteDefinition(WeapInfo.szWeaponName, WeapInfo.ucID)
          else
            ScriptorFile.WriteDefinition(WeapInfo.szWeaponName, WeapInfo.lWeaponIDCrack);
        end;
      end;
    end;

    ScriptorFile.Save;
  finally
    ScriptorFile.Free;
  end;
end;

end.

