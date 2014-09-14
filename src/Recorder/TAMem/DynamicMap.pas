unit DynamicMap;

interface

procedure SwapTNT(Idx: Byte);

implementation

uses
  Windows,
  SysUtils,
  TA_FunctionsU,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations;

procedure SwapTNT(Idx: Byte);
var
  sCurrentTNTPath : String;
  lpFileName : PAnsiChar;
  TNTFile : PTNTHeaderStruct;
  TNTMemStruct : PTNTMemStruct;
  PTR_TILE_SET, PTR_TILE_MAP, PTR_HEIGHT_MAP, PTR_MINIMAP : Pointer;
  p_TileSet, p_TileMap, p_PlotMemory : Pointer;
  TileMapSize, PlotSize : Cardinal;
  CurrentPlotPos : Integer;
  GridPlot : PPlotGrid;
  CurrHeightMap : Cardinal;
  GafFrame : TGAFFrame;
  offp : Cardinal;
  i : integer;
  UnitPtr : PUnitStruct;
  PlotGrid : PPlotGrid;
  x, z : Integer;
  AtLava : Boolean;
begin
  sCurrentTNTPath := PMapOTAFile(PTAdynmemStruct(TAData.MainStructPtr)^.p_MapOTAFile).sTNTFile;
  lpFileName := PAnsiChar(ChangeFileExt(sCurrentTNTPath, IntToStr(Idx) + '.TNT'));
  TNTFile := LoadTNTFile(lpFileName);

  TNTMemStruct := @PTAdynmemStruct(TAData.MainStructPtr).TNTMemStruct;
  CopyMemory(@PMapOTAFile(PTAdynmemStruct(TAData.MainStructPtr)^.p_MapOTAFile).sTNTFile,
             lpFileName, Length(lpFileName));

  // tileset pixels data
  PTR_TILE_SET := Pointer(Cardinal(TNTFile) + TNTFile.p_TileSet);
  p_TileSet := cmalloc_MM__((TNTFile.lTilesCount shl 10) + 8);
  PCardinal(p_TileSet)^ := TNTFile.lTilesCount;
  PCardinal(Cardinal(p_TileSet) + 4)^ := Cardinal(Pointer(Cardinal(p_TileSet) + 8));
  CopyMemory(Pointer(Cardinal(p_TileSet) + 8), PTR_TILE_SET, 4 * ((TNTFile.lTilesCount shl 10) shr 2));

  // tileset map
  TileMapSize := 2 * TNTMemStruct.lMapWidth div 32 * TNTMemStruct.lMapHeight div 32;
  p_TileMap := cmalloc_MM__(TileMapSize);
  PTR_TILE_MAP := Pointer(Cardinal(TNTFile) + TNTFile.p_TileMap);
  CopyMemory(p_TileMap, PTR_TILE_MAP, TileMapSize);

  // plot data
  PlotSize := TNTMemStruct.lTilesetMapSizeX * TNTMemStruct.lTilesetMapSizeY;
  p_PlotMemory := cmalloc_MM__(SizeOf(TPlotGrid) * PlotSize);
  CopyMemory(p_PlotMemory, TNTMemStruct.p_PlotMemory, SizeOf(TPlotGrid) * PlotSize);

  PTR_HEIGHT_MAP := Pointer(Cardinal(TNTFile) + TNTFile.p_HeightMap);
  GridPlot := PPlotGrid(p_PlotMemory);
  CurrHeightMap := Cardinal(PTR_HEIGHT_MAP);
  if PlotSize > 0 then
  begin
    CurrentPlotPos := PlotSize;
    repeat
      GridPlot.bHeight := PByte(CurrHeightMap)^;
      //GridPlot.nFeatureDefIndex := $FFFF;
      //if ( PWord(CurrHeightMap + 1)^ = $FFFC ) then
      //  PlaceFeatureOnMap(GridPlot, $FFFC, nil, nil, 10);
      GridPlot := Pointer(Cardinal(GridPlot) + SizeOf(TPlotGrid));
      Inc(CurrHeightMap, 4);
      Dec(CurrentPlotPos);
    until CurrentPlotPos <= 0;
  end;
  TNTMemStruct.p_TileSet := p_TileSet;
  TNTMemStruct.p_TileMap := p_TileMap;
  TNTMemStruct.p_PlotMemory := p_PlotMemory;
  LoadMap_AverageHeightMap;
  LoadMap_PLOT3;

  // reconstruct minimap
  PTR_MINIMAP := Pointer(Cardinal(TNTFile) + TNTFile.p_Minimap);
  GafFrame.Width := PCardinal(PTR_MINIMAP)^;
  GafFrame.Height := PCardinal(Pointer(Cardinal(PTR_MINIMAP) + 4))^;
  GafFrame.Left := 0;
  GafFrame.Top := 0;
  GafFrame.Background := 0;
  GafFrame.Compressed := 0;
  GafFrame.SubFrames := 0;
  GafFrame.IsCompressed := 0;
  GafFrame.PtrFrameData := nil;
  GafFrame.PtrFrameBits := Pointer(Pointer(Cardinal(PTR_MINIMAP) + 8));
  GafFrame.Bits2_Ptr := nil;
  TNTMemStruct.p_TedGeneratedPic := CompositeBuffer(PAnsiChar('TED GENERATED PIC'),
                                                    GafFrame.Width,
                                                    GafFrame.Height);
  CompositeBuf2_OFFSCREEN(@offp, TNTMemStruct.p_TedGeneratedPic);
  CopyGafToContext(@offp, @GafFrame, 0, 0);
  InitRadar;

  if (TAData.UnitsPtr <> nil) then
  begin
    for i := 1 to TAData.MaxUnitsID do
    begin
      UnitPtr := TAUnit.Id2Ptr(i);
      AtLava := False;
      if UnitPtr.nUnitInfoID <> 0 then
      begin
        for z := 0 to UnitPtr.nFootPrintZ do
        begin
          for x := 0 to UnitPtr.nFootPrintX do
          begin
            PlotGrid := GetGridPosPLOT(UnitPtr.nGridPosX + x, UnitPtr.nGridPosZ + z);
            if PlotGrid <> nil then
            begin
              //if GetPosHeight(@UnitPtr.Position) = 0 then
                if PlotGrid.nFeatureDefIndex = $FFFD then
                begin
                  AtLava := True;
                  Break;
                end;
            end;
          end;
          if AtLava then
            Break;
        end;
        if AtLava then
          if (UnitPtr.lUnitStateMask and 3) <> 2 then
            if (PMapOTAFile(PTAdynmemStruct(TAData.MainStructPtr)^.p_MapOTAFile).lIsLavaMap <> 0) then
              TAUnit.Kill(UnitPtr, 1)
            else
              TAUnit.Kill(UnitPtr, 0);
      end;
    end;
  end;
  //LoadFeatures;
  //LoadFeatureAnimData(TNTFile);
  //LoadMetalSpots;
  FreeMemory(TNTFile);
end;

end.
