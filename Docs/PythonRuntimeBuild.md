English | [中文](PythonRuntimeBuild.zh.md)

# Python 3.14 Runtime Build and Packaging Flow

This document records the source, build process, directory layout, and validation steps for the Python 3.14 runtime bundled with the current `UnrealPython` plugin.

The plugin does not use UE's built-in Python. UE's embedded Python version and this plugin's target version are different, and mixing them can cause ABI, symbol, and `sys.path` conflicts. The plugin should always link and initialize its own Python runtime.

## Target version

- Python: `3.14.5`
- Plugin directory name: `ThirdParty/python314`
- Android runtime: `ThirdParty/python314/android`
- Mac runtime: `ThirdParty/python314/Mac`
- iOS runtime: `ThirdParty/python314/IOS`

## Windows runtime

The Windows runtime uses the CPython embeddable package layout. The plugin does not extract `python314.zip`; at runtime it adds the zip file to `PyConfig.module_search_paths`, and Python reads the standard library through zipimport.

Key files in the plugin:

```text
ThirdParty/python314/include
ThirdParty/python314/Win64/libs/python314.lib
ThirdParty/python314/Win64/python314.dll
ThirdParty/python314/Win64/python314.zip
```

`python314.zip` is the compressed standard library. It must be placed next to the game executable; leaving it only under the plugin ThirdParty directory is not enough. The Win64 configuration in `Source/UnrealPython/UnrealPython.Build.cs`:

- Adds `ThirdParty/python314/include`.
- Links `ThirdParty/python314/Win64/libs/python314.lib`.
- Stages `python314.dll` and `python314.zip` to `$(BinaryOutputDir)` for Editor builds.
- Stages `python314.dll` and `python314.zip` to `$(TargetOutputDir)` for non-Editor builds.

Windows initialization lives in `Source/UnrealPython/Private/Core/UPyVirtualMachine.cpp`. At runtime it checks:

```text
<GameExeDir>/python314.zip
```

When found, the log contains:

```text
LogUnrealPython: Added bundled Windows Python path: .../python314.zip
```

### Windows project configuration

Project scripts are placed here by default:

```text
Content/Scripts
```

They should be staged as Non-UFS files during packaging so the packaged game can read scripts by file-system path.

Recommended project `Config/DefaultGame.ini`:

```ini
[/Script/UnrealEd.ProjectPackagingSettings]
+DirectoriesToAlwaysStageAsNonUFS=(Path="Scripts")

[UnrealPython]
ScriptPath=Content/Scripts
```

Notes:

- `DirectoriesToAlwaysStageAsNonUFS=(Path="Scripts")` corresponds to project `Content/Scripts`.
- `[UnrealPython] ScriptPath=Content/Scripts` keeps the same script directory convention in the editor and packaged builds.
- Project scripts are no longer copied at runtime; the script directory must enter the package during stage/package.

### Windows packaging flow

Run a full BuildCookRun from the Windows project root:

```powershell
& 'D:\Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun `
  -project='D:\Projects\SampleGame\SampleGame.uproject' `
  -noP4 `
  -platform=Win64 `
  -clientconfig=Development `
  -build `
  -cook `
  -stage `
  -pak `
  -package `
  -archive `
  -archivedirectory='D:\Projects\SampleGame\Saved\WindowsTest' `
  -utf8output
```

If you only need to validate that the plugin module compiles, run a Win64 target build first:

```bat
"D:\Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" SampleGame Win64 Development ^
  -Project="D:\Projects\SampleGame\SampleGame.uproject" ^
  -WaitMutex ^
  -NoHotReloadFromIDE
```

### Windows package validation

After packaging succeeds, check that the archive directory contains:

```text
SampleGame.exe
python314.dll
python314.zip
Content/Scripts
```

After launching the packaged game, focus on these logs:

```text
LogUnrealPython: Added bundled Windows Python path: .../python314.zip
LogUnrealPython: Python VM init success!
```

If this appears:

```text
Py_InitializeFromConfig() failed: Failed to import encodings module
```

first check whether `python314.zip` is next to the exe and whether UBT executed the Win64 `RuntimeDependencies` staging.

## Android runtime

The Android runtime is built from CPython 3.14.5 source, based on CPython's official `Android/android.py` cross-compilation flow. The official CPython Android release package produces shared `libpython3.14.so` by default. This plugin's Android integration chooses static linking, so the build script changes the official script's `--enable-shared --without-static-libpython` to `--disable-shared --with-static-libpython` in the source build directory before running the official build flow.

