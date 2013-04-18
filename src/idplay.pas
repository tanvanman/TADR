unit idplay;

interface

uses
  DPlay, Windows, Packet, DPLobby;

type
  TDPlay = class ({TObject} TInterfacedObject, IDirectPlay, IDirectPlay2, IDirectPlay3)
  private
    dp1 :IDirectPlay;
    dp3 :IDirectPlay3;

    tal :integer;
    started :boolean;

    RecCur :string;
    RecFirst :boolean;
    RecFrom, RecTo :TDPID;
    function Filter (p :TPacket) :boolean;
    function Showinlog (p :TPacket) :boolean;
  public
    constructor Create (realdp :IDirectPlay);

    function AddPlayerToGroup(pidGroup: TDPID; pidPlayer: TDPID) : HResult; overload; stdcall;
    function Close: HResult; overload; stdcall;
    function CreatePlayer(var lppidID: TDPID; lpPlayerFriendlyName: PChar;
        lpPlayerFormalName: PChar; lpEvent: PHandle) : HResult; overload; stdcall;
    function CreateGroup(var lppidID: TDPID; lpGroupFriendlyName: PChar;
        lpGroupFormalName: PChar) : HResult; overload; stdcall;

    function DeletePlayerFromGroup(pidGroup: TDPID; pidPlayer: TDPID) : HResult; overload; stdcall;
    function DestroyPlayer(pidID: TDPID) : HResult; overload; stdcall;
    function DestroyGroup(pidID: TDPID) : HResult; overload; stdcall;
    function EnableNewPlayers(bEnable: BOOL) : HResult; stdcall;
    function EnumGroupPlayers(pidGroupPID: TDPID; lpEnumPlayersCallback:
        TDPEnumPlayersCallback; lpContext: Pointer; dwFlags: DWORD) : HResult; overload; stdcall;
    function EnumGroups(dwSessionID: DWORD; lpEnumPlayersCallback:
        TDPEnumPlayersCallback; lpContext: Pointer; dwFlags: DWORD) : HResult; overload; stdcall;
    function EnumPlayers(dwSessionId: DWORD; lpEnumPlayersCallback:
        TDPEnumPlayersCallback; lpContext: Pointer; dwFlags: DWORD) : HResult; overload; stdcall;
    function EnumSessions(const lpSDesc: TDPSessionDesc; dwTimeout: DWORD;
        lpEnumSessionsCallback: TDPEnumSessionsCallback; lpContext: Pointer;
        dwFlags: DWORD) : HResult; overload; stdcall;
    function GetCaps(const lpDPCaps: TDPCaps) : HResult; overload; stdcall;
    function GetMessageCount(pidID: TDPID; var lpdwCount: DWORD) : HResult; overload; stdcall;
    function GetPlayerCaps(pidID: TDPID; const lpDPPlayerCaps: TDPCaps) :
        HResult; overload; stdcall;
    function GetPlayerName(pidID: TDPID; lpPlayerFriendlyName: PChar;
        var lpdwFriendlyNameLength: DWORD; lpPlayerFormalName: PChar;
        var lpdwFormalNameLength: DWORD) : HResult; overload; stdcall;
    function Initialize(const lpGUID: TGUID) : HResult; overload; stdcall;
    function Open(const lpSDesc: TDPSessionDesc) : HResult; overload; stdcall;
    function Receive(var lppidFrom, lppidTo: TDPID; dwFlags: DWORD;
        var lpvBuffer; var lpdwSize: DWORD) : HResult; overload; stdcall;
    function SaveSession(lpSessionName: PChar) : HResult; stdcall;
//    function Send(pidFrom: TDPID; pidTo: TDPID; dwFlags: DWORD;
//        const lpvBuffer; dwBuffSize: DWORD) : HResult; overload; stdcall;
    function SetPlayerName(pidID: TDPID; lpPlayerFriendlyName: PChar;
        lpPlayerFormalName: PChar) : HResult; overload; stdcall;


//    function AddPlayerToGroup(idGroup: TDPID; idPlayer: TDPID) : HResult; overload; stdcall;
//    function Close: HResult; stdcall;
    function CreateGroup(var lpidGroup: TDPID; lpGroupName: PDPName;
        const lpData; dwDataSize: DWORD; dwFlags: DWORD) : HResult; overload; stdcall;
    function CreatePlayer(var lpidPlayer: TDPID; pPlayerName: PDPName;
        hEvent: THandle; lpData: Pointer; dwDataSize: DWORD; dwFlags: DWORD) :
        HResult; overload; stdcall;
//    function DeletePlayerFromGroup(idGroup: TDPID; idPlayer: TDPID) : HResult; overload; stdcall;
//    function DestroyGroup(idGroup: TDPID) : HResult; overload; stdcall;
//    function DestroyPlayer(idPlayer: TDPID) : HResult; overload; stdcall;
    function EnumGroupPlayers(idGroup: TDPID; const lpguidInstance: TGUID;
        lpEnumPlayersCallback2: TDPEnumPlayersCallback2; lpContext: Pointer;
        dwFlags: DWORD) : HResult; overload; stdcall;
    function EnumGroups(lpguidInstance: PGUID; lpEnumPlayersCallback2:
        TDPEnumPlayersCallback2; lpContext: Pointer; dwFlags: DWORD) : HResult; overload; stdcall;
    function EnumPlayers(lpguidInstance: PGUID; lpEnumPlayersCallback2:
        TDPEnumPlayersCallback2; lpContext: Pointer; dwFlags: DWORD) : HResult; overload; stdcall;
    function EnumSessions(var lpsd: TDPSessionDesc2; dwTimeout: DWORD;
        lpEnumSessionsCallback2: TDPEnumSessionsCallback2; lpContext: Pointer;
        dwFlags: DWORD) : HResult; overload; stdcall;
    function GetCaps(var lpDPCaps: TDPCaps; dwFlags: DWORD) : HResult; overload; stdcall;
    function GetGroupData(idGroup: TDPID; lpData: Pointer; var lpdwDataSize: DWORD;
        dwFlags: DWORD) : HResult; stdcall;
    function GetGroupName(idGroup: TDPID; lpData: Pointer; var lpdwDataSize: DWORD) :
        HResult; stdcall;
