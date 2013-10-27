object fmBackwardCompat: TfmBackwardCompat
  Left = 228
  Top = 692
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsDialog
  Caption = 'Confirm'
  ClientHeight = 175
  ClientWidth = 382
  Color = clBtnFace
  Font.Charset = EASTEUROPE_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  OnCreate = FormCreate
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 8
    Top = 8
    Width = 365
    Height = 41
    Alignment = taCenter
    AutoSize = False
    Caption = 
      'Selected demo has been recorded with older version of TA Demo Re' +
      'corder'#13#10'or backward compatibility setting was used. You can choo' +
      'se now:'#13#10'Play this demo as mod #0 with current path:'
    WordWrap = True
  end
  object Label2: TLabel
    Left = 8
    Top = 52
    Width = 365
    Height = 13
    Alignment = taCenter
    AutoSize = False
  end
  object Label3: TLabel
    Left = 8
    Top = 100
    Width = 365
    Height = 29
    Alignment = taCenter
    AutoSize = False
    Caption = 
      'or cancel the playback and manually assign the demo file to a ex' +
      'isting game setup'
    WordWrap = True
  end
  object BitBtn1: TBitBtn
    Left = 92
    Top = 136
    Width = 101
    Height = 25
    TabOrder = 0
    Kind = bkOK
  end
  object cbfmbcompat: TCheckBox
    Left = 96
    Top = 72
    Width = 197
    Height = 17
    Caption = 'Always use this path for older demos'
    TabOrder = 1
  end
  object BitBtn2: TBitBtn
    Left = 204
    Top = 136
    Width = 95
    Height = 25
    TabOrder = 2
    Kind = bkCancel
  end
end
