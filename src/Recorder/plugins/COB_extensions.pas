unit COB_extensions;

interface
uses
  {$IFDEF DEBUG}logging, uDebug,{$ENDIF}
  PluginEngine, SynCommons, TA_MemoryLocations, TA_MemoryStructures;

// -----------------------------------------------------------------------------

const
  State_COB_extensions : boolean = true;         

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
 // returns local long id of unit
 ID_TO_STAMP = 76;
 CONFIRM_STAMP = 77;
 UNIT_TYPE_CRC = 78;
 UNIT_TYPE_CRC_TO_ID = 79;
 UNIT_TYPE_IN_CATEGORY = 80;
 PRIOR_UNIT = 81;
 TRANSPORTED_BY = 82;
 TRANSPORTING = 83;

 { Players }
 PLAYER_ACTIVE = 84;
 PLAYER_TYPE = 85;
 PLAYER_SIDE = 86;
 PLAYER_KILLS = 87;
 UNIT_IN_PLAYER_LOS = 88;
 POSITION_IN_PLAYER_LOS = 89;
 OWNED_BY_ALLY = 90;
 RESOURCES = 91;

 { Some specific }
 UNITX = 92;
 UNITZ = 93;
 UNITY = 94;
 TURNX = 95;
 TURNZ = 96;
 TURNY = 97;
 HEALTH_VAL = 98;
 MAKE_DAMAGE = 99;
 HEAL_UNIT = 100;
 GET_CLOAKED = 101;
 SET_CLOAKED = 102;
 STATE_UNIT = 103;
 SFX_OCCUPY_STATE = 104;
 SELECTABLE = 105;
 ATTACH_UNIT = 106;
 RANDOM_FREE_PIECE = 107;
 CUSTOM_BAR_PROGRESS = 108;
 MEX_RATIO = 109;

 { Weapons, attack info }
 WEAPON_PRIMARY = 110;
 WEAPON_SECONDARY = 111;
 WEAPON_TERTIARY = 112;
 UNIT_KILLS = 113;
 ATTACKER_ID = 114;
 LOCKED_TARGET_ID = 115;
 UNDER_ATTACK = 116;
 FIRE_WEAPON = 117;
 WEAPON_BUILD_PROGRESS = 118;

 { Creating and killing }
 GIVE_UNIT = 120;
 CREATE_UNIT = 121;
 KILL_THIS_UNIT = 122;
 KILL_OTHER_UNIT = 123;
 CREATE_MINIONS = 124;
 SWAP_UNIT_TYPE = 125;

 { Searching for units }
 UNITS_NEAR = 127;
 UNITS_YARDMAP = 128;
 UNITS_WHOLEMAP = 129;
 UNITS_ARRAY_RESULT = 130;
 RANDOMIZE_UNITS_ARRAY = 131;
 FREE_ARRAY_ID = 132;
 CLEAR_ARRAY_ID = 133;
 UNIT_NEAREST = 134;
 DISTANCE = 135;

 { Unit orders }
 CURRENT_ORDER_TYPE = 137;
 CURRENT_ORDER_TARGET_POS = 138;
 CURRENT_ORDER_TARGET_ID = 139;
 CURRENT_ORDER_PAR = 140;
 EDIT_CURRENT_ORDER_PAR = 141;
 ORDER_SELF = 142;
 ORDER_SELF_POS = 143;
 ORDER_UNIT_UNIT = 144;
 ORDER_UNIT_POS = 145;
 ORDER_SELF_UNIT_POS = 146;
 RESET_ORDER = 147;
 ADD_BUILD = 148;

 { Global unit template }
 GRANT_UNITINFO = 149;
 GET_UNITINFO = 150;
 SET_UNITINFO = 151;
 ENABLEDISABLE_UNIT = 152;
 MOBILE_PLANT = 153;

 { Sfx }
 UNIT_SPEECH = 154;
 PLAY_3D_SOUND = 155;
 PLAY_GAF_ANIM = 156;
 EMIT_SFX = 157;

 { Other }
 CALL_COB_PROC = 158;
 LOCAL_SHARED_DATA = 159;

 { Map }
 MAP_SEA_LEVEL = 162;
 IS_LAVA_MAP = 163;
 SURFACE_METAL = 164;
 UNIT_AT_POSITION = 165;
 TEST_UNLOAD_POS = 166;
 TEST_BUILD_SPOT = 167;
 PLANT_YARD_OCCUPIED = 168;
 UNIT_REBUILD_YARD = 169;

 MS_MOVE_CAM_POS = 175;
 MS_MOVE_CAM_UNIT = 176;
 MS_SHAKE = 177;
 MS_SCREEN_GAMMA = 178;
 MS_SCREEN_FADE = 179;
 MS_DRAW_MOUSE = 180;
 MS_DESELECT_UNITS = 181;
 MS_PLAY_SOUND_2D = 182;
 MS_PLAY_SOUND_3D = 183;
 MS_EMIT_SMOKE = 184;
 MS_PLACE_FEATURE = 185;
 MS_REMOVE_FEATURE = 186;
 MS_SWAP_TERRAIN = 187;
 MS_VIEW_PLAYER_ID = 188;
 MS_GAME_TIME = 189;
 MS_APPLY_UNIT_SCRIPT = 190;
 MS_FIRE_MAP_WEAPON = 191;

 { Math }
 LOWWORD = 200;
 HIGHWORD = 201;
 MAKEDWORD = 202;

 DBG_OUTPUT = 300;

 CUSTOM_LOW = WEAPON_AIM_ABORTED;
 CUSTOM_HIGH = DBG_OUTPUT;

