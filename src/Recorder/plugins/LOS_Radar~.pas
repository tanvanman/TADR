unit LOS_Radar;

interface
uses
  PluginEngine;
// note: not quite working properly. (player 1 cant see player 2/3?)

//radar
Procedure GameUnit_LOS_Radar_Start;
Procedure GameUnit_LOS_Radar_CanSeeUnit;

Procedure GetCodeInjections( PluginData : TPluginData );


Procedure ForceShareMapNRadar( Player1Number, Player2Number : integer; IsSharing : boolean);

implementation
uses
  LOS_extensions,
  TADemoConsts,
  TA_MemoryLocations;


{
419090 - ShareAll
418FD0 - ShareRadar
418E50 - ShareMapping
}
Procedure ForceShareMapNRadar( Player1Number, Player2Number : integer; IsSharing : boolean);
begin
// todo : implement code which forces a player to share mapping info & radar Info
asm
{
TAdynmemStruct.Players.PlayerInfo == $1b8a
PlayerData.field_0.SharedBits == $97
TAdynmemStruct.SharedBits == $2a42

SharedMetal = $2
SharedEnergy = $4
SharedLOS = $8
ShareRadar = $40
ShareMapping = $20

  xor     edx,edx
  mov     dl, [ecx+TAdynmemStruct.SharedBits]
  mov     eax, edx
  add     ecx, edx
  shl     eax, 5
  add     eax, edx
  lea     eax, [eax+eax*4]
  mov     ecx, [ecx+eax*2+TAdynmemStruct.Players.PlayerInfo]
  mov     ax, word ptr [ecx+PlayerData.field_0.SharedBits]
  mov     edx, eax
  not     edx
  xor     dl, al
  and     edx, SharedLOS/ShareRadar/ShareMapping
  xor     edx, eax
  mov     word ptr [ecx+PlayerData.field_0.SharedBits], dx
  mov     ecx, TAdynmemStructPtr
  xor     edx, edx
  mov     dl, [ecx+TAdynmemStruct.SharedBits]
  mov     eax, edx
  add     ecx, edx
  shl     eax, 5
  add     eax, edx
  lea     eax, [eax+eax*4]
  mov     ecx, [ecx+eax*2+TAdynmemStruct.Players.PlayerInfo]
  test    [ecx+PlayerData.field_0.SharedBits], SharedLOS/ShareRadar/ShareMapping

}
end;
end;
  
Procedure GetCodeInjections( PluginData : TPluginData );
begin
PluginData.MakeRelativeJmp( State_LOS_Radar,
                            'GameUnit_LOS_Radar_Start',
                            @GameUnit_LOS_Radar_Start ,
                            $467446,
                            1);

PluginData.MakeRelativeJmp( State_LOS_Radar,
                            'GameUnit_LOS_Radar_CanSeeUnit',
                            @GameUnit_LOS_Radar_CanSeeUnit,
                            $4674AB,
                            4);

// todo : loop 0x46750E - 0x4675D2 to process allies radar as well! (ebp is player)

0x4675F8 write if statement around
.text:004675F5 mov     al, [esi+6Dh]
.text:004675F8 mov     cl, [ebp+PlayerStruct.PlayerIndex]
.text:004675FE cmp     al, cl
.text:00467600 jz      short loc_46765A

end;



// -----------------------------------------------------------------------------
//    GameUnit_LOS_Radar
// -----------------------------------------------------------------------------

Procedure GameUnit_LOS_Radar_Start;
{
  Outgoing :
           esi - must equal [TADynmemStructPtr] before leaving
  Scratch  :
           ecx, esi
}
asm
{
  // function prolog
  sub esp, $20
  push ebx
  push esi
  mov esi, [TADynmemStructPtr]
  push edi
}
  // preserve the initial player viewpoint
  // ViewPlayer = [TADynmemStructPtr+TAdynmemStruct.LOS_Sight]
  mov esi, [TADynmemStructPtr]
  xor ecx, ecx
  mov cl, [esi+TAdynmemStruct_LOS_Sight]
  mov ViewPlayer, ecx;
  // esi must equal TADynmemStructPtr before leaving

  // jump to the start of the LOS Radar processing
  push $46744C
  call PatchNJump;
end; {GameUnit_LOS_Radar_Start}


Procedure GameUnit_LOS_Radar_CanSeeUnit;
{
  Incoming :
           dl - TAdynmemStruct.LOS_Sight player
           cl - unit's player
           edi - Unit
           eax - offset into unit
           esi - unit status flags

 Scratch   :
           ecx, ebx, esi
}
label
  CanSeeUnitOnRadar,
  Loop_Next, Loop_Body,
  DefaultExitPoint;
asm // uses ecx, ebx, esi
  // unit status flag manipulation
  and  esi, 0FFFFEFFFh
  mov  [eax], esi
  // check to see if this player is allowed to see the unit via radar
  xor ecx, ecx
  mov cl, [eax-11h]                   // UnitStruct.OwnerIndex

  // if ( ShareLosWith[ViewPlayer][UnitStruct.OwnerIndex] != -1){ goto 0x4674F4} else {goto $4674BA}
  mov ebx, ViewPlayer
  mov ebx, [Longword(ShareLosWith)+ebx*4]
  mov ecx, [ebx+ecx*4]                     
  cmp ecx, longword(-1)
  jnz CanSeeUnitOnRadar
  // we *might* can be able to see this unit via radar
  push $4674BA
  call PatchNJump;
CanSeeUnitOnRadar:
  // we definitely can see this unit via radar
  push $4674F4
  call PatchNJump;
end; {GameUnit_LOS_Radar_CanSeeUnit}



end.
 