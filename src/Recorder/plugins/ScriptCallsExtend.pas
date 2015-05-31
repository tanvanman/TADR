unit ScriptCallsExtend;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_ScriptCallsExtend: Boolean = True;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallScriptCallsExtend;
Procedure OnUninstallScriptCallsExtend;

// -----------------------------------------------------------------------------

implementation
uses
  IniOptions,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_MemUnits,
  TA_FunctionsU,
  COB_Extensions;

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

procedure CallWeaponHit(p_Projectile: PWeaponProjectile; Hit: Cardinal); stdcall;
var
  WeapID: Cardinal;
begin
  if p_Projectile.p_AttackerUnit <> nil then
  begin
    if p_Projectile.p_AttackerUnit.p_UnitScriptsData <> nil then
    begin
      WeapID := TAWeapon.GetWeaponID(p_Projectile.p_Weapon);

      TAUnit.CobStartScript(p_Projectile.p_AttackerUnit, 'WeaponHit',
                            @WeapID, @Hit,
                            @p_Projectile.Position_Curnt.X, @p_Projectile.Position_Curnt.Z,
                            True);
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

procedure TookDamage(p_Unit: PUnitStruct;
  DmgType: Cardinal; DmgAmount: Cardinal; AttackerID: Cardinal); stdcall;
begin
  TAUnit.CobStartScript( p_Unit, 'TookDamage',
                         @DmgType, @DmgAmount, @AttackerID, nil,
                         False );
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

procedure SetNewMaxReloadTime(p_Unit: PUnitStruct;
  WeapIdx: Byte; ReloadTime: Word); stdcall;
begin
  Inc(WeapIdx, WEAPON_PRIMARY);
  TAUnit.CobStartScript( p_Unit, 'SetNewMaxReloadTime',
                         @WeapIdx, @ReloadTime, nil, nil,
                         False );
end;

procedure ScriptCallsExtend_SetNewMaxReloadTime;
asm
  add     edx, eax
  mov     [esi-7], dx
  movzx   eax, byte ptr [esp+18h]
  pushAD
  push    edx
  push    eax
  push    edi
  call    SetNewMaxReloadTime;
  popAD
  push $0049E4F2;
  call PatchNJump;
end;

procedure ConfirmedKill(p_Unit: PUnitStruct;
  DeathType: Cardinal); stdcall;
begin
  TAUnit.CobStartScript( p_Unit, 'ConfirmedKill',
                         @DeathType, nil, nil, nil,
                         True );
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

procedure VTOLLoadUnload(p_Unit: PUnitStruct;
  TransportedUnit: PUnitStruct; Piece: Cardinal; Loading: Cardinal); stdcall;
var
  UnitID: Word;
begin
  if (p_Unit <> nil) and
     (TransportedUnit <> nil) then
  begin
    UnitID := TAUnit.GetId(TransportedUnit);

    TAUnit.CobStartScript(p_Unit, 'ConfirmVTOLTransport',
                        @Loading, @Piece, @UnitID, nil,
                        True);
  end;
end;

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
    Result := TPluginData.Create( False,
                                  'ScriptCallsExtend Plugin',
                                  State_ScriptCallsExtend,
                                  @OnInstallScriptCallsExtend,
                                  @OnUninstallScriptCallsExtend );

    Result.MakeRelativeJmp( State_ScriptCallsExtend,
                            'ScriptCallsExtend_AimPrimaryTurret_GetUnit',
                            @ScriptCallsExtend_AimPrimaryTurret_GetUnit,
                            $0049E21E, 1 );

    Result.MakeRelativeJmp( State_ScriptCallsExtend,
                            'ScriptCallsExtend_AimPrimaryTurret_ExpCall',
                            @ScriptCallsExtend_AimPrimaryTurret_ExpCall,
                            $0049E2E8, 3 );

    Result.MakeRelativeJmp( State_ScriptCallsExtend,
                            'ScriptCallsExtend_AimPrimaryBallistic',
                            @ScriptCallsExtend_AimPrimaryBallistic,
                            $0049E35D, 0 );

    Result.MakeRelativeJmp( State_ScriptCallsExtend,
                            'Did weapon hit the target ?',
                            @ScriptCallsExtend_WeaponHitTest,
                            $00406F5B, 2 );
 {
    Result.MakeRelativeJmp( State_ScriptCallsExtend,
                            'Include damage value for HitByWeapon',
                            @ScriptCallsExtend_HitByWeaponExpand,
                            $00489F32, 0 );     }

    Result.MakeRelativeJmp( State_ScriptCallsExtend,
                            'Unit took damage call',
                            @ScriptCallsExtend_TookDamage,
                            $00489D3F, 1 );

    Result.MakeRelativeJmp( State_ScriptCallsExtend,
                            'Tell unit whats its new max weapon reload time',
                            @ScriptCallsExtend_SetNewMaxReloadTime,
                            $0049E4EC, 1 );

    Result.MakeRelativeJmp( State_ScriptCallsExtend,
                            'Confirmed unit kill',
                            @ScriptCallsExtend_ConfirmedKill,
                            $004864B6, 0 );

    Result.MakeRelativeJmp( State_ScriptCallsExtend,
                            'call air load',
                            @ScriptCallsExtend_CallAirLoadScript,
                            $004114A0, 1 );

    Result.MakeRelativeJmp( State_ScriptCallsExtend,
                            'call air unload',
                            @ScriptCallsExtend_CallAirUnLoadScript,
                            $00411799, 1 );
  end else
    Result := nil;
end;

end.