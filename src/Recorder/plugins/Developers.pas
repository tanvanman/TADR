unit Developers;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_Developers: Boolean = True;

function GetPlugin: TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallDevelopers;
Procedure OnUninstallDevelopers;

// -----------------------------------------------------------------------------

implementation
uses
  SysUtils,
  TA_MemoryStructures,
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_FunctionsU,
  TA_MemUnits;

procedure DrawDevUnitStateProbes(p_Offscreen: Cardinal); stdcall;
begin
  if TAData.MainStruct.UnitStateProbeUnitEnab <> 0 then
    UnitStateProbe(p_Offscreen);
  if TAData.MainStruct.BuilderProbeEnab <> 0 then
    UnitBuilderProbe(p_Offscreen);
end;

procedure Developers_DevUnitProbes;
asm
  lea     eax, [esp+224h+OFFSCREEN_off]
  pushAD
  push    eax
  call    DrawDevUnitStateProbes
  popAD
  mov     eax, dword ptr [TAdynmemStructPtr]
  push $0046A0D2;
  call PatchNJump;
end;

function Developers_UnitStateProbe(p_Offscreen: Pointer;
  FontHeight: Integer; Top: Integer; p_Unit: PUnitStruct): Integer; stdcall;
var
  NewTop: Integer;
begin
  NewTop := FontHeight + Top;

  if p_Unit.p_UNITINFO <> nil then
  begin
    DrawTextCustomFont(p_Offscreen,
      PAnsiChar(Format('unit type: [%s] id:%d',
      [p_Unit.p_UNITINFO.szUnitName, p_Unit.nUnitInfoID])), 134, NewTop, -1);
    NewTop := FontHeight + NewTop;
  end;

  DrawTextCustomFont(p_Offscreen,
    PAnsiChar(Format('position: X:%d Y:%d Z:%d',
      [SHiWord(p_Unit.Position.X), SHiWord(p_Unit.Position.Z), SHiWord(p_Unit.Position.Y)])),
      134, NewTop, -1);
  NewTop := FontHeight + NewTop;

  DrawTextCustomFont(p_Offscreen,
    PAnsiChar(Format('grid position: X:%d Z:%d',
      [p_Unit.nGridPosX, p_Unit.nGridPosZ])), 134, NewTop, -1);
  NewTop := FontHeight + NewTop;

  DrawTextCustomFont(p_Offscreen,
    PAnsiChar(Format('state mask: $%s',
      [IntToHex(p_Unit.lUnitStateMask, 8)])), 134, NewTop, -1);
  NewTop := FontHeight + NewTop;

  DrawTextCustomFont(p_Offscreen,
    PAnsiChar(Format('basic state mask: $%s',
      [IntToHex(p_Unit.nUnitStateMaskBas, 4)])), 134, NewTop, -1);
  NewTop := FontHeight + NewTop;

  if p_Unit.p_TransporterUnit <> nil then
    if p_Unit.p_TransporterUnit.p_UNITINFO <> nil then
  begin
    DrawTextCustomFont(p_Offscreen,
      PAnsiChar(Format('transported by: [%s] id:%d',
      [p_Unit.p_TransporterUnit.p_UNITINFO.szUnitName,
        TAUnit.GetId(p_Unit.p_TransporterUnit)])), 134, NewTop, -1);
    NewTop := FontHeight + NewTop;
  end;

  if p_Unit.p_TransportedUnit <> nil then
    if p_Unit.p_TransportedUnit.p_UNITINFO <> nil then
  begin
    DrawTextCustomFont(p_Offscreen,
      PAnsiChar(Format('transporting: [%s] id:%d',
      [p_Unit.p_TransportedUnit.p_UNITINFO.szUnitName,
        TAUnit.GetId(p_Unit.p_TransportedUnit)])), 134, NewTop, -1);
    NewTop := FontHeight + NewTop;
  end;

  if p_Unit.p_PriorUnit <> nil then
    if p_Unit.p_PriorUnit.p_UNITINFO <> nil then
  begin
    DrawTextCustomFont(p_Offscreen,
      PAnsiChar(Format('prior unit: [%s] id:%d',
      [p_Unit.p_PriorUnit.p_UNITINFO.szUnitName,
        TAUnit.GetId(p_Unit.p_PriorUnit)])), 134, NewTop, -1);
    NewTop := FontHeight + NewTop;
  end;
  Result := NewTop;
end;

procedure Developers_UnitStateProbeExtra;
asm
  push    ebx
  push    edi

  push    ebx
  push    esi
  push    edi
  push    ebp
  call    Developers_UnitStateProbe
  mov     esi, eax
  pop     edi
  pop     ebx
  mov     ecx, [ebx+TUnitStruct.p_UNITINFO]
  push $00467FDD;
  call PatchNJump;
end;

procedure DrawDevBottomState(p_Offscreen: Pointer; Top: Integer); stdcall;
var
  Text: String;
begin
  Text := Format('CURSOR XY: %d %d',
    [TAData.MainStruct.nMouseMapPosX, TAData.MainStruct.nMouseMapPosY]);
  DrawTextCustomFont(p_Offscreen, PAnsiChar(Text), 640, top, -1);
end;

procedure Developers_DevBottomState;
asm
  push    esi
  push    edi
  call    DrawDevBottomState
  pop     edi
  pop     esi
  pop     ebp
  pop     ebx
  add     esp, 23Ch
  push $0046ABA0;
  call PatchNJump;
end;

Procedure OnInstallDevelopers;
begin
end;

Procedure OnUninstallDevelopers;
begin
end;

function GetPlugin: TPluginData;
begin
  if IsTAVersion31 and State_Developers then
  begin
    Result := TPluginData.Create( False, 'Developers Plugin',
                                  State_Developers,
                                  @OnInstallDevelopers,
                                  @OnUninstallDevelopers );

    Result.MakeReplacement( State_Developers,
                            'Don''t freeze game when minimized',
                            $004B5D1F,
                            [1] );

    Result.MakeRelativeJmp( State_Developers,
                            'Developers_DevUnitProbes',
                            @Developers_DevUnitProbes,
                            $0046A0CD, 0 );

    Result.MakeRelativeJmp( State_Developers,
                            'Developers_UnitStateProbeExtra',
                            @Developers_UnitStateProbeExtra,
                            $00467FD5, 1 );

    Result.MakeReplacement( State_Developers,
                            'Move [release] text a bit to right',
                            $0046A053, [192] );

    Result.MakeRelativeJmp( State_Developers,
                            'Developers_DevBottomState',
                            @Developers_DevBottomState,
                            $0046AB96, 5 );

    Result.MakeReplacement( State_Developers,
                            'Create audit file next to .SAV',
                            $00432A5B, [1] );

  end else
    Result := nil;
end;

end.
