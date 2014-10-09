unit LOS_PlayerSeeUnit;

interface
uses
  PluginEngine;

var
  // the register containing the player & unit pointers can be overwritten
  UnitPtr : pointer;
  PlayerPtr : pointer;
  // the player currently being tested to see if it can see a unit
  TestPlayer : longint;
  ViewPlayerPlSeeU: longint;
  TestPlayerPlSeeU: longint;

Procedure GameUnit_LOS_PlayerSeeUnit_IsOwnerAllowedToSee;
Procedure GameUnit_LOS_PlayerSeeUnit_EpilogCode_Exit;
Procedure GameUnit_LOS_PlayerSeeUnit_EpilogCode_Retry;
Procedure GameUnit_LOS_PlayerSeeUnit_EpilogCode_Test;
Procedure GameUnit_LOS_PlayerSeeUnit_EpilogCode_FinalCheck;



Procedure GetCodeInjections( PluginData : TPluginData );

implementation
uses
  LOS_extensions,
  TADemoConsts,
  TA_MemoryConstants,
  TA_MemoryStructures;

Procedure GetCodeInjections( PluginData : TPluginData );
begin
// todo : trace *all* ownership checks and remove them

// patch a call site to GameUnit_LOS_PlayerSeeUnit so they dont skip it
PluginData.MakeNOPReplacement( State_LOS_PlayerSeeUnit,
                               'GameUnit_LOS_Sight_PlayerSeeUnit_CallSite',
                               $48BC3D, $13 );

// patch the header for a check to
PluginData.MakeRelativeJmp( State_LOS_PlayerSeeUnit,
                 'GameUnit_LOS_PlayerSeeUnit_IsOwnerAllowedToSee',
                 @GameUnit_LOS_PlayerSeeUnit_IsOwnerAllowedToSee,
                 $465AD4);
{ note: no longer needed because of LOS sharing between allies
// patch the epilog/exit paths which GameUnit_LOS_PlayerSeeUnit can return false
PluginData.MakeRelativeJmp( State_LOS_PlayerSeeUnit,
                 'GameUnit_LOS_PlayerSeeUnit_EpilogCode_1',
                 @GameUnit_LOS_PlayerSeeUnit_EpilogCode_Retry,
                 $465AF1);

PluginData.MakeRelativeJmp( State_LOS_PlayerSeeUnit,
                 'GameUnit_LOS_PlayerSeeUnit_EpilogCode_2',
                 @GameUnit_LOS_PlayerSeeUnit_EpilogCode_Retry,
                 $465B50);

PluginData.MakeRelativeJmp( State_LOS_PlayerSeeUnit,
                 'GameUnit_LOS_PlayerSeeUnit_EpilogCode_3',
                 @GameUnit_LOS_PlayerSeeUnit_EpilogCode_Test,
                 $465D88);

PluginData.MakeRelativeJmp( State_LOS_PlayerSeeUnit,
                 'GameUnit_LOS_PlayerSeeUnit_EpilogCode_4',
                 @GameUnit_LOS_PlayerSeeUnit_EpilogCode_Test,
                 $465D9D);

PluginData.MakeRelativeJmp( State_LOS_PlayerSeeUnit,
                 'GameUnit_LOS_PlayerSeeUnit_EpilogCode_5',
                 @GameUnit_LOS_PlayerSeeUnit_EpilogCode_Test,
                 $465DD4);

PluginData.MakeRelativeJmp( State_LOS_PlayerSeeUnit,
                 'GameUnit_LOS_PlayerSeeUnit_EpilogCode_FinalCheck',
                 @GameUnit_LOS_PlayerSeeUnit_EpilogCode_FinalCheck,
                 $465E19);
}                 
{
LOS_PlayerCanSeeUnit results:
  1 - can see unit
  0 - Can not see unit
}
end; {GetCodeInjections}

Procedure GameUnit_LOS_PlayerSeeUnit_IsOwnerAllowedToSee;
{
  Incoming :
           esi - Player
           ebx - unit
  Scratch
           eax, ebp, edi, ecx, edx
}
label
  CanSeeUnit;
