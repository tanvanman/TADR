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

procedure SetColor(swaptype: byte; address: Integer; i: integer; colorbyte: PByte = nil; colorword: PWord = nil);
begin
  case swaptype of
    { just a color number }
    1: begin
        if i <> -1 then
          colorByte:= PByte(iniSettings.Colors[i]);
        if colorByte <> nil then
          ColorsPlugin.MakeReplacement( State_Colors, IntToStr(address), Address, colorByte, 1);
       end;
    { color number as offset from first guipal color }
    2: begin
        if i <> -1 then
          colorWord:= PWord(FIRST_GUIPAL_COLOR + iniSettings.Colors[i]);
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
    if IniSettings.Colors[i] <> -1 then
    begin
      case i of
      Ord(UNITSELECTIONBOX) : begin SetColor(2, $467A70, i); end;
      Ord(UNITHEALTHBARGOOD) : begin SetColor(1, $46A4E3, i); end;
      Ord(UNITHEALTHBARMEDIUM) : begin SetColor(1, $46A502, i); end;
      Ord(UNITHEALTHBARLOW) : begin SetColor(1, $46A51D, i); end;
      Ord(BUILDQUEUEBOXSELECTED1) : begin SetColor(2, $438D63, i); end;
      Ord(BUILDQUEUEBOXSELECTED2) : begin SetColor(2, $438D5D, i); end;
      Ord(BUILDQUEUEBOXNONSELECTED1) : begin SetColor(2, $438D79, i); end;
      Ord(BUILDQUEUEBOXNONSELECTED2) : begin SetColor(2, $438D73, i); end;
      Ord(LOADBARSTEXTURESREADY) : begin SetColor(1, $49876B, i); end;
      Ord(LOADBARSTEXTURESLOADING) : begin SetColor(1, $498768, i); end;
      Ord(LOADBARSTERRAINREADY): begin SetColor(1, $498848, i); end;
      Ord(LOADBARSTERRAINLOADING): begin SetColor(1, $498845, i); end;
      Ord(LOADBARSUNITSREADY): begin SetColor(1, $49891C, i); end;
      Ord(LOADBARSUNITSLOADING): begin SetColor(1, $498919, i); end;
      Ord(LOADBARSANIMATIONSREADY): begin SetColor(1, $4989F0, i); end;
      Ord(LOADBARSANIMATIONSLOADING): begin SetColor(1, $4989ED, i); end;
      Ord(LOADBARS3DDATAREADY): begin SetColor(1, $498AC4, i); end;
      Ord(LOADBARS3DDATALOADING): begin SetColor(1, $498AC1, i); end;
      Ord(LOADBARSEXPLOSIONSREADY): begin SetColor(1, $498BBD, i); end;
      Ord(LOADBARSEXPLOSIONSLOADING): begin SetColor(1, $498BBA, i); end;
      Ord(MAINMENUDOTS): begin SetColor(1, $425C56, i); end;
      Ord(NANOLATHEPARTICLEBASE): begin SetColor(1, $473F3D, i); end;
      Ord(NANOLATHEPARTICLECOLORS): begin SetColor(1, $4739D6, i); end;
      Ord(UNDERCONSTRUCTSURFACELO): begin SetColor(1, $458E6C, i); end;
      Ord(UNDERCONSTRUCTSURFACEHI): begin SetColor(1, $458E5F, i); end;
      Ord(UNDERCONSTRUCTOUTLINELO): begin SetColor(1, $458E88, i); end;
      Ord(UNDERCONSTRUCTOUTLINEHI): begin SetColor(1, $458E7B, i); end;
      Ord(MAINMENUDOTSDISABLED): begin SetColor(3, $426440, 7); end;
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
