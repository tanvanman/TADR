{

Fast Memory Manager: Replacement BorlndMM.DLL 1.02

Description:
 A replacement borlndmm.dll using FastMM instead of the RTL MM. This DLL may be
 used instead of the default DLL together with your own applications or the
 Delphi IDE, making the benefits of FastMM available to them.

Usage:
 1) Make sure the "NeverUninstall" conditional define is set in FastMM4.pas if
 you intend to use the DLL with the Delphi IDE, otherwise it must be off.
 2) Compile this DLL
 3) Ship it with your existing applications that currently uses the borlndmm.dll
 file that ships with Delphi for an improvement in speed.
 4) Copy it over the current borlndmm.dll in the Delphi \Bin\ directory (after
 renaming the old one) to speed up the IDE.*

Acknowledgements:
  - Arthur Hoornweg for notifying me of the image base being incorrect for
    borlndmm.dll.

Change log:
 Version 1.00 (28 June 2005):
  - Initial release.
 Version 1.01 (30 June 2005):
  - Added an unofficial patch for QC#14007 that prevented a replacement
    borlndmm.dll from working together with Delphi 2005.
  - Added the "NeverUninstall" option in FastMM4.pas to circumvent QC#14070,
    which causes an A/V on shutdown of Delphi if FastMM uninstalls itself in the
    finalization code of FastMM4.pas.
  Version 1.02 (19 July 2005):
  - Set the imagebase to $00D20000 to avoid relocation on load (and thus allow
    sharing of the DLL between processes). (Thanks to Arthur Hoornweg.)

*For this replacement borlndmm.dll to work together with Delphi 2005, you will
 need to apply the unofficial patch for QC#14007. To compile a replacement
 borlndmm.dll for use with the Delphi IDE the "NeverUninstall" option must be
 set (to circumvent QC#14070). For other uses the "NeverUninstall" option
 should be disabled. For a list of unofficial patches for Delphi 2005 (and
 where to get them), refer to the FastMM4_Readme.txt file. 

}

{--------------------Start of options block-------------------------}

{Set the following option to use the RTL MM instead of FastMM. Setting this
 option makes this replacement DLL almost identical to the default
 borlndmm.dll, unless the "FullDebugMode" option is also set.}
{.$define UseRTLMM}

{--------------------End of options block-------------------------}

{$Include FastMM4Options.inc}

{Cannot use the RTL MM with full debug mode}
{$ifdef FullDebugMode}
  {$undef UseRTLMM}
{$endif}

{Set the correct image base}
{$IMAGEBASE $00D20000}

library BorlndMM;

{$ifndef UseRTLMM}
uses
  FastMM4;
{$endif}

{$R *.RES}

function GetAllocMemCount: integer;
begin
  {Return stats for the RTL MM only}
{$ifdef UseRTLMM}
  Result := System.AllocMemCount;
{$else}
  Result := 0;
{$endif}
end;

function GetAllocMemSize: integer;
begin
  {Return stats for the RTL MM only}
{$ifdef UseRTLMM}
  Result := System.AllocMemSize;
{$else}
  Result := 0;
{$endif}
end;

{$ifndef UseRTLMM}
function GetFastMMHeapStatus: THeapStatus;
begin
  FillChar(Result, SizeOf(Result), 0);
end;
{$endif}

procedure DumpBlocks;
begin
  {Do nothing}
end;

function HeapRelease: Integer;
begin
  {Do nothing}
  Result := 2;
end;

function HeapAddRef: Integer;
begin
  {Do nothing}
  Result := 2;
end;

exports GetAllocMemSize index 1 name 'GetAllocMemSize';
exports GetAllocMemCount index 2 name 'GetAllocMemCount';
{$ifndef UseRTLMM}
exports GetFastMMHeapStatus index 3 name 'GetHeapStatus';
{$else}
exports System.GetHeapStatus index 3 name 'GetHeapStatus';
{$endif}
exports DumpBlocks index 4 name 'DumpBlocks';
exports System.ReallocMemory index 5 name 'ReallocMemory';
exports System.FreeMemory index 6 name 'FreeMemory';
exports System.GetMemory index 7 name 'GetMemory';
{$ifndef UseRTLMM}
{$ifndef FullDebugMode}
exports FastReallocMem index 8 name '@Borlndmm@SysReallocMem$qqrpvi';
exports FastFreeMem index 9 name '@Borlndmm@SysFreeMem$qqrpv';
exports FastGetMem index 10 name '@Borlndmm@SysGetMem$qqri';
{$else}
exports DebugReallocMem index 8 name '@Borlndmm@SysReallocMem$qqrpvi';
exports DebugFreeMem index 9 name '@Borlndmm@SysFreeMem$qqrpv';
exports DebugGetMem index 10 name '@Borlndmm@SysGetMem$qqri';
{$endif}
{$else}
exports System.SysReallocMem index 8 name '@Borlndmm@SysReallocMem$qqrpvi';
exports System.SysFreeMem index 9 name '@Borlndmm@SysFreeMem$qqrpv';
exports System.SysGetMem index 10 name '@Borlndmm@SysGetMem$qqri';
{$endif}
exports HeapRelease index 11 name '@Borlndmm@HeapRelease$qqrv';
exports HeapAddRef index 12 name '@Borlndmm@HeapAddRef$qqrv';

begin
  IsMultiThread := True;
end.
