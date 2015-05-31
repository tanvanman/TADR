unit MaxScriptSlots;

interface
uses
  PluginEngine, TA_MemoryLocations, TA_MemoryStructures, IniOptions;

// -----------------------------------------------------------------------------

const
  State_MaxScriptSlots: Boolean = True;

function GetPlugin: TPluginData;

// -----------------------------------------------------------------------------

const
  MAX_SCRIPT_SLOTS: Byte = 64;

implementation

Procedure OnInstallMaxScriptSlots;
begin
end;

Procedure OnUninstallMaxScriptSlots;
begin
end;

function GetPlugin : TPluginData;
var
  lReplacement: Cardinal;
begin
  if IsTAVersion31 and State_MaxScriptSlots then
  begin
    Result := TPluginData.Create( State_MaxScriptSlots,
                                  'Maximum unit script slots',
                                  State_MaxScriptSlots,
                                  @OnInstallMaxScriptSlots,
                                  @OnUninstallMaxScriptSlots );

    if IniSettings.ScriptSlotsLimit and
       (IniSettings.ModId > 1) then
    begin
      lReplacement := SizeOf(TNewScriptsData);
      Result.MakeReplacement( State_MaxScriptSlots,
                              'Init scripts data - Alloc memory increase',
                              $00485D74, lReplacement, SizeOf(lReplacement) );

      lReplacement := $0000291C;
      Result.MakeReplacement( State_MaxScriptSlots,
                              'Init scripts data - ScriptsData.lStartRunningNow',
                              $004B0638, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B0923, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B09CA, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B0A3F, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B0B88, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B0D22, lReplacement, SizeOf(lReplacement) );

      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B0D6A, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B19EF, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B19FB, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B1AA1, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B1AAD, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B1B68, lReplacement, SizeOf(lReplacement) );


      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B1B77, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B1F15, lReplacement, SizeOf(lReplacement) );
      result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004B211E, lReplacement, SizeOf(lReplacement) );

      lReplacement := $00002920;
      Result.MakeReplacement( State_MaxScriptSlots,
                              'Init scripts data - ScriptsData.pObject3d',
                              $00480D46, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480C56, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480C7F, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480C8C, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480C99, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480C36, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480CBA, lReplacement, SizeOf(lReplacement) );

      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480CF3, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480D0C, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480D1E, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480D2B, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480D5E, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480D8B, lReplacement, SizeOf(lReplacement) );

      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480D96, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480DB6, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480DDF, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480DF6, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480E20, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480E36, lReplacement, SizeOf(lReplacement) );

      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480E56, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480E76, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480EBD, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480EF4, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480F0C, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00480F5B, lReplacement, SizeOf(lReplacement) );

      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $0048115B, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $0048124C, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00481388, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00481397, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $004813EE, lReplacement, SizeOf(lReplacement) );

      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00481432, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              '',
                              $00481472, lReplacement, SizeOf(lReplacement) );

      Result.MakeReplacement( State_MaxScriptSlots,
                              'setters',
                              $00480B22, lReplacement, SizeOf(lReplacement) );
      Result.MakeReplacement( State_MaxScriptSlots,
                              'getters',
                              $00480772, lReplacement, SizeOf(lReplacement) );

      Result.MakeReplacement( State_MaxScriptSlots,
                              'start script',
                              $004B09E1, MAX_SCRIPT_SLOTS, 1 );
      Result.MakeReplacement( State_MaxScriptSlots,
                              'no xrefs',
                              $004B0A57, MAX_SCRIPT_SLOTS, 1 );
      Result.MakeReplacement( State_MaxScriptSlots,
                              'run script',
                              $004B0B9F, MAX_SCRIPT_SLOTS, 1 );
      Result.MakeReplacement( State_MaxScriptSlots,
                              'do scripts now, called from units loop',
                              $004B0D81, MAX_SCRIPT_SLOTS, 1 );
      Result.MakeReplacement( State_MaxScriptSlots,
                              'alloc mem and init script slots',
                              $004B0616, MAX_SCRIPT_SLOTS, 1 );

      Result.MakeReplacement( State_MaxScriptSlots,
                              'slots limiter',
                              $004B08E5, MAX_SCRIPT_SLOTS, 1 );

      Result.MakeReplacement( State_MaxScriptSlots,
                              'dispatcher 1',
                              $004B1AA7, MAX_SCRIPT_SLOTS, 1 );
      Result.MakeReplacement( State_MaxScriptSlots,
                              'dispatcher 2',
                              $004B1AEE, MAX_SCRIPT_SLOTS, 1 );
      Result.MakeReplacement( State_MaxScriptSlots,
                              'dispatcher 3',
                              $004B19F5, MAX_SCRIPT_SLOTS, 1 );
    end;
  end else
    Result := nil;
end;

end.
