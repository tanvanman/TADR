unit LOS_Radar;

interface
uses
  PluginEngine;

var
  // the player currently being tested to see if it can see a unit
  TestPlayerIndex : longint;
  TestPlayerPtr : pointer;
  PlayerPtr : pointer;
  ViewPlayerRadar: longint;

//radar
Procedure GameUnit_LOS_Radar_CanDirectlySeeUnit;

Procedure GameUnit_LOS_Radar_LoopStart;
Procedure GameUnit_LOS_Radar_LoopCondition;

Procedure GetCodeInjections( PluginData : TPluginData );


implementation
uses
  LOS_extensions,
  TADemoConsts,
  TA_MemoryStructures,
  TA_MemoryConstants;

{
419090 - ShareAll
418FD0 - ShareRadar
418E50 - ShareMapping
}

Procedure GetCodeInjections( PluginData : TPluginData );
begin
PluginData.MakeRelativeJmp( State_LOS_Radar,
                            'GameUnit_LOS_Radar_CanDirectlySeeUnit',
                            @GameUnit_LOS_Radar_CanDirectlySeeUnit,
                            $4674AB,
                            4);
// todo : figure out how to remove this loop!
PluginData.MakeRelativeJmp( State_LOS_Radar,
                            'GameUnit_LOS_Radar_LoopStart',
                            @GameUnit_LOS_Radar_LoopStart ,
                            $46750E,
                          //  $467446,
                            1);
PluginData.MakeRelativeJmp( State_LOS_Radar,
                            'GameUnit_LOS_Radar_LoopCondition',
                            @GameUnit_LOS_Radar_LoopCondition,
                            $46782F,
                            2);
end;

// -----------------------------------------------------------------------------
//    GameUnit_LOS_Radar
// -----------------------------------------------------------------------------

{
  Replaces the loop which determines if a unit can be directly seen on radar
}
Procedure GameUnit_LOS_Radar_CanDirectlySeeUnit;
{
  Incoming :
           dl - TAdynmemStruct.LOS_Radar player
           cl - unit's player
           edi - Unit
           eax - offset into unit
           esi - unit status flags

 Scratch   :
           ecx, ebx, esi
}
// todo : save "edi", fairly sure it changes during loop
label
  CanSeeUnitOnRadar,
  CanNotSeeUnitOnRadar,
  SetRadarVisibleState,
  SkipUnit;
asm // uses ecx, ebx, esi

  // check if we need to skip this unit
  mov ecx, [eax]
  test ecx, 10000000h
  jz SkipUnit
  // unit status flag manipulation
  mov esi, ecx
  xor ecx, ecx
  mov cl, [eax-11h]// UnitStruct.OwnerIndex
  and  esi, 0FFFFEFFFh
  mov  [eax], esi

  // if (UnitStruct.Owner.AlliedPlayers[Player.Index])
  mov ecx, [eax-7Ah]   // UnitStruct.Owner^
  xor ebx, ebx
  mov bl, [ebp+TPlayerStruct.cPlayerIndex]
  cmp byte [ebx+ecx+TPlayerStruct.cAllyFlagArray], 0   // check if the unit's player is allied
  jnz CanSeeUnitOnRadar

  // only chance to see units via radar if the player is watching
  cmp dword ptr [ebp], 0
  jz CanNotSeeUnitOnRadar
  mov ecx, [ebp+TPlayerStruct.PlayerInfo]
  test [ecx+TPlayerInfoStruct.SharedBits], 40h
  jnz CanSeeUnitOnRadar

// Cant directly see the unit on radar
CanNotSeeUnitOnRadar:
  mov ecx, [eax]
  and ch, 0F8h
  jmp SetRadarVisibleState
  
// we definitely can see this unit via radar
CanSeeUnitOnRadar:
  mov ecx, [eax]
  or ch, 3

SetRadarVisibleState:
  mov [eax], ecx

// unit isnt even up for consideration  
SkipUnit:
  push $4674FB
  call PatchNJump;
end;


Procedure GameUnit_LOS_Radar_LoopStart;
label
  l1,l2;
asm
  // code thumped by injecting the jmp statement
  mov esi, [ebp+TPlayerStruct.p_UnitsArray]
  mov eax, [ebp+TPlayerStruct.p_LastUnit]
  cmp esi, eax
  ja l2
  jmp l1;
  
