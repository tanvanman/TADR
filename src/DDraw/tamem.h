#ifndef tamemH
#define tamemH

#include <dsound.h>
#include <ddraw.h>


struct _GAFFrame; 

#pragma pack(1)



enum PlayerType;
enum PlayerPropertyMask;
struct PlayerStruct;
struct PlayerInfoStruct;
struct UnitStruct;
struct UnitOrdersStruct;
struct WeaponStruct;
struct GameingState;
struct UnitDefStruct;
struct GafAnimStruct;
struct Object3doStruct;
struct PrimitiveStruct;
struct PrimitiveInfoStruct;
struct ProjectileStruct;
struct FeatureDefStruct;
struct FXGafStruct;
struct FeatureStruct;
struct WreckageInfoStruct;
struct DebrisStruct;
struct Unk1Struct;
struct Point3;
struct SmokeListNode;
struct ParticleSystemStruct;
struct SmokeListNode;
struct ParticleBase;
struct SmokeGraphics;
struct RadarPicStruct;
struct DSoundStruct;
struct _OFFSCREEN;
struct PlayerResourcesStruct;
struct _RaceSideData;
struct _Vertices;
struct _Shatte;
struct _TAProgramStruct;
struct _GUIInfo;
struct _GUIMEMSTRUCT;
struct _GUI1IDControl;
struct _GUI0IDControl;
struct _GUI2IDControl;
struct _GUI3_4IDControl;
struct _GUI5IDControl;
struct _GUI78IDControl;
struct _GUI6IDControl;
struct _GUI9IDControl;
struct _TAProgramStruct ;
struct _GAFSequence;
struct RadarUnit_ ;
struct _MOUSEEVENT ;
struct _Position_Dword  ;
struct _COBHandle;
struct MapStartPosStruct;

struct Point3{
	int x;
	int y;
	int z;
};
typedef struct _Volume_Word
{
	unsigned short X;
	unsigned short Y;
	unsigned short Z;
}Volume_Word;

struct PlayerResourcesStruct
{
	float fCurrentEnergy;//here,  need to make sure, that float is 32 bit length
	float fEnergyProducton;
	float fEnergyExpense;
	float fCurrentMetal;
	float fMetalProduction ;
	float fMetalExpense  ;
	float fMaxEnergyStorage ;
	float fMaxMetalStorage ;
	double fTotalEnergyProduced ;
	double fTotalMetalProduced;
	double fTotalEnergyConsumed;
	double fTotalMetalConsumed ;
	double fEnergyWasted;
	double fMetalWasted;
	float fPlayerEnergyStorage ;
	float fPlayerMetalStorage ;
};

struct PlayerSharedResourcesStruct
{
	float fEnergyReceived;
	int field_4;
	int field_8;
	int field_c;
	int field_10;
	int field_14;
	float fMetalReceived;
	int field_1c;
	int field_20;
	int field_24;
};

struct PlayerStruct
{
	int PlayerActive;
	int DirectPlayID;
	int field_8;
	char PlayerNum;
	char field_D[3];
	int field_10;
	char field_14[14];
	char field_22;
	char field_23[4];
	PlayerInfoStruct * PlayerInfo;
	char Name[30];
	char SecondName[30];
	UnitStruct * Units;
	UnitStruct * UnitsAry_End;
	__int16 UnitsIndex_Begin;
	__int16 UnitsIndex_End;
	char My_PlayerType;		// aka PlayerStruct.Controller.Controller
	int AiConfig;
	int field_78;
	unsigned char * LOS_MEMORY_p;
	int LOS_Tilewidth;
	int LOS_Tileheight;
	int LOS_bitsNum;
	PlayerResourcesStruct PlayerRes;
	float ShareMetal;
	float ShareEnergy;
	PlayerSharedResourcesStruct* resourcesShared;
	int UpdateTime;
	int WinLoseTime;
	int DisplayTimer;
	__int16 Kills;
	__int16 Losses;
	int field_100;
	__int16 kills_2;
	__int16 losse_2;
	char AllyFlagAry[10];
	char field_112;
	char field_113;
	char field_114[21];
	char field_129;
	char field_12A[21];
	char AllyTeam;
	int WholeUnitsCounters;
	__int16 UnitsNumber;
	char PlayerAryIndex;
	char mapStartPos;
	char field_148;
	__int16 AddPlayerStorage_word;
};

struct WeaponStruct {
  char WeaponName[0x20];
  char WeaponDescription[0x40];
  int Unkn1;
  char data3[0x14];
  GafAnimStruct *LandExplodeAsGFX;
  GafAnimStruct *WaterExplodeAsGFX;
  char data4[0x54];
  short Damage; //d4
  short AOE;
  float EdgeEffectivnes;
  short Range;
  short data5;
  unsigned int coverage;
  char data6[0x26];
  unsigned char ID;
  char data8[1];
  char RenderType;
  char data9[4];
  unsigned int WeaponTypeMask;
};  //0x115

struct ExplosionStruct{
	DebrisStruct *Debris;
	short Frame;
	char data2[6];
	FXGafStruct *FXGaf;
	char data3[12];
	int XPos;
	int ZPos;
	int YPos;
	char data4[36];
	short XTurn; //0x4c
	short ZTurn;
	short YTurn;
	char data5[2];
};//0x54

struct DebrisStruct{
	char data1[0x24];
	Point3 *Vertices;
	Unk1Struct *Unk;
	char data2[8];
};//0x34

struct Unk1Struct{
	char data1[0x18];
	FXGafStruct *Texture;
	char data2[0x4];
};//0x20

struct FXGafStruct{
	short Frames;
	char data1[6];
	char Name[0x20];
	int FramePointers[1];
};

struct DSoundStruct{
	char data1[0x24];
	LPDIRECTSOUND Directsound;
	LPDIRECTSOUNDBUFFER DirectsoundBuffer;
	char data2[0xC];
	LPDIRECTSOUNDBUFFER DirectsoundBuffer2;
};

struct StartPositionsStruct
{
	int isActive[10];
	int field_28;
	int positions[10];
};

typedef void(__stdcall* InternalCommandFunctionPtr)(char* argv[]);
enum InternalCommandRunLevel
{
	CMD_LEVEL_NULL = 0,
	CMD_LEVEL_NORMAL = 1,
	CMD_LEVEL_CHEATING = 2,
	CMD_LEVEL_DEBUG = 4
};

struct InternalCommandTableEntryStruct
{
	const char* name;
	InternalCommandFunctionPtr function;
	InternalCommandRunLevel runLevel;
};

