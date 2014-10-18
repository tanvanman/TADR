unit GAFSequences;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_GAFSequences : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallGAFSequences;
Procedure OnUninstallGAFSequences;

// -----------------------------------------------------------------------------

procedure LoadExtraGAFAnimationsHook;
procedure CobExplodeHandlerHook;

implementation
uses
  SysUtils,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_FunctionsU;

var
  GAFSequencesPlugin: TPluginData;

const
  EXPLODE6   = 8448;   // Explode6 glow & smoke
  EXPLODE7   = 8704;   // Explode7 glow & smoke
  EXPLODE8   = 8960;   // Explode8 glow & smoke
  EXPLODE9   = 9216;   // Explode9 glow & smoke
  EXPLODE10  = 9472;   // Explode10 glow & smoke
  ANIMCUST1  = 9728;   // CustAnim1 no glow, no smoke
  ANIMCUST2  = 9984;   // CustAnim2 no glow, no smoke
  ANIMCUST3  = 10240;  // CustAnim3 no glow, no smoke
  ANIMCUST4  = 10496;  // CustAnim4 no glow, no smoke
  ANIMCUST5  = 10752;  // CustAnim5 no glow, no smoke
  ANIMCUST6  = 11008;  // CustAnim6 no glow, no smoke
  ANIMCUST7  = 11264;  // CustAnim7 no glow, no smoke
  ANIMCUST8  = 11520;  // CustAnim8 no glow, no smoke
  ANIMCUST9  = 11776;  // CustAnim9 no glow, no smoke
  ANIMCUST10 = 12032;  // CustAnim10 no glow, no smoke

Procedure OnInstallGAFSequences;
begin
end;

Procedure OnUninstallGAFSequences;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_GAFSequences then
  begin
    GAFSequencesPlugin := TPluginData.Create( True,
                            'GAFSequences',
                            State_GAFSequences,
                            @OnInstallGAFSequences,
                            @OnUninstallGAFSequences );

    GAFSequencesPlugin.MakeRelativeJmp(State_GAFSequences,
                                         'Load extra gaf sequences from FX.GAF',
                                         @LoadExtraGAFAnimationsHook,
                                         $00429AC5, 1);

    GAFSequencesPlugin.MakeRelativeJmp(State_GAFSequences,
                                         'COB explode handler with new sequences',
                                         @CobExplodeHandlerHook,
                                         $00481268, 2);

    Result:= GAFSequencesPlugin;
  end else
    Result := nil;
end;

procedure LoadExtraGAFAnimations(GAFHandle: Pointer); stdcall;
var
  i, j: integer;
  pSeq: Pointer;
begin
  j := 6;
  i := 0;
  while True do
  begin
    pSeq := GAF_Name2Sequence(GAFHandle, PAnsiChar('Explode' + IntToStr(j)));
    if pSeq <> nil then
    begin
      SetLength(ExtraGAFAnimations.Explode, High(ExtraGAFAnimations.Explode) + 2);
      PGAFSequence(pSeq).Signature := 0;
      ExtraGAFAnimations.Explode[i] := pSeq;
      Inc(i);
      Inc(j);
    end else
      Break;
  end;

  i := 0;
  while True do
  begin
    pSeq := GAF_Name2Sequence(GAFHandle, PAnsiChar('CustAnim' + IntToStr(i + 1)));
    if pSeq <> nil then
    begin
      SetLength(ExtraGAFAnimations.CustAnim, High(ExtraGAFAnimations.CustAnim) + 2);
      PGAFSequence(pSeq).Signature := 0;
      ExtraGAFAnimations.CustAnim[i] := pSeq;
      Inc(i);
    end else
      Break;
  end;

  i := 0;
  while True do
  begin
    pSeq := GAF_Name2Sequence(GAFHandle, PAnsiChar('flamestream' + IntToStr(i + 2)));
    if pSeq <> nil then
    begin
      SetLength(ExtraGAFAnimations.FlameStream, High(ExtraGAFAnimations.FlameStream) + 2);
      ExtraGAFAnimations.FlameStream[i] := pSeq;
      Inc(i);
    end else
      Break;
  end;
end;

procedure LoadExtraGAFAnimationsHook;
asm
//  mov     [eax+2], bl
  pushAD
  push    esi
  call    LoadExtraGAFAnimations
  popAD
  mov     ecx, [TADynmemStructPtr]
  push $00429ACB;
  call PatchNJump;
end;

procedure CobExplodeHandler(SeqType: Cardinal; Position: PPosition); stdcall;
var
  pSeq : Pointer;
  SeqId : Integer;
  Glow, Smoke : Integer;
begin
  // ShowExplodeGaf last 2 params
  // 0 = add smoke, 1 = exclude
  // -1 = don't add glow
  SeqType := SeqType and not 32;
  SeqType := SeqType and not 8192;

  if (SeqType <= EXPLODE10) then
  begin
    SeqId := (SeqType div $100) - 1;
    pSeq :=  ExtraGAFAnimations.Explode[SeqId];
    Glow := 2;
    Smoke := 0;
  end else
  begin
    SeqId := (SeqType div $100) - 5 - 1;
    pSeq := ExtraGAFAnimations.CustAnim[SeqId];
    Glow := -1;
    Smoke := 1;
  end;
  if pSeq <> nil then
    ShowExplodeGaf(Position, LongWord(pSeq), Glow, Smoke);
end;

procedure CobExplodeHandlerHook;
label
  CustomSequences;
asm
  mov     [esp+10h], edx
  mov     eax, [eax+8]
  cmp     ebx, 8224 // nuke1 + bitmaponly 8192 + 32
//  cmp     bh, 32
  ja      CustomSequences
  push $0048126F;
  call PatchNJump;
CustomSequences :
  lea     eax, [esp+$C] // seq draw position
  push    eax
  push    ebx
  call    CobExplodeHandler
  push $00481333;
  call PatchNJump;
end;

end.
