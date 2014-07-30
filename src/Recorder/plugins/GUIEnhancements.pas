unit GUIEnhancements;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_GUIEnhancements : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallGUIEnhancements;
Procedure OnUninstallGUIEnhancements;

// -----------------------------------------------------------------------------

procedure ExtraUnitBars_MainCall;
procedure GUIEnhancements_DevUnitProbes;
procedure TrueIncomeHook;
procedure SelectedUnitsCounter;
procedure DrawCircleUnitSelectHook;
procedure DrawBuildSpot_InitScript;
procedure DrawBuildSpot_NanoframeHook;
procedure DrawBuildSpot_QueueNanoframeHook;
procedure DrawBuildSpot_NanoframeShimmerHook;

implementation
uses
  IniOptions,
  SysUtils,
  Windows,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations,
  COB_Extensions,
  UnitsExtend,
  Math,
  TA_FunctionsU;

const
  STANDARDUNIT : cardinal = 250;
  MEDIUMUNIT : cardinal = 1900;
  BIGUNIT : cardinal = 3000;
  HUGEUNIT : cardinal = 5000;
  EXTRALARGEUNIT : cardinal = 10000;
  VETERANLEVEL_RELOADBOOST = 12; // 30 * 0.2
  OFFSCREEN_off = -$1F0;

const
  NanoUnitCreateInit : AnsiString = 'NanoFrameInit';

var
  GUIEnhancementsPlugin: TPluginData;

Procedure OnInstallGUIEnhancements;
begin
  if IniSettings.Plugin_HBDynamicSize then
  begin
    if IniSettings.Plugin_HBCategory1 <> 0 then
      STANDARDUNIT := IniSettings.Plugin_HBCategory1;
    if IniSettings.Plugin_HBCategory2 <> 0 then
      MEDIUMUNIT := IniSettings.Plugin_HBCategory2;
    if IniSettings.Plugin_HBCategory3 <> 0 then
      BIGUNIT := IniSettings.Plugin_HBCategory3;
    if IniSettings.Plugin_HBCategory4 <> 0 then
      HUGEUNIT := IniSettings.Plugin_HBCategory4;
    if IniSettings.Plugin_HBCategory5 <> 0 then
      EXTRALARGEUNIT := IniSettings.Plugin_HBCategory5;
  end;
end;

Procedure OnUninstallGUIEnhancements;
begin
end;

function GetPlugin : TPluginData;

begin
  if IsTAVersion31 and State_GUIEnhancements then
  begin
    GUIEnhancementsPlugin := TPluginData.Create( True,
                            'GUIEnhancements',
                            State_GUIEnhancements,
                            @OnInstallGUIEnhancements,
                            @OnUninstallGUIEnhancements );

    if (IniSettings.Plugin_HBWidth <> 0) or
       IniSettings.Plugin_HBDynamicSize or
       (IniSettings.Plugin_MinWeaponReload <> 0) then
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'ExtraUnitBars_MainCall',
                            @ExtraUnitBars_MainCall,
                            $00469CB1, 1);

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'GUIEnhancements_DevUnitProbes',
                            @GUIEnhancements_DevUnitProbes,
                            $00469BD0, 1);

    if IniSettings.Plugin_TrueIncome then
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'TrueIncomeHook',
                            @TrueIncomeHook,
                            $004695EE, 0);

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'DrawCircleUnitSelectHook',
                            @DrawCircleUnitSelectHook,
                            $00467AF1, 0);

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'DrawBuildSpot_InitScript',
                            @DrawBuildSpot_InitScript,
                            $00485DE1, 0);

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'DrawBuildSpot_NanoframeHook',
                            @DrawBuildSpot_NanoframeHook,
                            $00469F23, 1);

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'DrawBuildSpot_QueueNanoframeHook',
                            @DrawBuildSpot_QueueNanoframeHook,
                            $00438C38, 0);

    if not IniSettings.Plugin_BuildSpotNanoShimmer then
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              'DrawBuildSpot_NanoframeShimmerHook',
                              @DrawBuildSpot_NanoframeShimmerHook,
                              $00458E18, 2);


   { GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'SelectedUnitsCounter',
                            @SelectedUnitsCounter,
                            $0046ABE2, 1); }

    Result:= GUIEnhancementsPlugin;
  end else
    Result := nil;
end;

function DrawUnitState(Offscreen_p: Cardinal; Unit_p: Pointer; CenterPosX : Integer; CenterPosZ: Integer) : Integer; stdcall;
var
  { drawing }
  ColorsPal : Pointer;
  v65 : LongInt;
  RectDrawPos : tagRect;
  // initial drawing position for this unit
  //PosX, PosZ, PosY : Integer;
  LocalUnit, AlliedUnit : Boolean;

  { health bar }
  HPBackgRectWidth, HPFillRectWidth: Smallint;
  UnitHealth, HealthState : Word;
  UnitMaxHP : Cardinal;
  
  UnitInfo : PUnitInfo;
  UnitBuildTimeLeft : Single;
  //UnitPos : TPosition;
  UnitId, MaxUnitId : Cardinal;

  { hotkey group }
  BottomZ : Word;
  sGroup : PAnsiChar;
  AllowHotkeyDraw : Boolean;
  
  { weapons }
  MaxReloadTime : Integer;
  CurReloadTime : Integer;
  BarProgress : Integer;
  StockPile : Byte;
  WeapReloadColor : Byte;
  CustomReloadBar : Boolean;

  { transporters }
  TransportCount : Integer;
  TransportCap : Integer;
  WeightTransportCur : Integer;
  WeightTransportMax : Integer;
  WeightTransportPercent : Integer;

  { reclaim, resurrect feature }
  CurOrder : TTAActionType;
  OrderStateCorrect : Boolean;
  FeatureDefPtr : Pointer;
  FeatureDefID : Integer;
  ActionUnitType : Word;
  ActionUnitBuildTime : Cardinal;
  ActionUnitBuildCost1, ActionUnitBuildCost2 : Single;
  ActionTime : Double;
  ActionWorkTime : Word;
  MaxActionVal : Cardinal;
  CurActionVal : Cardinal;
  ReclaimColor : Byte;
  UnknownFeat : Pointer;
