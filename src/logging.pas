unit logging;

interface
uses
  SyncObjs,
  SysUtils,
  TextFileU;

type
  TLog = class
  private
    LogFile : TTextFile;
    FileName :string;
    starttime :TTimeStamp;
    fVerboseLoggingLevel : Integer;
    procedure Add2( aVerboseLoggingLevel : Integer; const s :String );
  public
    cs : TCriticalSection;
      
    constructor Create (const aFilename :string; reopenfile : Boolean; aVerboseLoggingLevel : Integer);
    destructor Destroy; override;

    {$IFDEF PacketLogging}
  protected
    LogFile2 : TTextFile;
    procedure Add4( aVerboseLoggingLevel : Integer; const s :String );
  public
    class procedure Add3( aVerboseLoggingLevel : Integer; const s :String );
    {$ENDIF}

    class procedure Add ( aVerboseLoggingLevel : Integer; const s :String ); overload;
    class procedure Add ( aVerboseLoggingLevel : Integer; const s :String ; g :PGuid); overload;
//    class procedure Add ( aVerboseLoggingLevel : Integer; const s :String ; p :pointer); overload;
    class procedure Add ( aVerboseLoggingLevel : Integer; const s :String ; dw :cardinal); overload;

    Class procedure Flush();

    property VerboseLoggingLevel : Integer read fVerboseLoggingLevel Write fVerboseLoggingLevel;
  end;

const
  VerboseLoggingLevelc = 5;
var
  logFilename : string;
  logdir : string = '.';
  Log_ : TLog;
//procedure Log (st :String);

procedure LogException(E: Exception);

procedure LogError( errorAddress : pointer ); overload;
procedure LogError( errorAddress : Longword ); overload;


implementation

uses
  windows, consts,
  TextData,
  uDebug,
  FreeListU;


procedure LogException(E: Exception);
begin
try
  Assert(E <> nil);
  TLog.Add(0, 'Exception: '+e.message );
  LogError( Longword(ExceptAddr) );
finally
  TLog.Flush;
end;
end; {LogException}

procedure LogError( errorAddress : longword );
var
  s : string;
begin  
try
  Assert(errorAddress <> 0);
  if not MapFileSetup then
    LoadAndParseMapFile;
  errorAddress := GetMapAddressFromAddress(Longword(errorAddress));
  s := GetModuleNameFromAddress(errorAddress);
  if s <> '' then
    TLog.Add( 0,s+':'+GetProcNameFromAddress(errorAddress)+':'+GetLineNumberFromAddress(errorAddress));
finally
  TLog.Flush;
end;
end;

procedure LogError( errorAddress : pointer );
begin
LogError( longword(errorAddress) );
end;

constructor TLog.Create(const aFilename : string; reopenfile : Boolean; aVerboseLoggingLevel : Integer);
begin
inherited Create;
cs := TCriticalSection.create;
if (Log_ <> nil) then
  raise Exception.Create('Only one instance of TLog can only be created at any given time');
Log_ := Self;
if ObjectsToFree <> nil then
  ObjectsToFree.add(Self);
logdir := IncludeTrailingPathDelimiter(logdir);
try
  ForceDirectories( logdir );
except
  on e : EInOutError do
    begin
    logdir := '';
    end;
end;
Self.Filename := logdir+RemoveInvalid(aFilename);
fVerboseLoggingLevel := aVerboseLoggingLevel;
if aFilename <> '' then
  begin
if reopenfile then
  begin
  try
    LogFile := TTextFile.Create_Append( Filename );
  except
    on e : EGpHugeFile do ;
  end;
  if LogFile <> nil then
    Add(0,'---- reopened --------------');
  end;
