unit ExtraUnitBars;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_ExtraUnitBars : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallExtraUnitBars;
Procedure OnUninstallExtraUnitBars;

// -----------------------------------------------------------------------------

procedure ExtraUnitBars_MainCall;
procedure TrueIncomeHook;
procedure SelectedUnitsCounter;

implementation
uses
  IniOptions,
  SysUtils,
  Windows,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_FunctionsU;

const
  STANDARDUNIT : cardinal = 250;
  MEDIUMUNIT : cardinal = 1900;
  BIGUNIT : cardinal = 3000;
  HUGEUNIT : cardinal = 5000;
  EXTRALARGEUNIT : cardinal = 10000;
  MIN_WEAP_RELOAD = 3 * 30; // seconds
  VETERANLEVEL_RELOADBOOST = 12; // 30 * 0.2
  OFFSCREEN_off = -$1F0;
  OFFSCREEN_off_BottomState = 4;

var
  ExtraUnitBarsPlugin: TPluginData;

Procedure OnInstallExtraUnitBars;
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

Procedure OnUninstallExtraUnitBars;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_ExtraUnitBars then
  begin
    ExtraUnitBarsPlugin := TPluginData.Create( True,
                            'ExtraUnitBars',
                            State_ExtraUnitBars,
                            @OnInstallExtraUnitBars,
                            @OnUninstallExtraUnitBars );

    if (IniSettings.Plugin_HBWidth <> 0) or
       IniSettings.Plugin_HBDynamicSize or
       IniSettings.Plugin_WeaponReload then
      ExtraUnitBarsPlugin.MakeRelativeJmp(State_ExtraUnitBars,
                            'ExtraUnitBars_MainCall',
                            @ExtraUnitBars_MainCall,
                            $00469CB1, 0);

    if IniSettings.Plugin_TrueIncome then
      ExtraUnitBarsPlugin.MakeRelativeJmp(State_ExtraUnitBars,
                            'TrueIncomeHook',
                            @TrueIncomeHook,
                            $004695EE, 0);

   { ExtraUnitBarsPlugin.MakeRelativeJmp(State_ExtraUnitBars,
                            'SelectedUnitsCounter',
                            @SelectedUnitsCounter,
                            $0046ABE2, 1); }

    Result:= ExtraUnitBarsPlugin;
  end else
    Result := nil;
end;

type
  TDrawPos = record
    x1 : Integer;
    y1 : Integer;
    x2 : Integer;
    y2 : Integer;
  end;

function DrawUnitState(Offscreen_p: Cardinal; Unit_p: Cardinal; CenterPosX : Integer; CenterPosZ: Integer) : Integer; stdcall;
var
  { drawing }
  ColorsPal : Pointer;
  RectDrawPos : TDrawPos;
  // initial drawing position for this unit
  //PosX, PosZ, PosY : Integer;

  { health bar }
  HPBackgRectWidth, HPFillRectWidth: Smallint;
  UnitHealth,  HealthState : Word;
  UnitMaxHP : Cardinal;
  
  UnitInfo : PUnitfInfo;
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
  WeapReloadPerc : Integer;
  StockPile : Byte;

  { transporters }
  TransportCount : Integer;

  { reclaim, resurrect feature }
  CurOrder : TTAActionType;
  OrderStateCorrect : Boolean;
  FeatureDefPtr : Pointer;
  FeatureDefID : Integer;
  ResurrectedUnitType : Word;
  ResurrectedUnitBuildTime : Cardinal;
  ResurrectorTime : Double;
  ResurrectorWorkTime : Word;
  MaxActionVal : Cardinal;
  CurActionVal : Cardinal;
  UnknownFeat : Pointer;
