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
  GetTAProgramStructHandler = function : Pointer; register;
var
  GetTAProgramStruct : GetTAProgramStructHandler = GetTAProgramStructHandler($004B6220);

type
  GameRunSecHandler = function : Cardinal; cdecl;
var
  GameRunSec : GameRunSecHandler = GameRunSecHandler($004B6340);

type
  GetGameingTypeHandler = function(a1, a2, a3: Cardinal): Cardinal; register;
var
  GetGameingType : GetGameingTypeHandler = GetGameingTypeHandler($00435100);

type
  TakeScreenshotHandler = function (FileName : PAnsiChar; OffscreenPtr: Pointer) : Integer; stdcall;
var
  TakeScreenshot : TakeScreenshotHandler = TakeScreenshotHandler($004CAEC0);

type
  TakeScreenshotOrgHandler = function (DirName : PAnsiChar; FileName : PAnsiChar) : Integer; stdcall;
var
  TakeScreenshotOrg : TakeScreenshotOrgHandler = TakeScreenshotOrgHandler($004CB170);

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
  TA_UpdateUnitLOSHandler = function (UnitPtr: Pointer): LongWord; stdcall;
var
  // called to update unit LOS
  TA_UpdateUnitLOS : TA_UpdateUnitLOSHandler = TA_UpdateUnitLOSHandler($00482AC0);

type
  TA_UpdateLOSHandler = function (PlayerIndex: Integer; FillFeatureMap_b: LongWord): LongWord; stdcall;
var
  // called to update LOS
  TA_UpdateLOS : TA_UpdateLOSHandler = TA_UpdateLOSHandler($004816A0);

// get map ground level at given coords
type
  GetPosHeightHandler = function (pPosition : Pointer): Integer; stdcall;
var
  GetPosHeight : GetPosHeightHandler = GetPosHeightHandler($485070);

type
  UnitInPlayerLOSHandler = function (PlayerPtr : Pointer; UnitPtr : Pointer): Integer; stdcall;
var
  UnitInPlayerLOS : UnitInPlayerLOSHandler = UnitInPlayerLOSHandler($00465AC0);

type
  PositionInPlayerLOSHandler = function (PlayerPtr : Pointer; Position : TPosition): LongBool; stdcall;
var
  PositionInPlayerLOS : PositionInPlayerLOSHandler = PositionInPlayerLOSHandler($00408090);

type
  UnitAtPositionHandler = function (Position : PPosition): Pointer; stdcall;
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
                                  UnitId: Cardinal) : Pointer; stdcall; // 0 if UnitId is unknown (?)
var
  Unit_Create : Unit_CreateHandler = Unit_CreateHandler($00485F50);

type
  Send_UnitBuildFinishedHandler = function ( UnitPtr: Pointer; Unit2Ptr: Pointer ): Integer; stdcall;
var
  Send_UnitBuildFinished : Send_UnitBuildFinishedHandler = Send_UnitBuildFinishedHandler($004560C0);

type
  Send_UnitDeathHandler = function ( UnitPtr: Pointer; a2: Integer ): Integer; stdcall;
var
  Send_UnitDeath : Send_UnitDeathHandler = Send_UnitDeathHandler($004864B0);

type
  FreeUnitOrdersHandler = procedure ( UnitPtr: Pointer ); stdcall;
var
  FreeUnitOrders : FreeUnitOrdersHandler = FreeUnitOrdersHandler($00489740);

type
  FreeObjectStateHandler = function ( ObjectState: Pointer ): Integer; stdcall;
var
  FreeObjectState : FreeObjectStateHandler = FreeObjectStateHandler($0045AAA0);

type
  FreeMoveClassHandler = function ( a1: pointer; a2: pointer; MoveClass: Pointer ): Integer; register;
var
  FreeMoveClass : FreeMoveClassHandler = FreeMoveClassHandler($0043DD10);

type
  free_MMwapper__Handler = procedure ( Memory: Pointer ); cdecl;
