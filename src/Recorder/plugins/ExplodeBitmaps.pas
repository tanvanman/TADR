unit ExplodeBitmaps;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_ExplodeBitmaps : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallExplodeBitmaps;
Procedure OnUninstallExplodeBitmaps;

// -----------------------------------------------------------------------------

procedure LoadExtraBitmaps;
procedure InGameChoseBitmap;

implementation
uses
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_FunctionsU;

const
  StrExplode6   : AnsiString = 'Explode6';
  StrExplode7   : AnsiString = 'Explode7';
  StrExplode8   : AnsiString = 'Explode8';
  StrExplode9   : AnsiString = 'Explode9';
  StrExplode10  : AnsiString = 'Explode10';
  StrCustAnim1  : AnsiString = 'CustAnim1';
  StrCustAnim2  : AnsiString = 'CustAnim2';
  StrCustAnim3  : AnsiString = 'CustAnim3';
  StrCustAnim4  : AnsiString = 'CustAnim4';
  StrCustAnim5  : AnsiString = 'CustAnim5';
  StrCustAnim6  : AnsiString = 'CustAnim6';
  StrCustAnim7  : AnsiString = 'CustAnim7';
  StrCustAnim8  : AnsiString = 'CustAnim8';
  StrCustAnim9  : AnsiString = 'CustAnim9';
  StrCustAnim10 : AnsiString = 'CustAnim10';

var
  ExplodeBitmapsPlugin: TPluginData;

Procedure OnInstallExplodeBitmaps;
begin
end;

Procedure OnUninstallExplodeBitmaps;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_ExplodeBitmaps then
  begin
    ExplodeBitmapsPlugin := TPluginData.Create( True,
                            'ExplodeBitmaps',
                            State_ExplodeBitmaps,
                            @OnInstallExplodeBitmaps,
                            @OnUninstallExplodeBitmaps );

    ExplodeBitmapsPlugin.MakeRelativeJmp(State_ExplodeBitmaps,
                                         'Load Extra Bitmaps',
                                         @LoadExtraBitmaps,
                                         $00429AC5, 0);

    ExplodeBitmapsPlugin.MakeRelativeJmp(State_ExplodeBitmaps,
                                         'In Game Chose Bitmap',
                                         @InGameChoseBitmap,
                                         $0048126F, 0); 

    Result:= ExplodeBitmapsPlugin;
  end else
    Result := nil;
end;

{
.text:00429A9A 120 E8 A1 F2 08 00                                                  call    Name2Sequence_Gaf

.text:00429A9F 118 8B 0D E8 1D 51 00                                               mov     ecx, TAMainStructPtr
.text:00429AA5 118 68 FC 35 50 00                                                  push    offset aExplode5 ; "explode5"     // puts another name but before
.text:00429AAA 11C 56                                                              push    esi             ; GafStruct       // name2gaf call
.text:00429AAB 120 89 81 03 48 01 00                                               mov     [ecx+TAMainStruct.explode4], eax  // fills pointer to anim before...
.text:00429AB1 120 8B 15 E8 1D 51 00                                               mov     edx, TAMainStructPtr
.text:00429AB7 120 8B 82 03 48 01 00                                               mov     eax, [edx+TAMainStruct.explode4]
.text:00429ABD 120 88 58 02                                                        mov     [eax+2], bl                       // and indicates it as loaded
.text:00429AC0 120 E8 7B F2 08 00                                                  call    Name2Sequence_Gaf

.text:00429AC5 118 8B 0D E8 1D 51 00                                               mov     ecx, TAMainStructPtr
.text:00429ACB 118 68 F4 35 50 00                                                  push    offset aNuke1   ; "nuke1"
}

procedure LoadExtraBitmaps;
asm
  mov     [eax+2], bl
  push    eax                 // preserve bitmap 5 readings
  mov     ecx, StrExplode6
  push    ecx
  push    esi
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx], eax

  mov     ecx, StrExplode7
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+4], eax
  mov     ecx, StrExplode8
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+8], eax

  mov     ecx, StrExplode9
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+$C], eax

  mov     ecx, StrExplode10
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+$10], eax

  mov     ecx, StrCustAnim1
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+$14], eax

  mov     ecx, StrCustAnim2
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+$18], eax

  mov     ecx, StrCustAnim3
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+$1C], eax

  mov     ecx, StrCustAnim4
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+$20], eax

  mov     ecx, StrCustAnim5
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+$24], eax

  mov     ecx, StrCustAnim6
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+$28], eax

  mov     ecx, StrCustAnim7
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+$2C], eax

  mov     ecx, StrCustAnim8
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+$30], eax

  mov     ecx, StrCustAnim9
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+$34], eax

  mov     ecx, StrCustAnim10
  push    ecx
  push    esi
  mov     [eax+2], bl
  call    GAF_Name2Sequence
  lea     ecx, ExtraAnimations
  mov     [ecx+$38], eax
  mov     [eax+2], bl
  pop     eax

  mov     ecx, [TADynmemStructPtr]
  push $00429ACB;
  call PatchNJump;
end;

procedure InGameChoseBitmap;
label
  Explode2Check,
  Explode3Check,
  Explode4Check,
  Explode5Check,
  Nuke1Check,
  Explode6Check,
  Explode7Check,
  Explode8Check,
  Explode9Check,
  Explode10Check,
  CustAnim1Check,
  CustAnim2Check,
  CustAnim3Check,
  CustAnim4Check,
  CustAnim5Check,
  CustAnim6Check,
  CustAnim7Check,
  CustAnim8Check,
  CustAnim9Check,
  CustAnim10Check,
  EndChecks;
