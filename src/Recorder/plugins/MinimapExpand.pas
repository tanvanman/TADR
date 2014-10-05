unit MinimapExpand;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_MinimapExpand : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallMinimapExpand;
Procedure OnUninstallMinimapExpand;

// -----------------------------------------------------------------------------

procedure ForceMinimapPicGenerate;
procedure ModifyMinimapSize;

implementation
uses
  Windows,
  TADemoConsts,
  TA_MemoryLocations,
  TA_MemoryStructures,
  TA_MemoryConstants;

var
  MinimapUIWidth : Byte = 192;

Procedure OnInstallMinimapExpand;
begin
end;

Procedure OnUninstallMinimapExpand;
begin
end;

function GetPlugin : TPluginData;
var
  Replacement : Byte;
  ReplacementInt : Integer;
begin
  if IsTAVersion31 and State_MinimapExpand then
  begin
    Result := TPluginData.create( False,
                                  'MinimapExpand Plugin',
                                  State_MinimapExpand,
                                  @OnInstallMinimapExpand,
                                  @OnUninstallMinimapExpand );

    Replacement := MinimapUIWidth;

    Result.MakeReplacement( State_MinimapExpand, 'health bars',
                            $00469C90, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'game UI rect left',
                            $004981CF, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'game UI rect left 2',
                            $00468D07, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'mousemappos',
                            $00468DF5, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'player los 1',
                            $004735C3, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'player los 2',
                            $00473A37, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'player los 3',
                            $004741A3, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'player los 4',
                            $00474605, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'player los 5',
                            $00474BB3, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'draw gaf',
                            $00475066, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'eyeball 1',
                            $004754E3, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'eyeball 2',
                            $00475744, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'load map plot',
                            $004833DC, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'no ref, get pos h',
                            $00417BD5, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'hot units 1',
                            $0048BC07, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'hot units 2',
                            $0048BC1E, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'unit select 1',
                            $0048C3D5, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'unit select 2',
                            $0048C3ED, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'unit select 3',
                            $0048C50F, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'unit at mouse',
                            $0048C789, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'weapon debris',
                            $0046BB59, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'build rect 1',
                            $00469E64, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'build rect 2',
                            $00469E5B, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'game screen',
                            $00468DF5, Replacement, 1);
    
    Result.MakeReplacement( State_MinimapExpand, 'drawing stuff model related 1',
                            $004597C7, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'drawing stuff model related 2',
                            $0045979F, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'drawing stuff model related 3',
                            $004593B3 , Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'drawing stuff model related 4',
                            $0045939C , Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'drawing stuff model related 5',
                            $00458533, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'draw order range',
                            $00439A84, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'draw weapon attack range',
                            $00439999, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'draw path range',
                            $004396FD, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'draw range circle 1',
                            $0043901A, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'draw range circle 2',
                            $0043900A, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'draw build spot queue 1',
                            $00438CD7, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'draw build spot queue 2',
                            $00438CCE, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'unk 1',
                            $0042126E, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'unk 2',
                            $0042122F, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'explosions 1',
                            $00420C2E, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'explosions 2',
                            $00420B89, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'no ref, unk 5',
                            $00417EFD, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'no ref, unk 6',
                            $00417EA6, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'no ref, unk 7',
                            $00417DE1, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'no ref, unk 8',
                            $00417D9F, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'no ref, unk 9',
                            $00417D0C, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'no ref, unk 10',
                            $00417CCA, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'no ref, unk 11',
                            $00417C3D, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'unk 12',
                            $00417BD5, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'unit select rect lines',
                            $00467ABE, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'Weapon projectile 1',
                            $0049BFA2, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'Weapon projectile 2',
                            $0049BFCB, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'Weapon projectile 3',
                            $0049C0ED, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'Weapon projectile 4',
                            $0049C1B7, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'Weapon projectile 5',
                            $0049C237, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'Weapon projectile 6',
                            $0049C2CD, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'Weapon projectile 7',
                            $0049C300, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'Weapon projectile 8',
                            $0049C3A9, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'Weapon projectile 9',
                            $0049C467, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'Weapon projectile 10',
                            $0049C6D5, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'Weapon projectile 11',
                            $0049C6FF, Replacement, 1);

