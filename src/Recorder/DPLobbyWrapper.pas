unit DPLobbyWrapper;

interface
uses
  Windows, sysutils, DPlay, DPLobby;

type
  TLobby = class (TInterfacedObject, IDirectPlayLobby, IDirectPlayLobby2)
  private
    lobby1 :IDirectPlayLobby;
    lobby2 :IDirectPlayLobby2;

  public
    constructor Create (reallobby :IDirectPlayLobby);

    (*** IDirectPlayLobby methods ***)
    function Connect(dwFlags: DWORD; var lplpDP: IDirectPlay2;
        pUnk: IUnknown) : HResult; stdcall;
    function CreateAddress(const guidSP, guidDataType: TGUID; const lpData;
        dwDataSize: DWORD; var lpAddress; var lpdwAddressSize: DWORD) : HResult; stdcall;
    function EnumAddress(lpEnumAddressCallback: TDPEnumAdressCallback;
        const lpAddress; dwAddressSize: DWORD; lpContext : Pointer) : HResult; stdcall;
    function EnumAddressTypes(lpEnumAddressTypeCallback:
        TDPLEnumAddressTypesCallback; const guidSP: TGUID; lpContext: Pointer;
        dwFlags: DWORD) : HResult; stdcall;
    function EnumLocalApplications(lpEnumLocalAppCallback:
        TDPLEnumLocalApplicationsCallback; lpContext: Pointer; dwFlags: DWORD)
        : HResult; stdcall;
    function GetConnectionSettings(dwAppID: DWORD; lpData: PDPLConnection;
        var lpdwDataSize: DWORD) : HResult; stdcall;
    function ReceiveLobbyMessage(dwFlags: DWORD; dwAppID: DWORD;
        var lpdwMessageFlags: DWORD; lpData: Pointer; var lpdwDataSize: DWORD) :
        HResult; stdcall;
    function RunApplication(dwFlags: DWORD; var lpdwAppId: DWORD;
        const lpConn: TDPLConnection; hReceiveEvent: THandle) : HResult; stdcall;
    function SendLobbyMessage(dwFlags: DWORD; dwAppID: DWORD; const lpData;
        dwDataSize: DWORD) : HResult; stdcall;
    function SetConnectionSettings(dwFlags: DWORD; dwAppID: DWORD;
        const lpConn: TDPLConnection) : HResult; stdcall;
    function SetLobbyMessageEvent(dwFlags: DWORD; dwAppID: DWORD;
        hReceiveEvent: THandle) : HResult; stdcall;

    function CreateCompoundAddress(const lpElements: TDPCompoundAddressElement;
        dwElementCount: DWORD; lpAddress: Pointer; var lpdwAddressSize: DWORD) :
        HResult; stdcall;

  end;

implementation
uses
  idplay,
  Logging,
  InitCode;

//----------------------------------------------

function TLobby.Connect(dwFlags: DWORD; var lplpDP: IDirectPlay2;
        pUnk: IUnknown) : HResult;
var
  dp :IDirectPlay2;
  dp1 :IDirectPlay;
  DPlay : TDPlay;
begin
try
  Result := lobby2.Connect (dwFlags, dp, pUnk);
  TLog.Add(5,'LOBBY.Connect');
  TLog.Add(5,' + dwFlags : ', dwFlags);
  TLog.Flush;
  dp.QueryInterface (IID_IDirectPlay, dp1);
  DPlay := TDPlay.Create( dp1 );
  try
     // todo : issue command to recorder backend to prepare for a new session
  finally
    lplpdp := DPlay;
  end;
  startedfrom := 'TLobby.Connect';
except
  on e : Exception do
     begin
     lplpdp := nil;
     LogException(e);
     raise;
    end;
end;
end;

function TLobby.CreateAddress(const guidSP, guidDataType: TGUID; const lpData;
        dwDataSize: DWORD; var lpAddress; var lpdwAddressSize: DWORD) : HResult;
