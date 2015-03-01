unit GUIEnhancements;

interface
uses
  PluginEngine, TA_MemoryStructures, Classes;

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
procedure DrawUnitSelectBox(OFFSCREEN_ptr: Cardinal; p_Unit: PUnitStruct); stdcall;
procedure DrawBuildSpot_InitScript;
procedure DrawBuildSpot_NanoframeHook;
procedure DrawBuildSpot_QueueNanoframeHook;
procedure DrawBuildSpot_NanoframeShimmerHook;
procedure DrawUnitRanges_ShowrangesOnHook;
procedure DrawUnitRanges_CustomRanges;
procedure DrawExplosionsRectExpandHook;
procedure DrawExplosionsRectExpandHook2;
procedure LoadArmCore32ltGafSequences;
procedure DrawScoreboard(p_Offscreen: Cardinal); stdcall;
procedure BroadcastNanolatheParticles_BuildingBuild;
procedure BroadcastNanolatheParticles_MobileBuild;
procedure BroadcastNanolatheParticles_HelpBuild;
procedure BroadcastNanolatheParticles_Resurrect;
procedure BroadcastNanolatheParticles_RepairResurrect;
//procedure BroadcastNanolatheParticles_RepairUnk2;
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
procedure DontRadarSonarJammAllies;

implementation
uses
  Windows,
  IniOptions,
  SysUtils,
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_MemPlayers,
  TA_MemUnits,
  ExtensionsMem,
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
  SCOREBOARD_WIDTH = 180;

const
  NanoUnitCreateInit : AnsiString = 'NanoFrameInit';
  Core32Lt : AnsiString = 'Core32Dk';
  Arm32Lt : AnsiString = 'Arm32Dk';
  RaceLogo : AnsiString = 'RaceLogo';

var
  GUIEnhancementsPlugin: TPluginData;

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

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'TrueIncomeHook',
                            @TrueIncomeHook,
                            $004695EE, 0);

    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                            'HealthPercentage',
                            @HealthPercentage,
                            $0046B088, 1);

    GUIEnhancementsPlugin.MakeStaticCall(State_GUIEnhancements,
                          'Draw unit selection box',
                          @DrawUnitSelectBox,
                          $00469B8A);
    GUIEnhancementsPlugin.MakeStaticCall(State_GUIEnhancements,
                          'Draw unit selection box 2',
                          @DrawUnitSelectBox,
                          $004699EB);

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

    if IniSettings.DrawBuildSpotQueueNano then
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              'DrawBuildSpot_QueueNanoframeHook',
                              @DrawBuildSpot_QueueNanoframeHook,
                              $00438C38, 0);

    if not IniSettings.BuildSpotNanoShimmer then
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

    if IniSettings.ExplosionsGameUIExpand > 0 then
    begin
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @DrawExplosionsRectExpandHook,
                              $00420C47, 0);
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              '',
                              @DrawExplosionsRectExpandHook2,
                              $00420BA2, 0);
    end;

    // ---------------------------------
    // new score board
    // ---------------------------------

    if IniSettings.ScoreBoard then
    begin
      GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                              'load core and arm logos colored',
                              @LoadArmCore32ltGafSequences,
                              $0043190B, 1);

      GUIEnhancementsPlugin.MakeStaticCall(State_GUIEnhancements,
                              'Draw score board with ping and side logos',
                              @DrawScoreboard,
                              $00469F65);
    end;

    // ---------------------------------
    // nanolathe particles broadcast
    // ---------------------------------

    if IniSettings.BroadcastNanolathe then
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
                                   {
    GUIEnhancementsPlugin.MakeRelativeJmp(State_GUIEnhancements,
                           'DontRadarSonarJammAllies',
                           @DontRadarSonarJammAllies,
                           $00467608, 5 );
                                   }
    Result:= GUIEnhancementsPlugin;
  end else
    Result := nil;
end;

function DrawUnitState(p_Offscreen: Cardinal; Unit_p: PUnitStruct;
  CenterPosX : Integer; CenterPosZ: Integer) : Integer; stdcall;
