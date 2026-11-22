#pragma once

struct UnitStruct;

namespace RepairHealthSync
{
	void Install();
	void Shutdown();
	// Called by the shared nanoframe-render entry hook. Any temporary display
	// substitution is restored by this module's matching renderer-exit hooks.
	void BeginNanoframeDraw(UnitStruct* unit);
}
