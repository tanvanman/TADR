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
 CONFIRM_LONG_ID = 78;
 // GET/SET returns or set unit type (don't affect COB, model, weapons...)
 UNIT_TYPE = 79;

 { Some specific }
 UNITX = 81;
 UNITY = 82;
 UNITZ = 83;
 TURNX = 84;
 TURNY = 85;
 TURNZ = 86;
 HEALTH_VAL = 87;
 CLOAKED = 88;

 { Weapons }
 WEAPON1 = 110;
 WEAPON2 = 111;
 WEAPON3 = 112;

 { Searching for units }
 UNITS_NEAR = 135;
 UNITS_YARDMAP = 136;
 UNITS_WHOLEMAP = 137;
 UNIT_NEAREST = 138;
 UNITS_ARRAY_RESULT = 140;
 FREE_ARRAY_ID = 141;
 CLEAR_ARRAY_ID = 142;

 { Unit orders }
 CURRENTORDER = 160;
 CREATE_ORDER_SELF = 161;
 CREATE_ORDER_SELF_POS = 162;
 CREATE_ORDER_SELF_UNIT = 163;
 CREATE_ORDER_SELF_UNIT_POS = 164;
 CREATE_ORDER_UNIT_UNIT = 165;
 CREATE_ORDER_UNIT_SELF = 166;
 CREATE_ORDER_UNIT_POS = 167;
 TELEPORT = 168;

 { Units creation, killing }
 TEST_BUILD_SPOT = 185;
 CREATE_UNIT = 186;
 KILL = 187;
 MINIONS = 188;
 SWAP_TYPE = 189;

 { Other }
 // gets kills * 100
 VETERAN_LEVEL = 32;
 SELECTABLE = 210;

 { Sounds }
 SPEECH = 235;
 SOUND_EFFECT = 236;

 { Math }
 MAKE_LONG = 260;

 // all stuff below requires upgradeable 1
 // so keep it in the same range
 UPGRADEABLE = 285;

 // fbi masks, might be expanded so please leave some index margin before
 BUILDER = 310;
 FLOATER = 311;
 AMPHIBIOUS = 312;
 STEALTH = 313;
 ISAIRBASE = 314;
 TARGETTINGUPGRADE = 315;
 TELEPORTER = 316;
 HIDEDAMAGE = 317;
 SHOOTME = 318;
 CANFLY = 319;
 CANHOVER = 320;
 IMMUNETOPARALYZER = 321;
 HOVERATTACK = 322;
 ANTIWEAPONS = 323;
 DIGGER = 324;
 ONOFFABLE = 325;
 CANSTOP = 326;
 CANATTACK = 327;
 CANGUARD = 328;
 CANPATROL = 329;
 CANMOVE = 330;
 CANLOAD = 331;
 CANRECLAMATE = 332;
 CANRESURRECT = 333;
 CANCAPTURE = 334;
 CANDGUN = 335;
 KAMIKAZE = 336;
 COMMANDER = 337;
 SHOWPLAYERNAME = 338;
 CANTBERANSPORTED = 339;

 //NAME            = 363; //string
 //UNITNAME        = 364; //string
 //DESCRIPTION     = 365; //string
 //CATEGORY        = 367; //string

 // global template, make it one range
 SOUNDCTGR = 388;

 MOVEMENTCLASS_SAFE = 389;
 MOVEMENTCLASS = 390;

 MAXHEALTH = 391;
 HEALTIME        = 392;

 MAXSPEED        = 393;
 ACCELERATION    = 394;
 BRAKERATE       = 395;
 TURNRATE        = 396;
 CRUISEALT       = 397;
 MANEUVERLEASH   = 398;
 ATTACKRUNLEN    = 399;
 MAXWATERDEPTH   = 400;
 MINWATERDEPTH   = 401;
 MAXSLOPE        = 402;
 MAXWATERSLOPE   = 403;
 WATERLINE       = 404;

 TRANSPORTSIZE   = 405;
 TRANSPORTCAP    = 406;

 BANKSCALE       = 407;
 KAMIKAZEDIST    = 408;
 DAMAGEMODIFIER  = 409;

 WORKERTIME      = 410;
 BUILDDIST       = 411;

 SIGHTDIST       = 412;
 RADARDIST       = 413;
 SONARDIST       = 414;
 MINCLOAKDIST    = 415;
 RADARDISTJAM    = 416;
 SONARDISTJAM    = 417;

 MAKESMETAL      = 418;
 FENERGYMAKE     = 419; //Multiply (when setting) or divide by 100 after getting
 FMETALMAKE      = 420;
 FENERGYUSE      = 421;
 FMETALUSE       = 422;
 FENERGYSTOR     = 423;
 FMETALSTOR      = 424;
 FWINDGENERATOR  = 425;
 FTIDALGENERATOR = 426;
 FCLOAKCOST      = 427;
 FCLOAKCOSTMOVE  = 428; //end multiply

 EXPLODEAS       = 429;
 SELFDSTRAS      = 430;

 CUSTOM_LOW = MIN_ID;
 CUSTOM_HIGH = SELFDSTRAS;

