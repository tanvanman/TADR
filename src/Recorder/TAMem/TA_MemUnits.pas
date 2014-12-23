unit TA_MemUnits;

interface
uses
  TA_MemoryStructures;

type
  TAUnit = class
  public
    class Function GetHealth(p_Unit: PUnitStruct) : Word;
    class function MakeDamage(Makerp_Unit: PUnitStruct; Takerp_Unit: PUnitStruct;
      DamageType: TDmgType; Amount: Cardinal) : Word;
    class Function GetCloak(p_Unit: PUnitStruct) : LongWord;
    class procedure SetCloak(p_Unit: PUnitStruct; cloak: Word);

    class function GetUnitX(p_Unit: PUnitStruct) : Word;
    class function GetUnitZ(p_Unit: PUnitStruct) : Word;
    class function GetUnitY(p_Unit: PUnitStruct) : Word;

    class function GetTurnX(p_Unit: PUnitStruct) : Word;
    class procedure SetTurnX(p_Unit: PUnitStruct; X: Word);
    class function GetTurnZ(p_Unit: PUnitStruct) : Word;
    class procedure SetTurnZ(p_Unit: PUnitStruct; Z: Word);
    class function GetTurnY(p_Unit: PUnitStruct) : Word;
    class procedure SetTurnY(p_Unit: PUnitStruct; Y: Word);

    class function GetMovementClass(p_Unit: PUnitStruct) : Pointer;
    class procedure SetUnitInfo(p_Unit: PUnitStruct; NewUnitInfo: PUnitInfo; Broadcast: Boolean = False);

    class function GetWeapon(p_Unit: PUnitStruct; index: Cardinal) : Cardinal;
    class function SetWeapon(p_Unit: PUnitStruct; index: Cardinal; NewWeaponID: Cardinal) : Boolean;
    class function GetAttackerID(p_Unit: PUnitStruct) : LongWord;
    class function GetTransporterUnit(p_Unit: PUnitStruct) : Pointer;
    class function GetTransportingUnit(p_Unit: PUnitStruct) : Pointer;
    class function GetPriorUnit(p_Unit: PUnitStruct) : Pointer;
    class function GetRandomFreePiece(p_Unit: PUnitStruct; piecemin, piecemax : Integer) : Cardinal;
    class procedure AttachDetachUnit(Transported, Transporter: Pointer; Piece: Byte; Attach: Boolean);
    class function GetUnitAttachedTo(p_Unit: PUnitStruct; Piece: Byte) : Pointer;
    class function GetLoadWeight(p_Unit: PUnitStruct) : Integer;
    class function GetLoadCurAmount(p_Unit: PUnitStruct) : Integer;

    class function UpdateLos(p_Unit: PUnitStruct) : LongWord;
    class function FireWeapon(AttackerPtr : Pointer; WhichWeap : Byte; Targetp_Unit: PUnitStruct; TargetShortPosition : TShortPosition) : Integer;

    { position stuff }
    class function AtMouse: Pointer;
    class function AtPosition(Position: PPosition) : Cardinal;
    class function Position2Grid(Position: TPosition; UnitInfo: Pointer; out GridPosX, GridPosZ: Word ) : Boolean;
    class function GetCurrentSpeedPercent(p_Unit: PUnitStruct) : Cardinal;
    class function GetCurrentSpeedVal(p_Unit: PUnitStruct) : Cardinal;
    class procedure SetCurrentSpeed(p_Unit: PUnitStruct; NewSpeed: Cardinal);
    class function TestUnloadPosition(Unitinfo: PUnitInfo; Position: TPosition) : Boolean;

    { creating and killing unit }
    class function TestBuildSpot(PlayerIndex: Byte; UnitInfo: Pointer; nPosX, nPosZ: Word) : Boolean;
    class function IsPlantYardOccupied(BuilderPtr: PUnitStruct; State: Integer) : Boolean;
    class function TestAttachAtGridSpot(UnitInfo : Pointer; nPosX, nPosZ : Word) : Boolean;
    class function CreateUnit(OwnerIndex: LongWord; UnitInfo: PUnitInfo; Position: TPosition; Turn: PTurn; TurnZOnly, RandomTurnZ: Boolean; UnitState: LongWord) : Pointer;
    class Function GetBuildPercentLeft(p_Unit: PUnitStruct) : Cardinal;
    class Function GetMissingHealth(p_Unit: PUnitStruct) : Cardinal;
    class procedure Kill(p_Unit: PUnitStruct; deathtype: byte);
    class procedure SwapByKill(p_Unit: PUnitStruct; newUnitInfo: Pointer);

    { actions (orders, unit state) }
    class function GetCurrentOrderType(p_Unit: PUnitStruct) : TTAActionType;
    class function GetCurrentOrderParams(p_Unit: PUnitStruct; Par: Byte) : Integer;
    class function GetCurrentOrderState(p_Unit: PUnitStruct) : Cardinal;
    class function GetCurrentOrderPos(p_Unit: PUnitStruct) : Cardinal;
    class function GetCurrentOrderTargetUnit(p_Unit: PUnitStruct) : Pointer;

    class function EditCurrentOrderParams(p_Unit: PUnitStruct; Par: Byte; NewValue: LongWord) : Boolean;
    class function CreateMainOrder(p_Unit: PUnitStruct; Targetp_Unit: PUnitStruct; ActionType: TTAActionType; Position: PPosition; ShiftKey: Byte; Par1: LongWord; Par2: LongWord) : LongInt;

    { COB }
    class Function GetCOBDataPtr(p_Unit: PUnitStruct) : Pointer;
    class function CallCobProcedure(p_Unit: PUnitStruct; ProcName: String; Par1, Par2, Par3, Par4: PLongWord) : Cardinal;
    class function CallCobWithCallback(p_Unit: PUnitStruct; ProcName: String; Par1, Par2, Par3, Par4: LongWord) : Cardinal;
    class function GetCobString(p_Unit: PUnitStruct) : String;

    { id, owner, unit type etc. }
    class Function GetOwnerPtr(p_Unit: PUnitStruct) : Pointer;
    class Function GetOwnerIndex(p_Unit: PUnitStruct) : Integer;
    class Function GetId(p_Unit: PUnitStruct) : Word;
    class Function GetLongId(p_Unit: PUnitStruct) : Cardinal;
    class Function Id2Ptr(LongUnitId: Cardinal) : PUnitStruct;
    class Function Id2LongId(UnitId: Word) : Cardinal;
    class function GetUnitInfoId(p_Unit: PUnitStruct) : Word;
    class function GetUnitInfoCrc(p_Unit: PUnitStruct) : Cardinal;
    class function GetUnitInfoPtr(p_Unit: PUnitStruct) : Pointer;
    class Function IsOnThisComp(p_Unit: PUnitStruct; IncludeAI: Boolean) : Boolean;
    class function IsAllied(p_Unit: PUnitStruct; UnitId: LongWord) : Byte;
//    class Function IsRemoteIdLocal(p_Unit: PUnitStruct; remoteUnitId: PLongWord; out local: Boolean) : LongWord;

    class Function IsUnitTypeInCategory(CategoryType: TUnitCategories; UnitInfo: PUnitInfo; TargetUnitInfo: PUnitInfo) : Boolean;

    { cloning global template and setting its fields }
    class function GrantUnitInfo(p_Unit: PUnitStruct; ANewState: Byte; Broadcast: Boolean) : Boolean;
    class function GetUnitInfoField(p_Unit: PUnitStruct; fieldType: TUnitInfoExtensions) : LongWord;
    class function SetUnitInfoField(p_Unit: PUnitStruct; fieldType: TUnitInfoExtensions; value: Integer) : Boolean;
  end;

  TAUnits = class
  public
    class Function CreateMinions(p_Unit: PUnitStruct; Amount: Byte; UnitInfo: Pointer; Action: TTAActionType; ArrayId: Cardinal) : Integer;
    class procedure GiveUnit(Existingp_Unit: PUnitStruct; PlayerIdx: Byte);
    { searching units in game }
    class function CreateSearchFilter(Mask: Integer) : TUnitSearchFilterSet;
    class function GetRandomArrayId(ArrayType: Byte) : Word;
    class function SearchUnits(p_Unit: PUnitStruct; SearchId: LongWord; SearchType: Byte; MaxDistance: Integer; Filter: TUnitSearchFilterSet; UnitTypeFilter: Pointer ) : LongWord;
    class function UnitsIntoGetterArray(p_Unit: PUnitStruct; ArrayType: Byte; Id: LongWord; const UnitsArray: TFoundUnits) : Boolean;
    class procedure ClearSearchRec(Id: LongWord; ArrayType: Byte);
    class procedure RandomizeSearchRec(Id: LongWord; ArrayType: Byte);
    class function Distance(Pos1, Pos2 : PPosition) : Cardinal;
    class function CircleCoords(CenterPosition: TPosition; Radius: Integer; Angle: Integer; out x, z: Integer) : Boolean;
    class function UnitsFilterVsUnit(p_Unit: PUnitStruct; Filter: TUnitSearchFilterSet; Player: PPlayerStruct) : Boolean;
  end;

implementation
uses
  TA_MemoryLocations,
  TA_NetworkingMessages,
  TA_MemPlayers,
  TA_FunctionsU,
  UnitInfoExpand,
  COB_Extensions,
  idplay,
  Windows,
  Math;


// -----------------------------------------------------------------------------
// TAUnit
// -----------------------------------------------------------------------------

class Function TAUnit.GetHealth(p_Unit : PUnitStruct) : Word;
begin
result := p_Unit.nHealth;
end;

class function TAUnit.MakeDamage(Makerp_Unit: PUnitStruct; Takerp_Unit: PUnitStruct;
  DamageType: TDmgType; Amount: Cardinal) : Word;
var
  UnitInfoSt : PUnitInfo;
  Angle : Word;
  AtanX, AtanY : Integer;
