unit TA_FunctionsU;

interface

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
  TA_UpdateUnitLOSHandler = function (unitptr : longword) : longword; stdcall;
var
  // called to update unit LOS
  TA_UpdateUnitLOS : TA_UpdateUnitLOSHandler = TA_UpdateUnitLOSHandler($482AC0);

type
  TA_UpdateLOSHandler = function (a1 : longword; FillFeatureMap_b: longword) : longword; stdcall;
var
  // called to update LOS
  TA_UpdateLOS : TA_UpdateLOSHandler = TA_UpdateLOSHandler($4816A0);

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

type
  // creates unitstatemask based on actual unitinfo template
  // called for existing unit (!)
  Unit_CreateUnitsInGameHandler = function (unitptr : Cardinal;
                                            PosX_: Cardinal;
                                            PosZ_: Cardinal;
                                            PosY_: Cardinal;
                                            FullHp: Cardinal) : Cardinal; stdcall;
var
  Unit_CreateUnitsInGame : Unit_CreateUnitsInGameHandler = Unit_CreateUnitsInGameHandler($485A40);


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
  Unit_FixPositionZ_HoverFloaterHandler = procedure ( UnitPtr: Pointer ); stdcall;
var
  Unit_FixPositionZ_HoverFloater : Unit_FixPositionZ_HoverFloaterHandler = Unit_FixPositionZ_HoverFloaterHandler($48A870);

type
  Unit_PlayerActiveType3Handler = procedure ( UnitPtr: Pointer ); stdcall;
var
  Unit_PlayerActiveType3 : Unit_PlayerActiveType3Handler = Unit_PlayerActiveType3Handler($47CC30);

type
  // a2 probably preserves unit in array
  // 0 : found in unit "recreate" proc
  // 3 : typical kill
  // 8 : TA finalization, kill all proc etc.
  Unit_KillHandler = procedure ( UnitPtr: Pointer; a2: Cardinal ); stdcall;
var
  Unit_Kill: Unit_KillHandler = Unit_KillHandler($4864B0);

type
  Unit_KillMakeDamageHandler = procedure ( UnitPtr: Pointer; destructas: Cardinal ); stdcall;
var
  Unit_KillMakeDamage: Unit_KillMakeDamageHandler = Unit_KillMakeDamageHandler($49B000);

type
  Unit_MakeDamage_Handler = function ( UnitPtr: Pointer;
                                       UnitPtr2: Pointer;
                                       a3: LongInt;
                                       a4: Cardinal;
                                       a5: Word ): Cardinal; stdcall;
var
  Unit_MakeDamage_: Unit_MakeDamage_Handler = Unit_MakeDamage_Handler($489BB0);

type
  Unit_RecreateHandler = function ( PlayerIndex: Byte; UnitPtr: Pointer): Cardinal; stdcall;
var
  Unit_Recreate: Unit_RecreateHandler = Unit_RecreateHandler($4861D0);

// earthquakes etc.
type
  SendFireMapWeaponHandler = function ( WeaponTypePtr: Cardinal;
                                        Position: Cardinal;
                                        TargetPosition: Cardinal;
                                        a4: Cardinal): Cardinal; stdcall;
var
  SendFireMapWeapon: SendFireMapWeaponHandler = SendFireMapWeaponHandler($49DF10);

//Send_FireWeapon(int, int Victim_Ptr, int Attacker_Ptr, int Position_Start, int Position_Targat)
type
  Send_FireWeaponHandler = function ( WeaponId: Cardinal;
                                      Victim_Ptr: Pointer;
                                      Attacker_Ptr: Pointer;
                                      Position_Start: Pointer;
                                      Position_Target: Pointer): Integer; stdcall;
var
  Send_FireWeapon: Send_FireWeaponHandler = Send_FireWeaponHandler($499AB0);

type
  SetPrepareOrderHandler = function ( UnitPtr: Pointer; a2: Cardinal): LongInt; stdcall;
var
  SetPrepareOrder: SetPrepareOrderHandler = SetPrepareOrderHandler($419BE0);