The Android standard library is synchronized from the official Python Android embeddable package into the plugin ThirdParty directory. The current sync script is `Scripts/sync_official_android_stdlib.ps1`, which downloads by default:

```text
https://www.python.org/ftp/python/3.14.2/python-3.14.2-aarch64-linux-android.tar.gz
https://www.python.org/ftp/python/3.14.2/python-3.14.2-x86_64-linux-android.tar.gz
```

Synchronized directory layout:

```text
ThirdParty/python314/android/_android_support.py
ThirdParty/python314/android/aarch64-linux-android/lib/python3.14
ThirdParty/python314/android/x86_64-linux-android/lib/python3.14
```

`_android_support.py` comes from `prefix/lib/python3.14/_android_support.py` in the official package. The plugin only adds a `fileno()` compatibility guard so initialization does not fail when stdout/stderr in the UE embedded environment do not support `fileno()`.

References:

- CPython Android usage: <https://docs.python.org/3/using/android.html>
- CPython Android build script notes: <https://github.com/python/cpython/blob/3.14/Android/README.md>
- CPython 3.14.5 release directory: <https://www.python.org/ftp/python/3.14.5/>

Build environment:

```text
Linux, macOS, or WSL
Android SDK, with ANDROID_HOME pointing to the SDK root
Java, curl, tar, make, python3
```

Default target ABIs:

```text
aarch64-linux-android
x86_64-linux-android
```

The plugin currently commits these two hosts only. Official UE Android packages usually use `aarch64-linux-android`; `x86_64-linux-android` is for emulators or projects that need an x86_64 Android target.

### Android development machine setup

On a Windows development machine, first install the Android target platform components for the matching UE version and make sure the engine directory contains Android target files, for example:

```text
D:\Games\UE_5.7\Engine\Binaries\Android\UnrealGame.target
D:\Games\UE_5.7\Engine\Build\Android
D:\Games\UE_5.7\Engine\Build\Android\Java
```

SDK/JDK/NDK environment variables validated on the current machine:

```powershell
ANDROID_HOME=C:\Users\Nien\AppData\Local\Android\Sdk
ANDROID_SDK_ROOT=C:\Users\Nien\AppData\Local\Android\Sdk
JAVA_HOME=C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot
NDKROOT=C:\Users\Nien\AppData\Local\Android\Sdk\ndk\27.2.12479018
NDK_ROOT=C:\Users\Nien\AppData\Local\Android\Sdk\ndk\27.2.12479018
ANDROID_AVD_HOME=D:\Android\avd
```

Emulator testing uses the x86_64 ABI, so enable x86_64 Android packaging in the project. The current `Config/DefaultEngine.ini` uses:

```ini
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
bBuildForArm64=False
bBuildForX8664=True
SDKAPILevel=android-34
NDKAPILevel=android-26
```

For real arm64 device testing, usually switch back to:

```ini
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
bBuildForArm64=True
bBuildForX8664=False
SDKAPILevel=android-34
NDKAPILevel=android-26
```

### Android build commands

Run from the project root:

```sh
export ANDROID_HOME=/path/to/android-sdk

Plugins/UnrealPython/Scripts/build_android_python_runtime.sh \
  --host aarch64-linux-android \
  --host x86_64-linux-android
```

Optional parameters:

```sh
# Clean the CPython source/build directories managed by the script before rebuilding.
Plugins/UnrealPython/Scripts/build_android_python_runtime.sh --clean

# Build only Android arm64.
Plugins/UnrealPython/Scripts/build_android_python_runtime.sh --host aarch64-linux-android

# Use a custom work directory to avoid using the project Intermediate directory.
Plugins/UnrealPython/Scripts/build_android_python_runtime.sh --work-dir /tmp/unrealpython-python-android-build
```

The script performs these steps:

1. Downloads `https://www.python.org/ftp/python/3.14.5/Python-3.14.5.tgz`.
2. Extracts it to `Intermediate/UnrealPython/PythonAndroidBuild`.
3. Builds the specified host through CPython `Android/android.py`.
4. Generates and installs the CPython host prefix.
5. Copies headers and static libraries into the plugin:

