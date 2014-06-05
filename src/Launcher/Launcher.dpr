{

    The TA Launcher
    Copyright (C) 2013 Rime, N72

    e-mail: plobex@o2.pl

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom
    the Software is furnished to do so, subject to the following conditions:

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
    BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
    AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

}

program Launcher;

{$IFOPT D-}{$WEAKLINKRTTI ON}{$ENDIF}

uses
  Forms,
  main in 'main.pas' {fmLauncherMain},
  settings in 'settings.pas' {fmSettings},
  modslist in 'modslist.pas',
  display in 'display.pas',
  download in 'download.pas' {fmDownload},
  help in 'help.pas' {fmHelp},
  readme in 'readme.pas' {fmReadme},
  about in 'about.pas' {fmAbout},
  CheckPrevious in 'CheckPrevious.pas';

{$R *.res}
begin
  if not CheckPrevious.RestoreIfRunning(Application.Handle, 1) then
  begin
    Application.Initialize;
    Application.ShowMainForm:= False;
    Application.CreateForm(TfmLauncherMain, fmLauncherMain);
  Application.CreateForm(TfmSettings, fmSettings);
  Application.CreateForm(TfmDownload, fmDownload);
  Application.CreateForm(TfmHelp, fmHelp);
  Application.CreateForm(TfmReadme, fmReadme);
  Application.CreateForm(TfmAbout, fmAbout);
  Application.Run;
  end;
end.