type
  Order2UnitHandler = function ( ScriptIndex: Cardinal;
                                    ShiftKey: Cardinal;
                                    pUnitPtr: Pointer;
                                    pTargetUnitPtr: Pointer;
                                    pPosition: Pointer;
                                    a6: Cardinal;   // unit type id for nanolathe order or order time in game tick (* 30)
                                                    // selfdestructg calls it with 1
                                    a7: Cardinal    // prob out: pointer, build orders use it
                                    ): LongInt; stdcall;
var
//  NewOrder2Unit: NewOrder2UnitHandler = NewOrder2UnitHandler($43ADC0);
  Order2Unit: Order2UnitHandler = Order2UnitHandler($43AFC0);

type
//  ScriptAction_Name2IndexHandler = function ( out a2: Cardinal; ActionName_str: PAnsiChar): Cardinal; register;
  ScriptAction_Name2IndexHandler = function : Cardinal; stdcall;
var
  ScriptAction_Name2Index: ScriptAction_Name2IndexHandler = ScriptAction_Name2IndexHandler($438760);

type
  Script_RunCallBackHandler = function ( a1: Cardinal;
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
  Script_RunCallBack: Script_RunCallBackHandler = Script_RunCallBackHandler($4B0A70);

type
  sub_45A8D0Handler = function (unitptr : Cardinal) : integer; stdcall;
var
  sub_45A8D0 : sub_45A8D0Handler = sub_45A8D0Handler($45A8D0);

type
  // createobject3d0
  sub_45A950Handler = function (modelptr : longword; a2: longword; unitptr: longword) : longword; stdcall;
var
  sub_45A950 : sub_45A950Handler = sub_45A950Handler($45A950);

type
  PlaySound_UnitSpeechHandler = function (unitptr : longword; speechtype: longword; speechtext: PChar) : byte; stdcall;
var
  PlaySound_UnitSpeech : PlaySound_UnitSpeechHandler = PlaySound_UnitSpeechHandler($47F780);

type
  PlaySound_EffectNameHandler = function (VoiceName: PChar; unitptr: LongWord) : LongWord; stdcall;
var
  PlaySound_EffectName : PlaySound_EffectNameHandler = PlaySound_EffectNameHandler($47F1A0);

type
  PlaySound_EffectIdHandler = function (VoiceId: longword; unitptr: LongWord) : Integer; stdcall;
var
  PlaySound_EffectId : PlaySound_EffectIdHandler =  PlaySound_EffectIdHandler($47F0C0);

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
  // kills ALL units. yes, of all players
  KillAllUnitsHandler = procedure; stdcall;
var
  KillAllUnits : KillAllUnitsHandler = KillAllUnitsHandler($486ED0);

type
  PlaySoundEffectHandler = function ( VoiceName: AnsiChar; a2: Word ): LongInt ; stdcall;
var
  PlaySoundEffect : PlaySoundEffectHandler = PlaySoundEffectHandler($48CD80);

type
  // result = UNITINFO.UnitTypeID
  // 0 = unit type not found
  UnitName2IDHandler = function ( const UnitName: PAnsiChar ): Word; stdcall;
var
  UnitName2ID : UnitName2IDHandler = UnitName2IDHandler($488B10);

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

type //find unit at position
	GetGridPosPLOTHandler = function (PosX, PosY: Integer) : Cardinal; stdcall;
var
  GetGridPosPLOT : GetGridPosPLOTHandler = GetGridPosPLOTHandler($481550);

type
  PtagRECT = ^tagRECT;
  tagRECT = packed record
            Left : Longint;
            Top : Longint;
            Right : Longint;
            Bottom : Longint;
            end;
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
implementation

procedure InterpretInternalCommand(CommandText: string);
begin
  InterpretCommand(PChar(CommandText),access);
end;

procedure SendTextLocal(Text: string);
{var
  TmpResult: LongInt; }
begin
  SendText(PAnsiChar(Text), 0);
end;

end.