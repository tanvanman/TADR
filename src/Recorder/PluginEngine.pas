unit PluginEngine;

interface
// todo : implement some better patching code.
// Currently a single form of code injection is allowed,
// need to offer a few more types
// Should implement patch conflict detection too.
const
  NOP_Instruction      = $90;
  StaticCall_Instruction = $E8;
  RelativeJump_Instruction = $e9;


type
  PRelativeJumpRec = ^TRelativeJumpRec;
  TRelativeJumpRec = packed record
    RelativeJmpInstruction : Byte;
    DistanceToJump : Longword;
  end;

  PRelativeStaticCallRec = ^TRelativeStaticCallRec;
  TRelativeStaticCallRec = packed record
    StaticCall : Byte;
    DistanceToJump : Longword;
  end;
  
  TByteArray = array of byte;

  TCodeInjection = class;
  
  TCodeInjectionClass = class of TCodeInjection;

  TCodeInjection = class
  private
    fInstalled : boolean;
    fOriginalData : TByteArray;
  protected
    fName : string;
    fEnabled : boolean;

    Procedure WriteProc; virtual; abstract;
    Procedure RestoreProc; virtual;
  public
    InjectionPoint : Longword;

    Constructor Create; virtual;
        
    Property Name : string read fName write fName;
    Property Enabled : boolean read fEnabled write fEnabled;
    Property Installed : boolean read fInstalled;
  end; {TCodeInjection}

  TCodeInjection_Replacement = class(TCodeInjection)
  protected
    Procedure WriteProc; override;
  public
    NewData : TByteArray;
  end;

  TWriteRec1 = packed record
    RelativeJump : TRelativeJumpRec;
    nops : packed array [0..$ff-1] of byte
  end;  
  TCodeInjection_RelativeJmp = class(TCodeInjection)
  protected
    data : TWriteRec1;
    Procedure WriteProc; override;
  public
    JumpToAddress : Pointer;
    WriteSize : Longword;
    Constructor Create; override;
  end;
   
  TCodeInjection_StaticCall = class(TCodeInjection)
  protected
    Procedure WriteProc; override;
  public
    JumpToAddress : Pointer;
  end;

  

  TCodeInjections = array of TCodeInjection;

  PluginInstallEvent = procedure ;
  PluginUnInstallEvent = procedure ;// of object;

  TPluginData = class
  private
    fInstalled : boolean;
  protected
    fName : string;
    fEnabled : boolean;
    fOnInstall : PluginInstallEvent;
    fOnUnInstall : PluginUnInstallEvent;
    fCodeInjections : TCodeInjections;
    fRequireOnMainRun : boolean;
  public
    Constructor create( aRequireOnMainRun : boolean;
                        aName : string;
                        aEnabled : boolean;
                        aOnInstall : PluginInstallEvent;
                        aOnUnInstall : PluginUnInstallEvent  );

    Function NewCodeInjection( CodeInjectionClass : TCodeInjectionClass ): TCodeInjection;


    function MakeStaticCall( State : boolean;
                             const name : string;
                             JumpToAddress : pointer;
                             PatchAddress : longword ) : TCodeInjection_StaticCall;
    
    function MakeRelativeJmp( State : boolean;
                              const name : string;
                              JumpToAddress : pointer;
                              PatchAddress : longword;
                              extraSize : integer = 0 ) : TCodeInjection_RelativeJmp;

    function MakeNOPReplacement( State : boolean;
                                 const name : string;
                                 PatchAddress : longword;
                                 size : integer ) : TCodeInjection_Replacement;

    function MakeReplacement( State : boolean;
                                 const name : string;
                                 PatchAddress : longword;
                                 data : array of byte ) : TCodeInjection_Replacement; overload;

    function MakeReplacement( State : boolean;
                                 const name : string;
                                 PatchAddress : longword;
                                 var data; Size : integer ) : TCodeInjection_Replacement; overload;

    Property Name : string read fName;
    Property Enabled : boolean read fEnabled write fEnabled;
    Property Installed : boolean read fInstalled;
    Property RequireOnMainRun : boolean read fRequireOnMainRun;


    Property OnInstall : PluginInstallEvent read fOnInstall;
    Property OnUnInstall : PluginUnInstallEvent read fOnUnInstall;
    Property CodeInjections : TCodeInjections read fCodeInjections;
  end; {TPluginData}



