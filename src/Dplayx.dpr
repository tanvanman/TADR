library dplayx;

uses
  SysUtils,
  Classes,
  Windows,
  DPlay,
  DPLobby,
  logging in 'logging.pas',
  idplay in 'idplay.pas',
  packet_old in 'packet_old.pas',
  textdata in '.TextData.pas';

var
  lib      :HModule;
  SaveExit :pointer;

//Definition av de som vi skaffar oss
type TDirectPlayEnumerate = function (lpEnumDPCallback: TDPEnumDPCallback;
    lpContext: Pointer) : HResult; stdcall;
type TDirectPlayEnumerateA = function (lpEnumDPCallback: TDPEnumDPCallbackA;
    lpContext: Pointer) : HResult; stdcall;
type TDirectPlayEnumerateW = function (lpEnumDPCallback: TDPEnumDPCallbackW;
    lpContext: Pointer) : HResult; stdcall;
type TDirectPlayCreate = function (lpGUID: PGUID; var lplpDP: IDirectPlay;
    pUnk: IUnknown) : HResult; stdcall;
type TDirectPlayLobbyCreateW = function (lpguidSP: PGUID; var lplpDPL:
    IDirectPlayLobbyW; lpUnk: IUnknown; lpData: Pointer; dwDataSize: DWORD) :
    HResult; stdcall;
type TDirectPlayLobbyCreateA = function(lpguidSP: PGUID; var lplpDPL:
    IDirectPlayLobbyA; lpUnk: IUnknown; lpData: Pointer; dwDataSize: DWORD) :
    HResult; stdcall;

function DirectPlayEnumerate(lpEnumDPCallback: TDPEnumDPCallback;
    lpContext: Pointer) : HResult; stdcall;
var
  proc  :TDirectPlayEnumerate;
begin
  proc := GetProcAddress (lib, 'DirectPlayEnumerate');
  Result := proc (lpEnumDPCallback, lpContext);

  Log.Add ('DLL.DirectPlayEnumerate');
end;

function DirectPlayEnumerateA(lpEnumDPCallback: TDPEnumDPCallbackA;
    lpContext: Pointer) : HResult; stdcall;
var
  proc  :TDirectPlayEnumerateA;
begin
  proc := GetProcAddress (lib, 'DirectPlayEnumerateA');
  Result := proc (lpEnumDPCallback, lpContext);

  Log.Add ('DLL.DirectPlayEnumerateA');
end;

function DirectPlayEnumerateW(lpEnumDPCallback: TDPEnumDPCallbackW;
    lpContext: Pointer) : HResult; stdcall;
var
  proc  :TDirectPlayEnumerateW;
begin
  proc := GetProcAddress (lib, 'DirectPlayEnumerateW');
  Result := proc (lpEnumDPCallback, lpContext);

  Log.Add ('DLL.DirectPlayEnumerateW');
end;

function DirectPlayCreate(lpGUID: PGUID; var lplpDP: IDirectPlay;
    pUnk: IUnknown) : HResult; stdcall;
var
  proc  :TDirectPlayCreate;
  dp    :IDirectPlay;
begin
  proc := GetProcAddress (lib, 'DirectPlayCreate');
  proc (lpGUID, dp, pUnk);
//  Result := proc (lpGUID, lplpDP, pUnk);

  Log.Add ('DLL.DirectPlayCreate');
  Log.Add (' + lpGUID : ', lpGUID^);
  lplpDP := TDplay.Create (dp);

  Result := DP_OK;
end;

function DirectPlayLobbyCreateW(lpguidSP: PGUID; var lplpDPL:
    IDirectPlayLobbyW; lpUnk: IUnknown; lpData: Pointer; dwDataSize: DWORD) :
    HResult; stdcall;
var
  proc :TDirectPlayLobbyCreateW;
begin
  proc := GetProcAddress (lib, 'DirectPlayLobbyCreateW');
  Result := proc (lpguidSP, lplpDPL, lpUnk, lpData, dwDataSize);

  Log.Add ('DLL.DirectPlayLobbyCreateW');
end;

function DirectPlayLobbyCreateA(lpguidSP: PGUID; var lplpDPL:
    IDirectPlayLobbyA; lpUnk: IUnknown; lpData: Pointer; dwDataSize: DWORD) :
    HResult; stdcall;
var
  proc :  TDirectPlayLobbyCreateA;
  lb  :IDirectPlayLobby;
begin
  proc := GetProcAddress (lib, 'DirectPlayLobbyCreateA');
  Result := proc (lpguidSP, lb, lpUnk, lpData, dwDataSize);

  Log.Add ('DLL.DirectPlayLobbyCreateA');

  lplpDPL := TLobby.Create (lb);

//  Result := DP_OK;
end;

//var
//  gdwDPlaySPRefCount :Longword;
// SP = Service Provider
function gdwDPlaySPRefCount :HResult; stdcall;
begin
  Result := 0;//gdwDPlaySPRefCount;
  asm int 3 end;
  Log.Add ('DLL.gdwDPlaySPRefCount');
end;




function DllCanUnloadNow :HResult; stdcall;
begin
  Result := 0;

  Log.Add ('DLL.DllCanUnloadNow');
end;

function DllGetClassObject :HResult; stdcall;
begin
  Result := 0;

  Log.Add ('DLL.DllGetClassObject');
end;

{--------------------------------------------------------------------}

procedure LibExit;
begin
  FreeLibrary (lib);
  ExitProc := SaveExit;
end;


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

var
  p :PChar;
  s :string;
begin
  GetMem (p, 1000);

  GetSystemDirectory (p, 1000);
  s := p;
  s := s + '\dplayx.dll';

  lib := LoadLibrary (@s[1]); //'c:\windows\dplayx.dll'

  SaveExit := ExitProc;
  ExitProc := @LibExit;
end.






