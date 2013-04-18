unit main;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, Menus, ComCtrls, ExtCtrls, tasv, savefile, dplay, ActiveX,
  FileCtrl, ShellApi, Registry, textdata, unitid, unitsync{, dplobby};

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
    Button3: TButton;
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
    Button7: TButton;
    Button8: TButton;
    FileListBox1: TFileListBox;
    DirectoryListBox1: TDirectoryListBox;
    DriveComboBox1: TDriveComboBox;
    FilterComboBox1: TFilterComboBox;
    meGameInfo: TMemo;
    Button6: TButton;
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
    Label21: TLabel;
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
    procedure nbMainPageChanged(Sender: TObject);
    procedure FormActivate(Sender: TObject);
    procedure Button1Click(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure Button3Click(Sender: TObject);
    procedure Button4Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
    procedure Button5Click(Sender: TObject);
    procedure Button7Click(Sender: TObject);
    procedure procmess;
    procedure Button8Click(Sender: TObject);
    procedure FileListBox1Click(Sender: TObject);
    procedure FileListBox1Change(Sender: TObject);
    procedure tbSpeedChange(Sender: TObject);
    procedure Label12Click(Sender: TObject);
    procedure Button6Click(Sender: TObject);
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
  private
    { Private declarations }
    procedure ShowHint (Sender: TObject);
    procedure LoadOptions;
    procedure SaveOptions;
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
  end;

var
  fmMain: TfmMain;

var
  options :TOptions;

implementation

{$R *.DFM}

uses
  lobby, selip;

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
      0 :directorylistbox1.directory := options.lastdir;
      1 :directorylistbox1.directory := options.defdir;
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

procedure TfmMain.Button3Click(Sender: TObject);
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

procedure TfmMain.Button7Click(Sender: TObject);
begin
  meGameinfo.Lines.Add ('');
  meGameinfo.Lines.Add ('Processing demo file, this might take a few minutes');
  SaveOptions;
  Application.ProcessMessages;
  save := TSaveFile.Create (filelistbox1.items[filelistbox1.itemindex], false);
  if save.Error <> sfNone then
  begin
    MessageDlg ('Invalid file selected', mtError, [mbok], 0);
  end else
    nbMain.PageIndex := 0;
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
  Application.Terminate;
end;

procedure TfmMain.FileListBox1Click(Sender: TObject);
var
  i :integer;
  l :TStringList;
  tmp :string;
  us :TUnitSync;
begin
  save := TSaveFile.Create (filelistbox1.items[filelistbox1.itemindex], true);
  if save.error = sfNone then
  begin
    meGameInfo.Lines.Clear;
    megameinfo.lines.beginupdate;
//  megameinfo.visible := false;
    meGameInfo.Lines.Add ('Name of file: ' + filelistbox1.items[filelistbox1.itemindex]);
    meGameInfo.Lines.Add ('Recorded with version: ' +save.vername);
    if save.datum<>'' then
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

    Button7.Enabled := true;
    Button6.Enabled := true;
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
    Button7.Enabled := false;
    Button6.Enabled := false;

  end;

  //Scrollar upp..
  meGameInfo.Lines.Insert (0, 'hey man');
  meGameInfo.Lines.Delete (0);
  megameinfo.lines.endupdate;

//  megameinfo.visible := true;
  save.Free;
end;

procedure TfmMain.FileListBox1Change(Sender: TObject);
begin
//  Button7.Enabled := false;
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

procedure TfmMain.Button6Click(Sender: TObject);
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

procedure TfmMain.Button10Click(Sender: TObject);
var
  r :TRegistry;
  path :string;
  param :string;
  ip :string;
begin
  path := Trim(options.Tadir);

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

  path := path + '\totala.exe';

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
    param := '/n1:' + ip;
  end;

  ShellExecute (0, nil, @path[1], @param[1], nil, SW_NORMAL);
end;

procedure TfmMain.btUnitsClick(Sender: TObject);
begin
  nbmain.pageindex := 6;

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
  options.tadir := FixPath(edTADir.Text);
  options.usedir := rgUseDir.ItemIndex;
  options.defdir := FixPath(edDemoDir.Text);
  options.syncspeed := tbSync.Position;
  options.interval := tbinterval.Position;
  options.smooth := tbsmooth.Position;

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
  r := TRegIniFile.Create ('Software\Yankspankers\TA Demo');

  options.Tadir := r.ReadString ('Options', 'TADir', '');
  options.usedir := r.ReadInteger ('Options', 'usedir', 0);
  options.defdir := r.readstring ('Options', 'defdir', '');
  options.lastdir := r.ReadString ('Options', 'lastdir', '');
  options.syncspeed := r.ReadInteger ('Options', 'sync', 10);
  options.interval := r.ReadInteger ('Options', 'interval', 90);
  options.smooth := r.ReadInteger ('Options', 'smooth', 3);
  options.lastprot := r.ReadInteger ('Options', 'lastprot', -1);

  options.usecomp := r.ReadBool ('Options', 'usecomp', true);
  options.usenewtimer := r.Readbool ('Options', 'newtimer', true);
  options.ta3d := r.ReadBool ('Options', 'ta3d', true);
  options.fixall := r.readbool ('Options', 'fixall', false);
  options.autorec := r.readbool ('Options', 'autorec', false);
  options.sharemappos := r.readbool ('Options', 'sharepos', false);
  options.createtxt := r.readbool ('Options', 'createtxt', false);
  options.skippause := r.readbool ('Options', 'skippause', true);
  options.usenewtimer := r.readbool ('Options', 'newtime', true);
  options.playernames := r.readbool ('Options', 'playernames', false);

  r.WriteString ('Options', 'serverdir', ExtractFilePath (paramstr (0)));
  r.Free;
end;

procedure TfmMain.SaveOptions;
var
  r :TRegIniFile;
begin
  r := TRegIniFile.Create ('Software\Yankspankers\TA Demo');

  r.WriteString ('Options', 'TADir', options.tadir);
  r.writeinteger ('Options', 'usedir', options.usedir);
  r.writestring ('Options', 'defdir', options.defdir);
  r.writestring ('Options', 'lastdir', directorylistbox1.Directory);
  r.writeinteger ('Options', 'sync', options.syncspeed);
  r.writeinteger ('Options', 'interval', options.interval);
  r.writeinteger ('Options', 'smooth', options.smooth);
  r.writeinteger ('Options', 'lastprot', options.lastprot);

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


end.
