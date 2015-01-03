unit WeaponAimNTrajectory;

interface
uses
  PluginEngine, SysUtils,
  TA_MemoryStructures;

// -----------------------------------------------------------------------------

const
  State_WeaponAimNTrajectory : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallWeaponAimNTrajectory;
Procedure OnUninstallWeaponAimNTrajectory;

// -----------------------------------------------------------------------------

procedure WeaponAimNTrajectory_HighTrajectory;
procedure WeaponAimNTrajectory_PreserveAccuracy;
procedure WeaponAimNTrajectory_WeaponType2;
procedure WeaponAimNTrajectory_NoAirWeapon;
procedure WeaponAimNTrajectory_NoAirWeapon_Cursor;
function WeaponAimNTrajectory_SearchForEnemyNukes(UnitStruct: PUnitStruct;
  WeapStructIndex: Byte): PWeaponProjectile; stdcall;

//procedure WeaponAimNTrajectory_SecondPhaseSpray;
//procedure WeaponAimNTrajectory_WaterToGroundCheck_GrantFire;
//procedure WeaponAimNTrajectory_WaterToGroundCheck_Trajectory;


implementation
uses
  IniOptions,
  Windows,
  Math,
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_FunctionsU, logging;

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
                          'High Trajectory',
                          @WeaponAimNTrajectory_HighTrajectory,
                          $0049AA50, 0);

    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'Preserve accuracy of weapon, to be independet of unit kills counter',
                          @WeaponAimNTrajectory_PreserveAccuracy,
                          $0049D702, 0);

    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'twophase weapontype2',
                          @WeaponAimNTrajectory_WeaponType2,
                          $0049BB29, 1);

    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'Is target unit air',
                          @WeaponAimNTrajectory_NoAirWeapon,
                          $0049AD07, 0);

    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'WeaponAimNTrajectory_NoAirWeapon_Cursor',
                          @WeaponAimNTrajectory_NoAirWeapon_Cursor,
                          $0043E578, 0);

    if IniSettings.InterceptsOnlyList then
    begin
      WeaponAimNTrajectoryPlugin.MakeStaticCall( State_WeaponAimNTrajectory,
                            'Overwrite nukes search function call #1',
                            @WeaponAimNTrajectory_SearchForEnemyNukes,
                            $0049DC2F);
      WeaponAimNTrajectoryPlugin.MakeStaticCall( State_WeaponAimNTrajectory,
                            'Overwrite nukes search function call #2',
                            @WeaponAimNTrajectory_SearchForEnemyNukes,
                            $00408B48);
      WeaponAimNTrajectoryPlugin.MakeStaticCall( State_WeaponAimNTrajectory,
                            'Overwrite nukes search function call #3',
                            @WeaponAimNTrajectory_SearchForEnemyNukes,
                            $00408945);
    end;
{
    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'WeaponAimNTrajectory_InterceptorTest',
                          @WeaponAimNTrajectory_InterceptorTest,
                          $0049D16A, 1);
}
{
    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'Sprayangle for twophase weapons',
                          @WeaponAimNTrajectory_SecondPhaseSpray,
                          $0049BAF9, 0);
}
{    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'WaterToGroundCheck 1',
                          @WeaponAimNTrajectory_WaterToGroundCheck_GrantFire,
                          $0049AB47, 0);

    WeaponAimNTrajectoryPlugin.MakeRelativeJmp( State_WeaponAimNTrajectory,
                          'WaterToGroundCheck 1',
                          @WeaponAimNTrajectory_WaterToGroundCheck_Trajectory,
                          $0049B9EB, 0);        }

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
    WeapID := TAWeapon.GetWeaponID(WeaponPtr);
    if High(ExtraWeaponDefTags) >= WeapID then
    begin
      case PropertyType of
        1 : Result := ExtraWeaponDefTags[WeapID].HighTrajectory;
        2 : Result := ExtraWeaponDefTags[WeapID].PreserveAccuracy;
        3 : Result := ExtraWeaponDefTags[WeapID].NotAirWeapon;
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
  // eax for testing return address
  // ebx for weapon ptr
  pushf
  push    eax
  push    ebx
  mov     eax, [esp+$3A]
  cmp     eax, $0049AD6C
  jz      Context_Firing2
  mov     eax, [esp+$3A]
  cmp     eax, $0049D619
  jz      Context_Firing
  mov     eax, [esp+$3A]
  cmp     eax, $0049E293
  jz      Context_Aiming
  mov     eax, [esp+$3A]
  cmp     eax, $0049AB8C
  jz      Context_Aiming
Context_Firing:
  mov     ebx, [esp+$92]
  jmp     Context_FiringContinue
Context_Firing2:
  mov     ebx, edi