begin
  Result := 0;
  BottomZ := 0;
  // is drawing health bars enabled and any of local player units are actually on screen
  if ((PTAdynmemStruct(TAData.MainStructPtr)^.GameOptionMask and 1) = 1) or
     (PUnitStruct(Unit_p).HotKeyGroup <> 0) then
  begin
    UnitInfo := PUnitStruct(Unit_p).p_UnitDef;
    ColorsPal := Pointer(LongWord(TAData.MainStructPtr)+$DCB);
    
    if ((PTAdynmemStruct(TAData.MainStructPtr)^.GameOptionMask and 1) = 1) and
       (CenterPosX <> 0) and
       (CenterPosZ <> 0) and
       (UnitInfo <> nil) then
    begin
      //AlliedUnit := TAPlayer.GetAlliedState(TAUnit.GetOwnerPtr(Unit_p), TAData.ViewPlayer);
      LocalUnit := (PPlayerStruct(PUnitStruct(Unit_p).p_Owner).cPlayerIndexZero = TAData.ViewPlayer);
//      DrawTransparentBox(Offscreen_p, @Rect, -24);
      if LocalUnit then
      begin
        UnitHealth := PUnitStruct(Unit_p).nHealth;
        UnitBuildTimeLeft := PUnitStruct(Unit_p).lBuildTimeLeft;

        if (UnitHealth > 0) then
        begin
          UnitMaxHP := UnitInfo.lMaxHP;
          HPBackgRectWidth := 34;
          if IniSettings.Plugin_HBDynamicSize then
          begin
            if UnitMaxHP < STANDARDUNIT then
              HPBackgRectWidth := 28
            else
              if (UnitMaxHP >= STANDARDUNIT) and (UnitMaxHP < MEDIUMUNIT) then
                HPBackgRectWidth := 34
              else
                if (UnitMaxHP >= MEDIUMUNIT) and (UnitMaxHP < BIGUNIT) then
                  HPBackgRectWidth := 40
                else
                  if (UnitMaxHP >= BIGUNIT) and (UnitMaxHP < HUGEUNIT) then
                    HPBackgRectWidth := 46
                  else
                    if (UnitMaxHP >= HUGEUNIT) and (UnitMaxHP < EXTRALARGEUNIT) then
                      HPBackgRectWidth := 52
                    else
                      if UnitMaxHP >= HUGEUNIT then
                        HPBackgRectWidth := 58;
          end else
          begin
            if (IniSettings.Plugin_HBWidth <> 0) then
              HPBackgRectWidth := IniSettings.Plugin_HBWidth
          end;

          if (UnitHealth <= UnitMaxHP) and
             not ExtraUnitDefTags[UnitInfo.nCategory].HideHPBar then
          begin
            HPFillRectWidth := (HPBackgRectWidth div 2);

            RectDrawPos.Left := Word(CenterPosX) - HPFillRectWidth;
            RectDrawPos.Right := Word(CenterPosX) + HPFillRectWidth;
            RectDrawPos.Top := Word(CenterPosZ) - 2;
            RectDrawPos.Bottom := Word(CenterPosZ) + 2;

            DrawBar(Offscreen_p, @RectDrawPos, PByte(ColorsPal)^);
            Inc(RectDrawPos.Top);
            Dec(RectDrawPos.Bottom);
            Inc(RectDrawPos.Left);

            RectDrawPos.Right := Round(RectDrawPos.Left + ((HPBackgRectWidth-2) * UnitHealth) / UnitMaxHP);
            HealthState := UnitMaxHP div 3;

            if IniSettings.Plugin_Colors and (IniSettings.Colors[Ord(UNITHEALTHBARGOOD)] <> 0) then
            begin
              if UnitHealth <= (HealthState * 2) then
              begin
                if UnitHealth <= HealthState then
                  DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+IniSettings.Colors[Ord(UNITHEALTHBARLOW)])^)  // low
                else
                  DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+IniSettings.Colors[Ord(UNITHEALTHBARMEDIUM)])^); // yellow
              end else
                DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+IniSettings.Colors[Ord(UNITHEALTHBARGOOD)])^); // good
            end else
            begin
              if UnitHealth <= (HealthState * 2) then
              begin
                if UnitHealth <= HealthState then
                  DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+12)^)  // low
                else
                  DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+14)^); // medium
              end else
                DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+10)^); // good
            end;
          end;

        // weapons reload
        if (IniSettings.Plugin_MinWeaponReload <> 0) and
           (UnitBuildTimeLeft = 0.0) then
        begin
          if (PUnitStruct(Unit_p).UnitWeapons[0].p_Weapon <> nil) or
             (PUnitStruct(Unit_p).UnitWeapons[1].p_Weapon <> nil) or
             (PUnitStruct(Unit_p).UnitWeapons[2].p_Weapon <> nil) then
          begin
            MaxReloadTime := 0;
            CurReloadTime := 0;
            StockPile := 0;

            if (PUnitStruct(Unit_p).UnitWeapons[0].p_Weapon <> nil) then
            begin
              if (PWeaponDef(PUnitStruct(Unit_p).UnitWeapons[0].p_Weapon).lWeaponTypeMask and (1 shl 28) = 1 shl 28) then
                StockPile := 1;
              if PWeaponDef(PUnitStruct(Unit_p).UnitWeapons[0].p_Weapon).nReloadTime >= IniSettings.Plugin_MinWeaponReload * 30 then
              begin
                MaxReloadTime := PWeaponDef(PUnitStruct(Unit_p).UnitWeapons[0].p_Weapon).nReloadTime;
                CurReloadTime := MaxReloadTime - PUnitStruct(Unit_p).UnitWeapons[0].nReloadTime;
              end;
            end;
            if (PUnitStruct(Unit_p).UnitWeapons[1].p_Weapon <> nil) then
            begin
              if (PWeaponDef(PUnitStruct(Unit_p).UnitWeapons[1].p_Weapon).lWeaponTypeMask and (1 shl 28) = 1 shl 28) then
                StockPile := 2;
              if PWeaponDef(PUnitStruct(Unit_p).UnitWeapons[1].p_Weapon).nReloadTime >= IniSettings.Plugin_MinWeaponReload * 30 then
              begin
                MaxReloadTime := PWeaponDef(PUnitStruct(Unit_p).UnitWeapons[1].p_Weapon).nReloadTime;
                CurReloadTime := MaxReloadTime - PUnitStruct(Unit_p).UnitWeapons[1].nReloadTime;
              end;
            end;
            if (PUnitStruct(Unit_p).UnitWeapons[2].p_Weapon <> nil) then
            begin
              if (PWeaponDef(PUnitStruct(Unit_p).UnitWeapons[2].p_Weapon).lWeaponTypeMask and (1 shl 28) = 1 shl 28) then
                StockPile := 3;
              if PWeaponDef(PUnitStruct(Unit_p).UnitWeapons[2].p_Weapon).nReloadTime >= IniSettings.Plugin_MinWeaponReload * 30 then
              begin
                MaxReloadTime := PWeaponDef(PUnitStruct(Unit_p).UnitWeapons[2].p_Weapon).nReloadTime;
                CurReloadTime := MaxReloadTime - PUnitStruct(Unit_p).UnitWeapons[2].nReloadTime;
              end;
            end;

            // custom weapon reload bar progress
            CustomReloadBar := False;
            if Assigned(CustomUnitInfosArray) then
              if CustomUnitFieldsArr[TAUnit.GetId(Pointer(Unit_p))].LongID = TAUnit.GetLongId(Pointer(Unit_p)) then
                if CustomUnitFieldsArr[TAUnit.GetId(Pointer(Unit_p))].CustomWeapReload then
                begin
                  MaxReloadTime := CustomUnitFieldsArr[TAUnit.GetId(Pointer(Unit_p))].CustomWeapReloadMax;
                  CurReloadTime := CustomUnitFieldsArr[TAUnit.GetId(Pointer(Unit_p))].CustomWeapReloadCur;
                  CustomReloadBar := True;
                end;

            if MaxReloadTime <> 0 then
            begin
              BottomZ := 6;
              if (PUnitStruct(Unit_p).nKills >= 5) and
                 not CustomReloadBar then
              begin
                CurReloadTime := CurReloadTime - VETERANLEVEL_RELOADBOOST;
                MaxReloadTime := MaxReloadTime - VETERANLEVEL_RELOADBOOST;
              end;

              if CurReloadTime < 0 then
                CurReloadTime := 0;

              // stockpile weapon build progress instead of reload bar
              if (StockPile <> 0) and
                 IniSettings.Plugin_Stockpile and
                 not CustomReloadBar then
              begin
                if PUnitStruct(Unit_p).p_FutureOrder <> nil then
                begin
                  MaxReloadTime := 100;
                  CurReloadTime := GetUnit_BuildWeaponProgress(pointer(Unit_p));
                end else
                  MaxReloadTime := 0; // disable drawing bar
              end;

              if MaxReloadTime > 0 then
              begin
                RectDrawPos.Left := Word(CenterPosX) - 17;
                RectDrawPos.Right := Word(CenterPosX) + 17;
                RectDrawPos.Top := Word(CenterPosZ) + 5 - 2;
                RectDrawPos.Bottom := Word(CenterPosZ) + 5 + 2;
              {  if StockPile <> 0 then
                  DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+4)^)
                else   }
                DrawBar(Offscreen_p, @RectDrawPos, PByte(ColorsPal)^);

                Inc(RectDrawPos.Top);
                Dec(RectDrawPos.Bottom);
                Inc(RectDrawPos.Left);
                RectDrawPos.Right := Round(RectDrawPos.Left + (32 * CurReloadTime) / MaxReloadTime);

                if StockPile <> 0 then
                begin
                  if IniSettings.Plugin_Colors and (IniSettings.Colors[Ord(STOCKPILEBAR)] <> 0) then
                    DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+IniSettings.Colors[Ord(STOCKPILEBAR)])^)
                  else
                    DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+14)^);
                end else
                begin
                  BarProgress := Round((CurReloadTime / MaxReloadTime) * 100);
                  if IniSettings.Plugin_Colors and (IniSettings.Colors[Ord(WEAPONRELOADBAR)] <> 0) then
                    WeapReloadColor := IniSettings.Colors[Ord(WEAPONRELOADBAR)]
                  else
                    WeapReloadColor := 143;
                  case BarProgress of
                     0..15 : result := DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor)^);
                    16..30 : result := DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 1)^);
                    31..47 : result := DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 2)^);
                    48..64 : result := DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 3)^);
                    65..80 : result := DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 4)^);
                    81..94 : result := DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 5)^);
                   95..100 : result := DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 6)^);
                  end;
                end;
              end;
            end;
          end; { Unit got any weapons }
        end; { IniSettings.Plugin_WeaponReloadTimeBar }

        end; {UnitHealth > 0}

        if IniSettings.Plugin_MinReclaimTime > 0 then
        begin
          CurOrder := TAUnit.GetCurrentOrderType(Pointer(Unit_p));
          if (CurOrder = Action_Reclaim) or
             (CurOrder = Action_VTOL_Reclaim) or
             (CurOrder = Action_Capture) or
             (CurOrder = Action_Resurrect) then
          begin
            if (CurOrder = Action_Reclaim) or
               (CurOrder = Action_Resurrect) or
               (CurOrder = Action_Capture) then
              OrderStateCorrect := TAUnit.GetCurrentOrderState(Pointer(Unit_p)) and $400000 = $400000
            else
              OrderStateCorrect := TAUnit.GetCurrentOrderState(Pointer(Unit_p)) and $100000 = $100000;
            if OrderStateCorrect then
            begin
              if CurOrder = Action_Capture then
                FeatureDefID := 0
              else
                FeatureDefID := GetFeatureTypeOfOrder(@PUnitOrder(PUnitStruct(Pointer(Unit_p)).p_UnitOrders).Pos, PUnitStruct(Pointer(Unit_p)).p_UnitOrders, LongWord(@UnknownFeat));
              if FeatureDefID <> -1 then
              begin
                MaxActionVal := 0;
                CurActionVal := 0;
                case CurOrder of
                  Action_Reclaim :
                  begin
                    FeatureDefPtr := TAMem.FeatureDefId2Ptr(FeatureDefID);
                    MaxActionVal := Trunc( (PFeatureDefStruct(FeatureDefPtr).metal + PFeatureDefStruct(FeatureDefPtr).energy) / 2 + 15);
                    CurActionVal := TAUnit.GetCurrentOrderParams(Pointer(Unit_p), 1);
                  end;
                  Action_VTOL_Reclaim :
                  begin
                    FeatureDefPtr := TAMem.FeatureDefId2Ptr(FeatureDefID);
                    MaxActionVal := Trunc( (PFeatureDefStruct(FeatureDefPtr).metal + PFeatureDefStruct(FeatureDefPtr).energy) / 2 + 30);
                    CurActionVal := TAUnit.GetCurrentOrderParams(Pointer(Unit_p), 1);
                  end;
                  Action_Capture:
                  begin
                    ActionUnitType := TAUnit.GetUnitInfoId(TAUnit.GetCurrentOrderTargetUnit(Pointer(Unit_p)));
                    if ActionUnitType <> 0 then
                    begin
                      ActionUnitBuildCost1 := PUnitInfo(TAMem.UnitInfoId2Ptr(ActionUnitType)).lBuildCostEnergy * 30 * 0.00050000002;
                      ActionUnitBuildCost2 := PUnitInfo(TAMem.UnitInfoId2Ptr(ActionUnitType)).lBuildCostMetal * 30 * -0.0071428572;
                      ActionTime := (ActionUnitBuildCost1 - ActionUnitBuildCost2) + 150;

                      MaxActionVal := Round(ActionTime);
                      CurActionVal := MaxActionVal - TAUnit.GetCurrentOrderParams(Pointer(Unit_p), 1);
                    end;
                  end;
                  Action_Resurrect :
                  begin
                    FeatureDefPtr := TAMem.FeatureDefId2Ptr(FeatureDefID);
                    ActionUnitType := TAUnit.GetCurrentOrderParams(Pointer(Unit_p), 1);
                    if ActionUnitType <> 0 then
                    begin
                      ActionUnitBuildTime := PUnitInfo(TAMem.UnitInfoId2Ptr(ActionUnitType)).lBuildTime;
                      ActionWorkTime := PUnitInfo(PUnitStruct(Pointer(Unit_p)).p_UnitDef).nWorkerTime div 30;
                      ActionTime := (ActionUnitBuildTime * 0.3) / ActionWorkTime;
                      MaxActionVal := Trunc(ActionTime);
                      CurActionVal := TAUnit.GetCurrentOrderParams(Pointer(Unit_p), 2);
                    end;
                  end;
                end;

                if (MaxActionVal > 0) and
                   (CurActionVal > 0) and
                   (CurActionVal <= MaxActionVal) and  // TA changes par1 to it once feature is reclaimed but order state remains "reclaiming"...
                   (MaxActionVal >= LongWord(IniSettings.Plugin_MinReclaimTime * 30)) then
                begin
                  RectDrawPos.Left := Word(CenterPosX) - 17;
                  RectDrawPos.Right := Word(CenterPosX) + 17;
                  RectDrawPos.Top := Word(CenterPosZ) - 7;
                  RectDrawPos.Bottom := Word(CenterPosZ) - 3;

                  DrawBar(Offscreen_p, @RectDrawPos, PByte(ColorsPal)^);

                  Inc(RectDrawPos.Top);
                  Dec(RectDrawPos.Bottom);
                  Inc(RectDrawPos.Left);
                  RectDrawPos.Right := Round(RectDrawPos.Left + (32 * CurActionVal) / MaxActionVal);

                  BarProgress := Round((CurActionVal / MaxActionVal) * 100);
                  if IniSettings.Plugin_Colors and (IniSettings.Colors[Ord(RECLAIMBAR)] <> 0) then
                    ReclaimColor := IniSettings.Colors[Ord(RECLAIMBAR)]
                  else
                    ReclaimColor := 17;
                  case BarProgress of
                    0..20 : result := DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor)^);
                   21..40 : result := DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor + 1)^);
                   41..60 : result := DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor + 2)^);
                   61..80 : result := DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor + 3)^);
                  81..100 : result := DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor + 4)^);
                  end;
                end;
              end;
            end;
          end;
        end;
        //TAMem.GetFeatureDef($5C)

        // built weapons counter (nukes)
        if IniSettings.Plugin_Stockpile then
          if PUnitStruct(Unit_p).UnitWeapons[0].cStock > 0 then
            DrawText_Heavy(Offscreen_p, PAnsiChar(IntToStr(PUnitStruct(Unit_p).UnitWeapons[0].cStock)), Word(CenterPosX), Word(CenterPosZ) - 13, -1);

        // transporter count
        if IniSettings.Plugin_Transporters then
        begin
          if (UnitInfo.UnitTypeMask and 2048 = 2048) then   // unit is air
          begin
            if (ExtraUnitDefTags[UnitInfo.nCategory].MultiAirTransport = 0) then
              TransportCap := 0
            else
              if (ExtraUnitDefTags[UnitInfo.nCategory].TransportWeightCapacity = 0) then
              begin
                TransportCap := UnitInfo.cTransportCap;
              end else
              begin
                TransportCap := 0;
              end;
          end else
          begin
            TransportCap := UnitInfo.cTransportCap;
          end;

          if TransportCap > 0 then
          begin
          //  MaxUnitId := TAMem.GetMaxUnitId;
          {  for UnitId := 1 to MaxUnitId do
            begin
              if LongWord(PUnitStruct(TAUnit.Id2Ptr(UnitId)).p_TransporterUnit) = LongWord(Unit_p) then
                Inc(TransportCount);
            end;     }
            TransportCount := TAUnit.GetLoadCurAmount(Unit_p);
            if TransportCount > 0 then
              DrawText_Heavy(Offscreen_p, PAnsiChar(IntToStr(TransportCount) + '/' + IntToStr(TransportCap)), Word(CenterPosX) - 10, Word(CenterPosZ) - 70, -1);
          end else
          begin
            WeightTransportMax := ExtraUnitDefTags[UnitInfo.nCategory].TransportWeightCapacity;
            if WeightTransportMax > 0 then
            begin
              WeightTransportCur := TAUnit.GetLoadWeight(Unit_p);
              TransportCap := UnitInfo.cTransportCap;
              TransportCount := TAUnit.GetLoadCurAmount(Unit_p);

              if (WeightTransportCur > 0) then
              begin
                WeightTransportPercent := Round((WeightTransportCur / WeightTransportMax) * 100);
                if WeightTransportPercent > 100 then
                  WeightTransportPercent := 100;
                if TransportCount = TransportCap then
                  WeightTransportPercent := 100;
                v65 := sub_4C13F0;
                if (WeightTransportPercent > 85) then
                  SetFontColor(PByte(LongWord(ColorsPal)+18)^, v65)
                else
                  if (WeightTransportPercent > 50) then
                    SetFontColor(PByte(LongWord(ColorsPal)+17)^, v65)
                  else
                    SetFontColor(PByte(LongWord(ColorsPal)+16)^, v65);

                DrawText_Heavy(Offscreen_p,
                               PAnsiChar(IntToStr(WeightTransportPercent) + '%'),
                               Word(CenterPosX) - 10, Word(CenterPosZ) - 70, -1);
                SetFontColor(PByte(LongWord(ColorsPal)+255)^, v65);
              end;
            end;
          end;

        end;
      end;

      if (PPlayerStruct(PUnitStruct(Unit_p).p_Owner).cPlayerIndexZero = TAData.ViewPlayer) and
         (PUnitStruct(Unit_p).HotKeyGroup <> 0) then
      begin
        AllowHotkeyDraw := False;
        if PUnitStruct(Unit_p).p_TransporterUnit <> nil then
        begin
          if PUnitStruct(PUnitStruct(Unit_p).p_TransporterUnit).HotKeyGroup = 0 then
            AllowHotkeyDraw := True;
        end else
          AllowHotkeyDraw := True;
          
        if AllowHotkeyDraw then
        begin
          sGroup := PAnsiChar(PUnitStruct(Unit_p).HotKeyGroup + 48);
          DrawText_Heavy(Offscreen_p, @sGroup, Word(CenterPosX), Word(CenterPosZ) + 3 + BottomZ, -1);
        end;
      end;
    end; { Bars enabled }
  end;
