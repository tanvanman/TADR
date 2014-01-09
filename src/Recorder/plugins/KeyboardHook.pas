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
  TA_MemoryLocations, TA_FunctionsU, BattleRoomScroll,
  COB_Extensions;

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
  t: longword;
begin
  if nCode < 1 then
  begin
    CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
  end else
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
                  InterpretInternalCommand(PChar('shootall'));
                  if PlayerInfo.ShootAll then
                    SendTextLocal('Toggled ShootAll to: ON')
                  else
                    SendTextLocal('Toggled ShootAll to: OFF');
                end;
              end;
        $44 : begin     // left alt + shift + d
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                if FindMouseUnit <> 0 then
                    UnitAtMouse := Pointer(TAMem.GetUnitStruct(FindMouseUnit));
                  if UnitAtMouse <> nil then
                  begin
                    //TAUnit.setUpgradeable( pointer(TAMem.GetUnitPtr(FindMouseUnit)), 1, nil);
                    t:= TAUnit.GetUnitTemplateId(UnitAtMouse);
                    SendTextLocal(inttostr(t));
                  end;
                end;
              end;
        $45 : begin     // left alt + shift + e
                if ( ((GetAsyncKeyState(VK_MENU) and $8000) > 0) and
                     ((GetAsyncKeyState(VK_SHIFT) and $8000) > 0) ) then
                begin
                  if FindMouseUnit <> 0 then
                    UnitAtMouse := Pointer(TAMem.GetUnitStruct(FindMouseUnit));
                  if UnitAtMouse <> nil then
                  begin
                    //t2:= TAUnit.GetCOBDataPtr(Pointer(TAMem.GetUnitPtr(UnitAtMouse)));
                    //sub_45A950(LongWord(TAMem.GetModelPtr(31)), t2, LongWord(TAMem.GetUnitPtr(UnitAtMouse)));
                    //sub_45A8D0(LongWord(TAMem.GetUnitPtr(UnitAtMouse)));
                    TAUnit.SetWeapon(UnitAtMouse, WEAPON1, 91, false);
                    TAUnit.SetWeapon(UnitAtMouse, WEAPON3, 22, false);
                    TAUnit.SetTemplate(UnitAtMouse, UnitName2ID(PChar('ARMCOM')), true);
                    TAUnit.SetUpgradeable(UnitAtMouse, 1, nil);
                    TAUnit.SetUnitInfoField(UnitAtMouse, SOUNDCTGR, 0, nil);
                    t:= TAUnit.GetUnitInfoField(UnitAtMouse, MAXHEALTH, nil);
                    TAUnit.SetUnitInfoField(UnitAtMouse, FENERGYMAKE, 50000, nil);
                    TAUnit.SetUnitInfoField(UnitAtMouse, FMETALMAKE, 20000, nil);

                    TAUnit.SetHealth(UnitAtMouse, t);

                    TAUnit.Speech(LongWord(UnitAtMouse), 12, PChar('It''s-a me, Mario!'));
                    PlaySound_EffectName(PChar('ItsMeMario'), LongWord(UnitAtMouse));
                    //TAUnit.SoundEffect(LongWord(UnitAtMouse), 40);

                    TAUnit.UpdateLos(UnitAtMouse);
                    //TA_UpdateUnitLOS(LongWord(UnitAtMouse));
                    //TA_UpdateLOS(TAUnit.GetOwnerIndex(UnitAtMouse), 1);
                  end;
                end;
              end;

        end;
     end else
     begin
       if keyboardHookLevel > 0 then
       begin
         case wParam of
           VK_PRIOR: ScrollBattleRoomTextWindowUp;
           VK_NEXT : ScrollBattleRoomTextWindowDown;
         end;
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
var
  curShareEnergyVal: single;
begin
  curShareEnergyVal:= PlayerInfo.ShareEnergyVal;
  if curShareEnergyVal > 0 then
  begin
    if not PlayerInfo.ShareEnergy then
      InterpretInternalCommand('shareenergy');
    lastShareEnergyVal:= curShareEnergyVal;
    InterpretInternalCommand('setshareenergy 0');
  end else
  begin
    InterpretInternalCommand('setshareenergy '+IntToStr(Round(lastShareEnergyVal)));
  end;
end;

end.