// prevents relaying data to DirectPlay object underneath.
// calls get sent to redirection proxies instead
//{.$DEFINE DplayRedirector}
unit Dplayx_exports;
interface
uses
  windows,
  DPlay,
  DPLobby;

{$IFDEF DplayRedirector}
var
  dp_redirector : IDirectPlay3;
  CallingSelf : boolean;
{$ENDIF}

{$IFNDEF NoDplayExports}
var
 dplayxLibHandle : HModule = INVALID_HANDLE_VALUE;

// used to detect if the 'dplayxLibHandle' is invalid
Function DPlayxHandleInvalid : boolean;
{$ENDIF}

var
  // callback used todo initialization & Finalizalization. Keep init code out of scary voodoo unit

  // DoInitialize can be called either when the exe's Main is run
  // or on the first dxplay call 
  DoInitialize : procedure ( OnMainRun : boolean );
  DoFinalize : procedure;


type
  TDirectPlayEnumerate = function (lpEnumDPCallback: TDPEnumDPCallback;
    lpContext: Pointer) : HResult; stdcall;
 TDirectPlayEnumerateA = function (lpEnumDPCallback: TDPEnumDPCallbackA;
    lpContext: Pointer) : HResult; stdcall;
 TDirectPlayEnumerateW = function (lpEnumDPCallback: TDPEnumDPCallbackW;
    lpContext: Pointer) : HResult; stdcall;
 TDirectPlayCreate = function (lpGUID: PGUID; var lplpDP: IDirectPlay;
    pUnk: IUnknown) : HResult; stdcall;
 TDirectPlayLobbyCreateW = function (lpguidSP: PGUID; var lplpDPL:
    IDirectPlayLobbyW; lpUnk: IUnknown; lpData: Pointer; dwDataSize: DWORD) :
    HResult; stdcall;
 TDirectPlayLobbyCreateA = function(lpguidSP: PGUID; var lplpDPL:
    IDirectPlayLobbyA; lpUnk: IUnknown; lpData: Pointer; dwDataSize: DWORD) :
    HResult; stdcall;
{$IFDEF NoDplayExports}
type
{$IFDEF UNICODE}
  TDPEnumDPCallback = TDPEnumDPCallbackW;
{$ELSE}
  TDPEnumDPCallback = TDPEnumDPCallbackA;
{$ENDIF}

var
  DPlayDLL     : HMODULE = 0;
  DirectPlayCreate : TDirectPlayCreate;
  DirectPlayLobbyCreateW : TDirectPlayLobbyCreateW;
  DirectPlayLobbyCreateA : TDirectPlayLobbyCreateA;
  DirectPlayLobbyCreate : TDirectPlayCreate;
  DirectPlayEnumerate : TDirectPlayEnumerate;
  DirectPlayEnumerateA : TDirectPlayEnumerateA;
  DirectPlayEnumerateW : TDirectPlayEnumerateW;

{$ELSE}      
function DirectPlayCreate(lpGUID: PGUID; var lplpDP: IDirectPlay; pUnk: IUnknown) : HResult; stdcall;
function DirectPlayEnumerateA(lpEnumDPCallback: TDPEnumDPCallbackA; lpContext: Pointer) : HResult; stdcall;
function DirectPlayEnumerateW(lpEnumDPCallback: TDPEnumDPCallbackW; lpContext: Pointer) : HResult; stdcall;
function DirectPlayLobbyCreateA(lpguidSP: PGUID; var lplpDPL: IDirectPlayLobbyA; lpUnk: IUnknown; lpData: Pointer; dwDataSize: DWORD) : HResult; stdcall;
function DirectPlayLobbyCreateW(lpguidSP: PGUID; var lplpDPL: IDirectPlayLobbyW; lpUnk: IUnknown; lpData: Pointer; dwDataSize: DWORD) : HResult; stdcall;
var
  gdwDPlaySPRefCount :longword;
function DirectPlayEnumerate(lpEnumDPCallback: TDPEnumDPCallback; lpContext: Pointer) : HResult; stdcall;
function DllCanUnloadNow :HResult; stdcall;
function DllGetClassObject :HResult; stdcall;

function DirectPlayLobbyCreate(lpguidSP: PGUID; var lplpDPL:
    IDirectPlayLobby; lpUnk: IUnknown; lpData: Pointer; dwDataSize: DWORD) :
    HResult; stdcall;
    
exports
   DirectPlayCreate index 1,
   DirectPlayEnumerateA index 2,
   DirectPlayEnumerateW index 3,
   DirectPlayLobbyCreateA	index 4,
   DirectPlayLobbyCreateW index 5,
   gdwDPlaySPRefCount	index 6,
   DirectPlayEnumerate index 9,
   DllCanUnloadNow index 10,
   DllGetClassObject index 11;
{$ENDIF}
// --------------------------------------------------------------------
   
