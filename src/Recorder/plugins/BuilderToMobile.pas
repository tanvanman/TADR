unit BuilderToMobile;
{
  Based on http://www.tauniverse.com/forum/showpost.php?p=721369&postcount=523
  Thanks, Admiral_94
}
interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_BuilderToMobile : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallBuilderToMobile;
Procedure OnUninstallBuilderToMobile;

// -----------------------------------------------------------------------------

procedure BuilderToMobile_AddBuild;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryLocations;

var
  BuilderToMobilePlugin: TPluginData;

Procedure OnInstallBuilderToMobile;
var
  Replacement: Word;
begin
  if IniSettings.MobileToMobile then
  begin
    Replacement:= 0;
    //Allow registration of yardmap data for any unit, including mobile, into memory
    BuilderToMobilePlugin.MakeReplacement( State_BuilderToMobile, 'Yardmap for mobiles', $42CF3A, Replacement, 2); //35 01 -> 00 00

    //Allow mobile units to build mobile units
    {BuilderToMobilePlugin.MakeRelativeJmp( State_BuilderToMobile,
                          'Modify build check',
                          @BuilderToMobile_AddBuild,
                          $41AB62, ?
                          ? );   }
  end;
end;

Procedure OnUninstallBuilderToMobile;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_BuilderToMobile then
  begin
    BuilderToMobilePlugin := TPluginData.create( false,
                            'Mobile To Mobile',
                            State_BuilderToMobile,
                            @OnInstallBuilderToMobile,
                            @OnUnInstallBuilderToMobile );

    Result:= BuilderToMobilePlugin;
  end else
    Result := nil;
end;

{
TA:
.text:0041AB5D                                        loc_41AB5D:                             ; CODE XREF: RaceGEN_OnCommand+130j
.text:0041AB5D 144 66 85 DB                           test    bx, bx
.text:0041AB60 144 74 59                              jz      short loc_41ABBB
.text:0041AB62 144 8B CB                              mov     ecx, ebx
.text:0041AB64 144 81 E1 FF FF 00 00                  and     ecx, 0FFFFh
.text:0041AB6A 144 8B C1                              mov     eax, ecx
.text:0041AB6C 144 C1 E0 06                           shl     eax, 6
.text:0041AB6F 144 03 C1                              add     eax, ecx
.text:0041AB71 144 8D 0C C0                           lea     ecx, [eax+eax*8]
.text:0041AB74 144 A1 E8 1D 51 00                     mov     eax, TAMainStructPtr
.text:0041AB79 144 8B 90 9B 43 01 00                  mov     edx, [eax+1439Bh]
.text:0041AB7F 144 80 BC 11 2F 02 00 00 00            cmp     byte ptr [ecx+edx+22Fh], 0
.text:0041AB87 144 75 32                              jnz     short loc_41ABBB
.text:0041AB89 144 C6 80 C3 2C 00 00 0E               mov     byte ptr [eax+2CC3h], 0Eh
.text:0041AB90 144 A1 E8 1D 51 00                     mov     eax, TAMainStructPtr
.text:0041AB95 144 6A 00                              push    0               ; int
...
}

{
Admiral_94:
.text:0041AB5D                                        loc_41AB5D:                             ; CODE XREF: RaceGEN_OnCommand+130j
.text:0041AB5D 144 66 85 DB                           test    bx, bx
.text:0041AB60 144 74 59                              jz      short loc_41ABBB
.text:0041AB62 144 A1 E8 1D 51 00                     mov     eax, TAMainStructPtr
.text:0041AB67 144 8B 88 9E 7E 03 00                  mov     ecx, [eax+37E9Eh]
.text:0041AB6D 144 81 E1 FF FF 00 00                  and     ecx, 0FFFFh
.text:0041AB73 144 8B D1                              mov     edx, ecx
.text:0041AB75 144 C1 E2 06                           shl     edx, 6
.text:0041AB78 144 03 D1                              add     edx, ecx
.text:0041AB7A 144 8D 0C D2                           lea     ecx, [edx+edx*8]
.text:0041AB7D 144 8B 90 9B 43 01 00                  mov     edx, [eax+1439Bh]
.text:0041AB83 144 80 BC 11 2F 02 00 00 00            cmp     byte ptr [ecx+edx+22Fh], 0
.text:0041AB8B 144 74 2E                              jz      short loc_41ABBB
.text:0041AB8D 144 C6 80 C3 2C 00 00 0E               mov     byte ptr [eax+2CC3h], 0Eh
.text:0041AB94 144 90                                 nop
.text:0041AB95 144 6A 00                              push    0               ; int
...
}

procedure BuilderToMobile_AddBuild;
asm
  mov     eax, dword ptr [TAdynmemStructPtr]
...
  push ?;
  call PatchNJump;
end;

end.
