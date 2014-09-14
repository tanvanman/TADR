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

// -----------------------------------------------------------------------------

implementation
uses
  IniOptions,
  TADemoConsts,
  TA_MemoryLocations,
  SysUtils,
  windows;

var
  ColorsPlugin: TPluginData;

procedure SetColor(swaptype: byte; address: Integer; i: integer; arridx: byte; colorbyte: PByte = nil; colorword: PWord = nil);
begin
  case swaptype of
    { just a color number }
    1: begin
        if i <> -1 then
          colorByte:= PByte(iniSettings.Colors[arridx][i]);
        if colorByte <> nil then
          ColorsPlugin.MakeReplacement( State_Colors, IntToStr(address), Address, colorByte, 1);
       end;
    { color number as offset from first guipal color }
    2: begin
        if i <> -1 then
          colorWord:= PWord(FIRST_GUIPAL_COLOR + iniSettings.Colors[arridx][i]);
        if colorWord <> nil then
          ColorsPlugin.MakeReplacement( State_Colors, IntToStr(address), Address, colorWord, 2);
       end;
    3: begin
        if i <> -1 then
          ColorsPlugin.MakeNOPReplacement ( State_Colors, IntToStr(address), Address, i);
       end;
  end;
end;

Procedure OnInstallColors;
var
  i: integer;
begin
  for i:= Low(IniSettings.Colors) to High(IniSettings.Colors) do
  begin
    if IniSettings.Colors[0][i] <> 0 then
    begin
      case i of
      Ord(UNITSELECTIONBOX) : begin SetColor(2, $467A70, i, 0); end;
      Ord(BUILDQUEUEBOXSELECTED1) : begin SetColor(2, $438D63, i, 0); end;
      Ord(BUILDQUEUEBOXSELECTED2) : begin SetColor(2, $438D5D, i, 0); end;
      Ord(BUILDQUEUEBOXNONSELECTED1) : begin SetColor(2, $438D79, i, 0); end;
      Ord(BUILDQUEUEBOXNONSELECTED2) : begin SetColor(2, $438D73, i, 0); end;
      Ord(LOADBARSTEXTURESREADY) : begin SetColor(1, $49876B, i, 0); end;
      Ord(LOADBARSTEXTURESLOADING) : begin SetColor(1, $498768, i, 0); end;
      Ord(LOADBARSTERRAINREADY): begin SetColor(1, $498848, i, 0); end;
      Ord(LOADBARSTERRAINLOADING): begin SetColor(1, $498845, i , 0); end;
      Ord(LOADBARSUNITSREADY): begin SetColor(1, $49891C, i,0 ); end;
      Ord(LOADBARSUNITSLOADING): begin SetColor(1, $498919, i,0); end;
      Ord(LOADBARSANIMATIONSREADY): begin SetColor(1, $4989F0, i,0); end;
      Ord(LOADBARSANIMATIONSLOADING): begin SetColor(1, $4989ED, i,0); end;
      Ord(LOADBARS3DDATAREADY): begin SetColor(1, $498AC4, i,0); end;
      Ord(LOADBARS3DDATALOADING): begin SetColor(1, $498AC1, i,0); end;
      Ord(LOADBARSEXPLOSIONSREADY): begin SetColor(1, $498BBD, i,0); end;
      Ord(LOADBARSEXPLOSIONSLOADING): begin SetColor(1, $498BBA, i,0); end;
      Ord(MAINMENUDOTS): begin SetColor(1, $425C56, i,0); end;
      Ord(NANOLATHEPARTICLEBASE): begin SetColor(1, $473F3D, i,0); end;
      Ord(NANOLATHEPARTICLECOLORS): begin SetColor(1, $4739D6, i,0); end;
      Ord(UNDERCONSTRUCTSURFACELO): begin SetColor(1, $458E6C, i,0); end;
      Ord(UNDERCONSTRUCTSURFACEHI): begin SetColor(1, $458E5F, i,0); end;
      Ord(UNDERCONSTRUCTOUTLINELO): begin SetColor(1, $458E88, i,0); end;
      Ord(UNDERCONSTRUCTOUTLINEHI): begin SetColor(1, $458E7B, i,0); end;
      Ord(MAINMENUDOTSDISABLED): begin SetColor(3, $426440, 7,0); end;
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
