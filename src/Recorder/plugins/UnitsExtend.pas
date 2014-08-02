unit UnitsExtend;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_UnitsExtend : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallUnitsExtend;
Procedure OnUninstallUnitsExtend;

// -----------------------------------------------------------------------------

procedure UnitsExtend_NewPropertiesLoadHook;
function GetUnitExtProperty(UnitPtr: Pointer; PropertyType: Integer): Integer; stdcall;

implementation
uses
  IniOptions,
  TA_MemoryStructures,
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

var
  UnitsExtendPlugin: TPluginData;

Procedure OnInstallUnitsExtend;
begin
  SetLength(ExtraUnitDefTags, IniSettings.UnitType);
end;

Procedure OnUninstallUnitsExtend;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_UnitsExtend then
  begin
    UnitsExtendPlugin := TPluginData.create( false,
                            'Load new unit definition tags',
                            State_UnitsExtend,
                            @OnInstallUnitsExtend,
                            @OnUnInstallUnitsExtend );

    UnitsExtendPlugin.MakeRelativeJmp( State_UnitsExtend,
                          'Load new unit definition tags hook',
                          @UnitsExtend_NewPropertiesLoadHook,
                          $0042C401, 1);

    Result:= UnitsExtendPlugin;
  end else
    Result := nil;
end;

procedure UnitPropertyPutIntoArray(PropertyType: Integer; UnitTypeID: Cardinal; AValue: Integer); stdcall;
begin
  case PropertyType of
    1 : ExtraUnitDefTags[UnitTypeID].MultiAirTransport := AValue;
    2 : ExtraUnitDefTags[UnitTypeID].ExtraVTOLOrders := AValue;
    3 : ExtraUnitDefTags[UnitTypeID].TransportWeightCapacity := AValue;
    4 : ExtraUnitDefTags[UnitTypeID].HideHPBar := (AValue = 1);
    5 : ExtraUnitDefTags[UnitTypeID].NotLab := (AValue = 1);
    6 : ExtraUnitDefTags[UnitTypeID].DrawBuildSpotNanoFrame := (AValue = 1);
    7 : ExtraUnitDefTags[UnitTypeID].AiSquadNr := AValue;
  end;
end;

function GetUnitExtProperty(UnitPtr: Pointer; PropertyType: Integer): Integer; stdcall;
begin
  Result := 0;
  if UnitPtr <> nil then
  begin
    if High(ExtraUnitDefTags) >= PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory then
    begin
      case PropertyType of
        1 : Result := ExtraUnitDefTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].MultiAirTransport;
        2 : Result := ExtraUnitDefTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].ExtraVTOLOrders;
        3 : Result := ExtraUnitDefTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].TransportWeightCapacity;
        4 : Result := BoolValues[ExtraUnitDefTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].HideHPBar];
        5 : Result := BoolValues[ExtraUnitDefTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].NotLab];
        6 : Result := BoolValues[ExtraUnitDefTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].DrawBuildSpotNanoFrame];
        7 : Result := ExtraUnitDefTags[PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).nCategory].AiSquadNr;
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
procedure UnitsExtend_NewPropertiesLoadHook;
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
    call    UnitPropertyPutIntoArray
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
    call    UnitPropertyPutIntoArray
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
    call    UnitPropertyPutIntoArray
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
    call    UnitPropertyPutIntoArray
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
    call    UnitPropertyPutIntoArray
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
    call    UnitPropertyPutIntoArray
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
    call    UnitPropertyPutIntoArray
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

