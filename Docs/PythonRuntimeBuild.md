# Python 3.14 Runtime 构建与打包流程

本文记录 `UnrealPython` 插件当前自带 Python 3.14 运行时的来源、构建方式、目录结构和验证方法。

插件不使用 UE 自带 Python。原因是 UE 内置 Python 版本和插件目标版本不同，混用会带来 ABI、符号和 `sys.path` 冲突。插件应始终链接并初始化自己的 Python 运行时。

## 目标版本

- Python: `3.14.5`
- 插件目录名: `ThirdParty/python314`
- Android 运行时: `ThirdParty/python314/android`
- Mac 运行时: `ThirdParty/python314/Mac`
- iOS 运行时: `ThirdParty/python314/IOS`

## Windows 运行时

Windows 运行时使用 CPython embeddable package 形态。插件不解压 `python314.zip`，运行时直接把 zip 文件加入 `PyConfig.module_search_paths`，由 Python 的 zipimport 读取标准库。

插件内关键文件：

```text
ThirdParty/python314/include
ThirdParty/python314/Win64/libs/python314.lib
ThirdParty/python314/Win64/python314.dll
ThirdParty/python314/Win64/python314.zip
```

`python314.zip` 是标准库压缩包。它需要和游戏 exe 放在同一目录，不能只留在插件 ThirdParty 目录下。`Source/UnrealPython/UnrealPython.Build.cs` 的 Win64 配置会：

- 添加 `ThirdParty/python314/include`
- 链接 `ThirdParty/python314/Win64/libs/python314.lib`
- 在 Editor 构建中把 `python314.dll` 和 `python314.zip` staged 到 `$(BinaryOutputDir)`
- 在非 Editor 构建中把 `python314.dll` 和 `python314.zip` staged 到 `$(TargetOutputDir)`

Windows 初始化逻辑在 `Source/UnrealPython/Private/Core/UPyVirtualMachine.cpp`。运行时会检查：

```text
<GameExeDir>/python314.zip
```

如果存在，会记录日志：

```text
LogUnrealPython: Added bundled Windows Python path: .../python314.zip
```

### Windows 项目配置

项目脚本默认放在：

```text
Content/Scripts
```

打包时需要作为 Non-UFS 文件 staged，这样包体运行时可以按文件系统路径读取脚本。

推荐在项目 `Config/DefaultGame.ini` 中配置：

```ini
[/Script/UnrealEd.ProjectPackagingSettings]
+DirectoriesToAlwaysStageAsNonUFS=(Path="Scripts")

[UnrealPython]
ScriptPath=Content/Scripts
```

说明：

- `DirectoriesToAlwaysStageAsNonUFS=(Path="Scripts")` 对应项目 `Content/Scripts`。
- `[UnrealPython] ScriptPath=Content/Scripts` 保持编辑器和包体使用同一套脚本目录约定。
- 运行时不再复制项目脚本；脚本目录必须在 stage/package 阶段进入包体。

### Windows 打包流程

从 Windows 项目根目录执行完整 BuildCookRun：

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

如果只需要验证插件模块能编译，可以先跑 Win64 target build：

```bat
"D:\Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" SampleGame Win64 Development ^
  -Project="D:\Projects\SampleGame\SampleGame.uproject" ^
  -WaitMutex ^
  -NoHotReloadFromIDE
```

### Windows 包体验证

打包成功后检查 archive 目录中是否存在：

```text
SampleGame.exe
python314.dll
python314.zip
Content/Scripts
```

启动包体后重点检查日志：

```text
LogUnrealPython: Added bundled Windows Python path: .../python314.zip
LogUnrealPython: Python VM init success!
```

如果出现：

```text
Py_InitializeFromConfig() failed: Failed to import encodings module
```

优先检查 `python314.zip` 是否和 exe 在同一目录，以及 UBT 是否执行了 Win64 `RuntimeDependencies` staging。

## Android 运行时

