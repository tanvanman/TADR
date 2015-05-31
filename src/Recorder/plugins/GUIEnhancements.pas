unit GUIEnhancements;

interface
uses
  PluginEngine, TA_MemoryStructures, Classes, SysUtils;

// -----------------------------------------------------------------------------

const
  State_GUIEnhancements : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

var
  ForceBottomStateRefresh: Integer;
  FormatSettings: TFormatSettings;

implementation
uses
  Windows,
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_MemPlayers,
  TA_MemUnits,
  ExtensionsMem,
  UnitInfoExpand,
  Math,
  TA_FunctionsU,
  logging,
  Colors;

const
  STANDARDUNIT : cardinal = 250;
  MEDIUMUNIT : cardinal = 1900;
  BIGUNIT : cardinal = 3000;
  HUGEUNIT : cardinal = 5000;
  EXTRALARGEUNIT : cardinal = 10000;
  VETERANLEVEL_RELOADBOOST = 12; // 30 * 0.2
  SCOREBOARD_WIDTH = 180;

const
  Core32Lt : AnsiString = 'Core32Dk';
  Arm32Lt : AnsiString = 'Arm32Dk';
  RaceLogo : AnsiString = 'RaceLogo';

procedure DrawUnitState(p_Offscreen: Pointer;
  Unit_p: PUnitStruct; CenterPosX: Integer; CenterPosZ: Integer); stdcall;
var
  { drawing }
  ColorsPal: Pointer;
  FontBackgroundColor: Integer;
  RectDrawPos: tagRect;

  LocalUnit: Boolean;

  { health bar }
  HPBackgRectWidth, HPFillRectWidth: Smallint;
  UnitHealth, HealthState : Word;
  UnitMaxHP : Cardinal;

  UnitId: Word;
  UnitInfo : PUnitInfo;
  UnitBuildTimeLeft : Single;
  //UnitPos : TPosition;
  
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
  FeatureDefID : Word;
  ActionUnitType : Word;
  ActionUnit : PUnitStruct;
  ActionUnitBuildTime : Cardinal;
  ActionUnitBuildCost1, ActionUnitBuildCost2 : Single;
  ActionTime : Double;
  ActionWorkTime : Word;
  MaxActionVal : Integer;
  CurActionVal : Integer;
  ReclaimColor : Byte;
begin
  BottomZ := 0;
  try
  // is drawing health bars enabled and any of local player units are actually on screen
  if ((TAData.MainStruct.GameOptionMask and 1) = 1) or
     (Unit_p.HotKeyGroup <> 0) then
  begin
    UnitInfo := Unit_p.p_UnitInfo;
    UnitId := TAUnit.GetId(Unit_p);
    ColorsPal := TAData.ColorsPalette;

    if ((TAData.MainStruct.GameOptionMask and 1) = 1) and
       (CenterPosX <> 0) and
       (CenterPosZ <> 0) and
       (UnitInfo <> nil) then
    begin
      LocalUnit := (PPlayerStruct(Unit_p.p_Owner).cPlayerIndex = TAData.LocalPlayerID);
