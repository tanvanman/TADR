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

// -----------------------------------------------------------------------------
// actions for keys

procedure SwitchSetShareEnergy;

// -----------------------------------------------------------------------------

implementation
uses
  TA_MemoryLocations, TA_MemoryStructures, TA_FunctionsU, //BattleRoomScroll,
  COB_Extensions, AimPrimary;

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

  result := TPluginData.create( true,
                                'Keyboard Hook',
                                State_KeyboardHook,
                                @OnInstallKeyboardHook, @OnUnInstallKeyboardHook );

end
else
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
                  SwitchSetShareEnergy;
              end;
        $58 : begin     // left alt + shift + x
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                 InterpretInternalCommand(PChar('shareenergy'));
              end;
        $43 : begin     // left alt + shift + c
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                  InterpretInternalCommand(PChar('sharemetal'));
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
                    end;
                end;
              end;
     {   $44 : begin     // left alt + shift + d
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                  UnitAtMouse:= TAUnit.AtMouse;
                  UnitSt:= UnitAtMouse;
                  if (UnitAtMouse <> nil) then
                  begin
                    SendTextLocal('ptr: ' + IntToHex(LongWord(UnitAtMouse), 8));
//                    SendTextLocal(IntToHex(LongWord(PUnitStruct(UnitAtMouse).lUnitInGameIndex), 8));
                    SendTextLocal('par1: ' + IntToStr(TaUnit.GetCurrentOrderParams(UnitAtMouse, 0)));
                    SendTextLocal('par2: ' + IntToStr(TaUnit.GetCurrentOrderParams(UnitAtMouse, 1)));

                  end;
                end;
              end;
       $51 : begin     // left alt + shift + q
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                end;
              end;
        $57 : begin     // left alt + shift + w
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                end;
              end;
        $45 : begin     // left alt + shift + e
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                end;
              end;
        $46 : begin     // left alt + shift + f
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
Switch share energy to 0 or latest value
(also enable sharing if it's disabled)
}
procedure SwitchSetShareEnergy;
begin
  if PlayerInfo.ShareEnergyVal > 0 then
  begin
    if not PlayerInfo.ShareEnergy then
      InterpretInternalCommand('shareenergy');
    lastShareEnergyVal:= PlayerInfo.ShareEnergyVal;
    InterpretInternalCommand('setshareenergy 0');
  end else
  begin
    InterpretInternalCommand('setshareenergy '+IntToStr(Round(lastShareEnergyVal)));
  end;
end;

end.