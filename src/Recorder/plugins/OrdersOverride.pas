unit OrdersOverride;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_OrdersOverride: Boolean = True;

function GetPlugin: TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallOrdersOverride;
Procedure OnUninstallOrdersOverride;

// -----------------------------------------------------------------------------

implementation
uses
  idplay,
  SysUtils,
  UnitInfoExpand,
  UnitSearchHandlers,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_MemUnits,
  TA_MemPlayers,
  TA_FunctionsU;

var
  OrdersOverridePlugin: TPluginData;

// -----------------------------------------------------------------------------
// Teleporters
// -----------------------------------------------------------------------------

type
  TUnitToTeleport = record
    Id: Word;
    OffX: Integer;
    OffZ: Integer;
  end;

function TeleportGroupOfUnits(Teleporter: PUnitStruct; UnitsFilter: TUnitSearchFilterSet;
  CenterPosition, TargetPosition: PPosition; Distance: Integer): Integer;
var
  TargetPositionWithOffset: TPosition;
  nPosX, nPosZ: Word;
  UnitsArr: array of TUnitToTeleport;
  TestedUnit: PUnitStruct;
  i, j: Cardinal;
  UnitsToTeleportCount: Integer;
  TestedUnitInfo: PUnitInfo;
begin
  TestedUnit := TAData.UnitsArray_p;
  UnitsToTeleportCount := 0;
  while ( Cardinal(TestedUnit) <= Cardinal(TAData.EndOfUnitsArray_p) ) do
  begin
    if TestedUnit <> Teleporter then
    begin
      if TestedUnit.p_UnitInfo.cBMCode = 1 then
        if TAUnits.UnitsFilterVsUnit(TestedUnit, UnitsFilter, Teleporter.p_Owner) then
          if TAMem.DistanceBetweenPosCompare(@TestedUnit.Position, @Teleporter.Position, Distance) then
          begin
            Inc(UnitsToTeleportCount);
            SetLength(UnitsArr, High(UnitsArr) + 2);
            UnitsArr[UnitsToTeleportCount-1].Id := Word(TestedUnit.lUnitInGameIndex);
            UnitsArr[UnitsToTeleportCount-1].OffX := TestedUnit.Position.X - CenterPosition.X;
            UnitsArr[UnitsToTeleportCount-1].OffZ := TestedUnit.Position.Z - CenterPosition.Z;
          end;
    end;
    TestedUnit := Pointer(Cardinal(TestedUnit) + SizeOf(TUnitStruct));
  end;

  j := 0;
  if UnitsToTeleportCount > 0 then
  begin
    for i := 0 to UnitsToTeleportCount - 1 do
    begin
      TestedUnit := TAUnit.Id2Ptr(UnitsArr[i].Id);
      TestedUnitInfo := TestedUnit.p_UnitInfo;
      if TestedUnitInfo <> nil then
      begin
        if (TestedUnitInfo.cBMCode = 1) and
           ((TestedUnitInfo.UnitTypeMask2 and 2) = 2) and
           (TAUnit.GetUnitInfoField(TestedUnit, uiBUILDER) = 0) then  // can fire
        begin
          GetTPosition(SHiWord(TargetPosition.X + UnitsArr[i].OffX),
                       SHiWord(TargetPosition.Z + UnitsArr[i].OffZ),
                       TargetPositionWithOffset);

          if SHiWord(TargetPositionWithOffset.X) >= TAData.MainStruct.TNTMemStruct.lMapWidth - 128 then
            Continue;
          if SHiWord(TargetPositionWithOffset.Z) >= TAData.MainStruct.TNTMemStruct.lMapHeight - 32 then
            Continue;

          TAUnit.BuildPosition2Grid(TargetPositionWithOffset, TestedUnit.p_UnitInfo, nPosX, nPosZ);
          if (nPosX < TestedUnit.p_UnitInfo.nFootPrintSizeX) then
          begin
            nPosX := TestedUnit.p_UnitInfo.nFootPrintSizeX;
            TargetPositionWithOffset.X := TestedUnit.p_UnitInfo.nFootPrintSizeX * 16;
          end;
          if (nPosZ < TestedUnit.p_UnitInfo.nFootPrintSizeZ) then
          begin
            nPosZ := TestedUnit.p_UnitInfo.nFootPrintSizeZ;
            TargetPositionWithOffset.Z := TestedUnit.p_UnitInfo.nFootPrintSizeZ * 16;
          end;

          if TAUnit.TestAttachAtGridSpot(TestedUnit.p_UnitInfo, nPosX, nPosZ) then
          begin
            j := j + 1;
            TAUnit.CobStartScript(Teleporter, 'Teleporting', @TargetPositionWithOffset.X,
              @TargetPositionWithOffset.Z, @TargetPositionWithOffset.Y,
              @UnitsArr[i].ID, True);
            TAUnit.CreateMainOrder(TestedUnit, Teleporter, Action_Teleport, @TargetPositionWithOffset, 0, 1, 0);
          end else
            TAUnit.CreateMainOrder(TestedUnit, Teleporter, Action_Move_Ground, @TargetPositionWithOffset, 0, 1, 0);
        end;
      end;
    end;
  end;
  if j <> 0 then
    UnitsCustomFields[Word(Teleporter.lUnitInGameIndex)].TeleportReloadMax := TAMem.DistanceBetweenPos(CenterPosition, TargetPosition);
  Result := j;
