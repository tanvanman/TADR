unit TAExceptionsLog;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_TAExceptionsLog : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallTAExceptionsLog;
Procedure OnUninstallTAExceptionsLog;

// -----------------------------------------------------------------------------

//procedure TAExceptionsLog_LogHAPINET;
procedure TAExceptionsLog_COBCallbackEmptyCOB;
procedure TAExceptionsLog_COBRunScriptEmptyCOB;
procedure TAExceptionsLog_AIBuildUnitInfoTooHigh;
//procedure TAExceptionsLog_MissingObjectState;

implementation
uses
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_FunctionsU,
  TA_MemoryLocations,
  IniOptions,
  PauseLock,
  SysUtils,
  logging;

var
  TAExceptionsLogPlugin: TPluginData;

Procedure OnInstallTAExceptionsLog;
begin
end;

Procedure OnUninstallTAExceptionsLog;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_TAExceptionsLog then
  begin
    TAExceptionsLogPlugin := TPluginData.create( false,
                            '',
                            State_TAExceptionsLog,
                            @OnInstallTAExceptionsLog,
                            @OnUnInstallTAExceptionsLog );
    {
    TAExceptionsLogPlugin.MakeRelativeJmp( State_TAExceptionsLog,
                          'log HAPINET',
                          @TAExceptionsLog_LogHAPINET,
                          $004C9740, 0);
    }
    TAExceptionsLogPlugin.MakeRelativeJmp( State_TAExceptionsLog,
                          'prevent crash when pointer to COB is nil',
                          @TAExceptionsLog_COBCallbackEmptyCOB,
                          $004B0BC0, 0);

    TAExceptionsLogPlugin.MakeRelativeJmp( State_TAExceptionsLog,
                          'prevent crash when pointer to COB is nil',
                          @TAExceptionsLog_COBRunScriptEmptyCOB,
                          $004B0A70, 0);

    if IniSettings.Plugin_AiNukes then
      TAExceptionsLogPlugin.MakeRelativeJmp( State_TAExceptionsLog,
                            'AI build probability unit type too high',
                            @TAExceptionsLog_AIBuildUnitInfoTooHigh,
                            $0040BDE1, 0);

 {   TAExceptionsLogPlugin.MakeRelativeJmp( State_TAExceptionsLog,
                          'prevent crash when pointer to COB is nil',
                          @TAExceptionsLog_MissingObjectState,
                          $0048B4A4, 0);  }

    Result:= TAExceptionsLogPlugin;
  end else
    Result := nil;
end;

procedure COBCallbackEmptyCOB_Message(a4, a5, a6, a7: Cardinal; ScriptName: PAnsiChar; a1, a2 : Cardinal); stdcall;
var
  msg :string;
begin
  SetPausedState(True, True);
  msg := 'Error in processing callback of unit script' +#10#13+
         'a1: ' + IntToHex(a1, 8) +#10#13+
         'a2: ' + IntToHex(a2, 8) +#10#13+
         'a4: ' + IntToHex(a4, 8) +#10#13+
         'a5: ' + IntToHex(a5, 8) +#10#13+
         'a6: ' + IntToHex(a6, 8) +#10#13+
         'a7: ' + IntToHex(a7, 8) +#10#13+
         'script: '+ ScriptName;
  Msg_Reminder(PAnsiChar(msg), 1);
end;
{
procedure HAPINET_Log(LogMessage: PAnsiChar); stdcall;
begin
  TLog.Add(0, 'HAPINET log: ' + LogMessage);
end;

procedure TAExceptionsLog_LogHAPINET;
asm
  push ecx
  mov  ecx, [esp+$8]
  push ecx
  call HAPINET_Log
  pop  ecx
  retn
end;
}
procedure TAExceptionsLog_COBCallbackEmptyCOB;
label
  CorrectCOBCall;
asm
    push    ecx
    test    ecx, ecx
    jnz CorrectCOBCall
    //esp+8 scriptname
    //eax arg1
    //edx arg2
    mov     ecx, [esp+8]
    push    edx  //a2
    push    eax  //a1
    push    ecx  // scriptname
    // 16
    mov     ecx, [esp+36]
    mov     ebx, [esp+32]
    mov     eax, [esp+28]
    mov     edx, [esp+24]
    push    ecx
    push    ebx
    push    eax
    push    edx
    call COBCallbackEmptyCOB_Message
    add     esp, 4
    push $004B0C39;
    call PatchNJump;
CorrectCOBCall :
    mov     eax, [ecx+8]
    push    ebx
    push $004B0BC5;
    call PatchNJump;
end;

procedure LogMissingCob(ActionName : PAnsiChar; a2: Cardinal); stdcall;
var
  UnitPtr : Pointer;
  UnitInfo : PUnitInfo;
begin
  UnitPtr := nil;
  UnitInfo := nil;
  if ActionName = 'SetMaxReloadTime' then
  begin
    UnitPtr := Pointer(a2 - $73);
    if PUnitStruct(UnitPtr).p_UnitDef <> nil then
      UnitInfo := PUnitStruct(UnitPtr).p_UnitDef;
  end;
  if UnitPtr <> nil then
  begin
    if UnitInfo <> nil then
      TLog.Add(0, 'Error in processing script callback. UnitType: ' + UnitInfo.szUnitName + ', Script name: ' + ActionName)
    else
      TLog.Add(0, 'Error in processing script callback. UnitID: ' + IntToStr(PUnitStruct(UnitPtr).lUnitInGameIndex) + ', Script name: ' + ActionName);
  end else
    TLog.Add(0, 'Error in processing script callback. Script name: ' + ActionName);
end;

procedure TAExceptionsLog_COBRunScriptEmptyCOB;
label
  CorrectCOBCall;
asm
    push    ecx
    pushf
    test    ecx, ecx
    jnz CorrectCOBCall
    popf
    mov     ecx, [esp+4h+$4]
    push    esi
    push    ecx
    call LogMissingCob
    push $004B0AF7;
    call PatchNJump;
CorrectCOBCall :
    popf
    mov     eax, [ecx+8]
    push    ebx
    push $004B0A75;
    call PatchNJump;
end;

{
procedure TAExceptionsLog_MissingObjectState;
asm
  mov     ecx, esi
  mov     [eax+10h], ebp
end;
}

procedure LogAITooHighUnitInfo(CallerUnitInfo: PUnitInfo; UnitTypeID : Cardinal); stdcall;
begin
  TLog.Add(0, 'Error in AI build probability list test. Fault unit type: ' + CallerUnitInfo.szUnitName + '. Tried to build: ' + IntToStr(UnitTypeID));
end;

procedure TAExceptionsLog_AIBuildUnitInfoTooHigh;
label
  UnitInfoTooHigh;
asm
    mov     di, [ecx+ebx*2]
    push    eax
    push    edx
    mov     eax, [TAdynMemStructPtr]
    mov     edx, [eax+TTAdynmemStruct.lNumUnitTypeDefs]
    cmp     di, dx
    jg      UnitInfoTooHigh
    pop     edx
    pop     eax
    push    edi
    push $0040BDE6;
    call PatchNJump;
UnitInfoTooHigh :
    pop     edx
    pop     eax
    push    edi
    push    eax
    call    LogAITooHighUnitInfo
    xor     edi, edi
    push $0040BE9A;
    call PatchNJump;
end;

end.

