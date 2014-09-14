unit TA_MemoryStructures;

interface
uses Classes;

type

  TGameingType = ( gtMenu, gtCampaign, gtSkirmish, gtMultiplayer );

  TAIDifficulty = ( adEasy, adMedium, adHard );

  PMoveInfoClassStruct = ^TMoveInfoClassStruct;
  TMoveInfoClassStruct = packed record
    pName          : Pointer;
    nFootPrintX    : Word; // default: 0
    nFootPrintZ    : Word; // default: 0
    nMaxWaterDepth : SmallInt; // default: 1027 = 10000
    nMinWaterDepth : SmallInt; // default: F0D8 = -10000
    cMaxSlope      : ShortInt; // default: FF = -1
    cBadSlope      : ShortInt; // default: 7F (FF shr 1)
    cMaxWaterSlope : ShortInt; // default: FF = -1
    cBadWaterSlope : ShortInt; // default: 7F (FF shr 1)
    lUnknown1      : Cardinal; // always C0 00 00 00
    lUnknown2      : Cardinal; // always C4 00 00 00
    lUnknown3      : Cardinal; // pointer ?
    lUnknown4      : Cardinal; // always 00 00 00 00
  end;

  PMoveClass = ^TMoveClass;
  TMoveClass = packed record
    Move_CallBack   : Pointer;
    movementclass   : Pointer;
    field_8         : Cardinal;
    field_C         : Cardinal;
    field_10        : Cardinal;
    field_14        : Cardinal;
    field_18        : Cardinal;
    field_1C        : Cardinal;
    lCurrentSpeed   : Cardinal;
    field_24        : Word;               
    field_26        : Cardinal;
    field_2A        : Cardinal;
    Mask            : Byte;
  end;

  PTNTHeaderStruct = ^TTNTHeaderStruct;
  TTNTHeaderStruct = packed record
    IDversion   : Cardinal;
    Width       : Cardinal;
    Height      : Cardinal;
    p_TileMap   : Cardinal;
    p_HeightMap : Cardinal;
    p_TileSet   : Cardinal;
    lTilesCount : Cardinal;
    lAnimTiles  : Cardinal;
    PTRtileanim : Cardinal;
    sealevel    : Cardinal;
    p_Minimap   : Cardinal;
    unknown1    : Cardinal;
    pad         : array[0..3] of Cardinal;
  end;

  PPlotGrid = ^TPlotGrid;
  TPlotGrid = packed record
    nYard_color        : Word;
    field_3            : Word;
    bHeight            : Byte;
    bHeightAvg1        : Byte;
    bHeightAvg2        : Byte;
    bMetalExtract      : Byte;
    nFeatureDefIndex   : Word;
    nWreckageInfoIndex : Word;
    bYard_type         : Byte;
  end;

  TGAFFrame = packed record
    Width : Word;
    Height : Word;
    Left : Word;
    Top : Word;
    Background : Byte;
    Compressed : Byte;
    SubFrames : Byte;
    IsCompressed : Byte;
    PtrFrameData : Pointer;
    PtrFrameBits : Pointer;
    Bits2_Ptr : Pointer;
  end;

  PShortPosition = ^TShortPosition;
  TShortPosition = packed record
    X : Word;
    Z : Word;
  end;

  PPosition = ^TPosition;
  TPosition = packed record
    x_ : Word;
    X  : Word;
    y_ : Word;
    Y  : Word;
    z_ : Word;
    Z  : Word;
  end;

  PTurn = ^TTurn;
  TTurn = packed record
    X  : Word;
    Z  : Word;
    Y  : Word;
  end;

  PBaseObject = ^TBaseObject;
  TBaseObject = packed record
    BaseObjectInfo_p : Pointer;
    Position         : TPosition;
    Turn             : TTurn;
    data3            : array[0..17] of Byte;
    Visible          : Word;
    data2            : Byte;
    data4            : Byte;
    SiblingObject    : Pointer;
    ChildObject      : Pointer;
    ParrentObject    : Pointer;
  end;

  PObject3do = ^TObject3do;
  TObject3do = packed record
    nNumParts        : Word;                   //ByteAry, the Object3do's data save after struct
    ndata1           : Word;
    lTimeVisible     : Cardinal;
    lBackground      : Cardinal;
    pThisUnit        : Pointer;
    pCompositeBuffer : Pointer;
    nObject_States   : Word;
    nXWidth          : Word;
    Turn             : TTurn;
    pBaseObject      : PBaseObject;
  end;

  PUnitWeapon = ^TUnitWeapon;
  TUnitWeapon = packed record
    nTargetID         : Word;
    nUsedSpot         : Word;
    p_AutoAimCallback : Pointer;   //$4FD6F0
    lState            : Cardinal;  //1 if can shoot
    p_Weapon          : Pointer;
    unknow            : Cardinal;
    nReloadTime       : Word;
    nAngle            : Word;
    nTrajectoryResult : Word;
    cStock            : Byte;
    cStateMask        : Byte;      // and 1 = aiming, nand 10 = target locked, nand 1 and nand 10 = weapon reloaded, firing again
  end;

  // 0x118
  PUnitStruct = ^TUnitStruct;
  TUnitStruct = packed record
    p_MovementClass   : Pointer;
    UnitWeapons       : array[0..2] of TUnitWeapon;
    fMetalExtrRatio   : Single; //UnitInfo.extractsmetal * ExtractRatio * 0.0000152587890625;
    p_UnitOrders      : Pointer;
    p_FutureOrder     : Pointer;
    Turn              : TTurn;
    Position          : TPosition;
    nGridPosX         : Word;
    nGridPosZ         : Word;
    nLargeGridPosX    : Word;
    nLargeGridPosZ    : Word;
    nFootPrintX       : Word;
    nFootPrintZ       : Word;
    View_dw0          : Cardinal;
    p_TransporterUnit : Pointer;
    p_TransportedUnit : Pointer;
    p_PriorUnit       : Pointer;
    p_UnitDef         : Pointer;
    p_Owner           : Pointer;
    p_UnitScriptsData : Pointer;
    p_Object3DO       : PObject3do;
    Order_Unknow      : Cardinal;
    nUnitInfoID       : Word;
    lUnitInGameIndex  : Cardinal;
    HotKeyGroup       : Cardinal;
    lFireTimePlus600  : Cardinal;
    field_B4          : Cardinal;
    nKills            : Word;
    nProjectileState  : Word;      // Unknown11 and $4; when calling setter, also projectile state
    lResPercentEnergy : Cardinal;
    fWeapResConsume   : Single;
    fWeapResConsume2  : Single;
    field_BC          : array [0..7] of byte;
    field_D0          : Cardinal;
    lResPercentMetal  : Cardinal;
    lBuildWeapUnk     : Single;
    lBuildWeapUnk2    : Single;
    field_E0          : array [0..7] of byte;
    field_E8          : Cardinal;
    p_Owner2          : Pointer;
    p_Attacker        : Pointer;
    cOwningPlayerID   : Byte;
    cLastDamageType   : Byte;
    cHealthPercentA   : Byte;
    cHealthPercentB   : Byte;
    unKnow_13         : Byte;
    ucAttachedToPiece : Byte;      // index number of piece unit is attached to when its transported
    ucRecentDamage    : Byte;
    ucHeight          : Byte;
    nOwnerIndex       : Word;
    field_FE          : Byte;
    cMyLOSPlayerID    : Byte;
    unknow_14         : Cardinal;
    lBuildTimeLeft    : Single;
    nHealth           : Word;
    lSfxOccupy        : Cardinal;
    nUnitStateMaskBas : Word;      // cloak, activate, armored state
    lUnitStateMask    : Cardinal;
    Unknown_16        : Cardinal; //((TemplatePtr.UnitTypeMask_0 shr 7) and 1) or  $FE;
  end;

  // 0x249
  PUnitInfo = ^TUnitInfo;
  TUnitInfo = packed record
    szName             : array [0..31] of AnsiChar;
    szUnitName         : array [0..31] of AnsiChar;
    szUnitDescription  : array [0..63] of AnsiChar;
    szObjectName       : array [0..31] of AnsiChar;
    szSide             : array [0..7] of AnsiChar;
    nUID               : Word;
    Unknown1           : array [0..19] of Byte;
    AIWeight           : array [0..63] of Byte;
    AILimit            : array [0..63] of Byte;
    CRC_FBI            : Cardinal;
    CRC_allfiles       : Cardinal;
    p_field_146        : Pointer;
    nFootPrintX        : Word;
    nFootPrintZ        : Word;
    pYardMap           : Pointer;
    lAICanBuildCount   : Cardinal;
    pAICanBuildList    : Pointer;
    field_15A          : Cardinal;
    lWidthX            : Cardinal;
    lUnknown4          : Cardinal;
    lWidthZ            : Cardinal;
    lFootPrintX        : Cardinal;
    lWidthY            : Cardinal;
    lFootPrintZ        : Cardinal;
    lRelatedUnitXWidth : Cardinal;
    lRelatedUnitYWidth : Cardinal;
    lRelatedUnitZWidth : Cardinal;
    lWidthHypot        : Cardinal;
    lBuildCostEnergy   : Single;
    lBuildCostMetal    : Single;
    pCOBScript         : Pointer;
    lMaxSpeedRaw       : Cardinal;
    lMaxSpeedSlope     : Cardinal;    // (lMaxSpeedRaw shl 16) / ((cMaxSlope + 1) shl 16)
    lBrakeRate         : Single;
    lAcceleration      : Single;
    lBankScale         : Cardinal;
    lPitchScale        : Cardinal;
    lDamageModifier    : Cardinal;
    lMoveRate1         : Cardinal;
    lMoveRate2         : Cardinal;
    p_MovementClass    : Pointer;
    nTurnRate          : Word;
    nCorpseIndex       : Word;
    nMaxWaterDepth     : SmallInt;
    nMinWaterDepth     : SmallInt;
    fEnergyMake        : Single;
    fEnergyUse         : Single;
    fMetalMake         : Single;
    fMetalUse          : Single;
    fWindGenerator     : Single;
    fTidalGenerator    : Single;
    fCloakCost         : Single;
    fCloakCostMoving   : Single;
    lEnergyStorage     : Cardinal;
    lMetalStorage      : Cardinal;
    lBuildTime         : Cardinal;
    p_WeaponPrimary    : Pointer;
    p_WeaponSecondary  : Pointer;
    p_WeaponTertiary   : Pointer;
    lMaxHP             : Cardinal;
    nWorkerTime        : Word;
    nHealTime          : Word;
    nSightDistance     : Word;
    nRadarDistance     : Word;
    nSonarDistance     : Word;
    nMinCloakDistance  : Word;
    nRadarDistanceJam  : Word;
    nSonarDistanceJam  : Word;
    nSoundCategory     : Word;
    nBuildAngle        : Word;
    nBuildDistance     : Word;
    nManeuverLeashLen  : Word;
    nAttackRunLength   : Word;
    nKamikazeDistance  : Word;
    nSortBias          : Word;
    nCruiseAlt         : SmallInt;
    nCategory          : Word;
    p_ExplodeAs        : Pointer;
    p_SelfDestructAsAs : Pointer;
    cMaxSlope          : ShortInt;
    cMaxWaterSlope     : ShortInt;
    cTransportSize     : Byte;
    cTransportCap      : Byte;
    cWaterLine         : Byte;
    cMakesMetal        : Byte;
    cGUINum            : Byte;
    cBMCode            : Byte;
    cDefMissionType    : Byte;
    p_WeaponPrimaryBadTargetCategoryArray: Pointer;
    p_WeaponSecondaryBadTargetCategoryArray: Pointer;
    p_WeaponSpecialBadTargetCategoryArray: Pointer;
    p_NoChaseCategoryMaskArray: Pointer;
    UnitTypeMask       : Cardinal;
    UnitTypeMask2      : Cardinal;
  end;

  TTAPlayerType = (Player_LocalHuman = 1, Player_LocalAI = 2, Player_RemotePlayer = 3);
  TTAPlayerSide = (psUnknown, psArm, psCore, psWatch);

  PAlliedState = ^TAlliedState;
  TAlliedState = array [ 0..9 ] of Byte;

  PPlayerResourcesStruct = ^TPlayerResourcesStruct;
  TPlayerResourcesStruct = packed record
    fCurrentEnergy       : single;
    fEnergyProduction    : single;
    fEnergyExpense       : single;
    fCurrentMetal        : single;
    fMetalProduction     : single;
    fMetalExpense        : single;
    fEnergyStorageMax    : single;
    fMetalStorageMax     : single;
    dEnergyProducedTotal : double;
    dMetalProducedTotal  : double;
    dEnergyConsumedTotal : double;
    dMetalConsumedTotal  : double;
    dEnergyWastedTotal   : double;
    dMetalWastedTotal    : double;
    fEnergyStorage       : single;
    fMetalStoragePlayer  : single;
  end;

  TPlayerSharedStates = ( SharedState_SharedMetal = 2,
                          SharedState_SharedEnergy = 4,
                          SharedState_SharedLOS = 8,
                          SharedState_SharedMappings = $20,
                          SharedState_SharedRadar = $40 );
  TPlayerSharedStatesSet = set of TPlayerSharedStates;

  TPlayerPropertyMask = ( PropertyMask_Watcher = $40,
                          PropertyMask_Human = $80 );
  TPlayerPropertyMaskSet = set of TPlayerPropertyMask;

  TSwitchesMask = ( SwitchesMask_Drop,
                    SwitchesMask_DevMode,
                    SwitchesMask_SelBoxes,
                    SwitchesMask_TreeDeath,
                    SwitchesMask_NoShake,
                    SwitchesMask_Clock,
                    SwitchesMask_DoubleShot,
                    SwitchesMask_HalfShot,
                    SwitchesMask_Radar,
                    SwitchesMask_ShootAll );
  TSwitchesMaskSet = set of TSwitchesMask;

