unit TA_FunctionsU;

interface
uses
  TA_MemoryStructures;

//////////////////////////////////////////////////////////////////////////////////////////
/// Working.
//////////////////////////////////////////////////////////////////////////////////////////

const
  access = 1;

type
  Game_SetLOSStateHandler = function (flags : integer) : integer; stdcall;
var
  // called by to setup the LOS tables
  Game_SetLOSState : Game_SetLOSStateHandler = Game_SetLOSStateHandler($4816A0);

type
  Name2Sequence_GafHandler = function (GafStruct: Cardinal; SequenceName : Cardinal) : LongInt; stdcall;
var
  Name2Sequence_Gaf : Name2Sequence_GafHandler = Name2Sequence_GafHandler($004B8D40);

type
  ShowExplodeGafHandler = function (Position: PPosition; p_GAFAnim: Cardinal; AddGlow: LongInt; AddSmoke: LongInt) : Byte; stdcall;
var
  ShowExplodeGaf : ShowExplodeGafHandler = ShowExplodeGafHandler($00420A30);

type
  TA_UpdateUnitLOSHandler = function (unitptr : longword) : longword; stdcall;
var
  // called to update unit LOS
  TA_UpdateUnitLOS : TA_UpdateUnitLOSHandler = TA_UpdateUnitLOSHandler($482AC0);

type
  TA_UpdateLOSHandler = function (a1 : longword; FillFeatureMap_b: longword) : longword; stdcall;
var
  // called to update LOS
  TA_UpdateLOS : TA_UpdateLOSHandler = TA_UpdateLOSHandler($4816A0);

// get map ground level at given coords
type
  GetPosHeightHandler = function (pPosition : Pointer): Integer; stdcall;
var
  GetPosHeight : GetPosHeightHandler = GetPosHeightHandler($485070);

type
  CheckUnitInPlayerLOSHandler = function (PlayerPtr : Pointer; UnitPtr : Pointer): Integer; stdcall;
var
  CheckUnitInPlayerLOS : CheckUnitInPlayerLOSHandler = CheckUnitInPlayerLOSHandler($00465AC0);

type
  UnitAtPositionHandler = function (Position : PPosition): Cardinal; stdcall;
var
  UnitAtPosition : UnitAtPositionHandler = UnitAtPositionHandler($004815A0);

type
  Unit_CreateHandler = function ( PlayerAryIndex : Cardinal;  // owner index
                                  UnitInfoId: Cardinal;       // template id
                                  PosX_: Cardinal;
                                  PosZ_: Cardinal;
                                  PosY_: Cardinal;
                                  FullHp: Cardinal;           // 1 - unit with full hp, like comm spawn
                                  UnitStateMask: Cardinal;        // 0
                                  UnitId: Cardinal) : Cardinal; stdcall; // 0 if UnitId is unknown (?)
var
  Unit_Create : Unit_CreateHandler = Unit_CreateHandler($485F50);

// turret weap aiming
type
  sub_49D910Handler = function ( a1 : LongInt; a2 : LongInt; a3 : LongInt;
                                 a9 : LongInt; a8 : LongInt; a7 : LongInt;
                                 a6 : LongInt; a5 : LongInt; a4 : LongInt) : LongInt; register;
var
  sub_49D910 : sub_49D910Handler = sub_49D910Handler($49D910);

type
  GetUnit_BuildWeaponProgressHandler = function ( UnitPtr: Cardinal ): Cardinal; stdcall;
var
  GetUnit_BuildWeaponProgress : GetUnit_BuildWeaponProgressHandler = GetUnit_BuildWeaponProgressHandler($439D20);

type
  GetFeatureTypeOfOrderHandler = function ( OrderPos: Pointer; Order: Pointer; Unknown: Cardinal ): SmallInt; stdcall;
var
  GetFeatureTypeOfOrder : GetFeatureTypeOfOrderHandler = GetFeatureTypeOfOrderHandler($421DA0);

type
  // a2 probably preserves unit in array
  // 0 : found in unit "recreate" proc
  // 3 : typical kill
  // 8 : TA finalization, kill all proc etc.
  Unit_KillHandler = procedure ( UnitPtr: Pointer; a2: Cardinal ); stdcall;
var
  Unit_Kill: Unit_KillHandler = Unit_KillHandler($4864B0);

type
  UnitExplosionHandler = procedure ( UnitPtr: Pointer; destructas: Cardinal ); stdcall;
var
  UnitExplosion: UnitExplosionHandler = UnitExplosionHandler($49B000);

