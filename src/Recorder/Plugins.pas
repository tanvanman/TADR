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
  
  InputHook,
  SpeedHack,
  PauseLock,
  UnitLimit,
  MultiAILimit,
  //nocd,
  COB_extensions,
  LOS_extensions;

procedure Do_LoadTime_CodeInjections( OnMainRun : boolean );
begin
// only register once
if OnMainRun then
  begin
  RegisterPlugin( ErrorLog_ExtraData.GetPlugin() );
  RegisterPlugin( Thread_marshaller.GetPlugin() );


  RegisterPlugin( InputHook.GetPlugin() );
  RegisterPlugin( SpeedHack.GetPlugin() );
  RegisterPlugin( PauseLock.GetPlugin() );
  RegisterPlugin( UnitLimit.GetPlugin() );
  RegisterPlugin( LOS_extensions.GetPlugin() );
  RegisterPlugin( COB_extensions.GetPlugin() );
  RegisterPlugin( MultiAILimit.GetPlugin() );

  end;
// Run the code injection engine
InstallPlugins( OnMainRun );
end;

Procedure UninstallCodeInjections;
begin
UnInstallPlugins;
end;

end.
