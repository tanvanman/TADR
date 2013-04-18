program server;

uses
  Forms,
  main in 'main.pas' {fmMain},
  tasv in 'tasv.pas',
  packet in '..\packet.pas',
  textdata in '..\TextData.pas',
  lobby in 'lobby.pas',
  loading in 'loading.pas',
  savefile in 'savefile.pas',
  replay in 'replay.pas',
  unitsync in 'unitsync.pas',
  logging in '..\logging.pas',
  cstream in '..\cstream.pas',
  unitid in 'unitid.pas',
  selip in 'selip.pas' {fmSelIP};

{$R *.RES}

begin
  Application.Initialize;
  Application.CreateForm(TfmMain, fmMain);
  Application.CreateForm(TfmSelIP, fmSelIP);
  Application.Run;
end.