type
  MakeDamageToUnitHandler = function ( UnitPtr: Pointer;
                                       UnitPtr2: Pointer;
                                       a3: LongInt;
                                       a4: Cardinal;
                                       a5: Word ): Cardinal; stdcall;
var
  MakeDamageToUnit: MakeDamageToUnitHandler = MakeDamageToUnitHandler($489BB0);

type
  HealUnit_Handler = function ( UnitPtr: Pointer;
                                UnitPtr2: Pointer;
                                Amount: Single ): Integer; stdcall;
var
  HealUnit: HealUnit_Handler = HealUnit_Handler($0041BD10);

type
  TestHeal_Handler = function ( ResPercentage: Pointer;
                                Amount: Single ): Integer; stdcall;
var
  TestHeal: TestHeal_Handler = TestHeal_Handler($00401180);

// earthquakes etc.
type
  SendFireMapWeaponHandler = function ( WeaponTypePtr: Cardinal;
                                        Position: Cardinal;
                                        TargetPosition: Cardinal;
                                        a4: Cardinal): Cardinal; stdcall;
var
  SendFireMapWeapon: SendFireMapWeaponHandler = SendFireMapWeaponHandler($49DF10);

type
  fire_callbackHandler = function ( Attacker_UnitPtr: Pointer;
                                    Weapon_Target_ID: Pointer;
                                    Victim_UnitPtr: Pointer;
                                    Position_Target: Pointer ): Cardinal; stdcall;
var
  fire_callback1: fire_callbackHandler = fire_callbackHandler($0049DB70);
  fire_callback2: fire_callbackHandler = fire_callbackHandler($0049DD60);
  fire_callback3: fire_callbackHandler = fire_callbackHandler($0049D9C0);

type
  fire_callback0Handler = function ( Attacker_UnitPtr: Pointer;
                                     Weapon_Target_ID: Pointer;
                                     Victim_UnitPtr: Pointer;
                                     Position_Target: Pointer ): Cardinal; cdecl;
var
  fire_callback0 : fire_callback0Handler = fire_callback0Handler($0049D580);

type
  TdfFile__GetIntHandler = function ( TagName : Pointer;
                                      DefaultNumber : LongInt): Integer; stdcall;
var
  TdfFile__GetInt : TdfFile__GetIntHandler = TdfFile__GetIntHandler($004C46C0);

type
  SetPrepareOrderHandler = function ( UnitPtr: Pointer; a2: Cardinal): LongInt; stdcall;
var
  SetPrepareOrder: SetPrepareOrderHandler = SetPrepareOrderHandler($00419BE0);

type
  Order2UnitHandler = function ( ScriptIndex: Cardinal;
                                    ShiftKey: Cardinal;
                                    pUnitPtr: Pointer;
                                    pTargetUnitPtr: Pointer;
                                    pPosition: Pointer;
                                    a6: Cardinal;   // unit type id for nanolathe order or order time in game tick (* 30)
                                                    // selfdestructh calls it with 1
                                    a7: Cardinal    // for build orders - unit amount
                                    ): LongInt; stdcall;
var
  Order2Unit: Order2UnitHandler = Order2UnitHandler($43AFC0);
  
type
  GetUnitFirstOrderTargatHandler = function ( UnitPtr: Pointer ): Cardinal; stdcall;
var
  GetUnitFirstOrderTargat: GetUnitFirstOrderTargatHandler = GetUnitFirstOrderTargatHandler($439DD0);

type
  Script_RunScriptHandler = function ( a1: Cardinal;
                                       a2: Cardinal;
                                       UnitScriptsData_p: Cardinal;
                                       v4: Cardinal;
                                       v3: Cardinal;
                                       v2: Cardinal;
                                       v1: Cardinal;
                                       a8: Cardinal; // amount of out vars ?
                                       a9: Cardinal;
                                       a10: Cardinal;
                                       const Name: PAnsiChar): LongInt; register;
var
  Script_RunScript: Script_RunScriptHandler = Script_RunScriptHandler($4B0A70);

type
  Script_ProcessCallbackHandler = function ( a1: Pointer;  // dunno
                                             a2: Pointer;  // dunno
                                             UnitScriptsData_p: Cardinal;
                                             v4: Pointer;
                                             v3: Pointer;
                                             v2: Pointer;
                                             v1: Pointer;
                                             const Name: PAnsiChar): LongInt; register;
var
  Script_ProcessCallback: Script_ProcessCallbackHandler = Script_ProcessCallbackHandler($4B0BC0);

type
  DrawHealthBarsHandler = function (OFFSCREEN_ptr: LongWord; UnitInGame: LongWord; PosX: LongWord; PosY: LongWord) : LongInt; stdcall;
