unit StatsLogging;

interface
uses
  PluginEngine, SysUtils, Classes, PacketBufferU;

// -----------------------------------------------------------------------------

const
  State_StatsLogging : boolean = true;

function GetPlugin : TPluginData;

// -----------------------------------------------------------------------------

Procedure OnInstallStatsLogging;
Procedure OnUninstallStatsLogging;

// -----------------------------------------------------------------------------

procedure ReclaimFeatureFinished;
procedure PlayerDied;

type
  TEventType = ( etAddPlayer,
                 etGameStart,
                 etGameFinished,
                 etPlayerDeadOrQuit,
                 etEcoStats,
                 etShare,
                 etReclaim,
                 etNewUnit,
                 etUnitFinish,
                 etUnitKill,
                 etDamage );   

  TStatsLogging = class(TStringList)
  public
    FileName : String;
    constructor Create (const Filename :string);

    procedure Init_StatEvent;
    procedure AddPlayer_StatEvent(PlayerIdx: Byte; Side: Byte; Name: ShortString; StartID: Cardinal);
    procedure NewUnit_StatEvent(player:longword; unitid:word; typeid:word; tid:longword);
    procedure UnitFinished_StatEvent(player:word; unitid:word; tid:longword);
    procedure Damage_StatEvent(receiver:word; sender:word; amount:word; dmgtype:byte; tid:longword);
    procedure FeatureDestroyed_StatEvent(sender:word; weapid:byte; x,z: word; featureptr: pointer; tid:longword);
    procedure Kill_StatEvent(killed:word; killer:word; tid:longword);
    procedure ResourcesShare_StatEvent(fromplayer, toplayer:longword; howmuch:single; restype:byte; tid:longword);
    procedure Stats_StatEvent(player:longword; mstored:single; estored:single; mstorage:single; estorage:single; mincome:single; eincome:single; kills, losses: word; tid:longword);
    procedure GameFinished_StatEvent(tid:longword);
    procedure PlayerDeadOrQuit_StatEevent(playeridx: integer; tid:longword);
  end;

var
  statslog  : TStatsLogging = nil;

implementation

uses
  Windows,
  TA_FunctionsU,
  TADemoConsts,
  TA_MemoryConstants,
  TA_MemoryLocations,
  TA_MemPlayers,
  TA_MemUnits,
  TA_MemoryStructures,
  IniOptions;

Procedure OnInstallStatsLogging;
begin
end;

Procedure OnUninstallStatsLogging;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 and State_StatsLogging then
  begin
    Result := TPluginData.create( false,
                            '',
                            State_StatsLogging,
                            @OnInstallStatsLogging,
                            @OnUnInstallStatsLogging );

    Result.MakeRelativeJmp( State_StatsLogging,
                          'Log if feature/wreck got reclaimed',
                          @ReclaimFeatureFinished,
                          $00423961, 4);

    Result.MakeRelativeJmp( State_StatsLogging,
                          'Log if player died',
                          @PlayerDied,
                          $004504A8, 0);
  end else
    Result := nil;
end;

procedure StatLog_ReclaimFeatureLocal(PlayerPtr: PPlayerStruct; FeaturePtr: PFeatureDefStruct); stdcall;
begin
  if IniSettings.CreateStatsFile and not IniSettings.WeaponIdPatch then
    statslog.FeatureDestroyed_StatEvent(TAPlayer.GetPlayerByDPID(PlayerPtr.lDirectPlayID),$FF,0,0,
                                        FeaturePtr,
                                        TAData.GameTime);
end;

procedure ReclaimFeatureFinished;
label
  SkirmishGame;
asm
  push    ecx
  push    1
  push    ebp
  push    edi
  call    FEATURES_Destroy_3D
  mov     eax, [TADynMemStructPtr]
  mov     ecx, [eax+TTAdynmemStruct.p_MapOTAFile]
  call    GetGameingType
  cmp     eax, 3
  jnz     SkirmishGame
  pop     ecx
  mov     edx, [esi+TUnitStruct.p_Owner]
  push    ecx
  push    edx
  call    StatLog_ReclaimFeatureLocal
  push $0042397F;
  call PatchNJump;
SkirmishGame :
  pop     ecx
  push $004239A9;
  call PatchNJump;
end;

procedure StatLog_PlayerDied(PlayerIdx: Byte); stdcall;
begin
  if IniSettings.CreateStatsFile then
    if statslog <> nil then
      statslog.PlayerDeadOrQuit_StatEevent(PlayerIdx, TAData.GameTime);
end;

procedure PlayerDied;
asm
  pushAD
  mov     dl, [esi+TPlayerStruct.cPlayerIndex]
  push    edx
  call StatLog_PlayerDied
  popAD
  push    eax
  lea     eax, [esi+TPlayerStruct.szName]
  push    eax
  push $004504AD;
  call PatchNJump;
end;

constructor TStatsLogging.Create(const Filename :string);
begin
  Self.FileName := FileName;
end;

procedure TStatsLogging.Init_StatEvent;
begin
end;

procedure TStatsLogging.AddPlayer_StatEvent(PlayerIdx: Byte; Side: Byte; Name: ShortString; StartID: Cardinal);
begin
  Add(IntToStr(Ord(etAddPlayer)) + ';' + IntToStr(PlayerIdx) + ';' +
      IntToStr(Side) + ';' + IntToStr(StartID) + ';' + Name);
end;

procedure TStatsLogging.NewUnit_StatEvent(player:longword; unitid:word; typeid:word; tid:longword);
begin
  Add(IntToStr(Ord(etNewUnit)) + ';' + IntToStr(tid) + ';' + IntToStr(player) + ';' + IntToStr(unitid) + ';' +
      PUnitInfo(TAMem.UnitInfoId2Ptr(typeid)).szName);
