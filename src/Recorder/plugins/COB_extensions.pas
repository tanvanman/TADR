unit COB_extensions;

interface
uses
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
 WEAPON_LAUNCH_NOW = 23;
 ORIENTATION = 27;
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
 // returns team of unit given with parameter
 UNIT_TEAM = 72;
 // basically BUILD_PERCENT_LEFT, but comes with a unit parameter
 UNIT_BUILD_PERCENT_LEFT = 73;
 // is unit given with parameter allied to the unit of the current COB script. 1=allied, 0=not allied
 UNIT_ALLIED = 74;
 // indicates if the 1st parameter(a unit ID) is local to this computer
 UNIT_IS_ON_THIS_COMP = 75;
 // returns local long id of unit
 UNIT_ID_TO_LONGID = 76;
 // does unit exist
 UNIT_EXIST = 77;
 //
 UNIT_OWNED_BY_ALLY = 78;
 //CONFIRM_LONG_ID = 78;
 // GET/SET returns or set unit type (don't affect COB, model, weapons...)
 UNIT_TYPE = 79;

 { Some specific }
 UNITX = 80;
 UNITZ = 81;
 UNITY = 82;
 TURNX = 83;
 TURNZ = 84;
 TURNY = 85;
 HEALTH_VAL = 86;
 GET_CLOAKED = 87;
 SET_CLOAKED = 88;
 SELECTABLE = 89;
 MOBILE_PLANT = 90;

 { Weapons, attack info }
 KILLS = 91;
 ATTACKER_ID = 92;
 UNDER_ATTACK = 93;
 WEAPON_PRIMARY = 94;
 WEAPON_SECONDARY = 95;
 WEAPON_TERTIARY = 96;
 LOCKED_TARGET = 97;

 { Creating and killing }
 TEST_BUILD_SPOT = 100;
 CREATE_UNIT = 101;
 KILL = 102;
 MINIONS = 103;
 SWAP_TYPE = 104;

 { Searching for units }
 UNITS_NEAR = 110;
 UNITS_YARDMAP = 111;
 UNITS_WHOLEMAP = 112;
 UNITS_ARRAY_RESULT = 113;
 RANDOMIZE_UNITS_ARRAY = 114;
 FREE_ARRAY_ID = 115;
 CLEAR_ARRAY_ID = 116;
 UNIT_NEAREST = 117;
 DISTANCE = 118;

 { Unit orders }
 CURRENT_ORDER = 120;
 GET_CURRENT_ORDER_PAR = 121;
 EDIT_CURRENT_ORDER = 122;
 CREATE_ORDER_SELF = 123;
 CREATE_ORDER_SELF_POS = 124;
 CREATE_ORDER_UNIT_UNIT = 125;
 CREATE_ORDER_UNIT_POS = 126;
 CREATE_ORDER_SELF_UNIT_POS = 127;
 RESET_ORDER = 128;
 
 { Sfx }
 SPEECH = 130;
 SOUND_EFFECT = 131;
 PLAY_GAF_ANIM = 132;

 { Global unit template }
 GRANT_UNITINFO = 140;
 GET_UNITINFO = 141;
 SET_UNITINFO = 142;

 { Math }
 LOWWORD = 150;
 HIGHWORD = 151;
 MAKEDWORD = 152;

 { Map }
 MAP_SEA_LEVEL = 155;
 MAP_HEIGHT_VAL = 156;

 { Other }
 CALL_COB_PROC = 160;

 DBG_OUTPUT = 170;

 CUSTOM_LOW = WEAPON_AIM_ABORTED;
 CUSTOM_HIGH = DBG_OUTPUT;

// -----------------------------------------------------------------------------

var
  CustomUnitInfosArray : TUnitInfos;
  UnitSearchArr : TUnitSearchArr;
  SpawnedMinionsArr : TSpawnedMinionsArr;
  CustomUnitInfos, UnitSearchResults,
  SpawnedMinions : TDynArray;
  CustomUnitInfosCount, UnitSearchCount,
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
  TA_FunctionsU,
  SysUtils,
  IniOptions;

