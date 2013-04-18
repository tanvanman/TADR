unit PacketBufferU;
// enables checks for unreleased buffers. To prevent runaway allocation of new buffers
{$DEFINE SanityAllocCheck}
interface
uses
  sysutils,
  classes,
  ListsU;
const
  DefaultBufferCount = 256;//32;
  DefaultBufferSize = 2048;
  DefaultBufferFragmentCount = 256;
var
  InitialBufferCount : Integer = DefaultBufferCount;
  InitialBufferSize : Integer = DefaultBufferSize;
  InitialBufferFragmentCount : Integer = DefaultBufferFragmentCount;
type
  // Packet format info:
  //  <Payload> = (<1 byte kind><data...>)*
  THeaderState = (
                  // Full TA packet format
                  hs_CompletePacket, // <1 byte marker><2 byte checksum><4 timestamp> + <Payload>
                  // Header+data packet format
                  hs_Header,         // <1 byte marker><2 byte checksum> + <Payload>
                  // mini-header+data packet format
                  hs_MiniHeader,     // <1 byte marker> + <Payload>
                  // raw data packet format
                  hs_RawData         // <Payload>
                 );
const
// this contols the size of the header for compress, & encryption matters
  PacketHeaderSize : array [THeaderState] of integer = (
                                                        3, //7, // hs_CompletePacket
                                                        3, // hs_Header
                                                        1, // hs_MiniHeader
                                                        0  // hs_RawData
                                                       );
  // the number of bytes of overhead the packet has per headerstate                                                     
  PacketOverhead : array [THeaderState] of integer = (
                                                        7, // hs_CompletePacket
                                                        3, // hs_Header
                                                        1, // hs_MiniHeader
                                                        0  // hs_RawData
                                                       );

{
  Each packet TA sends contains the following info:
   - header : TChecksummedPacket
     - Header including a marker if the packet is compressed or not.
     - Includes checksum for the packet's payload
   - Data : TPacketPayload;
     The Data for the packet
}

Type
  PPacketPayload = ^TPacketPayload;
  TPacketPayload = packed record
    // Timestamp for every message in the packet
    TimeStamp : longword;
    // Collection of messages, 1 byte marker followed by the message data
    Data : packed array [ 0 .. high(integer) - 5] of Byte;
  end;
  
{
An Uncompressed data packet, data is encypted with a simple algo
}
Const
  TANM_ChecksummedPacket = $03;
{
  1st 3 bytes & last 3 bytes are not checksummed, but need to be copied to the dest buffer
  - This skips the Marker & Checksum at the start.
  - Unknown what the last 3 bytes are

 decryption algo:
  >>
  checksum := 0;
  for i := 3 to SourceBufferData^.Size - 4 do
    begin
    CheckSum := (CheckSum + SourceBufferData^.Data[i]) and $ffff;
    DestBufferData^.Data[i] := SourceBufferData^.Data[i] xor i;
    end;
  >>
 Encyption algo:
  >>
  checksum := 0;
  for i := 3 to SourceBufferData^.Size - 4 do
    begin
    DestBufferData^.Data[i] := SourceBufferData^.Data[i] xor i;
    checksum := (checksum + DestBufferData^.Data[i]) and $ffff;
    end;
  >>
}
type
  PChecksummedPacket = ^TChecksummedPacket;
  TChecksummedPacket = packed record
    Marker  : byte;       // TANM_ChecksummedPacket
    CheckSum : Word;      // checksum on data payload of the packet
//    Data : TPacketPayload; - The data for the packet
  end;

  
{
TA can use compression on each packet sent to DirectPlay.
It uses a version of lz77.
}
// todo : describe compression algos
Const
  TANM_CompressedPacket = $04;
type
  PCompressedPacket = ^TCompressedPacket;
  TCompressedPacket = packed record
    Marker  : byte;       // TANM_CompressedPacket
    CheckSum : Word;      // checksum on data payload of the packet
//    Data : TPacketPayload; - The data for the packet
  end;
    

type   
//  TByteArray = array [0.. high(integer) div sizeof(byte) -1] of byte;
//  PByteArray = ^TByteArray;

