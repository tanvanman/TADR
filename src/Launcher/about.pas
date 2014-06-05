unit about;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ShellAPI, Buttons, ExtCtrls;

type
  TfmAbout = class(TForm)
    lbAboutTitle: TLabel;
    lbAboutVersion: TLabel;
    lbAboutRime: TLabel;
    lbAboutLicense: TLabel;
    lbAboutBugs: TLabel;
    lbAboutURL: TLabel;
    lbAboutDgun: TLabel;
    btnAboutClose: TSpeedButton;
    trYuStillHere: TTimer;
    lbAboutN72: TLabel;
    procedure lbAboutURLClick(Sender: TObject);
    procedure btnAboutCloseClick(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure trYuStillHereTimer(Sender: TObject);
    procedure lbAboutRimeClick(Sender: TObject);
    procedure lbAboutN72Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  fmAbout: TfmAbout;
  stillhere: byte;

implementation

uses main;

{$R *.dfm}

procedure TfmAbout.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  trYuStillHere.Enabled:= False;
end;

procedure TfmAbout.FormShow(Sender: TObject);
begin
  trYuStillHere.Enabled:= True;
end;

procedure TfmAbout.lbAboutN72Click(Sender: TObject);
var
  s :string;
begin
  s := 'http://www.tauniverse.com/forum/member.php?u=20487';
  ShellExecute(0, nil, @s[1], nil, nil, SW_NORMAL);
end;

procedure TfmAbout.lbAboutRimeClick(Sender: TObject);
var
  s :string;
begin
  s := 'http://www.tauniverse.com/forum/member.php?u=9591';
  ShellExecute(0, nil, @s[1], nil, nil, SW_NORMAL);
end;

procedure TfmAbout.lbAboutURLClick(Sender: TObject);
var
  s :string;
begin
  s := 'http://www.tauniverse.com';
  ShellExecute(0, nil, @s[1], nil, nil, SW_NORMAL);
end;

procedure TfmAbout.trYuStillHereTimer(Sender: TObject);
begin
  Inc(stillhere);
  trYuStillHere.Enabled:= False;
  case stillhere of
    1: begin
         ShowMessage('This is the last warning. GO AND PLAY some TA maaaan... '+#10#13+'...please ? :C');
         trYuStillHere.Enabled:= True;
       end;
    2: begin
         ShowMessage('I''m done. Launching it for you right now, simply because I''m The Launcher. (ba dum tsss...)');
         trYuStillHere.Enabled:= False;
         fmLauncherMain.btnLaunch.Click;
       end else
         if stillhere > 69 then
           stillhere:= 0;
  end;
end;

procedure TfmAbout.btnAboutCloseClick(Sender: TObject);
begin
  ModalResult:= mrOk;
end;

end.
