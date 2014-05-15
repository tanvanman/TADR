object fmModsAssignList: TfmModsAssignList
  Left = 1459
  Top = 494
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsDialog
  Caption = 'Assign a mod ID to the selected demo file'
  ClientHeight = 374
  ClientWidth = 305
  Color = clBtnFace
  Font.Charset = EASTEUROPE_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  OnActivate = FormActivate
  OnShow = FormShow
  DesignSize = (
    305
    374)
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 8
    Top = 12
    Width = 178
    Height = 13
    Caption = 'Select mod from the list and press Ok'
  end
  object btnCancelModAssign: TBitBtn
    Left = 217
    Top = 340
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 0
    NumGlyphs = 2
  end
  object btnAcceptModAssign: TBitBtn
    Left = 137
    Top = 340
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    Caption = 'OK'
    Default = True
    Enabled = False
    ModalResult = 1
    TabOrder = 1
    NumGlyphs = 2
  end
  object lbModsAssign: TListBox
    Left = 8
    Top = 30
    Width = 289
    Height = 299
    Align = alCustom
    Font.Charset = EASTEUROPE_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'Tahoma'
    Font.Style = []
    ItemHeight = 16
    ParentFont = False
    ScrollWidth = 5
    TabOrder = 2
    OnClick = lbModsAssignClick
  end
end
