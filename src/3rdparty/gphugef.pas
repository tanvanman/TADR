{$B-,H+,J+,Q-,T-,X+}

{$UNDEF D3PLUS}
{$UNDEF D4PLUS}
{$IFDEF VER100}{$DEFINE D3PLUS}{$ENDIF}
{$IFDEF VER120}{$DEFINE D3PLUS}{$DEFINE D4PLUS}{$ENDIF}
{$IFDEF VER130}{$DEFINE D3PLUS}{$DEFINE D4PLUS}{$ENDIF}
{$IFDEF VER140}{$DEFINE D3PLUS}{$DEFINE D4PLUS}{$DEFINE D6PLUS}{$ENDIF}
{$IFDEF VER150}{$DEFINE D3PLUS}{$DEFINE D4PLUS}{$DEFINE D6PLUS}{$DEFINE D7PLUS}{$ENDIF}

(*:Interface to 64-bit file functions with some added functionality.
   @author Primoz Gabrijelcic
   @desc <pre>

This software is distributed under the BSD license.

Copyright (c) 2003, Primoz Gabrijelcic
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
- The name of the Primoz Gabrijelcic may not be used to endorse or promote
  products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   Author           : Primoz Gabrijelcic
   Creation date    : 1998-09-15
   Last modification: 2003-05-13
   Version          : 3.10
</pre>*)(*
   History:
     3.10: 2003-05-14
       - Compatible with Delphi 7.

     3.09a: 2003-02-12
       - Faster TGpHugeFile.EOF.

     3.09: 2003-02-12
       - EOF function added.
       - Seek in buffered write mode was not working. Fixed.

     3.08a: 2002-10-14
       - TGpHugeFileStream.Create failed when append mode was used and file did
         not exist. Fixed.

     3.08: 2002-04-24
       - File handle exposed through the Handle property.
       - Added THFOpenOption hfoCompressed.
       
     3.07a: 2001-12-15
       - Updated to compile with Delphi 6.

     3.07: 2001-07-02
       - Added TGpHugeFile.FileDate setter.

     3.06b: 2001-06-27
       - TGpHugeFile.FileSize function was returning wrong result when file
         was open for buffered write access.

     3.06a: 2001-06-24
       - Modified CreateEx behaviour - if DesiredShareMode is not set and
         file is open in GENERIC_READ mode, sharing will be set to
         FILE_SHARE_READ.

     3.06: 2001-06-22
       - Added parameter DesiredShareMode to the CreateEx constructor.

     3.05: 2001-02-27
       - Modified Reset and Rewrite methods to open file in buffered mode by
         default.

     3.04: 2001-01-31
       - All raised exceptions now have HelpContext set. All possible
         HelpContext values are enumerated in 'const' section at the very
         beginning of the unit. Thanks to Peter Evans for the suggestion.

     3.03: 2000-10-18
       - Fixed bugs in hfoCloseOnEOF support in TGpHugeFile.

     3.02: 2000-10-12
       - Fixed bugs in hfoCloseOnEOF support in TGpHugeFileStream.

     3.01: 2000-10-06
       - TGpHugeFileStream constructor now accepts THFOpenOptions parameter,
         which is passed to TGpHugeFile ResetEx/RewriteEx. Default open mode for
         stream files is now hfoBuffered.
       - TGpHugeFileStream constructor parameters are simpler -
         FlagsAndAttributes and DesiredAccess parameters are no longer present.
       - Added TGpHugeFileStream.CreateFromHandle constructor accepting instance
         of TGpHugeFile, which is then used for all stream access. This
         TGpHugeFile instance must already be created and open (Reset, Rewrite).
         It will not be destroyed in TGpHugeFileStream destructor.
       - Added read-only property TGpHugeFileStream.FileName.
       - Added read-only property TGpHugeFileStream.WindowsError.
       - Fully documented.
       - All language-dependant string constants moved to resourcestring
         section.

     3.0: 2000-10-03
       - Created TGpHugeFileStream - descendant of TStream that wraps
         TGpHugeFile. Although it does not support huge files fully (because of
         TStream limitations), you could still use it as a buffered file stream.

     2.33: 2000-09-04
       - TGpHugeFile now exposes WindowsError property, which is set to last
         Windows error wherever it is checked for.

     2.32: 2000-08-01
       - All raised exceptions converted to EGpHugeFile exceptions.
       - All windows exceptions are now caught and converted to EGpHugeFile
         exceptions.
       - If file is open in read buffered mode *and* is then seeked past EOF
         (Seek(FileSize)) *and* is then written into, it will switch to write
         buffered mode (previous versions of GpHugeFile raised exception under
         those conditions).

     2.31: 2000-05-15
       - Call to Truncate is now allowed in buffered write mode. It will cause
         buffer to be flushed, though.

     2.30a: 2000-05-15
       - Fix introduced in 2.29a sometimes caused BlockRead to return error even
         when there was some data present. This only happened when file was open
         for reading (via Reset) and then extended with BlockWrite.

     2.30: 2000-05-12
       - New property: IsBuffered. Returns true if file is open in buffered
         mode.

     2.29a: 2000-05-02
       - While reading near end of (buffered) file, ReadFile API was called much
         too often. Fixed.

     2.29: 2000-04-14
       - Added new ResetEx/RewriteEx parameter - waitObject. If not equal to
         zero, TGpHugeFile will check it periodically in the wait loop. If
         object becomes signalled, TGpHugeFile will stop trying to open the file
         and will return an error.

     2.28: 2000-04-12
       - Added new THFOpenOption: hfoCanCreate. Set it to allow ResetEx to
         create file when it does not exist.

     2.27: 2000-04-02
       - Added property FileDate.

     2.26a: 2000-03-07
       - Fixed bug in hfoCloseOnEOF processing.

     2.26: 2000-03-03
       - Added THFOpenOption hfoCloseOnEOF. If specified in a call to ResetEx
         TGpHugeFile will close file handle as soon as last block is read from
         the file. This will free file for other programs while main program may
         still read data from TGpHugeFile's buffer. {*}
         After the end of file is reached (and handle is closed):
           - FilePos may be used.
           - FileSize may be used.
           - Seek and BlockRead may be used as long as the request can be
             fulfilled from the buffer.
         Use of this option is not recommended when access to the file is
         random. {*} It was designed to use with sequential access to the file.
         hfoCloseOnEOF is ignored if hfoBuffered is not set.
         hfoCloseOnEOF is ignored if used in RewriteEx.

         {*} hfoCloseOnEOF can cope with a program that alternately calls
         BlockRead and Seek requests. When BlockRead reaches EOF, this condition
         will be marked but file handle will not be closed yet. Only when
         BlockRead is called again, file will be closed, but only if between
         those calls Seek did not invalidate the buffer (Seek that can be
         fulfilled from the buffer is OK). This works with programs that load a
         small buffer and then Seek somewhere in the middle of this buffer (like
         Readln function in TGpTextFile class).

     2.25a: 2000-02-19
       - Fixed bug where TGpHugeFile.Reset would create a file if file did not
         exist before. Thanks to Peter Evans for finding the bug and solution. 

     2.25: 1999-12-29
       - Changed implementation of TGpHugeFile.ResetEx and TGpHugeFile.RewriteEx
         (called from all Reset* and Rewrite* functions). Before the change,
         they were closing and reopening the file - not a very good idea if you
         share a file between applications.

     2.24e: 1999-12-22
       - Fixed broken TGpHugeFile.IsOpen. Thanks for Phil Hodgson for finding
         this bug.

     2.24d: 1999-11-22
       - Fixed small problem in file access routines. They would continue trying
         to access a file event if returned error was not sharing or locking
         error.

     2.24c: 1999-11-20
       - Behaviour changed. If you open file with GENERIC_READ access, sharing
         mode will be set to FILE_SHARE_READ. 2.24b and older set sharing mode
         to 0 in all occasions.  

     2.24b: 1999-11-06
       - Added (again) ResetBuffered and RewriteBuffered;

     2.24a: 1999-11-03
       - Fixed Reset and Rewrite.

     2.24: 1999-11-02
       - ResetBuffered and RewriteBuffered renamed to ResetEx and RewriteEx.
       - Parameters diskLockTimeout and diskRetryDelay added to ResetEx and
         RewriteEx.

     2.23: 1999-10-28
       - Compiles with D5.

     2.22: 1999-06-14
       - Better error reporting.

     2.21: 1998-12-21
       - Better error checking.

     2.2: 1998-12-14
       - New function IsOpen.
       - Lots of SetLastError(0) calls added.

     2.12: 1998-10-28
       - CreateEx enhanced.

     2.11: 1998-10-14
       - Error reporting in Block*Unsafe enhanced.

     2.1: 1998-10-13
       - FilePos works in buffered mode.
       - Faster FilePos in unbuffered mode.
       - Seek works in read buffered mode.
         - In FILE_FLAG_NO_BUFFERING mode Seek works only when offset is on a
           sector boundary.
         - Truncate works in read buffered mode (untested).
         - Dependance on MSString removed.

     2.0: 1998-10-08
       - Win32 API error checking.
       - Sequential access buffering (ResetBuffered, RewriteBuffered).
       - Buffered files can be safely accessed in FILE_FLAG_NO_BUFFERING mode.
       - New procedures BlockReadUnsafe, BlockWriteUnsafe.

     1.1: 1998-10-05
       - CreateEx constructor added.
         - can specify attributes (for example FILE_FLAG_SEQUENTIAL_SCAN)
       - D4 compatible.

     1.0: 1998-09-15
       - First published version.
*)

