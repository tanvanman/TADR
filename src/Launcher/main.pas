{

    The TA Launcher 1.x - Main unit
    Copyright (C) 2013 Rime, N72

    e-mail: plobex@o2.pl

    Licensed under the terms stored in launcher-license.txt

}

unit main;

interface

uses
  Windows, SysUtils, Classes, Controls, Forms, StdCtrls, Messages,
  INIFiles, Registry, Dialogs, ShFolder, ShellAPI, ExtCtrls, ImgList, Graphics, XPMan, CommCtrl, VistaAltFixUnit,
  Buttons;

type
  TfmLauncherMain = class(TForm)
    cbModsList: TComboBox;
    ilModsIcons: TImageList;
    XPManifest1: TXPManifest;
    btnLaunch: TBitBtn;
    btnGameranger: TBitBtn;
    btnWarzone: TBitBtn;
    btnReplayer: TBitBtn;
    btnSettings: TBitBtn;
    btnUpdate: TBitBtn;
    btnHelp: TBitBtn;
    btnExit: TBitBtn;
    trShowHelp: TTimer;
    procedure btnLaunchClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure btnSettingsClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure btnExitClick(Sender: TObject);
    procedure cbModsListChange(Sender: TObject);
    procedure btnUpdateClick(Sender: TObject);
    procedure btnReplayerClick(Sender: TObject);
    procedure btnWarZoneClick(Sender: TObject);
    procedure btnGamerangerClick(Sender: TObject);
    procedure btnHelpClick(Sender: TObject);
    procedure trShowHelpTimer(Sender: TObject);
  private
    procedure WMSysCommand(var Msg: TWMSysCommand) ; message WM_SYSCOMMAND;
  public
    //Options = launcher settings for main form position and last used mod
    function LoadOptions(out errornumber: integer): boolean;
    procedure SaveOptions;
    procedure FillModsListBox;
    procedure SetUIForMod(id: integer);
    procedure TryToLaunch(progid: integer);
  end;

const
  SC_ABOUT = WM_USER + 1;
  
var
  fmLauncherMain: TfmLauncherMain;
  runningDir: string;
  indexoffset: integer;
implementation

uses settings, modslist, display, download, help, about;

{$R *.dfm}

function GetTempDirectory: String;
var
  tempFolder: array[0..MAX_PATH] of Char;
begin
  GetTempPath(MAX_PATH, @tempFolder);
  result := IncludeTrailingPathDelimiter(StrPas(tempFolder));
end;

function TfmLauncherMain.LoadOptions(out errornumber: integer): boolean;
var
  reg: TRegistry;
  subKey: string;
  appdatapath   : array[0..MAX_PATH] of char;
begin
  Result:= False;

  errornumber:= 1;
  reg:= TRegistry.Create(KEY_READ);
  reg.RootKey := HKEY_CURRENT_USER;
  try
    try
      subKey:= 'Software\TA Patch\Launcher';
      reg.Access := KEY_READ;
      if reg.OpenKey(subKey, false) then
      begin
        LauncherSettings.LastMod:= reg.ReadInteger('LastMod');
        LauncherSettings.Icons:= reg.ReadBool('UseIcons');
      end;
    except
      LauncherSettings.LastMod:= -1;
    end;
  finally
    Reg.CloseKey;
  end;

  errornumber:= 2;
  try
    subKey:= 'Software\TA Patch\TA Demo\Options';
    if (not reg.KeyExists(subKey)) then
    begin
      Exit;
    end else
    begin
      reg.Access := KEY_READ;
      if reg.OpenKey(subKey, false) then
      begin
        LauncherSettings.ReplayerDir:= reg.ReadString('serverdir');
        errornumber:= 3;
        if LauncherSettings.ReplayerDir = '' then
          Exit;
      end;
    end;
    Reg.CloseKey;
  finally
    reg.Free;
  end;

  try
    //SHGetFolderPath(0, CSIDL_COMMON_APPDATA, 0, 0, @appdatapath);
    SHGetFolderPath(0, CSIDL_LOCAL_APPDATA, 0, 0, @appdatapath);
    LauncherSettings.AppDataDir:= appdatapath;
    LauncherSettings.AppDataDir:= IncludeTrailingPathDelimiter(LauncherSettings.AppDataDir) + 'TADR\';
    if not DirectoryExists(LauncherSettings.AppDataDir) then
      if not CreateDir(LauncherSettings.AppDataDir) then
        Exit;
  except
    Exit;
  end;

  Result:= True;
end;

procedure TfmLauncherMain.SaveOptions;
var
  reg: TRegistry;
  subKey: string;