var
  free_MMwapper__ : free_MMwapper__Handler = free_MMwapper__Handler($004B4F20);

// turret weap aiming
type
  sub_49D910Handler = function ( a1 : LongInt; a2 : LongInt; a3 : LongInt;
                                 a9 : LongInt; a8 : LongInt; a7 : LongInt;
                                 a6 : LongInt; a5 : LongInt; a4 : LongInt) : LongInt; register;
var
  sub_49D910 : sub_49D910Handler = sub_49D910Handler($49D910);

type
  GetUnit_BuildWeaponProgressHandler = function ( UnitPtr: Pointer ): Cardinal; stdcall;
var
  GetUnit_BuildWeaponProgress : GetUnit_BuildWeaponProgressHandler = GetUnit_BuildWeaponProgressHandler($439D20);

type
  GetFeatureTypeOfPosHandler = function ( OrderPos: Pointer; Order: Pointer; Unknown: Pointer ): SmallInt; stdcall;
var
  GetFeatureTypeOfPos : GetFeatureTypeOfPosHandler = GetFeatureTypeOfPosHandler($00421DA0);

type
  Feature_DestroyHandler = function ( X: Integer; Z: Integer; DesMethod: Integer ): Integer; stdcall;
var
  Feature_Destroy : Feature_DestroyHandler = Feature_DestroyHandler($00423550);

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
                                       a5: Word ): Pointer; stdcall;
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
  CreateProjectile_0_3Handler = function ( UnitWeapon: Pointer;
                                           Attacker_UnitPtr: Pointer;
                                           Position_Start: PPosition;
                                           Position_Target: PPosition ): LongBool; stdcall;
var
  CreateProjectile_0_3 : CreateProjectile_0_3Handler = CreateProjectile_0_3Handler($0049C9C0);

type
  CreateProjectile_1Handler = function ( UnitWeapon: Pointer;
                                         Attacker_UnitPtr: Pointer;
                                         Position_Start: PPosition;
                                         Position_Target: PPosition;
                                         TargetUnitPtr: Pointer;
                                         Interceptor: Integer ): LongBool; stdcall;
var
  CreateProjectile_1 : CreateProjectile_1Handler = CreateProjectile_1Handler($0049CC20);

type
  CreateProjectile_0Handler = function ( UnitWeapon: Pointer;
                                         Attacker_UnitPtr: Pointer;
                                         Position_Start: PPosition;
                                         Position_Target: PPosition;
                                         TargetUnitPtr: Pointer ): LongBool; stdcall;
var
  CreateProjectile_0 : CreateProjectile_0Handler = CreateProjectile_0Handler($0049CDE0);

type
  Trajectory3Handler = function ( AttackerUnitPtr: Pointer;
                                  Position1: PPosition;
                                  Position2: PPosition;
                                  WhichWeapon: Word ): Integer; stdcall;
var
  Trajectory3 : Trajectory3Handler = Trajectory3Handler($0049AA80);

type
  UnitAutoAim_CheckUnitWeaponHandler = function ( AttackerUnitPtr: Pointer;
                                                  TargetUnitPtr: Pointer;
                                                  WhichWeapon: Word ): Byte; stdcall;
var
  UnitAutoAim_CheckUnitWeapon : UnitAutoAim_CheckUnitWeaponHandler = UnitAutoAim_CheckUnitWeaponHandler($0049ABB0);

type
  TdfFile__GetIntHandler = function ( TagName : Pointer;
                                      DefaultNumber : LongInt): Integer; stdcall;
var
  TdfFile__GetInt : TdfFile__GetIntHandler = TdfFile__GetIntHandler($004C46C0);

type
  TdfFile__GetStrHandler = function ( ReceiveBuf : Pointer;
                                      Name : Pointer;
                                      BufLen : Cardinal;
                                      Default : Pointer): Integer; stdcall;
var
  TdfFile__GetStr : TdfFile__GetStrHandler = TdfFile__GetStrHandler($004C48C0);