Procedure RegisterPlugin( PluginData : TPluginData );

Procedure InstallPlugins( OnMainRun : boolean ); overload;
Procedure InstallPlugins(); overload;
Procedure UnInstallPlugins;

//Procedure InstallPlugin( const name : string );
//Procedure UnInstallPlugin( const name : string );


Type
  TBackupData = array [0..SizeOf(TRelativeJumpRec)-1] of Byte;

  TCodeInjectionData = record
    AddyToPatch : Pointer;
    MyAddy : Pointer;
    BackupData : TBackupData;
  end;


// patches the call location + push instruction into a direct jump to an address
{
 Expects to be called as:
  push <constant>
  call PatchNJump

 Patches the push statment into a jump statement
}
Procedure PatchNJump( AddressToJumpTo : pointer ); stdcall;

{
// scans a DLL for calls to PatchNJump and replaces them with what PatchNJump would do
Procedure PrePatchCalls;
}


procedure SpliceInJump( var CodeInjectionData : TCodeInjectionData );  stdcall;
procedure UnSpliceJump( const CodeInjectionData : TCodeInjectionData ); stdcall;
implementation
uses
  windows,
  sysutils,
  logging;

Procedure PatchNJump( AddressToJumpTo : pointer );  stdcall;
var
  CodeInjectionData : TCodeInjectionData;
asm
  // make sure we keep all the registers
  PushAD
  PushFD

  // get the addres to patch
  mov eax, [ebp + 04]
  sub eax, 10
  mov CodeInjectionData.AddyToPatch, eax
  mov eax, AddressToJumpTo
  mov CodeInjectionData.MyAddy, eax

  // actually patch the call site
  lea eax, CodeInjectionData
  push eax
  call SpliceInJump;

// wack the return value to point to the new address
// note; this will fuck over branch prediction, but it should only happen once
  mov eax, AddressToJumpTo
  mov [ebp + 04], eax
// restore any potentially used registers
  popFD
  popAD
end;

procedure SpliceInJump( var CodeInjectionData : TCodeInjectionData );  stdcall;
var
  RelativeJumpRec : TRelativeJumpRec;
  Count : Longword;

  Writesize, OldProtect,tmpOldProtect : longword;
begin
RelativeJumpRec.RelativeJmpInstruction := RelativeJump_Instruction;
RelativeJumpRec.DistanceToJump := longword( longint(CodeInjectionData.MyAddy) - longint(CodeInjectionData.AddyToPatch) - SizeOf(RelativeJumpRec) );


Writesize := Length(CodeInjectionData.BackupData);
Win32Check( VirtualProtect( CodeInjectionData.AddyToPatch, Writesize, PAGE_READWRITE, OldProtect ) );
try
  Win32Check( ReadProcessMemory( GetCurrentProcess, CodeInjectionData.AddyToPatch,
                                 @CodeInjectionData.BackupData[0],
                                 Writesize, Count ) );
  Win32Check( WriteProcessMemory( GetCurrentProcess, CodeInjectionData.AddyToPatch,
                                  @RelativeJumpRec,
                                 Writesize, Count ) );
finally
  Win32Check( VirtualProtect( CodeInjectionData.AddyToPatch, Writesize, OldProtect, tmpOldProtect ) );
end;                                  
end; {SpliceInJump}

procedure UnSpliceJump( const CodeInjectionData : TCodeInjectionData );  stdcall;
var
  Count : Longword;
  Writesize, OldProtect,tmpOldProtect : longword;  
begin
Writesize := Length(CodeInjectionData.BackupData);
Win32Check( VirtualProtect( CodeInjectionData.AddyToPatch, Writesize, PAGE_READWRITE, OldProtect ) );
try
  Win32Check( WriteProcessMemory( GetCurrentProcess,
                      CodeInjectionData.AddyToPatch,
                      @CodeInjectionData.BackupData[0],
                      Writesize, Count ) );
finally
  Win32Check( VirtualProtect( CodeInjectionData.AddyToPatch, Writesize, OldProtect, tmpOldProtect ) );