//      DrawTransparentBox(p_Offscreen, @Rect, -24);
      if LocalUnit then
      begin
        UnitHealth := Unit_p.nHealth;
        UnitBuildTimeLeft := Unit_p.fBuildTimeLeft;

        if (UnitHealth > 0) then
        begin
          UnitMaxHP := UnitInfo.lMaxDamage;
          HPBackgRectWidth := 34;
          if IniSettings.HealthBarDynamicSize then
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
            if (IniSettings.HealthBarWidth <> 0) then
              HPBackgRectWidth := IniSettings.HealthBarWidth
          end;

          if (UnitHealth <= UnitMaxHP) and
             not UnitInfoCustomFields[UnitInfo.nCategory].HideHPBar then
          begin
            HPFillRectWidth := (HPBackgRectWidth div 2);

            RectDrawPos.Left := Word(CenterPosX) - HPFillRectWidth;
            RectDrawPos.Right := Word(CenterPosX) + HPFillRectWidth;
            RectDrawPos.Top := Word(CenterPosZ) - 2;
            RectDrawPos.Bottom := Word(CenterPosZ) + 2;

            DrawBar(p_Offscreen, @RectDrawPos, PByte(ColorsPal)^);
            Inc(RectDrawPos.Top);
            Dec(RectDrawPos.Bottom);
            Inc(RectDrawPos.Left);

            RectDrawPos.Right := Round(RectDrawPos.Left + ((HPBackgRectWidth-2) * UnitHealth) / UnitMaxHP);
            HealthState := UnitMaxHP div 3;


            if UnitHealth <= (HealthState * 2) then
            begin
              if UnitHealth <= HealthState then
                DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+GetRaceSpecificColor(25))^)
              else
                DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+GetRaceSpecificColor(24))^);
            end else
              DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+GetRaceSpecificColor(23))^);
          end;

        // weapons reload
        if ((IniSettings.MinWeaponReload <> 0) or
           (UnitInfoCustomFields[UnitInfo.nCategory].UseCustomReloadBar)) and
           (UnitBuildTimeLeft = 0.0) then
        begin
          MaxReloadTime := 0;
          CurReloadTime := 0;
          StockPile := 0;
          CustomReloadBar := False;
          if UnitInfoCustomFields[UnitInfo.nCategory].UseCustomReloadBar then
          begin
            if UnitsCustomFields[UnitId].CustomWeapReloadMax > 0 then
            begin
              MaxReloadTime := UnitsCustomFields[UnitId].CustomWeapReloadMax;
              CurReloadTime := UnitsCustomFields[UnitId].CustomWeapReloadCur;
              CustomReloadBar := True;
            end;
          end else
          begin
            if (Unit_p.UnitWeapons[0].p_Weapon <> nil) or
               (Unit_p.UnitWeapons[1].p_Weapon <> nil) or
               (Unit_p.UnitWeapons[2].p_Weapon <> nil) then
            begin
              if (Unit_p.UnitWeapons[0].p_Weapon <> nil) then
              begin
                if (PWeaponDef(Unit_p.UnitWeapons[0].p_Weapon).lWeaponTypeMask and (1 shl 28) = 1 shl 28) then
                  StockPile := 1;
                if PWeaponDef(Unit_p.UnitWeapons[0].p_Weapon).nReloadTime >= IniSettings.MinWeaponReload * 30 then
                begin
                  MaxReloadTime := PWeaponDef(Unit_p.UnitWeapons[0].p_Weapon).nReloadTime;
                  CurReloadTime := MaxReloadTime - Unit_p.UnitWeapons[0].nReloadTime;
                end;
              end;
              if (Unit_p.UnitWeapons[1].p_Weapon <> nil) then
              begin
                if (PWeaponDef(Unit_p.UnitWeapons[1].p_Weapon).lWeaponTypeMask and (1 shl 28) = 1 shl 28) then
                  StockPile := 2;
                if PWeaponDef(Unit_p.UnitWeapons[1].p_Weapon).nReloadTime >= IniSettings.MinWeaponReload * 30 then
                begin
                  MaxReloadTime := PWeaponDef(Unit_p.UnitWeapons[1].p_Weapon).nReloadTime;
                  CurReloadTime := MaxReloadTime - Unit_p.UnitWeapons[1].nReloadTime;
                end;
              end;
              if (Unit_p.UnitWeapons[2].p_Weapon <> nil) then
              begin
                if (PWeaponDef(Unit_p.UnitWeapons[2].p_Weapon).lWeaponTypeMask and (1 shl 28) = 1 shl 28) then
                  StockPile := 3;
                if PWeaponDef(Unit_p.UnitWeapons[2].p_Weapon).nReloadTime >= IniSettings.MinWeaponReload * 30 then
                begin
                  MaxReloadTime := PWeaponDef(Unit_p.UnitWeapons[2].p_Weapon).nReloadTime;
                  CurReloadTime := MaxReloadTime - Unit_p.UnitWeapons[2].nReloadTime;
                end;
              end;
            end;
          end;

          if UnitsCustomFields[UnitId].TeleportReloadMax > 0 then
          begin
            MaxReloadTime := UnitsCustomFields[UnitId].TeleportReloadMax;
            CurReloadTime := UnitsCustomFields[UnitId].TeleportReloadCur;
            CustomReloadBar := True;
          end;

          if MaxReloadTime <> 0 then
          begin
            BottomZ := 6;
            if (Unit_p.nKills >= 5) and
               not CustomReloadBar then
            begin
              CurReloadTime := CurReloadTime - VETERANLEVEL_RELOADBOOST;
              MaxReloadTime := MaxReloadTime - VETERANLEVEL_RELOADBOOST;
            end;

            if CurReloadTime < 0 then
              CurReloadTime := 0;

            // stockpile weapon build progress instead of reload bar
            if (StockPile <> 0) and
               not CustomReloadBar then
            begin
              if (Unit_p.p_SubOrder <> nil) and
                 (IniSettings.Stockpile) then
              begin
                MaxReloadTime := 100;
                CurReloadTime := GetUnit_BuildWeaponProgress(Unit_p);
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
                DrawRectangle(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+4)^)
              else   }
              DrawBar(p_Offscreen, @RectDrawPos, PByte(ColorsPal)^);

              Inc(RectDrawPos.Top);
              Dec(RectDrawPos.Bottom);
              Inc(RectDrawPos.Left);
              RectDrawPos.Right := Round(RectDrawPos.Left + (32 * CurReloadTime) / MaxReloadTime);

              if StockPile <> 0 then
              begin
                DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+GetRaceSpecificColor(28))^)
              end else
              begin
                BarProgress := Round((CurReloadTime / MaxReloadTime) * 100);
                WeapReloadColor := GetRaceSpecificColor(26);
                case BarProgress of
                   0..15 : DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor)^);
                  16..30 : DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 1)^);
                  31..47 : DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 2)^);
                  48..64 : DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 3)^);
                  65..80 : DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 4)^);
                  81..94 : DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 5)^);
                 95..100 : DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 6)^);
                end;
              end;
            end;
          end;
        end; { IniSettings.WeaponReloadTimeBar }

        end; {UnitHealth > 0}

        if IniSettings.MinReclaimTime > 0 then
        begin
          CurOrder := TAUnit.GetCurrentOrderType(Unit_p);
          if (CurOrder = Action_Reclaim) or
             (CurOrder = Action_VTOL_Reclaim) or
             (CurOrder = Action_Capture) or
             (CurOrder = Action_Resurrect) then
          begin
            if (CurOrder = Action_Reclaim) or
               (CurOrder = Action_Resurrect) or
               (CurOrder = Action_Capture) then
              OrderStateCorrect := TAUnit.GetCurrentOrderState(Unit_p) and $400000 = $400000
            else
              OrderStateCorrect := TAUnit.GetCurrentOrderState(Unit_p) and $100000 = $100000;
            if OrderStateCorrect then
            begin
              if CurOrder = Action_Capture then
                FeatureDefID := 0
              else
                FeatureDefID := GetFeatureTypeFromOrder(@Unit_p.p_MainOrder.Position, Unit_p.p_MainOrder, nil);
              if FeatureDefID <> Word(-1) then
              begin
                MaxActionVal := 0;
                CurActionVal := 0;
                case CurOrder of
                  Action_Reclaim :
                  begin
                    FeatureDefPtr := TAMem.FeatureDefId2Ptr(FeatureDefID);
                    MaxActionVal := Trunc( (PFeatureDefStruct(FeatureDefPtr).fMetal + PFeatureDefStruct(FeatureDefPtr).fEnergy) / 2 + 15);
                    CurActionVal := TAUnit.GetCurrentOrderParams(Unit_p, 1);
                  end;
                  Action_VTOL_Reclaim :
                  begin
                    FeatureDefPtr := TAMem.FeatureDefId2Ptr(FeatureDefID);
                    MaxActionVal := Trunc( (PFeatureDefStruct(FeatureDefPtr).fMetal + PFeatureDefStruct(FeatureDefPtr).fEnergy) / 2 + 30);
                    CurActionVal := TAUnit.GetCurrentOrderParams(Unit_p, 1);
                  end;
                  Action_Capture:
                  begin
                    ActionUnit := TAUnit.GetCurrentOrderTargetUnit(Unit_p);
                    if ActionUnit <> nil then
                    begin
                      ActionUnitType := TAUnit.GetUnitInfoId(ActionUnit);
                      if ActionUnitType <> 0 then
                      begin
                        ActionUnitBuildCost1 := TAMem.UnitInfoId2Ptr(ActionUnitType).lBuildCostEnergy * 30 * 0.00050000002;
                        ActionUnitBuildCost2 := TAMem.UnitInfoId2Ptr(ActionUnitType).lBuildCostMetal * 30 * -0.0071428572;
                        ActionTime := (ActionUnitBuildCost1 - ActionUnitBuildCost2) + 150;

                        MaxActionVal := Round(ActionTime);
                        if MaxActionVal - TAUnit.GetCurrentOrderParams(Unit_p, 1) > 0 then
                        begin
                          CurActionVal := TAUnit.GetCurrentOrderParams(Unit_p, 1);
                          CurActionVal := MaxActionVal - CurActionVal;
                        end;
                      end;
                    end;
                  end;
                  Action_Resurrect :
                  begin
                    FeatureDefPtr := TAMem.FeatureDefId2Ptr(FeatureDefID);
                    ActionUnitType := TAUnit.GetCurrentOrderParams(Unit_p, 1);
                    if ActionUnitType <> 0 then
                    begin
                      ActionUnitBuildTime := TAMem.UnitInfoId2Ptr(ActionUnitType).lBuildTime;
                      ActionWorkTime := Unit_p.p_UnitInfo.nWorkerTime div 30;
                      ActionTime := (ActionUnitBuildTime * 0.3) / ActionWorkTime;
                      MaxActionVal := Trunc(ActionTime);
                      CurActionVal := TAUnit.GetCurrentOrderParams(Unit_p, 2);
                    end;
                  end;
                end;

                if (MaxActionVal > 0) and
                   (CurActionVal > 0) and
                   (CurActionVal <= MaxActionVal) and  // TA changes par1 to it once feature is reclaimed but order state remains "reclaiming"...
                   (MaxActionVal >= IniSettings.MinReclaimTime * 30) then
                begin
                  RectDrawPos.Left := Word(CenterPosX) - 17;
                  RectDrawPos.Right := Word(CenterPosX) + 17;
                  RectDrawPos.Top := Word(CenterPosZ) - 7;
                  RectDrawPos.Bottom := Word(CenterPosZ) - 3;

                  DrawBar(p_Offscreen, @RectDrawPos, PByte(ColorsPal)^);

                  Inc(RectDrawPos.Top);
                  Dec(RectDrawPos.Bottom);
                  Inc(RectDrawPos.Left);
                  RectDrawPos.Right := Round(RectDrawPos.Left + (32 * CurActionVal) / MaxActionVal);

                  BarProgress := Round((CurActionVal / MaxActionVal) * 100);
                  ReclaimColor := GetRaceSpecificColor(27);
                  case BarProgress of
                    0..20 : DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor)^);
                   21..40 : DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor + 1)^);
                   41..60 : DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor + 2)^);
                   61..80 : DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor + 3)^);
                  81..100 : DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor + 4)^);
                  end;
                end;
              end;
            end;
          end;
        end;

        // built weapons counter (nukes)
        if IniSettings.Stockpile then
          if Unit_p.UnitWeapons[0].cStock > 0 then
            DrawTextCustomFont(p_Offscreen, PAnsiChar(IntToStr(Unit_p.UnitWeapons[0].cStock)), Word(CenterPosX), Word(CenterPosZ) - 13, -1);

        // transporter count
        if IniSettings.Transporters then
        begin
          if (UnitInfo.UnitTypeMask and 2048 = 2048) then   // unit is air
          begin
            if (UnitInfoCustomFields[UnitInfo.nCategory].MultiAirTransport = 0) then
              TransportCap := 0
            else
              if (UnitInfoCustomFields[UnitInfo.nCategory].TransportWeightCapacity = 0) then
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
            TransportCount := TAUnit.GetLoadCurAmount(Unit_p);
            if TransportCount > 0 then
              DrawTextCustomFont(p_Offscreen, PAnsiChar(IntToStr(TransportCount) + '/' + IntToStr(TransportCap)), Word(CenterPosX) - 10, Word(CenterPosZ) - 70, -1);
          end else
          begin
            WeightTransportMax := UnitInfoCustomFields[UnitInfo.nCategory].TransportWeightCapacity;
            if WeightTransportMax > 0 then
            begin
              WeightTransportCur := TAUnit.GetLoadWeight(Unit_p);
              TransportCap := UnitInfo.cTransportCap;
              TransportCount := TAUnit.GetLoadCurAmount(Unit_p);

              if (WeightTransportCur > 0) then
              begin
                WeightTransportPercent := Round((WeightTransportCur / WeightTransportMax) * 100);
                FontBackgroundColor := GetFontBackgroundColor;
                if WeightTransportPercent > 100 then
                  WeightTransportPercent := 100;
                if TransportCount = TransportCap then
                  WeightTransportPercent := 100;
                if (WeightTransportPercent > 85) then
                  SetFontColor(PByte(LongWord(ColorsPal)+18)^, FontBackgroundColor)
                else
                  if (WeightTransportPercent > 50) then
                    SetFontColor(PByte(LongWord(ColorsPal)+17)^, FontBackgroundColor)
                  else
                    SetFontColor(PByte(LongWord(ColorsPal)+16)^, FontBackgroundColor);

                DrawTextCustomFont(p_Offscreen,
                               PAnsiChar(IntToStr(WeightTransportPercent) + '%'),
                               Word(CenterPosX) - 10, Word(CenterPosZ) - 70, -1);
                SetFontColor(PByte(LongWord(ColorsPal)+255)^, FontBackgroundColor);
              end;
            end;
          end;

        end;
      end;

      if (PPlayerStruct(Unit_p.p_Owner).cPlayerIndex = TAData.LocalPlayerID) and
         (Unit_p.HotKeyGroup <> 0) then
      begin
        AllowHotkeyDraw := False;
        if Unit_p.p_TransporterUnit <> nil then
        begin
          if Unit_p.p_TransporterUnit.HotKeyGroup = 0 then
            AllowHotkeyDraw := True;
        end else
          AllowHotkeyDraw := True;
          
        if AllowHotkeyDraw then
        begin
          sGroup := PAnsiChar(Unit_p.HotKeyGroup + 48);
          DrawTextCustomFont(p_Offscreen, @sGroup, Word(CenterPosX), Word(CenterPosZ) + 3 + BottomZ, -1);
        end;
      end;
    end; { Bars enabled }
  end;
  except
    on e: exception do
  end;