type
  TA_GiveUnitHandler = procedure ( UnitPtr : Pointer;
                                   PlayerStruct : PPlayerStruct;
                                   Packet : Pointer); stdcall;
var
  TA_GiveUnit : TA_GiveUnitHandler = TA_GiveUnitHandler($00488570);

type
  TA_AttachDetachUnitHandler = procedure (TransportedUnitPtr : Pointer;
                                          TransporterUnitPtr : Pointer;
                                          Piece : ShortInt;
                                          Unknown : Byte); stdcall;
var
  TA_AttachDetachUnit : TA_AttachDetachUnitHandler = TA_AttachDetachUnitHandler($0048AAC0);

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
                                 lPar1: Cardinal;
                                 lPar2: Cardinal ): Cardinal; stdcall;
var
  Order2Unit: Order2UnitHandler = Order2UnitHandler($0043AFC0);

type
  SubOrder2UnitHandler = function ( ScriptIndex: Cardinal;
                                    ShiftKey: Cardinal;
                                    pUnitPtr: Pointer;
                                    pTargetUnitPtr: Pointer;
                                    pPosition: Pointer;
                                    lPar1: Cardinal;
                                    lPar2: Cardinal ): Cardinal; stdcall;
var
  SubOrder2Unit: SubOrder2UnitHandler = SubOrder2UnitHandler($0043ADC0);


type
  GetUnitFirstOrderTargatHandler = function ( UnitPtr: Pointer ): Cardinal; stdcall;
var
  GetUnitFirstOrderTargat: GetUnitFirstOrderTargatHandler = GetUnitFirstOrderTargatHandler($439DD0);

type
  // not guaranteed to be called
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
  Script_RunScript: Script_RunScriptHandler = Script_RunScriptHandler($004B0A70);

type
  // guaranteed query call
  Script_ProcessCallbackHandler = function ( a1: Pointer;  // dunno
                                             a2: Pointer;  // dunno
                                             UnitScriptsData_p: Cardinal;
                                             v4: Pointer;
                                             v3: Pointer;
                                             v2: Pointer;
                                             v1: Pointer;
                                             const Name: PAnsiChar): LongInt; register;
var
  Script_ProcessCallback: Script_ProcessCallbackHandler = Script_ProcessCallbackHandler($004B0BC0);
{
type
  // guaranteed call
  // create, startbuilding etc.
  Script_CallScriptHandler = function ( a1: Pointer;
                                        a2: Pointer;
                                        UnitScriptsData_p: Cardinal;
                                        v4: Pointer;
                                        v3: Pointer;
                                        v2: Pointer;
                                        v1: Pointer;
                                        const Name: PAnsiChar): LongInt; register;
var
  Script_CallScript: Script_CallScriptHandler = Script_CallScriptHandler($004B0940);
}

type
  DrawGameScreenHandler = procedure (DrawUnits: Integer; BlitScreen: Integer); stdcall;
var
  DrawGameScreen : DrawGameScreenHandler = DrawGameScreenHandler($00468CF0);

type
  DrawHealthBarsHandler = function (OFFSCREEN_ptr: LongWord; UnitInGame: LongWord; PosX: LongWord; PosY: LongWord) : LongInt; stdcall;
var
  DrawHealthBars : DrawHealthBarsHandler = DrawHealthBarsHandler($0046A430);

// draw filled box  
type
  DrawBarHandler = function (OFFSCREEN_ptr: Cardinal; Position: Pointer; ColorOffset: Byte) : LongInt; stdcall;
var
  DrawBar : DrawBarHandler = DrawBarHandler($004BF6F0);

type
  DrawTranspRectangleHandler = function (OFFSCREEN_ptr: Cardinal; Position: Pointer; ColorOffset: Byte) : LongInt; stdcall;
var
  DrawTranspRectangle : DrawTranspRectangleHandler = DrawTranspRectangleHandler($004BF8C0);

