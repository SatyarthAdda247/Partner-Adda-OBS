; =============================================================================
; Adda Partner - Complete Standalone Installer
; Bundles: OBS Studio 32.1.2 + QR Plugin + AddaPartner Launcher
; Handles: Chrome/OBS process conflicts, silent OBS install, branding
; =============================================================================

#define AppName        "Adda Partner"
#define AppVersion     "1.6"
#define AppPublisher   "Adda247"
#define AppURL         "https://adda247.com"
#define OBSVersion     "32.1.2"
#define OBSInstaller   "OBS-Studio-32.1.2-Windows-x64-Installer.exe"
#define OBSInstallDir  "{commonpf64}\obs-studio"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}

; Install into OBS's standard directory so OBS and plugin share the same tree
DefaultDirName={#OBSInstallDir}
DefaultGroupName={#AppName}
DisableDirPage=yes
DisableProgramGroupPage=yes

; Output
OutputDir=Output
OutputBaseFilename=AddaPartner_Setup_v{#AppVersion}

; Branding
SetupIconFile=Partnerlogo.ico
WizardImageFile=Partnerlogo_wizard.png
WizardSmallImageFile=Partnerlogo_small.png
WizardImageStretch=yes
WizardStyle=modern

; Compression
Compression=lzma2/ultra64
SolidCompression=yes

; Requirements
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin

; Uninstall
UninstallDisplayName={#AppName}
UninstallDisplayIcon={#OBSInstallDir}\bin\64bit\AddaPartner.exe

ShowLanguageDialog=no
; Reserve 700MB for OBS + plugin
ExtraDiskSpaceRequired=734003200

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
WelcomeLabel1=Welcome to [name] Setup
WelcomeLabel2=This will install Adda Partner (powered by OBS Studio {#OBSVersion}) on your computer.%n%nAdda Partner includes:%n  %n  • OBS Studio {#OBSVersion} (full install)%n  • QR Code stream-key authentication plugin%n  • Adda Partner branded launcher%n%nPlease close Chrome and any running OBS windows before proceeding.%n%nClick Next to continue.
FinishedHeadingLabel=Adda Partner Installation Complete!
FinishedLabel=Adda Partner has been successfully installed.%n%nTo get started:%n  1. Launch Adda Partner from your Desktop%n  2. Go to Tools → Adda Partner Login%n  3. Scan the QR code with your mobile app%n%nClick Finish to exit Setup.

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut for Adda Partner"; GroupDescription: "Additional options:"
Name: "launchapp";   Description: "&Launch Adda Partner after installation";   GroupDescription: "Additional options:"

[Files]
; ── OBS Studio installer (extracted to temp, run silently) ───────────────────
Source: "{#OBSInstaller}"; \
  DestDir: "{tmp}"; \
  Flags: deleteafterinstall

; ── Adda Partner launcher (sits alongside obs64.exe) ────────────────────────
Source: "AddaPartner.exe"; \
  DestDir: "{app}\bin\64bit"; \
  Flags: ignoreversion

; ── QR plugin DLL ────────────────────────────────────────────────────────────
Source: "AddaPartnerInstaller_v1.0.0\plugin\obs-qr-stream-key.dll"; \
  DestDir: "{app}\obs-plugins\64bit"; \
  Flags: ignoreversion

; ── Plugin locale ─────────────────────────────────────────────────────────────
Source: "plugins\obs-qr-stream-key\data\locale\en-US.ini"; \
  DestDir: "{app}\data\obs-plugins\obs-qr-stream-key\locale"; \
  Flags: ignoreversion

; ── OpenSSL DLLs (required by QR plugin) ─────────────────────────────────────
Source: "AddaPartnerInstaller_v1.0.0\openssl\libssl-4-x64.dll"; \
  DestDir: "{app}\bin\64bit"; \
  Flags: ignoreversion
Source: "AddaPartnerInstaller_v1.0.0\openssl\libcrypto-4-x64.dll"; \
  DestDir: "{app}\bin\64bit"; \
  Flags: ignoreversion

; ── Branding logos ────────────────────────────────────────────────────────────
Source: "Partnerlogo.png"; \
  DestDir: "{app}"; \
  Flags: ignoreversion
Source: "Partnerlogo.ico"; \
  DestDir: "{app}"; \
  Flags: ignoreversion

[Dirs]
Name: "{app}\obs-plugins\64bit"
Name: "{app}\data\obs-plugins\obs-qr-stream-key\locale"

[Icons]
; Desktop shortcut
Name: "{commondesktop}\Adda Partner"; \
  Filename: "{app}\bin\64bit\AddaPartner.exe"; \
  WorkingDir: "{app}\bin\64bit"; \
  Tasks: desktopicon

; Start Menu
Name: "{group}\Adda Partner"; \
  Filename: "{app}\bin\64bit\AddaPartner.exe"; \
  WorkingDir: "{app}\bin\64bit"
Name: "{group}\Uninstall Adda Partner"; \
  Filename: "{uninstallexe}"

[Run]
; ── Step 1: Install OBS Studio via PowerShell (avoids CallSpawnServer bug) ───
; Using Start-Process -Wait properly handles NSIS sub-installer process spawning
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; \
  Parameters: "-ExecutionPolicy Bypass -WindowStyle Hidden -Command ""Start-Process -FilePath '{tmp}\{#OBSInstaller}' -ArgumentList '/S' -Wait"""; \
  StatusMsg: "Installing OBS Studio {#OBSVersion}... please wait (this may take a minute)"; \
  Flags: waituntilterminated runhidden

; ── Step 2: Launch Adda Partner after install (optional) ─────────────────────
Filename: "{app}\bin\64bit\AddaPartner.exe"; \
  Description: "Launch Adda Partner now"; \
  Flags: nowait postinstall skipifsilent shellexec; \
  Tasks: launchapp

[UninstallDelete]
Type: files;      Name: "{app}\bin\64bit\AddaPartner.exe"
Type: files;      Name: "{app}\obs-plugins\64bit\obs-qr-stream-key.dll"
Type: files;      Name: "{app}\data\obs-plugins\obs-qr-stream-key\locale\en-US.ini"
Type: dirifempty; Name: "{app}\data\obs-plugins\obs-qr-stream-key\locale"
Type: dirifempty; Name: "{app}\data\obs-plugins\obs-qr-stream-key"

[Code]
// ─────────────────────────────────────────────────────────────────────────────
// Helper: Kill a process by name, returns true if it was running
// ─────────────────────────────────────────────────────────────────────────────
function KillProcess(ProcessName: String): Boolean;
var
  ResultCode: Integer;
begin
  Result := Exec('taskkill', '/F /IM "' + ProcessName + '" /T',
                 '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

// ─────────────────────────────────────────────────────────────────────────────
// InitializeSetup: Kill conflicting processes BEFORE anything is extracted
// ─────────────────────────────────────────────────────────────────────────────
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
  Response: Integer;
begin
  Result := True;

  // Kill OBS if running
  Exec('taskkill', '/F /IM obs64.exe /T', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Exec('taskkill', '/F /IM obs32.exe /T', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Exec('taskkill', '/F /IM obs.exe /T',   '', SW_HIDE, ewWaitUntilTerminated, ResultCode);

  // Kill Chrome (it can hold OBS virtual camera DLLs)
  Response := MsgBox(
    'Adda Partner Setup needs to close Google Chrome temporarily.' + #13#10 +
    'Chrome holds OBS camera files that would block installation.' + #13#10 + #13#10 +
    'Please save any open work in Chrome, then click OK.' + #13#10 +
    '(Chrome will be closed automatically when you click OK)',
    mbInformation, MB_OKCANCEL);

  if Response = IDCANCEL then
  begin
    Result := False;
    Exit;
  end;

  // Close Chrome gracefully first, then force kill
  Exec('taskkill', '/IM chrome.exe',        '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(1500);
  Exec('taskkill', '/F /IM chrome.exe /T',  '', SW_HIDE, ewWaitUntilTerminated, ResultCode);

  // Also kill any OBS browser plugin processes
  Exec('taskkill', '/F /IM obs-browser-page.exe /T', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);

  Sleep(1000);
end;

// ─────────────────────────────────────────────────────────────────────────────
// CurStepChanged: Post-install verification only
// (OBS install is handled by [Run] section, after files are extracted)
// ─────────────────────────────────────────────────────────────────────────────
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    if FileExists(ExpandConstant('{app}\obs-plugins\64bit\obs-qr-stream-key.dll')) then
      Log('QR plugin installed OK')
    else
      Log('WARNING: QR plugin not found after install!');

    if FileExists(ExpandConstant('{app}\bin\64bit\AddaPartner.exe')) then
      Log('AddaPartner.exe launcher installed OK')
    else
      Log('WARNING: AddaPartner.exe not found after install!');
  end;
end;
