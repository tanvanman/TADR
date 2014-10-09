unit LOS_extensions;
interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_LOS_UpdateTables = true;
  State_LOS_MiniMap = true;
  State_LOS_Radar = true;
  State_LOS_PlayerSeeUnit = true;
  State_LOS_GUIText = true;
  State_LOS_AllyPlayer = true;

function GetPlugin : TPluginData;
  


// -----------------------------------------------------------------------------
//  Asm data structures. Most of this is used to simplify the asm at the expense of memory
// -----------------------------------------------------------------------------

Procedure ResetShareLosState;


var
  // The player which has the initial view point
  ViewPlayer : longint;

implementation
uses
  sysutils,
  classes,
  TADemoConsts,
  TA_MemoryLocations,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_FunctionsU,
  InputHook,
  LOS_Radar,
  LOS_MiniMapUnits,
  LOS_PlayerSeeUnit,
  LOS_GuiText,
  LOS_UpdateTables,
  LOS_AllyPlayer;


{
// 0x2A42 SharedBits
// 0x2A43 LOS_Sight
}                      
{

sub_416AB0 - sets LOS_Sight
0x46451B   - sets LOS_Sight & sharedbits to zero. Game reset state?
sub_466050 - Final stats?
47A7D2     - sets LOS_Sight & sharedbits to something

 functions to look at to doing code injections on:
 0x47F7E0 sub_47F7E0 - Touches LOS_Sight
 0x48D630 sub_48D630 - Touches LOS_Sight
 0x40EB70 sub_40EB70 - Touches LOS_Radar
 0x464F80 - (UpdateDatePlayers)
          - 0x46506A LOS_Sight Calls (GameUnit_LOS_RadarTargetting) 
 0x466DC0 (GameUnit_LOS_Minimap_Projectiles) - 0x467300 - Touches LOS_Sight

 
 
 Trace down from 0x464F80 (UpdateDatePlayers). Looks like a lot of stuff touches LOS stuff from there
 Looked at:
 0x408C40 sub_408C40
 sub_401360 



 0x416B25 looks interesting


See projectiles on mini-map;
.text:00466E6F 02C mov     dl, [ebx+UnitStruct.OwnerIndex]
.text:00466E75 02C mov     al, [esi+TAdynmemStruct.LOS_Sight]
.text:00466E7B 02C cmp     dl, al
.text:00466E7D 02C jnz     loc_46718C
loc_466E83:

(inject code so if the unit is owned by any of the allied players, allow)

sub_4B7F30
- Compares the player index against something, trues a value if it is something 
}

Procedure ResetShareLosState;
begin
ViewPlayer := TAData.LocalPlayerID;
end; {ResetShareLosState}

Procedure OnInstallShareLOS;
begin
// make sure the player can see himself
ResetShareLosState;

//UpdateShareLosState(1,2,true);
end; {OnInstallShareLOS}

Procedure OnUninstallShareLOS;
begin
ResetShareLosState;
end;

Procedure TextCommand_View_Hook;
asm
  mov  ecx, [TADynmemStructPtr]
  mov  [ecx+TTADynMemStruct.cViewPlayerID], al // The player to use for LOS calcs
  
  // update the internal viewplayer, or things can get a little *funky*
  xor ecx, ecx
  mov cl, al
  mov  ViewPlayer, ecx
  // make sure we signal the allied state change to force a proper LOS reset
  mov AlliedStateChanged, 1

  // jump back to TA
  push $416BC7
  call PatchNJump;
end;

Procedure TextCommand_ShareAll_Hook;
asm

//  call SetLosEnabledState;

  // code we replace, ecx is free
  xor edx, edx
  mov dl, [eax+TTADynMemStruct.cControlPlayerID]
  // jump back to TA
  push $4190BD;
  call PatchNJump;
end;

function GetPlugin : TPluginData;
begin
if IsTAVersion31 then
  begin

  result := TPluginData.create( false,
                                'Sharelos',
                                State_LOS_Radar or State_LOS_PlayerSeeUnit or State_LOS_GUIText,
                                @OnInstallShareLOS, @OnUnInstallShareLOS );
  // hook LOS_Set_LOS_State so we can be notified when the LOS state changes
  result.MakeRelativeJmp( result.Enabled, '+view hook', @TextCommand_View_Hook, $416BBB, 1 );

  // hook TextCommand_ShareAll so we can detect when ShareAll status is switched
//  result.MakeRelativeJmp( result.Enabled, '+shareall hook', @TextCommand_ShareAll_Hook $4190B5, 3 );
 
  LOS_Radar.GetCodeInjections( result );
  LOS_MiniMapUnits.GetCodeInjections( result );
  LOS_PlayerSeeUnit.GetCodeInjections( result );
  LOS_GuiText.GetCodeInjections( result );
  LOS_UpdateTables.GetCodeInjections( result );
  LOS_AllyPlayer.GetCodeInjections( result );
  end
else
  result := nil;  
end;

end.