asm  // uses eax, ecx
  // prolog code
  push edi

  xor ecx, ecx;
  // store the unit pointer
  mov UnitPtr, ebx;
  mov PlayerPtr, esi;
  mov TestPlayer, ecx;

  // preserve the initial player viewpoint
  // ViewPlayer = [TADynmemStructPtr+TAdynmemStruct.LOS_Sight]
  mov eax, [TADynmemStructPtr]
  mov cl, [eax+TTADynMemStruct.cViewPlayerID]
  mov ViewPlayerPlSeeU, ecx;
  // check to see if this player is allowed to see the unit
{
  xor eax, eax;
  xor ecx, ecx;    
  // if ( ShareLosWith[Player.PlayerNum][UnitStruct.OwnerIndex] != -1) goto CanSeeUnit 
  mov al, [esi+PlayerStruct_Index]
  mov cl, [ebx+UnitStruct_OwnerIndex]
  mov eax, [Longword(ShareLosWith)+eax*4]
  mov ecx, [eax+ecx*4]
  cmp ecx, longword(-1)
  jnz CanSeeUnit
}
  // check if the player is allied
  xor eax, eax  
  mov al, [esi+TPlayerStruct.cPlayerIndex]
  mov ecx, [ebx+TUnitStruct.p_Owner]
  cmp byte [eax+ecx+TPlayerStruct.cAllyFlagArray], 0   // check if the unit's player is allied
  jnz CanSeeUnit

  // we *might* be able to see this unit
  push $465AE8 // continue LOS checks
  call PatchNJump;
CanSeeUnit:
  // we definitely can see this unit
  mov eax, 1;
  jmp GameUnit_LOS_PlayerSeeUnit_EpilogCode_Exit;
end;

Procedure GameUnit_LOS_PlayerSeeUnit_EpilogCode_Test;
{
  Incoming :
           esi - Player
           ebx - unit
           ecx - function result value
  Outgoing :
           eax - function result value           
  Scratch  :
           edx
}
label // uses eax, ecx
  DoProlog, KeepLooping;
asm
  mov eax, ecx;
  // test the result, if zero retry the LOS checks, otherwise we can exit
  // if ( eax )  goto GameUnit_LOS_PlayerSeeUnit_EpilogCode_Exit;  else  goto GameUnit_LOS_PlayerSeeUnit_EpilogCode_Retry; 
  test eax, eax
  jz GameUnit_LOS_PlayerSeeUnit_EpilogCode_Retry;
  jmp GameUnit_LOS_PlayerSeeUnit_EpilogCode_Exit;
end;

Procedure GameUnit_LOS_PlayerSeeUnit_EpilogCode_Retry;
{
  Incoming :
           esi - Player
           ebx - unit
  Outgoing :
           eax - function result value
  Scratch  :
           ecx, edx, ebp, edi
}
label
  CanSeeUnit,
  CanNotSeeUnit,
  TestPlayerLOS,
  TryNextPlayer_NextValue,
  TryNextPlayer_Condition;
asm // uses ecx, eax, edi, ebp
  // restore the unit pointer
  mov ebx, UnitPtr;
  
  // if ( UnitStruct.Owner.AlliedPlayers[ViewPlayer] != 0) goto CanSeeUnit; else goto TryNextPlayer;
  
  mov edi, ViewPlayerPlSeeU
  mov ecx, [ebx+TUnitStruct.p_Owner]
  cmp byte [edi+ecx+TPlayerStruct.cAllyFlagArray], 0
  jnz CanSeeUnit

  // select a new player to test (skip ourself)
(*
  for (TestPlayer = 0; TestPlayer < 10; TestPlayer++)
  {
  NextPlayer = TAdynmemStruct.Players[ViewPlayer]
  if (NextPlayer.AlliedPlayers[TestPlayer] != 0)
    {
    if (NextPlayer = Player)  continue;
    if (!player^.Active)  continue;
    Player = NextPlayer;
    DoLOSChecks();
    }
  }
*)
  mov ebp, TestPlayerPlSeeU;
  jmp TryNextPlayer_Condition;
