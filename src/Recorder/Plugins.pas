unit Plugins;

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
  KeyboardHook,
  WeaponsExpand,
  UnitInfoExpand,
  Builders,
  ScriptCallsExtend,
  UnitActions,
  Transporters,
  OrdersOverride,
  WeaponAimNTrajectory,
  GAFSequences,
  MapExtensions,
  SideDataExpand,
  Colors,
  ClockPosition,
  GUIEnhancements,
  NanoFrameUnits,
  BroadcastNanolathe,
  BattleRoomEnhancements,
  SkirmishEnhancements,
  ExtensionsMem,
  COB_extensions,
  MaxScriptSlots,
  //KillDamage,
  //MinimapExpand,
  //PlayersSlotsExpand,
  SaveGame,
  {$IFDEF Debug}Developers,{$ENDIF}
  StatsLogging;

procedure Do_LoadTime_CodeInjections( OnMainRun : boolean );
begin
  // only register once
  if OnMainRun then
  begin
    RegisterPlugin( ErrorLog_ExtraData.GetPlugin() );
    RegisterPlugin( Thread_marshaller.GetPlugin() );
    RegisterPlugin( IniOptions.GetPlugin() );
    RegisterPlugin( InputHook.GetPlugin() );
    RegisterPlugin( SpeedHack.GetPlugin() );
    RegisterPlugin( PauseLock.GetPlugin() );
    RegisterPlugin( UnitLimit.GetPlugin() );

    RegisterPlugin( LOS_extensions.GetPlugin() );
    RegisterPlugin( MultiAILimit.GetPlugin() );
    RegisterPlugin( KeyboardHook.GetPlugin() );
    RegisterPlugin( WeaponsExpand.GetPlugin() );
    RegisterPlugin( UnitInfoExpand.GetPlugin() );
    RegisterPlugin( SideDataExpand.GetPlugin() );
    RegisterPlugin( RegPathFix.GetPlugin() );
    if IniSettings.ModId > 1 then
    begin
      //RegisterPlugin( PlayersSlotsExpand.GetPlugin() );
      RegisterPlugin( Builders.GetPlugin() );
      RegisterPlugin( ScriptCallsExtend.GetPlugin() );
      RegisterPlugin( OrdersOverride.GetPlugin() );
      RegisterPlugin( UnitActions.GetPlugin() );
      RegisterPlugin( Transporters.GetPlugin() );
      RegisterPlugin( WeaponAimNTrajectory.GetPlugin() );
      //RegisterPlugin( KillDamage.GetPlugin() );
      RegisterPlugin( GAFSequences.GetPlugin() );
      RegisterPlugin( MapExtensions.GetPlugin() );
      //if IniSettings.ExpandMinimap then
      //  RegisterPlugin( MinimapExpand.GetPlugin() );
    end;
    if IniSettings.Colors then
      RegisterPlugin( Colors.GetPlugin() );
    RegisterPlugin( ClockPosition.GetPlugin() );
    RegisterPlugin( GUIEnhancements.GetPlugin() );
    RegisterPlugin( NanoFrameUnits.GetPlugin() );
    if IniSettings.BroadcastNanolathe then
      RegisterPlugin( BroadcastNanolathe.GetPlugin() );
    if IniSettings.BattleRoomEnh then
      RegisterPlugin( BattleRoomEnhancements.GetPlugin() );
    RegisterPlugin( SkirmishEnhancements.GetPlugin() );
    RegisterPlugin( ExtensionsMem.GetPlugin() );
    RegisterPlugin( COB_extensions.GetPlugin() );
    RegisterPlugin( MaxScriptSlots.GetPlugin() );
    if IniSettings.CreateStatsFile then
      RegisterPlugin( StatsLogging.GetPlugin() );
    RegisterPlugin( SaveGame.GetPlugin() );
    {$IFDEF Debug}RegisterPlugin( Developers.GetPlugin() );{$ENDIF}
  end;
  // Run the code injection engine
  InstallPlugins( OnMainRun );
end;

Procedure UninstallCodeInjections;
begin
  UnInstallPlugins;
end;

end.