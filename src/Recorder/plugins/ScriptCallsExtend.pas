unit ScriptCallsExtend;

// to extend calls from TA to COB scripts
// 1) AimPrimary / AimSecondary / AimTertiary - by including target unit ID
// 2) WeaponHit - called when weapon fired from unit hit the target (or not), includes also weapon ID
// 3) ConfirmVTOLTransport - vtol loads or unloads a unit

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_ScriptCallsExtend : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallScriptCallsExtend;
Procedure OnUninstallScriptCallsExtend;

// -----------------------------------------------------------------------------

// AimPrimary / AimSecondary / AimTertiary
procedure ScriptCallsExtend_AimPrimaryTurret;
procedure ScriptCallsExtend_AimPrimaryBallistic;

// WeaponHit
procedure ScriptCallsExtend_WeaponHitTest;

// ConfirmVTOLTransport
procedure ScriptCallsExtend_CallAirLoadScript;
procedure ScriptCallsExtend_CallAirUnLoadScript;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_FunctionsU, logging, sysutils;

var
  ScriptCallsExtendPlugin: TPluginData;

Procedure OnInstallScriptCallsExtend;
begin
end;

Procedure OnUninstallScriptCallsExtend;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_ScriptCallsExtend then
  begin
    ScriptCallsExtendPlugin := TPluginData.create( false,
                            'ScriptCallsExtend Plugin',
                            State_ScriptCallsExtend,
                            @OnInstallScriptCallsExtend,
                            @OnUninstallScriptCallsExtend );

    ScriptCallsExtendPlugin.MakeRelativeJmp( State_ScriptCallsExtend,
                          'ScriptCallsExtend_AimPrimaryTurret',
                          @ScriptCallsExtend_AimPrimaryTurret,
                          $0049E2A5, 0);

    ScriptCallsExtendPlugin.MakeRelativeJmp( State_ScriptCallsExtend,
                          'ScriptCallsExtend_AimPrimaryBallistic',
                          @ScriptCallsExtend_AimPrimaryBallistic,
                          $0049E35D, 0);
    
    ScriptCallsExtendPlugin.MakeRelativeJmp( State_ScriptCallsExtend,
                          'Did weapon hit the target ?',
                          @ScriptCallsExtend_WeaponHitTest,
                          $00406F5B, 2);

    ScriptCallsExtendPlugin.MakeRelativeJmp( State_ScriptCallsExtend,
                          'call air load',
                          @ScriptCallsExtend_CallAirLoadScript,
                          $004114A0, 1);

    ScriptCallsExtendPlugin.MakeRelativeJmp( State_ScriptCallsExtend,
                          'call air unload',
                          @ScriptCallsExtend_CallAirUnLoadScript,
                          $00411799, 1);

    Result:= ScriptCallsExtendPlugin;
  end else
    Result := nil;
end;

var
  TargetUnit_Turret: Cardinal;
procedure ScriptCallsExtend_AimPrimaryTurret;
label
  ShootingGround,
  loc_49E2AE,
  loc_49E2CC,
  ContinueToCall,
  loc_49E3AE;
asm
    push    ebx
    pushf
    mov     bx, word [edi+$4+2]
    test    bh, $80
    jz      ShootingGround
    movzx   ebx, word [edi+$4]
    mov     TargetUnit_Turret, ebx
    jmp loc_49E2AE
ShootingGround:
    mov     TargetUnit_Turret, 0
loc_49E2AE:
    popf
    pop     ebx
    test    byte ptr [ebp+111h], 1
    jz      loc_49E2CC
    shr     al, 2
    lea     ecx, [esp+40h-$18]
    and     al, 3
    push    ecx
    push    eax
    lea     edx, [esp+48h+4]
    lea     eax, [esp+48h-$2C]
    push    edx
    push    eax
    push    ebp
    push    edi
    call    sub_49D910
    jmp     ContinueToCall
loc_49E2CC:
    xor     eax, eax;
ContinueToCall:
    mov     ecx, [esp+40h+4]
    test    eax, eax
    jz      loc_49E3AE
    mov     eax, [esp+40h-$2C]
    mov     dl, [esi]
    mov     [esi-3], cx
    mov     [esi-5], ax
    push    0 // par4
    and     ecx, 0FFFFh
    shr     edx, 2
    push    TargetUnit_Turret       // target unit long ID, instead of 0
    and     eax, 0FFFFh
    push    ecx
    and     edx, 3
    push    eax
    mov     dword ptr [esi-13h], 0
    mov     eax, $509688[edx*4]    // AimPrimary / AimSecondary / AimTertiary
    push    3                      // par count
    lea     ecx, [esi-17h]
    push    0

    //push $0049E30F
    push $0049E314
    call PatchNJump
