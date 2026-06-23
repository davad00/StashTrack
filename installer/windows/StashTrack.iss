#define AppName "StashTrack"
#define AppVersion "v0.2"
#define AppVersionNumeric "0.2.0"
#define AppPublisher "N9 Records"
#define AppURL "https://stashtrack.n9records.com"
#define AppSupportEmail "vsts@n9records.com"
#define AppExeName "StashTrack.vst3"

[Setup]
AppId={{B37088CC-7F94-4F0C-8F77-50A6E1212E55}
AppName=StashTrack
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
AppContact={#AppSupportEmail}
AppCopyright=Copyright (c) 2026 N9 Records
DefaultDirName={commoncf}\VST3\StashTrack.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallFilesDir={commonappdata}\N9 Records\StashTrack\Uninstall
OutputDir=..\..\dist
OutputBaseFilename=StashTrackv0.2Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
WizardSizePercent=110
UninstallDisplayName=StashTrack VST3 Plug-in
UninstallDisplayIcon={app}\Contents\x86_64-win\StashTrack.vst3
VersionInfoVersion={#AppVersionNumeric}
VersionInfoCompany={#AppPublisher}
VersionInfoCopyright=Copyright (c) 2026 N9 Records
VersionInfoDescription=StashTrack VST3 Plug-in Installer
VersionInfoProductName=StashTrack
VersionInfoProductVersion={#AppVersionNumeric}
LicenseFile=staging\LICENSE.txt
InfoAfterFile=staging\README-INSTALL.txt

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "staging\Payload\StashTrack.vst3\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "staging\Tools\uv.exe"; DestDir: "{app}\Contents\x86_64-win"; Flags: ignoreversion
Source: "staging\Tools\uvx.exe"; DestDir: "{app}\Contents\x86_64-win"; Flags: ignoreversion
Source: "staging\Tools\ffmpeg.exe"; DestDir: "{app}\Contents\x86_64-win"; Flags: ignoreversion
Source: "staging\Tools\deno.exe"; DestDir: "{app}\Contents\x86_64-win"; Flags: ignoreversion
Source: "staging\Runtimes\VC_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall
Source: "staging\README-INSTALL.txt"; DestDir: "{app}\Contents\Resources"; Flags: ignoreversion
Source: "staging\LICENSE.txt"; DestDir: "{app}\Contents\Resources"; Flags: ignoreversion
Source: "staging\THIRD-PARTY-NOTICES.txt"; DestDir: "{app}\Contents\Resources"; Flags: ignoreversion

[InstallDelete]
Type: filesandordirs; Name: "{app}"

[Dirs]
Name: "{commoncf}\VST3"

[Icons]
Name: "{autoprograms}\StashTrack\Uninstall StashTrack"; Filename: "{uninstallexe}"

[Code]
function IsVcRuntimeInstalled(): Boolean;
var
  installed: Cardinal;
begin
  Result := False;

  if RegQueryDWordValue(HKLM64,
       'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64',
       'Installed',
       installed) then
  begin
    Result := installed = 1;
  end;
end;

function ShouldInstallVcRuntime(): Boolean;
begin
  Result := not IsVcRuntimeInstalled();
end;

[Run]
Filename: "{tmp}\VC_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Microsoft Visual C++ runtime..."; Flags: waituntilterminated; Check: ShouldInstallVcRuntime
Filename: "{cmd}"; Parameters: "/c echo StashTrack installed to ""{app}"""; Flags: runhidden
