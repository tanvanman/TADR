unit ClockPosition;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_ClockPosition : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallClockPosition;
Procedure OnUninstallClockPosition;

// -----------------------------------------------------------------------------

procedure ClockMove_LeftBottom;
procedure ClockMove_RightBottom;
procedure ClockMove_LeftTop;
procedure ClockMove_RightTop;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_FunctionsU;

const
  OFFSCREEN_ptr = -$1F0;

var
  ClockPositionPlugin: TPluginData;

Procedure OnInstallClockPosition;
begin
end;

Procedure OnUninstallClockPosition;
begin
end;

function GetPlugin : TPluginData;
var
  ShortClockStr : PAnsiChar;
  GameTimeStr : PAnsiChar;
  AShortClock : Cardinal;
  AGameTime : Cardinal;
begin
  if IsTAVersion31 and State_ClockPosition then
  begin
    ShortClockStr := PAnsiChar('%s%02d:%02d:%02d');
    GameTimeStr := PAnsiChar('');
    AShortClock := LongWord(ShortClockStr);
    AGameTime := LongWord(GameTimeStr);

    ClockPositionPlugin := TPluginData.create( True,
                            'Clock position',
                            State_ClockPosition,
                            @OnInstallClockPosition,
                            @OnUninstallClockPosition );

    if IniSettings.ClockPosition <> 0 then
    begin
      ClockPositionPlugin.MakeReplacement(State_ClockPosition,
                          'ClockPos replace format string',
                          $0046A254,
                          AShortClock, 4);

      ClockPositionPlugin.MakeReplacement(State_ClockPosition,
                          'ClockPos replace gametime string',
                          $0046A242,
                          AGameTime, 4);

      case IniSettings.ClockPosition of
        1 : begin
              ClockPositionPlugin.MakeRelativeJmp(State_ClockPosition,
                              'Clock left bottom position',
                              @ClockMove_LeftBottom,
                              $0046A292, 0);
            end;
        2 : begin
              ClockPositionPlugin.MakeRelativeJmp(State_ClockPosition,
                              'Clock right bottom position',
                              @ClockMove_RightBottom,
                              $0046A292, 0);
            end;
        3 : begin
              ClockPositionPlugin.MakeRelativeJmp(State_ClockPosition,
                              'Clock left top position',
                              @ClockMove_LeftTop,
                              $0046A292, 0);
            end;
        4 : begin
              ClockPositionPlugin.MakeRelativeJmp(State_ClockPosition,
                              'Clock right top position',
                              @ClockMove_RightTop,
                              $0046A292, 0);
            end;
      end;
    end;

    Result:= ClockPositionPlugin;
  end else
    Result := nil;
end;

procedure ClockMove_LeftBottom;
asm
  push    esi
  push    132
  lea     edx, [esp+230h+OFFSCREEN_ptr]
  push    ecx
  push    edx
  call    DrawText_Heavy
  push $0046A2A3;
  call PatchNJump;
end;

procedure ClockMove_RightBottom;
asm
  push    esi
  mov     edx, dword ptr[$0051FBD0]
  mov     esi, [edx+$0D4]
  sub     esi, 50
  push    esi
  lea     edx, [esp+230h+OFFSCREEN_ptr]
  push    ecx
  push    edx
  call    DrawText_Heavy
  push $0046A2A3;
  call PatchNJump;
end;

procedure ClockMove_LeftTop;
asm
  push    36
  push    132
  lea     edx, [esp+230h+OFFSCREEN_ptr]
  push    ecx
  push    edx
  call    DrawText_Heavy
  push $0046A2A3;
  call PatchNJump;
end;

procedure ClockMove_RightTop;
asm
  push    36
  mov     edx, dword ptr[$0051FBD0]
  mov     esi, [edx+$0D4]
  sub     esi, 50
  push    esi
  lea     edx, [esp+230h+OFFSCREEN_ptr]
  push    ecx
  push    edx
  call    DrawText_Heavy
  push $0046A2A3;
  call PatchNJump;
end;

end.
