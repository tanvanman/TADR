unit Colors;
{
  Memory locations provided by Admiral_94
  Based on his topic: http://www.tauniverse.com/forum/showthread.php?t=43867
}
interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

type
  TColorEntry = record
    sName : String;
    lOffset : Cardinal;
    cSwapType : Byte;
    cDefaultVal : Byte;
  end;

const
  ColorsArray : array[0..30] of TColorEntry = (
    (sName: 'UnitSelectionBox'; lOffset: $00467A70; cSwapType: 2; cDefaultVal: 10;),
    (sName: 'BuildQueueBoxSelected1'; lOffset: $00438D63; cSwapType: 2; cDefaultVal: 10;),
    (sName: 'BuildQueueBoxSelected2'; lOffset: $00438D5D; cSwapType: 2; cDefaultVal: 3;),
    (sName: 'BuildQueueBoxNonSelected1'; lOffset: $00438D79; cSwapType: 2; cDefaultVal: 9;),
    (sName: 'BuildQueueBoxNonSelected2'; lOffset: $00438D73; cSwapType: 2; cDefaultVal: 1;),
    (sName: 'LoadBarsTexturesReady'; lOffset: $0049876B; cSwapType: 1; cDefaultVal: 10;),
    (sName: 'LoadBarsTexturesLoading'; lOffset: $00498768; cSwapType: 1; cDefaultVal: 2;),
    (sName: 'LoadBarsTerrainReady'; lOffset: $00498848; cSwapType: 1; cDefaultVal: 10;),
    (sName: 'LoadBarsTexturesLoading'; lOffset: $00498845; cSwapType: 1; cDefaultVal: 2;),
    (sName: 'LoadBarsUnitsReady'; lOffset: $0049891C; cSwapType: 1; cDefaultVal: 10;),
    (sName: 'LoadBarsUnitsLoading'; lOffset: $00498919; cSwapType: 1; cDefaultVal: 2;),
    (sName: 'LoadBarsAnimationsReady'; lOffset: $004989F0; cSwapType: 1; cDefaultVal: 10;),
    (sName: 'LoadBarsAnimationsLoading'; lOffset: $004989ED; cSwapType: 1; cDefaultVal: 2;),
    (sName: 'LoadBars3DDataReady'; lOffset: $00498AC4; cSwapType: 1; cDefaultVal: 10;),
    (sName: 'LoadBars3DDataLoading'; lOffset: $00498AC1; cSwapType: 1; cDefaultVal: 2;),
    (sName: 'LoadBarsExplosionsReady'; lOffset: $00498BBD; cSwapType: 1; cDefaultVal: 10;),
    (sName: 'LoadBarsExplosionsLoading'; lOffset: $00498BBA; cSwapType: 1; cDefaultVal: 2;),
    (sName: 'NanolatheParticleBase'; lOffset: $00473F3D; cSwapType: 1; cDefaultVal: 161;),
    (sName: 'NanolatheParticleColors'; lOffset: $004739D6; cSwapType: 1; cDefaultVal: 7;),
    (sName: 'UnderConstructSurfaceLo'; lOffset: $00458E6C; cSwapType: 1; cDefaultVal: 160;),
    (sName: 'UnderConstructSurfaceHi'; lOffset: $00458E5F; cSwapType: 1; cDefaultVal: 175;),
    (sName: 'UnderConstructOutlineLo'; lOffset: $00458E88; cSwapType: 1; cDefaultVal: 160;),
    (sName: 'UnderConstructOutlineHi'; lOffset: $00458E7B; cSwapType: 1; cDefaultVal: 175;),
    (sName: 'UnitHealthBarGood'; lOffset: 0; cSwapType: 0; cDefaultVal: 10;),
    (sName: 'UnitHealthBarMedium '; lOffset: 0; cSwapType: 0; cDefaultVal: 14;),
    (sName: 'UnitHealthBarLow'; lOffset: 0; cSwapType: 0; cDefaultVal: 12;),
    (sName: 'WeaponReloadBar'; lOffset: 0; cSwapType: 0; cDefaultVal: 143;),
    (sName: 'ReclaimBar'; lOffset: 0; cSwapType: 0; cDefaultVal: 17;),
    (sName: 'StockpileBar'; lOffset: 0; cSwapType: 0; cDefaultVal: 14;),
    (sName: 'MainMenuDots'; lOffset: $00425C56; cSwapType: 0; cDefaultVal: 0;),
    (sName: 'MainMenuDotsDisabled'; lOffset: $00426440; cSwapType: 0; cDefaultVal: 0;));

