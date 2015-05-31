unit Transporters;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_Transporters: Boolean = True;

function GetPlugin: TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallTransporters;
Procedure OnUninstallTransporters;

// -----------------------------------------------------------------------------

implementation
uses
  UnitInfoExpand,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_MemUnits;

function OverloadBugFix(p_Unit: PUnitStruct): LongBool; stdcall;
begin
  Result := False;
  if (TAUnit.GetLoadCurAmount(p_Unit) + 1) <= p_Unit.p_UnitInfo.cTransportCap then
    Result := True;
end;

procedure Transporters_TransportOverloadFix;
label
  RefuseLoad;
asm
  test    eax, eax
  jz      RefuseLoad
  push    edx
  push    eax
  push    ecx
  push    ecx
  call OverloadBugFix
  test    eax, eax
  pop     ecx
  pop     eax
  pop     edx
  jz      RefuseLoad
  push $00406791;
  call PatchNJump;
RefuseLoad :
  push $004068DC;
  call PatchNJump;
end;

procedure Transporters_VTOLTransportCapacityCanLoadTest;
label
  TestIsMultiTransp,
  StandardTransporter,
  RefuseLoad,
  AllowLoad;
asm
TestIsMultiTransp :
  // let's test is transporter capable to load multiple units
  push    edx
  push    eax
  push    ecx
  push    1
  push    esi
  call    GetUnitInfoProperty
  test    eax, eax
  jz      StandardTransporter
  pop     ecx
  pop     eax
  pop     edx
  jmp     AllowLoad
StandardTransporter :
  // not multi transporter so
  // we check has it got unit so we refuse loading another one
  pop     ecx
  pop     eax
  pop     edx
  test    ecx, ecx
  jz      AllowLoad
RefuseLoad :
  push $0041152B;
  call PatchNJump;
AllowLoad :
  // esi transporter ptr
  // eax transported ptr
  push    edx
  push    eax
  push    ecx
  push    esi
  call OverloadBugFix
  test    eax, eax
  pop     ecx
  pop     eax
  pop     edx
  jz      RefuseLoad
  push $0041122C;
  call PatchNJump;
end;

function CompareVTOLUnitWeight(Transporter, ToBeTransported: PUnitStruct): LongBool; stdcall;
var
  CurLoadWeight: Integer;
  TobeTransportedLoadWeight: Integer;
begin
  CurLoadWeight := TAUnit.GetLoadWeight(Transporter);
  TobeTransportedLoadWeight := Round(ToBeTransported.p_UNITINFO.lBuildCostMetal);
  Result := (TobeTransportedLoadWeight + CurLoadWeight) <= UnitInfoCustomFields[Transporter.p_UNITINFO.nCategory].TransportWeightCapacity;
end;

procedure Transporters_VTOLTransportSize;
label
  FootSizeCompare,
  RefuseLoad,
  ContinueTransportedTest,
  AllowLoadUnit,
  RefuseLoadReasonWeight;
asm
FootSizeCompare :
  movzx   cx, [ecx+TUnitInfo.cTransportSize]
  cmp     word ptr [eax+TUnitStruct.nFootPrintX], cx
  jle     ContinueTransportedTest
RefuseLoad :
  push $0041126D;
  call PatchNJump;
ContinueTransportedTest :
  pushAD
  mov     edi, eax
  push    3
  push    esi
  call    GetUnitInfoProperty
  test    eax, eax
  jz AllowLoadUnit
  push    edi
  push    esi
  call CompareVTOLUnitWeight
  test    eax, eax
  jz RefuseLoadReasonWeight
AllowLoadUnit :
  popAD
  push $00411288;
  call PatchNJump;
RefuseLoadReasonWeight :
  popAD
  push $0041126D;
  call PatchNJump;
end;

Procedure OnInstallTransporters;
begin
end;

Procedure OnUninstallTransporters;
begin
end;

function GetPlugin: TPluginData;
begin
  if IsTAVersion31 and State_Transporters then
  begin
    Result := TPluginData.Create( False, 'Transporters Plugin',
                                  State_Transporters,
                                  @OnInstallTransporters,
                                  @OnUninstallTransporters );

    Result.MakeRelativeJmp( State_Transporters,
                            'Ground transporter overload fix',
                            @Transporters_TransportOverloadFix,
                            $00406789, 3 );

    Result.MakeRelativeJmp( State_Transporters,
                            'Enable multi air load',
                            @Transporters_VTOLTransportCapacityCanLoadTest,
                            $00411224, 0 );

    Result.MakeRelativeJmp( State_Transporters,
                            'Enable weight air load',
                            @Transporters_VTOLTransportSize,
                            $0041125F, 3 );
  end else
    Result := nil;
end;

end.
