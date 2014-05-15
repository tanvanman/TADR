unit HealthBarMod;

interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_HealthBarMod : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallHealthBarMod;
Procedure OnUninstallHealthBarMod;

// -----------------------------------------------------------------------------

procedure HealthBarMod_FixBrushWidth;
procedure HealthBarMod_Dynamic_FixBrushWidth;
procedure HealthBarMod_Dynamic_UnitSizeX1X2;
//procedure HealthBarMod_DrawExtraBar;

implementation
uses
  IniOptions,
  TA_MemoryConstants,
  TA_MemoryStructures,
  TA_MemoryLocations,
  TA_FunctionsU;

const
  STANDARDUNIT : cardinal = 250;
  MEDIUMUNIT : cardinal = 1900;
  BIGUNIT : cardinal = 3000;
  HUGEUNIT : cardinal = 5000;
  EXTRALARGEUNIT : cardinal = 10000;

var
  HealthBarModPlugin: TPluginData;
  BrushWidth : Integer;
  CanvasWidth : Byte;
  CanvasHeight : Byte;
  x1, x2 : Byte;

Procedure OnInstallHealthBarMod;
begin
  if IniSettings.Plugin_HBDynamicSize then
  begin
    if IniSettings.Plugin_HBCategory1 <> 0 then
      STANDARDUNIT := IniSettings.Plugin_HBCategory1;
    if IniSettings.Plugin_HBCategory2 <> 0 then
      MEDIUMUNIT := IniSettings.Plugin_HBCategory2;
    if IniSettings.Plugin_HBCategory3 <> 0 then
      BIGUNIT := IniSettings.Plugin_HBCategory3;
    if IniSettings.Plugin_HBCategory4 <> 0 then
      HUGEUNIT := IniSettings.Plugin_HBCategory4;
    if IniSettings.Plugin_HBCategory5 <> 0 then
      EXTRALARGEUNIT := IniSettings.Plugin_HBCategory5;
  end;
end;

Procedure OnUninstallHealthBarMod;
begin
end;

function GetPlugin : TPluginData;
var
  i: byte;
begin
  if IsTAVersion31 and State_HealthBarMod then
  begin
    HealthBarModPlugin := TPluginData.create( True,
                            'Health Bar Mod',
                            State_HealthBarMod,
                            @OnInstallHealthBarMod,
                            @OnUninstallHealthBarMod );

    if IniSettings.Plugin_HBDynamicSize then
    begin
      BrushWidth := Byte(IniSettings.Plugin_HBWidth);

      HealthBarModPlugin.MakeRelativeJmp(State_HealthBarMod,
                            'Health Bar Mod Fix brush',
                            @HealthBarMod_Dynamic_UnitSizeX1X2,
                            $0046A45C, 0);

      HealthBarModPlugin.MakeRelativeJmp(State_HealthBarMod,
                            'Health Bar Mod Fix brush Dynamic',
                            @HealthBarMod_Dynamic_FixBrushWidth,
                            $0046A4B8, 0);   
    end else
    if IniSettings.Plugin_HBWidth <> -1 then
    begin
        BrushWidth := Byte(IniSettings.Plugin_HBWidth);
        // 1px borders
        CanvasWidth := BrushWidth + 2;
        i := CanvasWidth div 2;
        x1 := High(byte) - i + 1;
        x2 := i;

        HealthBarModPlugin.MakeReplacement(State_HealthBarMod,
                            'Health Bar Mod Canvas Width X1',
                            $0046A45E,
                            x1, 1);

        HealthBarModPlugin.MakeReplacement(State_HealthBarMod,
                            'Health Bar Mod Canvas Width X2',
                            $0046A461,
                            x2, 1);

        HealthBarModPlugin.MakeRelativeJmp(State_HealthBarMod,
                            'Health Bar Mod Fix brush',
                            @HealthBarMod_FixBrushWidth,
                            $0046A4B8, 0);
    end;
    
    if IniSettings.Plugin_HBHeight <> -1 then
    begin
      CanvasHeight := Byte(IniSettings.Plugin_HBHeight);
      HealthBarModPlugin.MakeReplacement(State_HealthBarMod,
                          'Health Bar Mod Height',
                          $0046A475,
                          CanvasHeight, 1);
    end;
{
    if IniSettings.Plugin_WeaponReloadTimeBar then
    begin
      HealthBarModPlugin.MakeRelativeJmp(State_HealthBarMod,
                            'Health Bar Mod drawing extra bars',
                            @HealthBarMod_DrawExtraBar,
                            //$00469CBE, 0);
                            $00469CAE, 0);
    end;
}
    Result:= HealthBarModPlugin;
  end else
    Result := nil;
