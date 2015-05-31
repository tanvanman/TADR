unit packet_old;
interface

const
  SY_UNIT = $92549357;
             
type
  TPacket = class
  protected
    procedure SetTimestamp (l :longword);
    function GetKind :byte;
    procedure SetKind (b :byte);
//    function GetAsciiData :string;
    function GetRawData :String;
    function GetRawData2 :String;
    function GetSize :integer;
    function GetTAData :string;
  public
    FData :string;
    constructor Create (const adata :String); overload;
//    constructor Create3(data :pointer; size :integer);// overload;
//    constructor Create (kind :byte; data :pointer; size :integer); overload; //När man vill göra egna paket
    constructor Create (from :TPacket); overload;
//    constructor Create (kind :byte; data :string); overload;
//    constructor CreateNew (data :string); overload;
    constructor SJCreateNew (data :string); overload;

// assumes input is in the a raw packet format    
    Class function KnownPacketType( const s : String; index :integer) : boolean;
// assumes input is in the a raw packet format
    class function PacketLength( const s :String; index :integer) :Integer;

// assumes input is in the header+data packet format
// output is in the header+data packet format    
    class function Split (var s :string) :string;

// assumes input is in the in the raw packet format
// output is in the uses raw packet format
    class function Split2 (var s :string;smartpak:Boolean; const FromPlayer, ToPlayer : string) :string;
// assumes input is in the header+data packet format
// output is in the header+data packet format
    class function Decrypt( Data : string ) : string;
// assumes input is in the header+data packet format
// output is in the header+data packet format    
    class function Encrypt( Data : string) : string;
// assumes input is in the header+data packet format
// output is in the header+data packet format
    class function Decompress( const Data :string ) : string;
// assumes input is in the header+data packet format
// output is in the header+data packet format
    class function Compress (const Data :String) :string;

    function GetTimestamp :Longword;

    property Timestamp :longword read GetTimestamp write SetTimestamp;
    property Kind :byte read GetKind write SetKind;
    property RawData :string read GetRawData;
    property RawData2 :string read GetRawData2;
//    property AsciiData :string read GetAsciiData;
    property Size :integer read GetSize;
    property TAData :string read GetTAData;       //Krypterar och komprimerar
  end;

var
  DecompressionBufferSize : Integer = 2048;
  UseCompression : Boolean = false;
implementation

uses
  TextData, Logging, IniOptions, SysUtils;

{$WARNINGS OFF}

{--------------------------------------------------------------------}

constructor TPacket.Create( const adata : String);
begin
inherited Create;
FData := Decompress( Decrypt( adata ) );
end;

{
//Dessa är för egna paket
constructor TPacket.Create (kind :byte; data :pointer; size :integer);
begin
  inherited Create;
  FData := #$3#00#00#$ff#$ff#$ff#$ff + Char(kind) + PtrToStr (data, size);
end;


constructor TPacket.Create3(data :pointer; size :integer);
begin
inherited Create;
FData := PtrToStr (data, size);
FData := Decrypt (FData);

if FData[1] = #$04 then
  FData := Decompress (FData);
end;

constructor TPacket.Create (kind :byte; data :string);
var
  p :pointer;
begin
  p := StrToData (data);
  FData := #$3#00#00#$ff#$ff#$ff#$ff + char(kind) + PtrToStr (p, DataSize (data));
  FreeMem (p, DataSize (data));
end;
}

constructor TPacket.Create (from :TPacket);
var
  s :String;
begin
  s := from.RawData;
  FData := #$3#00#00 + s;
  inherited Create;
end;
{
//Vill ha $-kodad data, ett helt paket bärjadens med ÿÿÿÿ alltså
constructor TPacket.CreateNew (data :string);
var
  p :pointer;
begin
  p := StrToData (data);
  FData := #$3#00#00 + PtrToStr (p, DataSize (data));
  FreeMem (p, DataSize (data));
end;
}
constructor TPacket.SJCreateNew (data :string);
begin
  FData := #$3#00#00#$ff#$ff#$ff#$ff + data;
end;
{--------------------------------------------------------------------}

function TPacket.GetTimestamp :longword;
begin
Result := Plongword(@FData[4])^;
end;

procedure TPacket.SetTimestamp (l :longword);
begin
Move (l, Fdata[4], sizeof(l));
end;

function TPacket.GetKind :byte;
begin
Result := Byte(FData[8]);
end;

procedure TPacket.SetKind (b :byte);
begin
FData[8] := char (b);
end;
{
function TPacket.GetAsciiData :string;
begin
Result := DataToStr (@FData[4], Length (FData) - 3);
end;
}
function TPacket.GetRawData :String;
begin
Result := Copy (FData, 4, Length(fData)-3);
end;

