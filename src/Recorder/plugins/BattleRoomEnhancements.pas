unit BattleRoomEnhancements;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_BattleRoomEnhancements: Boolean = True;

function GetPlugin: TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallBattleRoomEnhancements;
Procedure OnUninstallBattleRoomEnhancements;

// -----------------------------------------------------------------------------

implementation
uses
  Classes,
  SysUtils,
  IniOptions,
  PlayerDataU,
  idplay,
  TADemoConsts,
  TA_MemoryStructures,
  TA_MemoryConstants,
  TA_FunctionsU,
  TA_MemoryLocations,
  TA_MemPlayers;

type
  PMemoryStatusEx = ^TMemoryStatusEx;
  TMemoryStatusEx = packed record
    dwLength: Cardinal;
    dwMemoryLoad: Cardinal;
    ullTotalPhys: UInt64;
    ullAvailPhys: UInt64;
    ullTotalPageFile: UInt64;
    ullAvailPageFile: UInt64;
    ullTotalVirtual: UInt64;
    ullAvailVirtual: UInt64;
    ullAvailExtendedVirtual: UInt64;
  end;

function GlobalMemoryStatusEx(var lpBuffer: TMemoryStatusEx): Boolean; stdcall; external 'kernel32.dll';

function GetInstalledRAM: Word; cdecl;
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

procedure BattleRoomEnhancements_GetMemory;
label
  OrginalTAMemRead;
asm
  push    eax
  push    edx
  push    ecx
  call GetInstalledRAM
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

function GetRandomMapEx: String;
var
  st: String;
  nr: Integer;
  tot: Integer;
  i: Integer;
  currchar: Char;
  pos: Cardinal;
  mapname: String;
begin
  Result := '';
  if IsLocalPlayerHost then
  begin
    tot := IterateMaps(0, 0, 0);
    if (not Assigned(MapsList)) then
    begin
      MapsList := TStringList.create;
      if tot > 0 then
      begin
        pos:= PCardinal(MultiplayerMapsList)^;
        nr:= 1;
        while nr < tot do
        begin
          for i:= 1 to 64 do
          begin
            currchar:= PChar(pos)^;
            if currchar <> #0 then
            begin
              mapname:= mapname + currchar;
              Inc(pos);
            end else
            begin
              MapsList.AddObject (mapname, pointer(nr));
              Inc(nr);
              mapname:= '';
              Inc(pos);
              Break;
            end;
          end;
        end;
      end;
    end;
    if MapsList.Count = 0 then Exit;
    nr := Random(tot-1);
    st := MapsList.strings[nr];
    Result := st;
  end;
end;

procedure CommanderWarpButtonState(State: Integer); stdcall;
begin
  if GlobalDPlay.CommanderWarp <> State then
  begin
    GlobalDPlay.CommanderWarp := State;
    GlobalDPlay.BroadcastExtraBattleRoomSettings(0);
  end;
end;

procedure BattleRoom_FIXEDLOC;
label
  LocFixedOrRandom,
  LocManual;
asm
  push    FIXEDLOC
  push    esi
  call    GUIGADGET_GetStatus
  cmp     eax, 2
  jz      LocManual
LocFixedOrRandom :
  push    0
  call    CommanderWarpButtonState
  mov     eax, [esp+10h]
  mov     ecx, [eax+27h]
  push $00448696;
  call PatchNJump;
LocManual :
  push    1
  call    CommanderWarpButtonState
  push $00448B98;
  call PatchNJump;
end;

procedure OverrideButtonsState(GUI: Pointer); stdcall;
begin
  if GlobalDPlay.CommanderWarp = 1 then
    GUIGADGET_SetStatus(GUI, PAnsiChar(FIXEDLOC), 2);
end;

procedure BattleRoom_OverrideButtonsState;
asm
  add     ecx, TTAdynmemStruct.p_TAGUIObject
  pushAD
  push    ecx
  call    OverrideButtonsState
  popAD
  push $0044AF32;
  call PatchNJump;
end;

function NewButtonsClicked(GUIHandle: Pointer): LongBool; stdcall;
var
  sMapName: String;
begin
  Result := False;
  if GUIGADGET_WasPressed(GUIHandle, PAnsiChar('AUTOPAUSE')) then
  begin
    Result := True;
    GlobalDPlay.AutoPauseAtStart := GUIGADGET_GetStatus(GUIHandle, PAnsiChar('AUTOPAUSE')) <> 0;
    GlobalDPlay.BroadcastExtraBattleRoomSettings(0);
  end;

  if GUIGADGET_WasPressed(GUIHandle, PAnsiChar('SPEEDLOCK')) then
  begin
    Result := True;
    GlobalDPlay.SpeedLockNative := GUIGADGET_GetStatus(GUIHandle, PAnsiChar('SPEEDLOCK')) <> 0;
    GlobalDPlay.BroadcastExtraBattleRoomSettings(0);
  end;

  if GUIGADGET_WasPressed(GUIHandle, PAnsiChar('AIDIFF')) then
  begin
    Result := True;
    GlobalDPlay.AIDifficulty := GUIGADGET_GetStatus(GUIHandle, PAnsiChar('AIDIFF'));
    GlobalDPlay.BroadcastExtraBattleRoomSettings(0);
  end;

  if GUIGADGET_WasPressed(GUIHandle, PAnsiChar('RANDMAP')) then
  begin
    Result := True;
    sMapName := GetRandomMapEx;
    if GlobalDPlay.Players.Count > 1 then
      GlobalDPlay.SendChat('The randomly selected map is: ' + sMapName)
    else
      SendTextLocal('The randomly selected map is: ' + sMapName);
  end;

  if Result then
    PlaySound_2D_Name(PAnsiChar('Multi'), 0);
