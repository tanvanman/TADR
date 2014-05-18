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
  MIN_WEAP_RELOAD = 4 * 30; // seconds
  VETERANLEVEL_RELOADBOOST = 12; // 30 * 0.2
  OFFSCREEN_off = -$1F0;

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
       IniSettings.Plugin_WeaponReloadTimeBar or
       IniSettings.Plugin_GroupsBigFont then
      ExtraUnitBarsPlugin.MakeRelativeJmp(State_ExtraUnitBars,
                            'ExtraUnitBars_MainCall',
                            @ExtraUnitBars_MainCall,
                            $00469C4A, 0);
                            
    if IniSettings.Plugin_TrueIncome then
      ExtraUnitBarsPlugin.MakeRelativeJmp(State_ExtraUnitBars,
                            'TrueIncomeHook',
                            @TrueIncomeHook,
                            $004695EE, 0);

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

function sar32(value, shift: longint): longint;
asm
  mov ecx, edx
  sar eax, cl
end;

function DrawUnitState(Offscreen_p: Cardinal; Unit_p: Cardinal) : Integer; stdcall;
var
  { drawing }
  ColorsPal : Pointer;
  RectDrawPos : TDrawPos;
  // initial drawing position for this unit
  CenterPosX, CenterPosZ : Integer;
  PosX, PosZ, PosY : Integer;

  { health bar }
  HPBackgRectWidth, HPFillRectWidth: Smallint;
  UnitHealth, UnitMaxHP, HealthState : Word;

  UnitInfo : PGameUnitfInfo;
  UnitPos : TPosition;
  UnitId, MaxUnitId : Cardinal;

  { hotkey group }
  BottomZ : Word;
  sGroup : PAnsiChar;
  AllowHotkeyDraw : Boolean;
  
  { weapons }
  MaxReloadTime : Integer;
  CurReloadTime : Integer;
  WeapReloadPerc : Integer;

  { transporters }
  TransportCount : Integer;
