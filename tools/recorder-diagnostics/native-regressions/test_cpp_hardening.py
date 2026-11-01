"""Compile real old/new C++ methods, probe only synthetic buffers/private hive."""
from pathlib import Path
import subprocess, json
import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[3])
parser.add_argument('--out',type=Path,default=Path(__file__).resolve().parents[1]/'build'/Path(__file__).stem)
parser.add_argument('--baseline',default='dcff5ddeb6bd1030e3f452c0f16e5f005850f62f')
parser.add_argument('--vcvars',type=Path,default=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat'))
args=parser.parse_args()
ROOT=args.source.resolve(); OUT=args.out.resolve(); VC=args.vcvars.resolve()
OUT.mkdir(parents=True,exist_ok=True)
def read(name,old):
    if old: return subprocess.check_output(['git','-c','safe.directory='+str(ROOT),'-C',str(ROOT),'show',args.baseline+':'+name]).decode('latin1')
    return (ROOT/name).read_bytes().decode('latin1')
def part(s,start,end): return start+s.split(start,1)[1].split(end,1)[0]
base=r'''
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <cstddef>
#define private public
#include "__HEADER__"
#undef private
static int checks;
static void check(bool b, const char* s) { ++checks; if(!b) throw std::runtime_error(s); }
struct TextField { std::string m_registryKey, m_text; void RegistryRead(HKEY); void RegistryWrite(HKEY); };
__METHODS__
__KEY_METHOD__
TADRConfig::TADRConfig() {} // Do not run application startup or touch real settings.
int trimProbe(bool expectOld) {
    SYSTEM_INFO si; GetSystemInfo(&si);
    char* pages=static_cast<char*>(VirtualAlloc(NULL,si.dwPageSize*2,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    DWORD prior; VirtualProtect(pages,si.dwPageSize,PAGE_NOACCESS,&prior);
    char* text=pages+si.dwPageSize; strcpy_s(text,si.dwPageSize," \t\r\n");
    bool fault=false;
    __try { char* p=trim_crlf_(text); if(!expectOld && *p!=0) return 3; }
    __except(EXCEPTION_EXECUTE_HANDLER) { fault=true; }
    VirtualFree(pages,0,MEM_RELEASE);
    return fault==expectOld ? 0 : 2;
}
void configProbe(const char* mode) {
    TADRConfig c; std::string ini;
    if(!strcmp(mode,"longsection")) ini="["+std::string(1000,'a')+"]\r\n[REG]\r\n\"a\"=dword:7\r\n";
    else if(!strcmp(mode,"longname")) ini="[REG]\r\n\""+std::string(300,'a')+"\"=dword:7\r\n";
    else if(!strcmp(mode,"longline")) ini="[REG]\n"+std::string(511,' ')+"\n";
    else if(!strcmp(mode,"legacysection")) ini="[REGISTRY]\r\n\"a\"=dword:7\r\n";
    else ini="[REG]\r\n \t\r\n\"a\"=dword:7\r\n\"b\"=\"hello\"\r\n";
    c.LoadIniRegSetting(reinterpret_cast<BYTE*>(&ini[0]));
    if(!strcmp(mode,"valid") || !strcmp(mode,"longsection") || !strcmp(mode,"legacysection")) {
        check(c.RegDwords_vec.size()==1,"valid DWORD missing");
        check(c.RegDwords_vec[0]->Value()==7,"DWORD changed");
    }
    if(!strcmp(mode,"valid")) check(c.RegStrings_vec.size()==1 && !strcmp(c.RegStrings_vec[0]->Str(),"hello"),"valid string changed");
}
void registryProbe(const char* file, bool old) {
    HKEY hive=0,key=0; DWORD disp=0;
    check(RegLoadAppKeyA(file,&hive,KEY_ALL_ACCESS,REG_PROCESS_APPKEY,0)==0,"private hive");
    check(RegCreateKeyExA(hive,"Fixture",0,0,0,KEY_ALL_ACCESS,0,&key,&disp)==0,"fixture key");
    const char bad[]="bad";
    check(RegSetValueExA(key,"i",0,REG_SZ,reinterpret_cast<const BYTE*>(bad),sizeof(bad))==0,"put wrong type");
    DWORD value=2048;
    LONG rc=RegReadData(hive,"Fixture","i",REG_DWORD,&value,sizeof(value));
    if(old) check(rc==0 && value!=2048,"old type confusion not reproduced");
    else check(rc==ERROR_INVALID_DATA && value==2048,"wrong type clobbered default");
    DWORD before=0,after=0; GetProcessHandleCount(GetCurrentProcess(),&before);
    for(int i=0;i<1000;++i) RegReadData(hive,"Fixture","i",REG_SZ,&value,sizeof(value));
    GetProcessHandleCount(GetCurrentProcess(),&after);
    check(old ? after>=before+1000 : after<=before+2,"registry read handle lifetime");
    rc=RegReadData(hive,"Missing","x",REG_DWORD,&value,sizeof(value));
    HKEY missing=0; LONG exists=RegOpenKeyExA(hive,"Missing",0,KEY_READ,&missing);
    check(old ? exists==0 : exists==ERROR_FILE_NOT_FOUND,"read created settings key");
    if(missing) RegCloseKey(missing);
    TextField f; f.m_registryKey="text"; f.m_text="default";
    std::string text(1023,'x');
    check(RegSetValueExA(key,"text",0,REG_SZ,reinterpret_cast<const BYTE*>(text.data()),static_cast<DWORD>(text.size()))==0,"put unterminated legacy buffer");
    if(!old) {
        f.RegistryRead(key); check(f.m_text==text,"bounded legacy text read");
        text.assign(1025,'x');
        RegSetValueExA(key,"text",0,REG_SZ,reinterpret_cast<const BYTE*>(text.data()),static_cast<DWORD>(text.size()));
        f.m_text="default"; f.RegistryRead(key); check(f.m_text=="default","oversized text fallback");
        value=7; RegSetValueExA(key,"text",0,REG_DWORD,reinterpret_cast<BYTE*>(&value),4);
        f.m_text="default"; f.RegistryRead(key); check(f.m_text=="default","text type check");
        f.m_text="saved"; f.RegistryWrite(key);
        char out[16]; DWORD size=sizeof(out),kind=0;
        check(RegQueryValueExA(key,"text",0,&kind,reinterpret_cast<BYTE*>(out),&size)==0 && size==6 && out[5]==0,"text writer omitted NUL");
        char str[8]="keep"; RegSetValueExA(key,"s",0,REG_SZ,reinterpret_cast<const BYTE*>("abc"),3);
        check(RegReadData(key,NULL,"s",REG_SZ,str,sizeof(str))==0 && !strcmp(str,"abc"),"helper termination");
        value=7; RegSetValueExA(key,"i",0,REG_DWORD,reinterpret_cast<BYTE*>(&value),1);
        DWORD result=2048;
        check(RegReadData(key,NULL,"i",REG_DWORD,&result,4)==ERROR_INVALID_DATA && result==2048,"short DWORD clobbered default");
    }
    RegCloseKey(key); RegCloseKey(hive);
}
int main(int argc,char** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    try {
        bool old=__OLD__;
        if(argc<2) return 4;
        if(!strcmp(argv[1],"keys")) {
            TADRConfig c;
            for(int fail=0;fail<=2;++fail) {
                keyFail=fail;keyCalls=keyCloses=0;
                HKEY k=c.TARegPath_HKEY();
                check(fail ? k==NULL : k==reinterpret_cast<HKEY>(0x102),"invalid key returned");
                check(keyCloses==(fail==1 ? 0 : 1),"parent key lifetime");
            }
            return 0;
        }
        if(!strcmp(argv[1],"trim")) return trimProbe(old);
        if(!strcmp(argv[1],"registry")) registryProbe(argv[2],old);
        else configProbe(argv[1]);
        printf("PASS checks=%d\n",checks); return 0;
    } catch(const std::exception& e) { fprintf(stderr,"FAIL %s\n",e.what()); return 1; }
}
'''.replace('__HEADER__',str(ROOT/'src/DDraw/TAConfig.h').replace('\\','/'))
results=[]
key_stub=r'''
static int keyFail,keyCalls,keyCloses;
static LPCSTR CompanyName_CCSTR="FixtureCompany",GameName_CCSTR="FixtureGame";
static LONG WINAPI ProbeCreateKey(HKEY,LPCSTR,DWORD,LPSTR,DWORD,REGSAM,LPSECURITY_ATTRIBUTES,HKEY* out,LPDWORD) {
    ++keyCalls;
    *out=reinterpret_cast<HKEY>(keyCalls==keyFail ? 0xdead : 0x100+keyCalls);
    return keyCalls==keyFail ? ERROR_ACCESS_DENIED : ERROR_SUCCESS;
}
static LONG WINAPI ProbeCloseKey(HKEY) { ++keyCloses; return ERROR_SUCCESS; }
#undef RegCreateKeyEx
#define RegCreateKeyEx ProbeCreateKey
#define RegCloseKey ProbeCloseKey
__BODY__
#undef RegCreateKeyEx
#define RegCreateKeyEx RegCreateKeyExA
#undef RegCloseKey
'''
for old in (True,False):
    etc=read('src/DDraw/hook/etc.cpp',old); text=read('src/DDraw/Widgets/TextField.cpp',old); config=read('src/DDraw/TAConfig.cpp',old)
    methods=part(etc,'char * trim_crlf_ (char * Str)','LONG RegWriteData (')
    methods+=part(text,'void TextField::RegistryRead(HKEY hKey)','void TextField::RegistryWrite')
    methods+=text[text.index('void TextField::RegistryWrite(HKEY hKey)'):]
    methods+=part(config,'void TADRConfig::LoadIniRegSetting','int TADRConfig::EnumIniRegInfo_Begin')
    methods+=part(config,'TADRConfig::~TADRConfig','BOOL TADRConfig::SetIniBool')
    methods+=config[config.index('RegDword::RegDword'):]
    key_method=key_stub.replace('__BODY__',part(config,'HKEY TADRConfig::TARegPath_HKEY','LONG TADRConfig::WriteTAReg_Dword'))
    name='old' if old else 'fixed'
    template=base.replace('__KEY_METHOD__',key_method)
    cpp=OUT/(name+'.cpp'); cpp.write_text(template.replace('__METHODS__',methods).replace('__OLD__',str(old).lower()),encoding='latin1')
    cmd=OUT/(name+'.cmd'); cmd.write_text('@echo off\ncall "'+str(VC)+'" >nul\ncl /nologo /EHsc /Od /Zi /RTC1 /GS /D_CRT_SECURE_NO_WARNINGS "'+str(cpp)+'" /Fe:"'+str(OUT/(name+'.exe'))+'" /Fo:"'+str(OUT/(name+'.obj'))+'" /link advapi32.lib\n',encoding='ascii')
    build=subprocess.run(['cmd','/d','/c',str(cmd)],cwd=OUT,capture_output=True,text=True)
    if build.returncode: print(build.stdout,build.stderr); build.check_returncode()
    for mode in (('trim','registry','longsection','longname','longline','valid','legacysection') if old else ('trim','registry','longsection','longname','longline','valid','legacysection','keys')):
        args=[str(OUT/(name+'.exe')),mode,str(OUT/(name+'.hiv'))]
        run=subprocess.run(args,capture_output=True,text=True,timeout=30)
        expected_failure=old and mode in ('longsection','longname','longline')
        passed=(run.returncode!=0) if expected_failure else (run.returncode==0)
        results.append({'variant':name,'mode':mode,'exit':run.returncode,'expected_failure':expected_failure,'pass':passed,'out':run.stdout,'error':run.stderr})
        print(name,mode,run.returncode,'PASS' if passed else 'FAIL',run.stdout.strip(),run.stderr[:120])
    ctor=part(config,'TADRConfig::TADRConfig ()','TADRConfig::~TADRConfig')
    prefix=r'''
#include <limits.h>
#include <rtcapi.h>
#include <winioctl.h>
static int readMode;
struct DummyHPI { void free_readfile(char*) {} };
static DummyHPI* TAHPI;
LPCSTR TAIniStr="ddraw.ini";
static UINT WINAPI ProbeTempName(LPCSTR,LPCSTR,UINT,LPSTR) { return 0; }
static BOOL WINAPI ProbeRead(HANDLE f,LPVOID p,DWORD n,LPDWORD read,LPOVERLAPPED ov) {
    if(readMode==1) { *read=0; return FALSE; }
    return ReadFile(f,p,readMode==2 ? 1 : n,read,ov);
}
int TADRConfig::EnumIniRegInfo_Begin(PEnumRegInfo*,LPCSTR* name,LPCVOID*) { *name=NULL; return 0; }
int TADRConfig::EnumIniRegInfo_Next(PEnumRegInfo*,LPCSTR* name,LPCVOID*) { *name=NULL; return 0; }
int TADRConfig::EnumIniRegInfo_End(PEnumRegInfo*) { return 0; }
LONG TADRConfig::WriteTAReg_Dword(LPTSTR,DWORD) { throw std::runtime_error("unexpected registry write"); }
LONG TADRConfig::WriteTAReg_Str(LPTSTR,LPCSTR,DWORD) { throw std::runtime_error("unexpected registry write"); }
static int __cdecl RTCFailure(int,const char*,int,const char*,const char*,...) { ExitProcess(93); return 0; }
#define GetTempFileNameA ProbeTempName
#define ReadFile ProbeRead
'''
    ctorprobe=template.replace('__METHODS__',methods).replace('__OLD__',str(old).lower())
    ctorprobe=ctorprobe.replace('TADRConfig::TADRConfig() {} // Do not run application startup or touch real settings.',prefix+ctor+'\n#undef ReadFile\n#undef GetTempFileNameA\n')
    ctorprobe=ctorprobe.replace('bool old='+str(old).lower()+';', 'bool old='+str(old).lower()+';\n_RTC_SetErrorFunc(RTCFailure);\nif(argc>2) readMode=atoi(argv[2]);\nif(argc>1 && !strcmp(argv[1],"constructor")) { TADRConfig c; if(!old) check(c.RegDwords_vec.size()==static_cast<size_t>(atoi(argv[3])),"constructor parsed wrong number of values"); return 0; }')
    cpp.write_text(ctorprobe,encoding='latin1')
    build=subprocess.run(['cmd','/d','/c',str(cmd)],cwd=OUT,capture_output=True,text=True)
    if build.returncode: print(build.stdout,build.stderr); build.check_returncode()
    for case in ('empty','normal','readfail','shortread'):
        (OUT/'ddraw.ini').write_bytes(b'' if case=='empty' else b'[REG]\r\n"a"=dword:7\r\n')
        run=subprocess.run([str(OUT/(name+'.exe')),'constructor',str({'empty':0,'normal':0,'readfail':1,'shortread':2}[case]),str(int(case=='normal'))],capture_output=True,text=True,timeout=30)
        # Old empty input uses an uninitialized pointer (/RTC reports it).
        # Old read-error behavior is recorded, not asserted as a deterministic fault.
        passed=(run.returncode==93) if old and case=='empty' else (True if old else run.returncode==0)
        results.append({'variant':name,'mode':'constructor '+case,'exit':run.returncode,'pass':passed,'out':run.stdout,'error':run.stderr})
        print(name,'constructor',case,run.returncode,'PASS' if passed else 'FAIL')
(OUT/'results.json').write_text(json.dumps(results,indent=2))
assert all(r['pass'] for r in results)