end;

procedure HealthBarMod_FixBrushWidth;
asm
  mul     eax, BrushWidth
  div     dword ptr [esi+1FAh]
  push $0046A4C1;
  call PatchNJump;
end;

{
.text:0046A45C 020 8D 48 EF      lea     ecx, [eax-11h]
.text:0046A45F 020 83 C0 11      add     eax, 11h
}
procedure HealthBarMod_Dynamic_UnitSizeX1X2;
label
  UnitSize_Tiny,
  UnitSize_Standard,
  UnitSize_Medium,
  UnitSize_Big,
  UnitSize_Huge,
  UnitSize_ExtraLarge,
  BackToEngine;
asm
// ustawic brushwidth
  push    eax
  push    ecx
  mov     ecx, dword ptr [esi+$92]    // unitinfo struct
  xor     eax, eax
  mov     ax, word [ecx+$1FA]         // unitinfo.maxhealth
  mov     ecx, eax
  cmp     ecx, EXTRALARGEUNIT
  jge     UnitSize_ExtraLarge
  cmp     ecx, HUGEUNIT
  jge     UnitSize_Huge
  cmp     ecx, BIGUNIT
  jge     UnitSize_Big
  cmp     ecx, MEDIUMUNIT
  jge     UnitSize_Medium
  cmp     ecx, STANDARDUNIT
  jge     UnitSize_Standard
UnitSize_Tiny:
  mov     BrushWidth, 26
  pop     ecx
  pop     eax
  lea     ecx, [eax-14]
  add     eax, 14
  jmp BackToEngine
UnitSize_Standard:
  mov     BrushWidth, 32
  pop     ecx
  pop     eax
  lea     ecx, [eax-17]
  add     eax, 17
  jmp BackToEngine
UnitSize_Medium:
  mov     BrushWidth, 38
  pop     ecx
  pop     eax
  lea     ecx, [eax-20]
  add     eax, 20
  jmp BackToEngine
UnitSize_Big:
  mov     BrushWidth, 44
  pop     ecx
  pop     eax
  lea     ecx, [eax-23]
  add     eax, 23
  jmp BackToEngine
UnitSize_Huge:
  mov     BrushWidth, 50
  pop     ecx
  pop     eax
  lea     ecx, [eax-26]
  add     eax, 26
  jmp BackToEngine
UnitSize_ExtraLarge:
  mov     BrushWidth, 56
  pop     ecx
  pop     eax
  lea     ecx, [eax-29]
  add     eax, 29
BackToEngine:
  push $0046A462;
  call PatchNJump;
end;

procedure HealthBarMod_Dynamic_FixBrushWidth;
asm
  mul     eax, BrushWidth
  div     dword ptr [esi+1FAh]
  push $0046A4C1;
  call PatchNJump;
end;