begin
try
  Result := lobby2.CreateAddress (guidSp, guiddatatype, lpdata, dwdatasize, lpaddress, lpdwaddresssize);
  TLog.Add(5,'LOBBY.CreateAddress');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

function TLobby.EnumAddress(lpEnumAddressCallback: TDPEnumAdressCallback;
        const lpAddress; dwAddressSize: DWORD; lpContext : Pointer) : HResult;
begin
try
  Result := lobby2.enumaddress (lpenumaddresscallback, lpaddress, dwaddresssize, lpcontext);
  TLog.Add(5,'LOBBY.EnumAddress');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

function TLobby.EnumAddressTypes(lpEnumAddressTypeCallback:
        TDPLEnumAddressTypesCallback; const guidSP: TGUID; lpContext: Pointer;
        dwFlags: DWORD) : HResult;
begin
try
  result := lobby2.enumaddresstypes (lpenumaddresstypecallback, guidsp, lpcontext, dwflags);
  TLog.Add(5,'LOBBY.EnumAddressTypes');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

function TLobby.EnumLocalApplications(lpEnumLocalAppCallback:
        TDPLEnumLocalApplicationsCallback; lpContext: Pointer; dwFlags: DWORD)
        : HResult;
begin
try
  result := lobby2.EnumLocalApplications (lpenumlocalappcallback, lpcontext, dwflags);
  TLog.Add(5,'LOBBY.EnumLocalApplications');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

function TLobby.GetConnectionSettings(dwAppID: DWORD; lpData: PDPLConnection;
        var lpdwDataSize: DWORD) : HResult;
begin
try
  result := lobby2.getconnectionsettings (dwappid, lpdata, lpdwdatasize);
  TLog.Add(5,'LOBBY.GetConnectionSettings');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

function TLobby.ReceiveLobbyMessage(dwFlags: DWORD; dwAppID: DWORD;
        var lpdwMessageFlags: DWORD; lpData: Pointer; var lpdwDataSize: DWORD) :
        HResult;
begin
try
  result := lobby2.ReceiveLobbyMessage (dwflags, dwappid, lpdwmessageflags, lpdata, lpdwdatasize);

  TLog.Add(5,'LOBBY.ReceiveLobbyMessage');
  if result = DP_OK then
    begin
    TLog.Add(5,' + dwFlags          : ', dwflags);
    TLog.Add(5,' + lpdwmessageflags : ', lpdwmessageflags);
    TLog.Add(5,' + dwappid          : ', dwappid);
    TLog.Add(5, ' + lpdwdatasize     : ', lpdwdatasize);
//    TLog.Add(5,' + lpdata           : ', lpdata, lpdwDataSize);
    end
  else
    TLog.Add(5,'Error');
  TLog.Flush;    
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

function TLobby.RunApplication(dwFlags: DWORD; var lpdwAppId: DWORD;
        const lpConn: TDPLConnection; hReceiveEvent: THandle) : HResult;