// not sure about this 2
    Result.MakeReplacement( State_MinimapExpand, 'get pos height 1',
                            $00484B97, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'get pos height 2',
                            $00484B9D, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'Update frame UI',
                            $004A8284, Replacement, 1);
    ReplacementInt := -Replacement;
    Result.MakeReplacement( State_MinimapExpand, 'Update frame UI',
                            $004A8276, ReplacementInt, 4);

//    Result.MakeReplacement( State_MinimapExpand, 'load gui file',
//                            $004AAC6D, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'load panel gui',
                            $0045CF00, Replacement, 1);

//004BA0C3 018 81 FE 80 00 00 00                                   cmp     esi, 80h
//004BA118 018 81 F9 80 00 00 00                                   cmp     ecx, 80h

    Replacement := MinimapUIWidth + 1;

    Result.MakeReplacement( State_MinimapExpand, 'panelbot repeat 1',
                            $0046A8B6, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'panelbot repeat 2',
                            $0046AD65, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'paneltop 1',
                            $00467DD7, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'paneltop 2',
                            $00467E0E, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'paneltop repeat 1',
                            $00469051, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'paneltop repeat 2',
                            $0046905D, Replacement, 1);

    Result.MakeReplacement( State_MinimapExpand, 'draw bps 1',
                            $0046838C, Replacement, 1);
    Result.MakeReplacement( State_MinimapExpand, 'draw bps 2',
                            $004684C7, Replacement, 1);

    Replacement := MinimapUIWidth - 2;

    Result.MakeRelativeJmp( State_MinimapExpand,
                           'ModifyMinimapSize',
                           @ModifyMinimapSize,
                           $00466799, 1);

    Result.MakeRelativeJmp( State_MinimapExpand,
                           'ForceMinimapPicGenerate',
                           @ForceMinimapPicGenerate,
                           $0046684F, 1);
 {   Replacement := 12;
    Result.MakeReplacement( State_MinimapExpand, 'metal spots 8 to 12',
                            $00422070, Replacement, 1);}
  end else
    Result := nil;
end;

procedure ForceMinimapPicGenerate;
asm
  push $0046686C;
  call PatchNJump;
end;

var
  MinimapSizeRes : Cardinal;
procedure GetMinimapSize(MapGameHeight, MapGameWidth: Integer); stdcall;
var
  MiniMapSize : Byte;
  i : Integer;
begin
  MiniMapSize := MinimapUIWidth - 2;
  if ( MapGameWidth < MapGameHeight ) then
  begin
    i := MiniMapSize * MapGameWidth div MapGameHeight;
    PTAdynmemStruct(TAData.MainStructPtr).RadarPicRect_left := (MiniMapSize - i) div 2;
    PTAdynmemStruct(TAData.MainStructPtr).RadarPicRect_top := 0;
    MinimapSizeRes := MakeLong(MiniMapSize, i);
  end else
  begin
    PTAdynmemStruct(TAData.MainStructPtr).RadarPicRect_left := 1;
    i := MiniMapSize * MapGameHeight div MapGameWidth;
    PTAdynmemStruct(TAData.MainStructPtr).RadarPicRect_top := (MiniMapSize - i) div 2;
    MinimapSizeRes := MakeLong(i, MiniMapSize);
  end;
end;


procedure ModifyMinimapSize;
asm
  pushAD
  push    ecx    // map width
  push    edi    // map height
  call    GetMinimapSize
  popAD
  mov     eax, MinimapSizeRes
  movzx   edi, ax
  and     eax, $FFFF0000
  shr     eax, 16
  movzx   esi, ax
  mov     eax, [TADynmemStructPtr]

  push $00466808;
  call PatchNJump;
end;

//  push $0041126D;
//  call PatchNJump;

end.