// -----------------------------------------------------------------------------

var
  UnitSearchArr : TUnitSearchArr;
  SpawnedMinionsArr : TSpawnedMinionsArr;
  UnitSearchResults,
  SpawnedMinions : TDynArray;
  UnitSearchCount,
  SpawnedMinionsCount : Integer;

// -----------------------------------------------------------------------------

Procedure COB_Extensions_Handling;
Procedure COB_ExtensionsSetters_Handling;

implementation
uses
  idplay,
  Windows,
  TA_NetworkingMessages,
  TADemoConsts,
  TA_MemoryConstants,
  TA_MemPlayers,
  TA_MemUnits,
  TA_MemPlotData,
  TA_FunctionsU,
  SysUtils,
  MapExtensions,
  UnitInfoExpand,
  IniOptions;

Procedure OnInstallCobExtensions;
begin
end;

Procedure OnUninstallCobExtensions;
begin
end;

function GetPlugin : TPluginData;
var
  lReplacement : Cardinal;
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

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Maximum scripts slots that can be run by a unit in game
  //////////////////////////////////////////////////////////////////////////////////////////////////

  if IniSettings.Plugin_ScriptSlotsLimit and
     (IniSettings.ModId > 1) then
  begin
    lReplacement := SizeOf(TNewScriptsData);
    result.MakeReplacement( State_COB_extensions,
                            'Init scripts data - Alloc memory increase',
                            $00485D74, lReplacement, SizeOf(lReplacement));

    lReplacement := $0000291C;
    result.MakeReplacement( State_COB_extensions,
                            'Init scripts data - ScriptsData.lStartRunningNow',
                            $004B0638, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B0923, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B09CA, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B0A3F, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B0B88, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B0D22, lReplacement, SizeOf(lReplacement));

    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B0D6A, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B19EF, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B19FB, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B1AA1, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B1AAD, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B1B68, lReplacement, SizeOf(lReplacement));


    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B1B77, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B1F15, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004B211E, lReplacement, SizeOf(lReplacement));

    lReplacement := $00002920;
    result.MakeReplacement( State_COB_extensions,
                            'Init scripts data - ScriptsData.pObject3d',
                            $00480D46, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480C56, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480C7F, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480C8C, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480C99, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480C36, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480CBA, lReplacement, SizeOf(lReplacement));

    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480CF3, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480D0C, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480D1E, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480D2B, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480D5E, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480D8B, lReplacement, SizeOf(lReplacement));

    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480D96, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480DB6, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480DDF, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480DF6, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480E20, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480E36, lReplacement, SizeOf(lReplacement));

    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480E56, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480E76, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480EBD, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480EF4, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480F0C, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00480F5B, lReplacement, SizeOf(lReplacement));

    result.MakeReplacement( State_COB_extensions,
                            '',
                            $0048115B, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $0048124C, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00481388, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00481397, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $004813EE, lReplacement, SizeOf(lReplacement));

    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00481432, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            '',
                            $00481472, lReplacement, SizeOf(lReplacement));

    result.MakeReplacement( State_COB_extensions,
                            'setters',
                            $00480B22, lReplacement, SizeOf(lReplacement));
    result.MakeReplacement( State_COB_extensions,
                            'getters',
                            $00480772, lReplacement, SizeOf(lReplacement));

    result.MakeReplacement( State_COB_extensions,
                            'start script',
                            $004B09E1, MAX_SCRIPT_SLOTS, 1);
    result.MakeReplacement( State_COB_extensions,
                            'no xrefs',
                            $004B0A57, MAX_SCRIPT_SLOTS, 1);
    result.MakeReplacement( State_COB_extensions,
                            'run script',
                            $004B0B9F, MAX_SCRIPT_SLOTS, 1);
    result.MakeReplacement( State_COB_extensions,
                            'do scripts now, called from units loop',
                            $004B0D81, MAX_SCRIPT_SLOTS, 1);
    result.MakeReplacement( State_COB_extensions,
                            'alloc mem and init script slots',
                            $004B0616, MAX_SCRIPT_SLOTS, 1);

    result.MakeReplacement( State_COB_extensions,
                            'slots limiter',
                            $004B08E5, MAX_SCRIPT_SLOTS, 1);

    result.MakeReplacement( State_COB_extensions,
                            'dispatcher 1',
                            $004B1AA7, MAX_SCRIPT_SLOTS, 1);
    result.MakeReplacement( State_COB_extensions,
                            'dispatcher 2',
                            $004B1AEE, MAX_SCRIPT_SLOTS, 1);
    result.MakeReplacement( State_COB_extensions,
                            'dispatcher 3',
                            $004B19F5, MAX_SCRIPT_SLOTS, 1);
  end;

  end