//  TLongwordArray = array [0.. high(integer) div sizeof(longword) -1] of longword;
//  PLongwordArray = ^TLongwordArray;
  TArrayOfByte = array of byte;

  TBufferedData = class;
  TBufferData = class;
  TBufferDataArray = array of TBufferData;

  // Limit the amount fo direct fiddling of the TBufferData structure,
  // use TBufferedData class functions
  TBufferData = class
  protected
    fSize : integer;
{$IFDEF SanityAllocCheck}
    WasFragment : boolean;
{$ENDIF}    
    
    fFragments : TBufferDataArray;
    fFragmentCount : integer;
    // indicates that not only is this a fragement, but the max size more data can be added before
    // reallocation to a private buffer occurs
    FFragmentMaxSize : integer;
    fFragmentIndex : integer;

    fBufferedData : TBufferedData;
    Procedure SetDataSize( NewSize : integer);
    Procedure DoGrowth( NewSize : integer);
    Constructor Create(aBufferedData : TBufferedData);
    Procedure MakeFragment(aDataPtr : TArrayOfByte; StartOffset : integer; aSize : integer);
  protected
    fOffsetIndex : integer;
    Procedure SetOffset(Index : integer);
  protected
    fArraySize : integer;
    fData : PByteArray;
    fDataPtr : TArrayOfByte;
    fDataPtr2 : TArrayOfByte;
  public
    HeaderState : THeaderState;
    Destructor Destroy; override;

    Property Data : PByteArray read fData;
    Property DataPtr : TArrayOfByte read fDataPtr;
    Property ArraySize : integer read fArraySize;

    Procedure Reset;

    Property Size : integer read fSize write SetDataSize;
    // pre-grows the underlying array if required if the new growth will trigger it
    procedure CheckGrowth( SizeIncrease : integer);    
    // offsets the data buffer by 'index', a negitive amount moves it back if posible
    // returns the new size    
    Property OffsetIndex : integer read fOffsetIndex write SetOffset;
    // resets the offset to zero. Returns the old offset value
    function ResetOffsetIndex() : integer;
    
    Property BufferedData : TBufferedData read fBufferedData;

    // follows Delphi Insert procedure
    Procedure InsertData( index : integer;var buff; DataSize : integer);
    // follows Delpgi Delete procedure
    Procedure DeleteData( index : integer; len : integer);

    function AddByte( value : byte ) : integer;
    function AddWord( value : word ) : integer;
    function AddDWord( value : longword ) : integer;
    // returns < 0 if failed to add stuff
    function AddBuffer( SourceBufferData : TBufferData;
                        SourceIndex : integer = 0;
                        SourceLen : integer = -1 ) : integer;

    Procedure SetByte( index : integer; value : byte );
    Procedure SetWord( index : integer; value : word );
    Procedure SetDWord( index : integer; value : longword );

    function GetByte( index : integer ) : byte;
    function GetWord( index : integer) : word;
    function GetDWord( index : integer) : longword;

    function ToString( index : integer = 0; count : integer = -1) : String;
    function FragmentsToString( ) : String;


    Property Fragments : TBufferDataArray read fFragments;
    Property FragmentCount : integer read fFragmentCount;
    Function AllocFragment( StartIndex, aSize : integer) : TBufferData;

    // todo : reduce the number of copy operations TBufferData.CompileFragments does
    Procedure CompileFragments( DestBuffer : TBufferData ); overload;
    Procedure CompileFragments( ); overload;
    Procedure DiscardFragments( );
  end;

  TBufferedData = class
  protected
    fReleaseList : TStack;
    fFreeList : TStack;

    fBufferCount : integer;
    fCurrent : TBufferData;
{$IFDEF SanityAllocCheck}
    fAllocatedFragments : integer;
{$ENDIF}
    Function GetPendingReleaseCount : integer;
  public
  currentindex : integer;
    Constructor Create;
    Destructor Destroy; override;