typedef struct _GUIInfo
{
	int field_0;
	int commongui_GAF;
	char field_8[16];
	_GUIMEMSTRUCT * TheActive_GUIMEM;
	int Cursor_0;
	int Cursor_1;
	int Cursor_2;
	int field_28;
	int Cursor_3;
	__int16 field_30;
	__int16 field_32;
	int field_34;
	int field_38;
	char field_3C[24];
	int field_54;
	int field_58;
	int field_5C;
	int UIChange_f;  // index into TheActive_GUIMEM->ControlsAry that was fiddled, or -1
	int field_64;
	int field_68;
	int field_6C;
	int field_70;
	int field_74;
	int field_78;
	char field_7C[60];
	char  field_B8[2042];
	unsigned char RadarObjecColor[16];
	char field_8C2[1032];
	int GUIUpdated_b;
}GUIInfo;
typedef struct _RaceSideData
{
	char name[30];
	char nameprefix[4];
	char commanderUnitName[32];
	RECT Logo_rect;
	RECT Energy_rect;
	RECT EnergyNum_rect;
	RECT METALBAR_rect;
	RECT METALNUM_rect;
	RECT TOTALUNITS_rect;
	RECT TOTALTIME_rect;
	RECT ENERGYMAX_rect;
	RECT METALMAX_rect;
	RECT ENERGY0_rect;
	RECT METAL0_rect;
	RECT ENERGYPRODUCED;
	RECT ENERGYCONSUMED;
	RECT METALPRODUCED;
	RECT METALCONSUMED;
	RECT LOGO2;
	RECT UNITNAME;
	RECT DAMAGEBAR;
	RECT UNITENERGYMAKE;
	RECT UNITENERGYUSE;
	RECT UNITMETALMAKE;
	RECT UNITMETALUSE;
	RECT MISSIONTEXT;
	RECT UNITNAME2;
	RECT field_1C2;
	RECT NAME;
	RECT DESCRIPTION;
	RECT RELOAD_RaceID;
	char field_202[32];
	int energycolor;
	int metalcolor;
	unsigned int  RaceAryIndex ;
	unsigned int  Font_File ;
}RaceSideData, *PRaceSideData;

typedef struct _Position_Dword  
{
	unsigned short x_;
	unsigned short X;
	unsigned short z_;
	unsigned short Z;
	unsigned short y_;
	unsigned short Y;
} Position_Dword ;
//settimer 4fb368

struct HAPINETStruct {
	char data0[1205];			// 0x0000
	unsigned fromDpid;			// 0x04b5
	unsigned toDpid;			// 0x04b9
	char data4bd[8];			// 0x04bd
	void* directPlayInterface;	// 0x04c5
								// 0x04c9
};

struct SkirmishInfo
{
	char data0[268];
	unsigned mapping;
	unsigned lineOfSight;
	unsigned lineOfSightType;
	char location[260];
	char field_21c[16];
};

struct TAdynmemStruct{
	char data1[12];				// 0x0000
	_TAProgramStruct * TAProgramStruct_Ptr;	// 0x000c
	DSoundStruct *DSound;		// 0x0010
	HAPINETStruct hapinet;		// 0x0014	..	0x04dd
	char data2b[0x3c];			// 0x04dd   ..  0x0519
	_GUIInfo desktopGUI;		// 0x0519
	char data3[0x97C];
	PlayerStruct Players[10];	//0x1B63 , end at 0x2851
	char data4[331];			// 0x2851
	unsigned int data5;			// 0x299c
	SkirmishInfo* skirmishInfo;	// 0x29a0
	char data6[0x2c];			// 0x29a4
	StartPositionsStruct startPositions;// 0x29d0		// populated during multiplayer load
	char data6b[0x0c];				// 0x2a24
	void* RestrictUnitList;			// 0x2a30
	unsigned int PacketBufferSize;	// 0x2a34
	char* PacketBuffer_p;
	unsigned short PlayerCounters;
	unsigned int ChatTextIndex;
	char LocalHumanPlayer_PlayerID;
	char LOS_Sight_PlayerID;
	char WorkStatusMask ;// 02A44 
	char data7[0x231];  
	POINT CurtMousePostion;//0x2C76
	char data7_[0x10];  //
	short BuildPosX; //0x2C8E
	short BuildPosY;
	unsigned int CircleSelect_Pos1TAx;
	unsigned int CircleSelect_Pos1TAz;
	unsigned int CircleSelect_Pos1TAy;
	unsigned int CircleSelect_Pos2TAx;
	unsigned int CircleSelect_Pos2TAz;
	unsigned int CircleSelect_Pos2TAy;


	_Position_Dword  MouseMapPos; // 0x02CAA


	char data10[4];
	unsigned short MouseOverUnit; 
	unsigned short MouseOverFeature ;
	unsigned char CurrentCursora_Index;
	unsigned char field_2CBF;
	unsigned char field_2CC0;
	unsigned short field_2CC1;
	unsigned char PrepareOrder_Type;

	short BuildUnitID;  //0x2CC4,  unitindex for selected BeginUnitsArray_p  to build
	char BuildSpotState; //0x40=notoktobuild
	char data12[0x2C];

	WeaponStruct Weapons[256];  //0x2CF3  size=0x11500	weapon types definitions
	//char data7[4];
	int NumProjectiles;
	ProjectileStruct *Projectiles;    //0x141F7
	char data13a[3];                  //0x141f8
	char data13b[13];                 //TNTMemStruct 0x141fb..0x1428f (size 0x94)
	WreckageInfoStruct *WreckageInfo; //0x1420B = 0x141fb + 0x10
	unsigned int Feature_Unit;
	char data14[0x10];
	int  MapWidth ;
	int  MapHeight;