end;

procedure DrawTrueIncome(p_Offscreen: Pointer;
  RaceSideData: PRaceSideData; ViewResBar: PViewResBar); stdcall;
var
  ResourceIncome : Single;
  PlayerResourceString : PAnsiChar;
  ColorsPal : Pointer;
  FontBackgroundColor: Integer;
  SideIndex: Integer;
begin
  SideIndex := RaceSideData.lSideIdx;
  if SideIndex <= High(ExtraSideData) then
  begin
    ColorsPal := TAData.ColorsPalette;
    FontBackgroundColor := GetFontBackgroundColor;

    if ExtraSideData[SideIndex].rectRealEIncome.Left <> 0 then
    begin
      ResourceIncome := ViewResBar.fEnergyProduction - ViewResBar.fEnergyExpense;
      if ResourceIncome >= 0.0 then
      begin
        SetFontColor(PByte(LongWord(ColorsPal)+10)^, FontBackgroundColor);
        if ResourceIncome < 10000 then
          PlayerResourceString := PAnsiChar(Format('+%.0f', [ResourceIncome], FormatSettings))
        else
        begin
          ResourceIncome := ResourceIncome / 1000;
          PlayerResourceString := PAnsiChar(Format('+%.0fK', [ResourceIncome], FormatSettings));
        end;
      end else
      begin
        SetFontColor(PByte(LongWord(ColorsPal)+12)^, FontBackgroundColor);
        if ResourceIncome > -10000 then
          PlayerResourceString := PAnsiChar(Format('%.0f', [ResourceIncome], FormatSettings))
        else
        begin
          ResourceIncome := ResourceIncome / 1000;
          PlayerResourceString := PAnsiChar(Format('%.0fK', [ResourceIncome], FormatSettings));
        end;
      end;
      DrawTextCustomFont(p_Offscreen, PlayerResourceString,
        ExtraSideData[SideIndex].rectRealEIncome.Left,
        ExtraSideData[SideIndex].rectRealEIncome.Top, -1);
    end;

    if ExtraSideData[SideIndex].rectRealMIncome.Left <> 0 then
    begin
      ResourceIncome := ViewResBar.fMetalProduction - ViewResBar.fMetalExpense;
      if ResourceIncome >= 0.0 then
      begin
        SetFontColor(PByte(LongWord(ColorsPal)+10)^, FontBackgroundColor);
        if ResourceIncome < 10000 then
          PlayerResourceString := PAnsiChar(Format('+%.1f', [ResourceIncome], FormatSettings))
        else
        begin
          ResourceIncome := ResourceIncome / 1000;
          PlayerResourceString := PAnsiChar(Format('+%.1fK', [ResourceIncome], FormatSettings));
        end;
      end else
      begin
        SetFontColor(PByte(LongWord(ColorsPal)+12)^, FontBackgroundColor);
        if ResourceIncome > -10000 then
          PlayerResourceString := PAnsiChar(Format('%.1f', [ResourceIncome], FormatSettings))
        else
        begin
          ResourceIncome := ResourceIncome / 1000;
          PlayerResourceString := PAnsiChar(Format('%.1fK', [ResourceIncome], FormatSettings));
        end;
      end;
      DrawTextCustomFont(p_Offscreen, PlayerResourceString,
        ExtraSideData[SideIndex].rectRealMIncome.Left,
        ExtraSideData[SideIndex].rectRealMIncome.Top, -1);
    end;
  end;
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
  call    DrawTextCustomFont
  pushAD
  lea     ecx, [esp+224h-$1AC]
  mov     edx, esi
  push    ecx             // ViewResBar
  push    edx             // RaceSideData
  lea     ecx, [esp+22Ch+OFFSCREEN_off]
  push    ecx             // OFFSCREEN_ptr
  call    DrawTrueIncome
  popAD
  push $00469610;
  call PatchNJump;
end;

procedure DrawHealthPercentage(p_Offscreen: Pointer; p_Unit: PUnitStruct;
  SideData: PRaceSideData; CurrentHP, MaxHP: Cardinal; Yoffset: Integer); stdcall;
var
  HealthPercent : Single;
  HealthPercentStr : PAnsiChar;
  FontBackgroundColor: Integer;
  FormatSettings: TFormatSettings;
  GafFrame: Pointer;
begin
  if (MaxHP = 0) or (CurrentHP > MaxHP) then
    Exit;

  if SideData.lSideIdx <= High(ExtraSideData) then
  begin
    if UnitsCustomFields[TAUnit.GetId(p_Unit)].ShieldedBy <> nil then
    begin
      if ExtraSideData[SideData.lSideIdx].rectShieldIcon.Left <> 0 then
      begin
        if ExtraGAFAnimations.GafSequence_ShieldIcon <> nil then
        begin
          GafFrame := GAF_SequenceIndex2Frame(ExtraGAFAnimations.GafSequence_ShieldIcon, SideData.lSideIdx);
          CopyGafToContext(p_Offscreen, GafFrame,
            ExtraSideData[SideData.lSideIdx].rectShieldIcon.Left,
            ExtraSideData[SideData.lSideIdx].rectShieldIcon.Top + Yoffset);
        end;
      end;
    end;

    if ExtraSideData[SideData.lSideIdx].rectDamageVal.Left <> 0 then
    begin
      HealthPercent := (CurrentHP / MaxHP) * 100;
      if HealthPercent > 0.0 then
      begin
        FontBackgroundColor := GetFontBackgroundColor;
        SetFontColor(83, FontBackgroundColor);
        FormatSettings.DecimalSeparator := '.';
        HealthPercentStr := PAnsiChar(Format('%.1f%%', [HealthPercent], FormatSettings));
        DrawTextCustomFont(p_Offscreen, HealthPercentStr,
          ExtraSideData[SideData.lSideIdx].rectDamageVal.Left,
          ExtraSideData[SideData.lSideIdx].rectDamageVal.Top + Yoffset, -1);
      end;
    end;
  end;
