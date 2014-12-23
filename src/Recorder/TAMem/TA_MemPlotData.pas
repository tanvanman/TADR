unit TA_MemPlotData;

interface
uses
  TA_MemoryStructures, TA_MemoryLocations, TA_MemUnits;

type
  TAMap = class
  public
    class procedure SetCameraToUnit(p_Unit: PUnitStruct);
    class function PlaceFeatureOnMap(FeatureName: String;
      Position: TPosition; Turn: TTurn) : Boolean;
    class function RemoveMapFeature(X, Z: Integer; Method: Boolean) : Boolean;
    class function PositionInLOS(Player: PPlayerStruct; Position: PPosition) : Boolean;
  end;

implementation
uses
  TA_FunctionsU;

// -----------------------------------------------------------------------------
// TAMap
// -----------------------------------------------------------------------------

class procedure TAMap.SetCameraToUnit(p_Unit: PUnitStruct);
begin
  PTAdynmemStruct(TAData.MainStructPtr).pCameraToUnit := p_Unit;
end;

class function TAMap.PlaceFeatureOnMap(FeatureName: String;
  Position: TPosition; Turn: TTurn): Boolean;
var
  FeatureId : SmallInt;
  GridPlot : PPlotGrid;
  x, z : Integer;
  //FeatureDef : PFeatureDefStruct;
begin
  Result := False;
  FeatureId := FeatureName2ID(PAnsiChar(FeatureName));

  if FeatureId = -1 then
    FeatureId := LoadFeature(PAnsiChar(FeatureName));

  if FeatureId <> -1 then
  begin
    //FeatureDef := TAmem.FeatureDefId2Ptr(FeatureId);
    z := Position.Z div 16;
    x := Position.X div 16;
    GridPlot := GetGridPosPLOT(x, z);
    Result := (SpawnFeatureOnMap(GridPlot, FeatureId, @Position, @Turn, 10) <> nil);
  end;
end;

class function TAMap.RemoveMapFeature(X, Z: Integer;
  Method: Boolean): Boolean;
var
  GridPlot : PPlotGrid;
begin
  GridPlot := GetGridPosPLOT(X div 16, Z div 16);
  Result := FEATURES_Destroy(GridPlot, Method);
end;

class function TAMap.PositionInLOS(Player: PPlayerStruct;
  Position: PPosition): Boolean;
var
  X, Z : Integer;
  Pos : Cardinal;
begin
  Result := False;
  if PositionInPlayerMapped(Player, Position) then
  begin
    X := Position.X div 32;
    Z := (Position.Z - (Position.Y div 2)) div 32;
    if (X < Player.lLOS_Width) and
       (Z < Player.lLOS_Height) then
    begin
      Pos := Z * Player.lLOS_Width + X;
      Result := PByte(Cardinal(Player.LOS_MEMORY) + Pos)^ <> 0;
    end;
  end;
end;

end.