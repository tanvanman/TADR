// commands which are not server or self specific
function OnPlayers( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnShareLos( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnFixDisabled( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnFixEnabled( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnEhaoff( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnEhaOn( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnTaHookOff( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnColors( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function Onehareport( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnReport( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnReportMod( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnStatus( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnDate( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnVoteGo( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnGive( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnStopGive( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnUnits( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnTake( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnKill( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnHookReport( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnVoteReady( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
// commands which are server specific
function OnAutoPause( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function Onf1off( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnCmdwarp( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnSyncon( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnSyncOff( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnBase( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnBaseOff( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnDoBase( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
// commands which are self specific
function OnHelp( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnYankspank( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnCrash( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnPanic( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnAbout( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnFixFactoryExplosions( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnProtectDT( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnFixall( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnTime( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnSharemapPos( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnRecord( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnRecordStatus( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function On3dta( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnStopLog( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnOnlyUnits( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnCreatetxt( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnLockOn( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnRandMap( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnFakeWatch( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnForceCd( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
// Debug functions!
function OnReportFilter( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnDisableFilter( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnAddFilter( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnRemoveFilter( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnLookUpAddr( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;

function Onpolldp( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnCrashMe( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;
function OnLoggingVerbosity( const Command : string; Sender : TPlayerData; params : TStringList ) : Boolean;