Context_FiringContinue :
  push    ecx
  push    edx
  push    1
  push    ebx
  call    GetWeaponExtProperty
  test    eax, eax
  jnz     HighTrajectoryOn
  jmp     HighTrajectoryOff
Context_Aiming:
  push    ecx
  push    edx
  push    1
  push    ebx
  call    GetWeaponExtProperty
  test    eax, eax
  jnz     HighTrajectoryOn
HighTrajectoryOff:
  pop     edx
  pop     ecx
  pop     ebx
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
  pop     ebx
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
.text:0049D702 054 83 FB 01        cmp     ebx, 1
.text:0049D705 054 7E 0C           jle     short loc_49D713 ; if kills / 12 > 1
.text:0049D707 054 8B C1           mov     eax, ecx
}
procedure WeaponAimNTrajectory_PreserveAccuracy;
label
  PreserveAcc,
  DontApplyKillsAdv;
asm
  push    ecx
  push    edx
  push    2
  mov     ecx, [esi+TUnitWeapon.p_Weapon]
  push    ecx
  call    GetWeaponExtProperty
  test    eax, eax
  jnz     PreserveAcc
  pop     edx
  pop     ecx
  cmp     ebx, 1
  jle     DontApplyKillsAdv
  push $0049D707;
  call PatchNJump;
PreserveAcc :
  pop     edx
  pop     ecx
DontApplyKillsAdv :
  push $0049D713;
  call PatchNJump;
end;

procedure TwoPhaseExtension(WeaponPtr: Pointer; Projectile: Pointer); stdcall;
var
  CurrentProjectile : PWeaponProjectile;
  WeaponID : Integer;
  WeaponType2 : String;
begin
  if WeaponPtr <> nil then
  begin
    WeaponID := TAWeapon.GetWeaponID(WeaponPtr);
    WeaponType2 := ExtraWeaponDefTags[WeaponID].WeaponType2;
    if WeaponType2 <> '' then
    begin
      CurrentProjectile := PWeaponProjectile(Projectile);
      CurrentProjectile.Weapon := WEAPONS_Name2Ptr(PAnsiChar(WeaponType2));
    end;
  end;
end;

{
.text:0049BB32 024 75 06                                                           jnz     short loc_49BB3A
.text:0049BB34 024 89 7D 56                                                        mov     [ebp+56h], edi
.text:0049BB37 024 89 7D 4E                                                        mov     [ebp+4Eh], edi
.text:0049BB3A
.text:0049BB3A                                                     loc_49BB3A:
}
procedure WeaponAimNTrajectory_WeaponType2;
asm
  pushAD
  push    ebp
  push    ecx
  call TwoPhaseExtension
  popAD
  mov     eax, [ecx+111h]
  push $0049BB2F
  call PatchNJump
end;

