// Win32 native regression. Reads routines from the privately supplied,
// SHA-256-pinned retail EXE; neither its code nor assets are included here.
// No game/EXE entry point, DLL, import table or installation is executed.
#include <windows.h>
#include <wincrypt.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>
#include "../MapPreviewCopy.h"

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); std::exit(1); } } while (0)

struct Frame {
    unsigned short width, height, x, y;
    unsigned char key, compressed;
    unsigned short children;
    unsigned int unknown;
    unsigned char* pixels;
    unsigned int auxiliary;
};
struct Surface {
    int width, height, pitch;
    unsigned char* pixels;
    int unused[8];
};
static_assert(sizeof(void*) == 4, "Compile this test for Win32");
static_assert(sizeof(Frame) == 24 && sizeof(Surface) == 48, "Retail ABI");

unsigned char allocationPoison;
void* __cdecl PoisonAllocation(const char*, unsigned int size)
{
    void* memory = std::malloc(size);
    CHECK(memory);
    std::memset(memory, allocationPoison, size);
    return memory;
}

void VerifyIdentity(const std::vector<unsigned char>& file)
{
    const unsigned char expected[] = {
        0x3b,0x9c,0x0f,0xad,0xab,0xf3,0xdc,0x67,0xed,0x5f,0x05,0xa7,0x0f,0x1e,0x15,0x05,
        0xa0,0xc6,0x5d,0xea,0xdd,0x1a,0x3c,0x93,0x0a,0xdf,0xe3,0x0e,0x2a,0x84,0x99,0x5e
    };
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    CHECK(CryptAcquireContextA(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT));
    CHECK(CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash));
    CHECK(CryptHashData(hash, file.data(), static_cast<DWORD>(file.size()), 0));
    unsigned char actual[32];
    DWORD length = sizeof(actual);
    CHECK(CryptGetHashParam(hash, HP_HASHVAL, actual, &length, 0));
    CHECK(length == sizeof(expected) && std::memcmp(actual, expected, sizeof(expected)) == 0);
    CryptDestroyHash(hash);
    CryptReleaseContext(provider, 0);
}

const unsigned char* AtVA(const std::vector<unsigned char>& file, DWORD address, size_t size)
{
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data() + dos->e_lfanew);
    CHECK(nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386);
    DWORD rva = address - nt->OptionalHeader.ImageBase;
    const IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i)
    {
        if (rva >= sections[i].VirtualAddress &&
            rva + size <= sections[i].VirtualAddress + sections[i].SizeOfRawData)
        {
            size_t offset = sections[i].PointerToRawData + rva - sections[i].VirtualAddress;
            CHECK(offset + size <= file.size());
            return file.data() + offset;
        }
    }
    CHECK(false);
    return nullptr;
}

unsigned char* Routine(const std::vector<unsigned char>& file, DWORD address, size_t size,
    bool redirectAllocation = false)
{
    auto* code = static_cast<unsigned char*>(VirtualAlloc(nullptr, size,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    CHECK(code);
    std::memcpy(code, AtVA(file, address, size), size);
    if (redirectAllocation)
    {
        CHECK(code[0x18] == 0xe8); // allocator's only external call, cdecl(name,size)
        DWORD displacement = reinterpret_cast<DWORD>(&PoisonAllocation) -
            (reinterpret_cast<DWORD>(code) + 0x18 + 5);
        std::memcpy(code + 0x19, &displacement, sizeof(displacement));
    }
    DWORD oldProtect;
    CHECK(VirtualProtect(code, size, PAGE_EXECUTE_READ, &oldProtect));
    CHECK(FlushInstructionCache(GetCurrentProcess(), code, size));
    return code;
}

void Redirect(DWORD address, const void* target, unsigned char opcode)
{
    unsigned char patch[5] = {opcode};
    DWORD displacement = reinterpret_cast<DWORD>(target) - (address + 5);
    std::memcpy(patch + 1, &displacement, sizeof(displacement));
    DWORD oldProtect;
    CHECK(VirtualProtect(reinterpret_cast<void*>(address),sizeof(patch),PAGE_EXECUTE_READWRITE,&oldProtect));
    std::memcpy(reinterpret_cast<void*>(address),patch,sizeof(patch));
    CHECK(VirtualProtect(reinterpret_cast<void*>(address),sizeof(patch),oldProtect,&oldProtect));
    CHECK(FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address),sizeof(patch)));
}

