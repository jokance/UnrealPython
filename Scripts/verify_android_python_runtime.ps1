param(
	[string]$PluginDir = (Join-Path (Get-Location) "Plugins\UnrealPython"),
	[string[]]$Hosts = @("aarch64-linux-android", "x86_64-linux-android"),
	[int]$AndroidApiLevel = 24
)

$ErrorActionPreference = "Stop"

$PluginDir = (Resolve-Path -LiteralPath $PluginDir).Path
$ProjectRoot = (Resolve-Path -LiteralPath (Join-Path $PluginDir "..\..")).Path
$ProjectFile = Get-ChildItem -LiteralPath $ProjectRoot -Filter "*.uproject" -File | Select-Object -First 1
$RuntimeRoot = Join-Path $PluginDir "ThirdParty\python314\android"
$BuildCs = Join-Path $PluginDir "Source\UnrealPython\UnrealPython.Build.cs"
$RequiredRuntimeLibraries = @(
	"libpython3.14.a",
	"libUnrealPython3.14.a",
	"libssl.a",
	"libcrypto.a",
	"libUnrealPythonSSL.a",
	"libUnrealPythonCrypto.a",
	"libbz2.a",
	"libffi.a",
	"liblzma.a",
	"libzstd.a",
	"libmpdec.a"
)
$RequiredBuildCsLibraries = @(
	"libUnrealPython3.14.a",
	"libUnrealPythonSSL.a",
	"libUnrealPythonCrypto.a",
	"libbz2.a",
	"libffi.a",
	"liblzma.a",
	"libzstd.a",
	"libmpdec.a"
)

$Failures = New-Object System.Collections.Generic.List[string]
$Warnings = New-Object System.Collections.Generic.List[string]

function Add-Failure([string]$Message) {
	$Failures.Add($Message) | Out-Null
}

function Add-Warning([string]$Message) {
	$Warnings.Add($Message) | Out-Null
}

function Test-ArchiveFile([string]$Path) {
	if (!(Test-Path -LiteralPath $Path -PathType Leaf)) {
		Add-Failure "Missing file: $Path"
		return
	}

	$Item = Get-Item -LiteralPath $Path
	if ($Item.Length -le 1024) {
		Add-Failure "Archive is unexpectedly small: $Path ($($Item.Length) bytes)"
		return
	}

	$Stream = [System.IO.File]::OpenRead($Path)
	try {
		$Header = New-Object byte[] 8
		$Read = $Stream.Read($Header, 0, 8)
		if ($Read -ne 8 -or [System.Text.Encoding]::ASCII.GetString($Header) -ne "!<arch>`n") {
			Add-Failure "Not a Unix static archive: $Path"
		}
	}
	finally {
		$Stream.Dispose()
	}
}

function Assert-Regex([string]$Path, [string]$Pattern, [string]$Description) {
	if (!(Select-String -LiteralPath $Path -Pattern $Pattern -Quiet)) {
		Add-Failure "$Description was not found in $Path"
	}
}

if (!(Test-Path -LiteralPath $BuildCs -PathType Leaf)) {
	Add-Failure "Missing Build.cs: $BuildCs"
}
else {
	Assert-Regex $BuildCs 'ConfigureAndroidPython' "Android Python configure hook"
	Assert-Regex $BuildCs 'GetAndroidPythonHost' "Android host selection"
	foreach ($Library in $RequiredBuildCsLibraries) {
		Assert-Regex $BuildCs ([Regex]::Escape($Library)) "Build.cs reference to $Library"
	}
	foreach ($HostTriplet in $Hosts) {
		Assert-Regex $BuildCs ([Regex]::Escape($HostTriplet)) "Build.cs reference to $HostTriplet"
	}
}

if ($null -eq $ProjectFile) {
	Add-Failure "Could not find a .uproject file under $ProjectRoot"
}
else {
	$ProjectJson = Get-Content -LiteralPath $ProjectFile.FullName -Raw | ConvertFrom-Json
	$PluginEntry = @($ProjectJson.Plugins | Where-Object { $_.Name -eq "UnrealPython" -and $_.Enabled -eq $true })
	if ($PluginEntry.Count -eq 0) {
		Add-Failure "UnrealPython is not enabled in project file: $($ProjectFile.FullName)"
	}
}

foreach ($HostTriplet in $Hosts) {
	$HostRoot = Join-Path $RuntimeRoot $HostTriplet
	$IncludeRoot = Join-Path $HostRoot "include"
	$PythonIncludeRoot = Join-Path $IncludeRoot "python3.14"
	$LibRoot = Join-Path $HostRoot "lib"
	$PythonHeader = Join-Path $PythonIncludeRoot "Python.h"
	$PyConfig = Join-Path $PythonIncludeRoot "pyconfig.h"

	if (!(Test-Path -LiteralPath $HostRoot -PathType Container)) {
		Add-Failure "Missing Android host runtime directory: $HostRoot"
		continue
	}

	if (!(Test-Path -LiteralPath $PythonHeader -PathType Leaf)) {
		Add-Failure "Missing Python.h for ${HostTriplet}: $PythonHeader"
	}

	if (!(Test-Path -LiteralPath $PyConfig -PathType Leaf)) {
		Add-Failure "Missing pyconfig.h for ${HostTriplet}: $PyConfig"
	}
	else {
		Assert-Regex $PyConfig "#define ANDROID_API_LEVEL $AndroidApiLevel" "$HostTriplet Android API level $AndroidApiLevel"
		Assert-Regex $PyConfig '/\* #undef Py_ENABLE_SHARED \*/' "$HostTriplet static libpython setting"
	}

	foreach ($Library in $RequiredRuntimeLibraries) {
		Test-ArchiveFile (Join-Path $LibRoot $Library)
	}

	$SharedObjects = @(Get-ChildItem -LiteralPath $LibRoot -Filter "*.so" -File -Recurse -ErrorAction SilentlyContinue)
	if ($SharedObjects.Count -gt 0) {
		Add-Warning "$HostTriplet contains shared objects under lib; they are ignored by the current static-link Build.cs."
	}
}

if ($Warnings.Count -gt 0) {
	Write-Host "Warnings:" -ForegroundColor Yellow
	foreach ($Warning in $Warnings) {
		Write-Host "  - $Warning" -ForegroundColor Yellow
	}
}

if ($Failures.Count -gt 0) {
	Write-Host "Android Python runtime verification failed:" -ForegroundColor Red
	foreach ($Failure in $Failures) {
		Write-Host "  - $Failure" -ForegroundColor Red
	}
	exit 1
}

Write-Host "Android Python runtime verification passed." -ForegroundColor Green
Write-Host "Plugin: $PluginDir"
Write-Host "Hosts: $($Hosts -join ', ')"
