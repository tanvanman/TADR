program modstool;

{$R *.res}

uses
  SysUtils, INIFiles, Classes, StrUtils, Windows, SHFolder;


type
TModAction = (maNone, maAdd, maUpdate, maRemove);

TParams = record
  Func          : TModAction;
  ID            : string;
  Name          : string;
  Version       : string;
  Path          : string;
  RegName       : string;
  SFXLock       : string;
  UnitLimitLock : string;
end;
var Parameters  : TParams;

const
  MODSINI = 'mods.ini';
  INIFILENAME_MAXLENGTH = 12;

var
  appdatapath   : array[0..MAX_PATH] of char;
  appDataDir    : string;
  TotalaIniName : string;
  TotalaIni     : TIniFile;
  modsIniFile   : TIniFIle;
  i             : integer;
  currPar       : string;
  sectionName   : string;

function StreamPos(Stream: TStream; Offset: int64; const Buffer; Length: int64; CaseSensitive: boolean = TRUE): int64;

  function _Compare(const s1, s2: string;
    Index1, Index2, Len: integer): boolean;
  var
    i : integer;
  begin
    i := 0;
    repeat
      result := s1[Index1 +i] = s2[Index2 +i];
      Inc(i);
    until (i >= Len) or not result;
  end;

var
  target, buf : string;
  buflen, red, n : integer;
begin
  result := -1;

  if Offset < 0 then
    Offset := 0;

  if (Length > 0) and (Length <= Stream.Size -Offset) then
  begin
    SetLength(target, Length);
    MoveMemory(@target[1], @Buffer, Length);

    if not CaseSensitive then
      target := AnsiLowerCase(target);

    if Length -1 > $7FFF then
    begin
      if Length -1 > $FFFF then
        buflen := Length +1
      else
        buflen := $FFFF;
    end else
      buflen := $7FFF;

  SetLength(buf, buflen);

  Stream.Position := Offset;
  red := Stream.Read(buf[1], buflen);
  while (red > Length -1) and (result < 0) do
  begin
    if red < buflen then
    SetLength(buf, red);

    if not CaseSensitive then
      buf := AnsiLowerCase(buf);

    n := Pos(target, buf);

    if n > 0 then
    begin
      result := Stream.Position -red +n -1;
    end else
    begin
      if red > Length then
      begin
        n := red -Length;
        repeat
          Inc(n);
        until (n > red) or
              ((buf[n] = target[1]) and (buf[red] = target[red -n +1]) and
              _Compare(buf, target, n, 1, red -n +1 ));

        if (n <= red) and (buf[n] = target[1]) then
          Stream.Seek( -(red -n +1), soFromCurrent);
      end;

    end;
      red := Stream.Read(buf[1], buflen);
    end;
  end;
end;

function GetINIFileName(Path: string): string;
var
 iniName: string;
 TAExe: TFileStream;
 tadir: string;
 address: Int64;
 buffer: array of Byte;
 s: AnsiString;
begin
  Result:= #0;
  TAExe:= TFileStream.Create(Path, fmOpenRead or fmShareDenyNone);
  tadir:= ExtractFilePath(Path);
  s:= 'Preferences'+#0+'%s\';
  address := StreamPos(TAExe, 0, s[1], Length(s));
  try
    try
      if address > -1 then
      begin
        SetLength(Buffer, INIFILENAME_MAXLENGTH + 1);
        TAExe.Position:= address + Length(s);
        TAExe.ReadBuffer(Buffer[0], INIFILENAME_MAXLENGTH);
      end;
      SetString(ininame, PAnsiChar(@Buffer[0]), INIFILENAME_MAXLENGTH);
      Trim(ininame);
      if FileExists(tadir + ininame) then
          Result:= tadir + iniName
          else Result:= #0;
    except
      //shouldn't fail, however ...
      if FileExists(tadir + 'totala.ini') then
        Result:= tadir + 'totala.ini' else Result:= #0;
    end;
  finally
    TAExe.Free;
  end;
end;

function CopyParamStr: string;
begin
  Result:= Copy(ParamStr(i), 4, Length(ParamStr(i))-3);
