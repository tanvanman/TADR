unit ExtensionsMem;

interface
uses
  PluginEngine, TA_MemoryStructures;

// -----------------------------------------------------------------------------

const
  State_ExtensionsMem : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallExtensionsMem;
Procedure OnUninstallExtensionsMem;

// -----------------------------------------------------------------------------

procedure InitExtensionsMemory;
procedure FreeExtensionsMemory;

procedure InitExtensionsArrays; stdcall;
procedure ExtensionsFreeMemory; stdcall;
procedure FreeExtensionsMemory2;
procedure FreeUnitMem(p_Unit: PUnitStruct);

implementation
uses
  UnitInfoExpand,
  COB_Extensions,
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_FunctionsU,
  IniOptions, SysUtils;

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
    Result := TPluginData.Create( False,
                                  '',
                                  State_ExtensionsMem,
                                  @OnInstallExtensionsMem,
                                  @OnUnInstallExtensionsMem );

    Result.MakeRelativeJmp( State_ExtensionsMem,
                            'Init extensions search arrays etc.',
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

  end else
    Result := nil;
end;

procedure InitExtensionsArrays; stdcall;
var
  i: LongWord;
  UnitRec: TStoreUnitsRec;
begin
  ExtensionsFreeMemory;

  
  CustomUnitFields.Init(TypeInfo(TCustomUnitFieldsArr), CustomUnitFieldsArr, @CustomUnitFieldsCount);
  CustomUnitFields.Capacity := IniSettings.UnitLimit * MAXPLAYERCOUNT;

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

procedure ExtensionsFreeMemory; stdcall;
var
  i : Integer;
begin
  MouseLock := False;

  //ReleaseFeature_TdfVector;
  for i := Low(CustomUnitFieldsArr) to High(CustomUnitFieldsArr) do
    if CustomUnitFieldsArr[i].UnitInfo <> nil then
    begin
      MEM_Free(CustomUnitFieldsArr[i].UnitInfo);
      CustomUnitFieldsArr[i].UnitInfo := nil;
    end;

  if not CustomUnitFields.IsVoid then
    CustomUnitFields.Clear;
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

end.