#include "ConstructionKickout.h"

#include "dialog.h"
#include "iddrawsurface.h"
#include "hook/hook.h"
#include "tafunctions.h"

#include <cmath>
#include <set>

std::unique_ptr<ConstructionKickout> ConstructionKickout::m_instance = NULL;

unsigned int ActivateEnableBuildUnderOwnUnitsAddr = 0x04198c3;
BYTE ActivateEnableBuildUnderOwnUnitsBytes[] = { 0x01 };

unsigned int TargetAreaWasBlockedTimeoutAddr = 0x0403d05;
unsigned int TargetAreaWasBlockedTimeoutAddr_VTOL = 0x0414046;
BYTE TargetAreaWasBlockedTimeoutBytes[] = { 20 };

unsigned int TestGridSpot_Start_Addr = 0x47d2e0;
int __stdcall TestGridSpot_Start_Proc(PInlineX86StackBuffer X86StrackBuffer)
{
	// reset build square colour to green
	char byte = 6;
	WriteProcessMemory(GetCurrentProcess(), (void*)0x469e7f, &byte, 1, NULL);
	return 0;
}

unsigned int EnableBuildUnderOwnUnitsAddr = 0x47d554;
int __stdcall EnableBuildUnderOwnUnitsProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
	bool ENABLE_BUILD_UNDER_OWN_UNITS = 1 == *(int*)(X86StrackBuffer->Esp + 0x48);
	if (!ENABLE_BUILD_UNDER_OWN_UNITS || ta->PrepareOrder_Type != ordertype::BUILD)
	{
		return 0;
	}

	unsigned unitIndex = X86StrackBuffer->Ecx & 0xffff;
	UnitStruct* unit = &ta->BeginUnitsArray_p[unitIndex];
	if (ConstructionKickout::IsUnitControlledByPlayerOrCheats(unit))
	{
		// set build square colour to yellow
		char byte = 10;
		WriteProcessMemory(GetCurrentProcess(), (void*)0x469e7f, &byte, 1, NULL);
		// and allow user to build under the unit
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x47d567;
		return X86STRACKBUFFERCHANGE;
	}
	return 0;
}

unsigned int KickoutOnWaitingForTargetAreaToClearAddr = 0x403cd0;
unsigned int KickoutOnWaitingForTargetAreaToClearAddr_VTOL = 0x41400d;
int __stdcall KickoutOnWaitingForTargetAreaToClearProc(PInlineX86StackBuffer X86StrackBuffer)
{
	TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
	UnitOrdersStruct* orders = (UnitOrdersStruct*)X86StrackBuffer->Esi;
	UnitDefStruct* unitDef = (Position_Dword*)X86StrackBuffer->Ebx == &orders->Pos
		? (UnitDefStruct*)X86StrackBuffer->Ebp	// KickoutOnWaitingForTargetAreaToClearAddr
		: (UnitDefStruct*)X86StrackBuffer->Ebx;	// KickoutOnWaitingForTargetAreaToClearAddr_VTOL

	const int x0 = orders->Pos.X;
	const int y0 = orders->Pos.Y;
	const int z0 = orders->Pos.Z;
	std::vector<UnitStruct*> units = ConstructionKickout::GetUnitsAtBuildPos(x0, y0, z0, unitDef->FootX, unitDef->FootY);
	for (UnitStruct* unit : units)
	{
		if (unit &&
			ConstructionKickout::IsUnitControlledByPlayerOrCheats(unit) &&
			ConstructionKickout::ShouldKickout(unit, x0, y0, unitDef->FootX, unitDef->FootY))
		{
			int toX, toY;
			if (ConstructionKickout::GetInstance()->FindNearbyWalkablePosition(unit, x0, y0, z0, toX, toY, 24 * unitDef->FootX))
			{
				ConstructionKickout::GetInstance()->KickoutUnitTo(unit, toX, toY, unit->ZPos);
			}
		}
	}
	return 0;
}

