unit WeaponsExpand;

interface
uses
  PluginEngine, TA_MemoryStructures;

// -----------------------------------------------------------------------------

const
  State_WeaponsExpand: Boolean = True;

function GetPlugin: TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallWeaponsExpand;
Procedure OnUninstallWeaponsExpand;

// -----------------------------------------------------------------------------

function GetMaxWeapons: Cardinal;

var
  WeaponsPatchMainStruct: TWeaponsPatchMainStruct;
  p_WeaponsPatchMainStruct: Pointer;

implementation
uses
  Windows,
  SysUtils,
  IniOptions,
  Classes,
  TA_MemoryConstants,
  TA_FunctionsU,
  TA_MemoryLocations,
  TA_NetworkingMessages,
  TA_MemPlayers,
  TA_MemUnits,
  logging;

function GetMaxWeapons: Cardinal;
begin
  if IniSettings.WeaponsIDPatch then
    Result := MAX_WEAPONS_PATCHED
  else
    Result := MAX_WEAPONS;
end;

procedure WeaponsExpand_NewPropertiesLoad(TDFHandle: Cardinal; WeaponID: Cardinal); stdcall;
var
  Intercepts: array[0..1023] of AnsiChar;
begin
  ExtraWeaponDefTags[WeaponID].HighTrajectory := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('hightrajectory')) <> 0;
  ExtraWeaponDefTags[WeaponID].MaxBarrelAngle := TdfFile_GetFloat(0, 0, TDFHandle, 0, PAnsiChar('maxbarrelangle'));
  ExtraWeaponDefTags[WeaponID].PreserveAccuracy := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('preserveaccuracy'));
  ExtraWeaponDefTags[WeaponID].NotAirWeapon := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('notairweapon'));
  TdfFile_GetStr(0, 0, TDFHandle, Pointer(Null_str), $40, PAnsiChar('weapontype2'), @ExtraWeaponDefTags[WeaponID].WeaponType2);
  if TdfFile_GetStr(0, 0, TDFHandle, Pointer(Null_str), $400, PAnsiChar('intercepts'), @Intercepts) <> 0 then
  begin
    ExtraWeaponDefTags[WeaponID].Intercepts := TStringlist.Create;
    ExtraWeaponDefTags[WeaponID].Intercepts.DelimitedText := Intercepts;
  end;
end;

procedure WeaponsExpand_NewPropertiesLoadHook;
asm
  pushAD
  push    eax
  push    ecx
  call    WeaponsExpand_NewPropertiesLoad
  popAD
  lea     ecx, [eax+eax*2]
  shl     ecx, 3
  push $0042E474
  call PatchNJump;
end;

function WeaponName2WeaponTypeDef(const p_WeaponName: PAnsiChar): PWeaponDef; stdcall;
var
  i: Integer;
begin
  Result := nil;
  if p_WeaponName <> nil then
  begin
    if StrLen(p_WeaponName) <> 0 then
    begin
      for I := Low(WeaponsPatchMainStruct.Weapons) to High(WeaponsPatchMainStruct.Weapons) do
      begin
        if _strcmpi(@WeaponsPatchMainStruct.Weapons[I], p_WeaponName) = 0 then
        begin
          Result := @WeaponsPatchMainStruct.Weapons[I];
          Exit;
        end;
      end;
    end;
  end;
end;

procedure BroadcastFeatureActionPacket(WeaponID: Cardinal;
  X, Y: Word); stdcall;
var
  Buffer: TFeatureActionMessagePatched;
begin
  Buffer.Marker := TANM_FeatureAction;
  Buffer.WeaponID := WeaponID;
  Buffer.X := X;
  Buffer.Y := Y;
  HAPI_BroadcastMessage(TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID).lDirectPlayID, @Buffer, SizeOf(Buffer));
end;

procedure FEATURES_TakeWeaponDamagePatched(p_PlotGrid: PPlotGrid;
  X, Y: Word; p_Weapon: PWeaponDef); stdcall;
label
  TestBroadcast,
  TestHasObjectForNonFlamable;
