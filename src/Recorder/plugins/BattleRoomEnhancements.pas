unit BattleRoomEnhancements;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_BattleRoomEnhancements : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallBattleRoomEnhancements;
Procedure OnUninstallBattleRoomEnhancements;

// -----------------------------------------------------------------------------

procedure BattleRoomEnhancements_GetMemory;
procedure BattleRoom_FIXEDLOC;
procedure BattleRoom_OverrideButtonsHook;
procedure BattleRoom_NewButtonsPressHook;
procedure BattleRoom_EnterBattleRoomHook;
procedure BattleRoom_BattleRoomHostToGameHook;
procedure BattleRoom_BattleRoomToGameHook;
procedure BattleRoom_HostButtonsHook;
procedure BattleRoom_DrawModVersionOverLogo;
procedure BattleRoom_BroadcastModInfoHook;

implementation
uses
  Windows,
  Classes,
  IniOptions,
  SysUtils,
  PlayerDataU,
  idplay,
  TA_MemoryLocations,
  TA_MemoryStructures,
  TA_MemoryConstants,
  TADemoConsts,
  TA_FunctionsU;

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
  BattleRoomEnhancementsPlugin: TPluginData;
  
function GlobalMemoryStatusEx(var lpBuffer: TMemoryStatusEx): BOOL; stdcall; external kernel32;

Procedure OnInstallBattleRoomEnhancements;
begin
end;

Procedure OnUninstallBattleRoomEnhancements;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_BattleRoomEnhancements then
  begin
    BattleRoomEnhancementsPlugin := TPluginData.create( false,
                            '',
                            State_BattleRoomEnhancements,
                            @OnInstallBattleRoomEnhancements,
                            @OnUnInstallBattleRoomEnhancements );

    BattleRoomEnhancementsPlugin.MakeRelativeJmp( State_BattleRoomEnhancements,
                          '',
                          @BattleRoomEnhancements_GetMemory,
                          $004643B0, 0);
                            
    BattleRoomEnhancementsPlugin.MakeRelativeJmp( State_BattleRoomEnhancements,
                          'Handle new state of FIXEDLOC button',
                          @BattleRoom_FIXEDLOC,
                          $0044868F, 2);

    BattleRoomEnhancementsPlugin.MakeRelativeJmp( State_BattleRoomEnhancements,
                          'Override state of GUI buttons that are used by engine and TADR',
                          @BattleRoom_OverrideButtonsHook,
                          $0044AF2C, 1);

    BattleRoomEnhancementsPlugin.MakeRelativeJmp( State_BattleRoomEnhancements,
                          '',
                          @BattleRoom_NewButtonsPressHook,
                          $00448674, 0);

    BattleRoomEnhancementsPlugin.MakeRelativeJmp( State_BattleRoomEnhancements,
                          'Load settings',
                          @BattleRoom_EnterBattleRoomHook,
                          $0044A660, 1);

    BattleRoomEnhancementsPlugin.MakeRelativeJmp( State_BattleRoomEnhancements,
                          'Set and Save AI difficulty [host only]',
                          @BattleRoom_BattleRoomHostToGameHook,
                          $00448A0B, 3);

    BattleRoomEnhancementsPlugin.MakeRelativeJmp( State_BattleRoomEnhancements,
                          'Save Race side [players and host]',
                          @BattleRoom_BattleRoomToGameHook,
                          $00497FF6, 0);

    BattleRoomEnhancementsPlugin.MakeRelativeJmp( State_BattleRoomEnhancements,
                          'Disable host buttons only for players',
                          @BattleRoom_HostButtonsHook,
                          $0044600D, 0);

    BattleRoomEnhancementsPlugin.MakeRelativeJmp( State_BattleRoomEnhancements,
                          'Draw Mod Version instead of internal version',
                          @BattleRoom_DrawModVersionOverLogo,
                          $0044AF11, 0);
                            {
    BattleRoomEnhancementsPlugin.MakeRelativeJmp( State_BattleRoomEnhancements,
                          '',
                          @BattleRoom_BroadcastModInfoHook,
                          $00450FDC, 0);
                           }
    Result:= BattleRoomEnhancementsPlugin;
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

procedure BattleRoomEnhancements_GetMemory;
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


function GetRandomMapEx: String;
var
  st    :string;
  nr    :integer;
  tot   :integer;
  i     :integer;
  currchar :char;
  pos: Cardinal;
  mapname :string;
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

    if MapsList.Count = 0 then
      Exit;

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
    push    $505598 // 'FIXEDLOC'
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