```text
ThirdParty/python314/android/<host>/include
ThirdParty/python314/android/<host>/include/python3.14
ThirdParty/python314/android/<host>/lib/libpython3.14.a
ThirdParty/python314/android/<host>/lib/libUnrealPython3.14.a
ThirdParty/python314/android/<host>/lib/libssl.a
ThirdParty/python314/android/<host>/lib/libcrypto.a
ThirdParty/python314/android/<host>/lib/libUnrealPythonSSL.a
ThirdParty/python314/android/<host>/lib/libUnrealPythonCrypto.a
ThirdParty/python314/android/<host>/lib/libbz2.a
ThirdParty/python314/android/<host>/lib/libffi.a
ThirdParty/python314/android/<host>/lib/liblzma.a
ThirdParty/python314/android/<host>/lib/libzstd.a
ThirdParty/python314/android/<host>/lib/libmpdec.a
```

`libpython3.14.a`, `libssl.a`, and `libcrypto.a` are the original CPython/OpenSSL static libraries. `libUnrealPython3.14.a`, `libUnrealPythonSSL.a`, and `libUnrealPythonCrypto.a` are rewritten artifacts for UE Android linking. Do not manually delete these three `libUnrealPython*` libraries.

### Android Build.cs integration

Android linking logic lives in `Source/UnrealPython/UnrealPython.Build.cs`:

- Selects `aarch64-linux-android` or `x86_64-linux-android` from `Target.Architecture`.
- Adds `<host>/include` and `<host>/include/python3.14`.
- Links `libUnrealPython3.14.a`.
- Links the static library dependencies used by CPython modules, including OpenSSL, bz2, ffi, lzma, zstd, and mpdec.
- UE's built-in OpenSSL 1.1.1 library paths can appear before plugin library paths on Android, and the final Android game `.so` links all static libraries into the same binary. The plugin uses `libUnrealPython3.14.a`, `libUnrealPythonSSL.a`, and `libUnrealPythonCrypto.a` as rewritten Android link artifacts: OpenSSL 3 exported symbols are prefixed with `UPY_OPENSSL3_`, and Python references to OpenSSL in `_ssl/_hashopenssl` are rewritten as well to avoid conflicts with UE's OpenSSL.
- Builds `Plugins/UnrealPython/Intermediate/Android/unrealpython_bootstrap.zip`, which contains:
  - `android/_android_support.py`
  - `android/<host>/lib/python3.14`
  - project `Content/Scripts`
- Writes the bootstrap zip SHA256 to `UPY_PYTHON_BOOTSTRAP_VERSION`.
- Copies `unrealpython_bootstrap.zip` to APK `assets/unrealpython_bootstrap.zip` through `UnrealPython_UPL.xml`.
- Throws a `BuildException` directly if the host directory or any required library is missing.

Android cook loads the plugin as a commandlet. The plugin currently skips Python VM initialization under commandlets so Windows cook does not fail because the Win64 standard library `encodings` is unavailable. `StartupModule()` and `ShutdownModule()` must keep this skip logic symmetrical.

At runtime, before Python initialization, the Android packaged game extracts `unrealpython_bootstrap.zip` from APK assets through Java to:

```text
<ExternalFilesDir>/UnrealGame/<Project>/<Project>/Saved/UnrealPythonBootstrap
```

Then it adds these paths to `PyConfig.module_search_paths`:

```text
.../UnrealPythonBootstrap/android
.../UnrealPythonBootstrap/android/<host>/lib/python3.14
.../UnrealPythonBootstrap/Content/Scripts
```

`Content/Scripts` is still packed into UE pak/OBB data, but on Android Python startup depends on the bootstrap zip in APK assets. This avoids direct pak/OBB reads when CPython Android initialization requires real file-system paths.

Note: `unrealpython_bootstrap.zip` is shared between arm64 and x86_64 builds. If you frequently switch between `-clientarchitecture=x64` and `-clientarchitecture=arm64` in one workspace, make sure UBT reruns `UnrealPython.Build.cs`. If an arm64 device log still extracts `x86_64-linux-android`, delete:

```text
Plugins/UnrealPython/Intermediate/Android/unrealpython_bootstrap.zip
Plugins/UnrealPython/Intermediate/Android/BootstrapStaging
```

Then repackage with explicit `-clientarchitecture=arm64`.

### Android in-project validation

Run from the project root on a Windows development machine:

```powershell
powershell -ExecutionPolicy Bypass -File Plugins\UnrealPython\Scripts\verify_android_python_runtime.ps1
```

The validation script checks:

