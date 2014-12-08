unit SaveGame;

interface
uses
  PluginEngine,
  Classes,
  TA_MemoryStructures;

// -----------------------------------------------------------------------------

type
  TScriptSlotsSaveGameRec = packed record
    lCOBFileNode : Cardinal;
    SlotsData : array[0..63] of TScriptSlot;
    lStartRunningNow : Cardinal;
  end;

const
  State_SaveGame : boolean = true;
  CREATE_AUDIT_FILE : Byte = 1;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallSaveGame;
Procedure OnUninstallSaveGame;

// -----------------------------------------------------------------------------

procedure SaveGame_SaveUnitScriptSlots;
procedure SaveGame_LoadUnitScriptSlots_1;
procedure SaveGame_LoadUnitScriptSlots_2;
procedure SaveGame_SaveAdditionHook;
procedure SaveGame_LoadAdditionHook;
procedure SaveGame_LoadNoScriptsFix;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_FunctionsU,
  ExtensionsMem,
  COB_Extensions;

Procedure OnInstallSaveGame;
begin
end;

Procedure OnUninstallSaveGame;
begin
end;

function GetPlugin : TPluginData;
var
  lReplacement : Cardinal;
begin
  if IsTAVersion31 and State_SaveGame then
  begin
    Result := TPluginData.create( False, 'SaveGame Plugin',
                                  State_SaveGame,
                                  @OnInstallSaveGame,
                                  @OnUninstallSaveGame );
{
    Result.MakeReplacement( State_SaveGame,
                            'Create audit file next to .SAV',
                            $00432A5B, CREATE_AUDIT_FILE, 1);
}
    Result.MakeRelativeJmp( State_SaveGame,
                            'Save TADR structures to SAV file',
                            @SaveGame_SaveAdditionHook,
                            $00432A38, 0 );

    Result.MakeRelativeJmp( State_SaveGame,
                            'Load TADR structures from SAV file',
                            @SaveGame_LoadAdditionHook,
                            $0043267D, 1 );
                            
                             {
    Result.MakeRelativeJmp( State_SaveGame,
                            'Load game fix for units with no scripts',
                            @SaveGame_LoadNoScriptsFix,
                            $004875F9, 1 );
                             }

    if IniSettings.Plugin_ScriptSlotsLimit and
       (IniSettings.ModId > 1) then
    begin
      lReplacement := $00002908;
      Result.MakeReplacement( State_SaveGame,
                              'load game struct size fix',
                              $004B207A, lReplacement, SizeOf(lReplacement));

      Result.MakeRelativeJmp( State_SaveGame,
                              '',
                              @SaveGame_SaveUnitScriptSlots,
                              $004B1EDA, 1 );

      Result.MakeRelativeJmp( State_SaveGame,
                              '',
                              @SaveGame_LoadUnitScriptSlots_1,
                              $004B209A, 4 );

      Result.MakeRelativeJmp( State_SaveGame,
                              '',
                              @SaveGame_LoadUnitScriptSlots_2,
                              $004B20C1, 2 );
                              
      Result.MakeReplacement( State_SaveGame,
                              'load game struct size fix 3',
                              $004B20AC, lReplacement, SizeOf(lReplacement));
    end;
  end else
    Result := nil;
end;

var
  ScriptSlotsSaveGameRec : TScriptSlotsSaveGameRec;
procedure SaveUnitScriptSlots(ScriptData: PNewScriptsData); stdcall;
var
  lSlotIdx : Integer;
begin
  FillChar(ScriptSlotsSaveGameRec, SizeOf(ScriptSlotsSaveGameRec), 0);
  ScriptSlotsSaveGameRec.lCOBFileNode := Cardinal(ScriptData.pCOBFileNode);
  for lSlotIdx := 0 to MAX_SCRIPT_SLOTS - 1 do
  begin
    ScriptSlotsSaveGameRec.SlotsData[lSlotIdx] := ScriptData.ScriptSlots[lSlotIdx];
  end;
  ScriptSlotsSaveGameRec.lStartRunningNow := ScriptData.lStartRunningNow;
end;

procedure SaveGame_SaveUnitScriptSlots;
asm
  push    ebx
  push    edx
  push    ecx

  push    ebx                  // scriptsdata
  call    SaveUnitScriptSlots

  pop     ecx
  pop     edx
  pop     ebx

  mov     esi, [esp+5B0h]
  xor     ebp, ebp
  push    type TScriptSlotsSaveGameRec
  lea     eax, ScriptSlotsSaveGameRec
  push    eax
  mov     ecx, esi

  push $004B1F36
  Call PatchNJump;
end;

procedure SaveGame_LoadUnitScriptSlots_1;
asm
  lea     edx, ScriptSlotsSaveGameRec
  push    type TScriptSlotsSaveGameRec
  push    edx
  mov     ecx, ebp
  push $004B20A6
  Call PatchNJump;
end;

procedure LoadUnitScriptSlots(ScriptData: PNewScriptsData); stdcall;
var
  lSlotIdx : Integer;
begin
  ScriptData.pCOBFileNode := Pointer(ScriptSlotsSaveGameRec.lCOBFileNode);
  for lSlotIdx := 0 to MAX_SCRIPT_SLOTS - 1 do
  begin
    ScriptData.ScriptSlots[lSlotIdx] := ScriptSlotsSaveGameRec.SlotsData[lSlotIdx];
  end;
  ScriptData.lStartRunningNow := ScriptSlotsSaveGameRec.lStartRunningNow;
  FillChar(ScriptSlotsSaveGameRec, SizeOf(ScriptSlotsSaveGameRec), 0);
