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
procedure ScriptCallsExtend_AimPrimaryTurret_GetUnit;
procedure ScriptCallsExtend_AimPrimaryTurret_ExpCall;
procedure ScriptCallsExtend_AimPrimaryBallistic;

// WeaponHit
procedure ScriptCallsExtend_WeaponHitTest;

// HitByWeapon
procedure ScriptCallsExtend_HitByWeaponExpand;
procedure ScriptCallsExtend_ConfirmedKill;
procedure ScriptCallsExtend_TookDamage;

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
                          'ScriptCallsExtend_AimPrimaryTurret_GetUnit',
                          @ScriptCallsExtend_AimPrimaryTurret_GetUnit,
                          $0049E21E, 1);

    ScriptCallsExtendPlugin.MakeRelativeJmp( State_ScriptCallsExtend,
                          'ScriptCallsExtend_AimPrimaryTurret_ExpCall',
                          @ScriptCallsExtend_AimPrimaryTurret_ExpCall,
                          $0049E2E8, 3);

    ScriptCallsExtendPlugin.MakeRelativeJmp( State_ScriptCallsExtend,
                          'ScriptCallsExtend_AimPrimaryBallistic',
                          @ScriptCallsExtend_AimPrimaryBallistic,
                          $0049E35D, 0);

    ScriptCallsExtendPlugin.MakeRelativeJmp( State_ScriptCallsExtend,
                          'Did weapon hit the target ?',
                          @ScriptCallsExtend_WeaponHitTest,
                          $00406F5B, 2);
 {
    ScriptCallsExtendPlugin.MakeRelativeJmp( State_ScriptCallsExtend,
                          'Include damage value for HitByWeapon',
                          @ScriptCallsExtend_HitByWeaponExpand,
                          $00489F32, 0);     }

    ScriptCallsExtendPlugin.MakeRelativeJmp( State_ScriptCallsExtend,
                          'Unit took damage call',
                          @ScriptCallsExtend_TookDamage,
                          $00489D3F, 1);

    ScriptCallsExtendPlugin.MakeRelativeJmp( State_ScriptCallsExtend,
                          'Confirmed unit kill',
                          @ScriptCallsExtend_ConfirmedKill,
                          $004864B6, 0);

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
procedure ScriptCallsExtend_AimPrimaryTurret_GetUnit;
label
  ShootingGround,
  GoBack;
asm
    push    ebx
    pushf
    mov     bx, word [edi+$4+2]
    test    bh, $80
    movzx   ebx, word [edi+$4]
    mov     TargetUnit_Turret, ebx
    jnz     GoBack
ShootingGround:
    mov     TargetUnit_Turret, 0
GoBack:
    popf
    pop     ebx
    mov     ecx, [ebp+111h]
    push $0049E224
    call PatchNJump
end;

procedure ScriptCallsExtend_AimPrimaryTurret_ExpCall;
asm
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
    push $0049E314
    call PatchNJump
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

procedure CallWeaponHit(Projectile: PWeaponProjectile; Hit: Cardinal); stdcall;
var
  WeapID : Cardinal;
begin
  if PWeaponProjectile(Projectile).p_AttackerUnit <> nil then
  begin
    if PUnitStruct(PWeaponProjectile(Projectile).p_AttackerUnit).p_UnitScriptsData <> nil then
    begin
      WeapID := TAWeapon.GetWeaponID(PWeaponProjectile(Projectile).Weapon);
      Script_RunScript ( 0, 0, LongWord(PUnitStruct(PWeaponProjectile(Projectile).p_AttackerUnit).p_UnitScriptsData),
                         Projectile.Position_Curnt.Z, Projectile.Position_Curnt.X, Hit, WeapID,
                         4,
                         0, 0,
                         PAnsiChar('WeaponHit') );

  {    Script_ProcessCallback( nil,
                              nil,
                              LongWord(PUnitStruct(PWeaponProjectile(Projectile).p_AttackerUnit).p_UnitScriptsData),
                              nil, nil, @Hit, @WeapID, PAnsiChar('WeaponHit') );   }

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

