unit HighTrajectory_GetWeaps;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_HighTrajectory_GetWeaps : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallHighTrajectory_GetWeaps;
Procedure OnUninstallHighTrajectory_GetWeaps;

// -----------------------------------------------------------------------------

procedure HighTrajectory_GetMinMax;

implementation
uses
  TA_MemoryConstants,
  TA_MemoryLocations,
  HighTrajectory;

var
  HighTrajectoryGetWeapsPlugin: TPluginData;

Procedure OnInstallHighTrajectory_GetWeaps;
begin
end;

Procedure OnUninstallHighTrajectory_GetWeaps;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_HighTrajectory_GetWeaps then
  begin
    HighTrajectoryGetWeapsPlugin := TPluginData.create( false,
                            'HighTrajectory2',
                            State_HighTrajectory_GetWeaps,
                            @OnInstallHighTrajectory_GetWeaps,
                            @OnUninstallHighTrajectory_GetWeaps );

    HighTrajectoryGetWeapsPlugin.MakeRelativeJmp( State_HighTrajectory_GetWeaps,
                          'ShellWeapon Check',
                          @HighTrajectory_GetMinMax,
                          $483616, 0);

    Result:= HighTrajectoryGetWeapsPlugin;
  end else
    Result := nil;
end;

procedure HighTrajectory_GetMinMax;
asm
  mov eax, dword ptr[TADynmemStructPtr]
  pushf
  push ebx
  push edx
  mov ebx, eax
  add ebx, $2CF3
  mov MinWeap, ebx
  mov ebx, eax
  add ebx, $11500
  mov MaxWeap, ebx
  pop edx
  pop ebx
  popf
  mov eax, [TADynmemStructPtr]
  push $0048361B;
  call PatchNJump;
end;

end.