end;                       
end; {UnSpliceJump}
  
// -----------------------------------------------------------------------------
//   TCodeInjection
// -----------------------------------------------------------------------------

Constructor TCodeInjection.Create;
begin
inherited;
end;

Procedure TCodeInjection.RestoreProc;
var
  ReturnAddr: Pointer;
  CommittedBytes, Writesize: Longword;

  OldProtect,tmpOldProtect : longword;  
begin
Writesize := longword( length(fOriginalData) );

Win32Check( VirtualProtect( pointer(InjectionPoint), Writesize, PAGE_READWRITE, OldProtect ) );
try
  Win32Check( WriteProcessMemory( GetCurrentProcess,
                      pointer(InjectionPoint),
                      @fOriginalData,
                      Writesize,
                      CommittedBytes) );
  FlushInstructionCache( GetCurrentProcess, pointer(InjectionPoint), Writesize );
finally
  Win32Check( VirtualProtect( pointer(InjectionPoint), Writesize, OldProtect, tmpOldProtect ) );
end;
if CommittedBytes <> Writesize then
  begin
  asm
    push eax;
    MOV EAX,[EBP+4];
    Mov ReturnAddr, EAX;
    pop eax;
  end;
  raise Exception.Create('Error restoring '+name) at ReturnAddr;
  end;
fInstalled := false;
end; {RestoreProc}

// -----------------------------------------------------------------------------
//   TCodeInjection_Replacement
// -----------------------------------------------------------------------------

Procedure TCodeInjection_Replacement.WriteProc;
var
  CurrentProcessHandle : THandle;
  ReturnAddr: Pointer;
  Writesize, CommittedBytes : Longword;
  OldProtect, tmpOldProtect : longword;
begin
CurrentProcessHandle := GetCurrentProcess;
Writesize := length(NewData);
setlength(fOriginalData, Writesize);

Win32Check( VirtualProtect( pointer(InjectionPoint), Writesize, PAGE_READWRITE, OldProtect ) );
try
  Win32Check( ReadProcessMemory( CurrentProcessHandle,
                     pointer(InjectionPoint),
                     @fOriginalData[0],
                     Writesize,
                     CommittedBytes) );
  Win32Check( WriteProcessMemory( CurrentProcessHandle,
                      pointer(InjectionPoint),
                      @NewData[0],
                      Writesize,
                      CommittedBytes) );
  FlushInstructionCache( CurrentProcessHandle, pointer(InjectionPoint), Writesize );
finally
  Win32Check( VirtualProtect( pointer(InjectionPoint), Writesize, OldProtect, tmpOldProtect ) );
end;                  
if CommittedBytes <> Writesize then
  begin
  asm
    push eax;
    MOV EAX,[EBP+4];
    Mov ReturnAddr, EAX;
    pop eax;
  end;
  raise Exception.Create('Error injecting '+name) at ReturnAddr;
  end;
fInstalled := true;
end;

// -----------------------------------------------------------------------------
//   TCodeInjection_StaticCall
// -----------------------------------------------------------------------------


Procedure TCodeInjection_StaticCall.WriteProc;
var
  data : TRelativeStaticCallRec;
  
  CurrentProcessHandle : THandle;
  ReturnAddr: Pointer;
  CommittedBytes : Longword;

  OldProtect,tmpOldProtect : longword;
  Writesize : longword;
begin
CurrentProcessHandle := GetCurrentProcess;
data.StaticCall := StaticCall_Instruction;
data.DistanceToJump := Longword(  integer(JumpToAddress) - integer(InjectionPoint+5) );
Writesize := sizeof(data);
setlength(fOriginalData, WriteSize);

Win32Check( VirtualProtect( pointer(InjectionPoint), Writesize, PAGE_READWRITE, OldProtect ) );
try
  Win32Check( ReadProcessMemory( CurrentProcessHandle,
                     pointer(InjectionPoint),
                     @fOriginalData[0],
                     Writesize,
                     CommittedBytes) );
  Win32Check( WriteProcessMemory( CurrentProcessHandle,
                      pointer(InjectionPoint),
                      @data,
                      Writesize,
                      CommittedBytes) );
  FlushInstructionCache( CurrentProcessHandle, pointer(InjectionPoint), Writesize );