Android 运行时从 CPython 3.14.5 源码构建，使用 CPython 官方 `Android/android.py` 交叉编译流程作为基础。CPython 官方 Android release package 默认产出 shared `libpython3.14.so`；本插件的 Android 接入选择静态链接，所以构建脚本会在源码构建目录中把官方脚本的 `--enable-shared --without-static-libpython` 调整为 `--disable-shared --with-static-libpython`，再执行官方 build 流程。

Android 标准库使用 Python 官方 Android embeddable package 同步到插件 ThirdParty 目录。当前同步脚本为 `Scripts/sync_official_android_stdlib.ps1`，默认下载：

```text
https://www.python.org/ftp/python/3.14.2/python-3.14.2-aarch64-linux-android.tar.gz
https://www.python.org/ftp/python/3.14.2/python-3.14.2-x86_64-linux-android.tar.gz
```

同步后的目录结构：

```text
ThirdParty/python314/android/_android_support.py
ThirdParty/python314/android/aarch64-linux-android/lib/python3.14
ThirdParty/python314/android/x86_64-linux-android/lib/python3.14
```

`_android_support.py` 来自官方包的 `prefix/lib/python3.14/_android_support.py`，插件只补了 `fileno()` 兼容保护，避免 UE 嵌入环境中 stdout/stderr 不支持 `fileno()` 时初始化失败。

参考资料：

- CPython Android 使用说明: <https://docs.python.org/3/using/android.html>
- CPython Android 构建脚本说明: <https://github.com/python/cpython/blob/3.14/Android/README.md>
- CPython 3.14.5 发布目录: <https://www.python.org/ftp/python/3.14.5/>

构建环境：

```text
Linux, macOS, or WSL
Android SDK, with ANDROID_HOME pointing to the SDK root
Java, curl, tar, make, python3
```

默认目标 ABI：

```text
aarch64-linux-android
x86_64-linux-android
```

当前插件只提交这两个 host。UE Android 正式包一般使用 `aarch64-linux-android`；`x86_64-linux-android` 用于模拟器或需要 x86_64 Android target 的场景。

### Android 开发机配置

Windows 开发机需要先安装 UE 对应版本的 Android target platform 组件，并保证引擎目录下存在 Android target 文件，例如：

```text
D:\Games\UE_5.7\Engine\Binaries\Android\UnrealGame.target
D:\Games\UE_5.7\Engine\Build\Android
D:\Games\UE_5.7\Engine\Build\Android\Java
```

当前本机验证过的 SDK/JDK/NDK 环境变量：

```powershell
ANDROID_HOME=C:\Users\Nien\AppData\Local\Android\Sdk
ANDROID_SDK_ROOT=C:\Users\Nien\AppData\Local\Android\Sdk
JAVA_HOME=C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot
NDKROOT=C:\Users\Nien\AppData\Local\Android\Sdk\ndk\27.2.12479018
NDK_ROOT=C:\Users\Nien\AppData\Local\Android\Sdk\ndk\27.2.12479018
ANDROID_AVD_HOME=D:\Android\avd
```

模拟器测试使用 x86_64 ABI，所以项目里需要开启 x86_64 Android 包。当前 `Config/DefaultEngine.ini` 中使用：

```ini
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
bBuildForArm64=False
bBuildForX8664=True
SDKAPILevel=android-34
NDKAPILevel=android-26
```

真机 arm64 测试时通常应改回：

```ini
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
bBuildForArm64=True
bBuildForX8664=False
SDKAPILevel=android-34
NDKAPILevel=android-26
```

### Android 构建命令

从项目根目录执行：

```sh
export ANDROID_HOME=/path/to/android-sdk

Plugins/UnrealPython/Scripts/build_android_python_runtime.sh \
  --host aarch64-linux-android \
  --host x86_64-linux-android
```

可选参数：

