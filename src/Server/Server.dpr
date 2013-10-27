program server;

uses
  Forms,
  main in 'main.pas' {fmMain},
  tasv in 'tasv.pas',
  packet in 'packet.pas',
  textdata in 'textdata.pas',
  lobby in 'lobby.pas',
  loading in 'loading.pas',
  savefile in 'savefile.pas',
  replay in 'replay.pas',
  unitsync in 'unitsync.pas',
  logging in 'logging.pas',
  cstream in 'cstream.pas',
  unitid in 'unitid.pas',
  dplay2 in 'dplay2.pas',
  selip in 'selip.pas' {fmSelIP},
  modslist in 'modslist.pas' {fmModsAssignList},
  backwardcompat in 'backwardcompat.pas' {fmBackwardCompat},
  waitform in 'waitform.pas' {fmWait};

{$R *.RES}

begin
  Application.Initialize;
  Application.Title := 'TA Demo Replayer';
  Application.CreateForm(TfmMain, fmMain);
  Application.CreateForm(TfmSelIP, fmSelIP);
  Application.CreateForm(TfmModsAssignList, fmModsAssignList);
  Application.CreateForm(TfmBackwardCompat, fmBackwardCompat);
  Application.CreateForm(TfmWait, fmWait);
  Application.Run;
end.