end;

function DrawTrueIncome(OFFSCREEN_ptr: Cardinal; RaceSideData: PRaceSideDataStruct; ViewResBar: PViewResBar; MaxWidth: Integer): LongInt; stdcall;
var
  ResourceIncome : Single;
  PlayerResourceString : PAnsiChar;
  ColorsPal : Pointer;
  v65 : LongInt;
  FormatSettings: TFormatSettings;
begin
  Result := 0;
  ColorsPal := Pointer(LongWord(TAData.MainStructPtr)+$DCB);
  FormatSettings.DecimalSeparator := '.';
  v65 := sub_4C13F0;

  ResourceIncome := ViewResBar.fEnergyProduction - ViewResBar.fEnergyExpense;
  if ResourceIncome > 0.0 then
  begin
    SetFontColor(PByte(LongWord(ColorsPal)+10)^, v65);
    if ResourceIncome < 10000 then
      PlayerResourceString := PAnsiChar(Format('+%.0f', [ResourceIncome], FormatSettings))
    else
    begin
      ResourceIncome := ResourceIncome / 1000;
      PlayerResourceString := PAnsiChar(Format('+%.0fK', [ResourceIncome], FormatSettings));
    end;
  end else
  begin
    SetFontColor(PByte(LongWord(ColorsPal)+12)^, v65);
    if ResourceIncome > -10000 then
      PlayerResourceString := PAnsiChar(Format('%.0f', [ResourceIncome], FormatSettings))
    else
    begin
      ResourceIncome := ResourceIncome / 1000;
      PlayerResourceString := PAnsiChar(Format('%.0fK', [ResourceIncome], FormatSettings));
    end;
  end;
  DrawText_Heavy(OFFSCREEN_ptr, PlayerResourceString, RaceSideData.rectEnergyBar.Left, RaceSideData.rectEnergyNum.Top, MaxWidth);

  ResourceIncome := ViewResBar.fMetalProduction - ViewResBar.fMetalExpense;
  if ResourceIncome > 0.0 then
  begin
    SetFontColor(PByte(LongWord(ColorsPal)+10)^, v65);
    if ResourceIncome < 10000 then
      PlayerResourceString := PAnsiChar(Format('+%.1f', [ResourceIncome], FormatSettings))
    else
    begin
      ResourceIncome := ResourceIncome / 1000;
      PlayerResourceString := PAnsiChar(Format('+%.1fK', [ResourceIncome], FormatSettings));
    end;
  end else
  begin
    SetFontColor(PByte(LongWord(ColorsPal)+12)^, v65);
    if ResourceIncome > -10000 then
      PlayerResourceString := PAnsiChar(Format('%.1f', [ResourceIncome], FormatSettings))
    else
    begin
      ResourceIncome := ResourceIncome / 1000;
      PlayerResourceString := PAnsiChar(Format('%.1fK', [ResourceIncome], FormatSettings));
    end;
  end;
  DrawText_Heavy(OFFSCREEN_ptr, PlayerResourceString, RaceSideData.rectMetalBar.Left, RaceSideData.rectMetalNum.Top, MaxWidth);