```sh
# 清理脚本管理的 CPython source/build 目录后重建
Plugins/UnrealPython/Scripts/build_android_python_runtime.sh --clean

# 只构建 Android arm64
Plugins/UnrealPython/Scripts/build_android_python_runtime.sh --host aarch64-linux-android

# 使用自定义工作目录，避免占用项目 Intermediate
Plugins/UnrealPython/Scripts/build_android_python_runtime.sh --work-dir /tmp/unrealpython-python-android-build
```

脚本会完成以下步骤：

1. 下载 `https://www.python.org/ftp/python/3.14.5/Python-3.14.5.tgz`。
2. 解压到 `Intermediate/UnrealPython/PythonAndroidBuild`。
3. 基于 CPython `Android/android.py` 构建指定 host。
4. 生成并安装 CPython host prefix。
5. 把头文件和静态库复制到插件：

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

其中 `libpython3.14.a`、`libssl.a`、`libcrypto.a` 是原始 CPython/OpenSSL 静态库；`libUnrealPython3.14.a`、`libUnrealPythonSSL.a`、`libUnrealPythonCrypto.a` 是给 UE Android 链接使用的改写后产物。不要手工删除这三份 `libUnrealPython*` 库。

### Android Build.cs 接入

Android 链接逻辑在 `Source/UnrealPython/UnrealPython.Build.cs`：

- 根据 `Target.Architecture` 选择 `aarch64-linux-android` 或 `x86_64-linux-android`
- 添加 `<host>/include` 和 `<host>/include/python3.14`
- 链接 `libUnrealPython3.14.a`
- 链接 OpenSSL、bz2、ffi、lzma、zstd、mpdec 等 CPython 模块依赖的静态库
- Android 上 UE 自带 OpenSSL 1.1.1 的库路径可能先于插件库路径出现，且最终 Android game `.so` 会把所有静态库链接进同一个二进制。插件使用 `libUnrealPython3.14.a`、`libUnrealPythonSSL.a` 和 `libUnrealPythonCrypto.a` 作为改写后的 Android 链接产物：OpenSSL 3 导出符号会加上 `UPY_OPENSSL3_` 前缀，Python 中 `_ssl/_hashopenssl` 对 OpenSSL 的引用也会同步改写，避免和 UE 自带 OpenSSL 冲突。
- 构建时生成 `Plugins/UnrealPython/Intermediate/Android/unrealpython_bootstrap.zip`，其中包含：
  - `android/_android_support.py`
  - `android/<host>/lib/python3.14`
  - 项目 `Content/Scripts`
- 用 bootstrap zip 的 SHA256 写入 `UPY_PYTHON_BOOTSTRAP_VERSION`。
- 通过 `UnrealPython_UPL.xml` 把 `unrealpython_bootstrap.zip` 拷贝到 APK 的 `assets/unrealpython_bootstrap.zip`。
- 如果 host 目录或任一必需库缺失，UBT 会直接抛出 `BuildException`

Android cook 会以 commandlet 方式加载插件。当前插件在 commandlet 下跳过 Python VM 初始化，避免 Windows cook 阶段因为没有 Win64 标准库 `encodings` 而失败；`StartupModule()` 和 `ShutdownModule()` 都需要保持这个对称跳过逻辑。

Android 包体运行时会在 Python 初始化前通过 Java 从 APK assets 解压 `unrealpython_bootstrap.zip` 到：

```text
<ExternalFilesDir>/UnrealGame/<Project>/<Project>/Saved/UnrealPythonBootstrap
```

随后把以下路径加入 `PyConfig.module_search_paths`：

```text
.../UnrealPythonBootstrap/android
.../UnrealPythonBootstrap/android/<host>/lib/python3.14
.../UnrealPythonBootstrap/Content/Scripts
```

`Content/Scripts` 仍会被 UE 打进 pak/OBB 数据中，但 Android 上 Python 启动依赖的是 APK assets 里的 bootstrap zip，避免 CPython Android 初始化需要真实文件系统路径时直接读取 pak/OBB 失败。

