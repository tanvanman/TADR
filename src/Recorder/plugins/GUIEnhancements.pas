unit GUIEnhancements;

interface
uses
  PluginEngine, TA_MemoryStructures;

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
procedure HealthPercentage;
procedure DrawCircleUnitSelectHook;
procedure DrawBuildSpot_InitScript;
procedure DrawBuildSpot_NanoframeHook;
procedure DrawBuildSpot_QueueNanoframeHook;
procedure DrawBuildSpot_NanoframeShimmerHook;
procedure DrawUnitRanges_ShowrangesOnHook;
procedure DrawUnitRanges_CustomRanges;
//procedure LoadArmCore32ltGafSequences;
//procedure DrawScoreboard;
procedure BroadcastNanolatheParticles_BuildingBuild;
procedure BroadcastNanolatheParticles_MobileBuild;
procedure BroadcastNanolatheParticles_HelpBuild;
procedure BroadcastNanolatheParticles_Resurrect;
procedure BroadcastNanolatheParticles_RepairResurrect;
//procedure BroadcastNanolatheParticles_RepairUnk2;       // what is this
procedure BroadcastNanolatheParticles_VTOL_MobileBuild;
procedure BroadcastNanolatheParticles_VTOL_HelpBuild;
procedure BroadcastNanolatheParticles_VTOL_Repair;
procedure BroadcastNanolatheParticles_Capture;
procedure BroadcastNanolatheParticles_ReclaimUnit;
procedure BroadcastNanolatheParticles_ReclaimFeature;
procedure BroadcastNanolatheParticles_VTOLReclaimUnit;
procedure BroadcastNanolatheParticles_VTOLReclaimFeature; 
procedure ScreenFadeControl;
procedure WeaponProjectileFlameStreamHook;
{
function AddNanoUnit(x, y, color: Integer): LongBool; stdcall;
function InitNanoUnit: LongBool; stdcall;

exports
   AddNanoUnit index 12,
   InitNanoUnit index 13;
}
implementation
uses
  Windows,
  IniOptions,
  SysUtils,
  TA_MemoryConstants,
  TA_MemoryLocations,
  UnitInfoExpand,
  Math,
  TA_FunctionsU,
  logging,
  Colors,
  idplay,
  TA_NetworkingMessages;

const
  OFFSCREEN_off = -$1F0;
  STANDARDUNIT : cardinal = 250;
  MEDIUMUNIT : cardinal = 1900;
  BIGUNIT : cardinal = 3000;
  HUGEUNIT : cardinal = 5000;
  EXTRALARGEUNIT : cardinal = 10000;
  VETERANLEVEL_RELOADBOOST = 12; // 30 * 0.2

const
  NanoUnitCreateInit : AnsiString = 'NanoFrameInit';
  Core32Lt : AnsiString = 'Core32Dk';
  Arm32Lt : AnsiString = 'Arm32Dk';
  RaceLogo : AnsiString = 'RaceLogo';
  SCOREBOARD_WIDTH : Byte = 180;

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
                            'HealthPercentage',
                            @HealthPercentage,
                            $0046B088, 1);

    if IniSettings.Plugin_CircleUnitSelect then
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'DrawCircleUnitSelectHook',
                            @DrawCircleUnitSelectHook,
                            $00467AF1, 0);

    // ---------------------------------
    // nanoframe units
    // ---------------------------------

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'DrawBuildSpot_InitScript',
                            @DrawBuildSpot_InitScript,
                            $00485DE1, 0);

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'DrawBuildSpot_NanoframeHook',
                            @DrawBuildSpot_NanoframeHook,
                            $00469F23, 1);

    if IniSettings.Plugin_DrawBuildSpotQueueNano then
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              'DrawBuildSpot_QueueNanoframeHook',
                              @DrawBuildSpot_QueueNanoframeHook,
                              $00438C38, 0);

    if not IniSettings.Plugin_BuildSpotNanoShimmer then
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              'DrawBuildSpot_NanoframeShimmerHook',
                              @DrawBuildSpot_NanoframeShimmerHook,
                              $00458E18, 2);

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'Draw new unit ranges for +showranges mode',
                            @DrawUnitRanges_ShowrangesOnHook,
                            $0043924A, 2);

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'Draw new unit ranges, showranges disabled',
                            @DrawUnitRanges_CustomRanges,
                            $00439C9B, 1);


    // ---------------------------------
    // new score board
    // ---------------------------------
    {
    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'load core and arm logos colored',
                            @LoadArmCore32ltGafSequences,
                            $0043190B, 1);

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'new scoreboard :)',
                            @DrawScoreboard,
                            $00494A61, 2);

    GUIEnhancementsPlugin.MakeReplacement(State_GUIEnhancements,
                            '', $00494996, SCOREBOARD_WIDTH, 1);

    GUIEnhancementsPlugin.MakeReplacement(State_GUIEnhancements,
                            '', $004949ED, SCOREBOARD_WIDTH, 1);

    GUIEnhancementsPlugin.MakeReplacement(State_GUIEnhancements,
                            '', $00494A06, SCOREBOARD_WIDTH, 1);

    GUIEnhancementsPlugin.MakeReplacement(State_GUIEnhancements,
                            '', $00494A23, SCOREBOARD_WIDTH, 1);

    GUIEnhancementsPlugin.MakeReplacement(State_GUIEnhancements,
                            '', $00494A39, SCOREBOARD_WIDTH, 1);

    GUIEnhancementsPlugin.MakeReplacement(State_GUIEnhancements,
                            '', $00494A67, SCOREBOARD_WIDTH, 1);
    }

    // ---------------------------------
    // nanolathe particles broadcast
    // ---------------------------------

    if IniSettings.Plugin_BroadcastNanolathe then
    begin
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_BuildingBuild,
                              $00403EC4, 3);

      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_MobileBuild,
                              $00402AB3, 5);

      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_HelpBuild,
                              $004041BA, 3);

      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_Resurrect,
                              $00405097, 1);

      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_RepairResurrect,
                              $004056C7, 1);
{
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_RepairUnk2,
                              $004058EC, 1);
}
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_VTOL_MobileBuild,
                              $004142B0, 1);

      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_VTOL_HelpBuild,
                              $004146D2, 0);

      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_VTOL_Repair,
                              $004151E6, 1);

      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_Capture,
                              $00404670, 1);

      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_ReclaimUnit,
                              $00404A48, 1);

      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_ReclaimFeature,
                              $00404D35, 1);

      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_VTOLReclaimUnit,
                              $00414C3D, 1);

      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @BroadcastNanolatheParticles_VTOLReclaimFeature,
                              $00414A1A, 1);
    end;

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                           'Control game screen fade level',
                           @ScreenFadeControl,
                           $0046A2E2, 0 );

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                           'WeaponProjectileFlameStreamHook',
                           @WeaponProjectileFlameStreamHook,
                           $0049C39E, 1 );

    Result:= GUIEnhancementsPlugin;
  end else
    Result := nil;