type
  TDrawPos = record
    x1 : Cardinal;
    y1 : Cardinal;
    x2 : Cardinal;
    y2 : Cardinal;
  end;
{
function DrawBar(OFFSCREEN_ptr: Cardinal; UnitInGame: Cardinal; PosX: Cardinal; PosY: Cardinal) : Integer; stdcall;
var
  ColorsPal : Pointer;
//  UnitInfo : PGameUnitfInfo;
  v7 : Integer;
  DrawPos : TDrawPos;
  MaxReloadTime: Integer;
  CurReloadTime: Integer;
  WhichWeap: Byte;
begin
  Result := 0;
  WhichWeap := 0;
  if PUnitStruct(UnitInGame).p_Weapon1 <> nil then
    if PWeaponDef(PUnitStruct(UnitInGame).p_Weapon1).nReloadTime >= 4 * 30 then
      WhichWeap := 1;

  if PUnitStruct(UnitInGame).p_Weapon2 <> nil then
    if PWeaponDef(PUnitStruct(UnitInGame).p_Weapon2).nReloadTime >= 4 * 30 then
      WhichWeap := 2;

  if PUnitStruct(UnitInGame).p_Weapon3 <> nil then
    if PWeaponDef(PUnitStruct(UnitInGame).p_Weapon3).nReloadTime >= 4 * 30 then
      WhichWeap := 3;
      
  if WhichWeap <> 0 then
  begin
    case WhichWeap of
      1 : begin
            MaxReloadTime := PWeaponDef(PUnitStruct(UnitInGame).p_Weapon1).nReloadTime;
            CurReloadTime := MaxReloadTime - PUnitStruct(UnitInGame).Weapon1_ReloadTime;
          end;
      2 : begin
            MaxReloadTime := PWeaponDef(PUnitStruct(UnitInGame).p_Weapon2).nReloadTime;
            CurReloadTime := MaxReloadTime - PUnitStruct(UnitInGame).Weapon2_ReloadTime;
          end;
      3 : begin
            MaxReloadTime := PWeaponDef(PUnitStruct(UnitInGame).p_Weapon3).nReloadTime;
            CurReloadTime := MaxReloadTime - PUnitStruct(UnitInGame).Weapon3_ReloadTime;
          end;
    end;

    if CurReloadTime < 0 then CurReloadTime := 0;

    ColorsPal := Pointer(LongWord(TAData.MainStructPtr)+$DCB);
    DrawPos.x1 := PosX - 17;
    DrawPos.x2 := PosX + 17;
    DrawPos.y1 := PosY - 2;
    DrawPos.y2 := PosY + 2;
    DrawRectangle(OFFSCREEN_ptr, @DrawPos, PByte(ColorsPal)^);
    Inc(DrawPos.y1);
    //UnitHealth := PUnitStruct(UnitInGame).nHealth;
    //UnitInfo := PUnitStruct(UnitInGame).p_UnitDef;
    Dec(DrawPos.y2);
    Inc(DrawPos.x1);
    //DrawPos.x2 := Round(DrawPos.x1 + (32 * UnitHealth) / UnitInfo.nMaxHP);
    DrawPos.x2 := Round(DrawPos.x1 + (32 * CurReloadTime) / MaxReloadTime);
    v7 := Round((CurReloadTime / MaxReloadTime) * 100);
    case v7 of
      0..20  : result := DrawRectangle(OFFSCREEN_ptr, @DrawPos, PByte(LongWord(ColorsPal)+143)^);
      21..40 : result := DrawRectangle(OFFSCREEN_ptr, @DrawPos, PByte(LongWord(ColorsPal)+142)^);
      41..60 : result := DrawRectangle(OFFSCREEN_ptr, @DrawPos, PByte(LongWord(ColorsPal)+141)^);
      61..80 : result := DrawRectangle(OFFSCREEN_ptr, @DrawPos, PByte(LongWord(ColorsPal)+140)^);
      81..100 : result := DrawRectangle(OFFSCREEN_ptr, @DrawPos, PByte(LongWord(ColorsPal)+139)^);
    end;

    //10 red, 14 yellow, 12 green
  end;
end;

procedure HealthBarMod_DrawExtraBar;
asm
  lea     edx, [esi+0Ah]
  lea     eax, [esp+34h]
  push    edx             // PosY
  push    ebp             // PosX
  push    edi             // Unit
  push    eax             // OFFSCREEN_ptr
  call    DrawHealthBars
  lea     edx, [esi+0Ah]
  add     edx, 6
  lea     eax, [esp+34h]
  push    edx             // PosY
  push    ebp             // PosX
  push    edi             // Unit
  push    eax             // OFFSCREEN_ptr
  call    DrawBar
  push $00469CBE;
  call PatchNJump;
end;
}
end.