Procedure OnInstallCobExtensions;
var
  i: LongWord;
  UnitRec: TStoreUnitsRec;
begin
  CustomUnitInfos.Init(TypeInfo(TUnitInfos),CustomUnitInfosArray, @CustomUnitInfosCount);
  UnitSearchResults.Init(TypeInfo(TUnitSearchArr), UnitSearchArr, @UnitSearchCount);
  SpawnedMinions.Init(TypeInfo(TSpawnedMinionsArr), SpawnedMinionsArr, @SpawnedMinionsCount);
  CustomUnitInfos.Capacity := IniSettings.UnitLimit * MAXPLAYERCOUNT;
  UnitSearchResults.Capacity := High(Word);
  SpawnedMinions.Capacity := High(Word);
  for i := 0 to High(Word) - 1 do
  begin
    UnitSearchResults.Add(UnitRec);
    SpawnedMinions.Add(UnitRec);
  end;
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
                          $480770,
                          1 );

  result.MakeRelativeJmp( State_COB_extensions,
                          'COB Extensions Setters handler',
                          @COB_ExtensionsSetters_Handling,
                          $480B20,
                          1 );

  end
else
  result := nil;
end;

function CustomGetters( index : longword;
                        unitPtr : longword;
                        arg1, arg2, arg3, arg4 : longword) : longword; stdcall;
var
  pUnit : Pointer;
  Position : TPosition;
  i : integer;
  ExtensionsNotForDemos : Boolean;
