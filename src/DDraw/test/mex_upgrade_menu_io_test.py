#!/usr/bin/env python3
"""Exercise production human-menu discovery/cache with HPI I/O mocked.

All three menu lookup helpers are compiled verbatim. The runtime directory
lookup and the real bounded GUI parser are included.
"""

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


DDRAW = Path(__file__).resolve().parent.parent


def extract(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    tokens = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', re.S)
    depth = 0
    for token in tokens.finditer(source, opening):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return source[start:token.end()]
    raise RuntimeError("Unclosed production helper: " + signature)


HARNESS = r'''
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "MexUpgradeMenu.h"

struct UnitDefStruct {
    char UnitName[32] = {}; unsigned char GUINum = 0;
    unsigned UnitTypeMask_0 = 0x80000000u;
};
struct TAdynmemStruct {
    UnitDefStruct* UnitDef = nullptr; unsigned GameTime = 100;
    unsigned char desktopGUI[0xAB6] = {};
};
struct File {
    std::string bytes;
    unsigned reported = std::numeric_limits<unsigned>::max();
};
std::map<std::string, File> files;
std::set<void*> allocations;
std::vector<std::string> paths;
unsigned reads = 0, frees = 0;

std::string Normalized(std::string path) {
    for (char& c : path) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 'a' - 'A');
    return path;
}
char* readfile_HPI(char* path, unsigned* size) {
    ++reads; paths.push_back(path); *size = 0;
    const auto found = files.find(Normalized(path));
    if (found == files.end()) return nullptr;
    const auto& file = found->second;
    auto* data = static_cast<char*>(std::malloc(file.bytes.size() + 1));
    assert(data);
    std::memcpy(data, file.bytes.data(), file.bytes.size()); data[file.bytes.size()] = 0;
    assert(allocations.insert(data).second);
    *size = file.reported == std::numeric_limits<unsigned>::max()
        ? static_cast<unsigned>(file.bytes.size()) : file.reported;
    return data;
}
void freeTAMem(void* data) {
    assert(data && allocations.erase(data) == 1); ++frees; std::free(data);
}

// PRODUCTION_HELPERS

std::string Button(const char* name) {
    return std::string("[GADGET7]{[COMMON]{id=1;name=") + name + ";active=1;attribs=32;}}";
}
void Put(const std::string& path, const std::string& text,
    unsigned reported = std::numeric_limits<unsigned>::max()) {
    auto& file = files[Normalized(path)];file.bytes = text;file.reported = reported;
}
struct World {
    TAdynmemStruct ta;
    UnitDefStruct defs[2], otherDefs[2];
    World(unsigned pages = 4) {
        static unsigned sequence = 0;
        assert(allocations.empty()); files.clear(); paths.clear(); reads = frees = 0;
        Directory("guiE");
        std::snprintf(defs[0].UnitName,sizeof(defs[0].UnitName),"TEST%04u",++sequence);
        defs[0].GUINum = static_cast<unsigned char>(pages);
        ta.UnitDef = defs;
    }
    const UnitDefStruct& Con() const { return defs[0]; }
    void Directory(const std::string& name) {
        assert(name.size()<256);std::memset(ta.desktopGUI+0x9B6,0,256);
        std::memcpy(ta.desktopGUI+0x9B6,name.data(),name.size());
    }
    std::string Prefix() const {
        std::string path(BuildMenuDirectory(ta));
        if(path.back()!='\\' && path.back()!='/')path+='\\';
        return path+Con().UnitName;
    }
    void Page(unsigned number, const char* unit = "ARMMOHO") {
        Put(Prefix() + std::to_string(number) + ".GUI",Button(unit));
    }
    bool Has(const char* unit = "ARMMOHO") { return HasBuildMenuEntry(ta,Con(),unit); }
};

void TestNumberedAndFallbackPages() {
    World page3;
    page3.Page(1,"OTHER"); page3.Page(2,"OTHER"); page3.Page(3,"ARMUWMOHO");
    assert(page3.Has("ARMUWMOHO") && reads == 4 && frees == 3 && allocations.empty());
    assert(paths[0] == page3.Prefix()+"0.GUI" && paths.back() == page3.Prefix()+"3.GUI");
    const unsigned before = reads;
    assert(page3.Has("ARMUWMOHO") && reads == before);
    assert(!page3.Has("ARMMOHO") && reads == before+4 && frees == 6);
    const unsigned missing = reads;
    assert(!page3.Has("ARMMOHO") && reads == missing);

    World page0(1);page0.Page(0,"CORMOHO");
    assert(page0.Has("CORMOHO") && reads == 1 && frees == 1);
    assert(paths[0] == page0.Prefix()+"0.GUI");
    page0.defs[0].UnitTypeMask_0 = 0;
    assert(!page0.Has("CORMOHO") && reads == 1); // Page zero requires its native permission bit.
    page0.defs[0].UnitTypeMask_0 = 0x80000000u;
    assert(page0.Has("CORMOHO") && reads == 1); // The enabled answer has its own cache key.
    World noFallback(1);Put(noFallback.Prefix()+".GUI",Button("ARMMOHO"));
    assert(!noFallback.Has() && reads == 1 && frees == 0);
    assert(paths[0] == noFallback.Prefix()+"0.GUI");

    World firstMissing(3);
    firstMissing.Page(1,"OTHER"); firstMissing.Page(2,"OTHER"); firstMissing.Page(3);
    assert(!firstMissing.Has() && reads == 3 && frees == 2);
    World numberedWithoutZero(4);numberedWithoutZero.defs[0].UnitTypeMask_0 = 0;
    numberedWithoutZero.Page(1,"OTHER");numberedWithoutZero.Page(2,"OTHER");numberedWithoutZero.Page(3);
    assert(numberedWithoutZero.Has() && reads == 3 && paths.front() == numberedWithoutZero.Prefix()+"1.GUI");
    World slash(1);slash.Directory("GuiE\\");slash.Page(0);
    assert(slash.Has() && reads == 1 && paths[0] == std::string("GuiE\\")+slash.Con().UnitName+"0.GUI");
    World forward(1);forward.Directory("guiE/");forward.Page(0);
    assert(forward.Has() && reads == 1 && paths[0] == std::string("guiE/")+forward.Con().UnitName+"0.GUI");
}

void TestCacheInvalidation() {
    World world(2); world.Page(1);
    assert(world.Has() && reads == 2 && frees == 1); world.ta.GameTime = 101;
    assert(world.Has() && reads == 2);
    Put(world.Prefix()+"1.GUI",Button("OTHER"));
    assert(world.Has() && reads == 2);
    world.ta.GameTime = 1;
    assert(!world.Has() && reads == 4 && frees == 2);
    world.ta.UnitDef = world.otherDefs; world.Page(1);
    assert(world.Has() && reads == 6 && frees == 3);
    TAdynmemStruct another = world.ta;
    Put(world.Prefix()+"1.GUI",Button("OTHER"));
    assert(!HasBuildMenuEntry(another,world.Con(),"ARMMOHO") && reads == 8 && frees == 4);
    world.Page(1);assert(world.Has() && reads == 10 && frees == 5);
    const unsigned beforeDirectoryChange = reads;
    world.Directory("OtherGUI");
    assert(!world.Has() && reads == beforeDirectoryChange+2);
    world.Page(0);world.defs[0].GUINum = 1;
    assert(world.Has() && paths.back() == world.Prefix()+"0.GUI");
    assert(allocations.empty());
}

void TestBoundsAndBufferOwnership() {
    for (unsigned reported : {0u,1u,1024u*1024u+1u}) {
        World world(1);
        Put(world.Prefix()+"0.GUI","x",reported);
        assert(!world.Has() && reads == 1 && frees == 1 && allocations.empty());
    }
    World malformed(1);
    Put(malformed.Prefix()+"0.GUI","[GADGET1]{[COMMON]{name=ARMMOHO;id=1;attribs=32;}");
    assert(!malformed.Has() && reads == 1 && frees == 1 && allocations.empty());

    World absent(4);
    assert(!absent.Has() && reads == 4 && frees == 0 && allocations.empty());
    const auto before = reads;
    assert(!MenuFileContains(std::string(260,'x'),"ARMMOHO") && reads == before);
    assert(!MenuFileContains(std::string(259,'x'),"ARMMOHO") && reads == before+1);

    World invalid(1);
    assert(BuildMenuDirectory(invalid.ta) == reinterpret_cast<const char*>(invalid.ta.desktopGUI)+0x9B6);
    invalid.Directory("");assert(!invalid.Has() && reads == 0);
    std::memset(invalid.ta.desktopGUI+0x9B6,'x',256);
    assert(!invalid.Has() && reads == 0);
    invalid.Directory("guiE");invalid.defs[0].GUINum = 0;
    assert(!invalid.Has() && reads == 0);
    invalid.defs[0].GUINum = 1;invalid.defs[0].UnitName[0] = 0;
    assert(!invalid.Has() && reads == 0);

    World fullName(1);
    std::memset(fullName.defs[0].UnitName,'X',sizeof(fullName.defs[0].UnitName));
    const std::string path = std::string("guiE\\")+std::string(32,'X')+"0.GUI";
    Put(path,Button("ARMMOHO"));
    assert(fullName.Has() && paths.back() == path && frees == 1 && allocations.empty());
}

int main() {
    TestNumberedAndFallbackPages();
    TestCacheInvalidation();
    TestBoundsAndBufferOwnership();
    assert(allocations.empty());
    std::cout << "Extractor human-menu I/O and cache tests passed\n";
}
'''


def main():
    source = (DDRAW / "MexUpgrade.cpp").read_text()
    methods = "\n\n".join(extract(source, signature) for signature in (
        "const char* BuildMenuDirectory(", "bool MenuFileContains(", "bool HasBuildMenuEntry("))
    harness = HARNESS.replace("// PRODUCTION_HELPERS",methods)
    with tempfile.TemporaryDirectory(prefix="mex-upgrade-menu-io-") as temp:
        cpp = Path(temp) / "menu_io.cpp";cpp.write_text(harness)
        binary = Path(temp) / "menu_io"
        subprocess.run(shlex.split(os.environ.get("CXX","g++")) + [
            "-std=c++11","-Wall","-Wextra","-Werror","-g",
            "-fsanitize=address,undefined","-fno-omit-frame-pointer",
            "-I",str(DDRAW),str(cpp),"-o",str(binary),
        ],check=True)
        subprocess.run([str(binary)],check=True,
            # Native-buffer ownership is asserted explicitly above. LSan cannot
            # run under the desktop app's ptrace environment.
            env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0"})


if __name__ == "__main__":
    main()
