unit MultiAILimit;
{
Ups the unit limit from 500 to 1500 or 5000

}
interface
uses
  PluginEngine;
  
// -----------------------------------------------------------------------------

Procedure OnInstall;
Procedure OnUninstall;

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

const
  State_MultiAILimit : boolean = true;
  
function GetPlugin : TPluginData;
  
implementation
uses
  sysutils,
  TADemoConsts,
  TA_MemoryLocations;

Procedure OnInstall;
begin
end;

Procedure OnUninstall;
begin
end;

{
Nuke jump(6 bytes) to allow more than 1 AI.

.text:00447D80 call    Game_GetLocalAICount
.text:00447D85 test    eax, eax
.text:00447D87 jnz     loc_447E27




Optional; Modify AI's name to include number at:

.text:0045130F push    ecx
.text:00451310 push    offset aAiS                     ; "AI:%s"
.text:00451315 push    edx
.text:00451316 call    _sprintf


replace "_sprintf" with safe version
}

var
  AIName : string = 'AI:%s %d';
procedure BetterAIName( AIPlayerSlot : integer; name : pchar; buffer : pchar) stdcall;
var
  len : integer;
begin
buffer[16] := #0;
len := FormatBuf(buffer^,16,AIName[1],length(AIName),[name,AIPlayerSlot]);
buffer[len] := #0;
end;

procedure BetterAINameStub;
asm
  mov eax, [esp+$118+4];

  push edx
  push ecx
  push eax

  call BetterAIName


  push $451323;
  call PatchNJump;
end;


function GetPlugin : TPluginData;
begin
if IsTAVersion31 and State_MultiAILimit then
  begin
  result := TPluginData.create( true,
                                'Multi AI Limit',
                                State_MultiAILimit,
                                @OnInstall, @OnUnInstall );
  result.MakeNOPReplacement(State_MultiAILimit,'Remove 1 AI check',$447D87,6);
  result.MakeRelativeJmp(State_MultiAILimit,'Better AI naming',@BetterAINameStub,$45130F,1);

  end
else
  result := nil;
end;  

end.
 