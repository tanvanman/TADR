unit ResurrectPatrol;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_ResurrectPatrol : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallResurrectPatrol;
Procedure OnUninstallResurrectPatrol;

// -----------------------------------------------------------------------------

procedure ResurrectPatrol_ReclaimToResurrect;

implementation
uses
  TA_MemoryConstants,
  TA_MemoryLocations;

var
  ResurrectPatrolPlugin: TPluginData;

Procedure OnInstallResurrectPatrol;
begin
end;

Procedure OnUninstallResurrectPatrol;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_ResurrectPatrol then
  begin
    ResurrectPatrolPlugin := TPluginData.create( false,
                            'ResurrectPatrol',
                            State_ResurrectPatrol,
                            @OnInstallResurrectPatrol,
                            @OnUnInstallResurrectPatrol );

    ResurrectPatrolPlugin.MakeRelativeJmp( State_ResurrectPatrol,
                          'Swap reclaim order with resurrect',
                          @ResurrectPatrol_ReclaimToResurrect,
                          $00405BEF, 0);

    Result:= ResurrectPatrolPlugin;
  end else
    Result := nil;
end;

procedure ResurrectPatrol_ReclaimToResurrect;
label
  Reclaim,
  Resurrect,
  CreateOrder;
asm
  pushf
  push    eax
  push    ebx
  mov     eax, [esp+$46]  // unit
  mov     ebx, [eax+$92] // unit info struct
  mov     eax, [ebx+$245] // unit type mask #2
  and     ax, 2048       // canresurrect 0 / 1
  jz      Reclaim
Resurrect:
  pop     ebx
  pop     eax
  popf
  push    $005052F0   // PAnsiChar RESURRECT
  jmp     CreateOrder
Reclaim:
  pop     ebx
  pop     eax
  popf
  push    $005016C4  // PAnsiChar RECLAIM
CreateOrder:
  push $00405BF4;
  call PatchNJump;
end;

end.

