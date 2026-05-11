; Space Rock Breaker — Inno Setup script
; Project root: elk pad hieronder is relatief t.o.v. deze .iss naast CMakeLists.txt
;
; Versie (#define AppVersion):
; - Wordt automatisch gesynchroniseerd met version.txt tijdens CMake build
;   (zie cmake/bump_app_version.cmake, optie SRB_AUTO_BUMP_VERSION_ON_BUILD).
; - Handmatig Inno-compilen? Zorg dat deze string gelijk blijft aan version.txt.

#define AppName      "Space Rock Breaker"
#define AppVersion   "1.0.98"
#define AppPublisher "Chef Survival"
#define AppExeName   "SpaceRockLauncher.exe"
#define GameExeName  "SpaceRockBreaker.exe"
#define SourceDir    "build\Release"
#define LauncherDir  "build\launcher\Release"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=commandline
CloseApplications=yes
RestartApplications=no
UninstallRestartComputer=false

; x64 build (CMake preset / vcpkg x64-windows)
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64

; Altijd per-user locatie, zodat launcher-updates en paden consistent blijven.
DefaultDirName={localappdata}\Programs\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=no
AllowNetworkDrive=no

OutputDir=installer_output
OutputBaseFilename=SpaceRockBreakerSetup_{#AppVersion}

Compression=lzma2
SolidCompression=yes
WizardStyle=modern

UninstallDisplayIcon={app}\assets\icon.ico
SetupIconFile=assets\icon.ico

; Windows-installer uninstall-weergave
VersionInfoCompany={#AppPublisher}
VersionInfoProductName={#AppName}
VersionInfoVersion={#AppVersion}.0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "dutch";   MessagesFile: "compiler:Languages\Dutch.isl"

[Tasks]
Name: "desktopicon"; Description: "Maak een snelkoppeling op het bureaublad"; \
    GroupDescription: "Extra snelkoppelingen:"; Flags: unchecked

[Files]
; Launcher + game
Source: "{#LauncherDir}\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\{#GameExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "version.txt"; DestDir: "{app}"; Flags: ignoreversion

; SFML en andere DLLs
Source: "{#SourceDir}\*.dll"; DestDir: "{app}"; Flags: ignoreversion

; Assets inclusief font, icon en geluiden (build kopie naast exe)
Source: "{#SourceDir}\assets\*"; DestDir: "{app}\assets"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

; Icoon uit repo als dat in build\Release\assets nog ontbreekt
Source: "assets\icon.ico"; DestDir: "{app}\assets"; \
    Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; \
    IconFilename: "{app}\assets\icon.ico"
Name: "{group}\{#AppName} verwijderen"; Filename: "{uninstallexe}"

Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; \
    IconFilename: "{app}\assets\icon.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; \
    Description: "Start {#AppName}"; \
    Flags: nowait postinstall skipifsilent