type
  DrawPointHandler = function (OFFSCREEN_ptr: Cardinal; X, Y : Integer; ColorOffset: Byte) : LongInt; stdcall;
var
  DrawPoint : DrawPointHandler = DrawPointHandler($004BEE60);

type
  DrawUnk1Handler = function (OFFSCREEN_ptr: Cardinal; Position: Pointer; ColorOffset: Byte) : LongInt; stdcall;
var
  DrawUnk1 : DrawUnk1Handler = DrawUnk1Handler($004BFD60);

type
  DrawLineHandler = function (OFFSCREEN_ptr: Cardinal; X, Y, X2, Y2 : Integer; ColorOffset: Byte) : LongInt; stdcall;
var
  DrawLine : DrawLineHandler = DrawLineHandler($004BE950);

type
  DrawLine2Handler = function (OFFSCREEN_ptr: Cardinal; x1, y1, x2, y2 : Integer; Color: Byte) : LongInt; cdecl;
var
  DrawLine2 : DrawLine2Handler = DrawLine2Handler($004CC7AB);

type
  DrawCircleHandler = function (OFFSCREEN_ptr: Cardinal; CenterX, CenterY, Radius : Integer; ColorOffset: Byte) : LongInt; stdcall;
var
  DrawCircle : DrawCircleHandler = DrawCircleHandler($004C0070);

type
  DrawDotteCircleHandler = function (OFFSCREEN_ptr: Cardinal; CenterX, CenterY, Radius : Integer; ColorOffset: Integer; Spacing: Word; Dotte_b : Integer) : LongInt; stdcall;
var
  DrawDotteCircle : DrawDotteCircleHandler = DrawDotteCircleHandler($004C01A0);

type
  sub_4B7123Handler = function(a1: Word; a2: LongInt) : LongInt; cdecl;
var
  sub_4B7123 : sub_4B7123Handler = sub_4B7123Handler($004B7123);

type
  sub_4B70EFHandler = function(a1: Word; a2: LongInt) : LongInt; cdecl;
var
  sub_4B70EF : sub_4B70EFHandler = sub_4B70EFHandler($004B70EF);

type
  CorrecLinetPositionHandler = function(OFFSCREEN_ptr: Cardinal;
                                        x1: Pointer;
                                        y1: Pointer;
                                        x2: Pointer;
                                        y2: Pointer) : LongInt; stdcall;
var
  CorrecLinetPosition : CorrecLinetPositionHandler = CorrecLinetPositionHandler($004BEA20);

type
  DrawProgressBarHandler = function (OFFSCREEN_ptr: Cardinal; Position: Pointer; BarPosition: Integer) : LongInt; stdcall;
var
  DrawProgressBar : DrawProgressBarHandler = DrawProgressBarHandler($00468310);

type
  DrawTransparentBoxHandler = function (OFFSCREEN_ptr: Cardinal; Position: Pointer; Transp: Integer) : LongInt; stdcall;
var
  DrawTransparentBox : DrawTransparentBoxHandler = DrawTransparentBoxHandler($004BF4D0);

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
  Msg_ReminderHandler = function(Msg_Str: PAnsiChar; Msg_Type: Word) : Integer; stdcall;
var
  Msg_Reminder : Msg_ReminderHandler = Msg_ReminderHandler($046BC70);

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
  WeaponName2PtrHandler = function ( const WeaponName: PAnsiChar ): Pointer; stdcall;
var
  WeaponName2Ptr : WeaponName2PtrHandler = WeaponName2PtrHandler($0049E5B0);

type
  FindSpot_CategorysAryHandler = function ( const CategoryName: String ): Integer; stdcall;
var
  FindSpot_CategorysAry : FindSpot_CategorysAryHandler = FindSpot_CategorysAryHandler($00488C50);

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
  ClearChatHandler = function : Integer; cdecl;
var
  ClearChat : ClearChatHandler = ClearChatHandler($00463C80);

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