//    function GetMessageCount(idPlayer: TDPID; var lpdwCount: DWORD) : HResult; overload; stdcall;
    function GetPlayerAddress(idPlayer: TDPID; lpAddress: Pointer;
        var lpdwAddressSize: DWORD) : HResult; stdcall;
    function GetPlayerCaps(idPlayer: TDPID; var lpPlayerCaps: TDPCaps;
        dwFlags: DWORD) : HResult; overload; stdcall;
    function GetPlayerData(idPlayer: TDPID; lpData: Pointer; var lpdwDataSize: DWORD;
        dwFlags: DWORD) : HResult; stdcall;
    function GetPlayerName(idPlayer: TDPID; lpData: Pointer; var lpdwDataSize: DWORD)
        : HResult; overload; stdcall;
    function GetSessionDesc(lpData: Pointer; var lpdwDataSize: DWORD) : HResult; stdcall;
//    function Initialize(const lpGUID: TGUID) : HResult; overload; stdcall;
    function Open(var lpsd: TDPSessionDesc2; dwFlags: DWORD) : HResult; overload; stdcall;
    function Receive(var lpidFrom: TDPID; var lpidTo: TDPID; dwFlags: DWORD;
        lpData: Pointer; var lpdwDataSize: DWORD) : HResult; overload; stdcall;
    function Send(idFrom: TDPID; lpidTo: TDPID; dwFlags: DWORD; const lpData;
        lpdwDataSize: DWORD) : HResult; overload; stdcall;
    function SetGroupData(idGroup: TDPID; lpData: Pointer; dwDataSize: DWORD;
        dwFlags: DWORD) : HResult; stdcall;
    function SetGroupName(idGroup: TDPID; lpGroupName: PDPName;
        dwFlags: DWORD) : HResult; stdcall;
    function SetPlayerData(idPlayer: TDPID; lpData: Pointer; dwDataSize: DWORD;
        dwFlags: DWORD) : HResult; stdcall;
    function SetPlayerName(idPlayer: TDPID; lpPlayerName: PDPName;
        dwFlags: DWORD) : HResult; overload; stdcall;
    function SetSessionDesc(const lpSessDesc: TDPSessionDesc2; dwFlags: DWORD) :
        HResult; stdcall;

    {----------------------------------------------------------------}

    (*** IDirectPlay3 methods ***)
    function AddGroupToGroup(idParentGroup: TDPID; idGroup: TDPID) : HResult; stdcall;
    function CreateGroupInGroup(idParentGroup: TDPID; var lpidGroup: TDPID;
        lpGroupName: PDPName; lpData: Pointer; dwDataSize: DWORD;
        dwFlags: DWORD) : HResult; stdcall;
    function DeleteGroupFromGroup(idParentGroup: TDPID; idGroup: TDPID) :
        HResult; stdcall;
    function EnumConnections(const lpguidApplication: TGUID;
        lpEnumCallback: TDPEnumConnectionsCallback; lpContext: Pointer;
        dwFlags: DWORD) : HResult; stdcall;
    function EnumGroupsInGroup(idGroup: TDPID; const lpguidInstance: TGUID;
        lpEnumPlayersCallback2: TDPEnumPlayersCallback2; lpContext: Pointer;
        dwFlags: DWORD) : HResult; stdcall;
    function GetGroupConnectionSettings(dwFlags: DWORD; idGroup: TDPID;
        lpData: Pointer; var lpdwDataSize: DWORD) : HResult; stdcall;
    function InitializeConnection(var lpConnection: TDPLConnection; dwFlags: DWORD) :
         HResult; stdcall;
    function SecureOpen(const lpsd: TDPSessionDesc2; dwFlags: DWORD;
        const lpSecurity: TDPSecurityDesc; const lpCredentials: TDPCredentials)
        : HResult; stdcall;
    function SendChatMessage(idFrom: TDPID; idTo: TDPID; dwFlags: DWORD;
        const lpChatMessage: TDPChat) : HResult; stdcall;
    function SetGroupConnectionSettings(dwFlags: DWORD; idGroup: TDPID;
        const lpConnection: TDPLConnection) : HResult; stdcall;
    function StartSession(dwFlags: DWORD; idGroup: TDPID) : HResult; stdcall;
    function GetGroupFlags(idGroup: TDPID; var lpdwFlags: DWORD) : HResult; stdcall;
    function GetGroupParent(idGroup: TDPID; var lpidParent: TDPID) : HResult; stdcall;
    function GetPlayerAccount(idPlayer: TDPID; dwFlags: DWORD; var lpData;
        var lpdwDataSize: DWORD) : HResult; stdcall;
    function GetPlayerFlags(idPlayer: TDPID; var lpdwFlags: DWORD) : HResult; stdcall;



{    function QueryInterface(const IID: TGUID; out Obj): HResult; stdcall;
    function _AddRef: Integer; stdcall;
    function _Release: Integer; stdcall;}

  end;

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
  Logging, sysUtils, TextData;

{--------------------------------------------------------------------}

{--------------------------------------------------------------------}

function TDPlay.AddPlayerToGroup(pidGroup: TDPID; pidPlayer: TDPID) : HResult; stdcall;
begin
  Log.Add ('IDPLAY(23).AddPlayerToGroup');
end;

function TDPlay.Close: HResult;
begin
  Result := dp3.Close;
  Log.Add ('!IDPLAY(23).Close');
end;

function TDPlay.CreatePlayer(var lppidID: TDPID; lpPlayerFriendlyName: PChar;
        lpPlayerFormalName: PChar; lpEvent: PHandle) : HResult;
begin
  Log.Add ('IDPLAY.CreatePlayer');
end;

function TDPlay.CreateGroup(var lppidID: TDPID; lpGroupFriendlyName: PChar;
        lpGroupFormalName: PChar) : HResult;
begin
  Log.Add ('IDPLAY.CreateGroup');
end;

function TDPlay.DeletePlayerFromGroup(pidGroup: TDPID; pidPlayer: TDPID) : HResult;
begin
  Log.Add ('IDPLAY(23).DeletePlayerFromGroup');
end;

function TDPlay.DestroyPlayer(pidID: TDPID) : HResult;
begin
  Result := dp3.DestroyPlayer (pidID);
  Log.Add ('!IDPLAY(23).DestroyPlayer');
end;

function TDPlay.DestroyGroup(pidID: TDPID) : HResult;
begin
  Log.Add ('IDPLAY.DestroyGroup');
end;