- `aarch64-linux-android` and `x86_64-linux-android` host directories exist.
- `Python.h` and `pyconfig.h` exist.
- `ANDROID_API_LEVEL` in `pyconfig.h` is `24`.
- `Py_ENABLE_SHARED` is not enabled in `pyconfig.h`, confirming a static libpython artifact.
- All required `.a` files exist, have a valid size, and are Unix static archives.
- `UnrealPython.Build.cs` contains Android host selection and all required library references.
- `ThirdParty/python314/android/_android_support.py` exists and contains the Android logcat bridge.
- Each host has a `lib/python3.14` standard library and Android sysconfig files.
- `SampleGame.uproject` has `UnrealPython` enabled.

After validation passes, run the UE Android build:

```bat
"%UE_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat" SampleGame Android Development ^
  -Project="D:\Projects\SampleGame\SampleGame.uproject" ^
  -WaitMutex ^
  -NoHotReloadFromIDE
```

If you only want to verify that UBT can parse the plugin module, run `verify_android_python_runtime.ps1` first, then the Android target build. The former validates the third-party Python runtime files and project integration; the latter validates that UE/NDK/linker accept these static libraries.

### Android packaging flow

Before packaging, make sure the project Android ABI matches the target device:

```ini
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
; Android emulator
bBuildForArm64=False
bBuildForX8664=True

; Android real device
; bBuildForArm64=True
; bBuildForX8664=False
```

To resynchronize the official Android standard library, run from the UnrealPython plugin directory:

```powershell
powershell -ExecutionPolicy Bypass -File Scripts\sync_official_android_stdlib.ps1 `
  -PluginDir 'D:\Projects\SampleGame\Plugins\UnrealPython'
```

Then validate again:

```powershell
powershell -ExecutionPolicy Bypass -File Scripts\verify_android_python_runtime.ps1 `
  -PluginDir 'D:\Projects\SampleGame\Plugins\UnrealPython'
```

Run a full BuildCookRun from the Windows project root. Always specify `-clientarchitecture` explicitly instead of relying only on `DefaultEngine.ini`; this avoids reusing the wrong architecture when switching between arm64 device packages and x86_64 emulator packages.

Development single-APK arm64 package for a real device:

```powershell
$env:ANDROID_HOME=[Environment]::GetEnvironmentVariable('ANDROID_HOME','User')
$env:ANDROID_SDK_ROOT=[Environment]::GetEnvironmentVariable('ANDROID_SDK_ROOT','User')
$env:JAVA_HOME=[Environment]::GetEnvironmentVariable('JAVA_HOME','User')
$env:NDKROOT=[Environment]::GetEnvironmentVariable('NDKROOT','User')
$env:NDK_ROOT=[Environment]::GetEnvironmentVariable('NDK_ROOT','User')

& 'D:\Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun `
  -project='D:\Projects\SampleGame\SampleGame.uproject' `
  -noP4 `
  -platform=Android `
  -cookflavor=ASTC `
  -clientconfig=Development `
  -clientarchitecture=arm64 `
  -build `
  -cook `
  -stage `
  -pak `
  -package `
  -forcepackagedata `
  -archive `
  -archivedirectory='D:\Projects\SampleGame\Saved\AndroidDevelopmentInApk_arm64' `
  -utf8output
```

`-forcepackagedata` places UE-generated OBB data in the APK as `assets/main.obb.png`, producing a single APK. `UnrealPython_UPL.xml` also adds the Python bootstrap:

```text
assets/unrealpython_bootstrap.zip
```

Static APK check:

```powershell
tar -tf Saved\AndroidDevelopmentInApk_arm64\SampleGame-arm64.apk |
  Select-String 'lib/(arm64-v8a|x86_64)/libUnreal\.so|assets/(main\.obb\.png|unrealpython_bootstrap\.zip)'
```

Expected arm64-only result:

```text
lib/arm64-v8a/libUnreal.so
assets/main.obb.png
assets/unrealpython_bootstrap.zip
```

The first Android Gradle wrapper run downloads `gradle-8.7-all.zip` into:

```text
C:\Users\Nien\.gradle\wrapper\dists\gradle-8.7-all
```

If the network is slow, UAT may spend a long time at `:app:assembleDebug`. As long as `gradle-8.7-all.zip.part` keeps growing, the wrapper is still downloading; it is not a UE cook or link failure.

Use this for an x86_64 emulator APK+OBB package:

```powershell
& 'D:\Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat' BuildCookRun `
  -project='D:\Projects\SampleGame\SampleGame.uproject' `
  -noP4 `
  -platform=Android `
  -cookflavor=ASTC `
  -clientconfig=Development `
  -clientarchitecture=x64 `
  -build `
  -cook `
  -stage `
  -pak `
  -package `
  -archive `
  -archivedirectory='D:\Projects\SampleGame\Saved\AndroidDevelopmentObb_x64' `
  -utf8output
