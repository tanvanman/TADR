unit KeyboardHook;
{

}
interface
uses
  PluginEngine, Windows, Messages, SysUtils;

// -----------------------------------------------------------------------------

const
  State_KeyboardHook : boolean = true;
  
function GetPlugin : TPluginData;
  
// -----------------------------------------------------------------------------

procedure OnInstallKeyboardHook;
procedure OnUninstallKeyboardHook;

var
  hKeyboardHook: HHook;
  keyboardHookLevel: byte;

function KeyboardHookFunction(nCode: Integer; wParam: Word; lParam: LongInt): LRESULT; stdcall;
procedure SetInGameHotkeysLevel;
procedure CtrlFDontSelectNotLabs;

// -----------------------------------------------------------------------------
// actions for keys

procedure SwitchSetShareEnergy;
procedure UpdateSelectUnitEffect;
procedure ApplySelectUnitMenu_Wrapper;
function FindIdleFactory : Boolean;

// -----------------------------------------------------------------------------

implementation
uses
  idplay, TA_MemoryLocations, TA_MemoryStructures,TA_MemoryConstants, TA_FunctionsU, //BattleRoomScroll,
  COB_Extensions, TAMemManipulations, SaveUnitsWeaponsList, DynamicMap, GUIEnhancements,
  UnitInfoExpand;

const
  LastNum : Cardinal = 0;

var
  lastShareEnergyVal: single;
  Semaphore_IdleFactory : THandle;

Procedure OnInstallKeyboardHook;
begin
  keyboardHookLevel := 2;
  lastShareEnergyVal := 0;
  hKeyboardHook:= SetWindowsHookEx(WH_KEYBOARD, @KeyboardHookFunction, 0, GetCurrentThreadId);
  Semaphore_IdleFactory := CreateSemaphore(nil, 1, 1, '');
end;

Procedure OnUninstallKeyboardHook;
begin
  CloseHandle(Semaphore_IdleFactory);
  if (hKeyboardHook <> 0) then
    UnhookWindowsHookEx(hKeyboardHook);
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_KeyboardHook then
  begin
    Result := TPluginData.create( true,
                                'Keyboard Hook',
                                State_KeyboardHook,
                                @OnInstallKeyboardHook, @OnUnInstallKeyboardHook );

    Result.MakeRelativeJmp( State_KeyboardHook,
                            '',
                            @CtrlFDontSelectNotLabs,
                            $0048DB72, 1);

    Result.MakeRelativeJmp( State_KeyboardHook,
                            'set ingame hotkeys level',
                            @SetInGameHotkeysLevel,
                            $00498362, 1);
  end else
    result := nil;
end;

procedure SetInGameHotkeysLevel;
asm
  mov     keyboardHookLevel, 2
  mov     ecx, [TADynMemStructPtr]
  push $0049836D;
  call PatchNJump;
end;

function KeyboardHookFunction(nCode: Integer; wParam: Word; lParam: LongInt): LRESULT; stdcall;
var
  UnitAtMouse: PUnitStruct;
begin
  if nCode < 1 then
    CallNextHookEx(hKeyboardHook, nCode, wParam, lParam)
  else
  begin
    if ((lParam and $80000000) = 0) then
    begin
      if keyboardHookLevel > 1 then
      begin
        case wParam of
        $5A : begin     // left alt + shift + z
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                  SwitchSetShareEnergy;
                  Result := 1;
                  Exit;
                end;
              end;
        $58 : begin     // left alt + shift + x
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                  InterpretInternalCommand(PChar('shareenergy'));
                  Result := 1;
                  Exit;
                end;
              end;
        $43 : begin     // left alt + shift + c
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                  InterpretInternalCommand(PChar('sharemetal'));
                  Result := 1;
                  Exit;
                end;
              end;
        $41 : begin     // left alt + shift + a
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                  PlayerInfo.ShootAll:= not PlayerInfo.ShootAll;
                  if PlayerInfo.ShootAll then
                    SendTextLocal('Toggled ShootAll to: ON')
                  else
                    SendTextLocal('Toggled ShootAll to: OFF');
                  Result := 1;
                  Exit;
                end;
              end;
        $53 : begin     // left alt + shift + s
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                  UnitAtMouse:= TAUnit.AtMouse;
                  if (UnitAtMouse <> nil) then
                    if TAUnit.IsOnThisComp(UnitAtMouse, False) then
                    begin
                      PUnitOrder(UnitAtMouse.p_UnitOrders).p_NextOrder_uos := nil;
                      PUnitStruct(UnitAtMouse).p_FutureOrder := nil;
                      Result := 1;
                      Exit;
                    end;
                end;
              end;
       $51 : begin     // left alt + shift + q
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                  if TAData.DevMode then
                  begin
                    UnitAtMouse := TAUnit.AtMouse;
                    if (UnitAtMouse <> nil) then
                    begin
                      ClearChat;
