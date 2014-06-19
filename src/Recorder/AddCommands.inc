{ Commands.AddCommand prototype
    procedure Commands.AddCommand( const aName : string;
                          const aAboutCommand : string;
                          const aSyntax : string;
                          aRequiredParams : Integer;
                          aValidCommandIn : TTAStatuses;
                          aIsServer : Boolean;
                          aIsSelfOnly : Boolean;
                          aIsDevModeOnly : Boolean;
                          aRequireCompatibleTA : Boolean;
                          aCommandHandler : TCommandHandlerEvent );
  Commands.AddCommandAlias prototype
    procedure Commands.AddCommandAlias( const aName : string;
                               const aNewAlias : string );
}

// commands which are not server or self specific
  Commands.AddCommand( 'players', 'Reports the list of players each player sees',
              '', 0,
              [InGame,InBattleRoom], False, False, False, False, OnPlayers );

  Commands.AddCommand( 'report', 'All recorders in the game report their presence, version and status of the toggles:'#10+
                        '(T--) shows the game is being recorded'#10+
                        '(-T-) shows .fixfacexps is enabled'#10+
                        '(--T) shows .protectdt is enabled'#10+
                        'Any combination of the above is possible. No answer from a player means they don''t have TA Demo installed.',
              '',0,
              [InGame,InBattleRoom], False, False, False, False, OnReport );
  Commands.AddCommand( 'reportmod', 'All players in the game report their game mod versions'#10+
                        'No answer from a player means they don''t have mod configured.',
              '',0,
              [InGame,InBattleRoom], False, False, False, False, OnReportMod );
  Commands.AddCommand( 'date', 'All recorders in the game report the date',
              '',0,
              [InGame,InBattleRoom], False, false, False, False, OnDate);
  Commands.AddCommand( 'sharelos', 'Makes allied units and any hostiles in their LOS visible, though covered by fog of war, and shares allied radar. .sharelos is mutual - a player will neither receive nor grant LOS and radar to/from allies as long as he/she does not issue this command',
              '',0,
              [InGame],False, False, True, False, OnShareLos );
  Commands.AddCommand( 'ehaoff', 'Disable the interface upgrade. This will disable all new functions like the Idle Const Finder, Queue 100 units etc.',
              '',0,
              [InBattleRoom], True, False, False, False, OnEhaoff );
  Commands.AddCommand( 'ehaon', 'Enables the interface upgrade.',
              '',0,
              [InBattleRoom], True, False, False, False, OnEhaOn );
  Commands.AddCommand( 'tahookoff', 'This disables just the autoclick feature in every recorder in the game. ',
              '',0,
              [InGame,InBattleRoom],False, False, False, False, OnTaHookOff );
  Commands.AddCommand( 'ehareport', 'All recorders in game report the status of their interface upgrade component. Mainly for debugging purposes.',
              '',0,
              [InGame,InBattleRoom], False, False, False, False, Onehareport );
  Commands.AddCommand( 'status', 'Same as .report, except only your own recorder replies and noone else sees it. Useful to silently check if your recorder is turned on.',
              '',0,
              [InGame,InBattleRoom], False, False, False, False, OnStatus );
  Commands.AddCommand( 'votego', 'Signifies you''re ready to go. If all players issue .votego, this will forcibly click in watchers. Useful if one of them left to take a shower or something without clicking in. ',
              '',0,
              [InBattleRoom], False, False, False, False, OnVoteGo );
  Commands.AddCommand( 'forcego', 'If you are hosting the game, this will forcibly click in watchers.',
              '',0,
              [InBattleRoom], True, False, False, False, OnForceGo );
  Commands.AddCommand( 'give', 'Grants the ability to a player todo a .take',
              ' [list of nicks]',1,
              [InGame,InBattleRoom], False, False, False, False, OnGive);
  Commands.AddCommand( 'stopgive', 'Blocks the ability to a player todo a .take',
              ' [list of nicks]',1,
              [InGame,InBattleRoom], False, False, False, False, OnStopGive);
  Commands.AddCommand( 'units', 'Reports unit count, only allowed while in replayer mode. Will now deliver divine punishment to versions which will report this during gameplay',
              '',0,
              [InGame], False, False, False, False, OnUnits);
  Commands.AddCommand( 'take', 'When a reject screen for a player appears, an ally can use .take to claim the dropping player''s units and structures. ',
              '',0,
              [InGame], False, False, False, False, OnTake);
  Commands.AddCommand( 'takecmd', 'Same as .take, but includes the dropping player''s Commander giving you two of them to worry about and utilize. With Commander Dies: Game Ends settings, loosing any Commander loses you the game. Ctrl-C will also select all Commanders. ',
              '',0,
              [InGame], False, False, False, False, OnTake);
  Commands.AddCommand( 'baseoff', 'Disables prebuilt bases. The game must be re-hosted to enable reactivation. ',
              '',0,
              [InGame,InBattleRoom], True, False, False, False, OnBaseOff );
  Commands.AddCommand( 'hookreport', 'Reports the presence and version of each TA Hook in the game. All TA Demo 0.98b recorders will always reply with a positive respond to this query. Under Note that TA Hook is now obsolete - the functionality is built right into TA Demo itself. ',
              '',0,
              [InGame,InBattleRoom], False, False, False, False, OnHookReport );

  Commands.AddCommand( 'voteready', 'Indicates that the player is ready'#10+
                           'Once all players are ready, the game will be unpaused',
              '', 0,
              [InGame], False, False, False, False, OnVoteReady );
  Commands.AddCommandAlias( 'voteready', 'ready' );

