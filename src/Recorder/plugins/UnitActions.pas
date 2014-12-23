unit UnitActions;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_UnitActions : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallUnitActions;
Procedure OnUninstallUnitActions;

// -----------------------------------------------------------------------------

procedure RemoveBuildQueuesFromSelected;
procedure UnitActions_AssignAISquad;
procedure UnitActions_TransportOverloadFix;
procedure UnitActions_VTOLTransportCapacityCanLoadTest;
procedure UnitActions_VTOLTransportSize;
//procedure UnitActions_VTOLTransportSizeCursor;
procedure UnitActions_ExtraVTOLOrders;
procedure UnitActions_AllowOrderType;
procedure UnitActions_AllowOrderType_Action2IndexWrapper;
procedure UnitActions_NewOrdersButtonsWrapper;
procedure UnitActions_TeleportOrderWrapper;
procedure UnitActions_TeleportReloadWrapper;
procedure UnitActions_TeleportPositionOffset;
procedure UnitActions_AdvancedDefaultMission;
procedure UnitActions_DontHealTimeNotBuilt;

procedure BroadcastCommanderStartPosition; stdcall;

implementation
uses
  IniOptions,
  Windows,
  idplay,
  sysutils,
  TA_NetworkingMessages,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_MemPlayers,
  TA_MemUnits,
  TA_MemPlotData,
  TA_FunctionsU,
  COB_extensions,
  UnitInfoExpand;

Procedure OnInstallUnitActions;
begin
end;

Procedure OnUninstallUnitActions;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_UnitActions then
  begin
    Result := TPluginData.create( False, 'UnitActions Plugin',
                                  State_UnitActions,
                                  @OnInstallUnitActions,
                                  @OnUninstallUnitActions );

    Result.MakeRelativeJmp( State_UnitActions,
                            'Ground transporter overload fix',
                            @UnitActions_TransportOverloadFix,
                            $00406789, 3);

    Result.MakeRelativeJmp( State_UnitActions,
                            'Enable multi air load',
                            @UnitActions_VTOLTransportCapacityCanLoadTest,
                            $00411224, 0);

    Result.MakeRelativeJmp( State_UnitActions,
                            'Enable weight air load',
                            @UnitActions_VTOLTransportSize,
                            $0041125F, 3);
{
    UnitActionsPlugin.MakeRelativeJmp( State_UnitActions,
                          'Enable weight air load cursor',
                          @UnitActions_VTOLTransportSizeCursor,
                          $00489B0B, 3);
}
    Result.MakeRelativeJmp( State_UnitActions,
                            'Enable resurrect and capture orders fot VTOLs',
                            @UnitActions_ExtraVTOLOrders,
                            $00438AE3, 0);

    Result.MakeRelativeJmp( State_UnitActions,
                            'UnitActions_AssignAISquad',
                            @UnitActions_AssignAISquad,
                            $00408846, 0);

    Result.MakeRelativeJmp( State_UnitActions,
                            'set cursor for new orders',
                            @UnitActions_AllowOrderType,
                            $0043E4F5, 4);

    Result.MakeRelativeJmp( State_UnitActions,
                            'convert cursor type to action type',
                            @UnitActions_AllowOrderType_Action2IndexWrapper,
                            $0043F144, 4);

    Result.MakeRelativeJmp( State_UnitActions,
                            'translate orders buttons to action type',
                            @UnitActions_NewOrdersButtonsWrapper,
                            $00419C20, 4);

    Result.MakeRelativeJmp( State_UnitActions,
                            'complete rebuild of TA teleport order',
                            @UnitActions_TeleportOrderWrapper,
                            $00406AAB, 1);

    Result.MakeRelativeJmp( State_UnitActions,
                            'teleport reloader for units',
                            @UnitActions_TeleportReloadWrapper,
                            $0048ADDF, 1);

    Result.MakeRelativeJmp( State_UnitActions,
                            'multiple teleport, apply position offset',
                            @UnitActions_TeleportPositionOffset,
                            $0048D126, 1);

    Result.MakeRelativeJmp( State_UnitActions,
                            'Default unit mission with position parameter addition',
                            @UnitActions_AdvancedDefaultMission,
                            $0043B9DE, 1);

    Result.MakeRelativeJmp( State_UnitActions,
                            'dont heal time units that are under construction',
                            @UnitActions_DontHealTimeNotBuilt,
                            $0048AF58, 1);
  end else
    Result := nil;
end;

procedure BroadcastCommanderStartPosition; stdcall;
var
  Player : PPlayerStruct;
  p_Unit : PUnitStruct;