end;

function DrawUnitState(Offscreen_p: Cardinal; Unit_p: PUnitStruct;
  CenterPosX : Integer; CenterPosZ: Integer) : Integer; stdcall;
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
  FeatureDefID : SmallInt;
  ActionUnitType : Word;
  ActionUnitBuildTime : Cardinal;
  ActionUnitBuildCost1, ActionUnitBuildCost2 : Single;
  ActionTime : Double;
  ActionWorkTime : Word;
  MaxActionVal : Cardinal;
  CurActionVal : Cardinal;
  ReclaimColor : Byte;
begin
  Result := 0;
  BottomZ := 0;
  // is drawing health bars enabled and any of local player units are actually on screen
  if ((PTAdynmemStruct(TAData.MainStructPtr).GameOptionMask and 1) = 1) or
     (PUnitStruct(Unit_p).HotKeyGroup <> 0) then
  begin
    UnitInfo := PUnitStruct(Unit_p).p_UNITINFO;
    ColorsPal := Pointer(LongWord(TAData.MainStructPtr)+$DCB);
    
    if ((PTAdynmemStruct(TAData.MainStructPtr).GameOptionMask and 1) = 1) and
       (CenterPosX <> 0) and
       (CenterPosZ <> 0) and
       (UnitInfo <> nil) then
    begin
      //AlliedUnit := TAPlayer.GetAlliedState(TAUnit.GetOwnerPtr(Unit_p), TAData.ViewPlayer);
      LocalUnit := (PPlayerStruct(PUnitStruct(Unit_p).p_Owner).cPlayerIndex = TAData.LocalPlayerID);
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
             not ExtraUnitInfoTags[UnitInfo.nCategory].HideHPBar then
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

            if IniSettings.Plugin_Colors and (GetRaceSpecificColor(23) <> 0) then
            begin
              if UnitHealth <= (HealthState * 2) then
              begin
                if UnitHealth <= HealthState then
                  DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+GetRaceSpecificColor(25))^)
                else
                  DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+GetRaceSpecificColor(24))^);
              end else
                DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+GetRaceSpecificColor(23))^);
            end else
            begin
              if UnitHealth <= (HealthState * 2) then
              begin
                if UnitHealth <= HealthState then
                  DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+ColorsArray[25].cDefaultVal)^)  // low
                else
                  DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+ColorsArray[24].cDefaultVal)^); // medium
              end else
                DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+ColorsArray[23].cDefaultVal)^); // good
            end;
          end;

        // weapons reload
        if ((IniSettings.Plugin_MinWeaponReload <> 0) or
           (ExtraUnitInfoTags[UnitInfo.nCategory].UseCustomReloadBar)) and
           (UnitBuildTimeLeft = 0.0) then
        begin
          MaxReloadTime := 0;
          CurReloadTime := 0;
          StockPile := 0;
          CustomReloadBar := False;
          if ExtraUnitInfoTags[UnitInfo.nCategory].UseCustomReloadBar then
          begin
            if CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].LongID = TAUnit.GetLongId(Unit_p) then
            begin
              if CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].CustomWeapReload then
              begin
                MaxReloadTime := CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].CustomWeapReloadMax;
                CurReloadTime := CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].CustomWeapReloadCur;
                CustomReloadBar := True;
              end;
            end;
          end else
          begin
            if (PUnitStruct(Unit_p).UnitWeapons[0].p_Weapon <> nil) or
               (PUnitStruct(Unit_p).UnitWeapons[1].p_Weapon <> nil) or
               (PUnitStruct(Unit_p).UnitWeapons[2].p_Weapon <> nil) then
            begin
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
            end;
          end;

          if CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].TeleportReloadMax <> 0 then
          begin
            MaxReloadTime := CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].TeleportReloadMax;
            CurReloadTime := CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].TeleportReloadCur;
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
                DrawRectangle(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+4)^)
              else   }
              DrawBar(Offscreen_p, @RectDrawPos, PByte(ColorsPal)^);

              Inc(RectDrawPos.Top);
              Dec(RectDrawPos.Bottom);
              Inc(RectDrawPos.Left);
              RectDrawPos.Right := Round(RectDrawPos.Left + (32 * CurReloadTime) / MaxReloadTime);

              if StockPile <> 0 then
              begin
                if IniSettings.Plugin_Colors and (GetRaceSpecificColor(28) <> 0) then
                  DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+GetRaceSpecificColor(28))^)
                else
                  DrawBar(Offscreen_p, @RectDrawPos, PByte(LongWord(ColorsPal)+ColorsArray[28].cDefaultVal)^);
              end else
              begin
                BarProgress := Round((CurReloadTime / MaxReloadTime) * 100);
                if IniSettings.Plugin_Colors and (GetRaceSpecificColor(26) <> 0) then
                  WeapReloadColor := GetRaceSpecificColor(26)
                else
                  WeapReloadColor := ColorsArray[26].cDefaultVal;
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
        end; { IniSettings.Plugin_WeaponReloadTimeBar }

        end; {UnitHealth > 0}

        if IniSettings.Plugin_MinReclaimTime > 0 then
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
                FeatureDefID := GetFeatureTypeOfPos(@PUnitOrder(PUnitStruct(Unit_p).p_MainOrder).Pos, PUnitStruct(Unit_p).p_MainOrder, nil);
              if FeatureDefID <> -1 then
              begin
                MaxActionVal := 0;
                CurActionVal := 0;
                case CurOrder of
                  Action_Reclaim :
                  begin
                    FeatureDefPtr := TAMem.FeatureDefId2Ptr(FeatureDefID);
                    MaxActionVal := Trunc( (PFeatureDefStruct(FeatureDefPtr).metal + PFeatureDefStruct(FeatureDefPtr).energy) / 2 + 15);
                    CurActionVal := TAUnit.GetCurrentOrderParams(Unit_p, 1);
                  end;
                  Action_VTOL_Reclaim :
                  begin
                    FeatureDefPtr := TAMem.FeatureDefId2Ptr(FeatureDefID);
                    MaxActionVal := Trunc( (PFeatureDefStruct(FeatureDefPtr).metal + PFeatureDefStruct(FeatureDefPtr).energy) / 2 + 30);
                    CurActionVal := TAUnit.GetCurrentOrderParams(Unit_p, 1);
                  end;
                  Action_Capture:
                  begin
                    ActionUnitType := TAUnit.GetUnitInfoId(TAUnit.GetCurrentOrderTargetUnit(Unit_p));
                    if ActionUnitType <> 0 then
                    begin
                      ActionUnitBuildCost1 := PUnitInfo(TAMem.UnitInfoId2Ptr(ActionUnitType)).lBuildCostEnergy * 30 * 0.00050000002;
                      ActionUnitBuildCost2 := PUnitInfo(TAMem.UnitInfoId2Ptr(ActionUnitType)).lBuildCostMetal * 30 * -0.0071428572;
                      ActionTime := (ActionUnitBuildCost1 - ActionUnitBuildCost2) + 150;

                      MaxActionVal := Round(ActionTime);
                      if MaxActionVal - TAUnit.GetCurrentOrderParams(Unit_p, 1) > 0 then
                        CurActionVal := MaxActionVal - TAUnit.GetCurrentOrderParams(Unit_p, 1);
                    end;
                  end;
                  Action_Resurrect :
                  begin
                    FeatureDefPtr := TAMem.FeatureDefId2Ptr(FeatureDefID);
                    ActionUnitType := TAUnit.GetCurrentOrderParams(Unit_p, 1);
                    if ActionUnitType <> 0 then
                    begin
                      ActionUnitBuildTime := PUnitInfo(TAMem.UnitInfoId2Ptr(ActionUnitType)).lBuildTime;
                      ActionWorkTime := PUnitInfo(PUnitStruct(Unit_p).p_UNITINFO).nWorkerTime div 30;
                      ActionTime := (ActionUnitBuildTime * 0.3) / ActionWorkTime;
                      MaxActionVal := Trunc(ActionTime);
                      CurActionVal := TAUnit.GetCurrentOrderParams(Unit_p, 2);
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
                  if IniSettings.Plugin_Colors and (GetRaceSpecificColor(27) <> 0) then
                    ReclaimColor := GetRaceSpecificColor(27)
                  else
                    ReclaimColor := ColorsArray[27].cDefaultVal;
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

        // built weapons counter (nukes)
        if IniSettings.Plugin_Stockpile then
          if PUnitStruct(Unit_p).UnitWeapons[0].cStock > 0 then
            DrawText_Heavy(Offscreen_p, PAnsiChar(IntToStr(PUnitStruct(Unit_p).UnitWeapons[0].cStock)), Word(CenterPosX), Word(CenterPosZ) - 13, -1);

        // transporter count
        if IniSettings.Plugin_Transporters then
        begin
          if (UnitInfo.UnitTypeMask and 2048 = 2048) then   // unit is air
          begin
            if (ExtraUnitInfoTags[UnitInfo.nCategory].MultiAirTransport = 0) then
              TransportCap := 0
            else
              if (ExtraUnitInfoTags[UnitInfo.nCategory].TransportWeightCapacity = 0) then
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
            WeightTransportMax := ExtraUnitInfoTags[UnitInfo.nCategory].TransportWeightCapacity;
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

      if (PPlayerStruct(PUnitStruct(Unit_p).p_Owner).cPlayerIndex = TAData.LocalPlayerID) and
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
    if PTAdynmemStruct(TAData.MainStructPtr).field_391B3 <> 0 then
      UnitStateProbe(Offscreen_p);
    if PTAdynmemStruct(TAData.MainStructPtr).field_391B9 <> 0 then
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

