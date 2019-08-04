# Microsoft Developer Studio Project File - Name="ire" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=IRE - WIN32 RELEASE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ire.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ire.mak" CFG="IRE - WIN32 RELEASE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ire - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ire - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\SDL-0.9.13\INCLUDE" /I "..\..\midas\include" /I "..\..\midas\src\midas" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib ..\itg\win32\itg.lib ..\..\seer\lib\seer.lib ..\..\SDL-0.9.13\lib\sdl.lib ddraw.lib dxguid.lib ..\..\midas\lib\win32\vcretail\midas.lib ..\..\midas\lib\win32\vcretail\midasdll.lib /nologo /subsystem:console /machine:I386 /out:"..\irew.exe"
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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /ML /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "NO_ASM" /YX /FD /I /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:console /debug /machine:I386 /out:"..\irew.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "ire - Win32 Release"
# Name "ire - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\Alsound.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Console.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Darken_c.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Fortifyj.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Gamedata.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Init.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Ire.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Linklist.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Loaders.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Loadfile.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Loadsave.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Mainloop.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Map.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Media.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Mymode.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Nuspeech.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Object.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Oscli.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Pathfind.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Pe\Pe_lang.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Pe\Pe_sys.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Pe\Pe_vm.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Project.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Rar.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Script.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Seer.cpp

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Textfile.c

!IF  "$(CFG)" == "ire - Win32 Release"

!ELSEIF  "$(CFG)" == "ire - Win32 Debug"

# ADD CPP /I "j:\djgpp\allegro\include"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=..\Libs\Win\WSeeRC.lib
# End Source File
# Begin Source File

SOURCE=..\Libs\Win\jpeg.lib
# End Source File
# Begin Source File

SOURCE=..\Libs\Win\libithe.lib
# End Source File
# Begin Source File

SOURCE=..\Libs\Win\alleg.lib
# End Source File
# End Target
# End Project
