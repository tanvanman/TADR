program TA_Demo_Inspector;
uses
  Forms,
  MainForm in 'MainForm.pas' {Form1},
  MemMappedDataStructure in 'MemMappedDataStructure.pas';

{$R *.RES}

begin
  Application.Initialize;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
