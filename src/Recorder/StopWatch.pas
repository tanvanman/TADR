unit StopWatch;
 
interface
 
uses Windows, SysUtils, DateUtils;
 
type
  TStopWatch = class
  private
    fFrequency : TLargeInteger;
    fIsRunning : Boolean;
    fIsHighResolution : Boolean;
    fStartCount, fStopCount : TLargeInteger;
    procedure SetTickStamp(var lInt : TLargeInteger);
    function GetElapsedMilliseconds: TLargeInteger;
  public
    constructor Create(const startOnCreate : Boolean = False);
    procedure Start;
    procedure Stop;
    property IsHighResolution : Boolean read fIsHighResolution;
    property IsRunning : Boolean read fIsRunning;
    property ElapsedMilliseconds : TLargeInteger read GetElapsedMilliseconds;
  end;
 
implementation
 
constructor TStopWatch.Create(const startOnCreate : Boolean = False);
begin
  inherited Create;
 
  fIsRunning := false;
 
  fIsHighResolution := QueryPerformanceFrequency(fFrequency);
  if not fIsHighResolution then
    fFrequency := MSecsPerSec;
 
  if startOnCreate then Start;
end;
 
 //function TStopWatch.GetElapsedTicks: TLargeInteger;
 //begin
//   result := fStopCount - fStartCount;
// end;
 
procedure TStopWatch.SetTickStamp(var lInt : TLargeInteger);
begin
  if fIsHighResolution then
    QueryPerformanceCounter(lInt)
  else
    lInt := MilliSecondOf(Now);
end;
 
function TStopWatch.GetElapsedMilliseconds: TLargeInteger;
begin
  Result := (MSecsPerSec * (fStopCount - fStartCount)) div fFrequency;
end;
 
procedure TStopWatch.Start;
begin
  SetTickStamp(fStartCount);
  fIsRunning := True;
end;
 
procedure TStopWatch.Stop;
begin
  SetTickStamp(fStopCount);
  fIsRunning := False;
end;

end.
