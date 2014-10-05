{

    The TA Launcher 1.x - Settings unit
    Copyright (C) 2013 Rime, N72

    e-mail: plobex@o2.pl

    Licensed under the terms stored in launcher-license.txt

}

unit settings;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, Registry, ComCtrls, ExtCtrls, Buttons, IniFiles, StrUtils;

type
  TLaunchrSettings = record
    LastMod     : Integer;
    ReplayerDir : String;
    AppDataDir  : String;
    Icons       : Boolean;
  end;
var LauncherSettings: TLaunchrSettings;

type
  TGameSett = record
    LaunchCommand: string;
    LaunchParam: string;
    DisplaymodeHeight: Cardinal;
    DisplaymodeWidth: Cardinal;

    INIPath: string;
    { Video Settings }
    INIDisplaymodeHeight: Integer;
    INIDisplaymodeWidth: Integer;
    RenderingMode: ShortInt;
    MenuResolution: ShortInt;
    SfxLimit: Integer;
    { Audio Settings }
    SoundMode: ShortInt;
    SoundsLimit: SmallInt;
    MusicMode: ShortInt;
    Music: ShortInt;
    { Gameplay Settings }
    UnitLimit: SmallInt;
    Pathfinding: Integer;
    MaxSkirmish: ShortInt;
    GameSpeed: ShortInt;
    { Interface Settings }
    MouseMode: ShortInt;
    DblClickSel: ShortInt;
    GroupSelMod: ShortInt;
    MultiShare: ShortInt;
    { Megamap Settings }
    Megamap: ShortInt;
    MouseZooming: ShortInt;
    MouseZoomIn: ShortInt;
    DblClickZoom: ShortInt;
  end;
var GameSettings: TGameSett;