// commands which are server specific
  Commands.AddCommand( 'autopause', 'Toggles pausing the game once loading has finished'#10+
                           'At start, only the host can unpause or if the host drops or'#10+
                           'if all players do .voteready',
              '', 0,
              [InBattleRoom], True, False, False, False, OnAutoPause );

  Commands.AddCommand( 'fixoff', 'Disables all fixes for everyone.',
              '',0,
              [InGame,InBattleRoom],True, False, False, False, OnFixDisabled );
  Commands.AddCommand( 'fixon', 'Re-enables all fixes for everyone.',
              '',0,
              [InGame,InBattleRoom],True, False, False, False, OnFixEnabled );
  Commands.AddCommand( 'f1off', 'Turns the F1 key off for everyone',
              '',0,
              [InGame,InBattleRoom], True, False, False, False, Onf1off );
  Commands.AddCommand( 'cmdwarp', 'Enables a singleshot Commander warp. The game will start out paused. You can click on any spot on the map and your Commander will instantly be placed there. '+
              'Do this as often as you like. Once you have found a cozy spot for your Commander, click the Done button. Once everyone does so, the game will automatically unpause. '+
              'Alternatively, you can unpause the game manually - this will terminate the warp sequence for everyone in the game. Useful for fixing the start positions for 2v2 games on the MSN Gaming Zone for example. ',
              '',0,
              [InBattleRoom], True, False, False, False, OnCmdwarp );
  Commands.AddCommand( 'syncon', 'Gamespeed is locked to the range between the first (low limit) and second (high limit) number (e.g. .syncon -3 +2). Useful to prevent speedjacking.',
              'num num',2,
              [InGame,InBattleRoom], True, False, False, False, OnSyncon );
  Commands.AddCommand( 'syncoff', 'Gamespeed is unlocked',
              '',0,
              [InGame,InBattleRoom], True, False, False, False, OnSyncOff );
  Commands.AddCommand( 'base', 'Gives every player the possibility of placing a ready-built base around their first structure. The actual layout of the base is defined by the hosts base.txt file. If no filename is given the standard 0.93 base will be used.'+
                      ' Only the host needs TA Demo but there should be no 0.93 recorders in game or things might get messy. This command can only be issued by the host. See the accompanying base.txt for additional information. Note that base.txt '+
                      'uses net IDs to specify buildings, which change when adding units to the game, meaning that a base for OTA won''t work with UH or 3d party units in game. ',
              'filename',0,
              [InBattleRoom], True, False, False, False, OnBase );
  Commands.AddCommand( 'dobase', 'Places a predefined base around your first structure. This command has to be activated by the host by issuing .base in the Battle Room. Make sure there is flat ground to accomodate the base! ',
              '',0,
              [InGame], True, False, False, False, Ondobase );