end;

procedure DrawDevUnitStateProbes(Offscreen_p : Cardinal); stdcall;
begin
  //DrawTranspRectangle(Offscreen_p, @Rect, PByte(LongWord(ColorsPal)+10)^);
  if TAData.DevMode then
  begin
    if PTAdynmemStruct(TAData.MainStructPtr)^.field_391B3 <> 0 then
      UnitStateProbe(Offscreen_p);
    if PTAdynmemStruct(TAData.MainStructPtr)^.field_391B9 <> 0 then
      UnitBuilderProbe(Offscreen_p);
  end;
end;

procedure GUIEnhancements_DevUnitProbes;
asm
  lea     eax, [esp+224h+OFFSCREEN_off]
  pushAD
  push    eax
  call DrawDevUnitStateProbes
  popAD
  push 8
  push $00469BD6;
  call PatchNJump;
end;

procedure ExtraUnitBars_MainCall;
asm
  lea     eax, [esp+224h+OFFSCREEN_off]
  push    edx             // PosY
  push    ebp             // PosX
  push    edi             // p_Unit
  push    eax             // OFFSCREEN_ptr
  call    DrawUnitState
  push $00469CFE;
  call PatchNJump;
end;

procedure TrueIncomeHook;
asm
  mov     ecx, [esi+126h]
  mov     edx, [esi+122h]
  push    0FFFFFFFFh      // MaxWidth
  push    ecx             // top
  lea     eax, [esp+22Ch-$170]
  push    edx             // left
  lea     ecx, [esp+230h+OFFSCREEN_off]
  push    eax             // Source
  push    ecx             // OFFSCREEN_ptr
  call    DrawText_Heavy
  lea     ecx, [esp+224h-$1AC]
  mov     edx, esi
  push    0FFFFFFFFh      // MaxWidth
  push    ecx             // ViewResBar
  //lea     eax, [esp+22Ch-$170]
  push    edx             // RaceSideData
  lea     ecx, [esp+230h+OFFSCREEN_off]
  //push    eax             // Source
  push    ecx             // OFFSCREEN_ptr
  call    DrawTrueIncome
  push $00469610;
  call PatchNJump;
