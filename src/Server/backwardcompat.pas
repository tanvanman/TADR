unit backwardcompat;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, Buttons;

type
  TfmBackwardCompat = class(TForm)
    BitBtn1: TBitBtn;
    Label1: TLabel;
    Label2: TLabel;
    cbfmbcompat: TCheckBox;
    BitBtn2: TBitBtn;
    Label3: TLabel;
    procedure FormShow(Sender: TObject);
    procedure FormCreate(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  fmBackwardCompat: TfmBackwardCompat;

implementation
uses main, modslist;

{$R *.dfm}

procedure TfmBackwardCompat.FormShow(Sender: TObject);
begin
 Label2.Caption:= LoadedModsList[fmmain.FindModId(0)].Path;
end;

procedure TfmBackwardCompat.FormCreate(Sender: TObject);
begin
  cbfmbcompat.Checked:= False;
end;

end.
