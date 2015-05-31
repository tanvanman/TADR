unit SkirmishEnhancements;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_SkirmishEnhancements: Boolean = True;

function GetPlugin: TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallSkirmishEnhancements;
Procedure OnUninstallSkirmishEnhancements;

// -----------------------------------------------------------------------------

implementation
uses
  SysUtils,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_FunctionsU,
  TA_MemoryLocations;

procedure SkirmishPlayerLost(p_Player: PPlayerStruct); stdcall;

  function TitleCase(const s: String): String;
  var
    i: integer;
  begin
    if s = '' then
      Result := ''
    else
    begin
      Result := Uppercase(s[1]);
      for i := 2 to Length(s) do
        if s[i - 1] = ' ' then
          Result := Result + Uppercase(s[i])
        else
          Result := Result + Lowercase(s[i]);
    end;
  end;

var
  SideName: String;
  SideData: PRaceSideData;
  DeadMessageIdx: Integer;
  DeadMessage: String;
begin
  SideData := TAMem.RaceSideId2Data(p_Player.PlayerInfo.Raceside);
  if SideData <> nil then
    SideName := TitleCase(SideData.Name)
  else
    SideName := p_Player.szName;
  DeadMessageIdx := rand2;
  DeadMessage := Format('%s %s',
    [SideName, TranslateString(Pointer(PCardinal($507B88 + ((DeadMessageIdx mod 3) * 4))^))]);
  NewChatText(PAnsiChar(DeadMessage), 4, 0, p_Player.cPlayerIndex);
end;

Procedure OnInstallSkirmishEnhancements;
begin
end;

Procedure OnUninstallSkirmishEnhancements;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_SkirmishEnhancements then
  begin
    Result := TPluginData.Create( False, '',
                                  State_SkirmishEnhancements,
                                  @OnInstallSkirmishEnhancements,
                                  @OnUnInstallSkirmishEnhancements );

    Result.MakeStaticCall( State_SkirmishEnhancements,
                           'When skirmish AI player die, use correct side name',
                           @SkirmishPlayerLost,
                           $00486E54 );
  end else
    Result := nil;
end;

end.