begin
  if TAData.NetworkLayerEnabled then
  begin
    Player := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);
    p_Unit := Player.p_UnitsArray;
    if p_Unit <> nil then
      UNITS_NewUnitPosition( p_Unit,
                             PCardinal(@p_Unit.Position.x_)^,
                             PCardinal(@p_Unit.Position.y_)^,
                             PCardinal(@p_Unit.Position.z_)^,
                             1 );
  end;
end;

function OverloadBugFix(p_Unit: PUnitStruct): LongBool; stdcall;
begin
  Result := False;
  if (TAUnit.GetLoadCurAmount(p_Unit) + 1) <= p_Unit.p_UnitInfo.cTransportCap then
    Result := True;
end;

procedure UnitActions_TransportOverloadFix;
label
  RefuseLoad;
asm
  test    eax, eax
  jz      RefuseLoad
  push    edx
  push    eax
  push    ecx
  push    ecx
  call OverloadBugFix
  test    eax, eax
  pop     ecx
  pop     eax
  pop     edx
  jz      RefuseLoad
  push $00406791;
  call PatchNJump;
RefuseLoad :
  push $004068DC;
  call PatchNJump;
end;

procedure UnitActions_VTOLTransportCapacityCanLoadTest;
label
  TestIsMultiTransp,
  StandardTransporter,
  RefuseLoad,
  AllowLoad;
asm
TestIsMultiTransp :
  // let's test is transporter capable to load multiple units
  push    edx
  push    eax
  push    ecx
  push    1
  push    esi
  call    GetUnitInfoProperty
  test    eax, eax
  jz      StandardTransporter
  pop     ecx
  pop     eax
  pop     edx
  jmp     AllowLoad
StandardTransporter :
  // not multi transporter so
  // we check has it got unit so we refuse loading another one
  pop     ecx
  pop     eax
  pop     edx
  test    ecx, ecx
  jz      AllowLoad
RefuseLoad :
  push $0041152B;
  call PatchNJump;
AllowLoad :
  // esi transporter ptr
  // eax transported ptr
  push    edx
  push    eax
  push    ecx
  push    esi
  call OverloadBugFix
  test    eax, eax
  pop     ecx
  pop     eax
  pop     edx
  jz      RefuseLoad
  push $0041122C;
  call PatchNJump;
end;

function CompareVTOLUnitWeight(Transporter, ToBeTransported : Pointer) : LongBool; stdcall;
var
  MaxWeight : Integer;
  CurLoadWeight : Integer;
  TobeTransportedLoadWeight : Integer;
begin
  Result := False;
  MaxWeight := GetUnitInfoProperty(Transporter, 3);
  CurLoadWeight := TAUnit.GetLoadWeight(Transporter);
  TobeTransportedLoadWeight := Round(PUnitInfo(TAUnit.GetUnitInfoPtr(ToBeTransported)).lBuildCostMetal);
  if (TobeTransportedLoadWeight + CurLoadWeight) < MaxWeight then
    Result := True;
end;

procedure UnitActions_VTOLTransportSize;
label
  FootSizeCompare,
  RefuseLoad,
  ContinueTransportedTest,
  AllowLoadUnit,
  RefuseLoadReasonWeight;
asm
FootSizeCompare :
  movzx   cx, [ecx+TUnitInfo.cTransportSize]
  cmp     word ptr [eax+TUnitStruct.nFootPrintX], cx
  jle     ContinueTransportedTest
RefuseLoad :
  push $0041126D;
  call PatchNJump;
ContinueTransportedTest :
  pushAD
  mov     edi, eax
  push    3
  push    esi
  call    GetUnitInfoProperty
  test    eax, eax
  jz AllowLoadUnit
  push    edi
  push    esi
  call CompareVTOLUnitWeight
  test    eax, eax
  jz RefuseLoadReasonWeight
AllowLoadUnit :
  popAD
  push $00411288;
  call PatchNJump;
RefuseLoadReasonWeight :
  popAD
  push $0041126D;
  call PatchNJump;
end;

{
// edi - transporter unitinfo
// esi - transported unitinfo
.text:00489B0B 00C 66 0F B6 97 2A 02 00 00        movzx   dx, [edi+UNITINFO.transportsize] // 22Ah
.text:00489B13 00C 66 39 96 4A 01 00 00           cmp     [esi+UNITINFO.FootX], dx         // 14Ah
.text:00489B1A 00C 7E 08                          jle     short loc_489B24                 // continue
.text:00489B1C 00C 33 C0                          xor     eax, eax                         // refuse
}
{procedure UnitActions_VTOLTransportSizeCursor;
label
  FootSizeCompare,
  RefuseLoad,
  ContinueTransportedTest;
asm
FootSizeCompare :
  movzx   dx, [edi+TUnitInfo.cTransportSize]
  cmp     [esi+TUnitInfo.nFootPrintX], dx
  jle     ContinueTransportedTest
RefuseLoad :
  push $00489B1C;
  call PatchNJump;
ContinueTransportedTest :
  push $00489B24;
  call PatchNJump;
end;    }

