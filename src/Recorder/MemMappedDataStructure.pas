unit MemMappedDataStructure;

interface

const
 MemMapName = 'TADemo-MKChat';
 TAHookName = 'GlobalMap';
 TAHookSize = 6084;

type TTACheats = type longint;
const
  Cheat_Invulnerability : TTACheats = 1;
  Cheat_Invisible : TTACheats = 2;
  Cheat_FastBuild : TTACheats = 4;
  Cheat_InfiniteResources : TTACheats = 8;
  Cheat_LosRadar : TTACheats = $10;
  Cheat_ControlMenu : TTACheats = $20;
  Cheat_BuildAnywhere : TTACheats = $40;
  Cheat_InstantCapture : TTACheats = $80;
  Cheat_SpecialMove : TTACheats = $100;
  Cheat_JamAll : TTACheats = $200;
  Cheat_ExtraDamage : TTACheats = $400;
  Cheat_InstantBuild : TTACheats = $800;
  Cheat_ResourcesFreeze : TTACheats = $1000;
  //Cheat_DeveloperMode: TTACheats = $2000;
  Cheat_DoubleShot : TTACheats = $2000;

type
  PMKChatMem = ^MKChatMem;
  MKChatMem = record
    chat             : array [1..100] of char;
    NewData          : longint;
    deathtimes       : array [1..10] of longint;
    tastatus         : longint;
    playernames      : array [1..10,1..20] of char;
    incomeM          : array [1..10] of single;
    incomeE          : array [1..10] of single;
    totalM           : array [1..10] of single;
    totalE           : array [1..10] of single;
    playingDemo      : longint;
    allies           : array [1..10] of longint;
    yehaplayground   : array [1..10] of longint;
    storedM          : array [1..10] of single;
    storedE          : array [1..10] of single;
    storageM         : array [1..10] of single;
    storageE         : array [1..10] of single;
    ehaWarning       : longint;
    ehaOff           : longint;
    toAllies         : array [1..100] of char;
    toAlliesLength   : longint;
    fromAllies       : array [1..100] of char;
    fromAlliesLength : longint;
    mapX             : longint;
    mapY             : longint;
    otherMapX        : array [1..10] of longint;
    otherMapY        : array [1..10] of longint;
    F1Disable        : longint;
    commanderWarp    : longint;
    mapname          : array [1..100] of char;
    myCheats         : TTACheats;
    playerColors     : array [1..10] of longint;
    lockviewon       : longint;
    unitCount        : longword;
    ta3d             : longint;
  end;

Procedure OpenMemMap( var hMemMap : Cardinal; var chatview : PMKChatMem);
Procedure CloseMemMap( var hMemMap : Cardinal; var chatview : PMKChatMem);
function IsTAHookRunning : Boolean;

function ListCheats( cheats : TTACheats ) : string;
implementation
uses windows, SysUtils;


function ListCheats( cheats : TTACheats ) : string;

  function CheatEnabled( a : TTACheats ) : Boolean;
  begin
  Result := (cheats and a) = a;
  end;

  procedure AddCheatDescription(const s : string);
  begin
  if Result <> '' then
    Result := s+', '+Result
  else
    Result := s;
  end;

begin
Result := '';
if CheatEnabled(Cheat_Invulnerability) then
  AddCheatDescription('invulnerability');
if CheatEnabled(Cheat_Invisible) then
  AddCheatDescription('invisible');
if CheatEnabled(Cheat_FastBuild) then
  AddCheatDescription('fast build');
if CheatEnabled(Cheat_InfiniteResources) then
  AddCheatDescription('infinite resources');
if CheatEnabled(Cheat_LosRadar) then
  AddCheatDescription('los&radar');
if CheatEnabled(Cheat_ControlMenu) then
  AddCheatDescription('control menu');
if CheatEnabled(Cheat_BuildAnywhere) then
  AddCheatDescription('build anywhere');
if CheatEnabled(Cheat_InstantCapture) then
  AddCheatDescription('instant capture');
if CheatEnabled(Cheat_SpecialMove) then
  AddCheatDescription('special move');
if CheatEnabled(Cheat_JamAll) then
  AddCheatDescription('jam all' );
if CheatEnabled(Cheat_ExtraDamage) then
  AddCheatDescription('extra damage');
if CheatEnabled(Cheat_InstantBuild) then
  AddCheatDescription('instant build');
if CheatEnabled(Cheat_ResourcesFreeze) then
  AddCheatDescription('resources freeze');
{if CheatEnabled(Cheat_DeveloperMode) then
  AddCheatDescription('developer mode');}
if CheatEnabled(Cheat_DoubleShot) then
  AddCheatDescription('double shot');

end;

function IsTAHookRunning : boolean;
var
  hMemMap :thandle;
begin
hmemmap := CreateFileMapping ($ffffffff, nil, PAGE_READWRITE, 0, TAHookSize, TAHookName);
result := GetLastError = ERROR_ALREADY_EXISTS;
if hmemmap <> 0 then
  CloseHAndle (hmemmap);
end; 

Procedure OpenMemMap( var hMemMap : Cardinal; var chatview : PMKChatMem);
begin
hMemMap := CreateFileMapping($ffffffff,nil,PAGE_READWRITE,0,sizeof(MKChatMem),MemMapName );
if hMemMap = 0 then
  RaiseLastOSError;
chatview := MapViewOfFile(hMemMap,FILE_MAP_ALL_ACCESS,0,0,sizeof(MKChatMem));
if chatview = nil then
  RaiseLastOSError;
end;

Procedure CloseMemMap( var hMemMap : Cardinal; var chatview : PMKChatMem);
begin
if chatview <> nil then
  begin
  UnmapViewOfFile(chatview);
  chatview := NIL;
  end;
if hMemMap <> 0 then
  begin
  CloseHandle(hMemMap);
  hMemMap:= 0;
  end;
end;

end.