	int MapSizeX;
	int MapSizeY;
	int FeatureMapSizeX;              //0x14233 = 0x141fb + 0x38
	int FeatureMapSizeY;              //0x14237 = 0x141fb + 0x3c
	char data15[0x18];
	int NumFeatureDefs;               //0x14253 = 0x141fb + 0x58
	int field_5c;                     //0x14257 = 0x141fb + 0x5c
	int MinWindSpeed;                 //0x1425b = 0x141fb + 0x60
	int MaxWindSpeed;                 //0x1425f = 0x141fb + 0x64
	int Gravity;                      //0x14263 = 0x141fb + 0x68
	float TidalStrength;                //0x14267 = 0x141fb + 0x6c
	char data16[0x04];
	FeatureDefStruct *FeatureDef;     //0x1426F = 0x141fb + 0x74
	unsigned short * MAPPED_MEMORY_p;
	unsigned int LastZPos;
	unsigned short * MinimapMEMORY_p;
	unsigned char SeaLevel ;
	unsigned char mapDebugMode;       // 0x14280 = 0x141fb + 0x85;  render debug info.  values 0 to 4
	unsigned short LosType;           // 0x14281 = 0x141fb + 0x86
	unsigned int TILE_SET;
	FeatureStruct *FeatureMap;        // 0x14287 = 0x141fb + 0x8c
	char data20[0x40];                // TNTMemStruct ends in here near beginning 
	tagRECT MinimapRect;//0x142CB
	RadarPicStruct *RadarFinal; //0x142DB
	RadarPicStruct *RadarMapped;  //0x142DF
	RadarPicStruct *RadarPicture;  //0x142E3
	char data21[4];
	short RadarPicSizeX;  //0x142EB
	short RadarPicSizeY;  //0x142ED
	char data22[2];
	char AntiWeaponDotte_b;
	char field_142F2;
	UnitStruct * CameraToUnit;//0x142F3 //used in drawcircle funktion
	char data23[0x28];
	int EyeBallMapXPos;	//0x1431f
	int EyeBallMapYPos;   //0x14323
	int MapXScrollingTo; //0x14327
	int MapYScrollingTo; //0x1432B
	char data24[0x24];
	unsigned int  UnitsCounter;
	UnitStruct *BeginUnitsArray_p ;//UnitStruct *
	UnitStruct *EndOfUnitsArray_p; //0x1435B UnitStruct *
	short int *HotUnits;//0x1435F
	RadarUnit_ * RadarUnits;
	int NumHotUnits; //0x14367
	int NumHotRadarUnits;
	char data25[0x20];
	unsigned int UNITINFOCount;
	unsigned int UNITINFOCount_SignificantBitsCount;
	unsigned int LoadedUNITINFOs;
	UnitDefStruct *UnitDef;  //0x1439b 
	char data26[0x440];
	_GAFSequence * radlogo; //0147DF
	_GAFSequence * radlogohigh;
	_GAFSequence * nuclogo ;//00147E7
	char data26__[0x30];
	_GAFSequence * igpaused;
	char data26_[0x60];///[0x94];
	_GAFSequence * cursor_ary [0x15];//0x1487F
	_GAFSequence * pathicon;		//0x148d3

	//char data27[0x44];				//0x148d7
	char data27[4];						//0x148d7
	_GAFSequence* _32xLogos;			//0x148db
	char data27b[0x3c];					//0x148df

	int NumExplosions; //0x1491B
	//char data9[0x6270];
	ExplosionStruct Explosions[300]; //0x1491F
	LPVOID CalcedExplosion; //0x1AB8F
	char data29[0x1D28C];					//0x1AB93
	unsigned int  ScreenWidth;				//0x37e1f
	unsigned int  ScreenHeight ;			//0x37e23

	RECT GameSreen_Rect;					//0x37E27
	unsigned int  GameScreenWidth;			//0x37e37
	unsigned int  GameScreenHeight;			//0x37e3b
	char data30[0x5d];						//0x37e3f
	unsigned short ShowRangeUnitIndex;		//0x37e9c
	unsigned short  field_37E9E;			//0x37e9e
	char CurtUnitGUIName[0x1e];				//0x37ea0
	unsigned short  DesktopGUIState;		//0x37ebe

	unsigned short field_37EC0;
	unsigned short field_37EC2;
	unsigned int  WindSpeedGameTicksNextUpdate;// 0x37ec4
	unsigned int  WindSpeedHardLimit;		//0x37ec8
	unsigned int  field_37ECC;
	unsigned int  field_37ED0;
	unsigned int  field_37ED4;
	short int WindDirection;				//0x37ed8
	unsigned int WindSpeed;					//0x37eda
	float WindSpeedFractionOfLimit;			//0x37ede
	char field_37edc[4];					//0x37ee2
	unsigned short PlayerUnitsNumber_Skim ; //0x37ee6
	unsigned short field_37EE8;
	unsigned short ActualUnitLimit ;		//0x37eea
	unsigned short MaxUnitNumberPerPlayer ;	//0x37eec
	unsigned int  Difficulty;				//0x37eee
	unsigned int  side;						//0x37ef2
	unsigned int  field_37EF6;

	int InterfaceType;						//0x37efa

	//char data31[0x31];					//0x37efe
	char data37efe[8];						//0x37efe
	char GameOptionMask;					//0x37f06		DrawScoreBoard:0x80
	char data37f07[40];						//0x37f07

	unsigned short SoftwareDebugMode;		//0x37f2f
	unsigned int  field_37F31;
	unsigned int  Senderror;
	unsigned int  RaceCounter;
	RaceSideData RaceSideDataAry[5];
	unsigned int  RandNum_ ;
	unsigned int  field_38A3B;
	unsigned int  field_38A3F ;
	unsigned int  field_38A43 ;

	int GameTime; //0x38A47
	short int GameSpeed;
	short int GameSpeed_Init;
	short int field_38A4F;
	bool IsGamePaused;
	unsigned char field_38A52;

	char  Image_Output_Dir[256];  // 0x38a47+c= TA截图目录的字符串，即TA目录+当前用户名 
	char  Movie_Shot_Output_Dir[256];
	char data_33[0x56c];
	unsigned int  Showranges;
	unsigned int  bps;
	unsigned int  field_391C7;
	unsigned int  field_391CB;
	char field_391CF[26];
	GameingState *GameingState_Ptr; //0x391E9
	int data34;
	int State_GUI_CallBack;//0x0391F1
	LPVOID GUI_CallBack;
	void* COMIXFontHandle;
	void* smalFontHandle;
	RECT field_39201     ;
	int DPLAY_CONNECTION_INFO;
	int field_39215    ;
	int SingleCommanderDeath;
	int SingleMapping  ;
	int SingleLineOfSight;
	int SingleLOSType_ ;
	int MultiCommanderDeath;
	int MultiMapping   ;
	int MultiLineOfSight;
	int LOSTypeOptions ;
	short NetStateMask0;//  maybe another game state mask
	short GameStateMask;

};

struct WreckageInfoStruct{
	int unk1;
	LPVOID unk2;
	int XPos;
	int ZPos;
	int YPos;
	char data1[0xC];
	short ZTurn;
	short XTurn;
	short YTurn;
	char data2[0xA];
};

struct FeatureStruct{
	unsigned short occupyingUnitNumber;
	unsigned short deadspace;
	unsigned char height;
	unsigned char maxHeight2x2;  // maximum height of 2x2 patch starting at this coordinate. TA bug gives unpredictable values on right-hand edge of map?
	unsigned char minHeight2x2;  // minimum height of 2x2 patch starting at this coordinate  TA bug gives unpredictable values on right-hand edge of map?
	unsigned char MetalValue;
	unsigned short FeatureDefIndex;
	unsigned char FeatureDefDy;  // if FeatureDefIndex is 0xfffe, offset in map coordinates to the real FeatureDefIndex
	unsigned char FeatureDefDx;  // if FeatureDefIndex is 0xfffe, offset in map coordinates to the real FeatureDefIndex
	unsigned char field_0c;
}; //0xD

