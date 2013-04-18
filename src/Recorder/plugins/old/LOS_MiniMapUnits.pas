unit LOS_MiniMapUnits;

interface
uses
  PluginEngine;

// todo : needs work. Not to sure what this should be doing, but it isnt doing it right. Doesnt crash now!

Procedure GameUnit_LOS_MiniMapUnits_Start;
Procedure GameUnit_LOS_MiniMapUnits_CanSeeUnit;

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
CodeInjection.Enabled := State_LOS_MiniMap_units;
CodeInjection.Name:= 'GameUnit_LOS_MiniMapUnits_Start';
CodeInjection.JumpToAddress := @GameUnit_LOS_MiniMapUnits_Start ;
CodeInjection.InjectionPoint := $466C20;
CodeInjection.Writesize := 6;
CodeInjection := PluginData.NewCodeInjection();
// breaks at $466
CodeInjection.Enabled := State_LOS_MiniMap_units;
CodeInjection.Name:= 'GameUnit_LOS_MiniMapUnits_CanSeeUnit';
CodeInjection.JumpToAddress := @GameUnit_LOS_MiniMapUnits_CanSeeUnit ;
CodeInjection.InjectionPoint := $;
CodeInjection.Writesize := ;
end; 

// -----------------------------------------------------------------------------
//    GameUnit_LOS_MiniMap_units
// -----------------------------------------------------------------------------

Procedure GameUnit_LOS_MiniMapUnits_Start;
asm   // Scratch registers ecx, edx
  // function prolog
  sub esp, $24

  // preserve the initial player viewpoint
  // ViewPlayer = [TADynmemStrutPtr+TAdynmemStruct.LOS_Sight]
  mov edx, [$511de8] // mov esi, TADynmemStrutPtr
  xor ecx, ecx
  mov cl, [edx+$2a43] // mov cl, [esi+TAdynmemStruct.LOS_Sight]
  mov ViewPlayer, ecx;

  // ecx is require to be set to [$511de8] (TADynmemStrutPtr) before jumping to the start of the loop

  // jump to the start of the LOS Radar processing
  mov edx, $466C29
  jmp edx;
end; {GameUnit_LOS_MiniMapUnits_Start}

Procedure GameUnit_LOS_MiniMapUnits_CanSeeUnit;
label
  CanSeeUnit;
// Scratch registers: ecx, edx
asm // uses: ecx, edx


CanSeeUnit:


end;


end.
 