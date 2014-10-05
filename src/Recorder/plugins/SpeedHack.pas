unit SpeedHack;
{
Based off:
bLoBbY's TA Speed Hack
http://www.tauniverse.com/forum/showthread.php?p=465743

"Does it annoy you that you can only set the speed in TA to +10? Well no more!
With this hack, you can increase the speed as far as you want! (Only tested in
skirmish, not multiplayer)."


Links ".syncon <lower limit> <upper limit>" into the actual game. Completely prevents speedjacking
}
interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_SpeedHack : boolean = true;
  
function GetPlugin : TPluginData;
  
// -----------------------------------------------------------------------------

Procedure OnInstallSpeedHack;
Procedure OnUninstallSpeedHack;

// -----------------------------------------------------------------------------
//  Asm data structures. Most of this is used to simplify the asm at the expense of memory
// -----------------------------------------------------------------------------

Const
  // theoretical min/max speed
  Game_MaxSpeed = high(byte);
  Game_MinSpeed = low(byte);
  // highest stock TA will allow
  TA_MaxSpeed = 20;
  TA_MinSpeed = 0;

var
  Upperlimit : word = TA_MaxSpeed;
  LowerLimit : word = TA_MinSpeed;

Procedure ResetGameSpeedLimits;
// -----------------------------------------------------------------------------

// asm block
Procedure SpeedState_IncreaseSpeed_Check;
Procedure SpeedState_DecreaseSpeed_Check;
Procedure SpeedState_SetSpeed_Check;

implementation
uses
  TADemoConsts,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations;

Procedure ResetGameSpeedLimits;
begin
Upperlimit := TA_MaxSpeed;
LowerLimit := TA_MinSpeed;
end;
  
Procedure OnInstallSpeedHack;
begin
ResetGameSpeedLimits;
end;

Procedure OnUninstallSpeedHack;
begin
end;

{
Using a Hex editor (I use xvi32), open totala.exe (backup first!!):
Find (in hex):
*730E*25FFFF00006A014050E822A8FFFF
Replace with:
*9090*25FFFF00006A014050E822A8FFFF

Find (in hex):
*7E*05BB1400000083
Replace with:
*EB*05BB1400000083

The offsets in my TA (3.1.0.0) were:
0x000959BE  - virtual address: 0x4965BE
and
0x000901FD  - virtual address: 0x490DFD

@ 0x000901FD
jle short 0x00490E04 => jmp short 0X00490E04 (0x7E => 0xEB)
7E05BB1400000083FB017D05BB010000 => EB05BB1400000083FB017D05BB010000

@ 0x000959BE:
jnb short 0x004965CE => nop, nop (0x730E => 0x9090)
730E25FFFF00006A014050E822A8FFFF => 909025FFFF00006A014050E8F0FEFFFF

  result.MakeNOPReplacement( State_SpeedHack,
                             'Speed hack #1',
                             $4965BE,
                             2 );
  result.MakeReplacement( State_SpeedHack,
                          'Speed hack #2',
                          $490DFD,
                          [$EB] );
}

function GetPlugin : TPluginData;
begin
if IsTAVersion31 and State_SpeedHack then
  begin

  result := TPluginData.create( true,
                                'Speed hack',
                                State_SpeedHack,
                                @OnInstallSpeedHack, @OnUnInstallSpeedHack );

  result.MakeRelativeJmp( State_SpeedHack,
                          'Increase speed check',
                          @SpeedState_IncreaseSpeed_Check,
                          $4965B3,
                          $10 );

  result.MakeRelativeJmp( State_SpeedHack,
                          'Decrease speed check',
                          @SpeedState_DecreaseSpeed_Check,
                          $496559,
                          $10 );

  result.MakeRelativeJmp( State_SpeedHack,
                          'Set speed Check',
                          @SpeedState_SetSpeed_Check,
                          $490DF9,
                          $10 );
  end
else
  result := nil;  
end;

Procedure SpeedState_DecreaseSpeed_Check;
{
  Incoming :
            edx - TADynmemStructPtr
  Outgoing :
            eax - speed to send to network 
  Scratch  :
            ecx, edx, esi
} // uses : ecx
label
  DontSetSpeed;
asm
  // set up registers
  xor  ecx, ecx
  xor  eax, eax
  mov  cx, LowerLimit
  inc  cx
  // check to see if decreasing the gamespeed hit the minium
  mov  ax, [edx+TTADynMemStruct.nTAGameSpeed]
  cmp  ax, LowerLimit
  jbe  DontSetSpeed
  // decrease the gamespeed count & call the set speed function
  push 1
  dec  eax
  push $4965C8;
  call PatchNJump;
DontSetSpeed:
  push $4965CE;
  call PatchNJump;
end;

Procedure SpeedState_IncreaseSpeed_Check;
{
  Incoming :
            edx - TADynmemStructPtr
  Outgoing :
            eax - speed to send to network
  Scratch  :
            ecx
} // uses : ecx
label
  DontSetSpeed;
asm
  // set up registers
  xor  ecx, ecx
  xor  eax, eax
  mov  cx, UpperLimit
  // check to see if increasing the gamespeed hit the minium  
  mov  ax, [edx+TTADynMemStruct.nTAGameSpeed]
  cmp  ax, cx
  jnb  DontSetSpeed
  // increase the gamespeed count & call the set speed function
  push 1
  inc  eax
  push $4965C8;
  call PatchNJump;
DontSetSpeed:
  push $4965CE;
  call PatchNJump;
end;


procedure SpeedState_SetSpeed_Check;
{
  Incoming :
            ebx - new game speed
  Outgoing :
            ebx - new game speed
  Scratch  :
            eax, ecx, edi
} // uses : ecx
label
  DoMinSpeedCheck,
  ExitPoint;
asm
  push edi
  // do upper limit check
  xor  eax, eax
  mov  ax, UpperLimit

  cmp  ebx, eax
  jle  DoMinSpeedCheck
  // value is too high, cap it
  mov  ebx, eax
DoMinSpeedCheck:
  // do lower limit check
  mov  ax, lowerLimit
  
  cmp  ebx, eax
  jge  ExitPoint
  // value is too low, cap it  
  mov  ebx, eax
ExitPoint:
  // resume execution
  push $490E0E;
  call PatchNJump;
end;

end.