else
  result := nil;
end;

function CustomGetters( index : Cardinal;
                        p_Unit : PUnitStruct;
                        arg1, arg2, arg3, arg4 : Cardinal) : Cardinal; stdcall;
var
  UnitInfoSt : PUnitInfo;
  Position : TPosition;
  Turn: TTurn;
  b : Byte;
  i : Integer;
  ExtensionsNotForDemos : Boolean;
  {$IFDEF DEBUG}
  errorAddress,errorAddress2 : Longword;
  s:string;
  {$ENDIF}
begin
result := 0;
{$IFDEF DEBUG}
try
{$ENDIF}
if ((index >= CUSTOM_LOW) and (index <= CUSTOM_HIGH)) then
  begin

  if TAData.NetworkLayerEnabled then
    ExtensionsNotForDemos := GlobalDPlay.NotViewingRecording
  else
    ExtensionsNotForDemos := True;

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
    ID_TO_STAMP :
      begin
      if arg1 <> 0 then
        result := HiWord(TAUnit.Id2LongId(arg1));
      end;
    OWNED_BY_ALLY :
      begin
      result := BoolValues[TAPlayer.GetAlliedState(TAUnit.GetOwnerPtr(TAUnit.Id2Ptr(arg1)), TAData.LocalPlayerID)];
      end;
    RESOURCES :
      begin
      case arg2 of
        1 : Result := Round(PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).PlayerResources.fCurrentEnergy);
        2 : Result := Round(PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).PlayerResources.fCurrentMetal);
        3 : Result := Round(PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).PlayerResources.fEnergyProduction);
        4 : Result := Round(PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).PlayerResources.fMetalProduction);
        5 : Result := Round(PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).PlayerResources.fEnergyStorageMax);
        6 : Result := Round(PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).PlayerResources.fMetalStorageMax);
      end;
      end;
    CONFIRM_STAMP :
      begin
      result := BoolValues[(HiWord(TAunit.GetLongId(TAUnit.Id2Ptr(arg1))) = arg2)];
      end;
    UNIT_TYPE_CRC :
      begin
      if arg1 <> 0 then
        result := TAMem.Crc32ToCrc24(TAUnit.GetUnitInfoCrc(TAUnit.Id2Ptr(arg1)))
      else
        result := TAMem.Crc32ToCrc24(TAUnit.GetUnitInfoCrc(p_Unit));
      end;
    UNIT_TYPE_CRC_TO_ID :
      begin
      result := TAMem.UnitInfoCrc2Ptr(arg1).nCategory;
      end;
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
      result := Ord(TAPlayer.PlayerSide(TAPlayer.GetPlayerByIndex(arg1)));
      end;
    PLAYER_KILLS :
      begin
      result := PPlayerStruct(TAPlayer.GetPlayerByIndex(arg1)).nKills;
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
      result := TAUnit.GetUnitY(p_Unit);
      end;
    TURNX :
      begin
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
      result := Word(TAUnit.GetTurnY(p_Unit));
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
        result := UNITS_HealUnit(TAUnit.Id2Ptr(arg1), TAUnit.Id2Ptr(arg2), TAUnit.Id2Ptr(arg1).p_UNITINFO.nWorkerTime / 30 );
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
    STATE_UNIT :
      begin
      UNITS_SetStateMask(nil, nil, TAUnit.Id2Ptr(arg1), arg3, arg2);
      end;
    SFX_OCCUPY_STATE :
      begin
        if arg2 = 0 then
          result := TAUnit.Id2Ptr(arg1).lSfxOccupy
        else
          TAUnit.Id2Ptr(arg1).lSfxOccupy := arg2;
      end;
    CUSTOM_BAR_PROGRESS :
      begin
        if arg2 <> 0 then
        begin
          CustomUnitFieldsArr[TAUnit.GetId(p_Unit)].CustomWeapReload := True;
          CustomUnitFieldsArr[TAUnit.GetId(p_Unit)].CustomWeapReloadCur := arg1;
          CustomUnitFieldsArr[TAUnit.GetId(p_Unit)].CustomWeapReloadMax := arg2;
        end else
        begin
          CustomUnitFieldsArr[TAUnit.GetId(p_Unit)].CustomWeapReload := False;
          CustomUnitFieldsArr[TAUnit.GetId(p_Unit)].CustomWeapReloadCur := 0;
          CustomUnitFieldsArr[TAUnit.GetId(p_Unit)].CustomWeapReloadMax := 0;
        end;
      end;
    MEX_RATIO :
      begin
      result := Trunc(p_Unit.fMetalExtrRatio * 100);
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
    UNIT_TYPE_IN_CATEGORY :
      begin
      result := BoolValues[(TAUnit.IsUnitTypeInCategory(TUnitCategories(arg1), TAUnit.GetUnitInfoPtr(p_Unit), TAMem.UnitInfoCrc2Ptr(arg2)))];
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
    UNDER_ATTACK :
      begin
      result := p_Unit.ucRecentDamage;
      end;
    LOCKED_TARGET_ID :
      begin
        b := arg1 - WEAPON_PRIMARY;
        if p_Unit.UnitWeapons[b].nUsedSpot = $8000 then
          Result := p_Unit.UnitWeapons[b].nTargetID
        else
          Result := 0;
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
    WEAPON_PRIMARY..WEAPON_TERTIARY :
      begin
      result := TAUnit.GetWeapon(p_Unit, index);
      end;
    GIVE_UNIT :
      begin
      TAUnits.GiveUnit(TAUnit.Id2Ptr(arg1), arg2);
      end;
    CREATE_UNIT : //
      begin
        if ExtensionsNotForDemos then
        begin
          if GetTPosition(HiWord(arg2), LoWord(arg2), Position) <> nil then
          begin
            UnitinfoSt := TAMem.UnitInfoCrc2Ptr(arg1);
            if (UnitInfoSt.nCruiseAlt <> 0) and (arg4 = 6) then
              Position.Y := GetPosHeight(@Position) + UnitInfoSt.nCruiseAlt - PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.SeaLevel;
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
    UNITS_NEAR :
      begin
      result := TAUnits.SearchUnits(p_Unit, arg3, 2, arg2, TAUnits.CreateSearchFilter(arg1), TAMem.UnitInfoCrc2Ptr(arg4) );
      end;
    UNITS_YARDMAP :
      begin
      result := TAUnits.SearchUnits(p_Unit, arg3, 1, arg2, TAUnits.CreateSearchFilter(arg1), TAMem.UnitInfoCrc2Ptr(arg4) );
      end;
    UNITS_WHOLEMAP :
      begin
      result := TAUnits.SearchUnits(p_Unit, arg3, 4, arg2, TAUnits.CreateSearchFilter(arg1), TAMem.UnitInfoCrc2Ptr(arg4) );
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
      result := TAUnits.SearchUnits(p_Unit, 0, 3, arg2, TAUnits.CreateSearchFilter(arg1), TAMem.UnitInfoCrc2Ptr(arg3) );
      end;
    DISTANCE :
      begin
      if arg2 <> 0 then
        result := LongWord(TAUnits.Distance(@TAUnit.Id2Ptr(arg1).Position, @TAUnit.Id2Ptr(arg2).Position))
      else
        result := LongWord(TAUnits.Distance(@p_Unit.Position, @TAUnit.Id2Ptr(arg1).Position));
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
      result := TAUnit.GetCurrentOrderParams(p_Unit, arg1);
      end;
    EDIT_CURRENT_ORDER_PAR : //
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
          if GetTPosition(HiWord(arg3), LoWord(arg3), Position) <> nil then
            result := TAUnit.CreateMainOrder(p_Unit, nil, TTAActionType(arg1), @Position, arg2, LoWord(arg4), HiWord(arg4));
        end;
      end;
    ORDER_SELF_UNIT_POS : //
      begin
        if ExtensionsNotForDemos then
        begin
          if GetTPosition(HiWord(arg3), LoWord(arg3), Position) <> nil then
            result := TAUnit.CreateMainOrder(p_Unit, TAUnit.Id2Ptr(arg2), TTAActionType(arg1), @Position, LoWord(arg4), HiWord(arg4), 0);
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
          if GetTPosition(HiWord(arg3), LoWord(arg3), Position) <> nil then
            result := TAUnit.CreateMainOrder(TAUnit.Id2Ptr(arg2), nil, TTAActionType(arg1), @Position, LoWord(arg4), HiWord(arg4), 0);
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
    CALL_COB_PROC :
      begin
      result := TAUnit.CallCobWithCallback(TAUnit.Id2Ptr(arg1), GetEnumName(TypeInfo(TCobMethods), Integer(arg2))^, arg3, arg4, 0, 0);
      end;
    LOCAL_SHARED_DATA :
      begin
      if Boolean(arg1) then
        UnitsSharedData[arg2] := arg3
      else
        Result := UnitsSharedData[arg2];
      end;
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
      result := MakeLong(Word(arg1), Word(arg2));
      end;
    TEST_UNLOAD_POS :
      begin
      result := BoolValues[(TAUnit.TestUnloadPosition(TAUnit.GetUnitInfoPtr(TAUnit.Id2Ptr(arg1)), TAUnit.Id2Ptr(arg1).Position))];
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
    MAP_SEA_LEVEL :
      begin
      result := PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.SeaLevel;
      end;
    IS_LAVA_MAP :
      begin
      result := PMapOTAFile(PTAdynmemStruct(TAData.MainStructPtr).p_MapOTAFile).lIsLavaMap;
      end;
    SURFACE_METAL :
      begin
      if arg1 <> 0 then
        PMapOTAFile(PTAdynmemStruct(TAData.MainStructPtr).p_MapOTAFile).lSurfaceMetal := arg1;
      result := PMapOTAFile(PTAdynmemStruct(TAData.MainStructPtr).p_MapOTAFile).lSurfaceMetal;
      end;
    UNIT_IN_PLAYER_LOS :
      begin
      if arg1 <> 0 then
        if TAUnit.Id2Ptr(arg1).p_Owner <> nil then
          result := UnitInPlayerLOS(TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID), TAUnit.Id2Ptr(arg1));
      end;
    POSITION_IN_PLAYER_LOS :
      begin
      if GetTPosition(HiWord(arg2), LoWord(arg2), Position) <> nil then
        result := BoolValues[TAMap.PositionInLOS(TAPlayer.GetPlayerByIndex(arg1), @Position)];
      end;
    UNIT_AT_POSITION :
      begin
      if GetTPosition(HiWord(arg1), LoWord(arg1), Position) <> nil then
        result := TAUnit.AtPosition(@Position);
      end;
    DBG_OUTPUT :
      begin
      SendTextLocal(IntToStr(TAUnit.GetId(p_Unit)) + ' DBG_OUTPUT: [' + IntToStr(arg1) +'] + Value: ' + IntToStr(arg2) + ', Hex: ' + IntToHex(arg2, 8));
      end;
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
    ENABLEDISABLE_UNIT :
      begin
        if TAUnit.IsOnThisComp(p_Unit, False) then
        begin
          if (arg2 = 0) and (TAMem.UnitInfoCrc2Ptr(arg1).nCategory = 0) then
            Exit;
          if (arg2 <> 0) and (TAMem.UnitInfoCrc2Ptr(arg1).nCategory <> 0) then
            Exit;
          TAMem.ProtectMemoryRegion(Cardinal(TAData.UnitInfosPtr), True);
          if arg2 = 0 then
          begin
            Result := TAMem.UnitInfoCrc2Ptr(arg1).nCategory;
            TAMem.UnitInfoCrc2Ptr(arg1).nCategory := 0;
          end else
            TAMem.UnitInfoCrc2Ptr(arg1).nCategory := arg3;
          TAMem.ProtectMemoryRegion(Cardinal(TAData.UnitInfosPtr), False);
        end;
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
    PLAY_3D_SOUND : //
      begin
      if ExtensionsNotForDemos then
      begin
        if GetTPosition(HiWord(arg2), LoWord(arg2), Position) <> nil then
          result := TASfx.Play3DSound(arg1, Position, arg3 = 1);
      end;
      end;
    PLAY_GAF_ANIM :
      begin
      result := TASfx.PlayGafAnim(arg1, HiWord(arg2), LoWord(arg2), arg3, arg4);
      end;
    EMIT_SFX :
      begin
      Result := BoolValues[TASfx.EmitSfxFromPiece(p_Unit, TAUnit.Id2Ptr(arg3), arg1, arg2, p_Unit.ucOwnerID = TAData.LocalPlayerID) <> 0];
      end;
    MS_MOVE_CAM_POS :
      begin
      ScrollView(arg1, arg2, Boolean(arg3));
      end;
    MS_MOVE_CAM_UNIT :
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
      result := PTAdynmemStruct(TAData.MainStructPtr).Gamma;
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
        GetTPosition(arg2, arg3, Position);
        case arg1 of
          0 : EmitSfx_SmokeInfinite(@Position, 4);
          1 : EmitSfx_GraySmoke(@Position, 9);
          2 : EmitSfx_BlackSmoke(@Position, 9);
          3 : ;
          4 : ;
          5 : ;
        end;
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
                                       PAnsiChar(MapMissionsUnitsInitialMissions[arg2]),
                                       nil);
      end;
    MS_PLACE_FEATURE :
      begin
      GetTPosition(arg2, arg3, Position);
      Turn.Z := arg4;
      Result := BoolValues[TAMap.PlaceFeatureOnMap(MapMissionsFeatures[arg1], Position, Turn)];
      end;
    MS_REMOVE_FEATURE :
      begin
      Result := BoolValues[TAMap.RemoveMapFeature(arg1, arg2, (arg3 = 1))];
      end;
    MS_FIRE_MAP_WEAPON :
      begin
      TAWeapon.FireMap_Weapon(TAWeapon.WeaponId2Ptr(arg1), HiWord(arg2), LoWord(arg2));
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

