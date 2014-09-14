unit UnitActions;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_UnitActions : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallUnitActions;
Procedure OnUninstallUnitActions;

// -----------------------------------------------------------------------------

procedure UnitActions_UnitBuildWeapons;
procedure UnitActions_AssignAISquad;
procedure UnitActions_TransportOverloadFix;
procedure UnitActions_VTOLTransportCapacityCanLoadTest; // order init capacity
procedure UnitActions_VTOLTransportSize;          // load unit cursor test
//procedure UnitActions_VTOLTransportSizeCursor;
procedure UnitActions_ExtraVTOLOrders;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_FunctionsU,
  UnitsExtend;

Procedure OnInstallUnitActions;
begin
end;

Procedure OnUninstallUnitActions;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_UnitActions then
  begin
    Result := TPluginData.create( false,
                            'UnitActions Plugin',
                            State_UnitActions,
                            @OnInstallUnitActions,
                            @OnUninstallUnitActions );

    Result.MakeRelativeJmp( State_UnitActions,
                          'Ground transporter overload fix',
                          @UnitActions_TransportOverloadFix,
                          $00406789, 3);

    Result.MakeRelativeJmp( State_UnitActions,
                          'Enable multi air load',
                          @UnitActions_VTOLTransportCapacityCanLoadTest,
                          $00411224, 0);

    Result.MakeRelativeJmp( State_UnitActions,
                          'Enable weight air load',
                          @UnitActions_VTOLTransportSize,
                          $0041125F, 3);
{
    UnitActionsPlugin.MakeRelativeJmp( State_UnitActions,
                          'Enable weight air load cursor',
                          @UnitActions_VTOLTransportSizeCursor,
                          $00489B0B, 3);
}
    Result.MakeRelativeJmp( State_UnitActions,
                            'Enable resurrect and capture orders fot VTOLs',
                            @UnitActions_ExtraVTOLOrders,
                            $00438AE3, 0);
{
     Result.MakeRelativeJmp( State_UnitActions,
                            'UnitActions_UnitBuildWeapons',
                            @UnitActions_UnitBuildWeapons,
                            $00419B59, 0);
}
     Result.MakeRelativeJmp( State_UnitActions,
                            'UnitActions_AssignAISquad',
                            @UnitActions_AssignAISquad,
                            $00408846, 0);

  end else
    Result := nil;
end;

function OverloadBugFix(UnitPtr : Pointer): LongBool; stdcall;
begin
  Result := False;
  if (TAUnit.GetLoadCurAmount(UnitPtr) + 1) <= PUnitInfo(PUnitStruct(UnitPtr).p_UnitDef).cTransportCap then
    Result := True;
end;

{
.text:00406789 008 85 C0                        test    eax, eax
.text:0040678B 008 0F 84 3A 01 00 00            jz      loc_4068CB
.text:00406791 008 F6 44 24 14 08               test    [esp+8+arg_8], 8
}
procedure UnitActions_TransportOverloadFix;
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

{
.text:00411224 024 85 C9                         test    ecx, ecx
.text:00411226 024 0F 85 FF 02 00 00             jnz     loc_41152B
.text:0041122C 024 33 C9                         xor     ecx, ecx
}
procedure UnitActions_VTOLTransportCapacityCanLoadTest;
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
  call    GetUnitExtProperty
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

{
.text:00406789 008 85 C0                                                           test    eax, eax
.text:0040678B 008 0F 84 3A 01 00 00                                               jz      loc_4068CB
.text:00406791 008 F6 44 24 14 08                                                  test    [esp+8+arg_8], 8
}

function CompareVTOLUnitWeight(Transporter, ToBeTransported : Pointer) : LongBool; stdcall;
var
  MaxWeight : Integer;
  CurLoadWeight : Integer;
  TobeTransportedLoadWeight : Integer;
