unit Packet;
interface
uses
  PacketBufferU,
  TA_NetworkingMessages;
const
  // magic number of the TA unit which shares adds megasonar
  SY_UNIT = $92549357;

type
  TPacket = class
  public


// -----------------------------------------------------------------------------

// assumes input is in an unecrypted & uncompressed state

    // 0 based index!
    Class function KnownPacketType( BufferData : TBufferData; index :integer; var value : byte) : boolean;
    // Returns the size/length of a packet type. Returns 0 if it is unknown.
    class function PacketLength( BufferData : TBufferData; index :integer; var value : byte) :Integer;

    // Wants a raw packet... <marker>checkcheckffff<kind><data>
    // Returns next packet and removes it from S. Returns "" when s is empty... Duh.

    // Decrypts & Decompresses, *then* splits and then Encrypts & Compresses both chunks!
    // Slow, try not to use it
// assumes input is in the header+data packet format
// output is in the header+data packet format
    //class function Split (var s :string) :string;
    
    // Will take any packet format, convert it to a raw data format and split
    // the packet into a series of fragments. Each fragment is a TA network message
    // Call BufferedData.Current.CompileFragments to merge them into a single buffer
    // Fragments are stored in "BufferedData.Current.Fragments"
    class function Split2( BufferedData : TBufferedData;
                           smartpak:Boolean;
                           const FromPlayer, ToPlayer : string) : boolean;
        
    // writes a non-corrupt packet to the current buffer which blanks out the
    // corrupted packet
// output is in the complete packet format     
    class Procedure RemoveCorruptedPacket( BufferedData : TBufferedData );

    // These allocate a new buffer and write all changes there. the source buffer
    // will /never/ be changed

//    HeaderSize - this describes the number of bytes in the stream which is the header
//                 Valid values are 0, 1 or 3.
//  Decrypt, Encrypt, Decompress & Compress will automatically work with any packet format

    class Procedure Decrypt( BufferedData : TBufferedData );
    // Takes a packet with controlcodes and checksum (will overwrite checksum)
    class Procedure Encrypt( BufferedData : TBufferedData );
    // Takes a packet with controlcodes and checksum
    class Procedure Decompress( BufferedData : TBufferedData );
    // Takes a packet with controlcodes and checksum (will overwrite controlcode)
    // If the buffer cant be compressed, the current/source buffer isnt touched!
    class Procedure Compress( BufferedData : TBufferedData );
// -----------------------------------------------------------------------------
  protected
    fBufferData : TBufferData;

    function GetSize : integer;
    Function GetTimeStamp : longword;
    Procedure SetTimeStamp(value : longword);
    Function GetPacketMarker : byte;
    Procedure SetPacketMarker(value : byte);
  public
    Constructor Create(aBufferData : TBufferData);

    property BufferData : TBufferData read fBufferData;

    Property TimeStamp : longword read GetTimeStamp write SetTimeStamp;
    property PacketMarker : byte read GetPacketMarker write SetPacketMarker;
    Property Size : integer read GetSize;
  end;

var
  DecompressionBufferSize : Integer = 2048;
  UseCompression : Boolean = false;

  DoDecompresionWarning : boolean = true;
implementation

uses
  TextData, Logging, INI_Options, SysUtils;


// -----------------------------------------------------------------------------

Class function TPacket.KnownPacketType( BufferData : TBufferData; index :integer; var value : byte) : boolean;
begin
Result := TPacket.PacketLength( BufferData, index, value ) <> 0;
end;