end;

function TeleportUnitsFromYardmap(TeleporterUnit: PUnitStruct;
  UnitsFilter: TUnitSearchFilterSet; UnitOrder: PUnitOrder): Integer;
var
  j : Integer;
  TeleporterUnitInfo : PUnitInfo;
  GridPosX, GridPosY, GridPosZ: Integer;
  FootPrintX, FootPrintY, FootPrintZ: Integer;
  CurUnit : PUnitStruct;
  NewPos: TPosition;
begin
  TeleporterUnitInfo := TeleporterUnit.p_UnitInfo;

  j := 0;

  GridPosX := TeleporterUnit.Position.X + TeleporterUnitInfo.lModelWidthX;
  GridPosY := TeleporterUnit.Position.Y + TeleporterUnitInfo.lModelWidthY;
  GridPosZ := TeleporterUnit.Position.Z + TeleporterUnitInfo.lModelHeight;

  FootPrintX := TeleporterUnit.Position.X + TeleporterUnitInfo.lFootPrintX;
  FootPrintY := TeleporterUnit.Position.Y + TeleporterUnitInfo.lFootPrintY;
  FootPrintZ := TeleporterUnit.Position.Z + TeleporterUnitInfo.lFootPrintZ;

  CurUnit := TAData.UnitsArray_p;
  if ( Cardinal(CurUnit) <= Cardinal(TAData.EndOfUnitsArray_p) ) then
  begin
    while ( True ) do
    begin
      if ( CurUnit.Position.X >= GridPosX ) then
      begin
        if ( CurUnit.Position.X <= FootPrintX ) then
        begin
          if ( CurUnit.Position.Z >= GridPosZ ) then
          begin
            if ( CurUnit.Position.Z <= FootPrintZ ) then
            begin
              if ( CurUnit.Position.Y >= GridPosY ) then
              begin
                if ( CurUnit.Position.Y <= FootPrintY ) and
                   ( TeleporterUnit <> CurUnit ) then
                begin
                  if not TAUnits.UnitsFilterVsUnit(CurUnit, UnitsFilter, TeleporterUnit.p_Owner) then
                  begin
                    Inc(j);
                    NewPos.X := CurUnit.Position.x + UnitOrder.Position.x - TeleporterUnit.Position.X;
                    NewPos.Y := CurUnit.Position.y + UnitOrder.Position.y - TeleporterUnit.Position.Y;
                    NewPos.Z := CurUnit.Position.z + UnitOrder.Position.z - TeleporterUnit.Position.Z;
                    UNITS_NewUnitPosition(CurUnit, NewPos.X, NewPos.Y, NewPos.Z, 1);
                    TASfx.EmitSfxFromPiece(TeleporterUnit, CurUnit, 0, 8, True);
                  end;
                end;
              end;
            end;
          end;
        end;
      end;
      CurUnit := Pointer(Cardinal(CurUnit) + SizeOf(TUnitStruct));
      if ( Cardinal(CurUnit) > Cardinal(TAData.EndOfUnitsArray_p) ) then
        Break;
    end;
  end;
  Result := j;
