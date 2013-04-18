object Form1: TForm1
  Left = 264
  Top = 227
  Width = 560
  Height = 146
  Caption = 'Form1'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object DataPanel: TPanel
    Left = 0
    Top = 0
    Width = 552
    Height = 119
    Align = alClient
    TabOrder = 0
    object DataReport: TMemo
      Left = 1
      Top = 1
      Width = 550
      Height = 117
      Align = alClient
      ScrollBars = ssVertical
      TabOrder = 0
    end
  end
  object TAHookCheck: TTimer
    OnTimer = TAHookCheckTimer
    Left = 8
    Top = 8
  end
  object Update: TTimer
    Enabled = False
    Interval = 100
    OnTimer = UpdateTimer
    Left = 8
    Top = 40
  end
end
