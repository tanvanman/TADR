unit PauseLock;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_PauseLock : boolean = true;
  
function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstall;
Procedure OnUninstall;

// -----------------------------------------------------------------------------
//  Asm data structures. Most of this is used to simplify the asm at the expense of memory
// -----------------------------------------------------------------------------

var
  StopPauseStateChange : integer = 0;

Procedure SetPausedState( LockState : boolean; pause : boolean);
  
// -----------------------------------------------------------------------------

Procedure PauseHandling;

implementation
uses
  TADemoConsts,
  TA_MemoryLocations;

Procedure OnInstall;
begin
end;

Procedure OnUninstall;
begin
end;

function GetPlugin : TPluginData;
begin
if IsTAVersion31 and State_PauseLock then
  begin

  result := TPluginData.create( false,
                                'Pause lock',
                                State_PauseLock,
                                @OnInstall, @OnUnInstall );

  result.MakeRelativeJmp( false,//State_PauseLock,
                          'Lock pause state handler',
                          @PauseHandling,
                          $496099,
                          $1 );

// 495B7E - controls if the "PAUSED" image appears
                          
// 49524F
// 496918
  end
else
  result := nil;  
end;


Procedure SetPausedState( LockState : boolean; pause : boolean);
begin
StopPauseStateChange := BoolValues[LockState];
TAData.Paused := Pause;
end;


Procedure PauseHandling;
label
  PauseStateLocked;
asm
  mov eax, StopPauseStateChange
  cmp eax, 0
  jnz PauseStateLocked

  mov ecx, [TAdynmemStructPtr]

  // get TA todo it's stuff
  push $49609F;
  call PatchNJump;
  
PauseStateLocked:
  // nukes the pause state change attempt
  push $4965CE;
  call PatchNJump;
end;

end.