var
  p_FeatureDef: PFeatureDefStruct;
  BroadcastMessage: Boolean;
  Buffer: TFeatureActionMessagePatched;
  p_AnimFeature: PFeatureAnimData;
  nDamage: Word;
begin
  if (TAData.SwitchesMask and SwitchesMasks[SwitchesMask_TreeDeath]) = 0 then Exit;
  if p_PlotGrid.nFeatureDefIndex >= 65531 then Exit;
  FillChar(Buffer, SizeOf(TFeatureActionMessagePatched), 0);
  p_FeatureDef := TAMem.FeatureDefId2Ptr(p_PlotGrid.nFeatureDefIndex);
  if (p_FeatureDef.nMask and FeatureMaskArr[fiIndestructible]) <> 0 then
    Exit;
  if TAData.GameingType = gtMultiplayer then
  begin
    if PlayerState_Watcher in TAPlayer.GetPlayerByIndex(TAData.ViewPlayerID).PlayerInfo.SharedBits then
    begin
      Buffer.Marker := TANM_FeatureAction;
      Buffer.WeaponID := p_Weapon.lWeaponIDCrack;
      Buffer.X := X;
      Buffer.Y := Y;
      HAPI_SendMessage(GetLocalPlayerDPID, GetSharedPlayerDPID,
        @Buffer, SizeOf(TFeatureActionMessagePatched));
      Exit;
    end;
    BroadcastMessage := True;
    Buffer.WeaponID := MAX_WEAPONS_PATCHED-4;
  end else
    BroadcastMessage := False;

  if ((p_FeatureDef.nMask and FeatureMaskArr[fiFlamable]) <> 0) and
     (p_Weapon.ucFireStarter <> 0) then
  begin
    if (p_PlotGrid.bYard_type and 1) = 0 then
    begin
      FEATURES_StartBurn(X, Y, False);
      goto TestBroadcast;
    end;
TestHasObjectForNonFlamable:    
    if (p_FeatureDef.nMask and FeatureMaskArr[fiHasObject]) = 0 then
    begin
      p_AnimFeature := TAMem.FeatureAnimId2Ptr(p_PlotGrid.nFeatureAnimIndex);
      if (p_AnimFeature.GridX <> X) or
         (p_AnimFeature.GridY <> Y) then Exit;
      if (p_AnimFeature.Damage + p_Weapon.nDefaultDamage) <= High(Word) then
        p_AnimFeature.Damage := p_AnimFeature.Damage + p_Weapon.nDefaultDamage
      else
        p_AnimFeature.Damage := High(Word);
      if (p_AnimFeature.Damage + p_Weapon.nDefaultDamage) >= p_FeatureDef.nDamage then
      begin
        FEATURES_Destroy_3D(p_AnimFeature.GridX, p_AnimFeature.GridY, False);
        Buffer.WeaponID := MAX_WEAPONS_PATCHED-3;
      end;
    end;
    goto TestBroadcast;
  end;

  if (p_PlotGrid.bYard_type and 1) <> 0 then
    goto TestHasObjectForNonFlamable;
  nDamage := p_PlotGrid.nFeatureAnimIndex + p_Weapon.nDefaultDamage;
  if ( nDamage < p_FeatureDef.nDamage ) then
    p_PlotGrid.nFeatureAnimIndex := nDamage
  else
  begin
    FEATURES_Destroy_3D(X, Y, False);
    Buffer.WeaponID := MAX_WEAPONS_PATCHED-3;
  end;
TestBroadcast:
  if BroadcastMessage then
  begin
    if ( Buffer.WeaponID > MAX_WEAPONS_PATCHED-4 ) then
      BroadcastFeatureActionPacket(Buffer.WeaponID, X, Y);
  end;
end;

procedure ReceiveFeatureActionPacket(p_Packet: PFeatureActionMessagePatched); stdcall;
var
  p_PlotGrid: PPlotGrid;
