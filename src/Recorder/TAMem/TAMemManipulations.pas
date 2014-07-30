unit TAMemManipulations;

interface
uses
  windows, sysutils,
  Messages,
  Classes,
  ZLibEx,
  MemMappedDataStructure;

type
  TCheatType = (ctMemory, ctProcesses, ctScreenshot);

type
  TEnumWindowsThread = class(TThread)
  protected
    procedure Execute; override;
  end;

type
  TCheckMemoryCheats = class(TThread)
  protected
    procedure Execute; override;
  end;

type
  TScreenshotCreate = class(TThread)
  protected
    procedure Execute; override;
  end;

function TestByte( p : longword; b : byte) : Boolean; overload;
function TestByte( p : longword; b : byte; var failvalue : byte) : Boolean; overload;

function TestBytes( p : longword;
                    ExpectedData : PByteArray; len : integer;
                    var FailIndex : integer; var failvalue : byte) : Boolean;

procedure SetByte( p : pointer; data : Byte ); overload;
procedure SetByte( p : longword; data : Byte ); overload;
procedure SetWord( p : pointer; data : Word ); overload;
procedure SetWord( p : longword; data : Word ); overload;
procedure SetLongword( p : pointer; data : longword); overload;
procedure SetLongword( p : longword; data : longword); overload;

procedure CheckForCheats(CheatType: TCheatType);

implementation
uses
  idplay,
  textdata,
  TA_FunctionsU,
  TA_MemoryLocations,
  TA_MemoryStructures,
  TADemoConsts,
  IdMultipartFormData, IdHTTP;

const
  Xorkey: array [0..3] of Byte = ($91, $64, $66, $56);

var
  ProhibitedFound: Boolean;

function TestByte(p : cardinal;b : byte) : Boolean;
begin
result:= IsBadReadPtr(Pbyte(p),1) or (Pbyte(p)^ = b);
end;

function TestByte( p : longword; b : byte; var FailValue : byte) : Boolean; overload;
begin
if not IsBadReadPtr(Pbyte(p),1) then
  begin
  FailValue := Pbyte(p)^;
  result := (FailValue = b);
  end
else
  begin
  result := false;
  failvalue := not b;
  end;
end;

function TestBytes( p : longword;
                    ExpectedData : PByteArray; len : integer;
                    var FailIndex : integer; var failvalue : byte) : Boolean;
var
  i : integer;
begin
for i := 0 to len -1 do
  begin
  if not TestByte( p, ExpectedData[i], FailValue) then
    begin
    FailIndex := i;
    result := false;
    exit;
    end;
  inc(p);  
  end;
FailIndex := -1;
failvalue := 0;
result := true;
end;

procedure SetByte( p : pointer; data : Byte );
begin
PByte(p)^ := data;
end;

procedure SetByte( p : longword; data : Byte );
begin
PByte(p)^ := data;
end;

procedure SetWord( p : pointer; data : Word );
begin
PWord(p)^ := data;
end;

procedure SetWord( p : longword; data : Word );
begin
PWord(p)^ := data;
end;

procedure SetLongword( p : pointer; data : longword);
begin
PLongword(p)^ := data;
end;

procedure SetLongword( p : longword; data : longword);
begin
PLongword(p)^ := data;
end;

procedure FindprohibitedProcesses;
var
  CheckThread: TEnumWindowsThread;
begin
  CheckThread:= TEnumWindowsThread.Create(False);
  CheckThread.FreeOnTerminate:= True;
end;

function EnumWindowsProc(Wnd: HWND): BOOL; stdcall;
var
  Caption: Array [0..128] of Char;
  sCaption: String;
begin
  SendMessage(Wnd, WM_GETTEXT, Sizeof(Caption), integer(@Caption));
  sCaption:= Caption;

  if Pos('Total Annihilation Trainer', sCaption) <> 0 then
  begin
    ProhibitedFound:= True;
    Result:= False;
  end else
    Result:= True;
end;

procedure TEnumWindowsThread.Execute;
var
  Cheats: TTACheats;
