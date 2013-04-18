object fmMain: TfmMain
  Left = 192
  Top = 107
  BorderStyle = bsDialog
  Caption = 'TA Demo Windows 9x patcher'
  ClientHeight = 271
  ClientWidth = 482
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnActivate = FormActivate
  PixelsPerInch = 96
  TextHeight = 13
  object Bevel1: TBevel
    Left = 16
    Top = 160
    Width = 449
    Height = 57
    Shape = bsFrame
  end
  object Label1: TLabel
    Left = 24
    Top = 16
    Width = 433
    Height = 33
    Alignment = taCenter
    AutoSize = False
    Caption = 
      'This application patches TOTALA.EXE so that it can run the TA De' +
      'mo Recorder on Windows 9x systems.  Select a file that should be' +
      ' patched and click on Patch! to proceed.'
    WordWrap = True
  end
  object Label2: TLabel
    Left = 24
    Top = 170
    Width = 61
    Height = 13
    Caption = 'File to patch:'
  end
  object Label3: TLabel
    Left = 24
    Top = 64
    Width = 433
    Height = 33
    Alignment = taCenter
    AutoSize = False
    Caption = 
      'If you fail to patch TOTALA.EXE the TA Demo Recorder will not wo' +
      'rk correctly. Specifically the interface upgrade and the 3D repl' +
      'ayer will not work until this patch is applied.'
    WordWrap = True
  end
  object Label4: TLabel
    Left = 24
    Top = 112
    Width = 433
    Height = 33
    Alignment = taCenter
    AutoSize = False
    Caption = 
      'You can patch all versions of the TA exe (even modified ones use' +
      'd with mods etc). If you later decide to uninstall TA Demo Recor' +
      'der, remember to unpatch your TOTALA.EXE first!'
    WordWrap = True
  end
  object edFile: TEdit
    Left = 24
    Top = 184
    Width = 345
    Height = 21
    TabOrder = 0
    Text = 'edFile'
  end
  object btBrowse: TButton
    Left = 376
    Top = 183
    Width = 75
    Height = 25
    Caption = 'Browse..'
    TabOrder = 1
    OnClick = btBrowseClick
  end
  object btPatch: TButton
    Left = 376
    Top = 232
    Width = 75
    Height = 25
    Caption = 'Patch!'
    Default = True
    TabOrder = 2
    OnClick = btPatchClick
  end
  object btQuit: TButton
    Left = 30
    Top = 232
    Width = 75
    Height = 25
    Caption = 'Quit'
    TabOrder = 3
    OnClick = btQuitClick
  end
  object btUnpatch: TButton
    Left = 288
    Top = 232
    Width = 75
    Height = 25
    Caption = 'Unpatch'
    TabOrder = 4
    OnClick = btUnpatchClick
  end
  object odFile: TOpenDialog
    DefaultExt = 'exe'
    Filter = 'Executable files (*.exe)|*.exe|All files (*.*)|*.*'
    Left = 168
    Top = 224
  end
end
