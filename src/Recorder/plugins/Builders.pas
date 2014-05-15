unit Builders;
{
  Plugins related to constructing units by plants and mobile builders.
  1) Allow plants to build nonmobile units (bmcode = 0)

  Based on http://www.tauniverse.com/forum/showpost.php?p=721369&postcount=523 by Admiral_94
  2) Allow registration of yardmap data for any unit, including mobile, into memory
  3) Have mobile units (those being built from mobile) use the typical nano colors while under construction
  4) Allow mobile units to build mobile units
}
interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_Builders : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallBuilders;
Procedure OnUninstallBuilders;

// -----------------------------------------------------------------------------

procedure Builders_MobileAddBuild;
procedure Builders_PlantBuildNonMobile;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryLocations;

var
  BuildersPlugin: TPluginData;

Procedure OnInstallBuilders;
begin
end;

Procedure OnUninstallBuilders;
begin
end;

function GetPlugin : TPluginData;
var
  Replacement: Word;
begin
  if IsTAVersion31 and State_Builders then
  begin
    BuildersPlugin := TPluginData.create( false,
                            'Mobile To Mobile',
                            State_Builders,
                            @OnInstallBuilders,
                            @OnUnInstallBuilders );

    Replacement:= 0;
    //Allow registration of yardmap data for any unit, including mobile, into memory
    BuildersPlugin.MakeReplacement( State_Builders, 'Yardmap for mobiles', $42CF3A, Replacement, 2); //35 01 -> 00 00

    //Have mobile units (those being built from mobile) use the typical nano colors while under construction
    BuildersPlugin.MakeReplacement( State_Builders, 'Nano colors for mobiles', $45961B, Replacement, 1); //13 -> 00

    // extraSize is how much data should be written out to ensure the patch site is nice & neat. Extra data is NOPs
    // jump is 5 bytes (instruction + 4 byte address)
    // Very handy for debugging, as well as being able to jump back to just after the injection site

    // the patch site before patching:
// .text:0041AB62 144 8B CB                              mov     ecx, ebx
// .text:0041AB64 144 81 E1 FF FF 00 00                  and     ecx, 0FFFFh                          
    // the patch site before patching:
// .text:0041AB62 144 8B XX XX XX XX                    jmp     xxxxxh
// .text:0041AB67 144 90                                NOP
// .text:0041AB68 144 90                                NOP
// .text:0041AB69 144 90                                NOP

    //Allow mobile units to build mobile units
    BuildersPlugin.MakeRelativeJmp( State_Builders,
                          'Modify build check',
                          @Builders_MobileAddBuild,
                          $41AB62, 3);

    BuildersPlugin.MakeRelativeJmp( State_Builders,
                          'Modify build check',
                          @Builders_PlantBuildNonMobile,
                          $0047DBD6, 0);

    Result:= BuildersPlugin;
  end else
    Result := nil;
end;

procedure Builders_MobileAddBuild;
label
  PreventBuilding;
asm
    mov     eax, dword ptr [TAdynmemStructPtr]
    test    bx, bx
    jz      PreventBuilding
    mov     eax, dword ptr [TAdynmemStructPtr]
    mov     ecx, [eax+37E9Eh]
    and     ecx, 0FFFFh
    mov     edx, ecx
    shl     edx, 6
    add     edx, ecx
    lea     ecx, [edx+edx*8]
    mov     edx, [eax+1439Bh]
    cmp     byte ptr [ecx+edx+22Fh], 0
    jz      PreventBuilding
    mov     byte ptr [eax+2CC3h], 0Eh   
    // jump to the "push 0" at the end of the original TA code when handling adding items to the build queue
    push $0041AB95;
    call PatchNJump; 
    // jump to the location for preventing adding items to the TA build queue
PreventBuilding:
    push $0041ABBB;
    call PatchNJump;
end;

{
.text:0047DBD6 024 80 BD 2F 02 00 00 00                                            cmp     [ebp+UNITINFO.bmcode], 0 ; MOBILE or BUILDING
.text:0047DBDD 024 75 15                                                           jnz     short loc_47DBF4
}

procedure Builders_PlantBuildNonMobile;
label
  BuildMobile;
asm
    cmp byte [ebp+$22F], 0;
    jnz BuildMobile;
    //bmcode=0, check for brakerate
    cmp dword [ebp+$19A], 0;
    jnz BuildMobile;
    push $0047DBDF;
    call PatchNJump;
BuildMobile:
    //bmcode=1 or bmcode=0 and brakerate<>0
    push $0047DBF4;
    call PatchNJump;
end;

end.

