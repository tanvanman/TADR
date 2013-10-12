object fmModsAssignList: TfmModsAssignList
  Left = 1459
  Top = 494
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsDialog
  Caption = 'Assign a mod ID to the selected demo file'
  ClientHeight = 455
  ClientWidth = 305
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  OnActivate = FormActivate
  OnShow = FormShow
  DesignSize = (
    305
    455)
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 8
    Top = 4
    Width = 175
    Height = 13
    Caption = 'Select mod from the list and press Ok'
  end
  object btnCancelModAssign: TBitBtn
    Left = 137
    Top = 421
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    TabOrder = 0
    Kind = bkCancel
  end
  object btnAcceptModAssign: TBitBtn
    Left = 217
    Top = 421
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    Enabled = False
    TabOrder = 1
    Kind = bkOK
  end
  object lbModsAssign: TListBox
    Left = 8
    Top = 22
    Width = 289
    Height = 387
    Align = alCustom
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ItemHeight = 16
    ParentFont = False
    ScrollWidth = 5
    TabOrder = 2
    OnClick = lbModsAssignClick
  end
end