var
  DrawHealthBars : DrawHealthBarsHandler = DrawHealthBarsHandler($0046A430);

type
  DrawRectangleHandler = function (OFFSCREEN_ptr: Cardinal; Position: Pointer; ColorOffset: Byte) : LongInt; stdcall;
var
  DrawRectangle : DrawRectangleHandler = DrawRectangleHandler($004BF6F0);

type
  DrawText_HeavyHandler = function (OFFSCREEN_ptr: Cardinal; const str: PAnsiChar; left: Integer; top: Integer; MaxWidth: Integer) : LongInt; stdcall;
var
  DrawText_Heavy : DrawText_HeavyHandler = DrawText_HeavyHandler($004C14F0);

type
  DrawTextHandler = function (OFFSCREEN_ptr: Cardinal; const str: PAnsiChar; left: Integer; top: Integer; MaxLen: Integer; Background: Integer) : LongInt; stdcall;
var
  DrawText : DrawTextHandler = DrawTextHandler($004A50E0);

type
  DrawText_ThinHandler = function (OFFSCREEN_ptr: Cardinal; const str: PAnsiChar; left: Integer; top: Integer; MaxLen: Integer; a6: Integer; Background: Integer) : LongInt; stdcall;
var
  DrawText_Thin : DrawText_ThinHandler = DrawText_ThinHandler($004A51D0);

type
  SetFONTLENGTH_ptrHandler = function (NewFontLength: Cardinal) : LongInt; stdcall;
var
  SetFONTLENGTH_ptr : SetFONTLENGTH_ptrHandler = SetFONTLENGTH_ptrHandler($004C1420);

type
  sub_4C13F0Handler = function : LongInt; cdecl;
var
  sub_4C13F0 : sub_4C13F0Handler = sub_4C13F0Handler($004C13F0);

type
  SetFontColorHandler = function(a5: LongInt; a4: LongInt) : LongInt; stdcall;
var
  SetFontColor : SetFontColorHandler = SetFontColorHandler($004C13A0);

type
  PlaySound_UnitSpeechHandler = function (unitptr : longword; speechtype: longword; speechtext: PChar) : byte; stdcall;
var
  PlaySound_UnitSpeech : PlaySound_UnitSpeechHandler = PlaySound_UnitSpeechHandler($47F780);

type
  PlaySound_2DHandler = function (VoiceId: longword; unitptr: LongWord) : Integer; stdcall;
var
  PlaySound_2D : PlaySound_2DHandler =  PlaySound_2DHandler($47F0C0);

type
  Receive_SoundHandler = function (EffectNum: LongWord; Position: Pointer; Broadcast : LongWord) : Integer; stdcall;
var
  Receive_Sound : Receive_SoundHandler =  Receive_SoundHandler($47F300);

type
  //Access 1 = no cheats, 3 = cheats
  InterpretCommandHandler = procedure ( Command : PChar; access : Longint); stdcall;
var
  // called by GUI to process player commands
  InterpretCommand : InterpretCommandHandler = InterpretCommandHandler($417B50);
  procedure InterpretInternalCommand(CommandText: string);

type
  //Access 1 = no cheats, 3 = cheats
  DoInterpretCommandHandler = procedure ( var Command : PChar; access : Longint); stdcall;
var
  // called by GUI+engine to process all commands
  DoInterpretCommand : DoInterpretCommandHandler = DoInterpretCommandHandler($4B7900);

type
  // a1 = 0, a2 = 0, a3 = 0
  // result = current amount of maps
  CreateMultiplayerMapsListHandler = function ( a1: Longint; a2: Longint; a3: Longint): Longint; stdcall;
var
  CreateMultiplayerMapsList : CreateMultiplayerMapsListHandler = CreateMultiplayerMapsListHandler($434BF0);

type
  CreateMovementClassHandler = function ( a1: Longint ): Longint; stdcall;
var
  CreateMovementClass : CreateMovementClassHandler = CreateMovementClassHandler($485E50);

type
  PlayerArryIndex2IDHandler = function ( a1: LongWord ): Longint; stdcall;
var
  PlayerArryIndex2ID : PlayerArryIndex2IDHandler = PlayerArryIndex2IDHandler($44FFD0);

type
  DirectID2PlayerAryHandler = function ( a1: LongInt ): Byte; stdcall;
var
  DirectID2PlayerAry : DirectID2PlayerAryHandler = DirectID2PlayerAryHandler($44FE40);

type
  // sets hot key group of a unit
  // a1 = group number
  // TA calls it with -1 when received unit death packet of unitptr unit
  SetHotGroupHandler = procedure ( unitptr: longword; a1: longint ); stdcall;
