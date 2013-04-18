;
; TA Demo 0.99b2 installerare
;

; Fina globaler
!define VERSION "TA Demo 0.99b2"
!define EXENAME "tademo99b2.exe"

; Ändrar defaultprylar
Name "${VERSION}"
OutFile "${EXENAME}"
Caption "${VERSION} Installer"
CRCCheck On
WindowIcon off
ShowInstDetails show
ShowUninstDetails show
SetDateSave off

; Skriver ut en fin licens
LicenseText "The ${VERSION} license agreement"
LicenseData license.txt

; The default installation directory
InstallDir $PROGRAMFILES\TADemo

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "SOFTWARE\Yankspankers\TA Demo" "Install_Dir"

; The text to prompt the user to enter a directory
ComponentText "This will install ${VERSION} on your computer. Select which optional things you want installed."
; The text to prompt the user to enter a directory
DirText "Choose a directory to install in to:" 

; The stuff to install
Section "TA Demo Recorder (required)"

  ; Börja med ett shortcut-entry
  CreateDirectory "$SMPROGRAMS\TA Demo"

  ; Saker som ska in i TA-katalogen
  SetOutPath $6

  File "..\recorder\dplayx.dll"
  File "..\ddraw.dll"
  File "..\tademo.ufo"

  CopyFiles /SILENT /FILESONLY $6\ddraw.dll $6\spank.dll 268

  ; Saker som ska in i vanliga demo-katalogen
  SetOutPath $INSTDIR

  File "..\server\server.exe"
  File "..\server\maps.txt"
  File "..\server\unitid.txt"
  File "..\vispatcher\vispatcher.exe"

  CreateShortCut "$SMPROGRAMS\TA Demo\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  CreateShortCut "$SMPROGRAMS\TA Demo\Replayer.lnk" "$INSTDIR\server.exe"
  CreateShortCut "$SMPROGRAMS\TA Demo\Windows 9x Patcher.lnk" "$INSTDIR\vispatcher.exe"

  ; Dokumentationen
  SetOutPath $INSTDIR\docs

  File "..\docs\*.*"

  CreateShortCut "$SMPROGRAMS\TA Demo\Documentation.lnk" "$INSTDIR\docs\readme.html"

  ; Skriv ner en regkey som denna installer kikar på sen
  WriteRegStr HKLM "SOFTWARE\Yankspankers\TA Demo" "Install_Dir" "$INSTDIR"
  WriteRegStr HKLM "SOFTWARE\Yankspankers\TA Demo" "TA_Dir" $6

  ; Skriv uninstaller
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TA Demo Recorder" "TA Demo Recorder 0.99ß2" "TA Demo Recorder (remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TA Demo Recorder" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteUninstaller "uninstall.exe"

  ; Fixa associationer
  WriteRegStr HKCR ".tad" "" "server.Document" 
  WriteRegStr HKCR "server.Document" "" "TA Demo" 
  WriteRegStr HKCR "server.Document\shell\open\command" "" '"$INSTDIR\SERVER.EXE" %1'

  ; Laga dx-paths ifall usern vill det
  StrCmp $5 "0" done
    WriteRegStr HKLM "SOFTWARE\Microsoft\DirectPlay\Applications\Total Annihilation" "Path" $6
    WriteRegStr HKLM "SOFTWARE\Microsoft\DirectPlay\Applications\Total Annihilation" "File" "totala.exe"
    WriteRegStr HKLM "SOFTWARE\Microsoft\DirectPlay\Applications\Total Annihilation" "Guid" "{99797420-F5F5-11CF-9827-00A0241496C8}"

  done:
SectionEnd

; optional section
Section "3D Replayer"

  SetOutPath $INSTDIR
  File "..\3dta\3dta.exe"
  File "..\3dta\3dtaconfig.exe"
  File "..\3dta\bagge.fnt"
  File "..\3dta\HPIutil.dll"
  File "..\3dta\palette.pal"
  File "..\3dta\uikeys.txt"

  CreateShortCut "$SMPROGRAMS\TA Demo\3D Replayer.lnk" "$INSTDIR\3dta.exe" 
  CreateShortCut "$SMPROGRAMS\TA Demo\3D Replayer Configuration.lnk" "$INSTDIR\3dtaconfig.exe" 

  SetOutPath $INSTDIR\bitmaps
  File "..\3dta\bitmaps\*.*"

SectionEnd

; uninstall stuff

UninstallText "This will uninstall ${VERSION}. Hit next to continue."

; special uninstall section.
Section "Uninstall"
  ; Kolla i vilken katalog vi installerade dplayx.dll etc
  ReadRegStr $R0 HKLM "SOFTWARE\Yankspankers\TA Demo" "TA_Dir"

  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TA Demo Recorder"
  DeleteRegKey HKLM "SOFTWARE\Yankspankers"
  DeleteRegKey HKCU "SOFTWARE\Yankspankers"

  ; Ta bort filassociationen
  DeleteRegKey HKCR ".tad"
  DeleteRegKey HKCR "server.Document"

  ; Ta bort saker ur ta-katalogen
  Delete $R0\dplayx.dll
  Delete $R0\ddraw.dll
  Delete $R0\tademo.ufo

  ; Saker ur underkataloger kan vi köra *.* på
  Delete $INSTDIR\Docs\*.*
  Delete $INSTDIR\bitmaps\*.*

  ; Men i huvudkatalogen känns bäst att specificera
  Delete $INSTDIR\server.exe
  Delete $INSTDIR\maps.txt
  Delete $INSTDIR\unitid.txt
  Delete $INSTDIR\vispatcher.exe

  Delete $INSTDIR\3dta.exe
  Delete $INSTDIR\3dtaconfig.exe
  Delete $INSTDIR\bagge.fnt
  Delete $INSTDIR\hpiutil.dll
  Delete $INSTDIR\palette.pal
  Delete $INSTDIR\uikeys.txt
  Delete $INSTDIR\tatex.*

  ; Själva installern ska också bort
  Delete $INSTDIR\uninstall.exe

  ; remove shortcuts, if any.
  Delete "$SMPROGRAMS\TA Demo\*.*"

  ; remove directories used.
  RMDir "$SMPROGRAMS\TA Demo"
  RMDir "$INSTDIR\docs"
  RMDir "$INSTDIR\bitmaps"
  RMDir "$INSTDIR"
SectionEnd

; Saker för att hantera våra custom dialogrutor

; $9 = counter
; $8 = DLL
; $7 = ini
; $6 = tadir
; $5 = 1 om man ska fixa pathar
Function .onInit
  StrCpy $9 0
  GetTempFileName $8
  GetTempFileName $7
  File /oname=$8 "InstallOptions.dll"
  File /oname=$7 "tadir.ini"

  ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\DirectPlay\Applications\Total Annihilation" "Path"
  IfErrors fel klart
  fel:
  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Total Annihilation" "Dir"  
  IfErrors fel2 klart 
  fel2:
  StrCpy $R0 "C:\Cavedog\Totala"

  klart:
  WriteINIStr $7 "Field 3" State $R0

  WriteINIStr $7 "Settings" Title "${VERSION} Installer: Specify TA directory"
FunctionEnd

; cleanup on exit.
Function .onInstSuccess
  Call Cleanup

  ExecWait '"$INSTDIR\vispatcher.exe" -silent'

  MessageBox MB_YESNO "The installation completed successfully. Would you like to read the documentation now?" IDNO NoReadme
    ExecShell "open" $INSTDIR\docs\readme.html ; view readme or whatever, if you want.
  NoReadme:
FunctionEnd

Function .onInstFailed
Call Cleanup
FunctionEnd

Function .onUserAbort
Call Cleanup
FunctionEnd

Function Cleanup
  Delete $8
  Delete $7
FunctionEnd

Function .onNextPage
  StrCmp $9 2 tadir
    IntOp $9 $9 + 1
    Return
  tadir:
  Call RunConfigure
  Pop $0
  StrCmp $0 "back" "" noback
    Abort
  noback:
  IntOp $9 $9 + 1
FunctionEnd

Function .onPrevPage
  StrCmp $9 3 good
    IntOp $9 $9 - 1
    Return
  good:
  Call RunConfigure
  Pop $0
  StrCmp $0 "back" back
    Abort
  back:
  IntOp $9 $9 - 1
FunctionEnd

; Lägger resultatet från CallInstDLL på stacken innan avslut
Function RunConfigure

  again:
  Push $7
  CallInstDLL $8 dialog
  Pop $R0

  ; Lägg in katalognamnet i global $6 och "fixa-pathar" i $5
  ReadINIStr $6 $7 "Field 3" State
  ReadINIStr $5 $7 "Field 5" State

  ; Om man tryckt på cancel så ska det avslutas
  StrCmp $R0 "cancel" "" nocancel
    Call Cleanup
    Quit
  nocancel:

  ; Trycker man på back så behöver man inte validera katalogen
  StrCmp $R0 "back" "" noback
    Push "back"
    Return 
  noback:

  ; Men trycker man på next måste man ha valt en giltig TA-katalog, annars körs väljsidan igen
  StrCpy $R1 $6\Totala.exe
  IfFileExists $R1 done
    MessageBox MB_OK|MB_ICONSTOP "The specified directory is not a valid TA directory. (It does not contain the file totala.exe). Please select a different directory."
    Goto again

  done:
  Push "next"
FunctionEnd