type
  TfmSettings = class(TForm)
    btnCancel: TBitBtn;
    btnSaveOptions: TBitBtn;
    cbDisplayModes: TComboBox;
    lbGameResolution: TLabel;
    rgRenderingMode: TRadioGroup;
    rgMenuResolution: TRadioGroup;
    cbSfxLimit: TComboBox;
    lbSfxLimit: TLabel;
    gbVideoSettings: TGroupBox;
    gbAudioSettings: TGroupBox;
    cbSoundsLimit: TComboBox;
    lbSoundsLimit: TLabel;
    rgMusic: TRadioGroup;
    rgSoundMode: TRadioGroup;
    lbMusicMode: TLabel;
    cbMusicMode: TComboBox;
    gbGameplaySettings: TGroupBox;
    cbUnitLimit: TComboBox;
    lbUnitLimit: TLabel;
    lbPathfinding: TLabel;
    cbPathfinding: TComboBox;
    lbGameSpeed: TLabel;
    cbGameSpeed: TComboBox;
    cbMaxSkirmish: TComboBox;
    lbMaxSkirmish: TLabel;
    gbInterfaceSettings: TGroupBox;
    rgMouseMode: TRadioGroup;
    rgDblClickSel: TRadioGroup;
    rgGroupSelMod: TRadioGroup;
    rgMultiShare: TRadioGroup;
    gbMegamapSettings: TGroupBox;
    rgMegamap: TRadioGroup;
    rgMouseZooming: TRadioGroup;
    rgMouseZoomIn: TRadioGroup;
    rgDblClickZoom: TRadioGroup;
    gbHint: TGroupBox;
    lbHint: TLabel;
    procedure FormShow(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure btnSaveOptionsClick(Sender: TObject);
    procedure lbGameResolutionDblClick(Sender: TObject);
    procedure FormActivate(Sender: TObject);
    procedure FormDeactivate(Sender: TObject);
    procedure cbUnitLimitChange(Sender: TObject);
    procedure cbUnitLimitClick(Sender: TObject);
  private
    cbLastIndex: Integer;
    UnitLimitLock: boolean;
    SFXLock: boolean;
    UnitsOldHint, SFXOldHint: string;
    procedure ShowHint (Sender: TObject);
    procedure OnClickRenderingMode (Sender: TObject);
  public
    function FindResolutionOnList: integer;
    function GetINIFileName(Path: string): string;
    procedure LoadGamePatchSettings(id: integer; SettingsWindow: boolean);
  end;

  function StreamPos(Stream: TStream; Offset: int64; const Buffer; Length: int64; CaseSensitive: boolean = TRUE): int64;

const
  INIFILENAME_MAXLENGTH = 12;
  LABEL_CAN_NOT_SET = 'This setting should not be changed for the currently selected mod.';
var
  fmSettings: TfmSettings;
  TAIni: TIniFile;
  
implementation
uses ModsList, display, main;

{$R *.dfm}

procedure TfmSettings.ShowHint (Sender: TObject);
begin
  lbHint.Caption := Application.Hint;
  lbHint.Update;
end;

procedure TfmSettings.btnSaveOptionsClick(Sender: TObject);
var
  w,h: integer;
  s: string;
begin
  Screen.Cursor := crHourGlass;

  { Video Settings }
  s:= cbDisplayModes.Text;
  w:= StrToInt(Copy(s, 1, Pos('x', s)-1));
  h:= StrToInt(Copy(s, Pos('x', s)+1, Length(s)));
  GameSettings.DisplaymodeWidth:= w;
  GameSettings.DisplaymodeHeight:= h;
  
  case rgRenderingMode.ItemIndex of
   0: GameSettings.LaunchParam:= '';
   1: GameSettings.LaunchParam:= '-d';
  end;

  GameSettings.MenuResolution:= rgMenuResolution.ItemIndex;

  case cbSfxLimit.ItemIndex of
    0: GameSettings.SfxLimit:= 400;
    1: GameSettings.SfxLimit:= 20480;
  end;

  { Audio Settings }
  GameSettings.SoundMode := rgSoundMode.ItemIndex;

  if cbMusicMode.ItemIndex > -1 then
    GameSettings.MusicMode:= cbMusicMode.ItemIndex + 1;

  case cbSoundsLimit.ItemIndex of
    0: GameSettings.SoundsLimit:= 8;
    1: GameSettings.SoundsLimit:= 16;
    2: GameSettings.SoundsLimit:= 32;
    3: GameSettings.SoundsLimit:= 128;
  end;

  GameSettings.Music:= rgMusic.ItemIndex;

  { Gameplay Settings }
  case cbUnitLimit.ItemIndex of
    0: GameSettings.UnitLimit:= 250;
    1: GameSettings.UnitLimit:= 500;
    2: GameSettings.UnitLimit:= 1000;
    3: GameSettings.UnitLimit:= 1500;
    4: GameSettings.UnitLimit:= 5000;
  end;

  case cbPathfinding.ItemIndex of
    0: GameSettings.Pathfinding:= 1333;
    1: GameSettings.Pathfinding:= 15996;
    2: GameSettings.Pathfinding:= 33325;
    3: GameSettings.Pathfinding:= 66650;
  end;

  GameSettings.GameSpeed:= cbGameSpeed.ItemIndex;
  if cbMaxSkirmish.ItemIndex > -1 then
    GameSettings.MaxSkirmish:= cbMaxSkirmish.ItemIndex+2;

  { Interface Settings }
  GameSettings.MouseMode:= rgMouseMode.ItemIndex;
  GameSettings.DblClickSel:= rgDblClickSel.ItemIndex;
  GameSettings.GroupSelMod:= rgGroupSelMod.ItemIndex;
  GameSettings.MultiShare:= rgMultiShare.ItemIndex;

  { Megamap Settings }
  GameSettings.Megamap:= rgMegamap.ItemIndex;
  GameSettings.MouseZooming:= rgMouseZooming.ItemIndex;
  GameSettings.MouseZoomIn:= rgMouseZoomIn.ItemIndex;
  GameSettings.DblClickZoom:= rgDblClickZoom.ItemIndex;

end;

procedure TfmSettings.cbUnitLimitChange(Sender: TObject);
begin
  if cbUnitLimit.Text = '5000' then
    If MessageDlg('Warning: Unit limits higher than 1500 can cause instability on some machines.'+#10#13+
       'Do you want to continue ?', mtWarning, [mbYES,mbNO], 0) = ID_YES Then
         Exit
       else begin
         case GameSettings.UnitLimit of
         250: cbUnitLimit.ItemIndex:= 0;
         500: cbUnitLimit.ItemIndex:= 1;
         1000: cbUnitLimit.ItemIndex:= 2;
         1500: cbUnitLimit.ItemIndex:= 3;
         5000: cbUnitLimit.ItemIndex:= 4;
         else
           cbUnitLimit.ItemIndex:= 5;
         end;
       end;
end;

procedure TfmSettings.cbUnitLimitClick(Sender: TObject);
begin
  cbUnitlimit.Refresh;
  if Boolean (cbUnitLimit.Items.Objects [cbUnitLimit.ItemIndex])
    then cbUnitLimit.ItemIndex := cbLastIndex;
  cbLastIndex := cbUnitLimit.ItemIndex;
end;

function TfmSettings.FindResolutionOnList: integer;
begin
  Result:= cbDisplayModes.Items.IndexOf(
    IntToStr(GameSettings.DisplaymodeWidth) +
    'x' + IntToStr(GameSettings.DisplaymodeHeight));
end;

procedure TfmSettings.FormShow(Sender: TObject);

  procedure ResetGUI;
  var
    i : integer;
  begin
    cbDisplayModes.Enabled:= True;
    UnitLimitLock:= False;
    SFXLock:= False;
    if LoadedModsList[FindModID(LauncherSettings.LastMod)].UnitsLimit = '1' then
    begin
      cbUnitLimit.Enabled:= False;
      lbUnitLimit.Hint:= LABEL_CAN_NOT_SET;
      UnitLimitLock:= True;
    end else
    begin
      cbUnitLimit.Enabled:= True;
      lbUnitLimit.Hint:= UnitsOldHint;
    end;

    if LoadedModsList[FindModID(LauncherSettings.LastMod)].SFXLock = '1' then
    begin
      cbSfxLimit.Enabled:= False;
      lbSfxLimit.Hint:= LABEL_CAN_NOT_SET;
      SFXLock:= True;
    end else
    begin
      cbSfxLimit.Enabled:= True;
      lbSfxLimit.Hint:= SFXOldHint;
    end;
      
    try
      for i := 0 to ComponentCount - 1 do
      begin
        if Components[i] is TComboBox then
          TComboBox(Components[i]).ItemIndex := -1
        else
          if Components[i] is TRadioGroup then
            TRadioGroup(Components[i]).ItemIndex := -1;
      end;
    except
      on E : Exception do
      begin
        ShowMessage(E.Message);
        exit;
      end;
    end;
  end;

begin
 ResetGUI;

 Caption:= LoadedModsList[FindModId(LauncherSettings.LastMod)].Name + ' Settings';
 { Video Settings }
 if GameSettings.DisplaymodeHeight <> 0 then
   cbDisplayModes.ItemIndex:= FindResolutionOnList;

 if Integer(cbDisplayModes.Items.Objects[cbDisplayModes.Items.Count-1]) = 1 then
   cbDisplayModes.Items.Delete(cbDisplayModes.Items.Count-1);
 
 if GameSettings.INIDisplaymodeHeight > 0 then
 begin
   cbDisplayModes.AddItem(IntToStr(GameSettings.INIDisplaymodeWidth)+'x'+IntToStr(GameSettings.INIDisplaymodeHeight), TObject(1));
   cbDisplayModes.ItemIndex:= cbDisplayModes.Items.Count-1;
   cbDisplayModes.Enabled:= False;
   // ustaw hint ze double click odblokuje
 end;

 if GameSettings.LaunchParam = '-d' then
   rgRenderingMode.ItemIndex:= 1
 else
   rgRenderingMode.ItemIndex:= 0;

 rgRenderingMode.OnClick:= OnClickRenderingMode;

 rgMenuResolution.ItemIndex:= GameSettings.MenuResolution;

 if cbSfxLimit.Items.Count > 2 then cbSfxLimit.Items.Delete(2);
 case GameSettings.SfxLimit of
   400: cbSfxLimit.ItemIndex:= 0;
   20480: cbSfxLimit.ItemIndex:= 1;
   else begin
     cbSfxLimit.Items.Add('Custom ('+IntToStr(GameSettings.SfxLimit)+')');
     cbSfxLimit.ItemIndex:= 2;
   end;
 end;

 { Audio Settings }
 rgSoundMode.ItemIndex:= GameSettings.SoundMode;

 if GameSettings.MusicMode > -1 then
   cbMusicMode.ItemIndex:= GameSettings.MusicMode - 1;

 if cbSoundsLimit.Items.Count > 4 then cbSoundsLimit.Items.Delete(4);
 case GameSettings.SoundsLimit of
   8: cbSoundsLimit.ItemIndex:= 0;
   16: cbSoundsLimit.ItemIndex:= 1;
   32: cbSoundsLimit.ItemIndex:= 2;
   else begin
     if GameSettings.SoundsLimit > 32 then cbSoundsLimit.ItemIndex:= 3;
     if GameSettings.SoundsLimit < 32 then
     begin
       cbSoundsLimit.Items.Add('Custom ('+IntToStr(GameSettings.SoundsLimit)+')');
       cbSoundsLimit.ItemIndex:= 4;
     end;
   end;
 end;

 rgMusic.ItemIndex:= GameSettings.Music;

 { Gameplay Settings }
 if cbUnitLimit.Items.Count > 5 then cbUnitLimit.Items.Delete(5);
 case GameSettings.UnitLimit of
   250: cbUnitLimit.ItemIndex:= 0;
   500: cbUnitLimit.ItemIndex:= 1;
   1000: cbUnitLimit.ItemIndex:= 2;
   1500: cbUnitLimit.ItemIndex:= 3;
   5000: cbUnitLimit.ItemIndex:= 4;
   else begin
     cbUnitLimit.Items.Add('Custom ('+IntToStr(GameSettings.UnitLimit)+')');
     cbUnitLimit.ItemIndex:= 5;
   end;
 end;

 if UnitLimitLock then
   cbUnitLimit.ItemIndex:= 2;

 cbLastIndex:= cbUnitLimit.ItemIndex;

 if cbPathfinding.Items.Count > 4 then cbPathfinding.Items.Delete(4);
 case GameSettings.Pathfinding of
   1333: cbPathfinding.ItemIndex:= 0;
   15996: cbPathfinding.ItemIndex:= 1;
   33325: cbPathfinding.ItemIndex:= 2;
   66650: cbPathfinding.ItemIndex:= 3;
   else begin
     cbPathfinding.Items.Add('Custom ('+IntToStr(GameSettings.Pathfinding)+')');
     cbPathfinding.ItemIndex:= 4;
   end;
 end;

 case GameSettings.GameSpeed of
   0..20: cbGameSpeed.ItemIndex:= GameSettings.GameSpeed;
 end;

 if GameSettings.MaxSkirmish > -1 then
   cbMaxSkirmish.ItemIndex:= GameSettings.MaxSkirmish-2;

 { Interface Settings }
 rgMouseMode.ItemIndex:= GameSettings.MouseMode;
 rgDblClickSel.ItemIndex:= GameSettings.DblClickSel;
 rgGroupSelMod.ItemIndex:= GameSettings.GroupSelMod;
 rgMultiShare.ItemIndex:= GameSettings.MultiShare;

 { Megamap Settings }
 rgMegamap.ItemIndex:= GameSettings.Megamap;
 rgMouseZooming.ItemIndex:= GameSettings.MouseZooming;
 rgMouseZoomIn.ItemIndex:= GameSettings.MouseZoomIn;
 rgDblClickZoom.ItemIndex:= GameSettings.DblClickZoom;

end;

procedure TfmSettings.FormCreate(Sender: TObject);
begin
  if not LauncherSettings.Icons then
  begin
    btnSaveOptions.Glyph:= nil;
    btnSaveOptions.Margin:= -1;
    btnCancel.Glyph:= nil;
    btnCancel.Margin:= -1;
  end;
  EnumerateDisplayModes;
  UnitsOldHint:= lbUnitLimit.Hint;
  SFXOldHint:= lbSfxLimit.Hint;
  cbDisplayModes.Hint:= lbGameResolution.Hint;
  cbSfxLimit.Hint:= lbSfxLimit.Hint;
  cbUnitLimit.Hint:= lbUnitLimit.Hint;
  cbPathfinding.Hint:= lbPathfinding.Hint;
  cbGameSpeed.Hint:= lbGameSpeed.Hint;
  cbMaxSkirmish.Hint:= lbMaxSkirmish.Hint;
  cbMusicMode.Hint:= lbMusicMode.Hint;
  cbSoundsLimit.Hint:= lbSoundsLimit.Hint;
end;

procedure TfmSettings.FormDeactivate(Sender: TObject);
begin
  Application.OnHint := nil;
  Application.HintPause:= 500;
end;

procedure TfmSettings.FormActivate(Sender: TObject);
begin
  Application.OnHint := ShowHint;
  Application.HintPause:= 500000;
end;

procedure TfmSettings.FormClose(Sender: TObject; var Action: TCloseAction);

  procedure WriteIniVal(sect: string; ident: string; val: integer);
  begin
    if val <> -1 then
    begin
      TAIni.WriteString(sect, ident, ' ' + IntToStr(val)+';');
    end;
  end;

  procedure WriteIniBool(sect: string; ident: string; val: shortint);
  begin
    case val of
      0: TAIni.WriteString(sect, ident, ' FALSE;');
      1: TAIni.WriteString(sect, ident, ' TRUE;');
    end;
  end;

  procedure WriteIniDword(sect: string; ident: string; val: integer);
  begin
    if val <> -1 then
    begin
      TAIni.WriteString(sect, ident, ' dword:' + IntToStr(val));
    end;
  end;

var
  reg: TRegistry;
  subKey: string;
begin
  rgRenderingMode.OnClick:= nil;
  if ModalResult = mrOk then
  begin
    reg:= TRegistry.Create(KEY_READ);
    try
      reg.RootKey := HKEY_CURRENT_USER;
      subKey:= 'Software\'+LoadedModsList[fmLauncherMain.cbModsList.ItemIndex].RegName+'\Total Annihilation';
      reg.Access := KEY_WRITE;
      if reg.OpenKey(subKey, True) then
      begin
        reg.WriteString('LaunchParam', GameSettings.LaunchParam);
        { Video Settings }
        reg.WriteInteger('DisplaymodeHeight', GameSettings.DisplaymodeHeight);
        reg.WriteInteger('DisplaymodeWidth', GameSettings.DisplaymodeWidth);
        { Audio Settings }
        reg.WriteInteger('musicmode', GameSettings.Music);
        { Interface Settings }
        if GameSettings.MouseMode <> -1 then
          reg.WriteInteger('Interface Type', GameSettings.MouseMode);
      end;
      reg.CloseKey;
     finally
       reg.Free;
     end;

    TAIni:= TiniFile.Create(GameSettings.INIPath);
    try
      { Video Settings }
      if GameSettings.INIDisplaymodeHeight = 0 then
      begin
        TAIni.DeleteKey('REG', '"DisplayModeWidth"');
        TAIni.DeleteKey('REG', '"DisplayModeHeight"');
      end;
      WriteIniVal('Preferences', 'SfxLimit', GameSettings.SfxLimit);
      WriteIniBool('Preferences', 'MenuResolution', GameSettings.MenuResolution);
      { Audio Settings }
      WriteIniDword('REG', '"Sound Mode"', GameSettings.SoundMode);
      WriteIniDword('REG', '"CDMode"', GameSettings.MusicMode);
      WriteIniDword('REG', '"MixingBuffers"', GameSettings.SoundsLimit);
      { Gameplay Settings }
      WriteIniVal('Preferences', 'UnitLimit', GameSettings.UnitLimit);
      WriteIniVal('Preferences', 'AISearchMapEntries', GameSettings.Pathfinding);
      WriteIniDword('REG', '"NumSkirmishPlayers"', GameSettings.MaxSkirmish);
      WriteIniDword('REG', '"GameSpeed"', GameSettings.GameSpeed);
      { Interface Settings }
      WriteIniBool('Preferences', 'DoubleClick', GameSettings.DblClickSel);
      WriteIniDword('REG', '"SwitchAlt"', GameSettings.GroupSelMod);
      WriteIniBool('Preferences', 'ShareDialogExpand', GameSettings.MultiShare);
      { Megamap Settings }
      WriteIniBool('Preferences', 'FullScreenMinimap', GameSettings.Megamap);
      WriteIniBool('Preferences', 'WheelZoom', GameSettings.MouseZooming);
      WriteIniBool('Preferences', 'WheelMoveMegaMap', GameSettings.MouseZoomIn);
      WriteIniBool('Preferences', 'DoubleClickMoveMegamap', GameSettings.DblClickZoom);
    finally
      TAIni.Free;
      Screen.Cursor := crDefault;
    end;
  end;
end;

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

// search INI file name from TA's EXE
// offset isn't constant! :/
function TfmSettings.GetINIFileName(Path: string): string;
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

procedure TfmSettings.lbGameResolutionDblClick(Sender: TObject);
begin
  if Integer(cbDisplayModes.Items.Objects[cbDisplayModes.Items.Count-1]) = 1 then
  begin
    If MessageDlg('You are about to disable custom resolution setting.'+#10#13+
       'Do you want to continue ?', mtWarning, [mbYES,mbNO], 0) = ID_YES then
    begin
      cbDisplayModes.Items.Delete(cbDisplayModes.Items.Count-1);
      cbDisplayModes.ItemIndex:= FindResolutionOnList;
      GameSettings.INIDisplaymodeHeight:= 0;
      GameSettings.INIDisplaymodeWidth:= 0;
      cbDisplayModes.Enabled:= True;
    end;
  end;
end;

procedure TfmSettings.LoadGamePatchSettings(id: integer; SettingsWindow: boolean);

  function ReadIniBool(sect: string; ident: string): ShortInt;
  var
    temp: string;
  begin
    Result:= -1;
    if TAIni.ValueExists(sect, ident) then
    begin
      temp:= TAIni.ReadString(sect, ident, '');
      if Length(temp) >= 1 then
      begin
        if Pos('FALSE', temp) <> 0 then
          Result:= 0;
        if Pos('TRUE', temp) <> 0 then
          Result:= 1;
      end else
      begin
        ShowMessage('Error reading setting [' + ident + ']');
        Exit;
      end;
    end;
  end;

  function ReadIniValue(sect: string; ident: string): Integer;
  var
    temp: string;
  begin
    Result:= -1;
    if TAIni.ValueExists(sect, ident) then
    begin
      temp:= TAIni.ReadString(sect, ident, '');
      if Length(temp) >= 1 then
      begin
        try
          if Pos(';', temp) <> 0 then
            temp:= LeftStr(temp, Length(temp) - 1);
          Trim(temp);
          Result:= StrToInt(temp);
        except
          ShowMessage('Error reading setting [' + ident + ']');
          Exit;
        end;
      end;
    end;
  end;

  function ReadIniDword(sect: string; ident: string): Integer;
  var
    temp: string;
  begin
    Result:= -1;
    if TAIni.ValueExists(sect, ident) then
    begin
      temp:= TAIni.ReadString(sect, ident, '');
      if Length(temp) >= 1 then
      begin
        try
          Trim(temp);
          temp:= Copy(temp, Pos(':', temp) + 1, Length(temp) - Pos(':', temp));
          Result:= StrToInt(temp);
        except
          ShowMessage('Error reading setting [' + ident + ']');
          Exit;
        end;
      end;
    end;
  end;

var
   reg: TRegistry;
   subKey: string;
begin
  reg:= TRegistry.Create(KEY_READ);
  try
    reg.RootKey := HKEY_CURRENT_USER;
    subKey:= 'Software\'+LoadedModsList[id].RegName+'\Total Annihilation';
    if (not reg.KeyExists(subKey)) then
    begin
      reg.Access := KEY_WRITE;
      if reg.OpenKey(subKey, True) then
      begin
        if (not SettingsWindow) then
        begin
          reg.WriteString('LaunchParam', GameSettings.LaunchParam);
        end;
      end;
    end else
    begin
      reg.Access := KEY_READ;
      if reg.OpenKey(subKey, False) then
      begin
        if (not SettingsWindow) then
        begin
          GameSettings.LaunchParam:= reg.ReadString('LaunchParam');
        end else
        begin
          { Video Settings }
          if reg.ValueExists('DisplaymodeHeight') then
            GameSettings.DisplaymodeHeight:= reg.ReadInteger('DisplaymodeHeight');
          if reg.ValueExists('DisplaymodeWidth') then
            GameSettings.DisplaymodeWidth:= reg.ReadInteger('DisplaymodeWidth');
          { Audio Settings }
          if reg.ValueExists('musicmode') then
            GameSettings.Music:= reg.ReadInteger('musicmode');
          { Interface Settings }
          if reg.ValueExists('Interface Type') then
            GameSettings.MouseMode:= reg.ReadInteger('Interface Type');
        end;
      end;
    end;
    Reg.CloseKey;
  finally
    reg.Free;
  end;

  GameSettings.LaunchCommand:= LoadedModsList[id].Path;
  if SettingsWindow then
  begin
    if FileExists(LoadedModsList[id].Path) then
    begin
      GameSettings.INIPath:= fmSettings.GetINIFileName(LoadedModsList[id].Path);
      if GameSettings.INIPath <> #0 then
      begin
        TAIni:= TIniFile.Create(GameSettings.INIPath);
        try
          { Video Settings }
          GameSettings.INIDisplaymodeHeight:= ReadIniDword('REG', '"DisplayModeHeight"');
          GameSettings.INIDisplaymodeWidth:= ReadIniDword('REG', '"DisplayModeWidth"');
          GameSettings.MenuResolution:= ReadIniBool('Preferences', 'MenuResolution');
          GameSettings.SfxLimit:= ReadIniValue('Preferences', 'SfxLimit');
          { Audio Settings }
          GameSettings.SoundMode:= ReadIniDword('REG', '"Sound Mode"');
          GameSettings.MusicMode:= ReadIniDword('REG', '"CDMode"');
          GameSettings.SoundsLimit:= ReadIniDword('REG', '"MixingBuffers"');
          { Gameplay Settings }
          GameSettings.UnitLimit:= ReadIniValue('Preferences', 'UnitLimit');
          GameSettings.Pathfinding:= ReadIniValue('Preferences', 'AISearchMapEntries');
          GameSettings.MaxSkirmish:= ReadIniDword('REG', '"NumSkirmishPlayers"');
          GameSettings.GameSpeed:= ReadIniDword('REG', '"GameSpeed"');
          { Interface Settings }
          GameSettings.DblClickSel:= ReadIniBool('Preferences', 'DoubleClick');
          GameSettings.GroupSelMod:= ReadIniDword('REG', '"SwitchAlt"');
          GameSettings.MultiShare:= ReadIniBool('Preferences', 'ShareDialogExpand');
          { Megamap Settings }
          GameSettings.Megamap:= ReadIniBool('Preferences', 'FullScreenMinimap');
          GameSettings.MouseZooming:= ReadIniBool('Preferences', 'WheelZoom');
          GameSettings.MouseZoomIn:= ReadIniBool('Preferences', 'WheelMoveMegaMap');
          GameSettings.DblClickZoom:= ReadIniBool('Preferences', 'DoubleClickMoveMegamap');
        finally
          TAIni.Free;
          fmSettings.ShowModal;
        end;
      end else
        ShowMessage('Error: Couldn''t read ini file !');
    end;
  end;

end;

procedure TfmSettings.OnClickRenderingMode(Sender: TObject);
begin
  if (rgRenderingMode.ItemIndex = 1) then
  begin
    If MessageDlg('Warning: Windowed mode does not support many new patch features such as:'+#10#13+
       'the megamap, whiteboard, line building, and multiplayer allied resource bars.'+#10#13+
       'Do you want to continue ?', mtWarning, [mbYES,mbNO], 0) = ID_YES Then
        Exit
       else
        rgRenderingMode.ItemIndex:= 0;
  end;
end;

end.