const
  State_Colors: Boolean = True;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallColors;
Procedure OnUninstallColors;

// -----------------------------------------------------------------------------

Const
  FIRST_GUIPAL_COLOR = $0DCB;

// -----------------------------------------------------------------------------

function GetRaceSpecificColor(ColorNr: Byte) : Byte;

implementation
uses
  Windows,
  SysUtils,
  IniOptions,
  TA_MemoryLocations,
  TA_FunctionsU;

function GetRaceSpecificColor(ColorNr: Byte) : Byte;
begin
  if IniSettings.Colors then
  begin
    Result := IniSettings.CustomColors[Ord(TAData.RaceSide) + 1][ColorNr];
    if Result = 0 then
      Result := IniSettings.CustomColors[0][ColorNr];
    if Result = 0 then
      Result := ColorsArray[ColorNr].cDefaultVal;
  end else
    Result := ColorsArray[ColorNr].cDefaultVal;
end;

Procedure InstallColors(RaceSpecific: Boolean); stdcall;
var
  cColor: Byte;
  nColor: Word;
  nCurColor: Word;
  i: integer;
  CurrentProcessHandle: THandle;
  CommittedBytes: Longword;
  OldProtect, tmpOldProtect: longword;
begin
  CurrentProcessHandle := GetCurrentProcess;
  for i := 0 to 28 do
  begin
    nCurColor := 0;
    if RaceSpecific then
      if IniSettings.CustomColors[Ord(TAData.RaceSide) + 1][i] <> 0 then
        nCurColor := GetRaceSpecificColor(i)
    else
      nCurColor := IniSettings.CustomColors[0][i];
    if nCurColor <> 0 then
    begin
      case ColorsArray[i].cSwapType of
        1: begin
             Win32Check( VirtualProtect( pointer(ColorsArray[i].lOffset), 1, PAGE_READWRITE, OldProtect ) );
             cColor := nCurColor;
             Win32Check( WriteProcessMemory( CurrentProcessHandle,
                              pointer(ColorsArray[i].lOffset),
                              @cColor,
                              1,
                              CommittedBytes) );
             FlushInstructionCache( CurrentProcessHandle, pointer(ColorsArray[i].lOffset), 1 );
             Win32Check( VirtualProtect( pointer(ColorsArray[i].lOffset), 2, OldProtect, tmpOldProtect ) );
           end;
        2: begin
             Win32Check( VirtualProtect( pointer(ColorsArray[i].lOffset), 2, PAGE_READWRITE, OldProtect ) );
             nColor := FIRST_GUIPAL_COLOR + nCurColor;
             Win32Check( WriteProcessMemory( CurrentProcessHandle,
                              pointer(ColorsArray[i].lOffset),
                              @nColor,
                              2,
                              CommittedBytes) );
             FlushInstructionCache( CurrentProcessHandle, pointer(ColorsArray[i].lOffset), 2 );
             Win32Check( VirtualProtect( pointer(ColorsArray[i].lOffset), 2, OldProtect, tmpOldProtect ) );
           end;
      end;
    end;
  end;
end;

procedure ApplyPerRaceColors; stdcall;
begin
  InstallColors(True);
end;

procedure Colors_ApplyPerRaceColors;
asm
  pushAD
  call    ApplyPerRaceColors
  popAD
  call    LoadGameData_Main
  push $00497586;
  call PatchNJump;
end;

Procedure OnInstallColors;
begin
  InstallColors(False);
end;

Procedure OnUninstallColors;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_Colors then
  begin
    Result := TPluginData.create( False, 'Colors',
                                  State_Colors,
                                  @OnInstallColors,
                                  @OnUnInstallColors );

    Result.MakeRelativeJmp( State_Colors,
                            'Apply per race specific colors at game load',
                            @Colors_ApplyPerRaceColors,
                            $00497581, 0 );

    if IniSettings.Colors_DisableMenuDots then
      Result.MakeNOPReplacement( State_Colors,
                                 '',
                                 ColorsArray[30].lOffset, 7);

    if IniSettings.Colors_MenuDots <> 0 then
      Result.MakeReplacement( State_Colors,
                              '',
                              ColorsArray[29].lOffset,
                              IniSettings.Colors_MenuDots, 1);
  end else
    Result := nil;
end;

end.
