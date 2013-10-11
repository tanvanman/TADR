unit logging;

interface

type
  TLog = class
  private
    FileName :string;
  public
    constructor Create (Filename :string);
    destructor Destroy; override;

    procedure Add (st :string); overload;
    procedure Add (st :string; g :TGuid); overload;
    procedure Add (st :string; p :pointer); overload;
    procedure Add (st :string; dw :cardinal); overload;
    procedure Add (st :string; p :pointer; size :integer); overload;
  end;

var
  Log :TLog;
//procedure Log (st :String);

implementation

uses
  SysUtils, TextData;

var
  SaveExit :pointer;
  starttime :TTimeStamp;

constructor TLog.Create (Filename :string);
var
  LogFile :TextFile;
begin
  inherited Create;

{$IFNDEF RELEASE}
  Self.Filename := Filename;
  Assign (LogFile, Filename);
  Rewrite (Logfile);
  Close (Logfile);
  starttime := DateTimeToTimeStamp (now);

  Add ('---- start ------------------');
{$ENDIF}
end;

destructor TLog.Destroy;
begin
  Add ('---- slut -------------------');
  inherited Destroy;
end;

procedure TLog.Add (st :String);
var
  LogFile :TextFile;
  tme :TTimeStamp;
begin
  {$I-}

{$IFNDEF RELEASE}
  AssignFile (LogFile, Filename);
  Append (Logfile);

  tme := DateTimeToTimeStamp (now);

  if Length (st) = 0 then
  begin
    Writeln (logfile, 'huh trying to log an empty string');
    exit;
  end;

  if st[1]='!' then
     Writeln (Logfile, st, ' - ', tme.Time - starttime.Time)
  else
      Writeln(Logfile,st);

  CloseFile (LogFile);
 {$ENDIF}

  {$I+}
end;

procedure TLog.Add (st :string; g :TGuid);
var
  s :String;
  i :integer;
begin
  s := '{' + HexToStr (g.d1, 8) + '-' + HexToStr (g.d2, 4) + '-' +
    HexToStr (g.d3, 4) + '-';

  for i := 0 to 7 do
  begin
    s := s + HexToStr(g.d4[i], 2);
  end;
  s := s + '}{';

  Add (st + s);
end;

procedure TLog.Add (st :string; p :pointer);
begin
  Add (st + HexToStr (Cardinal (p), 8));
end;

procedure TLog.Add (st :string; dw :cardinal);
begin
  Add (st + IntToStr (dw));
end;

procedure TLog.Add (st :string; p :pointer; size :integer);
begin
  Add (st + DataToStr (p, size));
end;

procedure LogExit;
begin
  Log.Free;
  ExitProc := SaveExit;
end;

begin
  Log := TLog.Create('c:\log.txt');

  SaveExit := ExitProc;
  ExitProc := @LogExit;
end.