```

Successful output example:

```text
Saved/AndroidDevelopmentObb_x64/SampleGame-x64.apk
Saved/AndroidDevelopmentObb_x64/main.1.com.YourCompany.SampleGame.obb
Saved/AndroidDevelopmentObb_x64/Install_SampleGame-x64.bat
Saved/AndroidDevelopmentObb_x64/Uninstall_SampleGame-x64.bat
Saved/AndroidDevelopmentObb_x64/win-x64/UnrealAndroidFileTool.exe
```

`Install_SampleGame-x64.bat` installs more than the APK; it also pushes the OBB through `UnrealAndroidFileTool.exe`. Prefer this script for manual testing so you do not install only the APK and then fail to find pak/OBB data at runtime.

### Android emulator installation and startup validation

Confirm the emulator is online:

```powershell
$adb=Join-Path ([Environment]::GetEnvironmentVariable('ANDROID_HOME','User')) 'platform-tools\adb.exe'
& $adb devices
```

Install APK and OBB:

```powershell
cd D:\Projects\SampleGame\Saved\AndroidTest
.\Install_SampleGame-x64.bat emulator-5554
```

Launch and capture key logs:

```powershell
$adb=Join-Path ([Environment]::GetEnvironmentVariable('ANDROID_HOME','User')) 'platform-tools\adb.exe'
& $adb -s emulator-5554 logcat -c
& $adb -s emulator-5554 shell monkey -p com.YourCompany.SampleGame -c android.intent.category.LAUNCHER 1
Start-Sleep -Seconds 20
& $adb -s emulator-5554 shell pidof com.YourCompany.SampleGame
& $adb -s emulator-5554 logcat -d -t 1200 |
  Select-String -Pattern 'SampleGame|Unreal|UnrealPython|Python|Fatal|AndroidRuntime|Exception|signal|crash|Py_Initialize|encodings'
```

Key logs seen in the current passing validation:

```text
Mounted main OBB: /storage/emulated/0/Android/obb/com.YourCompany.SampleGame/main.1.com.YourCompany.SampleGame.obb
LogPluginManager: Mounting Project plugin UnrealPython
LogUnrealPython: Added packaged script path: .../Content/Scripts
LogUnrealPython: Extracted UnrealPython bootstrap to: .../Saved/UnrealPythonBootstrap
LogUnrealPython: Added Android Python bootstrap support path: .../UnrealPythonBootstrap/android
LogUnrealPython: Added Android Python bootstrap stdlib path: .../UnrealPythonBootstrap/android/x86_64-linux-android/lib/python3.14
LogUnrealPython: Added Android Python bootstrap script path: .../UnrealPythonBootstrap/Content/Scripts
LogUnrealPython: Python VM init success!
```

And after 60 seconds:

```text
pidof com.YourCompany.SampleGame
8301

topResumedActivity=ActivityRecord{... com.YourCompany.SampleGame/com.epicgames.unreal.GameActivity ...}
```

The following errors should not appear:

```text
LogUnrealPython: Error: Py_InitializeFromConfig() failed: Failed to import encodings module
Fatal error
AndroidRuntime
signal 6
signal 11
```

If the package starts but Python initialization fails, check first:

- Whether only the APK was installed and the OBB was missed.
- Whether `Install_SampleGame-x64.bat` or the arm64 install script successfully pushed `main.<StoreVersion>.<PackageName>.obb`.
- Whether the runtime log contains `Added Android Python bootstrap stdlib path`.
- Whether the project Android ABI matches the device, for example `x86_64-linux-android` for emulator and usually `aarch64-linux-android` for real devices.

## Mac runtime

The Mac runtime is not rebuilt from source. It is extracted from a local Homebrew Python 3.14.5 installation.

Packaged contents:

- Python headers
- `libpython3.14.dylib`
- `lib/python3.14` standard library

Typical source path:

```sh
brew --prefix python@3.14
```

After packaging into the plugin, the core directory layout is:

```text
ThirdParty/python314/Mac/include
ThirdParty/python314/Mac/lib/libpython3.14.dylib
ThirdParty/python314/Mac/lib/python3.14
```

Trimmed content:

```text
test
idlelib
turtledemo
tkinter
ensurepip
venv
__phello__
pydoc_data
__pycache__
*.pyc
symlink
```

Mac linking logic in `Source/UnrealPython/UnrealPython.Build.cs`:

- Adds `ThirdParty/python314/Mac/include`.
- Links `ThirdParty/python314/Mac/lib/libpython3.14.dylib`.
- Copies `libpython3.14.dylib` and the trimmed standard library to the plugin binary output directory.

Mac initialization in `Source/UnrealPython/Private/Core/UPyVirtualMachine.cpp`:

- Prefers the Mac Python standard library path inside the plugin.
- Also supports reading the runtime copied by UBT under `Binaries/Mac/python3.14`.
- Uses `PyConfig.module_search_paths`, avoiding system Python or UE Python.

Validation command:

```sh
'UnrealEngine/Engine/Build/BatchFiles/Mac/Build.sh' \
  SampleGameEditor Mac Development \
  -Project='/PathToYourProject/SampleGame/SampleGame.uproject' \
  -WaitMutex \
  -NoHotReloadFromIDE
