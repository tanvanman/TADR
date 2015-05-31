unit ExtensionsMem;

interface
uses
  PluginEngine, TA_MemoryStructures;

// -----------------------------------------------------------------------------

const
  State_ExtensionsMem: Boolean = True;

function GetPlugin: TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallExtensionsMem;
Procedure OnUninstallExtensionsMem;

// -----------------------------------------------------------------------------

procedure FreeUnitMem(p_Unit: PUnitStruct);

implementation
uses
  SysUtils,
  UnitInfoExpand,
  COB_Extensions,
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_FunctionsU,
  IniOptions;

procedure ExtensionsFreeMemory; stdcall;
var
  i: Integer;
begin
  MouseLock := False;

  //ReleaseFeature_TdfVector;
  for i := Low(UnitsCustomFields) to High(UnitsCustomFields) do
    if UnitsCustomFields[i].UnitInfo <> nil then
    begin
      MEM_Free(UnitsCustomFields[i].UnitInfo);
      UnitsCustomFields[i].UnitInfo := nil;
    end;

  if not UnitsCustomFieldsDynArr.IsVoid then
    UnitsCustomFieldsDynArr.Clear;
  if not UnitSearchResults.IsVoid then
    UnitSearchResults.Clear;
  if not SpawnedMinions.IsVoid then
    SpawnedMinions.Clear;

  if MapMissionsUnit.p_UnitScriptsData <> nil then
  begin
    UNITS_KillUnit(@MapMissionsUnit, 8);
    FreeUnitMem(@MapMissionsUnit);
  end;

  if MapMissionsSounds <> nil then
    FreeAndNil(MapMissionsSounds);
  if MapMissionsFeatures <> nil then
    FreeAndNil(MapMissionsFeatures);
  if MapMissionsUnitsInitialMissions <> nil then
    FreeAndNil(MapMissionsUnitsInitialMissions);
  if MapMissionsTextMessages <> nil then
    FreeAndNil(MapMissionsTextMessages);

  FillChar(MapMissionsUnit, SizeOf(TUnitStruct), 0);
  FillChar(NanoSpotUnitSt, SizeOf(TUnitStruct), 0);
  FillChar(NanoSpotQueueUnitSt, SizeOf(TUnitStruct), 0);
  FillChar(NanoSpotUnitInfoSt, SizeOf(TUnitInfo), 0);
  FillChar(NanoSpotQueueUnitInfoSt, SizeOf(TUnitInfo), 0);
  FillChar(UnitsSharedData, SizeOf(UnitsSharedData), 0);
end;

procedure FreeExtensionsMemory;
asm
  pushAD
  call    ExtensionsFreeMemory
  popAD
  mov     eax, [TAdynMemStructPtr]
  push    $00496B1A
  call    PatchNJump;
end;

procedure FreeExtensionsMemory2;
asm
  pushAD
  call    ExtensionsFreeMemory
  popAD
  mov     edx, [TAdynMemStructPtr]
  push    $00491BBE
  call    PatchNJump;
end;

procedure InitExtensionsArrays; stdcall;
var
  i: Word;
  UnitRec: TStoreUnitsRec;
begin
  ExtensionsFreeMemory;
  UnitsCustomFieldsDynArr.Init(TypeInfo(TUnitsCustomFields), UnitsCustomFields, @UnitsCustomFieldsCount);
  UnitsCustomFieldsDynArr.Capacity := 1 + (IniSettings.UnitLimit * MAXPLAYERCOUNT);
  UnitSearchResults.Init(TypeInfo(TUnitSearchArr), UnitSearchArr, @UnitSearchCount);
  UnitSearchResults.Capacity := High(Word);
  SpawnedMinions.Init(TypeInfo(TSpawnedMinionsArr), SpawnedMinionsArr, @SpawnedMinionsCount);
  SpawnedMinions.Capacity := High(Word);
  for i := 0 to High(Word) - 1 do
  begin
    UnitSearchResults.Add(UnitRec);
    SpawnedMinions.Add(UnitRec);
  end;
end;

procedure InitExtensionsMemory;
asm
  pushAD
  call InitExtensionsArrays
  popAD
  mov     eax, [TADynMemStructPtr]
  push $004971B8
  call PatchNJump
end;