注意：`unrealpython_bootstrap.zip` 文件名在 arm64 和 x86_64 架构间共享。如果在同一工作区频繁切换 `-clientarchitecture=x64` 和 `-clientarchitecture=arm64`，必须确保 UBT 重新执行 `UnrealPython.Build.cs`。出现 arm64 真机日志仍解压出 `x86_64-linux-android` 时，先删除：

```text
Plugins/UnrealPython/Intermediate/Android/unrealpython_bootstrap.zip
Plugins/UnrealPython/Intermediate/Android/BootstrapStaging
```

然后用显式 `-clientarchitecture=arm64` 重新打包。

### Android 工程内验证

在 Windows 开发机上，从项目根目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File Plugins\UnrealPython\Scripts\verify_android_python_runtime.ps1
```

验证脚本会检查：

- `aarch64-linux-android` 和 `x86_64-linux-android` host 目录存在
- `Python.h` 和 `pyconfig.h` 存在
- `pyconfig.h` 中 `ANDROID_API_LEVEL` 为 `24`
- `pyconfig.h` 中 `Py_ENABLE_SHARED` 未启用，确认当前产物是静态 libpython 形态
- 所有必需 `.a` 文件存在、大小正常，并且是 Unix static archive 格式
- `UnrealPython.Build.cs` 中包含 Android host 选择和所有必需库引用
- `ThirdParty/python314/android/_android_support.py` 存在并包含 Android logcat bridge
- 每个 host 的 `lib/python3.14` 标准库存在，并包含 Android sysconfig 文件
- `SampleGame.uproject` 中已启用 `UnrealPython`

验证通过后再执行 UE Android 编译：

```bat
"%UE_ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat" SampleGame Android Development ^
  -Project="D:\Projects\SampleGame\SampleGame.uproject" ^
  -WaitMutex ^
  -NoHotReloadFromIDE
```

如果只想验证插件模块能被 UBT 解析，先运行 `verify_android_python_runtime.ps1`，再运行 Android target build。前者验证第三方 Python runtime 文件和工程接入；后者验证 UE/NDK/链接器实际接受这些静态库。

### Android 打包流程

打包前确认项目 Android ABI 与要验证的设备一致：

```ini
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
; Android 模拟器
bBuildForArm64=False
bBuildForX8664=True

; Android 真机
; bBuildForArm64=True
; bBuildForX8664=False
```

如果要重新同步官方 Android 标准库，从 UnrealPython 插件目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File Scripts\sync_official_android_stdlib.ps1 `
  -PluginDir 'D:\Projects\SampleGame\Plugins\UnrealPython'
```

同步后再执行：

```powershell
powershell -ExecutionPolicy Bypass -File Scripts\verify_android_python_runtime.ps1 `
  -PluginDir 'D:\Projects\SampleGame\Plugins\UnrealPython'
```

从 Windows 项目根目录执行完整 BuildCookRun。建议始终显式指定 `-clientarchitecture`，不要只依赖 `DefaultEngine.ini`，避免在 arm64 真机包和 x86_64 模拟器包之间切换时复用错误架构。

真机 arm64 单 APK Development 包：

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

`-forcepackagedata` 会把 UE 生成的 OBB 数据作为 `assets/main.obb.png` 放入 APK，最终产物是单 APK。`UnrealPython_UPL.xml` 还会把 Python bootstrap 放入：

```text
assets/unrealpython_bootstrap.zip
```

静态检查 APK：

```powershell
tar -tf Saved\AndroidDevelopmentInApk_arm64\SampleGame-arm64.apk |
  Select-String 'lib/(arm64-v8a|x86_64)/libUnreal\.so|assets/(main\.obb\.png|unrealpython_bootstrap\.zip)'
```

预期只包含 arm64：

```text
lib/arm64-v8a/libUnreal.so
assets/main.obb.png
assets/unrealpython_bootstrap.zip
```

首次运行 Android Gradle wrapper 时会下载 `gradle-8.7-all.zip`，缓存位置为：

