unit DPlay2;

interface

uses
  DPLobby2, ActiveX, Classes, Windows, Dialogs;

type
  TOnHostSession = procedure(Sender: TObject; Result: Boolean; const guidInstance: TGUID) of object;
  TOnJoinSession = TOnHostSession;

  TDPlay2 = class
  private
    FDPApplications: TStringList;
    FLastError: String;
    FIPAddr: String;
    FInitialized: Boolean;

    dwAppID: DWORD;
    hReceiveEvent: THandle;
    ReceiveThread: TThread;
    bHost: Boolean;

//    FOnHostSession: TOnHostSession;
//    FOnJoinSession: TOnJoinSession;
//    FOnAppTerminated: TNotifyEvent;

    function CreateAddress(var lpAddress: Pointer; var dwAddressSize: DWORD): Boolean;
    function RunApplication(const applicationGUID, instanceGUID: TGUID;
        const strSessionName, strPlayerName, strPassword: String; pAddress: Pointer;
        dwAddrSize: Cardinal; bHostSession: Boolean; MaxPlayers: Integer = 0): Boolean;
    function GetLastError: String;
  public
    constructor Create;
    destructor Destroy; override;

    function Initialize: Boolean;
    //function HostSession(const ApplicationGUID: TGUID; const SessionName,
    //  PlayerName: String; const Password: String = ''; MaxPlayers: Integer = 0): Boolean;
    function JoinSession(const ApplicationGUID, guidInstance: TGUID;
      const IPAddr, PlayerName: String; const Password: String = ''): Boolean;

    property LastError: String read GetLastError;

    property Initialized: Boolean read FInitialized;
//    property OnHostSession: TOnHostSession read FOnHostSession write FOnHostSession;
//    property OnJoinSession: TOnJoinSession read FOnJoinSession write FOnJoinSession;
//    property OnAppTerminated: TNotifyEvent read FOnAppTerminated write FOnAppTerminated;
  end;

const
  { log codes }
  LC_DP_ERROR                   = $00;
  LC_INIT_DP_INTERFACES         = $01;
  LC_INIT_DP_INTERFACES_DONE    = $02;
  LC_ENUM_DP_LOCAL_APPS         = $05;
  LC_ENUM_DP_LOCAL_APPS_DONE    = $06;
  LC_HOSTING_SESSION            = $07;
  LC_HOSTING_SESSION_DONE       = $08;
  LC_JOINING_SESSION            = $09;
  LC_JOINING_SESSION_DONE       = $0A;
  LC_ERROR_CREATING_GUID        = $0B;
  LC_ERROR_CREATING_EVENT       = $0C;

const
  TA_GUID: TGUID = '{99797420-F5F5-11CF-9827-00A0241496C8}';

implementation

uses
  SysUtils;

var
  lpDPLobby: IDirectPlayLobby3A;
  lpDP: IDirectPlay3;
  Session: TGUID;

  function EnumSessionsCallback(const lpThisSD: TDPSessionDesc2;
    var lpdwTimeOut: DWORD; dwFlags: DWORD; lpContext: Pointer): BOOL; stdcall;
  begin
    if dwFlags and DPESC_TIMEDOUT<>0 then
    begin
      Result := False;
      Exit;
    end;
    Session:= lpThisSD.guidInstance;

    Result := True;
  end;

{ TDPlay }

constructor TDPlay2.Create;
begin
  inherited Create;
  lpDPLobby := nil;
  FDPApplications := TStringList.Create;
  FLastError := '';
  FIPAddr := '';
  FInitialized := False;
//  FOnHostSession := nil;
//  FOnJoinSession := nil;
//  FOnAppTerminated := nil;
  dwAppID := 0;
  hReceiveEvent := 0;
  ReceiveThread := nil;
  bHost := True;
end;

destructor TDPlay2.Destroy;
var
  lpGUID: PGUID;
  i: Integer;
begin
  for i := 0 to FDPApplications.Count - 1 do
  begin
    lpGUID := PGUID(FDPApplications.Objects[i]);
    Dispose(lpGUID);
  end;
  FDPApplications.Free;

  if Assigned(ReceiveThread) then
  begin
    ReceiveThread.Terminate;
    SetEvent(hReceiveEvent);
    ReceiveThread.WaitFor;
    ReceiveThread.Free;
  end;
  inherited Destroy;
end;

function TDPlay2.GetLastError: String;
begin
  Result := FLastError;
end;

function TDPlay2.Initialize: Boolean;
var
  hr, hr2: HRESULT;