begin
  Result := 0;
  BottomZ := 0;
  // is drawing health bars enabled and any of local player units are actually on screen
  if ((PTAdynmemStruct(TAData.MainStructPtr)^.GameOptionMask and 1) = 1) or
     (PUnitStruct(Unit_p).HotKeyGroup <> 0) then
  begin
   // UnitPos := PUnitStruct(Unit_p).Position;
{    PosX := UnitPos.X;
    PosZ := UnitPos.Z;
    PosY := sar32(UnitPos.Y, 1);

    CenterPosX := PosX - PTAdynmemStruct(TAData.MainStructPtr)^.lEyeBallMapX + 128;
    CenterPosZ := PosZ - PTAdynmemStruct(TAData.MainStructPtr)^.lEyeBallMapY - PosY + 32; }
    UnitInfo := PUnitStruct(Unit_p).p_UnitDef;

    if ((PTAdynmemStruct(TAData.MainStructPtr)^.GameOptionMask and 1) = 1) and
       (CenterPosX <> 0) and
       (CenterPosZ <> 0) and
       (UnitInfo <> nil) then
    begin
      if PPlayerStruct(PUnitStruct(Unit_p).p_Owner).cPlayerIndexZero = TAData.ViewPlayer then
      begin
        UnitHealth := PUnitStruct(Unit_p).nHealth;
        UnitBuildTimeLeft := PUnitStruct(Unit_p).lBuildTimeLeft;

        if (UnitHealth > 0) then
        begin
          ColorsPal := Pointer(LongWord(TAData.MainStructPtr)+$DCB);

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

          if UnitHealth <= UnitMaxHP then
          begin
            HPFillRectWidth := (HPBackgRectWidth div 2);

            RectDrawPos.x1 := Word(CenterPosX) - HPFillRectWidth;
            RectDrawPos.x2 := Word(CenterPosX) + HPFillRectWidth;
            RectDrawPos.y1 := Word(CenterPosZ) - 2;
            RectDrawPos.y2 := Word(CenterPosZ) + 2;

            DrawRectangle(Offscreen_p, @RectDrawPos, PByte(ColorsPal)^);
            Inc(RectDrawPos.y1);
            Dec(RectDrawPos.y2);
            Inc(RectDrawPos.x1);

            RectDrawPos.x2 := Round(RectDrawPos.x1 + ((HPBackgRectWidth-2) * UnitHealth) / UnitMaxHP);
            HealthState := UnitMaxHP div 3;

            if IniSettings.Plugin_Colors then
            begin
              if UnitHealth <= (HealthState * 2) then
              begin
                if UnitHealth <= HealthState then
                  DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+IniSettings.Colors[Ord(UNITHEALTHBARLOW)])^)  // low
                else
                  DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+IniSettings.Colors[Ord(UNITHEALTHBARMEDIUM)])^); // yellow
              end else
                DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+IniSettings.Colors[Ord(UNITHEALTHBARGOOD)])^); // good
            end else
            begin
              if UnitHealth <= (HealthState * 2) then
              begin
                if UnitHealth <= HealthState then
                  DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+12)^)  // low
                else
                  DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+14)^); // medium
              end else
                DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+10)^); // good
            end;
          end;

        // weapons reload
        if IniSettings.Plugin_WeaponReload and (UnitBuildTimeLeft = 0.0) then
        begin
          if (PUnitStruct(Unit_p).p_Weapon1 <> nil) or
             (PUnitStruct(Unit_p).p_Weapon2 <> nil) or
             (PUnitStruct(Unit_p).p_Weapon3 <> nil) then
          begin
            MaxReloadTime := 0;
            CurReloadTime := 0;
            StockPile := 0;

            if (PUnitStruct(Unit_p).p_Weapon1 <> nil) then
            begin
              if (PWeaponDef(PUnitStruct(Unit_p).p_Weapon1).lWeaponTypeMask and (1 shl 28) = 1 shl 28) then
                StockPile := 1;
              if PWeaponDef(PUnitStruct(Unit_p).p_Weapon1).nReloadTime >= MIN_WEAP_RELOAD then
              begin
                MaxReloadTime := PWeaponDef(PUnitStruct(Unit_p).p_Weapon1).nReloadTime;
                CurReloadTime := MaxReloadTime - PUnitStruct(Unit_p).nWeapon1_ReloadTime;
              end;
            end;
            if (PUnitStruct(Unit_p).p_Weapon2 <> nil) then
            begin
              if (PWeaponDef(PUnitStruct(Unit_p).p_Weapon2).lWeaponTypeMask and (1 shl 28) = 1 shl 28) then
                StockPile := 2;
              if PWeaponDef(PUnitStruct(Unit_p).p_Weapon2).nReloadTime >= MIN_WEAP_RELOAD then
              begin
                MaxReloadTime := PWeaponDef(PUnitStruct(Unit_p).p_Weapon2).nReloadTime;
                CurReloadTime := MaxReloadTime - PUnitStruct(Unit_p).nWeapon2_ReloadTime;
              end;
            end;
            if (PUnitStruct(Unit_p).p_Weapon3 <> nil) then
            begin
              if (PWeaponDef(PUnitStruct(Unit_p).p_Weapon3).lWeaponTypeMask and (1 shl 28) = 1 shl 28) then
                StockPile := 3;
              if PWeaponDef(PUnitStruct(Unit_p).p_Weapon3).nReloadTime >= MIN_WEAP_RELOAD then
              begin
                MaxReloadTime := PWeaponDef(PUnitStruct(Unit_p).p_Weapon3).nReloadTime;
                CurReloadTime := MaxReloadTime - PUnitStruct(Unit_p).nWeapon3_ReloadTime;
              end;
            end;

            if MaxReloadTime <> 0 then
            begin
              BottomZ := 6;
              if PUnitStruct(Unit_p).nKills >= 5 then
              begin
                CurReloadTime := CurReloadTime - VETERANLEVEL_RELOADBOOST;
                MaxReloadTime := MaxReloadTime - VETERANLEVEL_RELOADBOOST;
              end;

              if CurReloadTime < 0 then
                CurReloadTime := 0;

              // stockpile weapon build progress instead of reload bar
              if (StockPile <> 0) and IniSettings.Plugin_Stockpile then
              begin
                if PUnitStruct(Unit_p).p_FutureOrder <> nil then
                begin
                  MaxReloadTime := 100;
                  CurReloadTime := GetUnit_BuildWeaponProgress(Unit_p);
                end else
                  MaxReloadTime := 0; // disable drawing bar
              end;

              if MaxReloadTime > 0 then
              begin
                RectDrawPos.x1 := Word(CenterPosX) - 17;
                RectDrawPos.x2 := Word(CenterPosX) + 17;
                RectDrawPos.y1 := Word(CenterPosZ) + 5 - 2;
                RectDrawPos.y2 := Word(CenterPosZ) + 5 + 2;
              {  if StockPile <> 0 then
                  DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+4)^)
                else   }
                DrawRectangle(Offscreen_p, @RectDrawPos, PByte(ColorsPal)^);

                Inc(RectDrawPos.y1);
                Dec(RectDrawPos.y2);
                Inc(RectDrawPos.x1);
                RectDrawPos.x2 := Round(RectDrawPos.x1 + (32 * CurReloadTime) / MaxReloadTime);

                if StockPile <> 0 then
                  DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+14)^)
                else
                begin
                  WeapReloadPerc := Round((CurReloadTime / MaxReloadTime) * 100);
                  case WeapReloadPerc of
                     0..20 : result := DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+142)^);
                    21..40 : result := DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+141)^);
                    41..60 : result := DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+140)^);
                    61..80 : result := DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+139)^);
                   81..100 : result := DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+138)^);
                  end;
                end;
              end;
            end;
          end; { Unit got any weapons }
        end; { IniSettings.Plugin_WeaponReloadTimeBar }

        end; {UnitHealth > 0}

        if IniSettings.Plugin_MinReclaimTime > 0 then
        begin
          CurOrder := TAUnit.GetCurrentOrder(Pointer(Unit_p));
          if (CurOrder = Action_Reclaim) or
             (CurOrder = Action_VTOL_Reclaim) or
             (CurOrder = Action_Resurrect) then
          begin
            if (CurOrder = Action_Reclaim) or
               (CurOrder = Action_Resurrect) then
              OrderStateCorrect := TAUnit.GetCurrentOrderState(Pointer(Unit_p)) and $400000 = $400000
            else
              OrderStateCorrect := TAUnit.GetCurrentOrderState(Pointer(Unit_p)) and $100000 = $100000;
            if OrderStateCorrect then
            begin
              FeatureDefID := GetFeatureTypeOfOrder(@PUnitOrder(PUnitStruct(Pointer(Unit_p)).p_UnitOrders).Pos, PUnitStruct(Pointer(Unit_p)).p_UnitOrders, LongWord(@UnknownFeat));
              if FeatureDefID <> -1 then
              begin
                FeatureDefPtr := TAMem.FeatureDefId2Ptr(FeatureDefID);
                MaxActionVal := 0;
                CurActionVal := 0;
                case CurOrder of
                  Action_Reclaim :
                  begin
                    MaxActionVal := Trunc( (PFeatureDefStruct(FeatureDefPtr).metal + PFeatureDefStruct(FeatureDefPtr).energy) / 2 + 15);
                    CurActionVal := TAUnit.GetCurrentOrderParams(Pointer(Unit_p), 0);
                  end;
                  Action_VTOL_Reclaim :
                  begin
                    MaxActionVal := Trunc( (PFeatureDefStruct(FeatureDefPtr).metal + PFeatureDefStruct(FeatureDefPtr).energy) / 2 + 30);
                    CurActionVal := TAUnit.GetCurrentOrderParams(Pointer(Unit_p), 0);
                  end;
                  Action_Resurrect :
                  begin
                    ResurrectedUnitType := TAUnit.GetCurrentOrderParams(Pointer(Unit_p), 0);
                    if ResurrectedUnitType <> 0 then
                    begin
                      ResurrectedUnitBuildTime := PUnitfInfo(TAMem.UnitInfoId2Ptr(ResurrectedUnitType)).lBuildTime;
                      ResurrectorWorkTime := PUnitfInfo(PUnitStruct(Pointer(Unit_p)).p_UnitDef).nWorkerTime div 30;
                      ResurrectorTime := (ResurrectedUnitBuildTime * 0.3) / ResurrectorWorkTime;
                      MaxActionVal := Trunc(ResurrectorTime);
                      CurActionVal := TAUnit.GetCurrentOrderParams(Pointer(Unit_p), 1);
                    end;
                  end;
                end;

                if (MaxActionVal <> 0) and
                   (CurActionVal <> 0) and
                   (CurActionVal <= MaxActionVal) and  // TA changes par1 to it once feature is reclaimed but order state remains "reclaiming"...
                   (MaxActionVal >= LongWord(IniSettings.Plugin_MinReclaimTime * 30)) then
                begin
                  RectDrawPos.x1 := Word(CenterPosX) - 17;
                  RectDrawPos.x2 := Word(CenterPosX) + 17;
                  RectDrawPos.y1 := Word(CenterPosZ) - 7;
                  RectDrawPos.y2 := Word(CenterPosZ) - 3;

                  DrawRectangle(Offscreen_p, @RectDrawPos, PByte(ColorsPal)^);

                  Inc(RectDrawPos.y1);
                  Dec(RectDrawPos.y2);
                  Inc(RectDrawPos.x1);
                  RectDrawPos.x2 := Round(RectDrawPos.x1 + (32 * CurActionVal) / MaxActionVal);

                  WeapReloadPerc := Round((CurActionVal / MaxActionVal) * 100);
                  case WeapReloadPerc of
                    0..20 : result := DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+17)^);
                   21..40 : result := DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+18)^);
                   41..60 : result := DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+19)^);
                   61..80 : result := DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+20)^);
                  81..100 : result := DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+21)^);
                  end;
                end;
              end;
            end;
          end;
        end;
        //TAMem.GetFeatureDef($5C)

        // built weapons counter (nukes)
        if IniSettings.Plugin_Stockpile then
          if PUnitStruct(Unit_p).Weapon1Stock > 0 then
            DrawText_Heavy(Offscreen_p, PAnsiChar(IntToStr(PUnitStruct(Unit_p).Weapon1Stock)), Word(CenterPosX), Word(CenterPosZ) - 13, -1);

        // transporter count
        if IniSettings.Plugin_Transporters and (UnitInfo.cTransportCap > 0) then
        begin
          MaxUnitId := TAMem.GetMaxUnitId;
          TransportCount := 0;
          for UnitId := 1 to MaxUnitId do
          begin
            if LongWord(PUnitStruct(TAUnit.Id2Ptr(UnitId)).TransporterUnit_p) = LongWord(Unit_p) then
              Inc(TransportCount);
          end;
          if TransportCount > 0 then
            DrawText_Heavy(Offscreen_p, PAnsiChar(IntToStr(TransportCount) + '/' + IntToStr(UnitInfo.cTransportCap)), Word(CenterPosX) - 10, Word(CenterPosZ) - 80, -1);
        end;
      end;

      if (PPlayerStruct(PUnitStruct(Unit_p).p_Owner).cPlayerIndexZero = TAData.ViewPlayer) and
         (PUnitStruct(Unit_p).HotKeyGroup <> 0) then
      begin
        AllowHotkeyDraw := False;
        if PUnitStruct(Unit_p).TransporterUnit_p <> nil then
        begin
          if PUnitStruct(PUnitStruct(Unit_p).TransporterUnit_p).HotKeyGroup = 0 then
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

