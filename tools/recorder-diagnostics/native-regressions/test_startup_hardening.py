"""Real method bodies with small COM/game-state doubles, no game or networking."""
from pathlib import Path
import subprocess,json
import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[3])
parser.add_argument('--out',type=Path,default=Path(__file__).resolve().parents[1]/'build'/Path(__file__).stem)
parser.add_argument('--baseline',default='dcff5ddeb6bd1030e3f452c0f16e5f005850f62f')
parser.add_argument('--fpc',type=Path,required=True,help='FPC root containing bin/i386-win32 and units/i386-win32')
args=parser.parse_args()
ROOT=args.source.resolve(); OUT=args.out.resolve(); FPC=args.fpc.resolve()
OUT.mkdir(parents=True,exist_ok=True)
def read(name,old=False):
    return (subprocess.check_output(['git','-c','safe.directory='+str(ROOT),'-C',str(ROOT),'show',args.baseline+':'+name]).decode('cp1252') if old else (ROOT/name).read_text(encoding='cp1252'))
def body(s,start,end):return start+s.split(start,1)[1].split(end,1)[0]
methods=''
for old in (True,False):
    name='Old' if old else 'New'
    s=read('src/Recorder/plugins/BattleRoomEnhancements.pas',old)
    methods+=body(s,'procedure EnterBattleRoom; stdcall;','procedure BattleRoom_EnterBattleRoom').replace('EnterBattleRoom;',name+'EnterBattleRoom;')
    s=read('src/Recorder/DPLobbyWrapper.pas',old)
    methods+=body(s,'function TLobby.Connect(dwFlags: DWORD; var lplpDP: IDirectPlay2;','function TLobby.CreateAddress').replace('TLobby.Connect(', 'TLobby.'+name+'Connect(')
