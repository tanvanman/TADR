{

    The TA Launcher 1.x - Help unit
    Copyright (C) 2013 Rime, N72

    e-mail: plobex@o2.pl

    Licensed under the terms stored in launcher-license.txt

}

unit help;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ShellApi, IniFiles, ExtCtrls;

type
  TfmHelp = class(TForm)
    lbModName: TLabel;
    lbModVersion: TLabel;
    lbModDescription: TLabel;
    stWebsite: TLabel;
    stForum: TLabel;
    stReadme: TLabel;
    stChangelog: TLabel;
    lbWebsite: TLabel;
    lbForum: TLabel;
    lbReadme: TLabel;
    gbDescription: TGroupBox;
    lbChangelog: TLabel;
    procedure lbReadmeClick(Sender: TObject);
    procedure lbChangelogClick(Sender: TObject);
    procedure lbForumClick(Sender: TObject);
    procedure lbWebsiteClick(Sender: TObject);
    procedure FormShow(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  fmHelp: TfmHelp;

implementation

uses readme, modslist, main;

{$R *.dfm}

procedure TfmHelp.FormShow(Sender: TObject);
var
 ItemIndex: integer;
begin
  ItemIndex:= fmLauncherMain.cbModsList.ItemIndex;
  lbModName.Caption:= '';
  lbModVersion.Caption:= '';
  lbModDescription.Caption:= '';

  lbWebsite.Caption:= '';
  lbForum.Caption:= '';
  lbReadme.Caption:= '';
  lbChangelog.Caption:= '';

  lbWebsite.Visible:= False;
  lbForum.Visible:= False;
  lbReadme.Visible:= False;
  lbChangelog.Visible:= False;

  stWebsite.Visible:= False;
  stForum.Visible:= False;
  stReadme.Visible:= False;
  stChangelog.Visible:= False;

  lbModName.Caption:= LoadedModsList[ItemIndex].Name;
  lbModVersion.Caption:= LoadedModsList[ItemIndex].Version;

  with TIniFile.Create(LoadedModsList[ItemIndex].InfoPath) do
    try
      lbModDescription.Caption:= ReadString('info', 'Description', '');
      LoadedModsList[ItemIndex].Website:= ReadString('info', 'Website', '');
      LoadedModsList[ItemIndex].Forum:= ReadString('info', 'Forum', '');
      lbWebsite.Caption:= ReadString('info', 'Website', '');
      lbForum.Caption:= ReadString('info', 'Forum', '');
      if ReadString('info', 'Readme', '') <> '' then
      begin
        lbReadme.Caption:= 'Click to open';
        LoadedModsList[ItemIndex].Readme:= IncludeTrailingPathDelimiter(ExtractFilePath(LoadedModsList[ItemIndex].Path)) +
          ReadString('info', 'Readme', '');
      end;
      if ReadString('info', 'Changelog', '') <> '' then
      begin
        lbChangelog.Caption:= 'Click to open';
        LoadedModsList[ItemIndex].Changelog:= IncludeTrailingPathDelimiter(ExtractFilePath(LoadedModsList[ItemIndex].Path)) +
          ReadString('info', 'Changelog', '');
      end;
    finally
      Free;
    end;

  lbForum.Visible:= False;
  lbReadme.Visible:= False;
  lbChangelog.Visible:= False;
  //fmHelp.ClientHeight:= 241;

  if lbWebsite.Caption <> '' then
  begin
    //fmHelp.ClientHeight:= 286;
    stWebsite.Visible:= True;
    lbWebsite.Visible:= True;
  end;

  if lbForum.Caption <> '' then
  begin
    //fmHelp.ClientHeight:= 336;
    stForum.Visible:= True;
    lbForum.Visible:= True;
  end;

  if lbReadme.Caption <> '' then
  begin
    //fmHelp.ClientHeight:= 386;
    stReadme.Visible:= True;
    lbReadme.Visible:= True;
  end;

  if lbChangelog.Caption <> '' then
  begin
    //fmHelp.ClientHeight:= 320;
    stChangelog.Visible:= True;
    lbChangelog.Visible:= True;
  end;
end;

procedure TfmHelp.lbChangelogClick(Sender: TObject);
begin
  try
    fmReadme.reReadme.Lines.LoadFromFile(LoadedModsList[fmLauncherMain.cbModsList.ItemIndex].Changelog);
  except
    ShowMessage('Changelog file is missing.');
    Exit;
  end;
  fmReadme.Caption:= 'Change log';
  fmReadme.Show;
end;

procedure TfmHelp.lbForumClick(Sender: TObject);
var
  s :string;
begin
  s := LoadedModsList[fmLauncherMain.cbModsList.ItemIndex].Forum;
  ShellExecute(0, nil, @s[1], nil, nil, SW_NORMAL);
end;

procedure TfmHelp.lbReadmeClick(Sender: TObject);
begin
  try
    fmReadme.reReadme.Lines.LoadFromFile(LoadedModsList[fmLauncherMain.cbModsList.ItemIndex].Readme);
  except
    ShowMessage('Readme file is missing.');
    Exit;
  end;
  fmReadme.Caption:= 'Readme';
  fmReadme.Show;
end;

procedure TfmHelp.lbWebsiteClick(Sender: TObject);
var
  s :string;
begin
  s := LoadedModsList[fmLauncherMain.cbModsList.ItemIndex].Website;
  ShellExecute(0, nil, @s[1], nil, nil, SW_NORMAL);
end;

end.