// providing Player will also check is grid spot in player LOS
// just like unmapped terrain build spot check works
type
  TestGridSpotHandler = function ( BuildUnit : Pointer;
                                    Pos : Cardinal;
                                    unk : Word; //unk=zero
                                    Player : Pointer
                                    ): Integer; stdcall;
var
  TestGridSpot : TestGridSpotHandler = TestGridSpotHandler($0047D2E0);

type
  TestGridSpotAIHandler = function ( UnitInfo : Pointer;
                                    GridPos : Cardinal
                                    ): Integer; stdcall;
var
  TestGridSpotAI : TestGridSpotAIHandler = TestGridSpotAIHandler($0047D820);

type //fill TAdynmem->MouseMapPosX & TAdynmem->MouseMapPosY first
	TestBuildSpotHandler = procedure (); stdcall;
var
  TestBuildSpot : TestBuildSpotHandler = TestBuildSpotHandler($004197D0);

type
  CanAttachAtGridSpotHandler = function ( UnitInfo : PUnitInfo;
                                          UnitID : Word;
                                          GridPos : Cardinal;
                                          State : Integer): Boolean; stdcall;
var
  CanAttachAtGridSpot : CanAttachAtGridSpotHandler = CanAttachAtGridSpotHandler($0047DB70);

type
  CanCloseOrOpenYardHandler = function ( UnitPtr: Pointer; NewState: Integer): Boolean; stdcall;
var
  CanCloseOrOpenYard : CanCloseOrOpenYardHandler = CanCloseOrOpenYardHandler($0047D970);

type
	GetPiecePositionHandler = function(PositionOut: PPosition;
                                     UnitPtr: PUnitStruct;
                                     PieceIdx: Integer): PPosition; stdcall;
var
  GetPiecePosition : GetPiecePositionHandler = GetPiecePositionHandler($0043E060);

type //find unit under mouse
	GetUnitAtMouseHandler = function () : Cardinal; stdcall;
var
  GetUnitAtMouse : GetUnitAtMouseHandler = GetUnitAtMouseHandler($0048CD80);

type //find unit at position
	GetUnitAtCoordsHandler = function (Position: Pointer) : Cardinal; stdcall;
var
  GetUnitAtCoords : GetUnitAtCoordsHandler = GetUnitAtCoordsHandler($004815F0);

type
  GetTPositionHandler = function (X, Z : Integer; out Position : TPosition): Pointer; stdcall;
var
  GetTPosition : GetTPositionHandler = GetTPositionHandler($00484B50);

type
	GetGridPosPLOTHandler = function (PosX, PosY: Integer) : Pointer; stdcall;
var
  GetGridPosPLOT : GetGridPosPLOTHandler = GetGridPosPLOTHandler($00481550);

type
	TADrawRectHandler = procedure ( unk : PtagRECT; rect : PtagRECT; colour : Longint); stdcall;
var
  TADrawRect : TADrawRectHandler = TADrawRectHandler($004BF8C0);

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

Type
  AllowMemReadWriteHandler = function(Address : Cardinal): Integer; cdecl;
var
  AllowMemReadWrite : AllowMemReadWriteHandler = AllowMemReadWriteHandler($004D8780);

Type
  SetMemReadOnlyHandler = function(Address : Cardinal): Integer; cdecl;
var
  SetMemReadOnly : SetMemReadOnlyHandler = SetMemReadOnlyHandler($004D8710);

Type
  rand_Handler = function(Range : Integer): Integer; cdecl;
var
  rand_ : rand_Handler = rand_Handler($004B6C30);

Type
  TA_Atan2Handler = function(y, x : Integer): Integer; cdecl;
var
  TA_Atan2 : TA_Atan2Handler = TA_Atan2Handler($004B715A);

Type
  InsertToHPIAryHandler = function(Filename: PAnsiChar; Priority: Integer): Integer; stdcall;
var
  InsertToHPIAry : InsertToHPIAryHandler = InsertToHPIAryHandler($004BE0B0);

Type
  SetCurrentDirectoryToTAPathHandler = function(): Boolean; cdecl;
