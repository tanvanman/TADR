unit SideDataExpand;

interface
uses
  PluginEngine, TA_MemoryStructures;

// -----------------------------------------------------------------------------

const
  State_SideDataExpand : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallSideDataExpand;
Procedure OnUninstallSideDataExpand;

// -----------------------------------------------------------------------------

procedure SideDataExpand_LoadHook;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_FunctionsU,
  TA_MemoryLocations,
  TA_MemUnits;

var
  SideDataExpandPlugin: TPluginData;

Procedure OnInstallSideDataExpand;
begin
end;

Procedure OnUninstallSideDataExpand;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_SideDataExpand then
  begin
    SideDataExpandPlugin := TPluginData.create( True,
                            '',
                            State_SideDataExpand,
                            @OnInstallSideDataExpand,
                            @OnUnInstallSideDataExpand );

    SideDataExpandPlugin.MakeRelativeJmp( State_SideDataExpand,
                          '',
                          @SideDataExpand_LoadHook,
                          $00432487, 0);

    Result := SideDataExpandPlugin;
  end else
    Result := nil;
end;

procedure SideDataExpand_Load(TDFHandle: Cardinal; p_SideData: PRaceSideData); stdcall;

  // TA comes with similar TDF class method, but it does terminate the process if section is missing
  procedure TdfFile_GetTagRect(TDFHandle: Cardinal; SectionName: String; p_tagRect: PtagRECT);
  var
    OldTDFRoot: Cardinal;
  begin
    OldTDFRoot := PCardinal(TDFHandle + 4)^;
    if TdfFile_SectionExists(0, 0, TDFHandle, PAnsiChar(SectionName)) then
    begin
      p_tagRect^.Left := TdfFile_GetInt(0, 0, PCardinal(TDFHandle + 4)^, 0, PAnsiChar('x1'));
      p_tagRect^.Top := TdfFile_GetInt(0, 0, PCardinal(TDFHandle + 4)^, 0, PAnsiChar('y1'));
      p_tagRect^.Right := TdfFile_GetInt(0, 0, PCardinal(TDFHandle + 4)^, 0, PAnsiChar('x2'));
      p_tagRect^.Bottom := TdfFile_GetInt(0, 0, PCardinal(TDFHandle + 4)^, 0, PAnsiChar('y2'));
    end;
    PCardinal(TDFHandle + 4)^ := OldTDFRoot;
  end;

var
  SideIdx: Integer;
begin
  SideIdx := p_SideData.lSideIdx;
  if High(ExtraSideData) < (SideIdx + 1) then
    SetLength(ExtraSideData, SideIdx + 1);

  TdfFile_GetTagRect(TDFHandle, 'DAMAGEVAL', @ExtraSideData[SideIdx].rectDamageVal);
  TdfFile_GetTagRect(TDFHandle, 'REALMETALINCOME', @ExtraSideData[SideIdx].rectRealMIncome);
  TdfFile_GetTagRect(TDFHandle, 'REALENERGYINCOME', @ExtraSideData[SideIdx].rectRealEIncome);
end;

procedure SideDataExpand_LoadHook;
asm
    lea     esi, [esp+10h]
    pushAD
    push    ebp
    push    esi
    call    SideDataExpand_Load
    popAD

    mov     esi, 1
    push $0043248C;
    call PatchNJump;
end;

end.