function TPacket.GetRawData2 :String;
begin
Result := Copy (FData, 8, Length(fData)-7);
end;

function TPacket.GetSize :integer;
begin
Result := Length (FData) - 3;
end;

function TPacket.GetTAData :string;
begin
Result := Encrypt( Compress( FData ) );
end;



{--------------------------------------------------------------------}

class function TPacket.KnownPacketType( const s : String; index :integer) : Boolean;
begin
Result := PacketLength( s, index ) <> 0;
end; {KnownPacketType}

//Returnerar längden av en paketsort. returnerar 0 om den är okänd
class function TPacket.PacketLength(const s :string; index :integer) :integer;
begin
case byte(s[index]) of
  $02       :Result := 13;
  $06       :Result := 1;
  $07       :Result := 1;
  $1a       :Result := 14;
  $17       :Result := 2;
  $18       :Result := 2;
  $15       :Result := 1;
  $08       :Result := 1;
  $05       :
            begin
            Result := 65; // text message, 64 bytes of actual message
            if s[index+Result-1] <> #0 then
              begin
              // older recorder versions sometimes emit more text than they should
              // however, it is send as a single packet.
              result := length(s) - index +1;
              // And if map position is enabled, the last 5 bytes should be the map
              // pos data
              if s[length(s) - 5+1] = #$fc then
                dec(result, 5 );
              end;
            end;
  // making teams in battle room
  $20       :Result := 192;
  $24       :Result := 6;
  $26       :Result := 41; // byte('&')

  $2e       :Result := 9;
  $22       :Result := 6;  // byte('"')
  $2a       :Result := 2;  // byte('*') multiplayer finished loading - players synchronization complete
  $1e       :Result := 2;
  $2c       :Result := pword(@s[index+1])^; // byte(',')

    //Nya paket

  $09       :result := 23;        //Verkar vara paketet som ger order om att nåt nybyggt ska visas direkt. Visar dock för fel person..
  $11       :result := 4;         // unit short mask state
  $10       :result := 22;        //Ger explosioner! Dock visas de på fel ställe
  $12       :result := 5;         //?? krasch
  $0a       :result := 7;         //?? krasch
  $28       :result := 58;        //?? ingen skillnad
  $19       :result := 3;         // Speed/pause change
//  $0d       :if iniSettings.ModId > 1 then Result:= 40 else Result := 36;        //Skott. dock stannar skotten kvar. och de missar..
  $0d       :Result := 36;        //Skott. dock stannar skotten kvar. och de missar..

  $0b       :result := 9;         //Eliminerar skottrester
//  $0f       :if iniSettings.ModId > 1 then Result:= 8 else Result := 6;         //Får commanderns överkropp att vridas rätt när han bygger bl.a
  $0f       :Result := 6;         //Får commanderns överkropp att vridas rätt när han bygger bl.a
  $0c       :result := 11;        //hmm. verkar ge explosioner med

  $1f       :result := 5;
  $23       :result := 14;

  $16       :result := 17;  // resource share packet
  $1b       :result := 6;
  $29       :result := 3;
  $14       :result := 24;

  $21       :result := 10;
  $03       :result := 7;
//  $0e       :if iniSettings.ModId > 1 then Result:= 17 else result := 14;
  $0e       :result := 14;

  $f6       :result := 1;
  $f9       :result := 73;          //enemy-chat
  $fa       :result := 1;
  $fb       :result := Integer(s[index+1])+3; //recorder data connect
  $fc       :Result := 5;  //map position
  $fd       :Result := pword(@s[index+1])^-4; //smartpak
  $fe       :result := 5;           //smartpak
  $ff       :result := 1;         //smartpak paket ska inte finnas i vilt tillstånd
  else       result := 0;
end;
end;

//Vill ha in ett rått paket.. <kompress>checkcheckffff<kind><data>
//Returnerar nästa paket, och tar bort det ur S. Returnerar "" när s är tom.. duh.
class function TPacket.Split (var s :string) :string;
var
  len :byte;
  tmp, ut :string;
begin
if s = '' then
  begin
  Result := '';
  exit;
  end;

tmp := Decompress( Decrypt( s ) );

//Nu är tmp på rå form.
len := PacketLength(tmp, 8);

ut := #3#0#0#$ff#$ff#$ff#$ff + Copy (tmp, len + 3 + 4 + 1, 10000);
  
if len = 0 then
  begin
    TLog.Add(1,'Unknown packet : ' + intToStr (byte(tmp[8])));
    ut := #3#0#0#$ff#$ff#$ff#$ff;
//    tmp := '<unknown packet : ' + tmp[8] + '>';
  end
else
    tmp := Copy (tmp, 1, len + 3 + 4);    //<compress>checkcheckffff