begin
  reg:= TRegistry.Create(KEY_WRITE);
  try
    reg.RootKey := HKEY_CURRENT_USER;
    subKey:= 'Software\TA Patch\Launcher';
    reg.Access := KEY_WRITE;
    if reg.OpenKey(subKey, true) then
    begin
      reg.WriteInteger('LastMod', LauncherSettings.LastMod);
      if not reg.KeyExists('UseIcons') then
        reg.WriteBool('UseIcons', LauncherSettings.Icons);
    end;
    Reg.CloseKey;
  finally
    reg.Free;
  end;
end;

procedure TfmLauncherMain.btnLaunchClick(Sender: TObject);
begin
  SaveOptions;
  ShellExecute (0, nil, @GameSettings.LaunchCommand[1], @GameSettings.LaunchParam[1], nil, SW_NORMAL);
end;

procedure TfmLauncherMain.btnReplayerClick(Sender: TObject);
var
  path: string;
  param: string;
begin
  path:= IncludeTrailingPathDelimiter(LauncherSettings.ReplayerDir)+'server.exe';
  param:= '-m:' + IntToStr(LauncherSettings.LastMod);
  if FileExists(path) then
    ShellExecute (0, nil, @path[1], @param[1], nil, SW_NORMAL)
  else
    ShowMessage('Replayer not found !');
end;

procedure TfmLauncherMain.FormClose(Sender: TObject;
  var Action: TCloseAction);
begin
  SaveOptions;
end;

procedure TfmLauncherMain.btnSettingsClick(Sender: TObject);
begin
  fmSettings.LoadGamePatchSettings(cbModsList.ItemIndex, true);
end;

procedure TfmLauncherMain.FillModsListBox;
var
  Icon: TIcon;
  FileInfo: SHFILEINFO;
  i: integer;
begin
  cbModsList.Items.Clear;
  for i:= Low(LoadedModslist) to High(LoadedModslist) do
  begin
    icon:= TIcon.Create;
    try
    if SHGetFileInfo(PChar(LoadedModsList[i].Path), 0, FileInfo, SizeOf(FileInfo), SHGFI_ICON) <> 0 then
    begin
      icon.Handle := FileInfo.hIcon;
      ilModsIcons.AddIcon(icon);
      LoadedModsList[i].IconIndex[0]:= ilModsIcons.Count - 1;
    end else
      LoadedModsList[i].IconIndex[0]:= -1;

    if SHGetFileInfo(PChar(LoadedModsList[i].Path), 0, FileInfo, SizeOf(FileInfo), SHGFI_ICON or SHGFI_SMALLICON) <> 0 then
    begin
      icon.Handle := FileInfo.hIcon;
      ilModsIcons.AddIcon(icon);
      LoadedModsList[i].IconIndex[1]:= ilModsIcons.Count - 1;
    end else
      LoadedModsList[i].IconIndex[1]:= -1;

    finally
      icon.Free;
    end;
    cbModsList.Items.BeginUpdate;
    cbModsList.Items.Add(LoadedModsList[i].Name + ' ' +LoadedModsList[i].Version);
    cbModsList.Items.EndUpdate;
  end;
end;

procedure TfmLauncherMain.FormCreate(Sender: TObject);
const
   sAbout = 'About...';
var
  errNr: integer;
  paramType: string;
  SysMenu : HMenu;
  i: integer;
