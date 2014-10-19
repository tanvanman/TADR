unit WeaponsExpand;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_WeaponsExpand : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallWeaponsExpand;
Procedure OnUninstallWeaponsExpand;

// -----------------------------------------------------------------------------

procedure WeaponsExpand_NewPropertiesLoadHook;

implementation
uses
  IniOptions,
  Classes,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_FunctionsU,
  TA_MemoryLocations,
  logging;

var
  WeaponsExpandPlugin: TPluginData;

Procedure OnInstallWeaponsExpand;
begin
  if IniSettings.WeaponType > 256 then
    WeaponLimitPatchArr := Pointer(PCardinal(PCardinal($0042CDCE)^)^);  // get ptr to ddraw's weapon id patch array
  SetLength(ExtraWeaponDefTags, IniSettings.WeaponType);
end;

Procedure OnUninstallWeaponsExpand;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_WeaponsExpand then
  begin
    WeaponsExpandPlugin := TPluginData.create( false,
                            '',
                            State_WeaponsExpand,
                            @OnInstallWeaponsExpand,
                            @OnUnInstallWeaponsExpand );

    WeaponsExpandPlugin.MakeRelativeJmp( State_WeaponsExpand,
                          'Load new weapon tags',
                          @WeaponsExpand_NewPropertiesLoadHook,
                          $0042E46E, 1);

    Result:= WeaponsExpandPlugin;
  end else
    Result := nil;
end;

procedure WeaponsExpand_NewPropertiesLoad(TDFHandle: Cardinal; WeaponID: Cardinal); stdcall;
var
  Intercepts: array[0..1023] of AnsiChar;
begin
  ExtraWeaponDefTags[WeaponID].HighTrajectory := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('hightrajectory'));
  ExtraWeaponDefTags[WeaponID].PreserveAccuracy := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('preserveaccuracy'));
  ExtraWeaponDefTags[WeaponID].NotAirWeapon := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('notairweapon'));
  TdfFile_GetStr(0, 0, TDFHandle, Pointer(Null_str), $40, PAnsiChar('weapontype2'), @ExtraWeaponDefTags[WeaponID].WeaponType2);
  if TdfFile_GetStr(0, 0, TDFHandle, Pointer(Null_str), $400, PAnsiChar('intercepts'), @Intercepts) <> 0 then
  begin
    ExtraWeaponDefTags[WeaponID].Intercepts := TStringlist.Create;
    ExtraWeaponDefTags[WeaponID].Intercepts.DelimitedText := Intercepts;
  end;
end;

procedure WeaponsExpand_NewPropertiesLoadHook;
asm
    pushAD
    push    eax
    push    ecx
    call    WeaponsExpand_NewPropertiesLoad
    popAD
    lea     ecx, [eax+eax*2]
    shl     ecx, 3
    push $0042E474;
    call PatchNJump;
end;

end.