{
.text:00489F32 00C 6A 00                               push    0                           ; a10
.text:00489F34 010 6A 00                               push    0                           ; a9
.text:00489F36 014 50                                  push    eax                         ; a8
.text:00489F37 018 53                                  push    ebx                         ; a7
.text:00489F38 01C 6A 02                               push    2                           ; a6
.text:00489F3A 020 6A 00                               push    0                           ; a5
.text:00489F3C 024 6A 00                               push    0                           ; a4
.text:00489F3E 028 68 74 8D 50 00                      push    offset aHitbyweapon         ; "HitByWeapon"
.text:00489F43 02C E8 28 6B 02 00                      call    Script_RunCallBack
}

procedure HitByWeaponNew(UnitPtr : Cardinal; anglex, anglez, damage : Cardinal); stdcall;
begin
  if PUnitStruct(Pointer(UnitPtr)).p_UnitScriptsData <> nil then
  begin
  Script_ProcessCallback( nil, nil, LongWord(PUnitStruct(Pointer(UnitPtr)).p_UnitScriptsData),
                          nil, @damage, @anglez, @anglex,
                          PAnsiChar('HitByWeapon') );
  end;
end;

procedure ScriptCallsExtend_HitByWeaponExpand;
asm
  pushAD
  movzx   edx, word ptr [edi+5]
  push    edx
  push    eax
  push    ebx
  push    esi
  call    HitByWeaponNew
  popAD
  push $00489F48;
  call PatchNJump;
end;

procedure TookDamage(UnitPtr: Pointer; DmgType: Cardinal; DmgAmount: Cardinal; AttackerID: Cardinal); stdcall;
begin
  if PUnitStruct(Pointer(UnitPtr)).p_UnitScriptsData <> nil then
  begin
    Script_ProcessCallback( nil, nil, LongWord(PUnitStruct(Pointer(UnitPtr)).p_UnitScriptsData),
                            nil, @AttackerID, @DmgAmount, @DmgType,
                            PAnsiChar('TookDamage') );
  end;  
end;

procedure ScriptCallsExtend_TookDamage;
label
  DontMakeDmg;
asm
  mov     eax, [esi+TUnitStruct.lUnitStateMask]
  test    eax, 10000000h
  jz      DontMakeDmg
  test    ah, 40h
  jnz     DontMakeDmg
  pushAD
  movzx   edx, word ptr [edi+3]
  push    edx
  movzx   edx, word ptr [edi+5]
  push    edx
  movzx   edx, byte ptr [edi+8]
  push    edx
  push    esi
  call    TookDamage
  popAD
  // edi+5 dmg amount   word
  // edi+8 dmg type     byte
  // edi+3 attacker id  word
  push $00489D59;
  call PatchNJump;
DontMakeDmg :
  push $00489F93;
  call PatchNJump;
end;

procedure ConfirmedKill(UnitPtr: Pointer; DeathType : Cardinal); stdcall;
begin
  if PUnitStruct(Pointer(UnitPtr)).p_UnitScriptsData <> nil then
  begin
    Script_ProcessCallback( nil,
                            nil,
                            LongWord(PUnitStruct(UnitPtr).p_UnitScriptsData),
                            nil, nil, nil, @DeathType, PAnsiChar('ConfirmedKill') );
  end;
end;

procedure ScriptCallsExtend_ConfirmedKill;
asm
  // ebx deathtype
  // esi target unit
  mov     esi, [esp+1Ch+4]
  mov     ebx, [esp+1Ch+8]
  pushAD
  push    ebx
  push    esi
  call ConfirmedKill
  popAD
  push    edi
  push $004864BB;
  call PatchNJump;
end;

procedure VTOLLoadUnload(UnitPtr: Pointer; TransportedUnit: Pointer; Piece: Cardinal; Loading: Cardinal); stdcall;
var
  UnitID : Word;
begin
  if PUnitStruct(UnitPtr).p_UnitScriptsData <> nil then
  begin
    UnitID := TAUnit.GetId(TransportedUnit);
    Script_ProcessCallback( nil,
                            nil,
                            LongWord(PUnitStruct(UnitPtr).p_UnitScriptsData),
                            nil, @UnitID, @Piece, @Loading, PAnsiChar('ConfirmVTOLTransport') );
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