begin
result := 0;
if ((index >= CUSTOM_LOW) and (index <= CUSTOM_HIGH)) then
  begin

  if Assigned(globalDplay) then
    ExtensionsNotForDemos := globalDplay.NotViewingRecording
  else
    ExtensionsNotForDemos := True;

  case index of
    CURRENT_SPEED :
      begin
      result := TAUnit.GetCurrentSpeed(pointer(unitptr));
      end;
    VETERAN_LEVEL :
      begin
      result := TAUnit.getKills(pointer(unitptr)) * 100;
      end;
    MIN_ID :
      begin
      result := 1;
      end;
    MAX_ID :
      begin
      result := TAMem.GetMaxUnitId;
      end;
    MY_ID :
      begin
      result := TAUnit.GetId(pointer(unitptr));
      end;
    UNIT_TEAM :
      begin
      result := TAUnit.Team(arg1);
      end;
    UNIT_BUILD_PERCENT_LEFT :
      begin
      if arg1 <> 0 then
        result := Trunc(TAUnit.GetBuildTimeLeft(TAUnit.Id2Ptr(arg1)) * 100)
      else
        result := Trunc(TAUnit.GetBuildTimeLeft(pointer(unitptr)) * 100);
      end;
    UNIT_ALLIED :
      begin
      result := TAUnit.IsAllied(pointer(unitptr), arg1);
      end;
    UNIT_IS_ON_THIS_COMP :
      begin
      if TAUnit.IsOnThisComp(TAUnit.Id2Ptr(arg1), True) then
        result := 1;
      end;
    UNIT_ID_TO_LONGID :
      begin
      if arg1 <> 0 then
        result := TAUnit.Id2LongId(arg1)
      else
        result := TAUnit.GetLongId(pointer(unitptr));
      end;
    UNIT_EXIST :
      begin
      result := BoolValues[(PUnitStruct(TAUnit.Id2Ptr(arg1)).nUnitCategoryID <> 0)];
      end;
 {   CONFIRM_LONG_ID :
      begin
      result := BoolValues[(TAunit.GetLongId(TAUnit.Id2Ptr(arg1)) = arg1)];
      end;  }
    UNIT_OWNED_BY_ALLY :
      begin
      result := TAUnit.IsFromAlly(pointer(unitptr));
      end;
    UNIT_TYPE :
      begin
      if arg1 <> 0 then
        result := TAUnit.GetUnitInfoId(TAUnit.Id2Ptr(arg1))
      else
        result := TAUnit.GetUnitInfoId(pointer(unitptr));
      end;
    UNITX :
      begin
      result := TAUnit.GetUnitX(pointer(unitptr));
      end;
    UNITZ:
      begin
      result := TAUnit.GetUnitZ(pointer(unitptr));
      end;
    UNITY :
      begin
      result := TAUnit.GetUnitY(pointer(unitptr));
      end;
    TURNX :
      begin
      result := Word(TAUnit.GetTurnX(pointer(unitptr)));
      end;
    TURNZ :
      begin
      result := Word(TAUnit.GetTurnZ(pointer(unitptr)));
      end;
    TURNY :
      begin
      result := Word(TAUnit.GetTurnY(pointer(unitptr)));
      end;
    HEALTH_VAL :
      begin
      if arg1 <> 0 then
        result := TAUnit.GetHealth(TAUnit.Id2Ptr(arg1))
      else
        result := TAUnit.GetHealth(pointer(unitptr));
      end;
    GET_CLOAKED :
      begin
      if arg1 <> 0 then
        result := TAUnit.GetCloak(TAUnit.Id2Ptr(arg1))
      else
        result := TAUnit.GetCloak(pointer(unitptr));
      end;
    SET_CLOAKED : //
      begin
      if ExtensionsNotForDemos then
      if arg2 <> 0 then
        TAUnit.SetCloak(TAUnit.Id2Ptr(arg2), arg1)
      else
        TAUnit.SetCloak(pointer(unitptr), arg1);
      end;
    KILLS :
      begin
      if arg1 <> 0 then
        result := TAUnit.GetKills(TAUnit.Id2Ptr(arg1))
      else
        result := TAUnit.GetKills(pointer(unitptr));
      end;
    ATTACKER_ID :
      begin
      result := TAUnit.GetAttackerID(pointer(unitptr));
      end;
    UNDER_ATTACK :
      begin
      result := PUnitStruct(pointer(unitptr)).ucRecentDamage;
      end;
    LOCKED_TARGET :
      begin
      if Byte(PUnitStruct(pointer(unitptr)).lUsedSpot) and $80 = $80 then
        result := LoWord(PUnitStruct(pointer(unitptr)).lUsedSpot);
      end;
    WEAPON_PRIMARY..WEAPON_TERTIARY :
      begin
      result := TAUnit.GetWeapon(pointer(UnitPtr), index);
      end;
    TEST_BUILD_SPOT :
      begin
      result := BoolValues[(TAunit.TestBuildSpot(TAUnit.GetOwnerIndexBuildspotSafe(pointer(UnitPtr)), arg1, HiWord(arg2), LoWord(arg2)) = True)];
      end;
    CREATE_UNIT : //
      begin
        if ExtensionsNotForDemos then
        begin
          Position := TAUnit.CreatePositionOfCoords(arg2, arg3, 0);
          Position.Y := GetPosHeight(@Position);
          pUnit := TAUnit.CreateUnit(TAUnit.GetOwnerIndexBuildspotSafe(pointer(UnitPtr)), arg1, Position , nil, False, False, 1, arg4);
          if pUnit <> nil then
            result := Word(PUnitStruct(pUnit).lUnitInGameIndex);
        end;
      end;
    KILL : //
      begin
      if ExtensionsNotForDemos then
        TAUnit.Kill(TAUnit.Id2Ptr(arg1), arg2);
      end;
    MINIONS : //
      begin
      if ExtensionsNotForDemos then
        result := TAUnits.CreateMinions(pointer(unitptr), arg2, MinionsPattern_Random, arg1, TTAActionType(arg3), arg4);
      end;
    UNITS_NEAR :
      begin
      result := TAUnits.SearchUnits(pointer(unitptr), arg3, 2, arg2, TAUnits.CreateSearchFilter(arg1), arg4 );
      end;
    UNITS_YARDMAP :
      begin
      result := TAUnits.SearchUnits(pointer(unitptr), arg3, 1, arg2, TAUnits.CreateSearchFilter(arg1), arg4 );
      end;
    UNITS_WHOLEMAP :
      begin
      result := TAUnits.SearchUnits(pointer(unitptr), arg3, 4, arg2, TAUnits.CreateSearchFilter(arg1), arg4 );
      end;
    UNITS_ARRAY_RESULT :
      begin
      case arg3 of
        1 : begin
            if Assigned(UnitSearchArr[arg1].UnitIds) then
              result:= UnitSearchArr[arg1].UnitIds[arg2 - 1];
            end;
        2 : begin
            if Assigned(SpawnedMinionsArr[arg1].UnitIds) then
              result := SpawnedMinionsArr[arg1].UnitIds[arg2 - 1];
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
      result := TAUnits.SearchUnits(pointer(unitptr), 0, 3, arg2, TAUnits.CreateSearchFilter(arg1), arg3 );
      end;
    DISTANCE :
      begin
      if arg2 <> 0 then
        result := LongWord(TAUnits.Distance(arg1, arg2))
      else
        result := LongWord(TAUnits.Distance(TAUnit.GetLongId(Pointer(UnitPtr)), arg2));
      end;
    CURRENT_ORDER :
      begin
      if arg1 <> 0 then
        result := Ord(TAUnit.GetCurrentOrder(TAUnit.Id2Ptr(arg1)))
      else
        result := Ord(TAUnit.GetCurrentOrder(pointer(unitptr)));
      end;
    GET_CURRENT_ORDER_PAR :
      begin
      result := TAUnit.GetCurrentOrderParams(pointer(unitptr), arg1);
      end;
    EDIT_CURRENT_ORDER : //
      begin
      if ExtensionsNotForDemos then
        result := BoolValues[(TAUnit.EditCurrentOrderParams(pointer(unitptr), arg1, arg2)) = True];
      end;
    CREATE_ORDER_SELF : //
      begin
      if ExtensionsNotForDemos then
        result := TAUnit.CreateOrder(pointer(unitptr), nil, TTAActionType(arg1), nil, arg2, arg3, arg4);
      end;
    CREATE_ORDER_SELF_POS : //
      begin
        if ExtensionsNotForDemos then
        begin
          Position := TAUnit.CreatePositionOfCoords(HiWord(arg3), LoWord(arg3), 0);
          Position.Y := GetPosHeight(@Position);
          result := TAUnit.CreateOrder(pointer(unitptr), nil, TTAActionType(arg1), @Position, arg2, LoWord(arg4), HiWord(arg4));
        end;
      end;
    CREATE_ORDER_SELF_UNIT_POS : //
      begin
        if ExtensionsNotForDemos then
        begin
          Position := TAUnit.CreatePositionOfCoords(HiWord(arg3), LoWord(arg3), 0);
          Position.Y := GetPosHeight(@Position);
          result := TAUnit.CreateOrder(pointer(unitptr), TAUnit.Id2Ptr(arg2), TTAActionType(arg1), @Position, LoWord(arg4), HiWord(arg4), 0);
        end;
      end;
    CREATE_ORDER_UNIT_UNIT : //
      begin
      if ExtensionsNotForDemos then
        result := TAUnit.CreateOrder(TAUnit.Id2Ptr(arg3), TAUnit.Id2Ptr(arg2), TTAActionType(arg1), nil, LoWord(arg4), HiWord(arg4), 0);
      end;
    CREATE_ORDER_UNIT_POS : //
      begin
        if ExtensionsNotForDemos then
        begin
          Position := TAUnit.CreatePositionOfCoords(HiWord(arg3), LoWord(arg3), 0);
          Position.Y := GetPosHeight(@Position);
          result := TAUnit.CreateOrder(TAUnit.Id2Ptr(arg2), nil, TTAActionType(arg1), @Position, LoWord(arg4), HiWord(arg4), 0);
        end;
      end;
    RESET_ORDER :
      begin
      PUnitStruct(pointer(unitptr)).p_UnitOrders := nil;
      end;
    CALL_COB_PROC :
      begin
      result := TAUnit.CallCobWithCallback(TAUnit.Id2Ptr(arg1), GetEnumName(TypeInfo(TCobMethods), Integer(arg2))^, arg3, arg4, 0, 0);
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
    MAP_SEA_LEVEL :
      begin
      result := PTAdynmemStruct(TAData.MainStructPtr)^.SeaLevel;
      end;
    MAP_HEIGHT_VAL :
      begin
      Position := TAUnit.CreatePositionOfCoords(HiWord(arg1), LoWord(arg1), 0);
      i := GetPosHeight(@Position);
      if i <> -1 then
        result := i
      else
        result := High(Cardinal);
      end; 
    DBG_OUTPUT :
      begin
      SendTextLocal(IntToStr(TAUnit.GetId(pointer(unitptr))) + ' DBG_OUTPUT: [' + IntToStr(arg1) +'] + Value: ' + IntToStr(arg2) + ', Hex: ' + IntToHex(arg2, 8));
      end;
    GET_UNITINFO :
      begin
      if arg2 <> 0 then
        result := TAUnit.getUnitInfoField(TAUnit.Id2Ptr(arg2), TUnitInfoExtensions(arg1), nil)
      else
        result := TAUnit.getUnitInfoField(pointer(unitptr), TUnitInfoExtensions(arg1), nil);
      end;
    SET_UNITINFO : //
      begin
      if ExtensionsNotForDemos then
        if TAUnit.setUnitInfoField(pointer(unitptr), TUnitInfoExtensions(arg1), arg2, nil) then
          if Assigned(globalDplay) then
            globalDplay.SendCobEventMessage(TANM_Rec2Rec_UnitInfoEdit, @unitptr, @arg1, nil, nil, @index);
      end;
    SELECTABLE :
      begin
      result := BoolValues[(PUnitStruct(pointer(unitptr)).lUnitStateMask and 32 = 32)];
      end;
    PLAY_GAF_ANIM :
      begin
      result := TAunit.PlayGafAnim(arg1, HiWord(arg2), LoWord(arg2), arg3, arg4);
      end;
    end;
  end;