procedure BattleRoom_OverrideButtons(GUI: Pointer); stdcall;
begin
  if GlobalDPlay.CommanderWarp = 1 then
    GUIGADGET_SetStatus(GUI, PAnsiChar($505598), 2);
end;

procedure BattleRoom_OverrideButtonsHook;
asm
    add     ecx, TTAdynmemStruct.p_TAGUIObject
    pushAD
    push    ecx
    call    BattleRoom_OverrideButtons
    popAD
    push $0044AF32;
    call PatchNJump;
end;

function BattleRoom_NewButtonsPress(GUIHandle: Pointer): LongBool; stdcall;
var
  sMapName : String;
  LocalPlayer, Player: PPlayerStruct;
  i: Integer;
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
    LocalPlayer := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);

    sub_435D30(nil, nil, PTADynMemStruct(TAData.MainStructPtr).p_MapOTAFile, 0);

    LoadMap(nil, nil,
            PTADynMemStruct(TAData.MainStructPtr).p_MapOTAFile,
            PAnsiChar(sMapName));

    LocalPlayer.PlayerInfo.lCRC_OTA :=
      CalculateOTACRC(nil, nil, PTADynMemStruct(TAData.MainStructPtr).p_MapOTAFile);
    PTADynMemStruct(TAData.MainStructPtr).p_MapOTAFile.pCurrentMapName :=
      @PTADynMemStruct(TAData.MainStructPtr).p_MapOTAFile.pMapName;

    FillChar(LocalPlayer.PlayerInfo.MapName, 32, 0);
    StrPLCopy(LocalPlayer.PlayerInfo.MapName, sMapName, 32);
    GUIGADGET_SetText(GUIHandle, PAnsiChar('MAPNAME'), @LocalPlayer.PlayerInfo.MapName, 0);

    LocalPlayer.PlayerInfo.lCRC_OTA :=
      CalculateOTACRC(nil, nil, PTADynMemStruct(TAData.MainStructPtr).p_MapOTAFile);

    Send_PacketPlayerInfo();
    REPORTER_PlayerInfo(5);
    UpdateGameInfo();

    for i:= 0 to 10 do
    begin
      Player := TAPlayer.GetPlayerByIndex(i);
      if (TAPlayer.PlayerController(Player) = Player_LocalHuman) or
         (TAPlayer.PlayerController(Player) = Player_LocalAI) then
        Player.PlayerInfo.PropertyMask := Player.PlayerInfo.PropertyMask and $FFDF;
    end;
  end;

  if Result then
    PlaySound_2D_Name(PAnsiChar('Multi'), 0);
end;

procedure BattleRoom_NewButtonsPressHook;
label
  ButtonHandled;
asm
    pushAD
    push    esi
    call    BattleRoom_NewButtonsPress
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

procedure BattleRoom_EnterBattleRoom; stdcall;
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
      if PTADynMemStruct(TAData.MainStructPtr).RaceSideData[Buffer].Name <> #0 then
        Player.PlayerInfo.Raceside := Buffer;
    end;

    if IsLocalPlayerHost then
    begin
      if REGISTRY_ReadInteger(PAnsiChar('Total Annihilation'),
        PAnsiChar('MultiDifficulty'), @Buffer) <> 0 then
        GlobalDPlay.AIDifficulty := Buffer
      else
        GlobalDPlay.AIDifficulty := 1;

      GUIGADGET_SetStatus(@PTADynMemStruct(TAData.MainStructPtr).p_TAGUIObject,
          PAnsiChar('AIDIFF'), GlobalDPlay.AIDifficulty);
    end;
    GlobalDplay.BroadcastModInfo;
  end;
end;

procedure BattleRoom_EnterBattleRoomHook;
asm
    pushAD
    call    BattleRoom_EnterBattleRoom
    popAD
    add     edx, 519h
    push $0044A666;
    call PatchNJump;
end;

procedure BattleRoom_HostBattleRoomToGame; stdcall;
begin
  PTADynMemStruct(TAData.MainStructPtr).lCurrentAIProfile := 2;
  if GlobalDPlay.AIDifficulty <> 0 then
  begin
    PTADynMemStruct(TAData.MainStructPtr).lCurrentAIProfile := GlobalDPlay.AIDifficulty + 1;
    REGISTRY_WriteInteger(PAnsiChar('Total Annihilation'),
      PAnsiChar('MultiDifficulty'), GlobalDPlay.AIDifficulty);
  end;
end;