end;

procedure HealthPercentage;
asm
  add     eax, ebx
  mov     [esp+18h], ecx
  pushAD
  push    ebx
  mov     ebx, [esp+34h]
  push    ebx      // max
  push    edx      // current
  push    ebp      // sidedata
  mov     ebx, [esp+54h]
  push    ebx
  push    edi      // offscreen
  call    DrawHealthPercentage
  popAD
  push $0046B08E;
  call PatchNJump;
end;

function DrawUnitRangesShowrangesOn(p_Offscreen: Pointer; CirclePointer: Cardinal; UnitInfo: PUnitInfo;
  UnitOrder: PUnitOrder; ReturnVal: Integer): Integer; stdcall;
begin
  // as a result give amount of circles that were drawn
  if ( UnitInfoCustomFields[UnitInfo.nCategory].TeleportMinDistance <> 0 ) then
  begin
    Inc(ReturnVal);
    DrawRangeCircle(
        p_Offscreen,
        CirclePointer,
        @UnitOrder.p_Unit.Position,
        UnitInfoCustomFields[UnitInfo.nCategory].TeleportMinDistance,
        138,
        PAnsiChar('minteleport'),
        ReturnVal);
  end;
  if ( UnitInfoCustomFields[UnitInfo.nCategory].TeleportMaxDistance <> 0 ) then
  begin
    Inc(ReturnVal);
    DrawRangeCircle(
        p_Offscreen,
        CirclePointer,
        @UnitOrder.p_Unit.Position,
        UnitInfoCustomFields[UnitInfo.nCategory].TeleportMaxDistance,
        140,
        PAnsiChar('maxteleport'),
        ReturnVal);
  end;
  Result := ReturnVal;
end;

procedure DrawUnitRanges_ShowrangesOnHook;
asm
  push    edi
  push    edx
  push    ecx
  push    ebx
  push    eax

  push    esi
  push    ebx
  push    eax // unitinfo
  push    edi // circle point
  push    ebp
  call    DrawUnitRangesShowrangesOn
  mov     esi, eax

  pop     eax
  pop     ebx
  pop     ecx
  pop     edx
  pop     edi

  mov     dx, [eax+TUnitInfo.nSightDistance]
  push $00439251;
  call PatchNJump;
end;

procedure DrawUnitRangesShowrangesOff(p_Offscreen: Pointer;
  CirclePointer: Cardinal; UnitOrder: PUnitOrder); stdcall;
var
  CustomRange: Integer;
  Radius: Integer;
  GameTime: Integer;
  UnitInfo: PUnitInfo;
begin
  UnitInfo := UnitOrder.p_Unit.p_UnitInfo;
  if UnitInfo = nil then
    Exit;
    
  if ( UnitInfoCustomFields[UnitInfo.nCategory].CustomRange1Distance <> 0 ) then
  begin
    DrawRangeCircle(
        p_Offscreen,
        CirclePointer,
        @UnitOrder.p_Unit.Position,
        UnitInfoCustomFields[UnitInfo.nCategory].CustomRange1Distance,
        UnitInfoCustomFields[UnitInfo.nCategory].CustomRange1Color,
        nil,
        0); 
  end;
  if ( UnitInfoCustomFields[UnitInfo.nCategory].CustomRange2Distance <> 0 ) then
  begin
    if UnitInfoCustomFields[UnitInfo.nCategory].CustomRange2Animate then
    begin
      CustomRange := UnitInfoCustomFields[UnitInfo.nCategory].CustomRange2Distance;
      Radius := 8;
      GameTime := TAData.GameTime mod 60;
      if ((2 * CustomRange * GameTime div 60) >= 8 ) then
        Radius := 2 * CustomRange * GameTime div 60;
      if ( Radius >= CustomRange ) then
        Radius := UnitInfoCustomFields[UnitInfo.nCategory].CustomRange2Distance;
    end else
      Radius := UnitInfoCustomFields[UnitInfo.nCategory].CustomRange2Distance;

    DrawRangeCircle( p_Offscreen,
                     CirclePointer,
                     @PUnitStruct(UnitOrder.p_Unit).Position,
                     Radius,
                     UnitInfoCustomFields[UnitInfo.nCategory].CustomRange2Color,
                     nil, 0 );
  end;
end;

procedure DrawUnitRanges_CustomRanges;
label
  loc_439CC8;
asm
  push    edx
  mov     edx, [esp+30h]
  push    ecx
  push    ebx
  push    eax
  mov     eax, [esp+38h]

  push    esi // unit order
  push    edx // circle point
  push    eax
  call    DrawUnitRangesShowrangesOff

  pop     eax
  pop     ebx
  pop     ecx
  pop     edx

  and     eax, edx;
  test    al, 10h;
  jz      loc_439CC8
  push $00439CA1;
  call PatchNJump;
loc_439CC8 :
  push $00439CC8;
  call PatchNJump;
end;

function DrawExplosionsRectExpand(Rect: PtagRECT; x, y: Integer): Boolean; stdcall;
begin
  Result := (x >= (Rect.Left - IniSettings.ExplosionsGameUIExpand)) and
            (x <= (Rect.Right + IniSettings.ExplosionsGameUIExpand)) and
            (y >= (Rect.Top - IniSettings.ExplosionsGameUIExpand)) and
            (y <= (Rect.Bottom + IniSettings.ExplosionsGameUIExpand));
end;

procedure DrawExplosionsRectExpandHook;
asm
  push    edi                         // y
  push    ebx                         // x
  push    eax                         // Rect
  call    DrawExplosionsRectExpand
  push $00420C4F;
  call PatchNJump;
end;

procedure DrawExplosionsRectExpandHook2;
asm
  push    edi                         // y
  push    esi                         // x
  push    eax                         // Rect
  call    DrawExplosionsRectExpand
  push $00420BAA;
  call PatchNJump;
end;

procedure LoadArmCore32ltGafSequences;
asm
  push    eax

  mov     eax, [TADynMemStructPtr]
  mov     ecx, [eax+TTAdynmemStruct.p_LogosGaf]
  push    Arm32Lt
  push    ecx
  call    GAF_Name2Sequence
  mov     ExtraGAFAnimations.GafSequence_Arm32lt, eax

  mov     eax, [TADynMemStructPtr]
  mov     ecx, [eax+TTAdynmemStruct.p_LogosGaf]
  push    Core32Lt
  push    ecx
  call    GAF_Name2Sequence
  mov     ExtraGAFAnimations.GafSequence_Core32lt, eax

  pop     eax
  mov     edx, [TADynMemStructPtr]
  push $00431911;
  call PatchNJump;
end;

function GetStorageText(AValue: Single): String;
begin
  Result := '';
  if AValue < 10000 then
  begin
    Result := Format('%.0f', [AValue], FormatSettings);
  end else
  begin
    if AValue < 100000 then
    begin
      AValue := AValue / 1000;
      Result := Format('%.1fK', [AValue], FormatSettings);
    end;
  end;
  if Result = '' then
  begin
    AValue := AValue / 1000;
    Result := Format('%.0fK', [AValue], FormatSettings);
  end;
end;

procedure DrawShadowedText(p_Offscreen: Pointer;
  Str: String; left: Integer; top: Integer; MaxWidth: Integer; Color: Byte; AlignRight: Boolean);