var
  { drawing }
  ColorsPal : Pointer;
  FontColor : LongInt;
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
  ActionUnit : PUnitStruct;
  ActionUnitBuildTime : Cardinal;
  ActionUnitBuildCost1, ActionUnitBuildCost2 : Single;
  ActionTime : Double;
  ActionWorkTime : Word;
  MaxActionVal : Integer;
  CurActionVal : Integer;
  ReclaimColor : Byte;
begin
  Result := 0;
  BottomZ := 0;
  // is drawing health bars enabled and any of local player units are actually on screen
  if ((TAData.MainStruct.GameOptionMask and 1) = 1) or
     (Unit_p.HotKeyGroup <> 0) then
  begin
    UnitInfo := Unit_p.p_UnitInfo;
    ColorsPal := Pointer(LongWord(TAData.MainStruct)+$DCB);
    
    if ((TAData.MainStruct.GameOptionMask and 1) = 1) and
       (CenterPosX <> 0) and
       (CenterPosZ <> 0) and
       (UnitInfo <> nil) then
    begin
      //AlliedUnit := TAPlayer.GetAlliedState(TAUnit.GetOwnerPtr(Unit_p), TAData.ViewPlayer);
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
             not ExtraUnitInfoTags[UnitInfo.nCategory].HideHPBar then
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
           (ExtraUnitInfoTags[UnitInfo.nCategory].UseCustomReloadBar)) and
           (UnitBuildTimeLeft = 0.0) then
        begin
          MaxReloadTime := 0;
          CurReloadTime := 0;
          StockPile := 0;
          CustomReloadBar := False;
          if ExtraUnitInfoTags[UnitInfo.nCategory].UseCustomReloadBar then
          begin
            if CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].CustomWeapReload then
            begin
              MaxReloadTime := CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].CustomWeapReloadMax;
              CurReloadTime := CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].CustomWeapReloadCur;
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

          if CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].TeleportReloadMax <> 0 then
          begin
            MaxReloadTime := CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].TeleportReloadMax;
            CurReloadTime := CustomUnitFieldsArr[TAUnit.GetId(Unit_p)].TeleportReloadCur;
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
               IniSettings.Stockpile and
               not CustomReloadBar then
            begin
              if Unit_p.p_SubOrder <> nil then
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
                   0..15 : result := DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor)^);
                  16..30 : result := DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 1)^);
                  31..47 : result := DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 2)^);
                  48..64 : result := DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 3)^);
                  65..80 : result := DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 4)^);
                  81..94 : result := DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 5)^);
                 95..100 : result := DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+WeapReloadColor - 6)^);
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
                FeatureDefID := GetFeatureTypeOfPos(@PUnitOrder(Unit_p.p_MainOrder).Pos, Unit_p.p_MainOrder, nil);
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
                    ActionUnit := TAUnit.GetCurrentOrderTargetUnit(Unit_p);
                    if ActionUnit <> nil then
                    begin
                      ActionUnitType := TAUnit.GetUnitInfoId(ActionUnit);
                      if ActionUnitType <> 0 then
                      begin
                        ActionUnitBuildCost1 := PUnitInfo(TAMem.UnitInfoId2Ptr(ActionUnitType)).lBuildCostEnergy * 30 * 0.00050000002;
                        ActionUnitBuildCost2 := PUnitInfo(TAMem.UnitInfoId2Ptr(ActionUnitType)).lBuildCostMetal * 30 * -0.0071428572;
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
                      ActionUnitBuildTime := PUnitInfo(TAMem.UnitInfoId2Ptr(ActionUnitType)).lBuildTime;
                      ActionWorkTime := PUnitInfo(Unit_p.p_UnitInfo).nWorkerTime div 30;
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
                    0..20 : result := DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor)^);
                   21..40 : result := DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor + 1)^);
                   41..60 : result := DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor + 2)^);
                   61..80 : result := DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor + 3)^);
                  81..100 : result := DrawBar(p_Offscreen, @RectDrawPos, PByte(LongWord(ColorsPal)+ReclaimColor + 4)^);
                  end;
                end;
              end;
            end;
          end;
        end;

        // built weapons counter (nukes)
        if IniSettings.Stockpile then
          if Unit_p.UnitWeapons[0].cStock > 0 then
            DrawText_Heavy(p_Offscreen, PAnsiChar(IntToStr(Unit_p.UnitWeapons[0].cStock)), Word(CenterPosX), Word(CenterPosZ) - 13, -1);

        // transporter count
        if IniSettings.Transporters then
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
              if LongWord(PUnitStruct(TAUnit.Id2Ptr(UnitId)).pTransporterUnit) = LongWord(Unit_p) then
                Inc(TransportCount);
            end;     }
            TransportCount := TAUnit.GetLoadCurAmount(Unit_p);
            if TransportCount > 0 then
              DrawText_Heavy(p_Offscreen, PAnsiChar(IntToStr(TransportCount) + '/' + IntToStr(TransportCap)), Word(CenterPosX) - 10, Word(CenterPosZ) - 70, -1);
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
                FontColor := GetFontColor;
                if WeightTransportPercent > 100 then
                  WeightTransportPercent := 100;
                if TransportCount = TransportCap then
                  WeightTransportPercent := 100;
                if (WeightTransportPercent > 85) then
                  SetFontColor(PByte(LongWord(ColorsPal)+18)^, FontColor)
                else
                  if (WeightTransportPercent > 50) then
                    SetFontColor(PByte(LongWord(ColorsPal)+17)^, FontColor)
                  else
                    SetFontColor(PByte(LongWord(ColorsPal)+16)^, FontColor);

                DrawText_Heavy(p_Offscreen,
                               PAnsiChar(IntToStr(WeightTransportPercent) + '%'),
                               Word(CenterPosX) - 10, Word(CenterPosZ) - 70, -1);
                SetFontColor(PByte(LongWord(ColorsPal)+255)^, FontColor);
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
          DrawText_Heavy(p_Offscreen, @sGroup, Word(CenterPosX), Word(CenterPosZ) + 3 + BottomZ, -1);
        end;
      end;
    end; { Bars enabled }
  end;
