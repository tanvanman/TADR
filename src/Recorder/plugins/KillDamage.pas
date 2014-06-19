unit KillDamage;

// increasing damage given to units to kill them will fix veteran clone bug
// for units with max hp ~30000

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_KillDamage : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallKillDamage;
Procedure OnUninstallKillDamage;

// -----------------------------------------------------------------------------

procedure KillDamage_FixVeteranCloneBug;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryLocations;

var
  KillDamagePlugin: TPluginData;

Procedure OnInstallKillDamage;
begin
end;

Procedure OnUninstallKillDamage;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_KillDamage then
  begin
    KillDamagePlugin := TPluginData.create( false,
                            'Increase damage to kill units',
                            State_KillDamage,
                            @OnInstallKillDamage,
                            @OnUnInstallKillDamage );
{
    //KillDamagePlugin.MakeReplacement( State_KillDamage, '', $00402141, MAXDAMAGE, SizeOf(Cardinal)); //order self destruct kill
    KillDamagePlugin.MakeReplacement( State_KillDamage, '', $004026FB, MAXDAMAGE, SizeOf(Cardinal)); //remove unit from lab queue
    KillDamagePlugin.MakeReplacement( State_KillDamage, '', $0041BC43, MAXDAMAGE, SizeOf(Cardinal)); //
    KillDamagePlugin.MakeReplacement( State_KillDamage, '', $004867FF, MAXDAMAGE, SizeOf(Cardinal)); //receive unit death
    KillDamagePlugin.MakeReplacement( State_KillDamage, '', $00486F8E, MAXDAMAGE, SizeOf(Cardinal));
    KillDamagePlugin.MakeReplacement( State_KillDamage, '', $0048869D, MAXDAMAGE, SizeOf(Cardinal)); // giveunit 1
    KillDamagePlugin.MakeReplacement( State_KillDamage, '', $004887C9, MAXDAMAGE, SizeOf(Cardinal)); // giveunit 2
    KillDamagePlugin.MakeReplacement( State_KillDamage, '', $00489BD3, MAXDAMAGE, SizeOf(Cardinal)); // unit make damage check
}
    KillDamagePlugin.MakeRelativeJmp( State_KillDamage,
                          'Modify build check',
                          @KillDamage_FixVeteranCloneBug,
                          $00489C2F, 0);
{
.text:00489C2F 018 C1 E8 1F                                                        shr     eax, 1Fh
.text:00489C32 018 03 D0                                                           add     edx, eax
.text:00489C34 018 EB 04                                                           jmp     short loc_489C3A
}
    Result:= KillDamagePlugin;
  end else
    Result := nil;
end;

procedure KillDamage_FixVeteranCloneBug;
label
  GiveUnitCall;
asm
   shr     eax, 1Fh
   add     edx, eax
   pushf
   cmp     ebx, 4                     // if DmgType = 4
   jz      GiveUnitCall
   popf
   push $00489C34;
   call PatchNJump;
GiveUnitCall :
   popf
   mov     eax, [esi+92h]             // UNITNIFO
   mov     edx, [eax+1FAh]            // lMaxHP
   push $00489C34;
   call PatchNJump;
end;

end.