TryNextPlayer_NextValue:
  // TestPlayer++
  inc ebp;
  // if (TestPlayer >= 10) then exit;
TryNextPlayer_Condition:
  cmp ebp, $A;
  jnb CanNotSeeUnit;
  //  if (UnitStruct.Owner.AlliedPlayers[TestPlayer] == 0) goto TryNextPlayer_NextValue
  cmp byte [ebp+ecx+TPlayerStruct.cAllyFlagArray], 0
  jz TryNextPlayer_NextValue
// }
  //  if (UnitStruct.Owner.Index == TestPlayer) goto TryNextPlayer_NextValue
  cmp byte [ecx+TPlayerStruct.cPlayerIndex], bl
  jnz TryNextPlayer_NextValue

  mov TestPlayerPlSeeU, ebp;
    
  // NextPlayer = [TADynmemStructPtr+TAdynmemStruct.Players[TestPlayer]]
  mov eax, ebp
  mov ecx, Integer(SizeOf(TPlayerStruct))
  mul ecx;
  mov ecx, [TADynmemStructPtr];
  lea ecx, [ecx+Integer(TTAdynmemStruct.Players)];
  add eax, ecx;
  // if (NextPlayer = Player)  continue; 
  mov esi, PlayerPtr;
  cmp eax, esi
  jz TryNextPlayer_NextValue;
  // Player = NextPlayer;
  mov esi, eax;
  // if !(player^.Active)  continue;
  mov eax, [esi]
  test eax, eax
  jz TryNextPlayer_NextValue;
TestPlayerLOS:
  // TestPlayer++
  inc ebp;
  mov TestPlayerPlSeeU, ebp;  
  
  // restore the player pointer
  // we *might* can be able to see this unit
  push $465AFD
  call PatchNJump;
CanSeeUnit:
  // we definitely can see this unit
  mov eax, 1;
  // restore the player pointer
  mov esi, PlayerPtr;
  jmp GameUnit_LOS_PlayerSeeUnit_EpilogCode_Exit;
CanNotSeeUnit:
  mov TestPlayerPlSeeU, ebp;
  // we definitely can *not* see this unit
  xor eax, eax;
  // restore the player pointer
  mov esi, PlayerPtr;
  jmp GameUnit_LOS_PlayerSeeUnit_EpilogCode_Exit;
end;

Procedure GameUnit_LOS_PlayerSeeUnit_EpilogCode_FinalCheck;
{
  Incoming :
           esi - Player
           ebx - [TADynmemStructPtr]
           ecx - can player see unit (bool) 
  Outgoing :
           eax - function result value
        OR ebx - unit
  Scratch  :
           ebp, edi
}
label
  CheckNextPlayer_value,
  CheckNextPlayer_Condition,

  CheckReturn,
  DoReturn;
asm
  test ecx, ecx
  jnz DoReturn

  // while ( ShareLosWith[ViewPlayer][TestPlayer] != -1)
  // {
  xor edi, edi;
  jmp CheckNextPlayer_Condition;
CheckNextPlayer_value:
  inc edi;
CheckNextPlayer_Condition:
  // if (TestPlayer >= 10) then exit;
  cmp ebp, $A;
  jnb DoReturn;

  // LOS calcs
  push eax;
  push edx;  
  mov     cl, [ebx+TTADynMemStruct.cViewPlayerID]
  shl     edx, cl
  and     eax, edx
  neg     eax
  sbb     eax, eax
  xor     ecx, ecx
  neg     eax
  test    eax, eax
  setnz   cl
  mov     eax, ecx
  pop edx;
  pop eax;
  // check result
  test ecx, ecx
  jnz DoReturn
  jmp CheckNextPlayer_value;
  // }

DoReturn:
  mov     eax, ecx;
  add     esp, $0C;
  ret    8;
end;

Procedure GameUnit_LOS_PlayerSeeUnit_EpilogCode_Exit;
{
  Incoming :
           eax - function result value

  Scatch   :
           everything else      
}
asm
  // epilog code
  pop edi
  pop esi
  pop ebp
  pop ebx
  add esp, $0C
  ret 8
end;


end.
