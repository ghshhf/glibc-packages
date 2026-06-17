; SkyNet NSIS Installer Extensions
; Adds custom branding and post-install steps

Section "SkyNet SSI Runtime"
  SectionIn RO

  ; Create log directory
  CreateDirectory "$APPDATA\SkyNet\logs"

  ; Write uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; Registry for Add/Remove Programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SkyNet" \
    "DisplayName" "SkyNet SSI Runtime"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SkyNet" \
    "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SkyNet" \
    "DisplayIcon" "$INSTDIR\SkyNet SSI Runtime.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SkyNet" \
    "DisplayVersion" "1.0.0"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SkyNet" \
    "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SkyNet" \
    "NoRepair" 1
SectionEnd