begin
  TVistaAltFix.Create(Self);
  runningDir:= IncludeTrailingPathDelimiter(ExtractFilePath(ParamStr(0)));
  if not LoadOptions(errNr) then
  begin
    ShowMessage('Couldn''t load settings file. Exiting. (' + IntToStr(errNr)+')');
    Application.Terminate;
  end else
  begin
    if not LoadModsList(errNr) then
    begin
      ShowMessage('Couldn''t load mods list. Exiting. (' + IntToStr(errNr)+')');
      Application.Terminate;
    end else
    begin
    { main create/run }
    if not LauncherSettings.Icons then
    begin
      for i := 0 to fmLauncherMain.ComponentCount-1 do
      begin
        if fmLauncherMain.Components[i] is TBitBtn then
          (fmLauncherMain.Components[i] as TBitbtn).Glyph:= nil;
      end;
    end;
    FillModsListBox;
    if FindModID(LauncherSettings.LastMod) <> -1 then
      cbModsList.ItemIndex:= FindModID(LauncherSettings.LastMod)
    else
      cbModsList.ItemIndex:= 0;

    paramType:= Copy(ParamStr(1), 2, 1);

    if paramType = '' then
    begin
      LauncherSettings.LastMod:= LoadedModsList[cbModsList.ItemIndex].ID;
      SetUIForMod(cbModsList.ItemIndex);
      fmSettings.LoadGamePatchSettings(cbModsList.ItemIndex, false);
      Show;
      SysMenu := GetSystemMenu(Handle, FALSE) ;
      AppendMenu(SysMenu, MF_SEPARATOR, 0, '') ;
      AppendMenu(SysMenu, MF_STRING, SC_ABOUT, sAbout) ;
      Exit;
    end;

    paramType:= UpperCase(paramType);
    
    // launch specific mod
    // -L:xxx
    if paramType = 'L' then
    begin
      if FindModID(StrToInt(Copy(ParamStr(1), 4, Length(ParamStr(1))-3))) <> -1 then
      begin
        paramType:= Copy(ParamStr(1), 4, Length(ParamStr(1))-3);
        fmSettings.LoadGamePatchSettings(FindModID(StrToInt(paramType)), false);
        ShellExecute (0, nil, @GameSettings.LaunchCommand[1], @GameSettings.LaunchParam[1], nil, SW_NORMAL);
        Application.Terminate;
      end else
      begin
        showmessage('Mod [' + Copy(ParamStr(1), 4, Length(ParamStr(1))-3) + '] not found !');
        LauncherSettings.LastMod:= LoadedModsList[cbModsList.ItemIndex].ID;
        SetUIForMod(cbModsList.ItemIndex);
        fmSettings.LoadGamePatchSettings(cbModsList.ItemIndex, false);
        Show;
      end;
    end;

    // show specific mod info
    // -H:xxx
    if paramType = 'H' then
    begin
      if FindModID(StrToInt(Copy(ParamStr(1), 4, Length(ParamStr(1))-3))) <> -1 then
      begin
        paramType:= Copy(ParamStr(1), 4, Length(ParamStr(1))-3);
        LauncherSettings.LastMod:= LoadedModsList[cbModsList.ItemIndex].ID;
        SetUIForMod(cbModsList.ItemIndex);
        fmSettings.LoadGamePatchSettings(FindModID(StrToInt(paramType)), false);
        trShowHelp.Enabled:= True;
      end else
      begin
        showmessage('Mod [' + Copy(ParamStr(1), 4, Length(ParamStr(1))-3) + '] not found !');
        Application.Terminate;
      end;
    end;
    end; { end main create }
  end;
end;

procedure TfmLauncherMain.WMSysCommand(var Msg : TWMSysCommand) ;
begin
  if Msg.CmdType = SC_ABOUT then
  begin
    fmAbout.ShowModal;
  end else
    inherited;
end;

procedure TfmLauncherMain.btnExitClick(Sender: TObject);
begin
  Close;
end;

procedure TfmLauncherMain.cbModsListChange(Sender: TObject);
begin
  LauncherSettings.LastMod:= LoadedModsList[cbModsList.ItemIndex].ID;
  SetUIForMod(cbModsList.ItemIndex);
  fmSettings.LoadGamePatchSettings(cbModsList.ItemIndex, false);
end;

procedure TfmLauncherMain.SetUIForMod(id: integer);
var
  osVerInfo: TOSVersionInfo;
  majorVer: Integer;
  Path: string;
  reg : TRegistry;
  subKey: string;
  smallicons: boolean;
