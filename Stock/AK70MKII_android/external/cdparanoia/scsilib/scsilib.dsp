# Microsoft Developer Studio Project File - Name="scsilib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=scsilib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "scsilib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "scsilib.mak" CFG="scsilib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "scsilib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "scsilib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl.exe

!IF  "$(CFG)" == "scsilib - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "./include" /I "." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MT" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "scsilib - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "./include" /I "." /D "_DEBUG" /D "__CYGWIN32__" /D "_MT" /D "WIN32" /D "_WINDOWS" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "scsilib - Win32 Release"
# Name "scsilib - Win32 Debug"
# Begin Source File

SOURCE=.\include\align.h
# End Source File
# Begin Source File

SOURCE=".\scg\aspi-win32.h"
# End Source File
# Begin Source File

SOURCE=.\astoi.c
# End Source File
# Begin Source File

SOURCE=.\include\avoffset.h
# End Source File
# Begin Source File

SOURCE=.\include\btorder.h
# End Source File
# Begin Source File

SOURCE=.\comerr.c
# End Source File
# Begin Source File

SOURCE=.\cvt.c
# End Source File
# Begin Source File

SOURCE=.\include\deflts.h
# End Source File
# Begin Source File

SOURCE=.\include\device.h
# End Source File
# Begin Source File

SOURCE=.\error.c
# End Source File
# Begin Source File

SOURCE=.\fconv.c
# End Source File
# Begin Source File

SOURCE=.\include\fctldefs.h
# End Source File
# Begin Source File

SOURCE=.\fgetline.c
# End Source File
# Begin Source File

SOURCE=.\filewrite.c
# End Source File
# Begin Source File

SOURCE=.\fillbytes.c
# End Source File
# Begin Source File

SOURCE=.\flag.c
# End Source File
# Begin Source File

SOURCE=.\flush.c
# End Source File
# Begin Source File

SOURCE=.\format.c
# End Source File
# Begin Source File

SOURCE=.\include\getargs.h
# End Source File
# Begin Source File

SOURCE=.\geterrno.c
# End Source File
# Begin Source File

SOURCE=.\gettime.c
# End Source File
# Begin Source File

SOURCE=.\include\intcvt.h
# End Source File
# Begin Source File

SOURCE=.\include\io.h
# End Source File
# Begin Source File

SOURCE=.\jsprintf.c
# End Source File
# Begin Source File

SOURCE=.\jssnprintf.c
# End Source File
# Begin Source File

SOURCE=.\jssprintf.c
# End Source File
# Begin Source File

SOURCE=.\include\libport.h
# End Source File
# Begin Source File

SOURCE=.\include\mconfig.h
# End Source File
# Begin Source File

SOURCE=.\movebytes.c
# End Source File
# Begin Source File

SOURCE=.\my_config.h
# End Source File
# Begin Source File

SOURCE=.\port_lib.c
# End Source File
# Begin Source File

SOURCE=.\printf.c
# End Source File
# Begin Source File

SOURCE=.\include\prototyp.h
# End Source File
# Begin Source File

SOURCE=.\raisecond.c
# End Source File
# Begin Source File

SOURCE=.\saveargs.c
# End Source File
# Begin Source File

SOURCE=.\scg\scgcmd.h
# End Source File
# Begin Source File

SOURCE=.\scg\scgio.h
# End Source File
# Begin Source File

SOURCE=.\include\schily.h
# End Source File
# Begin Source File

SOURCE=.\scsi_cmds.c
# End Source File
# Begin Source File

SOURCE=.\scsi_cmds.h
# End Source File
# Begin Source File

SOURCE=.\scg\scsicdb.h
# End Source File
# Begin Source File

SOURCE=.\scg\scsidefs.h
# End Source File
# Begin Source File

SOURCE=.\scsierrs.c
# End Source File
# Begin Source File

SOURCE=.\scsilib.h
# End Source File
# Begin Source File

SOURCE=.\scsiopen.c
# End Source File
# Begin Source File

SOURCE=.\scg\scsireg.h
# End Source File
# Begin Source File

SOURCE=.\scg\scsisense.h
# End Source File
# Begin Source File

SOURCE=.\scsitransp.c
# End Source File
# Begin Source File

SOURCE=.\scg\scsitransp.h
# End Source File
# Begin Source File

SOURCE=.\include\sigblk.h
# End Source File
# Begin Source File

SOURCE=.\scg\srb_os2.h
# End Source File
# Begin Source File

SOURCE=.\include\standard.h
# End Source File
# Begin Source File

SOURCE=.\include\statdefs.h
# End Source File
# Begin Source File

SOURCE=.\include\stdxlib.h
# End Source File
# Begin Source File

SOURCE=.\include\stkframe.h
# End Source File
# Begin Source File

SOURCE=.\include\strdefs.h
# End Source File
# Begin Source File

SOURCE=.\streql.c
# End Source File
# Begin Source File

SOURCE=.\include\timedefs.h
# End Source File
# Begin Source File

SOURCE=.\include\unixstd.h
# End Source File
# Begin Source File

SOURCE=.\include\unls.h
# End Source File
# Begin Source File

SOURCE=.\usleep.c
# End Source File
# Begin Source File

SOURCE=.\include\utypes.h
# End Source File
# Begin Source File

SOURCE=.\include\vadefs.h
# End Source File
# Begin Source File

SOURCE=.\include\waitdefs.h
# End Source File
# Begin Source File

SOURCE=.\include\xconfig.h
# End Source File
# Begin Source File

SOURCE=.\include\xmconfig.h
# End Source File
# End Target
# End Project