begin
  if AlignRight then
    Left := Left - GetCustomFontStrExtent(GetFontType, PAnsiChar(Str));
  SetFontColor(0, GetFontBackgroundColor);
  DrawTextCustomFont(p_Offscreen, PAnsiChar(Str), Left + 2, Top + 2, MaxWidth);
  SetFontColor(Color, GetFontBackgroundColor);
  DrawTextCustomFont(p_Offscreen, PAnsiChar(Str), Left, Top, MaxWidth);
end;

procedure DrawScoreboard(p_Offscreen: Pointer); stdcall;
var
  i: Integer;
  v2, v3, c: Byte;
  v4, v5, v7, v8, v9: Integer;
  ScoreBoardPos, LocalPlayerBox: tagRect;
//  StrExt: Integer;
  PlayersDrawListTop: Integer;
  cCurActivePlayer, IteratePlayerIdx : Byte;
  cCurActiveSortPlayer: Byte;
  cIterateSort : Byte;
  PlayerPtr, PlayerSort: PPlayerStruct;
  PlayerType, PlayerSortType: TTAPlayerController;
  PlayerSide: TTAPlayerSide;
  TextLeftOff : Integer;
  p_ColorLogo : PGAFFrame;
  Counter : Integer;
  PlayerLogoRect, PlayerLogoTransform : TGAFFrameTransform;
  ScoreBoardWidth: Integer;
  bDraw: Boolean;
  ucAlliedPlayersCount: Byte;
  p_OldFont: Pointer;
  NextTop: Integer;
  BarTagRect: tagRECT;
label
  SortPlayers;
