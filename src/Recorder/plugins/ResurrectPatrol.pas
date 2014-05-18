unit ResurrectPatrol;

// to make resurrector units able to resurrect patrol
// todo :
// - fix resurrected unit height
// - reorder priority (resurrect should be before repair patrol and reclaim features) 405A6B

{
it seems there's a difference in a way RESURRECT and RECLAIM orders get called.
Resurrect order created with resurrect patrol hack (order type swap) fails in providing correct unit position
values from stack for unit creation function (here's where position values are being read: 0x004050D5 esi+22h).
And here some dumps for comparision:

cornecro position right after resurrect is finished (start to repair):
just to show where some of incorrect values come from
x_    x     h_    h     z_    z
7E 47 EF 01 00 00 80 00 55 03 C0 06

correct resurrected unit pos: (manual resurrect via mouse click)
x_    x     h_    h     z_    z
00 00 1C 02 00 00 78 00 00 00 A0 06

resurrected unit pos via resurrect patrol:
a bit of cornecro pos and garbage
x_    x     h_    h     z_    z
7E 47 1E 02 F8 09 65 08 55 03 8F 06
}

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