var
  SetHotGroup : SetHotGroupHandler = SetHotGroupHandler($480250);

type
  // result = UNITINFO.UnitTypeID
  // 0 = unit type not found
  UnitName2IDHandler = function ( const UnitName: PAnsiChar ): Word; stdcall;
var
  UnitName2ID : UnitName2IDHandler = UnitName2IDHandler($00488B10);

type
  WeaponName2IDHandler = function ( const WeaponName: PAnsiChar ): LongWord; stdcall;
var
  WeaponName2ID : WeaponName2IDHandler = WeaponName2IDHandler($49E5B0);

// Displays text to the screen (no parsing for commands)
type //TextType - 0 = chat, 1 = popup
  SendTextHandler = function ( Text : PChar;
                               TextType : Longint) : longint; stdcall;
var
  SendText : SendTextHandler = SendTextHandler($46bc70);
  procedure SendTextLocal(Text: string);

type
                               // ^PlayerStruct
                               // , int access, int type
  ShowTextHandler = procedure ( player : pointer;
                                Text : PChar;
                                Unknown1 : Longint; // uses a value of 4
                                Unknown2 : Longint  // uses a value of 0
                                ); stdcall;
var
  ShowText : ShowTextHandler = ShowTextHandler($463E50);



type
  PMsgStruct = ^TMsgStruct;
  TMsgStruct = packed record
		XPos : longint;
		YPos : longint;
		shiftstatus : Longint;
  end;

type //fill TAdynmem->MouseMapPosX & TAdynmem->MouseMapPosY first
  TAMapClickHandler = procedure ( msgstruct : PMsgStruct ); stdcall;
var
  TAMapClick : TAMapClickHandler = TAMapClickHandler($498F70);

type
  TestGridSpotHandler = function ( BuildUnit : Pointer;
                                    Pos : Cardinal;
                                    unk : Word; //unk=zero
                                    Player : Cardinal
                                    ): Integer; stdcall;
var
  TestGridSpot : TestGridSpotHandler = TestGridSpotHandler($47D2E0);

type //fill TAdynmem->MouseMapPosX & TAdynmem->MouseMapPosY first
	TestBuildSpotHandler = procedure (); stdcall;
var
  TestBuildSpot : TestBuildSpotHandler = TestBuildSpotHandler($4197D0);

type //find unit under mouse
	GetUnitAtMouseHandler = function () : Cardinal; stdcall;
var
  GetUnitAtMouse : GetUnitAtMouseHandler = GetUnitAtMouseHandler($48CD80);

type //find unit at position
	GetUnitAtCoordsHandler = function (Position: Pointer) : Cardinal; stdcall;
var
  GetUnitAtCoords : GetUnitAtCoordsHandler = GetUnitAtCoordsHandler($4815F0);

type
	GetGridPosPLOTHandler = function (PosX, PosY: Integer) : Cardinal; stdcall;
var
  GetGridPosPLOT : GetGridPosPLOTHandler = GetGridPosPLOTHandler($481550);
  
type
	TADrawRectHandler = procedure ( unk : PtagRECT; rect : PtagRECT; colour : Longint); stdcall;
var
  TADrawRect : TADrawRectHandler = TADrawRectHandler($4BF8C0);


type
  TADrawLineHandler = procedure ( Context : PChar; x1,y1,x2,y2 : Longint; Colour : Longint ) cdecl;
var
  TADrawLine : TADrawLineHandler = TADrawLineHandler($4CC7AB);

type // buffer should be at least 50 characters long 
  GetContextHandler = function ( ptr : PChar ) : Longint; Stdcall;
var
  GetContext : GetContextHandler = GetContextHandler($4C5E70);

//CirclePointer = CirclePointer in tadynmemstruct
type
  PPosRec = ^TPosRec;
  TPosRec = packed record
    x : Word;
    y : Word;
	end;

  TADrawCircleHandler = procedure ( Contect : PChar;
                                    CirclePointer : Pointer;
                                    Pos : PPosRec;
                                    radius : Integer;
                                    colour : Integer;
                                    Text : PChar;
                                    unknown1 : Integer // value of 1
                                    ); stdcall;
var
  TADrawCircle : TADrawCircleHandler = TADrawCircleHandler($438EA0);

type // used to replace TA deallocation of wreackagearray
  TADeleteMemHandler = procedure ( mem : Pointer ); cdecl;
var
  TADeleteMem : TADeleteMemHandler = TADeleteMemHandler($4D85A0);

