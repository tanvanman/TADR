unit NanoFrameUnits;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_NanoFrameUnits: Boolean = True;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallNanoFrameUnits;
Procedure OnUninstallNanoFrameUnits;

// -----------------------------------------------------------------------------

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_MemPlayers,
  TA_MemUnits,
  TA_FunctionsU,
  ExtensionsMem;

const
  NanoUnitCreateInit: AnsiString = 'NanoFrameInit';

procedure PrepareNanoUnit(p_Unit: PUnitStruct);
var
  UnitInfo : PUnitInfo;
begin
  UnitInfo := p_Unit.p_UnitInfo;
  if UnitInfo <> nil then
  begin
    UnitInfo.UnitTypeMask := UnitInfo.UnitTypeMask or $2000000;
    UnitInfo.nSightDistance := 0;
    UnitInfo.nRadarDistance := 0;
    UnitInfo.nSonarDistance := 0;
    UnitInfo.nSonarDistanceJam := 0;
    UnitInfo.nRadarDistanceJam := 0;
    UnitInfo.cMakesMetal := 0;
    UnitInfo.fEnergyMake := 0.0;
    UnitInfo.fEnergyUse := 0.0;
    UnitInfo.fMetalMake := 0.0;
    UnitInfo.fMetalUse := 0.0;
    UnitInfo.fWindGenerator := 0.0;
    UnitInfo.fTidalGenerator := 0.0;
    UnitInfo.lEnergyStorage := 0;
    UnitInfo.lMetalStorage := 0;
  end;
end;  

procedure DrawBuildSpotQueueNanoframe(pOffscreen: Cardinal;
  UnitOrder: PUnitOrder); stdcall;
var
  UnitInfo: PUnitInfo;
begin
  UnitInfo := TAMem.UnitInfoId2Ptr(UnitOrder.lPar1);
  // draw only queued (and not under construction already)
  if UnitOrder.ucState <= 1 then
  begin
    if NanoSpotQueueUnitSt.nUnitInfoID > 0 then
      FreeUnitMem(@NanoSpotQueueUnitSt);

    NanoSpotQueueUnitSt.nUnitInfoID := UnitInfo.nCategory;
    NanoSpotQueueUnitSt.p_UnitInfo := TAMem.UnitInfoId2Ptr(NanoSpotQueueUnitSt.nUnitInfoID);

    NanoSpotQueueUnitSt.p_Owner := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);
    if UNITS_AllocateUnit(@NanoSpotQueueUnitSt, UnitOrder.Position.X, UnitOrder.Position.Y, UnitOrder.Position.Z, 0) then
    begin
      if (UnitInPlayerLOS(TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID), @NanoSpotQueueUnitSt) <> 0) and
         TAUnit.IsInGameUI(@NanoSpotQueueUnitSt) then
      begin
        NanoSpotQueueUnitInfoSt := TAMem.UnitInfoId2Ptr(NanoSpotQueueUnitSt.nUnitInfoID)^;
        NanoSpotQueueUnitSt.p_UnitInfo := @NanoSpotQueueUnitInfoSt;
        NanoSpotQueueUnitSt.Position.Y := UnitOrder.Position.Y;
        NanoSpotQueueUnitSt.Turn.Z := 32768;

        if NanoSpotQueueUnitInfoSt.cBMCode = 1 then
        begin
          UNITS_AllocateMovementClass(@NanoSpotQueueUnitSt);
        end;
        
        if ((UnitOrder.p_Unit.lUnitStateMask shr 4) and 1 = 1) then
          NanoSpotQueueUnitSt.lUnitInGameIndex := $FFFF
        else
          NanoSpotQueueUnitSt.lUnitInGameIndex := $FFFE;

        if UNITS_CreateModelScripts(@NanoSpotQueueUnitSt) <> nil then
        begin
          PrepareNanoUnit(@NanoSpotQueueUnitSt);
          DrawUnit(Pointer(pOffscreen), @NanoSpotQueueUnitSt);
        end;
      end;
    end;
  end;
end;

procedure DrawBuildSpot_QueueNanoframeHook;
asm
  pushAD
  mov     eax, [esp+6Ch]
  mov     edx, [esp+64h]
  push    eax // unitorder
  push    edx // offscreen
  call    DrawBuildSpotQueueNanoframe
  popAD
  mov     eax, [edx+TTADynMemStruct.lGameTime]
  push $00438CBD;
  call PatchNJump;
end;  

procedure DrawBuildSpotNanoframe(pOffscreen: Cardinal); stdcall;
var
  X, Z, H: Integer;
