unit TextFileU;

interface
uses
  gphugef;

Type
  EGpHugeFile = gphugef.EGpHugeFile;
  TTextFile = class
  protected
    filehandle : TGpHugeFile;
    HasFlushed : boolean;    
    constructor Create;
  public
    constructor Create_Append( const FileName : string );
    constructor Create_Rewrite( const FileName : string );
    destructor Destroy; override;

    procedure Write( const s : string );
    procedure Writeln( const s : string );
    procedure Flush;
  end; {TTextFile}

implementation
uses
  TADemoConsts,
  windows, sysutils;

constructor TTextFile.Create;
begin
assert(false);
end;

constructor TTextFile.Create_Append( const FileName : string );
begin
inherited Create;
filehandle := TGpHugeFile.CreateEx( FileName,
                                    FileAttributes,
                                    AccessMode[true],
                                    ShareFlags[IsWin9x]);
Win32Check(filehandle.ResetEx(1,1024,0,0,FileFlags[SupportsCompression(FileName)],0) <> hfError);
filehandle.Seek( filehandle.FileSize );
HasFlushed := true;
end; {Create_Append}

constructor TTextFile.Create_Rewrite( const FileName : string );
begin
inherited create;
filehandle := TGpHugeFile.CreateEx( FileName,
                                    FileAttributes,
                                    AccessMode[true],
                                    ShareFlags[IsWin9x]);
Win32Check(filehandle.RewriteEx(1,1024,0,0,FileFlags[SupportsCompression(FileName)],0) <> hfError);
HasFlushed := true;
end; {Create_Rewrite}

destructor TTextFile.Destroy;
begin
FreeAndNil( filehandle );
inherited;
end; {Destroy}

procedure TTextFile.Flush;
begin
if not HasFlushed then
  begin
  HasFlushed := true;
  filehandle.Flush;
  end;
end; {Flush}

procedure TTextFile.Write( const s : string );
begin
if s <> '' then
  begin
  HasFlushed := false;
  filehandle.BlockWriteUnsafe( s[1], Length(s) );
  end;
end; {Write}

procedure TTextFile.Writeln( const s : string );
var s2 : string;
begin
HasFlushed := false;
s2 := s +#13#10;
filehandle.BlockWriteUnsafe( s2[1], Length(s2) );
end; {Writeln}

end.