implementation
uses
  sysutils,
  DPLobbyWrapper,
  idplay,
  logging;


{$IFNDEF NoDplayExports}
function DirectPlayLobbyCreate(lpguidSP: PGUID; var lplpDPL:
    IDirectPlayLobby; lpUnk: IUnknown; lpData: Pointer; dwDataSize: DWORD) :
    HResult; stdcall;
begin
{$IFDEF UNICODE}
  Result := DirectPlayLobbyCreateW (lpguidsp, lplpdpl, lpunk, lpdata, dwdatasize);
{$ELSE}
  Result := DirectPlayLobbyCreateA (lpguidsp, lplpdpl, lpunk, lpdata, dwdatasize);
{$ENDIF}
end;

Function DPlayxHandleInvalid : boolean;
begin
result := (dplayxLibHandle = INVALID_HANDLE_VALUE) or (dplayxLibHandle = 0);
end;

function OnInit() : boolean;
begin
{$IFDEF DplayRedirector}
if CallingSelf then
  begin
  CallingSelf := false;
  result := true;
  exit;
  end
else
  CallingSelf := true;  
{$ENDIF}
if DPlayxHandleInvalid then
  begin
  if assigned(DoInitialize) then
    DoInitialize( false );
  if DPlayxHandleInvalid then
    Raise Exception.create('Init code didnt load Dplayx dll');
  end;
result := false;
end;

var
  DirectPlayEnumerate_proc  :TDirectPlayEnumerate;
function DirectPlayEnumerate(lpEnumDPCallback: TDPEnumDPCallback;
    lpContext: Pointer) : HResult; stdcall;

begin
if OnInit() then begin result := DP_OK; exit; end else Result := DPERR_EXCEPTION;
try
  if @DirectPlayEnumerate_proc = nil then
    DirectPlayEnumerate_proc := GetProcAddress( dplayxLibHandle, 'DirectPlayEnumerate');
  if assigned(DirectPlayEnumerate_proc) then
    Result := DirectPlayEnumerate_proc(lpEnumDPCallback, lpContext);

  TLog.Add (5,'DLL.DirectPlayEnumerate');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

var
  DirectPlayEnumerateA_proc  :TDirectPlayEnumerateA;
function DirectPlayEnumerateA(lpEnumDPCallback: TDPEnumDPCallbackA;
    lpContext: Pointer) : HResult; stdcall;
begin
if OnInit() then begin result := DP_OK; exit; end else Result := DPERR_EXCEPTION;
try
  if @DirectPlayEnumerateA_proc = nil then
    DirectPlayEnumerateA_proc := GetProcAddress( dplayxLibHandle, 'DirectPlayEnumerateA');
  if assigned(DirectPlayEnumerateA_proc) then
    Result := DirectPlayEnumerateA_proc(lpEnumDPCallback, lpContext);

  TLog.Add (5,'DLL.DirectPlayEnumerateA');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

var
  DirectPlayEnumerateW_proc  :TDirectPlayEnumerateW;
function DirectPlayEnumerateW(lpEnumDPCallback: TDPEnumDPCallbackW;
    lpContext: Pointer) : HResult; stdcall;

begin
if OnInit() then begin result := DP_OK; exit; end else Result := DPERR_EXCEPTION;
try
  if @DirectPlayEnumerateW_proc = nil then
    DirectPlayEnumerateW_proc := GetProcAddress( dplayxLibHandle, 'DirectPlayEnumerateW');
  if assigned(DirectPlayEnumerateW_proc) then
    Result := DirectPlayEnumerateW_proc(lpEnumDPCallback, lpContext);

  TLog.Add (5,'DLL.DirectPlayEnumerateW');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

var
  DirectPlayCreate_proc  :TDirectPlayCreate;
function DirectPlayCreate(lpGUID: PGUID; var lplpDP: IDirectPlay;
    pUnk: IUnknown) : HResult; stdcall;
var
  dp    :IDirectPlay;
begin
if OnInit() then begin result := DP_OK; exit; end else Result := DPERR_EXCEPTION;
try
  if @DirectPlayCreate_proc = nil then
    DirectPlayCreate_proc := GetProcAddress( dplayxLibHandle, 'DirectPlayCreate');
  if assigned(DirectPlayCreate_proc) then
    result := DirectPlayCreate_proc(lpGUID, dp, pUnk);  
  if result = DP_OK then
    begin
    TLog.Add( 5,'DLL.DirectPlayCreate' );
    TLog.Add( 5,' + lpGUID : ', lpGUID );
    TLog.Flush;  
    lplpDP := TDplay.Create (dp);
    startedfrom := 'DirectPlayCreate';
    end
  else
    begin
    TLog.Add( 5,'DLL.DirectPlayCreate FAILED' );
    TLog.Add( 5,' + lpGUID : ', lpGUID );
    TLog.Add( 5,'Reason: '+ErrorString(result) );
    TLog.Flush;
    end;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