void __stdcall OpaqueFrameCopy(Surface* destination, Frame* source, int x, int y)
{
    CHECK(x == 0 && y == 0);
    CHECK(MapPreviewCopy::CopyOpaque(destination->pixels,destination->width,destination->height,
        destination->pitch,source->pixels,source->width,source->height));
}

unsigned char inertGraphicsContext[1024] = {};
void* __cdecl GraphicsContext() { return inertGraphicsContext; }

void ReplayStockScaler(const std::vector<unsigned char>& file)
{
    // Map the pinned image as laboratory code, not as a loaded application:
    // no loader, entry point, imports, DLL initialization or assets. Only the
    // scaler and its software drawing helpers run. Heap calls are substituted
    // for controlled poisoning; the graphics-state getter has an inert context.
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);
    const DWORD imageSize = nt->OptionalHeader.SizeOfImage;
    auto* mapped = static_cast<unsigned char*>(VirtualAlloc(nullptr,
        imageSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    CHECK(mapped);
    std::memcpy(mapped,file.data(),nt->OptionalHeader.SizeOfHeaders);
    const IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i)
        std::memcpy(mapped+sections[i].VirtualAddress,file.data()+sections[i].PointerToRawData,
            sections[i].SizeOfRawData);
    // This selected software-only closure uses relative code calls. Its sole
    // reached absolute global reader is the graphics-context getter below.
    // No application relocations/imports are processed or needed on this path.
    const auto va = [mapped](DWORD address) { return reinterpret_cast<DWORD>(mapped) + address - 0x400000; };
    DWORD oldProtect;
    CHECK(VirtualProtect(mapped,imageSize,PAGE_EXECUTE_READ,&oldProtect));
    Redirect(va(0x4d83b0),&PoisonAllocation,0xe9);
    Redirect(va(0x4d85a0),&std::free,0xe9);
    Redirect(va(0x4b6220),&GraphicsContext,0xe9);
    using Allocator = Frame* (__stdcall*)(const char*, int, int);
    using Scaler = void (__stdcall*)(Frame*, int, int, int, int);
    const auto allocate = reinterpret_cast<Allocator>(va(0x4b8da0));
    const auto scale = reinterpret_cast<Scaler>(va(0x4665d0));
    int corruptedCases = 0;
    for (const auto& world : {std::pair<int,int>(4128,4224), {8192,4096}, {4096,8192}})
    {
        std::vector<unsigned char> reference;
        for (int key = 0; key < 256; ++key)
        {
            for (int fixed = 0; fixed < 2; ++fixed)
            {
                Redirect(va(0x4666c0),fixed ? reinterpret_cast<void*>(&OpaqueFrameCopy) :
                    reinterpret_cast<void*>(va(0x4b7f90)),0xe8);
                allocationPoison = static_cast<unsigned char>(key);
                Frame* frame = allocate("source",252,252);
                for (int i = 0; i < 252*252; ++i)
                    frame->pixels[i] = static_cast<unsigned char>(1 + i%255);
                allocationPoison = static_cast<unsigned char>(key ^ 255);
                scale(frame,235,235,world.first,world.second);
                CHECK(frame->width == 235 && frame->height == 235);
                if (key == 0 && !fixed)
                    reference.assign(frame->pixels,frame->pixels+235*235);
                bool equal = std::memcmp(reference.data(),frame->pixels,reference.size()) == 0;
                if (fixed || key == 0) CHECK(equal);
                else { CHECK(!equal); ++corruptedCases; }
                std::free(frame);
            }
        }
    }
    CHECK(corruptedCases == 765);
    std::puts("PASS: original stock scaler/quad/span code: 765 poisoned-key corruptions reproduced; all 768 fixed square/wide/tall results equal pristine stock output byte for byte.");
    CHECK(VirtualFree(mapped,0,MEM_RELEASE));
}

