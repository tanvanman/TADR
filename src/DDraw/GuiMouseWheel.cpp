#include "GuiMouseWheel.h"
#include "oddraw.h"
#include "tamem.h"
#include "tafunctions.h"
#include "iddrawsurface.h"

#include <cstring>

// Native GUI scrolling, TAG_Venom. Use the same association/redraw path as
// TA's scrollbar and list auto-scroll; never inject clicks or navigation keys.
namespace
{
	const int RowsPerNotch = 3;
	static_assert(sizeof(GUI0IDControl) == 0x15B, "Native gadget stride");
	static_assert(offsetof(GUI2IDControl, selected_i) == 0xBA, "Native list selection");
	static_assert(offsetof(GUI2IDControl, itemheight) == 0xDA, "Native list row height");
	static_assert(offsetof(GUI3_4IDControl, knobpos) == 0x140, "Native slider position");
	static_assert(offsetof(GUI3_4IDControl, TAMainPtr) == 0x14A, "Native slider callback payload");
	static_assert(offsetof(TAProgramStruct, CursorPos_Buf) == 0x196, "Native logical mouse record");
	typedef void (__stdcall *GadgetProc)(GUIInfo*, int);
	const GadgetProc DrawList = reinterpret_cast<GadgetProc>(0x004A1B40);
	const GadgetProc Propagate = reinterpret_cast<GadgetProc>(0x004A2BE0);
	const GadgetProc SliderBack = reinterpret_cast<GadgetProc>(0x004A96D0);
	const GadgetProc SliderForward = reinterpret_cast<GadgetProc>(0x004A9780);

	template<class T> T& Field(void* record, unsigned offset)
	{
		return *reinterpret_cast<T*>(static_cast<BYTE*>(record) + offset);
	}

	struct Target
	{
		GUIMEMSTRUCT* page;
		GUI0IDControl* controls;
		int slider;
		int list;
	};

	GUIMEMSTRUCT* remainderPage = NULL;
	GUI0IDControl* remainderControls = NULL;
	int remainderSlider = 0;
	int wheelRemainder = 0;
	DWORD lastWheel = 0;
	char remainderName[16] = {};

	void Reset()
	{
		remainderPage = NULL;
		remainderControls = NULL;
		remainderSlider = 0;
		wheelRemainder = 0;
	}

	bool Contains(const GUI0IDControl& control, int x, int y)
	{
		return control.width > 0 && control.height > 0
			&& x >= control.xpos && x < int(control.xpos) + control.width
			&& y >= control.ypos && y < int(control.ypos) + control.height;
	}

	bool FindTarget(GUIInfo& gui, POINT mouse, Target& target)
	{
		GUIMEMSTRUCT* page = gui.TheActive_GUIMEM;
		if (!page || !page->ControlsAry || gui.field_64 != -1 || gui.field_78)
			return false; // Native mouse capture/drag takes precedence.

		GUI0IDControl* controls = page->ControlsAry;
		const int count = controls[0].totalgadgets;
		if (count < 1 || count >= 200 || !controls[0].OFFSCREEN_p
			|| !Contains(controls[0], mouse.x, mouse.y))
			return false;

		// Inspect only the top page. Native hover visits active records in order,
		// retaining the last hit. Non-scroll controls therefore also occlude a list.
		mouse.x -= controls[0].xpos;
		mouse.y -= controls[0].ypos;
		int hit = 0;
		for (int i = 1; i <= count; ++i)
			if (controls[i].active && Contains(controls[i], mouse.x, mouse.y))
				hit = i;
		if (!hit)
			return false;
		GUI0IDControl& hovered = controls[hit];
		if (hovered.id != 2 && hovered.id != 4
			&& !(hovered.id == 1 && (hovered.attribs & 0x1800)))
			return false;
		if (hovered.id == 1 && reinterpret_cast<GUI1IDControl&>(hovered).grayedout)
			return false;

		int slider = 0;
		int list = 0;
		for (int i = 1; i <= count; ++i)
		{
			if (controls[i].assoc != hovered.assoc)
				continue;
			if (controls[i].id == 4)
			{
				if (slider) return false; // Ambiguous association: leave it to TA.
				slider = i;
			}
			if (controls[i].id == 2)
			{
				if (list) return false;
				list = i;
			}
		}
		if (!slider)
			return false;
		GUI3_4IDControl& bar = reinterpret_cast<GUI3_4IDControl&>(controls[slider]);
		if (!bar.active || (bar.attribs & 0x10) || Field<int>(&bar, 0x157)
			|| bar.range <= 1)
			return false;

		if (list)
		{
			GUI0IDControl& rows = controls[list];
			// Text lists have a uniform item height. Picture lists can have
			// variable heights and even zero itemheight, which native scrolling
			// cannot propagate. Do not invent a different scroll model for them.
			if (!rows.active || !(rows.attribs & 0x10)
				|| Field<short>(&rows, 0xDA) <= 0 || Field<short>(&rows, 0xC0) <= 0)
				return false;
		}
		target = { page, controls, slider, list };
		return true;
	}