begin
  SendTextLocal(Format('Received: X: %d, Y: %d', [p_Packet.X, p_Packet.Y]));
  case p_Packet.WeaponID of
    MAX_WEAPONS_PATCHED-3 : FEATURES_Destroy_3D(p_Packet.X, p_Packet.Y, False);
    MAX_WEAPONS_PATCHED-2 : FEATURES_StartBurn(p_Packet.X, p_Packet.Y, True);
    MAX_WEAPONS_PATCHED-1 : FEATURES_Destroy_3D(p_Packet.X, p_Packet.Y, True);
    else
      begin
        p_PlotGrid := GetGridPosPLOT(p_Packet.X, p_Packet.Y);
        FEATURES_TakeWeaponDamagePatched(p_PlotGrid,
          p_Packet.X, p_Packet.Y, TAWeapon.WeaponId2Ptr(p_Packet.WeaponID));
      end;
  end;
end;

procedure ReceiveFeatureActionPacketHook;
asm
  mov     edi, [esp+10h]
  push    edi
  call    ReceiveFeatureActionPacket
  push    $0045549B
  call    PatchNJump
end;

procedure CreateFeatureBurnPacketHook;
asm
  push    ebp
  push    esi
  push    MAX_WEAPONS_PATCHED-2
  call    BroadcastFeatureActionPacket
  push    $0042353C
  call    PatchNJump
end;

procedure SendAreaOfEffectPacket(p_WeaponProjectile: PWeaponProjectile;
  p_WeaponProjectile2: PWeaponProjectile); stdcall;
var
  Buffer: TAreaOfEffectMessagePatched;
begin
  Buffer.Marker := TANM_AreaOfEffect;
  Buffer.Position := p_WeaponProjectile.Position_Start;
  Buffer.WeaponID := p_WeaponProjectile2.p_Weapon.lWeaponIDCrack;
  HAPI_BroadcastMessage(PPlayerStruct(p_WeaponProjectile2.p_AttackerUnit.p_Owner).lDirectPlayID,
    @Buffer, SizeOf(Buffer));
  Buffer.Position := p_WeaponProjectile2.Position_Target;
  HAPI_BroadcastMessage(PPlayerStruct(p_WeaponProjectile2.p_AttackerUnit.p_Owner).lDirectPlayID,
    @Buffer, SizeOf(Buffer));
end;

procedure Receive_AofEDamagePatched(p_Player: PPlayerStruct;
  Packet: TAreaOfEffectMessagePatched); stdcall;
var
  CurrProjectileIdx: Integer;
  ProjectilesCount: Integer;
  CurrProjectile: PWeaponProjectile;
begin
  CurrProjectileIdx := 0;
  ProjectilesCount := TAData.MainStruct.lNumProjectiles;
  CurrProjectile := TAData.MainStruct.p_Projectiles;
  if ProjectilesCount > 0 then
  begin
    while ((CurrProjectile.Position_Target.X <> Packet.Position.X) or
          (CurrProjectile.Position_Target.Z <> Packet.Position.Z) or
          (CurrProjectile.Position_Target.Y <> Packet.Position.Y) or
          (CurrProjectile.p_Weapon.lWeaponIDCrack <> Packet.WeaponID)) do
    begin
      Inc(CurrProjectileIdx);
      CurrProjectile := Pointer(Cardinal(CurrProjectile) + SizeOf(Pointer));
      if CurrProjectileIdx >= ProjectilesCount then
        Exit;
    end;
  end;
  WEAPONS_ProjectileDamage(CurrProjectile, nil);
end;

procedure SendAreaOfEffectPacketHook;
asm
  push    esi
  sub     ebx, 12
  push    ebx
  call    SendAreaOfEffectPacket
  push    $0049A7EE
  call    PatchNJump
end;

procedure SendWeaponFiredPacket(p_Attacker: PUnitStruct;
  p_TargetUnit: PUnitStruct; p_UnitWeapon: PUnitWeapon; p_PosStart: PPosition;
  p_PosEnd: PPosition); stdcall;
var
  Buffer: TWeaponFiredMessagePatched;
