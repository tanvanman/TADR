unit HighTrajectory;

// Admiral_94, Rime

interface
uses
  PluginEngine;

var
  MinWeap, MaxWeap: Cardinal;

// -----------------------------------------------------------------------------

const
  State_HighTrajectory : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallHighTrajectory;
Procedure OnUninstallHighTrajectory;

// -----------------------------------------------------------------------------

procedure HighTrajectory_ShellWeapon;

implementation
uses
  TA_MemoryConstants,
  TA_MemoryLocations;

var
  HighTrajectoryPlugin: TPluginData;

Procedure OnInstallHighTrajectory;
begin
end;

Procedure OnUninstallHighTrajectory;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_HighTrajectory then
  begin
    HighTrajectoryPlugin := TPluginData.create( false,
                            'HighTrajectory',
                            State_HighTrajectory,
                            @OnInstallHighTrajectory,
                            @OnUnInstallHighTrajectory );

    HighTrajectoryPlugin.MakeRelativeJmp( State_HighTrajectory,
                          'ShellWeapon Check',
                          @HighTrajectory_ShellWeapon,
                          $49AA50, 0);

    Result:= HighTrajectoryPlugin;
  end else
    Result := nil;
end;

procedure HighTrajectory_ShellWeapon;
label
  ShellWeapon1,
  ShellWeapon0,
  ContextTest_GreaterOrEqual,
  Context_Aiming,
  Context_Firing,
  DontAllowShoot;
asm
  pushf
  push    eax
  mov     eax, MinWeap
  cmp     ebx, eax
  jge     ContextTest_GreaterOrEqual
  jmp     Context_Firing
ContextTest_GreaterOrEqual:
  mov     eax, MaxWeap
  cmp     ebx, eax
  jle     Context_Aiming
Context_Firing:
  push    ebx
  mov     ebx, dword ptr[esp+$92]
  mov     eax, dword [ebx+$111]
  // shellweapon 00000100
  and     al, 4
  jnz     ShellWeapon1
  jmp     ShellWeapon0
Context_Aiming:
  push    ebx
  mov     ebx, dword ptr[esp-$36]
  mov     eax, dword [ebx+$111]
  // shellweapon 00000100
  and     al, 4
  jnz     ShellWeapon1
ShellWeapon0:
  pop     ebx
  pop     eax
  popf
  test    ah, 41h
  jz      DontAllowShoot
  // allow shooting
  push $0049AA55;
  call PatchNJump;
ShellWeapon1:
  pop     ebx
  pop     eax
  popf
  test    ah, 41h
  jnz     DontAllowShoot
  // allow shooting
  push $0049AA55;
  call PatchNJump;
DontAllowShoot:
  push $0049AA6C;
  call PatchNJump;
end;

end.

