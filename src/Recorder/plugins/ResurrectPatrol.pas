unit ResurrectPatrol;

// to make resurrector units able to resurrect patrol
// - reorder priority (resurrect should be before repair patrol and reclaim features) 405A6B

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_ResurrectPatrol : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallResurrectPatrol;
Procedure OnUninstallResurrectPatrol;

// -----------------------------------------------------------------------------

procedure ResurrectPatrol_ReclaimToResurrect;
procedure ResurrectPatrol_AvoidEnergyAndRepair;
procedure ResurrectPatrol_AvoidMetalCheck;

implementation
uses
  SysUtils,
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_MemoryStructures,
  TA_FunctionsU;

var
  ResurrectPatrolPlugin: TPluginData;

Procedure OnInstallResurrectPatrol;
begin
end;

Procedure OnUninstallResurrectPatrol;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_ResurrectPatrol then
  begin
    ResurrectPatrolPlugin := TPluginData.create( false,
                            'ResurrectPatrol',
                            State_ResurrectPatrol,
                            @OnInstallResurrectPatrol,
                            @OnUnInstallResurrectPatrol );

    ResurrectPatrolPlugin.MakeRelativeJmp( State_ResurrectPatrol,
                          'Swap reclaim order with resurrect',
                          @ResurrectPatrol_ReclaimToResurrect,
                          $00405BE3, 1);

    ResurrectPatrolPlugin.MakeRelativeJmp( State_ResurrectPatrol,
                          'Avoid checks',
                          @ResurrectPatrol_AvoidEnergyAndRepair,
                          $00405A03, 0);

    ResurrectPatrolPlugin.MakeRelativeJmp( State_ResurrectPatrol,
                          'Avoid metal check',
                          @ResurrectPatrol_AvoidMetalCheck,
                          $00405BC6, 0);

    Result:= ResurrectPatrolPlugin;
  end else
    Result := nil;
end;

var
  ResPosition : TPosition;
function FixFeaturePosition(Unit_p: Cardinal; PosToFix: Cardinal) : Cardinal; stdcall;
begin
  ResPosition := PPosition(Pointer(PosToFix))^;
  ResPosition.Y := GetPosHeight(@ResPosition);
  Result := LongWord(@ResPosition);
end;

{
.text:00405BE3 040 8B 4C 24 44                                                     mov     ecx, [esp+40h+a3]
.text:00405BE7 040 53                                                              push    ebx             ; UnitOrderFlags
.text:00405BE8 044 53                                                              push    ebx             ; lPar2
.text:00405BE9 048 53                                                              push    ebx             ; lPar1
.text:00405BEA 04C 51                                                              push    ecx             ; Position_DWORD_p
.text:00405BEB 050 53                                                              push    ebx             ; TargatUnit
.text:00405BEC 054 51                                                              push    ecx             ; Scrip_index
.text:00405BED 058 8B CC                                                           mov     ecx, esp        ; RtnIndex_ptr
.text:00405BEF 058 68 C4 16 50 00                                                  push    offset aReclaim ; "RECLAIM"
.text:00405BF4 05C E8 67 2B 03 00                                                  call    ScriptAction_Name2Index
}
procedure ResurrectPatrol_ReclaimToResurrect;
label
  Reclaim,
  Resurrect,
  CreateOrder;
asm
  pushf
  push    eax
  push    ebx
  mov     ebx, [esi+$92] // unit info struct
  mov     eax, [ebx+$245] // unit type mask #2
  and     ax, 2048       // canresurrect 0 / 1
  jz      Reclaim
Resurrect:
  mov     eax, esi  // unit
  mov     ebx, [esp+40h+$E] // pos to fix
  push    ebx
  push    eax
  call    FixFeaturePosition
  mov     ecx, eax
  pop     ebx
  pop     eax
  popf
  push    ebx             // UnitOrderFlags
  push    ebx             // lPar2
  push    ebx             // lPar1
  push    ecx             // Position_DWORD_p with fixed height
  push    ebx             // TargatUnit
  push    ecx             // Scrip_index
  mov     ecx, esp        // RtnIndex_ptr
  push    $005052F0   // PAnsiChar RESURRECT
  jmp     CreateOrder
