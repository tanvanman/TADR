unit textdata;

interface

uses
  activex, sysutils;

function HexToStr (num :Cardinal; digits :integer) :String;
function DataToStr (p :pointer; size :integer) :String;
function DataSize (s :string) :integer;
function StrToData (s :string) :pointer;
procedure StrData (source :string; var dest);
function PtrToStr (p :pointer; len :integer) :string;
function IsSameGuid (g1, t1 :TGuid) :boolean;
function StrToBin (s :String) :string;
function BinToInt (s :String; start, num :integer) :integer;
function SimpleCrypt (s :string) :string;
function CalcCRC (curcrc :longword; data :pointer; len :longword) :longword;
function RemoveInvalid (st :string) :string;

implementation

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


function HexToStr (num :Cardinal; digits :integer) :String;
var
  i :integer;
  ch :byte;
begin
  Result := '';
  for i := 1 to digits do
  begin
    case num mod 16 of
      0..9   : ch := (num mod 16) + Byte('0');
      10..15 : ch := (num mod 16) + Byte('A') - 10;
    end;
    Result := Char(ch) + Result;
    num := num div 16;
  end;

  if num > 0 then
    Result := Result + '(overflow)';
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
      s := s + '$' + HexToStr (pdata(p)^[i], 2)
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
    s := HexToStr (Cardinal (p), 8) + ' <illegal to read this>';
  end;

  Result := s;
end;

{Returnerar hur många bytes det är}
function DataSize (s :string) :integer;
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
      '$' : if (state = 1) then
            begin
              Inc (Result);
              State := 0;
            end else
              State := 1;
      '0'..'9',
      'A'..'F'  :case state of
                  0 : Inc (Result);
                  1 : Inc (state);
                  2 : begin Inc (Result); State := 0; end;
                end;
    else
      Inc (Result);
    end;
    Inc (i);
  end;
end;

{Allokerar minne åt den}
function StrToData (s :string) :pointer;
type
  PData = ^TData;
  TData = array[1..2000] of char;
var
  p :PData;
  i :integer;
  state :integer;
  index :integer;
  b :byte;
  x :integer;
begin
  GetMem (p, DataSize (s));
  Result := p;

  i := 1;
  state := 0;
  index := 1;

  while i <= Length (s) do
  begin
    case s[i] of
      '$' : if (state = 1) then
            begin
              p^[index] := '$';
              Inc (index);
              State := 0;
            end else
              State := 1;
      '0'..'9',
      'A'..'F' :case state of
                  0 : begin
                        p^[index] := s[i];
                        Inc (index);
                      end;
                  1 : Inc (state);
                  2 : begin
                        Val (Copy (s, i - 2, 3), b, x);
                        p^[index] := char (b);
                        Inc (index);
                        State := 0;
                      end;
                end;
    else
      begin
        p^[index] := s[i];
        Inc (Index);
      end;
    end;
    Inc (i);
  end;
end;

procedure StrData (source :string; var dest);
var
  p :pointer;
begin
  p := StrToData (source);
  Move (p^, dest, DataSize (source));
  FreeMem (p, DataSize (source));
end;

function PtrToStr (p :pointer; len :integer) :string;
type
  pdata = ^tdata;
  tdata = array[1..10000] of char;
var
  i :integer;
begin
{  SetLength (Result, len);
  Move (p^, Result[1], len);}     //hmm funka inte.. bah
  Result := '';
  for i := 1 to len do
  begin
    Result := Result + pdata(p)^[i];
  end;
end;

function StrToBin (s :String) :string;
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
function BinToInt (s :String; start, num :integer) :integer;
var
  i :integer;
  mask :byte;
  res :integer;
  b :byte;
  j :integer;
begin
  i := 1;
  while start > 7 do
  begin
    inc (i);
    dec (start, 8);
  end;

  res := 0;
  mask := 1 shl start;
  b := byte(s[i]);

  for j := 1 to num do
  begin
    if (b and mask) <> 0 then
    begin
      res := res or (1 shl (j - 1));
    end;

    inc (start);
    mask := mask shl 1;
    if start > 7 then
    begin
      inc (i);
      b := byte(s[i]);
      start := 0;
      mask := 1;
    end;
  end;

  Result := res;
end;

//xor'ar med 42 bara..
function SimpleCrypt (s :string) :string;
var
  res :string;
  i   :integer;
begin
  res := s;
  for i := 1 to length (res) do
    byte(res[i]) := byte(res[i]) xor 42;

  Result := res;
end;

Function  CRC32(value: Byte; crc: LongWord) : LongWord;
begin
  CRC32 := CRC32tab[Byte(crc xor LongWord(value))] xor
           ((crc shr 8) and $00ffffff);
end;

function CalcCRC (curcrc :longword; data :pointer; len :longword) :longword;
type
  PBuf = ^TBuf;
  TBuf = array[1..1000000] of byte;
var
  i :integer;
  c :longword;
begin
  c := curcrc;
  for i := 1 to len do
  begin
    c := crc32 (PBuf(data)^[i], c);
  end;

  Result := c;
end;

function RemoveInvalid (st :string) :string;
var
  i :integer;
begin
  for i := 1 to Length (st) do
  begin
    if not (st[i] in ['A'..'Z', 'a'..'z', '.', '-', '+', '|', '_', '0'..'9', '@',
                      '!', '#', '"', '%', '(', ')', '=', '[', ']', ',', ';',
                      ' ', '''']) then
      st [i] := '_';
  end;

  Result := st;
end;

end.