begin
  FillChar(Buffer, SizeOf(Buffer), 0);
  Buffer.Marker := TANM_WeaponFired;
  Buffer.Position_Start := p_PosStart^;
  Buffer.Position_Target := p_PosEnd^;
  Buffer.WeapIdx := (p_UnitWeapon.cStateMask shr 2) and 3;
  Buffer.WeaponID := p_UnitWeapon.p_Weapon.lWeaponIDCrack;
  if p_Attacker <> nil then
    Buffer.AttackerUnitId := TAUnit.GetId(p_Attacker)
  else
    Buffer.AttackerUnitId := 0;
  if p_TargetUnit <> nil then
    Buffer.TargetUnitId := TAUnit.GetId(p_TargetUnit)
  else
    Buffer.TargetUnitId := 0;
  Buffer.Angle := p_UnitWeapon.nAngle;
  Buffer.Trajectory := p_UnitWeapon.nTrajectoryResult;
  Buffer.Interceptor := Buffer.Interceptor xor ((Buffer.Interceptor xor (p_UnitWeapon.p_Weapon.lWeaponTypeMask shr 30)) and 1);
  SendTextLocal(Format('Send Weapon Fire: Target: %d, Attacker: %d, Weapon: %d', [Buffer.TargetUnitId, Buffer.AttackerUnitId, Buffer.WeaponID]));
  HAPI_BroadcastMessage(PPlayerStruct(p_Attacker.p_Owner).lDirectPlayID, @Buffer, SizeOf(Buffer));
end;

procedure SendFireCallback0Hook;
asm
  lea     ecx, [esp+18h]
  push    ebp // position target
  push    ecx // position start
  push    esi // unit weapon
  push    ebx // target unit ptr
  push    edi // attacker unit ptr
  call    SendWeaponFiredPacket
  push    $0049D85E
  call    PatchNJump
end;

procedure SendFireCallback1Hook;
asm
  lea     ecx, [esp+10h]
  mov     ebp, [esp+4Ch]
  push    edi // position target
  push    ecx // position start
  push    esi // unit weapon
  push    ebp // target unit ptr
  push    ebx // attacker unit ptr
  call    SendWeaponFiredPacket
  push    $0049DD30
  call    PatchNJump
end;

procedure SendFireCallback2Hook;
asm
  lea     ecx, [esp+10h]
  lea     eax, [esp+29h]
  mov     edx, [esp+4Ch]
  push    eax // position target
  push    ecx // position start
  push    ebx // unit weapon
  push    edx // target unit ptr
  push    edi // attacker unit ptr
  call    SendWeaponFiredPacket
  push    $0049DEF3
  call    PatchNJump
end;

procedure SendFireCallback3Hook;
asm
  lea     eax, [edi+TUnitStruct.Position]
  mov     ecx, [esp+4Ch]
  push    ebx // position target
  push    eax // position start
  push    esi // unit weapon
  push    ecx // target unit ptr
  push    edi // attacker unit ptr
  call    SendWeaponFiredPacket
  push    $0049DB52
  call    PatchNJump
end;

procedure BroadcastFeatureReclaimPacket(p_Player: PPlayerStruct;
  X, Y: Word); stdcall;
var
  Buffer: TFeatureActionMessagePatched;
begin
  Buffer.Marker := TANM_FeatureAction;
  Buffer.WeaponID := MAX_WEAPONS_PATCHED-1;
  Buffer.X := X;
  Buffer.Y := Y;
  HAPI_BroadcastMessage(p_Player.lDirectPlayID, @Buffer, SizeOf(Buffer));
  SendTextLocal(Format('Reclaimed: X: %d, Y: %d', [Buffer.X, Buffer.Y]));
end;

procedure FeatureReclaimFinishedHook;
asm
  mov     edx, [esi+TUnitStruct.p_Owner]
  push    ebp
  push    edi
  push    edx
  call    BroadcastFeatureReclaimPacket
  push    $004239A9
  call    PatchNJump
end;

procedure InitializeWeaponsArray; stdcall;
var
  i: Integer;
begin
  for i := Low(WeaponsPatchMainStruct.Weapons) to High(WeaponsPatchMainStruct.Weapons) do
  begin
    WeaponsPatchMainStruct.Weapons[i].lWeaponIDCrack := i;
    WeaponsPatchMainStruct.Weapons[i].szWeaponName := #0;
  end;
end;

procedure UNITS_StartWeaponsScriptsPatched(p_Unit: PUnitStruct); stdcall;
var
  WeaponReloadTime: Word;
  WeapIdx: Byte;
  WeapState: Byte;
  WeapIDValid: Byte;
  OutPos: TPosition;
  OutPos2: TPosition;