//    Property Buffers[index : integer] : TBufferData Read GetBuffers;
    Property BufferCount : integer Read fBufferCount;
    Property PendingReleaseCount : integer Read GetPendingReleaseCount;
    Property Current : TBufferData Read fCurrent;

    // Swaps the current and next buffer
    Procedure SwapBuffers;
  protected
    // being able to manuelly determine if something is added to the release list
    // can easily result in memory leaks. The functionaility is only there to support
    // buffer fragments
    Function NewBuffer(AddToReleaseList : boolean) : TBufferData;  overload;
  public  
    Function NewBuffer : TBufferData; overload;
    Function PrevBuffer : TBufferData;

    // releases a buffer, pushes it onto the free list and gets the next buffer
    // and stores it in the current buffer variable
    Procedure ReleaseBuffer;
    // Pushes the buffer and any buffer fragements onto the freelist + internal book-keeping
    Procedure DiscardBuffer( BufferData : TBufferData );    


    class Function MakeData(var Data : string; size : integer) : pointer;
    class Procedure CopyData(const s : string; offset: integer; size : integer; var dest);
  end;

implementation
uses
  textdata,
  logging;

class Function TBufferedData.MakeData(var Data : string; size : integer) : pointer;
begin
setlength(Data,size);
result := @Data[1];
assert(result <> nil);
end;

class Procedure TBufferedData.CopyData(const s : string; offset: integer; size : integer; var dest);
var
  lenToCopy : integer;
begin
if length(s)-Offset < size then
  lenToCopy := length(s)-Offset
else
  lenToCopy := size;
move(s[offset],dest,lenToCopy);
end;
// -----------------------------------------------------------------------------

Constructor TBufferData.Create(aBufferedData : TBufferedData);
begin
fBufferedData := aBufferedData;
HeaderState := hs_RawData;
setlength(fDataPtr2, InitialBufferSize);
fDataPtr := fDataPtr2;
fData := PByteArray(DataPtr);
fArraySize := InitialBufferSize;
end;

Destructor TBufferData.Destroy;
var i : integer;
begin
if fFragments <> nil then
  begin
  for i := length(fFragments)-1 downto 0 do
    FreeAndNil( fFragments[i] );
  fFragments := nil;
  end;
end;

Procedure TBufferData.MakeFragment(aDataPtr : TArrayOfByte; StartOffset : integer; aSize : integer);
begin
HeaderState := hs_RawData;
fSize := aSize;
fDataPtr := aDataPtr;
fArraySize := length(aDataPtr);
fData := @(DataPtr[StartOffset]);
FFragmentMaxSize := aSize;
fFragmentIndex := StartOffset;
{$IFDEF SanityAllocCheck}
WasFragment := true;
{$ENDIF}
end;

Procedure TBufferData.Reset();
begin
HeaderState := hs_RawData;
fDataPtr := fDataPtr2;
fData := @DataPtr[0];
fSize := 0;

DiscardFragments();
fOffsetIndex := 0;
fFragmentIndex := 0;
FFragmentMaxSize := 0;
end;

function TBufferData.ResetOffsetIndex( ) : integer;
begin
result := OffsetIndex;
inc(fSize, result);
fOffsetIndex := 0;
fData := @(DataPtr[fFragmentIndex]);
end;

Procedure TBufferData.SetOffset(Index : integer);
var
  newSize : integer;
begin
newSize := Size-index;
if (newSize > 0) and (newSize < Size) then
  begin
  fOffsetIndex := fOffsetIndex + Index;
  if fFragmentIndex+OffsetIndex >= length(DataPtr) then
    asm int 3 end;
  if fFragmentIndex+OffsetIndex < 0 then
    asm int 3 end;
  fData := PByteArray(@DataPtr[fFragmentIndex+OffsetIndex]);
  fSize := newSize;
  end
else
  begin
  fOffsetIndex := 0;
  fSize := 0;
  end;
end;

Procedure TBufferData.DoGrowth( NewSize : integer );
var
  newDataPtr : TArrayOfByte;
  copylen : integer;
  newBufferCount : integer;
  Procedure Report();
  begin
  Tlog.Add( 3, 'TBufferData.DoGrowth, increasing initial buffer size from ' +
               IntToStr(InitialBufferSize) +' to '+IntToStr(newBufferCount) );
  end;
