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

procedure FreeUnitMem(UnitPtr: PUnitStruct);

implementation
uses
  UnitInfoExpand,
  COB_Extensions,
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_FunctionsU;

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
  end else
    Result := nil;
end;

procedure InitExtensionsArrays; stdcall;
var
  i: LongWord;
  UnitRec: TStoreUnitsRec;
begin
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
begin
  MouseLock := False;

  //ReleaseFeature_TdfVector;
  if not UnitSearchResults.IsVoid then
    UnitSearchResults.Clear;
  if not SpawnedMinions.IsVoid then
    SpawnedMinions.Clear;

  if MapMissionsUnit.lUnitInGameIndex <> 0 then
  begin
    UNITS_KillUnit(@MapMissionsUnit, 8);
    FreeUnitMem(@MapMissionsUnit);
  end;

  if Assigned(MapMissionsSounds) then
    MapMissionsSounds.Free;
  if Assigned(MapMissionsFeatures) then
    MapMissionsFeatures.Free;
  if Assigned(MapMissionsUnitsInitialMissions) then
    MapMissionsUnitsInitialMissions.Free;

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

procedure FreeUnitMem(UnitPtr: PUnitStruct);
var
  CobScript : Pointer;
begin
  PUnitStruct(UnitPtr).nKills := 0;
  FreeUnitOrders(UnitPtr);
  CobScript := UnitPtr.p_UnitScriptsData;
  if CobScript <> nil then
  begin
    FreeUnitScriptData(nil, nil, UnitPtr.p_UnitScriptsData, 1);
    UnitPtr.p_UnitScriptsData := nil;
  end;
  if UnitPtr.p_Object3DO <> nil then
  begin
    FreeObjectState(UnitPtr.p_Object3DO);
    UnitPtr.p_Object3DO := nil;
  end;
  if UnitPtr.p_MovementClass <> nil then
  begin
    FreeMoveClass(UnitPtr.p_Object3DO, nil, UnitPtr.p_MovementClass);
    MEM_Free(UnitPtr.p_MovementClass);
    UnitPtr.p_MovementClass := nil;
  end;
  UnitPtr.nUnitInfoID := 0;
  FillChar(UnitPtr^, SizeOf(TUnitStruct), 0);
end;

end.
