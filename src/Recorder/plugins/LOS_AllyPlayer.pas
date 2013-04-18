unit LOS_AllyPlayer;

interface
uses
  PluginEngine;

{
Hooks when the allied state changes,
then signals another hook that code is required to run in the right thread.

}
Procedure Player_SetAlliedState_JumpHook;
Procedure Player_SetAlliedState_RetHook;
Procedure Signal_AlliedStateChange;

var
  AlliedStateChanged : integer;
  
Procedure GetCodeInjections( PluginData : TPluginData );

implementation
uses
  LOS_extensions,
  TADemoConsts,
  TA_MemoryLocations,
  TA_FunctionsU;


Procedure GetCodeInjections( PluginData : TPluginData );
begin
PluginData.MakeRelativeJmp( State_LOS_AllyPlayer,
                            'Player_SetAlliedState_Jump1Hook',
                            @Player_SetAlliedState_JumpHook,
                            $452B5E,
                            5);

PluginData.MakeRelativeJmp( State_LOS_AllyPlayer,
                            'Player_SetAlliedState_RetHook',
                            @Player_SetAlliedState_RetHook,
                            $452B54,
                            5);

PluginData.MakeRelativeJmp( State_LOS_AllyPlayer,
                            'Signal_AlliedStateChange',
                            @Signal_AlliedStateChange,
                            $46555F,
                            1);


end;                            
 
Procedure Signal_AlliedStateChange;
label
  SkipLOSReset,
  SkipRadarNMiniMapUpdate;
asm
  // check if the allied state has changed
  mov ecx, AlliedStateChanged
  cmp ecx, 0
  jz SkipLOSReset

  // reset the flags's state
  xor ecx,ecx
  mov AlliedStateChanged, ecx
  // make sure we keep all the registers
  PushAD
  PushFD
  // regenerate the LOS tables
  push 0
  mov ecx, Game_SetLOSState
  call ecx;
  // restore any potentially used registers
  popFD
  popAD  
SkipLOSReset:
  mov ecx, [TAdynmemStructPtr]
  cmp bl, [ecx+TAdynmemStruct_LOS_Sight]
  jnz SkipRadarNMiniMapUpdate
   
  push $46556D
  Call PatchNJump;
  
SkipRadarNMiniMapUpdate:
  push $4655A6
  Call PatchNJump;  
end;

Procedure Player_SetAlliedState_JumpHook;
asm
  mov AlliedStateChanged, 1

  xor     eax, eax

  pop edi
  pop esi
  pop ebp
  pop ebx
  pop ecx
  ret 10h
end;

Procedure Player_SetAlliedState_RetHook;
asm
  mov AlliedStateChanged, 1

  mov eax, ebx
  
  pop edi
  pop esi
  pop ebp
  pop ebx
  pop ecx
  ret 10h
end;

end.