end;

function OrdersOverride_Teleport(p_Unit: PUnitStruct;
  p_Order: PUnitOrder; LastState: Cardinal): Integer; stdcall;
var
  Distance: Integer;
  StartEndTeleport: Integer;
  TargetPosition: TPosition;
  UnitID: Word;
  p_PushedOrderMem: Pointer;
  p_PushedOrder: PUnitOrder;
  OrderFlags: Cardinal;
  OrderActionIdx: Byte;
  OrderPar1: Integer;
  OrderPar2: Integer;
  OrderTargetPos: PPosition;
  OrderTargetUnit: PUnitStruct;
begin
  UnitID := TAUnit.GetId(p_Unit);
  case p_Order.ucState of
    0 : begin
          if UnitsCustomFields[UnitID].TeleportReloadMax > 0 then
          begin
            // wait for teleport reload
            p_Order.lPauseState := p_Order.lPauseState or 1;
            p_Order.lRecallTime := 10 + TAData.GameTime;
            Result := 2;
            Exit;
          end else
          begin
            if UnitInfoCustomFields[p_Unit.p_UnitInfo.nCategory].TeleportMethod <= tmVTOLOthers then
            begin
              if UnitInfoCustomFields[p_Unit.p_UnitInfo.nCategory].TeleportMethod = tmVTOLOthers then
              begin
                TeleportGroupOfUnits( p_Unit,
                                      UnitInfoCustomFields[p_Unit.p_UnitInfo.nCategory].TeleportFilter,
                                      @p_Unit.Position,
                                      @p_Order.Position,
                                      UnitInfoCustomFields[p_Unit.p_UnitInfo.nCategory].TeleportMinDistance );
                ORDERS_BackupMainOrder(0, 0, p_Order, nil);
                p_PushedOrderMem := MEM_Alloc(SizeOf(TUnitOrder));
                if p_PushedOrderMem <> nil then
                begin
                  OrderFlags := 0;
                  OrderPar2 := 0;
                  OrderPar1 := 0;
                  OrderTargetPos := @p_Order.Position;
                  OrderTargetUnit := nil;
                  OrderActionIdx := TAMem.ScriptActionName2Index('VTOL_MOVE');
                  p_PushedOrder := ORDERS_CreateObject(0, 0, p_PushedOrderMem,
                    OrderFlags, OrderPar2, OrderPar1, OrderTargetPos, OrderTargetUnit, OrderActionIdx);
                end else
                  p_PushedOrder := nil;
                ORDERS_PushOrder(p_Unit, p_PushedOrder);
                ORDERS_BackupMainOrder(0, 0, p_Order, nil);
                p_Order.lPauseState := 0;
                Result := 5;
                Exit;
              end else
              begin
                if TAUnit.TestUnloadPosition(p_Unit.p_UnitInfo, p_Order.Position) then
                begin
                  if UnitInfoCustomFields[p_Unit.p_UnitInfo.nCategory].TeleportMethod <> tmNone then
                    UnitsCustomFields[UnitID].TeleportReloadCur := 0;
                  Result := 1;
                end else
                  Result := 7;
              end;
            end else
            begin
              UnitsCustomFields[UnitID].TeleportReloadCur := 0;
              if TeleportUnitsFromYardmap(p_Unit,
                                          UnitInfoCustomFields[p_Unit.p_UnitInfo.nCategory].TeleportFilter,
                                          p_Order) <> 0 then
              begin
                Distance := TAMem.DistanceBetweenPos(@p_Unit.Position, @p_Order.Position);
                UnitsCustomFields[UnitID].TeleportReloadMax := Distance;
              end;
              Result := 5;
            end;
          end;
        end;
    1 : begin
          if UnitInfoCustomFields[p_Unit.p_UnitInfo.nCategory].TeleportMethod = tmVTOLOthers then
          begin
            Result := 5;
            Exit;
          end;
          TargetPosition := p_Order.Position;
          if LastState = 1 then
          begin
            if UnitInfoCustomFields[p_Unit.p_UnitInfo.nCategory].TeleportMethod <> tmNone then
            begin
              Distance := TAMem.DistanceBetweenPos(@p_Unit.Position, @TargetPosition);
              if Distance < UnitInfoCustomFields[p_Unit.p_UnitInfo.nCategory].TeleportMinReloadTime then
                Distance := UnitInfoCustomFields[p_Unit.p_UnitInfo.nCategory].TeleportMinReloadTime;
              UnitsCustomFields[UnitID].TeleportReloadMax := Distance;
            end;

            if TAData.NetworkLayerEnabled then
              GlobalDPlay.Broadcast_NewUnitLocation(TAUnit.GetID(p_Unit), TargetPosition);
            UNITS_NewUnitPosition(p_Unit, TargetPosition.X, TargetPosition.Y, TargetPosition.Z, 1);
            if p_Order.lPar1 <> 0 then
              TASfx.EmitSfxFromPiece(p_Order.p_UnitTarget, p_Unit, 0, 8, True);
            StartEndTeleport := 1;
            TAUnit.CobStartScript(p_Unit, 'Teleporting', @TargetPosition.X,
              @TargetPosition.Z, @TargetPosition.Y, @StartEndTeleport, True);
            Result := 5
          end else
          begin
            StartEndTeleport := 0;
            TAUnit.CobStartScript(p_Unit, 'Teleporting', @TargetPosition.X,
              @TargetPosition.Z, @TargetPosition.Y, @StartEndTeleport, True);
            p_Order.lPauseState := p_Order.lPauseState or 1;
            p_Order.lRecallTime := 10 + TAData.GameTime;
            Result := 2;
          end;
        end;
    else
      Result := 7;
  end;