begin
try
  result := lobby2.RunApplication (dwflags, lpdwappid, lpconn, hreceiveevent);
  TLog.Add(5, 'LOBBY.RunApplication');
  TLog.Add(5, ' + dwFlags                                 : ' , dwFlags);
  TLog.Add(5, ' + lpdwappid                               : ', lpdwappid);
  TLog.Add(5, ' + lpconn.dwflags                          :', lpconn.dwflags);

  TLog.Add(5, ' + lconn.lpsessiondesc.dwFlags             : ', lpconn.lpsessiondesc.dwFlags);
  TLog.Add(5, ' + lpconn.lpsessiondesc.guidInstance       : ', @lpconn.lpsessiondesc.guidInstance);
  TLog.Add(5, ' + lpconn.lpsessiondesc.guidApplication    : ', @lpconn.lpsessiondesc.guidApplication);
  TLog.Add(5, ' + lpconn.lpsessiondesc.dwMaxPlayers       : ', lpconn.lpsessiondesc.dwMaxPlayers);
  TLog.Add(5, ' + lpconn.lpsessiondesc.dwCurrentPlayers   : ', lpconn.lpsessiondesc.dwCurrentPlayers);
  TLog.Add(5, ' + lpconn.lpsessiondesc.lpszSessionName    : ' + lpconn.lpsessiondesc.lpszSessionName);
  TLog.Add(5, ' + lpconn.dwUser1                          : ', lpconn.lpsessiondesc.dwUser1);
  TLog.Add(5, ' + lpconn.dwUser2                          : ', lpconn.lpsessiondesc.dwUser2);
  TLog.Add(5, ' + lpconn.dwUser3                          : ', lpconn.lpsessiondesc.dwUser3);
  TLog.Add(5, ' + lpconn.dwUser4                          : ', lpconn.lpsessiondesc.dwUser4);

  TLog.Add(5, ' + lpconn.pPlayerName.lpszLongName         : ' + lpconn.lpPlayerName.lpszLongName);
  TLog.Add(5, ' + lpconn.pPlayerName.lpszShortName        : ' + lpconn.lpPlayerName.lpszShortName);

  TLog.Add(5, ' + lpconn.guidsp                           : ', @lpconn.guidsp);
  TLog.Add(5, ' + lpconn.lpaddress                        : ', lpconn.lpaddress);
  TLog.Add(5, ' + lpconn.dwaddresssize                    : ', lpconn.dwaddresssize);

  TLog.Add(5, ' + hreceiveevent                           : ', hreceiveevent);
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

function TLobby.SendLobbyMessage(dwFlags: DWORD; dwAppID: DWORD; const lpData;
        dwDataSize: DWORD) : HResult;
begin
try
  result := lobby2.SendLobbyMessage (dwflags, dwappid, lpdata, dwdatasize);
  TLog.Add(5, 'LOBBY.SendLobbyMessage');
  if result = DP_OK then
    begin
    TLog.Add(5, ' + dwFlags    : ', dwflags);
    TLog.Add(5, ' + dwappid    : ', dwappid);
    TLog.Add(5, ' + dwdatasize : ', dwdatasize);
//    TLog.Add(5, ' + lpdata     :', @lpdata, dwDataSize);
    end
  else
    TLog.Add(5, 'Error');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

function TLobby.SetConnectionSettings(dwFlags: DWORD; dwAppID: DWORD;
        const lpConn: TDPLConnection) : HResult;
begin
try
  result := lobby2.SetConnectionSettings (dwflags, dwappid, lpconn);
  TLog.Add(5, 'LOBBY.SetConnectionSettings');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

function TLobby.SetLobbyMessageEvent(dwFlags: DWORD; dwAppID: DWORD;
        hReceiveEvent: THandle) : HResult;
begin
try
  result := lobby2.SetLobbyMessageEvent (dwflags, dwappid, hreceiveevent);
  TLog.Add(5, 'LOBBY.SetLobbyMessageEvent');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

{--------------------------------------------------------------------}

function TLobby.CreateCompoundAddress(const lpElements: TDPCompoundAddressElement;
        dwElementCount: DWORD; lpAddress: Pointer; var lpdwAddressSize: DWORD) :
        HResult;
begin
try
  result := lobby2.CreateCompoundAddress (lpelements, dwelementcount, lpaddress, lpdwaddresssize);
  TLog.Add(5, 'LOBBY2.CreateCompoundAddress');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

{--------------------------------------------------------------------}

constructor TLobby.Create (reallobby :IDirectPlayLobby);
begin
try
  inherited Create;
  lobby1 := reallobby;
  lobby1.QueryInterface (IID_IDirectPlayLobby2, lobby2);

  TLog.Add(5, 'TLobby.Create');
  TLog.Flush;
except
  on e : Exception do
     begin
     LogException(e);
     raise;
    end;
end;
end;

end.
