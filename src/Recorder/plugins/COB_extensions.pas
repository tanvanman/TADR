unit COB_extensions;

interface
uses
  {$IFDEF DEBUG}SysUtils, logging, uDebug,{$ENDIF}
  PluginEngine, SynCommons, TA_MemoryStructures;

// -----------------------------------------------------------------------------

const
  State_COB_extensions: Boolean = True;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallCobExtensions;
Procedure OnUninstallCobExtensions;

const
  WEAPON_AIM_ABORTED = 21;
  WEAPON_READY = 22;
  CURRENT_SPEED = 29;
  // gets kills * 100
  VETERAN_LEVEL = 32;
  { IDs, owner etc. }
  // returns the lowest valid unit ID number
  MIN_ID = 69;
  // returns the highest valid unit ID number
  MAX_ID = 70;
  // returns ID of current unit
  MY_ID = 71;
  // returns player id of unit given with parameter
  UNIT_TEAM = 72;
  // basically BUILD_PERCENT_LEFT, but comes with a unit parameter
  UNIT_BUILD_PERCENT_LEFT = 73;
  // is unit given with parameter allied to the unit of the current COB script. 1=allied, 0=not allied
  UNIT_ALLIED = 74;
  // indicates if the 1st parameter(a unit ID) is local to this computer
  UNIT_IS_ON_THIS_COMP = 75;
  UNIT_ALLIED_WITH_LOCAL = 76;
  UNIT_TYPE_CRC = 77;
  UNIT_TYPE_CRC_TO_ID = 78;
  UNIT_TYPE_ID_TO_CRC = 79;
  UNIT_TYPE_IN_CATEGORY = 80;
  PRIOR_UNIT = 81;
  TRANSPORTED_BY = 82;
  TRANSPORTING = 83;

  { Players }
  PLAYER_ACTIVE = 90;
  PLAYER_TYPE = 91;
  PLAYER_SIDE = 92;
  PLAYER_KILLS = 93;
  PLAYER_ECONOMY = 94;
  UNIT_IN_PLAYER_LOS = 95;
  POSITION_IN_PLAYER_LOS = 96;

  { Some specific }
  UNITX = 100;
  UNITZ = 101;
  UNITY = 102;
  TURNX = 103;
  TURNZ = 104;
  TURNY = 105;
  UNIT_GRID_XZ = 106;
  HEALTH_VAL = 107;
  MAKE_DAMAGE = 108;
  HEAL_UNIT = 109;
  GET_CLOAKED = 110;
  SET_CLOAKED = 111;
  UNIT_BAS_STATE_MASK = 112;
  UNIT_STATE_MASK = 113;
  SELECTABLE = 114;
  ATTACH_UNIT = 115;
  RANDOM_FREE_PIECE = 116;
  CUSTOM_BAR_PROGRESS = 117;
  MEX_RATIO = 118;

  { Weapons, attack info }
  WEAPON_PRIMARY = 130;
  WEAPON_SECONDARY = 131;
  WEAPON_TERTIARY = 132;
  UNIT_KILLS = 133;
  ATTACKER_ID = 134;
  LOCKED_TARGET_ID = 135;
  UNDER_ATTACK = 136;
  FIRE_WEAPON = 137;
  WEAPON_BUILD_PROGRESS = 138;

  { Creating and killing }
  GIVE_UNIT = 150;
  CREATE_UNIT = 151;
  KILL_THIS_UNIT = 152;
  KILL_OTHER_UNIT = 153;
  CREATE_MINIONS = 154;
  SWAP_UNIT_TYPE = 155;

  { Searching for units }
  UNITS_NEAR = 170;
  UNITS_YARDMAP = 171;
  UNITS_WHOLEMAP = 172;
  UNITS_ARRAY_RESULT = 173;
  RANDOMIZE_UNITS_ARRAY = 174;
  FREE_ARRAY_ID = 175;
  CLEAR_ARRAY_ID = 176;
  UNIT_NEAREST = 177;
  DISTANCE = 178;

  { Unit orders }
  CURRENT_ORDER_ABORT = 190;
  CURRENT_ORDER_TYPE = 191;
  CURRENT_ORDER_TARGET_POS = 192;
  CURRENT_ORDER_TARGET_ID = 193;
  CURRENT_ORDER_PAR = 194;
  CURRENT_ORDER_PAR_EDIT = 195;
  ORDER_SELF = 196;
  ORDER_SELF_POS = 197;
  ORDER_UNIT_UNIT = 198;
  ORDER_UNIT_POS = 199;
  ORDER_SELF_UNIT_POS = 200;
  RESET_ORDER = 201;
  ADD_BUILD = 202;

  { Global unit template }
  GRANT_UNITINFO = 220;
  GET_UNITINFO = 221;
  SET_UNITINFO = 222;
  UNIT_TYPE_LIMIT = 223;
  GUI_INDEX = 224;
  MOBILE_PLANT = 225;

  { Sfx }
  UNIT_SPEECH = 240;
  PLAY_3D_SOUND = 241;
  PLAY_GAF_ANIM = 242;
  EMIT_SFX = 243;

  { Other }
  COB_QUERY_SCRIPT = 250;
  COB_START_SCRIPT = 251;
  LOCAL_SHARED_DATA = 252;

  { Map }
  MAP_SEA_LEVEL = 270;
  IS_LAVA_MAP = 271;
  SURFACE_METAL = 272;
  UNIT_AT_POSITION = 273;
  TEST_UNLOAD_POS = 274;
  TEST_BUILD_SPOT = 275;
  PLANT_YARD_OCCUPIED = 276;
  UNIT_REBUILD_YARD = 277;
  FEATURE_TYPE_AT_POS = 278;
  FEATURE_INFO = 279;
  GRID_INFO = 280;

  { Map missions }
  MS_MOVE_CAM_POS = 300;
  MS_LOCK_CAM_TO_UNIT = 301;
  MS_SHAKE = 302;
  MS_SCREEN_GAMMA = 303;
  MS_SCREEN_FADE = 304;
  MS_DRAW_MOUSE = 305;
  MS_DESELECT_UNITS = 306;
  MS_PLAY_SOUND_2D = 307;
  MS_PLAY_SOUND_3D = 308;
  MS_EMIT_SMOKE = 309;
  MS_PLACE_FEATURE = 310;
  MS_REMOVE_FEATURE = 311;
  MS_SWAP_TERRAIN = 312;
  MS_VIEW_PLAYER_ID = 313;
  MS_GAME_TIME = 314;
  MS_APPLY_UNIT_SCRIPT = 315;
  MS_FIRE_MAP_WEAPON = 316;
  MS_SHOW_TEXTMSG = 317;
  MS_AI_DIFFICULTY = 318;

  { Math }
  LOWWORD = 370;
  HIGHWORD = 371;
  MAKEDWORD = 372;

  DBG_OUTPUT = 400;

  CUSTOM_LOW = WEAPON_AIM_ABORTED;
  CUSTOM_HIGH = DBG_OUTPUT;