procedure BattleRoom_BattleRoomHostToGameHook;
asm
    pushAD
    call    BattleRoom_HostBattleRoomToGame
    popAD
    push $00448A15;
    call PatchNJump;
end;

procedure BattleRoom_BattleRoomToGame; stdcall;
var
  Player: PPlayerStruct;
begin
  if TAData.PlayersStructPtr <> nil then
  begin
    Player := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);
    if Player.PlayerInfo.PropertyMask and $40 <> $40 then
      REGISTRY_WriteInteger(PAnsiChar('Total Annihilation'),
        PAnsiChar('RaceSide'), Player.PlayerInfo.Raceside);
  end;
  PTADynMemStruct(TAData.MainStructPtr).lCurrentAIProfile := 2;
  if GlobalDPlay.AIDifficulty <> 0 then
    PTADynMemStruct(TAData.MainStructPtr).lCurrentAIProfile := GlobalDPlay.AIDifficulty + 1;
end;

procedure BattleRoom_BattleRoomToGameHook;
asm
    pushAD
    call    BattleRoom_BattleRoomToGame
    popAD
    call    REGISTRY_SaveSettings
    push $00497FFB;
    call PatchNJump;
end;

procedure BattleRoom_HostButtons; stdcall;
var
  cGrayed : Byte;
  cStatus : Byte;
begin
  cStatus := 0;
  if IsLocalPlayerHost then
    cGrayed := 0
  else
    cGrayed := 1;

  GUIGADGET_SetGrayedOut(@PTADynMemStruct(TAData.MainStructPtr).p_TAGUIObject,
    PAnsiChar('AUTOPAUSE'), cGrayed);
  GUIGADGET_SetGrayedOut(@PTADynMemStruct(TAData.MainStructPtr).p_TAGUIObject,
    PAnsiChar('AIDIFF'), cGrayed);
  GUIGADGET_SetGrayedOut(@PTADynMemStruct(TAData.MainStructPtr).p_TAGUIObject,
    PAnsiChar('SPEEDLOCK'), cGrayed);
  GUIGADGET_SetGrayedOut(@PTADynMemStruct(TAData.MainStructPtr).p_TAGUIObject,
    PAnsiChar('RANDMAP'), cGrayed);

  if cGrayed = 1 then
  begin
    if GlobalDplay.AutoPauseAtStart then
      cStatus := 1;
    GUIGADGET_SetStatus(@PTADynMemStruct(TAData.MainStructPtr).p_TAGUIObject,
      PAnsiChar('AUTOPAUSE'), cStatus);
    GUIGADGET_SetStatus(@PTADynMemStruct(TAData.MainStructPtr).p_TAGUIObject,
      PAnsiChar('AIDIFF'), GlobalDPlay.AIDifficulty);
    if GlobalDplay.SpeedLockNative then
      cStatus := 1
    else
      cStatus := 0;
    GUIGADGET_SetStatus(@PTADynMemStruct(TAData.MainStructPtr).p_TAGUIObject,
      PAnsiChar('SPEEDLOCK'), cStatus);
  end;
end;

procedure BattleRoom_HostButtonsHook;
asm
    pushAD
    call    BattleRoom_HostButtons
    popAD
    call    GUIGADGET_SetStatus
    push $00446012;
    call PatchNJump;
end;

function DrawModVersionOverLogo(PlayerIdx: Integer; OFFSCREEN_ptr: Cardinal;
  const str: PAnsiChar; left: Integer; top: Integer; MaxLen: Integer;
  Background: Integer) : LongInt; stdcall;
var
  Player: PPlayerStruct;
  NewStr: String;
  StrExt: Integer;
begin
  Player := TAPlayer.GetPlayerByIndex(PlayerIdx);
  PlayerIdx := GlobalDPlay.Players.ConvertId(Player.lDirectPlayID,ZI_Everyone,false);

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
  Result := DrawText(OFFSCREEN_ptr, PAnsiChar(NewStr), left - StrExt, top, MaxLen + 2, Background);
end;

procedure BattleRoom_DrawModVersionOverLogo;
asm
    push    edi
    call    DrawModVersionOverLogo
    push $0044AF16;
    call PatchNJump;
end;
 
procedure BattleRoom_BroadcastModInfo(Player: PPlayerStruct); stdcall;
begin
  GlobalDplay.BroadcastModInfo;
end;

procedure BattleRoom_BroadcastModInfoHook;
asm
  pushAD
  push      ebp
  call      BattleRoom_BroadcastModInfo
  popAD
  mov       ecx, 2Eh
  push $00450FE1;
  call PatchNJump;
end;

end.
