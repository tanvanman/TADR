// -----------------------------------------------------------------------------

function TDPlay.AddPlayerToGroup(pidGroup: TDPID; pidPlayer: TDPID) : HResult; stdcall;
begin
try
//  TLog.Add ('IDPLAY(23).AddPlayerToGroup');
// TLog.Flush;
  asm int 3 end; Result := dp1.AddPlayerToGroup( pidGroup, pidPlayer);
except
  on e : Exception do
    begin
    if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.CreatePlayer(var lppidID: TDPID; lpPlayerFriendlyName: PChar;
        lpPlayerFormalName: PChar; lpEvent: PHandle) : HResult;
begin
try
//  TLog.Add ('IDPLAY.CreatePlayer');
// TLog.Flush;
  asm int 3 end; Result := dp1.CreatePlayer( lppidID, lpPlayerFriendlyName, lpPlayerFormalName, lpEvent );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
     end;
end;
end;

function TDPlay.CreateGroup(var lppidID: TDPID; lpGroupFriendlyName: PChar;
        lpGroupFormalName: PChar) : HResult;
begin
try
//  TLog.Add ('IDPLAY.CreateGroup');
// TLog.Flush;
  asm int 3 end; Result := dp1.CreateGroup( lppidID, lpGroupFriendlyName, lpGroupFormalName );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.DeletePlayerFromGroup(pidGroup: TDPID; pidPlayer: TDPID) : HResult;
begin
try
//  TLog.Add ('IDPLAY.DeletePlayerFromGroup');
// TLog.Flush;
  asm int 3 end; Result := dp1.DeletePlayerFromGroup( pidGroup, pidPlayer );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.DestroyGroup(pidID: TDPID) : HResult;
begin
try
//  TLog.Add ('IDPLAY.DestroyGroup');
// TLog.Flush;
  asm int 3 end; Result := dp1.DestroyGroup( pidID );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.EnableNewPlayers(bEnable: BOOL) : HResult;
begin
try
//  TLog.Add ('IDPLAY.EnableNewPlayers');
// TLog.Flush;
  asm int 3 end; Result := dp1.EnableNewPlayers( bEnable );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.EnumGroupPlayers(pidGroupPID: TDPID; lpEnumPlayersCallback:
        TDPEnumPlayersCallback; lpContext: Pointer; dwFlags: DWORD) : HResult;
begin
try
//  TLog.Add ('IDPLAY.EnumGroupPlayers');
// TLog.Flush;
  asm int 3 end; Result := dp1.EnumGroupPlayers( pidGroupPID, lpEnumPlayersCallback, lpContext, dwFlags );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.EnumGroups(dwSessionID: DWORD; lpEnumPlayersCallback:
        TDPEnumPlayersCallback; lpContext: Pointer; dwFlags: DWORD) : HResult;
begin
try
 // TLog.Add ('IDPLAY.EnumGroups');
// TLog.Flush;
  asm int 3 end; Result := dp1.EnumGroupPlayers( dwSessionID, lpEnumPlayersCallback, lpContext, dwFlags );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.EnumPlayers(dwSessionId: DWORD; lpEnumPlayersCallback:
        TDPEnumPlayersCallback; lpContext: Pointer; dwFlags: DWORD) : HResult;
begin
try
 // TLog.Add ('IDPLAY.EnumPlayers');
// TLog.Flush;
  asm int 3 end; Result := dp1.EnumGroupPlayers( dwSessionID, lpEnumPlayersCallback, lpContext, dwFlags );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.EnumSessions(const lpSDesc: TDPSessionDesc; dwTimeout: DWORD;
        lpEnumSessionsCallback: TDPEnumSessionsCallback; lpContext: Pointer;
        dwFlags: DWORD) : HResult;
begin
try
 // TLog.Add ('IDPLAY.EnumSessions');
// TLog.Flush;
  asm int 3 end; Result := dp1.EnumSessions( lpSDesc, dwTimeout, lpEnumSessionsCallback, lpContext, dwFlags );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.GetCaps(const lpDPCaps: TDPCaps) : HResult;
begin
try
 // TLog.Add ('IDPLAY.GetCaps');
// TLog.Flush;
  asm int 3 end; Result := dp1.GetCaps( lpDPCaps );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.GetMessageCount(pidID: TDPID; var lpdwCount: DWORD) : HResult;
begin
try
//  TLog.Add ('IDPLAY(23).GetMessageCount');
// TLog.Flush;
  asm int 3 end; Result := dp1.GetMessageCount( pidID, lpdwCount );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.GetPlayerCaps(pidID: TDPID; const lpDPPlayerCaps: TDPCaps) : HResult;