end;

procedure SaveGame_LoadUnitScriptSlots_2;
label
  ContinueParse;
asm
  mov     eax, [ebx+TNewScriptsData.pCOBFileNode]
  mov     ecx, ScriptSlotsSaveGameRec.lCOBFileNode
  cmp     eax, ecx
  jz      ContinueParse
  push $004B20CC
  call PatchNJump;
ContinueParse :
pushAD
  push    ebx
  call    LoadUnitScriptSlots
popAD
  mov     esi, [esp+10h]
  mov     edx, [ebx+10h]
  mov     ebp, [esp+54Ch]

  push $004B2122
  call PatchNJump;
end;

procedure SaveGame_SaveAddition(HAPIBANK: Pointer); stdcall;
var
  Stream: TMemoryStream;
begin
  if TAData.GUICallbackState = gsPlaying then
  begin
    HAPIBANK_OpenAccount(nil, nil, HAPIBANK, PAnsiChar('TADR Extensions'));
    HAPIBANK_AccessSafeDeposit(nil, nil, HAPIBANK, PAnsiChar('UnitsSharedData'));
    HAPIBANK_WriteBinData(nil, nil, HAPIBANK, 1024, @UnitsSharedData);

    if not UnitSearchResults.IsVoid then
    begin
      Stream := TMemoryStream.Create;
      try
        UnitSearchResults.SaveToStream(Stream);
        Stream.Position := 0;
        HAPIBANK_AccessSafeDeposit(nil, nil, HAPIBANK, PAnsiChar('UnitSearch')) ;
        HAPIBANK_WriteBinData(nil, nil, HAPIBANK, Stream.Size, Stream.Memory);
      finally
        Stream.Free;
      end;
    end;

    if not SpawnedMinions.IsVoid then
    begin
      Stream := TMemoryStream.Create;
      try
        SpawnedMinions.SaveToStream(Stream);
        Stream.Position := 0;
        HAPIBANK_AccessSafeDeposit(nil, nil, HAPIBANK, PAnsiChar('SpawnedMinions'));
        HAPIBANK_WriteBinData(nil, nil, HAPIBANK, Stream.Size, Stream.Memory);
      finally
        Stream.Free;
      end;
    end;
  end;
end;

procedure SaveGame_SaveAdditionHook;
asm
  pushAD
  lea     ecx, [esp+24h]
  push    ecx
  call    SaveGame_SaveAddition
  popAD
  mov     eax, [TADynMemStructPtr]
  push $00432A3D
  call PatchNJump;
end;

procedure SaveGame_LoadAddition(HAPIBANK: Pointer); stdcall;
var
  Stream: TMemoryStream;
  StreamSize: Integer;
begin
  if HAPIBANK_OpenAccount(nil, nil, HAPIBANK, PAnsiChar('TADR Extensions')) then
  begin
    if HAPIBANK_AccessSafeDeposit(nil, nil, HAPIBANK, PAnsiChar('UnitsSharedData')) then
      HAPIBANK_ReadBinData(nil, nil, HAPIBANK, 1024, @UnitsSharedData);

    if not UnitSearchResults.IsVoid then
    begin
      if HAPIBANK_AccessSafeDeposit(nil, nil, HAPIBANK, PAnsiChar('UnitSearch')) then
      begin
        Stream := TMemoryStream.Create;
        try
          StreamSize := HAPIBANK_GetItemSize(nil, nil, HAPIBANK);
          if StreamSize > 0 then
          begin
            Stream.SetSize(StreamSize);
            HAPIBANK_ReadBinData(nil, nil, HAPIBANK, StreamSize, Stream.Memory);
            Stream.Position := 0;
            UnitSearchResults.LoadFromStream(Stream);
          end;
        finally
          Stream.Free;
        end;
      end;
    end;

    if not SpawnedMinions.IsVoid then
    begin
      if HAPIBANK_AccessSafeDeposit(nil, nil, HAPIBANK, PAnsiChar('SpawnedMinions')) then
      begin
        Stream := TMemoryStream.Create;
        try
          StreamSize := HAPIBANK_GetItemSize(nil, nil, HAPIBANK);
          if StreamSize > 0 then
          begin
            Stream.SetSize(StreamSize);
            HAPIBANK_ReadBinData(nil, nil, HAPIBANK, StreamSize, Stream.Memory);
            Stream.Position := 0;
            SpawnedMinions.LoadFromStream(Stream);
          end;
        finally
          Stream.Free;
        end;
      end;
    end;

  end;
end;

procedure SaveGame_LoadAdditionHook;
asm
  pushAD
  push    esi
  call    SaveGame_LoadAddition
  popAD
  mov     edx, [TADynMemStructPtr]
  push $00432683
  call PatchNJump;
end;

procedure SaveGame_LoadNoScriptsFix;
label
  AvoidLoad;
asm
   mov     ecx, [esi+TUnitStruct.p_UnitScriptsData]
   test    ecx, ecx
   jz      AvoidLoad
   push $004875FF
   call PatchNJump;
AvoidLoad:
   push $00487605
   call PatchNJump;
end;

end.