begin
  UnitInfoSt := PUnitStruct(Takerp_Unit).p_UNITINFO;
  case DamageType of
    dtWeapon..dtParalyze :
      begin
        AtanY := PUnitStruct(Takerp_Unit).Position.z - PUnitStruct(Makerp_Unit).Position.z;
        AtanX := PUnitStruct(Takerp_Unit).Position.x - PUnitStruct(Makerp_Unit).Position.x;
        Angle := Word(TA_Atan2(AtanY, AtanX));
        UNITS_MakeDamage(Makerp_Unit, Takerp_Unit, Amount, Ord(DamageType), Angle);
      end;
    dtHeal :
      begin
        if PUnitStruct(Takerp_Unit).nHealth < UnitInfoSt.lMaxHP then
          UNITS_MakeDamage(Makerp_Unit, Takerp_Unit, Amount, 10, 0);
      end;
    else
      UNITS_MakeDamage(Makerp_Unit, Takerp_Unit, Amount, Ord(DamageType), 0);
  end;
  Result := TAUnit.GetHealth(Takerp_Unit);
end;

const
  Cloak_BitMask = $4;
  CloakUnitStateMask_BitMask = $800;
Class function TAUnit.GetCloak(p_Unit : PUnitStruct) : LongWord;
var
  Bitfield : Word;
begin
  Bitfield := p_Unit.nUnitStateMaskBas;
  if (Bitfield and Cloak_BitMask) = Cloak_BitMask then
    Result := 1
  else
    Result := 0;
end;

class procedure TAUnit.SetCloak(p_Unit : PUnitStruct; Cloak : Word);
begin
  if Cloak = 1 then
    p_Unit.lUnitStateMask := p_Unit.lUnitStateMask or CloakUnitStateMask_BitMask
  else
    p_Unit.lUnitStateMask := p_Unit.lUnitStateMask and not CloakUnitStateMask_BitMask;
end;

class function TAUnit.GetUnitX(p_Unit : PUnitStruct) : Word;
begin
result := p_Unit.Position.X;
end;

class function TAUnit.GetUnitZ(p_Unit : PUnitStruct) : Word;
begin
result := p_Unit.Position.Z;
end;

class function TAUnit.GetUnitY(p_Unit : PUnitStruct) : Word;
begin
result := p_Unit.Position.Y;
end;

class function TAUnit.GetTurnX(p_Unit : PUnitStruct) : Word;
begin
result := p_Unit.Turn.X;
end;

class procedure TAUnit.SetTurnX(p_Unit : PUnitStruct; X : Word);
begin
p_Unit.Turn.X:= X;
end;

class function TAUnit.GetTurnZ(p_Unit : PUnitStruct) : Word;
begin
result := p_Unit.Turn.Z;
end;

class procedure TAUnit.SetTurnZ(p_Unit : PUnitStruct; Z : Word);
begin
p_Unit.Turn.Z:= Z;
end;

class function TAUnit.GetTurnY(p_Unit : PUnitStruct) : Word;
begin
result := p_Unit.Turn.Y;
end;

class procedure TAUnit.SetTurnY(p_Unit : PUnitStruct; Y : Word);
begin
p_Unit.Turn.Y:= Y;
end;

class function TAUnit.GetMovementClass(p_Unit : PUnitStruct) : Pointer;
begin
result := p_Unit.p_MovementClass;
end;

class procedure TAUnit.SetUnitInfo(p_Unit: PUnitStruct; NewUnitInfo: PUnitInfo; Broadcast: Boolean = False);
begin
  p_Unit.nUnitInfoID := NewUnitInfo.nCategory;
  p_Unit.p_UNITINFO := NewUnitInfo;

  if Broadcast and TAData.NetworkLayerEnabled then
    GlobalDPlay.Broadcast_UnitInfoSwap(TAUnit.GetID(p_Unit), TAMem.Crc32ToCrc24(NewUnitInfo.CRC_FBI));
end;

class function TAUnit.GetWeapon(p_Unit: PUnitStruct; index: Cardinal) : Cardinal;
var
  Weapon: Pointer;
begin
  Result:= 0;
  if p_Unit <> nil then
  begin
    case index of
      WEAPON_PRIMARY   : Weapon := p_Unit.UnitWeapons[0].p_Weapon;
      WEAPON_SECONDARY : Weapon := p_Unit.UnitWeapons[1].p_Weapon;
      WEAPON_TERTIARY  : Weapon := p_Unit.UnitWeapons[2].p_Weapon;
      else Weapon := nil;
    end;
    if Weapon <> nil then
      Result := TAWeapon.GetWeaponID(Weapon);
  end;
end;

class function TAUnit.SetWeapon(p_Unit: PUnitStruct; index: Cardinal; NewWeaponID: Cardinal) : Boolean;
var
  Weapon: Pointer;
begin
  Result:= False;
  if p_Unit <> nil then
  begin
    Weapon:= TAWeapon.WeaponId2Ptr(NewWeaponID);
    case index of
      WEAPON_PRIMARY   : p_Unit.UnitWeapons[0].p_Weapon := Weapon;
      WEAPON_SECONDARY : p_Unit.UnitWeapons[1].p_Weapon := Weapon;
      WEAPON_TERTIARY  : p_Unit.UnitWeapons[2].p_Weapon := Weapon;
    end;
    Result:= True;
  end;
end;

class function TAUnit.GetAttackerID(p_Unit: PUnitStruct) : LongWord;
begin
result:= 0;
if p_Unit.p_Attacker <> nil then
  result:= TAUnit.GetId(p_Unit.p_Attacker);
end;

class function TAUnit.GetTransporterUnit(p_Unit: PUnitStruct) : Pointer;
begin
result:= nil;
if p_Unit.p_TransporterUnit <> nil then
  result:= p_Unit.p_TransporterUnit;
end;

class function TAUnit.GetTransportingUnit(p_Unit: PUnitStruct) : Pointer;
begin
result:= nil;
if p_Unit.p_TransportedUnit <> nil then
  result:= p_Unit.p_TransportedUnit;
end;

class function TAUnit.GetPriorUnit(p_Unit: PUnitStruct) : Pointer;
begin
  result:= p_Unit.p_PriorUnit;
end;

class function TAUnit.GetRandomFreePiece(p_Unit: PUnitStruct; piecemin, piecemax : Integer) : Cardinal;
var
 i, j : word;
 MaxElem : Integer;
 X : Cardinal;
 Pieces : array of ShortInt;
 PiecesLength : Integer;
 TransportedUnit : Pointer;
 AttachedToPiece : ShortInt;
begin
  Result := 0;
  Randomize;

  SetLength(Pieces, piecemax - piecemin + 1);
  for i := 0 to High(Pieces) do
  begin
    Pieces[i] := piecemin + i;
  end;

  TransportedUnit := TAUnit.GetTransportingUnit(p_Unit);
  if TransportedUnit = nil then
  begin
    result := Random(piecemax - piecemin + 1) + piecemin;
    Exit;
  end;
  while (TransportedUnit <> nil) do
  begin
    if TransportedUnit <> nil then
    begin
      AttachedToPiece := PUnitStruct(TransportedUnit).cAttachedToPiece;
      if AttachedToPiece <> 0 then
      begin
        for i := 0 to High(Pieces) do
        begin
          if Pieces[i] = AttachedToPiece then
          begin
            PiecesLength := Length(Pieces);
            for j := i + 1 to PiecesLength - 1 do
              Pieces[j - 1] := Pieces[j];
            SetLength(Pieces, PiecesLength - 1);
            Break;
          end;
        end;
      end;
    end;
    TransportedUnit := TAUnit.GetPriorUnit(TransportedUnit);
  end;

  MaxElem := High(Pieces);
  if High(Pieces) > 0 then
  begin
    for i := MaxElem downto 0 do
    begin
      j := Random(i) + 1;
      if not (i = j) then
      begin
        X := Pieces[i];
        Pieces[i] := Pieces[j];
        Pieces[j] := X;
      end;
    end;
    Result := Pieces[High(Pieces)];
  end else
    if High(Pieces) <> -1 then
      Result := Pieces[High(Pieces)];
end;

class procedure TAUnit.AttachDetachUnit(Transported, Transporter: Pointer; Piece: Byte; Attach: Boolean);
begin
  if Transported <> nil then
  begin
    if PUnitStruct(Transported).p_TransporterUnit <> nil then
      TA_AttachDetachUnit(Transported, nil, -1, 2);               // pop in transported units array
    if Attach then
      TA_AttachDetachUnit(Transported, Transporter, Piece, 0)
    else
      TA_AttachDetachUnit(Transported, nil, -1, 1);
  end;
end;

class function TAUnit.GetUnitAttachedTo(p_Unit: PUnitStruct; Piece: Byte) : Pointer;
var
  MaxUnitId, UnitId : Cardinal;
  TestedUnit : PUnitStruct;
begin
  Result := nil;

  MaxUnitId := TAData.MaxUnitsID;
  for UnitId := 1 to MaxUnitId do
  begin
    TestedUnit := TAUnit.Id2Ptr(UnitId);
    if TAUnit.GetTransporterUnit(TestedUnit) = p_Unit then
    begin
      if TestedUnit.cAttachedToPiece = ShortInt(Piece) then
      begin
        Result := TestedUnit;
        Exit;
      end;
    end;
  end;
end;

class function TAUnit.GetLoadWeight(p_Unit: PUnitStruct) : Integer;
var
  TransportedIterr : Pointer;
  CurLoadWeight : Integer;
begin
  CurLoadWeight := 0;
  TransportedIterr := TAUnit.GetTransportingUnit(p_Unit);
  while TransportedIterr <> nil do
  begin
    if TAUnit.GetTransporterUnit(TransportedIterr) = p_Unit then
    begin
      CurLoadWeight := CurLoadWeight +
                       Round(PUnitInfo(TAUnit.GetUnitInfoPtr(TransportedIterr)).lBuildCostMetal);
    end;
    TransportedIterr := TAUnit.GetPriorUnit(TransportedIterr);
  end;
  Result := CurLoadWeight;
end;

class function TAUnit.GetLoadCurAmount(p_Unit: PUnitStruct) : Integer;
var
  TransportedIterr : Pointer;
  i : Integer;
begin
  i := 0;
  TransportedIterr := TAUnit.GetTransportingUnit(p_Unit);
  while TransportedIterr <> nil do
  begin
    if TAUnit.GetTransporterUnit(TransportedIterr) = p_Unit then
    begin
      Inc(i);
    end;
    TransportedIterr := TAUnit.GetPriorUnit(TransportedIterr);
  end;
  Result := i;