procedure DrawHealthPercentage(top, curr, left, max: Cardinal; offscreen_p: Cardinal); register;
var
  HealthPercent : Single;
  HealthPercentStr : PAnsiChar;
  v65 : LongInt;
  FormatSettings: TFormatSettings;
begin
  if (Max = 0) or (Curr > Max) then
    Exit;

  FormatSettings.DecimalSeparator := '.';
  v65 := sub_4C13F0;
  HealthPercentStr := '';

  HealthPercent := (Curr / Max) * 100;
  if HealthPercent > 0.0 then
  begin
    SetFontColor(83, v65);
    HealthPercentStr := PAnsiChar(Format('%.1f%%', [HealthPercent], FormatSettings))
  end;
  DrawText_Heavy(offscreen_p, HealthPercentStr, Left - 38, Top - 4, -1);
end;

procedure HealthPercentage;
asm
  add     eax, ebx
  mov     [esp+18h], ecx
  pushAD
  mov     ebx, [esp+30h]
  mov     ecx, [esp+34h]
  push    ebx
  push    edi
  call    DrawHealthPercentage
  popAD
  push $0046B08E;
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
  UnitInfo := UnitPtr.p_UNITINFO;

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

procedure PrepareNanoUnit(UnitPtr: PUnitStruct);
var
  UnitInfo : PUnitInfo;
