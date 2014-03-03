unit LOS_MiniMapUnits;

interface
uses
  PluginEngine;

Procedure GameUnit_LOS_MiniMap_UnitsProjectiles_IsOwner;

Procedure GetCodeInjections( PluginData : TPluginData );

implementation
uses
  LOS_extensions,
  TADemoConsts,
  TA_MemoryConstants;


Procedure GetCodeInjections( PluginData : TPluginData );
begin
PluginData.MakeRelativeJmp( State_LOS_MiniMap,
                            'GameUnit_LOS_MiniMap_UnitsProjectiles_CanSeeUnit',
                            @GameUnit_LOS_MiniMap_UnitsProjectiles_IsOwner ,
                            $466E6F,
                            1);
end;

// -----------------------------------------------------------------------------
//    GameUnit_LOS_MiniMap_UnitsProjectiles
// -----------------------------------------------------------------------------

Procedure GameUnit_LOS_MiniMap_UnitsProjectiles_IsOwner;
{
  Incoming :
           ebx - Unit
           esi - TAdynmemStruct

 Scratch   :
           eax, edx
}
label
  IsOwnerAlly;
asm 
  // if (UnitStruct.OwnerPtr.AlliedPlayers[TAdynmemStruct.LOS_Sight])
  xor eax, eax
  mov al, [esi+TAdynmemStruct_LOS_Sight]
  mov edx, [ebx+UnitStruct_OwnerPtr]
  cmp byte [eax+edx+PlayerStruct_AlliedPlayers], 0
  jnz IsOwnerAlly

  push $46718C
  call PatchNJump;
IsOwnerAlly:
  push $466E83
  call PatchNJump;
end;

end.
 