{
procedure TwoPhaseExtension(WeaponPtr: Pointer; Projectile: Pointer); stdcall;
var
  NewAngle: Extended;
  SprayAngle, SprayAngleOrg : Word;
  i, SprayRate : Integer;
  //Angle : Single;
  Angle : Word;
  //fAngle : Extended;
  Atany, Atanx : Integer;
  NewTarget_x, NewTarget_z : Integer;
  Distance, NewDistance : Extended;

  HypotX, HypotZ : Extended;
  MapWidth : Integer;
  MapHeight : Integer;

  AttackerUnit : PUnitStruct;
  VictimUnit : PUnitStruct;
  CurrentProjectile : PWeaponProjectile;
  NewPosition : TPosition;
  NewUnitWeapon : TUnitWeapon;
begin
  //v10 := PWeaponProjectile(Projectile).XTurn - (SprayAngle shr 1) + rand_(SprayAngle);
  //v11 := sub_4B7123(PWeaponProjectile(Projectile).ZTurn, PWeaponDef(WeaponPtr).lWeaponVelocity);
  SprayAngleOrg := 2000;//GetWeaponExtProperty(WeaponPtr, 4);
  SprayRate := 3;//GetWeaponExtProperty(WeaponPtr, 5);
  CurrentProjectile := PWeaponProjectile(Projectile);
  // angle
  SprayAngle := SprayAngleOrg;
  Atany := CurrentProjectile.Position_Start.z - CurrentProjectile.Position_Target2.z;
  Atanx := CurrentProjectile.Position_Start.x - CurrentProjectile.Position_Target2.x;
  Angle := Word(TA_Atan2(Atany, Atanx));
  NewAngle := Angle - (SprayAngle div 2) + rand_(SprayAngle);

  //NewAngle := (SprayAngle / High(Word) * 360);
  //fAngle := (ArcTan2(AtanY, AtanX) * 180 / pi) - (NewAngle / 2) + rand_(Round(NewAngle));

  // distance
  HypotX := CurrentProjectile.Position_Start.x - CurrentProjectile.Position_Target2.x;
  HypotZ := CurrentProjectile.Position_Start.z - CurrentProjectile.Position_Target2.z;
  Distance := Hypot(HypotX, HypotZ);
  //SprayAngle := SprayAngleOrg div 64;
  //Randomize;
  //NewDistance := Round(Distance - (SprayAngle div 2) + rand_(SprayAngle));

  //NewAngle := NewAngle / High(Word) * 360;
  //NewAngle := Angle / High(Word) * 360;
  //Distance := DistanceOrg;

  MapWidth := TAData.MainStruct.MapWidth;
  MapHeight := TAData.MainStruct.MapHeight;
  AttackerUnit := CurrentProjectile.pAttackerUnit;

  for i := 1 to SprayRate do
  begin
    SprayAngle := SprayAngleOrg div 16;
    NewTarget_x := CurrentProjectile.Position_Target2.x - (SprayAngle div 2) + rand_(SprayAngle);
    SprayAngle := SprayAngleOrg div 16;
    NewTarget_z := CurrentProjectile.Position_Target2.z - (SprayAngle div 2) + rand_(SprayAngle);

    if (NewTarget_x > 0) and (NewTarget_z > 0) and (NewTarget_x < MapWidth) and (NewTarget_z < MapHeight) then
    begin
      NewPosition := CurrentProjectile.Position_Target2;
      NewPosition.x := NewTarget_x;
      NewPosition.z := NewTarget_z;

      NewUnitWeapon := AttackerUnit.UnitWeapons[0];
      NewUnitWeapon.p_Weapon := WeaponName2Ptr(PAnsiChar('ARMTRUCK_MISSILE'));

      CurrentProjectile.Weapon := WeaponName2Ptr(PAnsiChar('CRBLMSSL'));

      CreateProjectile_0(@NewUnitWeapon,
                         AttackerUnit,
                         @CurrentProjectile.Position_Curnt,
                         @NewPosition,
                         CurrentProjectile.p_TargetUnit);

      if CreateProjectile_1(@NewUnitWeapon,
                            AttackerUnit,
                            @CurrentProjectile.Position_Curnt,
                            @NewPosition,
                            CurrentProjectile.p_TargetUnit,
                            0) then
    end;
  end;
end;
}
{
.text:0049BB32 024 75 06                                                           jnz     short loc_49BB3A
.text:0049BB34 024 89 7D 56                                                        mov     [ebp+56h], edi
.text:0049BB37 024 89 7D 4E                                                        mov     [ebp+4Eh], edi
.text:0049BB3A
.text:0049BB3A                                                     loc_49BB3A:
}
{
procedure WeaponAimNTrajectory_SecondPhaseSpray;
label
//  loc_49BB3A,
  ApplySprayAngle;
asm
  push    edx
  push    ecx
  push    eax
  push    4                      //twophase sprayangle
  push    ecx
  call    GetWeaponExtProperty
  cmp     eax, 0
  //jnz     ApplySprayAngle
  jmp     ApplySprayAngle
  pop     eax
  pop     ecx
  pop     edx
  mov     ebx, [TADynmemStructPtr]
  push $0049BAFF
  call PatchNJump
ApplySprayAngle :
  pop     eax
  pop     ecx
  pop     edx
  pushAD
  push    ebp
  push    ecx
  call TwoPhaseExtension
  popAD
  mov     ebx, [TADynmemStructPtr]
  push $0049BAFF
  call PatchNJump
end;
}
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
    push    3
    push    edi
    call    GetWeaponExtProperty
    test    eax, eax
    jnz     NoAirWeaponCheckUnit
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
    jz      DontShoot  // if unit is ground
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

{
.text:0043E578 014 8B 44 24 1C        mov     eax, [esp+14h+AttackerUnit]
.text:0043E57C 014 8B 08              mov     ecx, [eax]
.text:0043E57E 014 8B 70 10           mov     esi, [eax+10h]
.text:0043E581 014 85 C9              test    ecx, ecx
.text:0043E583 014 74 0D              jz      short loc_43E592 // unit dont have move class
}
procedure WeaponAimNTrajectory_NoAirWeapon_Cursor;
label
  StandardWeaponContinue,
  NoAirWeaponCheckUnit,
  DontShoot,
  DontShoot_NoRange;
asm
    mov     eax, [esp+14h+$8] // attacker unit
    mov     ecx, [eax]
    mov     esi, [eax+10h]    // weapon
    push    edx
    push    ecx
    push    eax
    push    3
    push    esi
    call    GetWeaponExtProperty
    test    eax, eax
    jz      StandardWeaponContinue
    test    edi, edi                // if unit at mouse
    jz      StandardWeaponContinue
