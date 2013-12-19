unit Colors;
{
  Memory locations provided by Admiral_94
  Based on his topic: http://www.tauniverse.com/forum/showthread.php?t=43867
}
interface
uses
  PluginEngine;

// -----------------------------------------------------------------------------

const
  State_Colors : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallColors;
Procedure OnUninstallColors;
// -----------------------------------------------------------------------------

Const
  FIRST_GUIPAL_COLOR = $0DCB;
  MEMRUNTIME = $400C00;
  UNIT_SELECTIONBOX = $0;
  UNIT_HEALTHBARGOOD = $1;
  UNIT_HEALTHBARMEDIUM = $2;
  UNIT_HEALTHBARLOW = $3;
  BUILDQUEUEBOX_SELECTED1 = $4;
  BUILDQUEUEBOX_SELECTED2 = $5;
  BUILDQUEUEBOX_NONSELECTED1 = $6;
  BUILDQUEUEBOX_NONSELECTED2 = $7;
  LOADBARS_TEXTURESREADY = $8;
  LOADBARS_TEXTURESLOADING = $9;
  LOADBARS_TERRAINREADY = $0A;
  LOADBARS_TERRAINLOADING = $0B;
  LOADBARS_UNITSREADY = $0C;
  LOADBARS_UNITSLOADING = $0D;
  LOADBARS_ANIMATIONSREADY = $0E;
  LOADBARS_ANIMATIONSLOADING = $0F;
  LOADBARS_3DDATAREADY = $10;
  LOADBARS_3DDATALOADING = $11;
  LOADBARS_EXPLOSIONSREADY = $12;
  LOADBARS_EXPLOSIONSLOADING = $13;
  MAINMENUDOTS = $14;
  NANOLATHEPARTICLE_BASE = $15;
  NANOLATHEPARTICLE_COLORS = $16;
  UNDERCONSTRUCT_SURFACELO = $17;
  UNDERCONSTRUCT_SURFACEHI = $18;
  UNDERCONSTRUCT_OUTLINELO = $19;
  UNDERCONSTRUCT_OUTLINEHI = $1A;
  MAINMENUDOTSDISABLED = $1B;

// -----------------------------------------------------------------------------

implementation
uses
  INI_Options,
  TADemoConsts,
  TA_MemoryLocations,
  SysUtils,
  windows;

var
  ColorsPlugin: TPluginData;

procedure setColor(swapType: byte; address: Integer; i: integer; colorByte: PByte = nil; colorWord: PWord = nil);
begin
  case swaptype of
    { just a color number }
    1: begin
        if i <> -1 then
          colorByte:= PByte(iniSettings.Colors[i]);
        if colorByte <> nil then
          ColorsPlugin.MakeReplacement( State_Colors, IntToStr(address), MEMRUNTIME + Address, colorByte, 1);
       end;
    { color number as offset from first guipal color }
    2: begin
        if i <> -1 then
          colorWord:= PWord(FIRST_GUIPAL_COLOR + iniSettings.Colors[i]);
        if colorWord <> nil then
          ColorsPlugin.MakeReplacement( State_Colors, IntToStr(address), MEMRUNTIME + Address, colorWord, 2);
       end;
    3: begin
        if i <> -1 then
          ColorsPlugin.MakeNOPReplacement ( State_Colors, IntToStr(address), MEMRUNTIME + Address, i);
       end;
  end;
end;

Procedure OnInstallColors;
var
  i: integer;
begin
  for i:= Low(iniSettings.Colors) to High(iniSettings.Colors) do
  begin
    if iniSettings.Colors[i] <> -1 then
    begin
      case i of
        UNIT_SELECTIONBOX : begin setColor(2, $66E70, i); end;
        UNIT_HEALTHBARGOOD : begin setColor(1, $698E3, i); end;
        UNIT_HEALTHBARMEDIUM : begin setColor(1, $69902, i); end;
        UNIT_HEALTHBARLOW : begin setColor(1, $6991D, i); end;
        BUILDQUEUEBOX_SELECTED1 : begin setColor(2, $38163, i); end;
        BUILDQUEUEBOX_SELECTED2 : begin setColor(2, $3815D, i); end;
        BUILDQUEUEBOX_NONSELECTED1 : begin setColor(2, $38179, i); end;
        BUILDQUEUEBOX_NONSELECTED2 : begin setColor(2, $38173, i); end;
        LOADBARS_TEXTURESREADY : begin setColor(1, $97B6B, i); end;
        LOADBARS_TEXTURESLOADING : begin setColor(1, $97B68, i); end;
        LOADBARS_TERRAINREADY: begin setColor(1, $97C48, i); end;
        LOADBARS_TERRAINLOADING: begin setColor(1, $97C45, i); end;
        LOADBARS_UNITSREADY: begin setColor(1, $97D1C, i); end;
        LOADBARS_UNITSLOADING: begin setColor(1, $97D19, i); end;
        LOADBARS_ANIMATIONSREADY: begin setColor(1, $97DF0, i); end;
        LOADBARS_ANIMATIONSLOADING: begin setColor(1, $97DED, i); end;
        LOADBARS_3DDATAREADY: begin setColor(1, $97EC4, i); end;
        LOADBARS_3DDATALOADING: begin setColor(1, $97EC1, i); end;
        LOADBARS_EXPLOSIONSREADY: begin setColor(1, $97FBD, i); end;
        LOADBARS_EXPLOSIONSLOADING: begin setColor(1, $97FBA, i); end;
        MAINMENUDOTS: begin setColor(1, $25056, i); end;
        NANOLATHEPARTICLE_BASE: begin setColor(1, $7333D, i); end;
        NANOLATHEPARTICLE_COLORS: begin setColor(1, $72DD6, i); end;
        UNDERCONSTRUCT_SURFACELO: begin setColor(1, $5826C, i); end;
        UNDERCONSTRUCT_SURFACEHI: begin setColor(1, $5825F, i); end;
        UNDERCONSTRUCT_OUTLINELO: begin setColor(1, $58288, i); end;
        UNDERCONSTRUCT_OUTLINEHI: begin setColor(1, $5827B, i); end;
        MAINMENUDOTSDISABLED: begin setColor(3, $25840, 7); end;
      end;
    end;
  end;
end;

Procedure OnUninstallColors;
begin
end;

function GetPlugin : TPluginData;
begin
if IsTAVersion31 and State_Colors then
  begin

  ColorsPlugin := TPluginData.create( false,
                                'Colors',
                                State_Colors,
                                @OnInstallColors, @OnUnInstallColors );
  Result:= ColorsPlugin;

  end
else
  result := nil;  
end;

end.
