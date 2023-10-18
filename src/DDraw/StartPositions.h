#include <cinttypes>
#include <memory>
#include <windows.h>

class SingleHook;

// the provider of start position should initialise this no later than when host clicks start
struct StartPositionsShare
{
	std::uint32_t positionCount;
	std::uint32_t orderedDirectplayIds[10];

	// initialised to 0 by provider, set to 1 by consumer as the positions are locked in
	std::uint32_t usedPositions[10];
};

// creates a shared memory mapping of an StartPositionsShare instance with name "TADemo-ShartPositions"
// and hooks TA to enforce those start positions in multiplayer-fixed-positions
class StartPositions
{
public:
	// Create and Get the instance
	static StartPositions* GetInstance();
	~StartPositions();

	StartPositionsShare* GetSharedMemory();

private:
	StartPositions();

	void CreateSharedMemory();

	static std::unique_ptr<StartPositions> m_instance;
	StartPositionsShare* m_startPositionsShare;
	HANDLE m_hMemMap;
	std::unique_ptr<SingleHook> m_hook;
};