begin
  if ( PInteger($51F2F4)^ < TAData.GameTime ) then
  begin
    PInteger($51F2F4)^ := TAData.GameTime + 1;
    i := 0;
    repeat
      v2 := PByte($51F2C8 + i)^;
      if (v2 > 0) then
        PByte($51F2C8 + i)^ := v2 - 2;
      v3 := PByte($51E810 + i)^;
      if (v3 > 0) then
        PByte($51E810 + i)^ := v3 - 2;
      Inc(i);
    until ( i = 10 );
  end;

  if TAData.NetworkLayerEnabled then
    ScoreBoardWidth := SCOREBOARD_WIDTH
  else
    ScoreBoardWidth := 200;

  v4 := PInteger(Cardinal(TAData.MainStruct) + $57D)^;
  c := 3;
  if v4 <> -1 then
    c := PByte(PCardinal(PCardinal(Cardinal(TAData.MainStruct) + $531)^+4)^+347*Cardinal(v4))^;
  if (((TAData.MainStruct.GameOptionMask and $80) <> 0) or (GetAsyncKeyState(VK_SPACE) <> 0)) and
     ((v4 = -1) or (c <> 3)) then
  begin
    v8 := PInteger(ScoreBoardRoll)^;
    if ( PInteger(ScoreBoardRoll)^ < ScoreBoardWidth ) then
    begin
      if ( PInteger(ScoreBoardRoll)^ <= 0 ) then
      begin
        PlaySound_2D_Name(PAnsiChar('Panel'), 0);
        v8 := PInteger(ScoreBoardRoll)^;
      end;
      v9 := (ScoreBoardWidth - v8) div 4;
      if ( v9 <= 1 ) then
        v9 := 1;
      PInteger(ScoreBoardRoll)^ := v9 + v8;
      if ( v9 + v8 >= ScoreBoardWidth ) then
      begin
        PInteger(ScoreBoardRoll)^ := ScoreBoardWidth;
        PlaySound_2D_Name(PAnsiChar('Options'), 0);
      end;
    end;
  end else
  begin
    v5 := PInteger(ScoreBoardRoll)^;
    if ( PInteger(ScoreBoardRoll)^ <= 0 ) then
      Exit;
    if ( PInteger(ScoreBoardRoll)^ = ScoreBoardWidth ) then
    begin
      PlaySound_2D_Name(PAnsiChar('Panel'), 0);
      v5 := PInteger(ScoreBoardRoll)^;
    end;
    v7 := v5 div 4;
    if ( v5 div 4 <= 1 ) then
      v7 := 1;
    PInteger(ScoreBoardRoll)^ := v5 - v7;
    if ( v5 - v7 <= 0 ) then
    begin
      PInteger(ScoreBoardRoll)^ := 0;
      PlaySound_2D_Name(PAnsiChar('Options'), 0);
    end;
  end;

  ScoreBoardPos.Left := GetTA_ScreenWidth - PInteger(ScoreBoardRoll)^;
  ScoreBoardPos.Right := ScoreBoardPos.Left + ScoreBoardWidth;
  ScoreBoardPos.Top := 32;
  ucAlliedPlayersCount := 0;
  if TAData.GameingType = gtSkirmish then
  begin
    for i := 0 to 9 do
    begin
      if TAData.LocalPlayerID = i then Continue;
      PlayerPtr := TAPlayer.GetPlayerByIndex(i);
      if TAPlayer.GetAlliedState(PlayerPtr, TAData.LocalPlayerID) then
       Inc(ucAlliedPlayersCount);
    end;
  end;
  ScoreBoardPos.Bottom := 40 * (TAData.MainStruct.nActivePlayersCount - ucAlliedPlayersCount) + 46 +
    (75 * ucAlliedPlayersCount);

  DrawTransparentBox(p_Offscreen, @ScoreBoardPos, -24);
  DrawText(p_Offscreen, TranslateString(PAnsiChar('Kills')),
    ScoreBoardPos.Left + 5, ScoreBoardPos.Top, 119, 0);

  DrawText(p_Offscreen, TranslateString(PAnsiChar('Losses')),
           ScoreBoardPos.Right - GetStrExtent(TranslateString(PAnsiChar('Losses'))) - 2,
           ScoreBoardPos.Top, 119, 0);
  {
  if TAData.NetworkLayerEnabled then
  begin
    StrExt := GetStrExtent(TranslateString(PAnsiChar('Ping')));
    DrawText(p_Offscreen, TranslateString(PAnsiChar('Ping')),
      ScoreBoardPos.Right - StrExt - 2, ScoreBoardPos.Top, 119, 0);
  end;
  }
  PlayersDrawListTop := ScoreBoardPos.Top + 15;
  cCurActivePlayer := 0;
  if TAData.MainStruct.nActivePlayersCount > 0 then
  begin
    repeat
      PlayerPtr := TAPlayer.GetPlayerByIndex(0);
      IteratePlayerIdx := 0;
      bDraw := True;
      while True do
      begin
        if TAPlayer.IsActive(PlayerPtr) then
        begin
          PlayerType := TAPlayer.PlayerController(PlayerPtr);
          if (PlayerType = Player_LocalHuman) or
             (PlayerType = Player_LocalAI) or
             (PlayerType = Player_RemotePlayer) then
          begin
            if (PlayerPtr.cPlayerIndex <> 10) then
              if (PlayerPtr.nNumUnits <> 0) and (PlayerPtr.lUnitsCounter <> 0) then
                if ((PlayerPtr.PlayerInfo^.PropertyMask and $40) = 0) then
                  if (PlayerPtr.cPlayerScoreboard = cCurActivePlayer) then
                    Break;
          end;
        end;
        Inc(IteratePlayerIdx);
        PlayerPtr := TAPlayer.GetPlayerByIndex(IteratePlayerIdx);
        if IteratePlayerIdx = 10 then
        begin
          bDraw := False;
          Break;
        end;
      end;

      if not bDraw then goto SortPlayers;

      LocalPlayerBox.Left := ScoreBoardPos.Left + 4;
      LocalPlayerBox.Right := ScoreBoardPos.Right - 4;
      LocalPlayerBox.Top := PlayersDrawListTop - 1;
      LocalPlayerBox.Bottom := PlayersDrawListTop + 37;
      if ( IteratePlayerIdx = TAData.LocalPlayerID ) then
        DrawTransparentBox(p_OFFSCREEN, @LocalPlayerBox, -24)
      else
        DrawTransparentBox(p_OFFSCREEN, @LocalPlayerBox, -19);

      PlayerSide := TAPlayer.PlayerSide(PlayerPtr);
      case PlayerSide of
        psArm :
          p_ColorLogo := GAF_SequenceIndex2Frame(ExtraGAFAnimations.GafSequence_Arm32lt,
            TAPlayer.PlayerLogoIndex(PlayerPtr));
        psCore :
          p_ColorLogo := GAF_SequenceIndex2Frame(ExtraGAFAnimations.GafSequence_Core32lt,
            TAPlayer.PlayerLogoIndex(PlayerPtr));
        else p_ColorLogo := nil;
      end;

      if p_ColorLogo <> nil then
      begin
        TextLeftOff := ScoreBoardPos.Left + 39 + 3;
        PlayerLogoRect.Rect1.Left := ScoreBoardPos.Left + 7;
        PlayerLogoRect.Rect1.Top := PlayersDrawListTop + 2;
        PlayerLogoRect.Rect1.Right := ScoreBoardPos.Left + 39;
        PlayerLogoRect.Rect1.Bottom := PlayersDrawListTop + 2;

        PlayerLogoRect.Rect2.Left := ScoreBoardPos.Left + 39;
        PlayerLogoRect.Rect2.Top := PlayersDrawListTop + 34;
        PlayerLogoRect.Rect2.Right := ScoreBoardPos.Left + 7;
        PlayerLogoRect.Rect2.Bottom := PlayersDrawListTop + 34;

        PlayerLogoTransform.Rect1.Left := 0;
        PlayerLogoTransform.Rect1.Top := 0;
        PlayerLogoTransform.Rect1.Right := p_ColorLogo.Width - 1;
        PlayerLogoTransform.Rect1.Bottom := 0;

        PlayerLogoTransform.Rect2.Left := p_ColorLogo.Width - 1;
        PlayerLogoTransform.Rect2.Top := p_ColorLogo.Height - 1;
        PlayerLogoTransform.Rect2.Right := 0;
        PlayerLogoTransform.Rect2.Bottom := p_ColorLogo.Height - 1;

        GAF_DrawTransformed(p_Offscreen, p_ColorLogo, @PlayerLogoRect, @PlayerLogoTransform);
      end else
      begin
        TextLeftOff := ScoreBoardPos.Left + 7 + 3;
      end;
      DrawText(p_Offscreen, PlayerPtr.szName,
        TextLeftOff, PlayersDrawListTop + 1, ScoreBoardWidth-6, 0);
      if ( TAData.MainStruct.bAlterKills = 2 ) then
        Counter := PlayerPtr.nKills_Last
      else
        Counter := PlayerPtr.nKills;
      DrawText(p_Offscreen, PAnsiChar(IntToStr(Counter)),
        TextLeftOff, PlayersDrawListTop + 21, 119, PByte($51F2C8 + IteratePlayerIdx)^);

      if ( TAData.MainStruct.bAlterKills = 2 ) then
        Counter := PlayerPtr.nLosses_Last
      else
        Counter := PlayerPtr.nLosses;
      DrawText(p_Offscreen, PAnsiChar(IntToStr(Counter)),
        ScoreBoardPos.Left + ScoreBoardWidth-6 - GetStrExtent(PAnsiChar(IntToStr(Counter))) - 2,
        PlayersDrawListTop + 21, 119, PByte($51E810 + IteratePlayerIdx)^);

      NextTop := 40;
      if TAData.GameingType = gtSkirmish then
      begin
        p_OldFont := GetFontType;
        SetFontType(TAData.MainStruct.p_Font_SMLFONT);
        //SetFontType(Fonts.p_Courier);
        if TAPlayer.GetAlliedState(PlayerPtr, TAData.LocalPlayerID) and
           (TAData.LocalPlayerID <> PlayerPtr.cPlayerIndex) then
        begin
          NextTop := 75;
          DrawShadowedText(p_Offscreen, GetStorageText(PlayerPtr.Resources.fCurrentMetal),
            ScoreBoardPos.Left + 37, PlayersDrawListTop + 42, 30, 145, True);
          DrawShadowedText(p_Offscreen, GetStorageText(PlayerPtr.Resources.fCurrentEnergy),
            ScoreBoardPos.Left + 37, PlayersDrawListTop + 58, 30, 145, True);

          DrawShadowedText(p_Offscreen, Format('+%.1f', [PlayerPtr.Resources.fMetalProduction], FormatSettings),
            ScoreBoardPos.Left + 150, PlayersDrawListTop + 42, 50, 145, False);
          DrawShadowedText(p_Offscreen, Format('+%.0f', [PlayerPtr.Resources.fEnergyProduction], FormatSettings),
            ScoreBoardPos.Left + 150, PlayersDrawListTop + 58, 50, 145, False);

          BarTagRect.Left := ScoreBoardPos.Left + 39 + 3;
          BarTagRect.Top := PlayersDrawListTop + 44;
          BarTagRect.Right := BarTagRect.Left + 100;
          BarTagRect.Bottom := PlayersDrawListTop + 47;
          DrawBar(p_Offscreen, @BarTagRect, 0);
          BarTagRect.Right := BarTagRect.Left +
            Round((PlayerPtr.Resources.fCurrentMetal / PlayerPtr.Resources.fMetalStorageMax) * 100);
          DrawBar(p_Offscreen, @BarTagRect, 128);

          BarTagRect.Top := PlayersDrawListTop + 60;
          BarTagRect.Right := BarTagRect.Left + 100;
          BarTagRect.Bottom := PlayersDrawListTop + 63;
          DrawBar(p_Offscreen, @BarTagRect, 0);
          BarTagRect.Right := BarTagRect.Left +
            Round((PlayerPtr.Resources.fCurrentEnergy / PlayerPtr.Resources.fEnergyStorageMax) * 100);
          DrawBar(p_Offscreen, @BarTagRect, 193);

          DrawLine(p_Offscreen, ScoreBoardPos.Left + 7, PlayersDrawListTop + 72,
            ScoreBoardPos.Left + ScoreBoardWidth-7, PlayersDrawListTop + 72, 120);

        end;
        SetFontType(p_OldFont);
      end;
{      if TAData.NetworkLayerEnabled then
      begin
        if ( IteratePlayerIdx = TAData.LocalPlayerID ) then
        begin
          StrExt := GetStrExtent(PAnsiChar('n/a'));
          DrawText(p_Offscreen, PAnsiChar('n/a'),
            ScoreBoardPos.Left + ScoreBoardWidth-6 - StrExt - 2,
            PlayersDrawListTop + 21, 119, 0);
        end else
        begin
          Counter := PlayerPtr.nPing;
          StrExt := GetStrExtent(PAnsiChar(IntToStr(Counter)));
          DrawText(p_Offscreen, PAnsiChar(IntToStr(Counter)),
            ScoreBoardPos.Left + ScoreBoardWidth-6 - StrExt - 2,
            PlayersDrawListTop + 21, 119, 0);
        end;
      end;  }
      PlayersDrawListTop := PlayersDrawListTop + NextTop;
