$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$cmake = Get-Content -Raw (Join-Path $root "CMakeLists.txt")
$readme = Get-Content -Raw (Join-Path $root "README.md")
$license = Get-Content -Raw (Join-Path $root "LICENSE.md")
$renderYaml = Get-Content -Raw (Join-Path $root "render.yaml")
$landingPackage = Get-Content -Raw (Join-Path $root "stashtrack-landing/package.json")
$landingNextConfig = Get-Content -Raw (Join-Path $root "stashtrack-landing/next.config.mjs")
$landingPage = Get-Content -Raw (Join-Path $root "stashtrack-landing/app/page.tsx")
$releaseInstallerUrl = "https://github.com/davad00/StashTrack/releases/download/v0.2/StashTrackv0.2Setup.exe"

function Assert-Contains {
    param(
        [string] $Text,
        [string] $Needle,
        [string] $Message
    )

    if (-not $Text.Contains($Needle)) {
        throw $Message
    }
}

Assert-Contains $cmake 'project(StashTrack VERSION 0.2.0)' 'CMake project must be named StashTrack with version 0.2.0.'
Assert-Contains $cmake 'juce_add_plugin(StashTrack' 'JUCE plug-in target must be named StashTrack.'
Assert-Contains $cmake 'COMPANY_NAME              "N9 Records"' 'JUCE company name must be N9 Records.'
Assert-Contains $cmake 'COMPANY_WEBSITE           "https://stashtrack.n9records.com"' 'JUCE company website must be stashtrack.n9records.com.'
Assert-Contains $cmake 'COMPANY_EMAIL             "vsts@n9records.com"' 'JUCE company email must be vsts@n9records.com.'
Assert-Contains $cmake 'COMPANY_COPYRIGHT         "Copyright (c) 2026 N9 Records"' 'JUCE copyright must be assigned to N9 Records.'
Assert-Contains $cmake 'BUNDLE_ID                 "com.n9records.stashtrack"' 'JUCE bundle ID must be stable and branded.'
Assert-Contains $cmake 'DESCRIPTION               "Download, clip, preview, and drag audio from yt-dlp-supported URLs into your DAW."' 'JUCE description must explain StashTrack.'
Assert-Contains $cmake 'PLUGIN_MANUFACTURER_CODE  Ycmp' 'JUCE manufacturer code must remain stable so FL Studio can reopen existing StashTrack entries.'
Assert-Contains $cmake 'PRODUCT_NAME              "StashTrack"' 'JUCE product name must be StashTrack.'
Assert-Contains $readme '# StashTrack JUCE Plug-in' 'README title must use StashTrack.'
Assert-Contains $readme 'Publisher: N9 Records' 'README must document the publisher.'
Assert-Contains $readme 'Website: https://stashtrack.n9records.com' 'README must document the website.'
Assert-Contains $readme 'Support: vsts@n9records.com' 'README must document the support email.'
Assert-Contains $readme 'Version: v0.2' 'README must document the preferred display version.'
Assert-Contains $readme 'License: StashTrack Non-Commercial License v0.1' 'README must document the custom non-commercial license.'
Assert-Contains $license '# StashTrack Non-Commercial License v0.1' 'License file must use the custom StashTrack license title.'
Assert-Contains $license 'No Commercial Use Or Profit' 'License file must prohibit commercial use and profit.'
Assert-Contains $license 'Nobody may profit from StashTrack or from any original StashTrack code' 'License file must explicitly forbid profit from StashTrack code.'
Assert-Contains $renderYaml 'runtime: node' 'Render Blueprint must deploy the landing page as a live Node web service.'
Assert-Contains $renderYaml 'plan: free' 'Render Blueprint must use the free tier.'
Assert-Contains $renderYaml 'rootDir: stashtrack-landing' 'Render Blueprint must build from the landing page folder.'
Assert-Contains $renderYaml 'buildCommand: bun install --frozen-lockfile && bun run build' 'Render Blueprint must use Bun for the landing page build.'
Assert-Contains $renderYaml 'startCommand: bun run start -- -H 0.0.0.0 -p $PORT' 'Render Blueprint must start Next on Render PORT.'
Assert-Contains $renderYaml 'healthCheckPath: /' 'Render Blueprint must health check the landing page root.'
Assert-Contains $renderYaml 'stashtrack.n9records.com' 'Render Blueprint must configure the StashTrack subdomain.'
Assert-Contains $landingPackage '"packageManager": "bun@' 'Landing package must declare Bun as the package manager.'
Assert-Contains $landingPackage '"build": "next build"' 'Landing package must expose the Next build script.'
Assert-Contains $landingPackage '"start": "next start"' 'Landing package must expose the Next start script.'
Assert-Contains $landingPage $releaseInstallerUrl 'Landing page must download the v0.2 installer from the GitHub Release asset.'