end;

procedure BattleRoom_NewButtonsPress;
label
  ButtonHandled;
asm
  pushAD
  push    esi
  call    NewButtonsClicked
  test    eax, eax
  popAD
  jnz     ButtonHandled
  push    $505598
  push $00448679;
  call PatchNJump;
ButtonHandled:
  push $00448B98;
  call PatchNJump;
end;

procedure EnterBattleRoom; stdcall;
var
  Player: PPlayerStruct;
  Buffer: Integer;
begin
  if TAData.PlayersStructPtr <> nil then
  begin
    Player := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);
    if REGISTRY_ReadInteger(PAnsiChar('Total Annihilation'),
      PAnsiChar('RaceSide'), @Buffer) <> 0 then
    begin
      if TAData.MainStruct.RaceSideData[Buffer].Name <> #0 then
        Player.PlayerInfo.Raceside := Buffer;
    end;
    if IsLocalPlayerHost then
    begin
      if REGISTRY_ReadInteger(PAnsiChar('Total Annihilation'),
        PAnsiChar('MultiDifficulty'), @Buffer) <> 0 then
        GlobalDPlay.AIDifficulty := Buffer
      else
        GlobalDPlay.AIDifficulty := 1;
      GUIGADGET_SetStatus(@TAData.MainStruct.p_TAGUIObject,
          PAnsiChar('AIDIFF'), GlobalDPlay.AIDifficulty);
    end;
    GlobalDPlay.BroadcastModInfo;
  end;
end;

procedure BattleRoom_EnterBattleRoom;
asm
  pushAD
  call    EnterBattleRoom
  popAD
  add     edx, 519h
  push $0044A666;
  call PatchNJump;
end;

procedure HostBattleRoomToGame; stdcall;
begin
  TAData.MainStruct.lCurrentAIProfile := 2;
  if GlobalDPlay.AIDifficulty <> 0 then
  begin
    TAData.MainStruct.lCurrentAIProfile := GlobalDPlay.AIDifficulty + 1;
    REGISTRY_WriteInteger(PAnsiChar('Total Annihilation'),
      PAnsiChar('MultiDifficulty'), GlobalDPlay.AIDifficulty);
  end;
end;

procedure BattleRoom_BattleRoomHostToGame;
asm
  pushAD
  call    HostBattleRoomToGame
  popAD
  push $00448A15;
  call PatchNJump;
end;

procedure BattleRoomToGame; stdcall;
var
  Player: PPlayerStruct;
begin
  if TAData.GameingType = gtMultiplayer then
  begin
    if TAData.PlayersStructPtr <> nil then
    begin
      Player := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);
      if Player.PlayerInfo.PropertyMask and $40 <> $40 then
        REGISTRY_WriteInteger(PAnsiChar('Total Annihilation'),
          PAnsiChar('RaceSide'), Player.PlayerInfo.Raceside);
    end;
    TAData.MainStruct.lCurrentAIProfile := 2;
    if GlobalDPlay.AIDifficulty <> 0 then
      TAData.MainStruct.lCurrentAIProfile := GlobalDPlay.AIDifficulty + 1;
  end;
end;

procedure BattleRoom_BattleRoomToGame;
asm
  pushAD
  call    BattleRoomToGame
  popAD
  call    REGISTRY_SaveSettings
  push $00497FFB;
  call PatchNJump;
end;

procedure DisableHostButtons; stdcall;
var
  cGrayed: Byte;
  cStatus: Byte;
begin
  cStatus := 0;
  if IsLocalPlayerHost then
    cGrayed := 0
  else
    cGrayed := 1;
  GUIGADGET_SetGrayedOut(@TAData.MainStruct.p_TAGUIObject,
    PAnsiChar('AUTOPAUSE'), cGrayed);
  GUIGADGET_SetGrayedOut(@TAData.MainStruct.p_TAGUIObject,
    PAnsiChar('AIDIFF'), cGrayed);
  GUIGADGET_SetGrayedOut(@TAData.MainStruct.p_TAGUIObject,
    PAnsiChar('SPEEDLOCK'), cGrayed);
  GUIGADGET_SetGrayedOut(@TAData.MainStruct.p_TAGUIObject,
    PAnsiChar('RANDMAP'), cGrayed);
  if cGrayed = 1 then
  begin
    if GlobalDPlay.AutoPauseAtStart then
      cStatus := 1;
    GUIGADGET_SetStatus(@TAData.MainStruct.p_TAGUIObject,
      PAnsiChar('AUTOPAUSE'), cStatus);
    GUIGADGET_SetStatus(@TAData.MainStruct.p_TAGUIObject,
      PAnsiChar('AIDIFF'), GlobalDPlay.AIDifficulty);
    if GlobalDPlay.SpeedLockNative then
      cStatus := 1
    else
      cStatus := 0;
    GUIGADGET_SetStatus(@TAData.MainStruct.p_TAGUIObject,
      PAnsiChar('SPEEDLOCK'), cStatus);
  end;
