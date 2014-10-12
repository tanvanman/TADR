unit UnitInfoExpand;

interface
uses
  PluginEngine, SynCommons, TA_MemoryStructures;

// -----------------------------------------------------------------------------

const
  State_UnitInfoExpand : boolean = true;

var
  CustomUnitInfosArray : TUnitInfos;
  CustomUnitInfos,
  CustomUnitFields : TDynArray;
  CustomUnitInfosCount,
  CustomUnitFieldsCount : Integer;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallUnitInfoExpand;
Procedure OnUninstallUnitInfoExpand;

// -----------------------------------------------------------------------------

procedure UnitInfoExpand_NewPropertiesLoadHook;
function GetUnitInfoProperty(UnitPtr: Pointer; PropertyType: Integer): Integer; stdcall;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_FunctionsU,
  TA_MemoryLocations;

var
  UnitInfoExpandPlugin: TPluginData;

Procedure OnInstallUnitInfoExpand;
begin
  SetLength(ExtraUnitInfoTags, IniSettings.UnitType);

  CustomUnitInfos.Init(TypeInfo(TUnitInfos),CustomUnitInfosArray, @CustomUnitInfosCount);
  CustomUnitInfos.Capacity := IniSettings.UnitLimit * MAXPLAYERCOUNT;

  CustomUnitFields.Init(TypeInfo(TCustomUnitFieldsArr), CustomUnitFieldsArr, @CustomUnitFieldsCount);
  CustomUnitFields.Capacity := IniSettings.UnitLimit * MAXPLAYERCOUNT;
end;

Procedure OnUninstallUnitInfoExpand;
begin
  if not CustomUnitInfos.IsVoid then
    CustomUnitInfos.Clear;
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

    Result:= UnitInfoExpandPlugin;
  end else
    Result := nil;
end;

function GetUnitInfoProperty(UnitPtr: Pointer; PropertyType: Integer): Integer; stdcall;
begin
  Result := 0;
  if UnitPtr <> nil then
  begin
    if High(ExtraUnitInfoTags) >= PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory then
    begin
      case PropertyType of
        1 : Result := ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].MultiAirTransport;
        2 : Result := ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].ExtraVTOLOrders;
        3 : Result := ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].TransportWeightCapacity;
        4 : Result := BoolValues[ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].HideHPBar];
        5 : Result := BoolValues[ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].NotLab];
        6 : Result := BoolValues[ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].DrawBuildSpotNanoFrame];
        7 : Result := ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].AiSquadNr;
        8 : Result := ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].TeleportMethod;
        9 : Result := ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].TeleportMinReloadTime;
       10 : Result := ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].TeleportMaxDistance;
       11 : Result := ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].TeleportMinDistance;
       12 : Result := ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].CustomRange1Distance;
       13 : Result := ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].CustomRange1Color;
       14 : Result := ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].CustomRange2Distance;
       15 : Result := ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].CustomRange2Color;
       16 : Result := BoolValues[ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].CustomRange2Animate];
       17 : Result := Trunc(ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].SolarGenerator * 100);
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
  ExtraUnitInfoTags[UnitInfoID].TeleportMethod := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportmethod'));
  ExtraUnitInfoTags[UnitInfoID].TeleportMinReloadTime := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportminreloadtime'));
  ExtraUnitInfoTags[UnitInfoID].TeleportMaxDistance := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportmaxdist'));
  ExtraUnitInfoTags[UnitInfoID].TeleportMinDistance := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('teleportmindist'));
  ExtraUnitInfoTags[UnitInfoID].CustomRange1Distance := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange1dist'));
  ExtraUnitInfoTags[UnitInfoID].CustomRange1Color := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange1color'));
  ExtraUnitInfoTags[UnitInfoID].CustomRange2Distance := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange2dist'));
  ExtraUnitInfoTags[UnitInfoID].CustomRange2Color := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange2color'));
  ExtraUnitInfoTags[UnitInfoID].CustomRange2Animate := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('customrange2anim')) <> 0;
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

end.