end;

procedure OrdersOverride_TeleportPositionOffset;
label
  UseOffsetPosition,
  UseNonOffsetPosition;
asm
  test    byte ptr [eax+11h], 2
  jz      UseNonOffsetPosition
UseOffsetPosition:
  push $0048D130;
  call PatchNJump;
UseNonOffsetPosition:
  mov     al, byte ptr [esp+50h] // action index
  cmp     al, 47                 // teleport self button
  jz      UseOffsetPosition
  push $0048D1D9;
  call PatchNJump;
end;

// -----------------------------------------------------------------------------
// Repair patrol with resurrector
// -----------------------------------------------------------------------------

function OrdersOverride_RepairPatrol(p_Unit: PUnitStruct;
  p_Order: PUnitOrder; LastState: Integer): Integer; stdcall;
var
  p_UnitOwner: PPlayerStruct;
  UnitSearchCallbackRec: TUnitSearchCallbackRec;
  UnitSearchUnitsArray: TUnitSearchUnitsArray;
  FoundUnitsCount: Integer;
  p_FoundUnit: PUnitStruct;
  p_FoundUnitArray: Pointer;
  ActionIndex: Byte;
  lPauseState: Byte;

  MetalFeaturePosition: TPosition;
  p_MetalFeaturePosition: Pointer;
  EnergyFeaturePosition: TPosition;
  p_EnergyFeaturePosition: Pointer;

  FeatureEnergy: Single;
  FeatureMetal: Single;

  p_PushedOrderMem: Pointer;
  p_PushedOrder: PUnitOrder;
  OrderFlags: Cardinal;
  OrderActionIdx: Byte;
  OrderPar1: Integer;
  OrderPar2: Integer;
  OrderTargetPos: PPosition;
  OrderTargetUnit: PUnitStruct;

  IsResurrector: Boolean;
  HasEnergy: Boolean;
  WreckTypeName: String;
  FeatureTypeID: Word;
  WreckUnitInfoID: Word;
