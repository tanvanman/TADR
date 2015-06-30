unit SideDataExpand;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_SideDataExpand: Boolean = True;

function GetPlugin: TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallSideDataExpand;
Procedure OnUninstallSideDataExpand;

// -----------------------------------------------------------------------------

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_FunctionsU,
  TA_MemoryLocations,
  TA_MemUnits;

procedure SideDataExpand_Load(TDFHandle: Cardinal; p_SideData: PRaceSideData); stdcall;

  // TA comes with similar TDF class method, but it does terminate the process if section is missing
  procedure TdfFile_GetTagRect(TDFHandle: Cardinal; SectionName: String; p_tagRect: PtagRECT);
  var
    OldTDFRoot: Cardinal;
  begin
    OldTDFRoot := TdfFile_GetRoot(0, 0, TDFHandle);
    if TdfFile_SectionExists(0, 0, TDFHandle, PAnsiChar(SectionName)) then
    begin
      p_tagRect^.Left := TdfFile_GetInt(0, 0, PCardinal(TDFHandle + 4)^, 0, PAnsiChar('x1'));
      p_tagRect^.Top := TdfFile_GetInt(0, 0, PCardinal(TDFHandle + 4)^, 0, PAnsiChar('y1'));
      p_tagRect^.Right := TdfFile_GetInt(0, 0, PCardinal(TDFHandle + 4)^, 0, PAnsiChar('x2'));
      p_tagRect^.Bottom := TdfFile_GetInt(0, 0, PCardinal(TDFHandle + 4)^, 0, PAnsiChar('y2'));
    end;
    TdfFile_SetRoot(0, 0, TDFHandle, OldTDFRoot);
  end;

var
  SideIdx: Integer;
begin
  SideIdx := p_SideData.lSideIdx;
  TdfFile_GetTagRect(TDFHandle, 'DAMAGEVAL', @ExtraSideData[SideIdx].rectDamageVal);
  TdfFile_GetTagRect(TDFHandle, 'REALMETALINCOME', @ExtraSideData[SideIdx].rectRealMIncome);
  TdfFile_GetTagRect(TDFHandle, 'REALENERGYINCOME', @ExtraSideData[SideIdx].rectRealEIncome);
  TdfFile_GetTagRect(TDFHandle, 'SHIELDICON', @ExtraSideData[SideIdx].rectShieldIcon);
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

procedure SideDataExpand_LoadGaf(TDFHandle: Cardinal; SideDataIdx: Integer); stdcall;

  function TdfFile_GetString(TDFHandle: Cardinal; SectionName: String): String;
  const
    BufferSize = 32;
  var
    Buff: array[0..BufferSize-1] of AnsiChar;
  begin
    Result := '';
    if TdfFile_GetStr(0, 0, TDFHandle, Pointer(Null_str), BufferSize, PAnsiChar(SectionName), @Buff) <> 0 then
      SetString(Result, PChar(@Buff[0]), Length(Buff));
  end;

begin
  if High(ExtraSideData) < (SideDataIdx + 1) then
    SetLength(ExtraSideData, SideDataIdx + 1);
  ExtraSideData[SideDataIdx].logoGAF := TdfFile_GetString(TDFHandle, 'logogaf');
end;

procedure SideDataExpand_LoadGafHook;
asm
  pushAD
  push    edi
  push    ecx
  call    SideDataExpand_LoadGaf
  popAD
  push    offset Null_Str
  push $00429F66;
  call PatchNJump;
end;

Procedure OnInstallSideDataExpand;
begin
end;

Procedure OnUninstallSideDataExpand;
begin
end;

function GetPlugin: TPluginData;
begin
  if IsTAVersion31 and State_SideDataExpand then
  begin
    Result := TPluginData.Create( True, 'Load more data from SIDEDATA.TDF',
                                  State_SideDataExpand,
                                  @OnInstallSideDataExpand,
                                  @OnUnInstallSideDataExpand );

    Result.MakeRelativeJmp( State_SideDataExpand,
                            '',
                            @SideDataExpand_LoadHook,
                            $00432487, 0);

    Result.MakeRelativeJmp( State_SideDataExpand,
                            '',
                            @SideDataExpand_LoadGafHook,
                            $00429F61, 0);
  end else
    Result := nil;
end;

end.
