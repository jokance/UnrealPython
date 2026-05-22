# Python 3.14 Runtime 构建与打包流程

本文记录 `UnrealPython` 插件当前自带 Python 3.14 运行时的来源、构建方式、目录结构和验证方法。

插件不使用 UE 自带 Python。原因是 UE 内置 Python 版本和插件目标版本不同，混用会带来 ABI、符号和 `sys.path` 冲突。插件应始终链接并初始化自己的 Python 运行时。

## 目标版本

- Python: `3.14.5`
- 插件目录名: `ThirdParty/python314`
- Mac 运行时: `ThirdParty/python314/Mac`
- iOS 运行时: `ThirdParty/python314/IOS`

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
