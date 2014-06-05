{

    The TA Launcher 1.x - Readme unit
    Copyright (C) 2013 Rime, N72

    e-mail: plobex@o2.pl

    Licensed under the terms stored in launcher-license.txt

}

unit readme;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ComCtrls;

type
  TfmReadme = class(TForm)
    reReadme: TRichEdit;
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  fmReadme: TfmReadme;

implementation

{$R *.dfm}

procedure TfmReadme.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  reReadme.Clear;
end;

end.