end;

procedure DrawTrueIncome(OFFSCREEN_ptr: Cardinal; RaceSideData: PRaceSideData;
  ViewResBar: PViewResBar); stdcall;
var
  ResourceIncome : Single;
  PlayerResourceString : PAnsiChar;
  ColorsPal : Pointer;
  FontColor : LongInt;
  FormatSettings: TFormatSettings;
  SideIndex: Integer;
begin
  SideIndex := RaceSideData.lSideIdx;
  if SideIndex <= High(ExtraSideData) then
  begin
    ColorsPal := Pointer(LongWord(TAData.MainStruct)+$DCB);
    FormatSettings.DecimalSeparator := '.';
    FontColor := GetFontColor;

    if ExtraSideData[SideIndex].rectRealEIncome.Left <> 0 then
    begin
      ResourceIncome := ViewResBar.fEnergyProduction - ViewResBar.fEnergyExpense;
      if ResourceIncome >= 0.0 then
      begin
        SetFontColor(PByte(LongWord(ColorsPal)+10)^, FontColor);
        if ResourceIncome < 10000 then
          PlayerResourceString := PAnsiChar(Format('+%.0f', [ResourceIncome], FormatSettings))
        else
        begin
          ResourceIncome := ResourceIncome / 1000;
          PlayerResourceString := PAnsiChar(Format('+%.0fK', [ResourceIncome], FormatSettings));
        end;
      end else
      begin
        SetFontColor(PByte(LongWord(ColorsPal)+12)^, FontColor);
        if ResourceIncome > -10000 then
          PlayerResourceString := PAnsiChar(Format('%.0f', [ResourceIncome], FormatSettings))
        else
        begin
          ResourceIncome := ResourceIncome / 1000;
          PlayerResourceString := PAnsiChar(Format('%.0fK', [ResourceIncome], FormatSettings));
        end;
      end;
      DrawText_Heavy(OFFSCREEN_ptr, PlayerResourceString,
        ExtraSideData[SideIndex].rectRealEIncome.Left,
        ExtraSideData[SideIndex].rectRealEIncome.Top, -1);
    end;

    if ExtraSideData[SideIndex].rectRealMIncome.Left <> 0 then
    begin
      ResourceIncome := ViewResBar.fMetalProduction - ViewResBar.fMetalExpense;
      if ResourceIncome >= 0.0 then
      begin
        SetFontColor(PByte(LongWord(ColorsPal)+10)^, FontColor);
        if ResourceIncome < 10000 then
          PlayerResourceString := PAnsiChar(Format('+%.1f', [ResourceIncome], FormatSettings))
        else
        begin
          ResourceIncome := ResourceIncome / 1000;
          PlayerResourceString := PAnsiChar(Format('+%.1fK', [ResourceIncome], FormatSettings));
        end;
      end else
      begin
        SetFontColor(PByte(LongWord(ColorsPal)+12)^, FontColor);
        if ResourceIncome > -10000 then
          PlayerResourceString := PAnsiChar(Format('%.1f', [ResourceIncome], FormatSettings))
        else
        begin
          ResourceIncome := ResourceIncome / 1000;
          PlayerResourceString := PAnsiChar(Format('%.1fK', [ResourceIncome], FormatSettings));
        end;
      end;
      DrawText_Heavy(OFFSCREEN_ptr, PlayerResourceString,
        ExtraSideData[SideIndex].rectRealMIncome.Left,
        ExtraSideData[SideIndex].rectRealMIncome.Top, -1);
    end;
  end;
