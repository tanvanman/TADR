unit UnitLimit;
{
Ups the unit limit from 500 to 1500 or 5000

}
interface
uses
  PluginEngine;
  
// -----------------------------------------------------------------------------

Procedure OnInstallUnitLimit;
Procedure OnUninstallUnitLimit;

// -----------------------------------------------------------------------------

const 
  NewUnitLimit : word = 1500;

// -----------------------------------------------------------------------------

const
  State_UnitLimit : boolean = true;
  
function GetPlugin : TPluginData;
  
implementation
uses
  sysutils,
  TADemoConsts,
  TA_MemoryLocations,
  IniOptions;

Procedure OnInstallUnitLimit;
begin
// reload the unit limit from the ini, since
{
.text:0049163A mov     eax, TAdynmemStructPtr
.text:0049163F push    5DCh
.text:00491644 push    offset aUnitlimit               ; "UnitLimit"
.text:00491649 mov     dword ptr [eax+TAdynmemStruct.AlteredUnitLimit], 1
.text:00491653 call    ReadIniFileValue
.text:00491658 cmp     eax, 5DCh
.text:0049165D jle     short loc_491678
.text:0049165F mov     ecx, TAdynmemStructPtr
.text:00491665 mov     eax, 5DCh
.text:0049166A mov     [ecx+TAdynmemStruct.MaxUnitLimit], ax
.text:00491671 pop     edi
.text:00491672 pop     esi
.text:00491673 pop     ebx
.text:00491674 add     esp, 24h
.text:00491677 retn
.text:00491678 ; ---------------------------------------------------------------------------
.text:00491678 
.text:00491678 loc_491678:                             ; CODE XREF: sub_491200+45Dj
.text:00491678 cmp     eax, 14h
.text:0049167B jge     short loc_491682
.text:0049167D mov     eax, 14h
.text:00491682 
.text:00491682 loc_491682:                             ; CODE XREF: sub_491200+47Bj
.text:00491682 mov     ecx, TAdynmemStructPtr
.text:00491688 pop     edi
.text:00491689 pop     esi
.text:0049168A pop     ebx
.text:0049168B mov     [ecx+TAdynmemStruct.MaxUnitLimit], ax
.text:00491692 add     esp, 24h

}
end; 

Procedure OnUninstallUnitLimit;
begin
end;

function GetPlugin : TPluginData;
var
  aUnitLimit : Word;
begin
if IsTAVersion31 and State_UnitLimit then
  begin
  if IniSettings.UnitLimit <> 0 then
    aUnitLimit := Word(IniSettings.UnitLimit)
  else
  begin
    IniSettings.UnitLimit := NewUnitLimit;
    aUnitLimit := NewUnitLimit;
  end;

  result := TPluginData.create( true,
                                IntToStr(NewUnitLimit)+' unit limit',
                                State_UnitLimit,
                                @OnInstallUnitLimit, @OnUnInstallUnitLimit );
  result.MakeReplacement( State_UnitLimit,
                          'Default unit limit',
                          $491640,
                          aUnitLimit,sizeof(aUnitLimit));
  result.MakeReplacement( State_UnitLimit,
                          'Unit Limit Compare',
                          $491659,
                          aUnitLimit,sizeof(aUnitLimit));
  result.MakeReplacement( State_UnitLimit,
                          'Unit Limit SetMax',
                          $491666,
                          aUnitLimit,sizeof(aUnitLimit));

  end
else
  result := nil;  
end;  

end.
 