// TODO 4 -oPrimoz Gabrijelcic: Change EGpHugeFile to CreateFmtHelp

// removes virtual function declarations for preformance
{$DEFINE NoVirtuals}
unit GPHugeF;

interface

uses
  SysUtils,
  Windows,
  Classes;

// HelpContext values for all raised exceptions
const
  //:Exception was handled and converted to EGpHugeFile but was not expected and is not categorised.
  hcHFUnexpected              = 1000;
  //:Windows error.
  hcHFWindowsError            = 1001;
  //:Unknown Windows error.
  hcHFUnknownWindowsError     = 1002;
  //:Invalid block size.
  hcHFInvalidBlockSize        = 1003;
  //:Invalid file handle.
  hcHFInvalidHandle           = 1004;
  //:Failed to allocate buffer.
  hcHFFailedToAllocateBuffer  = 1005;
  //:Write operation encountered while in buffered read mode.
  hcHFWriteInBufferedReadMode = 1006;
  //:Read operation encountered while in buffered write mode.
  hcHFReadInBufferedWriteMode = 1007;
  //:Unexpected end of file.
  hcHFUnexpectedEOF           = 1008;
  //:Write failed - not all data was saved.
  hcHFWriteFailed             = 1009;
  //:Invalid 'mode' parameter passed to Seek function.
  hcHFInvalidSeekMode         = 1010;

