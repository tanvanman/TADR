unit BroadcastNanolathe;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_BroadcastNanolathe: Boolean = True;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallBroadcastNanolathe;
Procedure OnUninstallBroadcastNanolathe;

// -----------------------------------------------------------------------------

implementation
uses
  idplay,
  IniOptions,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_FunctionsU;

procedure BroadcastNanolatheParticles(PosStart: PPosition;
  PosTarget: PNanolathePos; Reverse: Integer); stdcall;
begin
  if TAData.NetworkLayerEnabled then
    GlobalDPlay.Broadcast_SetNanolatheParticles(PosStart^, PosTarget^, Reverse);
end;

procedure BroadcastNanolatheParticles_BuildingBuild;
asm
  mov     [esp+34h], edx
  mov     [esp+38h], edi
  pushAD
  push    0
  push    eax
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $00403ECC;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_MobileBuild;
asm
  add     edi, [eax+TUnitInfo.lFootPrintY]
  mov     [esp+44h], edi
  pushAD
  push    0
  push    ecx
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $00402ABD;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_HelpBuild;
asm
  mov     [esp+34h], edx
  mov     [esp+38h], esi
  pushAD
  push    0
  push    eax
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $004041C2;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_Resurrect;
asm
  push    eax
  push    ecx
  mov     [esp+38h], ebp
  pushAD
  push    0
  push    eax
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $0040509D;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_RepairResurrect;
asm
  add     edi, [eax+16Eh]
  mov     [esp+38h], edi
  pushAD
  push    0
  push    ecx
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $004056D1;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_RepairUnitNoMove;
asm
  add     edi, [eax+16Eh]
  mov     [esp+38h], edi
  pushAD
  push    0
  push    ecx
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $004058F6;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_VTOL_MobileBuild;
asm
  push    ecx
  push    edx
  mov     [esp+38h], edi
  pushAD
  push    0
  push    ecx
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $004142B6;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_VTOL_HelpBuild;
asm
  push    ecx
  mov     [esp+38h], edi
  pushAD
  push    0
  push    eax
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $004146D7;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_VTOL_Repair;
asm
  push    ecx
  push    edx
  mov     [esp+38h], edi
  pushAD
  push    0
  push    ecx
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $004151EC;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_Capture;
asm
  push    ecx
  push    edx
  mov     [esp+38h], edi
  pushAD
  push    1
  push    edx
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $00404676;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_ReclaimUnit;
asm
  push    ecx
  push    edx
  mov     [esp+40h], edi
  pushAD
  push    1
  push    edx
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $00404A4E;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_ReclaimFeature;
asm
  push    eax
  push    ecx
  mov     [esp+38h], edi
  pushAD
  push    1
  push    ecx
  push    eax
  call    BroadcastNanolatheParticles
  popAD
  call    EmitSfx_NanoParticlesReverse
  lea     edx, [esp+14h]
  push    6
  lea     eax, [esp+24h]
  push    edx
  push    eax
  pushAD
  push    1
  push    eax
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $00404D4C;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_VTOLReclaimUnit;
asm
  push    ecx
  push    edx
  mov     [esp+40h], edi
  pushAD
  push    1
  push    edx
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $00414C43;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_VTOLReclaimFeature;
asm
  push    eax
  push    ecx
  mov     [esp+34h], edi
  pushAD
  push    1
  push    ecx
  push    eax
  call    BroadcastNanolatheParticles
  popAD
  call    EmitSfx_NanoParticlesReverse
  lea     edx, [esp+10h]
  push    6
  lea     eax, [esp+20h]
  push    edx
  push    eax
  pushAD
  push    1
  push    eax
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $00414A31;
  call PatchNJump;
end;

Procedure OnInstallBroadcastNanolathe;
begin
end;

Procedure OnUninstallBroadcastNanolathe;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_BroadcastNanolathe then
  begin
    Result := TPluginData.create( False,
                                  'Broadcast nanolathe particles for each order type that use it',
                                  State_BroadcastNanolathe,
                                  @OnInstallBroadcastNanolathe,
                                  @OnUninstallBroadcastNanolathe );
                                  
    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_BuildingBuild,
                            $00403EC4, 3 );

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_MobileBuild,
                            $00402AB3, 5 );

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_HelpBuild,
                            $004041BA, 3 );

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_Resurrect,
                            $00405097, 1 );

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_RepairResurrect,
                            $004056C7, 1 );

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_RepairUnitNoMove,
                            $004058EC, 1 );

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_VTOL_MobileBuild,
                            $004142B0, 1);

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_VTOL_HelpBuild,
                            $004146D2, 0);

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_VTOL_Repair,
                            $004151E6, 1);

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_Capture,
                            $00404670, 1);

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_ReclaimUnit,
                            $00404A48, 1);

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_ReclaimFeature,
                            $00404D35, 1);

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_VTOLReclaimUnit,
                            $00414C3D, 1);

    Result.MakeRelativeJmp( State_BroadcastNanolathe,
                            '',
                            @BroadcastNanolatheParticles_VTOLReclaimFeature,
                            $00414A1A, 1);
  end else
    Result := nil;
end;

end.
