unit waitform;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls;

type
  TfmWait = class(TForm)
    lbWait: TLabel;
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
