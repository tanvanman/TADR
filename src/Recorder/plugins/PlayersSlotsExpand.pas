unit PlayersSlotsExpand;

interface
uses
  PluginEngine,
  TA_MemoryLocations,
  TA_MemoryStructures,
  TA_MemoryConstants;

// -----------------------------------------------------------------------------

const
  State_PlayersSlotsExpand : boolean = true;

var
  PlayersExp : array[0..32] of TPlayerStruct;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallPlayersSlotsExpand;
Procedure OnUninstallPlayersSlotsExpand;

// -----------------------------------------------------------------------------

Procedure InitPlayersArray;
Procedure SetPlayerStructMem;

implementation
uses
  Windows,
  TADemoConsts,
  TA_FunctionsU;

Procedure OnInstallPlayersSlotsExpand;
begin
end;

Procedure OnUninstallPlayersSlotsExpand;
begin
end;

function GetPlugin : TPluginData;
var
  cMaxplayers : Byte;
begin
  if IsTAVersion31 and State_PlayersSlotsExpand then
  begin
    Result := TPluginData.create( State_PlayersSlotsExpand,
                                  'PlayersSlotsExpand Plugin',
                                  State_PlayersSlotsExpand,
                                  @OnInstallPlayersSlotsExpand,
                                  @OnUninstallPlayersSlotsExpand );

    cMaxplayers := 32;
    
    Result.MakeReplacement( State_PlayersSlotsExpand,
                            '',
                            $00464995, cMaxPlayers, 1);

    Result.MakeRelativeJmp( State_PlayersSlotsExpand,
                            'Load game init players array',
                            @InitPlayersArray,
                            $00464992, 0);
    
    Result.MakeRelativeJmp( State_PlayersSlotsExpand,
                            '',
                            @SetPlayerStructMem,
                            $00401083, 1);
  end else
    Result := nil;
end;

procedure InitPlayersArr; stdcall;
var
  i : Integer;
begin
  for i := 0 to 32 do
  begin
    InitPlayerStruct(@PlayersExp[i]);
    // once finished this should be commented
    if i < 11 then
      InitPlayerStruct(@TAData.MainStruct.Players[i]);
  end;
end;

procedure InitPlayersArray;
asm
  pushAD
  call    InitPlayersArr
  popAD
  push $004649BF;
  call    PatchNJump;
end;

procedure SetPlayerStructMem;
asm
  lea     esi, PlayersExp
  and     eax, 255
  pop     edi
  mov     ecx, eax
  shl     ecx, 5
  add     ecx, eax
  lea     ecx, [ecx+ecx*4]
  lea     eax, [esi+ecx*2]
  push $004010A2;
  call PatchNJump;
end;