end;

procedure CustomSetters( index: longword; unitptr : longword; arg1: longword); stdcall;
var
  ExtensionsNotForDemos: Boolean;
begin
  if ((index >= CUSTOM_LOW) and (index <= CUSTOM_HIGH)) then
  begin
    if TAUnit.IsOnThisComp(pointer(unitptr), True) then
    begin
      case index of
        WEAPON_AIM_ABORTED :
          begin
          PUnitStruct(pointer(unitptr)).cWeaponStateMask := PUnitStruct(pointer(unitptr)).cWeaponStateMask and not $11;
          PUnitStruct(pointer(unitptr)).lUsedSpot := Cardinal(MakeLong(0, HiWord(PUnitStruct(pointer(unitptr)).lUsedSpot)));
          end;
        WEAPON_LAUNCH_NOW :
          begin
          PUnitStruct(pointer(unitptr)).cWeaponStateMask := PUnitStruct(pointer(unitptr)).cWeaponStateMask or $10;
          end;
        SPEECH : //
          begin
          TAUnit.Speech(unitptr, arg1, nil);
          end;
        SOUND_EFFECT : //
          begin
          TAUnit.SoundEffectId(unitptr, arg1);
          end;
        MOBILE_PLANT : //
          begin
          if arg1 = 1 then
          begin
            PUnitStruct(pointer(unitptr)).lUnitStateMask:= PUnitStruct(pointer(unitptr)).lUnitStateMask or $20000000;
            PUnitStruct(pointer(unitptr)).nUnitStateMaskBas := PUnitStruct(pointer(unitptr)).nUnitStateMaskBas and not 1;
          end else
          begin
            PUnitStruct(pointer(unitptr)).lUnitStateMask:= PUnitStruct(pointer(unitptr)).lUnitStateMask and not $20000000;
            PUnitStruct(pointer(unitptr)).nUnitStateMaskBas := PUnitStruct(pointer(unitptr)).nUnitStateMaskBas or 1;
          end;
          end;
        SELECTABLE : //
          begin
          if arg1 = 1 then
            PUnitStruct(pointer(unitptr)).lUnitStateMask := PUnitStruct(pointer(unitptr)).lUnitStateMask or 32
          else
            PUnitStruct(pointer(unitptr)).lUnitStateMask := PUnitStruct(pointer(unitptr)).lUnitStateMask and not 32;
          end;
      end;

      if Assigned(globalDplay) then
        ExtensionsNotForDemos := globalDplay.NotViewingRecording
      else
        ExtensionsNotForDemos := True;

      if ExtensionsNotForDemos then
      begin
        case index of
          UNITX:
            begin
            TAUnit.SetUnitX(pointer(unitptr), arg1);
            end;
          UNITZ:
            begin
            TAUnit.SetUnitZ(pointer(unitptr), arg1);
            end;
          UNITY:
            begin
            TAUnit.SetUnitY(pointer(unitptr), arg1);
            end;
          TURNX:
            begin
            TAUnit.setTurnX(pointer(unitptr), arg1);
            end;
          TURNY:
            begin
            TAUnit.setTurnY(pointer(unitptr), arg1);
            end;
          TURNZ:
            begin
            TAUnit.setTurnZ(pointer(unitptr), arg1);
            end;
          HEALTH_VAL :
            begin
            TAUnit.setHealth(pointer(unitptr), arg1);
            end;
          SWAP_TYPE :
            begin
            TAUnit.SwapByKill(pointer(unitptr), arg1);
            end;
          WEAPON_PRIMARY..WEAPON_TERTIARY :
            begin
            if TAUnit.setWeapon(pointer(unitptr), index, Word(arg1)) then
              if Assigned(globalDplay) then
                globalDplay.SendCobEventMessage(TANM_Rec2Rec_UnitWeapon, @unitptr, @index, nil, @arg1, nil);
            end;
          GRANT_UNITINFO :
            begin
            if TAUnit.GrantUnitInfo(pointer(unitptr), arg1, nil) then
              if Assigned(globalDplay) then
                globalDplay.SendCobEventMessage(TANM_Rec2Rec_UnitGrantUnitInfo, @unitptr, nil, @arg1, nil, nil);
            end;
          UNIT_TYPE :
            begin
            if TAUnit.setTemplate(pointer(unitptr), arg1) then
              if Assigned(globalDplay) then
                globalDplay.SendCobEventMessage(TANM_Rec2Rec_UnitTemplate, @unitptr, @index, nil, @arg1, nil);
            end;
        end;
      end;
    end;
  end;
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
  mov     eax, [ecx+540h]
  mov     ecx, [esp+4]//[esp+arg_0]
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
  cmp eax, UNIT_BUILD_PERCENT_LEFT
  jnz GeneralCaseGetter

  mov ecx,[TADynmemStructPtr]
  mov esi,[esp+28h+$8]
  imul esi, UnitStructSize
  mov eax, [ecx+TAdynmemStruct_Units]
  add esi, eax

  cmp esi, eax
  jb DoReturn

  mov eax, [ecx+TAdynmemStruct_Units_EndMarker]
  cmp esi, eax
  jae DoReturn  

  push $480A44
  Call PatchNJump;

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
  DefaultCase,
  DoReturn,
  GeneralCaseSetter;
asm
  // function prolog
  mov     eax, [ecx+540h]
  mov     ecx, [esp+4]//[esp+arg_0]
  push    esi
  // start of case statement
  mov     esi, [eax+0Ch];
  mov     esi, [eax+0Ch]
  lea     eax, [ecx-1]    // switch 20 cases
  cmp     eax, 13h
  ja      DefaultCase

  xor edx, edx
  mov dl, ds:$480C18[eax]
  jmp     ds:$480BFC[edx*4] // switch jump
  // should not get here
  int 3
  
DefaultCase:
  inc eax;
  cmp eax, UNIT_BUILD_PERCENT_LEFT
  jnz GeneralCaseSetter

  mov ecx,[TADynmemStructPtr]
  mov esi,[esp+28h+$8]
  imul esi, UnitStructSize
  mov eax, [ecx+TAdynmemStruct_Units]
  add esi, eax

  cmp esi, eax
  jb DoReturn

  mov eax, [ecx+TAdynmemStruct_Units_EndMarker]
  cmp esi, eax
  jae DoReturn  

  push $480BF1
  Call PatchNJump;

GeneralCaseSetter:
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
