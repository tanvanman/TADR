program idanalyze;

uses
  Forms,
  main in 'main.pas' {Form1},
  unitlist in 'unitlist.pas',
  textdata in '..\TextData.pas',
  cstream in '..\cstream.pas';

{$R *.RES}

begin
  Application.Initialize;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
