#!/usr/bin/env python3
"""Exercise the real native-menu adapter with mock native drawing/click bridges.

The packed 0x15b gadget size and relevant offsets are preserved. Host-width
layer callback pointers allow executing the real wrapper on Linux; retail
addresses, Windows ABI and rendered visuals remain build/game checks.
"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def adapted_source():
    text = (ROOT / 'LabBuildCycleMenu.cpp').read_text()
    text = '\n'.join(line for line in text.splitlines() if not line.startswith('#include'))
    for old, new in (
        ('NativeCommand const OriginalCommand = reinterpret_cast<NativeCommand>(kBuildClick);',
         'NativeCommand const OriginalCommand = &FakeOriginalCommand;'),
        ('NativeDraw const DrawButton = reinterpret_cast<NativeDraw>(kDrawButton);',
         'NativeDraw const DrawButton = &FakeDrawButton;'),
        ('NativeRender const RenderLayer = reinterpret_cast<NativeRender>(kRenderLayer);',
         'NativeRender const RenderLayer = &FakeRenderLayer;'),
        ('NativeSound const PlayToggleSound = reinterpret_cast<NativeSound>(kPlaySound);',
         'NativeSound const PlayToggleSound = &FakePlayToggleSound;'),
    ):
        assert text.count(old) == 1
        text = text.replace(old, new)
    text = text.replace('__try', 'try').replace('__except (EXCEPTION_EXECUTE_HANDLER)', 'catch (...)')
    assert '__finally' not in text and '__except' not in text
    return text


HARNESS = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "LabBuildCycleMenu.h"
#include "LabBuildCycleArt.h"
#define __stdcall
using SIZE_T=std::size_t;
#pragma pack(push,1)
#define GADGET_COMMON \
    char id,assoc,name[16],gap_12;short xpos,ypos,width,height; \
    int attribs,colorf,colorb;char texturenumber,fontnumber,active,commonattribs; \
    unsigned GadgetArchive,GadgetGaf;char help[128],gap_B3;short gaffile;
struct GUI0IDControl {
    GADGET_COMMON short totalgadgets;char toSurface[4];
    std::uintptr_t OFFSCREEN_p;char tail[0x15b-0xbc-sizeof(std::uintptr_t)];
};
struct GUI1IDControl {
    GADGET_COMMON char text[128];char stages,status_curnt;short status_init,quickkey;
    int grayedout;short field_140,field_142;int unk_proc;short field_148;int TAMainPtr;char tail[13];
};
#undef GADGET_COMMON
struct GUIMEMSTRUCT {
    GUIMEMSTRUCT* per_active=nullptr;GUI0IDControl* ControlsAry=nullptr;
    std::uintptr_t OnCommand=0;int Active_b=0,field_20=-1;
};
struct GUIInfo {
    GUIMEMSTRUCT* TheActive_GUIMEM=nullptr;
    int UIChange_f=-1,PressedGadgetIndex=-1,field_68=-1,field_6C=-1;
    char toColors[0x8b2-sizeof(void*)-4*sizeof(int)]={};
    unsigned char GuiColors[256]={};char toUpdated[792]={};int GUIUpdated_b=0;
};
#pragma pack(pop)
static_assert(offsetof(GUIInfo,GuiColors)==0x8b2,"native indexed GUI palette offset");
static_assert(offsetof(GUIInfo,GUIUpdated_b)==0xcca,"native GUI dirty offset");
static_assert(offsetof(GUI0IDControl,OFFSCREEN_p)==0xbc,"native root cached surface offset");
struct OFFSCREEN {int Width=0,Height=0,lPitch=0;void* lpSurface=nullptr;};
#pragma pack(push,1)
struct GAFFrame {
    unsigned short Width=0,Height=0,xPosition=0,yPosition=0;
    unsigned char Background=0,Compressed=0;unsigned short FramePointers=0;
    unsigned Unknown0=0;unsigned char* PtrFrameBits=nullptr;unsigned FPS=0;
};
struct GAFENTRY {GAFFrame* PtrFrame=nullptr;int Animated=0;};
struct GAFSequence {
    unsigned short Frames=0;unsigned Signature=0;unsigned short Padding=0;
    char Name[32]={};GAFENTRY PtrFrameAry[1];
};
#pragma pack(pop)
using PGAFSequence=GAFSequence*;using PGAFFrame=GAFFrame*;
struct PlayerInfo {unsigned char RaceSide=0;};
struct PlayerStruct {::PlayerInfo* PlayerInfo=nullptr;};
struct TAdynmemStruct {
    GUIInfo desktopGUI;unsigned ScreenHeight=768;int LocalHumanPlayer_PlayerID=1;
    PlayerStruct Players[10];
};
TAdynmemStruct* currentWorld=nullptr;
TAdynmemStruct** TAmainStruct_PtrPtr=&currentWorld;
bool selectionSupported=true,cycleEnabled=false,selectionThrows=false,toggleFails=false;
unsigned toggleCalls=0,originalCalls=0,drawCalls=0,renderCalls=0,readCalls=0;
unsigned badAddress=0,shortReadAddress=0;
int lastOriginalIndex=0,lastDrawIndex=0;
bool drawThrows=false;
std::array<unsigned char,256> observedPalette={};
int observedFont=-1;
GAFSequence* observedArt=nullptr;
unsigned observedFrame=0;
std::vector<unsigned char> observedImage;
std::vector<std::string> logs;
std::vector<std::string> sounds;
int PlaySound_Effect(char* name,int mode){assert(mode==0);sounds.emplace_back(name);return 1;}
void FakePlayToggleSound(char* name,int mode){assert(mode==0);sounds.emplace_back(name);}
struct IDDrawSurface {static void OutptTxt(const char* text){logs.emplace_back(text);}};
namespace LabBuildCycles {
bool SelectionState(bool& enabled){if(selectionThrows)throw std::runtime_error("selection read");enabled=cycleEnabled;return selectionSupported;}
bool ToggleSelection(){++toggleCalls;if(!selectionSupported||toggleFails)return false;cycleEnabled=!cycleEnabled;return true;}
}
void FakeOriginalCommand(GUIInfo* gui){++originalCalls;lastOriginalIndex=gui?gui->UIChange_f:-2;}
void FakeDrawButton(GUIInfo* gui,int index){
    ++drawCalls;lastDrawIndex=index;
    assert(gui&&gui->TheActive_GUIMEM&&index>0);
    auto* b=reinterpret_cast<GUI1IDControl*>(&gui->TheActive_GUIMEM->ControlsAry[index]);
    assert(b->id==1&&b->active);observedFont=b->fontnumber;
    gui->TheActive_GUIMEM->Active_b=1;
    std::copy(std::begin(gui->GuiColors),std::end(gui->GuiColors),observedPalette.begin());
    observedArt=reinterpret_cast<GAFSequence*>(static_cast<std::uintptr_t>(b->GadgetGaf));
    observedImage.clear();GAFFrame* image=nullptr;
    if(observedArt){
        assert(observedArt->Frames==4);observedFrame=static_cast<unsigned char>(b->status_curnt);
        const unsigned frameBase=static_cast<unsigned short>(b->quickkey)>>8;
        // Retail DrawButton's stage/press/disabled frame selection. This is
        // only the native drawing fixture; the feature's binding is unmodified.
        if(b->grayedout&1){
            if(b->attribs&0x100)observedFrame=observedArt->Frames-1;
            else if(!b->stages)observedFrame=frameBase;
        }else if(b->status_init&&observedArt->Frames>static_cast<unsigned char>(b->stages))
            observedFrame=b->stages?observedArt->Frames-2:frameBase+b->status_init;
        else if(!b->stages)observedFrame=frameBase;
        assert(observedFrame<observedArt->Frames);
        const auto* entry=reinterpret_cast<const GAFENTRY*>(reinterpret_cast<const unsigned char*>(observedArt)
            +offsetof(GAFSequence,PtrFrameAry)+observedFrame*sizeof(GAFENTRY));
        image=entry->PtrFrame;assert(image&&image->PtrFrameBits&&!image->Compressed&&!image->FramePointers);
        observedImage.assign(image->PtrFrameBits,image->PtrFrameBits+image->Width*image->Height);
        assert(b->colorf==0); // no DrawFrameLit remapping of the fixed texture
    }
    auto* surface=reinterpret_cast<OFFSCREEN*>(gui->TheActive_GUIMEM->ControlsAry[0].OFFSCREEN_p);
    if(surface&&surface->lpSurface&&surface->Width>0&&surface->Width<=4096
        &&surface->Height>0&&surface->Height<=16384&&surface->lPitch>=surface->Width
        &&surface->lPitch<=65536&&b->xpos>=0&&b->ypos>=0
        &&b->xpos+b->width<=surface->Width&&b->ypos+b->height<=surface->Height){
        auto* pixels=static_cast<unsigned char*>(surface->lpSurface);
        if(image){
            for(int y=0;y<image->Height;++y)for(int x=0;x<image->Width;++x){
                const int px=b->xpos+static_cast<short>(image->xPosition)+x;
                const int py=b->ypos+static_cast<short>(image->yPosition)+y;
                const auto color=image->PtrFrameBits[y*image->Width+x];
                if(px>=0&&py>=0&&px<surface->Width&&py<surface->Height&&color!=image->Background)
                    pixels[py*surface->lPitch+px]=color;
            }
        }else for(int y=b->ypos;y<b->ypos+b->height;++y)
            for(int x=b->xpos;x<b->xpos+b->width;++x)pixels[y*surface->lPitch+x]=gui->GuiColors[0x14];
    }
    if(drawThrows)throw std::runtime_error("native draw fault");
}
int FakeRenderLayer(GUIInfo*,unsigned flags){++renderCalls;assert(flags==0x40);return 1;}
bool ReadProcessMemory(void*,const void*,void*,SIZE_T,SIZE_T*);
void* GetCurrentProcess(){return nullptr;}

// PRODUCTION

bool ReadProcessMemory(void*,const void* pointer,void* output,SIZE_T length,SIZE_T* read){
    ++readCalls;const auto address=reinterpret_cast<std::uintptr_t>(pointer);
    const unsigned char* expected=nullptr;std::size_t size=0;
#if LAB_BUILD_CYCLES_ENABLE
    if(address==kBuildClick){expected=kBuildClickBytes;size=sizeof(kBuildClickBytes);}
    if(address==kDrawButton){expected=kDrawButtonBytes;size=sizeof(kDrawButtonBytes);}
    if(address==kRenderLayer){expected=kRenderLayerBytes;size=sizeof(kRenderLayerBytes);}
    if(address==kPlaySound){expected=kPlaySoundBytes;size=sizeof(kPlaySoundBytes);}
    if(address==kLayerAllocation){expected=kLayerAllocationBytes;size=sizeof(kLayerAllocationBytes);}
    if(address==kInlineEntries){expected=kInlineEntriesBytes;size=sizeof(kInlineEntriesBytes);}
#endif
    if(!expected||length!=size){*read=0;return false;}
    std::memcpy(output,expected,length);*read=length;
    if(address==badAddress)static_cast<unsigned char*>(output)[0]^=1;
    if(address==shortReadAddress)--*read;
    return true;
}

struct LayerFixture {
    // A real native layer has 200 inline 0x15b-byte gadgets at layer+0x3f.
    std::array<unsigned char,0x3f+200*0x15b+32> storage={};
    GUIMEMSTRUCT* layer;
    GUI0IDControl* entries;
    LayerFixture():layer(new(storage.data()) GUIMEMSTRUCT),entries(reinterpret_cast<GUI0IDControl*>(storage.data()+0x3f)){
        layer->ControlsAry=entries;layer->OnCommand=0x0041AA00u;
        entries[0].id=0;entries[0].width=128;entries[0].height=640;entries[0].ypos=128;
    }
    int Button(const char* name,int x,int y,int width,int height,bool gray=false){
        const int index=++entries[0].totalgadgets;assert(index<200);
        auto* b=reinterpret_cast<GUI1IDControl*>(entries+index);b->id=1;b->active=1;
        std::strncpy(b->name,name,16);b->xpos=static_cast<short>(x);b->ypos=static_cast<short>(y);
        b->width=static_cast<short>(width);b->height=static_cast<short>(height);
        b->colorf=83;b->fontnumber=2;b->grayedout=gray?1:0;return index;
    }
    std::vector<unsigned char> OriginalEntries(){
        auto* first=reinterpret_cast<unsigned char*>(entries+1);
        return {first,first+entries[0].totalgadgets*sizeof(GUI0IDControl)};
    }
    void Unchanged(const std::vector<unsigned char>& before){
        assert(std::memcmp(entries+1,before.data(),before.size())==0);
    }
    bool Guards(){for(unsigned i=0;i<32;++i)if(storage[storage.size()-32+i]!=0)return false;return true;}
};
struct Fixture {
    TAdynmemStruct ta;LayerFixture native;PlayerInfo info;
    OFFSCREEN cached;std::vector<unsigned char> pixels;
    Fixture(){currentWorld=nullptr;LabBuildCycleMenu::Shutdown();currentWorld=&ta;
        selectionSupported=true;cycleEnabled=false;selectionThrows=false;toggleFails=false;drawThrows=false;badAddress=shortReadAddress=0;
        ta.Players[1].PlayerInfo=&info;
        for(unsigned i=0;i<256;++i)ta.desktopGUI.GuiColors[i]=static_cast<unsigned char>(i^0x5b);
        pixels.assign(64+160*640+64,0xa5);cached={128,640,160,pixels.data()+64};
        native.entries[0].OFFSCREEN_p=reinterpret_cast<std::uintptr_t>(&cached);
        ta.desktopGUI.TheActive_GUIMEM=native.layer;assert(LabBuildCycleMenu::Install());}
    ~Fixture(){LabBuildCycleMenu::Shutdown();currentWorld=nullptr;}
    int Index(){
        for(int i=1;i<=native.entries[0].totalgadgets;++i)
            if(std::strncmp(native.entries[i].name,"TACYCLE",16)==0)return i;
        return -1;
    }
    GUI1IDControl* Cycle(){const int i=Index();return i<0?nullptr:reinterpret_cast<GUI1IDControl*>(native.entries+i);}
    void Click(int index){ta.desktopGUI.UIChange_f=index;reinterpret_cast<void(*)(GUIInfo*)>(native.layer->OnCommand)(&ta.desktopGUI);}
    void PaintReference(int index){
        const auto& b=native.entries[index];auto* data=static_cast<unsigned char*>(cached.lpSurface);
        for(int y=b.ypos;y<b.ypos+b.height;++y)for(int x=b.xpos;x<b.xpos+b.width;++x)data[y*cached.lPitch+x]=0x22;
        for(int x=b.xpos;x<b.xpos+b.width;++x){data[(b.ypos+1)*cached.lPitch+x]=0x2e;data[(b.ypos+b.height-2)*cached.lPitch+x]=0x0f;}
        // A few text/texture pixels in the sample strip must not replace its modal body colour.
        for(int x=b.xpos+3;x<b.xpos+6;++x)data[(b.ypos+b.height/2)*cached.lPitch+x]=0x77;
    }
    void FreeLayout(){native.Button("ARMFLASH",0,0,64,64);const int fire=native.Button("ARMFIREORD",6,400,55,20);PaintReference(fire);}
    int ActualLayout(bool cloakUsable=false,unsigned side=0,bool rendered=false){
        // Representative installed ESC/ProTA lab layout: build cards above
        // the native full-width CLOAK/FIRE/MOVE rows and ten order buttons.
        for(int row=0;row<6;++row)for(int col=0;col<2;++col)native.Button("ARMFLASH",col*64,row*64,64,64);
        native.Button("ARMPREV",0,384,64,24);native.Button("ARMNEXT",64,384,64,24);
        const int width=rendered&&side?114:113,height=rendered?21:20;
        const int cloak=native.Button("ARMCLOAK",6,409,width,height,!cloakUsable);
        const int fire=native.Button("ARMFIREORD",6,432,width,height);native.Button("ARMMOVEORD",6,455,width,height);PaintReference(fire);PaintReference(fire+1);
        if(side)for(int index=cloak;index<=fire+1;++index)std::memcpy(native.entries[index].name,"COR",3);
        for(int row=0;row<5;++row)for(int col=0;col<2;++col)native.Button("ARMORDER",6+col*58,479+row*32,55,31);
        return cloak;
    }
};

#if LAB_BUILD_CYCLES_ENABLE
void OrdinaryButtonsAndClicks(){
    Fixture f;f.FreeLayout();const auto before=f.native.OriginalEntries();const int count=f.native.entries[0].totalgadgets;
    LabBuildCycleMenu::Update();assert(f.Index()==count+1);f.native.Unchanged(before);
    auto* b=f.Cycle();assert(b&&b->id==1&&b->active&&b->stages==2&&b->status_curnt==0);
    assert(std::strcmp(b->text,"CYCLE OFF")==0&&std::strcmp(b->text+10,"CYCLE ON")==0);
    assert((static_cast<unsigned short>(b->quickkey)&0xffu)=='C'&&(static_cast<unsigned short>(b->quickkey)>>8)==0);
    assert(b->width>=90&&b->height>=16&&b->fontnumber==2&&b->colorf==0);
    assert(b->ypos>=420&&b->ypos+b->height<=608&&f.native.Guards());
    const auto drawn=drawCalls;LabBuildCycleMenu::Update();assert(drawCalls==drawn&&f.native.entries[0].totalgadgets==count+1);
    const auto toggles=toggleCalls,originals=originalCalls;
    const auto soundCount=sounds.size();
    f.Click(f.Index());assert(toggleCalls==toggles+1&&cycleEnabled&&b->status_curnt==1&&f.ta.desktopGUI.UIChange_f==-1);
    assert(sounds.size()==soundCount+1&&sounds.back()=="specialorders");
    f.Click(f.Index());assert(!cycleEnabled&&b->status_curnt==0&&originalCalls==originals);
    assert(sounds.size()==soundCount+2&&sounds.back()=="specialorders");
    toggleFails=true;f.Click(f.Index());toggleFails=false;assert(!cycleEnabled&&sounds.size()==soundCount+2);
    f.Click(1);assert(originalCalls==originals+1&&lastOriginalIndex==1);
    f.Click(-1);assert(originalCalls==originals+2&&lastOriginalIndex==-1);
    selectionSupported=false;const auto rendered=renderCalls;LabBuildCycleMenu::Update();
    assert(!b->active&&renderCalls>rendered);f.native.Unchanged(before);
    selectionSupported=true;cycleEnabled=true;LabBuildCycleMenu::Update();assert(b->active&&b->status_curnt==1);
    assert(sounds.size()==soundCount+2); // redraws and ordinary commands are silent here
    const int cycle=f.Index();f.native.layer->field_20=cycle;
    auto& gui=f.ta.desktopGUI;gui.UIChange_f=gui.PressedGadgetIndex=gui.field_68=gui.field_6C=cycle;
    LabBuildCycleMenu::Shutdown();assert(f.native.layer->OnCommand==0x0041AA00u);
    assert(f.native.entries[0].totalgadgets==count&&f.Index()==-1);
    assert(gui.UIChange_f==-1&&gui.PressedGadgetIndex==-1&&gui.field_68==-1&&gui.field_6C==-1&&f.native.layer->field_20==-1);
    f.native.Unchanged(before);assert(f.native.Guards());
}

void ActualFilledMenus(){
    Fixture f;const int cloak=f.ActualLayout();const auto before=f.native.OriginalEntries();const int count=f.native.entries[0].totalgadgets;
    LabBuildCycleMenu::Update();assert(f.Index()==count+1);auto* b=f.Cycle();assert(b&&b->active&&b->ypos==409);
    assert(!f.native.entries[cloak].active&&std::strcmp(f.native.entries[cloak].name,"ARMCLOAK")==0);
    for(int i=1;i<=count;++i)if(i!=cloak)
        assert(std::memcmp(f.native.entries+i,before.data()+(i-1)*sizeof(GUI0IDControl),sizeof(GUI0IDControl))==0);
    selectionSupported=false;LabBuildCycleMenu::Update();assert(!b->active&&f.native.entries[cloak].active);
    selectionSupported=true;LabBuildCycleMenu::Update();assert(b->active&&!f.native.entries[cloak].active);
    // Capability refresh makes cloak usable: it must immediately regain its
    // native position/operation, and the optional cycle gadget must yield.
    auto* nativeCloak=reinterpret_cast<GUI1IDControl*>(f.native.entries+cloak);nativeCloak->grayedout=0;
    LabBuildCycleMenu::Update();assert(nativeCloak->active&&!b->active);
    nativeCloak->grayedout=1;LabBuildCycleMenu::Update();assert(b->active&&!nativeCloak->active);
    LabBuildCycleMenu::Shutdown();assert(nativeCloak->active);f.native.Unchanged(before);
    Fixture usable;usable.ActualLayout(true);const auto untouched=usable.native.OriginalEntries();
    LabBuildCycleMenu::Update();assert(usable.Index()==-1);usable.native.Unchanged(untouched);
}

void GuardedMenusAndSignatures(){
    Fixture f;f.FreeLayout();const auto untouched=f.native.OriginalEntries();
    const auto guiBefore=f.ta.desktopGUI;const auto rendersBefore=renderCalls;
    LabBuildCycleMenu::Shutdown();assert(renderCalls==rendersBefore);
    assert(std::memcmp(&f.ta.desktopGUI,&guiBefore,sizeof(guiBefore))==0);f.native.Unchanged(untouched);
    assert(LabBuildCycleMenu::Install());
    f.native.layer->OnCommand=123;LabBuildCycleMenu::Update();assert(f.Index()==-1);f.native.Unchanged(untouched);
    f.native.layer->OnCommand=0x0041AA00u;
    GUI0IDControl outside[3]={};auto* saved=f.native.layer->ControlsAry;f.native.layer->ControlsAry=outside;
    LabBuildCycleMenu::Update();f.native.layer->ControlsAry=saved;assert(f.Index()==-1);
    f.native.entries[0].totalgadgets=199;LabBuildCycleMenu::Update();assert(f.Index()==-1);f.native.entries[0].totalgadgets=2;
    f.native.entries[0].width=80;LabBuildCycleMenu::Update();assert(f.Index()==-1);f.native.entries[0].width=128;
    selectionThrows=true;LabBuildCycleMenu::Update();assert(f.Index()==-1);selectionThrows=false;
    selectionSupported=false;LabBuildCycleMenu::Update();assert(f.Index()==-1);selectionSupported=true;
    f.native.Button("TACYCLE",6,430,116,20);const auto collision=f.native.OriginalEntries();
    LabBuildCycleMenu::Update();assert(f.native.layer->OnCommand==0x0041AA00u);f.native.Unchanged(collision);
    LabBuildCycleMenu::Shutdown();
    for(unsigned address:{kBuildClick,kDrawButton,kRenderLayer,kPlaySound,kLayerAllocation,kInlineEntries}){
        badAddress=address;assert(!LabBuildCycleMenu::Install());LabBuildCycleMenu::Update();
        assert(f.native.layer->OnCommand==0x0041AA00u);
        const auto nativeBefore=f.native.storage;
        const auto guiBefore=f.ta.desktopGUI;
        const auto renders=renderCalls,draws=drawCalls;
        LabBuildCycleMenu::Shutdown();LabBuildCycleMenu::Shutdown();
        assert(renderCalls==renders&&drawCalls==draws&&f.native.storage==nativeBefore);
        assert(std::memcmp(&f.ta.desktopGUI,&guiBefore,sizeof(guiBefore))==0);
    }
    badAddress=0;shortReadAddress=kInlineEntries;assert(!LabBuildCycleMenu::Install());shortReadAddress=0;
    assert(LabBuildCycleMenu::Install());
}
void PageAndSuffixCleanup(){
    Fixture f;f.FreeLayout();const int original=f.native.entries[0].totalgadgets;
    LabBuildCycleMenu::Update();const int firstCycle=f.Index();
    LayerFixture page;page.Button("CORFIREORD",7,432,113,20);const int secondOriginal=page.entries[0].totalgadgets;
    page.layer->per_active=f.native.layer;f.ta.desktopGUI.TheActive_GUIMEM=page.layer;
    LabBuildCycleMenu::Update();assert(page.entries[0].totalgadgets==secondOriginal+1);
    LabBuildCycleMenu::Shutdown();assert(page.layer->OnCommand==0x0041AA00u&&f.native.layer->OnCommand==0x0041AA00u);
    assert(page.entries[0].totalgadgets==secondOriginal&&f.native.entries[0].totalgadgets==original);
    const auto rendered=renderCalls;const auto guiBefore=f.ta.desktopGUI;LabBuildCycleMenu::Shutdown();
    assert(renderCalls==rendered&&std::memcmp(&f.ta.desktopGUI,&guiBefore,sizeof(guiBefore))==0);
    // Another native extension may add a suffix after our gadget. Do not
    // shift its indices or erase its allocation when removing our button.
    f.ta.desktopGUI.TheActive_GUIMEM=f.native.layer;assert(LabBuildCycleMenu::Install());LabBuildCycleMenu::Update();
    assert(f.Index()==firstCycle);const int suffix=f.native.Button("OTHERPATCH",6,500,116,20);
    const GUI0IDControl saved=f.native.entries[suffix];LabBuildCycleMenu::Shutdown();
    assert(f.native.entries[0].totalgadgets==suffix&&!f.native.entries[firstCycle].active);
    assert(std::memcmp(&saved,&f.native.entries[suffix],sizeof(saved))==0);
}
void FixedArtworkAndFeedback(){
    for(unsigned side=0;side<2;++side){
        Fixture f;f.info.RaceSide=static_cast<unsigned char>(1-side);f.FreeLayout();
        // Captured factories use their own menu artwork, rather than the
        // local player's original faction. Prefix and player side disagree.
        if(side)std::memcpy(f.native.entries[2].name,"COR",3);
        auto& gui=f.ta.desktopGUI;
        std::array<unsigned char,256> originalPalette;
        std::copy(std::begin(gui.GuiColors),std::end(gui.GuiColors),originalPalette.begin());
        const auto originalPixels=f.pixels;const auto soundCount=sounds.size();
        auto unchangedPalette=[&](){assert(std::memcmp(gui.GuiColors,originalPalette.data(),originalPalette.size())==0);};
        auto expectedFrame=[&](unsigned frame){
            assert(observedArt&&observedFrame==frame);
            assert(observedArt->Signature==1&&observedArt->Padding==0&&std::strcmp(observedArt->Name,"TACYCLE")==0);
            for(unsigned i=0;i<4;++i){
                const auto* entry=reinterpret_cast<const GAFENTRY*>(reinterpret_cast<const unsigned char*>(observedArt)
                    +offsetof(GAFSequence,PtrFrameAry)+i*sizeof(GAFENTRY));
                const auto* expectedPixels=side?LabBuildCycleArt::Core[i]:LabBuildCycleArt::Arm[i];
                const auto* image=entry->PtrFrame;
                assert(entry->Animated==10&&image&&image->Width==(side?LabBuildCycleArt::CoreWidth:LabBuildCycleArt::ArmWidth));
                assert(image->Height==LabBuildCycleArt::Height&&image->Background==LabBuildCycleArt::Transparent);
                assert(!image->Compressed&&!image->FramePointers&&!image->xPosition&&!image->yPosition);
                assert(std::memcmp(image->PtrFrameBits,expectedPixels,image->Width*image->Height)==0);
            }
            const auto* expected=side?LabBuildCycleArt::Core[frame]:LabBuildCycleArt::Arm[frame];
            const unsigned width=side?LabBuildCycleArt::CoreWidth:LabBuildCycleArt::ArmWidth;
            assert(observedImage==std::vector<unsigned char>(expected,expected+width*LabBuildCycleArt::Height));
            assert(observedFont==2&&f.Cycle()->fontnumber==2&&f.Cycle()->colorf==0);
            assert(f.Cycle()->GadgetArchive==0&&(f.Cycle()->gaffile&1)==0);
            unchangedPalette();
        };
        LabBuildCycleMenu::Update();expectedFrame(0);
        const int index=f.Index();auto* button=f.Cycle();const auto bound=button->GadgetGaf;
        const unsigned width=side?LabBuildCycleArt::CoreWidth:LabBuildCycleArt::ArmWidth;
        const unsigned height=LabBuildCycleArt::Height;
        assert(width>0&&height>0&&height<=32&&width<128);
        // Real production pixels must be copied from the fixed image, never
        // synthesized from another button's current pixels/palette.
        for(int y=0;y<640;++y)for(int x=0;x<160;++x){
            const std::size_t at=64+static_cast<std::size_t>(y)*160+x;
            const int rx=x-button->xpos,ry=y-button->ypos;
            unsigned char expected=originalPixels[at];
            if(rx>=0&&ry>=0&&static_cast<unsigned>(rx)<width&&static_cast<unsigned>(ry)<height){
                const auto color=side?LabBuildCycleArt::Core[0][ry*width+rx]:LabBuildCycleArt::Arm[0][ry*width+rx];
                if(color!=LabBuildCycleArt::Transparent)expected=color;
            }
            assert(f.pixels[at]==expected);
        }
        for(unsigned i=0;i<64;++i)assert(f.pixels[i]==0xa5&&f.pixels[f.pixels.size()-1-i]==0xa5);
        const auto beforeRedraw=drawCalls;LabBuildCycleMenu::Update();
        assert(button->GadgetGaf==bound&&drawCalls==beforeRedraw&&sounds.size()==soundCount);
        // Change every former sampling input, including Fire At Will's state.
        auto* fire=reinterpret_cast<GUI1IDControl*>(f.native.entries+2);
        fire->status_curnt=2;fire->status_init=1;fire->grayedout=1;
        for(int y=fire->ypos;y<fire->ypos+fire->height;++y)
            for(int x=fire->xpos;x<fire->xpos+fire->width;++x)
                static_cast<unsigned char*>(f.cached.lpSurface)[y*f.cached.lPitch+x]=0xee;
        LabBuildCycleMenu::Update();assert(button->GadgetGaf==bound&&sounds.size()==soundCount);
        FakeDrawButton(&gui,index);expectedFrame(0);
        // Mouse and native C hotkey dispatch both arrive at the same real
        // OnCommand wrapper; the native shortcut itself is recorded in quickkey.
        f.Click(index);assert(button->status_curnt==1);expectedFrame(1);
        assert(sounds.size()==soundCount+1&&sounds.back()=="specialorders");
        button->status_init=1;FakeDrawButton(&gui,index);expectedFrame(2);
        assert(sounds.size()==soundCount+1); // a press/redraw is not another order
        button->status_init=0;FakeDrawButton(&gui,index);expectedFrame(1);
        // A native forced page refresh can replace the art binding. The late
        // presentation update repairs exactly this button before the page blit.
        button->GadgetGaf=0;FakeDrawButton(&gui,index);assert(!observedArt);
        LabBuildCycleMenu::Update();assert(button->GadgetGaf==bound);expectedFrame(1);
        assert(sounds.size()==soundCount+1);
        button->status_init=1;button->GadgetGaf=0;LabBuildCycleMenu::Update();
        assert(button->status_init==1);expectedFrame(2);button->status_init=0;FakeDrawButton(&gui,index);expectedFrame(1);
        button->quickkey=static_cast<short>(0x7f00u|'C');LabBuildCycleMenu::Update();
        assert(button->quickkey=='C'&&sounds.size()==soundCount+1);expectedFrame(1);
        // Never replace a GAF that another extension marks archive-owned.
        button->GadgetArchive=0x12345678u;assert(!SetButtonArt(f.native.layer,index));
        assert(button->GadgetArchive==0x12345678u&&button->GadgetGaf==bound);button->GadgetArchive=0;
        button->gaffile|=1;assert(!SetButtonArt(f.native.layer,index));assert(button->GadgetGaf==bound);button->gaffile&=~1;
        toggleFails=true;f.Click(index);toggleFails=false;
        assert(button->status_curnt==1&&sounds.size()==soundCount+1);
        drawThrows=true;button->GadgetGaf=0;LabBuildCycleMenu::Update();drawThrows=false;unchangedPalette();
        selectionSupported=false;LabBuildCycleMenu::Update();assert(!button->active&&sounds.size()==soundCount+1);
        selectionSupported=true;LabBuildCycleMenu::Update();assert(button->active&&button->GadgetGaf==bound);
        f.Click(index);expectedFrame(0);assert(sounds.size()==soundCount+2);
        // Native unload frees only archives marked owned. The fixed image
        // binding is borrowed, so it can never free the DLL's static frames.
        assert(button->GadgetArchive==0&&(button->gaffile&1)==0);
        LabBuildCycleMenu::Shutdown();assert(button->GadgetGaf==0&&button->GadgetArchive==0);
        unchangedPalette();
    }
}
void RenderedGeometryAndForcedRefresh(){
    for(unsigned side=0;side<2;++side)for(bool rendered:{false,true}){
        Fixture f;const int cloak=f.ActualLayout(false,side,rendered);const auto before=f.native.OriginalEntries();
        const int count=f.native.entries[0].totalgadgets;LabBuildCycleMenu::Update();auto* button=f.Cycle();
        assert(button&&button->active&&button->TAMainPtr==cloak);
        assert(button->width==f.native.entries[cloak].width&&button->height==f.native.entries[cloak].height);
        const auto bound=button->GadgetGaf;const auto soundCount=sounds.size();
        button->GadgetGaf=0;button->width=40;button->height=14;button->status_init=1;
        const auto renderedBefore=renderCalls;
        LabBuildCycleMenu::Update();assert(button->GadgetGaf==bound&&button->status_init==1);
        assert(renderCalls==renderedBefore+1); // clear any larger stale fallback before redrawing the fixed face
        assert(button->width==f.native.entries[cloak].width&&button->height==f.native.entries[cloak].height);
        assert(observedFrame==2&&sounds.size()==soundCount);
        const int artWidth=side?LabBuildCycleArt::CoreWidth:LabBuildCycleArt::ArmWidth;
        for(int i=1;i<=count;++i)if(i!=cloak)
            assert(!Overlaps(button->xpos,button->ypos,artWidth,LabBuildCycleArt::Height,f.native.entries[i]));
        // A close may follow force-initialization before another presentation
        // update can repair the fallback's dimensions. Restore the hidden
        // native control even in that case.
        button->width=250;button->height=100;button->GadgetGaf=0;
        LabBuildCycleMenu::Shutdown();assert(f.native.entries[cloak].active);f.native.Unchanged(before);
    }
    // The first native full initialization can resize an original 20px GUI
    // control to its actual 21px GAF frame after Cycle was already inserted.
    Fixture f;const int cloak=f.ActualLayout();LabBuildCycleMenu::Update();auto* button=f.Cycle();
    assert(button&&button->height==20);f.native.entries[cloak].height=21;
    button->width=42;button->height=12;button->GadgetGaf=0;LabBuildCycleMenu::Update();
    assert(button->height==21&&button->width==f.native.entries[cloak].width);
    LabBuildCycleMenu::Shutdown();assert(f.native.entries[cloak].active);
}
int main(){OrdinaryButtonsAndClicks();ActualFilledMenus();GuardedMenusAndSignatures();PageAndSuffixCleanup();FixedArtworkAndFeedback();RenderedGeometryAndForcedRefresh();std::cout<<"Native lab-cycle menu: fixed production images, C shortcut, native press frames, sounds and guarded ownership passed\n";}
#else
int main(){
    const auto reads=readCalls,draws=drawCalls,renders=renderCalls;
    assert(!LabBuildCycleMenu::Install());LabBuildCycleMenu::Update();LabBuildCycleMenu::Shutdown();
    assert(readCalls==reads&&drawCalls==draws&&renderCalls==renders&&originalCalls==0&&toggleCalls==0);
    std::cout<<"Compiled-out lab-cycle menu leaves native UI untouched\n";
}
#endif
'''


def main():
    source = HARNESS.replace('// PRODUCTION', adapted_source())
    compiler = shlex.split(os.environ.get('CXX', 'g++'))
    with tempfile.TemporaryDirectory(prefix='lab-cycle-menu-') as directory:
        path = Path(directory)
        (path / 'test.cpp').write_text(source)
        for enabled, profile in ((1, 'escalation'), (1, 'prota'), (0, 'disabled')):
            executable = path / f'lab-menu-{profile}'
            definitions = ['-DTDRAW_CONFIG_PROTA=1'] if profile == 'prota' else []
            subprocess.run(compiler + ['-std=c++17', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-fno-pie', '-no-pie', '-g',
                f'-DLAB_BUILD_CYCLES_ENABLE={enabled}', '-I', str(ROOT),
                str(path / 'test.cpp'), '-o', str(executable)] + definitions, check=True)
            subprocess.run([str(executable)], check=True,
                env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))


if __name__ == '__main__':
    main()
