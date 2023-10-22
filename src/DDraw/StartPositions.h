#include <cinttypes>
#include <memory>
#include <windows.h>

class SingleHook;

// the provider of start position should initialise this no later than when host clicks start
struct StartPositionsData
{
	std::uint32_t positionCount;
	char orderedPlayerNames[10][32];

	// initialised to 0 by provider, set to 1 by consumer as the positions are locked in
	std::uint32_t usedPositions[10];
};

// creates a shared memory mapping of an StartPositionsData instance with name "TADemo-ShartPositions"
// and hooks TA to enforce those start positions in multiplayer-fixed-positions
class StartPositions
{
public:
	// Create and Get the instance
	static StartPositions* GetInstance();
	~StartPositions();

	const void GetStartPositions(int isActivePlayer[10], int startPositions[10], bool randomised);

private:
	StartPositions();
	void CreateSharedMemory();
	StartPositionsData* GetSharedMemory();
	static void GetTeamsFromAlliances(int playerTeamNumbers[10], bool randomise);
	static int CountLargestTeamSize(const int playerTeamNumbers[10]);
	static void GetStartPositionsFromTeamNumbers(const int playerTeamNumbers[10], int isActivePlayer[10], int startPositions[10], bool randomise);
	static void GetStartPositionsFromSharedMemory(const StartPositionsData* sm, int isActivePlayer[10], int startPositions[10]);
	static int GetStartPositionsSequentialy(int isActivePlayer[10], int startPositions[10], bool randomise);

	static std::unique_ptr<StartPositions> m_instance;
	StartPositionsData* m_startPositionsShare;
	HANDLE m_hMemMap;
	std::unique_ptr<SingleHook> m_initialiseHook;
	std::unique_ptr<SingleHook> m_fixedPositionsHook;
	std::unique_ptr<SingleHook> m_randomPositionsHook;
	std::unique_ptr<SingleHook> m_dbg1Hook;
	std::unique_ptr<SingleHook> m_dbg2Hook;
	std::unique_ptr<SingleHook> m_teamBugfixHook;
};
