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

const
  UnitInfo_MultiAirTransport : AnsiString = 'multiairtransport';
  UnitInfo_TransportWeightCapacity : AnsiString = 'transportweightcapacity';
  UnitInfo_ExtraVTOLOrders : AnsiString = 'extravtolorders';
  UnitInfo_HideHPBar : AnsiString = 'hidehpbar';
  UnitInfo_NotLab : AnsiString = 'notlab';
  UnitInfo_DrawBuildSpotNanoFrame : AnsiString = 'drawbuildspotnano';
  UnitInfo_AISquadNr : AnsiString = 'aisquadnr';
  UnitInfo_TeleportMethod : AnsiString = 'teleportmethod';
  UnitInfo_TeleportMinReloadTime : AnsiString = 'teleportminreloadtime';
  UnitInfo_TeleportMaxDistance : AnsiString = 'teleportmaxdist';
  UnitInfo_TeleportMinDistance : AnsiString = 'teleportmindist';
  UnitInfo_CustomRange1Distance : AnsiString = 'customrange1dist';
  UnitInfo_CustomRange1Color : AnsiString = 'customrange1color';
  UnitInfo_CustomRange2Distance : AnsiString = 'customrange2dist';
  UnitInfo_CustomRange2Color : AnsiString = 'customrange2color';
  UnitInfo_CustomRange2Animate : AnsiString = 'customrange2anim';
  UnitInfo_SolarGenerator : AnsiString = 'solargenerator';

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

procedure UnitInfoPropertyToArray(PropertyType: Integer; UnitTypeID: Cardinal; AValue: Integer); stdcall;
begin
  case PropertyType of
    1 : ExtraUnitInfoTags[UnitTypeID].MultiAirTransport := AValue;
    2 : ExtraUnitInfoTags[UnitTypeID].ExtraVTOLOrders := AValue;
    3 : ExtraUnitInfoTags[UnitTypeID].TransportWeightCapacity := AValue;
    4 : ExtraUnitInfoTags[UnitTypeID].HideHPBar := (AValue = 1);
    5 : ExtraUnitInfoTags[UnitTypeID].NotLab := (AValue = 1);
    6 : ExtraUnitInfoTags[UnitTypeID].DrawBuildSpotNanoFrame := (AValue = 1);
    7 : ExtraUnitInfoTags[UnitTypeID].AiSquadNr := AValue;
    8 : ExtraUnitInfoTags[UnitTypeID].TeleportMethod := AValue;
    9 : ExtraUnitInfoTags[UnitTypeID].TeleportMinReloadTime := AValue;
   10 : ExtraUnitInfoTags[UnitTypeID].TeleportMaxDistance := AValue;
   11 : ExtraUnitInfoTags[UnitTypeID].TeleportMinDistance := AValue;
   12 : ExtraUnitInfoTags[UnitTypeID].CustomRange1Distance := AValue;
   13 : ExtraUnitInfoTags[UnitTypeID].CustomRange1Color := AValue;
   14 : ExtraUnitInfoTags[UnitTypeID].CustomRange2Distance := AValue;
   15 : ExtraUnitInfoTags[UnitTypeID].CustomRange2Color := AValue;
   16 : ExtraUnitInfoTags[UnitTypeID].CustomRange2Animate := (AValue = 1);
   17 : ExtraUnitInfoTags[UnitTypeID].SolarGenerator := AValue / 100;
  end;
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
       17 : Result := Trunc(ExtraUnitInfoTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].SolarGenerator);
      end;
    end;
  end;
end;

{
.text:0042C3FC 530 E8 BF 82 09 00      call    ?GetInt@TdfFile@@QADHPADH@Z ; TdfFile::GetInt(char *,int)
.text:0042C401 528 56                  push    esi             ; DefaultNumber
.text:0042C402 52C 68 94 3C 50 00      push    offset aBmcode  ; "bmcode"
.text:0042C407 530 8B 4C 24 1C         mov     ecx, [esp+530h+TdfFileClass_.ParsedTdf_]
}
procedure UnitInfoExpand_NewPropertiesLoadHook;
asm
    pushAD
    movzx   ebx, word [ebp+TUnitInfo.nCategory]
    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_MultiAirTransport
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    1
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_ExtraVTOLOrders
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    2
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_TransportWeightCapacity
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    3
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_HideHPBar
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    4
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_NotLab
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    5
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_DrawBuildSpotNanoFrame
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    6
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_AISquadNr
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    7
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_TeleportMethod
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    8
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_TeleportMinReloadTime
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    9
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_TeleportMaxDistance
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    10
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_TeleportMinDistance
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    11
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_CustomRange1Distance
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    12
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_CustomRange1Color
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    13
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_CustomRange2Distance
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    14
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_CustomRange2Color
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    15
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_CustomRange2Animate
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    16
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    push    ecx
    push    edx
    push    eax
    push    0
    push    UnitInfo_SolarGenerator
    call    TdfFile__GetInt
    push    eax
    push    ebx
    push    17
    call    UnitInfoPropertyToArray
    pop     esi
    pop     edx
    pop     ecx

    popAD
    push    esi
    push    503C94h
    push $0042C407;
    call PatchNJump;
end;

end.