```

Check the link result:

```sh
otool -L Plugins/UnrealPython/Binaries/Mac/UnrealEditor-UnrealPython.dylib | grep python
```

Expected output:

```text
@rpath/libpython3.14.dylib
```

## iOS runtime

The iOS runtime is built from CPython 3.14.5 source through CPython's official Apple build script.

Build environment:

```text
Xcode 16.4
iPhoneOS18.5 SDK
iPhoneSimulator18.5 SDK
```

Build command:

```sh
mkdir -p /tmp/unrealpython-python-build
cd /tmp/unrealpython-python-build

curl -fL https://www.python.org/ftp/python/3.14.5/Python-3.14.5.tgz -o Python-3.14.5.tgz
tar -xzf Python-3.14.5.tgz

cd Python-3.14.5
python3.14 Apple build iOS --clean
```

Official script outputs:

```text
/tmp/unrealpython-python-build/Python-3.14.5/cross-build/iOS/Python.xcframework
/tmp/unrealpython-python-build/Python-3.14.5/cross-build/dist/python-3.14.5-iOS-XCframework.tar.gz
```

`Python.xcframework` contains two slices:

```text
ios-arm64/Python.framework
ios-arm64_x86_64-simulator/Python.framework
```

Plugin target layout:

```text
ThirdParty/python314/IOS/Python.xcframework
ThirdParty/python314/IOS/Runtime/IOSDevice/python
ThirdParty/python314/IOS/Runtime/IOSDevice/Frameworks
ThirdParty/python314/IOS/Runtime/IOSSimulator/python
ThirdParty/python314/IOS/Runtime/IOSSimulator/Frameworks
```

## iOS lib-dynload handling

iOS cannot place Python extension modules as bare `.so` files inside the app bundle. CPython's official iOS flow requires every binary extension module to be converted into an independent framework, with a `.fwork` marker left at the original location.

Example for `_ssl`:

Original form:

```text
python/lib/python3.14/lib-dynload/_ssl.cpython-314-iphoneos.so
```

iOS runtime form in the plugin:

```text
Frameworks/_ssl.framework/_ssl
Frameworks/_ssl.framework/Info.plist
Frameworks/_ssl.framework/_ssl.origin
python/lib/python3.14/lib-dynload/_ssl.cpython-314-iphoneos.fwork
```

The `.fwork` file points to the real binary inside the framework:

```text
Frameworks/_ssl.framework/_ssl
```

The `.origin` file points back to the original import location:

```text
python/lib/python3.14/lib-dynload/_ssl.cpython-314-iphoneos.fwork
```

If an extension module has an `.xcprivacy` file, move it into the corresponding framework:

```text
Frameworks/_ssl.framework/PrivacyInfo.xcprivacy
```

After processing, the runtime should not contain bare `.so` files:

```sh
find Plugins/UnrealPython/ThirdParty/python314/IOS/Runtime -name '*.so' -print
```

Expected output: none.

## iOS Build.cs integration

iOS linking logic in `Source/UnrealPython/UnrealPython.Build.cs`:

- Adds headers from `Python.xcframework`.
- Uses `PublicAdditionalFrameworks` to link and copy `Python.xcframework`.
- Adds `CoreFoundation`.
- Adds `dl`.
- Selects the device or simulator runtime based on `Target.Architecture`.
- Copies the matching runtime `python` and `Frameworks` into the app as bundle resources.

Key code:

```csharp
PublicAdditionalFrameworks.Add(
    new Framework("Python", PythonXCFrameworkRelativePath, Framework.FrameworkMode.LinkAndCopy)
);