class function TPacket.PacketLength( BufferData : TBufferData; index :integer; var value : byte) :Integer;
begin
assert( BufferData <> nil );
assert( index < BufferData.Size );
value := BufferData.Data[index];
case value of
  $02       :Result := 13;
  $06       :Result := 1;
  $07       :Result := 1;
  $20       :Result := 192;
  $1a       :Result := 14;
  $17       :Result := 2;
  $18       :Result := 2;
  $15       :Result := 1;
  $08       :Result := 1;
  $05       :
            begin
            Result := 65; // text message, 64 bytes of actual message
            if BufferData.GetByte(index+Result-1) <> 0 then
              begin
              // older recorder versions sometimes emit more text than they should
              // however, it is send as a single packet.
              result := BufferData.Size - index;
              // And if map position is enabled, the last 5 bytes should be the map
              // pos data
              if BufferData.GetByte(BufferData.Size - 5) = $fc then
                dec(result, 5 );
              end;
            end;
  $26       :Result := 41; // byte('&')
  $22       :Result := 6;  // byte('"')
  $2a       :Result := 2;  // byte('*')
  $1e       :Result := 2;
  $2c       :Result := BufferData.GetWord(index+1); // byte(',')

    //New packets

  // Appears as the packet that gives the orders that something newly built
  // shall be shown immediately. But it shows/is shown to the wrong person...
  $09       :result := 23;
  $11       :result := 4;         // ?? crash
  // Gives/does/creates explosions! They are shown in the wrong place, though.
  $10       :result := 22;
  $12       :result := 5;         //?? crash
  $0a       :result := 7;         //?? crash
  //- ?? No difference
  $28       :result := 58;
  // Speed/pause change
  $19       :result := 3;
  //Bullet. They stay put, though. And they miss...
  $0d       :if iniSettings.weaponidpatch then Result:= 40 else Result := 36;
  // Eliminates bullet scraps/rests of bullets
  $0b       :result := 9;
  // Makes the Commander's upperbody/torso to turn correctly when he's building, among other things (or perhaps: etc).
  $0f       :Result := 6;
  // Hmm. Seems to give/do/create explosions too/with
  $0c       :result := 11;      

  $1f       :result := 5;
  $23       :result := 14;
  $16       :result := 17;
  $1b       :if iniSettings.weaponidpatch then Result:= 8 else result := 6;
  $29       :result := 3;
  $14       :result := 24;

  $21       :result := 10;
  $03       :result := 7;
  $0e       :if iniSettings.weaponidpatch then Result:= 17 else result := 14;

  // TADR packet types
  $f6       :result := 1;
  //enemy-chat
  $f9       :result := 73;
  $fa       :result := 1;
  //recorder data connect
  $fb       :result := BufferData.GetByte(index+1)+3;
  //map position
  $fc       :Result := 5;
  //smartpak
  $fd       :Result := BufferData.GetWord(index+1)-4;
  //smartpak
  $fe       :result := 5;
  // Smartpak packet should not exist in wild condition
  $ff       :result := 1;         
  else       result := 0;
end;
end;

class Procedure TPacket.RemoveCorruptedPacket( BufferedData : TBufferedData );
var BufferData : TBufferData;
begin
BufferData := BufferedData.Current;
TLog.Add(0,'Removing corrupted packet ');
// make sure the buffer is the right size for when we write stuff to it
BufferData.Size := 0;
BufferData.CheckGrowth(9);
// write data to the buffer
BufferData.HeaderState := hs_CompletePacket;
BufferData.AddByte(  TANM_ChecksummedPacket );
BufferData.AddWord(  0 );
BufferData.AddDWord( $ffffffff );
BufferData.AddByte(  $2a );
BufferData.AddByte(  $64 );
end;

{
      // copy the packet to be split to the new buffer
      DestBufferData.Size := packetLength;
      move( SourceBufferData.Data[0], DestBufferData.Data[0], packetLength);
      // move the old data around
      LeftoversLen := SourceLen-packetLength;
    if LeftoversLen > 0 then
      move( SourceBufferData.Data[packetLength], SourceBufferData.Data[0], LeftoversLen)
    else
      LeftoversLen := 0;
    SourceBufferData.Size := LeftoversLen;
    }
class function TPacket.Split2( BufferedData : TBufferedData;
                               smartpak:Boolean;
                               const FromPlayer, ToPlayer : string) : boolean;