probe=r'''program StartupHardening;
{$mode delphi}{$H+}{$R+}{$Q+}
uses Windows, SysUtils;
const IID_IDirectPlay: TGUID = '{00000001-0000-0000-0000-000000000001}'; DP_OK=0;
type
  IDirectPlay=interface(IUnknown) ['{00000001-0000-0000-0000-000000000001}'] end;
  IDirectPlay2=interface(IUnknown) ['{00000002-0000-0000-0000-000000000002}'] end;
  TMockDP=class(TInterfacedObject,IDirectPlay,IDirectPlay2)
    QueryResult: HResult; NilQuery: Boolean;
    function QueryInterface(constref IID: TGUID; out Obj): HResult; stdcall;
  end;
  TMockLobby=class
    ConnectResult: HResult; DP: IDirectPlay2;
    function Connect(Flags: DWORD; var OutDP: IDirectPlay2; Unk: IUnknown): HResult;
  end;
  TLobby=class
    lobby2: TMockLobby;
    function OldConnect(dwFlags: DWORD; var lplpDP: IDirectPlay2; pUnk:IUnknown):HResult;
    function NewConnect(dwFlags: DWORD; var lplpDP: IDirectPlay2; pUnk:IUnknown):HResult;
  end;
  TDPlay=class(TInterfacedObject,IDirectPlay2)
    AIDifficulty: Byte;
    constructor Create(DP: IDirectPlay);
    procedure BroadcastModInfo;
  end;
  TLog=class
    class procedure Add(Level: Integer; const S: string); overload; static;
    class procedure Add(Level: Integer; const S: string; Value: DWORD); overload; static;
    class procedure Flush; static;
  end;
  TPlayerStruct=record PlayerInfo: record Raceside: Byte; end; end;
  PPlayerStruct=^TPlayerStruct;
  TMain=record RaceSideData: array[0..4] of record Name: AnsiChar; end; p_TAGUIObject: Pointer; end;
  TPlayerLookup=class
    class function GetPlayerByIndex(Index: Integer): PPlayerStruct; static;
  end;
var TAData: record PlayersStructPtr: Pointer; LocalPlayerID: Integer; MainStruct: TMain; end;
  TAPlayer: TPlayerLookup;
  Player: TPlayerStruct; GlobalDPlay: TDPlay;
  SideValue,DifficultyValue,Checks,Creates: Integer;
  startedfrom: string;
procedure Check(OK: Boolean; const S: string);
begin Inc(Checks); if not OK then raise Exception.Create(S); end;
function REGISTRY_ReadInteger(Section, Name: PAnsiChar; Value: PInteger): Integer;
begin if string(Name)='RaceSide' then Value^:=SideValue else Value^:=DifficultyValue; Result:=1; end;
function IsLocalPlayerHost: Boolean; begin Result:=True; end;
procedure GUIGADGET_SetStatus(Gadget: Pointer; Name:PAnsiChar; Value: Byte); begin end;
class function TPlayerLookup.GetPlayerByIndex(Index:Integer):PPlayerStruct; begin Result:=@Player; end;
constructor TDPlay.Create(DP:IDirectPlay); begin inherited Create; Inc(Creates); end;
procedure TDPlay.BroadcastModInfo; begin end;
class procedure TLog.Add(Level:Integer; const S:string); begin end;
class procedure TLog.Add(Level:Integer; const S:string; Value:DWORD); begin end;
class procedure TLog.Flush; begin end;
procedure LogException(E:Exception); begin end;
function TMockDP.QueryInterface(constref IID:TGUID; out Obj):HResult; stdcall;
begin
  Pointer(Obj):=nil;
  if IsEqualGUID(IID,IID_IDirectPlay) then begin
    if NilQuery or (QueryResult<>S_OK) then Result:=QueryResult
    else Result:=inherited QueryInterface(IID,Obj);
  end else Result:=inherited QueryInterface(IID,Obj);
end;
function TMockLobby.Connect(Flags:DWORD; var OutDP:IDirectPlay2; Unk:IUnknown):HResult;
begin OutDP:=DP; Result:=ConnectResult; end;
__METHODS__
procedure TestBattleRoom;
var Failed: Boolean; I: Integer;
begin
  GlobalDPlay:=TDPlay.Create(nil);
  try
    TAData.PlayersStructPtr:=@Player;
    for I:=0 to 4 do TAData.MainStruct.RaceSideData[I].Name:='x';
    SideValue:=5; DifficultyValue:=1; Failed:=False;
    try OldEnterBattleRoom; except on E:ERangeError do Failed:=True; end;
    Check(Failed,'old side array fault not reproduced');
    Player.PlayerInfo.Raceside:=1; NewEnterBattleRoom;
    Check(Player.PlayerInfo.Raceside=1,'invalid side altered current side');
    SideValue:=1; DifficultyValue:=256; Failed:=False;
    try OldEnterBattleRoom; except on E:ERangeError do Failed:=True; end;
    Check(Failed,'old difficulty range fault not reproduced');
    NewEnterBattleRoom; Check(GlobalDPlay.AIDifficulty=1,'difficulty default');
    for I:=0 to 4 do begin SideValue:=I; NewEnterBattleRoom; Check(Player.PlayerInfo.Raceside=I,'valid side changed'); end;
    for I:=0 to 255 do begin DifficultyValue:=I; NewEnterBattleRoom; Check(GlobalDPlay.AIDifficulty=I,'valid byte difficulty changed'); end;
    SideValue:=-1; DifficultyValue:=-1; NewEnterBattleRoom;
    Check(GlobalDPlay.AIDifficulty=1,'negative difficulty');
  finally GlobalDPlay.Free; end;
  Writeln('battle room: old side/difficulty exceptions reproduced; valid values preserved');
end;
procedure TestLobby;
var L:TLobby; OutDP:IDirectPlay2; DP:TMockDP; Failed:Boolean; Before:Integer;
begin
  L:=TLobby.Create; L.lobby2:=TMockLobby.Create;
  try
    L.lobby2.ConnectResult:=E_FAIL; Failed:=False; Before:=Creates;
    try L.OldConnect(0,OutDP,nil); except on E:EAccessViolation do Failed:=True; end;
    Check(Failed,'old failed Connect nil dereference not reproduced');
    Check(L.NewConnect(0,OutDP,nil)=E_FAIL,'Connect error overwritten');
    Check((OutDP=nil) and (Creates=Before),'failed Connect built wrapper');
    L.lobby2.ConnectResult:=S_OK;
    Check(L.NewConnect(0,OutDP,nil)=E_NOINTERFACE,'nil successful Connect');
    DP:=TMockDP.Create; L.lobby2.DP:=DP;
    DP.QueryResult:=E_NOINTERFACE;
    Check(L.NewConnect(0,OutDP,nil)=E_NOINTERFACE,'QI error overwritten');
    Check((OutDP=nil) and (Creates=Before),'failed QI built wrapper');
    DP.QueryResult:=S_OK; DP.NilQuery:=True;
    Check(L.NewConnect(0,OutDP,nil)=E_NOINTERFACE,'nil successful QI');
    DP.NilQuery:=False;
    Check(L.NewConnect(0,OutDP,nil)=S_OK,'valid Connect');
    Check((OutDP<>nil) and (Creates=Before+1),'valid wrapper missing'); OutDP:=nil;
  finally L.lobby2.Free; L.Free; end;
  Writeln('lobby: old failed Connect AV reproduced; HRESULTs and success path pass');
end;
procedure TestTimestamp;
var A:Integer; Prev:Longword; Failed:Boolean;
begin
  A:=Integer(Longword($f59c05e8)); Failed:=False;
  try Prev:=A; except on E:ERangeError do Failed:=True; end;
  Check(Failed,'old recorder-message timestamp fault');
  Prev:=Longword(A); Check(Prev=$f59c05e8,'explicit timestamp bit cast');
end;
begin TestBattleRoom; TestLobby; TestTimestamp; Writeln('PASS checks=',Checks); end.
'''.replace('__METHODS__',methods)
path=OUT/'startup.pas';path.write_text(probe,encoding='cp1252')
compile=subprocess.run([str(FPC/'bin/i386-win32/ppc386.exe'),'-n','-Mdelphi','-Cr','-Co','-gl','-Fu'+str(FPC/'units/i386-win32/rtl'),'-FU'+str(OUT),'-FE'+str(OUT),str(path)],capture_output=True,text=True)
print(compile.stdout,compile.stderr,end='');compile.check_returncode()
run=subprocess.run([str(OUT/'startup.exe')],capture_output=True,text=True)
print(run.stdout,run.stderr,end='')
(OUT/'startup-result.json').write_text(json.dumps({'exit':run.returncode,'out':run.stdout,'err':run.stderr},indent=2));run.check_returncode()