type
  {:Alias for int64 so it is Delphi-version-independent (as much as that is
    possible at all).
  }
  HugeInt = LONGLONG;

  {:Base exception class for all exceptions raised in TGpHugeFile and
    descendants.
  }
  EGpHugeFile       = class(Exception);

  {:Base exception class for exceptions created in TGpHugeFileStream.
  }
  EGpHugeFileStream = class(EGpHugeFile);

  {:Result of TGpHugeFile reset and rewrite methods.
    @enum hfOK         File opened successfully.
    @enum hfFileLocked Access to file failed because it is already open and
                       compatible sharing is not allowed.
    @enum hfError      Other file access errors (file/path not found...).
   }
  THFError = (hfOK, hfFileLocked, hfError);

  {:TGpHugeFile reset/rewrite options.
    @enum hfoBuffered   Open file in buffered mode. Buffer size is either
                        default (BUF_SIZE, currently 64 KB) or specified by the
                        caller in ResetEx or RewriteEx methods.
    @enum hfoLockBuffer Buffer must be locked (Windows require that for direct
                        access files (FILE_FLAG_NO_BUFFERING) to work
                        correctly).
    @enum hfoCloseOnEOF Valid only when file is open for reading. If set,
                        TGpHugeFile will close file handle as soon as last block
                        is read from the file. This will free file for other
                        programs while main program may still read data from
                        TGpHugeFile's buffer. (*)                                <br>
                        After the end of file is reached (and handle is closed): <ul><li>
                          FilePos may be used.                                   </li><li>
                          FileSize may be used.                                  </li><li>
                          Seek and BlockRead may be used as long as the request
                          can be fulfilled from the buffer.                      </li></ul><br>
                        Use of this option is not recommended when access to the
                        file is random. (*) It was designed to use with
                        sequential or almost sequential access to the file.
                        hfoCloseOnEOF is ignored if hfoBuffered is not set.
                        hfoCloseOnEOF is ignored if used in RewriteEx.           <br>
                        (*) hfoCloseOnEOF can cope with a program that
                        alternately calls BlockRead and Seek requests. When
                        BlockRead reaches EOF, this condition will be marked but
                        file handle will not be closed yet. When BlockRead is
                        called again, file will be closed, but only if between
                        those calls Seek did not invalidate the buffer (Seek
                        that can be fulfilled from the buffer is OK). This works
                        with programs that load a small buffer and then Seek
                        somewhere in the middle of this buffer (like Readln
                        function in TGpTextFile class does).
    @enum hfoCanCreate  Reset is allowed to create a file if it doesn't exist.   <br>
    @enum hfoCompressed Valid only when file is opened for writing. Will try
                        to set the "compressed" attribute (when running on NT
                        and file is on NTFS drive).
  }
  THFOpenOption  = (hfoBuffered, hfoLockBuffer, hfoCloseOnEOF, hfoCanCreate,
                    hfoCompressed);

  {:Set of all TGpHugeFile reset/rewrite options.
  }
  THFOpenOptions = set of THFOpenOption;

  {:Encapsulation of 64-bit file functions, supporting normal, buffered, and
    direct access with some additional twists.
  }
  TGpHugeFile = class
  private
    hfBlockSize       : DWORD;
    hfBuffer          : pointer;
    hfBuffered        : boolean;
    hfBufferSize      : DWORD;
    hfBufFileOffs     : HugeInt;
    hfBufFilePos      : HugeInt;
    hfBufOffs         : DWORD;
    hfBufSize         : DWORD;
    hfBufWrite        : boolean;
    hfCachedSize      : HugeInt;
    hfCanCreate       : boolean;
    hfCloseOnEOF      : boolean;
    hfCloseOnNext     : boolean;
    hfCompressed      : boolean;
    hfDesiredAcc      : DWORD;
    hfDesiredShareMode: DWORD;
    hfFlagNoBuf       : boolean;
    hfFlags           : DWORD;
    hfHalfClosed      : boolean;
    hfHandle          : THandle;
    hfIsOpen          : boolean;
    hfLastSize        : integer;
    hfLockBuffer      : boolean;
    hfName            : string;
    hfReading         : boolean;
    hfShareModeSet    : boolean;
    hfWindowsError    : DWORD;
  protected
    function  _FilePos: HugeInt; {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    procedure _Seek(offset: HugeInt; movePointer: boolean); {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    function  AccessFile(blockSize: integer; reset: boolean;
      diskLockTimeout: integer; diskRetryDelay: integer;
      waitObject: THandle): THFError; {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    procedure AllocBuffer; {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    procedure CheckHandle(); {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    procedure Fetch(var buf; count: DWORD; var transferred: DWORD); {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    function  FlushBuffer: boolean; {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    procedure FreeBuffer; {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    function  GetDate: TDateTime; {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    procedure InitReadBuffer; {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    procedure InitWriteBuffer; {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    function  LoadedToTheEOF: boolean; {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    function  RoundToPageSize(bufSize: DWORD): DWORD; {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    procedure SetDate(const Value: TDateTime); {$IFNDEF NoVirtuals} virtual; {$ENDIF}
    procedure Transmit(const buf; count: DWORD; var transferred: DWORD); {$IFNDEF NoVirtuals} virtual; {$ENDIF}
  public
    procedure Win32Check(condition: boolean; const method: string); {$IFNDEF NoVirtuals} virtual; {$ENDIF}
  public
    constructor Create(const fileName: string);
    constructor CreateEx(const fileName: string;
      FlagsAndAttributes: DWORD {$IFDEF D4plus}= FILE_ATTRIBUTE_NORMAL{$ENDIF};
      DesiredAccess: DWORD      {$IFDEF D4plus}= GENERIC_READ+GENERIC_WRITE{$ENDIF};
      DesiredShareMode: DWORD   {$IFDEF D4plus}= $FFFF{$ENDIF});
    procedure   Reset(blockSize: integer {$IFDEF D4plus}= 1{$ENDIF});
    procedure   Rewrite(blockSize: integer {$IFDEF D4plus}= 1{$ENDIF});
    procedure   ResetBuffered(
      blockSize: integer  {$IFDEF D4plus}= 1{$ENDIF};
      bufferSize: integer {$IFDEF D4plus}= 0{$ENDIF};
      lockBuffer: boolean {$IFDEF D4plus}= false{$ENDIF});
    procedure   RewriteBuffered(
      blockSize: integer  {$IFDEF D4plus}= 1{$ENDIF};
      bufferSize: integer {$IFDEF D4plus}= 0{$ENDIF};
      lockBuffer: boolean {$IFDEF D4plus}= false{$ENDIF});
    function    ResetEx(
      blockSize: integer       {$IFDEF D4plus}= 1{$ENDIF};
      bufferSize: integer      {$IFDEF D4plus}= 0{$ENDIF};
      diskLockTimeout: integer {$IFDEF D4plus}= 0{$ENDIF};
      diskRetryDelay: integer  {$IFDEF D4plus}= 0{$ENDIF};
      options: THFOpenOptions  {$IFDEF D4plus}= []{$ENDIF};
      waitObject: THandle      {$IFDEF D4plus}= 0{$ENDIF}): THFError;
    function    RewriteEx(
      blockSize: integer       {$IFDEF D4plus}= 1{$ENDIF};
      bufferSize: integer      {$IFDEF D4plus}= 0{$ENDIF};
      diskLockTimeout: integer {$IFDEF D4plus}= 0{$ENDIF};
      diskRetryDelay: integer  {$IFDEF D4plus}= 0{$ENDIF};
      options: THFOpenOptions  {$IFDEF D4plus}= []{$ENDIF};
      waitObject: THandle      {$IFDEF D4plus}= 0{$ENDIF}): THFError;
    destructor  Destroy; override;
    procedure BlockRead(var buf; count: DWORD; var transferred: DWORD);
    procedure BlockReadUnsafe(var buf; count: DWORD);
    procedure BlockWrite(const buf; count: DWORD; var transferred: DWORD);
    procedure BlockWriteUnsafe(const buf; count: DWORD);
    procedure Close;
    function  EOF: boolean;
    function  FileExists: boolean;
    function  FilePos: HugeInt;

    procedure Flush;
    function  IsOpen: boolean;
    procedure Seek(offset: HugeInt);
    procedure Truncate;


    Function FileSize : HugeInt;
    function FileSizeNoCache : HugeInt; {$IFNDEF NoVirtuals} virtual; {$ENDIF}

    //:File date/time.
    property FileDate: TDateTime read GetDate write SetDate;
    //:File name.
    property FileName: string read hfName;
    //:True if access to file is buffered.
    property IsBuffered: boolean read hfBuffered;
    //:File handle.
    property Handle: THandle read hfHandle;
    //:Last Windows error code.
    property WindowsError: DWORD read hfWindowsError;
  end; { TGpHugeFile }

  {:All possible ways to access TGpHugeFileStream.
    @enum accRead      Read access.
    @enum accWrite     Write access.
    @enum accReadWrite Read and write access.
    @enum accAppend    Same as accReadWrite, just that Position is set
                       immediatly after the end of file.
  }
  TGpHugeFileStreamAccess = (accRead, accWrite, accReadWrite, accAppend);

  {:TStream descendant, wrapping a TGpHugeFile. Although it does not support
    huge files fully (because of TStream limitations - 'longint' is used instead
    of 'int64' in critical places), you can still use it as a buffered file
    stream.
  }
  TGpHugeFileStream = class(TStream)
  private
    hfsExternalHF  : boolean;
    hfsFile        : TGpHugeFile;
    hfsWindowsError: DWORD;
  protected
    function  GetFileName: string; virtual;
    function  GetWindowsError: DWORD; virtual;
    procedure SetSize(newSize: longint); override;
    procedure Win32Check(condition: boolean; const method: string); virtual;
    {$IFDEF D7PLUS}
    function  GetSize: int64; override;
    procedure SetSize(const newSize: int64); overload; override;
    procedure SetSize64(const newSize: int64); 
    {$ELSE}
    function  GetSize: longint; virtual;
    {$ENDIF D7PLUS}
  public
    constructor Create(const fileName: string; access: TGpHugeFileStreamAccess;
      openOptions: THFOpenOptions {$IFDEF D4plus}= [hfoBuffered]{$ENDIF});
    constructor CreateFromHandle(hf: TGpHugeFile); 
    destructor  Destroy; override;
    function  Read(var buffer; count: longint): longint; override;
    function  Seek(offset: longint; mode: word): longint; {$IFDEF D7PLUS}overload;{$ENDIF D7PLUS} override;
    {$IFDEF D7PLUS}
    function  Seek(const offset: Int64; origin: TSeekOrigin): int64; overload; override;
    {$ENDIF D7PLUS}
    function  Write(const buffer; count: longint): longint; override;
    //:Name of underlying file.
    property FileName: string read GetFileName;
    //:Stream size. Reintroduced to override GetSize (static in TStream) with faster version.
    {$IFDEF D7PLUS}
    property Size: int64 read GetSize write SetSize64;
    {$ELSE}
    property Size: longint read GetSize write SetSize;
    {$ENDIF D7PLUS}
    //:Last Windows error code.
    property WindowsError: DWORD read GetWindowsError;
  end; { TGpHugeFileStream }

const
  {:Default buffer size. 64 KB, small enough to be VirtualLock'd in NT 4
  }
  BUF_SIZE = 64*1024;

const
  sBlockSizeMustBeGreaterThanZero = 'TGpHugeFile(%s):BlockSize must be greater than zero!';
  sFailedToAllocateBuffer         = 'TGpHugeFile(%s):Failed to allocate buffer!';
  sFileNotOpen                    = 'TGpHugeFile(%s):File not open!';
  sInvalidMode                    = 'TGpHugeFileStream(%s):Invalid mode!';
  sReadWhileInBufferedWriteMode   = 'TGpHugeFile(%s):Read while in buffered write mode!';
  sFileFailed                     = 'TGpHugeFile.%s(%s) failed. ';
  sStreamFailed                   = 'TGpHugeFileStream.%s(%s) failed. ';
  sWriteFailed                    = 'TGpHugeFile(%s):Write failed!';
  sWriteWhileInBufferedReadMode   = 'TGpHugeFile(%s):Write while in buffered read mode!';

implementation

uses
  SysConst;

{$IFDEF D4plus}
type
  {:D4 and newer define TLargeInteger as int64.
  }
  TLargeInteger = LARGE_INTEGER;
{$ENDIF}

const
  COMPRESSION_FORMAT_DEFAULT = 1;
  FILE_DEVICE_FILE_SYSTEM    = 9;
  METHOD_BUFFERED            = 0;
  FILE_READ_DATA             = 1;
  FILE_WRITE_DATA            = 2;
  FSCTL_SET_COMPRESSION = (FILE_DEVICE_FILE_SYSTEM shl 16) OR
                          ((FILE_READ_DATA OR FILE_WRITE_DATA) shl 14) OR
                          (16 shl 2) OR
                          METHOD_BUFFERED;
  COMPRESSION_FORMAT_NONE    = 0;

function Compress(const fileName: string; fileHandle: THandle): boolean;
var
  comp            : SHORT;
  isFileCompressed: boolean;
  res             : DWORD;
begin
  Result := true;
  if Win32Platform = VER_PLATFORM_WIN32_NT then begin { only NT can compress files }
    isFileCompressed := (GetFileAttributes(PChar(fileName)) AND
      FILE_ATTRIBUTE_COMPRESSED) = FILE_ATTRIBUTE_COMPRESSED;
    if not isFileCompressed then begin
      res := 0;
      comp := COMPRESSION_FORMAT_DEFAULT;
      Result := DeviceIoControl (fileHandle, FSCTL_SET_COMPRESSION, @comp,
        SizeOf(SHORT), nil, 0, res, nil);
    end;
  end;
end; { CompressUncompress }

{ TGpHugeFile }

{:Standard TGpHugeFile constructor. Prepares file for full, share none, access.
  @param   fileName Name of file to be accessed.
}
constructor TGpHugeFile.Create(const fileName: string);
begin
  CreateEx(fileName,FILE_ATTRIBUTE_NORMAL,GENERIC_READ+GENERIC_WRITE,0);
  hfShareModeSet := false;
end; { TGpHugeFile.Create }

{:Extended TGpHugeFile constructor. Caller can specify desired flags,
  attributes, and access mode.
  @param   fileName           Name of file to be accessed.
  @param   FlagsAndAttributes Flags and attributes, see CreateFile help for more
                              details.
  @param   DesiredAccess      Desired access flags, see CreateFile help for more
                              details.
}
constructor TGpHugeFile.CreateEx(const fileName: string; FlagsAndAttributes,
  DesiredAccess, DesiredShareMode: DWORD);
begin
  inherited Create;
  hfBlockSize        := 1;
  hfBuffer           := nil;
  hfBuffered         := false;
  hfCachedSize       := -1;
  hfDesiredAcc       := DesiredAccess;
  hfDesiredShareMode := DesiredShareMode;
  hfShareModeSet     := true;
  hfFlagNoBuf        := ((FILE_FLAG_NO_BUFFERING AND FlagsAndAttributes) <> 0);
  hfFlags            := FlagsAndAttributes;
  hfHandle           := INVALID_HANDLE_VALUE;
  hfName             := fileName;
end; { TGpHugeFile.CreateEx }

{:TGpHugeFile destructor. Will close file if it is still open.
}
destructor TGpHugeFile.Destroy;
begin
  Close;
  inherited Destroy;
end; { TGpHugeFile.Destroy }

{:Tests if a specified file exists.
  @returns True if file exists.
}
function TGpHugeFile.FileExists: boolean;
begin
  FileExists := SysUtils.FileExists(hfName);
end; { TGpHugeFile.FileExists }

{:Opens/creates a file. AccessFile centralizes file opening in TGpHugeFile. It
  will set appropriate sharing mode, open or create a file, and even retry in
  a case of locked file (if so required).
  @param   blockSize       Basic unit of access (same as RecSize parameter in
                           Delphi's Reset and Rewrite).
  @param   reset           True if file is to be reset, false if it is to be
                           rewritten.
  @param   diskLockTimeout Max time (in milliseconds) AccessFile will wait for
                           lock file to become free.
  @param   diskRetryDelay  Delay (in milliseconds) between attempts to open
                           locked file.
  @param   waitObject      Handle of 'terminate' event (semaphore, mutex). If
                           this parameter is specified (not zero) and becomes
                           signalled, AccessFile will stop trying to open locked
                           file and will exit with.
  @returns Status (ok, file locked, other error).
  @raises  EGpHugeFile if 'blockSize' is less or equal to zero.
  @seeAlso ResetEx, RewriteEx
}
function TGpHugeFile.AccessFile(blockSize: integer; reset: boolean;
  diskLockTimeout: integer; diskRetryDelay: integer;
  waitObject: THandle): THFError;
var
  start: int64;

  function Elapsed: boolean;
  var
    stop: int64;
  begin
    if diskLockTimeout = 0 then
      Result := true
    else begin
      stop := GetTickCount;
      if stop < start then
        stop := stop + $100000000;
      Result := ((stop-start) > diskLockTimeout);
    end;
  end; { Elapsed }

const
  FILE_SHARING_ERRORS: set of byte = [ERROR_SHARING_VIOLATION, ERROR_LOCK_VIOLATION];

var
  awaited  : boolean;
  creat    : DWORD;
  shareMode: DWORD;

begin { TGpHugeFile.AccessFile }
  if blockSize <= 0 then
    raise EGpHugeFile.CreateFmtHelp(sBlockSizeMustBeGreaterThanZero,[FileName],hcHFInvalidBlockSize);
  hfBlockSize := blockSize;
  start := GetTickCount;
  repeat
    if reset then begin
      if hfCanCreate then
        creat := OPEN_ALWAYS
      else
        creat := OPEN_EXISTING;
    end
    else
      creat := CREATE_ALWAYS;
    SetLastError(0);
    hfWindowsError := 0;
    if hfShareModeSet then begin
      if hfDesiredShareMode = $FFFF then begin
        if hfDesiredAcc = GENERIC_READ then
          shareMode := FILE_SHARE_READ
        else
          shareMode := 0
      end
      else
        shareMode := hfDesiredShareMode
    end
    else begin
      if hfDesiredAcc = GENERIC_READ then
        shareMode := FILE_SHARE_READ
      else
        shareMode := 0;
    end;
    hfHandle := CreateFile(PChar(hfName),hfDesiredAcc,shareMode,nil,creat,hfFlags,0);
    awaited := false;
    if hfHandle = INVALID_HANDLE_VALUE then begin
      hfWindowsError := GetLastError; 
      if (hfWindowsError in FILE_SHARING_ERRORS) and (diskRetryDelay > 0) and (not Elapsed) then
        if waitObject <> 0 then
          awaited := WaitForSingleObject(waitObject, diskRetryDelay) <> WAIT_TIMEOUT
        else
          Sleep(diskRetryDelay);
    end
    else begin
      hfWindowsError := 0;
      hfIsOpen := true;
    end;
  until (hfWindowsError = 0) or (not (hfWindowsError in FILE_SHARING_ERRORS)) or Elapsed or awaited;
  if (hfWindowsError = 0) and hfCompressed then begin
    if not Compress(hfName, hfHandle) then
      hfWindowsError := GetLastError;
  end;
  if hfWindowsError = 0 then begin
    Result := hfOK;
  end
  else if hfWindowsError in FILE_SHARING_ERRORS then
    Result := hfFileLocked
  else
    Result := hfError;
  if Result = hfOK then
    begin
    AllocBuffer;
    hfCachedSize := FileSizeNoCache;
    end;
end; { TGpHugeFile.AccessFile }

{:Simplest form of Reset, emulating Delphi's Reset.
  @param   blockSize Basic unit of access (same as RecSize parameter in Delphi's
                     Reset and Rewrite).
  @raises  EGpHugeFile if file could not be opened.
}
procedure TGpHugeFile.Reset(blockSize: integer);
begin
  Win32Check(ResetEx(blockSize,0,0,0,[hfoBuffered]) = hfOK,'Reset');
end; { TGpHugeFile.Reset }

{:Simplest form of Rewrite, emulating Delphi's Rewrite.
  @param   blockSize       Basic unit of access (same as RecSize parameter in
                           Delphi's Rewrite).
  @raises  EGpHugeFile if file could not be opened.
}
procedure TGpHugeFile.Rewrite(blockSize: integer);
begin
  Win32Check(RewriteEx(blockSize,0,0,0,[hfoBuffered]) = hfOK,'Rewrite');
end; { TGpHugeFile.Rewrite }

{:Buffered Reset. Caller can specifiy size of buffer and require that buffer is
  locked in memory (Windows require that for direct access files
  (FILE_FLAG_NO_BUFFERING) to work correctly).
  @param   blockSize  Basic unit of access (same as RecSize parameter in
                      Delphi's Reset).
  @param   bufferSize Size of buffer. 0 means default size (BUF_SIZE, currently
                      64 KB).
  @param   lockBuffer If true, buffer will be locked.
  @raises  EGpHugeFile if file could not be opened.
  @seeAlso BUF_SIZE
}
procedure TGpHugeFile.ResetBuffered(blockSize, bufferSize: integer;
  lockBuffer: boolean);
var
  options: THFOpenOptions;
begin
  options := [hfoBuffered];
  if lockBuffer then
    Include(options,hfoLockBuffer);
  Win32Check(ResetEx(blockSize,bufferSize,0,0,options) = hfOK,'ResetBuffered');
end; { TGpHugeFile.ResetBuffered }

{:Buffered Rewrite. Caller can specifiy size of buffer and require that buffer
  is locked in memory (Windows require that for direct access files
  (FILE_FLAG_NO_BUFFERING) to work correctly).
  @param   blockSize  Basic unit of access (same as RecSize parameter in
                      Delphi's Rewrite).
  @param   bufferSize Size of buffer. 0 means default size (BUF_SIZE, currently
                      64 KB).
  @param   lockBuffer If true, buffer will be locked.
  @raises  EGpHugeFile if file could not be opened.
  @seeAlso BUF_SIZE
}
procedure TGpHugeFile.RewriteBuffered(blockSize, bufferSize: integer;
  lockBuffer: boolean);
var
  options: THFOpenOptions;
begin
  options := [hfoBuffered];
  if lockBuffer then
    Include(options,hfoLockBuffer);
  Win32Check(RewriteEx(blockSize,bufferSize,0,0,options) = hfOK,'RewriteBuffered');
end; { TGpHugeFile.RewriteBuffered }

{:Full form of Reset. Will retry if file is locked by another application (if
  diskLockTimeout and diskRetryDelay are specified). Allows caller to specify
  additional options. Does not raise an exception on error. 
  @param   blockSize       Basic unit of access (same as RecSize parameter in
                           Delphi's Reset).
  @param   bufferSize      Size of buffer. 0 means default size (BUF_SIZE,
                           currently 64 KB).
  @param   diskLockTimeout Max time (in milliseconds) AccessFile will wait for
                           lock file to become free.
  @param   diskRetryDelay  Delay (in milliseconds) between attempts to open
                           locked file.
  @param   options         Set of possible open options.
  @param   waitObject      Handle of 'terminate' event (semaphore, mutex). If
                           this parameter is specified (not zero) and becomes
                           signalled, AccessFile will stop trying to open locked
                           file and will exit with.
  @returns Status (ok, file locked, other error).
}
function TGpHugeFile.ResetEx(blockSize, bufferSize: integer;
  diskLockTimeout: integer; diskRetryDelay: integer;
  options: THFOpenOptions; waitObject: THandle): THFError;
begin
  hfWindowsError := 0;
  try
    { There's a reason behind this 'if IsOpen...' behaviour. We definitely
      don't want to release file handle if ResetEx is called twice in a row as
      that could lead to all sorts of sharing problems.
      Delphi does this wrong - if you Reset a file twice in a row, handle will
      be closed and file will be reopened.
    }
    if hfCloseOnEOF and IsOpen then
      Close; //2.26
    if IsOpen then begin
      if not hfReading then
        FlushBuffer;
      hfBuffered := false;
      Seek(0);
      FreeBuffer;
    end;
    hfBuffered := hfoBuffered in options;
    hfCloseOnEOF := ([hfoCloseOnEOF,hfoBuffered] * options) = [hfoCloseOnEOF,hfoBuffered];
    hfCanCreate := hfoCanCreate in options;
    if hfBuffered then begin
      hfBufferSize := bufferSize;
      hfLockBuffer := hfoLockBuffer in options;
    end;
    if not IsOpen then
      Result := AccessFile(blockSize,true,diskLockTimeout,diskRetryDelay,waitObject)
    else begin
      hfBlockSize := blockSize;
      AllocBuffer;
      Result := hfOK;
    end;
    if Result <> hfOK then
      Close
    else begin
      if hfBuffered then
        InitReadBuffer;
      hfBufFilePos := 0;
      hfReading := true;
      hfHalfClosed := false;
    end;
  except
    Result := hfOK;
  end;
end; { TGpHugeFile.ResetEx }

{:Full form of Rewrite. Will retry if file is locked by another application (if
  diskLockTimeout and diskRetryDelay are specified). Allows caller to specify
  additional options. Does not raise an exception on error.
  @param   blockSize       Basic unit of access (same as RecSize parameter in
                           Delphi's Rewrite).
  @param   bufferSize      Size of buffer. 0 means default size (BUF_SIZE,
                           currently 64 KB).
  @param   diskLockTimeout Max time (in milliseconds) AccessFile will wait for
                           lock file to become free.
  @param   diskRetryDelay  Delay (in milliseconds) between attempts to open
                           locked file.
  @param   options         Set of possible open options.
  @param   waitObject      Handle of 'terminate' event (semaphore, mutex). If
                           this parameter is specified (not zero) and becomes
                           signalled, AccessFile will stop trying to open locked
                           file and will exit with.
  @returns Status (ok, file locked, other error).
}
function TGpHugeFile.RewriteEx(blockSize, bufferSize: integer;
  diskLockTimeout: integer; diskRetryDelay: integer;
  options: THFOpenOptions; waitObject: THandle): THFError;
begin
  hfWindowsError := 0;
  try
    { There's a reason behind this 'if IsOpen...' behaviour. We definitely
      don't want to release file handle if ResetEx is called twice in a row as
      that could lead to all sorts of sharing problems.
      Delphi does this wrong - if you Reset file twice in a row, handle will be
      closed and file will be reopened.
    }
    if hfCloseOnEOF and IsOpen then
      Close; //2.26
    if IsOpen then begin
      hfBuffered := false;
      Seek(0);
      Truncate;
      FreeBuffer;
    end;
    hfBuffered := hfoBuffered in options;
    if hfBuffered then begin
      hfBufferSize := bufferSize;
      hfLockBuffer := hfoLockBuffer in options;
    end;
    hfCompressed := hfoCompressed in options;
    if not IsOpen then
      Result := AccessFile(blockSize,false,diskLockTimeout,diskRetryDelay,waitObject)
    else begin
      hfBlockSize := blockSize;
      AllocBuffer;
      Result := hfOK;
    end;
    if Result <> hfOK then
      Close
    else begin
      if hfBuffered then
        InitWriteBuffer;
      hfBufFilePos := 0;
      hfReading := false;
      hfHalfClosed := false;
    end;
  except
    Result := hfOK;
  end;
end; { TGpHugeFile.RewriteEx }

{:Closes open file. If file is not open, do nothing.
  @raises  EGpHugeFile on Windows errors.
}
procedure TGpHugeFile.Close;
begin
  try
    if IsOpen then begin
      FreeBuffer;
      if hfHandle <> INVALID_HANDLE_VALUE then begin // may be freed in BlockRead
        CloseHandle(hfHandle);
        hfHandle := INVALID_HANDLE_VALUE;
      end;
      hfHalfClosed := false;
      hfIsOpen := false;
      hfCloseOnEOF := false;
    end;
  except
    on EGpHugeFile do
      raise;
    on E:Exception do
      raise EGpHugeFile.CreateHelp(E.Message,hcHFUnexpected);
  end;
end; { TGpHugeFile.Close }

{:Checks if file is open. Called from various TGpHugeFile methods.
  @raises  EGpHugeFile if file is not open.
}
procedure TGpHugeFile.CheckHandle();
begin
  if hfHandle = INVALID_HANDLE_VALUE then
    raise EGpHugeFile.CreateFmtHelp(sFileNotOpen,[FileName],hcHFInvalidHandle);
end; { TGpHugeFile.CheckHandle }

{:Returns the size of file in 'block size' units (see 'blockSize' parameter to
  Reset and Rewrite methods).
  @returns Size of file in 'block size' units.
  @raises  EGpHugeFile on Windows errors.
  @seeAlso Reset, Rewrite
}
function TGpHugeFile.FileSizeNoCache : HugeInt;
var
  realSize: HugeInt;
  size    : TLargeInteger;
begin
  try
    if hfHalfClosed then
      result := hfLastSize //2.26: hfoCloseOnEOF support
    else begin
      // TODO 1 -oPrimoz Gabrijelcic: Optimize!
      CheckHandle;
      SetLastError(0);
      size.LowPart := GetFileSize(hfHandle,@size.HighPart);
      Win32Check(size.LowPart<>$FFFFFFFF,'FileSize');
      if hfBufFilePos > size.QuadPart then
        realSize := hfBufFilePos
      else
        realSize := size.QuadPart;
      if hfBlockSize <> 1 then
        result := {$IFDEF D4plus}Trunc{$ELSE}int{$ENDIF}
                    (realSize/hfBlockSize)
      else
        result := realSize;
    end;
    hfCachedSize := result;
  except
    on EGpHugeFile do
      raise;
    on E:Exception do
      raise EGpHugeFile.CreateHelp(E.Message,hcHFUnexpected);
  end;
end; { TGpHugeFile.UpdateFileSizeCache }

{:Writes 'count' number of 'block size' large units (see 'blockSize' parameter
  to Reset and Rewrite methods) to a file (or buffer if access is buffered).
  @param   buf         Data to be written.
  @param   count       Number of 'block size' large units to be written.
  @param   transferred (out) Number of 'block size' large units actually written.
  @raises  EGpHugeFile on Windows errors.
  @seeAlso Reset, Rewrite
}
procedure TGpHugeFile.BlockWrite(const buf; count: DWORD; var transferred: DWORD);
var
  trans: DWORD;
begin
  try
    CheckHandle;
    if hfBlockSize <> 1 then
      count := count * hfBlockSize;
    if hfBuffered then
      Transmit(buf,count,trans)
    else begin
      SetLastError(0);
      Win32Check(WriteFile(hfHandle,buf,count,trans,nil),'BlockWrite');
      hfBufFilePos := hfBufFilePos + trans;
    end;
    if hfBlockSize <> 1 then
      transferred := trans div hfBlockSize
    else
      transferred := trans;
    hfCachedSize := -1;
  except
    on EGpHugeFile do
      raise;
    on E:Exception do
      raise EGpHugeFile.CreateHelp(E.Message,hcHFUnexpected);
  end;
end; { TGpHugeFile.BlockWrite }

{:Reads 'count' number of 'block size' large units (see 'blockSize' parameter
  to Reset and Rewrite methods) from a file (or buffer if access is buffered).
  @param   buf         Buffer for read data.
  @param   count       Number of 'block size' large units to be read.
  @param   transferred (out) Number of 'block size' large units actually read.
  @raises  EGpHugeFile on Windows errors.
  @seeAlso Reset, Rewrite
}
procedure TGpHugeFile.BlockRead(var buf; count: DWORD; var transferred: DWORD);
var
  closeNow  : boolean;
  oldBufSize: DWORD;
  trans     : DWORD;
begin
  try
    if (not hfBuffered) or (not hfHalfClosed) then 
      CheckHandle;
    closeNow := hfCloseOnNext;
    if hfBlockSize <> 1 then
      count := count * hfBlockSize;
    oldBufSize := hfBufSize;
    if hfBuffered then
      Fetch(buf,count,trans)
    else begin
      SetLastError(0);
      Win32Check(ReadFile(hfHandle,buf,count,trans,nil),'BlockRead');
      hfBufFilePos := hfBufFilePos + trans;
    end;
    if hfBlockSize <> 1 then
      transferred := trans div hfBlockSize
    else
      transferred := trans;
    if hfCloseOnEOF then begin
      if closeNow then begin
        if _FilePos >= FileSizeNoCache then begin
          hfLastSize := FileSize;
          CloseHandle(hfHandle);
          hfHandle := INVALID_HANDLE_VALUE;
          hfHalfClosed := true; // allow FilePos to work until TGpHugeFile.Close
          hfCloseOnNext := false;
          //3.03: reset the buffer pointer
          hfBufOffs := hfBufOffs + (oldBufSize - hfBufSize);
          //2.26: rewind the buffer for Seek to work
          hfBufSize := oldBufSize;
        end;
      end
      else
        hfCloseOnNext := (hfHandle <> INVALID_HANDLE_VALUE) and LoadedToTheEOF;
    end;
  except
    on EGpHugeFile do
      raise;
    on E:Exception do
      raise EGpHugeFile.CreateHelp(E.Message,hcHFUnexpected);
  end;
end; { TGpHugeFile.BlockRead }

{:Internal implementation of Seek method. Called from other methods, too. Moves
  actual file pointer only when necessary or required by caller. Handles
  hfoCloseOnEOF files if possible.
  @param   offset      Offset from beginning of file in 'block size' large units
                       (see 'blockSize' parameter to Reset and Rewrite methods).
  @param   movePointer If true, Windows file pointer will always be moved. If
                       false, it will only be moved when Seek destination does
                       not lie in the buffer.
  @raises  Various system exceptions.
  @seeAlso Reset, Rewrite
}
procedure TGpHugeFile._Seek(offset: HugeInt; movePointer: boolean);
var
  off: TLargeInteger;
begin
  if (not hfBuffered) or movePointer or (not hfHalfClosed) then
    CheckHandle;
  if hfBlockSize <> 1 then
    off.QuadPart := offset*hfBlockSize
  else
    off.QuadPart := offset;
  if hfBuffered then begin
    if hfBufWrite then begin
      FlushBuffer;
      //<3.08: Cope with the delayed seek
      Win32Check(SetFilePointer(
        hfHandle,longint(off.LowPart),@off.HighPart,FILE_BEGIN)<>$FFFFFFFF,'_Seek');
      //>
    end
    else begin
      if not movePointer then begin
        if (off.QuadPart >= hfBufFileOffs) or
           (off.QuadPart < (hfBufFileOffs-hfBufSize)) then
          movePointer := true
        else
          hfBufOffs := {$IFNDEF D4plus}Trunc{$ENDIF}
                         (off.QuadPart-(hfBufFileOffs-hfBufSize));
      end;
      if movePointer then begin
        if hfHalfClosed then begin
          if off.QuadPart <> hfBufFileOffs then //2.26: allow seek to EOF
            CheckHandle; // bang!
        end
        else begin
          SetLastError(0);
          Win32Check(SetFilePointer(
            hfHandle,longint(off.LowPart),@off.HighPart,FILE_BEGIN)<>$FFFFFFFF,'_Seek');
        end;
        //3.02: Seek to EOF in hfHalfClosed state must not invalidate the buffer
        if not (hfHalfClosed and (off.QuadPart = hfBufFileOffs)) then begin
          hfBufFileOffs := off.QuadPart;
          hfBufFilePos  := off.QuadPart;
          hfBufOffs     := 0;
          hfBufSize     := 0;
          hfCloseOnNext := false;
        end;
      end
      else if not LoadedToTheEOF then
        hfCloseOnNext := false;
    end;
  end
  else begin
    SetLastError(0);
    Win32Check(SetFilePointer(hfHandle,longint(off.LowPart),@off.HighPart,FILE_BEGIN)<>$FFFFFFFF,'Seek');
  end;
  hfBufFilePos := off.QuadPart;
end; { TGpHugeFile._Seek }

{:Repositions file pointer. Moves actual file pointer only when necessary.
  @param   offset Offset from beginning of file in 'block size' large units (see
           'blockSize' parameter to Reset and Rewrite methods).
  @raises  EGpHugeFile on Windows errors.
  @seeAlso Reset, Rewrite
}
procedure TGpHugeFile.Seek(offset: HugeInt);
begin
  try
    _Seek(offset,false);
  except
    on EGpHugeFile do
      raise;
    on E:Exception do
      raise EGpHugeFile.CreateHelp(E.Message,hcHFUnexpected);
  end;
end; { TGpHugeFile.Seek }

{:Returns file pointer position in bytes. Used only internally.
  @returns File pointer position in bytes.
  @raises  Various system exceptions.
}
function TGpHugeFile._FilePos: HugeInt;
var
  off: TLargeInteger;
begin
  CheckHandle;
  off.QuadPart := 0;
  off.LowPart := SetFilePointer(hfHandle,longint(off.LowPart),@off.HighPart,FILE_CURRENT);
  Win32Check(off.LowPart <> $FFFFFFFF,'_FilePos');
  Result := off.QuadPart;
end; { TGpHugeFile. }

{:Truncates file at current position.
  @raises  EGpHugeFile on Windows errors.
}
procedure TGpHugeFile.Truncate;
begin
  try
    CheckHandle;
    if hfBuffered then
      _Seek(FilePos,true);
    SetLastError(0);
    Win32Check(SetEndOfFile(hfHandle),'Truncate');
    hfCachedSize := -1;
  except
    on EGpHugeFile do
      raise;
    on E:Exception do
      raise EGpHugeFile.CreateHelp(E.Message,hcHFUnexpected);
  end;
end; { TGpHugeFile.Truncate }

{:Returns EOF indicator.
  @since   2003-02-12
}
function TGpHugeFile.EOF: boolean;
begin
  if hfFlagNoBuf then
    Result := (FilePos >= FileSizeNoCache)
  else
    Result := (FilePos >= FileSize);
end; { TGpHugeFile.EOF }

{:Returns file pointer position in 'block size' large units (see 'blockSize'
  parameter to Reset and Rewrite methods). Position is retrieved from cached
  value.
  @returns File pointer position in 'block size' large units.
  @raises  EGpHugeFile on Windows errors.
  @seeAlso Reset, Rewrite
}
function TGpHugeFile.FilePos: HugeInt;
begin
  try
    if not hfHalfClosed then
      CheckHandle;
    if hfBlockSize <> 1 then
      Result := {$IFDEF D4plus}Trunc{$ELSE}int{$ENDIF}(hfBufFilePos/hfBlockSize)
    else
      Result := hfBufFilePos;
  except
    on EGpHugeFile do
      raise;
    on E:Exception do
      raise EGpHugeFile.CreateHelp(E.Message,hcHFUnexpected);
  end;
end; { TGpHugeFile.FilePos }

{:Flushed file buffers.
  @raises  EGpHugeFile on Windows errors.
}
procedure TGpHugeFile.Flush;
begin
  CheckHandle;
  SetLastError(0);
  Win32Check(FlushBuffer,'Flush');
  SetLastError(0);
  Win32Check(FlushFileBuffers(hfHandle),'Flush');
end; {  TGpHugeFile.Flush  }

{:Rounds parameter next multiplier of system page size. Used to determine
  buffer size for direct access files (FILE_FLAG_NO_BUFFERING).
  @param   bufSize Initial buffer size.
  @returns bufSize Required buffer size.
}
function TGpHugeFile.RoundToPageSize(bufSize: DWORD): DWORD;
var
  sysInfo: TSystemInfo;
begin
  GetSystemInfo(sysInfo);
  Result := (((bufSize-1) div sysInfo.dwPageSize) + 1) * sysInfo.dwPageSize;
end; { TGpHugeFile.RoundToPageSize }

{:Allocates file buffer (after freeing old buffer if allocated). Calculates
  correct buffer size for direct access files and locks buffer if required. Used
  only internally.
  @raises Various system exceptions.
}
procedure TGpHugeFile.AllocBuffer;
begin
  FreeBuffer;
  if hfBufferSize = 0 then
    hfBufferSize := BUF_SIZE;
  // round up buffer size to be the multiplier of page size
  // needed for FILE_FLAG_NO_BUFFERING access, does not hurt in other cases
  hfBufferSize := RoundToPageSize(hfBufferSize);
  SetLastError(0);
  hfBuffer := VirtualAlloc(nil,hfBufferSize,MEM_RESERVE+MEM_COMMIT,PAGE_READWRITE);
  Win32Check(hfBuffer<>nil,'AllocBuffer');
  if hfLockBuffer then begin
    SetLastError(0);
    Win32Check(VirtualLock(hfBuffer,hfBufferSize),'AllocBuffer');
    if hfBuffer = nil then
      raise EGpHugeFile.CreateFmtHelp(sFailedToAllocateBuffer,[FileName],hcHFFailedToAllocateBuffer);
  end;
end; { TGpHugeFile.AllocBuffer }

{:Frees memory buffer if allocated. Used only internally.
  @raises  Various system exceptions.
}
procedure TGpHugeFile.FreeBuffer;
begin
  if hfBuffer <> nil then begin
    SetLastError(0);
    Win32Check(FlushBuffer,'FreeBuffer');
    if hfLockBuffer then begin
      SetLastError(0);
      Win32Check(VirtualUnlock(hfBuffer,hfBufferSize),'FreeBuffer');
    end;
    SetLastError(0);
    Win32Check(VirtualFree(hfBuffer,0,MEM_RELEASE),'FreeBuffer');
    hfBuffer := nil;
  end;
end; { TGpHugeFile.FreeBuffer }

{:Offsets pointer by a given ammount.
  @param   ptr    Original pointer.
  @param   offset Offset (in bytes).
  @returns New pointer.
}
function OffsetPtr(ptr: pointer; offset: DWORD): pointer;
begin
  Result := pointer(DWORD(ptr)+offset);
end; { OffsetPtr }

{:Writes 'count' number of bytes large units to a file (or buffer if access is
  buffered).
  @param   buf         Data to be written.
  @param   count       Number of bytes to be written.
  @param   transferred (out) Number of bytes actually written.
  @raises  EGpHugeFile when trying to write while in buffered read mode and file
           pointer is not at end of file.
  @raises  Various system exceptions.
  @seeAlso Reset, Rewrite
}
procedure TGpHugeFile.Transmit(const buf; count: DWORD; var transferred: DWORD);
var
  place  : DWORD;
  bufp   : pointer;
  send   : DWORD;
  written: DWORD;
begin
  if not hfBufWrite then begin
    //2.32: If we are at the end of file, we can switch into write mode
    if FilePos = FileSize then begin
      InitWriteBuffer;
      hfReading := false;
    end
    else
      raise EGpHugeFile.CreateFmtHelp(sWriteWhileInBufferedReadMode,[FileName],hcHFWriteInBufferedReadMode);
  end;
  //<3.08b: Cope with the delayed seek
  if (hfBufFilePos <> hfBufFileOffs) and (hfBufOffs = 0) then
    _Seek(hfBufFilePos, true); 
  //>
  transferred := 0;
  place := hfBufferSize-hfBufOffs;
  if place <= count then begin
    Move(buf,OffsetPtr(hfBuffer,hfBufOffs)^,place); // fill the buffer
    hfBufOffs := hfBufferSize;
    hfBufFilePos := hfBufFileOffs+hfBufOffs;
    if not FlushBuffer then
      Exit;
    transferred := place;
    Dec(count,place);
    bufp := OffsetPtr(@buf,place);
    if count >= hfBufferSize then begin // transfer N*(buffer size)
      send := (count div hfBufferSize)*hfBufferSize;
      if not WriteFile(hfHandle,bufp^,send,written,nil) then
        Exit;
      hfBufFileOffs := hfBufFileOffs+written;
      hfBufFilePos := hfBufFileOffs;
      Inc(transferred,written);
      Dec(count,send);
      bufp := OffsetPtr(bufp,send);
    end;                           
  end
  else
    bufp := @buf;
  if count > 0 then begin // store leftovers
    Move(bufp^,OffsetPtr(hfBuffer,hfBufOffs)^,count);
    Inc(hfBufOffs,count);
    Inc(transferred,count);
    hfBufFilePos := hfBufFileOffs+hfBufOffs;
  end;
end; { TGpHugeFile.Transmit }

{:Reads 'count' number of bytes large units from a file (or buffer if access is
  buffered).
  @param   buf         Buffer for read data.
  @param   count       Number of bytes to be read..
  @param   transferred (out) Number of bytes actually read..
  @raises  EGpHugeFile when trying to read while in buffered write mode.
  @raises  Various system exceptions.
  @seeAlso Reset, Rewrite
}
procedure TGpHugeFile.Fetch(var buf; count: DWORD; var transferred: DWORD);
var
  got  : DWORD;
  bufp : pointer;
  read : DWORD;
  trans: DWORD;
begin
  if hfBufWrite then
    raise EGpHugeFile.CreateFmtHelp(sReadWhileInBufferedWriteMode,[FileName],hcHFReadInBufferedWriteMode);
  transferred := 0;
  got := hfBufSize-hfBufOffs;
  if got <= count then begin
    if got > 0 then begin // read from buffer
      Move(OffsetPtr(hfBuffer,hfBufOffs)^,buf,got);
      transferred := got;
      Dec(count,got);
      hfBufFilePos := hfBufFileOffs-hfBufSize+hfBufOffs+got;
    end;
    bufp := OffsetPtr(@buf,got);
    hfBufOffs := 0;
    if count >= hfBufferSize then begin // read directly
      read := (count div hfBufferSize)*hfBufferSize;
      if hfHalfClosed then
        trans := 0 //2.26
      else if not ReadFile(hfHandle,bufp^,read,trans,nil) then
        Exit;
      hfBufFileOffs := hfBufFileOffs+trans;
      hfBufFilePos := hfBufFileOffs;
      Inc(transferred,trans);
      Dec(count,read);
      bufp := OffsetPtr(bufp,read);
      if trans < read then
        Exit; // EOF
    end;
    // fill the buffer
    if not hfHalfClosed then begin 
      if LoadedToTheEOF then
        hfBufSize := 0
      else begin
        SetLastError(0);
        Win32Check(ReadFile(hfHandle,hfBuffer^,hfBufferSize,hfBufSize,nil),'Fetch');
        hfBufFileOffs := hfBufFileOffs+hfBufSize;
      end;
    end
    else begin
      //3.03: when reacing end of buffer in hfHalfClosed mode, buffer must not
      //      be invalidated
      hfBufOffs := hfBufSize;
      Exit;
    end;
  end
  else
    bufp := @buf;
  if count > 0 then begin // read from buffer
    got := hfBufSize-hfBufOffs;
    if got < count then
      count := got;
    if count > 0 then
      Move(OffsetPtr(hfBuffer,hfBufOffs)^,bufp^,count);
    Inc(hfBufOffs,count);
    Inc(transferred,count);
    hfBufFilePos := hfBufFileOffs-hfBufSize+hfBufOffs;
  end;
end; { TGpHugeFile.Fetch }

{:Flushed file buffers (internal implementation).
  @returns False if data could not be written.
}
function TGpHugeFile.FlushBuffer: boolean;
var
  written: DWORD;
begin
  if (hfBufOffs > 0) and hfBufWrite then begin
    if hfFlagNoBuf then
      hfBufOffs := RoundToPageSize(hfBufOffs);
    Result := WriteFile(hfHandle,hfBuffer^,hfBufOffs,written,nil);
    hfBufFileOffs := hfBufFileOffs+written;
    hfBufOffs     := 0;
    hfBufFilePos  := hfBufFileOffs;
    if hfFlagNoBuf then
      FillChar(hfBuffer^,hfBufferSize,0);
  end
  else
    Result := true;
end; { TGpHugeFile.FlushBuffer }

{:Reads 'count' number of 'block size' large units (see 'blockSize' parameter
  to Reset and Rewrite methods) from a file (or buffer if access is buffered).
  @param   buf         Buffer for read data.
  @param   count       Number of 'block size' large units to be read.
  @raises  EGpHugeFile on Windows errors or if not enough data could be read
           from file.
  @seeAlso Reset, Rewrite
}
procedure TGpHugeFile.BlockReadUnsafe(var buf; count: DWORD);
var
  transferred: DWORD;
begin
  BlockRead(buf,count,transferred);
  if count <> transferred then begin
    if hfBuffered then
      raise EGpHugeFile.CreateHelp(sEndOfFile,hcHFUnexpectedEOF)
    else
      Win32Check(false,'BlockReadUnsafe');
  end;
end; { TGpHugeFile.BlockReadUnsafe }

{:Writes 'count' number of 'block size' large units (see 'blockSize' parameter
  to Reset and Rewrite methods) to a file (or buffer if access is buffered).
  @param   buf         Data to be written.
  @param   count       Number of 'block size' large units to be written.
  @raises  EGpHugeFile on Windows errors or if data could not be written
                       completely.
  @seeAlso Reset, Rewrite
}
procedure TGpHugeFile.BlockWriteUnsafe(const buf; count: DWORD);
var
  transferred: DWORD;
begin
  BlockWrite(buf,count,transferred);
  if count <> transferred then begin
    if hfBuffered then
      raise EGpHugeFile.CreateFmtHelp(sWriteFailed,[FileName],hcHFWriteFailed)
    else
      Win32Check(false,'BlockWriteUnsafe');
  end;
end; { BlockWriteUnsafe }

{:Returns true if file is open.
  @returns True if file is open.
}
function TGpHugeFile.IsOpen: boolean;
begin
  Result := hfIsOpen;
end; { TGpHugeFile.IsOpen }

{:Checks condition and creates appropriately formatted EGpHugeFile exception.
  @param   condition If false, Win32Check will generate an exception.
  @param   method    Name of TGpHugeFile method that called Win32Check.
  @raises  EGpHugeFile if (not condition).
}


procedure TGpHugeFile.Win32Check(condition: boolean; const method: string);
var
  Error: EGpHugeFile;

  Procedure ThrowCase1;
  begin
  Error := EGpHugeFile.CreateFmtHelp(sFileFailed+
        {$IFNDEF D6PLUS}SWin32Error{$ELSE}SOSError{$ENDIF},
        [method,hfName,hfWindowsError,SysErrorMessage(hfWindowsError)],
        hcHFWindowsError)
  end;

  Procedure ThrowCase2;
  begin
  Error := EGpHugeFile.CreateFmtHelp(sFileFailed+
        {$IFNDEF D6PLUS}SUnkWin32Error{$ELSE}SUnkOSError{$ENDIF},
        [method,hfName],hcHFUnknownWindowsError)
  end;  
begin
  if not condition then begin
    hfWindowsError := GetLastError;
    if hfWindowsError <> ERROR_SUCCESS then
      ThrowCase1
    else
      ThrowCase2;
    raise Error;
  end;
end; { TGpHugeFile.Win32Check }

{:Returns file date in Delphi format.
  @returns Returns file date in Delphi format.
  @raises  EGpHugeFile on Windows errors.
}
function TGpHugeFile.GetDate: TDateTime;
begin
  try
    CheckHandle;
    Result := FileDateToDateTime(FileAge(FileName));
  except
    on EGpHugeFile do
      raise;
    on E:Exception do
      raise EGpHugeFile.CreateHelp(E.Message,hcHFUnexpected);
  end;
end; { TGpHugeFile.GetDate }

{:Sets file date.
  @param   Value new file date.
}
procedure TGpHugeFile.SetDate(const Value: TDateTime);
var
  err: integer;
begin
  try
    CheckHandle;
    err := FileSetDate(hfHandle,DateTimeToFileDate(Value));
    if err <> 0 then
      raise EGpHugeFile.CreateFmtHelp(sFileFailed+SysErrorMessage(err),
        ['SetDate',hfName],hcHFWindowsError);
  except
    on EGpHugeFile do
      raise;
    on E:Exception do
      raise EGpHugeFile.CreateHelp(E.Message,hcHFUnexpected);
  end;
end; { TGpHugeFile.SetDate }

{:Returns true if file is loaded into the buffer up to the last byte.
  @returns Returns true if file is loaded into the buffer up to the last byte.
}
function TGpHugeFile.LoadedToTheEOF: boolean;
begin
if (hfBlockSize = 1) then
  Result :=  (hfBufFileOffs >= FileSize)
else
  Result := (hfBufFileOffs >= (FileSize*hfBlockSize))
end; { TGpHugeFile.LoadedToTheEOF }

{:Returns file size. If available, returns cached size.
  @returns File size in bytes.
  @raises  EGpHugeFile on Windows errors.
}
function TGpHugeFile.FileSize: HugeInt;
begin
  if hfCachedSize < 0 then
    hfCachedSize := FileSizeNoCache;
  Result := hfCachedSize;
end; { TGpHugeFile.FileSize }

{:Initializes buffer for writing.
}
procedure TGpHugeFile.InitWriteBuffer;
begin
  hfBufSize     := 0;
  hfBufOffs     := 0;
  hfBufFileOffs := 0;
  hfBufWrite    := true;
end; { TGpHugeFile.InitWriteBuffer }

{:Initializes buffer for reading.
}
procedure TGpHugeFile.InitReadBuffer;
begin
  hfBufOffs     := 0;
  hfBufSize     := 0;
  hfBufFileOffs := 0;
  hfBufWrite    := false;
end; { TGpHugeFile.InitReadBuffer }

{ TGpHugeFileStream }

{:Initializes stream and opens file in required access mode.
  @param   fileName    Name of file to be accessed.
  @param   access      Required access mode.
  @param   openOptions Set of possible open options.
}
constructor TGpHugeFileStream.Create(const fileName: string;
  access: TGpHugeFileStreamAccess; openOptions: THFOpenOptions);
begin
  inherited Create;
  hfsExternalHF := false;
  case access of
    accRead:
      begin
        hfsFile := TGpHugeFile.CreateEx(fileName, FILE_ATTRIBUTE_NORMAL, GENERIC_READ);
        hfsFile.Win32Check(hfsFile.ResetEx(1,0,0,0,openOptions) = hfOK, 'Reset');
      end; //accRead
    accWrite:
      begin
        hfsFile := TGpHugeFile.CreateEx(fileName, FILE_ATTRIBUTE_NORMAL, GENERIC_WRITE);
        hfsFile.Win32Check(hfsFile.RewriteEx(1,0,0,0,openOptions) = hfOK, 'Rewrite');
      end; //accWrite
    accReadWrite:
      begin
        hfsFile := TGpHugeFile.CreateEx(fileName, FILE_ATTRIBUTE_NORMAL, GENERIC_READ+GENERIC_WRITE);
        hfsFile.Win32Check(hfsFile.ResetEx(1,0,0,0,openOptions) = hfOK, 'Reset');
      end; // accReadWrite
    accAppend:
      begin
        hfsFile := TGpHugeFile.CreateEx(fileName, FILE_ATTRIBUTE_NORMAL, GENERIC_READ+GENERIC_WRITE);
        hfsFile.Win32Check(hfsFile.ResetEx(1,0,0,0,openOptions+[hfoCanCreate]) = hfOK, 'Append');
        hfsFile.Seek(hfsFile.FileSizeNoCache);
      end; //accAppend
  end; //case
end; { TGpHugeFileStream.Create }

{:Initializes stream and assigns it an already open TGpHugeFile object.
  @param   hf TGpHugeFile object to be used for data storage.
}
constructor TGpHugeFileStream.CreateFromHandle(hf: TGpHugeFile);
begin
  inherited Create;
  hfsExternalHF := true;
  hfsFile := hf;
end; { TGpHugeFileStream.Create/CreateFromHandle }

{:Destroys stream and file access object (if created in constructor).
}
destructor TGpHugeFileStream.Destroy;
begin
  if (not hfsExternalHF) and assigned(hfsFile) then begin
    hfsFile.Close;
    hfsFile.Free;
    hfsFile := nil;
  end;
  inherited Destroy;
end; { TGpHugeFileStream.Destroy }

{:Returns file name.
  @returns Returns file name or empty string if file is not open.
}
function TGpHugeFileStream.GetFileName: string;
begin
  if assigned(hfsFile) then
    Result := hfsFile.FileName
  else
    Result := '';
end; { TGpHugeFileStream.GetFileName }

{:Returns file size. Better compatibility with hfCloseOnEOF files than default
  TStream.GetSize.
  @returns Returns file size in bytes or -1 if file is not open.
}
{$IFDEF D7PLUS}
function TGpHugeFileStream.GetSize: int64;
{$ELSE}
function TGpHugeFileStream.GetSize: longint;
{$ENDIF D7PLUS}
begin
  if assigned(hfsFile) then
    Result := hfsFile.FileSize
  else
    Result := -1;
end; { TGpHugeFileStream.GetSize }

{:Returns last Windows error code.
  @returns Last Windows error code.
}
function TGpHugeFileStream.GetWindowsError: DWORD;
begin
  if hfsWindowsError <> 0 then
    Result := hfsWindowsError
  else if assigned(hfsFile) then
    Result := hfsFile.WindowsError
  else
    Result := 0;
end; { TGpHugeFileStream.GetWindowsError }

{:Reads 'count' number of bytes into buffer.
  @param   buffer Buffer for read data.
  @param   count  Number of bytes to be read.
  @returns Actual number of bytes read.
  @raises  EGpHugeFile on Windows errors.
}
function TGpHugeFileStream.Read(var buffer; count: longint): longint;
var
  bytesRead: cardinal;
begin
  hfsFile.BlockRead(Buffer,Count,bytesRead);
  Result := longint(bytesRead);
end; { TGpHugeFileStream.Read }

{:Repositions stream pointer.
  @param   offset Offset from start, current position, or end of stream (as set
                  by the 'mode' parameter).
  @param   mode   Specifies starting point for offset calculation
                  (soFromBeginning, soFromCurrent, soFromEnd).
  @returns New position of stream pointer.
  @raises  EGpHugeFile on Windows errors.
  @raises  EGpHugeFileStream on invalid value of 'mode' parameter.
}
function TGpHugeFileStream.Seek(offset: longint; mode: word): longint;
begin
  if mode = soFromBeginning then
    hfsFile.Seek(offset)
  else if mode = soFromCurrent then
    hfsFile.Seek(hfsFile.FilePos+offset)
  else if mode = soFromEnd then
    hfsFile.Seek(hfsFile.FileSize+offset)
  else
    raise EGpHugeFileStream.CreateFmtHelp(sInvalidMode,[FileName],hcHFInvalidSeekMode);
  Result := hfsFile.FilePos;
end; { TGpHugeFileStream.Seek }

{$IFDEF D7PLUS}
{:Delphi 7-compatible seek.
}
function TGpHugeFileStream.Seek(const offset: Int64; origin: TSeekOrigin): int64;
begin
  if origin = soBeginning then
    hfsFile.Seek(offset)
  else if origin = soCurrent then
    hfsFile.Seek(hfsFile.FilePos+offset)
  else if origin = soEnd then
    hfsFile.Seek(hfsFile.FileSize+offset)
  else
    raise EGpHugeFileStream.CreateFmtHelp(sInvalidMode,[FileName], hcHFInvalidSeekMode);
  Result := hfsFile.FilePos;
end; { TGpHugeFileStream.Seek }
{$ENDIF D7PLUS}

{:Sets stream size. Truncates underlying file at specified position.
  @param   newSize New stream size.
  @raises  EGpHugeFile on Windows errors.
}
procedure TGpHugeFileStream.SetSize(newSize: longint);
begin
  hfsFile.Seek(newSize);
  hfsFile.Truncate;
end; { TGpHugeFileStream.SetSize }

{$IFDEF D7PLUS}
{:Sets stream size. Truncates underlying file at specified position.
  @param   newSize New stream size.
  @raises  EGpHugeFile on Windows errors.
}
procedure TGpHugeFileStream.SetSize(const newSize: int64);
begin
  SetSize64(newSize);
end; { TGpHugeFileStream.SetSize }

procedure TGpHugeFileStream.SetSize64(const newSize: int64);
begin
  hfsFile.Seek(newSize);
  hfsFile.Truncate;
end; { TGpHugeFileStream.SetSize64 }
{$ENDIF D7PLUS}

{:Checks condition and creates appropriately formatted EGpHugeFileStream
  exception.
  @param   condition If false, Win32Check will generate an exception.
  @param   method    Name of TGpHugeFileStream method that called Win32Check.
  @raises  EGpHugeFileStream if (not condition).
}
procedure TGpHugeFileStream.Win32Check(condition: boolean; const method: string);
var
  Error: EGpHugeFileStream;
begin
  if not condition then begin
    hfsWindowsError := GetLastError;
    if hfsWindowsError <> ERROR_SUCCESS then
      Error := EGpHugeFileStream.CreateFmtHelp(sStreamFailed+{$IFDEF D6PLUS}SOSError{$ELSE}SWin32Error{$ENDIF},
        [method,FileName,hfsWindowsError,SysErrorMessage(hfsWindowsError)],
        hcHFWindowsError)
    else
      Error := EGpHugeFileStream.CreateFmtHelp(sStreamFailed+{$IFDEF D6PLUS}SUnkOSError{$ELSE}SUnkWin32Error{$ENDIF},
        [method,FileName],hcHFUnknownWindowsError);
    raise Error;
  end;
end; { TGpHugeFileStream.Win32Check }

{:Writes 'count' number of bytes to the file.
  @param   buffer Data to be written.
  @param   count  Number of bytes to be written.
  @returns Actual number of bytes written.
  @raises  EGpHugeFile on Windows errors.
}
function TGpHugeFileStream.Write(const buffer; count: longint): longint;
var
  bytesWritten: cardinal;
begin
  hfsFile.BlockWrite(buffer,count,bytesWritten);
  Result := longint(bytesWritten);
end; { TGpHugeFileStream.Write }

end.