end;

class function TAUnit.UpdateLos(p_Unit: PUnitStruct) : LongWord;
begin
  UNITS_RebuildLOS(p_Unit);
  //Result := TA_UpdateLOS(TAUnit.GetOwnerIndex(p_Unit), 0);
  Result := TA_UpdateLOS(0);
end;

class function TAUnit.FireWeapon(AttackerPtr : Pointer; WhichWeap : Byte;
  Targetp_Unit: PUnitStruct; TargetShortPosition : TShortPosition) : Integer;
var
  WeapPtr : PWeaponDef;
  UnitSt : PUnitStruct;
  WeapTypeMask : Cardinal;
  WeapType : Integer;
  WeapStatePtr : Pointer;
  WeapTargetIdPtr : Pointer;
  TargetPosition : TPosition;
begin
  Result := 0;
  WeapType := -1;
  UnitSt := AttackerPtr;
  case WhichWeap of
    WEAPON_PRIMARY : WeapPtr := UnitSt.UnitWeapons[0].p_Weapon;
    WEAPON_SECONDARY : WeapPtr := UnitSt.UnitWeapons[1].p_Weapon;
    WEAPON_TERTIARY : WeapPtr := UnitSt.UnitWeapons[2].p_Weapon;
    else WeapPtr := nil;
  end;

  if (WeapPtr <> nil) and (AttackerPtr <> nil) then
  begin
    WeapTypeMask := WeapPtr.lWeaponTypeMask;
    if ((WeapTypeMask shr 19) and 1) = 1 then
    begin
      WeapType := 0;
    end else
    begin
      if ((WeapTypeMask shr 4) and 1) = 1 then
      begin
        WeapType := 1;
      end else
      begin
        if  ((WeapTypeMask and 1) = 1) or ((WeapTypeMask and $100000) = $100000) then
        begin
          WeapType := 3;
        end else
        begin
          WeapTypeMask := WeapTypeMask shr 8;
          if (WeapTypeMask and 1) = 1 then
            WeapType := 2;
        end;
      end;
    end;

    if WeapType <> -1 then
    begin
      if WeapPtr.p_FireCallback <> nil then
      begin
        case WhichWeap of
          WEAPON_PRIMARY :
            begin
              WeapStatePtr := @UnitSt.UnitWeapons[0].lState;
              WeapTargetIDPtr := @UnitSt.UnitWeapons[0].nTargetID;
            end;
          WEAPON_SECONDARY :
            begin
              WeapStatePtr := @UnitSt.UnitWeapons[1].lState;
              WeapTargetIDPtr := @UnitSt.UnitWeapons[1].nTargetID;
            end;
          WEAPON_TERTIARY :
            begin
              WeapStatePtr := @UnitSt.UnitWeapons[2].lState;
              WeapTargetIDPtr := @UnitSt.UnitWeapons[2].nTargetID;
            end;
          else begin
            WeapStatePtr := nil;
            WeapTargetIDPtr := nil;
          end;
        end;
        PLongWord(WeapStatePtr)^ := PLongWord(WeapStatePtr)^ or $1;
        if (Targetp_Unit <> nil) then
          TargetPosition := PUnitStruct(Targetp_Unit).Position
        else
          GetTPosition(TargetShortPosition.X, TargetShortPosition.Z, TargetPosition);

        case WeapType of
          0 : Result := fire_callback0(AttackerPtr, WeapTargetIDPtr, Targetp_Unit, @TargetPosition);
          1 : Result := fire_callback1(AttackerPtr, WeapTargetIDPtr, Targetp_Unit, @TargetPosition);
          2 : Result := fire_callback2(AttackerPtr, WeapTargetIDPtr, Targetp_Unit, @TargetPosition);
          3 : Result := fire_callback3(AttackerPtr, WeapTargetIDPtr, Targetp_Unit, @TargetPosition);
        end;
        PLongWord(WeapStatePtr)^ := PLongWord(WeapStatePtr)^ and not $1;
      end;
    end;
  end;
end;

class function TAUnit.AtMouse: Pointer;
var
  Id: LongWord;
begin
  Result:= nil;
  Id := GetUnitAtMouse;
  if (Id <> 0) and (TAUnit.Id2Ptr(Id) <> TAUnit.Id2Ptr(0)) then
    Result:= TAUnit.Id2Ptr(Id);
end;

class function TAUnit.AtPosition(Position: PPosition) : Cardinal;
var
  PlotGrid : PPlotGrid;
begin
  Result := 0;
  if Position <> nil then
  begin
    PlotGrid := Position2GridPlot(Position);
    Result := Word(PCardinal(PlotGrid)^);
  end;
end;

class function TAUnit.Position2Grid(Position: TPosition; UnitInfo: Pointer; out GridPosX, GridPosZ: Word ) : Boolean;
var
  PosX, PosZ: Integer;
begin
 // Result := False;
  try
    PosX := MakeLong(Position.x_, Position.X);
    GridPosX := (PosX - (SmallInt(PUnitInfo(UnitInfo).nFootPrintX shl 19)) + $80000) shr 20;
    //GridPosX := (PosX - (PUnitInfo(UnitInfo).nFootPrintX shl 19) + $80000) shr 20;
    PosZ := MakeLong(0, Position.Z);
    GridPosZ := (PosZ - (SmallInt(PUnitInfo(UnitInfo).nFootPrintZ shl 19)) + $80000) shr 20;
    //GridPosZ := (PosZ - (PUnitInfo(UnitInfo).nFootPrintZ shl 19) + $80000) shr 20;
  finally
    Result := True;
  end;
end;

class function TAUnit.GetCurrentSpeedPercent(p_Unit: PUnitStruct) : Cardinal;
begin
  result := 0;
  if p_Unit.p_MovementClass <> nil then
  begin
    if p_Unit.lSfxOccupy = 4 then
      result := Trunc(((p_Unit.p_MovementClass.lCurrentSpeed) /
                        (p_Unit.p_UNITINFO.lMaxSpeedRaw)) * 100)
    else
      result := Trunc(((p_Unit.p_MovementClass.lCurrentSpeed) /
                        Trunc((p_Unit.p_UNITINFO.lMaxSpeedRaw) / 2)) * 100);
    if result > 100 then
      result := 100;
  end;
end;

class function TAUnit.GetCurrentSpeedVal(p_Unit: PUnitStruct) : Cardinal;
begin
  result := 0;
  if p_Unit.p_MovementClass <> nil then
  begin
    result := (PMoveClass(p_Unit.p_MovementClass).lCurrentSpeed);
  end;
end;

class procedure TAUnit.SetCurrentSpeed(p_Unit: PUnitStruct; NewSpeed: Cardinal); // percentage
begin
  if p_Unit.p_MovementClass <> nil then
    PMoveClass(p_Unit.p_MovementClass).lCurrentSpeed := NewSpeed;
end;

class function TAUnit.TestUnloadPosition(UnitInfo: PUnitInfo; Position: TPosition) : Boolean;
var
  nPosX, nPosZ: Word;
begin
  Position2Grid(Position, UnitInfo, nPosX, nPosZ);
  result := TAUnit.TestAttachAtGridSpot(UnitInfo, nPosX, nPosZ);
end;

class function TAUnit.TestBuildSpot(PlayerIndex: Byte; UnitInfo: Pointer; nPosX, nPosZ: Word ) : Boolean;
var
  TestPos: LongWord;
  UnitInfoSt: PUnitInfo;
  PlayerSt: PPlayerStruct;
  GridPosX, GridPosZ: Word;
  Pos : TPosition;
begin
  Result:= False;
  UnitInfoSt := UnitInfo;
  PlayerSt := TAPlayer.GetPlayerByIndex(PlayerIndex);

  Pos.X := nPosX;
  Pos.Z := nPosZ;
  if (UnitInfoSt <> nil) and (PlayerSt <> nil) then
    if TAUnit.Position2Grid(Pos, UnitInfoSt, GridPosX, GridPosZ ) then
    begin
      TestPos := MakeLong(GridPosX, GridPosZ);
      Result := (TestGridSpot(UnitInfoSt, TestPos, 0, PlayerSt) = 1);
    end;
end;

class function TAUnit.IsPlantYardOccupied(BuilderPtr: PUnitStruct; State: Integer) : Boolean;
begin
  Result := CanCloseOrOpenYard(BuilderPtr, State);
end;

class function TAUnit.TestAttachAtGridSpot(UnitInfo : Pointer; nPosX, nPosZ : Word) : Boolean;
var
  GridPos : Cardinal;
begin
  GridPos := MakeLong(nPosX, nPosZ);
  Result := CanAttachAtGridSpot(UnitInfo, 0, GridPos, 1);
end;

class function TAUnit.CreateUnit( OwnerIndex: LongWord;
                                  UnitInfo: PUnitInfo;
                                  Position: TPosition;
                                  Turn: PTurn;
                                  TurnZOnly: Boolean;
                                  RandomTurnZ: Boolean;
                                  UnitState: LongWord ) : Pointer;
var
  UnitSt: PUnitStruct;
begin
  if OwnerIndex = 10 then
    OwnerIndex := TAData.LocalPlayerID;
    
  Result := UNITS_Create( OwnerIndex,
                          UnitInfo.nCategory,
                          MakeLong(0, Position.X),
                          MakeLong(0, Position.Y),
                          MakeLong(0, Position.Z),
                          1,
                          UnitState,
                          0 );



  if (Result <> nil) then
  begin
    UnitSt := Pointer(Result);
    if (Turn <> nil) then
    begin
      UnitSt.Turn.Z := Turn.Z;
      if not TurnZOnly then
      begin
        UnitSt.Turn.X := Turn.X;
        UnitSt.Turn.Y := Turn.Y;
      end;
    end else
    if RandomTurnZ then
    begin
      UnitSt.Turn.Z := Random(High(SmallInt));
    end;
  end;
end;

class Function TAUnit.GetBuildPercentLeft(p_Unit: PUnitStruct) : Cardinal;
begin
  if ( p_Unit.fBuildTimeLeft = 0.0 ) then
    Result := 0
  else
    Result := Trunc(1 - (p_Unit.fBuildTimeLeft * -99.0));
end;