AdditionalBundleResources.Add(new BundleResource(Path.Combine(PythonRuntimePath, PlatformRuntimeName, "python")));
AdditionalBundleResources.Add(new BundleResource(Path.Combine(PythonRuntimePath, PlatformRuntimeName, "Frameworks")));
```

In UE 5.7, `UnrealArch.IOSSimulator` matches the `ios-arm64_x86_64-simulator` slice and uses the arm64 simulator slice.

## iOS initialization paths

iOS initialization in `Source/UnrealPython/Private/Core/UPyVirtualMachine.cpp`:

- Uses `FPlatformProcess::BaseDir()` to get the app bundle directory.
- Sets Python home to the `python` directory inside the bundle.
- Adds these search paths:

```text
python/lib/python3.14
python/lib/python3.14/lib-dynload
```

Initialization still uses `PyConfig` and does not read UE's built-in Python path.

## iOS build validation

Build command:

```sh
'UnrealEngine/Engine/Build/BatchFiles/Mac/Build.sh' \
  SampleGame IOS Development \
  -Project='/PathToYourProject/SampleGame/SampleGame.uproject' \
  -WaitMutex \
  -NoHotReloadFromIDE
```

If UBT reports:

```text
Missing files required to build IOS targets. Enable IOS as an optional download component in the Epic Games Launcher.
```

the local engine does not have the iOS target components installed. Enable the iOS target platform in the Epic Games Launcher Options for that UE version, or sync a complete `Engine/Platforms/IOS` into the local engine.

## SampleGame iOS packaging flow

This section records the currently usable iOS packaging configuration and commands for `SampleGame`. Project scripts live under `Content/Scripts` and must be staged as Non-UFS files into the app bundle so the UnrealPython runtime can load scripts by file-system path.

### Project configuration

The plugin must be enabled in the project. The current `UnrealPython.uplugin` has `EnabledByDefault=true` and allows the Runtime module on `IOS`:

```json
{
  "Name": "UnrealPython",
  "Type": "Runtime",
  "LoadingPhase": "Default",
  "PlatformAllowList": [
    "Win64",
    "Android",
    "Mac",
    "IOS"
  ]
}
```

If the project does not rely on default plugin enablement, enable it explicitly in `.uproject`:

```json
{
  "Name": "UnrealPython",
  "Enabled": true
}
```

`Config/DefaultGame.ini` must contain:

```ini
[/Script/UnrealEd.ProjectPackagingSettings]
+DirectoriesToAlwaysStageAsNonUFS=(Path="Scripts")

[UnrealPython]
ScriptPath=Content/Scripts
```

Notes:

- `DirectoriesToAlwaysStageAsNonUFS=(Path="Scripts")` corresponds to project `Content/Scripts` and ends up under `cookeddata/<project>/content/scripts`.
- `ScriptPath=Content/Scripts` makes the UnrealPython runtime use the project script directory.
- The iOS app must also include `Python.framework` and extension module frameworks; this is handled by `AdditionalBundleResources` and `PublicAdditionalFrameworks` in the plugin `Build.cs`.

### Compile the iOS target

First build the iOS Development target. With local UE 5.7, the Xcode environment variable below can generate the target metadata required by later stage/package steps:

```sh
env UE_BUILD_FROM_XCODE=1 \
  'UnrealEngine/Engine/Build/BatchFiles/Mac/Build.sh' \
  SampleGame IOS Development \
  -Project='/PathToYourProject/SampleGame/SampleGame.uproject' \
  -WaitMutex \
  -NoHotReloadFromIDE
```

Expected output:

```text
Binaries/IOS/SampleGame.target
```

It should also complete iOS arm64 compilation and linking for `UnrealPython`.

### Cook

During cook, prefer disabling local Zen DDC and falling back to the installed no-Zen cache, to avoid local Zen cache port or lock-file issues affecting automated packaging:

```sh
env UE-SharedDataCachePath=None UE-CloudDataCachePath=None uebp_CodeSignWhenStaging=0 \
  'UnrealEngine/Engine/Build/BatchFiles/RunUAT.sh' BuildCookRun \
  -project='/PathToYourProject/SampleGame/SampleGame.uproject' \
  -noP4 \
  -platform=IOS \
  -clientconfig=Development \
  -cook \
  -stage \
  -package \
  -archive \
  -archivedirectory='/PathToYourProject/SampleGame/Saved/IOSPackage' \
  -pak \
  -iostore \
  -NoCodeSign \
  -skipbuild \
  -AdditionalCookerOptions='-DDC=InstalledNoZenLocalFallback'
