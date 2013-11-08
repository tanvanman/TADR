unit BattleRoomScroll;
{
Based on:
Prognosis TA4 Battle Room Chat Scroller
http://www.tauniverse.com/forum/showpost.php?p=717847&postcount=442
Automatic scrolling back when ne message arrives has to be fixed
OFFSET_NUM_TEXTLINES = (((curOFFSET_TEXTLINESTARTINDEX + nUpClicks) modulo 30 ) * 65537 ) + MAX_DISPLAYABLE_LINES
}
interface
uses
  PluginEngine, Windows;

// -----------------------------------------------------------------------------

const
  State_BattleRoomScroll : boolean = true;
  
function GetPlugin : TPluginData;
  
// -----------------------------------------------------------------------------

procedure OnInstallBattleRoomScroll;
procedure OnUninstallBattleRoomScroll;

const
  OFFSET_NUM_TEXTLINES = $2A3E;
  OFFSET_TEXTLINESTARTINDEX	= $2A40;
  OFFSET_ADDR_FIRSTVISIBLELINE = $511A84;
  //gnome's mod allows 10 lines of text
  MAX_DISPLAYABLE_LINES = $0A;
  //TAMainStruct.textlines
  OFFSET_NUM_TEXTLINES2 = $37F27;

var
  nUpClicks: Word;

procedure ScrollBattleRoomTextWindowUp;
procedure ScrollBattleRoomTextWindowDown;

// -----------------------------------------------------------------------------

// asm block
procedure BattleRoomTextReceived;
procedure BattleRoomScrollBackAfterTextReceived;

implementation
uses
  TA_MemoryLocations;

Procedure OnInstallBattleRoomScroll;
begin
end;

Procedure OnUninstallBattleRoomScroll;
begin
end;

function GetPlugin : TPluginData;
begin
if IsTAVersion31 and State_BattleRoomScroll then
  begin

  result := TPluginData.create( true,
                                'Battle room scroll',
                                State_BattleRoomScroll,
                                @OnInstallBattleRoomScroll, @OnUnInstallBattleRoomScroll );

  result.MakeRelativeJmp( State_BattleRoomScroll,
                          'Battle room scroll new text',
                          @BattleRoomTextReceived,
                          $463CB0,
                          $1 ); 

  result.MakeRelativeJmp( State_BattleRoomScroll,
                          'Battle room scroll back',
                          @BattleRoomScrollBackAfterTextReceived,
                          $463DCF,
                          $1 );  
end
else
  result := nil;
end;

procedure BattleRoomTextReceived;
asm
  push eax

  mov ecx, dword ptr [TAdynmemStructPtr]
  mov ax, nUpClicks
  add word ptr [ecx + OFFSET_TEXTLINESTARTINDEX], ax
  add word ptr [ecx + OFFSET_NUM_TEXTLINES], ax
  mov ebp, dword ptr [ecx + OFFSET_NUM_TEXTLINES2]

  pop eax
  push $463CB6;
  call PatchNJump;
end;

procedure BattleRoomScrollBackAfterTextReceived;
asm
  push ecx

  mov eax, dword ptr [TAdynmemStructPtr]
  inc word ptr [eax + OFFSET_NUM_TEXTLINES]
  mov cx, nUpClicks
  sub word ptr [eax + OFFSET_TEXTLINESTARTINDEX], cx
  sub word ptr [eax + OFFSET_NUM_TEXTLINES], cx

  pop ecx
  push $463DD6;
  call PatchNJump;
end;

procedure ScrollBattleRoomTextWindowUp;
begin
  if PLongWord(Plongword(TAdynmemStructPtr)^+OFFSET_NUM_TEXTLINES)^ > MAX_DISPLAYABLE_LINES then
  begin
    Dec(PLongWord(PLongWord(TAdynmemStructPtr)^+OFFSET_TEXTLINESTARTINDEX)^);
    Dec(PLongWord(PLongWord(TAdynmemStructPtr)^+OFFSET_NUM_TEXTLINES)^);
    Inc(nUpClicks);
  end;
end;

procedure ScrollBattleRoomTextWindowDown;
begin
  if nUpClicks > 0 then
  begin
    Inc(PLongWord(PLongWord(TAdynmemStructPtr)^+OFFSET_TEXTLINESTARTINDEX)^);
    Inc(PLongWord(PLongWord(TAdynmemStructPtr)^+OFFSET_NUM_TEXTLINES)^);
    Dec(nUpClicks);
  end;
end;

end.