end;

var
  lastunit : Cardinal;
procedure TStatsLogging.UnitFinished_StatEvent(player:word; unitid:word; tid:longword);
begin
  if lastunit <> TAUnit.Id2LongId(unitid) then
  begin
    if (TAUnit.Id2Ptr(unitid).p_UNITINFO.szUnitName <> 'None') and
       (((TAUnit.Id2Ptr(unitid).p_UNITINFO.UnitTypeMask2 shl 18) and 1) <> 1) then
      Add(IntToStr(Ord(etUnitFinish)) + ';' + IntToStr(tid) + ';' +
          IntToStr(player) + ';' + IntToStr(unitid) + ';' +
          TAUnit.Id2Ptr(unitid).p_UNITINFO.szName);
    lastunit := TAUnit.Id2LongId(unitid);
  end;
end;

procedure TStatsLogging.Damage_StatEvent(receiver:word; sender:word; amount:word; dmgtype:byte; tid:longword);
var
  attplyr, defplyr : integer;
begin
  attplyr := TAUnit.GetOwnerIndex(TAUnit.Id2Ptr(sender));
  defplyr := TAUnit.GetOwnerIndex(TAUnit.Id2Ptr(receiver));
  if (attplyr <> -1) and
     (defplyr <> -1) then
    Add(IntToStr(Ord(etDamage)) + ';' + IntToStr(tid) + ';' +
        IntToStr(sender) + ';' + IntToStr(receiver) + ';' +
        IntToStr(attplyr) + ';' + IntToStr(defplyr) + ';' +
        IntToStr(dmgtype) + ';' + IntToStr(amount));
end;

procedure TStatsLogging.FeatureDestroyed_StatEvent(sender:word; weapid:byte; x,z: word; featureptr: pointer; tid:longword);
var
  Position : TPosition;
  FeatureTypeID : SmallInt;
  FeatureDefPtr : PFeatureDefStruct;
  Ukn : Cardinal;
begin
  FeatureDefPtr := featureptr;
  if FeatureDefPtr = nil then
  begin
    Position.X := x * 16;
    Position.Z := z * 16;
    FeatureTypeID := GetFeatureTypeOfPos(@Position, nil, @Ukn);
    if FeatureTypeID <> -1 then
      FeatureDefPtr := TAMem.FeatureDefId2Ptr(FeatureTypeID);
    if FeatureDefPtr = nil then
      Exit;
  end;
  // 253 exploded
  // 254 start burn
  // 255 reclaimed
  case weapid of
    $FF : begin
            Add(IntToStr(Ord(etReclaim)) + ';' + IntToStr(tid) + ';' + IntToStr(sender) + ';' +
                FeatureDefPtr.Name + ';' +
                FloatToStrF(FeatureDefPtr.metal, ffGeneral, 12, 4) + ';' +
                FloatToStrF(FeatureDefPtr.energy, ffGeneral, 12, 4));
          end;
  end;
end;

procedure TStatsLogging.Kill_StatEvent(killed:word; killer:word; tid:longword);
var
  KilledUnit : PUnitStruct;
begin
  if TAData.UnitsArray_p <> nil then
  begin
    KilledUnit := TAUnit.Id2Ptr(killed);
    if KilledUnit.p_UNITINFO <> nil then
      Add(IntToStr(Ord(etUnitKill)) + ';' + IntToStr(tid) + ';' +
          IntToStr(killer) + ';' + IntToStr(killed) + ';' +
          PUnitInfo(KilledUnit.p_UNITINFO).szName)
    else
      Add(IntToStr(Ord(etUnitKill)) + ';' + IntToStr(tid) + ';' +
          IntToStr(killer) + ';' + IntToStr(killed));
  end;
end;

procedure TStatsLogging.ResourcesShare_StatEvent(fromplayer, toplayer:longword; howmuch:single; restype:byte; tid:longword);
//var
//  Player : PPlayerStruct;
begin
  //Player := TAPlayer.GetPlayerByIndex(fromplayer);
  //if Player.PlayerInfo.Raceside <> 2 then
    Add(IntToStr(Ord(etShare)) + ';' + IntToStr(tid) + ';' +
        IntToStr(fromplayer) + ';' + IntToStr(toplayer) + ';' +
        IntToStr(restype) + ';' + FloatToStrF(howmuch, ffGeneral, 12, 4));
end;

procedure TStatsLogging.Stats_StatEvent(player:longword; mstored:single; estored:single; mstorage:single; estorage:single; mincome:single; eincome:single; kills, losses: word; tid:longword);
begin
  Add(IntToStr(Ord(etEcoStats)) + ';' + IntToStr(tid) + ';' + IntToStr(player) + ';' +
      FloatToStrF(mstored, ffGeneral, 12, 4) + ';' + FloatToStrF(estored, ffGeneral, 12, 4) + ';' +
      FloatToStr(mstorage) + ';' + FloatToStr(estorage) + ';' +
      FloatToStrF(mincome, ffGeneral, 12, 4) + ';' + FloatToStrF(eincome, ffGeneral, 12, 4));
end;

procedure TStatsLogging.GameFinished_StatEvent(tid:longword);
begin
  Add(IntToStr(Ord(etGameFinished)) + ';' + IntToStr(tid));
  // save to file
  Self.SaveToFile(Self.FileName);
end;

procedure TStatsLogging.PlayerDeadOrQuit_StatEevent(playeridx: integer; tid:longword);
begin
  Add(IntToStr(Ord(etPlayerDeadOrQuit)) + ';' + IntToStr(tid) + ';' + IntToStr(playeridx));
end;

end.