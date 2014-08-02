unit KeyboardHook;
{

}
interface
uses
  PluginEngine, Windows, SysUtils;

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
  lastShareEnergyVal: single;

function KeyboardHookFunction(nCode: Integer; wParam: Word; lParam: LongInt): LRESULT; stdcall;
procedure CtrlFDontSelectNotLabs;

// -----------------------------------------------------------------------------
// actions for keys

procedure SwitchSetShareEnergy;

// -----------------------------------------------------------------------------

implementation
uses
  TA_MemoryLocations, TA_MemoryStructures, TA_FunctionsU, //BattleRoomScroll,
  COB_Extensions, TAMemManipulations, DropUnitsWeaponsList;

Procedure OnInstallKeyboardHook;
begin
  keyboardHookLevel:= 2;
  lastShareEnergyVal:= 0;
  hKeyboardHook:= SetWindowsHookEx(WH_KEYBOARD, @KeyboardHookFunction, 0, GetCurrentThreadId());
end;

Procedure OnUninstallKeyboardHook;
begin
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
  end else
    result := nil;
end;

function KeyboardHookFunction(nCode: Integer; wParam: Word; lParam: LongInt): LRESULT; stdcall;
var
  UnitAtMouse: Pointer;
  UnitSt: PUnitStruct;
  //PlayerSt: PPlayerStruct;
//  FoundArray: TFoundUnits;
//  i, FoundCount: Integer;
//  tadynm: PTAdynmemStruct;
 // res: integer;
  //ResultUnit: Pointer;
  //ScreenshotCreate: TScreenshotCreate;
begin
  if nCode < 1 then
    CallNextHookEx(hKeyboardHook, nCode, wParam, lParam)
  else
  begin
    // dont continue if key isn't pressed (i.e. don't continue on repeat, keyup, etc)
    if (lParam and $C0000000) = 0 then
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
                  UnitSt:= UnitAtMouse;
                  if (UnitAtMouse <> nil) then
                    if TAUnit.IsOnThisComp(UnitAtMouse, False) then
                    begin
                      PUnitOrder(UnitSt.p_UnitOrders).p_NextOrder_uos := nil;
                      PUnitStruct(UnitSt).p_FutureOrder := nil;
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
      {  $70 : begin     // mouse button 4
                if TAData.DevMode then
                begin
                  if ((GetAsyncKeyState($5) and $8000) > 0) then
                  begin
                    UnitAtMouse := TAUnit.AtMouse;
                    if (UnitAtMouse <> nil) then
                    begin
                      PTAdynmemStruct(TAData.MainStructPtr)^.field_391B3 := 1;
                      PTAdynmemStruct(TAData.MainStructPtr)^.field_391B7 := PTAdynmemStruct(TAData.MainStructPtr)^.unMouseOverUnit;
                    end else
                    begin
                      PTAdynmemStruct(TAData.MainStructPtr)^.field_391B3 := 0;
                    end;
                    Result := 1;
                    Exit;
                  end;
                  if ((GetAsyncKeyState($6) and $8000) > 0) then
                  begin
                    UnitAtMouse := TAUnit.AtMouse;
                    if (UnitAtMouse <> nil) then
                    begin
                      PTAdynmemStruct(TAData.MainStructPtr)^.field_391B9 := 1;
                      PTAdynmemStruct(TAData.MainStructPtr)^.field_391BD := PTAdynmemStruct(TAData.MainStructPtr)^.unMouseOverUnit;
                    end else
                    begin
                      PTAdynmemStruct(TAData.MainStructPtr)^.field_391B9 := 0;
                    end;
                    Result := 1;
                    Exit;
                  end;
                end;
              end; }
{        $45 : begin     // left alt + shift + e
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                  ScreenshotCreate:= TScreenshotCreate.Create(False);
                  ScreenshotCreate.FreeOnTerminate:= True;
                end;
              end;     }
{        $46 : begin     // left alt + shift + f
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                end;
              end;
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

end.