[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string] $Configuration = "Release",

    [string] $BuildDir,

    [switch] $SkipVstBuild,

    [switch] $ForceDownloads
)

$ErrorActionPreference = "Stop"

if (-not [Environment]::Is64BitOperatingSystem) {
    throw "StashTrack's Windows installer is x64-only and must be built on a 64-bit Windows installation."
}

$installerRoot = Split-Path -Parent $PSCommandPath
$root = Resolve-Path (Join-Path $installerRoot "..\..")

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $root "build-vs"
}

$staging = Join-Path $installerRoot "staging"
$cache = Join-Path $installerRoot "cache"
$payload = Join-Path $staging "Payload"
$tools = Join-Path $staging "Tools"
$runtimes = Join-Path $staging "Runtimes"
$dist = Join-Path $root "dist"

$pluginSource = Join-Path $BuildDir "StashTrack_artefacts\$Configuration\VST3\StashTrack.vst3"
$pluginDestination = Join-Path $payload "StashTrack.vst3"
$setupOutput = Join-Path $dist "StashTrackv0.2Setup.exe"

function Get-FullPath {
    param([string] $Path)
    return [System.IO.Path]::GetFullPath($Path)
}

function Assert-PathIsInside {
    param(
        [string] $Path,
        [string] $Parent
    )

    $fullPath = Get-FullPath $Path
    $fullParent = Get-FullPath $Parent

    if (-not $fullParent.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $fullParent += [System.IO.Path]::DirectorySeparatorChar
    }

    if (-not $fullPath.StartsWith($fullParent, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to modify path outside expected folder. Path: $fullPath Parent: $fullParent"
    }
}

function Reset-Directory {
    param(
        [string] $Path,
        [string] $AllowedParent
    )

    Assert-PathIsInside -Path $Path -Parent $AllowedParent

    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Recurse -Force
    }

    New-Item -ItemType Directory -Force -Path $Path | Out-Null
}

function Invoke-Download {
    param(
        [string] $Url,
        [string] $Destination
    )

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Destination) | Out-Null

    if ($ForceDownloads -or -not (Test-Path -LiteralPath $Destination)) {
        Write-Host "Downloading $Url"
        Invoke-WebRequest -Uri $Url -OutFile $Destination -UseBasicParsing
    } else {
        Write-Host "Using cached $(Split-Path -Leaf $Destination)"
    }
}

function Find-Executable {
    param([string] $Name)

    $command = Get-Command $Name -ErrorAction SilentlyContinue | Select-Object -First 1

    if ($null -ne $command -and -not [string]::IsNullOrWhiteSpace($command.Source) -and (Test-Path -LiteralPath $command.Source)) {
        return $command.Source
    }

    $candidates = @(
        (Join-Path $env:USERPROFILE ".local\bin\$Name"),
        (Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Packages\Astral-Sh.uv*\$Name"),
        (Join-Path $env:LOCALAPPDATA "Programs\uv\$Name")
    )

    foreach ($candidate in $candidates) {
        $matches = Get-ChildItem -Path $candidate -ErrorAction SilentlyContinue | Select-Object -First 1

        if ($null -ne $matches -and (Test-Path -LiteralPath $matches.FullName)) {
            return $matches.FullName
        }
    }

    return $null
}

function Ensure-UvTools {
    New-Item -ItemType Directory -Force -Path $tools | Out-Null

    $uv = Find-Executable "uv.exe"
    $uvx = Find-Executable "uvx.exe"

    if ($uv -and $uvx) {
        Copy-Item -LiteralPath $uv -Destination (Join-Path $tools "uv.exe") -Force
        Copy-Item -LiteralPath $uvx -Destination (Join-Path $tools "uvx.exe") -Force
        Write-Host "Copied local uv.exe and uvx.exe into installer staging."
        return
    }

    Write-Host "uv was not found locally. Installing uv into installer staging..."

    $previousInstallDir = $env:UV_UNMANAGED_INSTALL

    try {
        $env:UV_UNMANAGED_INSTALL = $tools
        powershell -NoProfile -ExecutionPolicy Bypass -Command "irm https://astral.sh/uv/install.ps1 | iex"

        if ($LASTEXITCODE -ne 0) {
            throw "uv installer exited with code $LASTEXITCODE."
        }
    } finally {
        $env:UV_UNMANAGED_INSTALL = $previousInstallDir
    }

    foreach ($name in @("uv.exe", "uvx.exe")) {
        if (-not (Test-Path -LiteralPath (Join-Path $tools $name))) {
            throw "uv installer completed, but $name was not found in $tools."
        }
    }
}

function Ensure-Ffmpeg {
    New-Item -ItemType Directory -Force -Path $tools | Out-Null
    New-Item -ItemType Directory -Force -Path $cache | Out-Null

    $ffmpegZipUrl = "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip"
    $ffmpegShaUrl = "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip.sha256"
    $ffmpegZip = Join-Path $cache "ffmpeg-release-essentials.zip"
    $ffmpegSha = Join-Path $cache "ffmpeg-release-essentials.zip.sha256"
    $ffmpegExtract = Join-Path $cache "ffmpeg-release-essentials"

    Invoke-Download -Url $ffmpegZipUrl -Destination $ffmpegZip
    Invoke-Download -Url $ffmpegShaUrl -Destination $ffmpegSha

    $shaText = Get-Content -Raw -LiteralPath $ffmpegSha
    $expectedHash = [regex]::Match($shaText, "[A-Fa-f0-9]{64}").Value.ToUpperInvariant()

    if ([string]::IsNullOrWhiteSpace($expectedHash)) {
        throw "Could not parse ffmpeg SHA256 from $ffmpegSha."
    }

    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $ffmpegZip).Hash.ToUpperInvariant()

    if ($actualHash -ne $expectedHash) {
        throw "ffmpeg SHA256 mismatch. Expected $expectedHash but got $actualHash."
    }

    Reset-Directory -Path $ffmpegExtract -AllowedParent $cache
    Expand-Archive -LiteralPath $ffmpegZip -DestinationPath $ffmpegExtract -Force

    $ffmpegExe = Get-ChildItem -LiteralPath $ffmpegExtract -Recurse -Filter "ffmpeg.exe" -File |
        Select-Object -First 1

    if ($null -eq $ffmpegExe) {
        throw "Could not find ffmpeg.exe inside $ffmpegZip."
    }

    Copy-Item -LiteralPath $ffmpegExe.FullName -Destination (Join-Path $tools "ffmpeg.exe") -Force
    Write-Host "Staged ffmpeg.exe."
}