l2:
  push $4675D2;
  call PatchNJump;
l1:

//  push edi
  // Scratch registers: ecx, esi

  // TestPlayerIndex = 0  
  xor ecx, ecx
  mov TestPlayerIndex, ecx;
  
  // ViewPlayer = [TADynmemStrutPtr+TAdynmemStruct.LOS_Sight]
  mov esi, [TADynmemStructPtr] // mov esi, TADynmemStrutPtr
  mov cl, [esi+TTADynMemStruct.cViewPlayerID]
  mov ViewPlayerRadar, ecx;

  // TestPlayerPtr = TAdynmemStruct.Players[0]
  mov eax, esi
  add eax, Integer(TTAdynmemStruct.Players)
  mov TestPlayerPtr, eax
  
  // PlayerPtr = TAdynmemStruct.Players[ViewPlayer]
  add esi, Integer(TTAdynmemStruct.Players)
  mov eax, Integer(SizeOf(TPlayerStruct))
  mul eax, ecx
  add esi, eax
  mov PlayerPtr, esi



  jmp GameUnit_LOS_Radar_LoopCondition;
end;


Procedure GameUnit_LOS_Radar_LoopCondition;
{
Outgoing :
          esi - [TADynmemStructPtr]
}
label
  NextValue,
  CleanupNExit,
  SkipTest,
  TryNextPlayer_NextValue,
  TryNextPlayer_Condition;
// Scratch registers: ecx, esi
asm // uses: ecx, esi
  mov ecx, TestPlayerIndex;
  mov eax, TestPlayerPtr;
  mov esi, ViewPlayerRadar;
  jmp TryNextPlayer_Condition
  // todo : fix me!
TryNextPlayer_NextValue:
  // TestPlayerIndex++
  inc ecx;
  // TestPlayerPtr++
  add eax, Integer(SizeOf(TPlayerStruct))
  // if (TestPlayerIndex >= 10) then exit; 
TryNextPlayer_Condition:
  cmp ecx, 10
  jnb CleanupNExit
  // if (TAdynmemStruct.Players[TestPlayerIndex].AlliedPlayers[ViewPlayer]) continue;
  cmp byte [eax+esi+TPlayerStruct.cAllyFlagArray], 0
  jz TryNextPlayer_NextValue

  // setup for calcing LOS with this player
  mov esi, [TADynmemStructPtr] 
  mov [esi+TTADynMemStruct.cViewPlayerID], cl

  inc ecx
  mov TestPlayerIndex, ecx
  add eax, Integer(SizeOf(TPlayerStruct))
  mov TestPlayerPtr, eax

  mov     dl, [esi+TTADynMemStruct.cViewPlayerID]
  mov     edi, [esi+TTADynMemStruct.p_Units]
  mov     eax, edx
  mov     ebx, [esi+TTADynMemStruct.p_Units]
  and     eax, 0FFh
  add     edi, 118h
  mov     ecx, eax
  add     esi, eax
  shl     ecx, 5
  add     ecx, eax
  cmp     edi, ebx
  mov     [esp+14h], edi // [esp+38h+Units_Index]
  mov     [esp+10h], ebx  // [esp+38h+Units_EndMarker]
  lea     ecx, [ecx+ecx*4]
  lea     ebp, [esi+ecx*2+Integer(TTAdynmemStruct.Players)]


  // code thumped by injecting the jmp statement
  mov esi, [ebp+TPlayerStruct.p_UnitsArray]
  mov eax, [ebp+TPlayerStruct.p_LastUnit]
  cmp esi, eax
  ja SkipTest

  push $46751C;
  call PatchNJump  
  
SkipTest:
  push $4675D2;
  call PatchNJump;

{
  // jump to the start of the LOS Radar processing
  push $46744D
  call PatchNJump
}

CleanupNExit:
  // finished doing LOS_Radar stuff, cleanup

  // [TADynmemStrutPtr+TAdynmemStruct.LOS_Sight] = ViewPlayer
  mov ecx, ViewPlayerRadar;
  mov esi, [TADynmemStructPtr]
  mov [esi+TTADynMemStruct.cViewPlayerID], cl
  xor eax, eax
  // function epilog
  pop edi
  pop esi
  pop ebp
  pop ebx
  add esp, $28
  retn
end;

end.