NoAirWeaponCheckUnit :
    pop     eax
    pop     ecx
    pop     edx
    push    0                           // weap idx
    mov     eax, [esp+18h+$10]
    push    eax                         // pos trgt
    mov     eax, [esp+1Ch+8]
    lea     ecx, [eax+6Ah]
    push    ecx                         // pos start
    push    eax                         // attacker ptr
    call    Trajectory3
    test    eax, eax
    jz      DontShoot_NoRange
    push    0                           // weap idx
    mov     edx, [esp+18h+8]
    push    edi                         // Targetp_Unit
    push    edx                         // Attackerp_Unit
    call    UnitAutoAim_CheckUnitWeapon
    test    eax, eax
    jz      DontShoot
    mov     ecx, [edi+110h]
    and     ecx, 3
    cmp     cl, 2
    jz      DontShoot
    //mov     eax, 1
    push $0043E585
    //push $0043E592;
    call PatchNJump
DontShoot :
    mov     eax, 19
    push $0043E58A
    call PatchNJump
DontShoot_NoRange :
    mov     eax, 3
    push $0043E58A
    call PatchNJump
StandardWeaponContinue :
    pop     eax
    pop     ecx
    pop     edx
    push $0043E581
    call PatchNJump
end;

function WeaponAimNTrajectory_SearchForEnemyNukes(UnitStruct: PUnitStruct;
  WeapStructIndex: Byte): PWeaponProjectile; stdcall;
var
  CurProjectileIdx : Integer;
  ProjectilesCount : Integer;
  InterceptorCount : Integer;
  CurInterceptor : Pointer;
  Projectile, FirstProjectile : PWeaponProjectile;
  Coverage : Integer;
  WeapIdx : Cardinal;
  bAllowShoot : Boolean;
  i : Integer;
label
  AntiNukeNotInStock;
begin
  CurProjectileIdx := 0;
  ProjectilesCount := TAData.MainStruct.lNumProjectiles;
  FirstProjectile := TAData.MainStruct.p_Projectiles;
  Projectile := TAData.MainStruct.p_Projectiles;
  Coverage := UnitStruct.UnitWeapons[WeapStructIndex].p_Weapon.lCoverage shl 16;

  if ( UnitStruct.UnitWeapons[WeapStructIndex].cStock <> 0 ) and
     ( ProjectilesCount > 0 ) then
  begin
    while ( True ) do
    begin
      if ( Projectile.cOwnerID <> UnitStruct.ucOwnerID ) then
        if ( (Projectile.Weapon.lWeaponTypeMask shr 29) and 1 = 1 ) then
        begin
          if ( PInteger(@UnitStruct.Position.x_)^ - PInteger(@Projectile.Position_Target2.x_)^ + Coverage <= (2 * Coverage) ) and
             ( PInteger(@UnitStruct.Position.z_)^ - PInteger(@Projectile.Position_Target2.z_)^ + Coverage <= (2 * Coverage) ) then
          begin
            // if tag is empty = intercept all
            bAllowShoot := True;
            WeapIdx := TAWeapon.GetWeaponID(UnitStruct.UnitWeapons[WeapStructIndex].p_Weapon);
            if ExtraWeaponDefTags[WeapIdx].Intercepts <> nil then
            begin
              bAllowShoot := False;
              for i := 0 to ExtraWeaponDefTags[WeapIdx].Intercepts.Count - 1 do
              begin
                if ExtraWeaponDefTags[WeapIdx].Intercepts[i] = String(PWeaponDef(Projectile.Weapon).szWeaponName) then
                begin
                  bAllowShoot := True;
                  Break;
                end;
              end;
            end;
            if bAllowShoot then
            begin
              InterceptorCount := 0;
              if ( ProjectilesCount > 0 ) then
              begin
                CurInterceptor := @FirstProjectile.lInterceptor;
                repeat
                  if ( Pointer(CurInterceptor^) = Projectile ) then
                    Break;
                  Inc(InterceptorCount);
                  CurInterceptor := Pointer(Cardinal(CurInterceptor) + SizeOf(TWeaponProjectile));
                until ( InterceptorCount >= ProjectilesCount );
              end;
              if ( InterceptorCount = ProjectilesCount ) then
                Break;
            end;
          end;
        end;
      Inc(CurProjectileIdx);
      Projectile := Pointer(Cardinal(Projectile) + SizeOf(TWeaponProjectile));
      if ( CurProjectileIdx >= ProjectilesCount ) then
        goto AntiNukeNotInStock;
    end;
  end else
  begin
AntiNukeNotInStock:
    Projectile := nil;
  end;
  Result := Projectile;
end;

end.