var
  DirectPlayLobbyCreateW_proc :TDirectPlayLobbyCreateW;
function DirectPlayLobbyCreateW(lpguidSP: PGUID; var lplpDPL:
    IDirectPlayLobbyW; lpUnk: IUnknown; lpData: Pointer; dwDataSize: DWORD) :
    HResult; stdcall;

begin
if OnInit() then begin result := DP_OK; exit; end else Result := DPERR_EXCEPTION;
try
  if @DirectPlayLobbyCreateW_proc = nil then
    DirectPlayLobbyCreateW_proc := GetProcAddress( dplayxLibHandle, 'DirectPlayLobbyCreateW');
  if assigned(DirectPlayLobbyCreateW_proc) then
    Result := DirectPlayLobbyCreateW_proc(lpguidSP, lplpDPL, lpUnk, lpData, dwDataSize);

  TLog.Add (5,'DLL.DirectPlayLobbyCreateW');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

var
  DirectPlayLobbyCreateA_proc :  TDirectPlayLobbyCreateA;
function DirectPlayLobbyCreateA(lpguidSP: PGUID; var lplpDPL:
    IDirectPlayLobbyA; lpUnk: IUnknown; lpData: Pointer; dwDataSize: DWORD) :
    HResult; stdcall;
var
  lb  :IDirectPlayLobby;
begin
if OnInit() then begin result := DP_OK; exit; end else Result := DPERR_EXCEPTION;
try
  if @DirectPlayLobbyCreateA_proc = nil then
    DirectPlayLobbyCreateA_proc := GetProcAddress( dplayxLibHandle, 'DirectPlayLobbyCreateA');
  if assigned(DirectPlayLobbyCreateA_proc) then  
    Result := DirectPlayLobbyCreateA_proc(lpguidSP, lb, lpUnk, lpData, dwDataSize);  
  if Result = DP_OK then
    begin
    TLog.Add (5,'DLL.DirectPlayLobbyCreateA');
    TLog.Add( 5,' + lpGUID : ', lpguidSP );
    TLog.Flush;
    {$IFDEF DplayRedirector}
     lplpDPL := lb;
    {$ELSE}
    lplpDPL := TLobby.Create (lb);
    {$ENDIF}
    end
  else
    begin
    TLog.Add( 5,'DLL.DirectPlayLobbyCreateA FAILED' );
    TLog.Add( 5,' + lpGUID : ', lpguidSP );
    TLog.Add( 5,'Reason: '+ErrorString(result) );
    TLog.Flush;
    end;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

{// SP = Service Provider
function gdwDPlaySPRefCount :HResult; stdcall;
begin
  Result := 0;

  TLog.Add (5,'DLL.gdwDPlaySPRefCount');
end;
}
function DllCanUnloadNow :HResult; stdcall;
begin
if OnInit() then begin result := DP_OK; exit; end;
try
  Result := DP_OK;

  TLog.Add (5,'DLL.DllCanUnloadNow');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

function DllGetClassObject :HResult; stdcall;
begin
if OnInit() then begin result := DP_OK; exit; end;
try
  Result := DP_OK;

  TLog.Add (5,'DLL.DllGetClassObject');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

// --------------------------------------------------------------------

{$ENDIF}
Procedure StubCall();
begin
{$IFNDEF NoDplayExports}
gdwDPlaySPRefCount := 0;
{$ENDIF}
  writeln('should not get here');
end;

initialization
{$IFDEF NoDplayExports}
    DPlayDLL := LoadLibrary('DPlayX.dll');

    DirectPlayEnumerateA := GetProcAddress(DPlayDLL,'DirectPlayEnumerateA');
    DirectPlayEnumerateW := GetProcAddress(DPlayDLL,'DirectPlayEnumerateW');
  {$IFDEF UNICODE}
    DirectPlayEnumerate := DirectPlayEnumerateW;
  {$ELSE}
    DirectPlayEnumerate := DirectPlayEnumerateA;
  {$ENDIF}

    DirectPlayCreate := GetProcAddress(DPlayDLL,'DirectPlayCreate');

    DirectPlayLobbyCreateW := GetProcAddress(DPlayDLL,'DirectPlayLobbyCreateW');
    DirectPlayLobbyCreateA := GetProcAddress(DPlayDLL,'DirectPlayLobbyCreateA');
  {$IFDEF UNICODE}
    DirectPlayLobbyCreate := DirectPlayLobbyCreateW;
  {$ELSE}
    DirectPlayLobbyCreate := DirectPlayLobbyCreateA;
  {$ENDIF}
{$ENDIF}
if not assigned(DoInitialize) then
  DoInitialize := @StubCall;
if not assigned(DoFinalize) then
  DoFinalize := @StubCall;

finalization
{$IFDEF NoDplayExports}
  if DPlayDLL <> 0 then FreeLibrary(DPlayDLL);
{$ENDIF}  
end.