begin
  Cheats:= 0;
  try
    ProhibitedFound:= False;
    try
      EnumWindows(@EnumWindowsProc, 0);
    except
    end;
  finally
    if ProhibitedFound then
      Cheats:= Cheat_ProhibitedTask;
    if GlobalDPlay <> nil then
      GlobalDPlay.OnFinishedCheatsCheck(Cheats);
  end;
end;

procedure FindMemoryCheats;
var
  CheckThread: TCheckMemoryCheats;
begin
  CheckThread:= TCheckMemoryCheats.Create(False);
  CheckThread.FreeOnTerminate:= True;
end;

procedure TCheckMemoryCheats.Execute;
var
  Cheats: TTACheats;
begin
  Cheats:= 0;
  // here be magic numbers
  if not TestByte($489CF5,4) then
    Cheats:=Cheats or Cheat_Invulnerability;

  if not TestByte($401805,$67) or not TestByte($4017E7,$0f) or
     not TestByte($401808,$76) or not TestByte($40181E,$d9) then
    Cheats:=Cheats or Cheat_Invisible;

  if not TestByte($402AD9,1) or not TestByte($402AD9,1) or
     not TestByte($403EFF,1) or not TestByte($4041F5,1) or
     not TestByte($4142D2,1) or not TestByte($4146F3,1) then
    Cheats:=Cheats or Cheat_FastBuild;
  if not TestByte($4018BD,$d8) or not TestByte($4018D9,$d8) then
    Cheats:=Cheats or Cheat_FastBuild;

  if not TestByte($4018BD,$d8) or not TestByte($4018D9,$d8) then
    Cheats:=Cheats or Cheat_InfiniteResources;

  if not TestByte($484470,$7d) or not TestByte($4844A9,$0f) or
     not TestByte($466CCB,$0f) or not TestByte($466C38,$0f) or
     not TestByte($466D16,$75) or not TestByte($466E31,8) or
     not TestByte($48BC5E,$1e) then
    Cheats:=Cheats or Cheat_LosRadar;

  if not TestByte($457ACF,$74) then
    Cheats:=Cheats or Cheat_ControlMenu;

  if not TestByte($47D4C0,$0f) then
    Cheats:=Cheats or Cheat_BuildAnywhere;

  if not TestByte($404298,$8a) then
    Cheats:=Cheats or Cheat_InstantCapture;

  if not TestByte($43cf98,$91) then
    Cheats:=Cheats or Cheat_SpecialMove;

  if not TestByte($46704A,$31) or not TestByte($467041,$88) then
    Cheats:=Cheats or Cheat_JamAll;

  if not TestByte($499DE7,$c1) then
    Cheats:=Cheats or Cheat_ExtraDamage;

  if not TestByte($41BACD,$db) or not TestByte($41BACE,$81) or
     not TestByte($41BB37,$81) or not TestByte($41BB38,$86) then
   Cheats:= Cheats or Cheat_InstantBuild;

  if TestByte($401AC1,$90) and TestByte($401AC2,$90) and
     TestByte($401AC3,$90) and TestByte($401AC4,$90) and
     TestByte($401AC5,$90) and TestByte($401AC6,$90) then
   Cheats:= Cheats or Cheat_ResourcesFreeze;
  if TestByte($401AFE,$90) and TestByte($401AFF,$90) and
     TestByte($401B00,$90) and TestByte($401B01,$90) and
     TestByte($401B02,$90) and TestByte($401B03,$90) then
   Cheats:= Cheats or Cheat_ResourcesFreeze;

{if (PByte(Plongword($511de8)^+$37F2F)^ and (1 shl 1) ) <> 0 then
 result:= result or Cheat_DeveloperMode; }

  if (PByte(Plongword($511DE8)^+$37F2F)^ and (1 shl 7) ) <> 0 then
   Cheats:= Cheats or Cheat_DoubleShot;

  if GlobalDPlay <> nil then
    GlobalDPlay.OnFinishedCheatsCheck(Cheats);
end;

procedure SendScreenShot;
var
  ScreenshotCreate: TScreenshotCreate;
begin
  ScreenshotCreate:= TScreenshotCreate.Create(False);
  ScreenshotCreate.FreeOnTerminate:= True;