// -----------------------------------------------------------------------------

var
  CustomUnitInfosArray: TUnitInfos;
  UnitSearchArr: TUnitSearchArr;
  SpawnedMinionsArr: TSpawnedMinionsArr;
  CustomUnitInfos, UnitSearchResults, SpawnedMinions: TDynArray;
  CustomUnitInfosCount, UnitSearchCount, SpawnedMinionsCount: Integer;

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
  CustomUnitInfos.Capacity:= 1500 * MAXPLAYERCOUNT;
  UnitSearchResults.Capacity:= High(Word);
  SpawnedMinions.Capacity:= High(Word);
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
                          $1 );

  result.MakeRelativeJmp( State_COB_extensions,
                          'COB Extensions Setters handler',
                          @COB_ExtensionsSetters_Handling,
                          $480B20,
                          $1 );


  end
else
  result := nil;
end;

function CustomGetters( index : longword;
                        unitPtr : longword;
                        arg1, arg2, arg3, arg4 : longword) : longword; stdcall;
var
  Position: TPosition;
  pUnit: Pointer;
begin
result := 0;

if (index = VETERAN_LEVEL) or ((index >= CUSTOM_LOW) and (index <= CUSTOM_HIGH)) then
  begin
  case index of
    MIN_ID :
      begin
      result := 1;
      end;
    MAX_ID :
      begin
      result := TAMem.GetMaxUnitId;
      end;
    MY_ID :
      result := TAUnit.GetId(pointer(unitptr));
    UNIT_TEAM :
      begin
      result:= TAUnit.Team(arg1);
      end;
    UNIT_ALLIED :
      begin
      result:= TAUnit.IsAllied(pointer(unitptr), arg1);
      end;
    UNIT_ID_TO_LONGID :
      begin
      if arg1 <> 0 then result := TAUnit.Id2LongId(arg1)
        else result := TAUnit.GetLongId(pointer(unitptr));
      end;
    CONFIRM_LONG_ID :
      begin
      result:= BoolValues[(TAunit.GetLongId(TAUnit.Id2Ptr(arg1)) = arg1)];
      end;
    UNIT_EXIST :
      begin
      result:= BoolValues[(PUnitStruct(TAUnit.Id2Ptr(arg1)).nUnitCategoryID <> 0)];
      end;
    UNIT_TYPE :
      begin
      if arg1 <> 0 then result := TAUnit.GetUnitInfoId(TAUnit.Id2Ptr(arg1))
        else result := TAUnit.GetUnitInfoId(pointer(unitptr));
      end;
    CURRENTORDER :
      begin
      if arg1 <> 0 then result := Ord(TAUnit.GetCurrentOrder(TAUnit.Id2Ptr(arg1)))
        else result := Ord(TAUnit.GetCurrentOrder(pointer(unitptr)));
      end;
    VETERAN_LEVEL :
      begin
      result := TAUnit.getKills(pointer(unitptr))*100;
      end;
    HEALTH_VAL :
      begin
      result := TAUnit.getHealth(pointer(unitptr));
      end;
    CLOAKED :
      begin
      result := TAUnit.getCloak(pointer(unitptr));
      end;
    UNIT_IS_ON_THIS_COMP:
      begin
      result := TAUnit.IsOnThisComp(TAUnit.Id2Ptr(arg1), True);
      end;
    UNITX:
      begin
      result:= TAUnit.getUnitX(pointer(unitptr)) div 163840;
      end;
    UNITY:
      begin
      result:= TAUnit.getUnitY(pointer(unitptr)) div 163840;
      end;
    UNITZ:
      begin
      result:= TAUnit.getUnitZ(pointer(unitptr)) div 163840;
      end;
    TURNX:
      begin
      result:= Word(TAUnit.getTurnX(pointer(unitptr)));
      end;
    TURNZ:
      begin
      result:= Word(TAUnit.getTurnZ(pointer(unitptr)));
      end;
    TURNY:
      begin
      result:= Word(TAUnit.getTurnY(pointer(unitptr)));
      end;
    CREATE_UNIT:
      begin
      Position.x_:= LoWord(arg2);
      Position.X:= HiWord(arg2);
      Position.z_:= LoWord(arg3);
      Position.Z:= HiWord(arg3);
      Position.y_:= LoWord(arg4);
      Position.Y:= HiWord(arg4);
      pUnit:= TAUnit.CreateUnit(PUnitStruct(Pointer(unitptr)).cMyLOSPlayerID, arg1, Position, nil, False, False, 1);
      if pUnit <> nil then
        result:= Word(PUnitStruct(pUnit).lUnitInGameIndex);
      end;
    KILL :
      begin
      if arg2 <> 0 then
        TAUnit.Kill(pointer(unitptr), arg1)
      else
        TAUnit.Kill(pointer(unitptr), arg1);
      end;
    MAXHEALTH..FCLOAKCOSTMOVE :
      begin
      result:= TAUnit.getUnitInfoField(pointer(unitptr), index, nil);
      end;
    MINIONS :
      begin
      result:= TAUnits.CreateMinions(pointer(unitptr), arg2, MinionsPattern_Random, arg1, TTAActionType(arg3), arg4);
      end;
    UNITS_NEAR :
      begin
      result:= TAUnits.SearchUnits(pointer(unitptr), arg3, 2, arg2, TAUnits.CreateSearchFilter(arg1), arg4 );
      end;
    UNITS_YARDMAP :
      begin
      result:= TAUnits.SearchUnits(pointer(unitptr), arg3, 1, arg2, TAUnits.CreateSearchFilter(arg1), arg4 );
      end;
    UNITS_WHOLEMAP :
      begin
      result:= TAUnits.SearchUnits(pointer(unitptr), arg3, 4, arg2, TAUnits.CreateSearchFilter(arg1), arg4 );
      end;
    UNIT_NEAREST :
      begin
      result:= TAUnits.SearchUnits(pointer(unitptr), 0, 3, arg2, TAUnits.CreateSearchFilter(arg1), arg3 );
      end;
    UNITS_ARRAY_RESULT :
      begin
      case arg3 of
        1 : begin
              if Assigned(UnitSearchArr[arg1].UnitIds) then
                result:= UnitSearchArr[arg1].UnitIds[arg2 - 1]
              else
                result:= 0;
            end;
        2 : begin
              if Assigned(SpawnedMinionsArr[arg1].UnitIds) then
                result:= SpawnedMinionsArr[arg1].UnitIds[arg2 - 1]
              else
                result:= 0;
            end;
      end;
      end;
    FREE_ARRAY_ID :
      begin
      result:= TAUnits.GetRandomArrayId(arg1);
      end;
    MAKE_LONG:
      begin
      result:= MakeLong(Word(arg1), Word(arg2));
      end;
    TELEPORT :
      begin
      result:= TAUnits.Teleport(pointer(unitptr), arg2, arg1);
      end;
    CREATE_ORDER_SELF :
      begin
      result:= TAUnit.CreateOrder(pointer(unitptr), nil, TTAActionType(arg1), nil, arg2, arg3);
      end;
    CREATE_ORDER_SELF_UNIT :
      begin
      result:= TAUnit.CreateOrder(pointer(unitptr), TAUnit.Id2Ptr(arg2), TTAActionType(arg1), nil, arg3, arg4);
      end;
    CREATE_ORDER_SELF_UNIT_POS :
      begin
      {Position.X:= HiWord(LoWord(arg3) * 163840);
      Position.x_:= LoWord(LoWord(arg3) * 163840);
      Position.Z:= HiWord(HiWord(arg3) * 163840);
      Position.z_:= LoWord(HiWord(arg3) * 163840);
      Position.Y:= HiWord(arg4 * 163840);
      Position.y_:= LoWord(arg4 * 163840);
      arg3:= PUnitStruct(UnitPtr).Position.X;  }
      Position:= PUnitStruct(UnitPtr).Position;
      Inc(Position.X, 200);
      result:= TAUnit.CreateOrder(pointer(unitptr), TAUnit.Id2Ptr(arg2), TTAActionType(arg1), @Position, 1, 0);
      end;
    CREATE_ORDER_SELF_POS :
      begin
      Position.X:= HiWord(arg2);
      Position.Z:= LoWord(arg2);
      Position.Y:= arg3;
      result:= TAUnit.CreateOrder(pointer(unitptr), nil, TTAActionType(arg1), @Position, LoWord(arg4), HiWord(arg4));
      end;
    CREATE_ORDER_UNIT_UNIT :
      begin
      result:= TAUnit.CreateOrder(TAUnit.Id2Ptr(arg3), TAUnit.Id2Ptr(arg2), TTAActionType(arg1), nil, LoWord(arg4), HiWord(arg4));
      end;
    CREATE_ORDER_UNIT_SELF :
      begin
      result:= TAUnit.CreateOrder(TAUnit.Id2Ptr(arg2), nil, TTAActionType(arg1), nil, arg3, arg4);
      end;
    CREATE_ORDER_UNIT_POS :
      begin
      Position.X:= HiWord(arg3);
      Position.Z:= LoWord(arg3);
      Position.Y:= arg4;
      result:= TAUnit.CreateOrder(TAUnit.Id2Ptr(arg2), nil, TTAActionType(arg1), @Position, HiWord(arg4), 0);
      end;
    SELECTABLE :
      begin
      result:= BoolValues[(PUnitStruct(pointer(unitptr)).lUnitStateMask and 32 = 32)];
      end;
    CLEAR_ARRAY_ID :
      begin
      TAUnits.ClearArrayElem(arg2, arg1);
      end;
    end;
  end;
