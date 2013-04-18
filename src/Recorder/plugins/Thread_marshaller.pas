unit Thread_marshaller;

interface
uses
  PluginEngine;
  
{
This allows the network thread to marshall code into the GUI/input thread
}

{
// reset LOS before this:
0x46556D

}

function GetPlugin : TPluginData;

type
  TEventMethod = procedure () of object; register;

Procedure QueueMethod( EventMethod : TEventMethod );

implementation
uses
{$IFDEF ThreadLogging}threadlogging, {$ENDIF}
  TADemoConsts,
  TA_MemoryLocations,
  TA_FunctionsU;

Procedure OnInstall;
begin
end;

Procedure OnUnInstall;
begin
end;

{$IFDEF ThreadLogging}
Procedure ThreadLoggingThunk2;
begin
ThreadLogger.LogThreadID;
end;

Procedure ThreadLoggingThunk1;
label
  loc_4816FD;
asm
  push    ebx
  push    ebp
  push    esi
  push    edi

  // todo : check exactly which threads are touching  LOS_Set_LOS_State
  PushAD
  PushFD
  call ThreadLoggingThunk2
  popFD
  popAD
  
  test    eax, eax  
  jz      loc_4816FD

  push $4816AF
  call PatchNJump;

loc_4816FD:
  push $4816FD
  call PatchNJump;

end;
{$ENDIF}
function GetPlugin : TPluginData;
begin
if IsTAVersion31 then
  begin

  result := TPluginData.create( false,
                                'Thread marshaller',
                                true,
                                @OnInstall, @OnUnInstall );
{$IFDEF ThreadLogging}
  result.MakeRelativeJmp( true,
                 'thread logging on point SetLOS',
                 @ThreadLoggingThunk1,
                 $4816A7,
                 1);
{$ENDIF}
  end
else
  result := nil;  
end;  

Procedure QueueMethod( EventMethod : TEventMethod );
begin
// todo : complete me
end;

end.
