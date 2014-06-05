object fmDownload: TfmDownload
  Left = 0
  Top = 0
  BorderStyle = bsDialog
  Caption = 'Downloading...'
  ClientHeight = 73
  ClientWidth = 302
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  OnActivate = FormActivate
  PixelsPerInch = 96
  TextHeight = 13
  object pbDownloadProgress: TProgressBar
    Left = 24
    Top = 22
    Width = 256
    Height = 27
    TabOrder = 0
  end
end
