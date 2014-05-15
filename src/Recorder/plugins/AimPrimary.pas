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

procedure AimPrimary_Turret_GetUnitPtr;
procedure AimPrimary_Turret_ExpandCall;
procedure AimPrimary_Ballistic_ExpandCall;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryLocations;

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
                          'AimPrimary Turret_GetUnitPtr',
                          @AimPrimary_Turret_GetUnitPtr,
                          $0049E2A5, 0);

    AimPrimaryPlugin.MakeRelativeJmp( State_AimPrimary,
                          'AimPrimary Turret_ExtendCall',
                          @AimPrimary_Turret_ExpandCall,
                          $0049E2F3, 0);

    AimPrimaryPlugin.MakeRelativeJmp( State_AimPrimary,
                          'AimPrimary Ballistic_ExtendCall',
                          @AimPrimary_Ballistic_ExpandCall,
                          $0049E35D, 0);  

    Result:= AimPrimaryPlugin;
  end else
    Result := nil;
end;

procedure AimPrimary_Turret_GetUnitPtr;
label
  ResetTargetUnit,
  ContinueToGame,
  loc_49E2CC;
asm
    push    ebx
    pushf
    mov     ebx, eax
    and     bx, $10
    jz      ResetTargetUnit
    push    eax
    mov     ebx, [esp-$56]            // Target unit pointer
    mov     eax, [ebx+$A8]            // PUnitStruct.lUnitInGameIndex
    mov     TargetUnit_Turret, eax
    pop     eax
    jmp ContinueToGame
ResetTargetUnit:
    mov     TargetUnit_Turret, 0
ContinueToGame:
    popf
    pop     ebx
    test    byte ptr [ebp+111h], 1
    jz      loc_49E2CC;
    push $0049E2AE;
    call PatchNJump;
loc_49E2CC:
    push $0049E2CC;
    call PatchNJump;
end;

procedure AimPrimary_Turret_ExpandCall;
asm
    push    TargetUnit_Turret       // target unit long ID, instead of 0
    and     eax, 0FFFFh
    push    ecx
    and     edx, 3
    push    eax
    mov     dword ptr [esi-13h], 0
    mov     eax, $509688[edx*4]     // AimPrimary / AimSecondary / AimTertiary
    push    3                       // par count
    push $0049E30F
    call PatchNJump
end;

procedure AimPrimary_Ballistic_ExpandCall;
label
   GroundTarget,
   ContinueToGame;
asm
    push    ebx
    pushf
    mov     ebx, eax
    and     bx, $10                   // is unit targeting at other unit or ground
    jz      GroundTarget
    mov     ebx, [esp-$56]            // Target unit pointer
    mov     edx, [ebx+$A8]            // PUnitStruct.lUnitInGameIndex
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

