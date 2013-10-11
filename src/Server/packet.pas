unit packet;

interface

const
  SY_UNIT = $92549357;

type
  TPacket = class
  protected
    procedure SetSerial (l :longword);
    function GetKind :byte;
    procedure SetKind (b :byte);
    function GetAsciiData :string;
    function GetRawData :String;
    function GetRawData2 :String;
    function GetSize :integer;
    function GetTAData :string;
  public
    FData :string;
    constructor Create (data :pointer; size :integer); overload;
    constructor Create (data :String); overload; //För $-kodad data
    constructor Create (kind :byte; data :pointer; size :integer); overload; //När man vill göra egna paket
    constructor Create (from :TPacket); overload;
    constructor Create (kind :byte; data :string); overload;
    constructor CreateNew (data :string); overload;
    constructor SJCreateNew (data :string); overload;

    class function Split (var s :string) :string;
    class function Split2 (var s :string;smartpak:boolean) :string;
    class function Decrypt (Data :string) :string;
    class function Encrypt (Data :string) :string;
    class function Decompress (Data :string) :string;
    class function Compress (Data :String) :string;
    class function PacketLength (s :String; index :integer) :integer;
    function GetSerial :Longword;

    property Serial :longword read GetSerial write SetSerial;
    property Kind :byte read GetKind write SetKind;
    property RawData :string read GetRawData;
    property RawData2 :string read GetRawData2;
    property AsciiData :string read GetAsciiData;
    property Size :integer read GetSize;
    property TAData :string read GetTAData;       //Krypterar och komprimerar
  end;

implementation

uses
  TextData, Logging, sysutils;

{--------------------------------------------------------------------}

constructor TPacket.Create (data :pointer; size :integer);
begin
  inherited Create;
  FData := PtrToStr (data, size);
  FData := Decrypt (FData);

  if FData[1] = #$04 then
  begin
    FData := Decompress (FData);
  end;
end;

constructor TPacket.Create (data :String);
var
  p :pointer;
begin
  inherited Create;
  p := StrToData (data);
  FData := PtrToStr (p, DataSize (data));
  FreeMem (p, DataSize (data));
  FData := Decrypt (FData);

  if FData[1] = #$04 then
  begin
    FData := Decompress (FData);
  end;
end;

//Dessa är för egna paket
constructor TPacket.Create (kind :byte; data :pointer; size :integer);
begin
  inherited Create;
  FData := #$3#00#00#$ff#$ff#$ff#$ff + Char(kind) + PtrToStr (data, size);
end;

constructor TPacket.Create (kind :byte; data :string);
var
  p :pointer;
begin
  p := StrToData (data);
  FData := #$3#00#00#$ff#$ff#$ff#$ff + char(kind) + PtrToStr (p, DataSize (data));
  FreeMem (p, DataSize (data));
end;

constructor TPacket.Create (from :TPacket);
var
  s :String;
begin
  s := from.RawData;
  FData := #$3#00#00 + s;
  inherited Create;
end;

//Vill ha $-kodad data, ett helt paket bärjadens med ÿÿÿÿ alltså
constructor TPacket.CreateNew (data :string);
var
  p :pointer;
begin
  p := StrToData (data);
  FData := #$3#00#00 + PtrToStr (p, DataSize (data));
  FreeMem (p, DataSize (data));
end;

constructor TPacket.SJCreateNew (data :string);
begin
  FData := #$3#00#00#$ff#$ff#$ff#$ff + data;
end;
{--------------------------------------------------------------------}

function TPacket.GetSerial :longword;
var
  p :^longword;
begin
  p := @FData[4];
  Result := p^;
end;

procedure TPacket.SetSerial (l :longword);
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

function TPacket.GetAsciiData :string;
begin
  Result := DataToStr (@FData[4], Length (FData) - 3);
end;

function TPacket.GetRawData :String;
begin
  Result := Copy (FData, 4, 10000);
end;

function TPacket.GetRawData2 :String;
begin
  Result := Copy (FData, 8, 10000);
end;

function TPacket.GetSize :integer;
begin
  Result := Length (FData) - 3;