var
  SourceBufferData : TBufferData;
  SourceLen : integer;
  packetLength :integer;
  CurrentIndex : integer;
  
  // Do error output here so string cleanup doesnt slow down the main function
  Procedure Error1;
  begin
  Tlog.add(1,'From '+FromPlayer+' to '+ ToPlayer+',Warning erroneous compression assumption');
  Tlog.add(1,'Packet:'+ DataToStr2( SourceBufferData.ToString( CurrentIndex ) ) );
  end;

  Procedure Error2;
  begin
  Tlog.add(1, 'From '+FromPlayer+' to '+ ToPlayer+
              ', Error subpacket ('+inttostr(packetLength)+') longer than packet ('+inttostr(SourceLen-CurrentIndex)+
              ')');
  end;

  Procedure Error3;
  begin
  TLog.Add (1, 'From '+FromPlayer+' to '+ ToPlayer+', unknown packet : $' +
               IntToHex( SourceBufferData.GetByte( CurrentIndex ), 2) + ' '+
               DataToStr2( SourceBufferData.ToString( CurrentIndex ) ));
  end;

  Procedure AllocPacket( packetLength : integer);
  begin
  SourceBufferData.AllocFragment( CurrentIndex, packetLength );
  end;
var
  value : byte;
  resetOffsetOnExit : boolean;
  OldHeaderState : THeaderState;
begin
resetOffsetOnExit := false;
assert( BufferedData.BufferCount >= 1);
// get the source buffer
try
SourceBufferData := BufferedData.Current;
if (SourceBufferData = nil) or (SourceBufferData.Size <= 0)  then
  begin // fail
  result := false;
  end
else
  begin
  OldHeaderState := SourceBufferData.HeaderState;  
  if (SourceBufferData.HeaderState <> hs_RawData) then
    begin // we expect a different buffer format, convert it
    if SourceBufferData.Data[ 0 ] = TANM_CompressedPacket then
      Decompress( BufferedData );
    Decrypt( BufferedData );
    SourceBufferData.HeaderState := hs_RawData;
    resetOffsetOnExit := true;
    SourceBufferData.OffsetIndex := PacketOverhead[OldHeaderState];
    end;
  result := true;
  SourceLen := SourceBufferData.Size;
  CurrentIndex := 0;
  while ( CurrentIndex < SourceLen) do
    begin
    packetLength := TPacket.PacketLength( SourceBufferData, CurrentIndex, value );
    if not smartpak and ( (value = $ff) or (value = $fe) or (value = $fd)) then
      begin
      Error1;
      end;
    if CurrentIndex+packetLength > SourceLen then
      begin
      Error2;
      packetLength := 0;
      end;
    if packetLength <= 0 then
      begin
      Error3;
      AllocPacket(SourceLen - CurrentIndex);
      break;
      end
    else
      AllocPacket(packetLength);
    inc(CurrentIndex,packetLength);  
    end;
  end
finally
  if resetOffsetOnExit then
    begin
    SourceBufferData.HeaderState := OldHeaderState;
    SourceBufferData.OffsetIndex := -PacketOverhead[OldHeaderState];
    end;
end;
end;

class Procedure TPacket.Decrypt( BufferedData : TBufferedData);
var
  SourceBufferData : TBufferData;
  DestBufferData : TBufferData;
  SourceData : PByteArray;
  DestData : PByteArray;    
  SourceLen : Integer;
  i : integer;
  CheckSum :word;
  HeaderSize : integer;
  xorKey : integer;
begin
assert( BufferedData.BufferCount >= 1);
// allocate the destination buffer & get the source buffer
DestBufferData := BufferedData.NewBuffer;
SourceBufferData := BufferedData.PrevBuffer;
if (SourceBufferData = nil) then
  begin // marks the destination buffer as unused, reverts to the source buffer
  BufferedData.ReleaseBuffer;
  exit;
  end;
