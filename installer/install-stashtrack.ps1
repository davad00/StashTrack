[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string] $Configuration = "Release",

    [string] $BuildDir,

    [string] $ToolsDir,

    [switch] $User,

    [switch] $DryRun
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $root "build-vs"
}

function Test-IsAdministrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function ConvertTo-QuotedArgument {
    param([string] $Value)

    if ($Value -match '\s') {
        return '"' + ($Value -replace '"', '\"') + '"'
    }

    return $Value
}

if (-not $User -and -not $DryRun -and -not (Test-IsAdministrator)) {
    $arguments = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", (ConvertTo-QuotedArgument $PSCommandPath),
        "-Configuration", $Configuration
    )

    if (-not [string]::IsNullOrWhiteSpace($BuildDir)) {
        $arguments += @("-BuildDir", (ConvertTo-QuotedArgument $BuildDir))
    }

    if (-not [string]::IsNullOrWhiteSpace($ToolsDir)) {
        $arguments += @("-ToolsDir", (ConvertTo-QuotedArgument $ToolsDir))
    }

    Write-Host "Installing to the system VST3 folder requires administrator permission."
    Write-Host "Opening an elevated installer window..."
    $process = Start-Process -FilePath "powershell" -ArgumentList $arguments -Verb RunAs -Wait -PassThru
    exit $process.ExitCode
}

$source = Join-Path $BuildDir "StashTrack_artefacts/$Configuration/VST3/StashTrack.vst3"
$destinationRoot = if ($User) {
    Join-Path $env:LOCALAPPDATA "Programs/Common/VST3"
} else {
    Join-Path $env:CommonProgramFiles "VST3"
}

$destination = Join-Path $destinationRoot "StashTrack.vst3"
$binaryFolder = Join-Path $destination "Contents/x86_64-win"

Write-Host "StashTrack VST3 installer"
Write-Host "Source:      $source"
Write-Host "Destination: $destination"

if ($User) {
    Write-Host "Mode:        User-local install"
} else {
    Write-Host "Mode:        System VST3 install"
}

if ($DryRun) {
    Write-Host "[dry-run] Would create $destinationRoot"
    Write-Host "[dry-run] Would copy StashTrack.vst3 to $destination"
    Write-Host "[dry-run] Would verify uv.exe and uvx.exe availability for uv-managed yt-dlp"
    Write-Host "[dry-run] Would install uv into the plug-in bundle if uv is missing"

    if (-not [string]::IsNullOrWhiteSpace($ToolsDir)) {
        Write-Host "[dry-run] Would copy ffmpeg.exe from $ToolsDir to $binaryFolder when present"
    }

    exit 0
}

if (-not (Test-Path $source)) {
    throw "Built plug-in was not found at $source. Build StashTrack_VST3 first."
}

New-Item -ItemType Directory -Force -Path $destinationRoot | Out-Null

if (Test-Path $destination) {
    Remove-Item -Recurse -Force -Path $destination
}

Copy-Item -Recurse -Force -Path $source -Destination $destination

function Find-Executable {
    param([string] $Name)

    $command = Get-Command $Name -ErrorAction SilentlyContinue | Select-Object -First 1

    if ($null -ne $command -and -not [string]::IsNullOrWhiteSpace($command.Source) -and (Test-Path $command.Source)) {
        return $command.Source
    }

    $candidates = @(
        (Join-Path $env:USERPROFILE ".local\bin\$Name"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python312\Scripts\$Name"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python311\Scripts\$Name"),
        (Join-Path $env:LOCALAPPDATA "Programs\Python\Python310\Scripts\$Name")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    $pythonRoots = Get-ChildItem -Directory (Join-Path $env:LOCALAPPDATA "Programs\Python") -ErrorAction SilentlyContinue

    foreach ($root in $pythonRoots) {
        $candidate = Join-Path $root.FullName "Scripts\$Name"

        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Install-UvIntoBundle {
    param([string] $TargetFolder)

    New-Item -ItemType Directory -Force -Path $TargetFolder | Out-Null

    Write-Host "uv was not found. Downloading and installing uv into the StashTrack VST3 bundle..."

    $previousInstallDir = $env:UV_UNMANAGED_INSTALL

    try {
        $env:UV_UNMANAGED_INSTALL = $TargetFolder
        powershell -NoProfile -ExecutionPolicy ByPass -Command "irm https://astral.sh/uv/install.ps1 | iex"

        if ($LASTEXITCODE -ne 0) {
            throw "uv installer exited with code $LASTEXITCODE."
        }
    } finally {
        $env:UV_UNMANAGED_INSTALL = $previousInstallDir
    }

    $uv = Join-Path $TargetFolder "uv.exe"
    $uvx = Join-Path $TargetFolder "uvx.exe"

    if (-not (Test-Path $uv) -or -not (Test-Path $uvx)) {
        throw "uv installer completed, but uv.exe or uvx.exe was not found in $TargetFolder."
    }
}

function Ensure-UvInBundle {
    param([string] $TargetFolder)

    New-Item -ItemType Directory -Force -Path $TargetFolder | Out-Null

    $uv = Find-Executable "uv.exe"
    $uvx = Find-Executable "uvx.exe"

    if ($uv -and $uvx) {
        Copy-Item -Force -Path $uv -Destination (Join-Path $TargetFolder "uv.exe")
        Copy-Item -Force -Path $uvx -Destination (Join-Path $TargetFolder "uvx.exe")
        Write-Host "Copied uv.exe and uvx.exe into the VST3 bundle."
        return
    }

    Install-UvIntoBundle $TargetFolder
}

Ensure-UvInBundle $binaryFolder

if (-not [string]::IsNullOrWhiteSpace($ToolsDir)) {
    if (-not (Test-Path $ToolsDir)) {
        throw "Tools directory was not found: $ToolsDir"
    }

    New-Item -ItemType Directory -Force -Path $binaryFolder | Out-Null

    foreach ($tool in @("ffmpeg.exe")) {
        $toolPath = Join-Path $ToolsDir $tool

        if (Test-Path $toolPath) {
            Copy-Item -Force -Path $toolPath -Destination (Join-Path $binaryFolder $tool)
            Write-Host "Copied $tool"
        } else {
            Write-Host "Skipped $tool because it was not found in $ToolsDir"
        }
    }
}

Write-Host "Installed StashTrack to $destination"
