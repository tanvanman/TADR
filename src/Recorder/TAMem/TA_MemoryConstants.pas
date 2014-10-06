unit TA_MemoryConstants;

interface

const
  MAXPLAYERCOUNT = 10;

  TAdynmemStructPtr = $511DE8;  //Boneyards 00525BE8
  TAMovementClassArray = $512358;
  TAunitsCategory = $51E6B0;
  COBScriptHandler_Begin = $512344;
  COBScriptHandler_End = $512348;

  TAdynmemStruct_SharedBits = $2A42; //asm
  TAdynmemStruct_LOS_Sight = $2A43; // Byte;  //asm
  TAdynmemStruct_LOS_Type = $14281; //asm

  PlayerStruct_PlayerType = $73; //asm
  PlayerStruct_Index = $146;  //asm
  PlayerStruct_AlliedPlayers = $108; //asm
  PlayerStruct_Units = $67;  //asm
  PlayerStruct_Units_End = $6B;  //asm

  Null_str = $005119B8;
  OFFSCREEN_off = -$1F0;

implementation

end.
