param(
	[string]$PluginDir = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path,
	[string]$PythonVersion = "3.14.2",
	[string[]]$Hosts = @("aarch64-linux-android", "x86_64-linux-android"),
	[string]$WorkDir
)

$ErrorActionPreference = "Stop"

$PluginDir = (Resolve-Path -LiteralPath $PluginDir).Path
if ([string]::IsNullOrWhiteSpace($WorkDir)) {
	$ProjectRoot = (Resolve-Path -LiteralPath (Join-Path $PluginDir "..\..")).Path
	$WorkDir = Join-Path $ProjectRoot "Intermediate\UnrealPython\OfficialAndroidPython"
}

$AndroidRuntimeRoot = Join-Path $PluginDir "ThirdParty\python314\android"
$DownloadDir = Join-Path $WorkDir "downloads"
New-Item -ItemType Directory -Force -Path $DownloadDir | Out-Null
New-Item -ItemType Directory -Force -Path $AndroidRuntimeRoot | Out-Null

function Copy-StdLib([string]$SourceDir, [string]$DestinationDir) {
	if (Test-Path -LiteralPath $DestinationDir -PathType Container) {
		Remove-Item -LiteralPath $DestinationDir -Recurse -Force
	}
	New-Item -ItemType Directory -Force -Path $DestinationDir | Out-Null

	Get-ChildItem -LiteralPath $SourceDir -Recurse -File | ForEach-Object {
		$RelativePath = $_.FullName.Substring($SourceDir.Length + 1)
		$RelativePathUnix = $RelativePath -replace "\\", "/"
		if ($RelativePathUnix -eq "_android_support.py") {
			return
		}
		if ($RelativePathUnix.EndsWith(".pyc") -or $RelativePathUnix.Contains("/__pycache__/")) {
			return
		}
		if ($RelativePathUnix.StartsWith("lib-dynload/") -or $RelativePathUnix.StartsWith("site-packages/")) {
			return
		}

		$TargetPath = Join-Path $DestinationDir $RelativePath
		New-Item -ItemType Directory -Force -Path (Split-Path -Parent $TargetPath) | Out-Null
		Copy-Item -LiteralPath $_.FullName -Destination $TargetPath
	}
}

$SupportModuleCopied = $false

foreach ($HostTriplet in $Hosts) {
	$ArchiveName = "python-$PythonVersion-$HostTriplet.tar.gz"
	$ArchivePath = Join-Path $DownloadDir $ArchiveName
	$ExtractDir = Join-Path $WorkDir "python-$PythonVersion-$HostTriplet"
	$StdLibDir = Join-Path $ExtractDir "prefix\lib\python3.14"

	if (!(Test-Path -LiteralPath $ArchivePath -PathType Leaf)) {
		$Url = "https://www.python.org/ftp/python/$PythonVersion/$ArchiveName"
		Write-Host "Downloading $Url"
		Invoke-WebRequest -Uri $Url -OutFile $ArchivePath
	}

	if (!(Test-Path -LiteralPath $StdLibDir -PathType Container)) {
		if (Test-Path -LiteralPath $ExtractDir -PathType Container) {
			Remove-Item -LiteralPath $ExtractDir -Recurse -Force
		}
		New-Item -ItemType Directory -Force -Path $ExtractDir | Out-Null
		& tar -xzf $ArchivePath -C $ExtractDir
		if (!(Test-Path -LiteralPath $StdLibDir -PathType Container)) {
			throw "Missing Android stdlib after extracting official package: $StdLibDir"
		}
	}

	$DestinationDir = Join-Path $AndroidRuntimeRoot "$HostTriplet\lib\python3.14"
	Copy-StdLib $StdLibDir $DestinationDir
	Write-Host "$HostTriplet stdlib files: $((Get-ChildItem -LiteralPath $DestinationDir -Recurse -File | Measure-Object).Count)"

	if (!$SupportModuleCopied) {
		$SupportSource = Join-Path $StdLibDir "_android_support.py"
		$SupportDestination = Join-Path $AndroidRuntimeRoot "_android_support.py"
		if (!(Test-Path -LiteralPath $SupportSource -PathType Leaf)) {
			throw "Missing Android support module in official package: $SupportSource"
		}

		Copy-Item -LiteralPath $SupportSource -Destination $SupportDestination
		$SupportText = Get-Content -LiteralPath $SupportDestination -Raw
		$OldFilenoBlock = "            fileno = original.fileno()`n"
		$NewFilenoBlock = "            try:`n                fileno = original.fileno()`n            except (AttributeError, OSError, io.UnsupportedOperation):`n                fileno = None`n"
		if (!$SupportText.Contains($NewFilenoBlock)) {
			if (!$SupportText.Contains($OldFilenoBlock)) {
				throw "Could not find fileno block in $SupportDestination"
			}
			$SupportText.Replace($OldFilenoBlock, $NewFilenoBlock) | Set-Content -LiteralPath $SupportDestination -NoNewline
		}
		$SupportModuleCopied = $true
	}
}

Write-Host "Official Android Python stdlib synced to $AndroidRuntimeRoot"
