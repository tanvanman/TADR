unit WeaponAimNTrajectory;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_WeaponAimNTrajectory: Boolean = True;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallWeaponAimNTrajectory;
Procedure OnUninstallWeaponAimNTrajectory;

// -----------------------------------------------------------------------------

implementation
uses
  IniOptions,
  TA_MemoryStructures,
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_FunctionsU;

function GetWeaponExtProperty(WeaponPtr: PWeaponDef;
  PropertyType: Integer): Integer; stdcall;
var
  WeapID: Integer;
begin
  Result := 0;
  if WeaponPtr <> nil then
  begin
    WeapID := TAWeapon.GetWeaponID(WeaponPtr);
    if High(ExtraWeaponDefTags) >= WeapID then
    begin
      case PropertyType of
        1 : if ExtraWeaponDefTags[WeapID].HighTrajectory then Result := 1;
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
  Coverage := UnitStruct.UnitWeapons[WeapStructIndex].p_Weapon.lCoverage;

  if ( UnitStruct.UnitWeapons[WeapStructIndex].cStock <> 0 ) and
     ( ProjectilesCount > 0 ) then
  begin
    while ( True ) do
    begin
      if ( Projectile.cOwnerID <> UnitStruct.ucOwnerID ) then
        if ( (Projectile.p_Weapon.lWeaponTypeMask shr 29) and 1 = 1 ) then
        begin
          if TAMem.DistanceBetweenPosCompare(@UnitStruct.Position, @Projectile.Position_Target2, Coverage) then
          begin
            // if tag is empty = intercept all
            bAllowShoot := True;
            WeapIdx := TAWeapon.GetWeaponID(UnitStruct.UnitWeapons[WeapStructIndex].p_Weapon);
            if ExtraWeaponDefTags[WeapIdx].Intercepts <> nil then
            begin
              bAllowShoot := False;
              for i := 0 to ExtraWeaponDefTags[WeapIdx].Intercepts.Count - 1 do
              begin
                if ExtraWeaponDefTags[WeapIdx].Intercepts[i] = String(PWeaponDef(Projectile.p_Weapon).szWeaponName) then
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
    Result := TPluginData.Create( False, 'WeaponAimNTrajectory',
                                  State_WeaponAimNTrajectory,
                                  @OnInstallWeaponAimNTrajectory,
                                  @OnUnInstallWeaponAimNTrajectory );

    Result.MakeRelativeJmp( State_WeaponAimNTrajectory,
                            'High Trajectory',
                            @WeaponAimNTrajectory_HighTrajectory,
                            $0049AA50, 0 );

    Result.MakeRelativeJmp( State_WeaponAimNTrajectory,
                            'Preserve accuracy of weapon, to be independet of unit kills counter',
                            @WeaponAimNTrajectory_PreserveAccuracy,
                            $0049D702, 0 );

    Result.MakeRelativeJmp( State_WeaponAimNTrajectory,
                            'Is target unit air',
                            @WeaponAimNTrajectory_NoAirWeapon,
                            $0049AD07, 0 );

    Result.MakeRelativeJmp( State_WeaponAimNTrajectory,
                            'WeaponAimNTrajectory_NoAirWeapon_Cursor',
                            @WeaponAimNTrajectory_NoAirWeapon_Cursor,
                            $0043E578, 0 );

    if IniSettings.InterceptsOnlyList then
    begin
      Result.MakeStaticCall( State_WeaponAimNTrajectory,
                             'Overwrite nukes search function call #1',
                             @WeaponAimNTrajectory_SearchForEnemyNukes,
                             $0049DC2F );
      Result.MakeStaticCall( State_WeaponAimNTrajectory,
                             'Overwrite nukes search function call #2',
                             @WeaponAimNTrajectory_SearchForEnemyNukes,
                             $00408B48 );
      Result.MakeStaticCall( State_WeaponAimNTrajectory,
                             'Overwrite nukes search function call #3',
                             @WeaponAimNTrajectory_SearchForEnemyNukes,
                             $00408945 );
    end;
  end else
    Result := nil;
end;


end.