begin
  UnitInfo := UnitPtr.p_UNITINFO;
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

procedure DrawBuildSpotNanoframe(pOffscreen: Cardinal); stdcall;
var
  X, Z : Integer;
  Position : TPosition;
begin
  if PByte($004BF8C0)^ = $C2 then
    Exit;

  if NanoSpotUnitSt.nUnitInfoID <> 0 then
    FreeUnitMem(@NanoSpotUnitSt);

  if (PTAdynmemStruct(TAData.MainStructPtr).nBuildNum <> 0) and
     (PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType = $E) then
  begin
    NanoSpotUnitSt.nUnitInfoID := PTAdynmemStruct(TAData.MainStructPtr).nBuildNum;
    NanoSpotUnitSt.p_UNITINFO := TAMem.UnitInfoId2Ptr(NanoSpotUnitSt.nUnitInfoID);
    if (ExtraUnitInfoTags[PUnitInfo(NanoSpotUnitSt.p_UNITINFO).nCategory].DrawBuildSpotNanoFrame = True) or
       IniSettings.Plugin_ForceDrawBuildSpotNano then
    begin
      X := PTAdynmemStruct(TAData.MainStructPtr).lBuildPosRealX;
      Z := PTAdynmemStruct(TAData.MainStructPtr).lBuildPosRealY;
      if (X < 0) or (Z < 0) then
        Exit;
      GetTPosition(X, Z, Position);
      NanoSpotUnitSt.p_Owner := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);
      if UNITS_AllocateUnit(@NanoSpotUnitSt, X, Position.Y, Z, 0) then
      begin
        NanoSpotUnitInfoSt := PUnitInfo(TAMem.UnitInfoId2Ptr(NanoSpotUnitSt.nUnitInfoID))^;
        NanoSpotUnitSt.p_UNITINFO := @NanoSpotUnitInfoSt;

        NanoSpotUnitSt.Position.X := NanoSpotUnitSt.Position.x_ + NanoSpotUnitSt.nFootPrintX * 8;
        NanoSpotUnitSt.Position.x_ := 0;
        NanoSpotUnitSt.Position.Z := NanoSpotUnitSt.Position.z_ + NanoSpotUnitSt.nFootPrintZ * 8;
        NanoSpotUnitSt.Position.z_ := 0;

        if NanoSpotUnitInfoSt.cBMCode = 1 then
        begin
          UNITS_AllocateMovementClass(@NanoSpotUnitSt);
        end;

        NanoSpotUnitSt.Turn.Z := 32768;
        NanoSpotUnitSt.Position.y_ := 0;
        UNITS_FixYPos(@NanoSpotUnitSt);
        NanoSpotUnitSt.Position.Y := Position.Y;
        if NanoSpotUnitInfoSt.nMinWaterDepth <> -10000 then
          if NanoSpotUnitInfoSt.cWaterLine = 0 then
            NanoSpotUnitSt.Position.Y := 0;

        if (PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $40 = $40) then
          NanoSpotUnitSt.nKills := 1
        else
          NanoSpotUnitSt.nKills := 2;

        if UNITS_CreateModelScripts(@NanoSpotUnitSt) <> nil then
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
{
function AddNanoUnit(x, y, color: Integer): LongBool; stdcall;
var
  Position : TPosition;
  i : Integer;
begin
  Result := False;

  if (X < 0) or (Y < 0) then
    Exit;

  SetLength(LineNanoSpotUnitSt, High(LineNanoSpotUnitSt) + 2);
  i := High(LineNanoSpotUnitSt);

  LineNanoSpotUnitSt[i].nUnitInfoID := PTAdynmemStruct(TAData.MainStructPtr).nBuildNum;
  LineNanoSpotUnitSt[i].p_UNITINFO := TAMem.UnitInfoId2Ptr(LineNanoSpotUnitSt[i].nUnitInfoID);
  LineNanoSpotUnitInfoSt := PUnitInfo(LineNanoSpotUnitSt[i].p_UNITINFO)^;
  
  GetTPosition(X, Y, Position);
  LineNanoSpotUnitSt[i].p_Owner := TAPlayer.GetPlayerByIndex(TAData.ViewPlayer);

  if UNITS_AllocateUnit(@LineNanoSpotUnitSt[i], X, Position.Y, Y, 0) then
  begin
    LineNanoSpotUnitSt[i].p_UNITINFO := @LineNanoSpotUnitInfoSt;
    PrepareNanoUnit(@LineNanoSpotUnitSt[0]);

    LineNanoSpotUnitSt[i].Position.X := LineNanoSpotUnitSt[i].Position.x_ + LineNanoSpotUnitSt[i].nFootPrintX * 8;
    LineNanoSpotUnitSt[i].Position.x_ := 0;
    LineNanoSpotUnitSt[i].Position.Z := LineNanoSpotUnitSt[i].Position.z_ + LineNanoSpotUnitSt[i].nFootPrintZ * 8;
    LineNanoSpotUnitSt[i].Position.z_ := 0;
    UNITS_FixYPos(@LineNanoSpotUnitSt[i]);

    if LineNanoSpotUnitInfoSt.cBMCode = 1 then
    begin
      UNITS_AllocateMovementClass(@LineNanoSpotUnitSt[i]);
    end;

    LineNanoSpotUnitSt[i].Turn.Z := 32768;
    LineNanoSpotUnitSt[i].Position.y_ := 0;

    if (PTAdynmemStruct(TAData.MainStructPtr).cBuildSpotState and $40 = $40) then
      LineNanoSpotUnitSt[i].nKills := 1
    else
      LineNanoSpotUnitSt[i].nKills := 2;

    if UNITS_CreateModelScripts(@LineNanoSpotUnitSt[i]) <> nil then
    begin
      DrawUnit(0, @LineNanoSpotUnitSt[i]);
      FreeUnitMem(@LineNanoSpotUnitSt[i]);
    end;
  end;
  Result := True;
end;

function InitNanoUnit: LongBool; stdcall;
begin
  FillChar(LineNanoSpotUnitInfoSt, SizeOf(TUnitInfo), 0);
  SetLength(LineNanoSpotUnitSt, 0);
  Result := True;
end;
}
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
    NanoSpotQueueUnitSt.p_UNITINFO := TAMem.UnitInfoId2Ptr(NanoSpotQueueUnitSt.nUnitInfoID);
    X := Position.X;
    Z := Position.Z;
    GetTPosition(X, Z, Position);

    NanoSpotQueueUnitSt.p_Owner := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);
    if UNITS_AllocateUnit(@NanoSpotQueueUnitSt, X, Position.Y, Z, 0) then
    begin
      NanoSpotQueueUnitInfoSt := PUnitInfo(TAMem.UnitInfoId2Ptr(NanoSpotQueueUnitSt.nUnitInfoID))^;
      NanoSpotQueueUnitSt.p_UNITINFO := @NanoSpotQueueUnitInfoSt;

      NanoSpotQueueUnitSt.Position.X := NanoSpotQueueUnitSt.Position.x_;
      NanoSpotQueueUnitSt.Position.x_ := 0;
      NanoSpotQueueUnitSt.Position.Z := NanoSpotQueueUnitSt.Position.z_;
      NanoSpotQueueUnitSt.Position.z_ := 0;

      if NanoSpotQueueUnitInfoSt.cBMCode = 1 then
      begin
        UNITS_AllocateMovementClass(@NanoSpotQueueUnitSt);
      end;

      NanoSpotQueueUnitSt.Turn.Z := 32768;
      NanoSpotQueueUnitSt.Position.Y := Position.Y;
        if NanoSpotQueueUnitInfoSt.nMinWaterDepth <> -10000 then
          if NanoSpotQueueUnitInfoSt.cWaterLine = 0 then
            NanoSpotQueueUnitSt.Position.Y := 0;

      if ((PUnitStruct(UnitOrder.p_Unit).lUnitStateMask shr 4) and 1 = 1) then
        NanoSpotQueueUnitSt.nKills := 1
      else
        NanoSpotQueueUnitSt.nKills := 2;

      if UNITS_CreateModelScripts(@NanoSpotQueueUnitSt) <> nil then
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