// -----------------------------------------------------------------------------

var
  UnitSearchArr: TUnitSearchArr;
  SpawnedMinionsArr: TSpawnedMinionsArr;
  UnitSearchResults,
  SpawnedMinions: TDynArray;
  UnitSearchCount,
  SpawnedMinionsCount: Integer;

// -----------------------------------------------------------------------------

Procedure COB_Extensions_Handling;
Procedure COB_ExtensionsSetters_Handling;

implementation
uses
  idplay,
  Windows,
  TA_MemoryLocations,
  TA_MemPlayers,
  TA_MemUnits,
  TA_MemPlotData,
  TA_FunctionsU,
  MapExtensions,
  IniOptions;

Procedure OnInstallCobExtensions;
begin
end;

Procedure OnUninstallCobExtensions;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_COB_extensions then
  begin
    result := TPluginData.create( State_COB_extensions,
                                  'COB Extensions',
                                  State_COB_extensions,
                                  @OnInstallCobExtensions, @OnUnInstallCobExtensions );

    result.MakeRelativeJmp( State_COB_extensions,
                            'COB Extensions handler',
                            @COB_Extensions_Handling,
                            $00480776,
                            2 );

    result.MakeRelativeJmp( State_COB_extensions,
                            'COB Extensions Setters handler',
                            @COB_ExtensionsSetters_Handling,
                            $00480B26,
                            1 );
  end else
    Result := nil;
end;

function CustomGetters(index: Cardinal; p_Unit: PUnitStruct;
  arg1, arg2, arg3, arg4: Integer): Integer; stdcall;
var
  UnitInfoSt: PUnitInfo;
  Position: TPosition;
  Turn: TTurn;
  GridPlot: PPlotGrid;
  b: Byte;
  i: Integer;
  ExtensionsNotForDemos: Boolean;
  UnitID: Word;
  {$IFDEF DEBUG}
  ErrorAddress, ErrorAddress2: Cardinal;
  s: String;
  {$ENDIF}
