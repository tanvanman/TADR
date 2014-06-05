object fmHelp: TfmHelp
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  BorderStyle = bsDialog
  Caption = 'Help'
  ClientHeight = 307
  ClientWidth = 529
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  OnShow = FormShow
  DesignSize = (
    529
    307)
  PixelsPerInch = 96
  TextHeight = 14
  object lbModName: TLabel
    Left = 0
    Top = 17
    Width = 529
    Height = 21
    Alignment = taCenter
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    Caption = 'lbModName'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -15
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
    ExplicitWidth = 512
  end
  object lbModVersion: TLabel
    Left = 0
    Top = 44
    Width = 529
    Height = 23
    Alignment = taCenter
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    Caption = 'lbModVersion'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -15
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    ExplicitWidth = 512
  end
  object stWebsite: TLabel
    Left = 40
    Top = 197
    Width = 49
    Height = 18
    AutoSize = False
    Caption = 'Website:'
    Layout = tlCenter
  end
  object stForum: TLabel
    Left = 40
    Top = 221
    Width = 38
    Height = 18
    AutoSize = False
    Caption = 'Forum:'
    Layout = tlCenter
  end
  object stReadme: TLabel
    Left = 40
    Top = 245
    Width = 48
    Height = 18
    AutoSize = False
    Caption = 'Readme:'
    Layout = tlCenter
  end
  object stChangelog: TLabel
    Left = 40
    Top = 269
    Width = 61
    Height = 18
    AutoSize = False
    Caption = 'Changelog:'
    Layout = tlCenter
  end
  object lbWebsite: TLabel
    Left = 136
    Top = 197
    Width = 350
    Height = 18
    Cursor = crHandPoint
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    Caption = 'Website:'
    EllipsisPosition = epWordEllipsis
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlue
    Font.Height = -12
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    Layout = tlCenter
    WordWrap = True
    OnClick = lbWebsiteClick
    ExplicitWidth = 333
  end
  object lbForum: TLabel
    Left = 136
    Top = 221
    Width = 350
    Height = 18
    Cursor = crHandPoint
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    Caption = 'http://www.tauniverse.com/forum/forumdisplay.php?f=162'
    EllipsisPosition = epWordEllipsis
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlue
    Font.Height = -12
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    Layout = tlCenter
    OnClick = lbForumClick
    ExplicitWidth = 333
  end
  object lbReadme: TLabel
    Left = 136
    Top = 245
    Width = 350
    Height = 18
    Cursor = crHandPoint
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    Caption = 'Readme:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlue
    Font.Height = -12
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    Layout = tlCenter
    OnClick = lbReadmeClick
    ExplicitWidth = 333
  end
  object lbChangelog: TLabel
    Left = 136
    Top = 269
    Width = 350
    Height = 18
    Cursor = crHandPoint
    Anchors = [akLeft, akTop, akRight]
    AutoSize = False
    Caption = 'Changelog:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clBlue
    Font.Height = -12
    Font.Name = 'Tahoma'
    Font.Style = []
    ParentFont = False
    Layout = tlCenter
    OnClick = lbChangelogClick
    ExplicitWidth = 333
  end
  object gbDescription: TGroupBox
    Left = 40
    Top = 73
    Width = 446
    Height = 113
    Anchors = [akLeft, akTop, akRight]
    Padding.Left = 4
    Padding.Right = 4
    Padding.Bottom = 4
    TabOrder = 0
    ExplicitWidth = 429
    object lbModDescription: TLabel
      Left = 6
      Top = 16
      Width = 434
      Height = 91
      Align = alClient
      Alignment = taCenter
      AutoSize = False
      Caption = 'lbModDescription'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -12
      Font.Name = 'Tahoma'
      Font.Style = []
      ParentFont = False
      Layout = tlCenter
      WordWrap = True
      ExplicitLeft = -4
      ExplicitTop = 14
      ExplicitWidth = 438
      ExplicitHeight = 92
    end
  end
end