end;

function DrawCircleUnitSelect(Unitunk: PPosition; Offscreen_p : Cardinal; Botx1, Boty1, Rx2, Ry2 : Integer; UnitVolume: Cardinal): Integer; stdcall;
var
  UnitPtr : PUnitStruct;
  UnitInfo : PUnitInfo;
  Radius : Extended;
  ColorsPal : Pointer;
  CenterX, CenterY : Integer;
  lSpacing : Cardinal;
  nRadiusJig : Byte;
  lRadiusJigTmp : Integer;
  CurOrder : TTAActionType;
begin
  Result := 0;
  UnitPtr := Pointer(UnitVolume - $64);
  UnitInfo := UnitPtr.p_UnitDef;

  CenterX := Round((Rx2+Botx1)/2);
  CenterY := Round((Boty1+Ry2)/2);

  Radius := UnitInfo.lWidthHypot div 32768;
  {if (Rx2 >= BotX1) and (Ry2 >= Boty1) then
    Radius := Hypot(Rx2-Botx1, Ry2-Boty1)
  else
    if (Rx2 >= BotX1) and (Ry2 <= Boty1) then
      Radius := Hypot(Rx2-Botx1, Boty1-Ry2)
    else
      if (Rx2 <= BotX1) and (Ry2 >= Boty1) then
        Radius := Hypot(Botx1-Rx2, Ry2-Boty1)
      else
        Radius := Hypot(Botx1-Rx2, Boty1-Ry2);   }

  ////if CustomUnitFieldsArr[TAUnit.GetId(pointer(UnitPtr))].CircleSelectTick >= 2 then
  //begin
  //  CustomUnitFieldsArr[TAUnit.GetId(pointer(UnitPtr))].CircleSelectTick := 0;
    if CustomUnitFieldsArr[TAUnit.GetId(UnitPtr)].CircleSelectProgress = Low(Cardinal) then
      CustomUnitFieldsArr[TAUnit.GetId(UnitPtr)].CircleSelectProgress := High(Cardinal);
    Dec(CustomUnitFieldsArr[TAUnit.GetId(UnitPtr)].CircleSelectProgress);
  //end else
  //  Inc(CustomUnitFieldsArr[TAUnit.GetId(pointer(UnitPtr))].CircleSelectTick);

  lSpacing := CustomUnitFieldsArr[TAUnit.GetId(UnitPtr)].CircleSelectProgress;
  ColorsPal := Pointer(LongWord(TAData.MainStructPtr)+$DCB);

  CurOrder := TAUnit.GetCurrentOrderType(UnitPtr);
  if (CurOrder = Action_Ready) or
     (CurOrder = Action_Standby) or
     (CurOrder = Action_VTOL_Standby) or
     (CurOrder = Action_Wait) or
     (CurOrder = Action_Guard_NoMove) or
     (CurOrder = Action_NoResult) then
  begin
    nRadiusJig := CustomUnitFieldsArr[TAUnit.GetId(UnitPtr)].CircleSelectRadJig;
    lRadiusJigTmp := nRadiusJig - 1;
    if lRadiusJigTmp > Low(Byte) then
    begin
      nRadiusJig := lRadiusJigTmp;
      CustomUnitFieldsArr[TAUnit.GetId(UnitPtr)].CircleSelectRadJig := nRadiusJig;
    end else
      nRadiusJig := Low(Byte);

    DrawDotteCircle(Offscreen_p, CenterX, CenterY,
                              Round(Radius / 2 + nRadiusJig / 16),
                              PByte(LongWord(ColorsPal)+10)^,
                              Round(Radius / 2),
                              lSpacing div 200);

  end else
  begin
    nRadiusJig := CustomUnitFieldsArr[TAUnit.GetId(UnitPtr)].CircleSelectRadJig;
    lRadiusJigTmp := nRadiusJig + 1;

    if lRadiusJigTmp >= 64 then
    begin
      nRadiusJig := 64;
      CustomUnitFieldsArr[TAUnit.GetId(UnitPtr)].CircleSelectRadJig := nRadiusJig;
    end else
      CustomUnitFieldsArr[TAUnit.GetId(UnitPtr)].CircleSelectRadJig := lRadiusJigTmp;

    Result := DrawDotteCircle(Offscreen_p, CenterX, CenterY,
                              Round(Radius / 2 + nRadiusJig / 16),
                              PByte(LongWord(ColorsPal)+10)^,
                              Round(Radius / 2),
                              lSpacing div 100);
  end;