Type // +los cheat
  TextCommand_LOSHandler = procedure ; stdcall;
var  
  TextCommand_LOS : TextCommand_LOSHandler = TextCommand_LOSHandler($416D50);

//////////////////////////////////////////////////////////////////////////////////////////
/// Not working.
//////////////////////////////////////////////////////////////////////////////////////////
type
  YardOpenHandler = function (UnitPtr: Pointer; NewState: integer): Integer; stdcall;
var
  YardOpen: YardOpenHandler = YardOpenHandler($47DAC0);

type
  TestGridSpotAIHandler = function ( BuildUnit : Cardinal; Pos : Cardinal ): Integer; stdcall;
var
  TestGridSpotAI : TestGridSpotAIHandler = TestGridSpotAIHandler($47D820);

type
  TestUnitAIHandler = function ( BuildUnit : Cardinal; UnitPtr : Cardinal ): Byte; stdcall;
var
  TestUnitAI : TestUnitAIHandler = TestUnitAIHandler($47DDC0);

type // should draw selected unit state on screen
	UnitStateProbeHandler = function (OFFSCREEN_p: Cardinal) : LongInt; stdcall;
var
  UnitStateProbe : UnitStateProbeHandler = UnitStateProbeHandler($467E50);

//////////////////////////////////////////////////////////////////////////////////////////
/// Not used.
//////////////////////////////////////////////////////////////////////////////////////////
type
  // creates unitstatemask based on actual unitinfo template
  // called for existing unit (!)
  Unit_CreateUnitsInGameHandler = function (unitptr : Cardinal;
                                            PosX_: Cardinal;
                                            PosZ_: Cardinal;
                                            PosY_: Cardinal;
                                            FullHp: Cardinal) : Cardinal; stdcall;
var
  Unit_CreateUnitsInGame : Unit_CreateUnitsInGameHandler = Unit_CreateUnitsInGameHandler($485A40);  // boneyards 45BD31

type
  Unit_CreateModelAndCobHandler = function (UnitPtr : Pointer) : integer; stdcall;
var
  Unit_CreateModelAndCob : Unit_CreateModelAndCobHandler = Unit_CreateModelAndCobHandler($485D40);

type
  Unit_SetSpeedHandler = procedure ( UnitPtr: Pointer ); stdcall;
var
  Unit_SetSpeed : Unit_SetSpeedHandler = Unit_SetSpeedHandler($437840);
  
type
  Unit_StartWeaponsScriptsHandler = procedure ( UnitPtr: Pointer ); stdcall;
var
  Unit_StartWeaponsScripts : Unit_StartWeaponsScriptsHandler = Unit_StartWeaponsScriptsHandler($49E070);

type
  Unit_RecreateHandler = function ( PlayerIndex: Byte; UnitPtr: Pointer): Cardinal; stdcall;
var
  Unit_Recreate: Unit_RecreateHandler = Unit_RecreateHandler($4861D0);
  
type
  cmalloc_MM__Handler = function (Size : Cardinal) : Integer; cdecl;
var
  cmalloc_MM__ : cmalloc_MM__Handler =  cmalloc_MM__Handler($004B4F10);

type
  AirOrder_InitHandler = function (UnitOrder : Cardinal; Pos_p: Cardinal) : Cardinal; stdcall;
var
  AirOrder_Init : AirOrder_InitHandler = AirOrder_InitHandler($0044E2D0);

type
  AirOrder_BeginLiftHandler = function (a1 : Cardinal; a2: Cardinal; UnitOrder_p: Cardinal; a4: Cardinal) : Byte; register;
var
  AirOrder_BeginLift : AirOrder_BeginLiftHandler = AirOrder_BeginLiftHandler($0044E6C0);

type
  AirOrder_CallOrderHandler = function (OrderCallBack_p : Cardinal) : Cardinal; stdcall;
var
  AirOrder_CallOrder : AirOrder_CallOrderHandler = AirOrder_CallOrderHandler($004388D0);

// prepare wings etc.
type
  AirOrder_SetShortMaskStateHandler = function (a1 : Cardinal;
                                                a2 : Cardinal;
                                                a3 : Cardinal;
                                                a4 : Cardinal;
                                                a5 : Cardinal) : Byte; register;
var
  AirOrder_SetShortMaskState : AirOrder_SetShortMaskStateHandler = AirOrder_SetShortMaskStateHandler($0043D210);

implementation

procedure InterpretInternalCommand(CommandText: string);
begin
  InterpretCommand(PChar(CommandText),access);
end;

procedure SendTextLocal(Text: string);
begin
  SendText(PAnsiChar(Text), 0);
end;

end.
