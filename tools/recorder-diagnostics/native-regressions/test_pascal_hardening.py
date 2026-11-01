"""Native x86 tests of actual source; generated probes stay outside the PR.

Uses portable FPC in Delphi mode, R+/Q+. This is not a Delphi 7 release build.
Registry tests use only a process-private file hive, never HKCU/HKLM.
"""
from pathlib import Path
import subprocess
import json

import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[3])
parser.add_argument('--out',type=Path,default=Path(__file__).resolve().parents[1]/'build'/Path(__file__).stem)
parser.add_argument('--baseline',default='dcff5ddeb6bd1030e3f452c0f16e5f005850f62f')
parser.add_argument('--fpc',type=Path,required=True,help='FPC root containing bin/i386-win32 and units/i386-win32')
args=parser.parse_args()
ROOT=args.source.resolve(); OUT=args.out.resolve(); FPC=args.fpc.resolve()
OUT.mkdir(parents=True,exist_ok=True)
source = (ROOT/'src/packet_old.pas').read_text(encoding='cp1252')
old = subprocess.check_output(['git','-c','safe.directory='+str(ROOT),'-C',str(ROOT),'show',args.baseline+':src/packet_old.pas']).decode('cp1252')
def decomp(s, name):
    first = 'class function TPacket.Decompress(const Data :string) :string;'
    return (first+s.split(first,1)[1].split('end; {Decompress}',1)[0]+'end;').replace('TPacket.Decompress',name)