```text
C:\Users\Nien\.gradle\wrapper\dists\gradle-8.7-all
```

如果网络较慢，UAT 可能长时间停在 `:app:assembleDebug`。只要 `gradle-8.7-all.zip.part` 持续增长，就表示 wrapper 仍在下载，不是 UE cook 或链接失败。

模拟器 x86_64 APK+OBB 包使用：

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

成功输出示例：

```text
Saved/AndroidDevelopmentObb_x64/SampleGame-x64.apk
Saved/AndroidDevelopmentObb_x64/main.1.com.YourCompany.SampleGame.obb
Saved/AndroidDevelopmentObb_x64/Install_SampleGame-x64.bat
Saved/AndroidDevelopmentObb_x64/Uninstall_SampleGame-x64.bat
Saved/AndroidDevelopmentObb_x64/win-x64/UnrealAndroidFileTool.exe
```

`Install_SampleGame-x64.bat` 不只是安装 APK，还会通过 `UnrealAndroidFileTool.exe` 推送 OBB。手工测试时优先使用这个脚本，避免只装 APK 导致运行时找不到 pak/obb 数据。

### Android 模拟器安装与启动验证

确认模拟器在线：

```powershell
$adb=Join-Path ([Environment]::GetEnvironmentVariable('ANDROID_HOME','User')) 'platform-tools\adb.exe'
& $adb devices
```

安装 APK 和 OBB：

```powershell
cd D:\Projects\SampleGame\Saved\AndroidTest
.\Install_SampleGame-x64.bat emulator-5554
```

启动并抓取关键日志：

```powershell
$adb=Join-Path ([Environment]::GetEnvironmentVariable('ANDROID_HOME','User')) 'platform-tools\adb.exe'
& $adb -s emulator-5554 logcat -c
& $adb -s emulator-5554 shell monkey -p com.YourCompany.SampleGame -c android.intent.category.LAUNCHER 1
Start-Sleep -Seconds 20
& $adb -s emulator-5554 shell pidof com.YourCompany.SampleGame
& $adb -s emulator-5554 logcat -d -t 1200 |
  Select-String -Pattern 'SampleGame|Unreal|UnrealPython|Python|Fatal|AndroidRuntime|Exception|signal|crash|Py_Initialize|encodings'
```

当前验证通过时看到的关键日志：

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

并且 60 秒后：

```text
pidof com.YourCompany.SampleGame
8301

topResumedActivity=ActivityRecord{... com.YourCompany.SampleGame/com.epicgames.unreal.GameActivity ...}
```

不应出现以下错误：

```text
LogUnrealPython: Error: Py_InitializeFromConfig() failed: Failed to import encodings module
Fatal error
AndroidRuntime
signal 6
signal 11
```

如果包能启动但 Python 初始化失败，优先检查：

- 是否只安装了 APK，漏推 OBB。
- `Install_SampleGame-x64.bat` 或 arm64 安装脚本是否成功推送 `main.<StoreVersion>.<PackageName>.obb`。
- 运行日志里是否出现 `Added Android Python bootstrap stdlib path`。
- 项目 Android ABI 是否和运行设备一致，例如模拟器需要 `x86_64-linux-android`，真机通常需要 `aarch64-linux-android`。

## Mac 运行时

Mac 运行时没有从源码重新构建，而是从本机 Homebrew 安装的 Python 3.14.5 提取运行时文件。

打包内容：

- Python 头文件
- `libpython3.14.dylib`
- `lib/python3.14` 标准库

典型来源路径：

```sh
brew --prefix python@3.14
```

打包到插件后，核心目录如下：

```text
ThirdParty/python314/Mac/include
ThirdParty/python314/Mac/lib/libpython3.14.dylib
ThirdParty/python314/Mac/lib/python3.14
```

裁剪内容：

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

Mac 链接逻辑在 `Source/UnrealPython/UnrealPython.Build.cs`：