begin
  smallicons := False;
  osVerInfo.dwOSVersionInfoSize := SizeOf(TOSVersionInfo);
  if GetVersionEx(osVerInfo) then
  begin
    majorVer := osVerInfo.dwMajorVersion;
    // draw huge icon in windows vista+ for app and small for form
    // small for app if "use small icons" is set in explorer settings
    if (majorVer > 5) then
    begin

     reg:= TRegistry.Create(KEY_READ);
     try
       try
         reg.RootKey := HKEY_CURRENT_USER;
         subKey:= 'Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced';
         if reg.KeyExists(subKey) then
         begin
           reg.Access := KEY_READ;
           if reg.OpenKey(subKey, false) then
           begin
             if reg.ReadInteger('TaskbarSmallIcons') = 1 then
             smallicons := True
           else
             smallicons := False;
           end;
         end;
         Reg.CloseKey;
        except
          smallicons := False;
        end;
     finally
       reg.Free;
     end;

      if (not smallicons) then
      begin
        if LoadedModsList[id].IconIndex[0] <> - 1 then
          ilModsIcons.GetIcon(LoadedModsList[id].IconIndex[0], Application.Icon);
        if LoadedModsList[id].IconIndex[1] <> - 1 then
          ilModsIcons.GetIcon(LoadedModsList[id].IconIndex[1], fmLauncherMain.Icon);
      end else
      begin
        if LoadedModsList[id].IconIndex[1] <> - 1 then
          ilModsIcons.GetIcon(LoadedModsList[id].IconIndex[1], Application.Icon);
      end;
    end else
    begin
    // draw small icon in older win
      if LoadedModsList[id].IconIndex[1] <> - 1 then
        ilModsIcons.GetIcon(LoadedModsList[id].IconIndex[1], Application.Icon);
    end;
  end else
  begin
    // unknown win, draw small icon
      if LoadedModsList[id].IconIndex[1] <> - 1 then
        ilModsIcons.GetIcon(LoadedModsList[id].IconIndex[1], Application.Icon);
  end;

  Application.Title:= LoadedModsList[id].Name;
  fmLauncherMain.Caption:= Application.Title;

  Path:= IncludeTrailingPathDelimiter(ExtractFilePath(LoadedModsList[id].Path))+'updater.exe';
  if FileExists(Path) then
  begin
    LoadedModsList[id].UpdaterPath := Path;
    btnUpdate.Margin:= -1;
    btnUpdate.Enabled:= True;
  end else
  begin
    if LauncherSettings.Icons then btnUpdate.Margin:= 65;
    btnUpdate.Enabled:= False;
  end;

  Path:= IncludeTrailingPathDelimiter(ExtractFilePath(LoadedModsList[id].Path))+'modinfo.cfg';
  if FileExists(Path) then
  begin
    LoadedModsList[id].InfoPath := Path;
    btnHelp.Margin:= -1;
    btnHelp.Enabled:= True;
  end else
  begin
    if LauncherSettings.Icons then btnHelp.Margin:= 100;
    btnHelp.Enabled:= False;
  end;
end;

procedure TfmLauncherMain.trShowHelpTimer(Sender: TObject);
begin
  trShowHelp.Enabled:= False;
  fmHelp.ShowModal;
  Application.Terminate;
end;

procedure TfmLauncherMain.btnUpdateClick(Sender: TObject);
var
  Path: string;
begin
  Path:= LoadedModsList[cbModsList.ItemIndex].UpdaterPath;
  ShellExecute (0, nil, @Path[1], nil, nil, SW_NORMAL);
end;

procedure TfmLauncherMain.btnGamerangerClick(Sender: TObject);
begin
  TryToLaunch(1);
end;

procedure TfmLauncherMain.btnHelpClick(Sender: TObject);
begin
  fmHelp.ShowModal;
end;

procedure TfmLauncherMain.btnWarZoneClick(Sender: TObject);
begin
  TryToLaunch(2);
end;

procedure TfmLauncherMain.TryToLaunch(progid: integer);
  procedure DownloadAndLaunch(confmssg: string);
  begin
    if MessageDlg(
        confmssg+
        ' is a multiplayer client that can be used to play Total Annihilation online. Do you want to download and install it?',
        mtConfirmation, [mbYes, mbNo], 0 ) = mrYes then
    if fmDownload.ShowModal = mrOk then
      ShellExecute (0, nil, @Downloader.sFileName[1], nil, nil, SW_NORMAL);
  end;
var
   reg: TRegistry;
   subKey: string;
   path: string;
   confmssg: string;
begin
  reg:= TRegistry.Create(KEY_READ);
  try
    case progid of
     1: begin
          reg.RootKey := HKEY_CURRENT_USER;
          subKey:= 'SOFTWARE\GameRanger';
          Downloader.sURL:= 'http://www.gameranger.com/download/GameRangerSetup.exe';
          Downloader.sFileName:= GetTempDirectory+'GameRangerSetup.exe';
          confmssg:= 'GameRanger';
        end;
     2: begin
          reg.RootKey := HKEY_LOCAL_MACHINE;
          subKey:= 'SOFTWARE\WarZone';
          Downloader.sURL:= 'http://www.ewarzone.com/downloads/WarZoneInstall.exe';
          Downloader.sFileName:= GetTempDirectory+'WarZoneInstall.exe';
          confmssg:= 'WarZone';
        end;
    end;
    if (not reg.KeyExists(subKey)) then
    begin
        DownloadAndLaunch(confmssg);
    end else
    begin
      reg.Access := KEY_READ;
      if reg.OpenKey(subKey, false) then
      begin
        case progid of
          1: path:= reg.ReadString('ExecutablePath');
          2: path:= IncludeTrailingPathDelimiter(reg.ReadString('InstallPath'))+'LobbyClient.exe';
        end;
        if FileExists(path) then
          ShellExecute (0, nil, @path[1], nil, nil, SW_NORMAL)
        else
          DownloadAndLaunch(confmssg);
      end;
    end;
    Reg.CloseKey;
  finally
    reg.Free;
  end;
end;

end.