begin
if NewSize > InitialBufferSize then
  begin
  while NewSize > InitialBufferSize do
    begin
    newBufferCount := (InitialBufferSize * 3) div 2 - 1;
    if Log_.VerboseLoggingLevel >= 3 then
       Report;
    InitialBufferSize := newBufferCount;
    end;
  end;
NewSize := InitialBufferSize;
assert( NewSize > 0 );
if (FFragmentMaxSize <> 0) then
  begin
  newDataPtr := fDataPtr2;
  if NewSize > length(newDataPtr) then
    setlength(newDataPtr, NewSize);
  if (NewSize >= FFragmentMaxSize) then
    copylen := FFragmentMaxSize
  else
    copylen := NewSize;
  move( DataPtr[fFragmentIndex], newDataPtr[0], copylen);
  fDataPtr := newDataPtr;
  fDataPtr2 := newDataPtr;
  // no long a fragment to somewhere else
  fFragmentIndex := 0;
  FFragmentMaxSize := 0;
  end
else
  begin
  setlength(fDataPtr, NewSize);
  fDataPtr2 := fDataPtr;
  end;
fArraySize := NewSize;
fData := PByteArray(@DataPtr[fFragmentIndex+OffsetIndex]);
end; {DoGrowth}

procedure TBufferData.CheckGrowth( SizeIncrease : integer);
var NewSize : integer;
begin
NewSize := OffsetIndex+Size + SizeIncrease;
if NewSize > ArraySize then
  DoGrowth( NewSize )
else if (FFragmentMaxSize <> 0) and (NewSize > FFragmentMaxSize) then
  DoGrowth( NewSize );
end;

Procedure TBufferData.SetDataSize( NewSize : integer);
begin
if NewSize <> fSize then
  begin
  NewSize := OffsetIndex+NewSize;
  if NewSize > ArraySize then
    DoGrowth( NewSize )
  else if (FFragmentMaxSize <> 0) and (NewSize > FFragmentMaxSize) then
    DoGrowth( NewSize );
  fSize := NewSize-OffsetIndex;
  end;
end;


function TBufferData.AddBuffer( SourceBufferData : TBufferData;
                                SourceIndex : integer = 0;
                                SourceLen : integer = -1 ) : integer;
begin
assert( SourceBufferData <> nil );
if SourceIndex < 0 then
  SourceIndex := 0;
if (SourceLen < 0) or
   (SourceLen + SourceIndex > SourceBufferData.Size) then
  SourceLen := SourceBufferData.Size - SourceIndex;
if SourceLen > 0 then
  begin
  result := Size;
  Size := result+SourceLen;
  move( SourceBufferData.Data[SourceIndex], Data[result], SourceLen );
  end
else
  result := -1;  
end;

Procedure TBufferData.InsertData( index : integer; var Buff; DataSize : integer);
begin
Size := Size + DataSize;
move( Data[index], Data[index+DataSize], Size-DataSize);
move( Buff, Data[index], DataSize);
end;

Procedure TBufferData.DeleteData( index : integer; len : integer);
begin
if (index < 0) or (index >= Size) or (len <= 0) then
 exit;
if index+len >= Size then
  len := Size - index;
if len < Size -1 then
  move( Data[index+len], Data[index], len);
Size := Size -len;
end;

function TBufferData.AddByte( value : byte ) : integer;
begin
result := Size;
Size := Size + sizeof(value);
Data[result] := value;
end;                  

function TBufferData.AddWord( value : word ) : integer;
begin
result := Size;
Size := Size + sizeof(value);
PWord(@Data[result])^ := value;
end;

function TBufferData.AddDWord(value : longword ) : integer;
begin
result := Size;
Size := Size + sizeof(value);
Plongword(@Data[result])^ := value;
end;

Procedure TBufferData.SetByte( index : integer; value : byte );
begin
assert( index+(sizeof(value)-1) < Size );
Data[index] := value;
end;                  

Procedure TBufferData.SetWord( index : integer; value : word );
begin
assert( index+(sizeof(value)-1) < Size );
PWord(@Data[index])^ := value;
end;

Procedure TBufferData.SetDWord( index : integer; value : longword );
begin
assert( index+(sizeof(value)-1) < Size );
Plongword(@Data[index])^ := value;
end;