end;

function TPacket.GetTAData :string;
var
  s :String;
begin
  s := FData;
  s := Compress (s);
  s := Encrypt (s);
  Result := s;
end;

{--------------------------------------------------------------------}

class function TPacket.Decrypt (Data :string) :string;
var
  i :integer;
  check :word;
  p :^word;
begin
  if length(data)<4 then begin
    result:=data+#$06;;
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
    Result := '<error in checksum?!?!>'
  else
    Result := data;
end;

//Tar ett helt paket med kontrollkod och checksum i alltså (skriver över checksummen förstås)
class function TPacket.Encrypt (Data :string) :string;
var
  i :integer;
  check :word;
  p :^word;
begin
  if Length (data) < 4 then
  begin
    Result := '<no data!>';
    exit;
  end;

  check := 0;
  for i := 4 to Length (Data) - 3 do
  begin
    Data [i] := Char(Byte(Data[i]) xor (i - 1));
    Check := Check + byte(data[i]);
  end;
  p := @data[2];
  p^ := check;

  Result := Data;
end;

//Tar ett helt paket med kontrollkod och checksum i alltså
class function TPacket.Decompress (Data :string) :string;
var
  index, nump, uop, a, cbf, b :integer;
  inbuf :string;
begin
  if Data[1] <> #$04 then
  begin
    Result := Data;
    exit;
  end;

  inbuf := Copy (Data, 4, 10000);
  Result := '';
  index := 1;

  while index <= length (inbuf) do
  begin
    cbf := byte (inbuf [index]);
    Inc (index);

    for nump := 0 to 7 do
    begin
      if index>length (inbuf) then
      begin
        Result := Copy (Data, 1, 3) + Result;
        exit;
      end;
      if ((cbf shr nump) and 1) = 0 then
      begin
        Result := Result + inbuf [index];
        Inc (index);
      end else
      begin
        uop := (Byte (inbuf [index+1]) shl 8) + Byte (inbuf [index]);
        Inc (index, 2);
        a := uop shr 4;
      	if a=0 then
        begin
          Result := Copy (Data, 1, 3) + Result;
          Result[1] := #3;
          exit;
        end;
        uop := uop and $0f;
        for b := a to uop + a + 1 do
        begin
          Result := Result + Result [b];  // b + 1??
        end;
      end;
    end;

  end;

  Result := Copy (Data, 1, 3) + Result;
  Result[1] := #3;
end;

//Tar ett helt paket med kontrollkod och checksum i alltså (skriver över kontrollkoden)
class function TPacket.Compress (Data :String) :string;
var
  index,cbf,count,a,matchl,cmatchl       : integer;
  kommando,match                         : word;
  p                                      : ^word;
begin
{  Result := Data;
  Result[1] := #3;
  exit;
}
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
    byte(result[cbf]):=byte(result[cbf]) or ($ff shl (count+1));
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
function TPacket.Compress (Data :String) :string;
begin
  Data[1] := #$3;       //Uncompressed. ;)
  Result := Data;
end;
}

//Returnerar längden av en paketsort. returnerar 0 om den är okänd
class function TPacket.PacketLength (s :string; index :integer) :integer;
type
  pword = ^word;