int main(int argc, char** argv)
{
    CHECK(argc == 2);
    std::ifstream input(argv[1], std::ios::binary);
    CHECK(input.good());
    const std::vector<unsigned char> file((std::istreambuf_iterator<char>(input)), {});
    VerifyIdentity(file);
    const unsigned char context[] = {0x6a,0,0x6a,0,0x8d,0x54,0x24,0x68,0x57,0x52,
        0xe8,0xcb,0x18,0x05,0};
    CHECK(std::memcmp(AtVA(file, 0x4666b6, sizeof(context)), context, sizeof(context)) == 0);
    auto* allocatorCode = Routine(file, 0x4b8da0, 83, true);
    auto* blitterCode = Routine(file, 0x4cbe70, 129);
    using Allocator = Frame* (__stdcall*)(const char*, int, int);
    using Blitter = void (__cdecl*)(Surface*, Surface*, RECT*, RECT*, char);
    auto allocate = reinterpret_cast<Allocator>(allocatorCode);
    auto blit = reinterpret_cast<Blitter>(blitterCode);

    // Every key must survive the actual stock allocator, including 0 and 255.
    // The actual stock blitter then skips that index. Poison ensures visibility.
    for (int key = 0; key < 256; ++key)
    {
        allocationPoison = static_cast<unsigned char>(key);
        Frame* source = allocate("test", 64, 4);
        CHECK(source->key == key && source->compressed == 0 && source->children == 0);
        CHECK(source->pixels == reinterpret_cast<unsigned char*>(source) + 24);
        for (int i = 0; i < 256; ++i) source->pixels[i] = static_cast<unsigned char>(i);
        allocationPoison = static_cast<unsigned char>(key ^ 255);
        Frame* temporary = allocate("temp", 64, 4);
        Surface src = {64,4,64,source->pixels,{}};
        Surface dst = {64,4,64,temporary->pixels,{}};
        RECT rectangle = {0,0,63,3};
        blit(&dst, &src, &rectangle, &rectangle, static_cast<char>(source->key));
        int differences = 0;
        for (int i = 0; i < 256; ++i)
        {
            if (temporary->pixels[i] != source->pixels[i]) ++differences;
            CHECK(temporary->pixels[i] == (i == key ? (key ^ 255) : i));
        }
        CHECK(differences == 1);
        CHECK(MapPreviewCopy::CopyOpaque(temporary->pixels,64,4,64,source->pixels,64,4));
        CHECK(std::memcmp(temporary->pixels,source->pixels,256) == 0);
        std::free(temporary);
        std::free(source);
    }
    std::puts("PASS: actual stock allocator retains all 256 poisoned color keys; actual blitter drops each key; fixed copy preserves every index.");

    // Odd widths, stored/canvas-like sizes, padded pitch and whole-row canaries.
    for (const auto& shape : {std::pair<int,int>(1,1), {17,13}, {235,235}, {252,252}, {511,127}})
    {
        int width = shape.first, height = shape.second, pitch = width + 7;
        std::vector<unsigned char> source(width * height);
        std::vector<unsigned char> destination(pitch * height + 32,0x5d);
        for (size_t i = 0; i < source.size(); ++i) source[i] = static_cast<unsigned char>(i * 37);
        CHECK(MapPreviewCopy::CopyOpaque(destination.data()+16,width,height,pitch,source.data(),width,height));
        for (int y = 0; y < height; ++y)
        {
            CHECK(std::memcmp(destination.data()+16+y*pitch,source.data()+y*width,width) == 0);
            for (int x = width; x < pitch; ++x) CHECK(destination[16+y*pitch+x] == 0x5d);
        }
        for (int i = 0; i < 16; ++i)
            CHECK(destination[i] == 0x5d && destination[destination.size()-1-i] == 0x5d);
        auto unchanged = destination;
        CHECK(!MapPreviewCopy::CopyOpaque(destination.data()+16,width,height,width-1,source.data(),width,height));
        CHECK(!MapPreviewCopy::CopyOpaque(destination.data()+16,width-1,height,pitch,source.data(),width,height));
        CHECK(destination == unchanged);
    }
    CHECK(!MapPreviewCopy::CopyOpaque(nullptr,1,1,1,nullptr,1,1));
    std::puts("PASS: five shapes, odd widths, padded pitch, guard bytes and invalid-copy refusals.");
    ReplayStockScaler(file);
    VirtualFree(allocatorCode,0,MEM_RELEASE);
    VirtualFree(blitterCode,0,MEM_RELEASE);
    return 0;
}