procedure UnitActions_ExtraVTOLOrders;
label
  AirUnitChase,
  GroundMovementChase;
asm
  test    dh, 8
  jnz     AirUnitChase
  jmp GroundMovementChase
AirUnitChase :
  pushAD
  push    2
  push    eax
  call    GetUnitInfoProperty
  test    eax, eax
  popAD
  jnz     GroundMovementChase
  push $00438B60;
  call PatchNJump;
GroundMovementChase :
  push $00438AE8;
  call PatchNJump;
end;

var
  squadnr : Integer;
procedure UnitActions_AssignAISquad;
label
  ForceSquad;
asm
  // esi unit
  pushAD
  push    7
  push    esi
  xor     eax, eax
  call    GetUnitInfoProperty
  test    ax, ax
  jnz     ForceSquad
  popAD
  mov     eax, [esi+110h]
  push $0040884C;
  call PatchNJump;
ForceSquad:
  mov     squadnr, eax
  popAD
  push    squadnr
  push $004088F9;
  call PatchNJump;
end;

function SetCursorNewOrders(ActionType: Integer; OrderUnit: PUnitStruct;
  TargetUnit: PUnitStruct; TargetPosition: PPosition): Integer; stdcall;
var
  ReturnVal : Integer;
  Distance : Integer;
  InDistance : Boolean;
  CanTeleport : Boolean;
begin
  ReturnVal := 0;
  case ActionType of
    BUTTON_ORDER_TELEPORTNEW-1 :
      begin
        ReturnVal := 14;
        if TAUnit.AtMouse = nil then
        begin
          Distance := TAUnits.Distance(@OrderUnit.Position, TargetPosition);
          InDistance := (Distance <= ExtraUnitInfoTags[PUnitInfo(OrderUnit.p_UnitInfo).nCategory].TeleportMaxDistance) and
            (Distance >= ExtraUnitInfoTags[PUnitInfo(OrderUnit.p_UnitInfo).nCategory].TeleportMinDistance);

          CanTeleport := InDistance;

          if ExtraUnitInfoTags[PUnitInfo(OrderUnit.p_UnitInfo).nCategory].TeleportToLoSOnly then
            CanTeleport := CanTeleport and
                           TAMap.PositionInLOS(OrderUnit.p_Owner, TargetPosition);

          if CanTeleport then
            ReturnVal := 9;
        end;
      end;
  end;
  Result := ReturnVal;
end;

procedure UnitActions_AllowOrderType;
label
  custom_or_default_order,
  default_order;
asm
  cmp     eax, 0Dh
  ja      custom_or_default_order
  push $0043E4FE;
  call PatchNJump;
custom_or_default_order:
  // edx order unit ptr
  // edi target unit ptr
  push    ecx
  push    edx
  push    ebx
  push    esi
  push    edi

  mov     ebx, [esp+38h]
  push    ebx
  push    edi
  push    edx
  push    eax
  call    SetCursorNewOrders

  pop     edi
  pop     esi
  pop     ebx
  pop     edx
  pop     ecx

  test    eax, eax
  jz      default_order
  push $0043EA5A;      // return eax
  call PatchNJump;
default_order:
  push $0043F098;      // return 19
  call PatchNJump;
end;

type
  TUnitToTeleport = record
    Id : Word;
    OffX : Integer;
    OffZ : Integer;
  end;

function TeleportGroupOfUnits(Teleporter: PUnitStruct; UnitsFilter: TUnitSearchFilterSet;
  CenterPosition, TargetPosition: PPosition; Distance: Cardinal): Integer;
var
  TargetPositionWithOffset : TPosition;
  nPosX, nPosZ : Word;
  UnitsArr : array of TUnitToTeleport;
  TestedUnit : PUnitStruct;
  i, j : Cardinal;
  UnitsToTeleportCount : Integer;
  TestedUnitInfo : PUnitInfo;
