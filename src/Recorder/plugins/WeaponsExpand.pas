unit WeaponsExpand;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_WeaponsExpand: Boolean = True;

function GetPlugin: TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallWeaponsExpand;
Procedure OnUninstallWeaponsExpand;

// -----------------------------------------------------------------------------

implementation
uses
  IniOptions,
  Classes,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_FunctionsU,
  TA_MemoryLocations,
  logging;

procedure WeaponsExpand_NewPropertiesLoad(TDFHandle: Cardinal; WeaponID: Cardinal); stdcall;
var
  Intercepts: array[0..1023] of AnsiChar;
begin
  ExtraWeaponDefTags[WeaponID].HighTrajectory := TdfFile_GetInt(0, 0, TDFHandle, 0, PAnsiChar('hightrajectory')) <> 0;
  ExtraWeaponDefTags[WeaponID].MaxBarrelAngle := TdfFile_GetFloat(0, 0, TDFHandle, 0, PAnsiChar('maxbarrelangle'));
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

Procedure OnInstallWeaponsExpand;
begin
  SetLength(ExtraWeaponDefTags, 256);
end;

Procedure OnUninstallWeaponsExpand;
begin
end;

function GetPlugin: TPluginData;
begin
  if IsTAVersion31 and State_WeaponsExpand then
  begin
    Result := TPluginData.Create( False,
                          '',
                          State_WeaponsExpand,
                          @OnInstallWeaponsExpand,
                          @OnUnInstallWeaponsExpand );

    Result.MakeRelativeJmp( State_WeaponsExpand,
                          'Load new weapon tags',
                          @WeaponsExpand_NewPropertiesLoadHook,
                          $0042E46E, 1);
  end else
    Result := nil;
end;

end.