begin
  Result := False;

  if FInitialized then
  begin
    FLastError := DPErrorString(DPERR_ALREADYINITIALIZED);
    Exit;
  end;

  hr := CoCreateInstance(CLSID_DirectPlayLobby, nil, CLSCTX_INPROC_SERVER,
            IID_IDirectPlayLobby3A, lpDPLobby);

  hr2 := CoCreateInstance(CLSID_DirectPlay, nil, CLSCTX_INPROC_SERVER,
            IID_IDirectPlay3A, lpDP);

  if FAILED(hr2) then
  begin
    FLastError := DPErrorString(hr);
    //fmMain.Log.Lines.Add('Initialize E: ' +FLastError);
    Exit;
  end;

  FInitialized := True;
  Result := True;
end;

function TDPlay2.CreateAddress(var lpAddress: Pointer; var dwAddressSize: DWORD): Boolean;
var
  addressElements: array[0..2] of TDPCompoundAddressElement;
  dwElementCount: DWORD;
  hr: HRESULT;
begin
  Result := False;

  ZeroMemory(@addressElements, SizeOf(addressElements));
  dwElementCount := 0;

  addressElements[dwElementCount].guidDataType := DPAID_ServiceProvider;
  addressElements[dwElementCount].dwDataSize := SizeOf(TGUID);
  addressElements[dwElementCount].lpData := @DPSPGUID_TCPIP;
  Inc(dwElementCount);

  addressElements[dwElementCount].guidDataType := DPAID_INet;
  addressElements[dwElementCount].dwDataSize := Length(FIPAddr) + 1;
  addressElements[dwElementCount].lpData := PChar(FIPAddr);
  Inc(dwElementCount);

  { See how much room is needed to store this address }
  hr := lpDPLobby.CreateCompoundAddress(addressElements[0], dwElementCount, nil, dwAddressSize);
  if (hr <> DPERR_BUFFERTOOSMALL) then
  begin
    FLastError := DPErrorString(hr);
    Exit;
  end;

  try
    GetMem(lpAddress, dwAddressSize);

    { Create the address }
    hr := lpDPLobby.CreateCompoundAddress(addressElements[0], dwElementCount, lpAddress, dwAddressSize);

    if FAILED(hr) then
    begin
      FreeMem(lpAddress);
      lpAddress := nil;
      FLastError := DPErrorString(hr);
      //fmMain.Log.Lines.Add('Create address: ' +FLastError);
      Exit;
    end;

    Result := True;
  except
    on E: EOutOfMemory do
    begin
      FLastError := 'There is not enough free memory';
      Exit;
    end;
  end;
end;

function TDPlay2.RunApplication(const applicationGUID, instanceGUID: TGUID;
    const strSessionName, strPlayerName, strPassword: String; pAddress: Pointer;
    dwAddrSize: Cardinal; bHostSession: Boolean; MaxPlayers: Integer = 0): Boolean;

var
  connectInfo: TDPLConnection;
  sessionInfo: TDPSessionDesc2;
  sesje: TDPSessionDesc2;
  playerName: TDPName;
  hr: HRESULT;
begin

  bHost := bHostSession;

if bHostSession = False then
begin
 ZeroMemory(@sesje, SizeOf(TDPSessionDesc2));
  with sesje do
  begin
    dwSize := SizeOf(TDPSessionDesc2);
    guidApplication := applicationGUID;  // GUID of the DirectPlay application
  end;

hr:=  lpdp.InitializeConnection(pAddress, 0);
  if hr <> S_OK then
  begin
    FLastError := DPErrorString(hr);
    //fmMain.Log.Lines.Add('Initialize connection E: ' +FLastError);
  end;

hr:= lpDP.EnumSessions(sesje, 0, @EnumSessionsCallback, Self, DPENUMSESSIONS_AVAILABLE);
  if hr <> S_OK then
  begin
    FLastError := DPErrorString(hr);
    //fmMain.Log.Lines.Add('Enum sessions E: ' +FLastError);
  end;
end else
begin
 CreateGuid(Session);