function TBufferData.GetByte( index : integer ) : byte;
begin
assert( index+(sizeof(result)-1) < Size );
result := PByte(@Data[index])^;
end;

function TBufferData.GetWord( index : integer) : word;
begin
if index+(sizeof(result)-1) >= Size then
  asm int 3 end;
assert( index+(sizeof(result)-1) < Size );
result := PWord(@Data[index])^;
end;

function TBufferData.GetDWord( index : integer) : longword;
begin
assert( index+(sizeof(result)-1) < Size );
result := Plongword(@Data[index])^;
end;

function TBufferData.ToString( index : integer = 0; count : integer = -1) : String;
begin
if index < Size then
  begin
  if (count < 0) or (index + count >= Size) then
    count := Size - index
  else if (count = 0) then
    begin
    result := '';
    exit;
    end;
  result := PtrToStr(@Data[index], count)
  end
else
  result := '';  
end;

function TBufferData.FragmentsToString( ) : String;
var
  i : integer;
begin
if fFragmentCount > 0 then
  begin
  result := '';
  for i := 0 to fFragmentCount -1 do
    result := result + fFragments[i].ToString();
  end
else
  result := ToString( );
end;

Function TBufferData.AllocFragment( StartIndex, aSize : integer) : TBufferData;

  Procedure Report;
  begin
  Tlog.Add( 3, 'TBufferData.AllocFragment, increasing fragment buffer size from ' +
               IntToStr(fFragmentCount) +' to '+IntToStr(InitialBufferFragmentCount) );
  end;
begin
if fFragmentCount >= length(fFragments) then
  begin
  if fFragmentCount <> 0 then
    begin
    InitialBufferFragmentCount := (fFragmentCount * 3) div 2 - 1;
    if Log_.VerboseLoggingLevel >= 3 then
      Report();
    end;
  setlength( fFragments, InitialBufferFragmentCount );  
  end;
result := BufferedData.NewBuffer(false);
inc( BufferedData.fAllocatedFragments );
result.MakeFragment( DataPtr,fFragmentIndex+fOffsetIndex+ StartIndex, aSize);
fFragments[fFragmentCount] := result;
inc(fFragmentCount);
end;


Procedure TBufferData.DiscardFragments( );
begin
if fFragments <> nil then
while fFragmentCount > 0 do
  begin
  dec(fFragmentCount);
  BufferedData.DiscardBuffer( fFragments[fFragmentCount] );
  fFragments[fFragmentCount] := nil;
  end;
end;

Procedure TBufferData.CompileFragments( DestBuffer : TBufferData );
var
  i : integer;
  TotalSize : integer;
begin
if (fFragments <> nil) and (fFragmentCount > 0) then
  begin
  assert(DestBuffer <> nil);
//  WriteOffset := TBufferData(fFragments[0]).fFragmentIndex;
  TotalSize := 0;
  for i := fFragmentCount-1 downto 0 do
    inc( TotalSize, fFragments[i].Size );
  DestBuffer.HeaderState := hs_RawData;
  DestBuffer.CheckGrowth( TotalSize );
  //  if WriteOffset <> 0 then
//    DestBuffer.AddBuffer( self, 0, WriteOffset );
  while (fFragmentCount > 0) do
    begin
    dec(fFragmentCount);
    DestBuffer.AddBuffer( fFragments[fFragmentCount] );
    BufferedData.DiscardBuffer( fFragments[fFragmentCount] );
    fFragments[fFragmentCount] := nil;
    end;
  end;
end;

Procedure TBufferData.CompileFragments( );
var
  DestBuffer : TBufferData;
begin
if (fFragments <> nil) and (fFragmentCount > 0) then
  begin
  DestBuffer := BufferedData.NewBuffer;

  CompileFragments( DestBuffer );

  BufferedData.SwapBuffers;
  BufferedData.ReleaseBuffer;
  end
end;

// -----------------------------------------------------------------------------

Constructor TBufferedData.Create;
begin
inherited;
if InitialBufferFragmentCount <= 0 then
  InitialBufferFragmentCount := DefaultBufferFragmentCount;
