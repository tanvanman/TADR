unit WeaponAimNTrajectory;

interface
uses
  PluginEngine;

var
  MinWeap, MaxWeap: Cardinal;

// -----------------------------------------------------------------------------

const
  State_WeaponAimNTrajectory : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallWeaponAimNTrajectory;
Procedure OnUninstallWeaponAimNTrajectory;

// -----------------------------------------------------------------------------

procedure WeaponAimNTrajectory_HighTrajectory;
//procedure WeaponAimNTrajectory_WaterToGroundCheck_GrantFire;
//procedure WeaponAimNTrajectory_WaterToGroundCheck_Trajectory;
//procedure WeaponAimNTrajectory_NoAirWeapon;
//procedure WeaponAimNTrajectory_NoAirWeapon_Cursor;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations;

var
  WeaponAimNTrajectoryPlugin: TPluginData;

Procedure OnInstallWeaponAimNTrajectory;
begin
end;

Procedure OnUninstallWeaponAimNTrajectory;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_WeaponAimNTrajectory then
  begin
    WeaponAimNTrajectoryPlugin := TPluginData.create( false,
                            'WeaponAimNTrajectory',
                            State_WeaponAimNTrajectory,
                            @OnInstallWeaponAimNTrajectory,
                            @OnUnInstallWeaponAimNTrajectory );

    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'ShellWeapon Check',
                          @WeaponAimNTrajectory_HighTrajectory,
                          $0049AA50, 0);

{    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'WaterToGroundCheck 1',
                          @WeaponAimNTrajectory_WaterToGroundCheck_GrantFire,
                          $0049AB47, 0);

    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'WaterToGroundCheck 1',
                          @WeaponAimNTrajectory_WaterToGroundCheck_Trajectory,
                          $0049B9EB, 0);        }

// not finished, setting cursor wrkin                        
{    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'WeaponAimNTrajectory_NoAirWeapon',
                          @WeaponAimNTrajectory_NoAirWeapon,
                          $0049AD07, 0);

    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'WeaponAimNTrajectory_NoAirWeapon_Cursor',
                          @WeaponAimNTrajectory_NoAirWeapon_Cursor,
                          $0043E585, 0);      }

    Result:= WeaponAimNTrajectoryPlugin;
  end else
    Result := nil;
end;

function GetWeaponExtProperty(WeaponPtr : Pointer; PropertyType : Integer) : Integer; stdcall;
var
  WeapID : Integer;
begin
  Result := 0;
  if WeaponPtr <> nil then
  begin
    if IniSettings.WeaponType <= 256 then
      WeapID := PWeaponDef(WeaponPtr).ucID
    else
      WeapID := PWeaponDef(WeaponPtr).lWeaponIDCrack;
    if High(ExtraWeaponTags) >= WeapID then
    begin
      case PropertyType of
        1 : Result := ExtraWeaponTags[WeapID].HighTrajectory;
        2 : Result := ExtraWeaponTags[WeapID].WaterToGround;
        3 : Result := ExtraWeaponTags[WeapID].NoAirWeapon;
      end;
    end;
  end;
end;

procedure WeaponAimNTrajectory_HighTrajectory;
label
  HighTrajectoryOn,
  HighTrajectoryOff,
  Context_Aiming,
  Context_Firing,
  Context_Firing2,
  Context_FiringContinue,
  DontAllowShoot;
asm
  pushf
  push    eax
  mov     eax, [esp+$36]
  cmp     eax, $0049AD6C
  jz      Context_Firing2
  mov     eax, [esp+$36]
  cmp     eax, $0049D619
  jz      Context_Firing
  mov     eax, [esp+$36]
  cmp     eax, $0049E293
  jz      Context_Aiming
  mov     eax, [esp+$36]
  cmp     eax, $0049AB8C
  jz      Context_Aiming
Context_Firing:
  mov     ebx, [esp+$8E]
  jmp     Context_FiringContinue
