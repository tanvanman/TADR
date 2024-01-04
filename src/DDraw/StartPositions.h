#include <cinttypes>
#include <memory>
#include <random>
#include <vector>
#include <windows.h>


class SingleHook;

// the provider of start position should initialise this no later than when host clicks start
struct StartPositionsData
{
	std::uint32_t positionCount;
	char orderedPlayerNames[10][32];
};

// creates a shared memory mapping of an StartPositionsData instance with name "TADemo-ShartPositions"
// and hooks TA to enforce those start positions in multiplayer-fixed-positions
class StartPositions
{
public:
	// Create and Get the instance
	static StartPositions* GetInstance();
	~StartPositions();

	void InitStartPositions(int isActivePlayer[10], int startPositions[10], bool randomised);
	bool GetInitedStartPositions(int isActivePlayer[10], int startPositions[10]);
	StartPositionsData* GetSharedMemory();

private:
	StartPositions();
	void CreateSharedMemory();
	static void GetTeamsFromAlliances(int playerTeamNumbers[10], bool randomise);
	static int CountLargestTeamSize(const int playerTeamNumbers[10]);
	static void GetStartPositionsFromTeamNumbers(const int playerTeamNumbers[10], int isActivePlayer[10], int startPositions[10], bool randomise);
	static void GetStartPositionsFromSharedMemory(const StartPositionsData* sm, int isActivePlayer[10], int startPositions[10]);
	static int GetStartPositionsSequentialy(int isActivePlayer[10], int startPositions[10], bool randomise);

	static std::unique_ptr<StartPositions> m_instance;
	static std::default_random_engine m_rng;
	StartPositionsData* m_startPositionsShare;
	HANDLE m_hMemMap;
	std::vector<std::shared_ptr<SingleHook> > m_hooks;
};