class Function TAUnit.GetMissingHealth(p_Unit: PUnitStruct) : Cardinal;
var
  ShouldBe : Cardinal;
  IsCurr : Cardinal;
  UnitInfo : PUnitInfo;
begin
  Result := 0;
  if (p_Unit <> nil) then
    if p_Unit.nUnitInfoID <> 0 then
    begin
      UnitInfo := TAMem.UnitInfoId2Ptr(p_Unit.nUnitInfoID);
      if ( p_Unit.fBuildTimeLeft = 0.0 ) then
        Result := 0
      else begin
        ShouldBe := Round((1 - p_Unit.fBuildTimeLeft) * UnitInfo.lMaxHP);
        IsCurr := p_Unit.nHealth;
        if IsCurr < ShouldBe then
          Result := ShouldBe - IsCurr;
      end;
    end;
end;

class procedure TAUnit.Kill(p_Unit: PUnitStruct; deathtype: byte);
begin
  if p_Unit <> nil then
  begin
    case deathtype of
      0 : UNITS_MakeDamage(nil, p_Unit, 30000, 3, 0);
      1 : UnitExplosion(p_Unit, 0);
      2 : UnitExplosion(p_Unit, 1);
      3 : UNITS_MakeDamage(nil, p_Unit, 30000, 4, 0);
      4 : TAUnit.CreateMainOrder(p_Unit, nil, Action_SelfDestruct, nil, 1, 0, 0);
      5 : p_Unit.lUnitStateMask:= p_Unit.lUnitStateMask or $4000;
    end;
    if (deathtype = 1) or (deathtype = 2) then
      UNITS_MakeDamage(nil, p_Unit, 30000, 4, 0);
  end;
end;

class procedure TAUnit.SwapByKill(p_Unit: PUnitStruct; newUnitInfo: Pointer);
var
  UnitSt: PUnitStruct;
  Position: TPosition;
  Turn: TTurn;
begin
  if (p_Unit <> nil) and
     (newUnitInfo <> nil) then
  begin
    UnitSt := p_Unit;
    Position := UnitSt.Position;
    Turn := UnitSt.Turn;
    if TAUnit.CreateUnit(TAUnit.GetOwnerIndex(p_Unit), newUnitInfo, Position, @Turn, True, False, 1) <> nil then
      TAUnit.Kill(p_Unit, 3);
  end;
end;

class function TAUnit.GetCurrentOrderType(p_Unit: PUnitStruct) : TTAActionType;
var
  UnitSt: PUnitStruct;
begin
  UnitSt:= p_Unit;
  if UnitSt.p_MainOrder <> nil then
    Result := TTAActionType(PUnitOrder(UnitSt.p_MainOrder).cOrderType)
  else
    Result := Action_NoResult;
end;

class function TAUnit.GetCurrentOrderParams(p_Unit: PUnitStruct; Par: Byte) : Integer;
begin
  Result := 0;
  if p_Unit.p_MainOrder <> nil then
  begin
    if Par = 1 then
      Result := p_Unit.p_MainOrder.lPar1
    else
      Result := p_Unit.p_MainOrder.lPar2;
  end;
end;

class function TAUnit.GetCurrentOrderState(p_Unit: PUnitStruct) : Cardinal;
begin
  Result := 0;
  if p_Unit.p_MainOrder <> nil then
    Result := PUnitOrder(p_Unit.p_MainOrder).lOrder_State;
end;

class function TAUnit.GetCurrentOrderPos(p_Unit: PUnitStruct) : Cardinal;
var
  tempx, tempz : Word;
begin
  Result := 0;
  if p_Unit.p_MainOrder <> nil then
  begin
    tempx := PUnitOrder(p_Unit.p_MainOrder).Pos.X;
    tempz := PUnitOrder(p_Unit.p_MainOrder).Pos.Z;
    Result := MakeLong(tempz, tempx);
  end;
end;

class function TAUnit.GetCurrentOrderTargetUnit(p_Unit: PUnitStruct) : Pointer;
begin
  Result := nil;
  if p_Unit.p_MainOrder <> nil then
  begin
    Result := PUnitOrder(p_Unit.p_MainOrder).p_UnitTarget;
  end;
end;

class function TAUnit.EditCurrentOrderParams(p_Unit: PUnitStruct; Par: Byte; NewValue: LongWord) : Boolean;
begin
  Result := False;
  if p_Unit.p_MainOrder <> nil then
  begin
    if Par = 1 then
      PUnitOrder(p_Unit.p_MainOrder).lPar1 := NewValue
    else
      PUnitOrder(p_Unit.p_MainOrder).lPar2 := NewValue;
    Result := True;
  end;
end;

class function TAUnit.CreateMainOrder(p_Unit: PUnitStruct; Targetp_Unit: PUnitStruct; ActionType: TTAActionType; Position: PPosition; ShiftKey: Byte; Par1: LongWord; Par2: LongWord) : LongInt;
begin
  Result:= Order2Unit(Ord(ActionType), ShiftKey, p_Unit, Targetp_Unit, Position, Par1, Par2);
end;

class Function TAUnit.GetCOBDataPtr(p_Unit: PUnitStruct) : Pointer;
begin
result := p_Unit.p_UnitScriptsData;
end;

class function TAUnit.CallCobProcedure(p_Unit: PUnitStruct; ProcName: String; Par1, Par2, Par3, Par4: PLongWord) : Cardinal;
var
  ParamsCount: LongWord;
  UnitSt: PUnitStruct;
  lPar1, lPar2, lPar3, lPar4: LongWord;
begin
  Result := 0;
  if p_Unit <> nil then
  begin
    ParamsCount:= 0;
    if Par1 <> nil then
    begin
      lPar1 := PLongWord(Par1)^;
      Inc(ParamsCount);
    end else
      lPar1 := 0;
    if Par2 <> nil then
    begin
      lPar2 := PLongWord(Par2)^;
      Inc(ParamsCount);
    end else
      lPar2 := 0;
    if Par3 <> nil then
    begin
      lPar3 := PLongWord(Par3)^;
      Inc(ParamsCount);
    end else
      lPar3 := 0;
    if Par4 <> nil then
    begin
      lPar4 := PLongWord(Par4)^;
      Inc(ParamsCount);
    end else
      lPar4 := 0;

    UnitSt:= p_Unit;
    Result := Script_RunScript ( 0, 0, LongWord(UnitSt.p_UnitScriptsData),
                                 lPar4, lPar3, lPar2, lPar1,
                                 ParamsCount,
                                 0, 0,
                                 PAnsiChar(ProcName) );
  end;
end;

class function TAUnit.CallCobWithCallback(p_Unit: PUnitStruct; ProcName: String; Par1, Par2, Par3, Par4: LongWord) : Cardinal;
var
  UnitSt: PUnitStruct;
begin
  Result := 0;
  if p_Unit <> nil then
  begin
    UnitSt := p_Unit;
    Result := Script_ProcessCallback( nil, nil, LongWord(UnitSt.p_UnitScriptsData),
                                      @Par4, @Par3, @Par2, @Par1,
                                      PAnsiChar(ProcName) );
    if Result = 1 then
      Result := Par1;
  end;
end;

class function TAUnit.GetCobString(p_Unit: PUnitStruct) : String;
begin
end;

class Function TAUnit.GetOwnerPtr(p_Unit: PUnitStruct) : Pointer;
begin
result := p_Unit.p_Owner;
end;

class Function TAUnit.GetOwnerIndex(p_Unit: PUnitStruct) : Integer;
begin
  Result := 0;
  if p_Unit <> nil then
    Result := p_Unit.ucOwnerID;
end;

class Function TAUnit.GetId(p_Unit: PUnitStruct) : Word;
begin
  Result := 0;
  if p_Unit <> nil then
    result := Word(p_Unit.lUnitInGameIndex);
end;

class Function TAUnit.GetLongId(p_Unit : PUnitStruct) : Cardinal;
begin
result := p_Unit.lUnitInGameIndex;
end;

class Function TAUnit.Id2Ptr(LongUnitId: Cardinal) : PUnitStruct;
begin
  Result := nil;
  if LongUnitId <> 0 then
  begin
    if (Word(LongUnitId) <= TAData.MaxUnitsID) then
      result := Pointer(LongWord(TAData.UnitsArray_p) + SizeOf(TUnitStruct)*Word(LongUnitId));
  end;
end;

class Function TAUnit.Id2LongId(UnitId: Word) : Cardinal;
var
  p_Unit: PUnitStruct;
begin
  Result := 0;
  if TAData.UnitsArray_p <> nil then
  begin
    p_Unit := TAUnit.Id2Ptr(UnitId);
    if p_Unit.nUnitInfoID <> 0 then
      Result := p_Unit.lUnitInGameIndex;
  end;
end;

class function TAUnit.GetUnitInfoId(p_Unit: PUnitStruct) : Word;
begin
result:= p_Unit.nUnitInfoID;
end;

class function TAUnit.GetUnitInfoCrc(p_Unit: PUnitStruct) : Cardinal;
begin
result := 0;
if TAUnit.GetUnitInfoPtr(p_Unit) <> nil then
  result := PUnitInfo(TAUnit.GetUnitInfoPtr(p_Unit)).CRC_FBI;
end;

class function TAUnit.GetUnitInfoPtr(p_Unit: PUnitStruct) : Pointer;
begin
result:= p_Unit.p_UNITINFO;
end;

class Function TAUnit.IsOnThisComp(p_Unit: PUnitStruct; IncludeAI: Boolean) : Boolean;
var
  playerPtr : Pointer;
  TAPlayerType : TTAPlayerController;
begin
result:= False;
try
  playerPtr := TAUnit.GetOwnerPtr(Pointer(p_Unit));
  //playerPtr := TAPlayer.GetPlayerByIndex(TAunit.GetOwnerIndex(p_Unit));
  if playerPtr <> nil then
  begin
    TAPlayerType := TAPlayer.PlayerController(playerPtr);
    case TAPlayerType of
      Player_LocalHuman:
        result := True;
      Player_LocalAI:
        if IncludeAI then
          result := True;
    end;
  end;
except
  result := False;
end;
end;

class Function TAUnit.IsAllied(p_Unit: PUnitStruct; UnitId: LongWord) : Byte;
var
  Unit2Ptr, playerPtr: Pointer;
  playerindex: Integer;
