unit ErrorLog_ExtraData;

interface
uses
  PluginEngine;
{
Adds extra data logging to TA's errorlog.txt file.

Lists the loaded modules *not* from the system directory.
  - Attributes listed are; filename, base address, size & version 
}
function GetPlugin : TPluginData;

Procedure Errorlog_Thunk1;

implementation
uses
  Windows, Tlhelp32,
  SysUtils,
  Contnrs,
  Classes,
  TADemoConsts,
  TA_MemoryLocations,
  TA_FunctionsU,
  logging;

  
Procedure OnInstall;
begin
end;

Procedure OnUnInstall;
begin
end;

function GetPlugin : TPluginData;
begin
  if IsTAVersion31 then
  begin
    Result := TPluginData.Create( False,
                                  'Errorlog extra data',
                                  True,
                                  @OnInstall,
                                  @OnUnInstall );

    Result.MakeRelativeJmp( True,
                            'Errorlog extra data',
                            @Errorlog_Thunk1,
                            $004D989B, 2 );

    Result.MakeReplacement( True,
                            'Errorlog entry spacer from 5 to 1 line',
                            $0050C35C,
                            [$0A, 0] );
  end else
    Result := nil;
end;

type
  TModuleInfo = class
    name: String;
    Fullimagepath: String;
    imagepath: String;
    basePTR: Pointer;
    size: LongWord;

    Constructor Create(aname, aimagepath: String; abasePTR: Pointer; asize: LongWord);
  end;

Constructor TModuleInfo.Create(aname, aimagepath: String; abasePTR: Pointer; asize: LongWord);
begin
  name := aname;
  Fullimagepath := aimagepath;
  imagepath := AnsiLowerCase(ExtractFileDir(aimagepath));
  basePTR := abasePTR;
  size := asize;
end;  

function ModuleInfoComparer(Item1, Item2: Pointer): Integer;
begin
  if Longword(TModuleInfo(Item1).basePTR) > Longword(TModuleInfo(Item2).basePTR) then
    result := 1
  else if Longword(TModuleInfo(Item1).basePTR) < Longword(TModuleInfo(Item2).basePTR) then
    result := -1
  else
    result := 0
end;

Procedure Errorlog_Thunk2( filehandle: THandle ); stdcall;
var
  data: String;
  DataWritten: LongWord;
  ModuleSnap: THandle;
  me32: MODULEENTRY32;

  moduleList: TObjectList;
  item: TModuleInfo;
  i: Integer;

  systemPath: String;
begin
  try
    //  Take a snapshot of all modules in the specified process.
    ModuleSnap := CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, 0 );
    if( ModuleSnap <> INVALID_HANDLE_VALUE ) then
      try
        //  Set the size of the structure before using it.
        me32.dwSize := sizeof( MODULEENTRY32 );
        // Retrieve information about the first module,
        // and exit if unsuccessful
        if not Module32First( ModuleSnap, me32 ) then
          exit;
        moduleList := TObjectList.create(True);
      try
        // determine the system path so we can exclude system DLLs
        systemPath := AnsiLowerCase( ExcludeTrailingPathDelimiter( GetSysDir() ) );

        //  Now walk the module list of the process & generate the list of modules
        repeat
          moduleList.Add( TModuleInfo.create( me32.szModule,
                                              me32.szExePath,
                                              me32.modBaseAddr,
                                              me32.modBaseSize) );
        until not Module32Next( ModuleSnap, me32 );
          // sort them all into the right order
        moduleList.Sort( ModuleInfoComparer );
        // output to the logfile in 1 hit
        data := '';
        if moduleList.count > 0 then
          data := 'Modules:' +#13#10;
        for i := 0 to moduleList.count-1 do
          begin
          item := TModuleInfo(moduleList[i]);
          if item.imagepath = systemPath then
            continue;
          data := data +
                  item.name+ ' : ' +
                  IntToHex(Longword(item.basePTR),8) +' : '+
                  IntToHex(item.size,8) +' : '+
                  GetFileVersion(item.Fullimagepath) + #13#10;
          end;
        // only output if there is stuff to output
        if length(data) > 0 then
        begin
          data := data + #13#10 + #13#10 + #13#10 + #13#10 + #13#10;
          WriteFile( filehandle, data[1], length(data), DataWritten, nil );
        end;
      finally
        moduleList.free;
      end;
      finally
        //  Do not forget to clean up the snapshot object.
        CloseHandle( ModuleSnap );
      end;
  finally
    CloseHandle(filehandle);
  end;
  if TLog <> nil then
    TLog.Flush;
end;

Procedure Errorlog_Thunk1;
asm
  push esi
  Call Errorlog_Thunk2;

  push $4D98A2
  call PatchNJump;
end;

end.