- 添加 `ThirdParty/python314/Mac/include`
- 链接 `ThirdParty/python314/Mac/lib/libpython3.14.dylib`
- 把 `libpython3.14.dylib` 和裁剪后的标准库复制到插件二进制输出目录

Mac 初始化逻辑在 `Source/UnrealPython/Private/Core/UPyVirtualMachine.cpp`：

- 优先加入插件内 Mac Python 标准库路径
- 同时支持从 `Binaries/Mac/python3.14` 读取被 UBT 拷贝后的运行时
- 使用 `PyConfig.module_search_paths`，避免依赖系统 Python 或 UE Python

验证命令：

```sh
'UnrealEngine/Engine/Build/BatchFiles/Mac/Build.sh' \
  SampleGameEditor Mac Development \
  -Project='/PathToYourProject/SampleGame/SampleGame.uproject' \
  -WaitMutex \
  -NoHotReloadFromIDE
```

检查链接结果：

```sh
otool -L Plugins/UnrealPython/Binaries/Mac/UnrealEditor-UnrealPython.dylib | grep python
```

预期能看到：

```text
@rpath/libpython3.14.dylib
```

## iOS 运行时

iOS 运行时从 CPython 3.14.5 源码构建，使用 CPython 官方 Apple 构建脚本。

构建环境：

```text
Xcode 16.4
iPhoneOS18.5 SDK
iPhoneSimulator18.5 SDK
```

构建命令：

```sh
mkdir -p /tmp/unrealpython-python-build
cd /tmp/unrealpython-python-build

curl -fL https://www.python.org/ftp/python/3.14.5/Python-3.14.5.tgz -o Python-3.14.5.tgz
tar -xzf Python-3.14.5.tgz

cd Python-3.14.5
python3.14 Apple build iOS --clean
```

官方脚本产物：

```text
/tmp/unrealpython-python-build/Python-3.14.5/cross-build/iOS/Python.xcframework
/tmp/unrealpython-python-build/Python-3.14.5/cross-build/dist/python-3.14.5-iOS-XCframework.tar.gz
```

`Python.xcframework` 包含两个 slice：

```text
ios-arm64/Python.framework
ios-arm64_x86_64-simulator/Python.framework
```

插件内目标结构：

```text
ThirdParty/python314/IOS/Python.xcframework
ThirdParty/python314/IOS/Runtime/IOSDevice/python
ThirdParty/python314/IOS/Runtime/IOSDevice/Frameworks
ThirdParty/python314/IOS/Runtime/IOSSimulator/python
ThirdParty/python314/IOS/Runtime/IOSSimulator/Frameworks
```

## iOS lib-dynload 处理

iOS 不能把 Python 扩展模块作为裸 `.so` 放进 app bundle。CPython iOS 官方要求每个二进制扩展模块转换成独立 framework，并在原位置留下 `.fwork` 标记文件。

例如 `_ssl`：

原始形态：

```text
python/lib/python3.14/lib-dynload/_ssl.cpython-314-iphoneos.so
```

插件内 iOS 运行时形态：

```text
Frameworks/_ssl.framework/_ssl
Frameworks/_ssl.framework/Info.plist
Frameworks/_ssl.framework/_ssl.origin
python/lib/python3.14/lib-dynload/_ssl.cpython-314-iphoneos.fwork
```

`.fwork` 文件内容指向 framework 内的真实二进制：

```text
Frameworks/_ssl.framework/_ssl
```

`.origin` 文件内容指回原始 import 位置：

```text
python/lib/python3.14/lib-dynload/_ssl.cpython-314-iphoneos.fwork
```

如果扩展模块带有 `.xcprivacy` 文件，需要移动到对应 framework：

```text
Frameworks/_ssl.framework/PrivacyInfo.xcprivacy
```

处理完成后，runtime 中不应再存在裸 `.so`：

```sh
find Plugins/UnrealPython/ThirdParty/python314/IOS/Runtime -name '*.so' -print
```