begin
  TestedUnit := TAData.UnitsArray_p;
  UnitsToTeleportCount := 0;
  while ( Cardinal(TestedUnit) <= Cardinal(TAData.EndOfUnitsArray_p) ) do
  begin
    if TestedUnit <> Teleporter then
    begin
      if PUnitInfo(TestedUnit.p_UnitInfo).cBMCode = 1 then
        if TAUnits.UnitsFilterVsUnit(TestedUnit, UnitsFilter, Teleporter.p_Owner) then
          if TAUnits.Distance(@TestedUnit.Position, @Teleporter.Position) <= Distance then
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
           (TAUnit.GetUnitInfoField(TestedUnit, UNITINFO_BUILDER) = 0) then  // can fire
        begin
          GetTPosition(TargetPosition.X + UnitsArr[i].OffX,
                       TargetPosition.Z + UnitsArr[i].OffZ,
                       TargetPositionWithOffset);

          if TargetPositionWithOffset.X >= PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.lMapWidth - 128 then
            Continue;
          if TargetPositionWithOffset.Z >= PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.lMapHeight - 32 then
            Continue;

          TAUnit.Position2Grid(TargetPositionWithOffset, TestedUnit.p_UnitInfo, nPosX, nPosZ);
          if (nPosX < PUnitInfo(TestedUnit.p_UnitInfo).nFootPrintX) then
          begin
            nPosX := PUnitInfo(TestedUnit.p_UnitInfo).nFootPrintX;
            TargetPositionWithOffset.X := PUnitInfo(TestedUnit.p_UnitInfo).nFootPrintX * 16;
          end;
          if (nPosZ < PUnitInfo(TestedUnit.p_UnitInfo).nFootPrintZ) then
          begin
            nPosZ := PUnitInfo(TestedUnit.p_UnitInfo).nFootPrintZ;
            TargetPositionWithOffset.Z := PUnitInfo(TestedUnit.p_UnitInfo).nFootPrintZ * 16;
          end;

          if TAUnit.TestAttachAtGridSpot(TestedUnit.p_UnitInfo, nPosX, nPosZ) then
          begin
            j := j + 1;
            if Teleporter.p_UnitScriptsData <> nil then
              TAUnit.CallCobProcedure(Teleporter, 'Teleporting', @Word(TargetPositionWithOffset.X), @Word(TargetPositionWithOffset.Z), @Word(TargetPositionWithOffset.Y), @UnitsArr[i].ID);
            TAUnit.CreateMainOrder(TestedUnit, Teleporter, Action_Teleport, @TargetPositionWithOffset, 0, 1, 0);
          end else
            TAUnit.CreateMainOrder(TestedUnit, Teleporter, Action_Move_Ground, @TargetPositionWithOffset, 0, 1, 0);
        end;
      end;
    end;
  end;
  if j <> 0 then
    CustomUnitFieldsArr[Word(Teleporter.lUnitInGameIndex)].TeleportReloadMax := TAUnits.Distance(CenterPosition, TargetPosition);
  Result := j;
end;

function TeleportUnitsFromYardmap(TeleporterUnit: PUnitStruct;
  UnitsFilter: TUnitSearchFilterSet; UnitOrder: PUnitOrder): Integer;
var
  j : Integer;
  TeleporterUnitInfo : PUnitInfo;
  GridPosX, GridPosY, GridPosZ : Cardinal;
  FootPrintX, FootPrintY, FootPrintZ : Cardinal;
  UnitPos : TPositionLong;
  CurUnit : PUnitStruct;
  NewPos : TPositionLong;
begin
  TeleporterUnitInfo := TeleporterUnit.p_UnitInfo;

  j := 0;

  UnitPos.X := MakeLong(0, TeleporterUnit.Position.X);
  UnitPos.Y := MakeLong(0, TeleporterUnit.Position.Y);
  UnitPos.Z := MakeLong(0, TeleporterUnit.Position.Z);

  GridPosX := UnitPos.X + TeleporterUnitInfo.lWidthX_;
  GridPosY := UnitPos.Y + TeleporterUnitInfo.lWidthY_;
  GridPosZ := UnitPos.Z + TeleporterUnitInfo.lWidthZ_;

  FootPrintX := UnitPos.X + TeleporterUnitInfo.lFootPrintX_;
  FootPrintY := UnitPos.Y + TeleporterUnitInfo.lFootPrintY_;
  FootPrintZ := UnitPos.Z + TeleporterUnitInfo.lFootPrintZ_;

  CurUnit := TAData.UnitsArray_p;
  if ( Cardinal(CurUnit) <= Cardinal(TAData.EndOfUnitsArray_p) ) then
  begin
    while ( True ) do
    begin
      if ( PCardinal(@CurUnit.Position.x_)^ >= GridPosX ) then
      begin
        if ( PCardinal(@CurUnit.Position.x_)^ <= FootPrintX ) then
        begin
          if ( PCardinal(@CurUnit.Position.z_)^ >= GridPosZ ) then
          begin
            if ( PCardinal(@CurUnit.Position.z_)^ <= FootPrintZ ) then
            begin
              if ( PCardinal(@CurUnit.Position.y_)^ >= GridPosY ) then
              begin
                if ( PCardinal(@CurUnit.Position.y_)^ <= FootPrintY ) and
                   ( TeleporterUnit <> CurUnit ) then
                begin
                  if TAUnits.UnitsFilterVsUnit(CurUnit, UnitsFilter, TeleporterUnit.p_Owner) then
                  begin
                    Inc(j);
                    NewPos.X := MakeLong(0, (CurUnit.Position.x + UnitOrder.Pos.x) - TeleporterUnit.Position.X);
                    NewPos.Y := MakeLong(0, (CurUnit.Position.y + UnitOrder.Pos.y) - TeleporterUnit.Position.Y);
                    NewPos.Z := MakeLong(0, (CurUnit.Position.z + UnitOrder.Pos.z) - TeleporterUnit.Position.Z);
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

