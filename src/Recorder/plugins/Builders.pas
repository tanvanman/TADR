unit Builders;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_Builders: Boolean = True;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallBuilders;
Procedure OnUninstallBuilders;

// -----------------------------------------------------------------------------

implementation
uses
  SysUtils,
  IniOptions,
  TA_FunctionsU,
  TA_MemoryStructures,
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_MemUnits;

function IsUnitWeaponNuke(UnitInfoId: Cardinal): LongBool; stdcall;
var
  UnitInfo: PUnitInfo;
  UnitInfoName: String;
begin
  Result := False;
  UnitInfo := TAMem.UnitInfoId2Ptr(Word(UnitInfoId));
  UnitInfoName := UnitInfo.szUnitName;
  if (Pos('MAKENUKE', UnitInfoName) <> 0) or
     (Pos('MAKEANTI', UnitInfoName) <> 0) then Result := True;
end;

procedure Builders_MobileAddBuild;
label
  AddToQueue;
asm
  test    bx, bx
  // unitinfo id of clicked type = 0
  jz      AddToQueue
  mov     eax, dword ptr [TAdynmemStructPtr]
  mov     ecx, [eax+37E9Eh]
  and     ecx, 0FFFFh
  mov     edx, ecx
  shl     edx, 6
  add     edx, ecx
  lea     ecx, [edx+edx*8]
  mov     edx, [eax+1439Bh]
  cmp     byte ptr [ecx+edx+TUnitInfo.cBMCode], 0
  // is builder - plant or mobile constructor
  jz      AddToQueue
  push    ecx
  push    ebx
  call    IsUnitWeaponNuke
  test    eax, eax
  mov     eax, dword ptr [TAdynmemStructPtr]
  mov     edx, [eax+1439Bh]
  pop     ecx
  jnz     AddToQueue
  mov     byte ptr [eax+2CC3h], 0Eh
  push $0041AB95;
  call PatchNJump;
AddToQueue:
  mov     eax, dword ptr [TAdynmemStructPtr]
  push $0041ABBB;
  call PatchNJump;
end;

procedure LimitAINonMobileStockpile(const UnitInfoName: PAnsiChar;
  p_Builder: PUnitStruct; QueuedAmount: Integer); stdcall;
begin
  if QueuedAmount > 0 then
  begin
    if (Pos('MAKENUKE', UnitInfoName) <> 0) or
       (Pos('MAKEANTI', UnitInfoName) <> 0) then
    begin
      if p_Builder.UnitWeapons[0].cStock > 0 then Exit;
      if p_Builder.p_SubOrder <> nil then
        if p_Builder.p_SubOrder.lPar2 > 0 then Exit;
    end;
  end;
  UnitSubBuildClick(UnitInfoName, p_Builder, QueuedAmount);
end;

function QueueIfStockpile(p_Unit: PunitStruct; UnitInfoId: Cardinal): LongBool; stdcall;
var
  UnitInfo: PUnitInfo;
  UnitInfoName: String;
begin
  Result := False;
  UnitInfo := TAMem.UnitInfoId2Ptr(Word(UnitInfoId));
  UnitInfoName := UnitInfo.szUnitName;
  if (Pos('MAKENUKE', UnitInfoName) <> 0) or
     (Pos('MAKEANTI', UnitInfoName) <> 0) then
  begin
    if p_Unit.UnitWeapons[0].cStock > 0 then Exit;
    if p_Unit.p_SubOrder <> nil then
      if p_Unit.p_SubOrder.lPar2 > 0 then Exit;
    UnitSubBuildClick(TAMem.UnitInfoId2Ptr(Word(UnitInfoId)).szUnitName, p_Unit, 1);
  end;
end;

procedure Builders_AI_MobileBuildStockpile;
label
  NoTypeFromProbabilityList,
  BuildAsUnit;
asm
  test    ax, ax
  jz      NoTypeFromProbabilityList
  push    edx
  push    eax
  push    ebx
  movzx   ebx, ax
  push    ecx
  push    ebx
  push    ebp
  call    QueueIfStockpile
  test    eax, eax
  pop     ecx
  pop     ebx
  pop     eax
  pop     edx
  jz      BuildAsUnit
  push $004082B5;
  call PatchNJump;
BuildAsUnit :
  push $004081B5;
  call PatchNJump;
NoTypeFromProbabilityList :
  push $004082B9;
  call PatchNJump;
end;

