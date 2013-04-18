unit LOS_Sight;

interface
uses
  PluginEngine;

//see objects
Procedure GameUnit_LOS_Sight_Start;
Procedure GameUnit_LOS_Sight_LoopCondition;
Procedure GameUnit_LOS_Sight_LoopBodyProlog;

Procedure GetCodeInjections( PluginData : TPluginData );

implementation
uses
  LOS_extensions,
  TADemoConsts,
  TA_MemoryLocations;


Procedure GetCodeInjections( PluginData : TPluginData );
var
  i : integer;
  CodeInjection : TCodeInjection;
begin
CodeInjection := PluginData.NewCodeInjection();
CodeInjection.Enabled := State_LOS_Sight;
CodeInjection.Name := 'GameUnit_LOS_Sight_Start';
CodeInjection.JumpToAddress := @GameUnit_LOS_Sight_Start ;
CodeInjection.InjectionPoint := $48bae5;
CodeInjection.Writesize := 6;
CodeInjection := PluginData.NewCodeInjection();
CodeInjection.Enabled := State_LOS_Sight;
CodeInjection.Name:= 'GameUnit_LOS_Sight_LoopCondition';
CodeInjection.JumpToAddress := @GameUnit_LOS_Sight_LoopCondition ;
CodeInjection.InjectionPoint := $48bca3;
CodeInjection.Writesize := 5;
end; {GetPlugin}



// -----------------------------------------------------------------------------
//    GameUnit_LOS_Sight
// -----------------------------------------------------------------------------

Procedure GameUnit_LOS_Sight_Start;
asm
{
  // function prolog
  sub esp, $20
  push ebx
  push esi
}
  // Scratch registers: ecx, esi

  // preserve the initial player viewpoint
  // ViewPlayer = [TADynmemStrutPtr+TAdynmemStruct.LOS_Sight]
  mov esi, [$511de8] // mov esi, TADynmemStrutPtr
  xor ecx, ecx
  mov cl, [esi+$2a43] // mov cl, [esi+TAdynmemStruct.LOS_Sight]
  mov ViewPlayer, ecx;
  // reset the current player todo LOS on counter
  mov CurrentLOS, 0;
  // PlayerShareWith = ShareLosWith[ViewPlayer][CurrentLOS]
  mov esi, [Longword(ShareLosWith)+ecx*4]  // PlayerShareWith = @ShareLosWith[ViewPlayer].Data[0]
  mov PlayerShareWith, esi
  // NumHotUnits = 0
  xor eax, eax
  mov [esp+$0c], eax  // NumHotUnits = eax

  cmp esi, 0
  jnz GameUnit_LOS_Sight_LoopBodyProlog;
  jmp GameUnit_LOS_Sight_LoopCondition;
end; {GameUnit_LOS_Sight_Start}

Procedure GameUnit_LOS_Sight_LoopCondition;
label
  NextValue;
// Scratch registers: ebx, ecx, esi
asm // uses: ecx, esi

  // Determine if we have itterated over everyone yet
  mov esi, CurrentLOS
  inc esi
  cmp esi, 10
  jb NextValue
  // finished doing LOS_Sight stuff, cleanup

  // [TADynmemStrutPtr+TAdynmemStruct.LOS_Sight] = ViewPlayer
  mov ecx, ViewPlayer;
  mov esi, [$511de8] // mov esi, TADynmemStrutPtr
  mov [esi+$2a43], cl // mov cl, [esi+TAdynmemStruct.LOS_Sight]

  // return value
  mov eax, [esp+$0c]  // NumHotUnits = eax

  // function epilog
  pop esi
  pop ebx
  add esp, $20
  retn;

NextValue:
  // Move to the next Player todo LOS calcs
  mov CurrentLOS, esi
  // PlayerShareWith++
  mov esi, PlayerShareWith
  add esi, 4
  mov PlayerShareWith, esi

  jmp GameUnit_LOS_Sight_LoopBodyProlog;
end; {GameUnit_LOS_Sight_LoopCondition}

// requires: eax to be set to NumHotUnits ([esp+$0c])
Procedure GameUnit_LOS_Sight_LoopBodyProlog;
// uses: ecx, esi
asm
  // determine if we are sharing LOS with this player, if not continue looping
  mov esi, PlayerShareWith
  mov ecx, [esi]
  cmp ecx, longword(-1)
  jz GameUnit_LOS_Sight_LoopCondition // if PlayerShareWith^ = -1 then continue;

  // setup for calcing LOS with this player
  mov esi, [$511de8] // mov esi, TADynmemStrutPtr
  mov [esi+$2a43], cl // mov [esi+TAdynmemStruct.LOS_Sight], PlayerShareWith^

  // This makes sure all the units are added to the list to recieve LOS
  // otherwise the list is reset.

  mov [esp+$0c], eax   // [esp+28h+NumHotUnits] = eax
  lea ecx,[esi+$37e27] // ecx = [esi+TAdynmemStruct.field_37E27]
  mov [esp+$08],ecx    // field_37E27 = ecx
  mov ecx,eax
  mov eax,[esi+$1435f] // [esi+TAdynmemStruct.HotUnits]
  shl ecx,1;
  add eax,ecx


  // esi is require to be set to [$511de8] (TADynmemStrutPtr) before jumping to the start of the loop
  // eax needs to be NumHotUnits
    
  // jump to the start of the LOS Sight processing
  mov ecx, $48BB03
  jmp ecx;
end; {GameUnit_LOS_Sight_LoopBodyProlog}
  
end.