end;

procedure DrawDevUnitStateProbes(p_Offscreen : Cardinal); stdcall;
begin
  //DrawTranspRectangle(p_Offscreen, @Rect, PByte(LongWord(ColorsPal)+10)^);
  if TAData.DevMode then
  begin
    if TAData.MainStruct.field_391B3 <> 0 then
      UnitStateProbe(p_Offscreen);
    if TAData.MainStruct.field_391B9 <> 0 then
      UnitBuilderProbe(p_Offscreen);
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
  push    ecx             // ViewResBar
  push    edx             // RaceSideData
  lea     ecx, [esp+22Ch+OFFSCREEN_off]
  push    ecx             // OFFSCREEN_ptr
  call    DrawTrueIncome
  push $00469610;
  call PatchNJump;
end;

procedure DrawHealthPercentage(p_Offscreen: Pointer; p_Unit: PUnitStruct;
  SideData: PRaceSideData; CurrentHP, MaxHP: Cardinal; Yoffset: Integer); stdcall;
var
  HealthPercent : Single;
  HealthPercentStr : PAnsiChar;
  FontColor : LongInt;
  FormatSettings: TFormatSettings;
  GafFrame: Pointer;
begin
  if (MaxHP = 0) or (CurrentHP > MaxHP) then
    Exit;

  if SideData.lSideIdx <= High(ExtraSideData) then
  begin
    if CustomUnitFieldsArr[TAUnit.GetId(p_Unit)].ShieldedBy <> nil then
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
        FontColor := GetFontColor;
        SetFontColor(83, FontColor);
        FormatSettings.DecimalSeparator := '.';
        HealthPercentStr := PAnsiChar(Format('%.1f%%', [HealthPercent], FormatSettings));
        DrawText_Heavy(Cardinal(p_Offscreen), HealthPercentStr,
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

procedure PrepareNanoUnit(p_Unit: PUnitStruct);
var
  UnitInfo : PUnitInfo;
begin
  UnitInfo := p_Unit.p_UnitInfo;
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
  mov     ebx, dword ptr [esi+TUnitStruct.lFireTimePlus600]
  test    ebx, ebx
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

  if (TAData.MainStruct.nBuildNum <> 0) and
     (TAData.MainStruct.ucPrepareOrderType = $E) then
  begin
    NanoSpotUnitSt.nUnitInfoID := TAData.MainStruct.nBuildNum;
    NanoSpotUnitSt.p_UnitInfo := TAMem.UnitInfoId2Ptr(NanoSpotUnitSt.nUnitInfoID);
    if (ExtraUnitInfoTags[PUnitInfo(NanoSpotUnitSt.p_UnitInfo).nCategory].DrawBuildSpotNanoFrame = True) or
       IniSettings.ForceDrawBuildSpotNano then
    begin
      X := TAData.MainStruct.lBuildPosRealX;
      Z := TAData.MainStruct.lBuildPosRealY;
      if (X < 0) or (Z < 0) then
        Exit;
      GetTPosition(X, Z, Position);
      NanoSpotUnitSt.p_Owner := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);
      if UNITS_AllocateUnit(@NanoSpotUnitSt, X, Position.Y, Z, 0) then
      begin
        NanoSpotUnitInfoSt := TAMem.UnitInfoId2Ptr(NanoSpotUnitSt.nUnitInfoID)^;
        NanoSpotUnitSt.p_UnitInfo := @NanoSpotUnitInfoSt;

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

        if (TAData.MainStruct.cBuildSpotState and $40 = $40) then
          NanoSpotUnitSt.lFireTimePlus600 := 1
        else
          NanoSpotUnitSt.lFireTimePlus600 := 2;

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