begin
  Result := 0;
{$IFDEF DEBUG}
try
{$ENDIF}
  if TAData.NetworkLayerEnabled then
    ExtensionsNotForDemos := GlobalDPlay.NotViewingRecording
  else
    ExtensionsNotForDemos := True;
  if (index <= TRANSPORTING) then
  begin
    case index of
      CURRENT_SPEED :
        begin
        if arg1 <> 0 then
          result := TAUnit.GetCurrentSpeedVal(TAUnit.Id2Ptr(arg1))
        else
          result := TAUnit.GetCurrentSpeedVal(p_Unit);
        end;
      VETERAN_LEVEL :
        begin
        Result := p_Unit.nKills * 100;
        end;
      MIN_ID :
        begin
        result := 1;
        end;
      MAX_ID :
        begin
        result := TAData.MaxUnitsID;
        end;
      MY_ID :
        begin
        result := TAUnit.GetId(p_Unit);
        end;
      UNIT_TEAM :
        begin
        result := TAUnit.GetOwnerIndex(TAUnit.Id2Ptr(arg1));
        end;
      UNIT_BUILD_PERCENT_LEFT :
        begin
        if arg1 <> 0 then
          result := TAUnit.GetBuildPercentLeft(TAUnit.Id2Ptr(arg1))
        else
          result := TAUnit.GetBuildPercentLeft(p_Unit);
        end;
      UNIT_ALLIED :
        begin
        result := TAUnit.IsAllied(p_Unit, arg1);
        end;
      UNIT_IS_ON_THIS_COMP :
        begin
        if arg1 <> 0 then
          result := BoolValues[TAUnit.IsOnThisComp(TAUnit.Id2Ptr(arg1), True)]
        else
          result := BoolValues[TAUnit.IsOnThisComp(p_Unit, True)];
        end;
      UNIT_ALLIED_WITH_LOCAL :
        begin
        result := BoolValues[TAPlayer.GetAlliedState(TAUnit.GetOwnerPtr(TAUnit.Id2Ptr(arg1)),
          TAData.LocalPlayerID)];
        end;
      UNIT_TYPE_CRC :
        begin
        if arg1 <> 0 then
          result := TAUnit.GetUnitInfoCrc(TAUnit.Id2Ptr(arg1))
        else
          result := TAUnit.GetUnitInfoCrc(p_Unit);
        end;
      UNIT_TYPE_CRC_TO_ID :
        begin
        if TAMem.UnitInfoCrc2Ptr(arg1) <> nil then
          result := TAMem.UnitInfoCrc2Ptr(arg1).nCategory;
        end;
      UNIT_TYPE_ID_TO_CRC :
        begin
        result := TAMem.UnitInfoId2Ptr(arg1).CRC_FBI;
        end;
      UNIT_TYPE_IN_CATEGORY :
        begin
        result := BoolValues[(TAUnit.IsUnitTypeInCategory(TUnitCategories(arg1),
          p_Unit.p_UNITINFO, TAMem.UnitInfoCrc2Ptr(arg2)))];
        end;
      PRIOR_UNIT :
        begin
        if arg1 <> 0 then
          result := TAUnit.GetId(TAUnit.Id2Ptr(arg1).p_PriorUnit)
        else
          result := TAUnit.GetId(p_Unit.p_PriorUnit);
        end;
      TRANSPORTED_BY :
        begin
        if arg1 <> 0 then
          result := TAUnit.GetId(TAUnit.GetTransporterUnit(TAUnit.Id2Ptr(arg1)))
        else
          result := TAUnit.GetId(TAUnit.GetTransporterUnit(p_Unit));
        end;
      TRANSPORTING :
        begin
        if arg1 <> 0 then
          result := TAUnit.GetId(TAUnit.GetTransportingUnit(TAUnit.Id2Ptr(arg1)))
        else
          result := TAUnit.GetId(TAunit.GetTransportingUnit(p_Unit));
        end;
    end;
    Exit;
  end;
  if (index >= PLAYER_ACTIVE) and (index <= POSITION_IN_PLAYER_LOS) then
  begin
    case index of
      PLAYER_ACTIVE :
        begin
        result := BoolValues[TAPlayer.IsActive(TAPlayer.GetPlayerByIndex(arg1))];
        end;
      PLAYER_TYPE :
        begin
        result := Ord(TAPlayer.PlayerController(TAPlayer.GetPlayerByIndex(arg1)));
        end;
      PLAYER_SIDE :
        begin
        result := TAPlayer.PlayerSideIdx(TAPlayer.GetPlayerByIndex(arg1));
        end;
      PLAYER_KILLS :
        begin
        result := PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).nKills;
        end;
      PLAYER_ECONOMY :
        begin
        case arg2 of
          1 : Result := Round(PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).Resources.fCurrentEnergy);
          2 : Result := Round(PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).Resources.fCurrentMetal);
          3 : Result := Round(PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).Resources.fEnergyProduction);
          4 : Result := Round(PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).Resources.fMetalProduction);
          5 : Result := Round(PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).Resources.fEnergyStorageMax);
          6 : Result := Round(PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).Resources.fMetalStorageMax);
        end;
        end;
      UNIT_IN_PLAYER_LOS :
        begin
        if arg1 <> 0 then
          if TAUnit.Id2Ptr(arg1).p_Owner <> nil then
            result := UnitInPlayerLOS(TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID), TAUnit.Id2Ptr(arg1));
        end;
      POSITION_IN_PLAYER_LOS :
        begin
        if GetTPosition(HiWord(Cardinal(arg2)), LoWord(Cardinal(arg2)), Position) <> nil then
          result := BoolValues[TAMap.PositionInLOS(TAPlayer.GetPlayerByIndex(arg1), @Position)];
        end;
    end;
    Exit;    
  end;
  if (index >= UNITX) and (index <= MEX_RATIO) then
  begin
    case index of
      UNITX :
        begin
        if arg1 <> 0 then
          result := TAUnit.GetUnitX(TAUnit.Id2Ptr(arg1))
        else
          result := TAUnit.GetUnitX(p_Unit);
        end;
      UNITZ:
        begin
        if arg1 <> 0 then
          result := TAUnit.GetUnitZ(TAUnit.Id2Ptr(arg1))
        else
          result := TAUnit.GetUnitZ(p_Unit);
        end;
      UNITY :
        begin
        if arg1 <> 0 then
          result := TAUnit.GetUnitY(TAUnit.Id2Ptr(arg1))
        else
          result := TAUnit.GetUnitY(p_Unit);
        end;
      TURNX :
        begin
        if arg1 <> 0 then
          result := Word(TAUnit.GetTurnX(TAUnit.Id2Ptr(arg1)))
        else
          result := Word(TAUnit.GetTurnX(p_Unit));
        end;
      TURNZ :
        begin
        if arg1 <> 0 then
          result := Word(TAUnit.GetTurnZ(TAUnit.Id2Ptr(arg1)))
        else
          result := Word(TAUnit.GetTurnZ(p_Unit));
        end;
      TURNY :
        begin
        if arg1 <> 0 then
          result := Word(TAUnit.GetTurnY(TAUnit.Id2Ptr(arg1)))
        else
          result := Word(TAUnit.GetTurnY(p_Unit));
        end;
      UNIT_GRID_XZ :
        begin
        if arg1 <> 0 then
          result := MakeLong(p_Unit.nGridPosX, p_Unit.nGridPosZ)
        else
          result := MakeLong(TAUnit.Id2Ptr(arg1).nGridPosX, TAUnit.Id2Ptr(arg1).nGridPosZ);
        end;
      HEALTH_VAL :
        begin
        if arg1 <> 0 then
          result := TAUnit.GetHealth(TAUnit.Id2Ptr(arg1))
        else
          result := TAUnit.GetHealth(p_Unit);
        end;
      MAKE_DAMAGE :
        begin
        if ExtensionsNotForDemos then
          TAUnit.MakeDamage(TAUnit.Id2Ptr(arg3), TAUnit.Id2Ptr(arg4), TDmgType(arg1), arg2);
        end;
      HEAL_UNIT :
        begin
        result := UNITS_HealUnit(TAUnit.Id2Ptr(arg1),
          TAUnit.Id2Ptr(arg2), TAUnit.Id2Ptr(arg1).p_UNITINFO.nWorkerTime / 30 );
        end;
      GET_CLOAKED :
        begin
        if arg1 <> 0 then
          result := TAUnit.GetCloak(TAUnit.Id2Ptr(arg1))
        else
          result := TAUnit.GetCloak(p_Unit);
        end;
      SET_CLOAKED : //
        begin
        if ExtensionsNotForDemos then
        if arg2 <> 0 then
          TAUnit.SetCloak(TAUnit.Id2Ptr(arg2), arg1)
        else
          TAUnit.SetCloak(p_Unit, arg1);
        end;
      UNIT_BAS_STATE_MASK :
        begin
          if arg1 <> 0 then
            result := TAUnit.Id2Ptr(arg1).nUnitStateMaskBas
          else
            result := p_Unit.nUnitStateMaskBas;
        end;
      UNIT_STATE_MASK :
        begin
          if arg1 <> 0 then
            result := TAUnit.Id2Ptr(arg1).lUnitStateMask
          else
            result := p_Unit.lUnitStateMask;
        end;
      SELECTABLE :
        begin
        if arg2 <> 0 then
        begin
          if arg1 = 1 then
            TAUnit.Id2Ptr(arg2).lUnitStateMask := TAUnit.Id2Ptr(arg2).lUnitStateMask or 32
          else
            TAUnit.Id2Ptr(arg2).lUnitStateMask := TAUnit.Id2Ptr(arg2).lUnitStateMask and not 32;
        end else
        begin
          if arg1 = 1 then
            p_Unit.lUnitStateMask := p_Unit.lUnitStateMask or 32
          else
            p_Unit.lUnitStateMask := p_Unit.lUnitStateMask and not 32;
        end;
        end;
      ATTACH_UNIT :
        begin
          if ExtensionsNotForDemos then
          begin
          case arg1 of
            0 : TAUnit.AttachDetachUnit(TAUnit.Id2Ptr(arg2), TAUnit.Id2Ptr(arg3), arg4, False);
            1 : TAUnit.AttachDetachUnit(TAUnit.Id2Ptr(arg2), TAUnit.Id2Ptr(arg3), arg4, True);
            2 : Result := TAUnit.GetId(TAUnit.GetUnitAttachedTo(p_Unit, arg2));
          end;
          end;
        end;
      RANDOM_FREE_PIECE :
        begin
        result := TAUnit.GetRandomFreePiece(p_Unit, arg1, arg2);
        end;
      CUSTOM_BAR_PROGRESS :
        begin
          UnitID := TAUnit.GetId(p_Unit);
          if arg2 <> 0 then
          begin
              UnitsCustomFields[UnitID].CustomWeapReloadCur := arg1;
              UnitsCustomFields[UnitID].CustomWeapReloadMax := arg2;
          end else
          begin
            UnitsCustomFields[UnitID].CustomWeapReloadCur := 0;
            UnitsCustomFields[UnitID].CustomWeapReloadMax := 0;
          end;
        end;
      MEX_RATIO :
        begin
        result := Trunc(p_Unit.fMetalExtrRatio * 100);
        end;
    end;
    Exit;    
  end;
  if (index >= WEAPON_PRIMARY) and (index <= WEAPON_BUILD_PROGRESS) then
  begin
    case index of
      WEAPON_PRIMARY..WEAPON_TERTIARY :
        begin
        result := TAUnit.GetWeapon(p_Unit, index);
        end;
      UNIT_KILLS :
        begin
        if arg1 <> 0 then
          result := TAUnit.Id2Ptr(arg1).nKills
        else
          result := p_Unit.nKills;
        end;
      ATTACKER_ID :
        begin
        result := TAUnit.GetAttackerID(p_Unit);
        end;
      LOCKED_TARGET_ID :
        begin
          b := arg1 - WEAPON_PRIMARY;
          if p_Unit.UnitWeapons[b].nUsedSpot = $8000 then
            Result := p_Unit.UnitWeapons[b].nTargetID
          else
            Result := 0;
        end;
      UNDER_ATTACK :
        begin
        result := p_Unit.ucRecentDamage;
        end;
      FIRE_WEAPON :
        begin
        if ExtensionsNotForDemos then
          result := TAUnit.FireWeapon(p_Unit, arg1, TAUnit.Id2Ptr(arg2), TShortPosition(arg3));
        end;
      WEAPON_BUILD_PROGRESS :
        begin
        if arg1 = 0 then
          result := GetUnit_BuildWeaponProgress(p_Unit)
        else
          result := p_Unit.UnitWeapons[0].cStock;
        end;
    end;
    Exit;    
  end;
  if (index >= GIVE_UNIT) and (index <= SWAP_UNIT_TYPE) then
  begin
    case index of
      GIVE_UNIT :
        begin
        TAUnits.GiveUnit(TAUnit.Id2Ptr(arg1), arg2);
        end;
      CREATE_UNIT : //
        begin
          if ExtensionsNotForDemos then
          begin
            if GetTPosition(HiWord(Cardinal(arg2)), LoWord(Cardinal(arg2)), Position) <> nil then
            begin
              UnitinfoSt := TAMem.UnitInfoCrc2Ptr(arg1);
              if (UnitInfoSt.nCruiseAlt <> 0) and (arg4 = 6) then
                Position.Y := (GetPosHeight(@Position) + UnitInfoSt.nCruiseAlt - TAData.MainStruct.TNTMemStruct.SeaLevel) * 65535;
              if arg3 = 10 then
                arg3 := TAUnit.GetOwnerIndex(p_Unit);
              p_Unit := TAUnit.CreateUnit(arg3, UnitinfoSt, Position, nil, False, False, arg4);
              if p_Unit <> nil then
              begin
                result := Word(p_Unit.lUnitInGameIndex);
                if TAData.NetworkLayerEnabled then
                  Send_UnitBuildFinished(p_Unit, p_Unit);
              end;
            end;
          end;
        end;
      KILL_OTHER_UNIT : //
        begin
        if ExtensionsNotForDemos then
          TAUnit.Kill(TAUnit.Id2Ptr(arg1), arg2);
        end;
      CREATE_MINIONS : //
        begin
        if ExtensionsNotForDemos then
          result := TAUnits.CreateMinions(p_Unit, arg2, TAMem.UnitInfoCrc2Ptr(arg1), TTAActionType(arg3), arg4);
        end;
    end;
    Exit;    
  end;
  if (index >= UNITS_NEAR) and (index <= DISTANCE) then
  begin
    case index of
      UNITS_NEAR :
        begin
        result := TAUnits.SearchUnits(p_Unit,
          arg3, 2, arg2, TAUnits.CreateSearchFilter(arg1), TAMem.UnitInfoCrc2Ptr(arg4));
        end;
      UNITS_YARDMAP :
        begin
        result := TAUnits.SearchUnits(p_Unit,
          arg3, 1, arg2, TAUnits.CreateSearchFilter(arg1), TAMem.UnitInfoCrc2Ptr(arg4));
        end;
      UNITS_WHOLEMAP :
        begin
        result := TAUnits.SearchUnits(p_Unit,
          arg3, 4, arg2, TAUnits.CreateSearchFilter(arg1), TAMem.UnitInfoCrc2Ptr(arg4));
        end;
      UNITS_ARRAY_RESULT :
        begin
        case arg3 of
          1 : begin
              if Assigned(UnitSearchArr[arg1].UnitIds) then
                result:= Word(UnitSearchArr[arg1].UnitIds[arg2 - 1]);
              end;
          2 : begin
              if Assigned(SpawnedMinionsArr[arg1].UnitIds) then
                result := Word(SpawnedMinionsArr[arg1].UnitIds[arg2 - 1]);
              end;
        end;
        end;
      RANDOMIZE_UNITS_ARRAY :
        begin
        TAUnits.RandomizeSearchRec(arg1, arg2);
        result := 1;
        end;
      FREE_ARRAY_ID :
        begin
        result := TAUnits.GetRandomArrayId(arg1);
        end;
      CLEAR_ARRAY_ID :
        begin
        TAUnits.ClearSearchRec(arg2, arg1);
        end;
      UNIT_NEAREST :
        begin
        result := TAUnits.SearchUnits(p_Unit,
          0, 3, arg2, TAUnits.CreateSearchFilter(arg1), TAMem.UnitInfoCrc2Ptr(arg3));
        end;
      DISTANCE :
        begin
        if arg2 <> 0 then
          result := Cardinal(TAMem.DistanceBetweenPos(@TAUnit.Id2Ptr(arg1).Position,
            @TAUnit.Id2Ptr(arg2).Position))
        else
          result := Cardinal(TAMem.DistanceBetweenPos(@p_Unit.Position, @TAUnit.Id2Ptr(arg1).Position));
        end;
    end;
    Exit;    
  end;

  if (index >= CURRENT_ORDER_ABORT) and (index <= ADD_BUILD) then
  begin
    case index of
      CURRENT_ORDER_ABORT :
        begin
        TAunit.CancelCurrentOrder(p_Unit);
        end;
      CURRENT_ORDER_TYPE :
        begin
        if arg1 <> 0 then
          result := Ord(TAUnit.GetCurrentOrderType(TAUnit.Id2Ptr(arg1)))
        else
          result := Ord(TAUnit.GetCurrentOrderType(p_Unit));
        end;
      CURRENT_ORDER_TARGET_POS :
        begin
        result := TAUnit.GetCurrentOrderPos(p_Unit);
        end;
      CURRENT_ORDER_TARGET_ID :
        begin
        result := TAUnit.GetId(TAUnit.GetCurrentOrderTargetUnit(p_Unit));
        end;
      CURRENT_ORDER_PAR :
        begin
        result := Cardinal(TAUnit.GetCurrentOrderParams(p_Unit, arg1));
        end;
      CURRENT_ORDER_PAR_EDIT : //
        begin
        if ExtensionsNotForDemos then
          result := BoolValues[(TAUnit.EditCurrentOrderParams(p_Unit, arg1, arg2)) = True];
        end;
      ORDER_SELF : //
        begin
        if ExtensionsNotForDemos then
          result := TAUnit.CreateMainOrder(p_Unit, nil, TTAActionType(arg1), nil, arg2, arg3, arg4);
        end;
      ORDER_SELF_POS : //
        begin
          if ExtensionsNotForDemos then
          begin
            if GetTPosition(HiWord(Cardinal(arg3)), LoWord(Cardinal(arg3)), Position) <> nil then
              result := TAUnit.CreateMainOrder(p_Unit, nil, TTAActionType(arg1), @Position, arg2, LoWord(arg4), HiWord(arg4));
          end;
        end;
      ORDER_UNIT_UNIT : //
        begin
        if ExtensionsNotForDemos then
          result := TAUnit.CreateMainOrder(TAUnit.Id2Ptr(arg2), TAUnit.Id2Ptr(arg3), TTAActionType(arg1), nil, LoWord(arg4), HiWord(arg4), 0);
        end;
      ORDER_UNIT_POS : //
        begin
          if ExtensionsNotForDemos then
          begin
            if GetTPosition(HiWord(Cardinal(arg3)), LoWord(Cardinal(arg3)), Position) <> nil then
              result := TAUnit.CreateMainOrder(TAUnit.Id2Ptr(arg2), nil, TTAActionType(arg1), @Position, LoWord(arg4), HiWord(arg4), 0);
          end;
        end;
      ORDER_SELF_UNIT_POS : //
        begin
          if ExtensionsNotForDemos then
          begin
            if GetTPosition(HiWord(Cardinal(arg3)), LoWord(Cardinal(arg3)), Position) <> nil then
              result := TAUnit.CreateMainOrder(p_Unit, TAUnit.Id2Ptr(arg2), TTAActionType(arg1), @Position, LoWord(arg4), HiWord(arg4), 0);
          end;
        end;
      RESET_ORDER :
        begin
        if ExtensionsNotForDemos then
          p_Unit.p_MainOrder := nil;
        end;
      ADD_BUILD :
        begin
        if ExtensionsNotForDemos then
          case arg1 of
            1 : ORDERS_NewSubBuildOrder(Ord(Action_BuildWeapon), p_Unit, 0, arg2);
            2 : p_Unit.UnitWeapons[arg3 - 1].cStock := p_Unit.UnitWeapons[arg3 - 1].cStock + arg2;
            3 : ORDERS_NewSubBuildOrder(Ord(Action_BuildingBuild), p_Unit, arg3, arg2);
          end;
        end;
    end;
    Exit;    
  end;
  if (index >= GRANT_UNITINFO) and (index <= MOBILE_PLANT) then
  begin
    case index of
      GET_UNITINFO :
        begin
        if arg2 <> 0 then
          result := TAUnit.getUnitInfoField(TAUnit.Id2Ptr(arg2), TUnitInfoExtensions(arg1))
        else
          result := TAUnit.getUnitInfoField(p_Unit, TUnitInfoExtensions(arg1));
        end;
      SET_UNITINFO : //
        begin
          i := arg2;
          if arg3 <> 0 then
            i := -i;
          if TAUnit.setUnitInfoField(p_Unit, TUnitInfoExtensions(arg1), i) then
            if TAData.NetworkLayerEnabled then
              GlobalDPlay.Broadcast_UnitInfoEdit(TAUnit.GetID(p_Unit), arg1, i);
        end;
      UNIT_TYPE_LIMIT :
        begin
          if TAUnit.IsOnThisComp(p_Unit, False) then
          begin
            TAMem.ProtectMemoryRegion(TAData.UnitInfosPtr, True);
            Result := TAMem.UnitInfoCrc2Ptr(arg1).lBuildLimit;
            TAMem.UnitInfoCrc2Ptr(arg1).lBuildLimit := arg2;
            TAMem.ProtectMemoryRegion(TAData.UnitInfosPtr, False);
          end;
        end;
    end;
    Exit;    
  end;
  if (index >= UNIT_SPEECH) and (index <= EMIT_SFX) then
  begin
    case index of
      PLAY_3D_SOUND : //
        begin
        if ExtensionsNotForDemos then
        begin
          if GetTPosition(HiWord(Cardinal(arg2)), LoWord(Cardinal(arg2)), Position) <> nil then
            result := TASfx.Play3DSound(arg1, Position, arg3 = 1);
        end;
        end;
      PLAY_GAF_ANIM :
        begin
        result := TASfx.PlayGafAnim(arg1, HiWord(arg2), LoWord(arg2), arg3, arg4);
        end;
      EMIT_SFX :
        begin
        Result := BoolValues[TASfx.EmitSfxFromPiece(p_Unit,
          TAUnit.Id2Ptr(arg3), arg1, arg2, p_Unit.ucOwnerID = TAData.LocalPlayerID) <> 0];
        end;
    end;
    Exit;    
  end;
  if (index >= COB_QUERY_SCRIPT) and (index <= LOCAL_SHARED_DATA) then
  begin
    case index of
    COB_QUERY_SCRIPT :
      begin
      result := TAUnit.CobQueryScript(TAUnit.Id2Ptr(arg1),
        GetEnumName(TypeInfo(TCobMethods), Integer(arg2))^, arg3, arg4, 0, 0);
      end;
    COB_START_SCRIPT :
      begin
        TAUnit.CobStartScript(TAUnit.Id2Ptr(arg1),
          GetEnumName(TypeInfo(TCobMethods), Integer(arg2))^, @arg3, @arg4, nil, nil, True);
      end;
    LOCAL_SHARED_DATA :
      begin
      if Boolean(arg1) then
        UnitsSharedData[arg2] := arg3
      else
        Result := UnitsSharedData[arg2];
      end;
    end;
    Exit;    
  end;
  if (index >= MAP_SEA_LEVEL) and (index <= GRID_INFO) then
  begin
    case index of
      MAP_SEA_LEVEL :
        begin
        result := TAData.MainStruct.TNTMemStruct.SeaLevel;
        end;
      IS_LAVA_MAP :
        begin
        result := PMapOTAFile(TAData.MainStruct.p_MapOTAFile).lIsLavaMap;
        end;
      SURFACE_METAL :
        begin
        if arg1 <> 0 then
          PMapOTAFile(TAData.MainStruct.p_MapOTAFile).lSurfaceMetal := arg1;
        result := PMapOTAFile(TAData.MainStruct.p_MapOTAFile).lSurfaceMetal;
        end;
      UNIT_AT_POSITION :
        begin
        if GetTPosition(HiWord(Cardinal(arg1)), LoWord(Cardinal(arg1)), Position) <> nil then
          result := TAUnit.AtPosition(@Position);
        end;
      TEST_UNLOAD_POS :
        begin
        result := BoolValues[(TAUnit.TestUnloadPosition(TAUnit.Id2Ptr(arg1).p_UNITINFO, TAUnit.Id2Ptr(arg1).Position))];
        end;
      TEST_BUILD_SPOT :
        begin
        result := BoolValues[(TAUnit.TestBuildSpot(TAUnit.GetOwnerIndex(p_Unit), TAMem.UnitInfoCrc2Ptr(arg1), HiWord(arg2), LoWord(arg2)) = True)];
        end;
      PLANT_YARD_OCCUPIED :
        begin
        result := BoolValues[TAUnit.IsPlantYardOccupied(p_Unit, arg1)];
        end;
      UNIT_REBUILD_YARD :
        begin
        result := UNITS_RebuildFootPrint(p_Unit);
        end;
      FEATURE_TYPE_AT_POS :
        begin
          i := GetGridPosFeature(GetGridPosPLOT(arg1, arg2));
          if i >= 0 then
            Result := Word(i);
        end;
      FEATURE_INFO :
        begin
        Result := TAMem.GetFeatureInfo(TAMem.FeatureDefId2Ptr(arg1), arg2);
        end;
      GRID_INFO :
        begin
        GridPlot := GetGridPosPLOT(arg1, arg2);
        if GridPlot <> nil then
        begin
          case arg3 of
            0: Result := GridPlot.bMetalExtract;
            1: Result := GridPlot.bHeight;
            2: Result := GridPlot.bYard_type;
          end;
        end;
        end;
    end;
    Exit;    
  end;
  if (index >= MS_MOVE_CAM_POS) and (index <= MS_AI_DIFFICULTY) then
  begin
    case index of
      MS_MOVE_CAM_POS :
        begin
        ScrollView(arg1, arg2, Boolean(arg3));
        end;
      MS_LOCK_CAM_TO_UNIT :
        begin
        if arg1 <> 0 then
          TAMap.SetCameraToUnit(TAUnit.Id2Ptr(arg1))
        else
          TAMap.SetCameraToUnit(p_Unit);
        end;
      MS_SHAKE :
        begin
        TAMem.ShakeCam(arg1, arg2, arg3);
        end;
      MS_SCREEN_GAMMA :
        begin
        result := TAData.MainStruct.Gamma;
        end;
      MS_DESELECT_UNITS :
        begin
        DeselectAllUnits;
        UpdateIngameGUI(1);
        end;
      MS_PLAY_SOUND_2D :
        begin
        if Assigned(MapMissionsSounds) then
        begin
          PlaySound_2D_Name(PAnsiChar(MapMissionsSounds[arg1]), arg2);
        end;
        end;
      MS_PLAY_SOUND_3D :
        begin
        if Assigned(MapMissionsSounds) then
        begin
          PlaySound_3D_Name(PAnsiChar(MapMissionsSounds[arg1]), @arg3, arg2);
        end;
        end;
      MS_EMIT_SMOKE :
        begin
          GetTPosition(Cardinal(arg2), Cardinal(arg3), Position);
          case arg1 of
            0 : EmitSfx_SmokeInfinite(@Position, 4);
            1 : EmitSfx_GraySmoke(@Position, 9);
            2 : EmitSfx_BlackSmoke(@Position, 9);
            3 : ;
            4 : ;
            5 : ;
          end;
        end;
      MS_PLACE_FEATURE :
        begin
        GetTPosition(Cardinal(arg2), Cardinal(arg3), Position);
        Turn.Z := arg4;
        Result := BoolValues[TAMap.PlaceFeatureOnMap(MapMissionsFeatures[arg1], Position, Turn)];
        end;
      MS_REMOVE_FEATURE :
        begin
        TAMap.RemoveMapFeature(arg1, arg2, (arg3 = 1));
        end;
      MS_VIEW_PLAYER_ID :
        begin
        result := TAData.LocalPlayerID;
        end;
      MS_GAME_TIME :
        begin
        result := TAData.GameTime;
        end;
      MS_APPLY_UNIT_SCRIPT :
        begin
        Campaign_ParseUnitInitialMission(TAUnit.Id2Ptr(arg1),
          PAnsiChar(MapMissionsUnitsInitialMissions[arg2]), nil);
        end;
      MS_FIRE_MAP_WEAPON :
        begin
        TAWeapon.FireMap_Weapon(TAWeapon.WeaponId2Ptr(arg1), HiWord(arg2), LoWord(arg2));
        end;
      MS_SHOW_TEXTMSG :
        begin
          case arg1 of
            0 :
              begin
                if arg4 <> 0 then
                  ShowChatMessage(TAPlayer.GetPlayerByIndex(arg2),
                    PAnsiChar(MapMissionsTextMessages[arg3]),
                    4, @TAPlayer.GetPlayerByIndex(arg4).szSecondName)
                else
                  ShowChatMessage(TAPlayer.GetPlayerByIndex(arg2),
                    PAnsiChar(MapMissionsTextMessages[arg3]),
                    4, nil);
              end;
            1 :
              begin
                ShowReminderMsg(PAnsiChar(MapMissionsTextMessages[arg2]), arg3);
              end;
          end;
        end;
      MS_AI_DIFFICULTY :
        begin
        Result := Ord(TAData.AIDifficulty);
        end;
    end;
    Exit;    
  end;
  if index >= LOWWORD then
  begin
    case index of
      LOWWORD :
        begin
        result := LoWord(arg1);
        end;
      HIGHWORD :
        begin
        result := HiWord(arg1);
        end;
      MAKEDWORD :
        begin
        result := Cardinal(MakeLong(Word(arg1), Word(arg2)));
        end;
      {$IFDEF DEBUG}
      DBG_OUTPUT :
        begin
        SendTextLocal(IntToStr(TAUnit.GetId(p_Unit)) + ' DBG_OUTPUT: [' + IntToStr(arg1) +'] + Value: ' + IntToStr(arg2) + ', Hex: ' + IntToHex(arg2, 8));
        end;
      {$ENDIF}
    end;
  end;
{$IFDEF DEBUG}
except
  on E: Exception do
    begin
    LogException(E);
    try
      errorAddress := Longword(ExceptAddr);
      if not MapFileSetup then
        LoadAndParseMapFile;
      errorAddress2 := GetMapAddressFromAddress(errorAddress);
      s := GetModuleNameFromAddress(errorAddress2);
      if s <> '' then
        begin
        TLog.Add(0, 'COB GETTER '+s+':'+
                    GetProcNameFromAddress(errorAddress2)+':'+
                    GetLineNumberFromAddress(errorAddress2)+':'+
                    IntToStr(index)+':'+
                    IntToStr(arg1)+':'+
                    IntToStr(arg2)+':'+
                    IntToStr(arg3)+':'+
                    IntToStr(arg4));
        if p_Unit.p_UNITINFO <> nil then
          TLog.Add(0, 'Called by ' + p_Unit.p_UNITINFO.szName + ' ' + p_Unit.p_UNITINFO.szUnitName);
        end;

      if E is EInvalidPointer then
        raise e at ExceptAddr;
    finally
      TLog.Flush;
    end;
  end;