if InitialBufferCount <= 0 then
  InitialBufferCount := DefaultBufferCount;
if InitialBufferSize <= 0 then
  InitialBufferSize := DefaultBufferSize;
fReleaseList := TStack.create(InitialBufferCount);
fFreeList := TStack.create(InitialBufferCount);
// allocate the 1st buffer
NewBuffer;
end;

Destructor TBufferedData.Destroy;
begin
if fReleaseList <> nil then
  begin
  while fReleaseList.Count > 0 do
    TObject(fReleaseList.Pop).Free;
  FreeAndNil(fReleaseList);
  end;  
if fFreeList <> nil then
  begin
  while fFreeList.Count > 0 do
    TObject(fFreeList.Pop).Free;
  FreeAndNil(fFreeList);  
  end;  
FreeAndNil( fcurrent );
fcurrent := nil;
inherited;
end;

Procedure TBufferedData.SwapBuffers;
var
  AlmostTop : TBufferData;
begin
assert( fReleaseList <> nil );
assert( fFreeList <> nil );
assert( fReleaseList.Count >= 1 );
AlmostTop := fReleaseList.Pop;
fReleaseList.Push( fCurrent );
fCurrent := AlmostTop;
end;

Function TBufferedData.NewBuffer : TBufferData;
begin
result := NewBuffer(true);
end;

Function TBufferedData.NewBuffer(AddToReleaseList : boolean) : TBufferData;

  var
    i : integer;
    oldBufferCount : integer;
  Procedure Report();
  begin
  Tlog.Add( 3, 'TBufferedData.NewBuffer, increasing initial buffer count from ' +
               IntToStr(InitialBufferCount) +' to '+IntToStr(fBufferCount) );
  end;

{$IFDEF SanityAllocCheck}
var
  ActualCount : integer;
{$ENDIF}
begin
{$IFDEF SanityAllocCheck}
// sanity check to make sure we arent leaking objects somewhere
ActualCount := fReleaseList.Count + fFreeList.Count + fAllocatedFragments;
if fCurrent <> nil then
  inc(ActualCount);
if fBufferCount <> ActualCount then
  asm int 3 end;
{$ENDIF}

if AddToReleaseList and (fCurrent <> nil) then
  begin
  fReleaseList.Push( fCurrent );
  fCurrent := nil;
  end;
if fFreeList.Count <= 0 then
  begin
  oldBufferCount := BufferCount;
  if BufferCount > 0 then
    begin
    // grow the list by ~50%
    fBufferCount := (InitialBufferCount * 3) div 2 - 1;
    if Log_.VerboseLoggingLevel >= 3 then
       Report;
    InitialBufferCount := fBufferCount;
    end
  else
    fBufferCount := InitialBufferCount; // use the inital buffer count
  fFreeList.Grow( fBufferCount );
  fReleaseList.Grow( fBufferCount );  
  for i := oldBufferCount to BufferCount-1 do
    fFreeList.Push( TBufferData.Create( self ) );    
  end;
result := fFreeList.Pop();
result.Reset();
if AddToReleaseList then
  fCurrent := result;
end;

Function TBufferedData.GetPendingReleaseCount : integer;
begin
result := fReleaseList.Count;
end;

Function TBufferedData.PrevBuffer : TBufferData;
begin
result := fReleaseList.Examine;
end;

Procedure TBufferedData.ReleaseBuffer;
begin
assert( fReleaseList <> nil );
If fReleaseList.Count >= 1 then
  begin
  DiscardBuffer( fCurrent );
  fCurrent := fReleaseList.Pop;
  end;
end;

Procedure TBufferedData.DiscardBuffer( BufferData : TBufferData );
begin
assert( fFreeList <> nil );
if BufferData <> nil then
  begin
{$IFDEF SanityAllocCheck}
  if BufferData.WasFragment then
    begin
    BufferData.WasFragment := false;
    dec( fAllocatedFragments );
    end;
{$ENDIF}    
  fFreeList.Push( BufferData );
  if fCurrent.fFragmentCount > 0 then
    BufferData.DiscardFragments();
  end;
end;


end.

