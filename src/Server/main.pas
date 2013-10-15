unit main;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, Menus, ComCtrls, ExtCtrls, tasv, savefile, dplay, dplay2, ActiveX,
  FileCtrl, ShellApi, Registry, textdata, unitid, unitsync{, dplobby}, IniFiles, ShlObj,
  ImgList, Buttons;

type
  TModsIniSettings = record
    ID: word;
    Name: string;
    Path: string;
    DemosPath: string;
    UseWeaponIdPatch: boolean;
    IconIndex : integer;
  end;
var LoadedModsList: array of TModsIniSettings;

type
  TOptions = record
    Tadir :string;
    usedir :integer;
    defdir :string;
    lastdir :string;
    syncspeed :integer;
    interval:integer;
    smooth:integer;
    lastprot :integer;
    mod2cstatus:integer; //borde inte vara här men error i tasv
    usemod0 :boolean;
    windowedmode :boolean;
    quickjoin :boolean;
    lastmoddir: integer;

    usecomp :boolean;
    usenewtimer :boolean;
    ta3d :Boolean;
    fixall :boolean;
    autorec :boolean;
    playernames :boolean;
    skippause : boolean;
    createtxt : boolean;
    sharemappos:boolean;
  end;

type
 TBckpRegDplay = record
   sCurrentDirectory : string;
   sPath: string;
   sFile : string;
   sCommandLine : string;
 end;
var bckregistrydplay: TBckpRegDplay;