struct FeatureDefStruct {
	char Name[0x20];
	char data1[0x60];
	char Description[20];
	short FootprintX;
	short FootprintZ;
	int objects3d;
	short unknownField_9C;
	void* unknownField_9E;
	char unknownFieldA2[6];
	void* Anims;
	char* SeqName;
	char* SeqNameShad;
	char* BurnName2Sequence;
	char* SeqNameBurnsShad;
	char* SeqNameDie;
	char* SeqNameDieShad;
	char* SeqNameReclamate;
	char* SeqNameReclamateShad;
	short unknownField_CC;
	short unknownField_CE;
	int equals0;
	int unknownField_D4;
	int unknownField_D8;
	int unknownField_DC;
	int unknownField_E0;
	WeaponStruct* BurnWeapon;
	short SparkTime;
	short Damage;
	float Energy;
	float Metal;
	char unknownField_F4[6];
	char Height;
	char SpreadChance;
	char Reproduce;
	char ReproduceArea;
	short FeatureMask;
	//char Data2[108];
}; //0x100

struct ProjectileStruct {
	WeaponStruct *Weapon;
	short int data0_1;
	short int XPos;
	short int data0_2;
	short int ZPos;
	short int data0_3;
	short int YPos;
	int XPosStart;
	int ZPosStart;
	int YPosStart;
	int XSpeed;
	int ZSpeed;
	int YSpeed;
	char data1[14];
	short XTurn;
	short ZTurn;
	short YTurn;
	char data2[42];
	char myLos_PlayerID;
	short int field_67;;

	struct {
		bool unk1 : 1;
		bool Inactive : 1;
		char unk2 : 6;
	} Inactive;
	char data3[1];
}; //0x6B


struct MissionUnitsStruct {
	const char* Unitname;		// 0x0000
	const char* Ident;			// 0x0004
	const char* InitialMission;	// 0x0008
	int XPos;	// *65k			// 0x000e
	int YPos;	// elevation	// 0x0012
	int ZPos;	// *65k			// 0x0016
	short unk3;
	char HealthPercentage;		// 0x001a
	char data1b;				// 0x001b
	int creationCountdown;		// 0x001c
	char data20[2];				// 0x0020
	short Player;	// 1..10	// 0x0022
};								// 0x0024

struct GameingState{
	unsigned int  State;		// 0x0000
	char data[0x200];			// 0x0004
	char TNTFile[MAX_PATH];		// 0x0204
	char data2[0x0a28];			// 0x0308
	unsigned surfaceMetal;		// 0x0d30
	unsigned minWindSpeed;		// 0x0d34
	unsigned maxWindSpeed;		// 0x0d38
	unsigned gravity;			// 0x0d3c
	unsigned tidalStrength;		// 0x0d40
	unsigned isLavaMap;			// 0x0d44
	char schemaInfo[100];		// 0x0d48
	MissionUnitsStruct* uniqueIdentifiers;	// 0x0dac
	unsigned uniqueIdentifierCount;// 0x0db0
	MapStartPosStruct* mapStartPosAry_;// 0x0db4
	unsigned mapStartPosCount;	// 0x0db8
};

struct MapStartPosStruct
{
	unsigned validStartMapPos;	// 0x0000
	unsigned playerId;			// 0x0004
	short X_Pos;				// 0x0008
	short Y_Pos;				// 0x000a
};

struct PlayerInfoStruct 
{
	char MapName[0x20];			// 0x0000
	char data_20[0x6b];			// 0x0020
	unsigned short screenWidth; // 0x008b
	unsigned short screenHeight;// 0x008d
	char data_90[5];			// 0x008f
	char PlayerType;			// 0x0094 local/remote human/AI
	char RaceSide;				// 0x0095
	char PlayerLogoColor;		// 0x0096
	char SharedBits;			// 0x0097 enum SharedStates
	char data2[3];				// 0x0098
	unsigned short PropertyMask;// 0x009b // & 0x4000:location random(0)/fixed(1)
};

struct CobHeader {
	int Version;
	int MethodCount;
	int PieceCount;
	int BytecodeLength;
	int StaticVariablesCount;
	int Always_0;
	unsigned *MethodEntryPoints;	// array of ints
	char **MethodNameOffsets;		// array of pointers into NameArray
	char **PieceNameOffsets;		// array of pointers into NameArray
	unsigned char* ByteCodeStart;
	char* NameArray;				// back-to-back null terminated strings
};