function Ensure-Deno {
    New-Item -ItemType Directory -Force -Path $tools | Out-Null
    New-Item -ItemType Directory -Force -Path $cache | Out-Null

    $localDeno = Find-Executable "deno.exe"

    if ($localDeno) {
        $versionText = & $localDeno --version 2>$null | Select-Object -First 1
        $versionMatch = [regex]::Match($versionText, '^deno\s+(\d+)\.(\d+)\.(\d+)')

        if ($versionMatch.Success) {
            $major = [int] $versionMatch.Groups[1].Value
            $minor = [int] $versionMatch.Groups[2].Value

            if ($major -gt 2 -or ($major -eq 2 -and $minor -ge 8)) {
                Copy-Item -LiteralPath $localDeno -Destination (Join-Path $tools "deno.exe") -Force
                Write-Host "Copied local deno.exe $versionText into installer staging."
                return
            }
        }

        Write-Host "Local deno.exe is missing or too old for yt-dlp EJS support. Downloading current Deno..."
    }

    $denoZipUrl = "https://github.com/denoland/deno/releases/latest/download/deno-x86_64-pc-windows-msvc.zip"
    $denoZip = Join-Path $cache "deno-x86_64-pc-windows-msvc.zip"
    $denoExtract = Join-Path $cache "deno-x86_64-pc-windows-msvc"

    Invoke-Download -Url $denoZipUrl -Destination $denoZip

    Reset-Directory -Path $denoExtract -AllowedParent $cache
    Expand-Archive -LiteralPath $denoZip -DestinationPath $denoExtract -Force

    $denoExe = Join-Path $denoExtract "deno.exe"

    if (-not (Test-Path -LiteralPath $denoExe)) {
        throw "Could not find deno.exe inside $denoZip."
    }

    Copy-Item -LiteralPath $denoExe -Destination (Join-Path $tools "deno.exe") -Force
    Write-Host "Staged deno.exe."
}

function Ensure-VcRuntime {
    New-Item -ItemType Directory -Force -Path $runtimes | Out-Null
    New-Item -ItemType Directory -Force -Path $cache | Out-Null

    $vcRedistUrl = "https://aka.ms/vc14/vc_redist.x64.exe"
    $vcRedistCache = Join-Path $cache "vc_redist.x64.exe"
    $vcRedist = Join-Path $runtimes "VC_redist.x64.exe"

    Invoke-Download -Url $vcRedistUrl -Destination $vcRedistCache

    if ((Get-Item -LiteralPath $vcRedistCache).Length -le 0) {
        throw "Downloaded vc_redist.x64.exe is empty."
    }

    Copy-Item -LiteralPath $vcRedistCache -Destination $vcRedist -Force
}