begin
  playerPtr := TAUnit.GetOwnerPtr(p_Unit);
  Unit2Ptr := TAUnit.Id2Ptr(Word(UnitId));
  playerIndex := TAUnit.GetOwnerIndex(Unit2Ptr);
  result := BoolValues[TAPlayer.GetAlliedState(playerPtr,playerIndex)];
end;
 {
class Function TAUnit.IsRemoteIdLocal(p_Unit: PUnitStruct; remoteUnitId: PLongWord; out Local: Boolean) : LongWord;
begin
  Local:= False;
  Result:= 0;
  if remoteUnitId <> nil then
  begin
    Local:= False;
    Result:= Word(remoteUnitId);
  end else
  begin
    if p_Unit <> nil then
    begin
      Local:= IsOnThisComp(Pointer(p_Unit), True);
      Result:= TAUnit.GetLongId(Pointer(p_Unit));
    end;
  end;
end; }

class Function TAUnit.IsUnitTypeInCategory(CategoryType: TUnitCategories; UnitInfo: PUnitInfo; TargetUnitInfo: PUnitInfo) : Boolean;
var
  pCategory : Pointer;
  UnitInfoIdMask : Cardinal;
  CategoryMask : Cardinal;
begin
  Result := False;
  pCategory := nil;
  if (UnitInfo <> nil) and
     (TargetUnitInfo <> nil) then
  begin
    case CategoryType of
      ucsNoChase : pCategory := UnitInfo.p_NoChaseCategoryMaskArray;
      ucsPriBadTarget : pCategory := UnitInfo.p_WeaponPrimaryBadTargetCategoryArray;
      ucsSecBadTarget : pCategory := UnitInfo.p_WeaponSecondaryBadTargetCategoryArray;
      ucsTerBadTarget : pCategory := UnitInfo.p_WeaponSpecialBadTargetCategoryArray;
    end;
    if pCategory <> nil then
    begin
      UnitInfoIdMask := 1 shl (TargetUnitInfo.nCategory and $1F);
      CategoryMask := PCardinal(Cardinal(pCategory) + 4 * (TargetUnitInfo.nCategory shr 5))^;
      if (UnitInfoIdMask and CategoryMask) <> 0 then
        Result := True;
    end;
  end;
end;

class function TAUnit.GrantUnitInfo(p_Unit: PUnitStruct; ANewState: Byte; Broadcast: Boolean) : Boolean;
var
  NewUnitInfo : PUnitInfo;
  UnitID : Word;
begin
  Result := False;
  UnitID := TAUnit.GetId(p_Unit);

  if ANewState = 1 then
  begin
    NewUnitInfo := MEM_Alloc(SizeOf(TUnitInfo));
    if NewUnitInfo <> nil then
    begin
      CopyMemory(NewUnitInfo, p_Unit.p_UNITINFO, SizeOf(TUnitInfo));
      CustomUnitFieldsArr[UnitID].UnitInfo := NewUnitInfo;
      p_Unit.p_UNITINFO := NewUnitInfo;
    end;
  end else
  begin
    p_Unit.p_UNITINFO := TAMem.UnitInfoId2Ptr(p_Unit.nUnitInfoID);
    if CustomUnitFieldsArr[TAUnit.GetId(p_Unit)].UnitInfo <> nil then
    begin
      FreeCustomUnitInfo(p_Unit);
      CustomUnitFieldsArr[TAUnit.GetId(p_Unit)].UnitInfo := nil;
    end;
  end;

  if Broadcast and TAData.NetworkLayerEnabled then
    GlobalDPlay.Broadcast_UnitGrantUnitInfo(TAUnit.GetId(p_Unit), ANewState);
end;

class function TAUnit.GetUnitInfoField(p_Unit: PUnitStruct; fieldType: TUnitInfoExtensions) : LongWord;
var
  UnitID : Word;
  UseTemplate: PUnitInfo;
begin
  Result := 0;
  UnitID := TAUnit.GetId(p_Unit);

  if CustomUnitFieldsArr[UnitID].UnitInfo = nil then
  begin
    if (p_Unit.p_UNITINFO <> nil) then
      UseTemplate := p_Unit.p_UNITINFO
    else
      Exit;
  end else
    UseTemplate := @CustomUnitFieldsArr[UnitID].UnitInfo;

  case fieldType of
    UNITINFO_MAXHEALTH       : Result := UseTemplate.lMaxHP;
    UNITINFO_HEALTIME        : Result := UseTemplate.nHealTime;

    UNITINFO_MAXSPEED        : Result := UseTemplate.lMaxSpeedRaw;
    UNITINFO_ACCELERATION    : Result := Trunc(UseTemplate.lAcceleration * 100);
    UNITINFO_BRAKERATE       : Result := Trunc(UseTemplate.lBrakeRate * 100);
    UNITINFO_TURNRATE        : Result := UseTemplate.nTurnRate;
    UNITINFO_CRUISEALT       : Result := UseTemplate.nCruiseAlt;
    UNITINFO_MANEUVERLEASH   : Result := UseTemplate.nManeuverLeashLen;
    UNITINFO_ATTACKRUNLEN    : Result := UseTemplate.nAttackRunLength;
    UNITINFO_MAXWATERDEPTH   : Result := UseTemplate.nMaxWaterDepth;
    UNITINFO_MINWATERDEPTH   : Result := UseTemplate.nMinWaterDepth;
    UNITINFO_MAXSLOPE        : Result := UseTemplate.cMaxSlope;
    UNITINFO_MAXWATERSLOPE   : Result := UseTemplate.cMaxWaterSlope;
    UNITINFO_WATERLINE       : Result := UseTemplate.cWaterLine;

    UNITINFO_TRANSPORTSIZE   : Result := UseTemplate.cTransportSize;
    UNITINFO_TRANSPORTCAP    : Result := UseTemplate.cTransportCap;

    UNITINFO_BANKSCALE       : Result := UseTemplate.lBankScale;
    UNITINFO_KAMIKAZEDIST    : Result := UseTemplate.nKamikazeDistance;
    UNITINFO_DAMAGEMODIFIER  : Result := UseTemplate.lDamageModifier;

    UNITINFO_WORKERTIME      : Result := UseTemplate.nWorkerTime;
    UNITINFO_BUILDDIST       : Result := UseTemplate.nBuildDistance;

    UNITINFO_SIGHTDIST       : Result := UseTemplate.nSightDistance;
    UNITINFO_RADARDIST       : Result := UseTemplate.nRadarDistance;
    UNITINFO_SONARDIST       : Result := UseTemplate.nSonarDistance;
    UNITINFO_MINCLOAKDIST    : Result := UseTemplate.nMinCloakDistance;
    UNITINFO_RADARDISTJAM    : Result := UseTemplate.nRadarDistanceJam;
    UNITINFO_SONARDISTJAM    : Result := UseTemplate.nSonarDistanceJam;

    UNITINFO_MAKESMETAL      : Result := UseTemplate.cMakesMetal;
    UNITINFO_FENERGYMAKE     : Result := Trunc(UseTemplate.fEnergyMake * 100);
    UNITINFO_FMETALMAKE      : Result := Trunc(UseTemplate.fMetalMake * 100);
    UNITINFO_FENERGYUSE      : Result := Abs(Trunc(UseTemplate.fEnergyUse * 100));
    UNITINFO_FMETALUSE       : Result := Abs(Trunc(UseTemplate.fMetalUse * 100));
    UNITINFO_FENERGYSTOR     : Result := UseTemplate.lEnergyStorage;
    UNITINFO_FMETALSTOR      : Result := UseTemplate.lMetalStorage;
    UNITINFO_FWINDGENERATOR  : Result := Trunc(UseTemplate.fWindGenerator * 100);
    UNITINFO_FTIDALGENERATOR : Result := Trunc(UseTemplate.fTidalGenerator * 100);
    UNITINFO_FCLOAKCOST      : Result := Trunc(UseTemplate.fCloakCost * 100);
    UNITINFO_FCLOAKCOSTMOVE  : Result := Trunc(UseTemplate.fCloakCostMoving * 100);

    UNITINFO_BUILDCOSTMETAL  : Result := Trunc(UseTemplate.lBuildCostMetal);
    UNITINFO_BUILDCOSTENERGY : Result := Trunc(UseTemplate.lBuildCostEnergy);

    UNITINFO_BUILDTIME       : Result := UseTemplate.lBuildTime;

// 1 ?
// 2 standingfireorder
// 3 ?
// 4 init_cloaked
// 5 downloadable
    UNITINFO_BUILDER         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 6)) > 0 );
// 7 zbuffer
    UNITINFO_STEALTH         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 8)) > 0 );
    UNITINFO_ISAIRBASE       : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 9)) > 0 );
    UNITINFO_TARGETTINGUPGRADE   : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 10)) > 0 );
    UNITINFO_CANFLY          : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 11)) > 0 );
    UNITINFO_CANHOVER        : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 12)) > 0 );
    UNITINFO_TELEPORTER      : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 13)) > 0 );
    UNITINFO_HIDEDAMAGE      : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 14)) > 0 );
    UNITINFO_SHOOTME         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 15)) > 0 );
    UNITINFO_HASWEAPONS      : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 16)) > 0 );
// 17 armoredstate
// 18 activatewhenbuilt
    UNITINFO_FLOATER         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 19)) > 0 );
    UNITINFO_UPRIGHT         : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 20)) > 0 );
    UNITINFO_BMCODE          : Result := UseTemplate.cBMCode;
    UNITINFO_AMPHIBIOUS      : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 21)) > 0 );
// 22 ?
// 23 internal command reload -> sub_42D1F0. probably reloads cob script
    UNITINFO_ISFEATURE       : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 24)) > 0 );
// 25 noshadow
    UNITINFO_IMMUNETOPARALYZER  : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 26)) > 0 );
    UNITINFO_HOVERATTACK     : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 27)) > 0 );
    UNITINFO_KAMIKAZE        : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 28)) > 0 );
    UNITINFO_ANTIWEAPONS     : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 29)) > 0 );
    UNITINFO_DIGGER          : Result := Byte((UseTemplate.UnitTypeMask and (1 shl 30)) > 0 );
