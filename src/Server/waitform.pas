unit waitform;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls;

type
  TfmWait = class(TForm)
    lbWait: TLabel;
    Shape1: TShape;
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  fmWait: TfmWait;

implementation

{$R *.dfm}

end.
