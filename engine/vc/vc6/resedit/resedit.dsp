# Microsoft Developer Studio Project File - Name="resedit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=resedit - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "resedit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "resedit.mak" CFG="resedit - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "resedit - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "resedit - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "resedit - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "USE_CONSOLE" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "resedit - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "j:\djgpp\allegro\include" /I ".\jpeg" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "NO_ASM" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "resedit - Win32 Release"
# Name "resedit - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\resedit\Chars.cpp
# End Source File
# Begin Source File

SOURCE=..\..\resedit\Code.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console.c
# End Source File
# Begin Source File

SOURCE=..\..\Dummy.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Dummy_c.c
# End Source File
# Begin Source File

SOURCE=..\..\Gamedata.c
# End Source File
# Begin Source File

SOURCE=..\..\Gui\Gfx.c
# End Source File
# Begin Source File

SOURCE=..\..\resedit\Grok.c
# End Source File
# Begin Source File

SOURCE=..\..\Gui\Igui.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Init.c
# End Source File
# Begin Source File

SOURCE=..\..\resedit\Lightmap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Linklist.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Loaders.c
# End Source File
# Begin Source File

SOURCE=..\..\Loadfile.c
# End Source File
# Begin Source File

SOURCE=..\..\Map.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Media.c
# End Source File
# Begin Source File

SOURCE=..\..\resedit\Music.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mymode.c
# End Source File
# Begin Source File

SOURCE=..\..\Nuspeech.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Object.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Oscli.c
# End Source File
# Begin Source File

SOURCE=..\..\Pe\Pe_lang.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Pe\Pe_sys.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Rar.c
# End Source File
# Begin Source File

SOURCE=..\..\resedit\Resedit.cpp
# End Source File
# Begin Source File

SOURCE=..\..\resedit\Rooftils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\resedit\S_about.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Script.cpp
# End Source File
# Begin Source File

SOURCE=..\..\resedit\Seqs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\resedit\Sprites.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Textfile.c
# End Source File
# Begin Source File

SOURCE=..\..\resedit\Tiles.cpp
# End Source File
# Begin Source File

SOURCE=..\..\resedit\Wavs.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res_icon.rc
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\Libs\Win\alleg.lib
# End Source File
# Begin Source File

SOURCE=..\..\libs\win\ithelib.lib
# End Source File
# Begin Source File

SOURCE=..\..\Libs\Win\jpeg.lib
# End Source File
# Begin Source File

SOURCE=..\..\libs\win\bgui.lib
# End Source File
# End Target
# End Project
