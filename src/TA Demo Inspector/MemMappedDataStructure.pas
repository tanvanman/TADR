unit MemMappedDataStructure;

interface

const
 MemMapName = 'TADemo-MKChat';

const
  // ddraw.dll is base 1(globally shared data), dplayx.dll is base 0. Which retard is responcible for that?!?
  offset = 0;

  TALobby = 1 -offset;
  TALoading = 2 -offset;
  TAInGame = 3 -offset;

type
  MKChatMem = record
    chat           :array [1..100] of char;
    dataexists     :integer;
    DeathTimes     :array [1..10] of integer;
    tastatus       :integer;
    PlayerNames    :array [1..10,1..20] of char;
    IncomeMetal    :array [1..10] of single;
    IncomeEnergy   :array [1..10] of single;
    TotalMetal     :array [1..10] of single;
    TotalEnergy    :array [1..10] of single;
    PlayingDemo    :integer;
    allies         :array [1..10] of integer;
    yehaplayground :array [1..10] of integer;
    storedM        :array [1..10] of single;
    storedE        :array [1..10] of single;
    storageM       :array [1..10] of single;
    storageE       :array [1..10] of single;
    ehaWarning     :integer;
    ehaOff         :integer;
    toAllies       :array [1..100] of char;
    toAlliesLength :integer;
    fromAllies     :array [1..100] of char;
    fromAlliesLength:integer;
    mapX           :integer;
    mapY           :integer;
    otherMapX      :array [1..10] of integer;
    otherMapY      :array [1..10] of integer;
    F1Disable      :integer;
    commanderWarp  :integer;
    mapname        :array [1..100] of char;
    myCheats       : integer;
    playerColors   : array [1..10] of integer;
    lockviewon     : integer;
    unitCount      : integer;
    ta3d           :integer;
  end;

type
 PMKChatMem = ^MKChatMem;

Procedure CreateMemMap( var hMemMap : Cardinal; var chatview : PMKChatMem);
Procedure OpenMemMap( var hMemMap : Cardinal; var chatview : PMKChatMem);
Procedure CloseMemMap( var hMemMap : Cardinal; var chatview : PMKChatMem);



implementation
uses windows;

Procedure CreateMemMap( var hMemMap : Cardinal; var chatview : PMKChatMem);
begin
hMemMap := CreateFileMapping($ffffffff,nil,PAGE_READWRITE,0,sizeof(MKChatMem),MemMapName );
chatview := MapViewOfFile(hMemMap,FILE_MAP_ALL_ACCESS,0,0,sizeof(MKChatMem));
end;

Procedure OpenMemMap( var hMemMap : Cardinal; var chatview : PMKChatMem);
begin
hMemMap := OpenFileMapping(FILE_MAP_WRITE , false,MemMapName );
if hMemMap <> 0 then
  chatview := MapViewOfFile(hMemMap,FILE_MAP_ALL_ACCESS,0,0,sizeof(MKChatMem))
else
  chatview := nil;
end;

Procedure CloseMemMap( var hMemMap : Cardinal; var chatview : PMKChatMem);
begin
UnmapViewOfFile( chatview );
chatview := nil;
CloseHandle( hMemMap );
hMemMap := 0;
end;


end.
