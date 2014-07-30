unit BattlerRoomMemFix;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_BattlerRoomMemFix : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallBattlerRoomMemFix;
Procedure OnUninstallBattlerRoomMemFix;

// -----------------------------------------------------------------------------

procedure BattlerRoomMemFix_GetMemory;

implementation
uses
  Windows,
  TA_MemoryConstants,
  TA_MemoryLocations;

type
  DWORDLONG = UInt64;

  PMemoryStatusEx = ^TMemoryStatusEx;
  TMemoryStatusEx = packed record
    dwLength: DWORD;
    dwMemoryLoad: DWORD;
    ullTotalPhys: DWORDLONG;
    ullAvailPhys: DWORDLONG;
    ullTotalPageFile: DWORDLONG;
    ullAvailPageFile: DWORDLONG;
    ullTotalVirtual: DWORDLONG;
    ullAvailVirtual: DWORDLONG;
    ullAvailExtendedVirtual: DWORDLONG;
  end;

var
  BattlerRoomMemFixPlugin: TPluginData;

function GlobalMemoryStatusEx(var lpBuffer: TMemoryStatusEx): BOOL; stdcall; external kernel32;

Procedure OnInstallBattlerRoomMemFix;
begin
end;

Procedure OnUninstallBattlerRoomMemFix;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_BattlerRoomMemFix then
  begin
    BattlerRoomMemFixPlugin := TPluginData.create( false,
                            '',
                            State_BattlerRoomMemFix,
                            @OnInstallBattlerRoomMemFix,
                            @OnUnInstallBattlerRoomMemFix );

    BattlerRoomMemFixPlugin.MakeRelativeJmp( State_BattlerRoomMemFix,
                          '',
                          @BattlerRoomMemFix_GetMemory,
                          $004643B0, 0);

    Result:= BattlerRoomMemFixPlugin;
  end else
    Result := nil;
end;

function GetMemory : Word; cdecl;
var
  MemStatus: TMemoryStatusEx;
  lTotalRAM : Integer;
begin
  Result := 0;
  FillChar(MemStatus, SizeOf(MemStatus), 0);
  MemStatus.dwLength := SizeOf(MemStatus);
  try
    if GlobalMemoryStatusEx(MemStatus) then
      lTotalRAM := MemStatus.ullTotalPhys div 1048576
    else
      Exit;
  except
    Exit;
  end;
    
  if lTotalRAM > High(Word) then
    Result := High(Word)
  else
    Result := lTotalRAM;
end;

{
and     edx, 0FFFFFh
add     eax, edx
sar     eax, 14h
inc     eax
mov     word ptr [ecx+PacketPlayerInfo.PlayerInfo.field_98], ax
}
procedure BattlerRoomMemFix_GetMemory;
label
  OrginalTAMemRead;
asm
    push    eax
    push    edx
    push    ecx
    call GetMemory
    test    eax, eax
    je OrginalTAMemRead
    pop     ecx
    mov     [ecx+99h], ax
    pop     edx
    pop     eax
    push $004643C3;
    call PatchNJump;
OrginalTAMemRead :
    pop     ecx
    pop     edx
    pop     eax
    and     edx, 0FFFFFh
    push $004643B6;
    call PatchNJump;
end;

end.