begin
try
  //TLog.Add ('IDPLAY.GetPlayerCaps');
// TLog.Flush;
  asm int 3 end; Result := dp1.GetPlayerCaps( pidID, lpDPPlayerCaps );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.GetPlayerName(pidID: TDPID; lpPlayerFriendlyName: PChar;
        var lpdwFriendlyNameLength: DWORD; lpPlayerFormalName: PChar;
        var lpdwFormalNameLength: DWORD) : HResult;
begin
try
//  TLog.Add ('IDPLAY.GetPlayerName');
// TLog.Flush;
  asm int 3 end; Result := dp1.GetPlayerName( pidID, lpPlayerFriendlyName, lpdwFriendlyNameLength,
                               lpPlayerFormalName, lpdwFormalNameLength );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;

function TDPlay.Initialize(const lpGUID: TGUID) : HResult;
begin
try
  //TLog.Add ('IDPLAY(23).Initialize');
// TLog.Flush;
  asm int 3 end; Result := dp1.Initialize(lpGUID);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;
    end;
end;
end;


function TDPlay.Open(const lpSDesc: TDPSessionDesc) : HResult;
begin
try
 // TLog.Add ('IDPLAY.Open');
// TLog.Flush;
  asm int 3 end; Result := dp1.Open(lpSDesc);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.Receive(var lppidFrom, lppidTo: TDPID; dwFlags: DWORD;
        var lpvBuffer; var lpdwSize: DWORD) : HResult;
begin
try
 // TLog.Add ('IDPLAY.Receive');
// TLog.Flush;
  asm int 3 end; Result := dp1.Receive( lppidFrom, lppidTo, dwFlags, lpvBuffer, lpdwSize);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.SaveSession(lpSessionName: PChar) : HResult;
begin
try
//  TLog.Add ('IDPLAY.SaveSession');
// TLog.Flush;
  asm int 3 end; Result := SaveSession( lpSessionName );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.SetPlayerName(pidID: TDPID; lpPlayerFriendlyName: PChar;
        lpPlayerFormalName: PChar) : HResult;
begin
try
 // TLog.Add ('IDPLAY.SetPlayerName');
// TLog.Flush;
  asm int 3 end; Result := dp1.SetPlayerName( pidID, lpPlayerFriendlyName, lpPlayerFormalName);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.CreateGroup(var lpidGroup: TDPID; lpGroupName: PDPName;
        const lpData; dwDataSize: DWORD; dwFlags: DWORD) : HResult;