Context_Firing2:
  mov     ebx, [esp-$46]
Context_FiringContinue :
  push    ecx
  push    edx
  push    1
  push    ebx
  call    GetWeaponExtProperty
  test    eax, 1
  jnz     HighTrajectoryOn
  jmp     HighTrajectoryOff
Context_Aiming:
  push    ecx
  push    edx
  push    1
  push    ebx
  call    GetWeaponExtProperty
  test    eax, 1
  jnz     HighTrajectoryOn
HighTrajectoryOff:
  pop     edx
  pop     ecx
  pop     eax
  popf
  test    ah, 41h
  jz      DontAllowShoot
  // allow shooting
  push $0049AA55;
  call PatchNJump;
HighTrajectoryOn:
  pop     edx
  pop     ecx
  pop     eax
  popf
  test    ah, 41h
  jnz     DontAllowShoot
  // allow shooting
  push $0049AA55;
  call PatchNJump;
DontAllowShoot:
  push $0049AA6C;
  call PatchNJump;
end;

{
.text:0049AB47 020 8A 95 7F 42 01 00                                               mov     dl, [ebp+TAMainStruct.TNTMemStruct.SeaLevel]  1427Fh
.text:0049AB4D 020 3B CA                                                           cmp     ecx, edx
.text:0049AB4F 020 7F 0C                                                           jg      short loc_49AB5D
.text:0049AB51 020 33 C0                                                           xor     eax, eax
}
{
procedure WeaponAimNTrajectory_WaterToGroundCheck_GrantFire;
label
  Result0,
  CalculateTrajectory,
  CalculateTrajectoryWaterToGround;
asm
    push    edx
    push    ecx
    push    eax
    push    3
    push    ebx
    call    GetWeaponExtProperty
    test    eax, 1
    // avoid (sea level + unit height) check, allowing weap to be fired for water level
    // if watertoground = 1
    jnz     CalculateTrajectoryWaterToGround
    pop     eax
    pop     ecx
    pop     edx
    mov     dl, [ebp+1427Fh] //TAMainStruct.TNTMemStruct.SeaLevel
    cmp     ecx, edx
    jg      CalculateTrajectory
Result0 :
    push $0049AB51;
    call PatchNJump
CalculateTrajectory :
    push $0049AB5D;
    call PatchNJump
CalculateTrajectoryWaterToGround :
    pop     eax
    pop     ecx
    pop     edx
    push $0049AB5D;
    call PatchNJump
end;
}
{
.text:0049B9EB 024 A9 00 00 01 00                                                  test    eax, 10000h
.text:0049B9F0 024 74 24                                                           jz      short loc_49BA16
.text:0049B9F2 024 66 0F B6 82 7F 42 01 00                                         movzx   ax, [edx+TAMainStruct.TNTMemStruct.SeaLevel]
}
// waterweapon + watertoground will get standard trajectory when projectile
// is above the water level, instead of waterweapon trajectory which cause
// projectile to pick random target position
{procedure WeaponAimNTrajectory_WaterToGroundCheck_Trajectory;
label
  loc_49BA16,
  loc_49BA16_WaterGround;
asm
    push    edx
    push    ecx
    push    eax
    push    3
    push    esi
    call    GetWeaponExtProperty
    test    eax, 1
    // avoid (sea level + unit height) check, allowing weap to be fired for water level
    // if watertoground = 1
    jnz     loc_49BA16_WaterGround
    pop     eax
    pop     ecx
    pop     edx
    test    eax, 10000h
    jz      loc_49BA16
    push $0049B9F2;
    call PatchNJump
loc_49BA16 :
    push $0049BA16;
    call PatchNJump
loc_49BA16_WaterGround :
    pop     eax
    pop     ecx
    pop     edx
    push $0049BA16;
    call PatchNJump
end;        }