procedure DrawBuildSpotQueueNanoframe(pOffscreen: Cardinal;
  UnitInfo: PUnitInfo; UnitOrder: PUnitOrder); stdcall;
var
  Position: TPosition;
begin
  // draw only queued (and not under construction already)
  if UnitOrder.ucState <= 1 then
  begin
    if NanoSpotQueueUnitSt.nUnitInfoID <> 0 then
      FreeUnitMem(@NanoSpotQueueUnitSt);

    NanoSpotQueueUnitSt.nUnitInfoID := UnitInfo.nCategory;
    NanoSpotQueueUnitSt.p_UnitInfo := TAMem.UnitInfoId2Ptr(NanoSpotQueueUnitSt.nUnitInfoID);
    GetTPosition(UnitOrder.Pos.X, UnitOrder.Pos.Z, Position);

    NanoSpotQueueUnitSt.p_Owner := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);
    if UNITS_AllocateUnit(@NanoSpotQueueUnitSt, UnitOrder.Pos.X, Position.Y, UnitOrder.Pos.Z, 0) then
    begin
      if (UnitInPlayerLOS(TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID), @NanoSpotQueueUnitSt) <> 0) and
         TAUnit.IsInGameUI(@NanoSpotQueueUnitSt) then
      begin
        NanoSpotQueueUnitInfoSt := TAMem.UnitInfoId2Ptr(NanoSpotQueueUnitSt.nUnitInfoID)^;
        NanoSpotQueueUnitSt.p_UnitInfo := @NanoSpotQueueUnitInfoSt;

        NanoSpotQueueUnitSt.Position.X := NanoSpotQueueUnitSt.Position.x_;
        NanoSpotQueueUnitSt.Position.x_ := 0;
        NanoSpotQueueUnitSt.Position.Z := NanoSpotQueueUnitSt.Position.z_;
        NanoSpotQueueUnitSt.Position.z_ := 0;

        if NanoSpotQueueUnitInfoSt.cBMCode = 1 then
        begin
          UNITS_AllocateMovementClass(@NanoSpotQueueUnitSt);
        end;

        NanoSpotQueueUnitSt.Turn.Z := 32768;
        NanoSpotQueueUnitSt.Position.y_ := 0;
        UNITS_FixYPos(@NanoSpotQueueUnitSt);
        NanoSpotQueueUnitSt.Position.Y := Position.Y;
        if NanoSpotQueueUnitInfoSt.nMinWaterDepth <> -10000 then
          if NanoSpotQueueUnitInfoSt.cWaterLine = 0 then
            NanoSpotQueueUnitSt.Position.Y := 0;

        if ((PUnitStruct(UnitOrder.p_Unit).lUnitStateMask shr 4) and 1 = 1) then
          NanoSpotQueueUnitSt.lFireTimePlus600 := 1
        else
          NanoSpotQueueUnitSt.lFireTimePlus600 := 2;

        if UNITS_CreateModelScripts(@NanoSpotQueueUnitSt) <> nil then
        begin
          PrepareNanoUnit(@NanoSpotQueueUnitSt);
          DrawUnit(pOffscreen, @NanoSpotQueueUnitSt);
        end;
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
  mov     ebx, dword ptr [edx+TUnitStruct.lFireTimePlus600]
  test    ebx, ebx
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
  UnitInfo := UnitOrder.p_Unit.p_UnitInfo;
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

procedure DrawScoreboard(p_Offscreen: Cardinal); stdcall;
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
label
  SortPlayers;