SortPlayers:
      if ( IteratePlayerIdx = 10 ) then
      begin
        cCurActiveSortPlayer := 0;
        PlayerSort := TAPlayer.GetPlayerByIndex(cCurActiveSortPlayer);
        cIterateSort := 10;
        repeat
          if ( TAPlayer.IsActive(PlayerSort) ) then
          begin
            PlayerSortType := TAPlayer.PlayerController(PlayerSort);
            if (PlayerSortType = Player_LocalHuman) or
               (PlayerSortType = Player_LocalAI) or
               (PlayerSortType = Player_RemotePlayer) then
            begin
              if (PlayerSort.cPlayerIndex <> 10) then
                if (PlayerSort.nNumUnits <> 0) then
                  if ((PlayerSort.PlayerInfo^.PropertyMask and $40) = 0) then
                    if (PlayerSort.cPlayerScoreboard > cCurActivePlayer) then
                      PlayerSort.cPlayerScoreboard := PlayerSort.cPlayerScoreboard - 1;
            end;
          end;
          Inc(cCurActiveSortPlayer);
          PlayerSort := TAPlayer.GetPlayerByIndex(cCurActiveSortPlayer);
          Dec(cIterateSort);
        until ( cIterateSort = 0 );
      end;
      Inc(cCurActivePlayer);
    until cCurActivePlayer = TAData.MainStruct.nActivePlayersCount;

    // now draw watchers
    for IteratePlayerIdx := 0 to 9 do
    begin
      PlayerPtr := TAPlayer.GetPlayerByIndex(IteratePlayerIdx);
      if TAPlayer.IsActive(PlayerPtr) then
      begin
        PlayerType := TAPlayer.PlayerController(PlayerPtr);
        if (PlayerType = Player_LocalHuman) or
           (PlayerType = Player_LocalAI) or
           (PlayerType = Player_RemotePlayer) then
        begin
          if (PlayerPtr.nNumUnits = 0) then
          begin
            TextLeftOff := ScoreBoardPos.Left + 7 + 3;

            DrawText(p_Offscreen, PlayerPtr.szName,
              TextLeftOff, PlayersDrawListTop + 1, ScoreBoardWidth-6, 0);

            DrawText(p_Offscreen, TranslateString(PAnsiChar('Watcher')),
              TextLeftOff, PlayersDrawListTop + 21, 119, 0);
            {
            if TAData.NetworkLayerEnabled then
            begin
              if ( IteratePlayerIdx = TAData.LocalPlayerID ) then
              begin
                StrExt := GetStrExtent(PAnsiChar('n/a'));
                DrawText(p_Offscreen, PAnsiChar('n/a'),
                  ScoreBoardPos.Left + ScoreBoardWidth-6 - StrExt - 2,
                  PlayersDrawListTop + 21, 119, 0);
              end else
              begin
                Counter := PlayerPtr.nPing;
                StrExt := GetStrExtent(PAnsiChar(IntToStr(Counter)));
                DrawText(p_Offscreen, PAnsiChar(IntToStr(Counter)),
                  ScoreBoardPos.Left + ScoreBoardWidth-6 - StrExt - 2,
                  PlayersDrawListTop + 21, 119, 0);
              end;
            end;
            }
            NextTop := 40;
            if TAData.GameingType = gtSkirmish then
            begin
              if TAPlayer.GetAlliedState(PlayerPtr, TAData.LocalPlayerID) and
                 (TAData.LocalPlayerID <> PlayerPtr.cPlayerIndex) then
                NextTop := 75;
            end;
            PlayersDrawListTop := PlayersDrawListTop + NextTop;
          end;
        end;
      end;
    end;
  end;
end;

function GetCustomFlameStream(Weapon: PWeaponDef): Pointer; stdcall;
begin
  if Length(ExtraGAFAnimations.FlameStream) > 0 then
  begin
    if ExtraGAFAnimations.FlameStream[Weapon.ucColor - 1] <> nil then
      Result := ExtraGAFAnimations.FlameStream[Weapon.ucColor - 1]
    else
      Result := TAData.MainStruct.flamestream;
  end else
    Result := TAData.MainStruct.flamestream;
end;

procedure WeaponProjectileFlameStreamHook;
label
  CustomFlameStream,
  GoBack;
asm
  mov     cl, byte ptr [ebx+TWeaponDef.ucColor]
  test    cl, cl
  jnz     CustomFlameStream
  mov     edi, [edi+TTADynMemStruct.flamestream]
  jmp GoBack
CustomFlameStream :
  push    esi
  push    eax
  push    ebx
  push    ecx
  push    edx
  push    ebx // weapon
  call    GetCustomFlameStream
  mov     edi, eax
  pop     edx
  pop     ecx
  pop     ebx
  pop     eax
  pop     esi
GoBack :
  push $0049C3A4
  call PatchNJump
end;

type
  TPoints = array of TPoint;

procedure DrawDashCircle(p_Offscreen: Pointer;
  CenterX, CenterZ, Radius: Integer; Angle: Integer; ColorOffset: Byte);

  procedure RotateCircle(const Source: TPoints;
    lAngle: Integer; var Output: TPoints);
  var
    i: Integer;
    c, s, dx, dy: Real;
  begin
    if Length(Source) > 0 then
    Begin
      lAngle := EnsureRange(lAngle, 0, 360);
      c := cos(lAngle * PI / 180);
      s := sin(lAngle * PI / 180);
      SetLength(Output, Length(Source));
      for i := Low(Source) to High(Source) do
      begin
        dx := Source[i].X - CenterX;
        dy := Source[i].Y - CenterZ;
        Output[i].X := Round(CenterX + dx*c - dy*s);
        Output[i].Y := Round(CenterZ + dx*s + dy*c);
      end;
    end;
  end;

var
  Points: TPoints;
  OutPoints: TPoints;
  x, y: Integer;
  lRadiusError: Integer;
  i: Integer;
  bDrawOrSkip: Boolean;
begin
  x := Radius;
  y := 0;
  lRadiusError := 1 - x;
  bDrawOrSkip := True;
  while (x >= y) do
  begin
    if (y mod (Round(Radius / PI) - 1)) = 0 then
      bDrawOrSkip := not bDrawOrSkip;
    if bDrawOrSkip then
    begin
      SetLength(Points, Length(Points) + 8);
      Points[High(Points)-7].X := x + CenterX;
      Points[High(Points)-7].Y := y + CenterZ;
      Points[High(Points)-6].X := y + CenterX;
      Points[High(Points)-6].Y := x + CenterZ;
      Points[High(Points)-5].X := -x + CenterX;
      Points[High(Points)-5].Y := y + CenterZ;
      Points[High(Points)-4].X := -y + CenterX;
      Points[High(Points)-4].Y := x + CenterZ;
      Points[High(Points)-3].X := -x + CenterX;
      Points[High(Points)-3].Y := -y + CenterZ;
      Points[High(Points)-2].X := -y + CenterX;
      Points[High(Points)-2].Y := -x + CenterZ;
      Points[High(Points)-1].X := x + CenterX;
      Points[High(Points)-1].Y := -y + CenterZ;
      Points[High(Points)].X := y + CenterX;
      Points[High(Points)].Y := -x + CenterZ;
    end;
    Inc(y);
    if lRadiusError < 0 then
      lRadiusError := lRadiusError + (2 * y + 1)
    else
    begin
      Dec(x);
      lRadiusError := lRadiusError + (2 * (y - x) + 1);
    end;
  end;
  if (Angle <> 0) or (Angle <> 360) then
    RotateCircle(Points, Angle, OutPoints)
  else
    OutPoints := Points;
  for i := Low(OutPoints) to High(OutPoints) do
    DrawPoint(p_Offscreen, OutPoints[I].X, OutPoints[I].Y, ColorOffset);
end;

procedure DrawUnitSelectBox(p_Offscreen: Pointer; p_Unit: PUnitStruct); stdcall;
var
  x, z, y: Integer;
  CenterX, CenterZ: Integer;
  Radius: Integer;
  InMove: Boolean;
  Speed: Integer;
  CurOrder: TTAActionType;
  AnimDelay: Integer;
  IsMobileUnit: Boolean;
  ColorOffset: Byte;
  Angle: Integer;

  OffY, OffX: Integer;
  p_GAFFrame: PGAFFrame;
  ScreenPos: TPosition;
  p_Animation: PGAFSequence;
  UnitID: Word;
