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

procedure WeaponsExpand_NewPropertiesLoadHook;
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
    push    Null_str                 // null str
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

