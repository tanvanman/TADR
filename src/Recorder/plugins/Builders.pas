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
procedure Builders_YardmapForMobile;
procedure Builders_PlantBuildNonMobile;

implementation
uses
  IniOptions,
  TA_FunctionsU,
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
  bReplacement : Byte;
begin
  if IsTAVersion31 and State_Builders then
  begin
    BuildersPlugin := TPluginData.create( false,
                            'Mobile To Mobile',
                            State_Builders,
                            @OnInstallBuilders,
                            @OnUnInstallBuilders );

    Replacement := 0;
    //Allow registration of yardmap data for any unit, including mobile, into memory
    BuildersPlugin.MakeRelativeJmp( State_Builders, '',
                                    @Builders_YardmapForMobile,
                                    $0042CF38, 1);

    //Have mobile units (those being built from mobile) use the typical nano colors while under construction
    BuildersPlugin.MakeReplacement( State_Builders, 'Nano colors for mobiles', $0045961B, Replacement, 1); //13 -> 00

    //Allow mobile units to build mobile units
    BuildersPlugin.MakeRelativeJmp( State_Builders,
                          'Modify build check',
                          @Builders_MobileAddBuild,
                          $0041AB62, 3);

    BuildersPlugin.MakeRelativeJmp( State_Builders,
                          'Modify build check',
                          @Builders_PlantBuildNonMobile,
                          $0047DBD6, 0);

    if IniSettings.Plugin_AiBuildList then
    begin
      bReplacement := 120;
      BuildersPlugin.MakeReplacement( State_Builders,
                                      'AI probability build list extend #1',
                                      $0042D972,
                                      bReplacement,
                                      1);

      BuildersPlugin.MakeReplacement( State_Builders,
                                      'AI probability build list extend #2',
                                      $0042DAC8,
                                      bReplacement,
                                      1);
    end;

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
  BuildMobile,
  ContinueBrakeRateCheck;
asm
    cmp     byte [ebp+$22F], 0;
    jnz     BuildMobile;
    pushf
    push    eax
    mov     eax, [esp+2Ah]
    cmp     eax, $0040289E
    jz      ContinueBrakeRateCheck
    pop     eax
    popf
    push $0047DBDF;
    call PatchNJump;
ContinueBrakeRateCheck :
    //bmcode=0, check for brakerate
    pop     eax
    popf
    cmp     dword [ebp+$19A], 0;
    jnz     BuildMobile;
    push $0047DBDF;
    call PatchNJump;
BuildMobile:
    //bmcode=1
    push $0047DBF4;
    call PatchNJump;
end;

var
  TempString : array[0..63] of AnsiChar;
procedure Builders_YardmapForMobile;
label
  ReadYardmap,
  NoYardmap;
asm
    pushAD
    mov     ecx, [esp+34h]
    push    $005119B8                // null str
    push    $40                      // buff len
    push    $00503988  // "YardMap"
    lea     ebx, TempString
    push    ebx
    call    TdfFile__GetStr
    test    eax, eax
    jz      NoYardmap
    popAD
ReadYardmap : //bmcode 0 or bmcode 1 with yardmap
    push $0042CF3E;
    call PatchNJump;
NoYardmap : //bmcode 1 without yardmap
    popAD
    push $0042D073;
    call PatchNJump;
end;

end.