type
  TfmMain = class(TForm)
    nbMain: TNotebook;
    sbMain: TStatusBar;
    lbProviders: TListBox;
    Label1: TLabel;
    Button1: TButton;
    Label2: TLabel;
    lbPlayers: TListBox;
    edChat: TEdit;
    lbChat: TListBox;
    Button2: TButton;
    btnPrevious_brscreen: TButton;
    lbCom: TListBox;
    Button4: TButton;
    pbLoading: TProgressBar;
    Label3: TLabel;
    lbLoading: TListBox;
    Label4: TLabel;
    Button5: TButton;
    tbSpeed: TTrackBar;
    pbGame2: TProgressBar;
    Label5: TLabel;
    Label6: TLabel;
    lbEvents: TListBox;
    Label7: TLabel;
    Label8: TLabel;
    Label9: TLabel;
    btnPlay: TButton;
    Button8: TButton;
    FileListBox1: TFileListBox;
    DirectoryListBox1: TDirectoryListBox;
    FilterComboBox1: TFilterComboBox;
    meGameInfo: TMemo;
    btnComments: TButton;
    btUnits: TButton;
    Button9: TButton;
    Button10: TButton;
    Comments: TMemo;
    addcomment: TButton;
    Nocomment: TButton;
    tidigare: TMemo;
    Label14: TLabel;
    Label15: TLabel;
    pbGame: TTrackBar;
    Button11: TButton;
    Button12: TButton;
    Label16: TLabel;
    edTADir: TEdit;
    rgUseDir: TRadioGroup;
    Label17: TLabel;
    Label18: TLabel;
    edDemoDir: TEdit;
    Label19: TLabel;
    Label20: TLabel;
    tbSync: TTrackBar;
    Label22: TLabel;
    Label23: TLabel;
    Label24: TLabel;
    Bevel1: TBevel;
    timemode: TCheckBox;
    tbinterval: TTrackBar;
    Label10: TLabel;
    Label11: TLabel;
    Label12: TLabel;
    Label13: TLabel;
    Bevel2: TBevel;
    tbSmooth: TTrackBar;
    Label25: TLabel;
    Label26: TLabel;
    Bevel3: TBevel;
    Bevel4: TBevel;
    Label27: TLabel;
    cbfixall: TCheckBox;
    cbautorec: TCheckBox;
    Label28: TLabel;
    cbCompress: TCheckBox;
    Label29: TLabel;
    Label30: TLabel;
    cbPlayernames: TCheckBox;
    Skippause: TCheckBox;
    Label31: TLabel;
    cbCreatetxt: TCheckBox;
    cbShareMapPos: TCheckBox;
    cb3DTA: TCheckBox;
    Button13: TButton;
    Button14: TButton;
    odSelectExe: TOpenDialog;
    cbUseMod0: TCheckBox;
    Bevel5: TBevel;
    btnModsEditor: TButton;
    Bevel8: TBevel;
    Bevel6: TBevel;
    Label21: TLabel;
    Bevel7: TBevel;
    cbWindowedMode: TCheckBox;
    cbQuickJoin: TCheckBox;
    Bevel9: TBevel;
    Label32: TLabel;
    Bevel10: TBevel;
    Bevel11: TBevel;
    listSelectMod: TListBox;
    ilModsIcons: TImageList;
    SpeedButton1: TSpeedButton;
    procedure nbMainPageChanged(Sender: TObject);
    procedure FormActivate(Sender: TObject);
    procedure Button1Click(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure btnPrevious_brscreenClick(Sender: TObject);
    procedure Button4Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure Button5Click(Sender: TObject);
    procedure btnPlayClick(Sender: TObject);
    procedure procmess;
    procedure Button8Click(Sender: TObject);
    procedure FileListBox1Click(Sender: TObject);
    procedure tbSpeedChange(Sender: TObject);
    procedure Label12Click(Sender: TObject);
    procedure btnCommentsClick(Sender: TObject);
    procedure Button9Click(Sender: TObject);
    procedure Button10Click(Sender: TObject);
    procedure btUnitsClick(Sender: TObject);
    procedure NocommentClick(Sender: TObject);
    procedure addcommentClick(Sender: TObject);
    procedure pbGameChange(Sender: TObject);
    procedure Button11Click(Sender: TObject);
    procedure Button12Click(Sender: TObject);
    procedure FormKeyDown(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure btnModsEditorClick(Sender: TObject);
    procedure Button14Click(Sender: TObject);
    procedure Button13Click(Sender: TObject);
    procedure FileListBox1MouseUp(Sender: TObject; Button: TMouseButton;
      Shift: TShiftState; X, Y: Integer);
    procedure FileListBox1DblClick(Sender: TObject);
    procedure listSelectModMeasureItem(Control: TWinControl;
      Index: Integer; var Height: Integer);
    procedure listSelectModDrawItem(Control: TWinControl; Index: Integer;
      Rect: TRect; State: TOwnerDrawState);
    procedure FileListBox1Change(Sender: TObject);
    procedure listSelectModClick(Sender: TObject);
    procedure SpeedButton1Click(Sender: TObject);
  private
    { Private declarations }
    JoinDPlay: TDPlay2;
    procedure ShowHint (Sender: TObject);
    procedure LoadOptions;
    function LoadModsList: boolean;
    procedure SaveOptions;
    function LoadSelectedDemo: boolean;
    function GetPath: string;
    procedure AfterPlayButton;
  public
    { Public declarations }
    running :integer;
    server :TAServer;
    save :TSaveFile;
    dp :IDirectPlay3;
    ids :TUnitIds;
    function CanWork :boolean;
    function getunits :TStringList;
    procedure CheckDir (st :string);
    function FindModName(id: string): string; //returns #0 if not present on list
    function FindModId(id: word): integer; //return -1 if not found
    procedure SetMod0(newlist: boolean);
    procedure FillModsSelectList;
  end;

const
 MODSINI = 'mods.ini';

var
  fmMain: TfmMain;
  options :TOptions;

implementation

{$R *.DFM}

uses
  lobby, selip, modslist, backwardcompat, waitform;

function BrowseDialog
 (const Title: string; const Flag: integer): string;
var
  lpItemID : PItemIDList;
  BrowseInfo : TBrowseInfo;
  DisplayName : array[0..MAX_PATH] of char;
  TempPath : array[0..MAX_PATH] of char;
begin
  Result:='';
  FillChar(BrowseInfo, sizeof(TBrowseInfo), #0);
  with BrowseInfo do begin
    hwndOwner := Application.Handle;
    pszDisplayName := @DisplayName;
    lpszTitle := PChar(Title);
    ulFlags := Flag;
  end;
  lpItemID := SHBrowseForFolder(BrowseInfo);
  if lpItemId <> nil then begin
    SHGetPathFromIDList(lpItemID, TempPath);
    Result := TempPath;
    GlobalFreePtr(lpItemID);
  end;
end;

function TfmMain.LoadSelectedDemo: boolean;
var
  i :integer;
  l :TStringList;
  tmp :string;
  us :TUnitSync;
  Icon: TIcon;
  FileInfo: SHFILEINFO;
  pathtemp: string;
  empty: hicon;
begin
  //imgIcon.Picture.Icon.Handle:= empty;
  Result:= False;
  ReadedTad.error:= True;
  ReadedTad.usemod:= FindModID(0);
  if filelistbox1.itemindex <> -1 then
  begin
  save := TSaveFile.Create (filelistbox1.items[filelistbox1.itemindex], true);
  if save.error = sfNone then
  begin
    //lbMod.Caption:= '';
    meGameInfo.Lines.Clear;
    megameinfo.lines.beginupdate;
//  megameinfo.visible := false;
    meGameInfo.Lines.Add ('Name of file: ' + filelistbox1.items[filelistbox1.itemindex]);
    meGameInfo.Lines.Add ('Recorded with version: ' +save.vername);
    // demo ID is present in TAD extra sector and it isn't OTA
    if (save.modId <> '') and (save.modId <> '0') then
    begin
      // is known (in mods.ini list)
      if FindModName(save.modId) <> #0 then
        begin
          meGameInfo.Lines.Add ('Mod: '+FindModName(save.modId));
          {lbMod.Caption:= (FindModName(save.modId));
          if FindModId(StrToInt(save.modid)) <> - 1 then
            if LoadedModsList[FindModId(StrToInt(save.modid))].Path <> '' then
              if FileExists(LoadedModsList[FindModId(StrToInt(save.modid))].Path) then
              begin
                Icon := TIcon.Create;
                try
                  pathtemp:= LoadedModsList[FindModId(StrToInt(save.modid))].Path;
                  SHGetFileInfo(PChar(pathtemp), 0, FileInfo, SizeOf(FileInfo), SHGFI_ICON);
                  icon.Handle := FileInfo.hIcon;
                  //DestroyIcon(FileInfo.hIcon);
                  imgIcon.Picture.Icon.Handle:= icon.Handle;
                finally
                  //Icon.Free;
                end;
              end; }
        end else
          meGameInfo.Lines.Add ('Mod: '+'['+save.modId+'] Unknown (not present in current mods list)');
          //lbMod.Caption:= ('['+save.modId+'] Unknown'); }
    ReadedTad.usemod:= FindModID(StrToInt(save.modId));
    end;
    if save.datum <> '' then
      meGameInfo.Lines.Add ('Recorded at: ' +save.Datum);
    meGameInfo.Lines.add ('Number of players: ' + IntToStr(save.numplayers));
    meGameInfo.Lines.add ('Map name: ' +save.map);
    meGameInfo.Lines.add ('Max units: ' + inttostr (save.maxunits));
    meGameInfo.Lines.Add ('Filesize: ' + save.fsize);
    {$IFDEF CRC}
    meGameInfo.Lines.Add ('Recorded from: ' + save.recfrom);
    us := TUnitsync.create (save.units);
    if save.crc = us.crc then
      meGameInfo.Lines.Add ('The crc status: Correct (' + inttostr (save.crc) + ')')
    else
      meGameInfo.Lines.Add ('The crc status: Incorrect! (file: ' + inttostr (us.crc) + ' calc: ' + inttostr (save.crc) + ')');
    us.free;
    {$ENDIF}
    meGameInfo.Lines.Add ('');
    meGameInfo.Lines.Add ('The players');
    meGameInfo.Lines.Add ('-----------------');
    for i := 1 to save.Numplayers do
    begin
      {$IFDEF CRC}
      if save.playeradress[i] <> '' then
        meGameInfo.Lines.add (save.players[i].name + ' - ' + save.playeradress [i])
      else
        meGameInfo.Lines.add (save.players[i].name);
      {$ELSE}
      if i = 1 then
        meGameInfo.Lines.add (save.players[i].name + ' <<-- Recorded the game')
      else
        meGameInfo.Lines.add (save.players[i].name);
      {$ENDIF}

{      if save.Playeradress[i]<>'' then
        meGameInfo.Lines.add (save.playeradress[i]);}
    end;

    meGameInfo.Lines.Add ('');
    l := getunits;
    if assigned (l) then
    begin
      if l.count > 0 then
      begin
        meGameInfo.Lines.Add ('Extra units used in this game:');
        meGameInfo.Lines.Add ('-----------------');
        for i := 0 to l.count - 1 do
          meGameInfo.Lines.Add (l.strings [i]);
      end;
//      l.Free;
    end;
    
    if save.comments<>'' then begin
      meGameInfo.Lines.Add ('');
      meGameInfo.Lines.add ('Comments in this file');
      meGameInfo.Lines.Add ('-----------------');
      meGameInfo.Lines.add (save.comments);
    end;



    if save.chat<>'' then begin
      meGameInfo.Lines.Add ('');
      meGameInfo.Lines.add ('Lobby chat');
      meGameInfo.Lines.Add ('-----------------');
      tmp := save.chat;
      while Length (tmp) > 1 do
      begin
        meGameInfo.Lines.add (Copy (tmp, 1, Pos (#13, tmp) - 1));
        Delete (tmp, 1, Pos (#13, tmp));
      end;
    end;

    btnPlay.Enabled := true;
    btnComments.Enabled := true;
  end else
  begin
    meGameInfo.Lines.Clear;
    meGameInfo.Lines.Add ('This is not a valid TA Demo file');
    meGameInfo.Lines.Add ('');
    case save.Error of
      sfUnknown :meGameInfo.Lines.Add ('Unknown error');
      sfOldVersion :meGameInfo.Lines.Add ('We are sorry, but old versions of the file format are not supported since they contain corrupted packets');
      sfNoMagic :meGameInfo.Lines.Add ('The header is incorrect');
      sfNewVersion :meGameInfo.Lines.Add ('This file is too new. Please upgrade your replayer');
    end;
    btnPlay.Enabled := false;
    btnComments.Enabled := false;

  end;

  //Scrollar upp..
  meGameInfo.Lines.Insert (0, 'hey man');
  meGameInfo.Lines.Delete (0);
  megameinfo.lines.endupdate;

//  megameinfo.visible := true;
  ReadedTad.modid:= save.modId;
  ReadedTad.filename:= filelistbox1.items[filelistbox1.itemindex];
  ReadedTad.version:= save.version;
  ReadedTad.error:= not (save.Error = sfNone);

  save.Free;
  Result:= True;
  end;
end;

procedure TfmMain.nbMainPageChanged(Sender: TObject);
begin
  case nbMain.PageIndex of
    0 : begin
          lbProviders.ItemIndex := options.lastprot;
          cbCompress.Checked := options.usecomp;
          timemode.Checked := options.usenewtimer;
          skippause.Checked := options.skippause;
          cb3dta.checked := options.ta3d;
        end;
    1 : begin
//          server.DoLobby;
        end;
    6 : begin
          edTADir.Text := options.Tadir;
          rgUseDir.ItemIndex := options.usedir;
          edDemoDir.Text := options.defdir;
          tbSync.Position := options.syncspeed;
          tbInterval.Position := options.interval;
          tbsmooth.Position := options.smooth;

          cbUseMod0.Checked := options.usemod0;
          cbWindowedMode.Checked:= options.windowedmode;
          cbQuickJoin.Checked:= options.quickjoin;
          cbfixall.Checked := options.fixall;
          cbautorec.Checked := options.autorec;
          cbcreatetxt.Checked := options.createtxt;
          cbsharemappos.Checked := options.sharemappos;
          timemode.Checked := options.usenewtimer;
          cb3dta.checked := options.ta3d;          
          skippause.Checked := options.skippause;
          cbPlayernames.checked := options.playernames;
        end;
  end;
end;

procedure TfmMain.procmess();
begin
   application.processmessages;
end;

procedure TfmMain.ShowHint (Sender: TObject);
begin
  sbMain.SimpleText := Application.Hint;
end;

procedure TfmMain.FormActivate(Sender: TObject);
var
  path, name :string;
  s :string;
  i :integer;
begin
  //Hindra att detta körs mer än en gång.. kanske orsakar 215-kraschen
  if running = 4711 then
    exit;
  running := 4711;

  Application.OnHint := ShowHint;

  {$IFDEF CRC}
  MessageDlg ('This is the CRC version. Not for distribution.', mtWarning, [mbok], 0);
  Caption := Caption + ' - CRC - Do not distribute';
  {$ENDIF}
  
  LoadOptions;
  LoadModsList;

  ids := TUnitIds.Create (ExtractFilePath (paramstr (0)) + '\unitid.txt');

  nbMain.PageIndex := 4;
  if not CanWork then
  begin
    Application.terminate;
  end else
  begin
    server := TAServer.Create (dp);
  end;

  //Aha
  if ParamCount <> 0 then
  begin
    s := '';
    for i := 1 to paramcount do
    begin
      s := s + paramstr (i) + ' ';
    end;
    s := trim (s);

    if s = '---cp' then
    begin
      nbMain.PageIndex := 6;
      exit;
    end;


    path := ExtractFilePath (s);
    name := ExtractFileName (s);

    DirectoryListbox1.Directory := path;
    filelistbox1.directory := path;

    filelistbox1.itemindex := filelistbox1.items.IndexOf (name);
//    filelistbox1.filename := s;
    FileListBox1Click(Self);
  end else
  begin
    case options.usedir of
      //0 :directorylistbox1.directory := options.lastdir;
      0 : begin
            if options.lastmoddir <> -1 then
              if FindModId(options.lastmoddir) <> - 1 then
              begin
                listSelectMod.ItemIndex:= FindModId(options.lastmoddir);
                if options.lastmoddir = 0 then
                  directorylistbox1.directory := options.defdir
                else
                  listSelectModClick(self);
              end;
          end;
      1 : begin
            if FindModId(0) <> -1 then
              listSelectMod.ItemIndex:= FindModId(0);
            directorylistbox1.directory := options.defdir;
          end;
    end;
  end;
end;

procedure TfmMain.Button1Click(Sender: TObject);
begin
  if lbProviders.Itemindex <> -1 then
  begin
    server.CreateSession (pointer (lbProviders.Items.Objects [lbProviders.ItemIndex]), save);
    nbMain.PageIndex := 1;
    options.lastprot := lbproviders.itemindex;
    options.usecomp := cbCompress.Checked;
    options.usenewtimer := timemode.Checked;
    options.skippause := skippause.Checked;
    options.ta3d := cb3dta.checked;
  end;
end;

procedure TfmMain.FormClose(Sender: TObject; var Action: TCloseAction);
begin
//  server.Leave;

  SaveOptions;
  server.Free;
end;

procedure TfmMain.btnPrevious_brscreenClick(Sender: TObject);
begin
  server.Leave;
  nbMain.PageIndex := 0;
end;

procedure TfmMain.Button4Click(Sender: TObject);
var
  chat :TChat;
begin
  chat := TChat.Create (edChat.text);
  lbChat.Items.Add (edChat.Text);
  edChat.Text := '';
  server.Send (1, 0, chat);
  chat.Free;
end;

procedure TfmMain.Button2Click(Sender: TObject);
begin
  if server.Launch then
  begin
    nbMain.PageIndex := 2;
    server.Load;
  end;
{  nbMain.Pageindex := 3;
  server.Play;}
end;

procedure TfmMain.Button5Click(Sender: TObject);
begin
  Application.Terminate;
//  server.quit := true;
end;

procedure TfmMain.btnPlayClick(Sender: TObject);
begin
  {meGameinfo.Lines.Add ('');
  meGameinfo.Lines.Add ('Processing demo file, this might take a few minutes');}
  fmWait.Show;
  SaveOptions;
  Application.ProcessMessages;
  save := TSaveFile.Create (filelistbox1.items[filelistbox1.itemindex], false);
  
  if options.quickjoin then
  begin
    fmWait.lbWait.Caption:= 'Preparing server...';
    fmWait.Refresh;
  end
  else fmWait.Close;

  if save.Error <> sfNone then
  begin
    MessageDlg ('Invalid file selected', mtError, [mbok], 0);
  end else
  begin
    if ReadedTad.version < 7 then
    begin
      if not options.usemod0 then
      begin
        fmBackwardCompat.ShowModal;
        if fmBackwardCompat.ModalResult = mrOK then
        begin
          if fmBackwardCompat.cbfmbcompat.Checked then
            options.usemod0:= True;
          ReadedTad.usemod:= 0;  
          AfterPlayButton;
        end else
          save.Free;
      end else
        AfterPlayButton;
    end else
      begin
        if ReadedTad.usemod <> - 1 then
        begin
          AfterPlayButton;
        end else
        begin
          //v7, stored mod number in tad but not found on list
          ShowMessage('This demo file has been recorded by game mod that is not present' +#10#13+
          'in '+MODSINI+' list. Please reassign this demo or install mod first.');
          save.Free;
          FileListBox1Click(Self);
        end;
      end;
  end;
end;

procedure TfmMain.AfterPlayButton;
var
r: tregistry;
begin
  if options.quickjoin then
  begin
   lbProviders.ItemIndex := options.lastprot;
   if lbProviders.Itemindex <> -1 then
   begin
     server.CreateSession (pointer (lbProviders.Items.Objects [lbProviders.ItemIndex]), save);
     fmWait.lbWait.Caption:= 'Launching the game...';
     fmWait.Refresh;
     //backup registry
     r := TRegistry.Create;
     r.Rootkey := HKEY_LOCAL_MACHINE;
     if r.OpenKey ('SOFTWARE\Microsoft\DirectPlay\Applications\Total Annihilation', false) then
     begin
      try
        bckregistrydplay.sCurrentDirectory:= r.ReadString('CurrentDirectory');
        bckregistrydplay.sPath:= r.ReadString('Path');
        bckregistrydplay.sFile:= r.ReadString('File');
        bckregistrydplay.sCommandLine:= r.ReadString('CommandLine');
        r.WriteString('CurrentDirectory', ExtractFilePath(getpath));
        r.WriteString('Path', ExtractFilePath(getpath));
        r.WriteString('File', ExtractFileName(getpath));
        if options.windowedmode then
          r.WriteString('CommandLine', '-d')
        else
          r.WriteString('CommandLine', '');
      finally
       r.Free;
      end;
    end;
     JoinDPlay := TDPlay2.Create;
     if JoinDPlay.Initialize then
     begin
       if JoinDPlay.JoinSession(TA_GUID, GUID_NULL, fmSelIP.GetIP (dp, server), 'TA DEMO') then
       begin
         fmWait.Close;
         nbMain.PageIndex:= 1;
       end;
     end;
    if nbMain.PageIndex <> 1 then fmWait.Close;
    fmWait.lbWait.Caption:= 'Processing demo file. Please wait...';
   end;
  end else
    nbMain.PageIndex:= 0;
end;

function TfmMain.CanWork :boolean;
var
  dp1  :IDirectPlay;
begin
  DirectPlayCreate (@GUID_NULL, dp1, nil);
  dp1.QueryInterface (IID_IDirectPlay3, dp);

  result := true;
end;


procedure TfmMain.Button8Click(Sender: TObject);
begin
  fmMain.Close;
end;

procedure TfmMain.FileListBox1Click(Sender: TObject);
var
  done: boolean;
begin
  done:= LoadSelectedDemo;
end;

procedure TfmMain.tbSpeedChange(Sender: TObject);
begin
  server.speed := 1000-tbSpeed.Position;
end;

procedure TfmMain.Label12Click(Sender: TObject);
var
  s :string;
begin
  s := 'http://www.clan-sy.com';
  ShellExecute (0, nil, @s[1], nil, nil, SW_NORMAL);
end;

procedure TfmMain.btnCommentsClick(Sender: TObject);
begin
  save := TSaveFile.Create (filelistbox1.items[filelistbox1.itemindex], true);
  if save.Error <> sfNone then
  begin
    MessageDlg ('Invalid file selected', mtError, [mbok], 0);
    save.Free;
  end else
  begin
    if Save.version < 4 then
    begin
      MessageDlg ('Comments can only be added to version 0.81a-files or newer', mtError, [mbok], 0);
      save.Free;
      exit;
    end;

    nbMain.PageIndex := 5;
    tidigare.Lines.Clear;
    comments.lines.clear;
    tidigare.Lines.Add (save.comments);
  end;

//  Application.Terminate;
end;

procedure TfmMain.Button9Click(Sender: TObject);
begin
  nbMain.PageIndex := 4;
  save.Free;
end;

function TfmMain.GetPath: string;
var
 r :TRegistry;
 path: string;
begin
path:= '';
  if ReadedTad.modid <> '' then
  begin
    if FindModId(StrToInt(ReadedTad.modid)) <> - 1 then
    begin
      ReadedTad.useweapid:= LoadedModsList[FindModId(StrToInt(ReadedTad.modid))].UseWeaponIdPatch;
      path:= LoadedModsList[FindModId(StrToInt(ReadedTad.modid))].Path;
    end;
  end else
  begin
    if FindModId(0) <> - 1 then
    path:= LoadedModsList[FindModId(0)].path;
  end;

  if path = '' then
    if ExtractFileExt(options.Tadir) <> '.exe' then
      path:= Trim(options.Tadir)+'\totala.exe'
    else
      path:= Trim(options.Tadir);

  if path = '' then
  begin
    r := TRegistry.Create;
    r.Rootkey := HKEY_LOCAL_MACHINE;
    if r.OpenKey ('Software\Microsoft\Windows\CurrentVersion\Uninstall\Total Annihilation', false) then
    begin
      try
        path := r.ReadString ('Dir');
      except
      end;
    end;
    r.free;

    if path = '' then
    begin
      MessageDlg ('Unable to find your TA Directory. Enter it manually on the options screen', mtError, [mbok], 0);
      exit;
    end;
  end;
  Result:= path;
end;

procedure TfmMain.Button10Click(Sender: TObject);
var
  path :string;
  param :string;
  ip :string;
begin
  path:= GetPath;
  
  param := '';

  if IsSameGuid (DPSPGUID_IPX, server.provider) then
    param := '/n2';
  if IsSameGuid (DPSPGUID_MODEM, server.provider) then
    param := '/n3';
  if IsSameGuid (DPSPGUID_SERIAL, server.provider) then
    param := '/n4';

  if IsSameGuid (DPSPGUID_TCPIP, server.provider) then
  begin
    ip := fmSelIP.GetIP (dp, server);
    if ip = '' then
      exit;
    case options.windowedmode of
    true: param := '-d /n1:' + ip;
    false: param := '/n1:' + ip;
    end;
  end;

  ShellExecute (0, nil, @path[1], @param[1], nil, SW_NORMAL);
end;

procedure TfmMain.btUnitsClick(Sender: TObject);
begin
  nbMain.pageindex := 6;
end;

procedure TfmMain.NocommentClick(Sender: TObject);
begin
  nbMain.PageIndex := 4;
end;

procedure TfmMain.addcommentClick(Sender: TObject);
begin
  save.AddCommentSector(Comments.text,filelistbox1.items[filelistbox1.itemindex]);
  save.Free;

  FileListBox1Click(Self);
  nbMain.PageIndex := 4;
end;

procedure TfmMain.pbGameChange(Sender: TObject);
begin
  server.SetPos (pbGame.Position);
end;

function TfmMain.getunits :TStringList;
var
  us :TUnitSync;
  i  :integer;
  s  :string;
  l  :TStringList;
  u  :Tlist;
begin
  us := TUnitSync.Create (save.units);
  us.Compact;
  l := TStringList.Create;
  l.Duplicates := dupIgnore;

  u := tlist.create;
  for i := 0 to us.units.count - 1 do
  begin
    u.add (pointer (us.get (i, false).id));
  end;
  result := ids.GetUnits (u);
  u.free;

{  for i := 0 to us.units.count - 1 do
  begin
    s := ids.GetName (us.Get (i, false).id);
    if s <> '' then
      l.Add (s);
  end;

  result := l; }

  us.Free;
end;

procedure TfmMain.Button11Click(Sender: TObject);
begin
  options.tadir := edTADir.Text;
  options.usedir := rgUseDir.ItemIndex;
  options.defdir := IncludeTrailingPathDelimiter(edDemoDir.Text);
  options.syncspeed := tbSync.Position;
  options.interval := tbinterval.Position;
  options.smooth := tbsmooth.Position;
  options.usemod0 := cbUseMod0.Checked;
  options.windowedmode := cbWindowedMode.Checked;
  options.quickjoin:= cbQuickJoin.Checked;
  
  options.fixall := cbfixall.Checked;
  options.autorec := cbautorec.checked;
  options.sharemappos := cbsharemappos.checked;
  options.createtxt := cbcreatetxt.checked;
  options.usenewtimer := timemode.Checked;
  options.ta3d := cb3dta.checked;
  options.skippause := skippause.Checked;
  options.playernames := cbplayernames.checked;

//  checkdir (options.tadir);
  checkdir (options.defdir);

  SetMod0(false);
  SaveOptions;

  if paramstr (1) = '---cp' then
    application.terminate
  else
    nbmain.PageIndex := 4;
end;

procedure TfmMain.LoadOptions;
var
  r :TRegIniFile;
begin
  r := TRegIniFile.Create ('Software\TA Patch\TA Demo');

  options.Tadir := r.ReadString ('Options', 'TADir', '');
  options.usedir := r.ReadInteger ('Options', 'usedir', 0);
  options.defdir := r.readstring ('Options', 'defdir', '');
  options.lastdir := r.ReadString ('Options', 'lastdir', '');
  options.syncspeed := r.ReadInteger ('Options', 'sync', 10);
  options.interval := r.ReadInteger ('Options', 'interval', 90);
  options.smooth := r.ReadInteger ('Options', 'smooth', 3);
  options.lastprot := r.ReadInteger ('Options', 'lastprot', -1);
  options.usemod0 := r.readbool ('Options', 'usemod0', false);
  options.windowedmode := r.readbool ('Options', 'windowedmode', false);
  options.quickjoin:= r.readbool('Options', 'quickjoin', false);
  options.lastmoddir:= r.ReadInteger ('Options', 'lastmoddir', -1);

  options.usecomp := r.ReadBool ('Options', 'usecomp', true);
  options.usenewtimer := r.Readbool ('Options', 'newtimer', true);
  options.ta3d := r.ReadBool ('Options', 'ta3d', false);
  options.fixall := r.readbool ('Options', 'fixall', false);
  options.autorec := r.readbool ('Options', 'autorec', true);
  options.sharemappos := r.readbool ('Options', 'sharepos', true);
  options.createtxt := r.readbool ('Options', 'createtxt', false);
  options.skippause := r.readbool ('Options', 'skippause', true);
  options.usenewtimer := r.readbool ('Options', 'newtime', true);
  options.playernames := r.readbool ('Options', 'playernames', true);

  r.WriteString ('Options', 'serverdir', ExtractFilePath (paramstr (0)));
  r.Free;
end;

procedure TfmMain.SaveOptions;
var
  r :TRegIniFile;
  r2 : TRegistry;
begin
  r := TRegIniFile.Create ('Software\TA Patch\TA Demo');

  r.WriteString ('Options', 'TADir', options.tadir);
  r.writeinteger ('Options', 'usedir', options.usedir);
  r.writestring ('Options', 'defdir', options.defdir);
  r.writestring ('Options', 'lastdir', directorylistbox1.Directory);
  r.writeinteger ('Options', 'sync', options.syncspeed);
  r.writeinteger ('Options', 'interval', options.interval);
  r.writeinteger ('Options', 'smooth', options.smooth);
  r.writeinteger ('Options', 'lastprot', options.lastprot);
  r.writebool ('Options', 'usemod0', options.usemod0);
  r.writebool ('Options', 'windowedmode', options.windowedmode);
  r.writebool ('Options', 'quickjoin', options.quickjoin);
  r.writeinteger ('Options', 'lastmoddir', options.lastmoddir);

  r.writebool ('Options', 'usecomp', options.usecomp);
  r.writebool ('Options', 'newtimer', options.usenewtimer);
  r.WriteBool ('Options', 'ta3d', options.ta3d);
  r.writebool ('Options', 'fixall', options.fixall);
  r.writebool ('Options', 'autorec', options.autorec);
  r.writebool ('Options', 'createtxt', options.createtxt);
  r.writebool ('Options', 'sharepos', options.sharemappos);
  r.writebool ('Options', 'newtime', options.usenewtimer);
  r.writebool ('Options', 'skippause', options.skippause);
  r.writebool ('Options', 'playernames', options.playernames);

  r.Free;

  //backup registry
  r2 := TRegistry.Create;
  r2.Rootkey := HKEY_LOCAL_MACHINE;
  if r2.OpenKey ('SOFTWARE\Microsoft\DirectPlay\Applications\Total Annihilation', false) then
  begin
    try
      if bckregistrydplay.sFile <> '' then
      begin
        r2.WriteString('CurrentDirectory', bckregistrydplay.sCurrentDirectory);
        r2.WriteString('Path', bckregistrydplay.sPath);
        r2.WriteString('File', bckregistrydplay.sFile);
        r2.WriteString('CommandLine', bckregistrydplay.sCommandLine);
      end;
    finally
       r2.Free;
    end;
  end;
end;

function TfmMain.LoadModsList: boolean;
var
  Sections: TStringList;
  i: word;
  incorrect: word;
  s: string;
begin
Result:= False;
incorrect:= 0;
if FileExists(IncludeTrailingPathDelimiter(ExtractFilePath(paramstr(0)))+MODSINI) then
begin
  with TIniFile.Create(IncludeTrailingPathDelimiter(ExtractFilePath(paramstr(0)))+MODSINI) do
    try
      Sections := TStringList.Create;
      try
        ReadSections(Sections);
        if Sections.Count > 0 then
        begin
        SetLength(LoadedModsList, Sections.Count);
        // add "all mods" to list mod
        for i := 0 to Sections.Count - 1 do
        begin
          if (ReadString(Sections[i], 'ID', '') <> '') and
             (ReadString(Sections[i], 'Path', '') <> '') then
          begin
            LoadedModsList[i].ID := ReadInteger(Sections[i], 'ID', 0);
            LoadedModsList[i].Name := ReadString(Sections[i], 'Name', '');
            LoadedModsList[i].Path := ReadString(Sections[i], 'Path', '');
            LoadedModsList[i].UseWeaponIdPatch := ReadBool(Sections[i], 'UseWeaponIdPatch', False);
            // dont add backward compatibility
            //if LoadedModsList[i].ID <> 0 then
            //begin
            {            icon:= TIcon.Create;
              try
                if SHGetFileInfo(PChar(LoadedModsList[i].Path), 0, FileInfo, SizeOf(FileInfo), SHGFI_ICON) <> 0 then
                begin
                  icon.Handle := FileInfo.hIcon;
                  ilModsIcons.AddIcon(icon);
                  LoadedModsList[i].IconIndex:= ilModsIcons.Count - 1;
                end else
                  LoadedModsList[i].IconIndex:= -1;
              finally
                icon.Free;
              end;
            listSelectMod.Items.BeginUpdate;
            listSelectMod.Items.Add(LoadedModsList[i].Name);
            listSelectMod.Items.EndUpdate;   }
            //end;

            {if LoadedModsList[i].ID <> 0 then
              FilterComboBox1.Filter:= FilterComboBox1.Filter +
                '|'+LoadedModsList[i].Name+'|*['+ IntToStr(LoadedModsList[i].ID) +'].tad'; }
          end else
            Inc(incorrect);
            Continue;
          end;
        FillModsSelectList;
        if not SectionExists('MOD0') then
        begin
          LoadedModsList[0].ID := 0;
          LoadedModsList[0].Name := 'Backward compatibility';
          LoadedModsList[0].Path := options.Tadir;
          LoadedModsList[0].UseWeaponIdPatch := False;
          WriteInteger('MOD0', 'ID', LoadedModsList[0].ID);
          WriteString('MOD0', 'Name', LoadedModsList[0].Name);
          WriteString('MOD0', 'Path', LoadedModsList[0].Path);
          WriteBool('MOD0', 'UseWeaponIdPatch', LoadedModsList[0].UseWeaponIdPatch);
        end;
        end else {sections count < 1 }
          begin
            s:= ExtractFileName(options.Tadir);
            if Copy(s, Length(s)-2, 3) <> 'exe' then
            begin
              if FileExists(IncludeTrailingPathDelimiter(options.Tadir) + 'TotalA.exe') then
                s := IncludeTrailingPathDelimiter(options.Tadir) + 'TotalA.exe';
              options.Tadir:= s;
            end;
            SetLength(LoadedModsList, 1);
            LoadedModsList[0].ID := 0;
            LoadedModsList[0].Name := 'Backward compatibility';
            LoadedModsList[0].Path := options.Tadir;
            LoadedModsList[0].UseWeaponIdPatch := False;
            WriteInteger('MOD0', 'ID', LoadedModsList[0].ID);
            WriteString('MOD0', 'Name', LoadedModsList[0].Name);
            WriteString('MOD0', 'Path', LoadedModsList[0].Path);
            WriteBool('MOD0', 'UseWeaponIdPatch', LoadedModsList[0].UseWeaponIdPatch);
          end;
      finally
        Sections.Free;
      end;
    finally
      Free;
    end;
  if incorrect > 0 then
    showmessage('Warning ! Found '+intToStr(incorrect)+' incorrect entries in '+MODSINI);
  if High(LoadedModsList) > 0 then
    Result:= True
  else
    SetMod0(False);
end else
begin
   SetMod0(True);
   FillModsSelectList;
end;
end;

procedure TfmMain.FillModsSelectList;
var
 Icon: TIcon;
 FileInfo: SHFILEINFO;
 i: integer;
begin
for i:= Low(LoadedModslist) to High(LoadedModslist) do
begin
icon:= TIcon.Create;
try
if SHGetFileInfo(PChar(LoadedModsList[i].Path), 0, FileInfo, SizeOf(FileInfo), SHGFI_ICON) <> 0 then
begin
icon.Handle := FileInfo.hIcon;
ilModsIcons.AddIcon(icon);
LoadedModsList[i].IconIndex:= ilModsIcons.Count - 1;
end else
LoadedModsList[i].IconIndex:= -1;
finally
icon.Free;
end;
listSelectMod.Items.BeginUpdate;
listSelectMod.Items.Add(LoadedModsList[i].Name);
listSelectMod.Items.EndUpdate;
end;
end;

procedure TfmMain.SetMod0(newlist: boolean);
var
 s : string;
begin
  if newlist then
  begin
    s:= ExtractFileName(options.Tadir);
    if Copy(s, Length(s)-2, 3) <> 'exe' then
    begin
      if FileExists(IncludeTrailingPathDelimiter(options.Tadir) + 'TotalA.exe') then
        s := IncludeTrailingPathDelimiter(options.Tadir) + 'TotalA.exe';
        options.Tadir:= s;
    end;
  end;

  with TIniFile.Create(IncludeTrailingPathDelimiter(ExtractFilePath(paramstr(0)))+MODSINI) do
    try
      if newlist then SetLength(LoadedModsList, 1);
      //fix loadedmodslist and write new path to ini
      if FindModId(0) <> - 1 then
      begin
        LoadedModsList[FindModId(0)].Path:= options.Tadir;
        LoadedModsList[FindModId(0)].Name:= 'Backward compatibility';
        LoadedModsList[FindModId(0)].ID:= 0;
        LoadedModsList[FindModId(0)].UseWeaponIdPatch:= False;
        WriteInteger('MOD0', 'ID', 0);
        WriteString('MOD0', 'Name', 'Backward compatibility');
        WriteString('MOD0', 'Path', options.Tadir);
        WriteBool('MOD0', 'UseWeaponIdPatch', False);
      end else
        begin
          WriteInteger('MOD0', 'ID', 0);
          WriteString('MOD0', 'Name', 'Backward compatibility');
          WriteString('MOD0', 'Path', options.Tadir);
          WriteBool('MOD0', 'UseWeaponIdPatch', False);
        end;
    finally
      Free;
    end;
end;

function TfmMain.FindModName(id: string): string;
var
  iid: word;
  i: word;
begin
  Result:= #0;
  if id <> '' then
  begin
    iid:= StrToInt(id);
    for i := Low(LoadedModsList) to High(LoadedModsList) do
    begin
      if LoadedModsList[i].ID = iid then
      begin
        Result:= LoadedModsList[i].Name;
        Break;
      end;
    end;
  end;
end;

function TfmMain.FindModId(id: word): integer;
var
  i: word;
begin
  Result:= -1;
  for i := Low(LoadedModsList) to High(LoadedModsList) do
  begin
    if LoadedModsList[i].ID = id then
    begin
      Result:= i;
      Break;
    end;
  end;
end;

procedure TfmMain.Button12Click(Sender: TObject);
begin
  if paramstr (1) = '---cp' then
    application.terminate
  else
    nbmain.PageIndex := 4;
end;

procedure TfmMain.FormKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
var
  name :string;
  nr   :integer;
begin
  if nbMain.PageIndex <> 4 then
    exit;

  try
    name := filelistbox1.items[filelistbox1.itemindex];
  except
    exit;
  end;
  if key = vk_delete then
  begin
    if MessageDlg ('Are you sure you want to delete ' + name + '?', mtConfirmation, [mbyes, mbno], 0) = mrYes then
    begin
      nr := filelistbox1.itemindex;
      DeleteFile (name);
      filelistbox1.update;
      if nr > 0 then
        nr := nr - 1;
      if filelistbox1.Items.count > 0 then
      begin
        filelistbox1.itemindex := nr;
        FileListBox1Click(Self);
//        filelistbox1.SetFocus;
      end else
        meGameInfo.lines.clear;
    end;
  end;

end;

procedure TfmMain.CheckDir (st :string);
var
  s :TFileStream;
begin
  try
    s := TFileStream.Create (st + '__tmp.txt', fmCreate);
  except
    if MessageDlg ('The directory "' + st + '" does not exist. Do you want to create it?', mtConfirmation, [mbYes, mbNo], 0) = mrYes then
    begin
      ForceDirectories (st);
    end;
    s := nil;
  end;

  if assigned (s) then
    s.free;

  DeleteFile (st + '__tmp.txt');
end;


procedure TfmMain.btnModsEditorClick(Sender: TObject);
begin
 // nbMain.PageIndex := 7;
end;

procedure TfmMain.Button14Click(Sender: TObject);
begin
  if options.Tadir <> '' then
    odSelectExe.InitialDir:= IncludeTrailingPathDelimiter(ExtractFileDir(options.Tadir));
  if odSelectExe.Execute then
  begin
    if odSelectExe.FileName <> '' then
      edTADir.Text:= odSelectExe.FileName;
  end;
end;

procedure TfmMain.Button13Click(Sender: TObject);
var sFolder: string;
begin
  sFolder := BrowseDialog('Select directory...', BIF_RETURNONLYFSDIRS);
  if sFolder <> '' then
  begin
    edDemoDir.text := IncludeTrailingPathDelimiter(sFolder);
    DirectoryListbox1.Directory:= edDemoDir.text;
  end;
end;

procedure TfmMain.FileListBox1MouseUp(Sender: TObject;
  Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
var
  APoint: TPoint;
  Index: integer;
  newid: integer;
  done: boolean;
  oldindex: integer;
begin
  if Button = mbRight then
  begin
    APoint.X := X;
    APoint.Y := Y;
    Index := FileListBox1.ItemAtPos(APoint, True);
    if Index > -1 then
      begin
      FileListBox1.Selected[Index] := True;
  done:= LoadSelectedDemo;
  if ReadedTad.Error = false then
  begin
    if ReadedTad.modId <> '' then setItemIndex:= FindModId(StrToInt(ReadedTad.modId))
      else if ReadedTad.version < 7 then
        if MessageDlg('This demo file has been recorded with backward compatibility mode enabled. '+
        #13#10+'Assigning it to mod that requires more than 256 weapon ID''s will make it unplayable'+
        ' for older Replayers (like 1.0.0.545).', mtConfirmation, mbOKCancel, 0) = mrCancel then
          Exit else setItemIndex:= -1;
    end;
    fmModsAssignList.ShowModal;
    if fmModsAssignList.ModalResult = mrOk then
      begin
        fmWait.lbWait.Caption:= 'Working...';
        fmWait.Show;
        fmWait.Refresh;
        //save new mod ID or/and tad version
        newid:= LoadedModsList[fmModsAssignList.lbModsAssign.ItemIndex].ID;
        case newid of
          -1: Exit;
          0: begin
               //set version to 5, remove mods sector
               save.ChangeModAssignation(true, 0, filelistbox1.items[filelistbox1.itemindex]);
             end;
          else
            begin
               //set version to 7, mod id to newid
               save.ChangeModAssignation(false, newid, filelistbox1.items[filelistbox1.itemindex]);
            end;
        end;
        oldindex:= FileListBox1.ItemIndex;
        directoryListBox1.fileList:= nil;
        fileListBox1.update;
        directoryListBox1.fileList:= fileListBox1;
        FileListBox1.ItemIndex:= oldindex;
        fmWait.Close;
        fmWait.lbWait.Caption:= 'Processing demo file. Please wait...';
        FileListBox1Click(Self);
        nbMain.PageIndex := 4;
      end else
        Exit;
      end;
  end;
end;

procedure TfmMain.FileListBox1DblClick(Sender: TObject);
begin
 if options.quickjoin then btnPlay.Click;
end;

procedure TfmMain.listSelectModMeasureItem(Control: TWinControl;
  Index: Integer; var Height: Integer);
begin
height := ilModsIcons.Height + 4;
end;

procedure TfmMain.listSelectModDrawItem(Control: TWinControl;
  Index: Integer; Rect: TRect; State: TOwnerDrawState);
var
CenterText : integer;

begin

listSelectMod.Canvas.FillRect (rect);

// now draw
if LoadedModsList[index].IconIndex <> - 1 then
  ilModsIcons.Draw(listSelectMod.Canvas,rect.Left + 4, rect.Top + 4, LoadedModsList[index].IconIndex );

// you have to center the text vertically besidethe bitmap, or it will appear a little heigher
CenterText := ( rect.Bottom - rect.Top - listSelectMod.Canvas.TextHeight(text)) div 2 ;

listSelectMod.Canvas.TextOut (rect.left + ilModsIcons.Width + 8 , rect.Top + CenterText,
listSelectMod.Items.Strings[index]);
end;

procedure TfmMain.FileListBox1Change(Sender: TObject);
begin
  FileListBox1.Hint:= FileListBox1.FileName;
end;

procedure TfmMain.listSelectModClick(Sender: TObject);
var
path: string;
begin
  options.lastmoddir:= LoadedModsList[listSelectMod.ItemIndex].ID;
  if LoadedModsList[listSelectMod.ItemIndex].ID <> 0 then
  begin
    path:= IncludeTrailingPathDelimiter(options.defdir);
    //path:= path + IncludeTrailingPathDelimiter(ExtractFileName(ExcludeTrailingPathDelimiter(ExtractFilePath(LoadedModsList[listSelectMod.ItemIndex].Path))));
    path:= path + IncludeTrailingPathDelimiter(LoadedModsList[listSelectMod.ItemIndex].Name);
  end else
    path:= options.defdir;
  if DirectoryExists(path) then
    directorylistbox1.Directory:= path
  else
    if ForceDirectories(path) then
      directorylistbox1.Directory:= path
    else
      directorylistbox1.Directory:= options.defdir;
end;

procedure TfmMain.SpeedButton1Click(Sender: TObject);
begin
     DirectoryListBox1.FileList := nil;
    FileListBox1.Directory := '.';
    DirectoryListBox1.FileList := FileListBox1 // reset
end;

end.