struct UnitDefStruct {
	/* 0x000 */ char Name[0x20];
	/* 0x020 */ char UnitName[0x20];
	/* 0x040 */ char UnitDescription[0x40];
	/* 0x080 */ char ObjectName[0x20];
	/* 0x0A0 */ char Side[8];
	/* 0x0A8 */ char data5[0xA2];
	/* 0x14A */ short FootX;
	/* 0x14C */ short FootY;
	/* 0x14E */ char* YardMap;
	/* 0x152 */ int canbuildCount;
	/* 0x156 */ short* CANBUILD_ptr;
	/* 0x15A */ int buildLimit;
	/* 0x15E */ unsigned short __X_Width;
	/* 0x160 */ unsigned short X_Width;
	/* 0x162 */ unsigned long data_7;
	/* 0x166 */ unsigned short Y_Width;
	/* 0x168 */ unsigned short __Y_Width;
	/* 0x16A */ unsigned long data_8;
	/* 0x16E */ unsigned short __Z_Width;
	/* 0x170 */ unsigned short Z_Width;
	/* 0x172 */ unsigned long data_9;
	/* 0x176 */ unsigned long data_10;
	/* 0x17A */ unsigned long data_11;
	/* 0x17E */ unsigned long data_12;
	/* 0x182 */ unsigned long data_13;
	/* 0x186 */ float buildcostenergy;
	/* 0x18A */ float buildcostmetal;
	/* 0x18E */ CobHeader* cobDataPtr;
	/* 0x192 */ unsigned long lRawSpeed_maxvelocity;
	/* 0x196 */ unsigned long data_15;
	/* 0x19A */ unsigned long data_16;
	/* 0x19E */ unsigned long cceleration;
	/* 0x1A2 */ unsigned long bankscale;
	/* 0x1A6 */ unsigned long pitchscale;
	/* 0x1AA */ unsigned long damagemodifier;
	/* 0x1AE */ unsigned long moverate1;
	/* 0x1B2 */ unsigned long moverate2;
	/* 0x1B6 */ unsigned long movementclass;
	/* 0x1BA */ unsigned short turnrate;
	/* 0x1BC */ unsigned short corpse;
	/* 0x1BE */ unsigned short maxwaterdepth;
	/* 0x1C0 */ unsigned short minwaterdepth;
	/* 0x1C2 */ unsigned long energymake;
	/* 0x1C6 */ float energyuse;
	/* 0x1CA */ unsigned long metalmake;
	/* 0x1CE */ unsigned long extractsmetal;
	/* 0x1D2 */ float windgenerator;
	/* 0x1D6 */ unsigned long tidalgenerator;
	/* 0x1DA */ unsigned long cloakcost;
	/* 0x1DE */ unsigned long cloakcostmoving;
	/* 0x1E2 */ unsigned long energystorage;
	/* 0x1E6 */ unsigned long metalstorage;
	/* 0x1EA */ unsigned long buildtime;
	/* 0x1EE */ WeaponStruct* weapon1;
	/* 0x1F2 */ WeaponStruct* weapon2;
	/* 0x1F6 */ WeaponStruct* weapon3;
	/* 0x1FA */ unsigned short nMaxHP;
	/* 0x1FC */ unsigned short data8;
	/* 0x1FE */ unsigned short nWorkerTime;
	/* 0x200 */ unsigned short nHealTime;
	/* 0x202 */ unsigned short nSightDistance;
	/* 0x204 */ unsigned short nRadarDistance;
	/* 0x206 */ unsigned short nSonarDistance;
	/* 0x208 */ unsigned short mincloakdistance;
	/* 0x20A */ unsigned short radardistancejam;
	/* 0x20C */ unsigned short sonardistancejam;
	/* 0x20E */ unsigned short SoundClassIndex;
	/* 0x210 */ unsigned short nBuildDistance;
	/* 0x212 */ unsigned short builddistance;
	/* 0x214 */ unsigned short nManeuverLeashLength;
	/* 0x216 */ unsigned short attackrunlength;
	/* 0x218 */ unsigned short kamikazedistance;
	/* 0x21A */ unsigned short sortbias;
	/* 0x21C */ unsigned char cruisealt;
	/* 0x21D */ unsigned char data4;
	/* 0x21E */ unsigned short UnitTypeID;
	/* 0x220 */ WeaponStruct* ExplodeAs;
	/* 0x224 */ WeaponStruct* SelfeDestructAs;
	/* 0x228 */ unsigned char maxslope;
	/* 0x229 */ unsigned char badslope;
	/* 0x22A */ unsigned char transportsize;
	/* 0x22B */ unsigned char transportcapacity;
	/* 0x22C */ unsigned char waterline;
	/* 0x22D */ unsigned short makesmetal;
	/* 0x22F */ unsigned char bmcode;
	/* 0x230 */ unsigned char defaultmissiontypeIndex;
	/* 0x231 */ unsigned long* wpri_badTargetCategory_MaskAryPtr;
	/* 0x235 */ unsigned long* wsec_badTargetCategory_MaskAryPtr;
	/* 0x239 */ unsigned long* wspe_badTargetCategory_MaskAryPtr;
	/* 0x23D */ unsigned long* noChaseCategory_MaskAryPtr;
	/* 0x241 */ unsigned long UnitTypeMask_0;
	/* 0x245 */ unsigned long UnitTypeMask_1;
	// 0x249
};

enum UnitSelectState
{
	UnitValid_State        = 0x4,
	UnitSelected_State     = 0x10,
	UnitValid2_State       = 0x20,
	UnitInSight_State		 = 0x40
};

struct UnitStruct {
  int IsUnit;
  char data1[12];
  WeaponStruct *Weapon1;
  char data2[10];
  char Weapon1Dotte;
  char Weapon1Valid;
  char data3_[12];
  WeaponStruct *Weapon2;
  char data4[10];
  char Weapon2Dotte;
  char Weapon2Valid;
  char data4_[12];
  WeaponStruct *Weapon3;
  char data5[10];
  char Weapon3Dotte;
  char Weapon3Valid;
  char Data5_[4];
  UnitOrdersStruct *UnitOrders;			//0x5c
  UnitOrdersStruct *BackgroundOrder;	//0x60
  _Volume_Word Turn;
//   unsigned short ZTurn;
//   unsigned short XTurn;
//   unsigned short YTurn;
  unsigned short XPos__ ;                    ; //0x6A
  unsigned short XPos;
  unsigned short ZPos__  ;
  unsigned short ZPos  ;
  unsigned short YPos__ ;
  unsigned short YPos ;
  short XGridPos;
  short YGridPos;
  short XLargeGridPos;
  short YLargeGridPos;
  char data8[4];
  LPVOID UnkPTR1;
  char data15[4];
  int Unknow_Order;//
  UnitStruct *FirstUnit; //?
  UnitDefStruct *UnitType; //0x92
  PlayerStruct  * Owner_PlayerPtr0; //?
  LPVOID UnkPTR2;
  Object3doStruct *Object3do;
  int Order_Unknow ;
  short int UnitID;
  short int UnitInGameIndex;
  char data9[14];

  short Kills;
  char data17[50];
  PlayerStruct * Owner_PlayerPtr1; //?
  char data16[6];
  char HealthPerA;  //health in percent
  char HealthPerB;  //health in percent, changes slower (?)
  char data19[2];
  unsigned char RecentDamage;  //0xFA
  unsigned char Height;		//0xfb
  short int  OwnerIndex ;	//0xfc
  char data28;				//0xfe
  char myLos_PlayerID;		//0xff
  char data10[4];			//0x100
  float Nanoframe;			//0x104
  short Health;				//0x108
  char TerrainLevel[4];		//0x10a
  unsigned short cIsCloaked;//0x10e
  unsigned int UnitSelected;//0x110: and UnitSelectState.  
							// & 0x000c0000: hold pos; maneuvre; roam.  
							// & 0x10: unit selected
							// & 0x20: sth to do with nanoframe / build completion 
							// & 0x100: radar/los
							// & 0x200: sonar/los
							// & 0x1000000: UnitAlive
  char data11[4];
}; //0x118

struct Object3doStruct {
	short NumParts;
	char data1[2];
	int TimeVisible;
	char data2[4];
	UnitStruct *ThisUnit;
	LPVOID *UnkPTR1;
	LPVOID *UnkPTR2;
	char data3[6];
	PrimitiveStruct *BaseObject;
};

