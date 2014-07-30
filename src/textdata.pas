unit textdata;
interface

uses
// activex,
  classes;
Procedure SetShortStr( var CharArray; len : integer; str : string);

function HexToStr (num :Cardinal; digits :integer) :string;
function DataToStr (p :pointer; size :integer) :String;
//function DataSize (const s :string) :integer;
function StrToData (const s :string; var aDataSize : integer) :pointer;
//procedure StrData (const source :string; var dest);
function PtrToStr (p :pointer; len :integer) :string;
function IsSameGuid (g1, t1 :TGuid) :boolean;
function StrToBin (const s :String) :string;
function BinToInt(const s :String; Startindex : integer; StartBit, BitCount :integer) :Integer;
function FixPath (const s :string) :string;
function SimpleCrypt (const s :string) :string;
function CalcCRC (curcrc :longword; data :pointer; len :longword) :longword;
function RemoveInvalid (const st :string) :string;
function RemoveInvalidIncSpace (const st :string) :string;
function LeftPad(S: string; Ch: Char; Len: Integer): string;

function DataToHex( const s : string ) : string; overload;
function DataToHex( const s : string; index : integer; len : integer ) : string; overload;
  function DataToStr2( const s : string) : string;


procedure ParseParams( ParamData : string; params : TStrings );

function IsStrBool( const s : string; var Value : Boolean ) : Boolean;
function IsStrInt( const s : string; var Value : integer ) : Boolean;
function IsStrByte( const s : string; var Value : byte ) : Boolean;

implementation
uses
  sysutils,
  logging;

Procedure SetShortStr( var CharArray; len : integer; str : string);
var
  alen : integer;
begin
if length(str) > len then
  begin
  TLog.Add(1,'Error when copying long sting to fixed size. Max size; '+IntToStr(len)+' but tried to add '+IntTostr(length(str)) );
  TLog.Add(1,'Error caused by the string: '+str);
  alen := len;
  end
else
  alen := length(str);
move( str[1], CharArray, alen );
end;

function DataToStr2( const s : string) : string;
var
  i : Integer;
begin
Result := '';
for i := 1 to Length(s) do
  Result := Result+'#$'+IntToHex(ord(s[i]),2);
end; {DataToStr2}

function HexToStr (num :Cardinal; digits :integer) :String;
var
  i :integer;
begin
SetLength( Result, digits );
for i := 1 to digits do
  begin
  case num mod 16 of
    0..9   : Result[i] := char( (num mod 16) + Byte('0') );
    10..15 : Result[i] := char( (num mod 16) + Byte('A') - 10 );
  end;
  num := num div 16;
  end;

if num > 0 then
  Result := Result + '(overflow)';
end;


function IsStrByte( const s : string; var Value : byte ) : Boolean;
begin
Value := StrToIntDef(s,$ff);
if Value = $ff then
  try
    Result := StrToInt(s) = Value;
  except
    On e : EConvertError do
      Result := False;
  end
else
  Result := True;
end;

function IsStrInt( const s : string; var Value : integer ) : Boolean;
begin
Value := StrToIntDef(s,-MaxInt);
if Value = -MaxInt then
  try
    Result := StrToInt(s) = Value;
  except
    On e : EConvertError do
      Result := False;
  end
else
  Result := True;
end;


function IsStrBool( const s : string; var Value : Boolean ) : Boolean;
begin
if (s = 'true') or (StrToIntDef(s,0) = 1) then
  begin
  Result := true;
  Value := True;
  end
else if (s = 'false') or (StrToIntDef(s,1) = 0) then
  begin
  Result := true;
  Value := false;
  end
else
  Result := false;
end;

function DataToHex( const s : string ) : string;
var i : Integer;
begin
Result := '';
for i := 1 to Length(s) do
  Result := Result+IntToHex( Ord(s[i]), 2) +' ';
end;

function DataToHex( const s : string; index : integer; len : integer ) : string; overload;
var i : Integer;
begin
Result := '';
for i := index to index+len do
  Result := Result+IntToHex( Ord(s[i]), 2) +' ';
end;

const StringChar = '"';

procedure ParseParams( ParamData : string; params : TStrings );

var
  SourceIndex : Integer;
  look : Char;

  Procedure GetChar;
  begin
  if SourceIndex <= Length(ParamData) then
    begin
    look := ParamData[SourceIndex];
    Inc(SourceIndex);
    end
  else
    look := #0;
  end; {GetChar}

var
  param : string;
  stringlen : Integer;
  i : Integer;