finally
  Win32Check( VirtualProtect( pointer(InjectionPoint), Writesize, OldProtect, tmpOldProtect ) );
end;
if CommittedBytes <> Writesize then
  begin
  asm
    push eax;
    MOV EAX,[EBP+4];
    Mov ReturnAddr, EAX;
    pop eax;
  end;
  raise Exception.Create('Error injecting '+name) at ReturnAddr;
  end;
fInstalled := true;
end; {WriteProc}


// -----------------------------------------------------------------------------
//   TCodeInjection_RelativeJmp
// -----------------------------------------------------------------------------

Constructor TCodeInjection_RelativeJmp.Create;
var i : integer;
begin
data.RelativeJump.RelativeJmpInstruction := RelativeJump_Instruction;
for i := 0 to high(data.nops) do
  data.nops[i] := NOP_Instruction;
end;

Procedure TCodeInjection_RelativeJmp.WriteProc;
var
  CurrentProcessHandle : THandle;
  ReturnAddr: Pointer;
  CommittedBytes : Longword;

  OldProtect,tmpOldProtect : longword;  
begin
CurrentProcessHandle := GetCurrentProcess;
data.RelativeJump.DistanceToJump := Longword(  integer(JumpToAddress) - integer(InjectionPoint+5) );
assert( Writesize <= sizeof(data));
setlength(fOriginalData, WriteSize);

Win32Check( VirtualProtect( pointer(InjectionPoint), Writesize, PAGE_READWRITE, OldProtect ) );
try
  Win32Check( ReadProcessMemory( CurrentProcessHandle,
                     pointer(InjectionPoint),
                     @fOriginalData[0],
                     Writesize,
                     CommittedBytes) );
  Win32Check( WriteProcessMemory( CurrentProcessHandle,
                      pointer(InjectionPoint),
                      @data,
                      Writesize,
                      CommittedBytes) );
  FlushInstructionCache( CurrentProcessHandle, pointer(InjectionPoint), Writesize );
finally
  Win32Check( VirtualProtect( pointer(InjectionPoint), Writesize, OldProtect, tmpOldProtect ) );
end;
if CommittedBytes <> Writesize then
  begin
  asm
    push eax;
    MOV EAX,[EBP+4];
    Mov ReturnAddr, EAX;
    pop eax;
  end;
  raise Exception.Create('Error injecting '+name) at ReturnAddr;
  end;
fInstalled := true;
end; {WriteProc}

// -----------------------------------------------------------------------------
//  TPluginData
// -----------------------------------------------------------------------------

Constructor TPluginData.create( aRequireOnMainRun : boolean;
                                aName : string;
                                aEnabled : boolean;
                                aOnInstall : PluginInstallEvent;
                                aOnUnInstall : PluginUnInstallEvent  );
begin
fRequireOnMainRun := aRequireOnMainRun;
fName := aName;
fEnabled := aEnabled;
fOnInstall := aOnInstall;
fOnUnInstall := aOnUnInstall;
end; {create}


function TPluginData.MakeRelativeJmp( State : boolean; const name : string; JumpToAddress : pointer; PatchAddress : longword; extraSize : integer = 0 ) : TCodeInjection_RelativeJmp;
begin
result := TCodeInjection_RelativeJmp(NewCodeInjection(TCodeInjection_RelativeJmp));
result.Enabled := State;
result.Name:= name;
result.JumpToAddress := JumpToAddress;
result.InjectionPoint := PatchAddress;
result.Writesize := 5+extraSize;
end;

function TPluginData.MakeStaticCall( State : boolean;
                             const name : string;
                             JumpToAddress : pointer;
                             PatchAddress : longword ) : TCodeInjection_StaticCall;
begin
result := TCodeInjection_StaticCall(NewCodeInjection(TCodeInjection_StaticCall));
result.Enabled := State;
result.Name:= name;
result.JumpToAddress := JumpToAddress;
result.InjectionPoint := PatchAddress;
end;


function TPluginData.MakeNOPReplacement( State : boolean;
                                         const name : string;
                                         PatchAddress : longword;
                                         size : integer ) : TCodeInjection_Replacement;
