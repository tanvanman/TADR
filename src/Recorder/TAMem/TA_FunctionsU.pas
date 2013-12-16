unit TA_FunctionsU;

interface

//////////////////////////////////////////////////////////////////////////////////////////
/// Working.
//////////////////////////////////////////////////////////////////////////////////////////

const
  access = 1;

type
  //flags 0 = toggle show all, 1 = rebuild LOS fields
  Game_SetLOSStateHandler = function (flags : integer) : integer; stdcall;
var
  // called by to setup the LOS tables
  Game_SetLOSState : Game_SetLOSStateHandler = Game_SetLOSStateHandler($4816A0);
  
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
  CreateMultiplayerMapsListHandler = function ( a1: Longint; a2: Longint; a3: Longint): Longint; stdcall;
  // a1 = 0, a2 = 0, a3 = 0
  // result = current amount of maps
var
  CreateMultiplayerMapsList : CreateMultiplayerMapsListHandler = CreateMultiplayerMapsListHandler($434BF0);


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

type //fill TAdynmem->MouseMapPosX & TAdynmem->MouseMapPosY first
	TestBuildSpotHandler = procedure (); stdcall;
var
  TestBuildSpot : TestBuildSpotHandler = TestBuildSpotHandler($4197D0);

type //find unit under mousepointer
	FindMouseUnitHandler = function () : word; stdcall;
var
  FindMouseUnit : FindMouseUnitHandler = FindMouseUnitHandler($48CD80);

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
    x : Longint;
    y : Longint;
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
  TestGridSpotHandler = procedure ( BuildUnit : Pointer; // BuildUnit : ^UnitStruct
                                    Pos : PPosRec;
                                    unk : Integer; //unk=zero
                                    Player : pointer // Player : ^PlayerStruct
                                    ); stdcall;
var
  TestGridSpot : TestGridSpotHandler = TestGridSpotHandler($47D2E0);
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
