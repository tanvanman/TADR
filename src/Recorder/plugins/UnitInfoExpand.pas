unit UnitInfoExpand;

interface
uses
  PluginEngine, SynCommons, TA_MemoryStructures;

// -----------------------------------------------------------------------------

const
  State_UnitInfoExpand : boolean = true;

var
  CustomUnitFields : TDynArray;
  CustomUnitFieldsCount : Integer;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallUnitInfoExpand;
Procedure OnUninstallUnitInfoExpand;

// -----------------------------------------------------------------------------

procedure UnitInfoExpand_NewPropertiesLoadHook;
procedure FreeCustomUnitInfoHook;
procedure FreeCustomUnitInfo(p_Unit: PUnitStruct); stdcall;
function GetUnitInfoProperty(p_Unit: PUnitStruct; PropertyType: Integer): Integer; stdcall;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_FunctionsU,
  TA_MemoryLocations,
  TA_MemUnits;

var
  UnitInfoExpandPlugin: TPluginData;

Procedure OnInstallUnitInfoExpand;
begin
  SetLength(ExtraUnitInfoTags, IniSettings.UnitType);
end;

Procedure OnUninstallUnitInfoExpand;
begin
  if not CustomUnitFields.IsVoid then
    CustomUnitFields.Clear;
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_UnitInfoExpand then
  begin
    UnitInfoExpandPlugin := TPluginData.create( false,
                            'Load new unit definition tags',
                            State_UnitInfoExpand,
                            @OnInstallUnitInfoExpand,
                            @OnUnInstallUnitInfoExpand );

    UnitInfoExpandPlugin.MakeRelativeJmp( State_UnitInfoExpand,
                          'Load new unit definition tags hook',
                          @UnitInfoExpand_NewPropertiesLoadHook,
                          $0042C401, 1);
                            
    UnitInfoExpandPlugin.MakeRelativeJmp( State_UnitInfoExpand,
                          'Free custom unitinfo at unit create',
                          @FreeCustomUnitInfoHook,
                          $00485A75, 1);

    Result:= UnitInfoExpandPlugin;
  end else
    Result := nil;
end;

function GetUnitInfoProperty(p_Unit: PUnitStruct; PropertyType: Integer): Integer; stdcall;
begin
  Result := 0;
  if p_Unit <> nil then
  begin
    if High(ExtraUnitInfoTags) >= p_Unit.p_UNITINFO.nCategory then
    begin
      case PropertyType of
        1 : Result := ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].MultiAirTransport;
        2 : Result := ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].ExtraVTOLOrders;
        3 : Result := ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].TransportWeightCapacity;
        4 : Result := BoolValues[ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].HideHPBar];
        5 : Result := BoolValues[ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].NotLab];
        6 : Result := BoolValues[ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].DrawBuildSpotNanoFrame];
        7 : Result := ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].AiSquadNr;
        8 : Result := Ord(ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].TeleportMethod);
        9 : Result := ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].TeleportMinReloadTime;
       10 : Result := ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].TeleportMaxDistance;
       11 : Result := ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].TeleportMinDistance;
       12 : Result := ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].CustomRange1Distance;
       13 : Result := ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].CustomRange1Color;
       14 : Result := ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].CustomRange2Distance;
       15 : Result := ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].CustomRange2Color;
       16 : Result := BoolValues[ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].CustomRange2Animate];
       17 : Result := Trunc(ExtraUnitInfoTags[p_Unit.p_UNITINFO.nCategory].SolarGenerator * 100);
      end;
    end;
  end;
end;

procedure UnitInfoExpand_NewPropertiesLoad(TDFHandle: Cardinal; UnitInfoID: Word); stdcall;
begin
  ExtraUnitInfoTags[UnitInfoID].MultiAirTransport := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('multiairtransport'));
  ExtraUnitInfoTags[UnitInfoID].SolarGenerator := TdfFile_GetFloat(0, 0, TDFHandle, 0.0, PAnsiChar('solargenerator'));
  ExtraUnitInfoTags[UnitInfoID].TransportWeightCapacity := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('transportweightcapacity'));
  ExtraUnitInfoTags[UnitInfoID].ExtraVTOLOrders := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('extravtolorders'));
  ExtraUnitInfoTags[UnitInfoID].HideHPBar := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('hidehpbar')) <> 0;
  ExtraUnitInfoTags[UnitInfoID].NotLab := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('notlab')) <> 0;
  ExtraUnitInfoTags[UnitInfoID].DrawBuildSpotNanoFrame := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('drawbuildspotnano')) <> 0;
  ExtraUnitInfoTags[UnitInfoID].AISquadNr := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('aisquadnr'));
  ExtraUnitInfoTags[UnitInfoID].TeleportMethod := TTeleportMethod(TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportmethod')));
  ExtraUnitInfoTags[UnitInfoID].TeleportMinReloadTime := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportminreloadtime'));
  ExtraUnitInfoTags[UnitInfoID].TeleportMaxDistance := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportmaxdist'));
  ExtraUnitInfoTags[UnitInfoID].TeleportMinDistance := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportmindist'));
  ExtraUnitInfoTags[UnitInfoID].TeleportCost := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportcost'));
  ExtraUnitInfoTags[UnitInfoID].TeleportFilter := TAUnits.CreateSearchFilter(TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportfilter')));
  ExtraUnitInfoTags[UnitInfoID].TeleportToLoSOnly := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleporttolosonly')) <> 0;
  ExtraUnitInfoTags[UnitInfoID].CustomRange1Distance := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange1dist'));
  ExtraUnitInfoTags[UnitInfoID].CustomRange1Color := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange1color'));
  ExtraUnitInfoTags[UnitInfoID].CustomRange2Distance := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange2dist'));
  ExtraUnitInfoTags[UnitInfoID].CustomRange2Color := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange2color'));
  ExtraUnitInfoTags[UnitInfoID].CustomRange2Animate := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange2anim')) <> 0;
  ExtraUnitInfoTags[UnitInfoID].UseCustomReloadBar := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customreloadbar')) <> 0;
  ExtraUnitInfoTags[UnitInfoID].DefaultMissionOrgPos := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('defaultmissionorgpos')) <> 0;
end;

procedure UnitInfoExpand_NewPropertiesLoadHook;
asm
    pushAD
    movzx   ebx, word [ebp+TUnitInfo.nCategory]
    push    ebx
    push    ecx
    call    UnitInfoExpand_NewPropertiesLoad
    popAD

    push    esi
    push    503C94h
    push $0042C407;
    call PatchNJump;
end;

procedure FreeCustomUnitInfo(p_Unit: PUnitStruct); stdcall;
begin
  if CustomUnitFieldsArr[Word(p_Unit.lUnitInGameIndex)].UnitInfo <> nil then
  begin
    MEM_Free(CustomUnitFieldsArr[Word(p_Unit.lUnitInGameIndex)].UnitInfo);
    CustomUnitFieldsArr[Word(p_Unit.lUnitInGameIndex)].UnitInfo := nil;
  end;
  CustomUnitFields.ElemClear(CustomUnitFieldsArr[Word(p_Unit.lUnitInGameIndex)]);
end;

procedure FreeCustomUnitInfoHook;
asm
  mov     [esi+92h], edx
  pushAD
  push    esi
  call    FreeCustomUnitInfo
  popAD
  push $00485A7B;
  call PatchNJump;
end;

end.