function DrawUnitRangesShowrangesOn(OFFSCREEN_Ptr: Cardinal; CirclePointer: Cardinal; UnitInfo: PUnitInfo;
  UnitOrder: PUnitOrder; ReturnVal: Integer): Integer; stdcall;
begin
  // as a result give amount of circles that were drawn
  if ( ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].TeleportMinDistance <> 0 ) then
  begin
    Inc(ReturnVal);
    DrawRangeCircle(
        OFFSCREEN_ptr,
        CirclePointer,
        @PUnitStruct(UnitOrder.p_Unit).Position,
        ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].TeleportMinDistance,
        138,
        PAnsiChar('minteleport'),
        ReturnVal);
  end;
  if ( ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].TeleportMaxDistance <> 0 ) then
  begin
    Inc(ReturnVal);
    DrawRangeCircle(
        OFFSCREEN_ptr,
        CirclePointer,
        @PUnitStruct(UnitOrder.p_Unit).Position,
        ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].TeleportMaxDistance,
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

procedure DrawUnitRangesShowrangesOff(OFFSCREEN_Ptr: Cardinal; CirclePointer: Cardinal;
  UnitOrder: PUnitOrder); stdcall;
var
  CustomRange : Integer;
  Radius : Integer;
  GameTime : Integer;
  UnitInfo : PUnitInfo;
