param(
    [string]$EngineDir = "",
    [string]$Target = "",
    [string]$Platform = "Win64",
    [string]$Configuration = "Development"
)

$ErrorActionPreference = "Stop"

function Get-FirstExistingPath {
    param([string[]]$Paths)

    foreach ($Path in $Paths) {
        if (![string]::IsNullOrWhiteSpace($Path) -and (Test-Path $Path)) {
            return (Resolve-Path $Path).Path
        }
    }

    return ""
}

function Get-EngineAssociation {
    param([string]$ProjectFile)

    $Project = Get-Content $ProjectFile -Raw | ConvertFrom-Json
    if ($Project.EngineAssociation) {
        return [string]$Project.EngineAssociation
    }

    return ""
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = (Resolve-Path (Join-Path $ScriptDir "..\..\..")).Path
$ProjectFile = Get-ChildItem -Path $ProjectRoot -Filter "*.uproject" | Select-Object -First 1
if ($null -eq $ProjectFile) {
    throw "No .uproject file found under '$ProjectRoot'."
}

if ([string]::IsNullOrWhiteSpace($Target)) {
    $TargetFile = Get-ChildItem -Path (Join-Path $ProjectRoot "Source") -Filter "*Editor.Target.cs" -Recurse | Select-Object -First 1
    if ($null -eq $TargetFile) {
        throw "No *Editor.Target.cs file found under '$ProjectRoot\Source'. Pass -Target explicitly."
    }

    $Target = $TargetFile.Name -replace "\.Target\.cs$", ""
}

$EngineAssociation = Get-EngineAssociation $ProjectFile.FullName
if ([string]::IsNullOrWhiteSpace($EngineDir)) {
    $EngineDir = Get-FirstExistingPath @(
        $env:UE_ENGINE_DIR,
        $env:UNREAL_ENGINE_DIR,
        $env:EngineDir
    )
}

if ([string]::IsNullOrWhiteSpace($EngineDir)) {
    $LauncherInstalled = Join-Path $env:ProgramData "Epic\UnrealEngineLauncher\LauncherInstalled.dat"
    if (Test-Path $LauncherInstalled) {
        $LauncherData = Get-Content $LauncherInstalled -Raw | ConvertFrom-Json
        $InstalledEngine = $LauncherData.InstallationList |
            Where-Object { $_.AppName -eq "UE_$EngineAssociation" -and $_.InstallLocation } |
            Select-Object -First 1
        if ($null -ne $InstalledEngine) {
            $EngineDir = [string]$InstalledEngine.InstallLocation
        }
    }
}

if ([string]::IsNullOrWhiteSpace($EngineDir)) {
    $EngineDir = Get-FirstExistingPath @(
        "C:\Program Files\Epic Games\UE_$EngineAssociation\Engine",
        "C:\Program Files\Epic Games\$EngineAssociation\Engine"
    )
}

if ([string]::IsNullOrWhiteSpace($EngineDir)) {
    throw "EngineDir was not found. Pass -EngineDir or set UE_ENGINE_DIR / UNREAL_ENGINE_DIR / EngineDir."
}

$BuildScript = Join-Path $EngineDir "Build\BatchFiles\Build.bat"
if (!(Test-Path $BuildScript)) {
    $NestedEngineDir = Join-Path $EngineDir "Engine"
    $NestedBuildScript = Join-Path $NestedEngineDir "Build\BatchFiles\Build.bat"
    if (Test-Path $NestedBuildScript) {
        $EngineDir = $NestedEngineDir
        $BuildScript = $NestedBuildScript
    }
}

if (!(Test-Path $BuildScript)) {
    throw "Build.bat was not found at '$BuildScript'. EngineDir must point to the Unreal Engine/Engine directory."
}

$env:GENERATE_UNREAL_PYTHON = "1"

Write-Host "Project: $($ProjectFile.FullName)"
Write-Host "Target: $Target"
Write-Host "Platform: $Platform"
Write-Host "Configuration: $Configuration"
Write-Host "EngineDir: $EngineDir"
Write-Host "GENERATE_UNREAL_PYTHON=1"

Push-Location $ProjectRoot
try {
    & $BuildScript $Target $Platform $Configuration $ProjectFile.FullName -NoHotReloadFromIDE -ForceHeaderGeneration -SkipBuild
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
finally {
    Pop-Location
}