预期无输出。

## iOS Build.cs 接入

iOS 链接逻辑在 `Source/UnrealPython/UnrealPython.Build.cs`：

- 添加 `Python.xcframework` 里的 Headers
- 使用 `PublicAdditionalFrameworks` 链接并拷贝 `Python.xcframework`
- 添加 `CoreFoundation`
- 添加 `dl`
- 根据 `Target.Architecture` 选择设备或模拟器 runtime
- 把对应 runtime 的 `python` 和 `Frameworks` 作为 bundle resource 拷进 app

关键点：

```csharp
PublicAdditionalFrameworks.Add(
    new Framework("Python", PythonXCFrameworkRelativePath, Framework.FrameworkMode.LinkAndCopy)
);

AdditionalBundleResources.Add(new BundleResource(Path.Combine(PythonRuntimePath, PlatformRuntimeName, "python")));
AdditionalBundleResources.Add(new BundleResource(Path.Combine(PythonRuntimePath, PlatformRuntimeName, "Frameworks")));
```

UE 5.7 中 `UnrealArch.IOSSimulator` 会匹配 `ios-arm64_x86_64-simulator` slice，并按 arm64 simulator 使用。

## iOS 初始化路径

iOS 初始化逻辑在 `Source/UnrealPython/Private/Core/UPyVirtualMachine.cpp`：

- 用 `FPlatformProcess::BaseDir()` 获取 app bundle 目录
- 设置 Python home 为 bundle 内 `python`
- 加入以下 search paths：

```text
python/lib/python3.14
python/lib/python3.14/lib-dynload
```

初始化仍使用 `PyConfig`，不读取 UE 内置 Python 路径。

## iOS 构建验证

构建命令：

```sh
'UnrealEngine/Engine/Build/BatchFiles/Mac/Build.sh' \
  SampleGame IOS Development \
  -Project='/PathToYourProject/SampleGame/SampleGame.uproject' \
  -WaitMutex \
  -NoHotReloadFromIDE
```

如果 UBT 报错：

```text
Missing files required to build IOS targets. Enable IOS as an optional download component in the Epic Games Launcher.
```

说明当前本地引擎没有安装 iOS target 组件。需要在 Epic Games Launcher 的 UE 版本 Options 里勾选 iOS target platform，或把安装完整的 `Engine/Platforms/IOS` 同步到当前使用的本地引擎。

## SampleGame iOS 打包流程

本节记录 `SampleGame` 项目当前可用的 iOS 打包配置和命令。项目脚本放在 `Content/Scripts`，打包时需要作为 Non-UFS 文件进入 app bundle，这样 UnrealPython 运行时可以按文件系统路径加载脚本。

### 项目配置

插件需要在项目中启用。当前 `UnrealPython.uplugin` 已设置 `EnabledByDefault=true`，并允许 Runtime 模块用于 `IOS`：

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

如果项目不依赖插件默认启用，也可以在 `.uproject` 中显式启用：

```json
{
  "Name": "UnrealPython",
  "Enabled": true
}
```

`Config/DefaultGame.ini` 需要包含：

```ini
[/Script/UnrealEd.ProjectPackagingSettings]
+DirectoriesToAlwaysStageAsNonUFS=(Path="Scripts")

[UnrealPython]
ScriptPath=Content/Scripts
```

说明：

- `DirectoriesToAlwaysStageAsNonUFS=(Path="Scripts")` 对应项目里的 `Content/Scripts`，最终会进入 `cookeddata/<project>/content/scripts`。
- `ScriptPath=Content/Scripts` 让 UnrealPython 运行时使用项目脚本目录。
- iOS app 内还需要包含 `Python.framework` 和扩展模块 framework，这部分由插件 `Build.cs` 的 `AdditionalBundleResources` 和 `PublicAdditionalFrameworks` 处理。

### 编译 iOS 目标

先生成 iOS Development 目标。使用本地 UE 5.7 时，下面的 Xcode 环境变量可以生成后续 stage/package 需要的 target metadata：