struct PrimitiveInfoStruct{
	char data1[28];
	char *Name;
};

struct PrimitiveStruct{
	PrimitiveInfoStruct *PrimitiveInfo;
	int XPos;
	int ZPos;
	int YPos;
	unsigned short XTurn;
	unsigned short ZTurn;
	unsigned short YTurn;
	char data3[18];
	struct {
	bool Visible: 1;
    bool unk1 : 7;
	} Visible;
	char data2[1];
	PrimitiveStruct *SiblingObject;
	PrimitiveStruct *ChildObject;
	PrimitiveStruct *ParrentObject;
}; //0x36


struct UnitOrdersStruct {
  void* functionPtrPtr;              // 0x00
  unsigned char COBHandler_index;    // 0x04
  unsigned char State;               // 0x05
  unsigned short unknow_1;           // 0x06
  unsigned short field_8;            // 0x08
  unsigned int unknow_0;             // 0x0A
  UnitStruct* Unit_ptr;              // 0x0E
  unsigned int field_12;             // 0x12
  UnitStruct* AttackTargat;          // 0x16
  unsigned int field_1A;             // 0x1A
  UnitOrdersStruct* ThisPTR;         // 0x1E
  Position_Dword Pos;                // 0x22
  char data3[4];                     // 0x2E
  short RemeberX;                    // 0x32
  short RemeberY;                    // 0x34
  unsigned int BuildUnitID;          // 0x36
  char data4[8];                     // 0x3A
  unsigned int Order_State;          // 0x42		// 0x40000: background order
  unsigned int StartTime;            // 0x46
  UnitOrdersStruct* NextOrder;       // 0x4A
  unsigned int mask;                 // 0x4E
  unsigned int Order_CallBack;       // 0x52
};


struct RadarPicStruct{
	int XSize;
	int YSize;
	int Unk1;
	LPVOID *PixelPTR;
};

struct ParticleSystemStruct{
	LPVOID DrawFunc; //4FD5F8 wake or smoke?; 4FD638 - Smoke1, 4FD618 - Smoke2; 4FD5B8, 4FD5A8 - Nanolath; 4FD5D8 - fire; //?
	char data1[8];
	int Type; //1 smoke, 2 wake, 6 nano, 7 fire
	SmokeGraphics* firstDraw;		//0 om denna partikel ej 鋜 aktiv (?)
	SmokeGraphics* lastDraw;			//rita alla fram till men inte denna ?
	LPVOID *Unk;				//? inte f鰎 sista ?
	char data2[48];
};//76

struct ParticleBase{
	char data[8];
	ParticleSystemStruct **particles;		
	char data2[8];
	ParticleSystemStruct **ParticlePTRArray;
	int SmokeParticleStructSize; //? 76
	int maxParticles; //? 1000
	int curParticles;			//antalet aktiva i arrayen men de 鋜 inte n鰀v鋘digtvis i ordning
};

struct SmokeGraphics{
	FXGafStruct* gaf;
	int XPos,ZPos,YPos;
	int unknown;
	int frame;
	int unknown2;
	int MoreSubs;  //0 ifall inga fler subparticles efter denna
};//0x20

/*struct SmokeListNode{
	SmokeParticleStruct* next;
	SmokeParticleStruct* me;
};*///?

typedef struct _OPENTAFILE 
{
	unsigned CFILE;
	_OPENTAFILE* Parent_ptr;
	unsigned int  *CHUNKSizes_Ptr;
	int Null;
	unsigned int  Chunk_Sizes;
	int field_14;
	char FilePath[256];
} OPENTAFILE, * POPENTAFILE;



typedef struct _Shatte
{
	int field_0;
	int field_4;
	int field_8;
	int field_C;
	int GafFramePtr;
	int field_14;
	int field_18;
	int Mask_B;
}Shatte, * PShatte;

typedef struct _Vertices
{
	int field_0;
	int field_4;
	int field_8;
}Vertices, * PVertices;

typedef struct _OFFSCREEN
{
	//_OFFSCREEN()
	//{ }

	//_OFFSCREEN(
	//	int width, int height, int pitch, LPVOID surface, int surfaceSize,
	//	int windowLeft, int windowTop, int windowRight, int windowBottom) :
	//	Width(width),
	//	Height(height),
	//	lPitch(pitch),
	//	lpSurface(surface),
	//	field_10(10000),
	//	field_14(0),
	//	field_18(0),
	//	field_1A(0),
	//	field_2C(1)
	//{
	//	ScreenRect.left = windowLeft;
	//	ScreenRect.top = windowTop;
	//	ScreenRect.right = windowRight;
	//	ScreenRect.bottom = windowBottom;
	//}

	//_OFFSCREEN(const DDSURFACEDESC& ddsd) :
	//	Width(ddsd.dwWidth),
	//	Height(ddsd.dwHeight),
	//	lPitch(ddsd.lPitch < ddsd.dwWidth ? ddsd.dwWidth : ddsd.lPitch),
	//	lpSurface(ddsd.lpSurface),
	//	field_10(10000),
	//	field_14(0),
	//	field_18(0),
	//	field_1A(0),
	//	field_2C(1)
	//{
	//	ScreenRect.left = 0;
	//	ScreenRect.top = 0;
	//	ScreenRect.right = ddsd.dwWidth - 1;
	//	ScreenRect.bottom = ddsd.dwHeight - 1;
	//}

	int  Width;
	int  Height ;
	int  lPitch  ;
	LPVOID  lpSurface ;
	unsigned int  field_10 ;
	unsigned int  field_14 ;
	unsigned  short field_18 ;
	unsigned short field_1A;
	RECT ScreenRect;
	unsigned int field_2C ;
} OFFSCREEN;

/*  105 */


/*  126 */

typedef struct _GUIMEMSTRUCT
{
	_GUIMEMSTRUCT * per_active;
	_GUI0IDControl * ControlsAry;
	unsigned int  OnCommand;
	int TAMainStructPtr;
	int field_10;
	int Active_b;
	int ActiveUp;
	int Update_proc;
	int field_20;
	int field_24;
	char field_28[19];
	int field_3B;
	char field_3F[2];
	char GUIName[16];
	char field_51[101];
	__int16 SubGUICount;
	int field_B8;
	int field_BC;
	int field_C0;
	char field_C4[153];
	char field_15D[2137];
	char HPIPath[256];
	char field_AB6[66660];
	char field_10F1A[61];
}GUIMEMSTRUCT, * LPGUIMEMSTRUCT;


/*  134 */