// 31 has GUI (does gui file in guis folder exists with name of tested unit) ? sub_42d2e0

    UNITINFO_ONOFFABLE       : Result := Byte((UseTemplate.UnitTypeMask2 and 4) > 0 );
    UNITINFO_CANSTOP         : Result := Byte((UseTemplate.UnitTypeMask2 and 8) > 0 );
    UNITINFO_CANATTACK       : Result := Byte((UseTemplate.UnitTypeMask2 and 16) > 0 );
    UNITINFO_CANGUARD        : Result := Byte((UseTemplate.UnitTypeMask2 and 32) > 0 );
    UNITINFO_CANPATROL       : Result := Byte((UseTemplate.UnitTypeMask2 and 64) > 0 );
    UNITINFO_CANMOVE         : Result := Byte((UseTemplate.UnitTypeMask2 and 128) > 0 );
    UNITINFO_CANLOAD         : Result := Byte((UseTemplate.UnitTypeMask2 and 256) > 0 );
    UNITINFO_CANRECLAMATE    : Result := Byte((UseTemplate.UnitTypeMask2 and 1024) > 0 );
    UNITINFO_CANRESURRECT    : Result := Byte((UseTemplate.UnitTypeMask2 and 2048) > 0 );
    UNITINFO_CANCAPTURE      : Result := Byte((UseTemplate.UnitTypeMask2 and 4096) > 0 );
    UNITINFO_CANDGUN         : Result := Byte((UseTemplate.UnitTypeMask2 and 16384) > 0 );
    UNITINFO_SHOWPLAYERNAME  : Result := Byte((UseTemplate.UnitTypeMask2 and 131072) > 0 );
    UNITINFO_COMMANDER       : Result := Byte((UseTemplate.UnitTypeMask2 and 262144) > 0 );
    UNITINFO_CANTBERANSPORTED: Result := Byte((UseTemplate.UnitTypeMask2 and 524288) > 0 );
    UNITINFO_FOOTPRINTX      : Result := UseTemplate.nFootPrintX;
    UNITINFO_FOOTPRINTZ      : Result := UseTemplate.nFootPrintZ;
  end;
end;

class function TAUnit.SetUnitInfoField(p_Unit: PUnitStruct; fieldType: TUnitInfoExtensions; value: Integer) : Boolean;
  procedure SetUnitTypeMask(UnitID: Word; unittypemask: byte; onoff: boolean; mask: Cardinal);
  begin
    case onoff of
    False :
      begin
      if unittypemask = 0 then
        CustomUnitFieldsArr[UnitID].UnitInfo.UnitTypeMask := CustomUnitFieldsArr[UnitID].UnitInfo.UnitTypeMask and not mask
      else
        CustomUnitFieldsArr[UnitID].UnitInfo.UnitTypeMask2 := CustomUnitFieldsArr[UnitID].UnitInfo.UnitTypeMask2 and not mask;
      end;
    True :
      begin
      if unittypemask = 0 then
        CustomUnitFieldsArr[UnitID].UnitInfo.UnitTypeMask := CustomUnitFieldsArr[UnitID].UnitInfo.UnitTypeMask or mask
      else
        CustomUnitFieldsArr[UnitID].UnitInfo.UnitTypeMask2 := CustomUnitFieldsArr[UnitID].UnitInfo.UnitTypeMask2 or mask;
      end;
    end;
  end;

var
  UnitID : Word;
  MovementClassStruct : PMoveInfoClassStruct;
  Z : Word;
  UnitInfo : PUnitInfo;
begin
  Result := False;

  UnitID := TAUnit.GetId(p_Unit);
  UnitInfo := CustomUnitFieldsArr[UnitID].UnitInfo;
  if UnitInfo <> nil then
  begin
    case fieldType of
      UNITINFO_SOUNDCTGR : UnitInfo.nSoundCategory := Word(value);
      UNITINFO_MOVEMENTCLASS_SAFE..UNITINFO_MOVEMENTCLASS :
        begin
        // movement class information is stored in both - specific and global template
        // we fix both of them to make TA engine happy
        UnitInfo.p_MovementClass := TAMem.MovementClassId2Ptr(Word(value));
        MovementClassStruct := UnitInfo.p_MovementClass;
        if MovementClassStruct.pName <> nil then
        begin
          UnitInfo.nMaxWaterDepth := MovementClassStruct.nMaxWaterDepth;
          UnitInfo.nMinWaterDepth := MovementClassStruct.nMinWaterDepth;
          UnitInfo.cMaxSlope := MovementClassStruct.cMaxSlope;
          UnitInfo.cMaxWaterSlope := MovementClassStruct.cMaxWaterSlope;
          if FieldType = UNITINFO_MOVEMENTCLASS_SAFE then
          begin
            Z := p_Unit.Turn.Z;
            UNITS_CreateMoveClass(TAUnit.Id2Ptr(Word(UnitID)));
            p_Unit.Turn.Z := Z;
          end;
        end else
            Exit;
        end;
      UNITINFO_MAXHEALTH       : UnitInfo.lMaxHP := Word(value);
      UNITINFO_HEALTIME        : UnitInfo.nHealTime := Word(value);

      UNITINFO_MAXSPEED        : UnitInfo.lMaxSpeedRaw := value;
      UNITINFO_ACCELERATION    : UnitInfo.lAcceleration := value / 100;
      UNITINFO_BRAKERATE       : UnitInfo.lBrakeRate := value / 100;
      UNITINFO_TURNRATE        : UnitInfo.nTurnRate := Word(value);
      UNITINFO_CRUISEALT       : UnitInfo.nCruiseAlt := Word(value);
      UNITINFO_MANEUVERLEASH   : UnitInfo.nManeuverLeashLen := Word(value);
      UNITINFO_ATTACKRUNLEN    : UnitInfo.nAttackRunLength := Word(value);
      UNITINFO_MAXWATERDEPTH   : UnitInfo.nMaxWaterDepth := SmallInt(value);
      UNITINFO_MINWATERDEPTH   : UnitInfo.nMinWaterDepth := SmallInt(value);
      UNITINFO_MAXSLOPE        : UnitInfo.cMaxSlope := ShortInt(value);
      UNITINFO_MAXWATERSLOPE   : UnitInfo.cMaxWaterSlope := ShortInt(value);
      UNITINFO_WATERLINE       : UnitInfo.cWaterLine := Byte(value);

      UNITINFO_TRANSPORTSIZE   : UnitInfo.cTransportSize := Byte(value);
      UNITINFO_TRANSPORTCAP    : UnitInfo.cTransportCap := Byte(value);

      UNITINFO_BANKSCALE       : UnitInfo.lBankScale := value;
      UNITINFO_KAMIKAZEDIST    : UnitInfo.nKamikazeDistance := Word(value);
      UNITINFO_DAMAGEMODIFIER  : UnitInfo.lDamageModifier := value;

      UNITINFO_WORKERTIME      : UnitInfo.nWorkerTime := Word(value);
      UNITINFO_BUILDDIST       : UnitInfo.nBuildDistance := Word(value);

      UNITINFO_SIGHTDIST :
        begin
        UnitInfo.nSightDistance := Word(value);
        TAUnit.UpdateLos(TAUnit.Id2Ptr(Word(UnitID)));
        end;
      UNITINFO_RADARDIST :
        begin
        UnitInfo.nRadarDistance := Word(value);
        TAUnit.UpdateLos(TAUnit.Id2Ptr(Word(UnitID)));
        end;
      UNITINFO_SONARDIST       : UnitInfo.nSonarDistance := Word(value);
      UNITINFO_MINCLOAKDIST    : UnitInfo.nMinCloakDistance := Word(value);
      UNITINFO_RADARDISTJAM    : UnitInfo.nRadarDistanceJam := Word(value);
      UNITINFO_SONARDISTJAM    : UnitInfo.nSonarDistanceJam := Word(value);

      UNITINFO_MAKESMETAL      : UnitInfo.cMakesMetal := Byte(value);
      UNITINFO_FENERGYMAKE     : UnitInfo.fEnergyMake := value / 100;
      UNITINFO_FMETALMAKE      : UnitInfo.fMetalMake := value / 100;
      UNITINFO_FENERGYUSE      : UnitInfo.fEnergyUse := value / 100;
      UNITINFO_FMETALUSE       : UnitInfo.fMetalUse := value / 100;
      UNITINFO_FENERGYSTOR     : UnitInfo.lEnergyStorage := value;
      UNITINFO_FMETALSTOR      : UnitInfo.lEnergyStorage := value;
      UNITINFO_FWINDGENERATOR  : UnitInfo.fWindGenerator := value / 100;
      UNITINFO_FTIDALGENERATOR : UnitInfo.fTidalGenerator := value / 100;
      UNITINFO_FCLOAKCOST      : UnitInfo.fCloakCost := value / 100;
      UNITINFO_FCLOAKCOSTMOVE  : UnitInfo.fCloakCostMoving := value / 100;

      UNITINFO_BMCODE          : UnitInfo.cBMCode := value;

      UNITINFO_EXPLODEAS       : UnitInfo.p_ExplodeAs := PLongWord(TAWeapon.WeaponId2Ptr(value));
      UNITINFO_SELFDSTRAS      : UnitInfo.p_SelfDestructAsAs := PLongWord(TAWeapon.WeaponId2Ptr(value));

// 1 ?
// 2 standingfireorder
// 3 ?
// 4 init_cloaked
// 5 downloadable
      UNITINFO_BUILDER         : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 6);
// 7 zbuffer
      UNITINFO_STEALTH         : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 8);
      UNITINFO_ISAIRBASE       : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 9);
      UNITINFO_TARGETTINGUPGRADE   : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 10);
      UNITINFO_CANFLY          : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 11);
      UNITINFO_CANHOVER        : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 12);
      UNITINFO_TELEPORTER      : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 13);
      UNITINFO_HIDEDAMAGE      : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 14);
      UNITINFO_SHOOTME         : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 15);
      UNITINFO_HASWEAPONS      : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 16);
// 17 armoredstate
// 18 activatewhenbuilt
      UNITINFO_FLOATER         : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 19);
      UNITINFO_UPRIGHT         : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 20);
      UNITINFO_AMPHIBIOUS      : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 21);
