unit InitCode;

interface
uses
  sysutils;

type
  EValidationFailed = class(Exception);

procedure OnInitialize( OnMainRun : boolean );
procedure OnFinalize;

implementation
uses
  mmsystem,
  windows,
  logging,
  Dplayx_exports,
  FreeListU,
  PluginCommandHandlerU,
{$IFDEF ThreadLogging}
  threadlogging,
{$ENDIF}  
  TADemoConsts,
  TA_MemoryLocations,
  Plugins;



{$IFDEF release}
(*
Procedure CheckTADemoIntegrity;
var
  hMemMap : THandle;
  filehandle : THandle;
  filesize : Longword;
  buffer :pchar;
  i :integer;
  crc :longword;
  fto :^longword;
  path : string;
begin
//  log.add (paramstr (0));
try
  path := SelfLocation;
  log.add(0, path );
  filehandle := FileOpen(path, fmOpenRead or fmShareDenyNone);
  if filehandle = HFILE_ERROR then
    //hmm file open error antagligen
    raise EValidationFailed.Create( 'Please put the TA Demo Recorder''s dplayx.dll in your TA directory ' + path);

  hMemMap := CreateFileMapping($ffffffff,nil,PAGE_READWRITE,0,sizeof(MKChatMem),MemMapName );
  filesize := 0;
  crc := 0;
  i := 0;
  repeat
    fto := @buffer[i];
    crc := crc xor fto^;
    inc (i, 4);
  until i > filesize - 10;

  fto := @buffer[filesize - 4];

  if fto^ <> crc then
    raise EValidationFailed.Create ('File has been altered. DLL will not load');
finally
  if( buffer <> nil) and (s <> nil) then
    freemem (buffer, s.size);
  s.free;
end;
end; {CheckTADemoIntegrity}
*)
Procedure CheckTADemoIntegrity;
begin
end;
{$ENDIF}


//function DisableThreadLibraryCalls : longbool; stdcall;  External kernel32 name 'DisableThreadLibraryCalls';

type
  TDisableThreadLibraryCallsHandler = function( hModule : THandle ) : longbool; stdcall;// External  name 'DisableThreadLibraryCalls';
var
  proc : TDisableThreadLibraryCallsHandler;
function DisableThreadLibraryCalls( hModule : THandle ) : longbool; stdcall; //External kernel32 name 'DisableThreadLibraryCalls';
begin
Result := True;
if kernellib = 0 then
  kernellib := LoadLibrary( kernel32 );
if kernellib <> 0 then
  begin
  proc := GetProcAddress( kernellib, 'DisableThreadLibraryCalls' );
  if Assigned(proc) then
    Result := proc( hModule );
  end;
end;{DisableThreadLibraryCalls}


procedure OnInitialize( OnMainRun : boolean );
var
  aTimeCaps : TTimeCaps;
begin
{$IFDEF release}
CheckTADemoIntegrity;
{$ENDIF}
try
if not OnMainRun then
  begin
  FreeListU.OnInitialize();
  
  // Helps reduce the number of DLLs which care about thread creation & deletion
  // Since when a thread is created, every DLL which cares needs to be notified
  DisableThreadLibraryCalls( hinstance );
  // Make sure we are using the smallest resolution for timeGetTime,
  // we dont bother about cleaning it up, cos the only time it needs to be is
  // when TA is just about to quit
  timeGetDevCaps( @aTimeCaps, SizeOf(aTimeCaps) );
  timeBeginPeriod( aTimeCaps.wPeriodMin );
  // create the logs directory
  if (logdir = '') or (logdir = '.') then
    logdir := IncludeTrailingPathDelimiter( ExtractFilePath( SelfLocation ) + 'log');
  // Determine the log file name
  if logFilename = '' then
    logFilename := 'TA Demo Recorder Log -'+DateTimeToStr(now)+'.txt';
  // create the log file
  if Log_ = nil then
    Log_ := TLog.Create( logFilename, False, VerboseLoggingLevelc );
  Tlog.add( 0, 'TADR version:'+ GetTADemoVersion );
  if IsWin9x then
    TLog.Add( 0, 'Running in Win9x compatibility mode' );
  if not IsTAVersion31 then
    TLog.Add( 0, 'Not running Total Annihilation 3.1' );
{$IFNDEF NoDplayExports}
  // load the actual dplayx.dll, we dont bother unloading it,as we only dont need it when TA is closing
  if DPlayxHandleInvalid then
    begin
{$IFDEF DplayRedirector}
    // we are redirecting calls back on ourself
    dplayxLibHandle := LoadLibrary(Pchar(Paramstr(0)));
{$ELSE}
    dplayxLibHandle := LoadLibrary(PChar( GetSysDir + 'dplayx.dll' ));
{$ENDIF}
    end;
{$ENDIF}    
  // seed the random number generator (not that random)
  Randomize;

{$IFDEF ThreadLogging}
  ThreadLogger := TThreadLogger.create;
  ObjectsToFree.add( ThreadLogger );
{$ENDIF}
  
  end;
  // Do code injections
  Do_LoadTime_CodeInjections( OnMainRun );
except
  on e : Exception do
    begin
    LogException(e);
    raise;
    end;
end;
end; {DoInitialize}

Procedure OnFinalize;
begin
UninstallCodeInjections;
FreeListU.OnFinalize();
{$IFNDEF NoDplayExports}
if not DPlayxHandleInvalid then
  begin
  FreeLibrary(dplayxLibHandle);
  dplayxLibHandle := 0;
  end;
{$ENDIF}
Tlog.Flush;
//logFilename := '';
end;

Initialization
DoInitialize := @OnInitialize;
DoFinalize := @OnFinalize;
end.