Result := Encrypt( Compress( tmp ) );

if Length(ut) = 7 then
  begin
    s := '';
    exit;
  end;
//&’`å$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00
//42 bytes??
  s := Encrypt( Compress( ut ) );
end;

class function TPacket.Split2 (var s :string;smartpak:boolean; const FromPlayer, ToPlayer : string) :string;
var
  len :integer;
  tmp :string;
begin
if s = '' then
  begin
  Result := '';
  exit;
  end;

len := PacketLength(s,1);
if (((s[1]=#$ff) or (s[1]=#$fe) or (s[1]=#$fd)) and not smartpak) then
  begin
  Tlog.add(1,'From '+FromPlayer+' to '+ ToPlayer+',Warning erroneous compression assumption');
  Tlog.add(1,'Packet:'+datatostr2(s));
  end;
if length(s) < len then
  begin
  Tlog.add(1,'From '+FromPlayer+' to '+ ToPlayer+', Error subpacket longer than packet '+inttostr(length(s))+' '+inttostr(len)+' '+datatostr2(s));
  len:=0;
  end;
if len = 0 then
  begin
  TLog.Add (1,'From '+FromPlayer+' to '+ ToPlayer+', unknown packet : $' + IntToHex( byte(s[1]), 2) + ' '+datatostr2(s));
  Result := s;
  s := '';
  end
else
  begin
  tmp := s;
  s := Copy( tmp, len+1 , Length(s) );
  result := Copy (tmp, 1, len);
  end;
end; {Split2}


{--------------------------------------------------------------------}

{$IFOPT R+} {$DEFINE tmp53262} {$R-} {$ENDIF}
{$IFOPT Q+} {$DEFINE tmp3654273} {$Q-}{$ENDIF}
class function TPacket.Decrypt (Data :string) :string;
var
  i :integer;
  check :word;
  p :^word;
begin
if length(data)<4 then
  begin
  result:=data+#$06;
  exit;
  end;
check := 0;
for i := 4 to Length (Data) - 3 do
  begin
  Check := Check + Byte(Data[i]);
  data[i] := Char(Byte(Data [i]) xor byte(i - 1));
  end;
p := @Data[2];
if Check <> p^ then
  begin
  TLog.Add(0,'Removing corrupted packet ');
  Result := #3#0#0#$ff#$ff#$ff#$ff#$2a'd'
  end
else
  Result := Data;
end; {Decrypt}
{$IFDEF tmp53262} {$UNDEF tmp53262} {$R+} {$ENDIF}
{$IFDEF tmp3654273} {$UNDEF tmp3654273} {$Q+}{$ENDIF}

{$IFOPT R+} {$DEFINE tmp53262} {$R-} {$ENDIF}
{$IFOPT Q+} {$DEFINE tmp3654273} {$Q-}{$ENDIF}
//Tar ett helt paket med kontrollkod och checksum i alltså (skriver över checksummen förstås)
class function TPacket.Encrypt( Data : string ) : string;
var
  i :integer;
  check :word;
  p :^Word;
begin
if Length (data) < 4 then
  begin
  TLog.Add(0,'Removing corrupted packet ');
  Result := #3#0#0#$ff#$ff#$ff#$ff#$2a'd';
  exit;
  end;
check := 0;
for i := 4 to Length (Data) - 3 do
  begin
  Data [i] := Char(Byte(Data[i]) xor (i - 1));
  Check := Check + Byte(Data[i]);
  end;
p := @data[2];
p^ := check;
//data[1] := #3;
Result := Data;
end;
{$IFDEF tmp53262} {$UNDEF tmp53262} {$R+} {$ENDIF}
{$IFDEF tmp3654273} {$UNDEF tmp3654273} {$Q+}{$ENDIF}

//Tar ett helt paket med kontrollkod och checksum i alltså
class function TPacket.Decompress(const Data :string) :string;
var
  SourceIndex, ChunkNumber :integer;
  SourceLen : Integer;

  a,uop : Integer;
  RunIndex : Integer;

  Fixup1stByte : Boolean;
  IsCompressedBitMask : byte;
  count : Integer;

  newBufferSize : Integer;

begin {Decompress}
if Data[1] <> #$04 then
  begin
  Result := Data;
  exit;
  end;
SourceLen := Length(Data);
SourceIndex := 4;
//setlength(Result, LastBufferSize );
//Count := 0;
//{
setlength(Result, DecompressionBufferSize );
Result[1] := Data[1];
Result[2] := Data[2];
Result[3] := Data[3];
count := 3;
//}
Fixup1stByte := True;
try
  while SourceIndex <= SourceLen do
    begin
    IsCompressedBitMask := byte(Data[SourceIndex]);
    Inc( SourceIndex );
    for ChunkNumber := 0 to 7 do
      begin
      if SourceIndex > SourceLen then
        begin
        Fixup1stByte := False;
        exit;
        end;
      if (( IsCompressedBitMask shr ChunkNumber) and 1) = 0 then
        begin
        inc( Count );
        if Count > DecompressionBufferSize then
          begin
          newBufferSize := 2*(DecompressionBufferSize+1);
          Tlog.Add( 3, 'TPacket.Decompress, increasing decompression buffer size from ' +IntToStr(DecompressionBufferSize) +' to '+IntToStr(newBufferSize) );
          DecompressionBufferSize := newBufferSize;
          setlength(Result, DecompressionBufferSize );
          end;
        Result[Count] := Data[SourceIndex];
        Inc( SourceIndex );
        end
      else
        begin
        uop := PWord(@Data[SourceIndex])^;
        Inc( SourceIndex, 2 );
        a := uop shr 4;
      	if a = 0 then
          exit;
        uop := uop and $0f;
        for RunIndex := a to a + uop + 1 do
          begin
          inc( Count );
          if Count > DecompressionBufferSize then
            begin
            newBufferSize := 2*(DecompressionBufferSize+1);
            Tlog.Add( 3, 'TPacket.Decompress, increasing decompression buffer size from ' +IntToStr(DecompressionBufferSize) +' to '+IntToStr(newBufferSize) );
            DecompressionBufferSize := newBufferSize;
            setlength(Result, DecompressionBufferSize );
            end;
          Result[Count] := Result[RunIndex+3];
          end;
        end;
      end;
    end;
finally
  SetLength( Result, count );
//  Result := Copy( Data, 1, 3) + Result;
  if Fixup1stByte then
    Result[1] := #3;
end;
end; {Decompress}

// Takes a packet with controlcodes and checksum (will overwrite controlcode)
//Tar ett helt paket med kontrollkod och checksum i alltså (skriver över kontrollkoden)
class function TPacket.Compress (const Data :String) :string;
var
  index,cbf,count,a,matchl,cmatchl       : integer;
  kommando,match                         : word;
  p                                      : ^word;
begin
  if not UseCompression then 
    begin
    Result := Data;
    Result[1] := #3;
    Exit;
    end;
  result:='';
  count:=7;
  index:=4;
  while index<length(data)+1 do  //upprepa över hela data intervallet
  begin
    if count=7 then         //slut på bf
    begin
      count:=-1;
      result:=result+#$0;
      cbf:=length(result);
    end;
    count:=count+1;
    if (index<6) or (index>2000) then          //försök inte komprimera 2 första bytsen
    begin
      result:=result+data[index];
      index:=index+1;
    end else
    begin
      matchl:=2;
      for a:=4 to index-2 do   //leta tidigare sekvenser
      begin
        cmatchl:=0;
        while (data[a+cmatchl]=data[index+cmatchl]) and ((index+cmatchl)<length(data)) and (a+cmatchl<index) do
          cmatchl:=cmatchl+1;
        if (cmatchl>matchl) then    //sekvens värd att spara?
        begin
          matchl:=cmatchl;
          match:=a;
          if matchl>17 then
            break;
        end;
      end;
      cmatchl:=0;
      while (data[index+cmatchl]=data[index-1]) and (index+cmatchl < length(data)) do //upprepning av samma tecken?
        cmatchl:=cmatchl+1;
      if (cmatchl>matchl) then    //sekvens värd att spara?
      begin
        matchl:=cmatchl;
        match:=index-1;
      end;
      if matchl>2 then  //fann vi någon tillräckligt lång sekvens?
      begin
        byte(result[cbf]):=byte(result[cbf]) or (1 shl count);
        matchl:=(matchl - 2) and $0f;
        kommando:=((match-3) shl 4) or matchl;
        result:=result+#0#0;
        p:=@result[length(result)-1];
        p^:=kommando;
        index:=index+matchl+2;
      end else
      begin                       //ingen sekvens värd att använda
        result:=result+data[index];
        index:=index+1;
      end;
    end;
  end;
  if count=7 then
    result:=result+#$ff
  else
    result[cbf] := char( byte(result[cbf]) or ($ff shl (count+1)) );
  result:=result+#0#0;

  if (length(result)+3 < length(data)) then
    result:=#$04+data[2]+data[3]+result
  else begin
    result:=data;
    result[1]:=#$03;
  end;
end;


{
//Tar ett helt paket med kontrollkod och checksum i alltså (skriver över kontrollkoden)
class function TPacket.Compress( Data : string ) : string;
begin
Data[1] := #$3;       //Uncompressed. ;)
Result := Data;
end;
}

end.