// 22 ?
// 23 internal command reload -> sub_42D1F0. probably reloads cob script
// 24 isfeature
// 25 noshadow
      UNITINFO_IMMUNETOPARALYZER  : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 26);
      UNITINFO_HOVERATTACK     : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 27);
      UNITINFO_KAMIKAZE        : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 28);
      UNITINFO_ANTIWEAPONS     : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 29);
      UNITINFO_DIGGER          : SetUnitTypeMask(UnitID, 0, (value = 1), 1 shl 30);
// 31 has GUI (does gui file in guis folder exists with name of tested unit) ? sub_42d2e0

      UNITINFO_ONOFFABLE       : SetUnitTypeMask(UnitID, 1, (value = 1), 4);
      UNITINFO_CANSTOP         : SetUnitTypeMask(UnitID, 1, (value = 1), 8);
      UNITINFO_CANATTACK       : SetUnitTypeMask(UnitID, 1, (value = 1), 16);
      UNITINFO_CANGUARD        : SetUnitTypeMask(UnitID, 1, (value = 1), 32);
      UNITINFO_CANPATROL       : SetUnitTypeMask(UnitID, 1, (value = 1), 64);
      UNITINFO_CANMOVE         : SetUnitTypeMask(UnitID, 1, (value = 1), 128);
      UNITINFO_CANLOAD         : SetUnitTypeMask(UnitID, 1, (value = 1), 256);
      UNITINFO_CANRECLAMATE    : SetUnitTypeMask(UnitID, 1, (value = 1), 1024);
      UNITINFO_CANRESURRECT    : SetUnitTypeMask(UnitID, 1, (value = 1), 2048);
      UNITINFO_CANCAPTURE      : begin SetUnitTypeMask(UnitID, 1, (value = 1), $1000); SetUnitTypeMask(UnitID, 1, (value = 1), $2000); end;
      UNITINFO_CANDGUN         : SetUnitTypeMask(UnitID, 1, (value = 1), $4000);
      UNITINFO_SHOWPLAYERNAME  : SetUnitTypeMask(UnitID, 1, (value = 1), $20000);
      UNITINFO_COMMANDER       : SetUnitTypeMask(UnitID, 1, (value = 1), $40000);
      UNITINFO_CANTBERANSPORTED: SetUnitTypeMask(UnitID, 1, (value = 1), $80000);
    end;
    Result:= True;
  end;
end;

class Function TAUnits.CreateMinions(p_Unit: PUnitStruct; Amount: Byte; UnitInfo: Pointer; Action: TTAActionType; ArrayId: Cardinal) : Integer;
const
  MAXRETRIES = 25 - 1;
var
  UnitSt: PUnitStruct;
  CallerUnitInfoSt, UnitInfoSt: PUnitInfo;
  PlayerIndex: Byte;
  UnitsArray: TFoundUnits;
  CallerPosition, TestPosition: TPosition;
  CallerTurn: TTurn;
  DestIsAir: Boolean;
  SpotTestFree: Boolean;
  NewPos: array [0..1] of LongInt;
  GridPos: array [0..1] of Word;
  ToBeSpawned: array of TPosition;
  UnitState : LongWord;
  ResultUnit: Pointer;
  //Retries: Integer;
  i, j: Integer;
  r, angle, jiggle: Integer;
  ModelDiagonal: array [0..1] of Cardinal;
  SpawnRange: Cardinal;
begin
  Result := 0;
  if (p_Unit <> nil) and
     (UnitInfo <> nil) then
  begin
    UnitSt := p_Unit;

    PlayerIndex := TAUnit.GetOwnerIndex(pointer(p_Unit));

    UnitInfoSt := UnitInfo;
    CallerUnitInfoSt := Pointer(TAMem.UnitInfoId2Ptr(UnitSt.nUnitInfoID));

    CallerPosition := UnitSt.Position;
    CallerTurn := UnitSt.Turn;

    ModelDiagonal[0] := Round(Hypot(CallerUnitInfoSt.nFootPrintX, CallerUnitInfoSt.nFootPrintZ) * 14);
    ModelDiagonal[1] := Round(Hypot(UnitInfoSt.nFootPrintX, UnitInfoSt.nFootPrintZ) * 14);
    SpawnRange := Round((ModelDiagonal[0] + ModelDiagonal[1]) *1.4);

    //Retries:= 0;
    Randomize;
    ResultUnit := nil;

    for i := 1 to Amount do
    begin
      r := SpawnRange;
      for j := MAXRETRIES downto 0 do
      begin
        jiggle := Round(Random(High(Byte))/2);
        angle := Random(360)+1;
        if CircleCoords(CallerPosition, r + jiggle, angle, NewPos[0], NewPos[1]) then
        begin
          if GetTPosition(NewPos[0], NewPos[1], TestPosition) <> nil then
          begin
            DestIsAir := (UnitInfoSt.UnitTypeMask and 2048 = 2048);
            SpotTestFree := TAUnit.TestBuildSpot(PlayerIndex, UnitInfoSt, TestPosition.X, TestPosition.Z);
            if SpotTestFree or DestIsAir then
            begin
              TAUnit.Position2Grid(TestPosition, UnitInfoSt, GridPos[0], GridPos[1]);
              SetLength(ToBeSpawned, High(ToBeSpawned) + 2);
              ToBeSpawned[High(ToBeSpawned)] := TestPosition;
              ToBeSpawned[High(ToBeSpawned)].X := Round(TestPosition.X + (UnitInfoSt.nFootPrintX / 2) * 16);
              ToBeSpawned[High(ToBeSpawned)].Z := Round(TestPosition.Z + (UnitInfoSt.nFootPrintZ / 2) * 16);
              if DestIsAir then
              begin
                if (UnitInfoSt.nCruiseAlt > 0) then
                  ToBeSpawned[High(ToBeSpawned)].Y := UnitInfoSt.nCruiseAlt;
                UnitState := 6;
              end else
              begin
                if GetPosHeight(@ToBeSpawned[High(ToBeSpawned)]) <> - 1 then
                  ToBeSpawned[High(ToBeSpawned)].Y := GetPosHeight(@ToBeSpawned[High(ToBeSpawned)]);
                UnitState := 1;
              end;

              ResultUnit := TAUnit.CreateUnit(PlayerIndex, UnitInfo, ToBeSpawned[High(ToBeSpawned)], nil, False, False, UnitState);
              if ResultUnit <> nil then
              begin
                if TAData.NetworkLayerEnabled then
                  Send_UnitBuildFinished(p_Unit, ResultUnit);
                SetLength(UnitsArray, High(UnitsArray) + 2);
                UnitsArray[High(UnitsArray)] := PUnitStruct(ResultUnit).lUnitInGameIndex;
                if Action <> Action_Ready then
                  TAUnit.CreateMainOrder(ResultUnit, p_Unit, Action, nil, 1, 0, 0);
                Break;
              end;
            end;
          end;
        end;
        //Inc(Retries);
      end;
    end;
    Result:= High(ToBeSpawned) + 1;
{    SendTextLocal('Caller model diag: ' + IntToStr(ModelDiagonal[0]));
    SendTextLocal('Spawn model diag: ' + IntToStr(ModelDiagonal[1]));
    SendTextLocal('Min range: ' + IntToStr(SpawnRange));
    SendTextLocal('Will spawn: ' + IntToStr(High(ToBeSpawned)+1));
    SendTextLocal('Retries: ' + IntToStr(Retries));  }

    if Result > 0 then
    begin
      if ArrayId = 65535 then
        Result:= TAunit.GetId(ResultUnit)
      else
        if ArrayId <> 0 then
        begin
          if not Assigned(SpawnedMinionsArr[ArrayId].UnitIds) then
            if not TAUnits.UnitsIntoGetterArray(p_Unit, 2, ArrayId, UnitsArray) then
              Result:= 0;
        end;
    end;
  end;
end;

class procedure TAUnits.GiveUnit(Existingp_Unit: PUnitStruct; PlayerIdx: Byte);
var
  PlayerStruct : PPlayerStruct;
begin
  if Existingp_Unit <> nil then
  begin
    PlayerStruct := TAPlayer.GetPlayerByIndex(PlayerIdx);
    UNITS_GiveUnit(Existingp_Unit, PlayerStruct, nil);
  end;
end;

class function TAUnits.CreateSearchFilter(Mask: Integer) : TUnitSearchFilterSet;
var
  a, b: word;
begin
  Result := [usfNone];
  if Mask = 0 then
    Exit;

  for a:= Ord(usfNone) to Ord(usfIncludeTransported) do
  begin
    b := Round(Power(2, a));
    if (Mask and b) = b then
    begin
      Include(Result, TUnitSearchFilter(a + 1));
    end;
  end;
end;

class function TAUnits.GetRandomArrayId(ArrayType: Byte) : Word;
var
  test: word;
  retry: integer;
  cond: boolean;
begin
  Result:= 0;
  retry:= 0;
  cond:= false;
  repeat
    inc(retry);
    test:= 1 + Random(High(Word) - 1);  //0..65534
    case ArrayType of
      1 : cond:= not Assigned(UnitSearchArr[test].UnitIds);
      2 : cond:= not Assigned(SpawnedMinionsArr[test].UnitIds);
      else
        exit;
    end;
    if (retry > 100) and (cond = false) then exit;
  until (cond = true);
    Result:= test;
end;

class function TAUnits.SearchUnits ( p_Unit: PUnitStruct; SearchId: LongWord; SearchType: Byte;
  MaxDistance: Integer; Filter: TUnitSearchFilterSet; UnitTypeFilter: Pointer ) : LongWord;
var
  FoundCount: Integer;
  MaxUnitId: LongWord;
  UnitSt, CheckedUnitSt: PUnitStruct;
  UnitInfoSt, CheckedUnitInfoSt: PUnitInfo;
  Px, Py, Rx, Ry, dx, dy: SmallInt;
  PosX, PosY: Integer;
  LastNearestUnitDistance, NearestUnitDistance, Distance: Integer;
  UnitId: LongWord;
  FoundArray: TFoundUnits;
  Condition: Boolean;
  i: Integer;
label
  AddFound;