loc_49E3AE:
    push $0049E3AE;
    call PatchNJump;
end;

procedure ScriptCallsExtend_AimPrimaryBallistic;
label
   GroundTarget,
   ContinueToGame;
asm
    push    ebx
    pushf
    mov     bx, word [edi+$4+2]
    test    bh, $80
    jz      GroundTarget
    movzx   ebx, word [edi+$4]
    mov     edx, ebx
    popf
    pop     ebx
    jmp ContinueToGame
GroundTarget:
    popf
    pop     ebx
    mov     edx, 0
ContinueToGame:
    push    edx                       // target unit long ID or 0
    shr     eax, 2
    push    0
    and     eax, 3
    push    0
    mov     dword ptr [esi-13h], 0
    mov     edx, $509688[eax*4]
    push    3
    push $0049E379;
    call PatchNJump;
end;

procedure CallWeaponHit(Projectile: Pointer; Hit: Cardinal); stdcall;
var
  WeapID : Cardinal;
begin
  if PWeaponProjectile(Projectile).p_AttackerUnit <> nil then
  begin
    if PUnitStruct(PWeaponProjectile(Projectile).p_AttackerUnit).p_UnitScriptsData <> nil then
    begin
      if IniSettings.WeaponType <= 256 then
        WeapID := PWeaponDef(PWeaponProjectile(Projectile).Weapon).ucID
      else
        WeapID := PWeaponDef(PWeaponProjectile(Projectile).Weapon).lWeaponIDCrack;

      Script_ProcessCallback( nil,
                              nil,
                              LongWord(PUnitStruct(PWeaponProjectile(Projectile).p_AttackerUnit).p_UnitScriptsData),
                              nil, nil, @Hit, @WeapID, PAnsiChar('WeaponHit') );

    end;
  end;
end;

procedure ScriptCallsExtend_WeaponHitTest;
label
  DidntHit;
asm
  mov     ecx, [esp+$C]
  lea     edx, [ecx+ecx]
  mov     ecx, [esp+8]
  cmp     ecx, edx
  pushAD
  jle     DidntHit          // hit or not
  or      byte ptr [eax+0BBh], 40h
  mov     ebx, [esp+$24]      // projectile
  mov     ecx, 1
  push    ecx
  push    ebx
  call    CallWeaponHit
  popAD
  push $00406F71;
  call PatchNJump;
DidntHit :
  or      byte ptr [eax+0BBh], 20h
  mov     ebx, [esp+$24]
  mov     ecx, 0
  push    ecx
  push    ebx
  call    CallWeaponHit
  popAD
  push $00406F7B;
  call PatchNJump;
end;

procedure VTOLLoadUnload(UnitPtr: Pointer; TransportedUnit: Pointer; Piece: Cardinal; Loading: Cardinal); stdcall;
begin
  if PUnitStruct(UnitPtr).p_UnitScriptsData <> nil then
  begin
    Script_RunScript( 0, 0, LongWord(PUnitStruct(UnitPtr).p_UnitScriptsData),
                      0, TAUnit.GetId(TransportedUnit), Piece, Loading,
                      3, 0, 0,
                      PAnsiChar('ConfirmVTOLTransport') );
  end;
end;

{
.text:004114A0 028 8B 4F 36                       mov     ecx, [edi+UnitOrder.lPar1]
.text:004114A3 028 51                             push    ecx             ; a3
.text:004114A4 02C 56                             push    esi             ; a2
.text:004114A5 030 50                             push    eax             ; a1
.text:004114A6 034 E8 15 96 07 00                 call    Send_LabStartBuild
}
procedure ScriptCallsExtend_CallAirLoadScript;
label
  SendLabStartBuild;
asm
  mov     ecx, [edi+TUnitOrder.lPar1]
  // ecx piece
  // esi transporter
  // eax transported unit
  pushAD
  push    1
  push    ecx
  push    eax
  push    esi
  call VTOLLoadUnload
  popAD

  push    ecx
  push    esi
  push    eax
SendLabStartBuild :
  push $004114A6;
  call PatchNJump;
end;

procedure ScriptCallsExtend_CallAirUnLoadScript;
asm
  mov     eax, [ebx+TUnitStruct.p_TransportedUnit]
  pushAD
  push    0
  push    0 // this order dont contain piece nr
  push    eax
  push    ebx
  call VTOLLoadUnload
  popAD
  push $0041179F;
  call PatchNJump;
end;


end.