begin
  if ( PCardinal($51F2F4)^ < GameRunSec ) then
  begin
    PCardinal($51F2F4)^ := GameRunSec + 1;
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
    ScoreBoardWidth := 140;

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
  ScoreBoardPos.Bottom := 40 * TAData.MainStruct.nActivePlayersCount + 46;

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
      PlayersDrawListTop := PlayersDrawListTop + 40;
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
            PlayersDrawListTop := PlayersDrawListTop + 40;
          end;
        end;
      end;
    end;
  end;
end;

procedure BroadcastNanolatheParticles(PosStart: PPosition;
  PosTarget: PNanolathePos; Reverse: Integer); stdcall;
begin
  if TAData.NetworkLayerEnabled then
    GlobalDPlay.Broadcast_SetNanolatheParticles(PosStart^, PosTarget^, Reverse);
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
  add     edi, [eax+TUnitInfo.nFootPrintY_]
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
  if ExtraGAFAnimations.FlameStream[Weapon.ucColor - 1] <> nil then
    Result := ExtraGAFAnimations.FlameStream[Weapon.ucColor - 1]
  else
    Result := TAData.MainStruct.flamestream;
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
  mov     cl, byte ptr [ebx+TWeaponDef.ucColor]
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

procedure DontRadarSonarJammAllies;
label
  NextUnit,
  NotARadarJammer;
asm
  xor     ecx, ecx
  mov     cl, [esi+6Dh]                         // unit owner index
  mov     al, byte ptr [ebp+TPlayerStruct.cAllyFlagArray[ecx]]
  test    al, al
  jnz     NextUnit
  mov     eax, [esi]
  cmp     [eax+TUnitInfo.nRadarDistanceJam], 0
  jz      NotARadarJammer
  push $00467614
  call PatchNJump
NotARadarJammer :
  push $00467631
  call PatchNJump
NextUnit :
  push $0046765A
  call PatchNJump
end;

procedure DrawUnitSelectBox(OFFSCREEN_ptr: Cardinal; p_Unit: PUnitStruct); stdcall;
var
  x, z, y: Integer;
  CenterX, CenterZ: Integer;
  ColorsPal: Cardinal;
  Radius: Integer;
  InMove: Boolean;
  Speed, GameTime: Integer;
  CurOrder: TTAActionType;
  AnimDelay: Integer;
  IsMobileUnit: Boolean;
begin
  IsMobileUnit := TAMem.UnitInfoId2Ptr(p_Unit.nUnitInfoID).cBMCode = 1;
  if ((IniSettings.UnitSelectBoxType = 1) and not IsMobileUnit) then
    DrawUnitSelectBoxRect(OFFSCREEN_ptr, p_Unit)
  else
    if (ExtraUnitInfoTags[p_Unit.nUnitInfoID].SelectBoxType = 1) or
       ((IniSettings.UnitSelectBoxType = 1) and IsMobileUnit) or
       (IniSettings.UnitSelectBoxType = 2) then
    begin
      x := p_Unit.Position.x - TAData.MainStruct.lEyeBallMapX;
      z := p_Unit.Position.z - TAData.MainStruct.lEyeBallMapY;
      y := p_Unit.Position.y;
      CenterX := x + 128;
      CenterZ := z - (y shr 1) + 32;
      Radius := 1 + p_Unit.p_UnitInfo.lWidthHypot shr 16;

      InMove := False;
      if p_unit.p_MovementClass <> nil then
      begin
        Speed := TAUnit.GetCurrentSpeedPercent(p_Unit);
        Radius := Round(Radius + (Radius * Speed) / 500);
        if Speed > 0 then
          InMove := True;
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
        AnimDelay := 20
      else
        AnimDelay := 40;
      GameTime := TAData.GameTime mod AnimDelay;
      if GameTime > (AnimDelay div 2) then
        AnimDelay := 1
      else
        AnimDelay := 0;

      ColorsPal := LongWord(TAData.MainStruct)+$DCB;
      DrawDotteCircle(OFFSCREEN_ptr, CenterX, CenterZ, Radius,
        PByte(ColorsPal+GetRaceSpecificColor(0))^, Radius, AnimDelay);
    end else
      if (ExtraUnitInfoTags[p_Unit.nUnitInfoID].SelectBoxType = 0) then
        DrawUnitSelectBoxRect(OFFSCREEN_ptr, p_Unit);
end;

end.