var
  SetCurrentDirectoryToTAPath : SetCurrentDirectoryToTAPathHandler = SetCurrentDirectoryToTAPathHandler($0049F540);

Type
  findfirst_HPIHandler = function(lpFileName: PAnsiChar; finddata_ptr: Pointer; SearchType: Integer; TryNext: Integer): Integer; stdcall;
var
  findfirst_HPI : findfirst_HPIHandler = findfirst_HPIHandler($004BC4B0);

Type
  findnext_HPIHandler = function(SearchHandle: Integer; finddata_ptr: Pointer): Integer; stdcall;
var
  findnext_HPI : findnext_HPIHandler = findnext_HPIHandler($004BC640);

Type
  findclose_HPIHandler = function(SearchHandle: Integer): Integer; stdcall;
var
  findclose_HPI : findclose_HPIHandler = findclose_HPIHandler($004BC8D0);

Type
  HAPI_BroadcastMessageHandler = function(FromPID: Integer; Buffer: Pointer; BufferSize: Integer): Integer; stdcall;
var
  HAPI_BroadcastMessage : HAPI_BroadcastMessageHandler = HAPI_BroadcastMessageHandler($00451DF0);

//int __stdcall FindUnitsInCategorysAry(const char *CategoryName)


//////////////////////////////////////////////////////////////////////////////////////////
/// Not working.
//////////////////////////////////////////////////////////////////////////////////////////
type
  YardOpenHandler = function (UnitPtr: Pointer; NewState: integer): Integer; stdcall;
var
  YardOpen: YardOpenHandler = YardOpenHandler($47DAC0);

type
  UnitStateProbeHandler = function (OffscreenPtr: Cardinal): Integer; stdcall;
var
  UnitStateProbe: UnitStateProbeHandler = UnitStateProbeHandler($00467E50);

type
  UnitBuilderProbeHandler = function (OffscreenPtr: Cardinal): Integer; stdcall;
var
  UnitBuilderProbe: UnitBuilderProbeHandler = UnitBuilderProbeHandler($004685A0);

type
  TestUnitAIHandler = function ( BuildUnit : Cardinal; UnitPtr : Cardinal ): Byte; stdcall;
var
  TestUnitAI : TestUnitAIHandler = TestUnitAIHandler($47DDC0);

//////////////////////////////////////////////////////////////////////////////////////////
/// Not used.
//////////////////////////////////////////////////////////////////////////////////////////
type
  // creates unitstatemask based on actual unitinfo template
  // called for existing unit (!)
  Unit_CreateUnitsInGameHandler = function (unitptr : Pointer;
                                            PosX: Integer;
                                            PosY: Integer;
                                            PosZ: Integer;
                                            FullHp: Integer) : LongBool; stdcall;
var
  Unit_CreateUnitsInGame : Unit_CreateUnitsInGameHandler = Unit_CreateUnitsInGameHandler($485A40);  // boneyards 45BD31

type
  Unit_CreateModelAndCobHandler = function (UnitPtr : Pointer) : Pointer; stdcall;
var
  Unit_CreateModelAndCob : Unit_CreateModelAndCobHandler = Unit_CreateModelAndCobHandler($00485D40);
  
type
  Unit_FixPosY_SeaHandler = function (UnitPtr : Pointer) : LongInt; stdcall;
var
  Unit_FixPosY_Sea : Unit_FixPosY_SeaHandler = Unit_FixPosY_SeaHandler($0048A870);

type
  DrawUnitHandler = function (OFFSCREEN_ptr: Cardinal; UnitPtr : Pointer) : LongInt; stdcall;
var
  DrawUnit : DrawUnitHandler = DrawUnitHandler($0045AC20);

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
  UNITS_AllocateMovementClassHandler = function ( UnitPtr: Pointer): Pointer; stdcall;
var
  UNITS_AllocateMovementClass: UNITS_AllocateMovementClassHandler = UNITS_AllocateMovementClassHandler($00485E50);
  
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
