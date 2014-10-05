# Microsoft Developer Studio Project File - Name="ddraw" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ddraw - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ddraw.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ddraw.mak" CFG="ddraw - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ddraw - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ddraw - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ddraw - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DDRAW_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DDRAW_EXPORTS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x41d /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "ddraw - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DDRAW_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DDRAW_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x41d /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"d:\cavedog\totala\ddraw.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ddraw - Win32 Release"
# Name "ddraw - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "3dta"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dddta.cpp

!IF  "$(CFG)" == "ddraw - Win32 Release"

!ELSEIF  "$(CFG)" == "ddraw - Win32 Debug"

# ADD CPP /FAs

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dddta.h
# End Source File
# Begin Source File

SOURCE=.\MinimapHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\minimaphandler.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\bookmarks.cpp
# End Source File
# Begin Source File

SOURCE=.\cgraphic.cpp
# End Source File
# Begin Source File

SOURCE=.\changequeue.cpp
# End Source File
# Begin Source File

SOURCE=.\cinomce.cpp
# End Source File
# Begin Source File

SOURCE=.\commanderwarp.cpp
# End Source File
# Begin Source File

SOURCE=.\ddraw.cpp
# End Source File
# Begin Source File

SOURCE=.\ddraw.rc
# End Source File
# Begin Source File

SOURCE=.\dialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ElementHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\font.cpp
# End Source File
# Begin Source File

SOURCE=.\iddraw.cpp
# End Source File
# Begin Source File

SOURCE=.\iddrawsurface.cpp
# End Source File
# Begin Source File

SOURCE=.\idlevillager.cpp
# End Source File
# Begin Source File

SOURCE=.\maprect.cpp
# End Source File
# Begin Source File

SOURCE=.\mkiddraw.cpp
# End Source File
# Begin Source File

SOURCE=.\mkiddrawsurface.cpp
# End Source File
# Begin Source File

SOURCE=.\PCX.CPP
# End Source File
# Begin Source File

SOURCE=.\pcxread.cpp
# End Source File
# Begin Source File

SOURCE=.\rings.cpp
# End Source File
# Begin Source File

SOURCE=.\tahook.cpp
# End Source File
# Begin Source File

SOURCE=.\unitrotate.cpp
# End Source File
# Begin Source File

SOURCE=.\weaponid.cpp
# End Source File
# Begin Source File

SOURCE=.\whiteboard.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\bookmarks.h
# End Source File
# Begin Source File

SOURCE=.\cgraphic.h
# End Source File
# Begin Source File

SOURCE=.\changequeue.h
# End Source File
# Begin Source File

SOURCE=.\cincome.h
# End Source File
# Begin Source File

SOURCE=.\commanderwarp.h
# End Source File
# Begin Source File

SOURCE=.\dialog.h
# End Source File
# Begin Source File

SOURCE=.\ElementHandler.h
# End Source File
# Begin Source File

SOURCE=.\font.h
# End Source File
# Begin Source File

SOURCE=.\iddraw.h
# End Source File
# Begin Source File

SOURCE=.\iddrawsurface.h
# End Source File
# Begin Source File

SOURCE=.\idlevillager.h
# End Source File
# Begin Source File

SOURCE=.\IRenderer.h
# End Source File
# Begin Source File

SOURCE=.\maprect.h
# End Source File
# Begin Source File

SOURCE=.\mkiddraw.h
# End Source File
# Begin Source File

SOURCE=.\mkiddrawsurface.h
# End Source File
# Begin Source File

SOURCE=.\oddraw.h
# End Source File
# Begin Source File

SOURCE=.\PCX.H
# End Source File
# Begin Source File

SOURCE=.\pcxread.h
# End Source File
# Begin Source File

SOURCE=.\rings.h
# End Source File
# Begin Source File

SOURCE=.\tafunctions.h
# End Source File
# Begin Source File

SOURCE=.\tahook.h
# End Source File
# Begin Source File

SOURCE=.\tamem.h
# End Source File
# Begin Source File

SOURCE=.\unitrotate.h
# End Source File
# Begin Source File

SOURCE=.\weaponid.h
# End Source File
# Begin Source File

SOURCE=.\whiteboard.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\bitmaps\3stagedbutton.pcx
# End Source File
# Begin Source File

SOURCE=.\bitmaps\CheckBox.pcx
# End Source File
# Begin Source File

SOURCE=.\bitmaps\ControPanelBG.pcx
# End Source File
# Begin Source File

SOURCE=.\ddraw.def
# End Source File
# Begin Source File

SOURCE=.\bitmaps\greenrect.pcx
# End Source File
# Begin Source File

SOURCE=.\bitmaps\hatfontLC.pcx
# End Source File
# Begin Source File

SOURCE=.\bitmaps\hatfontsmallLC.pcx
# End Source File
# Begin Source File

SOURCE=.\bitmaps\hatfontsmallUC.pcx
# End Source File
# Begin Source File

SOURCE=.\bitmaps\hatfontUC.pcx
# End Source File
# Begin Source File

SOURCE=.\bitmaps\inputbox.pcx
# End Source File
# Begin Source File

SOURCE=.\bitmaps\okbutton.pcx
# End Source File
# Begin Source File

SOURCE=.\bitmaps\smallcircle.pcx
# End Source File
# Begin Source File

SOURCE=.\bitmaps\StagedButtn1.pcx
# End Source File
# Begin Source File

SOURCE=.\bitmaps\standarbutton.pcx
# End Source File
# Begin Source File

SOURCE=.\bitmaps\tacursor.pcx
# End Source File
# End Target
# End Project