typedef struct _GUI1IDControl
{
	char id;
	char assoc;
	char name[16];
	char gap_12[1];
	__int16 xpos;
	__int16 ypos;
	__int16 width;
	__int16 height;
	int attribs;
	int colorf;
	int colorb;
	char texturenumber;
	char fontnumber;
	char active;
	char commonattribs;
	char gap_2B[8];
	char help[128];
	char gap_B3[1];
	__int16 gaffile;
	char text[128];
	char stages;
	char status_curnt;
	__int16 status_init;
	__int16 quickkey;
	int grayedout;
	__int16 field_140;
	__int16 field_142;
	int unk_proc;
	__int16 field_148;
	int TAMainPtr;
	char field_14E[13];
}GUI1IDControl;


/*  151 */

typedef struct _GUI0IDControl
{
	char id;
	char assoc;
	char name[16];
	char gap_12[1];
	__int16 xpos;
	__int16 ypos;
	__int16 width;
	__int16 height;
	int attribs;
	int colorf;
	int colorb;
	char texturenumber;
	char fontnumber;
	char active;
	char commonattribs;
	char field_2B[8];
	char help[128];
	char gap_B3[1];
	__int16 gaffile;
	__int16 totalgadgets;
	__int16 field_B8;
	__int16 field_BA;
	int OFFSCREEN_p;
	char field_C0[8];
	char field_C8;
	char major;
	char minor;
	char revision;
	char crdefault[16];
	char escdefault[16];
	char defaultfocus[16];
	char panel[16];
	char field_10C[42];
	__int16 field_136;
	__int16 status;
	__int16 field_13A;
	int field_13C;
	__int16 field_140;
	__int16 field_142;
	int unk_proc;
	__int16 field_148;
	int TAMainPtr;
	char field_14E[13];
}GUI0IDControl;


/*  152 */

typedef struct _GUI2IDControl
{
	char id;
	char assoc;
	char name[16];
	char gap_12[1];
	__int16 xpos;
	__int16 ypos;
	__int16 width;
	__int16 height;
	int attribs;
	int colorf;
	int colorb;
	char texturenumber;
	char fontnumber;
	char active;
	char commonattribs;
	char field_2B[8];
	char help[128];
	char field_B3[7];
	__int16 selected_i;
	char field_BC[22];
	int unk_3;
	int unk_0;
	int itemheight;
	int unk_1;
	char field_E2[98];
	int unk_proc;
	__int16 field_148;
	int TAMainPtr;
	char field_14E[13];
}GUI2IDControl;


/*  154 */
typedef void (__stdcall * _PosProc)(GUIInfo * , int);
typedef struct _GUI3_4IDControl
{
	char id;
	char assoc;
	char name[16];
	char gap_12[1];
	__int16 xpos;
	__int16 ypos;
	__int16 width;
	__int16 height;
	int attribs;
	int colorf;
	int colorb;
	char texturenumber;
	char fontnumber;
	char active;
	char commonattribs;
	char field_2B[8];
	char help[128];
	char field_B3[43];
	char field_DE[24];
	char text[64];
	__int16 range;
	char maxchars;
	char gap_139[3];
	int thick;
	__int16 knobpos;
	__int16 knobsize;
	_PosProc pos_proc;
	__int16 field_148;
	int TAMainPtr;
	char field_14E[13];
}GUI3_4IDControl;


/*  155 */

typedef struct _GUI5IDControl
{
	char id;
	char assoc;
	char name[16];
	char gap_12[1];
	__int16 xpos;
	__int16 ypos;
	__int16 width;
	__int16 height;
	int attribs;
	int colorf;
	int colorb;
	char texturenumber;
	char fontnumber;
	char active;
	char commonattribs;
	char field_2B[8];
	char help[128];
	char field_B3[67];
	char text[64];
	char link[16];
	char gap_146[1];
	char unk_0;
	char gap_148[2];
	int TAMainPtr;
	char field_14E[13];
}GUI5IDControl;


/*  156 */

typedef struct _GUI78IDControl
{
	char id;
	char assoc;
	char name[16];
	char gap_12[1];
	__int16 xpos;
	__int16 ypos;
	__int16 width;
	__int16 height;
	int attribs;
	int colorf;
	int colorb;
	char texturenumber;
	char fontnumber;
	char active;
	char commonattribs;
	char field_2B[8];
	char help[128];
	char field_B3[67];
	char filename[32];
	char field_116[52];
	int TAMainPtr;
	char field_14E[13];
}GUI78IDControl;


/*  157 */

typedef struct _GUI6IDControl
{
	char id;
	char assoc;
	char name[16];
	char gap_12[1];
	__int16 xpos;
	__int16 ypos;
	__int16 width;
	__int16 height;
	int attribs;
	int colorf;
	int colorb;
	char texturenumber;
	char fontnumber;
	char active;
	char commonattribs;
	char field_2B[8];
	char help[128];
	char field_B3[49];
	int hotornot;
	char field_E8[98];
	int TAMainPtr;
	char field_14E[13];
}GUI6IDControl;


/*  158 */

typedef struct _GUI9IDControl
{
	char id;
	char assoc;
	char name[16];
	char gap_12[1];
	__int16 xpos;
	__int16 ypos;
	__int16 width;
	__int16 height;
	int attribs;
	int colorf;
	int colorb;
	char texturenumber;
	char fontnumber;
	char active;
	char commonattribs;
	char field_2B[8];
	char help[128];
	char field_B3[67];
	int nuttin;
	char field_FA[80];
	int TAMainPtr;
	char field_14E[13];
}GUI9IDControl;


typedef struct RadarUnit_ 
{
	short int ID ;
	unsigned int x;
	unsigned int y;
}RadarUnit; 