begin
  Result := False;
  MaxWeight := GetUnitExtProperty(Transporter, 3);
  CurLoadWeight := TAUnit.GetLoadWeight(Transporter);
  TobeTransportedLoadWeight := Round(PUnitInfo(TAUnit.GetUnitInfoPtr(ToBeTransported)).lBuildCostMetal);
  if (TobeTransportedLoadWeight + CurLoadWeight) < MaxWeight then
    Result := True;
end;

{
// ecx - transporter unitinfo
// eax - transported unit

.text:0041125F 024 66 0F B6 89 2A 02 00 00       movzx   cx, [ecx+UNITINFO.transportsize]
.text:00411267 024 66 39 48 7E                   cmp     word ptr [eax+UnitsInGame.FootX], cx
.text:0041126B 024 7E 1B                         jle     short loc_411288
.text:0041126D 024 68 DC 1B 50 00                push    offset aUnitIsTooHeavy
}
procedure UnitActions_VTOLTransportSize;
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
  call    GetUnitExtProperty
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

{
// edi - transporter unitinfo
// esi - transported unitinfo
.text:00489B0B 00C 66 0F B6 97 2A 02 00 00        movzx   dx, [edi+UNITINFO.transportsize] // 22Ah
.text:00489B13 00C 66 39 96 4A 01 00 00           cmp     [esi+UNITINFO.FootX], dx         // 14Ah
.text:00489B1A 00C 7E 08                          jle     short loc_489B24                 // continue
.text:00489B1C 00C 33 C0                          xor     eax, eax                         // refuse
}
{procedure UnitActions_VTOLTransportSizeCursor;
label
  FootSizeCompare,
  RefuseLoad,
  ContinueTransportedTest;
asm
FootSizeCompare :
  movzx   dx, [edi+TUnitInfo.cTransportSize]
  cmp     [esi+TUnitInfo.nFootPrintX], dx
  jle     ContinueTransportedTest
RefuseLoad :
  push $00489B1C;
  call PatchNJump;
ContinueTransportedTest :
  push $00489B24;
  call PatchNJump;
end;    }

{
.text:00438AD4 008 8B 46 0E                      mov     eax, [esi+UnitOrder.Unit_ptr]
.text:00438AD7 008 8B 88 92 00 00 00             mov     ecx, [eax+UnitsInGame.UNITINFO_p]
.text:00438ADD 008 8B 91 41 02 00 00             mov     edx, [ecx+UNITINFO.UnitTypeMask_0]
.text:00438AE3 008 F6 C6 08                      test    dh, 8
.text:00438AE6 008 75 78                         jnz     short loc_438B60
.text:00438AE8 008 6A 18                         push    18h             ; Size
}
procedure UnitActions_ExtraVTOLOrders;
label
  AirUnitChase,
  GroundMovementChase;
asm
  test    dh, 8
  jnz     AirUnitChase
  jmp GroundMovementChase
AirUnitChase :
  pushAD
  push    2
  push    eax
  call    GetUnitExtProperty
  test    eax, eax
  popAD
  jnz     GroundMovementChase
  push $00438B60;
  call PatchNJump;
GroundMovementChase :
  push $00438AE8;
  call PatchNJump;
end;

procedure UnitActions_UnitBuildWeapons;
label
  BuildWeapon;
asm
  add     esp, 8
  test    eax, eax
  jnz     BuildWeapon
  push $00419B60;
  call PatchNJump;
BuildWeapon:
  push $00419B97;
  call PatchNJump;
end;

var
  squadnr : Integer;
procedure UnitActions_AssignAISquad;
label
  ForceSquad;
asm
  // esi unit
  pushAD
  push    7
  push    esi
  xor     eax, eax
  call    GetUnitExtProperty
  test    ax, ax
  jnz     ForceSquad
  popAD
  mov     eax, [esi+110h]
  push $0040884C;
  call PatchNJump;
ForceSquad:
  mov     squadnr, eax
  popAD
  push    squadnr
  push $004088F9;
  call PatchNJump;
end;

end.