if LogFile = nil then
  begin
  try
    LogFile := TTextFile.Create_Rewrite( Filename );
  except
    on e : EGpHugeFile do ;
  end;
  starttime := DateTimeToTimeStamp( Now );
  Add(0,'---- start -----------------');
  end;
  end;
{$IFDEF PacketLogging}
LogFile2 := TTextFile.Create_Rewrite( logdir+RemoveInvalid('data_'+aFilename) );
{$ENDIF}
TLog.Flush;
end; {create}

destructor TLog.Destroy;
begin
Log_ := Self;
Add(0,'---- end -------------------');
FreeAndNil( LogFile );
{$IFDEF PacketLogging}
FreeAndNil( LogFile2 );
{$ENDIF}
Log_ := nil;
if ObjectsToFree <> nil then
  ObjectsToFree.extract(Self);
FreeAndNil( cs );  
inherited Destroy;
end;

class procedure TLog.Flush;
begin
if (Log_ <> nil) then
  begin
  if Log_.LogFile <> nil then
    Log_.LogFile.Flush;
  {$IFDEF PacketLogging}
  if Log_.LogFile2 <> nil then
    Log_.LogFile2.Flush;
  {$ENDIF}
  end;
end;

procedure TLog.Add2( aVerboseLoggingLevel : Integer; const s :String );
var
  tme :TTimeStamp;
  s2 : string;
begin
//if LogFile = nil then Exit;
if aVerboseLoggingLevel > VerboseLoggingLevel then
  Exit;
tme := DateTimeToTimeStamp (now);
if (s <> '') and (s[1]='!') then
   s2 := s+ ' - '+IntToStr( tme.Time - starttime.Time )
else
   s2 := s;
if (LogFile <> nil) and (Filename <> '') then
  LogFile.Writeln( s2 )
else
  writeln(s);  
end; {Add2}

class procedure TLog.Add (aVerboseLoggingLevel : Integer;const s :String );
begin
if Log_ = nil then
  Log_ := TLog.Create( logFilename, True, VerboseLoggingLevelc );
Log_.cs.Acquire;
try
  Log_.Add2( aVerboseLoggingLevel, s );
finally
  Log_.cs.Release;
end;
end;

{$IFDEF PacketLogging}
procedure TLog.Add4( aVerboseLoggingLevel : Integer; const s :String );
var
  tme :TTimeStamp;
  s2 : string;
begin
if LogFile = nil then Exit;
if aVerboseLoggingLevel > VerboseLoggingLevel then
  Exit;
tme := DateTimeToTimeStamp (now);
if (s <> '') and (s[1]='!') then
   s2 := s+ ' - '+IntToStr( tme.Time - starttime.Time )
else
   s2 := s;
LogFile2.Writeln( s2 );
end; {Add4}

class procedure TLog.Add3( aVerboseLoggingLevel : Integer; const s :String );
begin
if Log_ = nil then
  Log_ := TLog.Create( logFilename, True, VerboseLoggingLevelc );
Log_.Add4( aVerboseLoggingLevel, s );
end;
{$ENDIF}
class procedure TLog.Add (aVerboseLoggingLevel : Integer;const s :String ; g :PGuid);
var
  s2 :String;
  i :integer;
begin
if g <> nil then
  begin
  s2 := '{' + IntToHex (g.d1, 8) + '-' + IntToHex (g.d2, 4) + '-' + IntToHex (g.d3, 4) + '-';
  for i := 0 to 7 do
    s2 := s2 + IntToHex(g.d4[i], 2);
  s2 := s2 + '}';
  end
else
  s2 := '{}';  
Add( aVerboseLoggingLevel, s + s2);
end;

{
class procedure TLog.Add (aVerboseLoggingLevel : Integer;const s :String ; p :pointer);
begin
Add( aVerboseLoggingLevel, s + IntToHex (Cardinal (p), 8));
end;
}

class procedure TLog.Add (aVerboseLoggingLevel : Integer;const s :String ; dw :cardinal);
begin
Add( aVerboseLoggingLevel, s + IntToStr (dw));
end;

end.