var
  ConvertActionIndex : Cardinal;
function UnitActions_AllowOrderType_Action2Index( ActionType: Byte;
                                                  OrderUnit, TargetUnit: PUnitStruct;
                                                  TargetPosition: PPosition): Pointer; stdcall;
var
  Distance : Integer;
begin
  ConvertActionIndex := 0;
  Result := nil;
  case ActionType of
    BUTTON_ORDER_TELEPORTNEW-1 :
      begin
        if PUnitInfo(OrderUnit.p_UnitInfo).cBMCode = 1 then
          ConvertActionIndex := TAMem.ScriptActionName2Index(PAnsiChar('MOVE_GROUND'));
        if (ExtraUnitInfoTags[PUnitInfo(OrderUnit.p_UnitInfo).nCategory].TeleportMethod <> tmNone) and
           (CustomUnitFieldsArr[TAUnit.GetId(OrderUnit)].TeleportReloadCur = 0) then
        begin
          Distance := TAUnits.Distance(@OrderUnit.Position, TargetPosition);
          if (Distance <= ExtraUnitInfoTags[PUnitInfo(OrderUnit.p_UnitInfo).nCategory].TeleportMaxDistance) and
             (Distance >= ExtraUnitInfoTags[PUnitInfo(OrderUnit.p_UnitInfo).nCategory].TeleportMinDistance) then
          begin
            case ExtraUnitInfoTags[PUnitInfo(OrderUnit.p_UnitInfo).nCategory].TeleportMethod of
              tmSelf : ;
              tmSelfLoS : if not TAMap.PositionInLOS(OrderUnit.p_Owner, TargetPosition) then
                            Exit;
              tmVTOLOthers :
                begin
                  ConvertActionIndex := TAMem.ScriptActionName2Index(PAnsiChar('VTOL_MOVE'));
                  TeleportGroupOfUnits(OrderUnit,
                                       ExtraUnitInfoTags[PUnitInfo(OrderUnit.p_UnitInfo).nCategory].TeleportFilter,
                                       @OrderUnit.Position,
                                       TargetPosition,
                                       ExtraUnitInfoTags[PUnitInfo(OrderUnit.p_UnitInfo).nCategory].TeleportMinDistance);
                end;
              tmYardmap :
                begin
                  ConvertActionIndex := TAMem.ScriptActionName2Index(PAnsiChar('TELEPORT'));
                end;
            end;
            if ExtraUnitInfoTags[PUnitInfo(OrderUnit.p_UnitInfo).nCategory].TeleportMethod < tmVTOLOthers then
            begin
              if TAUnit.TestUnloadPosition(OrderUnit.p_UnitInfo, TargetPosition^) then
                ConvertActionIndex := TAMem.ScriptActionName2Index(PAnsiChar('TELEPORT'));
            end;
          end;
        end; 
      end;
  end;
  Result := @ConvertActionIndex;
end;

procedure UnitActions_AllowOrderType_Action2IndexWrapper;
label
  custom_or_default_order,
  default_order;
asm
  cmp     ecx, 0Dh
  ja      custom_or_default_order
  push $0043F14D;
  call PatchNJump;
custom_or_default_order:
  // ebp order unit
  // edi target unit
  // esp+24h target position
  push    ecx
  push    edx
  push    ebx
  push    esi
  push    edi

  mov     ebx, [esp+38h]
  push    ebx              // position
  push    edi              // target unit
  push    ebp              // order unit
  push    ecx              // action type
  call    UnitActions_AllowOrderType_Action2Index

  pop     edi
  pop     esi
  pop     ebx
  pop     edx
  pop     ecx

  test    eax, eax
  jz      default_order
  push $00440196;      // return eax
  call PatchNJump;
default_order:
  push $004401DC;      // return nil
  call PatchNJump;
end;

procedure RemoveBuildQueuesFromSelected;
var
  Player : PPlayerStruct;
  PlayerMaxUnitID : Cardinal;
  CurrentUnit : PUnitStruct;
  i : Cardinal;