//                      PCardinal(Cardinal(TAData.MainStructPtr) + $391B9)^ := 1;
//                      PWord(Cardinal(TAData.MainStructPtr) + $391BD)^ := Word(PUnitStruct(UnitAtMouse).lUnitInGameIndex);
//                      Offscreen := PCardinal(Cardinal(TAData.MainStructPtr) + $37E1B)^;
//                      SendTextLocal(IntToStr(UnitBuilderProbe(Offscreen)));
//                      TAUnit.MakeDamage(UnitAtMouse, UnitAtMouse, dtUnknown1, 200);
                      SendTextLocal('ptr  : ' + IntToHex(Cardinal(UnitAtMouse), 8));
                      SendTextLocal('id  : ' + IntToHex(TAUnit.GetLongId(UnitAtMouse), 8));
                      SendTextLocal('pos : X:' + IntToStr(TAUnit.GetUnitX(UnitAtMouse)) + ' Z:' + IntToStr(TAUnit.GetUnitZ(UnitAtMouse)) + ' H:' + IntToStr(TAUnit.GetUnitY(UnitAtMouse)));
                      SendTextLocal('turn Z: ' + IntToStr(TAUnit.GetTurnZ(UnitAtMouse)));
                      SendTextLocal('unit type id  : ' + IntToStr(PUnitStruct(UnitAtMouse).nUnitInfoID));
                      SendTextLocal('transported by : ' + IntToStr(TAUnit.GetID(PUnitStruct(UnitAtMouse).p_TransporterUnit)));
                      SendTextLocal('transporting : ' + IntToStr(TAUnit.GetID(PUnitStruct(UnitAtMouse).p_TransportedUnit)));
                      SendTextLocal('prior unit : ' + IntToStr(TAUnit.GetID(PUnitStruct(UnitAtMouse).p_PriorUnit)));
                      SendTextLocal('unitstate  : ' + IntToHex(PUnitStruct(UnitAtMouse).lUnitStateMask, 8));
                      SendTextLocal('unitstatebas : ' + IntToHex(PUnitStruct(UnitAtMouse).nUnitStateMaskBas, 8));

                      SendTextLocal('sfx occupy  : ' + IntToHex(PUnitStruct(UnitAtMouse).lSfxOccupy, 8));
                      SendTextLocal('owner id       : ' + IntToStr(TAUnit.GetOwnerIndex(UnitAtMouse)));
                      if PUnitStruct(UnitAtMouse).UnitWeapons[0].p_Weapon <> nil then
                        if PWeaponDef(PUnitStruct(UnitAtMouse).UnitWeapons[0].p_Weapon).lWeaponTypeMask and (1 shl 28) = 1 shl 28 then
                        begin
                          PUnitStruct(UnitAtMouse).UnitWeapons[0].cStock := PUnitStruct(UnitAtMouse).UnitWeapons[0].cStock + 1;
                          SendTextLocal('Increased stockpile');
                        end;
                      SendTextLocal('');
                    end;
                  end;
                end;
              end;
        $57 : begin     // left alt + shift + w
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                  if TAData.DevMode then
                  begin
                    SaveUnitsWeaponsListToScriptorFile;
                    SendTextLocal('Saved current list of units and weapons to file');
                  end;
                end;
              end;
        $13 : begin     // pause button
                if TAData.NetworkLayerEnabled then
                  if globalDPlay.AutopauseAtStart and not globalDPlay.Players[TAData.LocalPlayerID+1].IsServer then
                  begin
                    Msg_Reminder(PAnsiChar('Only the host can unpause.' +#13#10+ 'You can also vote to go with .ready command'), 1);
                    Result := 1;
                    Exit;
                  end;
              end;
        $5D : begin     // MENU KEY
                InterpretCommand('showranges', 1);
              end;
        $45 : begin     // left alt + shift + e
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                  UnitAtMouse := TAUnit.AtMouse;
                  if (UnitAtMouse <> nil) then
                  begin
                    ORDERS_RemoveAllBuildQueues(UnitAtMouse, True);
                    UpdateIngameGUI(0);
                  end;
                end;
              end;
        $46 : begin     // ctrl + f
                if ( ((GetAsyncKeyState(VK_CONTROL) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) = 0) ) then
                begin
                  if FindIdleFactory then
                  begin
                    UpdateSelectUnitEffect;
                    ApplySelectUnitMenu_Wrapper;
                  end;
                  Result := 1;
                  Exit;
                end;
              end; {
        $47 : begin     // left alt + shift + g
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                end;
              end;   }
        end;
     end else
     begin
       if keyboardHookLevel > 0 then
       begin
         //case wParam of
         //  VK_PRIOR: ScrollBattleRoomTextWindowUp;
         //  VK_NEXT : ScrollBattleRoomTextWindowDown;
         //end;
       end; { hooklevel }
     end;
     end;
  end; { ncode }
  // don't block key
  Result:= 0;
end;

{
.text:0048DB6E 020 8B 4C 24 24                           mov     ecx, [esp+20h+arg_0]
.text:0048DB72 020 8B 86 AC 00 00 00                     mov     eax, [esi+0ACh]
.text:0048DB78 020 3B C1                                 cmp     eax, ecx
.text:0048DB7A 020 75 44                                 jnz     short loc_48DBC0
}
procedure CtrlFDontSelectNotLabs;
asm
    mov     eax, [esi+0ACh]
    push $0048DB78;
    call PatchNJump;
end;

{
Switch share energy to 0 or latest value
(also enable sharing if it's disabled)
}
procedure SwitchSetShareEnergy;
begin
  if not PlayerInfo.ShareEnergy then
    InterpretInternalCommand('shareenergy');
  if PlayerInfo.ShareEnergyVal > 0 then
  begin
    lastShareEnergyVal:= PlayerInfo.ShareEnergyVal;
    InterpretInternalCommand('setshareenergy 0');
  end else
    InterpretInternalCommand('setshareenergy '+IntToStr(Round(lastShareEnergyVal)));
end;

procedure UpdateSelectUnitEffect;
begin
	PTAdynmemStruct(TAData.MainStructPtr).DesktopGUIState := PTAdynmemStruct(TAData.MainStructPtr).DesktopGUIState or $10;
	PTAdynmemStruct(TAData.MainStructPtr).ShowRangeUnitIndex := 0;
end;

procedure ApplySelectUnitMenu_Wrapper;
var
  old : Byte;
begin
	old := PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType;
  ApplySelectUnitMenu;
  PTAdynmemStruct(TAData.MainStructPtr).ucPrepareOrderType := old;
end;

procedure ScrollCenterView(X, Z : Integer; Smooth: Boolean);
var
  MaxX, MaxZ : Integer;
begin
	X := X - (PTAdynmemStruct(TAData.MainStructPtr).ScreenWidth-128) div 2;
	Z := Z - (PTAdynmemStruct(TAData.MainStructPtr).ScreenHeight-64) div 2;

	if X < 0 then
		X := 0
  else begin
    MaxX := PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.lRadarPictureWidth - ((PTAdynmemStruct(TAData.MainStructPtr).ScreenWidth-128));
    if (X > MaxX) then
      X := MaxX;
  end;

	if Z < 0 then
		Z := 0
  else begin
    MaxZ := PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct.lRadarPictureHeight - ((PTAdynmemStruct(TAData.MainStructPtr).ScreenHeight-64));
    if (Z > MaxZ) then
      Z := MaxZ;
  end;

  ScrollView(X, Z, Smooth);
end;

function FindIdleFactory : Boolean;
label
  Retry;
var
  WaitResult : Cardinal;
  j : Cardinal;
  PlayerMaxUnitID : Cardinal;
  CurrentUnit : PUnitStruct;
  Player : PPlayerStruct;
begin
  Result := False;
  WaitResult := WaitForSingleObject(Semaphore_IdleFactory, INFINITE);
  if WaitResult = WAIT_FAILED then
    Exit;
  if WaitResult = WAIT_TIMEOUT then
  begin
    ReleaseSemaphore(Semaphore_IdleFactory, 1, nil);
    Exit;
  end;

Retry:
  j := LastNum;
  Player := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);
  PlayerMaxUnitID := Player.nNumUnits;
  CurrentUnit := TAData.UnitsPtr;
  
  while (j<=PlayerMaxUnitID) do
  begin
    CurrentUnit := Pointer(Cardinal(Player.p_UnitsArray) + j * SizeOf(TUnitStruct));
    if ((CurrentUnit.lUnitStateMask and UnitSelectState[UnitValid2_State]) = UnitSelectState[UnitValid2_State]) then
    begin
      if CurrentUnit.lBuildTimeLeft = 0.0 then
        if CurrentUnit.p_Owner <> nil then
        begin
          if CurrentUnit.p_UnitDef <> nil then
          begin
            if PUnitInfo(CurrentUnit.p_UnitDef).cBMCode = 0 then
              if TAUnit.GetUnitInfoField(CurrentUnit, UNITINFO_BUILDER, nil) <> 0 then
              begin
                if GetUnitInfoProperty(CurrentUnit, 5) <> 0 then
                begin
                  Inc(j);
                  Continue;
                end else
                begin
                  if CurrentUnit.p_UnitOrders <> nil then
                    if PUnitOrder(CurrentUnit.p_UnitOrders).cOrderType = $C then
                    begin
                      Inc(j);
                      Continue;
                    end;
                end;
                if (LastNum<j) then
                begin
   					      LastNum := j;
                  Break;
                end;
              end;
          end;
        end;
    end;
    Inc(j);
  end;
	if (j<=PlayerMaxUnitID) then
  begin
    DeselectAllUnits;
		CurrentUnit.lUnitStateMask := CurrentUnit.lUnitStateMask or UnitSelectState[UnitSelected_State];
    ScrollCenterView(CurrentUnit.Position.X, CurrentUnit.Position.Z, True);
    Result := True;
	end else
  begin
		if (LastNum<>0) then
		begin
			LastNum := 0;
			goto Retry;
		end;
  end;
  ReleaseSemaphore(Semaphore_IdleFactory, 1, nil);
end;

end.