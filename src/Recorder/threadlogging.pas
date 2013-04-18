unit threadlogging;

interface
{$IFDEF ThreadLogging}
uses
  contnrs,
  classes;
type
  TThreadLogger = class
  public
    KnownThreads : TObjectList;

    constructor Create;
    Destructor destroy; override;

    
    Procedure Report;
    procedure LogThreadID;
    function InputHookTest( const Command : string;
                                    params : TStringList ) : Boolean;


  end;

var
  ThreadLogger : TThreadLogger;                                   
{$ENDIF}

implementation
{$IFDEF ThreadLogging}
uses
  windows,
  sysutils,
  uDebug,
  logging,
  InputHook;

constructor TThreadLogger.Create;
begin
KnownThreads := TObjectList.create(true);
  AddInputHook(InputHookTest,'test');
end;

Destructor TThreadLogger.destroy;
begin
Report;
FreeAndNil( KnownThreads );
end;

type
  TMyLogObject = class
    ThreadID : longword;
    CallLocations : array [0..100] of Longword;
  end;

Procedure TThreadLogger.Report;
var
  i,i2 : Integer;
  CallerAddress : Longword;
  MyLogObject :TMyLogObject;
  s : string;
begin  
  if KnownThreads <> nil then
    begin
    if not MapFileSetup then
      LoadAndParseMapFile;
    TLog.Add( 0, 'Known thread IDs:'+IntToStr(KnownThreads.Count) );
    for i := 0 to KnownThreads.Count-1 do
      begin
      MyLogObject := TMyLogObject(KnownThreads[i]);
      TLog.Add( 0, 'ThreadID:'+IntToStr(MyLogObject.ThreadID) );
      for i2 := Low(MyLogObject.CallLocations) to High(MyLogObject.CallLocations) do
        begin
        if MyLogObject.CallLocations[i2] = 0 then
          Continue;
        CallerAddress := GetMapAddressFromAddress(MyLogObject.CallLocations[i2]);
        TLog.Add( 0, 'Caller #'+IntToStr(i2) );
        s := GetModuleNameFromAddress(CallerAddress);
        if s <> '' then
          TLog.Add( 0,s+':'+GetProcNameFromAddress(CallerAddress)+':'+GetLineNumberFromAddress(CallerAddress));
        end;
      end;
    end;
end;

function TThreadLogger.InputHookTest( const Command : string;
                                      params : TStringList ) : Boolean;
begin
LogThreadID;
result := false;
end;

procedure TThreadLogger.LogThreadID;
var
  i : Integer;
  ThreadID : Longword;
  MyLogObject : TMyLogObject;
  ReturnAddr : Longword;

  procedure FillCallerSlot;
  var i : Integer;
  begin
  Assert( MyLogObject <> nil );
  for i := Low(MyLogObject.CallLocations) to High(MyLogObject.CallLocations) do
    if (MyLogObject.CallLocations[i] = 0) then
      begin
      MyLogObject.CallLocations[i] := ReturnAddr;
      exit;
      end
    else if MyLogObject.CallLocations[i] = ReturnAddr then
      Exit;
  raise Exception.Create('should not get here');
  end; {FillCallerSlot}

begin
asm
  push eax;
  MOV EAX,[EBP+4];
  Mov ReturnAddr, EAX;
  pop eax;
end;
ThreadID := GetCurrentThreadId;
if KnownThreads <> nil then
  begin
  for i := 0 to KnownThreads.Count-1 do
    begin
    MyLogObject := TMyLogObject(KnownThreads[i]);
    if MyLogObject.ThreadID = ThreadID then
      begin
      FillCallerSlot;
      Exit;
      end;
    end;
  end
else KnownThreads := TObjectList.Create(true);

MyLogObject := TMyLogObject.Create;
KnownThreads.add( MyLogObject );
MyLogObject.ThreadID := ThreadID;
FillCallerSlot;
end; {LogThreadID}
{$ENDIF}


end.
