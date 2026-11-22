#pragma once

#include <memory>
#include "hook/hook.h"

// Makes the wreck of an aircraft killed over land fall to the ground instead of
// hanging in mid-air at the altitude it died.
//
// UNITS_CreateCorpse stores the dying unit's full Pos, airborne Y and all. Its water
// branch seeds a sink velocity; its land branch seeds nothing, and the wreck
// integrator at 0x424214 retires any node whose velocity is all-zero before
// integrating -- so a land wreck is never simulated.
//
// Hook: UNITS_CreateCorpse @ 0x486439, seed a downward velocity. Gravity does the rest.
class AirCorpseFall
{
public:
	static void Install();

private:
	AirCorpseFall();
	~AirCorpseFall();
	static AirCorpseFall* m_instance;
	std::unique_ptr<InlineSingleHook> m_placedHook;
	static int __stdcall PlacedRouter(PInlineX86StackBuffer pBuf);
};