HeaderSize := PacketHeaderSize[SourceBufferData.HeaderState];
assert(HeaderSize >= 0);
if SourceBufferData.Size < HeaderSize+1 then
  begin
  // buffer is too small for an encrypted packet
{
  TBufferedData.SetDataLength( DestBufferData, SourceLen );
  move( SourceBufferData.Data[0], DestBufferData.Data[0], SourceLen);
  TBufferedData.AddByte(  DestBufferData, $06 );
}
  RemoveCorruptedPacket( BufferedData );
  // swap buffers
  BufferedData.SwapBuffers;
  BufferedData.ReleaseBuffer;
  end
else 
  begin
  SourceLen := SourceBufferData.Size;
  DestBufferData.Size := SourceLen;
  SourceData := SourceBufferData.data;
  DestData := DestBufferData.data;
  // marker + checksum is to be skipped from the checksum process
  DestBufferData.HeaderState := SourceBufferData.HeaderState;
  // todo : TPacket.Decrypt - determine if the header should be forced to be set to a value
{
  if HeaderSize >= PacketHeaderSize[hs_MiniHeader] then
    DestBufferData.Data[ 0 ] := TANM_ChecksummedPacket;
}
  if HeaderSize > 0 then
    move( SourceData[0],  DestData[0], HeaderSize );
  // compute the checksum
  checksum := 0;
  xorKey := sizeof(TChecksummedPacket);
  for i := HeaderSize to SourceLen - 4 do
    begin
    CheckSum := word(CheckSum + SourceData[i]);
    DestData[i] := SourceData[i] xor byte(xorKey);
    inc(xorKey);
    end;
  // for some reason the last 3 bytes arnt checksummed, WTF
  i := SourceLen - 3;
  move( SourceData[i], DestData[i], 3 );
  // validate the checksum value
  if (HeaderSize >= PacketHeaderSize[hs_Header]) and
     (CheckSum <> SourceBufferData.GetWord( 1 )) then
    RemoveCorruptedPacket(BufferedData);
  // swap the source & dest buffers
  BufferedData.SwapBuffers;
  // release the source buffer, the dest buffer is now the active buffer
  BufferedData.ReleaseBuffer;
  end;
end;

class Procedure TPacket.Encrypt( BufferedData : TBufferedData );
var
  SourceBufferData : TBufferData;
  DestBufferData : TBufferData;
  SourceData : PByteArray;
  DestData : PByteArray;    
  SourceLen : Integer;
  i : integer;
  checksum :word;
  xorKey : integer;
  HeaderSize : integer;
begin
assert( BufferedData.BufferCount >= 1);
// allocate the destination buffer & get the source buffer
DestBufferData := BufferedData.NewBuffer;
SourceBufferData := BufferedData.PrevBuffer;
if (SourceBufferData = nil) then
  begin // marks the destination buffer as unused, reverts to the source buffer
  BufferedData.ReleaseBuffer;
  exit;
  end;
HeaderSize := PacketHeaderSize[SourceBufferData.HeaderState];
SourceLen := SourceBufferData.Size;
if SourceLen < HeaderSize+1 then
  begin
  RemoveCorruptedPacket( BufferedData );
  // swap buffers
  BufferedData.SwapBuffers;
  BufferedData.ReleaseBuffer;
  end
else
  begin
  DestBufferData.Size := SourceLen;
  SourceData := SourceBufferData.data;
  DestData := DestBufferData.data;
  // marker + checksum is to be skipped from the checksum process
  // checksum doesnt need copying in TPacket.Encrypt, we generate it
  DestBufferData.HeaderState := SourceBufferData.HeaderState;
  if HeaderSize > PacketHeaderSize[hs_MiniHeader] then
    DestBufferData.Data[ 0 ] := SourceBufferData.Data[ 0 ];
