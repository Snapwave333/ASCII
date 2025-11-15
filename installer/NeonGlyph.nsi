!define APPNAME "NeonGlyph Director"
!define COMPANY "NeonGlyph Studios"
!define APPVERSION "1.0.0"
!define APPEXE "NeonGlyph.exe"

OutFile "NeonGlyphDirectorSetup.exe"
InstallDir "$PROGRAMFILES64\${COMPANY}\${APPNAME}"
ShowInstDetails show
ShowUninstDetails show

Section "Install"
  SetOutPath "$INSTDIR"
  File /r "build64\Release\NeonGlyph.exe"
  File /r "assets\branding\*.*"
  File /r "config\themes\*.*"
  File /r "config\default.json"
  File /r "config\palettes.json"
  File /r "config\charsets.json"
  File /r "config\llm_role_prompt.txt"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Publisher" "${COMPANY}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$INSTDIR\Uninstall.exe"
  CreateShortCut "$SMPROGRAMS\${APPNAME}.lnk" "$INSTDIR\${APPEXE}"
SectionEnd

Section "Uninstall"
  Delete "$SMPROGRAMS\${APPNAME}.lnk"
  Delete "$INSTDIR\${APPEXE}"
  RMDir /r "$INSTDIR"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
SectionEnd