{
.text:0049AD07 01C A9 00 00 02 00                                                  test    eax, 20000h
.text:0049AD0C 01C 74 1A                                                           jz      short loc_49AD28
.text:0049AD0E 01C 8B 8B 10 01 00 00                                               mov     ecx, [ebx+110h]
.text:0049AD14 01C 83 E1 03                                                        and     ecx, 3
.text:0049AD17 01C 80 F9 02                                                        cmp     cl, 2
.text:0049AD1A 01C 74 0C                                                           jz      short loc_49AD28
.text:0049AD1C 01C 33 C0                                                           xor     eax, eax
.text:0049AD1E 01C 5F                                                              pop     edi
.text:0049AD1F 018 5E                                                              pop     esi
.text:0049AD20 014 5D                                                              pop     ebp
.text:0049AD21 010 5B                                                              pop     ebx
.text:0049AD22 00C 83 C4 0C                                                        add     esp, 0Ch
.text:0049AD25 000 C2 0C 00                                                        retn    0Ch
}
{
procedure WeaponAimNTrajectory_NoAirWeapon;
label
  ToAirWeapon,
  BallisticCheck,
  NoAirWeaponCheckUnit,
  DontShoot;
asm
    push    edx
    push    ecx
    push    eax
    push    2
    push    edi
    call    GetWeaponExtProperty
    test    eax, 1
    jz      NoAirWeaponCheckUnit
    pop     eax
    pop     ecx
    pop     edx
    jmp     ToAirWeapon
NoAirWeaponCheckUnit :
    pop     eax
    pop     ecx
    pop     edx
    mov     ecx, [ebx+110h]
    and     ecx, 3
    cmp     cl, 2
    jnz     DontShoot  // if unit is ground
ToAirWeapon :
    test    eax, 20000h
    jz      BallisticCheck   //loc_49AD28
    push $0049AD0E
    call PatchNJump
DontShoot :
    push $0049AD1C
    call PatchNJump
BallisticCheck :
    push $0049AD28
    call PatchNJump
end;
}
{
.text:0043E578 014 8B 44 24 1C                                                     mov     eax, [esp+14h+a1]
.text:0043E57C 014 8B 08                                                           mov     ecx, [eax]
.text:0043E57E 014 8B 70 10                                                        mov     esi, [eax+10h]
.text:0043E581 014 85 C9                                                           test    ecx, ecx
.text:0043E583 014 74 0D                                                           jz      short loc_43E592
.text:0043E585 014 B8 01 00 00 00                                                  mov     eax, 1
.text:0043E58A 014 5F                                                              pop     edi
}

// weapon in esi

{
.text:0043E5CF 014 F7 86 11 01 00 00 00 00 02 00                                   test    dword ptr [esi+111h], 20000h ; toairweapon cursor check
.text:0043E5D9 014 E9 B4 02 00 00                                                  jmp     loc_43E892
}
{
procedure WeaponAimNTrajectory_NoAirWeapon_Cursor;
label
  Result1,
  NoAirWeaponCheckUnit,
  SetResult;
asm
    push    edx
    push    ecx
    push    eax
    push    2
    push    esi
    call    GetWeaponExtProperty
    test    eax, 1
    jnz     NoAirWeaponCheckUnit
    jmp     Result1
NoAirWeaponCheckUnit :
    test    ebp, 1
    jz      Result1               // are we shooting ground ?
    mov     ecx, [edi+110h]       // unitptr.unit type mask
    and     ecx, 3
    cmp     cl, 2
    jnz     Result1               // unit is air or not
    test    ecx, ecx              // set ZF
    jmp     SetResult
Result1 :
    pop     eax
    pop     ecx
    pop     edx
    mov     eax, 1
    push $0043E58A
    call PatchNJump
SetResult :
    pop     eax
    pop     ecx
    pop     edx
    push $0043E892                // it will set then cursor depending on ZF
    call PatchNJump
end;          }

end.

