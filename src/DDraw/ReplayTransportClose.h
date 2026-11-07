#pragma once
// Replay transport-only closer. TA/recorder cleanup and rendering stay
// on the GUI thread. Only the system DirectPlay implementation runs here.
#include <windows.h>
#include <objbase.h>
#include <memory>
#include <vector>
#pragma comment(lib, "ole32.lib")

namespace replayclose {
using CloseProc = HRESULT (WINAPI*)(IUnknown*);
class Closer;
static Closer* router = nullptr;
static HRESULT WINAPI RouteClose(IUnknown* object);

class Closer {
    struct Slot { CloseProc* address; CloseProc original; };
    struct Job {
        IUnknown* object = nullptr;
        IUnknown* identity = nullptr;
        CloseProc original = nullptr;
        HMODULE codePin = nullptr, providerPin = nullptr;
        HANDLE thread = nullptr;
        HRESULT result = E_PENDING;
        DWORD elapsed = 0;
        bool started = false;
        ~Job() {
            if (object) object->Release();
            if (providerPin) FreeLibrary(providerPin);
            if (codePin) FreeLibrary(codePin);
            if (thread) CloseHandle(thread);
        }
    };
    std::vector<Slot> slots;
    std::vector<std::shared_ptr<Job>> jobs;
    HMODULE systemLibrary = nullptr;
    DWORD guiThread = GetCurrentThreadId();
    bool (*allowed)();

    static DWORD WINAPI Work(void* context) {
        auto owner = (std::shared_ptr<Job>*)context;
        auto job = *owner;
        delete owner;
        const HMODULE codePin = job->codePin;
        job->codePin = nullptr;
        HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const DWORD start = GetTickCount();
        job->result = job->original(job->object);
        job->object->Release();
        job->object = nullptr;
        job->elapsed = GetTickCount()-start;
        FreeLibrary(job->providerPin);
        job->providerPin = nullptr;
        if (SUCCEEDED(apartment)) CoUninitialize();
        job.reset();
        // Keep this module loaded until the final instruction of its worker.
        FreeLibraryAndExitThread(codePin, 0);
        return 0;
    }

    bool start(IUnknown* object, CloseProc original) {
        IUnknown* identity = nullptr;
        if (FAILED(object->QueryInterface(IID_IUnknown,(void**)&identity))) return false;
        identity->Release();
        for (const auto& pending : jobs) if (pending->identity==identity) return true;
        auto job = std::make_shared<Job>();
        job->identity = identity;
        job->original = original;
        const DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS;
        if (!GetModuleHandleExA(flags, (LPCSTR)Work, &job->codePin)) return false;
        if (!GetModuleHandleExA(flags, (LPCSTR)original, &job->providerPin)) return false;
        object->AddRef();
        job->object = object;
        std::unique_ptr<std::shared_ptr<Job>> owner(new std::shared_ptr<Job>(job));
        // Allocate/register all ownership before starting the worker. An
        // allocation failure must never start a second synchronous Close.
        jobs.push_back(job);
        // Native HAPINET_close still has to release its interface/lobby refs.
        // Those releases can take DirectPlay's global lock. Do not enter the
        // blocking Close until ALL native GUI-side releases have completed.
        job->thread = CreateThread(nullptr, 0, Work, owner.get(), CREATE_SUSPENDED, nullptr);
        if (!job->thread) {
            jobs.pop_back();
            return false;
        }
        owner.release();
        return true;
    }

public:
    explicit Closer(bool (*predicate)()) : allowed(predicate) {
        // Obtain native vtables through the system factory; never patch the
        // recorder wrapper's methods or assume a Windows DLL code address.
        wchar_t path[MAX_PATH]{};
        UINT length = GetSystemDirectoryW(path, MAX_PATH);
        if (!length || length+12 >= MAX_PATH) return;
        wcscat_s(path, L"\\dplayx.dll");
        systemLibrary = LoadLibraryW(path);
        if (!systemLibrary) return;
        using Create = HRESULT (WINAPI*)(GUID*, IUnknown**, IUnknown*);
        auto create = (Create)GetProcAddress(systemLibrary,"DirectPlayCreate");
        GUID noProvider{};
        IUnknown* raw = nullptr;
        if (!create || FAILED(create(&noProvider,&raw,nullptr))) return;
        const GUID interfaces[] = {
            {0x133efe41,0x32dc,0x11d0,{0x9c,0xfb,0,0xa0,0xc9,0x0a,0x43,0xcb}},
            {0x133efe40,0x32dc,0x11d0,{0x9c,0xfb,0,0xa0,0xc9,0x0a,0x43,0xcb}}
        };
        for (const GUID& iid : interfaces) {
            IUnknown* peer = nullptr;
            if (FAILED(raw->QueryInterface(iid,(void**)&peer))) continue;
            auto address = ((CloseProc*)*(void**)peer)+4;
            HMODULE implementation = nullptr;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                  (LPCSTR)*address,&implementation) && implementation==systemLibrary) {
                bool duplicate = false;
                for (const auto& slot : slots) if (slot.address==address) duplicate = true;
                if (!duplicate) slots.push_back({address,*address});
            }
            peer->Release();
        }
        raw->Release();
        router = this;
        for (const auto& slot : slots) {
            DWORD old;
            if (VirtualProtect(slot.address,sizeof(void*),PAGE_READWRITE,&old)) {
                InterlockedExchangePointer((void* volatile*)slot.address,(void*)RouteClose);
                VirtualProtect(slot.address,sizeof(void*),old,&old);
            }
        }
    }
    ~Closer() {
        for (const auto& slot : slots) {
            DWORD old;
            if (VirtualProtect(slot.address,sizeof(void*),PAGE_READWRITE,&old)) {
                InterlockedExchangePointer((void* volatile*)slot.address,(void*)slot.original);
                VirtualProtect(slot.address,sizeof(void*),old,&old);
            }
        }
        router = nullptr;
        if (systemLibrary) FreeLibrary(systemLibrary);
        // Each running worker owns its job, its native interface, and module
        // references independently. Neither TA memory nor this object is used.
    }
    HRESULT close(IUnknown* object) {
        auto address = ((CloseProc*)*(void**)object)+4;
        for (const auto& slot : slots) if (slot.address==address) {
            if (GetCurrentThreadId()==guiThread && allowed()) {
                try { if (start(object,slot.original)) return S_OK; }
                catch (...) { /* Preserve synchronous native teardown on allocation failure. */ }
            }
            return slot.original(object);
        }
        return E_UNEXPECTED;
    }
    template<class Report> void collect(Report report) {
        for (auto it=jobs.begin(); it!=jobs.end();) {
            const auto& job = *it;
            if (WaitForSingleObject(job->thread,0)!=WAIT_OBJECT_0) { ++it; continue; }
            report(job->result,job->elapsed);
            it = jobs.erase(it);
        }
    }
    void startQueued() {
        // Called only at the first native results frame, after HAPINET_close
        // and every GUI-side transport Release have returned.
        for (const auto& job : jobs) if (!job->started) {
            if (ResumeThread(job->thread)!=DWORD(-1)) job->started = true;
        }
    }
    bool pending() const { return !jobs.empty(); }
    unsigned hookedInterfaces() const { return (unsigned)slots.size(); }
};
static HRESULT WINAPI RouteClose(IUnknown* object) { return router->close(object); }
}