if ($renderYaml.Contains('runtime: static') -or $renderYaml.Contains('staticPublishPath:')) {
    throw 'Render Blueprint must not be configured as a static site.'
}

if ($landingNextConfig.Contains("output: 'export'")) {
    throw 'Landing Next config must not use static export for the live Render service.'
}

if (Get-ChildItem (Join-Path $root "stashtrack-landing/public") -Recurse -Filter "*.exe" -ErrorAction SilentlyContinue) {
    throw "Landing page must not commit installer binaries; use GitHub Releases for downloads."
}

$windowsInstaller = Join-Path $root "installer/install-stashtrack.ps1"
$unixInstaller = Join-Path $root "installer/install-stashtrack.sh"
$windowsExeInstallerScript = Join-Path $root "installer/windows/StashTrack.iss"
$windowsExeInstallerBuilder = Join-Path $root "installer/windows/build-installer.ps1"

if (-not (Test-Path $windowsInstaller)) {
    throw "Windows installer script is missing."
}

if (-not (Test-Path $unixInstaller)) {
    throw "macOS/Linux installer script is missing."
}

if (-not (Test-Path $windowsExeInstallerScript)) {
    throw "Windows EXE installer script is missing."
}

if (-not (Test-Path $windowsExeInstallerBuilder)) {
    throw "Windows EXE installer build script is missing."
}

$dryRunOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $windowsInstaller -DryRun -Configuration Release

if ($LASTEXITCODE -ne 0) {
    throw "Windows installer dry run failed with exit code $LASTEXITCODE."
}

$dryRunText = $dryRunOutput -join "`n"

if ($dryRunText -notmatch "StashTrack\.vst3") {
    throw "Windows installer dry run must reference StashTrack.vst3."
}

if ($dryRunText -notmatch [regex]::Escape((Join-Path $env:CommonProgramFiles "VST3\StashTrack.vst3"))) {
    throw "Windows installer must default to the system Common Files VST3 folder."
}

$userDryRunOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $windowsInstaller -DryRun -Configuration Release -User

if ($LASTEXITCODE -ne 0) {
    throw "Windows user-local installer dry run failed with exit code $LASTEXITCODE."
}

if (($userDryRunOutput -join "`n") -notmatch [regex]::Escape((Join-Path $env:LOCALAPPDATA "Programs\Common\VST3\StashTrack.vst3"))) {
    throw "Windows installer -User mode must target the user-local VST3 folder."
}

if ($dryRunText -notmatch "uv\.exe") {
    throw "Windows installer dry run must mention uv.exe availability."
}

if ($dryRunText -notmatch "install uv") {
    throw "Windows installer dry run must mention installing uv when it is missing."
}

$innoScript = Get-Content -Raw $windowsExeInstallerScript

foreach ($required in @(
    'AppName=StashTrack',
    '#define AppVersion "v0.2"',
    '#define AppVersionNumeric "0.2.0"',
    '#define AppPublisher "N9 Records"',
    '#define AppURL "https://stashtrack.n9records.com"',
    '#define AppSupportEmail "vsts@n9records.com"',
    'AppContact={#AppSupportEmail}',
    'AppCopyright=Copyright (c) 2026 N9 Records',
    'UninstallFilesDir={commonappdata}\N9 Records\StashTrack\Uninstall',
    'DefaultDirName={commoncf}\VST3\StashTrack.vst3',
    'PrivilegesRequired=admin',
    'VersionInfoCopyright=Copyright (c) 2026 N9 Records',
    'Source: "staging\Payload\StashTrack.vst3\*"',
    'uv.exe',
    'uvx.exe',
    'ffmpeg.exe',
    'deno.exe',
    'VC_redist.x64.exe',
    'Check: ShouldInstallVcRuntime',
    'function ShouldInstallVcRuntime(): Boolean',
    'RegQueryDWordValue(HKLM64'
)) {
    Assert-Contains $innoScript $required "Inno Setup script is missing required text: $required"
}

$builderScript = Get-Content -Raw $windowsExeInstallerBuilder

foreach ($required in @(
    'ffmpeg-release-essentials.zip',
    'ffmpeg-release-essentials.zip.sha256',
    'deno-x86_64-pc-windows-msvc.zip',
    'github.com/denoland/deno/releases/latest/download',
    '$minor -ge 8',
    'astral.sh/uv/install.ps1',
    'innosetup-6.7.3.exe',
    'vc_redist.x64.exe',
    '$vcRedistCache = Join-Path $cache "vc_redist.x64.exe"',
    'StashTrackv0.2Setup.exe',
    'ISCC.exe'
)) {
    Assert-Contains $builderScript $required "Windows EXE installer build script is missing required text: $required"
}

Write-Host "Packaging tests passed."
