unit loading;

interface

uses
  packet;
  
type
  TProgress = class (TPacket)
  protected
    function GetPercent :integer;
    procedure SetPercent (l :integer);
  public
    constructor Create (percent :integer); overload;

    property Percent :integer read GetPercent write SetPercent;
  end;

implementation

type
  PProgress = ^RProgress;
  RProgress = packed record
    Percent :byte;
    data1   :byte;
  end;

constructor TProgress.Create (percent :integer);
var
  r :RProgress;
begin
  Fillchar (r, sizeof (r), 0);
  r.Percent := percent;
  r.data1 := $06;
  inherited Create (byte ('*'), @r, sizeof (r));
end;

function TProgress.GetPercent :integer;
begin
  Result := PProgress (@Fdata[9])^.Percent;
end;

procedure TProgress.SetPercent (l :integer);
begin
  PProgress (@Fdata [9])^.Percent := l;
end;

end.