probe = r'''program NativeHardening;
{$mode delphi}{$H+}{$R+}{$Q+}
uses Windows, SysUtils, Registry, RecorderRegistry, RecorderClock;
type TLog = class
  class procedure Add(Level: Integer; const S: string); static;
end;
var DecompressionBufferSize: Integer; Checks: Integer;
class procedure TLog.Add(Level: Integer; const S: string); begin end;
procedure Check(OK: Boolean; const What: string);
begin Inc(Checks); if not OK then raise Exception.Create(What); end;
__OLD__
__NEW__
function RegLoadAppKeyA(Name: PAnsiChar; var Key: HKEY; Access, Options,
  Reserved: DWORD): Longint; stdcall; external 'advapi32.dll';
procedure TestPackets;
var S, Expected, Encoded, A, B: string; I, J, N, Cache: Integer;
    Failed: Boolean; Bad: array[0..4] of string;
begin
  // Real old implementation faults before decoding this valid six-byte packet.
  S := #4#0#0#8'ABC'#0#0;
  for Cache := 0 to 2 do begin
    DecompressionBufferSize := Cache; Failed := False;
    try A := OldDecompress(S); except on E: ERangeError do Failed := True; end;
    Check(Failed, 'baseline tiny cache did not reproduce');
    DecompressionBufferSize := Cache;
    Check(NewDecompress(S) = #3#0#0'ABC', 'tiny cache fix');
  end;
  DecompressionBufferSize := -1;
  Check(NewDecompress(S) = #3#0#0'ABC', 'negative cache');
  DecompressionBufferSize := High(Integer);
  Check(NewDecompress(S) = #3#0#0'ABC', 'huge cache must not allocate huge buffer');
  Check(NewDecompress(#4#0#0#6'A'#$10#0#0#0) = #3#0#0'AAA', 'overlap reference');
  for I := 0 to 15 do begin
    S := #4#0#0#6'A'+Char($10 or I)+#0#0#0;
    Expected := #3#0#0+StringOfChar('A',I+3);
    DecompressionBufferSize := 2048; Check(OldDecompress(S)=Expected,'old reference run');
    DecompressionBufferSize := 3; Check(NewDecompress(S)=Expected,'reference run changed');
  end;
  S := #4#12#34#0;
  Check(NewDecompress(S)=OldDecompress(S),'legacy partial flag group changed');
  S := #4#12#34;
  Check(NewDecompress(S)=OldDecompress(S),'legacy header-only packet changed');
  Bad[0] := ''; Bad[1] := #4; Bad[2] := #4#0;
  Bad[3] := #4#0#0#1#$10; Bad[4] := #4#0#0#1#$10#0;
  for I := 0 to High(Bad) do begin
    Failed := False;
    try A := NewDecompress(Bad[I]); except on E: EConvertError do Failed := True; end;
    Check(Failed, 'malformed packet was not explicitly rejected');
  end;
  // Generate legal wire packets independently, including overlapping references.
  RandSeed := 42731;
  for N := 1 to 1000 do begin
    Expected := #3#0#0; Encoded := #4#0#0;
    for I := 1 to 1+Random(8) do begin
      S := ''; Cache := 0;
      for J := 0 to 7 do begin
        if (Length(Expected)>3) and (Random(3)=0) then begin
          A := ''; Cache := Cache or (1 shl J);
          B := Expected; // Reference into the existing output, then allow overlap.
          repeat A := A + Char(Random(256)); until Length(A)=2;
          // Keep the word within the actual 12-bit reference format.
          DecompressionBufferSize := 1+Random(Length(Expected)-3);
          if DecompressionBufferSize>4095 then DecompressionBufferSize:=4095;
          A[1] := Char((DecompressionBufferSize shl 4) and $ff);
          A[2] := Char(DecompressionBufferSize shr 4);
          S := S+A;
          Expected := Expected+Expected[DecompressionBufferSize+3];
          Expected := Expected+Expected[DecompressionBufferSize+4];
        end else begin
          A := Char(Random(256)); S := S+A; Expected := Expected+A;
        end;
      end;
      Encoded := Encoded+Char(Cache)+S;
    end;
    Encoded := Encoded+#1#0#0; // Dedicated terminator group.
    DecompressionBufferSize := 2048; A := OldDecompress(Encoded);
    DecompressionBufferSize := 3; B := NewDecompress(Encoded);
    Check(A=Expected, 'baseline legal packet');
    Check(B=A, 'valid wire output changed');
  end;
  Writeln('packets: old cache fault reproduced; 1000 valid packets byte-identical');
end;
procedure TestClock;
var Current, Previous: Longword; A,B: Integer; Elapsed: Word;
  Failed: Boolean; I: Integer;
begin
  Current := $80000000; Previous := $7fffffff; Failed := False;
  try A := Integer(Current); B := A-Integer(Previous);
  except on E: EIntOverflow do Failed := True; end;
  Check(Failed,'old signed timestamp subtraction overflow not reproduced');
  Check(RecorderTimeDelta(Current,Previous)=1,'signed boundary delta');
  Check(RecorderTimeDelta(0,$ffffffff)=1,'DWORD rollover delta');
  Check(RecorderTimeDelta(1,$fffffffe)=3,'DWORD rollover delta 3');
  Check(RecorderTimeDelta(65536,0)=0,'two-byte format changed');
  Check(RecorderTimeDelta(65535,0)=65535,'maximum two-byte delta');
  for I := 1 to 4096 do begin
    Previous := (Longword(Random(65536)) shl 16) or Longword(Random(65536));
    Elapsed := Random(65536);
    Current := Longword((Int64(Previous)+Elapsed) and $ffffffff);
    Check(RecorderTimeDelta(Current,Previous)=Elapsed,'known elapsed delta changed');
  end;
  Writeln('clock: old signed-boundary overflow reproduced; wire deltas preserved');
end;
procedure TestRegistry;
var Hive, Key, Missing: HKEY; R, Denied: TRecorderRegistry; RC: Longint;
  Disp, Kind, Size, D: DWORD; S, Big: string; Small: Byte;
  FileName: string; OldRegistry: TRegistry; Failed: Boolean;
  procedure Put(const Name: string; AType: DWORD; P: Pointer; Count: DWORD);
  begin Check(RegSetValueExA(Key,PAnsiChar(Name),0,AType,P,Count)=0,'fixture write'); end;
begin
  FileName := ParamStr(1);
  Check(FileName<>'','private hive path required');
  Check(RegLoadAppKeyA(PAnsiChar(FileName),Hive,KEY_ALL_ACCESS,1,0)=0,'private hive');
  try
    Check(RegCreateKeyExA(Hive,'Fixture\Options',0,nil,0,KEY_ALL_ACCESS,nil,Key,@Disp)=0,'private key');
    R := TRecorderRegistry.Create(Hive,'Fixture');
    try
      Check(R.ReadInteger('Absent','i',37)=37,'missing integer default');
      Check(RegOpenKeyExA(Hive,'Fixture\Absent',0,KEY_READ,Missing)=ERROR_FILE_NOT_FOUND,'read created key');
      S := 'wrong-type'; Put('i',REG_SZ,PAnsiChar(S),Length(S));
      OldRegistry := TRegistry.Create(KEY_READ);
      try
        OldRegistry.RootKey := Hive;
        Check(OldRegistry.OpenKeyReadOnly('Fixture\Options'),'old typed reader fixture');
        Failed := False;
        try D := OldRegistry.ReadInteger('i');
        except on E: ERegistryException do Failed := True; end;
        Check(Failed,'old typed reader exception not reproduced');
      finally OldRegistry.Free; end;
      Check(R.ReadInteger('Options','i',2048)=2048,'wrong integer type');
      D := $ffffffff; Put('i',REG_DWORD,@D,4);
      Check(R.ReadInteger('Options','i',2048)=-1,'signed DWORD bits');
      Put('i',REG_DWORD,@D,1);
      Check(R.ReadInteger('Options','i',2048)=2048,'short DWORD');
      S := 'abc'; Put('path',REG_SZ,PAnsiChar(S),Length(S));
      Check(R.ReadString('Options','path','default')='abc','unterminated string');
      Put('path',REG_DWORD,@D,4);
      Check(R.ReadString('Options','path','default')='default','wrong string type');
      S := ''; Put('path',REG_SZ,nil,0);
      Check(R.ReadString('Options','path','default')='','empty string');
      S := 'a'#0'b'; Put('path',REG_SZ,PAnsiChar(S),Length(S));
      Check(R.ReadString('Options','path','default')='a','embedded nul');
      SetLength(Big,65537); FillChar(Big[1],Length(Big),Ord('x'));
      Put('path',REG_SZ,PAnsiChar(Big),Length(Big));
      Check(R.ReadString('Options','path','default')='default','oversized path');
      Small := 1; Put('b',REG_BINARY,@Small,1);
      Check(R.ReadBool('Options','b',False),'historical binary bool');
      D := 0; Put('b',REG_DWORD,@D,4);
      Check(not R.ReadBool('Options','b',True),'DWORD bool');
      Put('b',REG_SZ,PAnsiChar('false'),6);
      Check(R.ReadBool('Options','b',True),'wrong bool type default');
      Check(R.WriteString('Options','saved','abc'),'save string');
      Size := 4; D := 0;
      Check(RegQueryValueExA(Key,'saved',nil,@Kind,@D,@Size)=0,'query saved');
      Check((Size=4) and ((D shr 24)=0),'saved NUL byte');
      Denied := TRecorderRegistry.Create(0,'InvalidHandle');
      try
        Check(not Denied.WriteInteger('','i',1),'failed persistence not returned');
        Check(Denied.ReadInteger('','i',17)=17,'failed open default');
      finally Denied.Free; end;
    finally R.Free; RegCloseKey(Key); end;
  finally RegCloseKey(Hive); end;
  Check(not EnsureRecorderDirectory(''),'empty directory');
  Check(not EnsureRecorderDirectory(FileName+'\child'),'file used as directory');
  Writeln('registry: typed defaults, bounded paths, private hive, failed persistence');
end;
begin
  Checks := 0; TestPackets; TestRegistry; TestClock;
  Writeln('PASS checks=',Checks);
end.
'''.replace('__OLD__',decomp(old,'OldDecompress')).replace('__NEW__',decomp(source,'NewDecompress'))
path=OUT/'native_hardening.pas'; path.write_text(probe,encoding='cp1252')
args=[str(FPC/'bin/i386-win32/ppc386.exe'),'-n','-Mdelphi','-Cr','-Co','-gl',
      '-Fu'+str(FPC/'units/i386-win32/rtl'),
      '-Fu'+str(FPC/'units/i386-win32/rtl-objpas'),
      '-Fu'+str(FPC/'units/i386-win32/fcl-base'),
      '-Fu'+str(FPC/'units/i386-win32/fcl-registry'),'-Fu'+str(ROOT/'src/Recorder'),
      '-FU'+str(OUT),'-FE'+str(OUT),str(path)]
compiled=subprocess.run(args,capture_output=True,text=True)
print(compiled.stdout,compiled.stderr,end='')
compiled.check_returncode()
run=subprocess.run([str(OUT/'native_hardening.exe'),str(OUT/'fixture.hiv')],capture_output=True,text=True)
print(run.stdout,run.stderr,end='')
(OUT/'result.json').write_text(json.dumps({'compiler':'FPC 3.2.2 i386-win32 Delphi mode R+/Q+; not DCC7 release',
  'exit':run.returncode,'stdout':run.stdout,'stderr':run.stderr},indent=2))
run.check_returncode()
