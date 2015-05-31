unit UnitSearchHandlers;

interface
uses
  TA_MemoryStructures;

type
  PUnitCallbackProcHandler = ^TUnitCallbackProcHandler;  
  TUnitCallbackProcHandler = procedure( eax, edx: Pointer;
                                        CallbackRec: TUnitSearchCallbackRec;
                                        p_FoundUnit: PUnitStruct ); register;

procedure SetShield(eax, edx: Pointer;
  CallbackRec: TUnitSearchCallbackRec; p_FoundUnit: PUnitStruct); register;

const
  LookForRepairUnitsHandler: Pointer = Pointer($004FC960);
  VTOLSearchForLandHandler: Pointer = Pointer($004FCC60);
  VTOLSearchForLandRepairPatrolHandler: Pointer = Pointer($004FCC64);
  SonarJammingHandler: Pointer = Pointer($004FD554);
  RadarJammingHandler: Pointer = Pointer($004FD558);
  UnitGoesUnderWaterHandler: Pointer = Pointer($004FD55C);
  
var
  SetShieldHandler: PUnitCallbackProcHandler = @SetShield;

implementation
uses
  idplay,
  TA_MemoryLocations,
  GUIEnhancements,
  TA_MemUnits;

procedure SetShield(eax, edx: Pointer;
  CallbackRec: TUnitSearchCallbackRec; p_FoundUnit: PUnitStruct); register;
var
  UnitId: Word;
  bChangedState: Boolean;
  p_Shield: PUnitStruct;
begin
  p_Shield := CallbackRec.p_CallerUnit;
  if p_FoundUnit <> p_Shield then
  begin
    // found unit is other shield
    UnitId := TAUnit.GetId(p_FoundUnit);
    if UnitsCustomFields[UnitId].ShieldRange <> 0 then Exit;
    if (TAUnit.IsAllied(p_Shield, UnitId) = 1) or
       (p_Shield = nil) then
    begin
      bChangedState := (UnitsCustomFields[UnitId].ShieldedBy <> p_Shield);
      UnitsCustomFields[UnitId].ShieldedBy := p_Shield;
      if bChangedState then
      begin
        if ForceBottomStateRefresh = 0 then
          ForceBottomStateRefresh := 1;
        if TAData.NetworkLayerEnabled then
        begin
          if Assigned(GlobalDPlay) then
          begin
            if p_Shield <> nil then
              GlobalDPlay.Broadcast_ExtraUnitState(UnitID, 1, TAUnit.GetId(p_Shield))
            else
              GlobalDPlay.Broadcast_ExtraUnitState(UnitID, 1, 0);
          end;
        end;
      end;
    end;
  end;
end;

end.
