#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "config.h"
#include "LabBuildCycleMenu.h"

#if LAB_BUILD_CYCLES_ENABLE
#include "oddraw.h"
#include "tamem.h"
#include "tafunctions.h"
#include "iddrawsurface.h"
#include "Gaf.h"
#include "LabBuildCycleArt.h"
#include "LabBuildCycles.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace
{
    constexpr unsigned kBuildClick = 0x0041AA00u;
    constexpr unsigned kDrawButton = 0x004A5F40u;
    constexpr unsigned kRenderLayer = 0x004A81E0u;
    constexpr unsigned kPlaySound = 0x0047F1A0u;
    constexpr unsigned kLayerAllocation = 0x004AAA68u;
    constexpr unsigned kInlineEntries = 0x004AAA95u;
    constexpr std::size_t kEntriesOffset = 0x3fu;
    constexpr int kEntryCapacity = 200;
    constexpr char kButtonName[] = "TACYCLE";
    constexpr char kButtonText[] = "CYCLE OFF\0CYCLE ON\0";
    constexpr unsigned char kBuildClickBytes[] = {0x81,0xEC,0x34,0x01,0x00,0x00,0x57};
    constexpr unsigned char kDrawButtonBytes[] = {0x81,0xEC,0xD8,0x00,0x00,0x00,0x53,0x55};
    constexpr unsigned char kRenderLayerBytes[] = {0x8B,0x44,0x24,0x04,0x81,0xEC,0xC0,0x03};
    constexpr unsigned char kPlaySoundBytes[] = {0x8B,0x0D,0xE8,0x1D,0x51,0x00,0x53,0x56};
    constexpr unsigned char kLayerAllocationBytes[] = {0x68,0x57,0x0F,0x01,0x00};
    constexpr unsigned char kInlineEntriesBytes[] = {0x83,0xC5,0x3F};
    using NativeCommand = void (__stdcall*)(GUIInfo*);
    using NativeDraw = void (__stdcall*)(GUIInfo*,int);
    using NativeRender = int (__stdcall*)(GUIInfo*,unsigned);
    using NativeSound = void (__stdcall*)(char*,int);
    NativeCommand const OriginalCommand = reinterpret_cast<NativeCommand>(kBuildClick);
    NativeDraw const DrawButton = reinterpret_cast<NativeDraw>(kDrawButton);
    NativeRender const RenderLayer = reinterpret_cast<NativeRender>(kRenderLayer);
    NativeSound const PlayToggleSound = reinterpret_cast<NativeSound>(kPlaySound);
    bool g_installed = false;

    static_assert(sizeof(GUI0IDControl) == 0x15b, "Native gadget size changed");
    static_assert(sizeof(GUI1IDControl) == 0x15b, "Native button size changed");
    static_assert(offsetof(GUI1IDControl,text) == 0xb6, "Native button text moved");
    static_assert(offsetof(GUI1IDControl,status_curnt) == 0x137, "Native button stage moved");
    static_assert(offsetof(GUI1IDControl,GadgetArchive) == 0x2b, "Native archive address moved");
    static_assert(offsetof(GUI1IDControl,GadgetGaf) == 0x2f, "Native GAF address moved");

#pragma pack(push,1)
    struct ButtonSequence
    {
        unsigned short Frames;
        unsigned int Signature;
        unsigned short Padding;
        char Name[32];
        GAFENTRY PtrFrameAry[LabBuildCycleArt::Frames];
    };
#pragma pack(pop)

    struct ButtonArt
    {
        ButtonSequence Sequence = {};
        GAFFrame Frames[LabBuildCycleArt::Frames] = {};
        bool ready = false;
    };

    static_assert(offsetof(ButtonSequence,PtrFrameAry) == 0x28, "Native GAF frame table moved");
    static_assert(offsetof(GAFFrame,PtrFrameBits) == 0x10, "Native GAF pixels moved");
    ButtonArt g_armArt;
    ButtonArt g_coreArt;

    bool Matches(unsigned address, const unsigned char* bytes, std::size_t length)
    {
        unsigned char actual[8] = {};
        SIZE_T read = 0;
        return length <= sizeof(actual)
            && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),
                actual,length,&read) && read == length && std::memcmp(actual,bytes,length) == 0;
    }

    void __stdcall HandleCommand(GUIInfo* gui);

    std::uintptr_t OurHandler()
    {
        return reinterpret_cast<std::uintptr_t>(&HandleCommand);
    }

    bool Named(const GUI0IDControl& control, const char* name)
    {
        return std::strncmp(control.name,name,sizeof(control.name)) == 0;
    }

    bool Contains(const GUI0IDControl& control, const char* name)
    {
        char bounded[17] = {};
        std::memcpy(bounded,control.name,16);
        return std::strstr(bounded,name) != nullptr;
    }

    bool NativeLayer(const GUIMEMSTRUCT* layer)
    {
        if (!layer || !layer->ControlsAry) return false;
        // LoadGuiLayer reserves 200 inline gadgets after the packed 0x3f-byte
        // layer. Its allocation and entry-offset instructions are checked at
        // install time. Never extend another patch's separately allocated table.
        if (reinterpret_cast<std::uintptr_t>(layer->ControlsAry)
                != reinterpret_cast<std::uintptr_t>(layer)+kEntriesOffset) return false;
        const auto& root=layer->ControlsAry[0];
        return root.id == 0 && root.totalgadgets > 0 && root.totalgadgets < kEntryCapacity
            && root.xpos >= 0 && root.xpos < 128 && root.width > 0 && root.width <= 128
            && root.ypos >= 0 && root.height > 0;
    }

    int FindButton(const GUIMEMSTRUCT* layer)
    {
        const auto* entries=layer->ControlsAry;
        for (int i=1; i<=entries[0].totalgadgets; ++i)
            if (entries[i].id == 1 && Named(entries[i],kButtonName)) return i;
        return -1;
    }

    bool Overlaps(int x, int y, int width, int height, const GUI0IDControl& control)
    {
        return control.active && control.width > 0 && control.height > 0
            && x < control.xpos+control.width && control.xpos < x+width
            && y < control.ypos+control.height && control.ypos < y+height;
    }

    int BorrowedSlot(const GUIMEMSTRUCT* layer, int index)
    {
        const auto* entries=layer->ControlsAry;
        const auto* button=reinterpret_cast<const GUI1IDControl*>(&entries[index]);
        // Only this fresh button uses its otherwise unused type-1 user field
        // as a slot number. No native pointer or unit snapshot is retained.
        const int slot=button->TAMainPtr;
        if (slot <= 0 || slot >= index || slot > entries[0].totalgadgets) return -1;
        const auto& original=entries[slot];
        return original.id == 1 && Contains(original,"CLOAK")
            && original.xpos == button->xpos && original.ypos == button->ypos ? slot : -1;
    }

    void InitialiseArt(ButtonArt& art, unsigned width, const unsigned char* pixels)
    {
        if (art.ready) return;
        art.Sequence.Frames=LabBuildCycleArt::Frames;
        art.Sequence.Signature=1;
        std::memcpy(art.Sequence.Name,kButtonName,sizeof(kButtonName));
        for (unsigned i=0; i<LabBuildCycleArt::Frames; ++i)
        {
            auto& frame=art.Frames[i];
            frame.Width=static_cast<unsigned short>(width);
            frame.Height=LabBuildCycleArt::Height;
            frame.Background=LabBuildCycleArt::Transparent;
            frame.PtrFrameBits=const_cast<unsigned char*>(pixels+i*width*LabBuildCycleArt::Height);
            art.Sequence.PtrFrameAry[i].PtrFrame=&frame;
            art.Sequence.PtrFrameAry[i].Animated=10;
        }
        art.ready=true;
    }

    bool CoreMenu(const GUIMEMSTRUCT* layer)
    {
        const auto* entries=layer->ControlsAry;
        // A captured factory keeps its own faction's menu. Choose its artwork
        // from that menu, rather than from the local player's original side.
        for (int i=1; i<=entries[0].totalgadgets; ++i)
            if (entries[i].id == 1 && (Contains(entries[i],"FIREORD")
                    || Contains(entries[i],"MOVEORD") || Contains(entries[i],"CLOAK")))
            {
                if (std::strncmp(entries[i].name,"COR",3) == 0) return true;
                if (std::strncmp(entries[i].name,"ARM",3) == 0) return false;
            }
        return false;
    }

    bool SetButtonArt(GUIMEMSTRUCT* layer, int index)
    {
        auto* button=reinterpret_cast<GUI1IDControl*>(&layer->ControlsAry[index]);
        if (button->GadgetArchive || (button->gaffile & 1)) return false;
        ButtonArt& art=CoreMenu(layer) ? g_coreArt : g_armArt;
        if (&art == &g_coreArt)
            InitialiseArt(art,LabBuildCycleArt::CoreWidth,&LabBuildCycleArt::Core[0][0]);
        else InitialiseArt(art,LabBuildCycleArt::ArmWidth,&LabBuildCycleArt::Arm[0][0]);
        const auto address=reinterpret_cast<std::uintptr_t>(&art.Sequence);
        const int borrowed=BorrowedSlot(layer,index);
        const int width=borrowed > 0 ? layer->ControlsAry[borrowed].width : art.Frames[0].Width;
        const int height=borrowed > 0 ? layer->ControlsAry[borrowed].height : art.Frames[0].Height;
        const bool changed=static_cast<std::uintptr_t>(button->GadgetGaf) != address
            || button->colorf != 0 || (static_cast<unsigned short>(button->quickkey) & 0xff00u)
            || button->width != width || button->height != height;
        // Native cleanup owns only archive buffers, never this borrowed GAF
        // pointer. The sequence, raw frames and embedded pixels live in this
        // DLL until every modified menu has been restored during shutdown.
        button->GadgetGaf=static_cast<decltype(button->GadgetGaf)>(address);
        button->colorf=0; // DrawFrame, not DrawFrameLit: keep native indexed artwork intact.
        button->quickkey=static_cast<short>(static_cast<unsigned short>(button->quickkey) & 0xffu);
        // RenderLayer(force) sizes a fallback button to its temporary GAF.
        // Restore the borrowed native slot, or our own fixed artwork bounds.
        button->width=static_cast<short>(width);
        button->height=static_cast<short>(height);
        return changed;
    }

    int AddButton(TAdynmemStruct* ta, GUIMEMSTRUCT* layer)
    {
        auto* entries=layer->ControlsAry;
        auto& root=entries[0];
        if (root.totalgadgets >= kEntryCapacity-1) return -1;
        const GUI0IDControl* reference=nullptr;
        int bottom=0;
        int ordersTop=16384;
        // All existing visible controls retain their geometry and operation.
        // Prefer a row below them; compact menus can use an existing gap.
        // Stay inside the panel and above TA's bottom information strip.
        for (int i=1; i<=root.totalgadgets; ++i)
        {
            const auto& control=entries[i];
            if (!control.active || control.width <= 0 || control.height <= 0) continue;
            bottom=std::max(bottom,static_cast<int>(control.ypos)+control.height);
            if (control.id == 1 && (Contains(control,"FIREORD") || Contains(control,"MOVEORD")
                    || Contains(control,"ONOFF")))
            {
                reference=&control;
                ordersTop=std::min(ordersTop,static_cast<int>(control.ypos));
            }
        }
        if (!reference || ta->ScreenHeight > 16384u || ta->ScreenHeight <= 32u) return -1;
        // Some mod menus use two-column order buttons. The new toggle owns a
        // full-width row, rather than inheriting a half-width reference slot.
        int x=std::max(3,std::min(6,static_cast<int>(reference->xpos)));
        int width=CoreMenu(layer) ? LabBuildCycleArt::CoreWidth : LabBuildCycleArt::ArmWidth;
        int height=LabBuildCycleArt::Height;
        constexpr int artWidth=LabBuildCycleArt::CoreWidth;
        constexpr int artHeight=LabBuildCycleArt::Height;
        const int visible=std::min(static_cast<int>(root.height),
            static_cast<int>(ta->ScreenHeight)-32-static_cast<int>(root.ypos));
        if (x+artWidth > root.width || ordersTop < 0 || ordersTop+artHeight > visible)
            return -1;
        int y=bottom+2;
        int borrowed=-1;
        if (y+artHeight > visible)
        {
            // A compact menu can have a free row among the existing orders.
            // Find a genuine gap without moving, covering or removing a button.
            for (y=ordersTop; y+artHeight<=visible; ++y)
            {
                bool occupied=false;
                for (int i=1; i<=root.totalgadgets; ++i)
                    if (Overlaps(x,y,artWidth,artHeight,entries[i])) { occupied=true; break; }
                if (!occupied) break;
            }
            if (y+artHeight > visible)
            {
                // ESC/ProTA fill the order area but retain a greyed CLOAK
                // placeholder for ordinary factories. It has no usable action.
                // Keep that original gadget for native capability refresh, and
                // temporarily use only its unavailable space for the toggle.
                for (int i=1; i<=root.totalgadgets; ++i)
                {
                    const auto& control=entries[i];
                    const auto* original=reinterpret_cast<const GUI1IDControl*>(&control);
                    if (control.id == 1 && control.active && Contains(control,"CLOAK")
                        && (original->grayedout & 1)
                        && control.width >= 113 && control.width <= 114
                        && control.height >= 20 && control.height <= artHeight
                        && control.xpos >= 0 && control.ypos >= 0
                        && control.xpos+artWidth <= root.width
                        && control.ypos+artHeight <= visible)
                    {
                        bool blocked=false;
                        for (int j=1; j<=root.totalgadgets; ++j)
                            if (j != i && Overlaps(control.xpos,control.ypos,artWidth,
                                    artHeight,entries[j])) { blocked=true; break; }
                        if (blocked) continue;
                        borrowed=i;
                        x=control.xpos; y=control.ypos;
                        width=control.width; height=control.height;
                        break;
                    }
                }
                if (borrowed < 0) return -1;
            }
        }

        const int index=root.totalgadgets+1;
        auto* button=reinterpret_cast<GUI1IDControl*>(&entries[index]);
        std::memset(button,0,sizeof(*button));
        button->id=1;
        std::memcpy(button->name,kButtonName,sizeof(kButtonName));
        button->xpos=static_cast<short>(x);
        button->ypos=static_cast<short>(y);
        button->width=static_cast<short>(width);
        button->height=static_cast<short>(height);
        button->attribs=2; // native centered button text
        button->colorf=0;
        button->fontnumber=reference->fontnumber;
        button->active=1;
        button->stages=2;
        button->quickkey='C'; // Native hotkey; high byte (GAF frame base) stays zero.
        if (borrowed > 0)
        {
            button->TAMainPtr=borrowed;
            entries[borrowed].active=0;
        }
        std::memcpy(button->text,kButtonText,sizeof(kButtonText));
        // No archive or font ownership is copied. Bind a borrowed static GAF
        // face below; native DrawButton still supplies the labels and font.
        root.totalgadgets=static_cast<short>(index);
        SetButtonArt(layer,index);
        return index;
    }

    void MarkChanged(GUIInfo* gui)
    {
        gui->GUIUpdated_b=1;
        if (gui->TheActive_GUIMEM) gui->TheActive_GUIMEM->Active_b=1;
    }


    void UpdateImpl()
    {
        TAdynmemStruct* ta=*TAmainStruct_PtrPtr;
        if (!ta) return;
        GUIInfo* gui=&ta->desktopGUI;
        GUIMEMSTRUCT* layer=gui->TheActive_GUIMEM;
        if (!NativeLayer(layer)) return;
        const auto handler=static_cast<std::uintptr_t>(layer->OnCommand);
        if (handler != kBuildClick && handler != OurHandler()) return;
        bool enabled=false;
        bool supported=LabBuildCycles::SelectionState(enabled);
        int index=FindButton(layer);
        bool created=false;
        if (index < 0)
        {
            if (!supported || handler != kBuildClick) return;
            index=AddButton(ta,layer);
            if (index < 0) return;
            layer->OnCommand=static_cast<decltype(layer->OnCommand)>(OurHandler());
            created=true;
        }
        else if (handler != OurHandler()) return; // preserve another mod's TACYCLE gadget

        auto* button=reinterpret_cast<GUI1IDControl*>(&layer->ControlsAry[index]);
        const int borrowed=BorrowedSlot(layer,index);
        if (borrowed > 0)
        {
            auto& original=layer->ControlsAry[borrowed];
            const auto* originalButton=reinterpret_cast<const GUI1IDControl*>(&original);
            // A mod may give a factory a real cloak action. Restore it as soon
            // as native capability refresh makes it available.
            if (!(originalButton->grayedout & 1)) supported=false;
            original.active=supported ? 0 : 1;
        }
        const bool changed=button->active != supported
            || button->status_curnt != (enabled ? 1 : 0);
        button->active=supported ? 1 : 0;
        button->status_curnt=enabled ? 1 : 0;
        const bool artChanged=supported && SetButtonArt(layer,index);
        if (changed || created || artChanged)
        {
            if (changed || created) button->status_init=0;
            MarkChanged(gui);
            if (supported)
            {
                // A temporary fallback sprite can have larger bounds. Refresh
                // the native background to erase any pixels outside our face.
                if (artChanged && !created) RenderLayer(gui,0x40);
                DrawButton(gui,index);
            }
            else RenderLayer(gui,0x40);
        }
        // Full native page initialization can bind a generic fallback GAF.
        // Restore our fixed face before either map blits the cached menu;
        // ordinary input/pressed redraws already use the attached artwork.
    }

    void __stdcall HandleCommand(GUIInfo* gui)
    {
        // Native close notifications use index -1 and must pass through intact.
        bool cycleClick=false;
        __try
        {
            if (gui && NativeLayer(gui->TheActive_GUIMEM))
            {
                const auto* entries=gui->TheActive_GUIMEM->ControlsAry;
                const int index=gui->UIChange_f;
                cycleClick=index > 0 && index <= entries[0].totalgadgets
                    && entries[index].active && entries[index].id == 1
                    && Named(entries[index],kButtonName);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        if (cycleClick)
        {
            if (g_installed && LabBuildCycles::ToggleSelection())
                PlayToggleSound(const_cast<char*>("specialorders"),0);
            gui->UIChange_f=-1;
            LabBuildCycleMenu::Update();
            __try
            {
                const int index=FindButton(gui->TheActive_GUIMEM);
                if (index > 0 && gui->TheActive_GUIMEM->ControlsAry[index].active)
                {
                    reinterpret_cast<GUI1IDControl*>(&gui->TheActive_GUIMEM->ControlsAry[index])->status_init=0;
                    MarkChanged(gui);
                    DrawButton(gui,index);
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            return;
        }
        OriginalCommand(gui);
    }

    void RemoveButtons()
    {
        TAdynmemStruct* ta=*TAmainStruct_PtrPtr;
        if (!ta) return;
        GUIMEMSTRUCT* layer=ta->desktopGUI.TheActive_GUIMEM;
        bool removed=false;
        bool removedTop=false;
        for (int depth=0; layer && depth<32; ++depth)
        {
            if (NativeLayer(layer) && static_cast<std::uintptr_t>(layer->OnCommand) == OurHandler())
            {
                removed=true;
                if (layer == ta->desktopGUI.TheActive_GUIMEM) removedTop=true;
                layer->OnCommand=static_cast<decltype(layer->OnCommand)>(kBuildClick);
                const int index=FindButton(layer);
                if (index > 0)
                {
                    auto* entries=layer->ControlsAry;
                    const int borrowed=BorrowedSlot(layer,index);
                    if (borrowed > 0) entries[borrowed].active=1;
                    if (layer->field_20 == index) layer->field_20=-1;
                    GUIInfo& gui=ta->desktopGUI;
                    if (layer == gui.TheActive_GUIMEM)
                    {
                        if (gui.UIChange_f == index) gui.UIChange_f=-1;
                        if (gui.PressedGadgetIndex == index) gui.PressedGadgetIndex=-1;
                        if (gui.field_68 == index) gui.field_68=-1;
                        if (gui.field_6C == index) gui.field_6C=-1;
                    }
                    entries[index].active=0;
                    entries[index].GadgetGaf=0;
                    if (index == entries[0].totalgadgets)
                    {
                        std::memset(&entries[index],0,sizeof(entries[index]));
                        --entries[0].totalgadgets;
                    }
                }
                layer->Active_b=1;
            }
            layer=layer->per_active;
        }
        if (removed) ta->desktopGUI.GUIUpdated_b=1;
        if (removedTop)
            RenderLayer(&ta->desktopGUI,0x40);
    }
}

namespace LabBuildCycleMenu
{
    bool Install()
    {
        g_installed=Matches(kBuildClick,kBuildClickBytes,sizeof(kBuildClickBytes))
            && Matches(kDrawButton,kDrawButtonBytes,sizeof(kDrawButtonBytes))
            && Matches(kRenderLayer,kRenderLayerBytes,sizeof(kRenderLayerBytes))
            && Matches(kPlaySound,kPlaySoundBytes,sizeof(kPlaySoundBytes))
            && Matches(kLayerAllocation,kLayerAllocationBytes,sizeof(kLayerAllocationBytes))
            && Matches(kInlineEntries,kInlineEntriesBytes,sizeof(kInlineEntriesBytes));
        if (!g_installed)
            IDDrawSurface::OutptTxt("[LabBuildCycles] Native menu signature mismatch; build cycles disabled.");
        return g_installed;
    }

    void Update()
    {
        if (!g_installed) return;
        __try { UpdateImpl(); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void Shutdown()
    {
        // Failed preflight never gave us native GUI ownership. In particular,
        // do not call a renderer whose signature could not be verified.
        if (!g_installed) return;
        g_installed=false;
        __try { RemoveButtons(); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
}
#else
namespace LabBuildCycleMenu
{
    bool Install() { return false; }
    void Shutdown() {}
    void Update() {}
}
#endif