begin
  if PByte($004BF8C0)^ = $C2 then
    Exit;

  if NanoSpotUnitSt.nUnitInfoID <> 0 then
    FreeUnitMem(@NanoSpotUnitSt);

  if (TAData.MainStruct.nBuildNum <> 0) and
     (TAData.MainStruct.ucPrepareOrderType = $E) then
  begin
    NanoSpotUnitSt.nUnitInfoID := TAData.MainStruct.nBuildNum;
    NanoSpotUnitSt.p_UnitInfo := TAMem.UnitInfoId2Ptr(NanoSpotUnitSt.nUnitInfoID);
    if (UnitInfoCustomFields[NanoSpotUnitSt.p_UnitInfo.nCategory].DrawBuildSpotNanoFrame = True) or
       IniSettings.ForceDrawBuildSpotNano then
    begin
      X := TAData.MainStruct.lBuildPosRealX shl 16;
      Z := TAData.MainStruct.lBuildPosRealY shl 16;
      if (X < 0) or (Z < 0) then
        Exit;
      H := TAData.MainStruct.lBuildPosRealH shl 16;        
      NanoSpotUnitSt.p_Owner := TAPlayer.GetPlayerByIndex(TAData.LocalPlayerID);
      if UNITS_AllocateUnit(@NanoSpotUnitSt, X, H, Z, 0) then
      begin
        NanoSpotUnitInfoSt := TAMem.UnitInfoId2Ptr(NanoSpotUnitSt.nUnitInfoID)^;
        NanoSpotUnitSt.p_UnitInfo := @NanoSpotUnitInfoSt;

        NanoSpotUnitSt.Position.X := X + (NanoSpotUnitSt.nFootPrintX * 8) shl 16;
        NanoSpotUnitSt.Position.Z := Z + (NanoSpotUnitSt.nFootPrintZ * 8) shl 16;
        NanoSpotUnitSt.Position.Y := H;
        NanoSpotUnitSt.Turn.Z := 32768;

        if NanoSpotUnitInfoSt.cBMCode = 1 then
        begin
          UNITS_AllocateMovementClass(@NanoSpotUnitSt);
        end;

        if (TAData.MainStruct.cBuildSpotState and $40 = $40) then
          NanoSpotUnitSt.lUnitInGameIndex := $FFFF
        else
          NanoSpotUnitSt.lUnitInGameIndex := $FFFE;

        if UNITS_CreateModelScripts(@NanoSpotUnitSt) <> nil then
        begin
          PrepareNanoUnit(@NanoSpotUnitSt);
          DrawUnit(Pointer(pOffscreen), @NanoSpotUnitSt);
        end;
      end;
    end;
  end;
end;

procedure DrawBuildSpot_NanoframeHook;
asm
  lea     ecx, [esp+224h+OFFSCREEN_off]
  push    ecx
  call    DrawBuildSpotNanoframe
  mov     ecx, [TADynMemStructPtr]
  push $00469F29;
  call PatchNJump;
end;

procedure DrawBuildSpot_NanoframeShimmerHook;
label
  DrawNonGlitter,
  DrawMoreTransp,
  ComeBack;
asm
  mov     cx, [edx+0A8h]
  movzx   ebx, word ptr [edx+TUnitStruct.lUnitInGameIndex]
  cmp     ebx, $FFFE
  // edx unitstruct
  mov     edx, [TAdynmemStructPtr]
  jge     DrawNonGlitter
  mov     ebx, ecx
  xor     ecx, 9
  mov     esi, [edx+TTAdynmemStruct.lGameTime]
  xor     ebx, 5
  mov     edx, esi
  shl     edx, 5
  add     edx, esi
  mul     edx
  lea     eax, [esi+esi*8]
  shr     edx, 4
  lea     eax, [esi+eax*2]
  // ebx par1
  add     ebx, edx
  lea     edx, [eax+eax*2]
  mov     eax, 88888889h
  mul     edx
  shr     edx, 4
  // ecx par2
  add     ecx, edx
  jmp     ComeBack
DrawNonGlitter:
  cmp     ebx, $FFFE
  je      DrawMoreTransp
  mov     ebx, 66
  mov     ecx, 66
  jmp     ComeBack
DrawMoreTransp:
  mov     ebx, 165
  mov     ecx, 165
ComeBack:
  push $00458E56;
  call PatchNJump;
end;

procedure DrawBuildSpot_InitScript;
label
  CallInitScript;
asm
  push    ebx
  movzx   ebx, word ptr [esi+TUnitStruct.lUnitInGameIndex]
  cmp     ebx, $FFFE
  pop     ebx
  jge     CallInitScript
  push    $00508BE0 //"Create"
  push $00485DE6;
  call PatchNJump;
CallInitScript:
  push    NanoUnitCreateInit //"NanoFrameInit"
  push $00485DE6;
  call PatchNJump;
end;  

Procedure OnInstallNanoFrameUnits;
begin
end;

Procedure OnUninstallNanoFrameUnits;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_NanoFrameUnits then
  begin
    Result := TPluginData.create( False, 'Nanoframe units',
                                  State_NanoFrameUnits,
                                  @OnInstallNanoFrameUnits,
                                  @OnUninstallNanoFrameUnits );

    Result.MakeRelativeJmp( State_NanoFrameUnits,
                            'Call Create script for just created unit',
                            @DrawBuildSpot_InitScript,
                            $00485DE1, 0 );

    Result.MakeRelativeJmp( State_NanoFrameUnits,
                            'Draw nanoframe at build spot',
                            @DrawBuildSpot_NanoframeHook,
                            $00469F23, 1 );

    if IniSettings.DrawBuildSpotQueueNano then
      Result.MakeRelativeJmp( State_NanoFrameUnits,
                              'Draw nanoframes for queued buildings',
                              @DrawBuildSpot_QueueNanoframeHook,
                              $00438CB7, 1 );

    if not IniSettings.BuildSpotNanoShimmer then
      Result.MakeRelativeJmp( State_NanoFrameUnits,
                              'Enable/disable shimmering effect for nanoframe units',
                              @DrawBuildSpot_NanoframeShimmerHook,
                              $00458E18, 2 );
  end else
    Result := nil;
end;

end.