procedure Builders_PlantBuildNonMobile;
label
  BuildMobile,
  ContinueBrakeRateCheck;
asm
  cmp     byte [ebp+TUnitInfo.cBMCode], 0;
  jnz     BuildMobile;
  pushf
  push    eax
  mov     eax, [esp+2Ah]
  cmp     eax, $0040289E
  jz      ContinueBrakeRateCheck
  pop     eax
  popf
  push $0047DBDF;
  call PatchNJump;
ContinueBrakeRateCheck :
  //bmcode=0, check for brakerate
  pop     eax
  popf
  cmp     dword [ebp+$19A], 0;
  jnz     BuildMobile;
  push $0047DBDF;
  call PatchNJump;
BuildMobile:
  //bmcode=1
  push $0047DBF4;
  call PatchNJump;
end;

var
  TempString : array[0..63] of AnsiChar;
procedure Builders_YardmapForMobile;
label
  ReadYardmap,
  NoYardmap;
asm
  pushAD
  mov     ecx, [esp+34h]
  push    Null_str                 // null str
  push    $400                     // buff len
  push    $00503988  // "YardMap"
  lea     ebx, TempString
  push    ebx
  call    TdfFile_GetStr
  test    eax, eax
  jz      NoYardmap
ReadYardmap : //bmcode 0 or bmcode 1 with yardmap
  popAD
  push $0042CF3E;
  call PatchNJump;
NoYardmap : //bmcode 1 without yardmap
  popAD
  push $0042D073;
  call PatchNJump;
end;

procedure GUISwitcher(p_Unit: PUnitStruct; GUIIndex: Integer; Dest: PAnsiChar); stdcall;
var
  CustomGUIIdx: Integer;
  TmpString: String;
begin
  CustomGUIIdx := UnitInfoCustomFields[p_Unit.nUnitInfoID].CustomGUIIdx;
  if CustomGUIIdx <> 0 then
    TmpString := Format('%s%d_%d.GUI', [p_Unit.p_UNITINFO.szUnitName, GUIIndex, CustomGUIIdx])
  else
    TmpString := Format('%s%d.GUI', [p_Unit.p_UNITINFO.szUnitName, GUIIndex]);
  StrLCopy(Dest, PAnsiChar(TmpString), SizeOf(p_Unit.p_UNITINFO.szUnitName)-1);
end;

procedure Builders_GUISwitcher;
asm
  lea     ecx, [esp+150h]
  push    ecx
  push    esi
  push    ebp
  call    GUISwitcher
  mov     ecx, [TADynmemStructPtr]
  push $0041B7F7;
  call PatchNJump;
end;

Procedure OnInstallBuilders;
begin
end;

Procedure OnUninstallBuilders;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_Builders then
  begin
    Result := TPluginData.create( False,
                                  'Builders',
                                  State_Builders,
                                  @OnInstallBuilders,
                                  @OnUnInstallBuilders );

    Result.MakeRelativeJmp( State_Builders,
                            'Allow registration of yardmap data into memory for units that use tag',
                            @Builders_YardmapForMobile,
                            $0042CF38, 1 );

    Result.MakeReplacement( State_Builders,
                            'Have mobile units use the typical nano colors while under construction',
                            $0045961B, [0] );

    Result.MakeRelativeJmp( State_Builders,
                            'Allow mobile units to build mobile units',
                            @Builders_MobileAddBuild,
                            $0041AB5D, 0 );

    Result.MakeRelativeJmp( State_Builders,
                            'Plants can build non mobile units on itself (new upgrade solution)',
                            @Builders_PlantBuildNonMobile,
                            $0047DBD6, 0 );

    if IniSettings.AiNukes then
    begin
      Result.MakeReplacement( State_Builders,
                              'Stockpile buildings owned by AIs will produce nukes',
                              $004FC980,
                              [$D0, $86] );

      Result.MakeStaticCall( State_Builders,
                             'Limit AIs non mobile stockpile to 1',
                             @LimitAINonMobileStockpile,
                             $004087C4, );

      Result.MakeRelativeJmp( State_Builders,
                              'AIs mobile stockpile will produce weapons',
                              @Builders_AI_MobileBuildStockpile,
                              $004081AC, 4 );
    end;

    Result.MakeRelativeJmp( State_Builders,
                            'Swapping unit GUI from unit script',
                            @Builders_GUISwitcher,
                            $0041B7A4, 6 );
  end else
    Result := nil;
end;

end.