Reclaim:
  pop     ebx
  pop     eax
  popf
  mov     ecx, [esp+40h+$4]
  push    ebx             // UnitOrderFlags
  push    ebx             // lPar2
  push    ebx             // lPar1
  push    ecx             // Position_DWORD_p
  push    ebx             // TargatUnit
  push    ecx             // Scrip_index
  mov     ecx, esp        // RtnIndex_ptr
  push    $005016C4  // PAnsiChar RECLAIM
CreateOrder:
  push $00405BF4;
  call PatchNJump;
end;

procedure ResurrectPatrol_AvoidEnergyAndRepair;
label
  SearchForFeaturesResurrect,
  GoToRepair,
  SearchForFeatures;
asm
  pushf
  push    eax
  push    ebx
  mov     ebx, [esi+$92] // unit info struct
  mov     eax, [ebx+$245] // unit type mask #2
  and     ax, 2048       // canresurrect 0 / 1
  jnz     SearchForFeaturesResurrect
  pop     ebx
  pop     eax
  popf
  jz      GoToRepair
  push $00405A09
  call PatchNJump;
SearchForFeaturesResurrect :
  pop     ebx
  pop     eax
  popf
  push $00405B5A
  call PatchNJump;
GoToRepair:
  push $00405B18
  call PatchNJump;
end;

{
 replace '_' in corpse name with null character
 f.e. armguard_dead into armguard (armguard#0dead)
 this is native TA solution to get unit type from wreck
}
function WreckToUnitType(WreckTypeName : PAnsiChar): PAnsiChar; stdcall;
begin
  Result := PAnsiChar(StringReplace(WreckTypeName, '_', #0, []));
end;

{
.text:00405BC6 040 F6 C4 41                                                        test    ah, 41h
.text:00405BC9 040 75 5A                                                           jnz     short loc_405C25 ; fMaxMetalStorage * 0.2 > fCurrentMetal
.text:00405BCB 040 53                                                              push    ebx             ; OrderCallBack_p
}
procedure ResurrectPatrol_AvoidMetalCheck;
label
  MakeOrderResurrect,
  ContinueReclaimCheck,
  DontCheckIsWreck,
  GoToIsFeatureEnergy,
  StopResurrectNonWreck;
asm
  pushf
  push    eax
  push    ebx
  mov     ebx, [esi+$92] // unit info struct
  mov     eax, [ebx+$245] // unit type mask #2
  and     ax, 2048       // canresurrect 0 / 1
  jnz     MakeOrderResurrect
ContinueReclaimCheck :
  pop     ebx
  pop     eax
  popf
  test    ah, 41h
  jnz     GoToIsFeatureEnergy
  push $00405BCB
  call PatchNJump;
MakeOrderResurrect :
// check here is feature wreck
  lea     ebx, [esp+40h-$16]
  push    ecx
  push    edx
  push    0
  push    0
  push    ebx
  call    GetFeatureTypeOfOrder
  pop     edx
  cmp     ax, 0FFFFh
  jz      DontCheckIsWreck           // feature isnt there anymore
  mov     ecx, [TADynmemStructPtr]
  mov     ebx, [ecx+1426Fh]          // PTAdynmemStruct(TAData.MainStructPtr).p_FeatureDefs
  and     eax, $0000FFFF
  mov     ecx, type TFeatureDefStruct // size of TFeatureDefStruct
  push    edx
  mul     ecx
  add     ebx, eax // ebx := FeatureDefs[ecx*eax]
  push    ebx
  call    WreckToUnitType
  push    eax
  call    UnitName2ID
  pop     edx
  and     eax, 0FFFFh
  jz      StopResurrectNonWreck
DontCheckIsWreck :
  pop     ecx
  pop     ebx
  pop     eax
  popf
  push $00405BCB
  call PatchNJump;
GoToIsFeatureEnergy:
  push $00405C25
  call PatchNJump;
StopResurrectNonWreck :
  pop     ecx
  pop     ebx
  pop     eax
  popf
  push $00405C25
  call PatchNJump;
end;

end.

