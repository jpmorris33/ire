# Microsoft Developer Studio Project File - Name="ire" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ire - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ire.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ire.mak" CFG="ire - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ire - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ire - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ire - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "\allegro\include" /I "..\..\jpeg\win" /I "\fmod\api\inc" /I "\lpng125" /I "\zlib" /I "\sqlite" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D ASM=\"i386\" /D "USE_CONSOLE" /D "USE_FMOD3" /D "USE_ALLEGRO4" /D "BGUI" /D "NO_FILESIZE_EX" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 user32.lib fmodvc.lib ithelib.lib bgui2.lib alleg.lib zlib.lib libpng.lib libjpeg.lib sqlite.lib /nologo /subsystem:console /machine:I386 /libpath:"\fmod\api\lib" /libpath:"..\..\..\libs\win" /libpath:"\sqlite"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "\allegro\include" /I "..\..\jpeg\win" /I "\fmod\api\inc" /I "\lpng125" /I "\zlib" /I "\sqlite" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D ASM=\"i386\" /D "USE_CONSOLE" /D "USE_FMOD3" /D "USE_ALLEGRO4" /D "BGUI" /D "NO_FILESIZE_EX" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib fmodvc.lib ithelib.lib bgui2.lib alleg.lib zlib.lib libpng.lib libjpeg.lib sqlite.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /pdbtype:sept /libpath:"\fmod\api\lib" /libpath:"..\..\..\libs\win" /libpath:"\sqlite"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ire - Win32 Release"
# Name "ire - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "audio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\audio\allegro.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\audio\fmod3.cpp
# End Source File
# End Group
# Begin Group "graphics"

# PROP Default_Filter ""
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\graphics\common\bitfont.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\common\blenders.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\common\bytefont.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\common\darkness.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\common\iconsole.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\common\ilightmap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\common\keybuf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\common\picklist.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\common\primitve.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\common\textedit.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\common\dark_32.obj
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\common\font_32.obj
# End Source File
# End Group
# Begin Group "allegro4"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\graphics\allegro4\ibitmap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\allegro4\icursor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\allegro4\ifont.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\allegro4\ikeys.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\allegro4\imain.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\allegro4\isprite.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\graphics\allegro4\istartup.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\graphics\iregraph.cpp
# End Source File
# End Group
# Begin Group "pe"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\pe\pe_keyw.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\pe\pe_lang.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\pe\pe_sys.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\pe\pe_vm.cpp
# End Source File
# End Group
# Begin Group "widgets"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\widgets\w_generic.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\console.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\database.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\gamedata.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\init.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\ire.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\library.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\linklist.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\loaders.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\loadfile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\loadpng.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\loadsave.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\mainloop.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\map.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\media.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\mouse.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\mymode.c
# End Source File
# Begin Source File

SOURCE=..\..\..\nuspeech.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\object.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\oscli.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\pathfind.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\project.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\resource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\slotalloc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\textfile.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\ire_icon.rc
# End Source File
# End Group
# End Target
# End Project
