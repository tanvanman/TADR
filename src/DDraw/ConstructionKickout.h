#pragma once

#include "tamem.h"
#include <map>
#include <memory>
#include <vector>

#ifdef min
#undef min
#undef max
#endif

class SingleHook;


class ConstructionKickout
{
public:
	ConstructionKickout();
	bool Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	static ConstructionKickout* GetInstance();

	bool IsKickoutOverrideKeyPressed();
	bool IsUnitBeingKickedOut(UnitStruct* unit);
	static std::vector<UnitStruct*> GetUnitsAtBuildPos(int x0, int y0, int z0, int footX, int footY);
	static bool IsUnitControlledByPlayerOrCheats(UnitStruct* unit);
	static ordertype::ORDERTYPE GetOrderType(UnitOrdersStruct* orders);
	static bool ShouldKickout(UnitStruct* unit, int x, int y, int footX, int footY);
	bool FindNearbyWalkablePosition(UnitStruct* unit, int x0, int y0, int z0, int& x, int& y, int nominalRange);
	void KickoutUnitTo(UnitStruct* unit, int x, int y, int z);

private:
	static std::unique_ptr<ConstructionKickout> m_instance;
	std::vector< std::unique_ptr<SingleHook> > m_hooks;

	int KickOutUnitIndex;
	std::map<int, std::tuple<unsigned, unsigned, unsigned> > m_unitKickoutPositions;

};