begin
  WeaponReloadTime := 0;
  for WeapIdx := 0 to 2 do
  begin
    WeapState := p_Unit.UnitWeapons[WeapIdx].cStateMask and $F2 or 4 * (WeapIdx and 3);
    p_Unit.UnitWeapons[WeapIdx].nReloadTime := 0;
    p_Unit.UnitWeapons[WeapIdx].cStateMask := WeapState;
    p_Unit.UnitWeapons[WeapIdx].p_Weapon := p_Unit.p_UnitInfo.Weapons[WeapIdx];
    if p_Unit.p_UnitInfo.Weapons[WeapIdx].lWeaponIDCrack <> 0 then
      WeapIDValid := 1
    else
      WeapIDValid := 0;
    p_Unit.UnitWeapons[WeapIdx].cStock := 0;
    p_Unit.UnitWeapons[WeapIdx].cStateMask := WeapState and $FD or 2 * (WeapIDValid or 8);
    UNITS_QueryWeaponPosition(p_Unit, OutPos, WeapIdx, -1);
    UNITS_CallAimScripts(p_Unit, OutPos2, WeapIdx);
    p_Unit.UnitWeapons[WeapIdx].ZAngle := Trunc((OutPos.Z - OutPos2.Z) * 1.25);
    if p_Unit.UnitWeapons[WeapIdx].p_Weapon.nReloadTime > WeaponReloadTime then
      WeaponReloadTime := p_Unit.UnitWeapons[WeapIdx].p_Weapon.nReloadTime;
  end;
  COBEngine_StartScript(0, 0, p_Unit.p_UnitScriptsData, 0, 0, 0, 1000 * WeaponReloadTime div 30,
    1, False, nil, PAnsiChar('SetMaxReloadTime'));
end;

procedure InitializeWeaponsArrayHook;
asm
  call InitializeWeaponsArray
  push $0042E345;
  call PatchNJump;
end;

procedure LoadWeaponModel(p_Weapon: PWeaponDef;
  p_ModelName: PAnsiChar); stdcall;
label
  newModel;
var
  CurrWeapID: Integer;
  idcmp, idcmp2, idcmp3: Integer;
  Buffer: String[255];
  p_WeaponModel: Pointer;
begin
  CurrWeapID := p_Weapon.lWeaponIDCrack;
  idcmp := 0;
  idcmp3 := 0;
  idcmp2 := CurrWeapID;
  if ( CurrWeapID >= 0 ) then
  begin
    while ( True ) do
    begin
      idcmp3 := idcmp;
      if ( _strcmpi(p_ModelName, @WeaponsPatchMainStruct.Weapons[idcmp].szModelName[0]) = 0 ) then
        Break;
      idcmp := idcmp + 1;
      if ( idcmp >= idcmp2 ) then
        goto newModel;
    end;
    WeaponsPatchMainStruct.Weapons[idcmp2].szModelName[0] := #0;
    WeaponsPatchMainStruct.Weapons[idcmp2].p_WeaponModel := WeaponsPatchMainStruct.Weapons[idcmp3].p_WeaponModel;
  end else
  begin
newModel:
    GetLocalizedFilePath(@Buffer[1], PAnsiChar('objects3d'), p_ModelName, PAnsiChar('3DO'));
    p_WeaponModel := Open3DOFile(PAnsiChar(@Buffer[1]));
    if ( p_WeaponModel = nil ) then
      TerminateProcess_WithWarning(@Buffer[1]);
    Parse3DOFile(p_WeaponModel);
    TextureMatch3DO(p_WeaponModel, p_ModelName);
    WeaponsPatchMainStruct.Weapons[idcmp2].p_WeaponModel := p_WeaponModel;
    StrCopy(@WeaponsPatchMainStruct.Weapons[idcmp2].szModelName[0], p_ModelName);
  end;
end;

procedure LoadWeaponModelHook;
asm
  lea    eax, [esp+40h]
  pushAD
  push   eax
  push   ebp
  call   LoadWeaponModel
  popAD
  push $0042EDA1;
  call PatchNJump;
end;

