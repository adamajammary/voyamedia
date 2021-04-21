RequestExecutionLevel admin

!include "FileFunc.nsh"
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "WinVer.nsh"
!include "x64.nsh"

InstallDir "$PROGRAMFILES64\VoyaMedia"
Name       "__APP_NAME__ __APP_VERSION__"
OutFile    __SETUP_EXE__

ShowInstDetails   show
ShowUnInstDetails show

!insertmacro MUI_PAGE_WELCOME
!define MUI_PAGE_CUSTOMFUNCTION_SHOW KillProcess
!insertmacro MUI_PAGE_LICENSE "Release\docs\LICENSE.txt"
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_WELCOME
!define MUI_PAGE_CUSTOMFUNCTION_SHOW un.KillProcess
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "English"

Section Install
   SetOutPath "$INSTDIR"
   
   File /r Release\*
   
   ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
   
   IntFmt $0 "0x%08X" $0
   
   CreateShortCut  "$DESKTOP\__APP_NAME__.lnk" "$INSTDIR\VoyaMedia.exe"
   CreateDirectory "$SMPROGRAMS\__APP_NAME__"
   CreateShortCut  "$SMPROGRAMS\__APP_NAME__\__APP_NAME__.lnk" "$INSTDIR\VoyaMedia.exe"
   CreateShortCut  "$SMPROGRAMS\__APP_NAME__\Uninstall __APP_NAME__.lnk" "$INSTDIR\Uninstall.exe"
   
   WriteUninstaller "$INSTDIR\Uninstall.exe"
   
   WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__APP_NAME__" "DisplayIcon"     "$INSTDIR\VoyaMedia.exe"
   WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__APP_NAME__" "DisplayName"     "__APP_NAME__"
   WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__APP_NAME__" "DisplayVersion"  "__APP_VERSION__"
   WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__APP_NAME__" "EstimatedSize"   "$0"
   WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__APP_NAME__" "InstallLocation" "$INSTDIR"
   WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__APP_NAME__" "Publisher"       "__APP_COMPANY__"
   WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__APP_NAME__" "UninstallString" "$INSTDIR\Uninstall.exe"
   WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__APP_NAME__" "MajorVersion"    "__APP_VERSION_MAJOR__"
   WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__APP_NAME__" "MinorVersion"    "__APP_VERSION_MINOR__"
   WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__APP_NAME__" "VersionMajor"    "__APP_VERSION_MAJOR__"
   WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__APP_NAME__" "VersionMinor"    "__APP_VERSION_MINOR__"
SectionEnd

Section Uninstall
   Delete "$PROGRAMFILES64\VoyaMedia\Uninstall.exe" ; delete self
   Delete "$PROGRAMFILES64\VoyaMedia\docs\*"
   Delete "$PROGRAMFILES64\VoyaMedia\fonts\*"
   Delete "$PROGRAMFILES64\VoyaMedia\gui\*"
   Delete "$PROGRAMFILES64\VoyaMedia\img\*"
   Delete "$PROGRAMFILES64\VoyaMedia\lang\*"
   Delete "$PROGRAMFILES64\VoyaMedia\*"
   
   RMDir "$PROGRAMFILES64\VoyaMedia\docs"
   RMDir "$PROGRAMFILES64\VoyaMedia\fonts"
   RMDir "$PROGRAMFILES64\VoyaMedia\gui"
   RMDir "$PROGRAMFILES64\VoyaMedia\img"
   RMDir "$PROGRAMFILES64\VoyaMedia\lang"
   RMDir "$PROGRAMFILES64\VoyaMedia"
   
   Delete "$DESKTOP\__APP_NAME__.lnk"
   Delete "$SMPROGRAMS\__APP_NAME__\*.lnk"
   
   RMDir "$SMPROGRAMS\__APP_NAME__"
   
   DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\__APP_NAME__"
SectionEnd

Function KillProcess
   ${If} ${IsWinXP}
      ExecShell "open" "TSKILL.exe" "VoyaMedia /A /V" SW_HIDE
   ${Else}
      ExecShell "open" "TASKKILL.exe" "/F /IM VoyaMedia.exe /T" SW_HIDE
   ${EndIf}
FunctionEnd

Function un.KillProcess
   ${If} ${IsWinXP}
      ExecShell "open" "TSKILL.exe" "VoyaMedia /A /V" SW_HIDE
   ${Else}
      ExecShell "open" "TASKKILL.exe" "/F /IM VoyaMedia.exe /T" SW_HIDE
   ${EndIf}
FunctionEnd