end;

procedure DrawCircleUnitSelectHook;
label
  Rectangle,
  Circle,
  Return;
asm
  push    eax
  movzx   eax, IniSettings.Plugin_CircleUnitSelect
  test    eax, eax
  pop     eax
  jnz     Circle
Rectangle:
  // BOTTOM
  push    edi             // Color
  mov     ecx, [esi+0Ch]
  mov     edx, [esi+8]
  mov     eax, [esi+4]
  push    ecx             // y2
  mov     ecx, [esi]
  push    edx             // x2
  push    eax             // y1
  push    ecx             // x1
  push    ebx             // OFFSCREEN_ptr
  call    DrawLine

  // RIGHT
  mov     edx, [esi+14h]
  mov     eax, [esi+10h]
  mov     ecx, [esi+0Ch]
  push    edi             // Color
  push    edx             // y2
  mov     edx, [esi+8]
  push    eax             // x2
  push    ecx             // y1
  push    edx             // x1
  push    ebx             // OFFSCREEN_ptr
  call    DrawLine

  // TOP
  mov     eax, [esi+1Ch]
  mov     ecx, [esi+18h]
  mov     edx, [esi+14h]
  push    edi             // Color
  push    eax             // y2
  mov     eax, [esi+10h]
  push    ecx             // x2
  push    edx             // y1
  push    eax             // x1
  push    ebx             // OFFSCREEN_ptr
  call    DrawLine

  // LEFT
  mov     ecx, [esi+4]
  mov     edx, [esi]
  mov     eax, [esi+1Ch]
  push    edi             // Color
  push    ecx             // y2
  mov     ecx, [esi+18h]
  push    edx             // x2
  push    eax             // y1
  push    ecx             // x1
  push    ebx             // OFFSCREEN_ptr
  call    DrawLine
  jmp Return