procedure ReceiveWeaponFiredPatched(p_Player: PPlayerStruct;
  p_Packet: PWeaponFiredMessagePatched); stdcall;
var
  CurrProjectile: PWeaponProjectile;
  ProjectilesCount: Cardinal;
  p_Weapon: PWeaponDef;
  p_AttackerUnit: PUnitStruct;
  p_TargetUnit: PUnitStruct;
  Interceptor: Pointer;
  ProjectileIdx: Integer;
begin
  SendTextLocal(Format('Received Weapon Fire: Target: %d, Attacker: %d, Weapon: %d', [p_Packet.TargetUnitId, p_Packet.AttackerUnitId, p_Packet.WeaponID]));
  p_Weapon := TAWeapon.WeaponId2Ptr(p_Packet.WeaponID);
  if ((p_Weapon.lWeaponTypeMask shr 5) and 1) <> 0 then
  begin
    CurrProjectile := nil;
    ProjectilesCount := TAData.MainStruct.lNumProjectiles;
    if ProjectilesCount < 300 then
    begin
      CurrProjectile := Pointer(Cardinal(TAData.MainStruct.p_Projectiles) + SizeOf(TWeaponProjectile) * ProjectilesCount);
      TAData.MainStruct.lNumProjectiles := ProjectilesCount + 1;
      CurrProjectile.nState := CurrProjectile.nState and $FFFD;
      CurrProjectile.p_TargetUnit := nil;
    end;
    if CurrProjectile <> nil then
    begin
      InitProjectile(CurrProjectile,
                     p_Weapon,
                     @p_Packet.Position_Start,
                     nil,
                     TAData.GameTime,
                     nil);
      CurrProjectile.MoveStep := p_Packet.Position_Target;
    end;
  end else
  begin
    if p_Packet.AttackerUnitId <> 0 then
      p_AttackerUnit := TAUnit.Id2Ptr(p_Packet.AttackerUnitId)
    else
      p_AttackerUnit := nil;
    if p_AttackerUnit <> nil then
    begin
      if (p_AttackerUnit.lUnitStateMask and $10000000) <> 0 then
      begin
        p_AttackerUnit.UnitWeapons[p_Packet.WeapIdx].nTrajectoryResult := p_Packet.Trajectory;
        p_AttackerUnit.UnitWeapons[p_Packet.WeapIdx].nAngle := p_Packet.Angle;
        if p_Packet.TargetUnitId <> 0 then
          p_TargetUnit := TAUnit.Id2Ptr(p_Packet.TargetUnitId)
        else
          p_TargetUnit := nil;
        Interceptor := nil;
        if (p_Packet.Interceptor and 1) <> 0 then
        begin
          ProjectilesCount := TAData.MainStruct.lNumProjectiles;
          if ProjectilesCount > 0 then
          begin
            for ProjectileIdx := 0 to ProjectilesCount - 1 do
            begin
              CurrProjectile := Pointer(Cardinal(TAData.MainStruct.p_Projectiles) + SizeOf(TWeaponProjectile) * Cardinal(ProjectileIdx));
              if (CurrProjectile.cOwnerID <> TAData.LocalPlayerID) then
                if (CurrProjectile.Position_Target.X = p_Packet.Position_Target.X ) then
                  if (CurrProjectile.Position_Target.Z = p_Packet.Position_Target.Z ) then
                    if (CurrProjectile.Position_Target.Y = p_Packet.Position_Target.Y ) then
                    begin
                      if TAUnit.GetId(CurrProjectile.p_AttackerUnit) = p_Packet.TargetUnitId then
                      begin
                        Interceptor := CurrProjectile;
                        Break;
                      end;
                    end;
            end;
          end;
        end;
        if ((p_Weapon.lWeaponTypeMask shr 1) and 1) <> 0 then
        begin
          UNITS_FireProjectile_0(@p_AttackerUnit.UnitWeapons[p_Packet.WeapIdx],
                                 p_AttackerUnit,
                                 @p_Packet.Position_Start,
                                 @p_Packet.Position_Target,
                                 p_TargetUnit);
        end else
          if ((p_Weapon.lWeaponTypeMask shr 4) and 1) <> 0 then
          begin
            UNITS_FireProjectile_1(@p_AttackerUnit.UnitWeapons[p_Packet.WeapIdx],
                                   p_AttackerUnit,
                                   @p_Packet.Position_Start,
                                   @p_Packet.Position_Target,
                                   p_TargetUnit,
                                   Interceptor);
          end else
            if ((p_Weapon.lWeaponTypeMask and 1) <> 0) or
               ((p_Weapon.lWeaponTypeMask and $100000) <> 0) then
            begin
              UNITS_FireProjectile_0_3(@p_AttackerUnit.UnitWeapons[p_Packet.WeapIdx],
                                       p_AttackerUnit,
                                       @p_Packet.Position_Start,
                                       @p_Packet.Position_Target,
                                       p_TargetUnit);
            end else
              if ((p_Weapon.lWeaponTypeMask shr 8) and 1) <> 0 then
              begin
                CurrProjectile := nil;
                ProjectilesCount := TAData.MainStruct.lNumProjectiles;
                if ProjectilesCount < 300 then
                begin
                  CurrProjectile := Pointer(Cardinal(TAData.MainStruct.p_Projectiles) + SizeOf(TWeaponProjectile) * ProjectilesCount);
                  TAData.MainStruct.lNumProjectiles := ProjectilesCount + 1;
                  CurrProjectile.nState := CurrProjectile.nState and $FFFD;
                  CurrProjectile.p_TargetUnit := nil;
                end;
                if CurrProjectile <> nil then
                begin
                  InitProjectile(CurrProjectile,
                                 p_AttackerUnit.UnitWeapons[p_Packet.WeapIdx].p_Weapon,
                                 @p_Packet.Position_Start,
                                 nil,
                                 TAData.GameTime,
                                 p_AttackerUnit);
                  CurrProjectile.Velocity := nil;
                  CurrProjectile.Turn.Z := p_AttackerUnit.Turn.Z;
                  CurrProjectile.MoveStep.Y := 0;
                  CurrProjectile.MoveStep.X := -TurnXLookup(CurrProjectile.Turn.Z, p_AttackerUnit.p_MovementClass.lCurrentSpeed);
                  CurrProjectile.MoveStep.Z := -TurnZLookup(CurrProjectile.Turn.Z, p_AttackerUnit.p_MovementClass.lCurrentSpeed);;
                end;
              end;
      end;
    end;
  end;
