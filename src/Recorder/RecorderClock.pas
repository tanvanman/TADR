unit RecorderClock;

// Preserve the existing two-byte replay delta across DWORD clock boundaries.
// Use wide subtraction so Delphi's overflow checks cannot reject a valid tick.
interface

function RecorderTimeDelta(CurrentTick, PreviousTick: Longword): Word;

implementation

function RecorderTimeDelta(CurrentTick, PreviousTick: Longword): Word;
begin
  // Both DWORD wrap (2^32) and the file's truncation (2^16) have zero low bits.
  Result := Word((Int64(CurrentTick) - Int64(PreviousTick)) and $ffff);
end;

end.