Circle:
  mov     eax, [esp+24h]
  push    eax
  mov     ecx, [esi+10h]  // r x2
  mov     edx, [esi+14h]  // r y2
  push    edx
  push    ecx
  mov     eax, [esi]      // bot x1
  mov     ecx, [esi+4]    // bot y1
  push    ecx
  push    eax
  push    ebx
  sub     ebp, 48
  push    ebp
  call    DrawCircleUnitSelect
Return:
  push $00467B4B;
  call PatchNJump;
end;

procedure DrawBuildSpot_InitScript;
label
  CallInitScript;
asm
  push    ebx
  movzx   ebx, word ptr [esi+TUnitStruct.nKills]
  test    bx, bx
  pop     ebx
  jnz     CallInitScript
  push    $00508BE0 //"Create"
  push $00485DE6;
  call PatchNJump;
CallInitScript:
  push    NanoUnitCreateInit //"NanoFrameInit"
  push $00485DE6;
  call PatchNJump;
end;

procedure PrepareNanoUnit(UnitPtr: PUnitStruct);
var
  UnitInfo : PUnitInfo;
begin
  UnitInfo := UnitPtr.p_UnitDef;
  if UnitInfo <> nil then
  begin
    UnitInfo.UnitTypeMask := UnitInfo.UnitTypeMask or $2000000;
    UnitInfo.nSightDistance := 0;
    UnitInfo.nRadarDistance := 0;
    UnitInfo.nSonarDistance := 0;
    UnitInfo.nSonarDistanceJam := 0;
    UnitInfo.nRadarDistanceJam := 0;
    UnitInfo.cMakesMetal := 0;
    UnitInfo.fEnergyMake := 0.0;
    UnitInfo.fEnergyUse := 0.0;
    UnitInfo.fMetalMake := 0.0;
    UnitInfo.fMetalUse := 0.0;
    UnitInfo.fWindGenerator := 0.0;
    UnitInfo.fTidalGenerator := 0.0;
    UnitInfo.lEnergyStorage := 0;
    UnitInfo.lMetalStorage := 0;
  end;
end;

procedure FreeUnitMem(UnitPtr: PUnitStruct);
  procedure FreeCob(pUnit: PUnitStruct);
  asm
    mov     esi, pUnit
    mov     ecx, [esi+TUnitStruct.p_UnitScriptsData]
    mov     eax, [ecx]
    push    1
    call    dword ptr [eax+50h] // release units script
  end;
var
  CobScript : Pointer;
 // ObjectStateFreeRes : Integer;
begin
  FreeUnitOrders(UnitPtr);
  CobScript := UnitPtr.p_UnitScriptsData;
  if CobScript <> nil then
  begin
    FreeCob(UnitPtr);
    UnitPtr.p_UnitScriptsData := nil;
  end;
  if UnitPtr.p_Object3DO <> nil then
  begin
    FreeObjectState(UnitPtr.p_Object3DO);
    UnitPtr.p_Object3DO := nil;
  end;
  if UnitPtr.p_MovementClass <> nil then
  begin
    FreeMoveClass(UnitPtr.p_MovementClass);
    free_MMwapper__(UnitPtr.p_MovementClass);
    UnitPtr.p_MovementClass := nil;
  end;
  UnitPtr.nUnitInfoID := 0;
  ZeroMemory(UnitPtr, SizeOf(TUnitStruct));
end;

procedure DrawBuildSpotNanoframe(pOffscreen: Cardinal); stdcall;
var
  X, Z : Integer;
  Position : TPosition;
begin
  if NanoSpotUnitSt.nUnitInfoID <> 0 then
    FreeUnitMem(@NanoSpotUnitSt);

  if (PTAdynmemStruct(TAData.MainStructPtr)^.nBuildNum <> 0) and
     (PTAdynmemStruct(TAData.MainStructPtr)^.ucPrepareOrderType = $E) then
  begin
    NanoSpotUnitSt.nUnitInfoID := PTAdynmemStruct(TAData.MainStructPtr)^.nBuildNum;
    NanoSpotUnitSt.p_UnitDef := TAMem.UnitInfoId2Ptr(NanoSpotUnitSt.nUnitInfoID);
    if (GetUnitExtProperty(@NanoSpotUnitSt, 6) <> 0) or
       IniSettings.Plugin_ForceDrawBuildSpotNano then
    begin
      X := PTAdynmemStruct(TAData.MainStructPtr)^.lBuildPosRealX;
      Z := PTAdynmemStruct(TAData.MainStructPtr)^.lBuildPosRealY;
      if (X < 0) or (Z < 0) then
        Exit;
      GetTPosition(X, Z, Position);
      NanoSpotUnitSt.p_Owner := TAPlayer.GetPlayerByIndex(TAData.ViewPlayer);
      if Unit_CreateUnitsInGame(@NanoSpotUnitSt, X, Position.Y, Z, 0) then
      begin
        NanoSpotUnitInfoSt := PUnitInfo(TAMem.UnitInfoId2Ptr(NanoSpotUnitSt.nUnitInfoID))^;
        NanoSpotUnitSt.p_UnitDef := @NanoSpotUnitInfoSt;

        NanoSpotUnitSt.Position.X := NanoSpotUnitSt.Position.x_ + NanoSpotUnitSt.nFootPrintX * 8;
        NanoSpotUnitSt.Position.x_ := 0;
        NanoSpotUnitSt.Position.Z := NanoSpotUnitSt.Position.z_ + NanoSpotUnitSt.nFootPrintZ * 8;
        NanoSpotUnitSt.Position.z_ := 0;

        NanoSpotUnitSt.Turn.Z := 32768;
        NanoSpotUnitSt.Position.y_ := 0;
        Unit_FixPosY_Sea(@NanoSpotUnitSt);
        NanoSpotUnitSt.Position.Y := Position.Y;

        if (PTAdynmemStruct(TAData.MainStructPtr)^.cBuildSpotState and $40 = $40) then
          NanoSpotUnitSt.nKills := 1
        else
          NanoSpotUnitSt.nKills := 2;

        if Unit_CreateModelAndCob(@NanoSpotUnitSt) <> nil then
        begin
          PrepareNanoUnit(@NanoSpotUnitSt);
          DrawUnit(pOffscreen, @NanoSpotUnitSt);
        end;
      end;
    end;
  end;
end;

procedure DrawBuildSpot_NanoframeHook;
asm
  lea     ecx, [esp+224h+OFFSCREEN_off]
  push    ecx
  call    DrawBuildSpotNanoframe
  mov     ecx, [TADynMemStructPtr]
  push $00469F29;
  call PatchNJump;
