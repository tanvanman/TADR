unit main;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, Registry, ExtCtrls;

const
  VERSION = 'Visual Patcher 1.0';

type
  TfmMain = class(TForm)
    Label1: TLabel;
    Label2: TLabel;
    edFile: TEdit;
    btBrowse: TButton;
    odFile: TOpenDialog;
    Bevel1: TBevel;
    btPatch: TButton;
    btQuit: TButton;
    Label3: TLabel;
    btUnpatch: TButton;
    Label4: TLabel;
    procedure FormActivate(Sender: TObject);
    procedure btBrowseClick(Sender: TObject);
    procedure btQuitClick(Sender: TObject);

    procedure btPatchClick(Sender: TObject);
    procedure btUnpatchClick(Sender: TObject);  private
    { Private declarations }
  public
    { Public declarations }
    inited :boolean;
  end;

var
  fmMain: TfmMain;

implementation

{$R *.dfm}

type
  PFileData = ^TFileData;
  TFileData = array[0..3000000] of char;

function iswin9x :boolean;
var
  reg :TRegistry;
  ver :String;
begin
  reg := TRegistry.Create;
  try
    reg.RootKey := HKEY_LOCAL_MACHINE;
    if reg.OpenKeyReadOnly('SOFTWARE\Microsoft\Windows NT\CurrentVersion') then
    begin
      ver := reg.ReadString ('CurrentVersion');
      result := trim(ver) = '';
    end else
      Result := true;
  finally
    reg.Free;
  end;

//  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion

end;

function Find (st :string; data :PFileData; size :integer) :integer;
var
  pos, findpos :integer;
  tmp :char;
begin
  pos := 0;
  findpos := 1;
  st := lowercase (st);

  while (pos < size) do
  begin
    tmp := data^[pos];
    if tmp in ['A'..'Z'] then
      tmp := char (    byte (data^[pos]) + (byte ('a') - byte('A')));
    if (tmp = st [findpos]) then
      Inc (findpos)
    else
      findpos := 1;
    Inc (pos);

    if (findpos > Length (st)) then
    begin
      Result := pos - findpos;
      exit;
    end;
  end;

  Result := -1;
end;

function PatchTA (fn :String; unpatch :boolean) :boolean;
var
  fd :PFileData;
  f  :file;
  br :integer;
  i, j :integer;
  fs, ts :string;
begin
  if unpatch then
  begin
    fs := 'spank.dll';
    ts := 'ddraw.dll';
  end else
  begin
    fs := 'ddraw.dll';
    ts := 'spank.dll';
  end;

//  ShowMessage (version);

//  ShowMessage ('Patching ' + fn);
  New (fd);
  Result := false;
  try
//    ShowMessage ('Starting patch process');
    AssignFile (f, fn);
    Reset (f, 1);
    BlockRead (f, fd^, 3000000, br);
    CloseFile (f);

//    ShowMessage ('Read file');
    if br <> 1178624 then
    begin
      ShowMessage (version + ': Incorrect filesize. This patcher requires TA 3.1c to work!');
      exit;
    end;

    i := Find (ts, fd, br);
    if i <> -1 then
    begin
      if unpatch then
        ShowMEssage (version + ': File is already unpatched!')
      else
        ShowMEssage (version + ': File is already patched!');
      Result := true;
      exit;
    end;

    i := Find (fs, fd, br);
    if i = -1 then
    begin
      ShowMessage (version + ': Could not find location to patch');
      exit;
    end;

    for j := 1 to 5 do
      fd[i+j] := ts[j];

{    fd[i+1] := 's';
    fd[i+2] := 'p';
    fd[i+3] := 'a';
    fd[i+4] := 'n';
    fd[i+5] := 'k';}

    AssignFile (f, fn);
    Rewrite (f, 1);
    BlockWrite (f, fd^, br);
    CloseFile (f);

    Result := true;
  except
    on e: exception do
    begin
      ShowMessage (version + ': Exception occured (' + e.Message + ')');
      Result := false;
    end;
  end;

  Dispose (fd);
end;


procedure TfmMain.FormActivate(Sender: TObject);
var
  reg :TRegistry;
  tadir :string;
begin
  if inited then
    exit;

  if (not iswin9x) then
  begin
    if (paramstr (1) <> '-silent') then
    begin
      if MessageDlg ('You are not running Windows 9x. Therefore it is not necessary to run this patcher. Do you still want to continue?', mtError, [mbYes, mbNo], 0) <> mrYes then
        Application.Terminate;
    end else
      application.terminate;
  end;

  tadir := '';

  reg := TRegistry.Create;
  try
    reg.RootKey := HKEY_LOCAL_MACHINE;
    if reg.OpenKeyReadOnly('SOFTWARE\Yankspankers\TA Demo') then
      tadir := reg.ReadString ('TA_Dir')
    else
      if reg.OpenKeyReadOnly('SOFTWARE\Microsoft\DirectPlay\Applications\Total Annihilation') then
        tadir := reg.ReadString ('Path');
  finally
    reg.Free;
  end;

  if (tadir = '') then
    tadir := 'C:\Cavedog\Totala';
  if tadir[length(tadir)] = '\' then
    SetLength (tadir, length(tadir) - 1);

  tadir := tadir + '\totala.exe';
  odFile.Filename := tadir;

  edFile.Text := tadir;

  inited := true;
end;

procedure TfmMain.btBrowseClick(Sender: TObject);
begin
  if odFile.Execute then
  begin
    edFile.Text := odfile.FileName;
  end;
end;

procedure TfmMain.btQuitClick(Sender: TObject);
begin
  Application.Terminate;
end;

procedure TfmMain.btPatchClick(Sender: TObject);
begin
  if not FileExists (edFile.Text) then
  begin
    MessageDlg ('The specified file does not exist', mtError, [mbok], 0);
    exit;
  end;

  if PatchTA (edFile.Text, false) then
  begin
    MessageDlg ('Patching successful! A reboot may be required for the patch to take effect.', mtInformation, [mbok], 0);
    Application.Terminate;
  end else
  begin
  end;
end;

procedure TfmMain.btUnpatchClick(Sender: TObject);
begin
  if not FileExists (edFile.Text) then
  begin
    MessageDlg ('The specified file does not exist', mtError, [mbok], 0);
    exit;
  end;

  if PatchTA (edFile.Text, true) then
  begin
    MessageDlg ('Removal of patch successful! A reboot may be required for the removal to take effect.', mtInformation, [mbok], 0);
    Application.Terminate;
  end else
  begin
  end;
end;

end.
