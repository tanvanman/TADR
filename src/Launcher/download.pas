{

    The TA Launcher 1.x - Download unit
    Copyright (C) 2013 Rime, N72

    e-mail: plobex@o2.pl

    Licensed under the terms stored in launcher-license.txt

}

unit download;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, Buttons, ComCtrls, ExtActns;

const
  UM_ACTIVATED = WM_USER + 1;

type
  TfmDownload = class(TForm)
    pbDownloadProgress: TProgressBar;
    procedure FormActivate(Sender: TObject);
  private
    procedure UMActivated(var Message: TMessage); message UM_ACTIVATED;
    procedure URL_OnDownloadProgress(Sender: TDownLoadURL; Progress, ProgressMax: Cardinal; StatusCode: TURLDownloadStatus;
         StatusText: String; var Cancel: Boolean);
  public
    function DoDownload: boolean;
  end;

type TDownloadin = record
  sURL: string;
  sFileName: string;
end;
var Downloader : TDownloadin;

var
  fmDownload: TfmDownload;

implementation

{$R *.dfm}

procedure TfmDownload.FormActivate(Sender: TObject);
begin
 PostMessage(Handle, UM_ACTIVATED, 0, 0);
end;

procedure TfmDownload.UMActivated(var Message: TMessage);
begin
 if DoDownload then
 begin
   ModalResult:= mrOK;
 end else
 begin
   ModalResult:= mrCancel;
 end;
end;

procedure TfmDownload.URL_OnDownloadProgress;
begin
   pbDownloadProgress.Max:= ProgressMax;
   pbDownloadProgress.Position:= Progress;
end;

function TfmDownload.DoDownload: boolean;
begin
   with TDownloadURL.Create(self) do
   try
     try
       URL:= Downloader.sURL;
       FileName := Downloader.sFileName;
       OnDownloadProgress := URL_OnDownloadProgress;
       ExecuteTarget(nil);
     except
       on E: Exception do
       begin
         MessageDlg(E.Message,mtWarning, [mbOK], 0);
         Result:= False;
       end;
     end;
   finally
     Result:= True;
     Free;
   end;
end;

end.