function TDPlay.EnableNewPlayers(bEnable: BOOL) : HResult;
begin
  Log.Add ('IDPLAY.EnableNewPlayers');
end;

function TDPlay.EnumGroupPlayers(pidGroupPID: TDPID; lpEnumPlayersCallback:
        TDPEnumPlayersCallback; lpContext: Pointer; dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY.EnumGroupPlayers');
end;

function TDPlay.EnumGroups(dwSessionID: DWORD; lpEnumPlayersCallback:
        TDPEnumPlayersCallback; lpContext: Pointer; dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY.EnumGroups');
end;

function TDPlay.EnumPlayers(dwSessionId: DWORD; lpEnumPlayersCallback:
        TDPEnumPlayersCallback; lpContext: Pointer; dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY.EnumPlayers');
end;

function TDPlay.EnumSessions(const lpSDesc: TDPSessionDesc; dwTimeout: DWORD;
        lpEnumSessionsCallback: TDPEnumSessionsCallback; lpContext: Pointer;
        dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY.EnumSessions');
end;

function TDPlay.GetCaps(const lpDPCaps: TDPCaps) : HResult;
begin
  Log.Add ('IDPLAY.GetCaps');
end;

function TDPlay.GetMessageCount(pidID: TDPID; var lpdwCount: DWORD) : HResult;
begin
  Log.Add ('IDPLAY(23).GetMessageCount');
end;

function TDPlay.GetPlayerCaps(pidID: TDPID; const lpDPPlayerCaps: TDPCaps) :
        HResult;
begin
  Log.Add ('IDPLAY.GetPlayerCaps');
end;

function TDPlay.GetPlayerName(pidID: TDPID; lpPlayerFriendlyName: PChar;
        var lpdwFriendlyNameLength: DWORD; lpPlayerFormalName: PChar;
        var lpdwFormalNameLength: DWORD) : HResult;
begin
  Log.Add ('IDPLAY.GetPlayerName');
end;

function TDPlay.Initialize(const lpGUID: TGUID) : HResult;
begin
  Log.Add ('IDPLAY(23).Initialize');
end;

function TDPlay.Open(const lpSDesc: TDPSessionDesc) : HResult;
begin
  Log.Add ('IDPLAY.Open');
end;

function TDPlay.Receive(var lppidFrom, lppidTo: TDPID; dwFlags: DWORD;
        var lpvBuffer; var lpdwSize: DWORD) : HResult;
begin
  Log.Add ('IDPLAY.Receive');
end;

function TDPlay.SaveSession(lpSessionName: PChar) : HResult;
begin
  Log.Add ('IDPLAY.SaveSession');
end;

//Returnerar true om det ska vidare
function TDPlay.Filter (p :TPacket) :boolean;
begin
  Result := true;
  exit;

  Result := false;
  case p.kind of
    $6 :;
    $7 :;
//    $17 :;
    $9  :;
    $a0 :;
    $21 :;
//    $1e :;
//    $1f :;
    $d0 :;
    $50 :;
    byte('À') :;
{    $1a :case Byte(p.FData[9]) of
           $1 :;
         else
           Result := true;
         end; }
  else
    Result := true;
  end;
end;

function TDPlay.Showinlog (p :TPacket) :boolean;
begin
  Result := true;
  exit;

  Result := false;
  case p.kind of
    $2 :;
    $1a:;
    $6 :;
    $7 :;
    byte('*'):;
    $15:;
    $2c: if (p.rawdata2[2]<>#$0b) then
            result:=true;
  else
    Result := true;
  end;
end;

function TDPlay.Send(idFrom: TDPID; lpidTo: TDPID; dwFlags: DWORD; const lpData;
        lpdwDataSize: DWORD) : HResult;
var
  p     :TPacket;
  cur   :string;
  s     :string;
  first :boolean;
begin
  Result := DP_OK;
  first := true;

  s := PtrToStr (@lpdata, lpdwDataSize);
  repeat
    cur := TPacket.Split (s);
    p := TPacket.Create (@cur[1], length (cur));

    if Filter (p) then
    begin
      Result := dp3.Send (idFrom, lpidTo, dwFlags, cur[1], Length(cur));

      if ShowInLog (p) then
      begin
        if first then
        begin
          Log.Add ('!IDPLAY(23).Send');
          Log.Add (' + idFrom       : ', idFrom);
          Log.Add (' + lpidTo       : ', lpidTo);
          Log.Add (' + dwFlags      : ', dwFlags);
          first := false;
        end;
{        if p.Kind = $2c then
          Log.add (' + data         : ' + StrToBin (p.RawData2))
        else}
    //    if p.kind = $28 then
          Log.Add (' + data         : ' + p.AsciiData);
      end;
    end;

    p.Free;

  until s = '';
end;

function TDPlay.Receive(var lpidFrom: TDPID; var lpidTo: TDPID; dwFlags: DWORD;
        lpData: Pointer; var lpdwDataSize: DWORD) : HResult;
var
  p   :TPacket;
  cur :string;
begin
//  Result := dp3.Receive (lpidFrom, lpidTo, dwFlags, lpData, lpdwDataSize);
//  exit;

  if RecCur = '' then
  begin
    Result := dp3.Receive (lpidFrom, lpidTo, dwFlags, lpData, lpdwDataSize);
    if Result <> DP_OK then
      exit;

    if lpidFrom = 0 then    //Systemmeddelande
    begin
      Log.Add ('!IDPLAY2(3).Receive');
      Log.Add (' + idFrom       : ', lpidFrom);
      Log.Add (' + lpidTo       : ', lpidTo);
      Log.Add (' + dwFlags      : ', dwFlags);
      Log.Add (' + (raw data)   : ', lpData, lpdwDataSize);

      exit;
    end;

    RecCur := PtrToStr (lpdata, lpdwDataSize);
    RecFirst := true;
    RecFrom := lpIdFrom;
    RecTo := lpidTo;
  end;

  cur := TPacket.Split (RecCur);
  p := TPacket.Create (@cur[1], length (cur));

  if not Filter (p) then
  begin
    Result := DPERR_NOMESSAGES;
    p.Free;
    exit;
  end;

  Result := DP_OK;

  if ShowInLog (p) then
  begin
    if RecFirst then
    begin
      Log.Add ('!IDPLAY2(3).Receive');
      Log.Add (' + idFrom       : ', lpidFrom);
      Log.Add (' + lpidTo       : ', lpidTo);
      Log.Add (' + dwFlags      : ', dwFlags);
      RecFirst := false;
    end;
{    if p.Kind = $2c then
      Log.add (' + data         : ' + StrToBin (p.RawData2))
    else}
 //   if p.kind = $28 then
      Log.Add (' + data         : ' + p.AsciiData);
  end;

  lpidFrom := RecFrom;
  lpidTo := RecTo;
  cur := p.TAData;
  Move (cur[1], lpData^, Length (cur));
  lpdwDataSize := Length (cur);
end;



(*
function TDPlay.Send(idFrom: TDPID; lpidTo: TDPID; dwFlags: DWORD; const lpData;
        lpdwDataSize: DWORD) : HResult;
var
  p :TPacket;
  s :string;
  cur :string;
  tmp :string;
  header :boolean;
begin

{  Result := dp3.Send (idFrom, lpidTo, dwFlags, lpData, lpdwDataSize);

  exit;}


  header := false;
  p := TPacket.Create (@lpdata, lpdwDataSize);

{  Inc (tal);}

{  if (p.kind = byte ('"')) or (p.kind = $7) then
  begin
    p.free;
    exit;
  end;}
{  if started then
  begin
    Log.Add ('Started!');
    p.Free;
    exit;
  end;}

{  if p.Kind = byte(',') then
  begin
    Inc (tal);
    if tal > 14 then
    begin
      Log.Add ('No more send! ahaha!');
      p.Free;
      exit;
    end;
  end;}


{  if lpidto = 0 then
  begin}
//    Result := dp3.Send (idFrom, lpidTo, dwFlags, lpData, lpdwDataSize);


{    if (p.Kind = $2) or (p.kind = $6) then
    begin
      p.Free;
      exit;
    end;}

//    Log.Add ('!IDPLAY(23).Send');
//    Log.Add (' + idFrom       : ', idFrom);
//    Log.Add (' + lpidTo       : ', lpidTo);
//    Log.Add (' + dwFlags      : ', dwFlags);
//    Log.Add (' + lpData       : ' + p.AsciiData);
//    Log.Add (' +  -size       : ' + IntToStr (p.Size));
//    Log.Add (' + lpdwDataSize : ', lpdwDataSize);

    s := PtrToStr (@lpdata, lpdwDataSize);

    p.Free;
    repeat
      cur := TPacket.Split (s);
      p := TPacket.Create (@cur[1], length (cur));

      cur := p.TAData;
      Result := DP_OK;
      case p.kind of
        $6 :;
        $7 :;
        $17 :;
//        $18 :;
        $9  :;
        $a0 :;
        byte('À') :;
//        $1e :;
      else
{        if p.Kind = $1a then  //Unitsync
        begin
          cur := p.RawData;
          if cur[6] = #$03 then         //Byten efter kind
          begin
            if cur[16] = #0 then
              cur[16] := #1             //Fett med lurad
            else
              exit;
          end;

          p.Free;
          p := TPacket.Create ($1a, @cur[6], Length(cur) - 5);
          cur := p.TAData;
        end;}

        Result := dp3.Send (idFrom, lpidTo, dwFlags, cur[1], Length(cur));

//        case p.kind of
//          $2 :;
//          $20 :;
//          byte('&') :;
//          $1a :;
//          byte('*') :;
//        else
          begin
            if not header then
            begin
              Log.Add ('!IDPLAY(23).Send');
              Log.Add (' + idFrom       : ', idFrom);
              Log.Add (' + lpidTo       : ', lpidTo);
              Log.Add (' + dwFlags      : ', dwFlags);
              header := true;
            end;
            Log.Add (' + data         : ' + p.AsciiData);
          end;
        end;
//      end;

      p.Free;
    until s = '';

{  end else
    Log.Add ('Blocked Send');}

end;
*)
function TDPlay.SetPlayerName(pidID: TDPID; lpPlayerFriendlyName: PChar;
        lpPlayerFormalName: PChar) : HResult;
begin
  Log.Add ('IDPLAY.SetPlayerName');
end;

{--------------------------------------------------------------------}

{function TDPlay.AddPlayerToGroup(idGroup: TDPID; idPlayer: TDPID) : HResult;
begin
  Log.Add ('AddPlayerToGroup');
end;}

{function TDPlay.Close: HResult;
begin
  Log.Add ('Close');
end;}

function TDPlay.CreateGroup(var lpidGroup: TDPID; lpGroupName: PDPName;
        const lpData; dwDataSize: DWORD; dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY2(3).CreateGroup');
end;


function TDPlay.CreatePlayer(var lpidPlayer: TDPID; pPlayerName: PDPName;
        hEvent: THandle; lpData: Pointer; dwDataSize: DWORD; dwFlags: DWORD) :
        HResult;
begin
  Result := dp3.CreatePlayer (lpidPlayer, pPlayerName, hEvent, lpData, dwDataSize, dwFlags);
  Log.Add ('!IDPLAY2(3).CreatePlayer');
  Log.Add (' + lpidPlayer                : ', lpidPlayer);
  Log.Add (' + pPlayerName.lpszLongName  : ' + pPlayerName^.lpszLongName);
  Log.Add (' + pPlayerName.lpszShortName : ' + pPlayerName^.lpszShortName);
  Log.Add (' + hEvent                    : ', hEvent);
  Log.Add (' + lpData                    : ', lpData, dwDataSize);
  Log.Add (' + dwDataSize                : ', dwDataSize);
  Log.Add (' + dwFlags                   : ', dwFlags);
end;

{function TDPlay.DeletePlayerFromGroup(idGroup: TDPID; idPlayer: TDPID) : HResult;
begin
  Log.Add ('DeletePlayerFromGroup');
end;}

{function TDPlay.DestroyGroup(idGroup: TDPID) : HResult;
begin
  Log.Add ('DestroyGroup');
end;}

{function TDPlay.DestroyPlayer(idPlayer: TDPID) : HResult;
begin
  Log.Add ('DestroyPlayer');
end;}

function TDPlay.EnumGroupPlayers(idGroup: TDPID; const lpguidInstance: TGUID;
        lpEnumPlayersCallback2: TDPEnumPlayersCallback2; lpContext: Pointer;
        dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY2(3).EnumGroupPlayers');
end;

function TDPlay.EnumGroups(lpguidInstance: PGUID; lpEnumPlayersCallback2:
        TDPEnumPlayersCallback2; lpContext: Pointer; dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY2(3).EnumGroups');
end;

function TDPlay.EnumPlayers(lpguidInstance: PGUID; lpEnumPlayersCallback2:
        TDPEnumPlayersCallback2; lpContext: Pointer; dwFlags: DWORD) : HResult;
begin
  Result := dp3.EnumPlayers (lpguidInstance, lpEnumPlayersCallback2, lpContext, dwFlags);
  Log.Add ('!IDPLAY2(3).EnumPlayers');
  if lpguidInstance <> nil then
    Log.Add (' + lpguidInstance^ : ', lpguidInstance^)
  else
    Log.Add (' + lpguidInstance : ', lpguidInstance);

  Log.Add (' + lpEnumPlayersCallback2 : ', @lpEnumPlayersCallback2);
  Log.Add (' + lpContext : ', lpContext);
  Log.Add (' + dwFlags : ', dwFlags);
end;

function TDPlay.EnumSessions(var lpsd: TDPSessionDesc2; dwTimeout: DWORD;
        lpEnumSessionsCallback2: TDPEnumSessionsCallback2; lpContext: Pointer;
        dwFlags: DWORD) : HResult;
begin
  Result := dp3.EnumSessions (lpsd, dwTimeout, lpEnumSessionsCallback2, lpContext, dwFlags);
  Log.Add ('!IDPLAY2(3).EnumSessions');
  Log.Add (' + lpsd.dwFlags            : ', lpsd.dwFlags);
  Log.Add (' + lpsd.guidInstance       : ', lpsd.guidInstance);
  Log.Add (' + lpsd.guidApplication    : ', lpsd.guidApplication);
  Log.Add (' + lpsd.dwMaxPlayers       : ', lpsd.dwMaxPlayers);
  Log.Add (' + lpsd.dwCurrentPlayers   : ', lpsd.dwCurrentPlayers);
  Log.Add (' + lpsd.lpszSessionName    : ' + lpsd.lpszSessionName);
  Log.Add (' + lpsd.lpszPassword       : ' + lpsd.lpszPassWord);
  Log.Add (' + dwTimeout               : ', dwTimeout);
  Log.Add (' + lpEnumSessionsCallback2 : ', @lpenumsessionsCallback2);
  Log.Add (' + lpContext               : ', lpContext);
  Log.Add (' + dwFlags                 : ', dwFlags);
end;

function TDPlay.GetCaps(var lpDPCaps: TDPCaps; dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY2(3).GetCaps');
end;

function TDPlay.GetGroupData(idGroup: TDPID; lpData: Pointer; var lpdwDataSize: DWORD;
        dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY2(3).GetGroupData');
end;

function TDPlay.GetGroupName(idGroup: TDPID; lpData: Pointer; var lpdwDataSize: DWORD) :
        HResult;
begin
  Log.Add ('IDPLAY2(3).GetGroupName');
end;

{function TDPlay.GetMessageCount(idPlayer: TDPID; var lpdwCount: DWORD) : HResult;
begin
  Log.Add ('GetMessageCount');
end;}

function TDPlay.GetPlayerAddress(idPlayer: TDPID; lpAddress: Pointer;
        var lpdwAddressSize: DWORD) : HResult;
begin
  Result := dp3.GetPlayerAddress (idPlayer, lpAddress, lpdwAddressSize);
  Log.Add ('!IDPLAY2(3).GetPlayerAddress');
end;

function TDPlay.GetPlayerCaps(idPlayer: TDPID; var lpPlayerCaps: TDPCaps;
        dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY2(3).GetPlayerCaps');
end;

function TDPlay.GetPlayerData(idPlayer: TDPID; lpData: Pointer; var lpdwDataSize: DWORD;
        dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY2(3).GetPlayerData');
end;

function TDPlay.GetPlayerName(idPlayer: TDPID; lpData: Pointer; var lpdwDataSize: DWORD)
        : HResult;
begin
  Result := dp3.GetPlayerName (idPlayer, lpData, lpdwDataSize);
  Log.Add ('!IDPLAY2(3).GetPlayerName');
  if lpData = nil then
  begin
    Log.Add (' + <checking size>');
    Log.Add (' + idPlayer     : ', idPlayer);
    Log.Add (' + lpdwDataSize : ', lpdwDataSize);
  end else
  begin
    Log.Add (' + idPlayer     : ', idPlayer);
    Log.Add (' + lpdwDataSize : ', lpdwDataSize);
    Log.Add (' + lpData       : ', lpData, lpdwDataSize);
  end;
end;

function TDPlay.GetSessionDesc(lpData: Pointer; var lpdwDataSize: DWORD) : HResult;
var
  lpsd :TDPSessionDesc2;
begin
  Result := dp3.GetSessionDesc (lpData, lpdwDataSize);
  Log.Add ('!IDPLAY2(3).GetSessionDesc');

  if lpData = nil then
  begin
    Log.Add (' + <checking size>');
    Log.Add (' + lpdwDataSize : ', lpdwDataSize);
  end else
  begin
    lpsd := PDPSessionDesc2(lpData)^;
    Log.Add (' + <actually getting info>');
    Log.Add (' + lpdwDataSize            : ', lpdwDataSize);

    Log.Add (' + lpsd.dwFlags            : ', lpsd.dwFlags);
    Log.Add (' + lpsd.guidInstance       : ', lpsd.guidInstance);
    Log.Add (' + lpsd.guidApplication    : ', lpsd.guidApplication);
    Log.Add (' + lpsd.dwMaxPlayers       : ', lpsd.dwMaxPlayers);
    Log.Add (' + lpsd.dwCurrentPlayers   : ', lpsd.dwCurrentPlayers);
    Log.Add (' + lpsd.lpszSessionName    : ' + lpsd.lpszSessionName);
    Log.Add (' + lpsd.lpszPassword       : ' + lpsd.lpszPassWord);
  end;
end;

{function TDPlay.Initialize(const lpGUID: TGUID) : HResult;
begin
  Log.Add ('Initialize');
end;}

function TDPlay.Open(var lpsd: TDPSessionDesc2; dwFlags: DWORD) : HResult;
begin
  Result := dp3.Open (lpsd, dwFlags);
  Log.Add ('!IDPLAY2(3).Open');

  Log.Add (' + lpsd.dwFlags            : ', lpsd.dwFlags);
  Log.Add (' + lpsd.guidInstance       : ', lpsd.guidInstance);
  Log.Add (' + lpsd.guidApplication    : ', lpsd.guidApplication);
  Log.Add (' + lpsd.dwMaxPlayers       : ', lpsd.dwMaxPlayers);
  Log.Add (' + lpsd.dwCurrentPlayers   : ', lpsd.dwCurrentPlayers);
  Log.Add (' + lpsd.lpszSessionName    : ' + lpsd.lpszSessionName);
  Log.Add (' + lpsd.lpszPassword       : ' + lpsd.lpszPassWord);

  Log.Add (' + dwFlags                 : ', dwFlags);
end;
(*
function TDPlay.Receive(var lpidFrom: TDPID; var lpidTo: TDPID; dwFlags: DWORD;
        lpData: Pointer; var lpdwDataSize: DWORD) : HResult;
var
   olpidFrom: TDPID;
   olpidTo: TDPID;
   odwFlags: DWORD;
   olpdwDataSize: DWORD;
   p  :TPacket;
begin
  olpidFrom:=lpidFrom;
  olpidTo:=lpidTo;
  odwFlags:=dwFlags;
  olpdwDataSize:=lpdwDataSize;

  Result := dp3.Receive (lpidFrom, lpidTo, dwFlags, lpData, lpdwDataSize);

  exit;

  if Result = DPERR_NOMESSAGES then
  begin
    Log.Add ('<no messages waiting> ', olpidFrom);
  end else
  begin
{    if lpidfrom <> 0 then
    begin
      p := TPacket.Create (lpData, lpdwDataSize);
      if p.Serial = $ffffffff then
      begin
        Log.Add ('Blocked receive');
        p.Free;
        exit;
      end;
      p.Free;
    end;}



    Log.Add ('!IDPLAY2(3).Receive');
    Log.Add (' <before call>');
    Log.Add (' + lpidFrom     : ', olpidFrom);
    Log.Add (' + lpidTo       : ', olpidTo);
    Log.Add (' + dwFlags      : ', odwFlags);
 //   Log.Add (' + lpdwDataSize : ', olpdwDataSize);

    Log.Add (' <after call>');
    Log.Add (' + idFrom       : ', lpidFrom);
    Log.Add (' + idTo         : ', lpidTo);
    Log.Add (' + lpdwDataSize : ', lpdwDataSize);
    if lpidfrom = 0 then
      Log.Add (' + (raw data)   : ', lpData, lpdwDataSize)
    else
    begin
      p := TPacket.Create (lpData, lpdwDataSize);
//      Log.Add (' + lpdata       : ', lpData
      Log.Add (' + lpData       : ' + p.AsciiData);
      Log.Add (' +  -size       : ' + IntToStr (p.Size));
    end;
    Log.Add (' + <Result>     : ' + ErrorString (Result));
  end;
end;
*)

{function TDPlay.Send(idFrom: TDPID; lpidTo: TDPID; dwFlags: DWORD; const lpData;
        lpdwDataSize: DWORD) : HResult;
begin
  Log.Add ('Send');
end;}

function TDPlay.SetGroupData(idGroup: TDPID; lpData: Pointer; dwDataSize: DWORD;
        dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY2(3).SetGroupData');
end;

function TDPlay.SetGroupName(idGroup: TDPID; lpGroupName: PDPName;
        dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY2(3).SetGroupName');
end;

function TDPlay.SetPlayerData(idPlayer: TDPID; lpData: Pointer; dwDataSize: DWORD;
        dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY2(3).SetPlayerData');
end;

function TDPlay.SetPlayerName(idPlayer: TDPID; lpPlayerName: PDPName;
        dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY2(3).SetPlayerName');
end;

function TDPlay.SetSessionDesc(const lpSessDesc: TDPSessionDesc2; dwFlags: DWORD) :
        HResult;
var
  lpsd :TDPSessionDesc2;
begin
  lpsd := lpSessDesc;

  Result := dp3.SetSessionDesc (lpSessDesc, dwFlags);
  Log.Add ('!IDPLAY2(3).SetSessionDesc');
  Log.Add (' + lpsd.dwFlags            : ', lpsd.dwFlags);
  Log.Add (' + lpsd.guidInstance       : ', lpsd.guidInstance);
  Log.Add (' + lpsd.guidApplication    : ', lpsd.guidApplication);
  Log.Add (' + lpsd.dwMaxPlayers       : ', lpsd.dwMaxPlayers);
  Log.Add (' + lpsd.dwCurrentPlayers   : ', lpsd.dwCurrentPlayers);
  Log.Add (' + lpsd.lpszSessionName    : ' + lpsd.lpszSessionName);
  Log.Add (' + dwUser1                 : ', lpsd.dwUser1);
  Log.Add (' + dwUser2                 : ', lpsd.dwUser2);
  Log.Add (' + dwUser3                 : ', lpsd.dwUser3);
  Log.Add (' + dwUser4                 : ', lpsd.dwUser4);
//  Log.Add (' + lpsd.lpszPassword       : ' + lpsd.lpszPassWord);
  Log.Add (' + dwFlags                 : ', dwFlags);
end;

{--------------------------------------------------------------------}

function TDPlay.AddGroupToGroup(idParentGroup: TDPID; idGroup: TDPID) : HResult;
begin
  Log.Add ('IDPLAY3.AddGroupToGroup');
end;

function TDPlay.CreateGroupInGroup(idParentGroup: TDPID; var lpidGroup: TDPID;
        lpGroupName: PDPName; lpData: Pointer; dwDataSize: DWORD;
        dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY3.CreateGroupInGroup');
end;

function TDPlay.DeleteGroupFromGroup(idParentGroup: TDPID; idGroup: TDPID) :
        HResult;
begin
  Log.Add ('IDPLAY3.DeleteGroupFromGroup');
end;

function TDPlay.EnumConnections(const lpguidApplication: TGUID;
        lpEnumCallback: TDPEnumConnectionsCallback; lpContext: Pointer;
        dwFlags: DWORD) : HResult;
begin
  Result := dp3.EnumConnections (lpguidApplication, lpEnumCallback, lpContext, dwFlags);
  Log.Add ('!IDPLAY3.EnumConnections');
  Log.Add (' + lpguidApplication : ', lpguidapplication);
  Log.Add (' + lpEnumCallback    : ', @lpenumcallback);
  Log.Add (' + lpContext         : ', lpcontext);
  Log.Add (' + dwFlags           : ', dwFlags);
end;

function TDPlay.EnumGroupsInGroup(idGroup: TDPID; const lpguidInstance: TGUID;
        lpEnumPlayersCallback2: TDPEnumPlayersCallback2; lpContext: Pointer;
        dwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY3.EnumGroupsInGroup');
end;

function TDPlay.GetGroupConnectionSettings(dwFlags: DWORD; idGroup: TDPID;
        lpData: Pointer; var lpdwDataSize: DWORD) : HResult;
begin
  Log.Add ('IDPLAY3.GetGroupConnectionSettings');
end;

function TDPlay.InitializeConnection(var lpConnection: TDPLConnection; dwFlags: DWORD) :
         HResult;
begin
  Result := dp3.InitializeConnection (lpConnection, dwFlags);
  Log.Add ('!IDPLAY3.InitializeConnection');
  Log.Add (' + lpConnection.dwFlags       : ', lpConnection.dwFlags);
  Log.Add (' + lpConnection.lpSessionDesc : ', lpConnection.lpSessionDesc);
  Log.Add (' + lpConnection.lpPlayerName  : ', lpConnection.lpPlayerName);
  Log.Add (' + lpConnection.guidSP        : ', lpConnection.guidSP);
  Log.Add (' + lpConnection.lpAddress     : ', lpConnection.lpAddress);
  Log.Add (' + lpConnection.dwAddressSize : ', lpConnection.dwAddressSize);
end;

function TDPlay.SecureOpen(const lpsd: TDPSessionDesc2; dwFlags: DWORD;
        const lpSecurity: TDPSecurityDesc; const lpCredentials: TDPCredentials)
        : HResult;
begin
  Log.Add ('IDPLAY3.SecureOpen');
end;

function TDPlay.SendChatMessage(idFrom: TDPID; idTo: TDPID; dwFlags: DWORD;
        const lpChatMessage: TDPChat) : HResult;
begin
  Log.Add ('IDPLAY3.SendChatMessage');
end;

function TDPlay.SetGroupConnectionSettings(dwFlags: DWORD; idGroup: TDPID;
        const lpConnection: TDPLConnection) : HResult;
begin
  Log.Add ('IDPLAY3.SetGroupConnectionSettings');
end;

function TDPlay.StartSession(dwFlags: DWORD; idGroup: TDPID) : HResult;
begin
  Log.Add ('IDPLAY3.StartSession');
end;

function TDPlay.GetGroupFlags(idGroup: TDPID; var lpdwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY3.GetGroupFlags');
end;

function TDPlay.GetGroupParent(idGroup: TDPID; var lpidParent: TDPID) : HResult;
begin
  Log.Add ('IDPLAY3.GetGroupParent');
end;

function TDPlay.GetPlayerAccount(idPlayer: TDPID; dwFlags: DWORD; var lpData;
        var lpdwDataSize: DWORD) : HResult;
begin
  Log.Add ('IDPLAY3.GetPlayerAccount');
end;

function TDPlay.GetPlayerFlags(idPlayer: TDPID; var lpdwFlags: DWORD) : HResult;
begin
  Log.Add ('IDPLAY3.GetPlayerFlags');
end;

{function TDPlay.QueryInterface(const IID: TGUID; out Obj): HResult; stdcall;
begin
  Log.Add ('QueryInterface');
end;

function TDPlay._AddRef: Integer; stdcall;
begin
  Log.Add ('AddRef');
end;

function TDPlay._Release: Integer; stdcall;
begin
  Log.Add ('Release');
end;}


{function TDPlay.QueryInterface(const IID: TGUID; out Obj): HResult; stdcall;
begin
  Log.Add ('IDPLAY.QueryInterface');
  if IsSameGuid (IID, IID_IDirectPlay3) then
  begin
    Log.Add ('IDPLAY + IID_IDirectPlay3');
    TDPlay3(Obj) := TDPlay3.Create;
    Log.Add ('IDPLAY + Created');
    Result := DP_OK;
  end else
  begin
    Log.Add ('IDPLAY + Unsupported interface requested');
    Result := DPERR_NOINTERFACE;
  end;
end;

function TDPlay._AddRef: Integer; stdcall;
begin
  Log.Add ('IDPLAY._AddRef');
  Result := 2;
end;

function TDPlay._Release: Integer; stdcall;
begin
  Log.Add ('IDPLAY._Release');
end;}

{--------------------------------------------------------------------}

constructor TDPlay.Create (realdp :IDirectPlay);
begin
  inherited Create;

  dp1 := realdp;
  dp1.QueryInterface (IID_IDirectPlay3, dp3);

  tal := 0;
  started := false;

  RecFirst := true;
  RecCur := '';
end;


{--------------------------------------------------------------------}

function TLobby.Connect(dwFlags: DWORD; var lplpDP: IDirectPlay2;
        pUnk: IUnknown) : HResult;
var
  dp :IDirectPlay2;
  dp1 :IDirectPlay;
begin
  Result := lobby2.Connect (dwFlags, dp, pUnk);
  Log.Add ('LOBBY.Connect');
  Log.Add (' + dwFlags : ', dwFlags);

  dp.QueryInterface (IID_IDirectPlay, dp1);
  lplpdp := TDPlay.Create (dp1);
end;

function TLobby.CreateAddress(const guidSP, guidDataType: TGUID; const lpData;
        dwDataSize: DWORD; var lpAddress; var lpdwAddressSize: DWORD) : HResult;
begin
  Result := lobby2.CreateAddress (guidSp, guiddatatype, lpdata, dwdatasize, lpaddress, lpdwaddresssize);
  Log.Add ('LOBBY.CreateAddress');
end;

function TLobby.EnumAddress(lpEnumAddressCallback: TDPEnumAdressCallback;
        const lpAddress; dwAddressSize: DWORD; lpContext : Pointer) : HResult;
begin
  Result := lobby2.enumaddress (lpenumaddresscallback, lpaddress, dwaddresssize, lpcontext);
  Log.Add ('LOBBY.EnumAddress');
end;

function TLobby.EnumAddressTypes(lpEnumAddressTypeCallback:
        TDPLEnumAddressTypesCallback; const guidSP: TGUID; lpContext: Pointer;
        dwFlags: DWORD) : HResult;
begin
  result := lobby2.enumaddresstypes (lpenumaddresstypecallback, guidsp, lpcontext, dwflags);
  Log.Add ('LOBBY.EnumAddressTypes');
end;

function TLobby.EnumLocalApplications(lpEnumLocalAppCallback:
        TDPLEnumLocalApplicationsCallback; lpContext: Pointer; dwFlags: DWORD)
        : HResult;
begin
  result := lobby2.EnumLocalApplications (lpenumlocalappcallback, lpcontext, dwflags);
  Log.Add ('LOBBY.EnumLocalApplications');
end;

function TLobby.GetConnectionSettings(dwAppID: DWORD; lpData: PDPLConnection;
        var lpdwDataSize: DWORD) : HResult;
begin
  result := lobby2.getconnectionsettings (dwappid, lpdata, lpdwdatasize);
  Log.Add ('LOBBY.GetConnectionSettings');
end;

function TLobby.ReceiveLobbyMessage(dwFlags: DWORD; dwAppID: DWORD;
        var lpdwMessageFlags: DWORD; lpData: Pointer; var lpdwDataSize: DWORD) :
        HResult;
begin
  result := lobby2.ReceiveLobbyMessage (dwflags, dwappid, lpdwmessageflags, lpdata, lpdwdatasize);

  Log.Add ('LOBBY.ReceiveLobbyMessage');
  if result = DP_OK then
  begin
    Log.add (' + dwFlags          : ', dwflags);
    Log.Add (' + lpdwmessageflags : ', lpdwmessageflags);
    Log.Add (' + dwappid          : ', dwappid);
    Log.Add (' + lpdwdatasize     : ', lpdwdatasize);
    log.add (' + lpdata           : ', lpdata, lpdwDataSize);
  end else
    Log.Add ('Error');
end;

function TLobby.RunApplication(dwFlags: DWORD; var lpdwAppId: DWORD;
        const lpConn: TDPLConnection; hReceiveEvent: THandle) : HResult;
begin
  result := lobby2.RunApplication (dwflags, lpdwappid, lpconn, hreceiveevent);
  Log.Add ('LOBBY.RunApplication');
  Log.Add (' + dwFlags                                 : ' , dwFlags);
  Log.Add (' + lpdwappid                               : ', lpdwappid);
  Log.Add (' + lpconn.dwflags                          :', lpconn.dwflags);

  Log.Add (' + lconn.lpsessiondesc.dwFlags             : ', lpconn.lpsessiondesc.dwFlags);
  Log.Add (' + lpconn.lpsessiondesc.guidInstance       : ', lpconn.lpsessiondesc.guidInstance);
  Log.Add (' + lpconn.lpsessiondesc.guidApplication    : ', lpconn.lpsessiondesc.guidApplication);
  Log.Add (' + lpconn.lpsessiondesc.dwMaxPlayers       : ', lpconn.lpsessiondesc.dwMaxPlayers);
  Log.Add (' + lpconn.lpsessiondesc.dwCurrentPlayers   : ', lpconn.lpsessiondesc.dwCurrentPlayers);
  Log.Add (' + lpconn.lpsessiondesc.lpszSessionName    : ' + lpconn.lpsessiondesc.lpszSessionName);
  Log.Add (' + lpconn.dwUser1                          : ', lpconn.lpsessiondesc.dwUser1);
  Log.Add (' + lpconn.dwUser2                          : ', lpconn.lpsessiondesc.dwUser2);
  Log.Add (' + lpconn.dwUser3                          : ', lpconn.lpsessiondesc.dwUser3);
  Log.Add (' + lpconn.dwUser4                          : ', lpconn.lpsessiondesc.dwUser4);

  Log.Add (' + lpconn.pPlayerName.lpszLongName         : ' + lpconn.lpPlayerName.lpszLongName);
  Log.Add (' + lpconn.pPlayerName.lpszShortName        : ' + lpconn.lpPlayerName.lpszShortName);

  Log.Add (' + lpconn.guidsp                           : ', lpconn.guidsp);
  log.add (' + lpconn.lpaddress                        : ', lpconn.lpaddress);
  log.add (' + lpconn.dwaddresssize                    : ', lpconn.dwaddresssize);

  Log.add (' + hreceiveevent                           : ', hreceiveevent);
end;

function TLobby.SendLobbyMessage(dwFlags: DWORD; dwAppID: DWORD; const lpData;
        dwDataSize: DWORD) : HResult;
begin
  result := lobby2.SendLobbyMessage (dwflags, dwappid, lpdata, dwdatasize);
  Log.Add ('LOBBY.SendLobbyMessage');
  if result = DP_OK then
  begin
    Log.add (' + dwFlags    : ', dwflags);
    Log.Add (' + dwappid    : ', dwappid);
    Log.add (' + dwdatasize : ', dwdatasize);
    log.add (' + lpdata     :', @lpdata, dwDataSize);
  end else
    Log.Add ('Error');
end;

function TLobby.SetConnectionSettings(dwFlags: DWORD; dwAppID: DWORD;
        const lpConn: TDPLConnection) : HResult;
begin
  result := lobby2.SetConnectionSettings (dwflags, dwappid, lpconn);
  Log.Add ('LOBBY.SetConnectionSettings');
end;

function TLobby.SetLobbyMessageEvent(dwFlags: DWORD; dwAppID: DWORD;
        hReceiveEvent: THandle) : HResult;
begin
  result := lobby2.SetLobbyMessageEvent (dwflags, dwappid, hreceiveevent);
  Log.Add ('LOBBY.SetLobbyMessageEvent');
end;

{--------------------------------------------------------------------}

function TLobby.CreateCompoundAddress(const lpElements: TDPCompoundAddressElement;
        dwElementCount: DWORD; lpAddress: Pointer; var lpdwAddressSize: DWORD) :
        HResult;
begin
  result := lobby2.CreateCompoundAddress (lpelements, dwelementcount, lpaddress, lpdwaddresssize);
  Log.Add ('LOBBY2.CreateCompoundAddress');
end;

{--------------------------------------------------------------------}

constructor TLobby.Create (reallobby :IDirectPlayLobby);
begin
  inherited Create;

  lobby1 := reallobby;
  lobby1.QueryInterface (IID_IDirectPlayLobby2, lobby2);

  Log.Add ('TLobby.Create');
end;

end.