begin
//  raise Exception.Create ('packetlength is outdated');
{ TODO : packet length check for weapons id patch }
  Result := 0;
  case byte(s[index]) of
    $2        :Result := 13;
    $6        :Result := 1;
    $7        :Result := 1;
    $20       :Result := 192;
    $1a       :Result := 14;
    $17       :Result := 2;
    $18       :Result := 2;
    $15       :Result := 1;
    $8        :Result := 1;
    $5        :Result := 65;
    byte('&') :Result := 41;
    byte('"') :Result := 6;
    byte('*') :Result := 2;
    $1e       :Result := 2;
    byte(',') :Result := pword(@s[index+1])^;

    //Nya paket

    $09       :result := 23;        //Verkar vara paketet som ger order om att nåt nybyggt ska visas direkt. Visar dock för fel person..
    $11       :result := 4;         //?? krasch
    $10       :result := 22;        //Ger explosioner! Dock visas de på fel ställe
    $12       :result := 5;         //?? krasch
    $0a       :result := 7;         //?? krasch
    $28       :result := 58;        //?? ingen skillnad
    $19       :result := 3;         //??
    $0d       :Result := 40;        //Skott. dock stannar skotten kvar. och de missar..
    $0b       :result := 9;         //Eliminerar skottrester
    $0f       :Result := 8;         //Får commanderns överkropp att vridas rätt när han bygger bl.a
    $0c       :result := 11;        //hmm. verkar ge explosioner med

    $1f       :result := 5;
    $23       :result := 14;
    $16       :result := 17;
    $1b       :result := 6;
    $29       :result := 3;
    $14       :result := 24;

    $21       :result := 10;
    $03       :result := 7;
    $0e       :result := 17;

    $ff       :result := 1;           //smartpak paket ska inte finnas i vilt tillstånd
    $fe       :result := 5;
    $f9       :result := 73;          //enemy-chat
    $fa       :result := 1;
    $f6       :result := 1;    
  end;
end;

//Vill ha in ett rått paket.. <kompress>checkcheckffff<kind><data>
//Returnerar nästa paket, och tar bort det ur S. Returnerar "" när s är tom.. duh.
class function TPacket.Split (var s :string) :string;
var
  len :byte;
  tmp, ut :string;
begin
  if Length (s) = 0 then
  begin
    Result := '';
    exit;
  end;

  tmp := s;
  tmp := Decrypt (s);
  if tmp[1] = #4 then
    tmp := Decompress (tmp);


  //Nu är tmp på rå form.

  len := PacketLength (tmp, 8);

//  len := 0;

  ut := #3#0#0#$ff#$ff#$ff#$ff + Copy (tmp, len + 3 + 4 + 1, 10000);

  if len = 0 then
  begin
    Log.Add ('Okänt paket : ' + intToStr (byte(tmp[8])));
    ut := #3#0#0#$ff#$ff#$ff#$ff;
//    tmp := '<Okänt paket : ' + tmp[8] + '>';
  end else
    tmp := Copy (tmp, 1, len + 3 + 4);    //<compress>checkcheckffff

  tmp := Compress (tmp);
  tmp := Encrypt (tmp);
  Result := tmp;

  if Length (ut) = 7 then
  begin
    s := '';
    exit;
  end;
//&’`å$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00$00
//42 bytes??
  ut := Compress (ut);
  ut := Encrypt (ut);
  s := ut;
end;

class function TPacket.Split2 (var s :string;smartpak:boolean) :string;
var
  len :integer;
  tmp :string;