//  if HeaderSize > PacketHeaderSize[hs_MiniHeader] then
//    DestBufferData.Data[ 0 ] := TANM_ChecksummedPacket;
  // compute the checksum
  checksum := 0;
  xorKey := sizeof( TChecksummedPacket);   
  for i := HeaderSize to SourceLen - 4 do
    begin
    DestData[i] := SourceData[i] xor byte(xorKey);
    checksum := word(checksum + DestData[i]);
    inc(xorKey);    
    end;
  // for some reason the last 3 bytes arnt checksummed, WTF
  i := SourceLen - 3;
  move( SourceData[i], DestData[i], 3 );
  // write the checksum value, but only if we arent using mini-header mode
  if HeaderSize >= PacketHeaderSize[hs_Header] then
    DestBufferData.SetWord( 1, checksum );
  // swap the source & dest buffers
  BufferedData.SwapBuffers;
  // release the source buffer, the dest buffer is now the active buffer
  BufferedData.ReleaseBuffer;
  end;
end;

class Procedure TPacket.Decompress( BufferedData : TBufferedData );
var
  SourceBufferData : TBufferData;
  DestBufferData : TBufferData;

  SourceData : PByteArray;
  DestData : PByteArray;
  SourceIndex, ChunkNumber :integer;
  SourceLen : Integer;

  a,uop : Integer;
  RunIndex : Integer;

  Fixup1stByte : Boolean;
  IsCompressedBitMask : byte;
  count : Integer;

  Procedure DoBufferInc;
  var
    newBufferSize : Integer;
  begin
  newBufferSize := 2*(DecompressionBufferSize+1);
  Tlog.Add( 3, 'TPacket.Decompress, increasing decompression buffer size from ' +IntToStr(DecompressionBufferSize) +' to '+IntToStr(newBufferSize) );
  DecompressionBufferSize := newBufferSize;
  DestBufferData.Size := DestBufferData.Size + DecompressionBufferSize ;
  DestData := @DestBufferData.Data[0];
  end;

  Procedure DoWarning;
  begin
  DoDecompresionWarning := false;
  Tlog.Add( 0, 'Decompression of packet expecting more data, 1st byte still set as $4');
  end;
  
var
  HeaderSize : integer;
begin
assert( BufferedData.BufferCount >= 1);
// allocate the destination buffer & get the source buffer
DestBufferData := BufferedData.NewBuffer;
SourceBufferData := BufferedData.PrevBuffer;
if (SourceBufferData = nil) or (SourceBufferData.Data[0] <> TANM_CompressedPacket) then
  begin // marks the destination buffer as unused, reverts to the source buffer
  BufferedData.ReleaseBuffer;
  exit;
  end;
HeaderSize := PacketHeaderSize[SourceBufferData.HeaderState];
SourceLen := SourceBufferData.Size;
if (SourceLen < HeaderSize+1) then
  begin // marks the destination buffer as unused, reverts to the source buffer
  BufferedData.ReleaseBuffer;
  exit;
  end;
SourceData := SourceBufferData.Data;
DestBufferData.Size := DecompressionBufferSize;
DestData := DestBufferData.Data;
// skip the packet header (this is either "marker + checksum" or just "marker")
DestBufferData.HeaderState := SourceBufferData.HeaderState;
SourceIndex := HeaderSize;
count := 0;
while count < HeaderSize do
  begin
  DestData[count] := SourceData[count];
  inc(count);
  end;
Fixup1stByte := True;
try
  while SourceIndex <= SourceLen do
    begin
    IsCompressedBitMask := SourceData[SourceIndex];
    Inc( SourceIndex );
    for ChunkNumber := 0 to 7 do
      begin
      if SourceIndex >= SourceLen then
        begin // need more input data to actuallt decompress the stream
