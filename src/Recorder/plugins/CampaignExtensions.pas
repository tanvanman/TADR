unit CampaignExtensions;

interface
uses
  SysUtils, Classes, PluginEngine, IniFiles, Windows;

// -----------------------------------------------------------------------------

type
  TMemStreamIniFile = class(TMemIniFile)
  public
     procedure LoadFromStream(Stream: TStream);
  end;

const
  State_CampaignExtensions : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallCampaignExtensions;
Procedure OnUninstallCampaignExtensions;

// -----------------------------------------------------------------------------

procedure LoadingMapSchema;
procedure RunMapMissionScript;
procedure CheckMouseForLock;
procedure ScreenFadeControl;

procedure InitMapMissions; stdcall;

implementation
uses
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_FunctionsU;

Procedure OnInstallCampaignExtensions;
begin
end;

Procedure OnUninstallCampaignExtensions;
begin
end;

function GetPlugin : TPluginData;
//var
//  Replacement : array[0..4] of Byte;
begin
  if IsTAVersion31 and State_CampaignExtensions then
  begin
    Result := TPluginData.Create( False,
                                  '',
                                  State_CampaignExtensions,
                                  @OnInstallCampaignExtensions,
                                  @OnUnInstallCampaignExtensions );

    Result.MakeRelativeJmp(State_CampaignExtensions,
                           'Init map script',
                           @LoadingMapSchema,
                           $00497B4A, 0 );

    Result.MakeRelativeJmp(State_CampaignExtensions,
                           'Keep running map script (no more "sleep" freeze)',
                           @RunMapMissionScript,
                           //$0048ADEB, 0 );
                           $0048B028, 0 );

    Result.MakeRelativeJmp(State_CampaignExtensions,
                           'Locking mouse functionality',
                           @CheckMouseForLock,
                           $004B5E35, 1 );

    Result.MakeRelativeJmp(State_CampaignExtensions,
                           'Control game screen fade level',
                           @ScreenFadeControl,
                           $0046A2E2, 0 );

    // do not release TDF Features vector after loading the list
  {  FillMemory(@Replacement[0], 5, $90);
    Result.MakeReplacement(State_CampaignExtensions,
                           '',
                           $004918DE,
                           Replacement ); }
  end else
    Result := nil;
end;

procedure InitMapMissions; stdcall;
var
  MapOTAFile : PMapOTAFile;
  COBFilePath : String;
  TDFFilePath : String;
  TDFFileSize : Integer;
  p_TDFFile : Pointer;
  TDFFile : array of byte;
  msTDF : TMemoryStream;
  TDFIni : TMemStreamIniFile;
  AiDifficulty : Integer;
  GameType : Integer;
begin
  MouseLock := False;
  CameraFadeLevel := 0;

  MapOTAFile := PTAdynmemStruct(TAData.MainStructPtr).p_MapOTAFile;
  COBFilePath := ChangeFileExt(MapOTAFile.sTNTFile, '.cob');
  if _filelength_HPI(PAnsiChar(COBFilePath)) > 0 then
  begin
    TDFFilePath := ChangeFileExt(MapOTAFile.sTNTFile, '.tdf');
    TDFFileSize := _filelength_HPI(PAnsiChar(TDFFilePath));
    if TDFFileSize > 0 then
    begin
      p_TDFFile := LoadHPITerainFile(PAnsiChar(TDFFilePath));
      TDFIni := TMemStreamIniFile.Create('');
      try
        msTDF := TMemoryStream.Create;
        try
          SetLength(TDFFile, TDFFileSize);
          CopyMemory(TDFFile, p_TDFFile, TDFFileSize);
          msTDF.Write(TDFFile[0], TDFFileSize);
          msTDF.Position := 0;
          TDFIni.LoadFromStream(msTDF);
          if TDFIni.SectionExists('sounds') then
          begin
            MapMissionsSounds := TStringList.Create;
            TDFIni.ReadSection('sounds', MapMissionsSounds);
          end;
          if TDFIni.SectionExists('features') then
          begin
            MapMissionsFeatures := TStringList.Create;
            TDFIni.ReadSection('features', MapMissionsFeatures);
          end;
        finally
          msTDF.Free;
        end;
      finally
        TDFIni.Free;
      end;
    end;

    FreeUnitMem(@MapMissionsUnit);

    MapMissionsUnit.nUnitInfoID := 1;
    //MapMissionsUnit.p_UnitDef := TAMem.UnitInfoId2Ptr(0);

    MapMissionsUnit.p_Owner := TAPlayer.GetPlayerByIndex(TAData.ViewPlayer);
    //if UNITS_AllocateUnit(@MapMissionsUnit, 0, 0, 0, 1) then
    //begin
      MapMissionsUnitInfo := PUnitInfo(TAMem.UnitInfoId2Ptr(MapMissionsUnit.nUnitInfoID))^;
      MapMissionsUnitInfo.pCOBScript := COBEngine_LoadScriptFromFile(PAnsiChar(COBFilePath));
      MapMissionsUnit.p_UnitDef := @MapMissionsUnitInfo;
      UNITS_CreateModelScripts(@MapMissionsUnit);
      GameType := Ord(TAData.GameingType);
      if GameType <> 0 then
      begin
        AiDifficulty := Ord(TAData.AIDifficulty);
        TAUnit.CallCobProcedure(@MapMissionsUnit, 'MapMission', @GameType, @AiDifficulty, nil, nil);
      end;
      MapMissionsUnit.lUnitInGameIndex := MapMissionsUnit.lUnitInGameIndex or $0000FFFF;
    //end;
  end;
end;

procedure LoadingMapSchema;
asm
  pushAD
  call InitMapMissions
  popAD
  mov     ecx, [TADynMemStructPtr]
  //call LoadCampaign_UniqueUnits
  push $00497B50
  call PatchNJump
end;

procedure RunMapMissionScript;
label
  GoBack;
asm
  lea     esi, MapMissionsUnit
  push    esi
  mov     ecx, [esi+TUnitStruct.p_UnitScriptsData]
  test    ecx, ecx
  jz      GoBack
  push    1
  call    COBEngine_DoScriptsNow
GoBack :
  pop     esi
  mov     ecx, [TADynMemStructPtr]
  push $0048B02E
  call PatchNJump
end;

procedure CheckMouseForLock;
label
  NoNewMouseEvent,
  DiscardMouseEvent;
asm
  jz      NoNewMouseEvent
  mov     eax, MouseLock
  test    eax, eax
  jnz     DiscardMouseEvent
  push $004B5E3B
  call PatchNJump
NoNewMouseEvent :
  push $004B5F5F
  call PatchNJump
DiscardMouseEvent :
  push $004B5F56
  call PatchNJump
end;

procedure DrawFade(Offscreenp: Cardinal); stdcall;
begin
  if CameraFadeLevel <> 0 then
    DrawTransparentBox(Offscreenp, nil, CameraFadeLevel - 31);
end;

procedure ScreenFadeControl;
asm
  lea     ecx, [esp+224h+OFFSCREEN_off]
  push    ecx
  call    DrawFade
  lea     ecx, [esp+224h+OFFSCREEN_off]
  push    ecx
  push $0046A2E7
  call PatchNJump
end;

{ TMemStreamIniFile }

procedure TMemStreamIniFile.LoadFromStream(Stream: TStream);
var
  List : TStringList;
begin
  List := TStringList.Create;
  try
    List.LoadFromStream(Stream);
    SetStrings(List);
  finally
    List.Free;
  end;
end;

end.

