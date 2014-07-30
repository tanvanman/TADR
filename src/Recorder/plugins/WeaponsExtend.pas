unit WeaponsExtend;
{
 extend weapons TDF fields

 hightrajectory   - enables high trajectory for ballistic weapons
 preserveaccuracy - disables accuracy advantage for units with more than 12 kills,
                    usefull for vulcan etc. so it still shoots in "spray mode"
}

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_WeaponsExtend : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallWeaponsExtend;
Procedure OnUninstallWeaponsExtend;

// -----------------------------------------------------------------------------

procedure WeaponsExtend_NewPropertiesLoadHook;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_FunctionsU,
  TA_MemoryLocations,
  logging;

const
  WeapInfo_HighTrajectory : AnsiString = 'hightrajectory';
  WeapInfo_PreserveAccuracy : AnsiString = 'preserveaccuracy';
  WeapInfo_NotAirWeapon : AnsiString = 'notairweapon';
  WeapInfo_WeaponType2 : AnsiString = 'weapontype2';

var
  WeaponsExtendPlugin: TPluginData;

Procedure OnInstallWeaponsExtend;
begin
  if IniSettings.WeaponType > 256 then
    WeaponLimitPatchArr := Pointer(PCardinal(PCardinal($0042CDCE)^)^);  // get ptr to ddraw's weapon id patch array
  SetLength(ExtraWeaponDefTags, IniSettings.WeaponType);
end;

Procedure OnUninstallWeaponsExtend;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_WeaponsExtend then
  begin
    WeaponsExtendPlugin := TPluginData.create( false,
                            '',
                            State_WeaponsExtend,
                            @OnInstallWeaponsExtend,
                            @OnUnInstallWeaponsExtend );

    WeaponsExtendPlugin.MakeRelativeJmp( State_WeaponsExtend,
                          'Load new weapon tags',
                          @WeaponsExtend_NewPropertiesLoadHook,
                          $0042E46E, 1);

    Result:= WeaponsExtendPlugin;
  end else
    Result := nil;
end;

var
  TempString : array[0..63] of AnsiChar;
function WeaponPropertyPutIntoArray(PropertyType : Integer; WeapID : Cardinal; AValue : Cardinal): Integer; stdcall;
begin
  Result := WeapID;
  case PropertyType of
    1 : ExtraWeaponDefTags[WeapID].HighTrajectory := AValue;
    2 : ExtraWeaponDefTags[WeapID].PreserveAccuracy := AValue;
    3 : ExtraWeaponDefTags[WeapID].NotAirWeapon := AValue;
    4 : ExtraWeaponDefTags[WeapID].WeaponType2 := TempString;
  end;
  TempString := '';
end;

procedure WeaponsExtend_NewPropertiesLoadHook;
label
  NoWeaponType2;
asm
    //mov     edx, [TADynmemStructPtr]
    pushAD

    push    edx
    push    eax
    push    0
    push    WeapInfo_HighTrajectory
    call    TdfFile__GetInt
    // result in eax
    pop     ecx
    // weap id in ecx
    push    eax
    push    ecx
    push    1
    call    WeaponPropertyPutIntoArray
    pop     edx
    mov     ecx, ebx

    push    edx
    push    eax
    push    0
    push    WeapInfo_PreserveAccuracy
    call    TdfFile__GetInt
    pop     ecx
    push    eax
    push    ecx
    push    2
    call    WeaponPropertyPutIntoArray
    pop     edx
    mov     ecx, ebx

    push    edx
    push    eax
    push    0
    push    WeapInfo_NotAirWeapon
    call    TdfFile__GetInt
    pop     ecx
    push    eax
    push    ecx
    push    3
    call    WeaponPropertyPutIntoArray
    pop     edx
    mov     ecx, ebx

    push    ebx
    push    edx
    push    eax
    push    $005119B8                // null str
    push    $40                      // buff len
    push    WeapInfo_WeaponType2
    lea     ebx, TempString
    push    ebx
    call    TdfFile__GetStr
    test    eax, eax
    pop     eax
    jz      NoWeaponType2
    push    ebx
    push    eax
    push    4
    call    WeaponPropertyPutIntoArray
NoWeaponType2 :
    pop     edx
    pop     ebx
    mov     ecx, ebx

    popAD
    lea     ecx, [eax+eax*2]
    shl     ecx, 3
    push $0042E474;
    call PatchNJump;
end;

end.

