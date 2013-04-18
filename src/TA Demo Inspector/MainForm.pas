unit MainForm;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ExtCtrls, StdCtrls,
  MemMappedDataStructure;

type
  TForm1 = class(TForm)
    TAHookCheck: TTimer;
    Update: TTimer;
    DataPanel: TPanel;
    DataReport: TMemo;
    procedure TAHookCheckTimer(Sender: TObject);
    procedure UpdateTimer(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure FormCreate(Sender: TObject);
  private
    { Private declarations }
    hMemMap : THandle;
    SharedData : PMKChatMem;
    LocalData : MKChatMem;
    OldLocalData : MKChatMem;
  public
    { Public declarations }
    procedure ChangeState( state : Boolean );
    procedure DoSaveStats;
    procedure DoCheck;
  end;

var
  Form1: TForm1;

implementation

{$R *.DFM}

procedure TForm1.ChangeState( state : Boolean );
begin
Update.Enabled := state;
if state then
  Caption := 'TA Demo Recorder active ingame'
else
  Caption := 'TA Demo Recorder is not active';
//Update.Enabled := state;
end; {ChangeState}

procedure TForm1.DoSaveStats;
var
  i : Integer;
  s : string;
  AllyCount : Integer;
  ActivePlayers : array of Integer;
  Index : Integer;
begin
if SharedData = nil then Exit;
LocalData := SharedData^;

if CompareMem( @LocalData, @OldLocalData, SizeOf(LocalData) ) then
  Exit;
OldLocalData := LocalData;

DataReport.Lines.Clear;
{
DataReport.Lines.Add( 'chat:'+ LocalData.chat );
DataReport.Lines.Add( 'dataexists:'+IntToStr(LocalData.dataexists) );
DataReport.Lines.Add( 'tastatus:'+IntToStr(LocalData.tastatus ));
DataReport.Lines.Add( 'playingDemo:'+IntToStr(LocalData.playingDemo ));
DataReport.Lines.Add( 'ehaWarning:'+IntToStr(LocalData.ehaWarning) );
DataReport.Lines.Add( 'ehaOff:'+IntToStr(LocalData.ehaOff) );

if LocalData.toAlliesLength <> 0 then
  begin
  DataReport.Lines.Add( 'toAlliesLength:'+IntToStr(LocalData.toAlliesLength ));
  DataReport.Lines.Add( 'toAllies:'+LocalData.toAllies );
  end;
if LocalData.fromAlliesLength <> 0 then
  begin
  DataReport.Lines.Add( 'fromAlliesLength:'+IntToStr(LocalData.fromAlliesLength) );
  DataReport.Lines.Add( 'fromAllies:'+LocalData.fromAllies );
  end;
}
{
DataReport.Lines.Add( 'MapName:'+LocalData.MapName );
DataReport.Lines.Add( 'MaxUnitCount:'+IntToStr(LocalData.MaxUnitCount) );
DataReport.Lines.Add( 'myCheats:'+IntToStr(LocalData.myCheats) );
DataReport.Lines.Add( 'F1Disable:'+IntToStr(LocalData.F1Disable) );
DataReport.Lines.Add( 'commanderWarp:'+IntToStr(LocalData.commanderWarp) );
DataReport.Lines.Add( 'lockviewon:'+IntToStr(LocalData.lockviewon) );
DataReport.Lines.Add( 'ta3d:'+IntToStr(LocalData.ta3d) );
DataReport.Lines.Add( 'mapX:'+IntToStr(LocalData.mapX) );
DataReport.Lines.Add( 'mapY:'+IntToStr(LocalData.mapY) );
}
// discover the list of active players
Index := 0;
SetLength(ActivePlayers, 10);
for i := Low(LocalData.playernames) to High(LocalData.playernames) do
  if LocalData.playernames[i][1] <> #0 then
    begin
    ActivePlayers[index] := i;
    Inc(Index);
    end;
SetLength(ActivePlayers, Index);

s := IntTostr(Length(ActivePlayers))+ ' players reported by Recorder; ';
for i := Low(ActivePlayers) to High(ActivePlayers) do
  begin
  Index := ActivePlayers[i];
  if LocalData.PlayerNames[Index][1] <> #0 then
    begin
    s := s + PChar(@LocalData.playernames[Index]);
    if i <> High(ActivePlayers) then
      s := s +', ';
    end;
  end;
DataReport.Lines.Add( s );
DataReport.Lines.Add( '' );

s := 'Player:'+PChar(@LocalData.playernames[1]) + ' is allied with: ';
AllyCount := 0;
for i := Low(ActivePlayers) to High(ActivePlayers) do
  begin
  Index := ActivePlayers[i];
  if LocalData.allies[Index] <> 0 then
    begin
    Inc(AllyCount);
    s := s + Pchar(@LocalData.playernames[Index]);
    if i <> High(ActivePlayers) then
      s := s +', ';
    end;
  end;
if AllyCount = 0 then
  s := 'Player:'+PChar(@LocalData.playernames[1]) + ', allied no one.';
DataReport.Lines.Add( s );
{
for i := 1 to 10 do
  begin
  DataReport.Lines.Add( 'PlayerIndex:'+IntToStr(i) );
  DataReport.Lines.Add( 'PlayerName:'+LocalData.playernames[i] );
  DataReport.Lines.Add( 'PlayerColor:'+IntToStr(LocalData.playerColors[i]) );
  DataReport.Lines.Add( 'deathtime:'+IntToStr(LocalData.DeathTimes[i]) );
  DataReport.Lines.Add( 'incomeM:'+FloatToStr(LocalData.IncomeMetal[i] ));
  DataReport.Lines.Add( 'incomeE:'+FloatToStr(LocalData.IncomeEnergy[i] ));
  DataReport.Lines.Add( 'totalM:'+FloatToStr(LocalData.TotalMetal[i] ));
  DataReport.Lines.Add( 'totalE:'+FloatToStr(LocalData.TotalEnergy[i] ));;
  DataReport.Lines.Add( 'yehaplayground:'+IntToStr(LocalData.yehaplayground[i]) );
  DataReport.Lines.Add( 'storedM:'+FloatToStr(LocalData.storedM[i] ));
  DataReport.Lines.Add( 'storedE:'+FloatToStr(LocalData.storedE[i]));
  DataReport.Lines.Add( 'storageM:'+FloatToStr(LocalData.storageM[i]) );
  DataReport.Lines.Add( 'storageE:'+FloatToStr(LocalData.storageE[i] ));
  DataReport.Lines.Add( 'otherMapX:'+IntToStr(LocalData.otherMapX[i]) );
  DataReport.Lines.Add( 'otherMapY:'+IntToStr(LocalData.otherMapY[i]) );
  end;
}
end;{DoSaveStats}

procedure TForm1.DoCheck;
begin
// check the state of TA demo recorder
if hMemMap = 0 then
  OpenMemMap( hMemMap, SharedData );

// have a valid memmap but TA demo isnt garrientied to be in a valid state
if (SharedData <> nil) then
  ChangeState( (SharedData^.tastatus = TALobby) or
               (SharedData^.tastatus = TAInGame) );
end; {DoCheck}

procedure TForm1.TAHookCheckTimer(Sender: TObject);
begin
DoCheck;
end;

procedure TForm1.UpdateTimer(Sender: TObject);
begin
DoSaveStats;
end;

procedure TForm1.FormDestroy(Sender: TObject);
begin
CloseMemMap( hMemMap,SharedData );
end;

procedure TForm1.FormCreate(Sender: TObject);
begin
ChangeState(false);
DoCheck;
end;

end.