end;

function FindVolumeSerial(const Drive : PChar) : string;
var
   VolumeSerialNumber : DWORD;
   MaximumComponentLength : DWORD;
   FileSystemFlags : DWORD;
   SerialNumber : string;
begin
   Result:='';

   GetVolumeInformation(
        Drive,
        nil,
        0,
        @VolumeSerialNumber,
        MaximumComponentLength,
        FileSystemFlags,
        nil,
        0) ;
   SerialNumber :=
         IntToHex(HiWord(VolumeSerialNumber), 4) +
         IntToHex(LoWord(VolumeSerialNumber), 4) ;

   Result := SerialNumber;
end;

procedure DecompressFile(const SourceFile, DestFile: string);
var
  SourceStream: TStream;
  ZLibStream: TStream;
  DestStream: TStream;
  DecSize: Cardinal;
  Ms: TStream;
  i: Integer;
  Data: array of Byte;
  MsOut: TStream;
begin
  SourceStream := TFileStream.Create(SourceFile, fmOpenRead);
  Ms := TMemoryStream.Create;
  MsOut := TMemoryStream.Create;

  Ms.CopyFrom(SourceStream, SourceStream.Size);
  Ms.Position := 0;

  SetLength(Data, Ms.Size);
  Ms.Read(Data[0], Ms.Size);
  DecSize := PDword(@Data[0])^;

  for i := 0 to 3 do
   Data[i + 4] := Data[i + 4] xor Xorkey[i mod 4];

  MsOut.Size := Length(Data);
  MsOut.Write(Data[0], Length(Data));
  MsOut.Position := 4;
  try
    ZLibStream := TZDecompressionStream.Create(MsOut, -15);
    try
      DestStream := TFileStream.Create(DestFile, fmCreate or fmShareExclusive);
      try
        ZLibStream.Read(Data[0], SizeOf(DecSize));
        DestStream.Write(Data[0],SizeOf(DecSize));
        DestStream.CopyFrom(ZLibStream, DecSize -4);
      finally
        DestStream.Free;
      end;
    finally
      ZLibStream.Free;
    end;
  finally
    SourceStream.Free;
    Ms.Free;
    MsOut.Free;
  end;
end;

procedure CompressFile(const SourceFile, DestFile: string);
var
  SourceStream: TStream;
  ZLibStream: TStream;
  DestStream: TStream;
  DecSize, FixShit: Integer;
  i: Integer;
  Data: array of Byte;
begin
  SourceStream := TFileStream.Create(SourceFile, fmOpenRead);
  try
    DecSize := SourceStream.Size;
    DestStream := TFileStream.Create(DestFile, fmCreate or fmShareExclusive);

    DestStream.Write(DecSize,SizeOf(DecSize));
    try
      ZLibStream := TZCompressionStream.Create(DestStream, zcMax , -15, 8, zsDefault);
      try
       repeat
        FixShit := SourceStream.Read(DecSize, SizeOf(DecSize));
        ZLibStream.Write(DecSize, FixShit);
       until FixShit = 0;
      finally
        ZLibStream.Free;
      end;

      SetLength(Data, DestStream.Size);
      DestStream.Position := 0;
      DestStream.Read(Data[0],Length(Data));
      DestStream.Position := 0;

       for i := 0 to 3 do
        Data[i + 4] := Data[i + 4] xor Xorkey[i mod 4];

      DestStream.Write(Data[0],Length(Data));
    finally
      DestStream.Free;
    end;
  finally
    SourceStream.Free;
  end;
end;

procedure SendSS(FileName: String);
var
  Stream: TStringStream;
  ZipFileName : String;
  Params: TIdMultipartFormDataStream;
  HTTP : TIdHTTP;