end;

procedure DrawBuildSpotQueueNanoframe(pOffscreen: Cardinal; UnitInfo: PUnitInfo; UnitOrder: PUnitOrder); stdcall;
var
  X, Z : Integer;
  Position : TPosition;
begin
  // draw only queued (and not under construction already)
  if UnitOrder.cState <= 1 then
  begin
    if NanoSpotQueueUnitSt.nUnitInfoID <> 0 then
      FreeUnitMem(@NanoSpotQueueUnitSt);

    Position := UnitOrder.Pos;
    NanoSpotQueueUnitSt.nUnitInfoID := UnitInfo.nCategory;
    NanoSpotQueueUnitSt.p_UnitDef := TAMem.UnitInfoId2Ptr(NanoSpotQueueUnitSt.nUnitInfoID);
    X := Position.X;
    Z := Position.Z;
    GetTPosition(X, Z, Position);

    NanoSpotQueueUnitSt.p_Owner := TAPlayer.GetPlayerByIndex(TAData.ViewPlayer);
    if Unit_CreateUnitsInGame(@NanoSpotQueueUnitSt, X, Position.Y, Z, 0) then
    begin
      NanoSpotQueueUnitInfoSt := PUnitInfo(TAMem.UnitInfoId2Ptr(NanoSpotQueueUnitSt.nUnitInfoID))^;
      NanoSpotQueueUnitSt.p_UnitDef := @NanoSpotQueueUnitInfoSt;

      NanoSpotQueueUnitSt.Position.X := NanoSpotQueueUnitSt.Position.x_;
      NanoSpotQueueUnitSt.Position.x_ := 0;
      NanoSpotQueueUnitSt.Position.Z := NanoSpotQueueUnitSt.Position.z_;
      NanoSpotQueueUnitSt.Position.z_ := 0;

      NanoSpotQueueUnitSt.Turn.Z := 32768;
      NanoSpotQueueUnitSt.Position.Y := Position.Y;

      if ((PUnitStruct(UnitOrder.p_Unit).lUnitStateMask shr 4) and 1 = 1) then
        NanoSpotQueueUnitSt.nKills := 1
      else
        NanoSpotQueueUnitSt.nKills := 2;

      if Unit_CreateModelAndCob(@NanoSpotQueueUnitSt) <> nil then
      begin
        PrepareNanoUnit(@NanoSpotQueueUnitSt);
        DrawUnit(pOffscreen, @NanoSpotQueueUnitSt);
      end;
    end;
  end;
end;

{
.text:00438C38   040 03 C1                        add     eax, ecx
.text:00438C3A   040 8D 4A 22                     lea     ecx, [edx+UnitOrder.Pos]
}
procedure DrawBuildSpot_QueueNanoframeHook;
asm
  add     eax, ecx
  pushAD
  mov     ebx, [esp+60h+$4]
  push    edx // unitorder
  push    eax // unitinfo
  push    ebx // offscreen
  call    DrawBuildSpotQueueNanoframe
  popAD
  lea     ecx, [edx+TUnitOrder.Pos]
  push $00438C3D;
  call PatchNJump;
end;

procedure DrawBuildSpot_NanoframeShimmerHook;
label
  DrawNonGlitter,
  DrawMoreTransp,
  ComeBack;
asm
  mov     cx, [edx+0A8h]
  movzx   ebx, word ptr [edx+TUnitStruct.nKills]
  test    bx, bx
  jnz     DrawNonGlitter
  // edx unitstruct
  mov     edx, [TAdynmemStructPtr]
  jnz     DrawNonGlitter
  mov     ebx, ecx
  xor     ecx, 9
  mov     esi, [edx+TTAdynmemStruct.lGameTime]
  xor     ebx, 5
  mov     edx, esi
  shl     edx, 5
  add     edx, esi
  mul     edx
  lea     eax, [esi+esi*8]
  shr     edx, 4
  lea     eax, [esi+eax*2]
  // ebx par1
  add     ebx, edx
  lea     edx, [eax+eax*2]
  mov     eax, 88888889h
  mul     edx
  shr     edx, 4
  // ecx par2
  add     ecx, edx
  jmp     ComeBack
DrawNonGlitter:
  cmp     ebx, 2
  jge     DrawMoreTransp
  mov     ebx, 66
  mov     ecx, 66
  jmp     ComeBack
DrawMoreTransp:
  mov     ebx, 165
  mov     ecx, 165
ComeBack:
  push $00458E56;
  call PatchNJump;
end;

procedure DrawSelectedUnitsCounter(OFFSCREEN_ptr: Cardinal); stdcall;
var
  UnitId, MaxUnitId : Cardinal;
  SelectedCounter : Cardinal;
  SelectedCountStr : String;
  v65 : LongInt;
  ColorsPal : Pointer;
begin
  SelectedCounter := 0;
  MaxUnitId := TAMem.GetMaxUnitId;
  for UnitId := 1 to MaxUnitId do
  begin
    if PUnitStruct(TAUnit.Id2Ptr(UnitId)).lUnitStateMask and $50 = $50 then
      Inc(SelectedCounter);
  end;

  if SelectedCounter > 1 then
  begin
    SelectedCountStr := 'Selected ' + IntToStr(SelectedCounter) + ' units';
    ColorsPal := Pointer(LongWord(TAData.MainStructPtr)+$DCB);
    v65 := sub_4C13F0;
    SetFontColor(PByte(LongWord(ColorsPal)+15)^, v65);
    SetFONTLENGTH_ptr(PTAdynmemStruct(TAData.MainStructPtr)^.lengthOFsmlfontFnt);
    DrawText_Heavy(OFFSCREEN_ptr,
                   PAnsiChar(SelectedCountStr),
                   PTAdynmemStruct(TAData.MainStructPtr)^.lDisplayModeWidth - 105,
                   PTAdynmemStruct(TAData.MainStructPtr)^.lDisplayModeHeight - 20, -1);
  end;
end;

procedure SelectedUnitsCounter;
asm
  pushf
  push    edx
  push    ecx
  push    eax
  lea     eax, [esp+24Ch+$4A]
  push    eax
  call    DrawSelectedUnitsCounter
  pop     eax
  pop     ecx
  pop     edx
  popf
  mov     ecx, [esp+24Ch-$214]
  push $0046ABE6;
  call PatchNJump;
end;

end.