end;
{$ENDIF}
end;

procedure CustomSetters(index: Cardinal;
  p_Unit: PUnitStruct; arg1: Integer); stdcall;
var
  ExtensionsNotForDemos: Boolean;
  {$IFDEF DEBUG}
  errorAddress,errorAddress2: Longword;
  s: string;
  {$ENDIF}
begin
{$IFDEF DEBUG}
try
{$ENDIF}
  if ((index >= CUSTOM_LOW) and (index <= CUSTOM_HIGH)) then
  begin
    case index of
      MS_SCREEN_GAMMA :
        begin
        SetGamma(arg1 * 0.1);
        TAData.MainStruct.Gamma := arg1;
        end;
      MS_SCREEN_FADE :
        begin
        CameraFadeLevel := arg1;
        end;
      MS_DRAW_MOUSE :
        begin
        Mouse_SetDrawMouse(Boolean(arg1));
        if arg1 = 0 then
          MouseLock := True
        else
          MouseLock := False;
        end;
      MS_SWAP_TERRAIN :
        SwapTNT(arg1);
      UNITY :
        begin
          if arg1 <> -1 then
          begin
            UnitsCustomFields[TAUnit.GetId(p_Unit)].ForcedYPos := True;
            UnitsCustomFields[TAUnit.GetId(p_Unit)].ForcedYPosVal := arg1;
            p_Unit.Position.Y := arg1 shl 16;
          end else
          begin
            UnitsCustomFields[TAUnit.GetId(p_Unit)].ForcedYPos := False;
            UnitsCustomFields[TAUnit.GetId(p_Unit)].ForcedYPosVal := 0;
          end;
        end;
    end;
    if TAUnit.IsOnThisComp(p_Unit, True) then
    begin
      case index of
        UNIT_SPEECH : //
          begin
          TASfx.Speech(p_Unit, arg1, nil);
          end;
        MOBILE_PLANT : //
          begin
          if arg1 = 1 then
          begin
            p_Unit.lUnitStateMask:= p_Unit.lUnitStateMask or $20000000;
            p_Unit.nUnitStateMaskBas := p_Unit.nUnitStateMaskBas and not 1;
          end else
          begin
            p_Unit.lUnitStateMask:= p_Unit.lUnitStateMask and not $20000000;
            p_Unit.nUnitStateMaskBas := p_Unit.nUnitStateMaskBas or 1;
          end;
          end;
        GUI_INDEX :
          begin
          UnitInfoCustomFields[p_Unit.nUnitInfoID].CustomGUIIdx := arg1;
          end;
        MEX_RATIO :
          begin
          p_Unit.fMetalExtrRatio := arg1 / 100;
          end;
      end;

      if TAData.NetworkLayerEnabled then
        ExtensionsNotForDemos := GlobalDPlay.NotViewingRecording
      else
        ExtensionsNotForDemos := True;

      if ExtensionsNotForDemos then
      begin
        case index of
          CURRENT_SPEED :
            begin
            TAUnit.SetCurrentSpeed(p_Unit, arg1);
            end;
          WEAPON_AIM_ABORTED :
            begin
            if ((TAUnit.GetCurrentOrderType(p_Unit) >= Action_AirStrike) and
               (TAUnit.GetCurrentOrderType(p_Unit) <= Action_AttackUType)) or
               (TAUnit.GetCurrentOrderType(p_Unit) = Action_Guard_NoMove) then
              PUnitOrder(p_Unit.p_MainOrder).lMask := p_Unit.p_MainOrder.lMask or $8;
            case arg1 of
              WEAPON_PRIMARY   : p_Unit.UnitWeapons[0].nTargetID := $0;
              WEAPON_SECONDARY : p_Unit.UnitWeapons[1].nTargetID := $0;
              WEAPON_TERTIARY  : p_Unit.UnitWeapons[2].nTargetID := $0;
            end;
            end;
          WEAPON_READY :
            begin
            case arg1 of
              WEAPON_PRIMARY   : p_Unit.UnitWeapons[0].nReloadTime := 0;
              WEAPON_SECONDARY : p_Unit.UnitWeapons[1].nReloadTime := 0;
              WEAPON_TERTIARY  : p_Unit.UnitWeapons[2].nReloadTime := 0;
            end;
            end;
          TURNX:
            begin
            TAUnit.setTurnX(p_Unit, arg1);
            end;
          TURNY:
            begin
            TAUnit.setTurnY(p_Unit, arg1);
            end;
          TURNZ:
            begin
            TAUnit.setTurnZ(p_Unit, arg1);
            end;
          SWAP_UNIT_TYPE :
            begin
            TAUnit.SwapByKill(p_Unit, TAMem.UnitInfoCrc2Ptr(arg1));
            end;
          WEAPON_PRIMARY..WEAPON_TERTIARY :
            begin
            if TAUnit.setWeapon(p_Unit, index, arg1) then
              if TAData.NetworkLayerEnabled then
                GlobalDPlay.Broadcast_UnitWeapon(TAUnit.GetID(p_Unit), Index, arg1);
            end;
          KILL_THIS_UNIT :
            begin
            TAUnit.Kill(p_Unit, arg1);
            end;
          GRANT_UNITINFO :
            begin
            TAUnit.GrantUnitInfo(p_Unit, arg1, True);
            end;
          UNIT_TYPE_CRC :
            begin
            TAUnit.SetUnitInfo(p_Unit, TAMem.UnitInfoCrc2Ptr(arg1), True);
            end;
        end;
      end;
    end;
  end;
{$IFDEF DEBUG}
except
  on E: Exception do
    begin
    LogException(E);
    try
      errorAddress := Longword(ExceptAddr);
      if not MapFileSetup then
        LoadAndParseMapFile;
      errorAddress2 := GetMapAddressFromAddress(errorAddress);
      s := GetModuleNameFromAddress(errorAddress2);
      if s <> '' then
        begin
        TLog.Add(0, 'COB SETTER '+s+':'+
                    GetProcNameFromAddress(errorAddress2)+':'+
                    GetLineNumberFromAddress(errorAddress2)+':'+
                    IntToStr(index)+':'+
                    IntToStr(arg1));
        if p_Unit.p_UNITINFO <> nil then
          TLog.Add(0, 'Called by ' + p_Unit.p_UNITINFO.szName + ' ' + p_Unit.p_UNITINFO.szUnitName);
        end;

      if E is EInvalidPointer then
        raise e at ExceptAddr;
    finally
      TLog.Flush;
    end;
  end;