begin
  if p_Order.ucState <> 0 then
  begin
    if p_Order.ucState <> 1 then
    begin
      Result := 7;
      Exit;
    end;
    if (LastState and $E0) <> 0 then
    begin
      Result := 6;
      Exit;
    end;
    ORDERS_MovementRelated(0, 0, p_Order, 16, @p_Order.Position);
    ORDERS_DelayCallback(0, 0, p_Order, 60);
    lPauseState := p_Order.lPauseState;
    lPauseState := lPauseState or $E0;
    p_Order.lPauseState := lPauseState;

    p_UnitOwner := p_Unit.p_Owner;
    IsResurrector := TAUnit.GetUnitInfoField(p_Unit, uiCanResurrect) <> 0;
    if (p_UnitOwner.Resources.fEnergyStorageMax * 0.2) <= p_UnitOwner.Resources.fCurrentEnergy then
    begin
      HasEnergy := True;
      if not IsResurrector then
      begin
        FillChar(UnitSearchUnitsArray, SizeOf(TUnitSearchUnitsArray), 0);
        UnitSearchCallbackRec.p_OwnerPtr := p_Unit.p_Owner;
        UnitSearchCallbackRec.p_RetArray := @UnitSearchUnitsArray;
        UnitSearchCallbackRec.p_CallerUnit := p_Unit;
        UnitSearchCallbackRec.p_CallbackProc := LookForRepairUnitsHandler;
        CallbackForUnitsInDistance(@p_Unit.Position,
          p_Unit.p_UNITINFO.nSightDistance shl 16, @UnitSearchCallbackRec);
        p_FoundUnitArray := UnitSearchUnitsArray.p_ArrayBegin;
        if UnitSearchUnitsArray.p_ArrayBegin <> nil then
          FoundUnitsCount := Integer(Cardinal(UnitSearchUnitsArray.p_ArrayEnd) - Cardinal(UnitSearchUnitsArray.p_ArrayBegin)) div SizeOf(Pointer)
        else
          FoundUnitsCount := 0;
        if FoundUnitsCount > 0 then
        begin
          p_FoundUnit := PUnitStruct(PCardinal(Cardinal(UnitSearchUnitsArray.p_ArrayBegin) +
            Cardinal(4 * Random(FoundUnitsCount)))^);
          if TAPlayer.GetAlliedState(p_FoundUnit.p_Owner, TAPlayer.PlayerIndex(p_Unit.p_Owner)) then
          begin
            ScriptAction_Type2Index(@ActionIndex, 8, p_Unit, p_FoundUnit, nil);
            if ActionIndex <> 0 then
            begin
              if ORDERS_ChaseUnitToBeRepaired(p_Unit, p_FoundUnit, 0) then
              begin
                MEM_Free_PPointer(UnitSearchUnitsArray.p_ArrayBegin);
                Result := 6;
              end else
              begin
                MEM_Free_PPointer(UnitSearchUnitsArray.p_ArrayBegin);
                Result := 3;
              end;
              Exit;
            end;
          end;
          p_FoundUnitArray := UnitSearchUnitsArray.p_ArrayBegin;
        end;
        MEM_Free_PPointer(p_FoundUnitArray);
      end;
    end else
      HasEnergy := False;
      
    if ((p_UnitOwner.Resources.fEnergyStorageMax * 0.2 > p_UnitOwner.Resources.fCurrentEnergy) or
       (p_UnitOwner.Resources.fMetalStorageMax * 0.2 > p_UnitOwner.Resources.fCurrentMetal)) or
       IsResurrector then
    begin
      p_MetalFeaturePosition := @MetalFeaturePosition;
      p_EnergyFeaturePosition := @EnergyFeaturePosition;
      if SearchForReclamateFeatures(@p_Unit.Position,
                                    p_Unit.p_UNITINFO.nSightDistance shl 16,
                                    @p_EnergyFeaturePosition,
                                    @FeatureEnergy,
                                    @p_MetalFeaturePosition,
                                    @FeatureMetal) then
      begin
        if (p_MetalFeaturePosition <> nil) then
        begin
          if IsResurrector and HasEnergy then
          begin
            FeatureTypeID := GetFeatureTypeFromOrder(p_MetalFeaturePosition, nil, nil);
            if FeatureTypeID <> Word(-1) then
            begin
              WreckTypeName := StringReplace(TAMem.FeatureDefId2Ptr(FeatureTypeID).Name, '_', #0, []);
              WreckUnitInfoID := UNITINFO_Name2ID(PAnsiChar(WreckTypeName));
              if (WreckUnitInfoID <> 0) and
                 (p_UnitOwner.Resources.fMetalStorageMax * 0.2 <= p_UnitOwner.Resources.fCurrentMetal) then
              begin
                ORDERS_BackupMainOrder(0, 0, p_Order, nil);
                p_PushedOrderMem := MEM_Alloc(SizeOf(TUnitOrder));
                if p_PushedOrderMem <> nil then
                begin
                  OrderFlags := 0;
                  OrderPar2 := 0;
                  OrderPar1 := 0;
                  MetalFeaturePosition.Y := GetPosHeight(@MetalFeaturePosition) shl 16;
                  OrderTargetPos := @MetalFeaturePosition;
                  OrderTargetUnit := nil;
                  OrderActionIdx := TAMem.ScriptActionName2Index('RESURRECT');
                  p_PushedOrder := ORDERS_CreateObject(0, 0, p_PushedOrderMem,
                    OrderFlags, OrderPar2, OrderPar1, OrderTargetPos, OrderTargetUnit, OrderActionIdx);
                end else
                  p_PushedOrder := nil;
                ORDERS_PushOrder(p_Unit, p_PushedOrder);
                ORDERS_BackupMainOrder(0, 0, p_Order, nil);
                p_Order.lPauseState := 0;
                Result := 3;
                Exit;
              end;
            end;
          end;
        end;

        if (p_MetalFeaturePosition <> nil) and
           (p_UnitOwner.Resources.fMetalStorageMax * 0.2 > p_UnitOwner.Resources.fCurrentMetal) then
        begin
          ORDERS_BackupMainOrder(0, 0, p_Order, nil);
          p_PushedOrderMem := MEM_Alloc(SizeOf(TUnitOrder));
          if p_PushedOrderMem <> nil then
          begin
            OrderFlags := 0;
            OrderPar2 := 0;
            OrderPar1 := 0;
            MetalFeaturePosition.Y := GetPosHeight(@MetalFeaturePosition) shl 16;
            OrderTargetPos := @MetalFeaturePosition;
            OrderTargetUnit := nil;
            OrderActionIdx := TAMem.ScriptActionName2Index('RECLAIM');
            p_PushedOrder := ORDERS_CreateObject(0, 0, p_PushedOrderMem,
              OrderFlags, OrderPar2, OrderPar1, OrderTargetPos, OrderTargetUnit, OrderActionIdx);
          end else
            p_PushedOrder := nil;
          ORDERS_PushOrder(p_Unit, p_PushedOrder);
          ORDERS_BackupMainOrder(0, 0, p_Order, nil);
          p_Order.lPauseState := 0;
          Result := 3;
          Exit;
        end;

        if (p_EnergyFeaturePosition <> nil) and
           (p_UnitOwner.Resources.fEnergyStorageMax * 0.2 > p_UnitOwner.Resources.fCurrentEnergy) then
        begin
          ORDERS_BackupMainOrder(0, 0, p_Order, nil);
          p_PushedOrderMem := MEM_Alloc(SizeOf(TUnitOrder));
          if p_PushedOrderMem <> nil then
          begin
            OrderFlags := 0;
            OrderPar2 := 0;
            OrderPar1 := 0;
            EnergyFeaturePosition.Y := GetPosHeight(@EnergyFeaturePosition) shl 16;
            OrderTargetPos := @EnergyFeaturePosition;
            OrderTargetUnit := nil;
            OrderActionIdx := TAMem.ScriptActionName2Index('RECLAIM');
            p_PushedOrder := ORDERS_CreateObject(0, 0, p_PushedOrderMem,
              OrderFlags, OrderPar2, OrderPar1, OrderTargetPos, OrderTargetUnit, OrderActionIdx);
          end else
            p_PushedOrder := nil;
          ORDERS_PushOrder(p_Unit, p_PushedOrder);
          ORDERS_BackupMainOrder(0, 0, p_Order, nil);
          p_Order.lPauseState := 0;
          Result := 3;
          Exit;
        end;

        if (p_MetalFeaturePosition <> nil) and
           (p_UnitOwner.Resources.fCurrentMetal + FeatureMetal <= p_UnitOwner.Resources.fMetalStorageMax) then
        begin
          ORDERS_BackupMainOrder(0, 0, p_Order, nil);
          p_PushedOrderMem := MEM_Alloc(SizeOf(TUnitOrder));
          if p_PushedOrderMem <> nil then
          begin
            OrderFlags := 0;
            OrderPar2 := 0;
            OrderPar1 := 0;
            MetalFeaturePosition.Y := GetPosHeight(@MetalFeaturePosition) shl 16;
            OrderTargetPos := @MetalFeaturePosition;
            OrderTargetUnit := nil;
            OrderActionIdx := TAMem.ScriptActionName2Index('RECLAIM');
            p_PushedOrder := ORDERS_CreateObject(0, 0, p_PushedOrderMem,
              OrderFlags, OrderPar2, OrderPar1, OrderTargetPos, OrderTargetUnit, OrderActionIdx);
          end else
            p_PushedOrder := nil;
          ORDERS_PushOrder(p_Unit, p_PushedOrder);
          ORDERS_BackupMainOrder(0, 0, p_Order, nil);
          p_Order.lPauseState := 0;
          Result := 3;
          Exit;
        end;

        if (p_EnergyFeaturePosition <> nil) and
           (p_UnitOwner.Resources.fCurrentEnergy + FeatureEnergy <= p_UnitOwner.Resources.fEnergyStorageMax ) then
        begin
          ORDERS_BackupMainOrder(0, 0, p_Order, nil);
          p_PushedOrderMem := MEM_Alloc(SizeOf(TUnitOrder));
          if p_PushedOrderMem <> nil then
          begin
            OrderFlags := 0;
            OrderPar2 := 0;
            OrderPar1 := 0;
            EnergyFeaturePosition.Y := GetPosHeight(@EnergyFeaturePosition) shl 16;
            OrderTargetPos := @EnergyFeaturePosition;
            OrderTargetUnit := nil;
            OrderActionIdx := TAMem.ScriptActionName2Index('RECLAIM');
            p_PushedOrder := ORDERS_CreateObject(0, 0, p_PushedOrderMem,
              OrderFlags, OrderPar2, OrderPar1, OrderTargetPos, OrderTargetUnit, OrderActionIdx);
          end else
            p_PushedOrder := nil;
          ORDERS_PushOrder(p_Unit, p_PushedOrder);
          ORDERS_BackupMainOrder(0, 0, p_Order, nil);
          p_Order.lPauseState := 0;
          Result := 3;
          Exit;
        end;
      end;
    end;
    Result := 2;
    Exit;
  end;
  if p_Order.p_UnitTarget <> nil then
    p_Order.Position := p_Order.p_UnitTarget.Position;
  ORDERS_RecoverMainOrder(p_Unit, p_Order);
  Result := 1;
end;

Procedure OnInstallOrdersOverride;
var
  p_ActionHandler: PActionHandler;
  NewAddress: Cardinal;
begin
  p_ActionHandler := TAMem.ScriptActionIndex2Handler(TAMem.ScriptActionName2Index('TELEPORT'));
  NewAddress := Cardinal(@OrdersOverride_Teleport);
  OrdersOverridePlugin.MakeReplacement( True,
                                        'Override teleport order',
                                        Cardinal(@p_ActionHandler.p_Order),
                                        NewAddress,
                                        SizeOf(Pointer) );

  p_ActionHandler := TAMem.ScriptActionIndex2Handler(TAMem.ScriptActionName2Index('REPAIRPATROL'));
  NewAddress := Cardinal(@OrdersOverride_RepairPatrol);
  OrdersOverridePlugin.MakeReplacement( True,
                                        'Override repair patrol order to implement resurrect',
                                        Cardinal(@p_ActionHandler.p_Order),
                                        NewAddress,
                                        SizeOf(Pointer) );
end;

Procedure OnUninstallOrdersOverride;
begin
end;

function GetPlugin: TPluginData;
begin
  if IsTAVersion31 and State_OrdersOverride then
  begin
    OrdersOverridePlugin := TPluginData.Create( False, 'OrdersOverride Plugin',
                                                State_OrdersOverride,
                                                @OnInstallOrdersOverride,
                                                @OnUninstallOrdersOverride );

    OrdersOverridePlugin.MakeRelativeJmp( State_OrdersOverride,
                                          'multiple teleport, apply position offset',
                                          @OrdersOverride_TeleportPositionOffset,
                                          $0048D126, 1 );
                            
    Result := OrdersOverridePlugin;
  end else
    Result := nil;
end;

end.
