unit LOS_GUIText;

interface
uses
  PluginEngine;

Procedure LOS_GUIText_Unit_Resources;
Procedure LOS_GUIText_SeeCommHealth;
Procedure LOS_GUIText_Unit_KillsNStuff;

Procedure GetCodeInjections( PluginData : TPluginData );

implementation
uses
  LOS_extensions,
  TADemoConsts,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations;

Procedure GetCodeInjections( PluginData : TPluginData );
begin
// allows allies to see stats for ally's unit
PluginData.MakeRelativeJmp( State_LOS_GUIText,
                 'LOS_GUIText_Unit_Resources',
                 @LOS_GUIText_Unit_Resources,
                 $46AC85,1);

// lets you see the comm's health even if it is allied
PluginData.MakeRelativeJmp( State_LOS_GUIText,
                 'LOS_GUIText_SeeCommHealth',
                 @LOS_GUIText_SeeCommHealth,
                 $46B013,1);

// enables stuff like the kill count & vet level to be seen
PluginData.MakeRelativeJmp( State_LOS_GUIText,
                 'LOS_GUIText_Unit_KillsNStuff',
                 @LOS_GUIText_Unit_KillsNStuff,
                 $46B10E);

end; {GetCodeInjections}

Procedure LOS_GUIText_Unit_Resources;
{
  Incoming :
           esi - unitstruct
  Outgoing :
           edx - TAdynmemStructPtr          
  Scratch  :
           edx, eax, ecx
} 
Label
  CanSeeUnit,
  CanNotSeeUnit;
asm
  mov word ptr [esp+$5a], 0; //$24C+var_1F2

  mov edx, [TADynmemStructPtr]
  xor eax, eax;
  xor ecx, ecx; 

{
  // if ( ShareLosWith[TAdynmemStruct.LOS_Sight][UnitStruct.OwnerIndex] != -1) goto CanSeeUnit else goto CanNotSeeUnit
  mov cl, [esi+UnitStruct_OwnerIndex]//UnitStruct.OwnerIndex]
  mov al, [edx+TAdynmemStruct_LOS_Sight]
  mov eax, [Longword(ShareLosWith)+eax*4]
  mov ecx, [eax+ecx*4]
  cmp ecx, longword(-1)
  jnz CanSeeUnit
}
  // check if the player is allied
  xor eax, eax  
  mov al, [edx+TTADynMemStruct.cViewPlayerID]
  mov ecx, [esi+TUnitStruct.p_Owner]
  cmp byte [eax+ecx+TPlayerStruct.cAllyFlagArray], 0
  jnz CanSeeUnit

CanNotSeeUnit:
  push $46ACC8;
  call PatchNJump;
CanSeeUnit:
  push $46ACA0;
  call PatchNJump;
end;

Procedure LOS_GUIText_SeeCommHealth;
{
  Incoming :

  Outgoing :
           esi - unitstruct           
  Scratch  :
           edx, eax, ecx
} 
label
  CanNotSeeUnit,
  CanSeeUnit;
asm
  mov esi, [esp+$24] // [esp+24Ch+Unit]
  mov eax, [TADynmemStructPtr]
{
  xor edx, edx;
  xor ecx, ecx;

  // if ( ShareLosWith[TAdynmemStruct.LOS_Sight][UnitStruct.OwnerIndex] != -1) goto CanSeeUnit else goto CanNotSeeUnit
  mov cl, [esi+UnitStruct_OwnerIndex]//UnitStruct.OwnerIndex]
  mov dl, [eax+TAdynmemStruct_LOS_Sight]
  mov edx, [Longword(ShareLosWith)+edx*4]
  mov ecx, [edx+ecx*4]
  cmp ecx, longword(-1)
  jnz CanSeeUnit
}
  // check if the player is allied
  xor edx, edx  
  mov dl, [eax+TTADynMemStruct.cViewPlayerID]
  mov ecx, [esi+TUnitStruct.p_Owner]
  cmp byte [edx+ecx+TPlayerStruct.cAllyFlagArray], 0
  jnz CanSeeUnit
    
CanNotSeeUnit:
  push $46B033;
  call PatchNJump;

CanSeeUnit:
  push $46B048;
  call PatchNJump;
end;

Procedure LOS_GUIText_Unit_KillsNStuff;
{
  Incoming :
           esi - unitstruct
  Outgoing :
           eax - TAdynmemStructPtr          
  Scratch  :
           edx, eax, ecx
} 
label
  CanNotSeeUnit,
  CanSeeUnit;
asm
  mov eax, [TADynmemStructPtr]
{  xor edx, edx;
  xor ecx, ecx; 

  // if ( ShareLosWith[TAdynmemStruct.LOS_Sight][UnitStruct.OwnerIndex] != -1) goto CanSeeUnit else goto CanNotSeeUnit
  mov cl, [esi+UnitStruct_OwnerIndex]//UnitStruct.OwnerIndex]
  mov dl, [eax+TAdynmemStruct_LOS_Sight]
  mov edx, [Longword(ShareLosWith)+edx*4]
  mov ecx, [edx+ecx*4]
  cmp ecx, longword(-1)
  jnz CanSeeUnit
}
  // check if the player is allied
  xor edx, edx
  mov dl, [eax+TTADynMemStruct.cViewPlayerID]
  mov ecx, [esi+TUnitStruct.p_Owner]
  cmp byte [edx+ecx+TPlayerStruct.cAllyFlagArray], 0   
  jnz CanSeeUnit
    
CanNotSeeUnit:
  push $46B121;
  call PatchNJump;

CanSeeUnit:
  push $46B12E;
  call PatchNJump;
end;


end.
