unit main;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, textdata, unitlist;

type
  TForm1 = class(TForm)
    lbUnits: TListBox;
    btLoad: TButton;
    Button1: TButton;
    OpenDialog1: TOpenDialog;
    Button2: TButton;
    SaveDialog1: TSaveDialog;
    procedure btLoadClick(Sender: TObject);
    procedure btSaveClick(Sender: TObject);
    procedure Button1Click(Sender: TObject);
    procedure Button2Click(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    units :TUnitList;
  end;

var
  Form1: TForm1;

implementation

{$R *.DFM}

procedure TForm1.btLoadClick(Sender: TObject);
var
  i :integer;
  u :PUnit;
begin
  if opendialog1.execute then
  begin
    lbunits.items.clear;
    units := TUnitlist.Create (opendialog1.filename);
    for i := 0 to units.units.count - 1 do
    begin
      u := units.units.items[i];
      if u.limit < 101 then
        lbUnits.Items.Add (HexToStr (u.id, 8) + ' - ' + inttostr (u.limit));
    end;
  end;
end;

procedure TForm1.btSaveClick(Sender: TObject);
begin
//  units.Save (edSave.Text);
end;

procedure TForm1.Button1Click(Sender: TObject);
begin
  Application.Terminate;
end;

procedure TForm1.Button2Click(Sender: TObject);
begin
  if savedialog1.execute then
    units.Save (savedialog1.filename);
end;

end.
