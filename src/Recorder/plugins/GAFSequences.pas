unit GAFSequences;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_GAFSequences: Boolean = True;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallGAFSequences;
Procedure OnUninstallGAFSequences;

// -----------------------------------------------------------------------------

procedure LoadExtraGAFAnimationsHook;

implementation
uses
  SysUtils,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_FunctionsU;

var
  GAFSequencesPlugin: TPluginData;

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

    GAFSequencesPlugin.MakeRelativeJmp( State_GAFSequences,
                                        'Load gaf sequences from CUSTOMFX.GAF',
                                        @LoadExtraGAFAnimationsHook,
                                        $00429AC5, 1 );
    Result := GAFSequencesPlugin;
  end else
    Result := nil;
end;

procedure LoadExtraGAFAnimations; stdcall;
var
  i, j: integer;
  pSeq: PGAFSequence;
  CustomFXHandle: Pointer;
begin
  CustomFXHandle := nil;
  if HAPIFILE_GetFileLength(PAnsiChar('anims\customfx.gaf')) > 0 then
    CustomFXHandle := GAF_OpenAnimsFile(PAnsiChar('customfx'));
  if CustomFXHandle <> nil then
  begin
    j := 6;
    i := 0;
    while True do
    begin
      pSeq := GAF_Name2Sequence(CustomFXHandle, PAnsiChar('Explode' + IntToStr(j)));
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
      pSeq := GAF_Name2Sequence(CustomFXHandle, PAnsiChar('CustAnim' + IntToStr(i + 1)));
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
      pSeq := GAF_Name2Sequence(CustomFXHandle, PAnsiChar('flamestream' + IntToStr(i + 2)));
      if pSeq <> nil then
      begin
        SetLength(ExtraGAFAnimations.FlameStream, High(ExtraGAFAnimations.FlameStream) + 2);
        ExtraGAFAnimations.FlameStream[i] := pSeq;
        Inc(i);
      end else
        Break;
    end;
    ExtraGAFAnimations.GafSequence_ShieldIcon := GAF_Name2Sequence(CustomFXHandle, PAnsiChar('ShieldIcon'));
  end;
end;

procedure LoadExtraGAFAnimationsHook;
asm
//  mov     [eax+2], bl
  pushAD
  call    LoadExtraGAFAnimations
  popAD
  mov     ecx, [TADynmemStructPtr]
  push $00429ACB;
  call PatchNJump;
end;

end.