begin
  UnitInfo := UnitOrder.p_Unit.p_UNITINFO;
  if UnitInfo = nil then
    Exit;
    
  if ( ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].CustomRange1Distance <> 0 ) then
  begin
    DrawRangeCircle(
        OFFSCREEN_ptr,
        CirclePointer,
        @PUnitStruct(UnitOrder.p_Unit).Position,
        ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].CustomRange1Distance,
        ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].CustomRange1Color,
        nil,
        0); 
  end;
  if ( ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].CustomRange2Distance <> 0 ) then
  begin
    if ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].CustomRange2Animate then
    begin
      CustomRange := ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].CustomRange2Distance;
      Radius := 8;
      GameTime := TAData.GameTime mod 60;
      if ((2 * CustomRange * GameTime div 60) >= 8 ) then
        Radius := 2 * CustomRange * GameTime div 60;
      if ( Radius >= CustomRange ) then
        Radius := ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].CustomRange2Distance;
    end else
      Radius := ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].CustomRange2Distance;

    DrawRangeCircle(
        OFFSCREEN_ptr,
        CirclePointer,
        @PUnitStruct(UnitOrder.p_Unit).Position,
        Radius,
        ExtraUnitInfoTags[PUnitInfo(UnitInfo).nCategory].CustomRange2Color,
        nil,
        0);
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
{
procedure LoadArmCore32ltGafSequences;
asm
  push    eax

  mov     eax, [TADynMemStructPtr]
  mov     ecx, [eax+TTAdynmemStruct.p_LogosGaf]
  push    Arm32Lt
  push    ecx
  call    GAF_Name2Sequence
  mov     GafSequence_Arm32lt, eax

  mov     eax, [TADynMemStructPtr]
  mov     ecx, [eax+TTAdynmemStruct.p_LogosGaf]
  push    Core32Lt
  push    ecx
  call    GAF_Name2Sequence
  mov     GafSequence_Core32lt, eax

  pop     eax
  mov     edx, [TADynMemStructPtr]
  push $00431911;
  call PatchNJump;
end;

function DrawNewScoreboard(OFFSCREEN_Ptr: Cardinal): Cardinal; stdcall;
var
  ScoreBoardPos : tagRect;
  PlayersDrawListTop : Integer;
  Player, PlayerSort : PPlayerStruct;
  cCurActivePlayer, IteratePlayerIdx : Byte;
  cCurActiveSortPlayer : Byte;
  cIterateSort : Byte;
  PlayerType, PlayerSortType: TTAPlayerController;
  PlayerSide : TTAPlayerSide;
  LocalPlayerBox : tagRect;
  p_ColorLogo : PGAFFrame;
  Counter : Integer;
  PlayerLogoRect, PlayerLogoTransform : TGAFFrameTransform;
  TextLeftOff : Integer;
label
  SortPlayers;
  //IterateWatcher;
begin
  Result := 0;
  ScoreBoardPos.Left := GetTA_ScreenWidth - SCOREBOARD_WIDTH;//PInteger($0051F2D8)^;
  ScoreBoardPos.Right := ScoreBoardPos.Left + SCOREBOARD_WIDTH;
  ScoreBoardPos.Top := 32;
  ScoreBoardPos.Bottom := 40 * TAData.ActivePlayersCount + 46;

  DrawTransparentBox(OFFSCREEN_Ptr, @ScoreBoardPos, -23);

  DrawText(OFFSCREEN_Ptr, TranslateString(PAnsiChar('Player')), ScoreBoardPos.Left + 5, ScoreBoardPos.Top, 119, 0);
  DrawText(OFFSCREEN_Ptr, TranslateString(PAnsiChar('K/D')),
           ScoreBoardPos.Right - GetStrExtent(TranslateString(PAnsiChar('K/D'))) - 2,
           ScoreBoardPos.Top, 119, 0);

  PlayersDrawListTop := ScoreBoardPos.Top + 15;
  cCurActivePlayer := 0;
  if TAData.ActivePlayersCount <> 0 then
  begin
    repeat
      IteratePlayerIdx := 0;
      Player := TAPlayer.GetPlayerByIndex(IteratePlayerIdx);
      while True do
      begin
        if TAPlayer.IsActive(Player) then
        begin
          PlayerType := TAPlayer.PlayerController(Player);
          if ( PlayerType = Player_LocalHuman ) or
             ( PlayerType = Player_LocalAI ) or
             ( PlayerType = Player_RemotePlayer ) then
          begin
            if ( TAPlayer.PlayerIndex(Player) <> 10 ) then
              if ( (Player.nNumUnits <> 0) or (Player.lUnitsCounter = 0) ) then
                if ( Player.PlayerInfo.PropertyMask and $40 <> $40 ) then
                  if ( Player.cPlayerScoreboard = cCurActivePlayer ) then
                    Break;
          end;
        end;
        Inc(IteratePlayerIdx);
        Player := Pointer(Cardinal(Player) + SizeOf(TPlayerStruct));
        //TAPlayer.GetPlayerByIndex(IteratePlayerIdx);
        if ( IteratePlayerIdx >= 10 ) then
          goto SortPlayers;
      end;
      LocalPlayerBox.Left := ScoreBoardPos.Left + 4;
      LocalPlayerBox.Right := ScoreBoardPos.Right - 4;
      LocalPlayerBox.Top := PlayersDrawListTop - 1;
      LocalPlayerBox.Bottom := PlayersDrawListTop + 37;
      if ( IteratePlayerIdx = TAData.LocalPlayerID ) then
        DrawTransparentBox(OFFSCREEN_Ptr, @LocalPlayerBox, -19)
      else
        DrawTransparentBox(OFFSCREEN_Ptr, @LocalPlayerBox, -24);

      PlayerSide := TAPlayer.PlayerSide(Player);

      case PlayerSide of
        psArm :
          p_ColorLogo := GAF_SequenceIndex2Frame(GafSequence_Arm32lt, TAPlayer.PlayerLogoIndex(Player));
        psCore :
          p_ColorLogo := GAF_SequenceIndex2Frame(GafSequence_Core32lt, TAPlayer.PlayerLogoIndex(Player));
        else p_ColorLogo := nil;
      end;

      if p_ColorLogo <> nil then
      begin
        TextLeftOff := ScoreBoardPos.Left + 39 + 3;
        PlayerLogoRect.Rect1.Left := ScoreBoardPos.Left + 7;
        PlayerLogoRect.Rect1.Top := PlayersDrawListTop + 2;       // top
        PlayerLogoRect.Rect1.Right := ScoreBoardPos.Left + 39;    // width raw + 7
        PlayerLogoRect.Rect1.Bottom := PlayersDrawListTop + 2;

        PlayerLogoRect.Rect2.Left := ScoreBoardPos.Left + 39;     // width raw + 7
        PlayerLogoRect.Rect2.Top := PlayersDrawListTop + 34;      // height raw
        PlayerLogoRect.Rect2.Right := ScoreBoardPos.Left + 7;
        PlayerLogoRect.Rect2.Bottom := PlayersDrawListTop + 34;   // height raw

        PlayerLogoTransform.Rect1.Left := 0;
        PlayerLogoTransform.Rect1.Top := 0;
        PlayerLogoTransform.Rect1.Right := p_ColorLogo.Width - 1;
        PlayerLogoTransform.Rect1.Bottom := 0;

        PlayerLogoTransform.Rect2.Left := p_ColorLogo.Width - 1;
        PlayerLogoTransform.Rect2.Top := p_ColorLogo.Height - 1;
        PlayerLogoTransform.Rect2.Right := 0;
        PlayerLogoTransform.Rect2.Bottom := p_ColorLogo.Height - 1;

        GAF_DrawTransformed(OFFSCREEN_Ptr, p_ColorLogo, @PlayerLogoRect, @PlayerLogoTransform);
      end else
      begin
        TextLeftOff := ScoreBoardPos.Left + 7 + 3;
      end;

      DrawText(OFFSCREEN_Ptr, Player.szName, TextLeftOff, PlayersDrawListTop + 1, SCOREBOARD_WIDTH-6, 0);

      if ( PTAdynmemStruct(TAData.MainStructPtr).bAlterKills = 2 ) then
        Counter := Player.nKills_Last
      else
        Counter := Player.nKills;
      DrawText(OFFSCREEN_Ptr, PAnsiChar(IntToStr(Counter)), TextLeftOff, PlayersDrawListTop + 22, 119, PByte($51F2C8 + IteratePlayerIdx)^);

      if ( PTAdynmemStruct(TAData.MainStructPtr).bAlterKills = 2 ) then
        Counter := Player.nLosses_Last
      else
        Counter := Player.nLosses;
      DrawText(OFFSCREEN_Ptr, PAnsiChar(IntToStr(Counter)), ScoreBoardPos.Left + SCOREBOARD_WIDTH-6 - GetStrExtent(PAnsiChar(IntToStr(Counter))) - 2, PlayersDrawListTop + 22, 119, PByte($51E810 + IteratePlayerIdx)^);

      PlayersDrawListTop := PlayersDrawListTop + 40;
// drawings
     { if (PlayerSide <> psArm) and
         (PlayerSide <> psCore) then
      goto IterateWatcher;
// end drawings
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
            if ( PlayerSortType = Player_LocalHuman ) or
               ( PlayerSortType = Player_LocalAI ) or
               ( PlayerSortType = Player_RemotePlayer ) then
            begin
              if ( PlayerSort.cPlayerIndex <> 10 ) and
                 ( (PlayerSort.nNumUnits <> 0) or (PlayerSort.lUnitsCounter = 0) ) then
                if ( PlayerSort.PlayerInfo.PropertyMask and $40 <> $40 ) then
                  if ( PlayerSort.cPlayerScoreboard > cCurActivePlayer ) then
                    PlayerSort.cPlayerScoreboard := PlayerSort.cPlayerScoreboard - 1;
            end;
          end;
          Inc(cCurActiveSortPlayer);
          PlayerSort := TAPlayer.GetPlayerByIndex(cCurActiveSortPlayer);
          Dec(cIterateSort);
        until ( cIterateSort = 0 );
      end;
      Inc(cCurActivePlayer);
      Result := cCurActivePlayer;
    until (cCurActivePlayer >= TAData.ActivePlayersCount);

    // now draw watchers
    for IteratePlayerIdx := 0 to 9 do
    begin
      Player := TAPlayer.GetPlayerByIndex(IteratePlayerIdx);
      if TAPlayer.IsActive(Player) then
//        if ( TAPlayer.IsWatcher(Player) ) then
//        begin
          Result := Result + 1;
//          goto DrawPlayerScoreBoard;
        end;
IterateWatcher :
      Continue;
    end;
  end;
end;

procedure DrawScoreboard;
asm
  push    ebp
  call    DrawNewScoreboard
  push $00494E5D;
  call PatchNJump;
end;
}

procedure BroadcastNanolatheParticles(PosStart: PPosition;
  PosTarget: PNanolathePos; bReverse: Integer); stdcall;
begin
  if TAData.NetworkLayerEnabled then
    GlobalDplay.SendCobEventMessage(TANM_Rec2Rec_SetNanolatheParticles, 0, nil, @PosStart, @bReverse, nil, @PosTarget, nil);
end;

procedure BroadcastNanolatheParticles_BuildingBuild;
asm
  mov     [esp+34h], edx
  mov     [esp+38h], edi
  pushAD
  push    0
  push    eax
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $00403ECC;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_MobileBuild;
asm
  add     edi, [eax+TUnitInfo.lFootPrintY_]
  mov     [esp+44h], edi
  pushAD
  push    0
  push    ecx
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $00402ABD;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_HelpBuild;
asm
  mov     [esp+34h], edx
  mov     [esp+38h], esi
  pushAD
  push    0
  push    eax
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $004041C2;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_Resurrect;
asm
  push    eax
  push    ecx
  mov     [esp+38h], ebp
  pushAD
  push    0
  push    eax
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $0040509D;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_RepairResurrect;
asm
  add     edi, [eax+16Eh]
  mov     [esp+38h], edi
  pushAD
  push    0
  push    ecx
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $004056D1;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_RepairUnk2;
asm
  add     edi, [eax+16Eh]
  mov     [esp+38h], edi
  pushAD
  push    0
  push    ecx
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $004058F6;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_VTOL_MobileBuild;
asm
  push    ecx
  push    edx
  mov     [esp+38h], edi
  pushAD
  push    0
  push    ecx
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $004142B6;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_VTOL_HelpBuild;
asm
  push    ecx
  mov     [esp+38h], edi
  pushAD
  push    0
  push    eax
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $004146D7;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_VTOL_Repair;
asm
  push    ecx
  push    edx
  mov     [esp+38h], edi
  pushAD
  push    0
  push    ecx
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $004151EC;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_Capture;
asm
  push    ecx
  push    edx
  mov     [esp+38h], edi
  pushAD
  push    1
  push    edx 
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $00404676;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_ReclaimUnit;
asm
  push    ecx
  push    edx
  mov     [esp+40h], edi
  pushAD
  push    1
  push    edx
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $00404A4E;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_ReclaimFeature;
asm
  push    eax
  push    ecx
  mov     [esp+38h], edi
  pushAD
  push    1
  push    ecx
  push    eax
  call    BroadcastNanolatheParticles
  popAD
  call    EmitSfx_NanoParticlesReverse
  lea     edx, [esp+14h]
  push    6
  lea     eax, [esp+24h]
  push    edx
  push    eax
  pushAD
  push    1
  push    eax
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $00404D4C;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_VTOLReclaimUnit;
asm
  push    ecx
  push    edx
  mov     [esp+40h], edi
  pushAD
  push    1
  push    edx
  push    ecx
  call    BroadcastNanolatheParticles
  popAD
  push $00414C43;
  call PatchNJump;
end;

procedure BroadcastNanolatheParticles_VTOLReclaimFeature;
asm
  push    eax
  push    ecx
  mov     [esp+34h], edi
  pushAD
  push    1
  push    ecx
  push    eax
  call    BroadcastNanolatheParticles
  popAD
  call    EmitSfx_NanoParticlesReverse
  lea     edx, [esp+10h]
  push    6
  lea     eax, [esp+20h]
  push    edx
  push    eax
  pushAD
  push    1
  push    eax
  push    edx
  call    BroadcastNanolatheParticles
  popAD
  push $00414A31;
  call PatchNJump;
end;

procedure DrawFade(Offscreenp: Cardinal); stdcall;
begin
  if CameraFadeLevel <> 0 then
    DrawTransparentBox(Offscreenp, nil, CameraFadeLevel - 31);
end;

procedure ScreenFadeControl;
asm
  lea     ecx, [esp+224h+OFFSCREEN_off]
  push    ecx
  call    DrawFade
  lea     ecx, [esp+224h+OFFSCREEN_off]
  push    ecx
  push $0046A2E7
  call PatchNJump
end;

function WeaponProjectileFlameStream(Weapon: PWeaponDef): Pointer; stdcall;
begin
  if ExtraGAFAnimations.FlameStream[Weapon.cColor - 1] <> nil then
    Result := ExtraGAFAnimations.FlameStream[Weapon.cColor - 1]
  else
    Result := PTADynMemStruct(TAData.MainStructPtr).flamestream;
end;

{
.text:0049C39E 078 8B BF F3 47 01 00           mov     edi, [edi+TAMainStruct.flamestream]
.text:0049C3A4 078 0F BF 4D 06                 movsx   ecx, word ptr [ebp+6]
 }
procedure WeaponProjectileFlameStreamHook;
label
  CustomFlameStream;
asm
  push    ecx
  mov     cl, byte ptr [ebx+TWeaponDef.cColor]
  test    cl, cl
  pop     ecx
  jnz     CustomFlameStream
  mov     edi, [edi+TTADynMemStruct.flamestream]
  push $0049C3A4
  call PatchNJump
CustomFlameStream :
  push    esi
  push    eax
  push    ebx
  push    ecx
  push    edx

  push    ebx // weapon
  call    WeaponProjectileFlameStream
  mov     edi, eax

  pop     edx
  pop     ecx
  pop     ebx
  pop     eax
  pop     esi

  push $0049C3A4
  call PatchNJump
end;

end.