ConstructionKickout::ConstructionKickout()
{
	m_hooks.push_back(std::make_unique<InlineSingleHook>(EnableBuildUnderOwnUnitsAddr, 5, INLINE_5BYTESLAGGERJMP, EnableBuildUnderOwnUnitsProc));
	m_hooks.push_back(std::make_unique<SingleHook>(ActivateEnableBuildUnderOwnUnitsAddr, sizeof(ActivateEnableBuildUnderOwnUnitsBytes), INLINE_UNPROTECTEVINMENT, ActivateEnableBuildUnderOwnUnitsBytes));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(KickoutOnWaitingForTargetAreaToClearAddr, 5, INLINE_5BYTESLAGGERJMP, KickoutOnWaitingForTargetAreaToClearProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(KickoutOnWaitingForTargetAreaToClearAddr_VTOL, 5, INLINE_5BYTESLAGGERJMP, KickoutOnWaitingForTargetAreaToClearProc));
	m_hooks.push_back(std::make_unique<SingleHook>(TargetAreaWasBlockedTimeoutAddr, sizeof(TargetAreaWasBlockedTimeoutBytes), INLINE_UNPROTECTEVINMENT, TargetAreaWasBlockedTimeoutBytes));
	m_hooks.push_back(std::make_unique<SingleHook>(TargetAreaWasBlockedTimeoutAddr_VTOL, sizeof(TargetAreaWasBlockedTimeoutBytes), INLINE_UNPROTECTEVINMENT, TargetAreaWasBlockedTimeoutBytes));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(TestGridSpot_Start_Addr, 5, INLINE_5BYTESLAGGERJMP, TestGridSpot_Start_Proc));
}

ConstructionKickout* ConstructionKickout::GetInstance()
{
	if (!m_instance) {
		m_instance.reset(new ConstructionKickout());
	}
	return m_instance.get();
}

