#include "../DrawnMoveGeometry.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <set>
using namespace DrawnMoveGeometry;
bool near(double a,double b) { return std::abs(a-b)<1e-8; }
void expect(Point p,double x,double y) { assert(near(p.x,x) && near(p.y,y)); }
int main() {
    assert(Resample({},10).empty());
    assert(Resample({{0,0}},0).empty());
    auto straight=Resample({{0,0},{120,0}},5);
    for (std::size_t i=0;i<straight.size();++i) expect(straight[i],i*30,0);
    expect(Resample({{0,0},{100,0}},1)[0],50,0);
    auto corner=Resample({{0,0},{60,0},{60,40}},6);
    expect(corner[0],0,0); expect(corner[1],20,0); expect(corner[3],60,0); expect(corner[4],60,20); expect(corner[5],60,40);
    auto duplicate=Resample({{3,7},{3,7},{3,7}},7);
    for (auto p:duplicate) expect(p,3,7);
    auto trailing=Resample({{0,0},{0,0},{100,0},{100,0}},3);
    expect(trailing[0],0,0); expect(trailing[1],50,0); expect(trailing[2],100,0);
    auto reversed=Resample({{120,0},{0,0}},5);
    for (std::size_t i=0;i<5;++i) expect(reversed[i],straight[4-i].x,0);
    assert(Assign({{0,0}},{}).empty());
    auto crossing=Assign({{100,0},{0,0}},{{0,100},{100,100}});
    assert(crossing[0]==1 && crossing[1]==0);
    // Regression: greedy nearest produces a very costly second assignment;
    // the pair-improvement pass must repair it.
    auto improved=Assign({{40,0},{0,0}},{{0,0},{100,0}});
    assert(improved[0]==1 && improved[1]==0);
    std::mt19937 rng(712);
    std::uniform_real_distribution<double> coord(0,65000);
    for (int n : {1,2,10,100,1500}) {
        std::vector<Point> units,slots;
        for(int i=0;i<n;++i) { units.push_back({coord(rng),coord(rng)}); slots.push_back({coord(rng),coord(rng)}); }
        auto assigned=Assign(units,slots);
        assert(assigned.size()==units.size());
        assert(std::set<std::size_t>(assigned.begin(),assigned.end()).size()==units.size());
        assert(assigned==Assign(units,slots));
        double total=0,identity=0;
        for(int i=0;i<n;++i) { assert(assigned[i]<slots.size()); total+=DistanceSquared(units[i],slots[assigned[i]]); identity+=DistanceSquared(units[i],slots[i]); }
        assert(total<=identity); // randomized realistic assignment beats slot order
    }
    std::cout << "Drawn move geometry tests passed\n";
}