//  Tlog.Add( 0, 'Decompression of packet expecting more data, 1st byte still set as $4');
        if not DoDecompresionWarning then
          DoWarning;
        Fixup1stByte := False;
        exit;
        end;
      if (( IsCompressedBitMask shr ChunkNumber) and 1) = 0 then
        begin
        inc( Count );
        if Count > DecompressionBufferSize then
          DoBufferInc;
        DestData[Count-1] := SourceData[SourceIndex];
        Inc( SourceIndex );
        end
      else
        begin
        uop :=  SourceBufferData.GetWord( SourceIndex );
        Inc( SourceIndex, 2 );
        a := uop shr 4;
      	if a = 0 then
          exit;
        uop := uop and $0f;
        for RunIndex := a to a + uop + 1 do
          begin
          inc( Count );
          if Count > DecompressionBufferSize then
            DoBufferInc;
          DestData[Count-1] := DestData[RunIndex+2];
          end;
        end;
      end;
    end;
finally
  // finalize the length
  DestBufferData.Size := count;

  if Fixup1stByte and (HeaderSize >= PacketHeaderSize[hs_MiniHeader]) then
    DestBufferData.Data[0] := TANM_ChecksummedPacket;
  // swap the source & dest buffers
  BufferedData.SwapBuffers;
  // release the source buffer, the dest buffer is now the active buffer
  BufferedData.ReleaseBuffer;
end;
end;

class Procedure TPacket.Compress( BufferedData : TBufferedData );
var
  DestBufferData : TBufferData;
  SourceData : PByteArray;
  SourceBufferData : TBufferData;
  SourceIndex : integer;
  SourceLen : Integer;

  MatchLength, NewMatchLength : integer;
  MatchIndex, NewMatchIndex : word;
  EncodedChar : word;

  ChunkEncodingIndex: integer;
  ChunkCount : integer;

  HeaderSize : integer;
begin
assert( BufferedData.BufferCount >= 1);
SourceBufferData := BufferedData.Current;
if not UseCompression or (SourceBufferData = nil) then
  begin // marks the destination buffer as unused, reverts to the source buffer
{
  if PacketHeaderSize[SourceBufferData.HeaderState] >= PacketHeaderSize[hs_MiniHeader] then
    begin
    SourceBufferData.Data[ 0 ] := TANM_ChecksummedPacket;
    end;
}
  exit;
  end;
// allocate the destination buffer & get the source buffer
DestBufferData := BufferedData.NewBuffer;
SourceBufferData := BufferedData.PrevBuffer;
  
HeaderSize := PacketHeaderSize[SourceBufferData.HeaderState];
ChunkCount := 7;
// skip the 1st 3 bytes of the source
// marker + checksum
SourceData := @SourceBufferData.Data[0];
SourceLen := SourceBufferData.Size;
DestBufferData.CheckGrowth( SourceLen );
DestBufferData.Size := HeaderSize;
// marker + checksum
DestBufferData.HeaderState := SourceBufferData.HeaderState; 
if HeaderSize >= PacketHeaderSize[hs_MiniHeader] then
  DestBufferData.Data[ 0 ] := TANM_CompressedPacket;
SourceIndex := 1;
while SourceIndex < HeaderSize do
  begin
  DestBufferData.Data[SourceIndex] := SourceData[SourceIndex];
  inc(SourceIndex);
  end;
ChunkEncodingIndex := 0;
MatchIndex := 0; 
while SourceIndex < SourceLen do
  begin
  // We have encountered a new lz77 chunk block
  // allocate a byte for the encoded value & mark the location
  if ChunkCount = 7 then
    begin 
    ChunkCount := -1;
    ChunkEncodingIndex := DestBufferData.AddByte( 0 );
    end;
  inc(ChunkCount);
  // Do not attempt to compress the first 2 bytes
  if (SourceIndex < HeaderSize+2 ) or ( SourceIndex > 2000) then
    begin