begin
try
//  TLog.Add ('IDPLAY2(3).CreateGroup');
// TLog.Flush;
  asm int 3 end; Result := dp3.CreateGroup( lpidGroup, lpGroupName, lpData, dwDataSize, dwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.EnumGroupPlayers(idGroup: TDPID; const lpguidInstance: TGUID;
        lpEnumPlayersCallback2: TDPEnumPlayersCallback2; lpContext: Pointer;
        dwFlags: DWORD) : HResult;
begin
try
  //TLog.Add ('IDPLAY2(3).EnumGroupPlayers');
// TLog.Flush;
  asm int 3 end; Result := dp3.EnumGroupPlayers( idGroup, lpguidInstance,
                                                 lpEnumPlayersCallback2, lpContext, dwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.EnumGroups(lpguidInstance: PGUID; lpEnumPlayersCallback2:
        TDPEnumPlayersCallback2; lpContext: Pointer; dwFlags: DWORD) : HResult;
begin
try
  //TLog.Add ('IDPLAY2(3).EnumGroups');
// TLog.Flush;
  asm int 3 end; Result := dp3.EnumGroups(lpguidInstance,lpEnumPlayersCallback2,lpContext,dwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.GetCaps(var lpDPCaps: TDPCaps; dwFlags: DWORD) : HResult;
begin
try
//  TLog.Add ('IDPLAY2(3).GetCaps');
// TLog.Flush;
  asm int 3 end; Result := dp3.GetCaps( lpDPCaps, dwFlags );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.GetGroupData(idGroup: TDPID; lpData: Pointer; var lpdwDataSize: DWORD;
        dwFlags: DWORD) : HResult;
begin
try
 // TLog.Add ('IDPLAY2(3).GetGroupData');
// TLog.Flush;
  asm int 3 end; Result := dp3.GetGroupData( idGroup, lpData, lpdwDataSize, dwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.GetGroupName(idGroup: TDPID; lpData: Pointer; var lpdwDataSize: DWORD) :
        HResult;
begin
try
 // TLog.Add ('IDPLAY2(3).GetGroupName');
// TLog.Flush;
  asm int 3 end; Result := dp3.GetGroupName( idGroup, lpData, lpdwDataSize);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.GetPlayerCaps(idPlayer: TDPID; var lpPlayerCaps: TDPCaps;
        dwFlags: DWORD) : HResult;
begin
try
//  TLog.Add ('IDPLAY2(3).GetPlayerCaps');
// TLog.Flush;
  asm int 3 end; Result := dp3.GetPlayerCaps( idPlayer, lpPlayerCaps, dwFlags );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.GetPlayerData(idPlayer: TDPID; lpData: Pointer; var lpdwDataSize: DWORD;
        dwFlags: DWORD) : HResult;
begin
try
 // TLog.Add ('IDPLAY2(3).GetPlayerData');
// TLog.Flush;
  asm int 3 end; Result := dp3.GetPlayerData( idPlayer, lpData, lpdwDataSize, dwFlags );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.SetGroupData(idGroup: TDPID; lpData: Pointer; dwDataSize: DWORD;
        dwFlags: DWORD) : HResult;
begin
try
//  TLog.Add ('IDPLAY2(3).SetGroupData');
// TLog.Flush;
  asm int 3 end; Result := dp3.SetGroupData(idGroup,lpData, dwDataSize, dwFlags );
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.SetGroupName(idGroup: TDPID; lpGroupName: PDPName;
        dwFlags: DWORD) : HResult;
begin
try
  //TLog.Add ('IDPLAY2(3).SetGroupName');
// TLog.Flush;
  asm int 3 end; Result := dp3.SetGroupName(idGroup,lpGroupName,dwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.SetPlayerData(idPlayer: TDPID; lpData: Pointer; dwDataSize: DWORD;
        dwFlags: DWORD) : HResult;
begin
try
 // TLog.Add ('IDPLAY2(3).SetPlayerData');
// TLog.Flush;
  asm int 3 end; Result := dp3.SetPlayerData(idPlayer,lpData,dwDataSize,dwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.SetPlayerName(idPlayer: TDPID; lpPlayerName: PDPName;
        dwFlags: DWORD) : HResult;
begin
try
// TLog.Add ('IDPLAY2(3).SetPlayerName');
// TLog.Flush;
  asm int 3 end; Result := dp3.SetPlayerName(idPlayer,lpPlayerName,dwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.AddGroupToGroup(idParentGroup: TDPID; idGroup: TDPID) : HResult;
begin
try
 // TLog.Add ('IDPLAY3.AddGroupToGroup');
// TLog.Flush;
  asm int 3 end; Result := dp3.AddGroupToGroup(idParentGroup,idGroup);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.CreateGroupInGroup(idParentGroup: TDPID; var lpidGroup: TDPID;
        lpGroupName: PDPName; lpData: Pointer; dwDataSize: DWORD;
        dwFlags: DWORD) : HResult;
begin
try
//  TLog.Add ('IDPLAY3.CreateGroupInGroup');
// TLog.Flush;
  asm int 3 end; Result := dp3.CreateGroupInGroup(idParentGroup,lpidGroup,lpGroupName,lpData,dwDataSize,dwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.DeleteGroupFromGroup(idParentGroup: TDPID; idGroup: TDPID) :
        HResult;
begin
 // Tlog.Add ('IDPLAY3.DeleteGroupFromGroup');
 // TLog.Flush;
try
  asm int 3 end; Result := dp3.DeleteGroupFromGroup(idParentGroup,idGroup);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.EnumGroupsInGroup(idGroup: TDPID; const lpguidInstance: TGUID;
        lpEnumPlayersCallback2: TDPEnumPlayersCallback2; lpContext: Pointer;
        dwFlags: DWORD) : HResult;
begin
try
 // Tlog.Add ('IDPLAY3.EnumGroupsInGroup');
// TLog.Flush;
  asm int 3 end; Result := dp3.EnumGroupsInGroup(idGroup,lpguidInstance,lpEnumPlayersCallback2,lpContext,dwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.GetGroupConnectionSettings(dwFlags: DWORD; idGroup: TDPID;
        lpData: Pointer; var lpdwDataSize: DWORD) : HResult;
begin
try
//  Tlog.Add ('IDPLAY3.GetGroupConnectionSettings');
// TLog.Flush;
  asm int 3 end; Result := dp3.GetGroupConnectionSettings(dwFlags,idGroup,lpData,lpdwDataSize);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.SecureOpen(const lpsd: TDPSessionDesc2; dwFlags: DWORD;
        const lpSecurity: TDPSecurityDesc; const lpCredentials: TDPCredentials)
        : HResult;
begin
try
 // Tlog.Add ('IDPLAY3.SecureOpen');
// TLog.Flush;
  asm int 3 end; Result := dp3.SecureOpen(lpsd,dwFlags,lpSecurity,lpCredentials);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.SendChatMessage(idFrom: TDPID; idTo: TDPID; dwFlags: DWORD;
        const lpChatMessage: TDPChat) : HResult;
begin
 // Tlog.Add ('IDPLAY3.SendChatMessage');
// TLog.Flush;
try
  asm int 3 end; Result := dp3.SendChatMessage(idFrom,idTo,dwFlags,lpChatMessage);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.SetGroupConnectionSettings(dwFlags: DWORD; idGroup: TDPID;
        const lpConnection: TDPLConnection) : HResult;
begin
try
 // Tlog.Add ('IDPLAY3.SetGroupConnectionSettings');
// TLog.Flush;
  asm int 3 end; Result := dp3.SetGroupConnectionSettings(dwFlags,idGroup, lpConnection);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.StartSession(dwFlags: DWORD; idGroup: TDPID) : HResult;
begin
try
 // Tlog.Add ('IDPLAY3.StartSession');
// TLog.Flush;
  asm int 3 end; Result := dp3.StartSession(dwFlags,idGroup);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.GetGroupFlags(idGroup: TDPID; var lpdwFlags: DWORD) : HResult;
begin
try
  //Tlog.Add ('IDPLAY3.GetGroupFlags');
// TLog.Flush;
  asm int 3 end; Result := dp3.GetGroupFlags(idGroup,lpdwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.GetGroupParent(idGroup: TDPID; var lpidParent: TDPID) : HResult;
begin
try
 // Tlog.Add ('IDPLAY3.GetGroupParent');
// TLog.Flush;
  asm int 3 end; Result := dp3.GetGroupParent(idGroup,lpidParent);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.GetPlayerAccount(idPlayer: TDPID; dwFlags: DWORD; var lpData;
        var lpdwDataSize: DWORD) : HResult;
begin
try
 // Tlog.Add ('IDPLAY3.GetPlayerAccount');
// TLog.Flush;
  asm int 3 end; Result := dp3.GetPlayerAccount(idPlayer,dwFlags,lpData,lpdwDataSize);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.GetPlayerFlags(idPlayer: TDPID; var lpdwFlags: DWORD) : HResult;
begin
try
 // Tlog.Add ('IDPLAY3.GetPlayerFlags');
// TLog.Flush;
  asm int 3 end; Result := dp3.GetPlayerFlags(idPlayer,lpdwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

// -----------------------------------------------------------------------------

function TDPlay.CreatePlayer( var lpidPlayer: TDPID;
                              pPlayerName: PDPName;
                              hEvent: THandle;
                              lpData: Pointer;
                              dwDataSize: DWORD;
                              dwFlags: DWORD) : HResult;
begin
try
  Result := dp3.CreatePlayer (lpidPlayer, pPlayerName, hEvent, lpData, dwDataSize, dwFlags);
  Tlog.Add (5,'!IDPLAY2(3).CreatePlayer');
  Tlog.Add (5,' + lpidPlayer                : ', lpidPlayer);
  Tlog.Add (5,' + pPlayerName.lpszLongName  : ' + pPlayerName^.lpszLongName);
  Tlog.Add (5,' + pPlayerName.lpszShortName : ' + pPlayerName^.lpszShortName);
  Tlog.Add (5,' + hEvent                    : ', hEvent);
//  Tlog.Add (5,' + lpData                    : ', lpData, dwDataSize);
  Tlog.Add (5,' + dwDataSize                : ', dwDataSize);
  Tlog.Add (5,' + dwFlags                   : ', dwFlags);
  TLog.Flush;
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.DestroyPlayer(pidID: TDPID) : HResult;
begin
try
  Result := dp3.DestroyPlayer(pidID);
  Tlog.Add(5,'!IDPLAY(23).DestroyPlayer('+IntToStr(pidID)+' )');
  TLog.Flush;
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;

end;

function TDPlay.EnumPlayers(lpguidInstance: PGUID; lpEnumPlayersCallback2:
        TDPEnumPlayersCallback2; lpContext: Pointer; dwFlags: DWORD) : HResult;
begin
try
  Result := dp3.EnumPlayers (lpguidInstance, lpEnumPlayersCallback2, lpContext, dwFlags);
  Tlog.Add (5,'!IDPLAY2(3).EnumPlayers');
  TLog.Flush;
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
{
  if lpguidInstance <> nil then
    Tlog.Add (' + lpguidInstance^ : ', lpguidInstance^)
  else
    Tlog.Add (' + lpguidInstance : ', lpguidInstance);

  Tlog.Add (' + lpEnumPlayersCallback2 : ', @lpEnumPlayersCallback2);
  Tlog.Add (' + lpContext : ', lpContext);
  Tlog.Add (' + dwFlags : ', dwFlags); }
end;

function TDPlay.EnumSessions(var lpsd: TDPSessionDesc2; dwTimeout: DWORD;
        lpEnumSessionsCallback2: TDPEnumSessionsCallback2; lpContext: Pointer;
        dwFlags: DWORD) : HResult;
begin
try
  Result := dp3.EnumSessions (lpsd, dwTimeout, lpEnumSessionsCallback2, lpContext, dwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
 { Tlog.Add ('!IDPLAY2(3).EnumSessions');
  Tlog.Add (' + lpsd.dwFlags            : ', lpsd.dwFlags);
  Tlog.Add (' + lpsd.guidInstance       : ', lpsd.guidInstance);
  Tlog.Add (' + lpsd.guidApplication    : ', lpsd.guidApplication);
  Tlog.Add (' + lpsd.dwMaxPlayers       : ', lpsd.dwMaxPlayers);
  Tlog.Add (' + lpsd.dwCurrentPlayers   : ', lpsd.dwCurrentPlayers);
  Tlog.Add (' + lpsd.lpszSessionName    : ' + lpsd.lpszSessionName);
  Tlog.Add (' + lpsd.lpszPassword       : ' + lpsd.lpszPassWord);
  Tlog.Add (' + dwTimeout               : ', dwTimeout);
  Tlog.Add (' + lpEnumSessionsCallback2 : ', @lpenumsessionsCallback2);
  Tlog.Add (' + lpContext               : ', lpContext);
  TLog.Add (' + dwFlags                 : ', dwFlags);
  TLog.Flush;  }
end;

function TDPlay.GetPlayerAddress(idPlayer: TDPID; lpAddress: Pointer;
        var lpdwAddressSize: DWORD) : HResult;
begin
try
  Result := dp3.GetPlayerAddress (idPlayer, lpAddress, lpdwAddressSize);
  Tlog.Add (5,'!IDPLAY2(3).GetPlayerAddress');
  TLog.Flush;
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.GetSessionDesc(lpData: Pointer; var lpdwDataSize: DWORD) : HResult;
//var  lpsd :TDPSessionDesc2;
begin
try
  Result := dp3.GetSessionDesc (lpData, lpdwDataSize);
//  Tlog.Add ('!IDPLAY2(3).GetSessionDesc');
// TLog.Flush;
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;

end;

function TDPlay.SetSessionDesc(const lpSessDesc: TDPSessionDesc2; dwFlags: DWORD) :
        HResult;
begin
try
  Result := dp3.SetSessionDesc(lpSessDesc, dwFlags);
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.EnumConnections(const lpguidApplication: TGUID;
        lpEnumCallback: TDPEnumConnectionsCallback; lpContext: Pointer;
        dwFlags: DWORD) : HResult;
begin
  try
    Result := dp3.EnumConnections (lpguidApplication, lpEnumCallback, lpContext, dwFlags);
{  Tlog.Add ('!IDPLAY3.EnumConnections');
  Tlog.Add (' + lpguidApplication : ', lpguidapplication);
  Tlog.Add (' + lpEnumCallback    : ', @lpenumcallback);
  Tlog.Add (' + lpContext         : ', lpcontext);
  TLog.Add (' + dwFlags           : ', dwFlags);
  TLog.Flush;  }
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;

function TDPlay.InitializeConnection(var lpConnection: TDPLConnection; dwFlags: DWORD) :
         HResult;
begin
try
{  Tlog.Add ('!IDPLAY3.InitializeConnection');
  Tlog.Add (' + lpConnection.dwFlags       : ', lpConnection.dwFlags);
  Tlog.Add (' + lpConnection.lpSessionDesc : ', lpConnection.lpSessionDesc);
  Tlog.Add (' + lpConnection.lpPlayerName  : ', lpConnection.lpPlayerName);
  Tlog.Add (' + lpConnection.guidSP        : ', lpConnection.guidSP);
  Tlog.Add (' + lpConnection.lpAddress     : ', lpConnection.lpAddress);
  TLog.Add (' + lpConnection.dwAddressSize : ', lpConnection.dwAddressSize);
 TLog.Flush;
}
    Result := dp3.InitializeConnection (lpConnection, dwFlags);
//    if Result <> DP_OK then
//      raise Exception.Create(ErrorString(Result));
except
  on e : Exception do
     begin
     if OnException(e) then raise else Result := DP_OK;

    end;
end;
end;