typedef struct _TAProgramStruct 
{
	int HInstance;
	int CmdShow;
	int WindowsClassName;
	int WindowsName;
	int field_10;
	__int16 MenuResID;
	char field_16[2];
	WNDCLASS WndClass;
	HWND TAClass_Hwnd;
	int Screen_DIBSECTION;
	int WndMemHDC;
	int Palette_H;
	OFFSCREEN * DIB_OFFSCREEN;
	int BackMemHDC;
	char field_58[40];
	int field_80;
	int lpDD_DDraw;
	int lpDD_BackSurface;
	int lpDD_AttachedSurface;
	int field_90;
	int field_94;
	int field_98;
	int field_9C;
	RECT DDrawSurfaceRect;
	int field_B0;
	int lpDD_BackSurface_1;
	int field_B8;
	OFFSCREEN * CurrentOFFSCREEN;
	LPBYTE ALPHA_TABLE;
	LPBYTE SHADE_TABLE;
	LPBYTE LIGHT_TABLE;
	LPBYTE GRAY_TABLE;
	LPBYTE BLUE_TABLE;
	int ScreenWidth;
	int ScreenHeight;
	int NewOFFSCREEN_Notify;
	int minimized_b;
	int field_E4;
	int srandTick;
	int OrgMOUSETRAILS;
	char IsFullScreen_mask;
	char field_F1;
	int Max_InputBuffer;
	int InputBuffer[30];
	int Total_InputBuffer;
	int ReadCount_InputBuffer;
	char field_176[20];
	int MOUSE_EVENTS;
	int field_18E;
	int field_192;
	POINT CursorPos_Buf[3];
	char field_1AE;
	char field_1AF[3];
	_GAFFrame * Cursor;
	int CursorX;
	int CursorY;
	OFFSCREEN * SAVEMOUSE_1;
	OFFSCREEN *  SAVEMOUSE_2;
	OFFSCREEN *  SAVEMOUSE_3;
	int field_1CA;
	int field_1CE;
	int field_1D2;
	int MouseThreadRunning;
	int field_1DA;
	int field_1DE;
	int field_1E2;
	int field_1E6;
	int field_1EA;
	int field_1EE;
	int field_1F2;
	int field_1F6;
	int MainMenuWidth;
	int MainMenuHeight;
	__int16 IsFullScreen;
	unsigned char* fontHandle;	// fontHandle[0] == font height
	int fontFrontColour;
	int fontBackColour;
	int fontAlpha;
	char field_214;
	char field_215[111];
	int field_284;
	char field_288[908];
	int field_614;
	int HAPIFILE_array;
	int HAPIFILECount;
	int dwTotalPhys;
	int field_624;
	char TAPath[256];
	char field_728;
}TAProgramStruct;

typedef struct _MOUSEEVENT 
{
	unsigned int   X ;
	unsigned int  Y ;
	unsigned int  fwKeys ;
	unsigned int  PressTime_sec ;
	unsigned int  Msg  ;
	unsigned int  DblClick ;
}MOUSEEVENT ;



typedef struct _COBHandle
{
	const char* displayName;
	void* orderFunctionPointer;
	void* drawFunctionPointer;
	unsigned char COBScripMask;
	unsigned char field_d;
	unsigned char field_e;
	unsigned char field_f;
	unsigned char cursorIndex;
	unsigned char field_11;
	unsigned char field_12;
	unsigned char field_13;
	unsigned char field_14;
	const char* technicalName;
}COBHandle;

typedef struct _SerialBitArrayStruct {
	unsigned char* data;
	unsigned dword_ofs;
	unsigned bit_ofs;
} SerialBitArrayStruct;

typedef struct _PacketBuilderStruct {
	unsigned dword_count;
	unsigned bit_count;
	unsigned buffer_size;
	unsigned char* buffer_ptr;
	unsigned char initial_buffer[0x100];
} PacketBuilderStruct;

enum PlayerPropertyMask
{
	WATCH= 0x40,
	HUMANPLAYER= 0x80,
	PLAYERCHEATING= 0x2000,
	FIXEDSTARTPOS= 0x4000
};

enum PlayerType
{
	Player_none      = 0,
	Player_LocalHuman  = 1,
	Player_LocalAI   = 2,
	Player_RemoteHuman  = 3,
	Player_RemoteAI  = 4  //
};      

enum LOSTYPE
{
	 NOMAPPING        = 1,
	 Permanent        = 2,
	 LOSTYPE          = 4,
	 Updated         = 8
};

enum WeaponMask
{
	 cruise= 0x2000000,
	 commandfire= 0x4000000,
	 stockpile_mask   = 0x1000000,
	 targetable_mask  = 0x20000000,
	 interceptor_mask  = 0x40000000
};

enum SharedStates
{
	 IsHost           = 1,
	 SharedMetal      = 2,
	 SharedEnergy     = 4,
	 SharedLOS        = 8,
	 SharedMappings   = 0x20,
	 SharedRadar      = 0x40
};

enum UNITINFOMASK_0
{
	canmove          = 1,
	canfire          = 2,
	downloadable     = 0x20,
	builder          = 0x40,
	isairbase        = 0x200,
	canfly           = 0x800,
	canhover         = 0x1000,
	hidedamage		 = 0x4000,
	antiweapons      = 0x20000000
};
enum UNITINFOMASK_1
{
	 showplayername= 0x20000,
	 commander        = 0x40000
};
	

namespace ordertype
{
	enum ORDERTYPE
	{
		STOP             = 1,
		MOVE             = 2,
		ATTACK           = 3,
		BLAST            = 4,
		LOAD             = 5,
		UNLOAD           = 6,
		DEFEND           = 7,
		REPAIR           = 8,
		PATROL           = 9,
		RECLAIM          = 0xC,
		CAPTURE          = 0xD,
		BUILD         = 0xE
	};
}

namespace softwaredebugmode
{
	enum SOFTWAREDEBUGMODE
	{
		Drop             = 1,
		CheatsEnabled    = 2,
		SelectionBoxes   = 4,
		InvulnerableFeatures = 8,
		Noshake          = 0x10,
		Clock            = 0x40,
		Doubleshot       = 0x80,
		Halfshot         = 0x100,
		Radar            = 0x200,
		Shootall         = 0x400
	};
};

enum CURSORINDEX
{
	cursorattack = 1   ,
	cursorairstrike ,
	cursortoofar    ,
	cursorcapture   ,
	cursordefend    ,
	cursorrepair    ,
	cursorpatrol    ,
	cursorpickup    ,
	cursorteleport  ,
	cursorrevive    ,
	cursorreclamate ,
	cursorload      ,
	cursorunload    ,
	cursormove      ,
	cursorselect    ,
	cursorfindsite  ,
	cursorred       ,
	cursorgrn       ,
	cursornormal    ,
	cursorhourglass ,
};

enum MOUSESPOTSTATE
{
	IS_MOUSE_IN_GAME_UI = 2,
	 CIRCLESELECTING  = 8,
	 OK_TO_BUILD = 0x40	// or is it NOT_OK_TO_BUILD?
};
namespace gameingstate
{
	enum GAMINGTYPE
	{
		MENU,
		CAMPAIGN,
		SKIRMISH,
		MULTI,
		EXITGAME
	};
}

enum class FeatureMasks
{
	animating = 0x0002,
	animtrans = 0x0004,
	shadtrans = 0x0008,
	flamable = 0x0010,
	geothermal = 0x0020,
	blocking = 0x0040,
	reclaimable = 0x0080,
	autoreclaimable = 0x0100,
	indestructible = 0x0200,
	nodisplayinfo = 0x0400,
	nodrawundergray = 0x0800
};

#define PLAYERNUM (10)

#pragma pack()


#endif