end;

  { Fill out session description }
  ZeroMemory(@sessionInfo, SizeOf(TDPSessionDesc2));
  with sessionInfo do
  begin
    dwSize := SizeOf(TDPSessionDesc2);
    dwFlags := 0;
    guidInstance := Session;        // ID for the session instance
    guidApplication := applicationGUID;  // GUID of the DirectPlay application
    dwMaxPlayers := MaxPlayers;          // Maximum # of players allowed in session
    dwCurrentPlayers := 0;               // Current # of players in session (read only)
    lpszSessionNameA := PAnsiChar(strSessionName);  // ANSI name of the session
    if (strPassword = '') then
      lpszPasswordA := nil               // ANSI password of the session
    else
      lpszPasswordA := PAnsiChar(strPassword);
    dwReserved1 := 0;                    // Reserved for future MS use
    dwReserved2 := 0;
    dwUser1 := 0;                        // For use by the application
    dwUser2 := 0;
    dwUser3 := 0;
    dwUser4 := 0;
  end;

  { Fill out player name }
  ZeroMemory(@playerName, SizeOf(TDPName));
  with playerName do
  begin
    dwSize := SizeOf(TDPName);
    dwFlags := 0;                                // Not used, must be zero
    lpszShortNameA := PAnsiChar(strPlayerName);  // ANSI short or friendly name
    lpszLongNameA := PAnsiChar(strPlayerName);   // ANSI long or formal name
  end;

  //fmMain.Log.Lines.Add('Player name: ' + playerName.lpszShortName);

  { Fill out connection description }
  ZeroMemory(@connectInfo, SizeOf(TDPLConnection));
  with connectInfo do
  begin
    dwSize := SizeOf(TDPLConnection);
    if (bHostSession) then
      dwFlags := DPLCONNECTION_CREATESESSION
    else
      dwFlags := DPLCONNECTION_JOINSESSION;
    lpSessionDesc := @sessionInfo;  // Pointer to session desc to use on connect
    lpPlayerName := @playerName;    // Pointer to Player name structure
    guidSP := DPSPGUID_TCPIP;       // GUID of the DPlay SP to use
    lpAddress := pAddress;          // Address for service provider
    dwAddressSize := dwAddrSize;    // Size of address data
  end;

  //fmMain.Log.Lines.Add('SP GUID: ' + GUIDToString(connectinfo.guidSP));

  //fmMain.Log.Lines.Add('Password: ' + sessionInfo.lpszPassword);
  //fmMain.Log.Lines.Add('Using Guid Instance: ' + GUIDToString(Session));

  sessionInfo.guidInstance:= Session;
  { launch and connect the game }
  hr := lpDPLobby.RunApplication(0, dwAppID, connectInfo, hReceiveEvent);

  if FAILED(hr) then
    FLastError := DPErrorString(hr)
  else
  begin
 {   ReceiveThread := TDPlayReceiveThread.Create(Self);
    ReceiveThread.OnTerminate := ReceiveThreadTerminate;
    ReceiveThread.Resume;   }
  end;

  Result := SUCCEEDED(hr);
end;

{function TDPlay2.HostSession(const ApplicationGUID: TGUID; const SessionName,
  PlayerName: String; const Password: String = ''; MaxPlayers: Integer = 0): Boolean;
var
  dwAddressSize: DWORD;
  guidInstance: TGUID;
  lpAddress: Pointer;
begin
  Result := False;
  if not FInitialized then
  begin
    FLastError := DPErrorString(DPERR_UNINITIALIZED);
    Exit;
  end;

  if (CoCreateGuid(guidInstance) <> S_OK) then
  begin
    Exit;
  end;

  fmMain.Log.Lines.Add(GUIDToString(guidInstance));

  lpAddress := nil;
  dwAddressSize := 0;
	// Get address to use with this service provider
  CreateAddress(lpAddress, dwAddressSize);
  // Ignore the error because pAddress will just be null

  Result := RunApplication(ApplicationGUID, guidInstance, SessionName, PlayerName,
    Password, lpAddress, dwAddressSize, True, MaxPlayers);

  if Assigned(lpAddress) then
    FreeMem(lpAddress);
end;            }

function TDPlay2.JoinSession(const ApplicationGUID, guidInstance: TGUID;
  const IPAddr, PlayerName: String; const Password: String = ''): Boolean;
var
  lpAddress: Pointer;
  dwAddressSize: DWORD;
begin
  if not FInitialized then
  begin
    FLastError := DPErrorString(DPERR_UNINITIALIZED);
    Result := False;
    Exit;
  end;

  FIPAddr := IPAddr;

  lpAddress := nil;
  dwAddressSize := 0;
	{ Get address to use with this service provider }
  CreateAddress(lpAddress, dwAddressSize);
  { Ignore the error because pAddress will just be null }

  Result := RunApplication(ApplicationGUID, guidInstance, '', PlayerName,
   Password, lpAddress, dwAddressSize, False);

  if Assigned(lpAddress) then
    FreeMem(lpAddress);
end;

initialization
  CoInitialize(nil);

finalization
  CoUninitialize;

end.
