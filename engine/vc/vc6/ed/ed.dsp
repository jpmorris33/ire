# Microsoft Developer Studio Project File - Name="ed" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ed - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ed.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ed.mak" CFG="ed - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ed - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ed - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ed - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /GX /O2 /I "\allegro\include" /I ".\jpeg" /I "\fmod\api\inc" /I "\zlib" /I "\lpng125" /I "\sqlite" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "NO_ASM" /D "USE_CONSOLE" /D "USE_ALSOUND" /D "USE_ALLEGRO4" /D "WINDOWS_IS_RETARDED" /FD /c
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
# ADD LINK32 user32.lib fmodvc.lib ithelib.lib bgui2.lib alleg.lib zlib.lib libpng.lib libjpeg.lib sqlite.lib /nologo /subsystem:console /machine:I386 /out:"Release/ire-ed.exe" /libpath:"\fmod\api\lib" /libpath:"..\..\..\libs\win" /libpath:"\sqlite"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ed - Win32 Debug"

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
# ADD CPP /nologo /MT /W3 /Gm /GX /ZI /Od /I "\allegro\include" /I ".\jpeg" /I "\fmod\api\inc" /I "\zlib" /I "\lpng125" /I "\sqlite" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "NO_ASM" /D "USE_CONSOLE" /D "USE_ALSOUND" /D "USE_ALLEGRO4" /D "WINDOWS_IS_RETARDED" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib fmodvc.lib ithelib.lib bgui2.lib alleg.lib zlib.lib libpng.lib libjpeg.lib sqlite.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /out:"c:\ire\ed.exe" /pdbtype:sept /libpath:"\fmod\api\lib" /libpath:"..\..\..\libs\win" /libpath:"\sqlite"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ed - Win32 Release"
# Name "ed - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "audio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\audio\allegro.cpp
# End Source File
# End Group
# Begin Group "graphics"

# PROP Default_Filter ""
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
# End Group
# Begin Group "editor"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\editor\about.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\editor\backgrnd.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\editor\bigmap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\editor\editarea.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\editor\editor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\editor\gfx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\editor\igui.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\editor\lights.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\editor\objects.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\editor\pathgen.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\editor\roof.cpp
# End Source File
# End Group
# Begin Group "widgets"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\widgets\w_bill.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\console.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\database.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\dummy.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\dummy_c.c
# End Source File
# Begin Source File

SOURCE=..\..\..\gamedata.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\init.cpp
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

SOURCE=..\..\..\resource.cpp
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

SOURCE=.\ed_icon.rc
# End Source File
# End Group
# End Target
# End Project