end;

procedure CustomSetters( index: longword; unitptr : longword;
                        arg1: longword); stdcall;
begin
if ((index >= CUSTOM_LOW) and (index <= CUSTOM_HIGH)) then
begin
  if TAUnit.IsOnThisComp(pointer(unitptr), True) <> 0 then
  begin
    SendTextLocal(IntToStr(LongWord(unitptr))+': SET '+IntToStr(index)+' to '+IntToStr(arg1) + ' HEX: '+IntToHex(arg1, 8));
    case index of
      HEALTH_VAL : TAUnit.setHealth(pointer(unitptr), arg1);
      CLOAKED : TAUnit.setCloak(pointer(unitptr), arg1);
      WEAPON1..WEAPON3 :
        begin
        if TAUnit.setWeapon(pointer(unitptr), index, Word(arg1)) then
          globalDplay.SendCobEventMessage(TANM_Rec2Rec_UnitWeapon, @unitptr, @index, nil, @arg1, nil);
        end;
      UNITX :
        begin
        TAUnit.setUnitX(pointer(unitptr), arg1);
        end;
      UNITY :
        begin
        TAUnit.setUnitY(pointer(unitptr), arg1);
        end;
      UNITZ :
        begin
        TAUnit.setUnitZ(pointer(unitptr), arg1);
        end;
      TURNX:
        begin
        TAUnit.setTurnX(pointer(unitptr), SmallInt(arg1));
        end;
      TURNZ:
        begin
        TAUnit.setTurnZ(pointer(unitptr), SmallInt(arg1));
        end;
      TURNY:
        begin
        TAUnit.setTurnY(pointer(unitptr), SmallInt(arg1));
        end;
      SPEECH :
        begin
        TAUnit.Speech(unitptr, arg1, nil);
        end;
      SOUND_EFFECT :
        begin
        TAUnit.SoundEffectId(unitptr, arg1);
        end;
      UNIT_TYPE :
        begin
        if TAUnit.setTemplate(pointer(unitptr), arg1) then
          globalDplay.SendCobEventMessage(TANM_Rec2Rec_UnitTemplate, @unitptr, @index, nil, @arg1, nil);
        end;
      SWAP_TYPE :
        begin
        TAUnit.SwapByKill(pointer(unitptr), arg1);
        end;
      SELECTABLE :
        begin
        if arg1 = 1 then
          PUnitStruct(pointer(unitptr)).lUnitStateMask := PUnitStruct(pointer(unitptr)).lUnitStateMask or 32
        else
          PUnitStruct(pointer(unitptr)).lUnitStateMask := PUnitStruct(pointer(unitptr)).lUnitStateMask and not 32;
        end;
      UPGRADEABLE :
        begin
        if TAUnit.setUpgradeable(pointer(unitptr), arg1, nil) then
          globalDplay.SendCobEventMessage(TANM_Rec2Rec_UnitUpgradeable, @unitptr, nil, @arg1, nil, nil);
        end;
      BUILDER..SELFDSTRAS :
        begin
        if TAUnit.setUnitInfoField(pointer(unitptr), index, arg1, nil) then
          globalDplay.SendCobEventMessage(TANM_Rec2Rec_UnitEditTemplate, @unitptr, @arg1, nil, nil, @index);
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