procedure CustomSetters( index: longword; p_Unit : PUnitStruct; arg1: longword); stdcall;
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
        PTAdynmemStruct(TAData.MainStructPtr).Gamma := arg1;
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
// arg_4 
// arg_8
// arg_C
// arg_10

label
  DefaultCase,
  DoReturn,
  GeneralCaseGetter;
asm
  // function prolog
  mov     ecx, [esp+4]//[esp+GetterIdx]
  sub     esp, 24h
  push    esi
  // start of case statement
  mov     esi, [eax+0Ch];
  mov     esi, [eax+0Ch]
  lea     eax, [ecx-1]    // switch 20 cases

  cmp     eax, 13h
  ja      DefaultCase
  jmp     ds:$480AC4[eax*4] // switch jump
  // should not get here
  int 3

DefaultCase:

  inc eax;
{
  cmp eax, UNIT_BUILD_PERCENT_LEFT
  jnz GeneralCaseGetter

  mov ecx,[TADynmemStructPtr]
  mov esi,[esp+28h+$8]
  imul esi, type TUnitStruct
  mov eax, [ecx+TTADynMemStruct.p_Units]
  add esi, eax

  cmp esi, eax
  jb DoReturn

  mov eax, [ecx+TTADynMemStruct.p_LastUnitInArray]
  cmp esi, eax
  jae DoReturn

  push $480A44
  Call PatchNJump;
}
GeneralCaseGetter:
  mov ecx, [esp+28h+$14] // arg_14
  push ecx;
  mov ecx, [esp+2ch+$10] // arg_10
  push ecx;
  mov ecx, [esp+30h+$C] // arg_C
  push ecx;
  mov ecx, [esp+34h+$8] // arg_8
  push ecx;
  push esi;             // unitptr
  push eax;             // index
  // todo : fix this mess up
  call CustomGetters;

DoReturn:
  pop esi;
  add esp, 24h;
  ret 14h;  
end;

Procedure COB_ExtensionsSetters_Handling;

label
  DoReturn,
  GeneralCaseSetter;
asm
  // function prolog
  mov     ecx, [esp+4]//[esp+SetterIdx]
  push    esi
  // start of case statement
  mov     esi, [eax+0Ch];
  mov     esi, [eax+0Ch]
  lea     eax, [ecx-1]    // switch 20 cases
  cmp     eax, 13h
  ja      GeneralCaseSetter

  xor edx, edx
  mov dl, ds:$480C18[eax]
  jmp     ds:$480BFC[edx*4] // switch jump
  // should not get here
  int 3
  
GeneralCaseSetter:
  inc eax;
  mov ecx, [esp+4h+$8] // value to be set
  push ecx;
  push esi;             // unitPtr
  push eax;             // index
  call CustomSetters;

DoReturn:
  pop esi;
  ret 8h;
end;

end.