begin
  ZipFileName := Copy(FileName, 1, Length(FileName) - 9);
  ZipFileName := ChangeFileExt(ZipFileName, '.ac');
  CompressFile(FileName, ZipFileName);
  try
    DeleteFile(FileName);
  except
  end;
  Stream := TStringStream.Create('');
  HTTP := TIdHTTP.Create(nil);
  try
    Params := TIdMultipartFormDataStream.Create;
    try
      Params.AddFile('ssfile', ZipFileName, 'application/octet-stream');
      try
        HTTP.Post('http://plobex.pl/anticheat/up.php', Params, Stream);
      except
        Exit;
      end;
    finally
      Params.Free;
    end;
  finally
    Stream.Free;
    try
      DeleteFile(ZipFileName);
    except
    end;
  end;
end;

function FileSearch(const PathName, FileName, MaskName : string; Delete: Boolean): String ;
var
  Rec : TSearchRec;
  Path : string;
begin
  Path := IncludeTrailingPathDelimiter(PathName) ;
  if FindFirst(Path + FileName, faAnyFile - faDirectory, Rec) = 0 then
  try
    repeat
      if not Delete then
      begin
        if Pos(MaskName, Rec.Name) <> 0 then
        begin
          Result := Rec.Name;
          Break;
        end;
      end else
      begin
        DeleteFile(PathName + Rec.Name);
      end;
    until FindNext(Rec) <> 0;
  finally
    FindClose(Rec);
  end;
end;

procedure TScreenshotCreate.Execute;
var
  TAProgramStruct : Pointer;
  FileName, DirName, FinalFileName, Date, PlayerName : String;
  BuffLen : Cardinal;
  UserName : String;
  DriveSerial : String;
  sGameRunSec : String;
  UID : String;

  MakeSS : Boolean;

  HTTP : TIdHTTP;
  Response: String;
begin
  TAProgramStruct := GetTAProgramStruct;
  if TAProgramStruct <> nil then
  begin
    BuffLen := 255;
    SetLength(UserName, BuffLen);
    GetUserName(PChar(UserName), BuffLen);
    UserName := Copy(UserName, 1, BuffLen - 1);
    DriveSerial := RemoveInvalidIncSpace(FindVolumeSerial('c:\'));
    PlayerName := PPlayerStruct(TAPlayer.GetPlayerByIndex(TAData.ViewPlayer)).szName;
    UID := DriveSerial + '_' + RemoveInvalidIncSpace(UserName);

    Date := FormatDateTime('dd_mm_yyyy_hh_nn', Now);
    MakeSS := True;

    HTTP := TIdHTTP.Create(nil);
    try
      Response := HTTP.Get('http://plobex.pl/anticheat/check.php?uid=' + UID);
      if Response = 'w' then
        MakeSS := False;
    except
      MakeSS := False;
    end;

    if MakeSS then
    begin
      DirName := IncludeTrailingPathDelimiter(ExtractFilePath(SelfLocation)) + 'log\';
      sGameRunSec := IntToStr(PTAdynmemStruct(TAData.MainStructPtr)^.lGameTime);
      FileName := RemoveInvalidIncSpace(PlayerName) +
                  '#' + RemoveInvalidIncSpace(UserName) +
                  '#' + RemoveInvalidIncSpace(Date) +
                  '#' + DriveSerial +
                  '#' + sGameRunSec + '#';

      FileSearch(DirName, '*.pcx', '', True);

      //if TakeScreenshot(PAnsiChar(FileName), Pointer(PLongWord(LongWord(TAProgramStruct) + $BC)^)) = 1 then
      if TakeScreenshotOrg(PAnsiChar(DirName), PAnsiChar(FileName)) = 1 then
      begin
        PTAdynmemStruct(TAData.MainStructPtr)^.RandNum_ := GameRunSec;
        Sleep(5000);
        FinalFileName := FileSearch(DirName, '*.pcx', FileName, False);
        if (FinalFileName <> '') then
        begin
          FinalFileName := DirName + FinalFileName;
          if FileExists(FinalFileName) then
          begin
            SendSS(FinalFileName);
          end;
        end;
      end;
    end;
  end;
end;

procedure CheckForCheats(CheatType: TCheatType);
begin
  case CheatType of
    ctMemory: FindMemoryCheats;
    ctProcesses : FindProhibitedProcesses;
    ctScreenshot : SendScreenShot;
  end;
end; {CheckForCheats}

end.
