unit AimPrimary;

// to extend aimprimary call by including target unit ID
// along with heading and pitch in parameters list

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_AimPrimary : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallAimPrimary;
Procedure OnUninstallAimPrimary;

// -----------------------------------------------------------------------------

procedure AimPrimary_Turret_ExpandCall;
procedure AimPrimary_Ballistic_ExpandCall;

implementation
uses
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_FunctionsU;

var
  AimPrimaryPlugin: TPluginData;
  TargetUnit_Turret: Cardinal;

Procedure OnInstallAimPrimary;
begin
end;

Procedure OnUninstallAimPrimary;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_AimPrimary then
  begin
    AimPrimaryPlugin := TPluginData.create( false,
                            'AimPrimary Plugin',
                            State_AimPrimary,
                            @OnInstallAimPrimary,
                            @OnUninstallAimPrimary );

    AimPrimaryPlugin.MakeRelativeJmp( State_AimPrimary,
                          'AimPrimary_Turret_ExpandCall',
                          @AimPrimary_Turret_ExpandCall,
                          $0049E2A5, 0);

    AimPrimaryPlugin.MakeRelativeJmp( State_AimPrimary,
                          'AimPrimary Ballistic_ExtendCall',
                          @AimPrimary_Ballistic_ExpandCall,
                          $0049E35D, 0);  

    Result:= AimPrimaryPlugin;
  end else
    Result := nil;
end;

procedure AimPrimary_Turret_ExpandCall;
label
  ShootingGround,
  loc_49E2AE,
  loc_49E2CC,
  ContinueToCall,
  loc_49E3AE;
asm
    push    ebx
    pushf
    mov     bx, word [edi+$4+2]
    test    bh, $80
    jz      ShootingGround
    xor     ebx, ebx
    mov     bx, word [edi+$4]
    mov     TargetUnit_Turret, ebx
    jmp loc_49E2AE
ShootingGround:
    mov     TargetUnit_Turret, 0
loc_49E2AE:
    popf
    pop     ebx
    test    byte ptr [ebp+111h], 1
    jz      loc_49E2CC
    shr     al, 2
    lea     ecx, [esp+40h-$18]
    and     al, 3
    push    ecx
    push    eax
    lea     edx, [esp+48h+4]
    lea     eax, [esp+48h-$2C]
    push    edx
    push    eax
    push    ebp
    push    edi
    call    sub_49D910
    jmp     ContinueToCall
loc_49E2CC:
    xor     eax, eax;
ContinueToCall:
    mov     ecx, [esp+40h+4]
    test    eax, eax
    jz      loc_49E3AE
    mov     eax, [esp+40h-$2C]
    mov     dl, [esi]
    mov     [esi-3], cx
    mov     [esi-5], ax
    push    0
    and     ecx, 0FFFFh
    shr     edx, 2
    push    TargetUnit_Turret       // target unit long ID, instead of 0
    and     eax, 0FFFFh
    push    ecx
    and     edx, 3
    push    eax
    mov     dword ptr [esi-13h], 0
    mov     eax, $509688[edx*4]    // AimPrimary / AimSecondary / AimTertiary
    push    3                      // par count
    push $0049E30F
    call PatchNJump
loc_49E3AE:
    push $0049E3AE;
    call PatchNJump;
end;

procedure AimPrimary_Ballistic_ExpandCall;
label
   GroundTarget,
   ContinueToGame;
asm
    push    ebx
    pushf
    mov     bx, word [edi+$4+2]
    test    bh, $80
    jz      GroundTarget
    xor     ebx, ebx
    mov     bx, word [edi+$4]
    mov     edx, ebx
    popf
    pop     ebx
    jmp ContinueToGame
GroundTarget:
    popf
    pop     ebx
    mov     edx, 0
ContinueToGame:
    push    edx                       // target unit long ID or 0
    shr     eax, 2
    push    0
    and     eax, 3
    push    0
    mov     dword ptr [esi-13h], 0
    mov     edx, $509688[eax*4]
    push    3
    push $0049E379;
    call PatchNJump;
end;


end.

