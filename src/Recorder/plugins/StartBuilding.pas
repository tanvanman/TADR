unit StartBuilding;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_StartBuilding : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallStartBuilding;
Procedure OnUninstallStartBuilding;

// -----------------------------------------------------------------------------

procedure StartBuilding_ExpandCall;

implementation
uses
  TA_MemoryConstants,
  TA_MemoryLocations;

var
  StartBuildingPlugin: TPluginData;

Procedure OnInstallStartBuilding;
begin
end;

Procedure OnUninstallStartBuilding;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_StartBuilding then
  begin
    StartBuildingPlugin := TPluginData.create( false,
                            'StartBuilding Plugin',
                            State_StartBuilding,
                            @OnInstallStartBuilding,
                            @OnUninstallStartBuilding );

    StartBuildingPlugin.MakeRelativeJmp( State_StartBuilding,
                          'StartBuilding_ExpandCall',
                          @StartBuilding_ExpandCall,
                          $004385A7, 0);

    Result:= StartBuildingPlugin;
  end else
    Result := nil;
end;
{
.text:004385A7 00C 8B 8F 9A 00 00 00                                               mov     ecx, [edi+9Ah]
.text:004385AD 00C 6A 00                                                           push    0               ; par4
.text:004385AF 010 8B 74 24 1C                                                     mov     esi, [esp+10h+arg_8]
.text:004385B3 010 6A 00                                                           push    0               ; par3
.text:004385B5 014 81 E6 FF FF 00 00                                               and     esi, 0FFFFh
.text:004385BB 014 6A 00                                                           push    0               ; par2
.text:004385BD 018 56                                                              push    esi
.text:004385BE 01C 6A 01                                                           push    1               ; par count
.text:004385C0 020 8B D8                                                           mov     ebx, eax
.text:004385C2 020 6A 00                                                           push    0
.text:004385C4 024 6A 00                                                           push    0
.text:004385C6 028 53                                                              push    ebx
}

procedure StartBuilding_ExpandCall;
asm
    push    0                 // par4
    mov     esi, [esp+10h+$8] // unit order
    lea     ecx, [esi+$22]    // order position struct
    mov     ebp, ecx
    mov     bp, word [ecx+$A]
    push    ebp               // par3
    lea     ebp, [edi+$6A]
    mov     ecx, [edi+9Ah]
    mov     esi, [esp+10h+$10]
    and     esi, 0FFFFh
    push    0                 // par2
    push    esi               // par1 - heading
    push    2                 // par count
    mov     ebx, eax
    push    0
    push    0
    push    ebx
    push $004385C7;
    call PatchNJump;
end;

end.

