unit UnitInfoExpand;

interface
uses
  PluginEngine, SynCommons, TA_MemoryStructures;

// -----------------------------------------------------------------------------

const
  State_UnitInfoExpand: Boolean = True;

var
  UnitsCustomFieldsDynArr: TDynArray;
  UnitsCustomFieldsCount: Integer;

function GetPlugin: TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallUnitInfoExpand;
Procedure OnUninstallUnitInfoExpand;

// -----------------------------------------------------------------------------

procedure FreeCustomUnitInfo(p_Unit: PUnitStruct); stdcall;
function GetUnitInfoProperty(p_Unit: PUnitStruct; PropertyType: Integer): Integer; stdcall;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_FunctionsU,
  TA_MemoryLocations,
  TA_MemUnits;

function GetUnitInfoProperty(p_Unit: PUnitStruct; PropertyType: Integer): Integer; stdcall;
begin
  Result := 0;
  if p_Unit <> nil then
  begin
    if High(UnitInfoCustomFields) >= p_Unit.p_UNITINFO.nCategory then
    begin
      case PropertyType of
        1 : Result := UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].MultiAirTransport;
        2 : Result := UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].ExtraVTOLOrders;
        3 : Result := UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].TransportWeightCapacity;
        4 : Result := BoolValues[UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].HideHPBar];
        5 : Result := BoolValues[UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].NotLab];
        6 : Result := BoolValues[UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].DrawBuildSpotNanoFrame];
        7 : Result := UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].AiSquadNr;
        8 : Result := Ord(UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].TeleportMethod);
        9 : Result := UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].TeleportMinReloadTime;
       10 : Result := UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].TeleportMaxDistance;
       11 : Result := UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].TeleportMinDistance;
       12 : Result := UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].CustomRange1Distance;
       13 : Result := UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].CustomRange1Color;
       14 : Result := UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].CustomRange2Distance;
       15 : Result := UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].CustomRange2Color;
       16 : Result := BoolValues[UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].CustomRange2Animate];
       17 : Result := Trunc(UnitInfoCustomFields[p_Unit.p_UNITINFO.nCategory].SolarGenerator * 100);
      end;
    end;
  end;
end;

procedure UnitInfoExpand_NewPropertiesLoad(TDFHandle: Cardinal; UnitInfoID: Word); stdcall;
begin
  UnitInfoCustomFields[UnitInfoID].MultiAirTransport := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('multiairtransport'));
  UnitInfoCustomFields[UnitInfoID].SolarGenerator := TdfFile_GetFloat(0, 0, TDFHandle, 0.0, PAnsiChar('solargenerator'));
  UnitInfoCustomFields[UnitInfoID].TransportWeightCapacity := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('transportweightcapacity'));
  UnitInfoCustomFields[UnitInfoID].ExtraVTOLOrders := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('extravtolorders'));
  UnitInfoCustomFields[UnitInfoID].HideHPBar := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('hidehpbar')) <> 0;
  UnitInfoCustomFields[UnitInfoID].NotLab := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('notlab')) <> 0;
  UnitInfoCustomFields[UnitInfoID].DrawBuildSpotNanoFrame := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('drawbuildspotnano')) <> 0;
  UnitInfoCustomFields[UnitInfoID].AISquadNr := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('aisquadnr'));
  UnitInfoCustomFields[UnitInfoID].TeleportMethod := TTeleportMethod(TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportmethod')));
  UnitInfoCustomFields[UnitInfoID].TeleportMinReloadTime := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportminreloadtime'));
  UnitInfoCustomFields[UnitInfoID].TeleportMaxDistance := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportmaxdist'));
  UnitInfoCustomFields[UnitInfoID].TeleportMinDistance := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportmindist'));
  UnitInfoCustomFields[UnitInfoID].TeleportCost := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportcost'));
  UnitInfoCustomFields[UnitInfoID].TeleportFilter := TAUnits.CreateSearchFilter(TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportfilter')));
  UnitInfoCustomFields[UnitInfoID].TeleportToLoSOnly := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleporttolosonly')) <> 0;
  UnitInfoCustomFields[UnitInfoID].CustomRange1Distance := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange1dist'));
  UnitInfoCustomFields[UnitInfoID].CustomRange1Color := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange1color'));
  UnitInfoCustomFields[UnitInfoID].CustomRange2Distance := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange2dist'));
  UnitInfoCustomFields[UnitInfoID].CustomRange2Color := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange2color'));
  UnitInfoCustomFields[UnitInfoID].CustomRange2Animate := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange2anim')) <> 0;
  UnitInfoCustomFields[UnitInfoID].UseCustomReloadBar := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customreloadbar')) <> 0;
  UnitInfoCustomFields[UnitInfoID].DefaultMissionOrgPos := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('defaultmissionorgpos')) <> 0;
  UnitInfoCustomFields[UnitInfoID].ShieldRange := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('ShieldRange'));
  UnitInfoCustomFields[UnitInfoID].SelectBoxType := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('SelectBoxType'));
  UnitInfoCustomFields[UnitInfoID].SelectAnimation := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('SelectAnimation'));
  UnitInfoCustomFields[UnitInfoID].SelectAnimationAlpha := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('SelectAnimation')) <> 0;
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
var
  UnitID: Word;
begin
  if High(UnitsCustomFields) > 0 then
  begin
    UnitID := TAUnit.GetId(p_Unit);
    if UnitsCustomFields[UnitID].UnitInfo <> nil then
    begin
      MEM_Free(UnitsCustomFields[UnitID].UnitInfo);
      UnitsCustomFields[UnitID].UnitInfo := nil;
    end;
    UnitsCustomFieldsDynArr.ElemClear(UnitsCustomFields[UnitID]);
    UnitsCustomFields[UnitID].ShieldRange := UnitInfoCustomFields[p_Unit.nUnitInfoID].ShieldRange;
  end;
end;

procedure FreeCustomUnitInfoHook;
asm
  mov     [esi+TUnitStruct.p_UnitInfo], edx
  pushAD
  push    esi
  call    FreeCustomUnitInfo
  popAD
  push $00485A7B;
  call PatchNJump;
end;

Procedure OnInstallUnitInfoExpand;
begin
  SetLength(UnitInfoCustomFields, IniSettings.UnitType);
end;

Procedure OnUninstallUnitInfoExpand;
begin
  if not UnitsCustomFieldsDynArr.IsVoid then
    UnitsCustomFieldsDynArr.Clear;
end;

function GetPlugin: TPluginData;
begin
  if IsTAVersion31 and State_UnitInfoExpand then
  begin
    Result := TPluginData.Create( False,
                                  'Load more data from UNITINFO',
                                  State_UnitInfoExpand,
                                  @OnInstallUnitInfoExpand,
                                  @OnUnInstallUnitInfoExpand );

    Result.MakeRelativeJmp( State_UnitInfoExpand,
                            'Load new unit definition tags hook',
                            @UnitInfoExpand_NewPropertiesLoadHook,
                            $0042C401, 1 );

    Result.MakeRelativeJmp( State_UnitInfoExpand,
                            'Free custom unitinfo at unit create',
                            @FreeCustomUnitInfoHook,
                            $00485A75, 1 );
  end else
    Result := nil;
end;

end.