procedure FreeUnitMem(p_Unit: PUnitStruct);
begin
  p_Unit.nKills := 0;
  FreeUnitOrders(p_Unit);
  if p_Unit.p_UnitScriptsData <> nil then
  begin
    FreeUnitScriptData(nil, nil, p_Unit.p_UnitScriptsData, 1);
    p_Unit.p_UnitScriptsData := nil;
  end;
  if p_Unit.p_Object3DO <> nil then
  begin
    FreeObjectState(p_Unit.p_Object3DO);
    p_Unit.p_Object3DO := nil;
  end;
  if p_Unit.p_MovementClass <> nil then
  begin
    FreeMoveClass(p_Unit.p_Object3DO, nil, p_Unit.p_MovementClass);
    MEM_Free(p_Unit.p_MovementClass);
    p_Unit.p_MovementClass := nil;
  end;
  p_Unit.nUnitInfoID := 0;
  FillChar(p_Unit^, SizeOf(TUnitStruct), 0);
end;

procedure LoadFonts; stdcall;
var
  Buffer: String[255];
  tmp: Pointer;
begin
  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('COMIX'), PAnsiChar('FNT'));
  tmp := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);
  if tmp = nil then
    TerminateProcess_WithWarning(PAnsiChar(PAnsiChar(@Buffer[1])));
  TAData.MainStruct.p_Font_COMIX := tmp;

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('smlfont'), PAnsiChar('FNT'));
  tmp := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);
  if tmp = nil then
    TerminateProcess_WithWarning(PAnsiChar(PAnsiChar(@Buffer[1])));
  TAData.MainStruct.p_Font_SMLFONT := tmp;
{
  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('Armbrief'), PAnsiChar('FNT'));
  Fonts.p_ArmBrief := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('ARMBUTT'), PAnsiChar('FNT'));
  Fonts.p_ArmBut := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('ARMCONTR'), PAnsiChar('FNT'));
  Fonts.p_ArmContr := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('ARMFONT'), PAnsiChar('FNT'));
  Fonts.p_ArmFont := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('armfont827'), PAnsiChar('FNT'));
  Fonts.p_ArmFont827 := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('BUTTONS'), PAnsiChar('FNT'));
  Fonts.p_Buttons := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('CONSOLE'), PAnsiChar('FNT'));
  Fonts.p_Console := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('CORBUTT'), PAnsiChar('FNT'));
  Fonts.p_CorButt := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('CORBUTTX'), PAnsiChar('FNT'));
  Fonts.p_CorButtX := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('CORCONTR'), PAnsiChar('FNT'));
  Fonts.p_CorContr := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('COREFONT'), PAnsiChar('FNT'));
  Fonts.p_CoreFont := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('Corefont827'), PAnsiChar('FNT'));
  Fonts.p_CoreFont827 := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('COURIER'), PAnsiChar('FNT'));
  Fonts.p_Courier := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('HATT12'), PAnsiChar('FNT'));
  Fonts.p_Hatt12 := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('HATT14'), PAnsiChar('FNT'));
  Fonts.p_Hatt14 := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('MSCRIPT'), PAnsiChar('FNT'));
  Fonts.p_MScript := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('ROMAN10'), PAnsiChar('FNT'));
  Fonts.p_Roman10 := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);

  GetLocalizedFilePath(@Buffer[1], PAnsiChar('fonts'), PAnsiChar('ROMAN12'), PAnsiChar('FNT'));
  Fonts.p_Roman12 := HAPIFILE_ReadFile(PAnsiChar(@Buffer[1]), 0);
}
end;

Procedure OnInstallExtensionsMem;
begin
end;

Procedure OnUninstallExtensionsMem;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_ExtensionsMem then
  begin
    Result := TPluginData.Create( True,
                                  '',
                                  State_ExtensionsMem,
                                  @OnInstallExtensionsMem,
                                  @OnUnInstallExtensionsMem );

    Result.MakeRelativeJmp( State_ExtensionsMem,
                            'Init extensions search arrays, units custom fields etc.',
                            @InitExtensionsMemory,
                            $004971B3, 0 );

    Result.MakeRelativeJmp( State_ExtensionsMem,
                            '',
                            @FreeExtensionsMemory,
                            $00496B15, 0 );

    Result.MakeRelativeJmp( State_ExtensionsMem,
                            '',
                            @FreeExtensionsMemory2,
                            $00491BB8, 0 );

    Result.MakeStaticCall( State_ExtensionsMem,
                           'Load more fonts',
                           @LoadFonts,
                           $00491373 );

  end else
    Result := nil;
end;

end.
