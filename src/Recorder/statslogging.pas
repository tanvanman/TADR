unit statslogging;

interface
uses
  gphugef,
  PacketBufferU;
type
  TStatsLogging = class
  private
    FileName : string;
    filehandle : TGpHugeFile;
    HasFlushed : boolean;
    procedure Add( const text :string ); overload;
    procedure Add( const data : TArrayOfByte ); overload;
  public
    crc :longword;
    docrc :boolean;
    constructor Create (const Filename :string);
    destructor Destroy; override;

    //procedure Start_StatEvent(numplayers:longword; maxunits:longword);
    procedure NewUnit_StatEvent(player:longword; unitid:word; netid:word; tid:longword);
    procedure UnitFinished_StatEvent(unitid:word; tid:longword);
    procedure Damage_StatEvent(receiver:word; sender:word; amount:word; tid:longword);
    procedure Kill_StatEvent(killed:word; killer:word; tid:longword);
    procedure Stats_StatEvent(player:longword; mstored:single; estored:single; mstorage:single; estorage:single; mincome:single; eincome:single; tid:longword);

    procedure Flush();
  end;

implementation

uses
  windows,
  SysUtils,
  TADemoConsts,
  textdata,
  logging,
  uDebug;

constructor TStatsLogging.Create(const Filename :string);
begin
inherited Create;
Self.Filename := Filename;

filehandle := TGpHugeFile.CreateEx( FileName,
                                    FileAttributes,
                                    AccessMode[true],
                                    ShareFlags[IsWin9x]);
Win32Check(filehandle.RewriteEx(1,1024,0,0,[hfoBuffered,hfoCompressed],0) <> hfError);
HasFlushed := true;
crc := 0;
docrc := false;
end;

procedure TStatsLogging.Flush();
begin
if not HasFlushed then
  begin
  HasFlushed := true;
  filehandle.Flush;
  end;
end; {Flush}

procedure TStatsLogging.Add(const text :String);
var
  ReturnAddr : longword;
  len : Integer;
begin
assert( filehandle <> nil );
len := Length(text);
if len > 0 then
  begin
  HasFlushed := false;
  filehandle.BlockWriteUnsafe( text[1], len );
  if docrc then
    crc := CalcCRC(crc, @text[1], len);
  end
else
  begin
  asm
    PUSH EAX;
    MOV EAX,[EBP+4];
    MOV ReturnAddr, EAX;
    POP EAX;
  end;
  TLog.Add(0,'Empty string passed to TStatsLogging.Add from $'+IntToHex( ReturnAddr, 8) );
  LogError( ReturnAddr );
  end;
end;

procedure TStatsLogging.Add( const data : TArrayOfByte );
var
  ReturnAddr : Longword;
  len : Integer;
begin
assert( filehandle <> nil );
len := Length(data);
if len > 0 then
  begin
  HasFlushed := false;
  filehandle.BlockWriteUnsafe( data[0], len );
  if docrc then
    crc := CalcCRC( crc, @data[0], len);
  end
else
  begin
  asm
    PUSH EAX;
    MOV EAX,[EBP+4];
    MOV ReturnAddr, EAX;
    POP EAX;
  end;
  TLog.Add(0,'Empty data passed to TStatsLogging.Add from $'+IntToHex( ReturnAddr, 8) );
  LogError( ReturnAddr );
  end;
end; {Add}

procedure TStatsLogging.NewUnit_StatEvent(player:longword; unitid:word; netid:word; tid:longword);
begin
  Add(IntToStr(tid) + ';' + 'NewUnit' + ';'  + IntToStr(player) + ';' + IntToStr(unitid) + ';' + IntToStr(netid) + #13#10);
end;

procedure TStatsLogging.UnitFinished_StatEvent(unitid:word; tid:longword);
begin
  Add(IntToStr(tid) + ';' + 'UnitFinished' + ';' + IntToStr(unitid) + #13#10);
end;

procedure TStatsLogging.Damage_StatEvent(receiver:word; sender:word; amount:word; tid:longword);
begin
  Add(IntToStr(tid) + ';' + 'Damage' + ';' + IntToStr(sender) + ';' + IntToStr(receiver) + ';' + IntToStr(amount) + #13#10);
end;

procedure TStatsLogging.Kill_StatEvent(killed:word; killer:word; tid:longword);
begin
  Add(IntToStr(tid) + ';' + 'Kill' + ';' + IntToStr(killer) + ';' + IntToStr(killed) + #13#10);
end;

procedure TStatsLogging.Stats_StatEvent(player:longword; mstored:single; estored:single; mstorage:single; estorage:single; mincome:single; eincome:single; tid:longword);
begin
  Add(IntToStr(tid) + ';' + 'Stats' + ';' + IntToStr(player) + ';' + FloatToStrF(mstored, ffGeneral, 12, 4) + ';' + FloatToStrF(estored, ffGeneral, 12, 4) + ';' + FloatToStr(mstorage) + ';' + FloatToStr(estorage) + ';' + FloatToStrF(mincome, ffGeneral, 12, 4) + ';' + FloatToStrF(eincome, ffGeneral, 12, 4) + #13#10);
end;

destructor TStatsLogging.Destroy;
begin
FreeAndNil( filehandle );
inherited Destroy;
end;

end.
