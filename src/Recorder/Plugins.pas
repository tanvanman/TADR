unit Plugins;
{$DEFINE KeyboardHookPlugin}
interface

// some code injects must occur before the exe's main has run
procedure Do_LoadTime_CodeInjections( OnMainRun : boolean );

Procedure UninstallCodeInjections;

implementation
uses
  PluginEngine,
  ErrorLog_ExtraData,
  Thread_marshaller,
  IniOptions,
  RegPathFix,
  InputHook,
  SpeedHack,
  PauseLock,
  UnitLimit,
  LOS_extensions,
  MultiAILimit,
  {$IFDEF KeyboardHookPlugin}KeyboardHook,{$ENDIF}
  WeaponsExtend,
  UnitInfoExtend,
  Builders,
  ScriptCallsExtend,
  ResurrectPatrol,
  UnitActions,
  WeaponAimNTrajectory,
  //KillDamage,
  ExplodeBitmaps,
  TAExceptionsLog,
  CampaignExtensions,
  //MinimapExpand,
  Colors,
  ClockPosition,
  GUIEnhancements,
  BattlerRoomMemFix,
  COB_extensions,
  StatsLogging;

procedure Do_LoadTime_CodeInjections( OnMainRun : boolean );
begin
// only register once
if OnMainRun then
  begin
  RegisterPlugin( ErrorLog_ExtraData.GetPlugin() );
  RegisterPlugin( Thread_marshaller.GetPlugin() );
  RegisterPlugin( InputHook.GetPlugin() );
  RegisterPlugin( IniOptions.GetPlugin() );
  RegisterPlugin( SpeedHack.GetPlugin() );
  RegisterPlugin( PauseLock.GetPlugin() );
  RegisterPlugin( UnitLimit.GetPlugin() );

  RegisterPlugin( LOS_extensions.GetPlugin() );
  RegisterPlugin( MultiAILimit.GetPlugin() );
  {$IFDEF KeyboardHookPlugin}RegisterPlugin( KeyboardHook.GetPlugin() );{$ENDIF}
  //RegisterPlugin( BattleRoomScroll.GetPlugin() );

  RegisterPlugin( WeaponsExtend.GetPlugin() );
  RegisterPlugin( UnitInfoExtend.GetPlugin() );
  RegisterPlugin( RegPathFix.GetPlugin() );
  if IniSettings.ModId > 1 then
  begin
    RegisterPlugin( Builders.GetPlugin() );
    RegisterPlugin( ScriptCallsExtend.GetPlugin() );
    RegisterPlugin( ResurrectPatrol.GetPlugin() );
    RegisterPlugin( UnitActions.GetPlugin() );
    RegisterPlugin( WeaponAimNTrajectory.GetPlugin() );
    //RegisterPlugin( KillDamage.GetPlugin() );
    if IniSettings.Plugin_Gaf then
      RegisterPlugin( ExplodeBitmaps.GetPlugin() );
    RegisterPlugin( TAExceptionsLog.GetPlugin() );
    RegisterPlugin( CampaignExtensions.GetPlugin() );
//    if IniSettings.ExpandMinimap then
//      RegisterPlugin( MinimapExpand.GetPlugin() );
  end;
  if IniSettings.Plugin_Colors then
    RegisterPlugin( Colors.GetPlugin() );
  RegisterPlugin( ClockPosition.GetPlugin() );
  RegisterPlugin( GUIEnhancements.GetPlugin() );
  RegisterPlugin( BattlerRoomMemFix.GetPlugin() );
  RegisterPlugin( COB_extensions.GetPlugin() );
  if IniSettings.CreateStatsFile then
    RegisterPlugin( StatsLogging.GetPlugin() );
  end;
// Run the code injection engine
InstallPlugins( OnMainRun );
end;

Procedure UninstallCodeInjections;
begin
UnInstallPlugins;
end;

end.