```sh
env UE_BUILD_FROM_XCODE=1 \
  'UnrealEngine/Engine/Build/BatchFiles/Mac/Build.sh' \
  SampleGame IOS Development \
  -Project='/PathToYourProject/SampleGame/SampleGame.uproject' \
  -WaitMutex \
  -NoHotReloadFromIDE
```

预期会生成：

```text
Binaries/IOS/SampleGame.target
```

并完成 `UnrealPython` 的 iOS arm64 编译和链接。

### Cook

Cook 时建议禁用本地 Zen DDC 回退到 installed no-Zen cache，避免本机 Zen cache 端口或锁文件异常影响自动化打包：

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

Cook 日志里应能看到 UnrealPython 初始化成功：

```text
LogUnrealPython: Python VM init success!
```

### Package 和 Archive

如果本机没有 Apple Team、证书和 provisioning profile，可以先生成未签名的自包含 `.app`：

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

当前产物路径：

```text
Saved/IOSPackage/SampleGame.app
```

未签名包不能直接安装到真机。真机运行需要在项目 iOS 设置或 Xcode 工程里配置有效的 Apple Team、Signing Certificate 和 Provisioning Profile，然后重新签名或重新打包。

### 产物验证

检查 Python runtime、扩展 framework、脚本目录和 IoStore 包是否进入 `.app`：

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

未签名产物的 `codesign` 预期输出包含：

```text
code object is not signed at all
```

### 常见问题

- 如果 stage/package 报 `Missing receipt ... Binaries/IOS/SampleGame.target`，先执行带 `UE_BUILD_FROM_XCODE=1` 的 iOS 编译命令。
- 如果 Xcode stage 报缺少 `Build/IOS/UBTGenerated/Info.Template.plist`，可以从当前生成的 `Intermediate/IOS/SampleGame-Info.plist` 复制一份到该路径后重新 package。
- 如果 Xcode 报 `Signing for SampleGame requires a development team`，使用上面的无签名 `xcodebuildoptions` 只生成本地 `.app`；真机运行仍需要有效签名。
- 如果 UAT cook 卡在 Zen DDC 连接或端口错误，使用 `-AdditionalCookerOptions='-DDC=InstalledNoZenLocalFallback'` 重新 cook。

## 文件验证

检查 iOS Python framework：

```sh
file Plugins/UnrealPython/ThirdParty/python314/IOS/Python.xcframework/ios-arm64/Python.framework/Python
otool -L Plugins/UnrealPython/ThirdParty/python314/IOS/Python.xcframework/ios-arm64/Python.framework/Python
```

预期：

```text
Mach-O 64-bit dynamically linked shared library arm64
@rpath/Python.framework/Python
```

检查 iOS 扩展 framework：

```sh
file Plugins/UnrealPython/ThirdParty/python314/IOS/Runtime/IOSDevice/Frameworks/_ssl.framework/_ssl
otool -L Plugins/UnrealPython/ThirdParty/python314/IOS/Runtime/IOSDevice/Frameworks/_ssl.framework/_ssl
```

预期 `_ssl` 依赖插件内 Python framework：

```text
@rpath/Python.framework/Python
```

检查体积：

```sh
du -sh Plugins/UnrealPython/ThirdParty/python314/IOS
du -sh Plugins/UnrealPython/ThirdParty/python314/Mac
```

## 注意事项

- 不要链接 UE 自带 Python。
- 不要把 iOS `lib-dynload/*.so` 直接作为资源拷进 app。
- 不要把 CPython 的测试目录、pyc、symlink 一起提交进插件 runtime。
- 如果启用 UE 的 PythonScriptPlugin，可能会在同一进程里提前初始化另一个 Python 解释器；插件应避免与它同时启用。
- iOS framework 在最终 app 打包时需要被签名，UE 的 iOS 打包流程会处理 `Frameworks` 目录内的 framework。