	void Scroll(GUIInfo& gui, const Target& target, int direction)
	{
		GUI3_4IDControl& bar = reinterpret_cast<GUI3_4IDControl&>(target.controls[target.slider]);
		if (!target.list)
		{
			// A value slider uses precisely TA's existing one-unit key handler.
			if ((direction < 0 && bar.knobpos > 0)
				|| (direction > 0 && bar.knobpos < bar.range - 1))
				(direction < 0 ? SliderBack : SliderForward)(&gui, target.slider);
			return;
		}

		GUI0IDControl& list = target.controls[target.list];
		short& top = Field<short>(&list, 0xBC);
		const int visible = list.height / Field<short>(&list, 0xDA);
		const int maximum = Field<short>(&list, 0xC0) - visible;
		if (visible <= 0 || maximum <= 0)
			return;
		int next = int(top) + direction * RowsPerNotch;
		if (next < 0) next = 0;
		if (next > maximum) next = maximum;
		if (next == top)
			return;

		// Scroll a small number of rows, not 120 knob pixels. This is the native list
		// auto-scroll redraw/association path (004A3E83), preserving selection.
		// List -> slider sync can legitimately put knobpos == range at the end;
		// retaining TA's own conversion also avoids coarse jumps on huge lists.
		top = static_cast<short>(next);
		DrawList(&gui, target.list);
		Propagate(&gui, target.list);
		if (bar.pos_proc)
			bar.pos_proc(&gui, bar.TAMainPtr); // Native callback payload at +14A.
	}
}

bool GuiMouseWheel::Message(HWND, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_MOUSEWHEEL)
	{
		// Fractional movement must not leak across controls, clicks or focus.
		if (message == WM_MOUSEMOVE && remainderPage)
		{
			TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
			POINT mouse = { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
			Target target;
			if (!ta || !FindTarget(ta->desktopGUI, mouse, target)
				|| target.page != remainderPage || target.controls != remainderControls
				|| target.slider != remainderSlider)
				Reset();
		}
		if (message == WM_KILLFOCUS || message == WM_ACTIVATEAPP
			|| message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN)
			Reset();
		return false;
	}

	if (!LocalShare || !LocalShare->CompatibleVersion
		|| GetCurrentThreadId() != LocalShare->GuiThreadId
		|| (LOWORD(wParam) & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2)))
	{
		Reset();
		return false;
	}

	TAdynmemStruct* ta = *TAmainStruct_PtrPtr;
	if (!ta)
	{
		Reset();
		return false;
	}

	// Use TA's current mouse record, already in logical GUI coordinates.
	// Wrappers can scale/letterbox and rewrite wheel lParam to client coords;
	// applying ScreenToClient again would misidentify the hovered control.
	TAProgramStruct* program = *TAProgramStruct_PtrPtr;
	if (!program)
	{
		Reset();
		return false;
	}
	POINT mouse = program->CursorPos_Buf[0];
	Target target;
	if (!FindTarget(ta->desktopGUI, mouse, target))
	{
		Reset();
		return false;
	}

	const DWORD now = GetTickCount();
	if (target.page != remainderPage || target.controls != remainderControls
		|| target.slider != remainderSlider || now - lastWheel > 1000
		|| std::memcmp(target.page->GUIName, remainderName, sizeof remainderName))
		wheelRemainder = 0;
	const int delta = static_cast<short>(HIWORD(wParam));
	remainderPage = target.page;
	remainderControls = target.controls;
	remainderSlider = target.slider;
	std::memcpy(remainderName, target.page->GUIName, sizeof remainderName);
	lastWheel = now;
	wheelRemainder += delta;
	const int notches = wheelRemainder / WHEEL_DELTA;
	wheelRemainder %= WHEEL_DELTA;
	for (int i = 0; i < (notches < 0 ? -notches : notches); ++i)
	{
		// A native callback may replace the page or its controls.
		Target current;
		if (*TAmainStruct_PtrPtr != ta || !FindTarget(ta->desktopGUI, mouse, current)
			|| current.page != target.page || current.controls != target.controls
			|| current.slider != target.slider || current.list != target.list)
		{
			Reset();
			break;
		}
		Scroll(ta->desktopGUI, current, notches > 0 ? -1 : 1);
	}
	return true;
}
