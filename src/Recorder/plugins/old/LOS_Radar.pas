unit LOS_Radar;

interface
uses
  PluginEngine;
// note: not quite working properly. (player 1 cant see player 2/3?)

//radar
Procedure GameUnit_LOS_Radar_Start;
Procedure GameUnit_LOS_Radar_LoopCondition;
Procedure GameUnit_LOS_Radar_LoopBodyProlog;

Procedure GameUnit_LOS_Radar_Middle1;
Procedure GameUnit_LOS_Radar_Middle2;

Procedure GetCodeInjections( var CodeInjections : TCodeInjections );

implementation
uses
  LOS_extensions,
  TADemoConsts,
  TA_MemoryLocations;

Procedure GetCodeInjections( var CodeInjections : TCodeInjections );
var
  i : integer;
begin
i := length(CodeInjections);
setlength(CodeInjections, length(CodeInjections)+4);

CodeInjections[i].Enabled := State_LOS_Radar;
CodeInjections[i].Name:= 'GameUnit_LOS_Radar_Start';
CodeInjections[i].JumpToAddress := @GameUnit_LOS_Radar_Start ;
CodeInjections[i].InjectionPoint := $467446;
CodeInjections[i].Writesize := 6;
inc(i);
CodeInjections[i].Enabled := State_LOS_Radar;
CodeInjections[i].Name:= 'GameUnit_LOS_Radar_LoopCondition';
CodeInjections[i].JumpToAddress := @GameUnit_LOS_Radar_LoopCondition;
CodeInjections[i].InjectionPoint := $46782F;
CodeInjections[i].Writesize := 6;
inc(i);
CodeInjections[i].Enabled := State_LOS_Radar;
CodeInjections[i].Name:= 'GameUnit_LOS_Radar_Middle1';
CodeInjections[i].JumpToAddress := @GameUnit_LOS_Radar_Middle1;
CodeInjections[i].InjectionPoint := $4674a1;
CodeInjections[i].Writesize := 8;
inc(i);
CodeInjections[i].Enabled := State_LOS_Radar;
CodeInjections[i].Name:= 'GameUnit_LOS_Radar_Middle2';
CodeInjections[i].JumpToAddress := @GameUnit_LOS_Radar_Middle2;
CodeInjections[i].InjectionPoint := $4674f9;
CodeInjections[i].Writesize := 6;
end;
// -----------------------------------------------------------------------------
//    GameUnit_LOS_Radar
// -----------------------------------------------------------------------------

Procedure GameUnit_LOS_Radar_Start;
asm
{
  // function prolog
  sub esp, $20
  push ebx
  push esi
}
  push edi
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

  cmp esi, 0
  jnz GameUnit_LOS_Radar_LoopBodyProlog;
  jmp GameUnit_LOS_Radar_LoopCondition;
end; {GameUnit_LOS_Radar_Start}


Procedure GameUnit_LOS_Radar_LoopCondition;
label
  NextValue;
// Scratch registers: ecx, esi
asm // uses: ecx, esi

  // Determine if we have itterated over everyone yet
  mov esi, CurrentLOS
  inc esi
  cmp esi, 10
  jb NextValue
  // finished doing LOS_Radar stuff, cleanup

  // [TADynmemStrutPtr+TAdynmemStruct.LOS_Sight] = ViewPlayer
  mov ecx, ViewPlayer;
  mov esi, [$511de8] // mov esi, TADynmemStrutPtr
  mov [esi+$2a43], cl // mov cl, [esi+TAdynmemStruct.LOS_Sight]

  // return value

  // function epilog
  pop edi
  pop esi
  pop ebp
  pop ebx
  add esp, $28
  retn

NextValue:
  // Move to the next Player todo LOS calcs
  mov CurrentLOS, esi
  // PlayerShareWith++
  mov esi, PlayerShareWith
  add esi, 4
  mov PlayerShareWith, esi

  jmp GameUnit_LOS_Radar_LoopBodyProlog;
end; {GameUnit_LOS_Radar_LoopCondition}


Procedure GameUnit_LOS_Radar_LoopBodyProlog;
// uses: ecx, esi
asm
  // determine if we are sharing LOS with this player, if not continue looping
  mov esi, PlayerShareWith
  mov ecx, [esi]
  cmp ecx, longword(-1)
  jz GameUnit_LOS_Radar_LoopCondition // if PlayerShareWith^ = -1 then continue;

  // setup for calcing LOS with this player
  mov esi, [$511de8] // mov esi, TADynmemStrutPtr
  mov [esi+$2a43], cl // mov [esi+TAdynmemStruct.LOS_Sight], PlayerShareWith^

  // esi is require to be set to [$511de8] (TADynmemStrutPtr) before jumping to the start of the loop

  // jump to the start of the LOS Radar processing
  mov ecx, $46744D
  jmp ecx;
end; {GameUnit_LOS_Radar_LoopBodyProlog}


Procedure GameUnit_LOS_Radar_Middle1;
label
  l1;
asm
   test ecx, $10000000
   je l1
   mov esi,$4674A9
   jmp esi;
l1:
   mov esi,[esp+$10]
   mov ecx,$4674ff
   jmp ecx;
end; {GameUnit_LOS_Radar_Middle1}

Procedure GameUnit_LOS_Radar_Middle2;
label // scratch register : ebx
  l1;
asm
//   mov ebx, ViewPlayer
//   cmp CurrentLOS, ebx
   cmp CurrentLOS,0
   jnz l1;
   // set the unit's status indicator (bitmapped flags)
   mov [eax],ecx;
l1:
   mov esi,[esp+$10]
   mov ecx,$4674ff
   jmp ecx;
end; {GameUnit_LOS_Radar_Middle1}

end.
 