begin
  Player := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);
  PlayerMaxUnitID := Player.nNumUnits;
  for i := 0 to PlayerMaxUnitID do
  begin
    CurrentUnit := Pointer(Cardinal(Player.p_UnitsArray) + i * SizeOf(TUnitStruct));
    if ((CurrentUnit.lUnitStateMask and UnitSelectState[UnitSelected_State]) = UnitSelectState[UnitSelected_State]) then
    begin
      if (CurrentUnit.fBuildTimeLeft <> 0.0) or
         (CurrentUnit.p_Owner = nil) or
         (CurrentUnit.p_UnitInfo = nil) then
        Continue;

      if (PUnitInfo(CurrentUnit.p_UnitInfo).cBMCode <> 0) then
        Continue;

      if (TAUnit.GetUnitInfoField(CurrentUnit, UNITINFO_BUILDER) = 0) then
         Continue
      else begin
        ORDERS_RemoveAllBuildQueues(CurrentUnit, True);
        UpdateIngameGUI(0);
      end;
    end;
  end;
end;

const
  IMMEDIATEORDERS : AnsiString = 'immediateorders';
  SPECIALORDERS : AnsiString = 'specialorders';
function UnitActions_NewOrdersButtons(a2, v3, v4: Cardinal) : Integer; stdcall;
var
  DestStr : array[0..15] of AnsiChar;
  OrderName : String;
  ActionIndex : Cardinal;
label
  buildspot_state,
  play_immediateorders_return,
  play_specialorders_return;
begin
  OrderName := GetPrepareOrderName(a2, @DestStr, v3);

  if ( Pos('MOVE', OrderName) <> 0 ) then
  begin
    if ( PWord(v4 + $138)^ <> 0) then
    begin
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_MOVE;
      PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState :=
        PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $F7;
      PlaySound_2D_Name(PAnsiChar(IMMEDIATEORDERS), 0);
      Result := 1;
      Exit;
    end;
    PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_STOP;
    goto buildspot_state;
  end;

  if ( Pos('STOP', OrderName) <> 0 ) then
  begin
    PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_STOP;
    PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState :=
        PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $F7;

    if IniSettings.Plugin_StopButton then
      RemoveBuildQueuesFromSelected;

    ActionIndex := TAMem.ScriptActionName2Index(PAnsiChar('STOP'));
    MOUSE_EVENT_2UnitOrder(@PTAdynmemStruct(TAData.MainStructPtr).CurtMousePosition, 0, ActionIndex, 0, 0, 0);
play_immediateorders_return:
    PlaySound_2D_Name(PAnsiChar(IMMEDIATEORDERS), 0);
    Result := 1;
    Exit;
  end;

  if ( Pos('ATTACK', OrderName) <> 0 ) then
  begin
    if ( PWord(v4 + $138)^ <> 0) then
    begin
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_ATTACK;
      PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState :=
        PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $F7;
      PlaySound_2D_Name(PAnsiChar(IMMEDIATEORDERS), 0);
      Result := 1;
      Exit;
    end;
    PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_STOP;
    goto buildspot_state;
  end;

  if ( Pos('BLAST', OrderName) <> 0 ) then
  begin
    if ( PWord(v4 + $138)^ <> 0) then
    begin
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_BLAST;
      PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState :=
        PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $F7;
      PlaySound_2D_Name(PAnsiChar(IMMEDIATEORDERS), 0);
      Result := 1;
      Exit;
    end;
    PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_STOP;
    goto play_specialorders_return;
  end;

  if ( Pos('DEFEND', OrderName) <> 0 ) then
  begin
    if ( PWord(v4 + $138)^ <> 0) then
    begin
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_DEFEND;
      PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState :=
        PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $F7;
      PlaySound_2D_Name(PAnsiChar(IMMEDIATEORDERS), 0);
      Result := 1;
      Exit;
    end;
    PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_STOP;
    goto buildspot_state;
  end;

  if ( Pos('REPAIR', OrderName) <> 0 ) then
  begin
    if ( PWord(v4 + $138)^ <> 0) then
    begin
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_REPAIR;
      PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState :=
        PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $F7;
      PlaySound_2D_Name(PAnsiChar(SPECIALORDERS), 0);
      Result := 1;
      Exit;
    end;
    PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_STOP;
    goto play_specialorders_return;
  end;

  if ( Pos('PATROL', OrderName) <> 0 ) then
  begin
    if ( PWord(v4 + $138)^ <> 0) then
    begin
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_PATROL;
      PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState :=
        PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $F7;
      PlaySound_2D_Name(PAnsiChar(IMMEDIATEORDERS), 0);
      Result := 1;
      Exit;
    end;
    PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_STOP;
    goto buildspot_state;
  end;

  if ( Pos('RECLAIM', OrderName) <> 0 ) then
  begin
    if ( PWord(v4 + $138)^ <> 0) then
    begin
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_RECLAIM;
      PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState :=
        PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $F7;
      PlaySound_2D_Name(PAnsiChar(SPECIALORDERS), 0);
      Result := 1;
      Exit;
    end;
    PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_STOP;
    goto play_specialorders_return;
  end;

  if ( Pos('CAPTURE', OrderName) <> 0 ) then
  begin
    if ( PWord(v4 + $138)^ <> 0) then
    begin
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_CAPTURE;
      PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState :=
        PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $F7;
      PlaySound_2D_Name(PAnsiChar(SPECIALORDERS), 0);
      Result := 1;
      Exit;
    end;
    PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_STOP;
    goto play_specialorders_return;
  end;

  if ( Pos('TELEPORT', OrderName) <> 0 ) then
  begin
    if ( PWord(v4 + $138)^ <> 0) then
    begin
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_TELEPORTNEW;
      PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState :=
        PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $F7;
      PlaySound_2D_Name(PAnsiChar(IMMEDIATEORDERS), 0);
      Result := 1;
      Exit;
    end;
    PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_STOP;
    goto buildspot_state;
  end;

  if ( Pos('UNLOAD', OrderName) <> 0 ) then
  begin
    if ( PWord(v4 + $138)^ <> 0) then
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_UNLOAD
    else
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_STOP;
play_specialorders_return:
    PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState :=
      PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $F7;
    PlaySound_2D_Name(PAnsiChar(SPECIALORDERS), 0);
    Result := 1;
    Exit;
  end;

  if ( Pos('LOAD', OrderName) <> 0 ) then
  begin
    if ( PWord(v4 + $138)^ <> 0) then
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_LOAD
    else
      PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := BUTTON_ORDER_STOP;