var
  i : integer;
begin
result := TCodeInjection_Replacement(NewCodeInjection(TCodeInjection_Replacement));
result.Enabled := State;
result.Name := name;
setlength(result.NewData, size );
for i := 0 to high(result.NewData) do
  result.NewData[i] := NOP_Instruction;
result.InjectionPoint := PatchAddress;
end;

function TPluginData.MakeReplacement( State : boolean;
                                      const name : string;
                                      PatchAddress : longword;
                                      data : array of byte ) : TCodeInjection_Replacement;
var
  i : integer;
begin
result := TCodeInjection_Replacement(NewCodeInjection(TCodeInjection_Replacement));
result.Enabled := State;
result.Name := name;
setlength(result.NewData, length(data) );
for i := 0 to high(result.NewData) do
  result.NewData[i] := data[i];
result.InjectionPoint := PatchAddress;
end;

function TPluginData.MakeReplacement( State : boolean;
                                      const name : string;
                                      PatchAddress : longword;
                                      var data; Size : integer ) : TCodeInjection_Replacement;
var
  i : integer;
begin
result := TCodeInjection_Replacement(NewCodeInjection(TCodeInjection_Replacement));
result.Enabled := State;
result.Name := name;
setlength(result.NewData, Size );
for i := 0 to high(result.NewData) do
  result.NewData[i] := PByteArray(@data)[i];
result.InjectionPoint := PatchAddress;
end;

Function TPluginData.NewCodeInjection( CodeInjectionClass : TCodeInjectionClass ): TCodeInjection;
var
  i : integer;
begin
assert(CodeInjectionClass <> nil);
i := length(fCodeInjections);
setlength( fCodeInjections, i+1);
Result := CodeInjectionClass.create;
fCodeInjections[i] := result;
end;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------

var
  Plugins : array of TPluginData;
  
Procedure RegisterPlugin( PluginData : TPluginData );
var
  i : integer;
begin
if PluginData <> nil then
  begin
  i := length(Plugins);
  setlength( Plugins, i+1);
  Plugins[i] := PluginData;
  end;
end; {RegisterPlugin}

Procedure InstallPlugins( OnMainRun : boolean );
var
  i,i2 : integer;
  CodeInjections : TCodeInjections;
begin
// to code injections
CodeInjections := nil;
for i := 0 to high(Plugins) do
  if Plugins[i].Enabled and not Plugins[i].Installed and
     ( Plugins[i].RequireOnMainRun = OnMainRun) then
    begin
    if assigned(Plugins[i].OnInstall) then
      Plugins[i].OnInstall();
    CodeInjections := Plugins[i].CodeInjections;
    for i2 := 0 to high(CodeInjections) do
      if CodeInjections[i2].Enabled and not CodeInjections[i2].Installed then
        CodeInjections[i2].WriteProc();
    end;
end;

Procedure InstallPlugins;
var
  i,i2 : integer;
  CodeInjections : TCodeInjections;
begin
// to code injections
CodeInjections := nil;
for i := 0 to high(Plugins) do
  if Plugins[i].Enabled and not Plugins[i].Installed then
    begin
    if assigned(Plugins[i].OnInstall) then
      Plugins[i].OnInstall();
    CodeInjections := Plugins[i].CodeInjections;
    for i2 := 0 to high(CodeInjections) do
      if CodeInjections[i2].Enabled and not CodeInjections[i2].Installed then
        // todo : on error roll back patch install
        CodeInjections[i2].WriteProc();
    end;
end;

Procedure UnInstallPlugins;
var
  i,i2 : integer;
  CodeInjections : TCodeInjections;
begin
CodeInjections := nil;
for i := 0 to high(Plugins) do
  if Plugins[i].Installed then
    begin
    CodeInjections := Plugins[i].CodeInjections;
    for i2 := 0 to high(CodeInjections) do
      if CodeInjections[i2].Installed then
        CodeInjections[i2].RestoreProc();
    if assigned(Plugins[i].OnUnInstall) then
      Plugins[i].OnUnInstall();
    end;             
end; {UnInstallPlugins}

end.