//    if SourceIndex > 2000 then
//      asm int 3 end;
    DestBufferData.AddByte( SourceData[ SourceIndex ] );
    inc( SourceIndex );
    end
  else
    begin
    MatchLength := 2;
    // try to extend an earlier sequence of matching character
    for NewMatchIndex := HeaderSize to SourceIndex -2 do
      begin
      NewMatchLength := 0;
      while ((SourceIndex + NewMatchLength) < SourceLen-1 ) and
            (NewMatchIndex+NewMatchLength < SourceIndex) and
            (SourceData[NewMatchIndex+NewMatchLength] = SourceData[SourceIndex+NewMatchLength]) do
        inc( NewMatchLength );
      // Is sequence worth saving?
      if NewMatchLength > MatchLength then
        begin
        MatchLength := NewMatchLength;
        MatchIndex := NewMatchIndex+1;
        // check to see if it has reached the maxium sequance length
        if MatchLength > $11 then
          break;
        end;
      end;
    NewMatchLength := 0;
    //Repetition of the same character?
    while (SourceIndex+NewMatchLength < SourceLen-1) and
          (SourceData[SourceIndex+NewMatchLength] = SourceData[SourceIndex-1]) do
      inc( NewMatchLength );
    // Is sequence worth saving?
    if (NewMatchLength > MatchLength) then
      begin
      MatchLength := NewMatchLength;
      MatchIndex := SourceIndex;
      end;
    // Did we find a sufficently long sequence?  
    if MatchLength > 2 then
      begin
      DestBufferData.Data[ChunkEncodingIndex] := DestBufferData.Data[ChunkEncodingIndex]  or
                                                  ((1 shl ChunkCount) and $ff);
      MatchLength := (MatchLength - 2) and $0f;
      EncodedChar := ((MatchIndex-HeaderSize) shl 4) or MatchLength;
      DestBufferData.AddWord( EncodedChar );
      SourceIndex := SourceIndex + MatchLength+2;
      end
    else
      begin // No sequence worth using
      DestBufferData.AddByte( SourceData[SourceIndex] );
      inc(SourceIndex);
      end;
    end;
  end;
if ChunkCount = 7 then
  DestBufferData.AddByte( $ff )
else
  DestBufferData.SetByte( ChunkEncodingIndex,
                         DestBufferData.GetByte( ChunkEncodingIndex) or
                         (($ff shl (ChunkCount+1)) and $ff));
DestBufferData.AddWord( $0 );

if DestBufferData.Size < SourceLen then
  begin
  // swap the source & dest buffers
  BufferedData.SwapBuffers;
  end
{
else if HeaderSize >= PacketHeaderSize[hs_MiniHeader] then
  begin
  // we leave the souce buffer ALONE
  SourceData[ 0 ] := TANM_ChecksummedPacket;
  end};
// release the extra buffer
BufferedData.ReleaseBuffer;
end;

// -----------------------------------------------------------------------------

Constructor TPacket.Create(aBufferData : TBufferData);
begin
inherited create;
fBufferData := aBufferData;
assert(BufferData <> nil);
end;


function TPacket.GetSize : integer;
begin
result := BufferData.Size - PacketOverhead[BufferData.HeaderState];
end;

Function TPacket.GetTimeStamp : longword;
begin
if BufferData.HeaderState = hs_CompletePacket then
  result := BufferData.GetDWord(3)
else
  result := high(longword);
end;

Procedure TPacket.SetTimeStamp(value : longword);
begin
if PacketOverhead[BufferData.HeaderState] >= BufferData.Size then
  BufferData.CheckGrowth(PacketOverhead[BufferData.HeaderState]);
if BufferData.HeaderState = hs_CompletePacket then
  BufferData.SetDWord(3,value)
else
  begin
  asm int 3 end;
  end;
end;

Function TPacket.GetPacketMarker : byte;
begin
if PacketOverhead[BufferData.HeaderState] < BufferData.Size then
  result := BufferData.GetByte(PacketOverhead[BufferData.HeaderState])
else
  result := 0;
end;

Procedure TPacket.SetPacketMarker(value : byte);
begin
if PacketOverhead[BufferData.HeaderState] >= BufferData.Size then
  BufferData.CheckGrowth(PacketOverhead[BufferData.HeaderState]);
BufferData.SetByte(PacketOverhead[BufferData.HeaderState],value)
end;

end.