end;

begin
  try
    //SHGetFolderPath(0, CSIDL_COMMON_APPDATA, 0, 0, @appdatapath);
    SHGetFolderPath(0, CSIDL_LOCAL_APPDATA, 0, 0, @appdatapath);
    appDataDir:= appdatapath;
    appDataDir:= IncludeTrailingPathDelimiter(appDataDir) + 'TADR\';
    if not DirectoryExists(appDataDir) then
      if not CreateDir(appDataDir) then
        Exit;
  except
    Exit;
  end;

  if appDataDir = '' then
    Exit;

  if FindCmdLineSwitch('add', False) then Parameters.Func := maAdd;
  if FindCmdLineSwitch('update', False) then Parameters.Func := maUpdate;
  if FindCmdLineSwitch('remove', False) then Parameters.Func := maRemove;

  if (Parameters.Func = maAdd) or (Parameters.Func = maUpdate) then
  begin
    for i:= 2 to ParamCount do
    begin
      currPar:= Copy(ParamStr(i), 2, 1);
      // id, name, version, path, regname, sfx lock, unit limit lock
      if currPar = 'i' then Parameters.ID:= CopyParamStr;
      if currPar = 'n' then Parameters.Name:= CopyParamStr;
      if currPar = 'v' then Parameters.Version:= CopyParamStr;
      if currPar = 'p' then Parameters.Path:= CopyParamStr;
      if currPar = 'r' then Parameters.RegName:= CopyParamStr;
      if currPar = 's' then Parameters.SFXLock:= CopyParamStr;
      if currPar = 'u' then Parameters.UnitLimitLock:= CopyParamStr;
    end;
  end;

  if (Parameters.Func = maRemove) then
  begin
    for i:= 2 to ParamCount do
    begin
      currPar:= Copy(ParamStr(i), 2, 1);
      if currPar = 'i' then
        Parameters.ID:= CopyParamStr;
    end;
  end;

  if Parameters.Func <> maNone then
  begin
    modsIniFile := TIniFile.Create(appDataDir + MODSINI);
    sectionName := 'MOD' + Parameters.ID;
    try
      // cleanup mod section when adding/removing
      if (Parameters.Func = maAdd) or (Parameters.Func = maRemove) then
      begin
        if modsIniFile.SectionExists(sectionName) then
          modsIniFile.EraseSection(sectionName);
      end;
      // write/update fields
      if (Parameters.Func = maAdd) or (Parameters.Func = maUpdate) then
      begin
        if Parameters.ID <> '' then modsIniFile.WriteString(sectionName, 'ID', Parameters.ID);
        if Parameters.Name <> '' then modsIniFile.WriteString(sectionName, 'Name', Parameters.Name);
        if Parameters.Version <> '' then modsIniFile.WriteString(sectionName, 'Version', Parameters.Version);
        if Parameters.Path <> '' then modsIniFile.WriteString(sectionName, 'Path', Parameters.Path);
        if Parameters.RegName <> '' then modsIniFile.WriteString(sectionName, 'RegName', Parameters.RegName);
        if Parameters.SFXLock <> '' then modsIniFile.WriteString(sectionName, 'SFXLock', Parameters.SFXLock);
        if Parameters.UnitLimitLock <> '' then modsIniFile.WriteString(sectionName, 'UnitLimitLock', Parameters.UnitLimitLock);
      end;

      if Parameters.Path <> '' then
        if FileExists(Parameters.Path) then
        begin
          TotalaIniName := GetINIFileName(Parameters.Path);
          TotalaIni := TIniFile.Create(TotalaIniName);
          case Parameters.Func of
            maAdd..maUpdate :
            begin
              TotalaIni.WriteString('MOD', 'ID', Parameters.ID);
              if Parameters.RegName <> '' then
                TotalaIni.WriteString('Preferences', 'RegistryName', Parameters.RegName +';');
            end;
            maRemove :
              TotalaIni.EraseSection('MOD');
          end;
        end;
    finally
      modsIniFile.Free;
    end;
  end;

end.