```

The cook log should contain:

```text
LogUnrealPython: Python VM init success!
```

### Package and archive

If the machine does not have an Apple Team, certificate, or provisioning profile, first generate an unsigned self-contained `.app`:

```sh
env uebp_CodeSignWhenStaging=0 \
  'UnrealEngine/Engine/Build/BatchFiles/RunUAT.sh' BuildCookRun \
  -project='/PathToYourProject/SampleGame/SampleGame.uproject' \
  -noP4 \
  -platform=IOS \
  -clientconfig=Development \
  -skipcook \
  -stage \
  -package \
  -archive \
  -archivedirectory='/PathToYourProject/SampleGame/Saved/IOSPackage' \
  -pak \
  -iostore \
  -NoCodeSign \
  -skipbuild \
  -xcodebuildoptions='CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY= AD_HOC_CODE_SIGNING_ALLOWED=YES'
```

Current output path:

```text
Saved/IOSPackage/SampleGame.app
```

An unsigned package cannot be installed directly on a real device. Real-device execution requires a valid Apple Team, Signing Certificate, and Provisioning Profile in the project iOS settings or Xcode project, followed by resigning or repackaging.

### Artifact validation

Check that the Python runtime, extension frameworks, script directory, and IoStore package entered the `.app`:

```sh
find Saved/IOSPackage/SampleGame.app -maxdepth 5 \
  \( -path '*/content/scripts/ue_site.py' \
     -o -path '*/content/scripts/unreal_timer.py' \
     -o -path '*/Frameworks/Python.framework' \
     -o -path '*/Frameworks/zlib.framework' \
     -o -name 'SampleGame-ios.utoc' \) \
  -print

find Saved/IOSPackage/SampleGame.app/cookeddata/SampleGame/content/scripts -type f | wc -l
codesign -dv --verbose=2 Saved/IOSPackage/SampleGame.app
```

For an unsigned artifact, `codesign` is expected to include:

```text
code object is not signed at all
```

### Common issues

- If stage/package reports `Missing receipt ... Binaries/IOS/SampleGame.target`, first run the iOS compile command with `UE_BUILD_FROM_XCODE=1`.
- If Xcode stage reports missing `Build/IOS/UBTGenerated/Info.Template.plist`, copy the generated `Intermediate/IOS/SampleGame-Info.plist` to that path and package again.
- If Xcode reports `Signing for SampleGame requires a development team`, use the no-sign `xcodebuildoptions` above to generate a local `.app`; real-device execution still requires valid signing.
- If UAT cook hangs on a Zen DDC connection or port error, cook again with `-AdditionalCookerOptions='-DDC=InstalledNoZenLocalFallback'`.

## File validation

Check the iOS Python framework:

```sh
file Plugins/UnrealPython/ThirdParty/python314/IOS/Python.xcframework/ios-arm64/Python.framework/Python
otool -L Plugins/UnrealPython/ThirdParty/python314/IOS/Python.xcframework/ios-arm64/Python.framework/Python
```

Expected:

```text
Mach-O 64-bit dynamically linked shared library arm64
@rpath/Python.framework/Python
```

Check an iOS extension framework:

```sh
file Plugins/UnrealPython/ThirdParty/python314/IOS/Runtime/IOSDevice/Frameworks/_ssl.framework/_ssl
otool -L Plugins/UnrealPython/ThirdParty/python314/IOS/Runtime/IOSDevice/Frameworks/_ssl.framework/_ssl
```

Expected `_ssl` dependency:

```text
@rpath/Python.framework/Python
```

Check size:

```sh
du -sh Plugins/UnrealPython/ThirdParty/python314/IOS
du -sh Plugins/UnrealPython/ThirdParty/python314/Mac
```

## Notes

- Do not link UE's built-in Python.
- Do not copy iOS `lib-dynload/*.so` files directly into the app as resources.
- Do not commit CPython test directories, pyc files, or symlinks into the plugin runtime.
- If UE's PythonScriptPlugin is enabled, it may initialize a different Python interpreter earlier in the same process; this plugin should avoid being enabled together with it.
- iOS frameworks must be signed in the final app package. UE's iOS packaging flow handles frameworks under the `Frameworks` directory.
