UnitStruct units[4];
UnitDef defs[4];
PlayerInfo info;
LPARAM xy(int x,int y) { return (unsigned(y)&65535)<<16 | (unsigned(x)&65535); }
bool event(UINT message,int x=300,int y=300,unsigned keys=0,bool start=true) {
    return DrawnMove::Message(reinterpret_cast<HWND>(1),message,keys,xy(x,y),start);
}
void reset(int interface=1) {
    event(WM_CANCELMODE); event(WM_LBUTTONUP); event(WM_RBUTTONUP);
    assert(!DrawnMove::OwnsInput());
    world=TAdynmemStruct(); share=Share(); orders.clear();
    world.InterfaceType=interface;
    world.Players[0].PlayerInfo=&info; info.PropertyMask=0;
    world.BeginUnitsArray_p=units;world.EndOfUnitsArray_p=units+4;
    world.Players[0].Units=units;world.Players[0].UnitsAry_End=units+3;
    for(int i=0;i<4;++i) {
        units[i]=UnitStruct();defs[i]=UnitDef();
        units[i].UnitType=&defs[i];units[i].Owner_PlayerPtr0=&world.Players[0];
        units[i].XPos=100+i*50;
    }
    units[3].UnitSelected &= ~UnitSelected_State;
}
void selected() {
    for(int i=0;i<3;++i) assert(units[i].UnitSelected & UnitSelected_State);
    assert(!(units[3].UnitSelected & UnitSelected_State));
}
int main() {
    reset();
    assert(event(WM_RBUTTONDOWN));assert(orders.empty());
    assert(event(WM_RBUTTONUP));assert(orders.size()==1 && orders[0].selected.size()==3);selected();
    // A curve creates individual, distinct destinations, while retaining a
    // selected building and unrelated simulation bits in the selection word.
    reset();defs[2].bmcode=0;defs[2].UnitTypeMask_0=0;
    assert(event(WM_RBUTTONDOWN,300,300, MK_SHIFT));
    event(WM_MOUSEMOVE,400,300);event(WM_MOUSEMOVE,400,400);
    FakeSurface surface; DrawnMove::Draw(&surface);
    assert(!surface.locked && std::count(surface.pixels.begin(),surface.pixels.end(),234)>0);
    event(WM_RBUTTONUP,400,400);
    assert(orders.size()==2);assert(orders[0].selected.size()==1 && orders[1].selected.size()==1);
    assert(orders[0].selected[0] != orders[1].selected[0]);
    assert(orders[0].x!=orders[1].x || orders[0].y!=orders[1].y);
    for(auto& order:orders) assert(order.shift && order.selected[0]!=2);
    selected();assert(units[0].UnitSelected & 0x100);
    // Both interfaces and explicit Move. A release with no intermediate move
    // event must still honour the drag threshold.
    reset(0);assert(!event(WM_RBUTTONDOWN));assert(event(WM_LBUTTONDOWN));
    event(WM_LBUTTONUP,500,300);assert(orders.size()==3);selected();
    reset();world.PrepareOrder_Type=ordertype::MOVE;
    assert(!event(WM_RBUTTONDOWN));assert(event(WM_LBUTTONDOWN,300,300,MK_SHIFT));
    event(WM_LBUTTONUP,500,300);assert(world.PrepareOrder_Type==ordertype::MOVE);
    reset();assert(!event(WM_LBUTTONDOWN));
    // No stealing attack/guard, selection, replay or spectator input.
    world.CurrentCursora_Index=1;assert(!event(WM_RBUTTONDOWN));
    world.CurrentCursora_Index=cursormove;world.MouseOverUnit=1;assert(!event(WM_RBUTTONDOWN));
    world.MouseOverUnit=0;share.PlayingDemo=true;assert(!event(WM_RBUTTONDOWN));share.PlayingDemo=false;
    info.PropertyMask=WATCH;assert(!event(WM_RBUTTONDOWN));info.PropertyMask=0;
    assert(!event(WM_RBUTTONDOWN,300,300,MK_CONTROL));
    assert(!event(WM_RBUTTONDOWN,300,300,0,false));
    for(UINT cancel : {WM_KILLFOCUS,WM_CANCELMODE,WM_CAPTURECHANGED,WM_KEYDOWN,WM_MOUSEWHEEL}) {
        reset();event(WM_RBUTTONDOWN);event(WM_MOUSEMOVE,500,300);
        event(cancel);event(WM_RBUTTONUP,500,300);assert(orders.empty());selected();
        assert(event(WM_RBUTTONDOWN));event(WM_RBUTTONUP);assert(orders.size()==1);
    }
    reset();event(WM_RBUTTONDOWN);event(WM_MOUSEMOVE,50,300);event(WM_RBUTTONUP);assert(orders.empty());
    reset();event(WM_RBUTTONDOWN);units[1].UnitID=0;event(WM_RBUTTONUP,500,300);assert(orders.empty());
    reset();event(WM_RBUTTONDOWN);units[3].UnitSelected |= UnitSelected_State;event(WM_RBUTTONUP,500,300);assert(orders.empty());
    reset();event(WM_RBUTTONDOWN);event(WM_LBUTTONDOWN);event(WM_RBUTTONUP);assert(event(WM_LBUTTONUP));assert(orders.empty());
    reset();event(WM_RBUTTONDOWN);event(WM_KILLFOCUS);
    // Mouse was released outside the window. A fresh click must recover.
    assert(event(WM_RBUTTONDOWN));event(WM_RBUTTONUP);assert(orders.size()==1);
    std::cout << "Drawn move input tests passed\n";
}
