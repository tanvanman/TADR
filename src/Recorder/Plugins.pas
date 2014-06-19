unit Plugins;
{$DEFINE KeyboardHookPlugin}
{$DEFINE ExtraPlugins}
interface

// some code injects must occur before the exe's main has run
procedure Do_LoadTime_CodeInjections( OnMainRun : boolean );

Procedure UninstallCodeInjections;

implementation
uses
  PluginEngine,
  ErrorLog_ExtraData,
  Thread_marshaller,
  InputHook,
  SpeedHack,
  PauseLock,
  UnitLimit,
  MultiAILimit,
  {$IFDEF KeyboardHookPlugin}KeyboardHook,{$ENDIF}
  IniOptions,
  Colors,
  RegPathFix,
  {$IFDEF ExtraPlugins}
  Builders,
  AimPrimary,
  //StartBuilding,
  ResurrectPatrol,
  ExplodeBitmaps,
  WeaponsExtend,
  WeaponAimNTrajectory,
  KillDamage,
  {$ENDIF}
  ClockPosition,
  ExtraUnitBars,
  COB_extensions,
  LOS_extensions;

procedure Do_LoadTime_CodeInjections( OnMainRun : boolean );
begin
// only register once
if OnMainRun then
  begin
  RegisterPlugin( ErrorLog_ExtraData.GetPlugin() );
  RegisterPlugin( Thread_marshaller.GetPlugin() );

  RegisterPlugin( IniOptions.GetPlugin() );
  RegisterPlugin( RegPathFix.GetPlugin() );
  RegisterPlugin( InputHook.GetPlugin() );
  RegisterPlugin( SpeedHack.GetPlugin() );
  RegisterPlugin( PauseLock.GetPlugin() );
  RegisterPlugin( UnitLimit.GetPlugin() );

  RegisterPlugin( LOS_extensions.GetPlugin() );
  RegisterPlugin( COB_extensions.GetPlugin() );
  RegisterPlugin( MultiAILimit.GetPlugin() );
  {$IFDEF KeyboardHookPlugin}RegisterPlugin( KeyboardHook.GetPlugin() );{$ENDIF}
  //RegisterPlugin( BattleRoomScroll.GetPlugin() );
  if IniSettings.Plugin_Colors then
    RegisterPlugin( Colors.GetPlugin() );
  RegisterPlugin( ClockPosition.GetPlugin() );
  RegisterPlugin( ExtraUnitBars.GetPlugin() );

  {$IFDEF ExtraPlugins}
  if IniSettings.Plugin_Builders then
    RegisterPlugin( Builders.GetPlugin() );
  if IniSettings.Plugin_AimScriptExt then
    RegisterPlugin( AimPrimary.GetPlugin() );
  //RegisterPlugin( StartBuilding.GetPlugin() );
  RegisterPlugin( ResurrectPatrol.GetPlugin() );
  if IniSettings.Plugin_Gaf then
    RegisterPlugin( ExplodeBitmaps.GetPlugin() );
  //RegisterPlugin( KillDamage.GetPlugin() );
  RegisterPlugin( WeaponsExtend.GetPlugin() );
  RegisterPlugin( WeaponAimNTrajectory.GetPlugin() );
  {$ENDIF}
  end;
// Run the code injection engine
InstallPlugins( OnMainRun );
end;

Procedure UninstallCodeInjections;
begin
UnInstallPlugins;
end;

end.
