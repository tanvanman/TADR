unit modslist;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, Buttons, CheckLst;

type
  TReadedTADRec = record
    modid : string;
    filename : string;
    version: integer;
    usemod: integer;
    error: boolean;
  end;
var ReadedTad : TReadedTADRec;

type
  TfmModsAssignList = class(TForm)
    btnCancelModAssign: TBitBtn;
    btnAcceptModAssign: TBitBtn;
    Label1: TLabel;
    lbModsAssign: TListBox;
    procedure FormShow(Sender: TObject);
    procedure lbModsAssignClick(Sender: TObject);
    procedure FormActivate(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }

    procedure LoadModsListbox;
  end;

var
  fmModsAssignList: TfmModsAssignList;
  setItemIndex: integer;
   
implementation
uses main;

{$R *.dfm}

procedure TfmModsAssignList.FormShow(Sender: TObject);
begin
  LoadModsListbox;
  lbModsAssign.Items.BeginUpdate;
  lbModsAssign.ItemIndex := setItemIndex;
  lbModsAssign.Items.EndUpdate;
end;

procedure TfmModsAssignList.lbModsAssignClick(Sender: TObject);
begin
  if lbModsAssign.SelCount <> 0 then
    btnAcceptModAssign.Enabled:= True;
end;

procedure TfmModsAssignList.LoadModsListbox;
var
  i: word;
begin
  lbModsAssign.Items.Clear;
  lbModsAssign.Items.BeginUpdate;
  for i := Low(main.LoadedModsList) to High(main.LoadedModsList) do
  begin
    lbModsAssign.Items.Add(LoadedModsList[i].Name);
  end;
  lbModsAssign.Items.EndUpdate;
end;

procedure TfmModsAssignList.FormActivate(Sender: TObject);
begin
  lbModsAssign.SetFocus;
end;

end.
