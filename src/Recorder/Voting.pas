unit Voting;

interface
uses
  extctrls;

type
  TVoteTimer = Class(TTimer)
  protected
    procedure VoteExpired(Sender: TObject);
  public
    procedure SetExpire(Seconds: Word);
    procedure Stop;
  end;

var
  VoteInProgress : boolean;
  VoteTimer: TVoteTimer;

implementation
 
procedure TVoteTimer.VoteExpired(Sender: TObject);
begin
  VoteInProgress:= False;
  VoteTimer.Stop;
  // send reset gui packet
  // sendchat vote expired
end;

procedure TVoteTimer.SetExpire(Seconds: Word);
begin
  VoteTimer := TVoteTimer.Create(nil);
  VoteTImer.Interval:= Seconds * 1000;
  VoteTimer.OnTimer := VoteTimer.VoteExpired;
  VoteTimer.Enabled:= True;
end;

procedure TVoteTimer.Stop;
begin
  VoteTimer.Free;
end;

end.