// commands which are self specific

  Commands.AddCommand( 'help', 'Allows access various information about another command.'#10+
                      'info <command> - Returns the help information about a command'#10+
                      'syntax <command> - Returns info about the syntax of a command'#10+
                      'commands - lists the registered commands'#10,
              '[info/syntax <command>] / commands ',0,
              [InGame,InBattleRoom], False, True, False, False, OnHelp );
//'Ignore function, add player(s) to your ignore list. Also an .ignorewatchers, just to deal with those watchers who dont stay quiet.'

  Commands.AddCommand( 'yankspank', 'Nifty!',
              '',0,
              [InGame,InBattleRoom], False, True, False, False, Onyankspank);
  Commands.AddCommand( 'crash',
              'Mainly for debugging purposes: forces the recorder to actually crash if it is catching exceptions, so that you can get a full Errorlog.txt of the event. Usually you will want to use .panic. ',
              '',0,
              [InGame,InBattleRoom], False, True, False, False, OnCrash);
  Commands.AddCommand( 'panic',
              'If the recorder starts flooding you with Exception caught messages, use this command and pray that the recorder manages to process it. ',
              '',0,
              [InGame,InBattleRoom], False, True, False, False, OnPanic);
  Commands.AddCommand( 'about', 'Gets the about info for TA Demo Recorder',
              '',0,
              [InGame,InBattleRoom], False, True, False, False, OnAbout);
  Commands.AddCommand( 'fixfacexps', 'The recorder will protect new units for three seconds after they start building in a factory so that they are not killed by damage being dealt to an already dead unit, the cause for the annoying effect of "exploding build queues".'#10+
                            'NOTE: Because new units are invulnerable for 3 seconds, some aspects of the game change. This is most notable when a group of Skeeters attacks a shipyard producing units - the Skeeters deal most damage with their secondary weapon '+
                            'which you can''t force fire at the Shipyard, so units produced by the Shipyyard will in this case soak up a lot of the damage, making it harder to kick people out of the water using Skeeters. If this concerns you, see .fixoff. ',
              '',0,
              [InGame,InBattleRoom], False, True, False, False, OnFixFactoryExplosions);
  Commands.AddCommand( 'protectdt', 'Tries to make sure DTs aren''t lost. Without this fix, sometimes an opponent will see holes in an actually solid DT line you built. ',
              '',0,
              [InGame,InBattleRoom], False, True, False, False, OnProtectDT);
  Commands.AddCommand( 'fixall', 'Toggles both lag workarounds at once: .fixfacexps and .protectdt. ',
              '',0,
              [InGame,InBattleRoom], False, True, False, False, OnFixall);
  Commands.AddCommand( 'time', 'Displays the game time for all players according to their +clock. Useful for spotting a lagging player. ',
              '',0,
              [InGame], False, True, False, False, OnTime);
  Commands.AddCommand( 'sharemappos', 'Allows your allies to see where you are looking on the main game screen, indicated by a rectangle in the respective player''s color. ',
              '',0,
              [InGame], False, True, False, False, OnSharemappos);
  Commands.AddCommand( 'record', 'Records the game for later Perusing Pleasure™. By default the recorded game will end up in your TA directory, however this can be changed by '+
                        'the control panel''s Default TA Demo directory (3.) setting. If a recording with the same name already exists, it will be overwritten. ',
              'filename',0,
              [InBattleRoom,InGame], False, True, False, False, OnRecord);
  Commands.AddCommand( 'recordstatus', 'Allows the getting & setting of if a recording is going to be made',
              'true/false/1/0',0,
              [InBattleRoom,InGame], False, True, False, False, OnRecord);
  Commands.AddCommand( '3dta', 'Toggles the 3D version of the replayer, when a param is passed. Otherwise reports 3dta status.',
              'true/false/1/0',0,
              [InBattleRoom,InGame], False, True, False, False, On3dta);
  Commands.AddCommand( 'stoplog', 'Halts the recorder.',
              '',0,
              [InBattleRoom,InGame], False, True, False, False, OnStopLog);
  Commands.AddCommand( 'onlyunits', 'Experimental, do not use. '#10+
                           'Records only packets with class 2C. This shrinks demo files by about 50% but causes a lot of bad effects in replay, like invisible shots and misplaced units.',
              '',0,
              [InBattleRoom,InGame], False, True, False, True, OnOnlyUnits);
  Commands.AddCommand( 'unitsonly', 'Experimental, do not use. '#10+
                           'Records only packets with class 2C. This shrinks demo files by about 50% but causes a lot of bad effects in replay, like invisible shots and misplaced units.',
              '',0,
              [InBattleRoom,InGame], False, True, False, True, OnOnlyUnits);
  Commands.AddCommand( 'createtxt', 'Logs all game chat to a texfile with timestamped name in the Default TA Demo directory (3.) of the Control Center. ',
              '',0,
              [InBattleRoom,InGame], False, True, False, False, OnCreatetxt);
  Commands.AddCommand( 'lockon', 'If a player had issued .sharemappos during the original game, this command will lock your main display to follow his scrolling. Issue just .lockon without a nick to unlock your screen and scroll freely. ',
              '',0,
              [InBattleRoom,InGame], False, True, False, False, OnLockOn);
  Commands.AddCommand( 'randmap', 'Proposes a map randomly picked out of a list from a file that follows the format of the supplied maps.txt. The filename is optional and defaults to maps.txt if omitted. If you create your own favorite '+
                         'list of maps to play on you can use it by specifying the filename. ',
              'filename',0,
              [InBattleRoom], False, True, False, False, OnRandMap);
  Commands.AddCommand( 'randmapex', 'Proposes a map randomly picked out of a list from a maps that host has installed. ',
              '',0,
              [InBattleRoom], False, False, False, False, OnRandMapEx);
  Commands.AddCommandAlias( 'randmapex', 'rm' );
  Commands.AddCommand( 'fakewatch', 'Gives you the ability to chat while watching. Join the game as a regular player for it to work. You will appear as a watcher to everyone else and will be unable to affect anything, except that you '+
                           'will be able to chat. Note that fake watchers occupy the first slots in the game, causing some disruption for the rest of the players'' start positions. You may want to use .cmdwarp as a workaround. ',
              '',0,
              [InBattleRoom], False, True, False, False, OnFakeWatch);
  Commands.AddCommand( 'forcecd', 'Toggles an in-memory no-cd crack!',
              '',0,
              [InBattleRoom], False, True, False, False, OnForceCd);
  {$IFDEF WarZone}
  Commands.AddCommand( 'rank', 'todo: rank description',
              '',0,
              [InGame,InBattleRoom], False, True, False, False, OnRank);
  {$ENDIF}

  Commands.AddCommand( 'reportfilter', 'reports the filter list',
              '<netcode>',0,
              [InGame,InBattleRoom], False, True, False, True, OnReportFilter);
  Commands.AddCommand( 'disablefilter', 'disables packet filtering support',
              '<netcode>',0,
              [InGame,InBattleRoom], False, True, False, True, OnDisableFilter);
  Commands.AddCommand( 'removefilter', 'removes a packet(s) from the filter list',
              '<netcode>',0,
              [InGame,InBattleRoom], False, True, False, True, OnRemoveFilter);
  Commands.AddCommand( 'addfilter', 'adds a packet(s) from the filter list',
              '<netcode>',0,
              [InGame,InBattleRoom], False, True, False, True, OnAddFilter);

  Commands.AddCommand( 'polldp', 'lists the players DirectPlay sees',
              '<netcode>',0,
              [InGame,InBattleRoom], False, False, False, True, Onpolldp);

  Commands.AddCommand( 'crashme', 'Causes an exception to be raised',
              '',0,
              [InGame,InBattleRoom], False, True, False, True, OnCrashMe);
  Commands.AddCommand( 'loggingverbosity', 'Causes an exception to be raised',
              '<1-5>',0,
              [InGame,InBattleRoom], False, True, False, True, OnLoggingVerbosity);

  Commands.AddCommand( 'lookupaddr', 'Converts an address into a module,function & linenumber',
              'number',1,
              [InGame,InBattleRoom], False, True, False, True, OnLookUpAddr);
//  Commands.AddCommand( 'colors', 'lists players colors', '',0, [InGame,InBattleRoom], False, False, False, OnColors );
//  Commands.AddCommand( 'kill', 'emits the kill list', False, False, False, OnKill );