asm
  // ShowExplodeGaf last 2 params
  // 0 = add smoke, 1 = exclude
  // -1 = don't add glow
  mov     [esp+54h-$40], eax
//  test    bh, 1
//  jz      Explode2Check
  cmp     bh, 32
  ja      Explode6Check
  test    bh, 1
  jz      Explode2Check
  mov     ecx, [TADynmemStructPtr]
  push    0
  push    2
  lea     eax, [esp+5Ch-$48]
  mov     edx, [ecx+$147F7]
  push    edx
  push    eax
  call    ShowExplodeGaf
Explode2Check :
  test    bh, 2
  jz      Explode3Check
  mov     ecx, [TADynmemStructPtr]
  push    0
  push    2
  lea     eax, [esp+5Ch-$48]
  mov     edx, [ecx+$147FB]
  push    edx
  push    eax
  call    ShowExplodeGaf
Explode3Check :
  test    bh, 4
  jz      Explode4Check
  mov     ecx, [TADynmemStructPtr]
  push    0
  push    2
  lea     eax, [esp+5Ch-$48]
  mov     edx, [ecx+$147FF]
  push    edx
  push    eax
  call    ShowExplodeGaf
Explode4Check :
  test    bh, 8
  jz      Explode5Check
  mov     ecx, [TADynmemStructPtr]
  push    0
  push    2
  lea     eax, [esp+5Ch-$48]
  mov     edx, [ecx+$14803]
  push    edx
  push    eax
  call    ShowExplodeGaf
Explode5Check :
  test    bh, 16
  jz      Nuke1Check
  mov     ecx, [TADynmemStructPtr]
  push    0
  push    2
  lea     eax, [esp+5Ch-$48]
  mov     edx, [ecx+$14807]
  push    edx
  push    eax
  call    ShowExplodeGaf
Nuke1Check :
  test    bh, 32
  jz      EndChecks
  mov     ecx, [TADynmemStructPtr]
  push    0
  push    2
  lea     eax, [esp+5Ch-$48]
  mov     edx, [ecx+$1480B]
  push    edx
  push    eax
  call    ShowExplodeGaf
  jmp EndChecks
Explode6Check :
  cmp     bh, 33
  jne     Explode7Check
  push    0
  push    2
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx]
  push    edx
  push    eax
  call    ShowExplodeGaf
Explode7Check :
  cmp     bh, 34
  jne     Explode8Check
  push    0
  push    2
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$4]
  push    edx
  push    eax
  call    ShowExplodeGaf
Explode8Check :
  cmp     bh, 35
  jne     Explode9Check
  push    0
  push    2
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$8]
  push    edx
  push    eax
  call    ShowExplodeGaf
Explode9Check :
  cmp     bh, 36
  jne     Explode10Check
  push    0
  push    2
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$C]
  push    edx
  push    eax
  call    ShowExplodeGaf
Explode10Check :
  cmp     bh, 37
  jne     CustAnim1Check
  push    1
  push    2
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$10]
  push    edx
  push    eax
  call    ShowExplodeGaf
CustAnim1Check :
  cmp     bh, 38
  jne     CustAnim2Check
  push    1
  push    -1
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$14]
  push    edx
  push    eax
  call    ShowExplodeGaf
CustAnim2Check :
  cmp     bh, 39
  jne     CustAnim3Check
  push    1
  push    -1
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$18]
  push    edx
  push    eax
  call    ShowExplodeGaf
CustAnim3Check :
  cmp     bh, 40
  jne     CustAnim4Check
  push    1
  push    -1
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$1C]
  push    edx
  push    eax
  call    ShowExplodeGaf
CustAnim4Check :
  cmp     bh, 41
  jne     CustAnim5Check
  push    1
  push    -1
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$20]
  push    edx
  push    eax
  call    ShowExplodeGaf
CustAnim5Check :
  cmp     bh, 42
  jne     CustAnim6Check
  push    1
  push    -1
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$24]
  push    edx
  push    eax
  call    ShowExplodeGaf
CustAnim6Check :
  cmp     bh, 43
  jne     CustAnim7Check
  push    1
  push    -1
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$28]
  push    edx
  push    eax
  call    ShowExplodeGaf
CustAnim7Check :
  cmp     bh, 44
  jne     CustAnim8Check
  push    1
  push    -1
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$2C]
  push    edx
  push    eax
  call    ShowExplodeGaf
CustAnim8Check :
  cmp     bh, 45
  jne     CustAnim9Check
  push    1
  push    -1
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$30]
  push    edx
  push    eax
  call    ShowExplodeGaf
CustAnim9Check :
  cmp     bh, 46
  jne     CustAnim10Check
  push    1
  push    -1
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$34]
  push    edx
  push    eax
  call    ShowExplodeGaf
CustAnim10Check :
  cmp     bh, 47
  jne     EndChecks
  push    1
  push    -1
  lea     eax, [esp+5Ch-$48]
  lea     ecx, ExtraAnimations
  mov     edx, [ecx+$38]
  push    edx
  push    eax
  call    ShowExplodeGaf
EndChecks :
  push $00481333;
  call PatchNJump;
end;

end.