const
  SwitchesMasks : array [TSwitchesMask] of Word = ($1, $2, $4, $8, $10, $40, $80, $100, $200, $400);

type
  //0xB9
  PPlayerInfoStruct = ^TPlayerInfoStruct;
  TPlayerInfoStruct = packed record
    MapName         : array [0..31] of AnsiChar;
    field_20        : array [0..116] of Byte;
    Raceside        : Byte;
    PlayerLogoColor : Byte;
    SharedBits      : TPlayerSharedStatesSet;
    field_98        : array [0..2] of Byte;
    PropertyMask    : TPlayerPropertyMaskSet;
    field_9C        : array [0..28] of Byte;
  end;

  //0x14B
  PPlayerStruct = ^TPlayerStruct;
  TPlayerStruct = packed record
    lPlayerActive        : Cardinal;			// 0x00 - is this a char?
    lDirectPlayID        : Cardinal;			// 0x04 - player localness? I think this is the 0 based player index...
    Unknown1             : Cardinal;
    cPlayerOwnerIndexOne : Byte;	// 0x0C - The 1 based player index for who owns the player. 0 means it is an unused slot... is this accurate? looks like always zero?
    Unknown2             : array [0..25] of Byte;		// 0x0D
    PlayerInfo           : PPlayerInfoStruct;			// 0x27
    szName               : array [0..29] of AnsiChar;				// 0x2B
    szSecondName         : array [0..29] of AnsiChar;		// 0x49
    p_UnitsArray         : Pointer;			// 0x67
    p_LastUnit           : Pointer;			// 0x6B the last unit in the array
    nUnitsIndexBegin     : SmallInt;
    nUnitsIndexEnd       : SmallInt;
    en_cPlayerType       : TTAPlayerType;			// 0x73
    p_AIConfig           : Cardinal;
    p_Unknown3           : Cardinal;			// 0x78
    p_LOSStruturePtr     : Pointer;		// 0x7C - LOS related
    lLOSOffsetX          : Cardinal;			// 0x80 - LOS related
    lLOSOffsetY          : Cardinal;			// 0x84 - LOS related
    lLOSStructLength     : Cardinal;		// 0x88 - LOS related
    PlayerResources      : TPlayerResourcesStruct;		// 0x8C
    fShareLimitMetal     : single;		// 0xE4 //are these in the right order (metal/energy)?
    fShareLimitEnergy    : single;		// 0xE8
    p_Unknown4           : Cardinal;			// 0xEC
    lUpdateTime          : Cardinal;
    lWinLoseTime         : Cardinal;
    lDisplayTimer        : Cardinal;
    nKills               : Word;
    nLosses              : Word;
    lUnknown4            : Cardinal;
    nKills2              : SmallInt;
    nLosses2             : SmallInt;
    cAllyFlagArray       : TAlliedState;
    Unknown5             : array [0..44] of Byte;
    cAllyTeam            : Byte;	 //0x13F
    lUnitsCounters       : Cardinal;
    nNumUnits            : SmallInt;				// 0x144
    cPlayerIndexZero     : Byte;		// 0x146 - zero based index of the player. AI 1, 2, 3 etc.
    Unknown6             : Cardinal;			// 0x147
  end;

  TFoundUnits = array of Cardinal;

  // 0x115
  PWeaponDef = ^TWeaponDef;
  TWeaponDef = packed record
	  szWeaponName         : array [0..31] of AnsiChar;
	  szWeaponDescription  : array [0..63] of AnsiChar;
	  p_FireCallback       : Pointer;
	  p_WeaponDefUnkStruct : Pointer;
	  lWeaponVelocity      : Cardinal;
	  lStartVelocity       : Cardinal;
	  lWeaponAcceleration  : Cardinal;
	  lObjects3DFile       : Cardinal;
	  p_LandExplodeAsGFX   : Pointer;
	  p_WaterExplodeAsGFX  : Pointer;
	  ModelName            : array [0..47] of AnsiChar;
    field_B0             : Cardinal;
    field_B4             : Cardinal;
    field_B8             : Cardinal;
    lWeaponIDCrack       : Cardinal;
	  lEnergyPerShot       : Cardinal;
	  lMetalPerShot        : Cardinal;
	  lMinBarrelAngle      : Cardinal;
	  fShakeMagnitude      : Single;
	  lShakeDuration       : Cardinal;
	  nDefaultDamage       : Word;
	  nAreaOfEffect        : Word;
	  fEdgeEffectivnes     : Single;
	  lRange               : Cardinal;
	  lCoverage            : Cardinal;
	  nReloadTime          : Word;
	  nWeaponTimer         : Word;
	  nTurnRate            : Word;
	  nBurst               : Word;
	  nBurstRate           : Word;
	  nSprayAngle          : Word;
	  nDuration            : Word;
	  nRandomDecay         : Word;
	  nSoundStartEffectID  : SmallInt;
	  nSoundHitEffectID    : SmallInt;
	  nSoundWaterEffectID  : SmallInt;
	  nSmokeDelay          : Word;
	  nFlightTime          : Word;
	  nHoldTime            : Word;
	  nUnknown1            : Word;
	  nUnknown2            : Word;
	  nAccuracy            : Word;
	  nTolerance           : Word;
	  nPitchTolerance      : Word;
	  ucID                 : Byte;
	  cFireStarter         : Byte;
	  cRenderType          : Byte;
	  cColor               : Byte;
	  cColor2              : Byte;
	  Unknown3             : array [0..1] of Byte;
	  lWeaponTypeMask      : Cardinal;
  end;

  // 0x100
  PFeatureDefStruct = ^TFeatureDefStruct;
  TFeatureDefStruct = packed record
    Name                   : Array[0..31] of AnsiChar;
    data1                  : Array[0..95] of AnsiChar;
    Description            : Array[0..19] of AnsiChar;
    footprintx             : Word;
    footprintz             : Word;
    objects3d              : Pointer;
    field_9C               : Word;
    field_9E               : Cardinal;
    field_A2               : Array[0..5] of Byte;
    p_anims                : Pointer;
    p_seqname              : Pointer;
    p_seqnameshad          : Pointer;
    p_burnName2Sequence    : Pointer;
    p_seqnameburnshad      : Pointer;
    p_seqnamedie           : Pointer;
    p_seqnamedieshad       : Pointer;
    p_seqnamereclamate     : Pointer;
    p_seqnamereclamateshad : Pointer;
    field_CC               : Word;
    field_CE               : Word;
    equals0                : Cardinal;
    field_D4               : Cardinal;
    field_D8               : Word;
    field_DA               : Word;
    field_DC               : Cardinal;
    field_E0               : Cardinal;
    field_E4               : Cardinal;
    sparktime              : Word;
    damage                 : Word;
    energy                 : Single;
    metal                  : Single;
    field_F4               : Array[0..5] of Byte;
    height                 : Byte;
    spreadchance           : Byte;
    reproduce              : Byte;
    reproducearea          : Byte;
    FeatureMask            : Word;
  end; {FeatureDefStruct}

  //0x54
  TExplosion = packed record
	  p_Debris: Pointer;
	  nFrame: Word;
	  Unknown1: array [0..5] of Byte;
	  p_FXGAF: Pointer;
	  Unknown2: array [0..11] of Byte;
	  lXPos: Integer;
	  lZPos: Integer;
	  lYPos: Integer;
	  Unknown3: array [0..35] of Byte;
	  nXTurn: SmallInt; //0x4C
	  nZTurn: SmallInt;
	  nYTurn: SmallInt;
	  Unknown4: array [0..1] of Byte;
	end;

  PtagRECT = ^tagRECT;
  tagRECT = packed record
            Left : Longint;
            Top : Longint;
            Right : Longint;
            Bottom : Longint;
            end;

  PViewResBar = ^TViewResBar;
  TViewResBar = packed record
    PlayerAryIndex : Byte;
    fEnergyStorage : Single;
    fEnergyProduction : Single;
    fEnergyExpense : Single;
    fMetalStorage : Single;
    fMetalProduction : Single;
    fMetalExpense : Single;
    fMaxEnergyStorage : Single;
    fMaxMetalStorage : Single;
  end;

  PRaceSideDataStruct = ^TRaceSideDataStruct;
  TRaceSideDataStruct = packed record
	  Name               : array [0..29] of AnsiChar;
	  NamePrefix         : array [0..3] of AnsiChar;
	  CommanderUnitName  : array [0..31] of AnsiChar;
	  rectLogo           : tagRECT;
	  rectEnergyBar      : tagRECT;
	  rectEnergyNum      : tagRECT;
	  rectMetalBar       : tagRECT;
	  rectMetalNum       : tagRECT;
	  rectTotalUnits     : tagRECT;
	  rectTotalTime      : tagRECT;
	  rectEnergyMax      : tagRECT;
	  rectMetalMax       : tagRECT;
	  rectEnergy0        : tagRECT;
	  rectMetal0         : tagRECT;
	  rectEnergyProduced : tagRECT;
	  rectEnergyConsumed : tagRECT;
	  rectMetalProduced  : tagRECT;
	  rectMetalConsumed  : tagRECT;
	  rectLogo2          : tagRECT;
	  rectUnitName       : tagRECT;
	  rectDamageBar      : tagRECT;
	  rectUnitEnergyMake : tagRECT;
	  rectUnitEnergyUse  : tagRECT;
	  rectUnitMetalMake  : tagRECT;
	  rectUnitMetalUse   : tagRECT;
	  rectMissionText    : tagRECT;
	  rectUnitName2      : tagRECT;
	  rectField1C2       : tagRECT;
	  rectName           : tagRECT;
	  rectDescription    : tagRECT;
	  rectReload_RaceID  : tagRECT;
	  field_202          : array [0..31] of byte;
	  lEnergyColor       : Cardinal;
	  lMetalColor        : Cardinal;
	  field_22A          : Cardinal;
	  lFont_File         : Cardinal;
	end;

  PMapOTAFile = ^TMapOTAFile;
  TMapOTAFile = packed record
    MissionType        : Cardinal;          // 1 campaign
    sMissionName       : array [0..255] of AnsiChar;
    sMissionTDF        : array [0..255] of AnsiChar;
    sTNTFile           : array [0..255] of AnsiChar;
    sMissionBriefTXT   : array [0..255] of AnsiChar;
    sMissionBriefWav   : array [0..255] of AnsiChar;
    sMissionHintsTXT   : array [0..255] of AnsiChar;
    sMissionPCX        : array [0..255] of AnsiChar;
    sMissionUseOnlyTDF : array [0..255] of AnsiChar;
    sAIProfile         : array [0..255] of AnsiChar;
    F9                 : array [0..255] of AnsiChar;
    field_A04          : Pointer;
    FileHandle         : Pointer;
    field_A0C          : Pointer;
    field_A10          : Pointer;
    sOTAFile           : Pointer;
    field_A18          : Pointer;
    field_A1C          : array [0..247] of Byte;
    field_B14          : Pointer;
    field_B18          : array [0..251] of Byte;
    p_Briefing         : Pointer;
    field_C18          : Pointer;
    field_C1C          : array [0..263] of Byte;
    field_D24          : Pointer;
    field_D28          : Pointer;
    field_D2C          : Pointer;
    lSurfaceMetal      : Integer;
    lMinWindSpeed      : Single;
    lMaxWindSpeed      : Single;
    lGravity           : Single;
    lTidalStrength     : Single;
    lIsLavaMap         : Cardinal;
    lNoSeaLevelTrigger : Integer;
    lWaterDoesDamage   : Integer;
    lWaterDamage       : Integer;
    fKillMul           : Single;
    fTimeMul           : Single;
    lHumanMetal        : Integer;
    lComputerMetal     : Integer;
    SchemaInfo         : array [0..31] of Byte;
    lHumanEnergy       : Integer;
    lComputerEnergy    : Integer;
    SchemaInfo2        : array [0..31] of Byte;
    field_DAC          : Cardinal;
    field_DB0          : Cardinal;
    p_PlayersStartPos  : Pointer;
    PlayersStartPosNr  : Cardinal;
    field_DBC          : Cardinal;
    field_DC0          : Cardinal;
    sMemory            : array [0..255] of AnsiChar;  //0-127 memory, 128-255 - numplayers
  end;

  PTNTMemStruct = ^TTNTMemStruct;
  TTNTMemStruct = packed record
    p_SortUnitList         : Pointer;
    p_SortIndices          : Pointer;
    p_SortLineCount        : Pointer;
    p_PathFindStruct       : Pointer;
	  p_FeatureAnimData      : Pointer; //0x1420B all currently animated features on map, including wreckages
    Feature_Unit           : Pointer;
    field_18               : Cardinal;
    field_1C               : Cardinal;
    field_20               : Cardinal;
    field_24               : Cardinal;
    lMapWidth              : Cardinal;
    lMapHeight             : Cardinal;
	  lRadarPictureWidth     : Cardinal;
	  lRadarPictureHeight    : Cardinal;
	  lTilesetMapSizeX       : Cardinal; //0x14233 - this is the map width in units of 16 (multiply by 16 to get pixels)
	  lTilesetMapSizeY       : Cardinal; //0x14237 - this is the map height in units of 16 (multiply by 16 to get pixels)
	  lEyeballWidth          : Cardinal;
	  lEyeballHeight         : Cardinal;
    lEyeballBoxWidth       : Cardinal;
    lEyeballBoxHeight      : Cardinal;
    field_50               : Cardinal;
    field_54               : Cardinal;
	  lNumFeatureDefs        : Cardinal;
	  field_5C               : Cardinal;
    lMinWindSpeed          : Single;
    lMaxWindSpeed          : Single;
    lGravity               : Single;
    lTidalStrength         : Single;
	  p_TedGeneratedPic      : Pointer;
	  p_FeatureDefs          : Pointer; //0x1426F
	  p_MappedMemory         : Pointer; // Circular LOS table
	  lLastZPos              : Cardinal;
	  p_EyeBallMemory        : Pointer; //0x1427B
	  SeaLevel               : Byte;
    MapDebugMode           : Byte;
  	nLOS_Type              : Word; //xpoy's IDA db gives as "EyeBallState" with 0x0FFF7 == moving
	  p_TileSet              : Pointer;
	  p_PlotMemory           : Pointer; // features
    p_TileMap              : Pointer;
  end;

  PTAdynmemStruct = ^TTAdynmemStruct;
  TTAdynmemStruct = packed record
	  sTAVersionStr          : array [0..11] of AnsiChar;
	  p_TAProgram            : Pointer;
	  p_DSound               : Pointer;
	  p_HAPINETObject        : Pointer;
	  Unknown1               : array [0..1200] of Byte;
	  lLocalDirectPlayID     : Cardinal;
	  lUnknownPlayerID       : Cardinal;
	  Unknown2               : array [0..71] of Byte;
	  p_TAGUIObject          : Pointer;
	  Unknown3               : array [0..107] of Byte;
	  cAlteredUnitLimit      : Byte;              
	  Unkonwn4               : array [0..2126] of Byte;
	  cPlayerCameraRectColor : Byte;
	  Unknown5               : array [0..1300] of Byte;
	  p_ChatTextBegin        : Pointer;
	  Unknown6               : array [0..58] of Byte;
	  lUnknown7              : Cardinal;
	  cUnknown8              : array [0..1781] of Byte;
	  lUnknown9              : Cardinal;
	  Unknown10              : array [0..310] of Byte;
	  Players                : array [0..9] of TPlayerStruct; //starts at 0x1B63 and each player is 0x14B (331) bytes long
	  ExtraPlayer            : TPlayerStruct;   //extra player
	  lUnknown12             : Cardinal;
	  p_AllyData             : Pointer; //xon's IDA database gives as SkirmishCommanderDeath dd ?
	  Unknown13              : array [0..143] of Byte;
	  lPacketBufferSize      : Cardinal;
	  p_BacketBuffer         : Pointer;
	  nLOS_Radar             : Word; //xpoy's IDA db gives as "CurrentPlayerNumbers"
    lChatTextIndex         : Cardinal;
	  cSharedBits            : Byte; //player info! - xpoy's IDA db gives as "HumanPlayer_PlayerID"
	  cLOS_Sight_PlayerID    : Byte; //the player id to use for los calcs
	  cNetworkLayerEnabled   : Byte;
	  cUnknown14             : array [0..584] of Byte;
	  nBuildPosX             : SmallInt; //0x2C8E
	  nBuildPosY             : SmallInt;
	  lBuildPosRealX         : Integer; //0x2C92
	  lHeight                : Integer;
	  lBuildPosRealY         : Integer;
	  lUnknown15             : Integer;
	  lHeight2               : Integer;
	  Unknown16              : array [0..5] of Byte;
	  nMouseMapPosX          : SmallInt; //0x2CAC
	  Unknown17              : array [0..5] of Byte;
	  nMouseMapPosY          : SmallInt; //0x2CB4
	  Unknown18              : array [0..3] of Byte;
	  unMouseOverUnit        : Word; //0x2CBA
	  Unknown19              : array [0..6] of Byte;
    ucPrepareOrderType     : Byte;
	  nBuildNum              : Word; //0x2CC4, unitindex for selected unit to build
	  cBuildSpotState        : Byte; //0x40=notoktobuild
	  Unknown20              : array [0..43] of Byte;
	  Weapons                : array [0..255] of TWeaponDef; //0x2CF3 size=0x11500
	  lNumProjectiles        : Integer;
	  p_Projectiles          : Pointer; //0x141F7
    TNTMemStruct           : TTNTMemStruct;
	  Unknown25              : array [0..59] of Byte;
    MinimapRect            : tagRECT;//0x142CB
	  p_RadarFinal           : Pointer; //0x142DB
	  p_RadarMapped          : Pointer; //0x142DF
	  p_RadarPicture         : Pointer; //0x142E3
	  nXOffset_RadarMap      : SmallInt;
	  nYOffset_RadarMap      : SmallInt;
	  nRadarMinimap_SizeX    : SmallInt;
	  nRadarMinimap_SizeY    : SmallInt;
	  nUnknown26             : Word;
	  nUnknown27             : Word;
	  pCameraToUnit          : Pointer; //0x142F3
	  Unknown28              : array [0..39] of Byte;
	  lEyeBallMapX           : Integer; //0x1431F
	  lEyeBallMapY           : Integer; //0x14323
	  lEyeBallMapXScrollTo   : Integer; //0x14327
	  lEyeBallMapYScrollTo   : Integer; //0x1432B
    field_1432F            : Cardinal;
    field_14333            : Cardinal;
    ShakeMagnitude_1       : Cardinal;
    ShakeMagnitude_2       : Cardinal;
    field_1433F            : Cardinal;
    field_14343            : Cardinal;
    field_14347            : Cardinal;
    lastWeaponHoldTime     : Word;
	  bScrollSpeed           : Byte;
    cShake                 : Byte;
	  nEveryPlayerUnitsNr    : Word;
	  Unknown30              : array [0..1] of Byte;
	  lNumTotalGameUnits     : Cardinal;
	  p_Units                : Pointer; //0x14357
	  p_LastUnitInArray      : Pointer;
	  nHotUnits              : Cardinal;//0x1435F
	  nHotRadarUnits         : Cardinal;
	  lnNumHotUnits          : Cardinal; //0x14367
    lnNumHotRadarUnits     : Cardinal; //0x1436B
    unknow_20              : Word;
    Bigbrother             : Word;
    Bigbrother_            : Cardinal;
    p_MODEL_PTRS           : Pointer;
    ModelMapBuffer         : Cardinal;
    field_1437F            : Cardinal;
    TEMP_XFORM_PTS         : Cardinal;
    TEMP_PROJECTED_PTS     : Cardinal;
    ASSEM_PTS              : Cardinal;
    lNumUnitTypeDefs       : Cardinal;
    lNumUnitTypeDefs_Sqrt  : Cardinal;
    LoadedUNITINFO         : Cardinal;
	  p_UnitDefs             : Pointer; //0x1439B
	  Unknown33              : array [0..1083] of Byte;
	  l_0_FontValidCharNr    : Cardinal;
	  l_1_FontValidCharNr    : Cardinal;
	  Unknown34              : array [0..115] of Byte;
	  p_FogOfWar             : Pointer; //0x1485B
	  Unknown35              : array [0..35] of Byte;
	  lCursor_Attack         : Cardinal; //0x14883
	  lCursor_AirStrike      : Cardinal;
	  lCursor_TooFar         : Cardinal;
	  lCursor_Capture        : Cardinal;
	  lCursor_Defend         : Cardinal;
	  lCursor_Repair         : Cardinal;
	  lCursor_Patrol         : Cardinal;
	  lCursor_Pickup         : Cardinal;
	  lCursor_Teleport       : Cardinal;
	  lCursor_Revive         : Cardinal;
	  lCursor_Reclaim        : Cardinal;
	  lCursor_Load           : Cardinal;
	  lCursor_Unload         : Cardinal;
	  lCursor_Move           : Cardinal;
	  lCursor_Select         : Cardinal;
	  lCursor_FindSite       : Cardinal;
	  lCursor_Red            : Cardinal;
	  lCursor_Green          : Cardinal;
	  lCursor_Normal         : Cardinal;
	  lCursor_Hourglass      : Cardinal;
	  lCursor_PathIcon       : Cardinal; //0x148D3
	  Unknown36              : array [0..67] of Byte;
	  lNumExplosions         : Cardinal; //0x1491B
    Explosions             : array [0..299] of TExplosion; //0x1491F
	  pUnknown36             : Pointer; //0x1AB8F
	  Unknown37              : array [0..102011] of Byte;
	  lGUITextSound          : Cardinal; //0x33A0F
	  lGUISounds             : Cardinal; //0x33A13
	  Unknown38              : array [0..1019] of Byte;
	  lGUITextMap            : Cardinal; //0x33E13 - pointer to an array of strings (0x20 each)
	  Unknown39              : array [0..16379] of Byte;
	  pSoundClassAry         : Pointer; //0x37E13
	  lSoundClassNumber      : Cardinal; //0x37E17
    ScreenOFFSCREEN        : Pointer;
	  Unknown40              : array [0..23] of Byte;
	  lInGamePos_X           : Cardinal; //0x37E37
	  lInGamePos_Y           : Cardinal;
    ViewResBar             : array [0..20] of Byte;
    Active_BottomState     : array [0..47] of Byte;
	  Unknown41              : array [0..101] of Byte;
	  nPerMissionUnitLimit   : Word; //0x37EE6
	  nUnknown42             : Word;
	  nActualUnitLimit       : Word;
	  nMaxUnitLimitPerPlayer : Word;
	  lCurrenTAIProfile      : Cardinal; //0x37EEE - xpoy's gives this as "Difficulty"
	  lSide                  : Cardinal; //0x37EF2
	  lUnknown43             : Cardinal;
	  lInterfaceType         : Cardinal;
	  lUnknown44             : Cardinal;
	  lSingleLOSType         : Cardinal;
	  GameOptionMask         : Byte; //0x37F06
    damagebarsvalue        : Byte;
	  Gamma                  : Cardinal;
	  lFXVol                 : Cardinal; //0x37F0C
	  lMusicVol              : Cardinal;
	  nMusicMode             : Word;
	  cCDMode                : Byte; //0x37F16
	  cUnitChat              : Byte;
	  cUnitChatText          : Byte;
	  nackNBuildNSpeech_Fx   : Word; //0x37F19 - xpoy's gives as "SoundMode"
	  lDisplayModeWidth      : Cardinal;
	  lDisplayModeHeight     : Cardinal;
	  lTextScroll            : Cardinal; //0x37F23
	  lTextLines             : Cardinal;
	  lMouseSpeed            : Cardinal;
	  nSwitchesMask          : Word;
	  Unknown46              : array [0..11] of Byte;
    RaceSideData           : array [0..4] of TRaceSideDataStruct;
    RandNum_               : Cardinal;
    field_38A3B            : Cardinal;
    scrollLen_buf          : Cardinal;
    field_38A43            : Cardinal;
	  lGameTime              : Integer; //0x38A47
	  nTAGameSpeed           : Word; //0x38A4B
	  nTAGameSpeed_Init      : Word;
    field_38A4F            : Word;
  	cIsGamePaused          : Byte; //0x38A51
    field_38A52            : Byte;
    Image_Output_Dir       : array [0..255] of Byte;
    Movie_Shot_Output_Dir  : array [0..255] of Byte;
    field_38C53            : Cardinal;
	  lMovieOutputRate       : Cardinal; //0x38C57
    lMovieNextFrameTick    : Cardinal;
    field_38C5F            : Cardinal;
    Movie                  : Cardinal;
    field_38C67            : array [0..259] of Byte;
    field_38D6B            : Cardinal;
    bLoadProgTextures      : Byte;
    bLoadProgTerrain       : Byte;
    bLoadProgUnits         : Byte;
    bLoadProgAnims         : Byte;
    bLoadProg3DData        : Word;
    nPlayersSynchMask      : Word;
    SfxVectorArray_ptr     : Pointer;
    SmackMovie_ptr         : Pointer;
    field_38D7F            : Word;
	  lMaxPlayers            : Cardinal; //0x38D81 - xon's gives as "NumSkirmishPlayers"
    CurrenttTick           : Cardinal;
    field_38D89            : Cardinal;
    ProfileAry             : array[0..31] of Byte;
    field_38DAD            : Cardinal;
    field_38DB1            : Cardinal;
    field_38DB5            : array[0..31] of Byte;
    IsDrawProfile_b        : Cardinal;
    field_38DD9            : array[0..637] of Byte;
    field_39057            : Cardinal;
    field_3905B            : Cardinal;
    field_3905F            : Cardinal;
    field_39063            : Cardinal;
    field_39067            : Cardinal;
    field_3906B            : Cardinal;
    field_3906F            : Cardinal;
    field_39073            : Cardinal;
    field_39077            : Cardinal;
    field_3907B            : Cardinal;
    Palette                : Pointer;
    currentPalette         : Pointer;
    desiredPalette         : Pointer;
    FadeTable              : Pointer;
    field_3908F            : Byte;
    field_39090            : array[0..282] of Byte;
    field_391AB            : Cardinal;
    field_391AF            : Cardinal;
    field_391B3            : Cardinal;
    field_391B7            : Word;
    field_391B9            : Cardinal;
    field_391BD            : Word;
    Showranges             : Cardinal;
    bps                    : Cardinal;
    field_391C7            : Cardinal;
    field_391CB            : Cardinal;
    field_391CF            : array[0..25] of Byte;
	  p_MapOTAFile           : PMapOTAFile; //0x391E9
	  Unknown52              : array [0..3] of Byte; //0x391ED - there's references to [p_TAMemory + 0x391ED] in ta.exe, so this is definitely something
	  lGUICallbackState      : Cardinal; //0x391F1
	  lGUICallback           : Cardinal; //0x391F5
    lengthOfCOMIXFnt       : Cardinal;
    lengthOFsmlfontFnt     : Cardinal;
	  Unknown53              : array [0..23] of Byte;
	  lSingleCommanderDeath  : Cardinal; //0x39219
	  lSingleMapping         : Cardinal;
	  lSingleLOS             : Cardinal;
	  lSingleLOSTypeOptions  : Cardinal;
	  lMultiCommanderDeath   : Cardinal; //0x39229
	  lMultiMapping          : Cardinal;
	  lMultiLOS              : Cardinal;
	  lMultiLOSTypeOptions   : Cardinal; //0x39235
	  Unknown54              : array [0..1] of Byte;
	  nGameState             : Word; //0x3923B
	  Unknown55              : array [0..3522] of Byte; //to get size to 0x3A000 (what xpoy's IDA db says the size of this struct is
  end;

  TPacketUnitRecreate = packed record
    f0 : Byte;
    UTypeID : Word;
    UnitID : Word;
    Position : TPosition;
  end;

  PUnitOrder = ^TUnitOrder;
  TUnitOrder = packed record
    p_PriorOrder_uosp : Pointer;
    cOrderType        : Byte;
    cState            : Byte;
    unknow_0          : Word;
    field_8           : Word;
    unknow_1____      : Cardinal;
    p_Unit            : Pointer;
    field_12          : Cardinal;
    p_UnitTarget      : Pointer;
    field_1A          : Cardinal;
    p_ThisOrder       : Pointer;   // Just a pointer to this order.
    Pos               : TPosition;
    unknow_5          : Cardinal;
    field_32          : Word;
    nFootPrint        : Word;
    lPar1             : Cardinal;  // for lab build queue = unit type
    lPar2             : Cardinal;  // for lab build queue = amount of units
    lUnitOrderFlags   : Cardinal;
    lOrder_State      : Cardinal;  // reclaim - going to 00103801 / start reclaim 00503801
    lStartTime        : Cardinal;
    p_NextOrder_uos   : Pointer;
    lMask             : Cardinal;
    p_Order_CallBack  : Pointer;
  end;

  PWeaponProjectile = ^TWeaponProjectile;
  TWeaponProjectile = packed record
    Weapon          : Pointer;
    Position_Curnt  : TPosition;
    Position_Start  : TPosition;
    Position_Target : TPosition;
    Position_Target2: TPosition;
    field_34        : Word;
    Turn            : TTurn;
    data2           : Word;
    field_3E        : Integer;
    CreateTime      : Integer;
    TimeToDeath     : Integer;
    CreatingTime    : Integer;
    p_TargetUnit    : Pointer;
    p_AttackerUnit  : Pointer;
    lInterceptor    : Integer;     // enemy units nukes counter
    field_5A        : Integer;
    field_5E        : Word;
    nBurst          : Word;
    nObjectPiece    : Word;
    nPropellerSpeed : Word;
    cMyLos_PlayerID : Byte;
    field_67        : Word;
    Mask            : Byte;
    data3           : Byte;
  end;

  TTAActionType = ( Action_Ready = 0,
                    Action_Activate = 1,
                    Action_AirStrike = 2,
                    Action_AirToAir = 3,
                    Action_AirToGround = 4,
                    Action_AirToGroundHover = 5,
                    Action_Attack_Chase = 6,    //ATTACK_NOMOVE
                    Action_Attack_Kamikaze = 7,      // it works even for units with kamikaze=0 (except air)
                    Action_Attack_NoMove = 8,      // not weapon 3
                    Action_AttackSpecial = 9,
                    Action_AttackUType = 10,  // commander stuck
                    Action_BeCarried = 11,
                    Action_BuildingBuild = 12, // labs building
                    Action_BuildWeapon = 13,
                    Action_Capture = 14,
                    Action_Cloak_Off = 15,
                    Action_Cloak_On = 16,
                    Action_Deactivate = 17,
                    Action_Follow_Ground = 18,
                    Action_GetBuilt = 19,
                    Action_Ground_Pickup = 20, // ARMTSHIP
                    Action_Ground_Unload = 21,
                    Action_Guard_NoMove = 22, // ready for mobile units
                    Action_HelpBuild = 23, // other builder joins build
                    Action_MakeSelectable = 24,
                    Action_MobileBuild = 25,
                    Action_Move_Ground = 26,
                    Action_Paralyze = 27,
                    Action_Park = 28,
                    Action_Patrol = 29,
                    Action_QMove = 30,
                    Action_QPatrol = 31,
                    Action_Reclaim = 32,
                    Action_ReclaimUnit = 33,
                    Action_RepairPatrol = 34,
                    Action_RepairUnit = 35,
                    Action_RepairUnitNoMove = 36,
                    Action_Resurrect = 37,
                    Action_SelfDestruct = 38,
                    Action_SelfDestructFG = 39, //countdown
                    Action_SelfRepair = 40,
                    Action_Standby = 41,
                    Action_Standby_Mine = 42,
                    Action_Standing_FireOrder = 43,
                    Action_Standing_MoveOrder = 44,
                    Action_Stop = 45,
                    Action_Suppress = 46,  //
                    Action_Teleport = 47,
                    Action_VTOL_Evade = 48,
                    Action_VTOL_Follow = 49,
                    Action_VTOL_GetRepaired = 50,
                    Action_VTOL_HelpBuild = 51,
                    Action_VTOL_LandIfCan = 52,
                    Action_VTOL_Landing= 53,
                    Action_VTOL_MobileBuild = 54,
                    Action_VTOL_Move = 55,
                    Action_VTOL_Patrol = 56,
                    Action_VTOL_Pickup = 57,   // atlas -> ground unit
                    Action_VTOL_Reclaim = 58,
                    Action_VTOL_ReclaimUnit = 59,
                    Action_VTOL_RepairPatrol = 60,
                    Action_VTOL_RepairUnit = 61,
                    Action_VTOL_SeekAttack = 62,
                    Action_VTOL_SeekGuard = 63,
                    Action_VTOL_Standby = 64,
                    Action_VTOL_Unload = 65,
                    Action_Wait = 66,
                    Action_WaitForAttack = 67,
                    Action_NoResult = 68 );

  // custom structures used by TADR
  TCobMethods = ( Activate, Deactivate, Upgrade, Reminder, Cloak );

  TUnitCategories = ( ucsNoChase, ucsPriBadTarget, ucsSecBadTarget, ucsTerBadTarget );

  TDmgType = ( dtWeapon = 1,
               dtParalyze = 2,
               dtSelfDestruct = 3,// 30000 self, self
               dtGiveUnit = 4,    // 30000 nil, target
               dtReclaim = 5,
               dtUnitDeath = 6,   // or 3 (differs if unit died because of enemy unit or aoe of owner weapon) dmg 30000
               dtDieInLab = 9,    // 30000 builder, builded unit or self, self
               dtHeal = 10,       // one, two or self,self
               dtUnknown1 = 11 ); // maybe net packet unit health confirm

  TUnitSearchFilter = ( usfNone,
                        usfOwner,
                        usfAllied,
                        usfEnemy,
                        usfAI,
                        usfExcludeAir,
                        usfExcludeSea,
                        usfExcludeBuildings,
                        usfExcludeNonWeaponed,
                        usfIncludeInBuildState );

  TUnitSearchFilterSet = set of TUnitSearchFilter;

  TUnitInfoExtensions = (
    UNITINFO_BUILDER = 0,
		UNITINFO_FLOATER = 1,
		UNITINFO_AMPHIBIOUS = 2,
		UNITINFO_STEALTH = 3,
		UNITINFO_ISAIRBASE = 4,
		UNITINFO_TARGETTINGUPGRADE = 5,
		UNITINFO_TELEPORTER = 6,
		UNITINFO_HIDEDAMAGE = 7,
		UNITINFO_SHOOTME = 8,
    UNITINFO_HASWEAPONS = 9,
		UNITINFO_CANFLY = 10,
		UNITINFO_CANHOVER = 11,
		UNITINFO_IMMUNETOPARALYZER = 12,
		UNITINFO_HOVERATTACK = 13,
		UNITINFO_ANTIWEAPONS = 14,
		UNITINFO_DIGGER = 15,
		UNITINFO_ONOFFABLE = 16,
		UNITINFO_CANSTOP = 17,
		UNITINFO_CANATTACK = 18,
		UNITINFO_CANGUARD = 19,
		UNITINFO_CANPATROL = 20,
		UNITINFO_CANMOVE = 21,
		UNITINFO_CANLOAD = 22,
		UNITINFO_CANRECLAMATE = 23,
		UNITINFO_CANRESURRECT = 24,
		UNITINFO_CANCAPTURE = 25,
		UNITINFO_CANDGUN = 26,
		UNITINFO_KAMIKAZE = 27,
		UNITINFO_COMMANDER = 28,
		UNITINFO_SHOWPLAYERNAME = 29,
		UNITINFO_CANTBERANSPORTED = 30,
		UNITINFO_UPRIGHT = 31,
		UNITINFO_BMCODE = 32,

		UNITINFO_SOUNDCTGR = 33,

		UNITINFO_MOVEMENTCLASS_SAFE = 34,
		UNITINFO_MOVEMENTCLASS = 35,

		UNITINFO_MAXHEALTH = 36,
		UNITINFO_HEALTIME = 37,

		UNITINFO_MAXSPEED = 38,
		UNITINFO_ACCELERATION = 39,
		UNITINFO_BRAKERATE = 40,
		UNITINFO_TURNRATE = 41,
		UNITINFO_CRUISEALT = 42,
		UNITINFO_MANEUVERLEASH = 43,
		UNITINFO_ATTACKRUNLEN = 44,
		UNITINFO_MAXWATERDEPTH = 45,
		UNITINFO_MINWATERDEPTH = 46,
		UNITINFO_MAXSLOPE = 47,
		UNITINFO_MAXWATERSLOPE = 48,
		UNITINFO_WATERLINE = 49,

		UNITINFO_TRANSPORTSIZE = 50,
		UNITINFO_TRANSPORTCAP = 51,

		UNITINFO_BANKSCALE = 52,
		UNITINFO_KAMIKAZEDIST = 53,
		UNITINFO_DAMAGEMODIFIER = 54,

		UNITINFO_WORKERTIME = 55,
		UNITINFO_BUILDDIST = 56,

		UNITINFO_SIGHTDIST = 57,
		UNITINFO_RADARDIST = 58,
		UNITINFO_SONARDIST = 59,
		UNITINFO_MINCLOAKDIST = 60,
		UNITINFO_RADARDISTJAM = 61,
		UNITINFO_SONARDISTJAM = 62,

		UNITINFO_MAKESMETAL = 63,
		UNITINFO_FENERGYMAKE = 64,
		UNITINFO_FMETALMAKE = 65,
		UNITINFO_FENERGYUSE = 66,
		UNITINFO_FMETALUSE = 67,
		UNITINFO_FENERGYSTOR = 68,
		UNITINFO_FMETALSTOR = 69,
		UNITINFO_FWINDGENERATOR = 70,
		UNITINFO_FTIDALGENERATOR = 71,
		UNITINFO_FCLOAKCOST = 72,
		UNITINFO_FCLOAKCOSTMOVE = 73,

		UNITINFO_BUILDCOSTMETAL = 74,
		UNITINFO_BUILDCOSTENERGY = 75,
    UNITINFO_BUILDTIME = 76,

		UNITINFO_EXPLODEAS = 77,
		UNITINFO_SELFDSTRAS = 78,
    UNITINFO_ISFEATURE = 79 );
    
  PCustomUnitInfo = ^TCustomUnitInfo;
  TCustomUnitInfo = packed record
    unitId        : Cardinal;
    unitIdRemote  : Cardinal;  // owner's (unit upgrade packet sender) local unit id
    //OwnerPlayer  : Longint; // Buffer.EventPlayer_DirectID = PlayerAryIndex2ID(UnitInfo.ThisPlayer_ID   )
    InfoPtrOld   : Cardinal;  // local old global template Pointer
    InfoStruct   : TUnitInfo;
  end;
  TUnitInfos = array of TCustomUnitInfo;

  TStoreUnitsRec = packed record
    Id : Cardinal;
    UnitIds  : array of LongWord;
  end;
  TUnitSearchArr = array of TStoreUnitsRec;
  TSpawnedMinionsArr = array of TStoreUnitsRec;

  TCustomUnitFieldsRec = packed record
    LongID               : Cardinal;
    { GUI }
    CustomWeapReload     : Boolean;
    CustomWeapReloadMax  : Integer;
    CustomWeapReloadCur  : Integer;
    CircleSelectProgress : Cardinal;
    CircleSelectRadJig   : Byte;
  end;
  TCustomUnitFieldsArr = array of TCustomUnitFieldsRec;

  TExtraWeaponDefTagsRec = packed record
    HighTrajectory   : Integer;
    PreserveAccuracy : Integer;
    NotAirWeapon     : Integer;
    WeaponType2      : String[64];
  end;

  TExtraUnitDefTagsRec = packed record
    MultiAirTransport : Integer;
    ExtraVTOLOrders : Integer;
    TransportWeightCapacity : Integer;
    HideHPBar : Boolean;
    NotLab : Boolean;
    DrawBuildSpotNanoFrame : Boolean;
    AISquadNr : Integer;
  end;

var
  // additional GAF animations
  ExtraAnimations : array[0..15] of Pointer;

  ExtraWeaponDefTags : array of TExtraWeaponDefTagsRec;
  ExtraUnitDefTags : array of TExtraUnitDefTagsRec;

  // pointer to ddraw's weapon array
  WeaponLimitPatchArr : Pointer;

  NanoSpotUnitSt, NanoSpotQueueUnitSt : TUnitStruct;
  NanoSpotUnitInfoSt, NanoSpotQueueUnitInfoSt : TUnitInfo;

  LineNanoSpotUnitSt : array of TUnitStruct;
  LineNanoSpotUnitInfoSt : TUnitInfo;

  // map missions
  MapMissionsUnit : TUnitStruct;
  MapMissionsUnitInfo : TUnitInfo;
  MapMissionsCOB : Pointer;
  MapMissionsSounds : TStringList;
  MapMissionsFeatures : TStringList;
  MouseLock : LongBool;
  CameraFadeLevel : Integer;

  // data that can be shared globally between units
  UnitsSharedData : array[0..1024] of Integer;
  
implementation

end.