buildspot_state:
    PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState :=
      PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $F7;
    goto play_immediateorders_return;
  end;
  Result := 0;
end;

procedure UnitActions_NewOrdersButtonsWrapper;
label
  return;
asm
  push    edx
  push    ecx
  push    ebp
  push    eax
  push    esi
  call    UnitActions_NewOrdersButtons
return:
  pop     ecx
  pop     edx
  push $0041A0FB;
  call PatchNJump;
end;

function UnitActions_TeleportOrder(p_Unit: PUnitStruct; OrderPtr: PUnitOrder; OrderReady: Cardinal): Integer; stdcall;
var
  Distance : Integer;
  NewX, NewY, NewZ : Cardinal;
  TargetPosition : TPosition;
  UnitID : Word;
begin
  UnitID := TAUnit.GetId(p_Unit);
  case OrderPtr.ucState of
    0 : begin
          if CustomUnitFieldsArr[UnitID].TeleportReloadMax <> 0 then
          begin
            // wait for teleport reload
            OrderPtr.nPaused := OrderPtr.nPaused or 1;
            OrderPtr.lRecallTime := 10 + TAData.GameTime;
            Result := 2;
            Exit;
          end else
          begin
            if ExtraUnitInfoTags[PUnitInfo(p_Unit.p_UnitInfo).nCategory].TeleportMethod <= tmVTOLOthers then
            begin
              if TAUnit.TestUnloadPosition(p_Unit.p_UnitInfo, OrderPtr.Pos) then
              begin
                if ExtraUnitInfoTags[PUnitInfo(p_Unit.p_UnitInfo).nCategory].TeleportMethod <> tmNone then
                  CustomUnitFieldsArr[UnitID].TeleportReloadCur := 0;
                Result := 1;
              end else
                Result := 7;
            end else
            begin
              CustomUnitFieldsArr[UnitID].TeleportReloadCur := 0;
              if TeleportUnitsFromYardmap(p_Unit,
                                          ExtraUnitInfoTags[PUnitInfo(p_Unit.p_UnitInfo).nCategory].TeleportFilter,
                                          OrderPtr) <> 0 then
              begin
                Distance := TAUnits.Distance(@p_Unit.Position, @OrderPtr.Pos);
                CustomUnitFieldsArr[UnitID].TeleportReloadMax := Distance;
              end;
              Result := 5;
            end;
          end;
        end;
    1 : begin
          TargetPosition := OrderPtr.Pos;
          if OrderReady = 1 then
          begin
            if ExtraUnitInfoTags[PUnitInfo(p_Unit.p_UnitInfo).nCategory].TeleportMethod <> tmNone then
            begin
              Distance := TAUnits.Distance(@p_Unit.Position, @TargetPosition);
              if Distance < ExtraUnitInfoTags[PUnitInfo(p_Unit.p_UnitInfo).nCategory].TeleportMinReloadTime then
                Distance := ExtraUnitInfoTags[PUnitInfo(p_Unit.p_UnitInfo).nCategory].TeleportMinReloadTime;
              CustomUnitFieldsArr[UnitID].TeleportReloadMax := Distance;
            end;

            NewX := PCardinal(@TargetPosition.X_)^;
            NewY := PCardinal(@TargetPosition.Y_)^;
            NewZ := PCardinal(@TargetPosition.Z_)^;

            if TAData.NetworkLayerEnabled then
              GlobalDPlay.Broadcast_NewUnitLocation(TAUnit.GetID(p_Unit), NewX, NewZ, NewY);
            UNITS_NewUnitPosition(p_Unit, NewX, NewY, NewZ, 1);
            if OrderPtr.lPar1 <> 0 then
              TASfx.EmitSfxFromPiece(OrderPtr.p_UnitTarget, p_Unit, 0, 8, True);
            NewZ := 1;
            if p_Unit.p_UnitScriptsData <> nil then
              TAUnit.CallCobProcedure(p_Unit, 'Teleporting', @Word(TargetPosition.X), @Word(TargetPosition.Z), @Word(TargetPosition.Y), @NewZ);
            Result := 5
          end else
          begin
            NewZ := 0;
            if p_Unit.p_UnitScriptsData <> nil then
              TAUnit.CallCobProcedure(p_Unit, 'Teleporting', @Word(TargetPosition.X), @Word(TargetPosition.Z), @Word(TargetPosition.Y), @NewZ);
            OrderPtr.nPaused := OrderPtr.nPaused or 1;
            OrderPtr.lRecallTime := 10 + TAData.GameTime;
            Result := 2;
          end;
        end;
    else Result := 7;
  end;

