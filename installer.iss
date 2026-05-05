; Space Rock Breaker — Inno Setup script
; Plaats dit bestand in: C:\Users\thomas.drabbe\Documents\SpaceRockBreaker\

#define AppName      "Space Rock Breaker"
#define AppVersion   "1.0.1"
#define AppPublisher "Chef Survival"
#define AppExeName   "SpaceRockLauncher.exe"
#define GameExeName  "SpaceRockBreaker.exe"
#define SourceDir    "build\Release"
#define LauncherDir  "build\launcher\Release"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
OutputDir=installer_output
OutputBaseFilename=SpaceRockBreakerSetup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\assets\icon.ico
SetupIconFile=assets\icon.ico

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

; Assets inclusief font, icon en geluiden
Source: "{#SourceDir}\assets\*"; DestDir: "{app}\assets"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

; Icon apart zeker stellen
Source: "assets\icon.ico"; DestDir: "{app}\assets"; Flags: ignoreversion

[Icons]
; Startmenu
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; \
    IconFilename: "{app}\assets\icon.ico"
Name: "{group}\{#AppName} verwijderen"; Filename: "{uninstallexe}"

; Bureaublad (optioneel, vinkje in installer)
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; \
    IconFilename: "{app}\assets\icon.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; \
    Description: "Start {#AppName}"; \
    Flags: nowait postinstall skipifsilent