begin
  if s='' then
  begin
    Result := '';
    exit;
  end;

  tmp := s;

  len := 0;
  case byte(tmp[1]) of
    $2        :len := 13;
    $6        :len := 1;
    $7        :len := 1;
    $20       :len := 192;
    $1a       :len := 14;
    $17       :len := 2;
    $18       :len := 2;
    $15       :len := 1;
    $8        :len := 1;
    $5        :len := 65;
    byte('&') :len := 41;
    byte('"') :len := 6;
    byte('*') :len := 2;
    $1e       :len := 2;
    byte(',') : begin len := integer(byte(tmp[2]))+integer(byte(tmp[3])*256);
                end;
    //Nya paket

    $09       :len := 23;        //Verkar vara paketet som ger order om att nåt nybyggt ska visas direkt. Visar dock för fel person..
    $11       :len := 4;         //?? krasch
    $10       :len := 22;        //Ger explosioner! Dock visas de på fel ställe
    $12       :len := 5;         //?? krasch
    $0a       :len := 7;         //?? krasch
    $28       :len := 58;        //?? ingen skillnad
    $19       :len := 3;         //??
    $0d       :len := 36;        //Skott. dock stannar skotten kvar. och de missar..
    $0b       :len := 9;         //Eliminerar skottrester
    $0f       :len := 6;         //Får commanderns överkropp att vridas rätt när han bygger bl.a
    $0c       :len := 11;        //hmm. verkar ge explosioner med

    $1f       :len := 5;
    $23       :len := 14;
    $16       :len := 17;
    $1b       :len := 6;
    $29       :len := 3;
    $14       :len := 24;
    $21       :len := 10;
    $03       :len := 7;
    $0e       :len := 14;

    $ff       :len := 1;           //smartpak paket ska inte finnas i vilt tillstånd
    $fe       :len := 5;           //smartpak
    $fd       :len:= (integer(byte(tmp[2]))+integer(byte(tmp[3])*256))-4; //smartpak
    $f9       :len := 73;          //enemy-chat
    $fb       :len := integer(tmp[2])+3; //recorder data connect
    $fc       :len := 5;  //map position
    $fa       :len := 1;
    $f6       :len := 1;
  end;
  if (((s[1]=#$ff) or (tmp[1]=#$fe) or (tmp[1]=#$fd)) and not smartpak) then begin
    log.add('Warning erroneous compression assumption');
    log.add('Packet:'+datatostr(@tmp[1],length(tmp)));
  end;
  if length(tmp)<len then begin
    log.add('Error subpacket longer than packet '+inttostr(length(tmp))+' '+inttostr(len)+' '+datatostr(@tmp[1],length(tmp)));
    len:=0;
  end;
  if len = 0 then
  begin
    Log.Add ('Okänt paket : ' + intToStr (byte(tmp[1])) + ' '+datatostr(@tmp[1],length(tmp)));
    s := '';
    result:=tmp;
  end else begin
    s :=Copy (tmp, len+1 , 10000);
    result := Copy (tmp, 1, len);
  end;
end;

{
class function TPacket.Split2 (var s :string;smartpak:boolean) :string;
var
  len :integer;
  tmp :string;
begin
  if s='' then
  begin
    Result := '';
    exit;
  end;

  tmp := s;

  len := 0;
  case byte(tmp[1]) of
    $2        :len := 13;
    $6        :len := 1;
    $7        :len := 1;
    $20       :len := 192;
    $1a       :len := 14;
    $17       :len := 2;
    $18       :len := 2;
    $15       :len := 1;
    $8        :len := 1;
    $5        :len := 65;
    byte('&') :len := 41;
    byte('"') :len := 6;
    byte('*') :len := 2;
    $1e       :len := 2;
    byte(',') : begin len := integer(byte(tmp[2]))+integer(byte(tmp[3])*256);
                end;

    //Nya paket

    $09       :len := 23;        //Verkar vara paketet som ger order om att nåt nybyggt ska visas direkt. Visar dock för fel person..
    $11       :len := 4;         //?? krasch
    $10       :len := 22;        //Ger explosioner! Dock visas de på fel ställe
    $12       :len := 5;         //?? krasch
    $0a       :len := 7;         //?? krasch
    $28       :len := 58;        //?? ingen skillnad
    $19       :len := 3;         //??
    $0d       :len := 36;        //Skott. dock stannar skotten kvar. och de missar..
    $0b       :len := 9;         //Eliminerar skottrester
    $0f       :len := 6;         //Får commanderns överkropp att vridas rätt när han bygger bl.a
    $0c       :len := 11;        //hmm. verkar ge explosioner med

    $1f       :len := 5;
    $23       :len := 14;
    $16       :len := 17;
    $1b       :len := 6;
    $29       :len := 3;
    $14       :len := 24;

    $21       :len := 10;
    $03       :len := 7;
    $0e       :len := 14;

    $ff       :len := 1;           //smartpak paket ska inte finnas i vilt tillstånd
    $fe       :len := 5;

  end;

  if length(tmp)<len then begin
    log.add('Error subpacket longer then packet '+inttostr(length(tmp))+' '+inttostr(len)+' '+datatostr(@tmp[1],length(tmp)));
    len:=0;
  end;
  if len = 0 then
  begin
    Log.Add ('Okänt paket : ' + intToStr (byte(tmp[1])) + ' '+datatostr(@tmp[1],length(tmp)));
    s := '';
    result:=tmp;
  end else begin
    s :=Copy (tmp, len+1 , 10000);
    result := Copy (tmp, 1, len);
  end;
end;}
end.