{
ehhh

Orginal array is
0..9 - players
10   - "extra" player

All the iterations stop because of idx = 10 or offset higher than 10 * SizeOf(TPlayerStruct)
which is 10 * 331 = 3310
, 0CEEh
, 0Ah

InitPlayers : 0x464990

0040EC22   028 80 FA 0A                    cmp     dl, 0Ah
0040EC5B   028 80 B8 46 01 00 00 0A        cmp     [eax+PlayerStruct.cPlayerIndex], 0Ah
0040EC82   028 83 FA 0A                    cmp     edx, 0Ah
0040ECBC   028 3C 0A                       cmp     al, 0Ah
0041629A 000 3C 0A                       cmp     al, 0Ah
004162DE 000 80 B9 46 01 00 00 0A        cmp     [ecx+PlayerStruct.cPlayerIndex], 0Ah
00416918 008 3C 0A                       cmp     al, 0Ah
00416963 008 80 B8 46 01 00 00 0A        cmp     [eax+PlayerStruct.cPlayerIndex], 0Ah
00416AC0 004 3C 0A                       cmp     al, 0Ah
00416B60 004 3C 0A                       cmp     al, 0Ah
00416BA7 004 80 B8 46 01 00 00 0A        cmp     [eax+PlayerStruct.cPlayerIndex], 0Ah
00416BE0 004 3C 0A                       cmp     al, 0Ah
00416C30 004 80 B9 46 01 00 00 0A        cmp     [ecx+PlayerStruct.cPlayerIndex], 0Ah
00417186 008 3C 0A                       cmp     al, 0Ah
0041DD37 024 80 BF 46 01 00 00 0A        cmp     [edi+PlayerStruct.cPlayerIndex], 0Ah

00401083 008 8B 35 E8 1D 51 00           mov     esi, TAMainStructPtr

00406E05 054 BB 0A 00 00 00              mov     ebx, 0Ah
00406E8F 050 A1 E8 1D 51 00              mov     eax, TAMainStructPtr
00406E98 050 BD 0A 00 00 00              mov     ebp, 0Ah

0040918D 014 8D 84 4F 63 1B 00 00        lea     eax, [edi+ecx*2+TAMainStruct.Player_Ary]

0040A100 000 A1 E8 1D 51 00              mov     eax, TAMainStructPtr
00407325 030 80 FA 0A                    cmp     dl, 0Ah

0040BB07 00C 8B 15 E8 1D 51 00           mov     edx, TAMainStructPtr

00416937 008 8B 0D E8 1D 51 00           mov     ecx, TAMainStructPtr

00416BF0 004 8B 15 E8 1D 51 00           mov     edx, TAMainStructPtr

0041721C 008 8B 15 E8 1D 51 00           mov     edx, TAMainStructPtr

00418BDE 008 8B 15 E8 1D 51 00           mov     edx, TAMainStructPtr

// eax
0041973C 020 8B B4 48 CA 1B 00 00        mov     esi, [eax+ecx*2+TAMainStruct.Player_Ary.Units_Begin]

0041AD3C 32C 8D B4 41 63 1B 00 00        lea     esi, [ecx+eax*2+TAMainStruct.Player_Ary]

0041B2F6 250 8B 3D E8 1D 51 00           mov     edi, TAMainStructPtr

0041DD0B 024 A1 E8 1D 51 00              mov     eax, TAMainStructPtr
0041DD37 024 80 BF 46 01 00 00 0A        cmp     [edi+PlayerStruct.cPlayerIndex], 0Ah

0041F31B 014 8B 0D E8 1D 51 00           mov     ecx, TAMainStructPtr

004204F9 468 8B 0D E8 1D 51 00           mov     ecx, TAMainStructPtr

004281B5 10C 8B 0D E8 1D 51 00           mov     ecx, TAMainStructPtr

00428439 10C 8D B0 63 1B 00 00           lea     esi, [eax+TAMainStruct.Player_Ary]

004449B2 00C 8B 0D E8 1D 51 00           mov     ecx, TAMainStructPtr

00444E4D 00C 8B 15 E8 1D 51 00           mov     edx, TAMainStructPtr

004453F5 8B 0D E8 1D 51 00               mov     ecx, TAMainStructPtr

sub_445450

0044553E 164 8B 0D E8 1D 51 00           mov     ecx, TAMainStructPtr

// only offset
004455D9 420 C7 44 24 18 63 1B 00 00     mov     [esp+420h+var_408], TAMainStruct.Player_Ary

0044636C 01C 8B 15 E8 1D 51 00           mov     edx, TAMainStructPtr

0044662D 074 A1 E8 1D 51 00              mov     eax, TAMainStructPtr

004469C0 A1 E8 1D 51 00                  mov     eax, TAMainStructPtr

00446A6D 05C 8B 0D E8 1D 51 00           mov     ecx, TAMainStructPtr

00446CC6 018 A1 E8 1D 51 00              mov     eax, TAMainStructPtr

00446D42 018 8B 35 E8 1D 51 00           mov     esi, TAMainStructPtr

00446EA2 004 8B 15 E8 1D 51 00           mov     edx, TAMainStructPtr

00446F54 8B 15 E8 1D 51 00               mov     edx, TAMainStructPtr

sub_446FB0

00446FFA 040 A1 E8 1D 51 00              mov     eax, TAMainStructPtr

00447185 078 8B 35 E8 1D 51 00           mov     esi, TAMainStructPtr

004471BA 084 8B 15 E8 1D 51 00           mov     edx, TAMainStructPtr

00447386 0F8 8B 0D E8 1D 51 00           mov     ecx, TAMainStructPtr

0044747C 108 8B 0D E8 1D 51 00           mov     ecx, TAMainStructPtr

}

end.
