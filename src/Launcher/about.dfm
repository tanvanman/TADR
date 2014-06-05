object fmAbout: TfmAbout
  Left = 0
  Top = 0
  BorderIcons = []
  BorderStyle = bsDialog
  Caption = 'About...'
  ClientHeight = 300
  ClientWidth = 329
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  OnClose = FormClose
  OnShow = FormShow
  DesignSize = (
    329
    300)
  PixelsPerInch = 96
  TextHeight = 13
  object lbAboutTitle: TLabel
    Left = 0
    Top = 16
    Width = 330
    Height = 25
    Alignment = taCenter
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    BiDiMode = bdLeftToRight
    Caption = 'The TA Launcher'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -19
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentBiDiMode = False
    ParentFont = False
    ExplicitWidth = 257
  end
  object lbAboutVersion: TLabel
    Left = 0
    Top = 45
    Width = 330
    Height = 21
    Alignment = taCenter
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    BiDiMode = bdLeftToRight
    Caption = 'v 1.0.0.43'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -16
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentBiDiMode = False
    ParentFont = False
  end
  object lbAboutRime: TLabel
    Left = 0
    Top = 74
    Width = 330
    Height = 17
    Cursor = crHandPoint
    Alignment = taCenter
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    BiDiMode = bdLeftToRight
    Caption = 'Programmed by Rime'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlue
    Font.Height = -13
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentBiDiMode = False
    ParentFont = False
    OnClick = lbAboutRimeClick
    ExplicitWidth = 305
  end
  object lbAboutLicense: TLabel
    Left = 52
    Top = 128
    Width = 226
    Height = 17
    Alignment = taCenter
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    BiDiMode = bdLeftToRight
    Caption = 'License: launcher-license.txt'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentBiDiMode = False
    ParentFont = False
    WordWrap = True
  end
  object lbAboutBugs: TLabel
    Left = 52
    Top = 165
    Width = 226
    Height = 16
    Alignment = taCenter
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    BiDiMode = bdLeftToRight
    Caption = 'Please report any bugs on:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentBiDiMode = False
    ParentFont = False
    WordWrap = True
  end
  object lbAboutURL: TLabel
    Left = 52
    Top = 188
    Width = 226
    Height = 16
    Cursor = crHandPoint
    Alignment = taCenter
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    Caption = 'http://www.tauniverse.com'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlue
    Font.Height = -13
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    OnClick = lbAboutURLClick
  end
  object lbAboutDgun: TLabel
    Left = 52
    Top = 225
    Width = 226
    Height = 16
    Alignment = taCenter
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    BiDiMode = bdLeftToRight
    Caption = 'Now go D-Gun some stuff.'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentBiDiMode = False
    ParentFont = False
    WordWrap = True
  end
  object btnAboutClose: TSpeedButton
    Left = 94
    Top = 258
    Width = 145
    Height = 27
    Anchors = [akRight, akBottom]
    Caption = 'okey dokey.!'
    Flat = True
    OnClick = btnAboutCloseClick
    ExplicitTop = 243
  end
  object lbAboutN72: TLabel
    Left = 0
    Top = 97
    Width = 330
    Height = 17
    Cursor = crHandPoint
    Alignment = taCenter
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    BiDiMode = bdLeftToRight
    Caption = 'GUI ideas by N72'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlue
    Font.Height = -13
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentBiDiMode = False
    ParentFont = False
    OnClick = lbAboutN72Click
  end
  object trYuStillHere: TTimer
    Enabled = False
    Interval = 60000
    OnTimer = trYuStillHereTimer
    Left = 268
    Top = 236
  end
end