end;

Procedure OnInstallWeaponsExpand;
begin
  SetLength(ExtraWeaponDefTags, GetMaxWeapons);
end;

Procedure OnUninstallWeaponsExpand;
begin
end;

function GetPlugin: TPluginData;
var
  Replacement: Cardinal;
begin
  if IsTAVersion31 and State_WeaponsExpand then
  begin
    Result := TPluginData.Create( True, '',
                                  State_WeaponsExpand,
                                  @OnInstallWeaponsExpand,
                                  @OnUnInstallWeaponsExpand );

    Result.MakeRelativeJmp( State_WeaponsExpand,
                            'Load new weapon tags',
                            @WeaponsExpand_NewPropertiesLoadHook,
                            $0042E46E, 1 );

    if IniSettings.WeaponsIDPatch then
    begin
      p_WeaponsPatchMainStruct := @WeaponsPatchMainStruct;

      Result.MakeRelativeJmp( State_WeaponsExpand,
                              'Initialize expanded weapons array',
                              @InitializeWeaponsArrayHook,
                              $0042E31C, 1 );

      Result.MakeRelativeJmp( State_WeaponsExpand,
                              'Load weapon model',
                              @LoadWeaponModelHook,
                              $0042EC99, 1 );

      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @WeaponName2WeaponTypeDef,
                             $00422D23 );
      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @WeaponName2WeaponTypeDef,
                             $0042CDFB );
      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @WeaponName2WeaponTypeDef,
                             $0042CE36 );
      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @WeaponName2WeaponTypeDef,
                             $0042CE6F );
      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @WeaponName2WeaponTypeDef,
                             $0042CEA8 );
      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @WeaponName2WeaponTypeDef,
                             $0042CEE1 );
      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @WeaponName2WeaponTypeDef,
                             $00437CE9 );

      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @UNITS_StartWeaponsScriptsPatched,
                             $00485F04 );
      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @UNITS_StartWeaponsScriptsPatched,
                             $004860AC );
      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @UNITS_StartWeaponsScriptsPatched,
                             $004862C4 );

      Result.MakeRelativeJmp( State_WeaponsExpand, '',
                              @ReceiveFeatureActionPacketHook,
                              $0045544D, 1 );

      Result.MakeRelativeJmp( State_WeaponsExpand, '',
                              @CreateFeatureBurnPacketHook,
                              $00423516, 1 );

      Replacement := SizeOf(TWeaponDef) * Length(WeaponsPatchMainStruct.Weapons);
      Result.MakeReplacement( State_WeaponsExpand,
                              'Release weapons array size',
                              $0042F433, Replacement, SizeOf(Replacement) );

      Replacement := Cardinal(@p_WeaponsPatchMainStruct);
      Result.MakeReplacement( State_WeaponsExpand,
                              'loading unit weapon',
                              $0042CDCE, Replacement, SizeOf(Replacement) );
      Result.MakeReplacement( State_WeaponsExpand,
                              'releasing weapons array',
                              $0042F3AC, Replacement, SizeOf(Replacement) );
      Result.MakeReplacement( State_WeaponsExpand,
                              'set meteor weapons',
                              $00437CF9, Replacement, SizeOf(Replacement) );
      Result.MakeReplacement( State_WeaponsExpand,
                              'set meteor weapons 2',
                              $00437D15, Replacement, SizeOf(Replacement) );
      Result.MakeReplacement( State_WeaponsExpand,
                              'load weapon tdf',
                              $0042E46A, Replacement, SizeOf(Replacement) );

      Result.MakeRelativeJmp( State_WeaponsExpand, '',
                              @SendAreaOfEffectPacketHook,
                              $0049A769, 3 );

      Result.MakeRelativeJmp( State_WeaponsExpand, '',
                              @SendFireCallback0Hook,
                              $0049D7AD, 3 );
      Result.MakeRelativeJmp( State_WeaponsExpand, '',
                              @SendFireCallback1Hook,
                              $0049DC71, 3 );
      Result.MakeRelativeJmp( State_WeaponsExpand, '',
                              @SendFireCallback2Hook,
                              $0049DE32, 3 );
      Result.MakeRelativeJmp( State_WeaponsExpand, '',
                              @SendFireCallback3Hook,
                              $0049DAA2, 3 );

      Result.MakeRelativeJmp( State_WeaponsExpand, '',
                              @FeatureReclaimFinishedHook,
                              $0042397F, 1 );

      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @Receive_AofEDamagePatched,
                             $00455443 );

      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @FEATURES_TakeWeaponDamagePatched,
                             $0049A626 );

      Result.MakeStaticCall( State_WeaponsExpand, '',
                             @ReceiveWeaponFiredPatched,
                             $00455433 );

      { todo :
        0049DFA9 - fire map weapon id to packet
        v9 = WeaponType_Ptr->ID;
        if ( *(*&TAMainStructPtr + offsetof(TAMainStruct, WorkStatusMask)) & 1 )
        begin
          v10 = Postion_Start->y;
          PacketBuf.start.x = Postion_Start->x;
          v11 = Postion_Start->z;
          PacketBuf.start.y = v10;
          v12 = TargetPostion->x;
          PacketBuf.start.z = v11;
          v13 = TargetPostion->y;
          PacketBuf.end.x = v12;
          v14 = TargetPostion->z;
          PacketBuf.PacketType = 13;
          PacketBuf.end.y = v13;
          PacketBuf.end.z = v14;
          PacketBuf.WeaponID = v9;
          v15 = LocalPlayer_DirectID();
          HAPI_BroadcastMessage(v15, &PacketBuf, 36);
        end

        0040954D - ai related 1
        00409682 - ai related 2
        00409940 - ai related 3

        00487A1C - Save game unit weapons write, fix ID offset and hapi account size
        00487622 - Load game unit weapons
        00499B02 - send fire weapon packet
        00405210 - resurrect order
      }
    end;   
  end else
    Result := nil;
end;

end.

