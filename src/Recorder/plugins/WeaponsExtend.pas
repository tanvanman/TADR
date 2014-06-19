unit WeaponsExtend;
{
 extend weapons TDF fields
 finished :
 hightrajectory   - enables high trajectory for ballistic weapons

 not finished :
 watertoground    - from sub water level to air/ground weapons. (requires cursor fix)
 noairweapon      - dont shoot air units (only cursor fixed, need to revise units auto aim proc) 
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
  TA_MemoryLocations;

const
  WeapInfo_HighTrajectory : AnsiString = 'hightrajectory';
  WeapInfo_WaterToGround : AnsiString = 'watertoground';
  WeapInfo_NoAirWeapon : AnsiString = 'noairweapon';
  
var
  WeaponsExtendPlugin: TPluginData;

Procedure OnInstallWeaponsExtend;
begin
  if IniSettings.WeaponType > 256 then
    WeaponLimitPatchArr := Pointer(PCardinal(PCardinal($0042CDCE)^)^);  // get ptr to ddraw's weapon id patch array
  SetLength(ExtraWeaponTags, IniSettings.WeaponType);
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

function WeaponPropertyPutIntoArray(PropertyType : Integer; WeaponID : Cardinal; AValue : Integer): Integer; stdcall;
begin
  Result := WeaponID;
  case PropertyType of
    1 : ExtraWeaponTags[WeaponID].HighTrajectory := AValue;
    2 : ExtraWeaponTags[WeaponID].WaterToGround := AValue;
    3 : ExtraWeaponTags[WeaponID].NoAirWeapon := AValue;
  end;
end;

procedure WeaponsExtend_NewPropertiesLoadHook;
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
    call    WeaponPropertyPutIntoArray        // i dint know how to access records array :c
    pop     edx
    mov     ecx, ebx

    push    edx
    push    eax
    push    0
    push    WeapInfo_WaterToGround
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
    push    WeapInfo_NoAirWeapon
    call    TdfFile__GetInt
    pop     ecx
    push    eax
    push    ecx
    push    3
    call    WeaponPropertyPutIntoArray
    pop     edx
    mov     ecx, ebx

    popAD
    lea     ecx, [eax+eax*2]
    shl     ecx, 3
    push $0042E474;
    call PatchNJump;
end;

end.
 
