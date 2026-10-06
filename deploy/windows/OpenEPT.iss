#ifndef MyAppVersion
  #define MyAppVersion "0.0.0"
#endif
#ifndef MyAppSourceDir
  #define MyAppSourceDir "GUI_Deploy"
#endif

#define MyAppName "OpenEPT"
#define MyAppPublisher "OpenEPT"
#define MyAppExeName "OpenEPT.exe"

[Setup]
AppId={{A945B938-45C4-45B4-BE28-04E5159AE220}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
SetupIconFile=..\..\Source\main.ico

DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}

OutputDir=Output
OutputBaseFilename=OpenEPT_Setup_{#MyAppVersion}

Compression=lzma2
SolidCompression=yes

ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

PrivilegesRequired=admin

CloseApplications=yes
RestartApplications=no

UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}

[Files]
Source: "{#MyAppSourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\OpenEPT"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\OpenEPT"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch OpenEPT"; Flags: nowait postinstall skipifsilent

[Code]
const
  AppUninstallKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{A945B938-45C4-45B4-BE28-04E5159AE220}_is1';

function GetUninstallString(): String;
var
  value: String;
begin
  value := '';
  if not RegQueryStringValue(HKLM, AppUninstallKey, 'UninstallString', value) then
    RegQueryStringValue(HKCU, AppUninstallKey, 'UninstallString', value);
  Result := value;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  uninstaller: String;
  resultCode: Integer;
  waited: Integer;
begin
  uninstaller := RemoveQuotes(GetUninstallString());

  if (uninstaller <> '') and FileExists(uninstaller) then
  begin
    if not Exec(uninstaller, '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART',
                '', SW_HIDE, ewWaitUntilTerminated, resultCode) then
    begin
      Result := 'Could not uninstall the previous OpenEPT version. '
              + 'Please remove it manually and run the installer again.';
      exit;
    end;

    waited := 0;
    while FileExists(uninstaller) and (waited < 100) do
    begin
      Sleep(200);
      waited := waited + 1;
    end;
  end;

  Result := '';
end;