begin
  Result := 0;
  BottomZ := 0;
  // is drawing health bars enabled and any of local player units are actually on screen
  if ((PTAdynmemStruct(TAData.MainStructPtr)^.GameOptionMask and 1) = 1) or
     (PUnitStruct(Unit_p).HotKeyGroup <> 0) then
  begin
    UnitPos := PUnitStruct(Unit_p).Position;
    PosZ := UnitPos.Z;
    PosX := UnitPos.X;
    PosY := sar32(UnitPos.Y, 1);

    CenterPosX := PosX - PTAdynmemStruct(TAData.MainStructPtr)^.lEyeBallMapX + 128;
    CenterPosZ := PosZ - PTAdynmemStruct(TAData.MainStructPtr)^.lEyeBallMapY - PosY + 32;
    
    if (PTAdynmemStruct(TAData.MainStructPtr)^.GameOptionMask and 1) = 1 then
    begin
      if PPlayerStruct(PUnitStruct(Unit_p).p_Owner).cPlayerIndexZero = TAData.ViewPlayer then
      begin
        UnitHealth := PUnitStruct(Unit_p).nHealth;
        if UnitHealth > 0 then
        begin
          UnitInfo := PUnitStruct(Unit_p).p_UnitDef;
          ColorsPal := Pointer(LongWord(TAData.MainStructPtr)+$DCB);

          UnitMaxHP := UnitInfo.nMaxHP;
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

          HPFillRectWidth := (HPBackgRectWidth div 2);

          RectDrawPos.x1 := Word(CenterPosX) - HPFillRectWidth;
          RectDrawPos.x2 := Word(CenterPosX) + HPFillRectWidth;
          RectDrawPos.y1 := Word(CenterPosZ) + 10 - 2;
          RectDrawPos.y2 := Word(CenterPosZ) + 10 + 2;

          DrawRectangle(Offscreen_p, @RectDrawPos, PByte(ColorsPal)^);
          Inc(RectDrawPos.y1);
          Dec(RectDrawPos.y2);
          Inc(RectDrawPos.x1);

          RectDrawPos.x2 := Round(RectDrawPos.x1 + ((HPBackgRectWidth-2) * UnitHealth) / UnitMaxHP);
          HealthState := UnitMaxHP div 3;

          if UnitHealth <= (HealthState * 2) then
          begin
            if UnitHealth <= HealthState then
              DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+12)^)
            else
              DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+14)^);
          end else
            DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+10)^);
        end; {UnitHealth > 0}

        // weapons reload
        if IniSettings.Plugin_WeaponReloadTimeBar then
        begin
          if (PUnitStruct(Unit_p).p_Weapon1 <> nil) or
             (PUnitStruct(Unit_p).p_Weapon2 <> nil) or
             (PUnitStruct(Unit_p).p_Weapon3 <> nil) then
          begin
            MaxReloadTime := 0;
            CurReloadTime := 0;
            if (PUnitStruct(Unit_p).p_Weapon1 <> nil) then
              if PWeaponDef(PUnitStruct(Unit_p).p_Weapon1).nReloadTime >= MIN_WEAP_RELOAD then
              begin
                MaxReloadTime := PWeaponDef(PUnitStruct(Unit_p).p_Weapon1).nReloadTime;
                CurReloadTime := MaxReloadTime - PUnitStruct(Unit_p).Weapon1_ReloadTime;
              end;

            if (PUnitStruct(Unit_p).p_Weapon2 <> nil) then
              if PWeaponDef(PUnitStruct(Unit_p).p_Weapon2).nReloadTime >= MIN_WEAP_RELOAD then
              begin
                MaxReloadTime := PWeaponDef(PUnitStruct(Unit_p).p_Weapon2).nReloadTime;
                CurReloadTime := MaxReloadTime - PUnitStruct(Unit_p).Weapon2_ReloadTime;
              end;

            if (PUnitStruct(Unit_p).p_Weapon3 <> nil) then
              if PWeaponDef(PUnitStruct(Unit_p).p_Weapon3).nReloadTime >= MIN_WEAP_RELOAD then
              begin
                MaxReloadTime := PWeaponDef(PUnitStruct(Unit_p).p_Weapon3).nReloadTime;
                CurReloadTime := MaxReloadTime - PUnitStruct(Unit_p).Weapon3_ReloadTime;
              end;

            if MaxReloadTime > 0 then
            begin
              BottomZ := 6;
              if PUnitStruct(Unit_p).nKills >= 5 then
              begin
                CurReloadTime := CurReloadTime - VETERANLEVEL_RELOADBOOST;
                MaxReloadTime := MaxReloadTime - VETERANLEVEL_RELOADBOOST;
              end;

              if CurReloadTime < 0 then
                CurReloadTime := 0;

              RectDrawPos.x1 := Word(CenterPosX) - 17;
              RectDrawPos.x2 := Word(CenterPosX) + 17;
              RectDrawPos.y1 := Word(CenterPosZ) + 15 - 2;
              RectDrawPos.y2 := Word(CenterPosZ) + 15 + 2;

              DrawRectangle(Offscreen_p, @RectDrawPos, PByte(ColorsPal)^);
              Inc(RectDrawPos.y1);
              Dec(RectDrawPos.y2);
              Inc(RectDrawPos.x1);

              RectDrawPos.x2 := Round(RectDrawPos.x1 + (32 * CurReloadTime) / MaxReloadTime);
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
        end; { IniSettings.Plugin_WeaponReloadTimeBar }

        // transporter count
        if UnitInfo.cTransportCap > 0 then
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
          if IniSettings.Plugin_GroupsBigFont then
          begin
            SetFONTLENGTH_ptr(PTAdynmemStruct(TAData.MainStructPtr)^.lengthOfCOMIXFnt);
            DrawText_Heavy(Offscreen_p, @sGroup, Word(CenterPosX) - 2, Word(CenterPosZ) + 14 + BottomZ, -1);
            SetFONTLENGTH_ptr(PTAdynmemStruct(TAData.MainStructPtr)^.lengthOFsmlfontFnt);
          end else
            DrawText_Heavy(Offscreen_p, @sGroup, Word(CenterPosX), Word(CenterPosZ) + 14 + BottomZ, -1);
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
  push    edi             // Unit
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

end.