bool ConstructionKickout::Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	__try
	{
		if (TAInGame == DataShare->TAProgress)
		{
			TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
			switch (Msg)
			{
			case WM_LBUTTONDOWN:
				if (IsKickoutOverrideKeyPressed())
				{
					//kickout override button is down
					KickOutUnitIndex = FindMouseUnit();
					if (KickOutUnitIndex > 0 && IsUnitControlledByPlayerOrCheats(&taPtr->BeginUnitsArray_p[KickOutUnitIndex]))
					{
						return true;
					}
					KickOutUnitIndex = 0;
				}
				break;

			case WM_LBUTTONUP:
				if (KickOutUnitIndex > 0)
				{
					UnitStruct* unit = &taPtr->BeginUnitsArray_p[KickOutUnitIndex];
					if (IsKickoutOverrideKeyPressed() && IsUnitControlledByPlayerOrCheats(unit))
					{
						int x = taPtr->MouseMapPos.X;
						int y = taPtr->MouseMapPos.Y;
						int z = taPtr->MouseMapPos.Z;
						KickoutUnitTo(unit, x, y, z);
						return true;
					}
				}
				KickOutUnitIndex = 0;
				break;

			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_CHAR:
			case WM_MOUSEMOVE:
			case WM_MOUSEWHEEL:
				break;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
	return false;
}

bool ConstructionKickout::IsKickoutOverrideKeyPressed()
{
	return GetAsyncKeyState(((Dialog*)LocalShare->Dialog)->GetClickSnapOverrideKey()) & 0x8000;
}

bool ConstructionKickout::IsUnitBeingKickedOut(UnitStruct* unit)
{
	ordertype::ORDERTYPE unitOrdersType = GetOrderType(unit->UnitOrders);
	if (unitOrdersType == ordertype::MOVE)
	{
		auto it = m_unitKickoutPositions.find(unit->UnitInGameIndex);
		if (it != m_unitKickoutPositions.end())
		{
			if (unit->UnitOrders->Pos.X == std::get<0>(it->second) &&
				unit->UnitOrders->Pos.Y == std::get<1>(it->second) &&
				unit->UnitOrders->Pos.Z == std::get<2>(it->second))
			{
				return true;
			}
			m_unitKickoutPositions.erase(it);
			return false;
		}
	}
	return false;
}

std::vector<UnitStruct*> ConstructionKickout::GetUnitsAtBuildPos(int x0, int y0, int z0, int footX, int footY)
{
	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;

	std::set<UnitStruct*> result;
	for (int dx = 0; dx < footX; ++dx)
	{
		for (int dy = 0; dy < footY; ++dy)
		{
			int x = x0 / 16 - footX / 2 + dx;
			int y = y0 / 16 - footY / 2 + dy;
			if (x >= 0 && x < taPtr->FeatureMapSizeX && y >= 0 && y < taPtr->FeatureMapSizeY)
			{
				const int idx = x + y * taPtr->FeatureMapSizeX;
				unsigned unitIndex = taPtr->FeatureMap[idx].occupyingUnitNumber;
				if (unitIndex > 0u && &taPtr->BeginUnitsArray_p[unitIndex] < taPtr->EndOfUnitsArray_p)
				{
					UnitStruct* unit = &taPtr->BeginUnitsArray_p[unitIndex];
					result.insert(unit);
				}
			}
		}
	}
	return std::vector<UnitStruct*>(result.begin(), result.end());
}

bool ConstructionKickout::IsUnitControlledByPlayerOrCheats(UnitStruct* unit)
{
	if (!unit->IsUnit)
	{
		return false;
	}

	TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
	bool isOwnUnit = unit->Owner_PlayerPtr0->PlayerAryIndex == taPtr->LocalHumanPlayer_PlayerID;
	bool isCheatsEnabled = taPtr->SoftwareDebugMode & softwaredebugmode::CheatsEnabled;
	return isOwnUnit || isCheatsEnabled;
}

ordertype::ORDERTYPE ConstructionKickout::GetOrderType(UnitOrdersStruct* orders)
{
	if (orders)
	{
		typedef char* (__stdcall* _ScriptAction_Type2Index)(unsigned* RtnIndex_ptr, unsigned Action_ID, UnitStruct* OrderUnit, UnitStruct* TargetUnittr, Position_Dword* Position);
		_ScriptAction_Type2Index ScriptAction_Type2Index = (_ScriptAction_Type2Index)0x043f0e0;

		for (ordertype::ORDERTYPE testOrderType : {
			ordertype::MOVE, ordertype::BUILD, ordertype::DEFEND,
				ordertype::ATTACK, ordertype::BLAST, ordertype::CAPTURE,
				ordertype::LOAD, ordertype::PATROL, ordertype::RECLAIM,
				ordertype::REPAIR, ordertype::STOP, ordertype::UNLOAD
		})
		{
			unsigned index = -1;
			Position_Dword pos;
			//std::memcpy(&pos, &orders->Pos, sizeof(pos));
			std::memset(&pos, 0, sizeof(pos));
			ScriptAction_Type2Index(&index, testOrderType, orders->Unit_ptr, NULL, &pos);
			if ((unsigned char)index == orders->COBHandler_index)
			{
				return testOrderType;
			}
		}
	}
	return ordertype::STOP;
}

bool ConstructionKickout::ShouldKickout(UnitStruct* unit, int x, int y, int footX, int footY)
{
	UnitOrdersStruct* oldOrders = unit->UnitOrders;

	ordertype::ORDERTYPE oldOrderType = GetOrderType(oldOrders);
	if (oldOrderType == ordertype::MOVE)
	{
		int orderGridX = oldOrders->Pos.X / 16;
		int orderGridY = oldOrders->Pos.Y / 16;
		int kickoutGridX = x / 16 - footX / 2;
		int kickoutGridY = y / 16 - footY / 2;
		return
			orderGridX >= kickoutGridX && orderGridX < kickoutGridX + footX &&
			orderGridY >= kickoutGridY && orderGridY < kickoutGridY + footY;
	}
	else if (oldOrders && oldOrders->AttackTargat)
	{
		TAdynmemStruct* taPtr = *(TAdynmemStruct**)0x00511de8;
		double buildCostEnergy = taPtr->UnitDef[oldOrders->AttackTargat->UnitID].buildcostenergy;
		double energyInvested = buildCostEnergy * (1.0 - oldOrders->AttackTargat->Nanoframe);
		if (oldOrders->AttackTargat->Nanoframe > 0.0 && energyInvested < 600.0) // 20 sec before it evaporates
		{
			for (int idx = 0; idx < taPtr->MaxUnitNumberPerPlayer; ++idx)
			{
				UnitStruct* u = &unit->Owner_PlayerPtr0->Units[idx];
				if (u != unit && u->IsUnit)
				{
					if (u->UnitOrders->AttackTargat == unit->UnitOrders->AttackTargat && u->UnitOrders->State >= 2)
					{
						// only kickout if someone else is also building the target.  otherwise target might evaporate!
						return true;
					}
				}
			}
			return false;
		}
	}
	return true;
}

static bool QuadraticSolve(double a, double b, double c, double& x1, double& x2)
{
	double det = b * b - 4.0 * a * c;
	if (det < 0.0)
	{
		return false;
	}
	else
	{
		x1 = (-b + std::sqrt(det)) / a / 2.0;
		x2 = (-b - std::sqrt(det)) / a / 2.0;
		return true;
	}
}

static bool CircleLineSolve(double xc, double yc, double R, double xl, double yl, double thetal, double& x, double& y)
{
	double tanTh = std::tan(thetal);
	if (std::fabs(tanTh) <= 1.0)
	{
		double u = tanTh;
		double v = yl - yc - xl * tanTh;
		double x1, x2;
		bool ok = QuadraticSolve(u * u + 1.0, 2.0 * (u * v - xc), v * v + xc * xc - R * R, x1, x2);
		if (ok)
		{
			for (double xTest : {x1, x2})
			{
				x = xTest;
				y = yl + (x - xl) * tanTh;
				double dx = x - xl;
				double dy = y - yl;
				if (dx * std::cos(thetal) + dy * std::sin(thetal) >= 0.0)
				{
					return true;
				}
			}
		}
	}
	else
	{
		double u = 1.0 / tanTh;
		double v = xl - xc - yl / tanTh;
		double y1, y2;
		bool ok = QuadraticSolve(u * u + 1.0, 2.0 * (u * v - yc), v * v + yc * yc - R * R, y1, y2);
		if (ok)
		{
			for (double yTest : {y1, y2})
			{
				y = yTest;
				x = xl + (y - yl) / tanTh;
				double dx = x - xl;
				double dy = y - yl;
				if (dx * std::cos(thetal) + dy * std::sin(thetal) >= 0.0)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool ConstructionKickout::FindNearbyWalkablePosition(UnitStruct* unit, int x0, int y0, int z0, int& x, int& y, int nominalRange)
{
	static const double Pi = 3.141592654;
	static const double PiOn4 = 0.785398163;

	TAdynmemStruct* ta = *(TAdynmemStruct**)0x00511de8;
	ordertype::ORDERTYPE unitOrdersType = GetOrderType(unit->UnitOrders);

	x = unit->XPos;
	y = unit->YPos;

	int dx = x - x0;
	int dy = y - y0;
	double direction = double(std::rand() % 360) / 57.0;
	if (IsUnitBeingKickedOut(unit))
	{
		// random direction
	}
	else if (unit->UnitOrders && unit->UnitOrders->AttackTargat)
	{
		// +/-45deg to the side of attack target
		int dxTarget = unit->UnitOrders->AttackTargat->XPos - x;
		int dyTarget = unit->UnitOrders->AttackTargat->YPos - y;
		int dxKickout = x0 - x;
		int dyKickout = y0 - y;
		direction = std::atan2(dyTarget, dxTarget);
		double dot1 = std::cos(direction + PiOn4) * dxKickout + std::sin(direction + PiOn4) * dyKickout;
		double dot2 = std::cos(direction - PiOn4) * dxKickout + std::sin(direction - PiOn4) * dyKickout;
		direction = dot1 < dot2
			? direction + PiOn4
			: direction - PiOn4;

		double destX, destY;
		if (CircleLineSolve(x0, y0, nominalRange, x, y, direction, destX, destY))
		{
			if (destX > 16 && destX < ta->MapSizeX && destY > 16 && destY < ta->MapSizeY)
			{
				direction = std::atan2(destY - y0, destX - x0);
			}
		}
	}
	else if (dy * dy + dx * dx > 0)
	{
		// move away from build square
		direction = std::atan2(dy, dx);
  	}

	for (int _R = nominalRange; _R < 2 * nominalRange; _R += 16)
	{
		double R(_R);
		for (double th = 0.0; th < Pi; th += 16.0 / R)
		{
			for (int directionSign = -1; directionSign <= 1; directionSign += 2)
			{
				double _th = direction + double(directionSign) * th;
				x = x0 + int(R * std::cos(_th));
				y = y0 + int(R * std::sin(_th));
				if (x >= 16 && x < ta->MapSizeX && y >= 16 && y < ta->MapSizeY)
				{
					int xGrid = x / 16;
					int yGrid = y / 16;
					if (xGrid > 0 && xGrid + 1 < ta->FeatureMapSizeX &&
						yGrid > 0 && yGrid + 1 < ta->FeatureMapSizeY)
					{
						const int idx = xGrid + yGrid * ta->FeatureMapSizeX;
						unsigned unitIndex = ta->FeatureMap[idx].occupyingUnitNumber;
						if (unitIndex == 0)
						{
							FeatureStruct* f = &ta->FeatureMap[idx];
							unsigned short featureDefIndex = GetGridPosFeature(f);
							if (featureDefIndex >= ta->NumFeatureDefs || ta->FeatureDef[featureDefIndex].Height == 0)
							{
								int slope = ta->FeatureMap[idx].maxHeight2x2 - ta->FeatureMap[idx].minHeight2x2;
								if (slope < ta->UnitDef[unit->UnitID].maxslope)
								{
									return true;
								}
							}
						}
					}
				}
			}
		}
	}
	return false;
}

void ConstructionKickout::KickoutUnitTo(UnitStruct* unit, int x, int y, int z)
{
	UnitOrdersStruct* oldOrders = unit->UnitOrders;
	UnitOrdersStruct* nextOrders = unit->UnitOrders->NextOrder;
	ordertype::ORDERTYPE oldOrderType = GetOrderType(oldOrders);

	typedef int(__stdcall* _Order_Stop)(UnitStruct* CallerUnit, UnitOrdersStruct* OrderUnit_p, int LastState);
	_Order_Stop Order_Stop = (_Order_Stop)0x0401c20;

	if (oldOrderType == ordertype::BUILD && (!oldOrders->AttackTargat || oldOrders->AttackTargat->Nanoframe == 1.0))
	{
		oldOrders->State = 0;
		PushOrder(unit, x, y, z, NULL, ordertype::MOVE);
	}
	else if (oldOrders->Pos.X == 0 && oldOrders->Pos.Y == 0 && oldOrders->Pos.Z == 0)
	{
		SendOrder(unit, x, y, z, NULL, ordertype::MOVE, -1, false);
	}
	else if (IsUnitBeingKickedOut(unit))
	{
		oldOrders->NextOrder = NULL;
		SendOrder(unit, x, y, z, NULL, ordertype::MOVE, -1, false);
		unit->UnitOrders->NextOrder = nextOrders;
	}
	else
	{
		Order_Stop(unit, oldOrders, 0);

		// ditch old order, issue substitute order, append next orders, push new move order, 
		oldOrders->NextOrder = NULL;
		ordertype::ORDERTYPE replacementOrder = oldOrderType == ordertype::BUILD
			? ordertype::REPAIR
			: ordertype::ORDERTYPE(0);
		SendOrder(unit, oldOrders->Pos.X, oldOrders->Pos.Y, oldOrders->Pos.Z, oldOrders->AttackTargat, replacementOrder, oldOrders->COBHandler_index, false);
		unit->UnitOrders->NextOrder = nextOrders;
		PushOrder(unit, x, y, z, NULL, ordertype::MOVE);
	}
	m_unitKickoutPositions[unit->UnitInGameIndex] = std::make_tuple(
		unit->UnitOrders->Pos.X, unit->UnitOrders->Pos.Y, unit->UnitOrders->Pos.Z);
}