end;

procedure UnitActions_TeleportOrderWrapper;
asm
  mov     ecx, [esp+40h]
  mov     edi, [esp+3Ch]
  push    ecx
  push    edi
  push    esi
  call    UnitActions_TeleportOrder
  pop     edi
  pop     esi
  pop     ebp
  push $00406BE4;
  call PatchNJump;
end;

procedure UnitActions_TeleportReload(p_Unit: PUnitStruct); stdcall;
var
  UnitId : Cardinal;
begin
  UnitId := TAUnit.GetId(p_Unit);
  if CustomUnitFieldsArr[UnitId].TeleportReloadMax <> 0 then
    if CustomUnitFieldsArr[UnitId].TeleportReloadCur < CustomUnitFieldsArr[UnitId].TeleportReloadMax then
      Inc(CustomUnitFieldsArr[UnitId].TeleportReloadCur, 1)
    else begin
      CustomUnitFieldsArr[UnitId].TeleportReloadMax := 0;
      CustomUnitFieldsArr[UnitId].TeleportReloadCur := 0;
    end;
end;

procedure UnitActions_TeleportReloadWrapper;
asm
  pushAD
  push    esi
  call    UnitActions_TeleportReload
  popAD
  mov     ecx, [esi+TUnitStruct.p_UnitScriptsData]
  push $0048ADE5;
  call PatchNJump;
end;

procedure UnitActions_TeleportPositionOffset;
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

function AdvancedDefaultMission(p_Unit: PUnitStruct): PUnitOrder; stdcall;
var
  OrderMem : Pointer;
  Position : TPosition;
  //UnitID,
  UnitInfoID : Word;
begin
  OrderMem := MEM_Alloc(SizeOf(TUnitOrder));
  if OrderMem <> nil then
  begin
    UnitInfoID := p_Unit.p_UnitInfo.nCategory;
    //UnitID := TAUnit.GetID(p_Unit);

    if ExtraUnitInfoTags[UnitInfoID].DefaultMissionOrgPos then
      Position := p_Unit.Position
    else begin
      {
      GetTPosition(CustomUnitFieldsArr[UnitID].DefaultMissionPosX,
                   CustomUnitFieldsArr[UnitID].DefaultMissionPosZ,
                   Position);
      }
      Position.x_ := 0;
      Position.z_ := 0;
      Position.y_ := 0;
      Position.X  := 0;
      Position.Z  := 0;
      Position.Y  := 0;
    end;

    Result := ORDERS_CreateObject(nil, nil, OrderMem, 0,
                                  0, 0, @Position, nil,
                                  p_Unit.p_UnitInfo.cDefMissionType);
  end else
    Result := nil;
end;

procedure UnitActions_AdvancedDefaultMission;
asm
  push    ebx // old self order
  push    edi // unit
  call    AdvancedDefaultMission
  pop     ebx

  //push $0043BA88;
  push $0043BA05;
  call PatchNJump;
end;

procedure UnitActions_DontHealTimeNotBuilt;
label
  dont_heal,
  heal;
asm
  fld     [esi+TUnitStruct.fBuildTimeLeft]  // buildtimeleft
  fcomp   ds:$004FD748
  fnstsw  ax
  test    ah, 40h
  jz      dont_heal
heal :
  mov     ecx, [TaDynMemStructPtr]
  push $0048AF5E;
  call PatchNJump;
dont_heal :
  push $0048AF97;
  call PatchNJump;
end;

end.