function Find-InnoCompiler {
    $command = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue | Select-Object -First 1

    if ($null -ne $command -and -not [string]::IsNullOrWhiteSpace($command.Source) -and (Test-Path -LiteralPath $command.Source)) {
        return $command.Source
    }

    $candidateRoots = @(
        ${env:ProgramFiles(x86)},
        $env:ProgramFiles,
        $cache
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

    foreach ($candidateRoot in $candidateRoots) {
        foreach ($relative in @("Inno Setup 6\ISCC.exe", "InnoSetup6\ISCC.exe")) {
            $candidate = Join-Path $candidateRoot $relative

            if (Test-Path -LiteralPath $candidate) {
                return $candidate
            }
        }
    }

    return $null
}

function Ensure-InnoCompiler {
    $compiler = Find-InnoCompiler

    if ($compiler) {
        Write-Host "Using Inno Setup compiler at $compiler"
        return $compiler
    }

    New-Item -ItemType Directory -Force -Path $cache | Out-Null

    $innoUrl = "https://github.com/jrsoftware/issrc/releases/download/is-6_7_3/innosetup-6.7.3.exe"
    $innoInstaller = Join-Path $cache "innosetup-6.7.3.exe"
    $localInnoDir = Join-Path $cache "InnoSetup6"

    Invoke-Download -Url $innoUrl -Destination $innoInstaller

    Write-Host "Installing Inno Setup compiler locally..."
    $process = Start-Process -FilePath $innoInstaller `
        -ArgumentList @("/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/CURRENTUSER", "/DIR=$localInnoDir") `
        -Wait `
        -PassThru `
        -WindowStyle Hidden

    if ($process.ExitCode -ne 0) {
        throw "Inno Setup installer exited with code $($process.ExitCode)."
    }

    $compiler = Find-InnoCompiler

    if (-not $compiler) {
        throw "Inno Setup installed, but ISCC.exe was not found."
    }

    return $compiler
}

function Write-StagingDocuments {
    @"
StashTrack VST3 Plug-in
Publisher: N9 Records
Version: v0.2
Website: https://stashtrack.n9records.com
Support: vsts@n9records.com

This installer places StashTrack.vst3 in:
C:\Program Files\Common Files\VST3\StashTrack.vst3

Fresh-PC notes:
- uv.exe and uvx.exe are bundled beside the plug-in binary, so Python is not required.
- ffmpeg.exe is bundled beside the plug-in binary.
- deno.exe is bundled beside the plug-in binary so yt-dlp can solve YouTube JavaScript challenges.
- Microsoft Visual C++ Redistributable x64 is installed silently only when it is missing.
- On first download, uvx may fetch and cache yt-dlp and its EJS helper package from the internet.

After setup, open your DAW, rescan VST3 plug-ins, and load StashTrack.
"@ | Set-Content -LiteralPath (Join-Path $staging "README-INSTALL.txt") -Encoding UTF8

    @"
StashTrack
Copyright (c) 2026 N9 Records

Legal notice: downloading YouTube or other website content may violate that site's Terms of Service or copyright laws. Only download content you own, have licensed, or otherwise have the rights to use.

This installer is provided as-is for installing the StashTrack VST3 plug-in and its runtime helper tools.
"@ | Set-Content -LiteralPath (Join-Path $staging "LICENSE.txt") -Encoding UTF8

    @"
Third-Party Notices

StashTrack bundles these helper tools for convenience on fresh Windows systems:

- uv / uvx from Astral. See https://github.com/astral-sh/uv for license details.
- FFmpeg Windows release essentials build from gyan.dev. FFmpeg is licensed under LGPL/GPL terms depending on build configuration; see https://ffmpeg.org/legal.html.
- Deno JavaScript runtime. See https://github.com/denoland/deno for license details.
- Microsoft Visual C++ Redistributable x64 from Microsoft. It is installed under Microsoft's redistributable terms.
- Inno Setup is used to build this installer. See https://jrsoftware.org/.

At runtime, StashTrack launches yt-dlp through uvx. yt-dlp and its EJS helper package are downloaded/cached by uvx when needed; see https://github.com/yt-dlp/yt-dlp for license details.
"@ | Set-Content -LiteralPath (Join-Path $staging "THIRD-PARTY-NOTICES.txt") -Encoding UTF8
}

if (-not $SkipVstBuild) {
    Write-Host "Building StashTrack_VST3 ($Configuration)..."
    cmake --build $BuildDir --target StashTrack_VST3 --config $Configuration

    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed with exit code $LASTEXITCODE."
    }
}

if (-not (Test-Path -LiteralPath $pluginSource)) {
    throw "Built plug-in was not found at $pluginSource. Build StashTrack_VST3 first or pass a valid -BuildDir."
}

Reset-Directory -Path $staging -AllowedParent $installerRoot
New-Item -ItemType Directory -Force -Path $payload, $tools, $runtimes, $dist | Out-Null

Copy-Item -LiteralPath $pluginSource -Destination $pluginDestination -Recurse -Force
Write-StagingDocuments

Ensure-UvTools
Ensure-Ffmpeg
Ensure-Deno
Ensure-VcRuntime

if (Test-Path -LiteralPath $setupOutput) {
    Remove-Item -LiteralPath $setupOutput -Force
}

$iscc = Ensure-InnoCompiler

Write-Host "Compiling StashTrackv0.2Setup.exe..."
Push-Location $installerRoot
try {
    & $iscc "StashTrack.iss"

    if ($LASTEXITCODE -ne 0) {
        throw "Inno Setup compiler failed with exit code $LASTEXITCODE."
    }
} finally {
    Pop-Location
}

if (-not (Test-Path -LiteralPath $setupOutput)) {
    throw "Installer build completed, but $setupOutput was not created."
}

$installer = Get-Item -LiteralPath $setupOutput

if ($installer.Length -le 0) {
    throw "$setupOutput exists but is empty."
}

Write-Host "Created $($installer.FullName) ($([Math]::Round($installer.Length / 1MB, 1)) MB)"