end;
{$ENDIF}
end;

Procedure COB_Extensions_Handling;
label
  DoReturn, DoReturnWithBreak,
  GeneralCaseGetter;
asm
  // function prolog
  mov     ecx, [esp+4]//[esp+GetterIdx]
  sub     esp, 24h
  push    esi
  test    eax, eax
  jz      DoReturnWithBreak
  // start of case statement
  mov     esi, [eax+0Ch]
  lea     eax, [ecx-1]    // switch 20 cases
  cmp     eax, 13h
  ja      GeneralCaseGetter
  jmp     ds:$480AC4[eax*4] // switch jump
GeneralCaseGetter:
  inc     eax
  mov     ecx, [esp+28h+$14] // arg_14
  push    ecx;
  mov     ecx, [esp+2ch+$10] // arg_10
  push    ecx;
  mov     ecx, [esp+30h+$C] // arg_C
  push    ecx;
  mov     ecx, [esp+34h+$8] // arg_8
  push    ecx;
  push    esi;             // unitptr
  push    eax;             // index
  call    CustomGetters;
DoReturn:
  pop     esi;
  add     esp, 24h;
  ret 14h;
DoReturnWithBreak:
  xor     eax, eax
  pop     esi;
  add     esp, 24h;
  ret 14h;
end;

Procedure COB_ExtensionsSetters_Handling;

label
  DoReturn, DoReturnWithBreak,
  GeneralCaseSetter;
asm
  // function prolog
  mov     ecx, [esp+4]//[esp+SetterIdx]
  push    esi
  // start of case statement
  test    eax, eax
  jz      DoReturnWithBreak
  mov     esi, [eax+0Ch]
  lea     eax, [ecx-1]    // switch 20 cases
  cmp     eax, 13h
  ja      GeneralCaseSetter
  xor     edx, edx
  mov     dl, ds:$480C18[eax]
  jmp     ds:$480BFC[edx*4] // switch jump
GeneralCaseSetter:
  inc     eax;
  mov     ecx, [esp+4h+$8] // value to be set
  push    ecx;
  push    esi;             // unitPtr
  push    eax;             // index
  call    CustomSetters;
DoReturn:
  pop esi;
  ret 8h;
DoReturnWithBreak:
  xor     eax, eax
  pop esi;
  ret 8h;
end;

end.