end;

procedure BattleRoom_DisableHostButtons;
asm
  pushAD
  call    DisableHostButtons
  popAD
  call    GUIGADGET_SetStatus
  push $00446012;
  call PatchNJump;
end;

procedure DrawModVersionOverLogo(PlayerIdx: Integer; p_Offscreen: Pointer;
  const str: PAnsiChar; left: Integer; top: Integer; MaxLen: Integer;
  Background: Integer); stdcall;
var
  Player: PPlayerStruct;
  NewStr: String;
  StrExt: Integer;
begin
  Player := TAPlayer.GetPlayerByIndex(PlayerIdx);
  PlayerIdx := GlobalDPlay.Players.ConvertId(Player.lDirectPlayID, ZI_Everyone, False);
  if (GlobalDPlay.Players[PlayerIdx].ModInfo.ModID > 1) and
     (GlobalDPlay.Players[PlayerIdx].ModInfo.ModMajorVer <> '0') then
  begin
    // known mod and mod version
    NewStr := GlobalDPlay.Players[PlayerIdx].ModInfo.ModMajorVer + '.' +
              GlobalDPlay.Players[PlayerIdx].ModInfo.ModMinorVer;
  end else
  begin
    case GlobalDPlay.Players[PlayerIdx].ModInfo.ModID of
      -1   : NewStr := '';
      0, 1 : NewStr := Copy(GetTADemoVersion, 1, 3);
      else
      begin
        NewStr := '?';
        left := left + 6;
      end;
    end;
  end;
  StrExt := 0;
  if GetStrExtent(PAnsiChar(NewStr)) > 14 then
    StrExt := 1;
  DrawText(p_Offscreen, PAnsiChar(NewStr), left - StrExt, top, MaxLen + 2, Background);
end;

procedure BattleRoom_DrawModVersionOverLogo;
asm
  push    edi
  call    DrawModVersionOverLogo
  push $0044AF16;
  call PatchNJump;
end;

Procedure OnInstallBattleRoomEnhancements;
begin
end;

Procedure OnUninstallBattleRoomEnhancements;
begin
end;

function GetPlugin: TPluginData;
begin
  if IsTAVersion31 and State_BattleRoomEnhancements then
  begin
    Result := TPluginData.Create( False,
                                  '',
                                  State_BattleRoomEnhancements,
                                  @OnInstallBattleRoomEnhancements,
                                  @OnUnInstallBattleRoomEnhancements );

    Result.MakeRelativeJmp( State_BattleRoomEnhancements,
                            'Get more accurate installed RAM value (max 64GB)',
                            @BattleRoomEnhancements_GetMemory,
                            $004643B0, 0 );

    Result.MakeRelativeJmp( State_BattleRoomEnhancements,
                            'Handle new state of FIXEDLOC button',
                            @BattleRoom_FIXEDLOC,
                            $0044868F, 2 );

    Result.MakeRelativeJmp( State_BattleRoomEnhancements,
                            'Override state of GUI buttons that are used by engine and TADR',
                            @BattleRoom_OverrideButtonsState,
                            $0044AF2C, 1 );

    Result.MakeRelativeJmp( State_BattleRoomEnhancements,
                            'Handle click events for new buttons',
                            @BattleRoom_NewButtonsPress,
                            $00448674, 0 );

    Result.MakeRelativeJmp( State_BattleRoomEnhancements,
                            'Load extra battle room settings from registry',
                            @BattleRoom_EnterBattleRoom,
                            $0044A660, 1 );

    Result.MakeRelativeJmp( State_BattleRoomEnhancements,
                            'Set and save AI difficulty to registry [host only]',
                            @BattleRoom_BattleRoomHostToGame,
                            $00448A0B, 3 );

    Result.MakeRelativeJmp( State_BattleRoomEnhancements,
                            'Save race side to registry [players and host]',
                            @BattleRoom_BattleRoomToGame,
                            $00497FF6, 0 );

    Result.MakeRelativeJmp( State_BattleRoomEnhancements,
                            'Disable host buttons for non host player',
                            @BattleRoom_DisableHostButtons,
                            $0044600D, 0 );

    Result.MakeRelativeJmp( State_BattleRoomEnhancements,
                            'Draw game mod version instead of internal TA version',
                            @BattleRoom_DrawModVersionOverLogo,
                            $0044AF11, 0 );
  end else
    Result := nil;
end;

end.