begin
  if (UnitInfoCustomFields[p_Unit.nUnitInfoID].SelectBoxType = 2) and
     (UnitInfoCustomFields[p_Unit.nUnitInfoID].SelectAnimation <> 0) then
  begin
    p_Animation := ExtraGAFAnimations.CustAnim[UnitInfoCustomFields[p_Unit.nUnitInfoID].SelectAnimation - 1];
    if p_Animation <> nil then
    begin
      UnitID := TAUnit.GetId(p_Unit);
      ScreenPos.X := p_Unit.Position.X - (TAData.MainStruct.lEyeBallMapX shl 16);
      ScreenPos.Y := p_Unit.Position.Y;
      ScreenPos.Z := p_Unit.Position.Z - (TAData.MainStruct.lEyeBallMapY shl 16);
      OffY := SHIWORD(ScreenPos.z) - (SHIWORD(ScreenPos.y) div 2) + 32;
      OffX := SHIWORD(ScreenPos.x) + 128;
      if UnitsCustomFields[UnitID].SelectAnimStartTime = 0 then
        UnitsCustomFields[UnitID].SelectAnimStartTime := TAData.GameTime;
      p_GAFFrame := GAF_SequenceIndex2Frame(p_Animation,
        (TAData.GameTime - UnitsCustomFields[UnitID].SelectAnimStartTime) mod p_Animation.Frames);
      if UnitInfoCustomFields[p_Unit.nUnitInfoID].SelectAnimationAlpha then
        AlphaCompsteBuf2OFFScreen(p_Offscreen, p_GAFFrame, OffX, OffY)
      else
        CopyGafToContext(p_Offscreen, p_GAFFrame, OffX, OffY);
      Exit;
    end;
  end;
  IsMobileUnit := TAMem.UnitInfoId2Ptr(p_Unit.nUnitInfoID).cBMCode = 1;
  if ((IniSettings.UnitSelectBoxType = 1) and not IsMobileUnit) then
    DrawUnitSelectBoxRect(p_Offscreen, p_Unit)
  else
    if (UnitInfoCustomFields[p_Unit.nUnitInfoID].SelectBoxType = 1) or
       ((IniSettings.UnitSelectBoxType = 1) and IsMobileUnit) or
       (IniSettings.UnitSelectBoxType = 2) then
    begin
      x := SHiWord(p_Unit.Position.X - (TAData.MainStruct.lEyeBallMapX shl 16));
      z := SHiWord(p_Unit.Position.Z - (TAData.MainStruct.lEyeBallMapY shl 16));
      y := SHiWord(p_Unit.Position.Y);
      CenterX := x + 128;
      CenterZ := z - (y div 2) + 32;
      Radius := 1 + (p_Unit.p_UnitInfo.lWidthHypot shr 16);

      InMove := False;
      if p_unit.p_MovementClass <> nil then
      begin
        Speed := TAUnit.GetCurrentSpeedPercent(p_Unit);
        if Speed > 0 then
        begin
          InMove := True;
          Radius := Round(Radius + (Radius * Speed) / IniSettings.UnitSelectBoxZoomRatio);
        end;
      end else
      begin
        CurOrder := TAUnit.GetCurrentOrderType(p_Unit);
        if (CurOrder <> Action_Ready) and
           (CurOrder <> Action_Standby) and
           (CurOrder <> Action_VTOL_Standby) and
           (CurOrder <> Action_Wait) and
           (CurOrder <> Action_Guard_NoMove) and
           (CurOrder <> Action_NoResult) then
        begin
          InMove := True;
        end;
      end;
      if InMove then
        AnimDelay := 240
      else
        AnimDelay := 320;

      ColorOffset := PByte(Cardinal(TAData.ColorsPalette)+GetRaceSpecificColor(0))^;
      if (IniSettings.UnitSelectBoxAnimType and 1) = 1 then
      begin
        if (IniSettings.UnitSelectBoxAnimType and 4) = 4 then
          Angle := (p_Unit.Turn.Z div 182) mod 360
        else
          Angle := 360 - (360 * (TAData.GameTime mod AnimDelay) div AnimDelay);
        DrawDashCircle(p_Offscreen, CenterX, CenterZ, Radius, Angle, ColorOffset);
      end;
      if (IniSettings.UnitSelectBoxAnimType and 2) = 2 then
      begin
        if (IniSettings.UnitSelectBoxAnimType and 8) = 8 then
          Angle := 360 - (p_Unit.Turn.Z div 182) mod 360
        else
          Angle := 360 * (TAData.GameTime mod AnimDelay) div AnimDelay;
        DrawDashCircle(p_Offscreen, CenterX, CenterZ, Radius, Angle, ColorOffset);
      end;
    end else
      if (UnitInfoCustomFields[p_Unit.nUnitInfoID].SelectBoxType = 0) then
        DrawUnitSelectBoxRect(p_Offscreen, p_Unit)
end;

procedure DrawingBottomStateRefresh;
label
  ForceDraw,
  DoNotRefresh;
asm
  jnz     ForceDraw
  mov     ecx, ForceBottomStateRefresh
  test    ecx, ecx
  jz      DoNotRefresh
  mov     ForceBottomStateRefresh, 0
ForceDraw :
  push    $0046ACE3
  call    PatchNJump
DoNotRefresh :
  push    $0046B8EA
  call    PatchNJump
end;

Procedure OnInstallGUIEnhancements;
begin
  if IniSettings.HealthBarDynamicSize then
  begin
    if IniSettings.HealthBarCategories[0] <> 0 then
      STANDARDUNIT := IniSettings.HealthBarCategories[0];
    if IniSettings.HealthBarCategories[1] <> 0 then
      MEDIUMUNIT := IniSettings.HealthBarCategories[1];
    if IniSettings.HealthBarCategories[2] <> 0 then
      BIGUNIT := IniSettings.HealthBarCategories[2];
    if IniSettings.HealthBarCategories[3] <> 0 then
      HUGEUNIT := IniSettings.HealthBarCategories[3];
    if IniSettings.HealthBarCategories[4] <> 0 then
      EXTRALARGEUNIT := IniSettings.HealthBarCategories[4];
  end;
end;

Procedure OnUninstallGUIEnhancements;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_GUIEnhancements then
  begin
    Result := TPluginData.Create( True,
                                  'GUIEnhancements',
                                   State_GUIEnhancements,
                                   @OnInstallGUIEnhancements,
                                   @OnUninstallGUIEnhancements );

    Result.MakeRelativeJmp( State_GUIEnhancements,
                            'ExtraUnitBars_MainCall',
                            @ExtraUnitBars_MainCall,
                            $00469CB1, 1 );

    Result.MakeRelativeJmp( State_GUIEnhancements,
                            'TrueIncomeHook',
                            @TrueIncomeHook,
                            $004695EE, 0 );

    Result.MakeRelativeJmp( State_GUIEnhancements,
                            'HealthPercentage',
                            @HealthPercentage,
                            $0046B088, 1 );

    Result.MakeStaticCall( State_GUIEnhancements,
                           'Draw unit selection box',
                           @DrawUnitSelectBox,
                           $00469B8A );
    Result.MakeStaticCall( State_GUIEnhancements,
                           'Draw unit selection box 2',
                           @DrawUnitSelectBox,
                           $004699EB );

    Result.MakeRelativeJmp( State_GUIEnhancements,
                            'Draw new unit ranges for +showranges mode',
                            @DrawUnitRanges_ShowrangesOnHook,
                            $0043924A, 2 );

    Result.MakeRelativeJmp( State_GUIEnhancements,
                            'Draw new unit ranges, showranges disabled',
                            @DrawUnitRanges_CustomRanges,
                            $00439C9B, 1 );

    if IniSettings.ExplosionsGameUIExpand > 0 then
    begin
      Result.MakeRelativeJmp( State_GUIEnhancements,
                              '',
                              @DrawExplosionsRectExpandHook,
                              $00420C47, 0 );
      Result.MakeRelativeJmp( State_GUIEnhancements,
                              '',
                              @DrawExplosionsRectExpandHook2,
                              $00420BA2, 0 );
    end;

    if IniSettings.ScoreBoard then
    begin
      Result.MakeRelativeJmp( State_GUIEnhancements,
                              'load core and arm logos',
                              @LoadArmCore32ltGafSequences,
                              $0043190B, 1 );

      Result.MakeStaticCall( State_GUIEnhancements,
                             'Draw score board with side logos and allied AI economy',
                             @DrawScoreboard,
                             $00469F65 );
    end;

    Result.MakeRelativeJmp( State_GUIEnhancements,
                            'WeaponProjectileFlameStreamHook',
                            @WeaponProjectileFlameStreamHook,
                            $0049C39E, 1 );

    Result.MakeRelativeJmp( State_GUIEnhancements,
                            'Unit bottom state force refresh',
                            @DrawingBottomStateRefresh,
                            $0046ACDD, 1 );
  end else
    Result := nil;
end;

initialization
  FormatSettings.DecimalSeparator := '.';

end.