begin
// parse the params
while ParamData <> '' do
  begin
  ParamData := TrimLeft(ParamData);
  if (ParamData = '') then Break;
  if ParamData[1] = '"' then
    begin
    SetLength(param,16);
    SourceIndex := 2;
    stringlen := 0;
    GetChar;
    Repeat
      // check to see if this is the end of the string or just imbedding a " into the string
      // or it is the end of the string
      if (look = StringChar) then
        begin
        GetChar;
        if (look <> StringChar) then
          break;
        GetChar;
        param[ stringlen ] := StringChar;
        inc( stringlen );
        if stringlen > length(param) then
          setlength(param, stringlen*2);
        end;
      inc( stringlen );
      if stringlen > length(param) then
        setlength(param, stringlen*2);
      param[ stringlen ] := Look;
      GetChar;
    until (look = #0);
    setLength( param, stringlen);
    Delete( ParamData, 1, SourceIndex );
    end
  else
    begin
    i := pos(' ',ParamData)-1;
    if I = -1 then
      begin
      param := ParamData;
      ParamData := '';
      end
    else
      begin
      param := copy(ParamData,1,i);
      Delete( ParamData, 1, i+1 );
      end;
    end;
  params.Add(param);
  end;
end; {ParseParams}

{$R-}
{$Q-}
Const
  CRC32tab : Array[0..255] of LongWord = (
    $00000000, $77073096, $ee0e612c, $990951ba, $076dc419, $706af48f,
    $e963a535, $9e6495a3, $0edb8832, $79dcb8a4, $e0d5e91e, $97d2d988,
    $09b64c2b, $7eb17cbd, $e7b82d07, $90bf1d91, $1db71064, $6ab020f2,
    $f3b97148, $84be41de, $1adad47d, $6ddde4eb, $f4d4b551, $83d385c7,
    $136c9856, $646ba8c0, $fd62f97a, $8a65c9ec, $14015c4f, $63066cd9,
    $fa0f3d63, $8d080df5, $3b6e20c8, $4c69105e, $d56041e4, $a2677172,
    $3c03e4d1, $4b04d447, $d20d85fd, $a50ab56b, $35b5a8fa, $42b2986c,
    $dbbbc9d6, $acbcf940, $32d86ce3, $45df5c75, $dcd60dcf, $abd13d59,
    $26d930ac, $51de003a, $c8d75180, $bfd06116, $21b4f4b5, $56b3c423,
    $cfba9599, $b8bda50f, $2802b89e, $5f058808, $c60cd9b2, $b10be924,
    $2f6f7c87, $58684c11, $c1611dab, $b6662d3d, $76dc4190, $01db7106,
    $98d220bc, $efd5102a, $71b18589, $06b6b51f, $9fbfe4a5, $e8b8d433,
    $7807c9a2, $0f00f934, $9609a88e, $e10e9818, $7f6a0dbb, $086d3d2d,
    $91646c97, $e6635c01, $6b6b51f4, $1c6c6162, $856530d8, $f262004e,
    $6c0695ed, $1b01a57b, $8208f4c1, $f50fc457, $65b0d9c6, $12b7e950,
    $8bbeb8ea, $fcb9887c, $62dd1ddf, $15da2d49, $8cd37cf3, $fbd44c65,
    $4db26158, $3ab551ce, $a3bc0074, $d4bb30e2, $4adfa541, $3dd895d7,
    $a4d1c46d, $d3d6f4fb, $4369e96a, $346ed9fc, $ad678846, $da60b8d0,
    $44042d73, $33031de5, $aa0a4c5f, $dd0d7cc9, $5005713c, $270241aa,
    $be0b1010, $c90c2086, $5768b525, $206f85b3, $b966d409, $ce61e49f,
    $5edef90e, $29d9c998, $b0d09822, $c7d7a8b4, $59b33d17, $2eb40d81,
    $b7bd5c3b, $c0ba6cad, $edb88320, $9abfb3b6, $03b6e20c, $74b1d29a,
    $ead54739, $9dd277af, $04db2615, $73dc1683, $e3630b12, $94643b84,
    $0d6d6a3e, $7a6a5aa8, $e40ecf0b, $9309ff9d, $0a00ae27, $7d079eb1,
    $f00f9344, $8708a3d2, $1e01f268, $6906c2fe, $f762575d, $806567cb,
    $196c3671, $6e6b06e7, $fed41b76, $89d32be0, $10da7a5a, $67dd4acc,
    $f9b9df6f, $8ebeeff9, $17b7be43, $60b08ed5, $d6d6a3e8, $a1d1937e,
    $38d8c2c4, $4fdff252, $d1bb67f1, $a6bc5767, $3fb506dd, $48b2364b,
    $d80d2bda, $af0a1b4c, $36034af6, $41047a60, $df60efc3, $a867df55,
    $316e8eef, $4669be79, $cb61b38c, $bc66831a, $256fd2a0, $5268e236,
    $cc0c7795, $bb0b4703, $220216b9, $5505262f, $c5ba3bbe, $b2bd0b28,
    $2bb45a92, $5cb36a04, $c2d7ffa7, $b5d0cf31, $2cd99e8b, $5bdeae1d,
    $9b64c2b0, $ec63f226, $756aa39c, $026d930a, $9c0906a9, $eb0e363f,
    $72076785, $05005713, $95bf4a82, $e2b87a14, $7bb12bae, $0cb61b38,
    $92d28e9b, $e5d5be0d, $7cdcefb7, $0bdbdf21, $86d3d2d4, $f1d4e242,
    $68ddb3f8, $1fda836e, $81be16cd, $f6b9265b, $6fb077e1, $18b74777,
    $88085ae6, $ff0f6a70, $66063bca, $11010b5c, $8f659eff, $f862ae69,
    $616bffd3, $166ccf45, $a00ae278, $d70dd2ee, $4e048354, $3903b3c2,
    $a7672661, $d06016f7, $4969474d, $3e6e77db, $aed16a4a, $d9d65adc,
    $40df0b66, $37d83bf0, $a9bcae53, $debb9ec5, $47b2cf7f, $30b5ffe9,
    $bdbdf21c, $cabac28a, $53b39330, $24b4a3a6, $bad03605, $cdd70693,
    $54de5729, $23d967bf, $b3667a2e, $c4614ab8, $5d681b02, $2a6f2b94,
    $b40bbe37, $c30c8ea1, $5a05df1b, $2d02ef8d  );

function IsSameGuid (g1, t1 :TGuid) :boolean;
var
  i :integer;
begin
Result := false;
if g1.d1 <> t1.d1 then exit;
if g1.d2 <> t1.d2 then exit;
if g1.d3 <> t1.d3 then exit;
for i := 0 to 7 do
  if g1.d4[i] <> t1.d4[i] then exit;
Result := true;
end;

function DataToStr (p :pointer; size :integer) :string;
type
  pdata = ^tdata;
  tdata = array[1..100000] of byte;
var
  i :integer;
  s :string;
begin
  s := '';
  try
    for i := 1 to size do
    begin
      if (char(pdata(p)[i])<'A') or (char(pdata(p)[i])>'z') then
      s := s + '$' + IntToHex (pdata(p)^[i], 2)
      else s:= s + char(pdata(p)[i]);
{      case pdata(p)^[i] of
        0..255                            : s := s + '$' + HexToStr (pdata (p)^[i], 2);
{        0..35, 37..63, 128..255           : s := s +'$' + HexToStr (pdata(p)^[i], 2);
        36                                : s := s + '$$';
        64..127                           : s := s + char (pdata(p)^[i]);}
//        32..byte('$')-1, byte('$')+1..255 : s := s + char (pdata(p)^[i]);
{      end;}
    end;
  except
    s := IntToHex(Cardinal (p), 8) + ' <illegal to read this>';
  end;

  Result := s;
end;

{Returnerar hur många bytes det är}
function DataSize (const s :string) :integer;
var
  i :integer;
  state :integer;
begin
Result := 0;
i := 1;
state := 0;

while i <= Length (s) do
  begin
  case s[i] of
    '$' :
      if (state = 1) then
        begin
        Inc (Result);
        State := 0;
        end
      else
        State := 1;
    '0'..'9',
    'A'..'F'  :
      case state of
        0 : Inc (Result);
        1 : Inc (state);
        2 :
          begin
          Inc (Result);
          State := 0;
          end;
      end;
  else
    Inc (Result);
  end;
  Inc (i);
  end;
end;

{Allokerar minne åt den}
function StrToData (const s :string; var aDataSize : integer) :pointer;
type P_char = ^char;
var
  p :P_char;
  sourceIndex :integer;
  state :integer;
  b :byte;
  x :integer;
begin
  aDataSize := DataSize(s);
  GetMem (p, aDataSize);
  try
  Result := p;

  sourceIndex := 1;
  state := 0;

  while sourceIndex <= Length (s) do
  begin
    case s[sourceIndex] of
      '$' : if (state = 1) then
            begin
              p^ := '$';
              Inc( p );
              State := 0;
            end else
              State := 1;
      '0'..'9',
      'A'..'F' :case state of
                  0 : begin
                        p^ := s[sourceIndex];
                        Inc( p );
                      end;
                  1 : Inc (state);
                  2 : begin
                        Val (Copy (s, sourceIndex - 2, 3), b, x);
                        p^ := char (b);
                        Inc( p );
                        State := 0;
                      end;
                end;
    else
      begin
        p^ := s[sourceIndex];
        Inc( p );
      end;
    end;
    Inc(sourceIndex);
  end;
  except
    FreeMem( p );
    raise;
  end;
end;

procedure StrData (const source :string; var dest);
var
  p :pointer;
  size : integer;
begin
p := StrToData (source, size);
try
  Move (p^, dest, DataSize (source));
finally
  FreeMem(p);
end;
end;

function PtrToStr (p :pointer; len :integer) :string;
begin
SetLength( Result, len );
move( p^, Result[1], len );
end;

function StrToBin (const s :String) :string;
var
  b :byte;
  i, j :integer;
  res :string;
begin
  res := '';
  for i := 1 to length (s) do
  begin
    b := byte(s[i]);

    for j := 1 to 8 do
    begin
      if (b and $80) = 0 then
        res := res + '0'
      else
        res := res + '1';

      b := b shl 1;
    end;
    res := res + ' ';
  end;

  Result := res;
end;

// s är förväntad att vara en bitenkodad 2c-sträng typ
// 0-baserat så start = 0 börjar med första biten.. whee
function BinToInt(const s :String; Startindex : integer; StartBit, BitCount :integer) :integer;
var
  i : integer;
  BitMask : byte;
  value : byte;
  BitIndex : integer;
begin
i := 0;
Inc( i, Startindex );
while StartBit > 7 do
  begin
  inc( i );
  dec( StartBit, 8 );
  end;

result := 0;
BitMask := 1 shl StartBit;
value := byte( s[i] );

for BitIndex := 1 to BitCount do
  begin
  if (value and BitMask) <> 0 then
    result := result or (1 shl (BitIndex - 1));
  inc( StartBit );
  BitMask := BitMask shl 1;
  if StartBit > 7 then
    begin
    inc( i );
    value := byte(s[i]);
    StartBit := 0;
    BitMask := 1;
    end;
  end;
end; {BinToInt}

//Ser till att en sökväg slutar med \ (om den är tom returneras dock tomt)
function FixPath (const s :string) :string;
begin
  Result := s;
  Result := trim (Result);
  if Result = '' then
    exit;

  If Result[length(Result)] <> '\' then
    Result := Result + '\';
end;

//xor'ar med 42 bara..
function SimpleCrypt (const s :string) :string;
var
  i   :integer;
begin
  Result := s;
  for i := 1 to length (Result) do
    byte(Result[i]) := byte(Result[i]) xor 42;
end;

function CRC32(value: Byte; crc: LongWord) : LongWord;
begin
  CRC32 := CRC32tab[Byte(crc xor LongWord(value))] xor
           ((crc shr 8) and $00ffffff);
end;

function CalcCRC (curcrc :longword; data :pointer; len :longword) :longword;
var
  i :integer;
begin
Result := curcrc;
for i := 1 to len do
  Result := crc32( PByteArray(data)^[i], Result );
end;

function RemoveInvalid (const st :string) :string;
var
  i :integer;
begin
Result := st;
for i := 1 to Length (Result) do
  begin
  if not (Result[i] in ['A'..'Z', 'a'..'z', '.', '-', '+', '|', '_', '0'..'9', '@',
                      '!', '#', '"', '%', '(', ')', '=', '[', ']', ',', ';',
                      ' ', '''']) then
    Result[i] := '_';
  end;
end;

function RemoveInvalidIncSpace(const st :string) :string;
var
  i :integer;
begin
Result := st;
for i := 1 to Length (Result) do
  begin
  if not (Result[i] in ['A'..'Z', 'a'..'z', '-', '+', '|', '_', '0'..'9', '@',
                      '!', '"', '(', ')',
                      '''']) then
    Result[i] := '_';
  end;
end;

function LeftPad(S: string; Ch: Char; Len: Integer): string;
var
  RestLen: Integer;
begin
  Result  := S;
  RestLen := Len - Length(s);
  if RestLen < 1 then Exit;
  Result := S + StringOfChar(Ch, RestLen);
end;

end.