begin
  FoundCount:= 0;
  Result:= 0;
  //if (SearchType = 2) and (MaxDistance = 0) then Exit;
  UnitSt:= p_Unit;
  UnitInfoSt:= UnitSt.p_UNITINFO;
  MaxUnitId:= TAData.MaxUnitsID;

  for UnitId := 1 to MaxUnitId do
  begin
    CheckedUnitSt:= TAUnit.Id2Ptr(UnitId);
    if (CheckedUnitSt = nil) or
       (CheckedUnitSt = p_Unit) then
      Continue;
    if CheckedUnitSt.nUnitInfoID = 0 then
      Continue;
    if (UnitTypeFilter <> nil) then
      if (TAUnit.GetUnitInfoPtr(CheckedUnitSt) <> UnitTypeFilter) then
        Continue;

    if not TAUnits.UnitsFilterVsUnit(CheckedUnitSt, Filter, UnitSt.p_Owner) then
      Continue;        
      
    CheckedUnitInfoSt := Pointer(CheckedUnitSt.p_UNITINFO);
    if UnitId <> Word(UnitSt.lUnitInGameIndex) then
    begin
      case SearchType of
        1 : begin
              Rx:= UnitSt.nGridPosX;
              Ry:= UnitSt.nGridPosZ;
              dx:= UnitInfoSt.nFootPrintX;
              dy:= UnitInfoSt.nFootPrintZ;
              PosX:= MakeLong(CheckedUnitSt.Position.x_, CheckedUnitSt.Position.X);
              Px:= (PosX - (SmallInt(CheckedUnitInfoSt.nFootPrintX shl 19)) + $80000) shr 20;
              PosY:= MakeLong(CheckedUnitSt.Position.z_, CheckedUnitSt.Position.Z);
              Py:= (PosY - (SmallInt(CheckedUnitInfoSt.nFootPrintZ shl 19)) + $80000) shr 20;
              if MaxDistance = 0 then // here used as "sensitivity" switcher
                Condition:= (Rx < Px) and (Px < Rx + dx) and (Ry < Py) and (Py < Ry + dy)
              else
                Condition:= (Rx <= Px) and (Px <= Rx + dx) and (Ry <= Py) and (Py <= Ry + dy);
              if Condition then goto AddFound;
            end;
     2..3 : begin
              if MaxDistance > 0 then
              begin
                Distance:= TAUnits.Distance(@UnitSt.Position, @CheckedUnitSt.Position);
                if (Distance <> -1) and (Distance <= MaxDistance) then goto AddFound;
              end else
                goto AddFound;
            end;
        4 : goto AddFound;
      end;
    end;
    Continue;
    AddFound:
    Inc(FoundCount);
    SetLength(FoundArray, High(FoundArray) + 2);
    FoundArray[High(FoundArray)]:= CheckedUnitSt.lUnitInGameIndex;
    if SearchId = 65535 then
    begin
      Result := CheckedUnitSt.lUnitInGameIndex;
      Exit;
    end;
  end;

  // Single unit search
  if (SearchType = 3) and (FoundCount > 0) then
  begin
    LastNearestUnitDistance:= 0;
    if High(FoundArray) + 1 > 0 then
      for i:= Low(FoundArray) to High(FoundArray) do
      begin
        NearestUnitDistance := TAUnits.Distance(@UnitSt.Position, @TAUnit.Id2Ptr(FoundArray[i]).Position);
        if i = Low(FoundArray) then
        begin
          if NearestUnitDistance <> -1 then
            LastNearestUnitDistance := NearestUnitDistance;
          Result:= FoundArray[i];
        end else
          if (NearestUnitDistance < LastNearestUnitDistance) and (NearestUnitDistance <> -1) then
          begin
            LastNearestUnitDistance := NearestUnitDistance;
            Result:= FoundArray[i];
          end;
      end;
      Exit;
  end;

  if SearchId <> 65535 then
    if not Assigned(UnitSearchArr[SearchId].UnitIds) then
      if TAUnits.UnitsIntoGetterArray(p_Unit, 1, SearchId, FoundArray) then
        Result:= FoundCount;
end;

class function TAUnits.UnitsIntoGetterArray(p_Unit: PUnitStruct; ArrayType: Byte; Id: LongWord; const UnitsArray: TFoundUnits) : Boolean;
var
  i: integer;
  UnitRec: TStoreUnitsRec;
begin
  Result:= False;
  if High(UnitsArray) + 1 > 0 then
  begin
    UnitRec.Id:= Id;
    SetLength(UnitRec.UnitIds, High(UnitsArray)+1);
    for i:= Low(UnitsArray) to High(UnitsArray) do
      UnitRec.UnitIds[i]:= UnitsArray[i];
    case ArrayType of
      1 : UnitSearchResults.Insert(Id, UnitRec);
      2 : SpawnedMinions.Insert(Id, UnitRec);
    end;
    Result:= True;
  end;
end;

class procedure TAUnits.ClearSearchRec(Id: LongWord; ArrayType: Byte);
begin
  case ArrayType of
  1 : if UnitSearchArr[Id].UnitIds <> nil then
      begin
        UnitSearchArr[Id].UnitIds := nil;
        UnitSearchArr[Id].Id := 0;
      end;
  2 : if SpawnedMinionsArr[Id].UnitIds <> nil then
      begin
        SpawnedMinionsArr[Id].UnitIds := nil;
        SpawnedMinionsArr[Id].Id := 0;
      end;
  end;
end;

class procedure TAUnits.RandomizeSearchRec(Id: LongWord; ArrayType: Byte);
var
 i, j : word;
 MaxElem : Integer;
 X : Cardinal;
begin
  Randomize;
  case ArrayType of
  1 : if Assigned(UnitSearchArr[Id].UnitIds) then
      begin
        MaxElem := High(UnitSearchArr[Id].UnitIds);
        if MaxElem > 0 then
        begin
          for i := MaxElem downto 0 do
          begin
            j := Random(i) + 1;
            if not (i = j) then
            begin
              X := UnitSearchArr[Id].UnitIds[i];
              UnitSearchArr[Id].UnitIds[i] := UnitSearchArr[Id].UnitIds[j];
              UnitSearchArr[Id].UnitIds[j] := X;
            end;
          end;
        end;
      end;
  2 : if not Assigned(SpawnedMinionsArr[Id].UnitIds) then
      begin
        MaxElem := High(SpawnedMinionsArr[Id].UnitIds);
        if MaxElem > 0 then
        begin
          for i := MaxElem downto 0 do
          begin
            j := Random(i) + 1;
            if not (i = j) then
            begin
              X := SpawnedMinionsArr[Id].UnitIds[i];
              SpawnedMinionsArr[Id].UnitIds[i] := SpawnedMinionsArr[Id].UnitIds[j];
              SpawnedMinionsArr[Id].UnitIds[j] := X;
            end;
          end;
        end;
      end;
  end;
end;

class function TAUnits.Distance(Pos1, Pos2 : PPosition) : Cardinal;
var
  Distance : Extended;
begin
  Result := 0;
  if (Pos1 <> nil) and (Pos2 <> nil) then
  begin
    if ((PPosition(Pos1)^.X - PPosition(Pos2)^.X) <> 0) and
       ((PPosition(Pos1)^.Z - PPosition(Pos2)^.Z) <> 0) then
    begin
      Distance := Hypot(PPosition(Pos1)^.X - PPosition(Pos2)^.X,
                        PPosition(Pos1)^.Z - PPosition(Pos2)^.Z);
      Result := Round(Distance);
    end else
      Result := 0;
  end;
end;

class function TAUnits.CircleCoords(CenterPosition: TPosition; Radius: Integer; Angle: Integer; out x, z: Integer) : Boolean;
var
  MapWidth : Integer;
  MapHeight : Integer;
begin
  x := Round(CenterPosition.X + cos(Angle) * Radius);
  z := Round(CenterPosition.Z + sin(Angle) * Radius);
  MapWidth := PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.lMapWidth;
  MapHeight := PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.lMapHeight;
  if (x > 0) and (z > 0) and (x < MapWidth) and (z < MapHeight) then
    Result:= True
  else
    Result:= False;
end;

class function TAUnits.UnitsFilterVsUnit(p_Unit: PUnitStruct;
  Filter: TUnitSearchFilterSet; Player: PPlayerStruct) : Boolean;
begin
  Result := False;

  if not (usfIncludeTransported in Filter) then
    if p_Unit.cAttachedToPiece <> -1 then
      Exit;

  if Player <> nil then
  begin
    if (usfOwner in Filter) or
       (usfAllied in Filter) or
       (usfEnemy in Filter) then
    begin
      if usfAllied in Filter then
      begin
        if not TAPlayer.GetAlliedState(Player, TAPlayer.PlayerIndex(p_Unit.p_Owner)) then
          Exit;
      end else
      begin
        if usfOwner in Filter then
        begin
          if Player <> p_Unit.p_Owner then
            Exit;
        end else
        begin
          if usfEnemy in Filter then
          begin
            if TAPlayer.GetAlliedState(Player, TAPlayer.PlayerIndex(p_Unit.p_Owner)) then
              Exit;
          end;
        end;
      end;

      if not (usfAI in Filter) then
      begin
        if TAPlayer.PlayerController(p_Unit.p_Owner) = Player_LocalAI then
          Exit;
      end;
    end;
  end;

  if usfExcludeAir in Filter then
  begin
    if TAUnit.GetUnitInfoField(p_Unit, UNITINFO_CANFLY) = 1 then
      Exit;
  end;
  if usfExcludeSea in Filter then
  begin
    if TAUnit.GetUnitInfoField(p_Unit, UNITINFO_FLOATER) = 1 then
      Exit
    else
      //subs or pels/hovers currently in sea
      if not (GetPosHeight(@p_Unit.Position) > PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.SeaLevel) then
      begin
        Exit;
      end;
  end;
  if usfExcludeBuildings in Filter then
  begin
    if TAUnit.GetUnitInfoField(p_Unit, UNITINFO_BMCODE) = 0 then
      Exit;
  end;
  if usfExcludeNonWeaponed in Filter then
  begin
    if TAUnit.GetUnitInfoField(p_Unit, UNITINFO_HASWEAPONS) = 0 then
      Exit;
  end;

  if not (usfIncludeInBuildState in Filter) and
    (p_Unit.fBuildTimeLeft <> 0.0) then
    Exit;

  Result := True;
end;

end.
