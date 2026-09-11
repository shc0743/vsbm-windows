# vsbm for Windows

这是原 WebGL / GLSL 版本的原生 Win32 + Direct3D 11 实现。

## 功能

- Direct3D 11 Pixel Shader 执行 Raymarching / fractal 计算
- 自动旋转；Space 暂停/继续
- 鼠标左键旋转、右键平移、滚轮缩放
- Windows Touch：单指旋转、双指平移 + 捏合缩放
- 主窗口客户区保持正方形渲染
- 使用 `vsbm.ico` 作为窗口和通知区图标
- 任务栏通知区域（系统托盘）图标：
  - 左键：显示/恢复主窗口
  - 右键：`&Show`、`E&xit`
- 系统菜单追加：
  - `&Kernel...`
  - `Hide to taskbar`
  - `Hide while working`
  - `&Help...`

## 暂停

按 Space 切换暂停状态。

暂停时不推进自动旋转，也不提交新的动画帧；标题会变为：

`vsbm for Windows - Paused`

继续后恢复正常自动旋转。

## Hide to taskbar

执行后：

1. 保存当前暂停状态。
2. 强制进入暂停。
3. 先 `ShowWindow(SW_MINIMIZE)`，让 Windows 播放正常的最小化动画。
4. 再 `ShowWindow(SW_HIDE)`。

窗口任务栏按钮消失，但通知区图标仍然存在。

从通知区 `Show` 恢复时，程序按要求先调用 `ShowWindow(SW_MINIMIZE)` 取消隐藏，再调用 `ShowWindow(SW_RESTORE)`，最后恢复进入隐藏前的暂停状态。

## Hide while working

这个模式用于长时间压力测试等场景：

- 保持窗口原来的位置和大小
- Alpha 设置为 0
- 添加 `WS_EX_TRANSPARENT`
- 添加 `WS_EX_TOOLWINDOW`
- 移除 `WS_EX_APPWINDOW`
- 同时增加 `WS_EX_NOACTIVATE`，并在 `WM_NCHITTEST` / `WM_MOUSEACTIVATE` 中避免用户交互
- **不暂停渲染**，动画和 GPU 工作继续进行

从通知区 `Show` 恢复时，窗口会恢复原来的扩展样式、透明度，并重新设置 `WS_EX_APPWINDOW`。

## Kernel 编辑器

主窗口系统菜单中的 `&Kernel...` 会打开独立的 Win32 编辑窗口。

编辑器：

- 可调整大小
- 可最大化
- 支持 PerMonitorV2 DPI
- 使用现代 Common Controls
- 多行 `EDIT`
- `Apply` / `Cancel`

程序内部把 LF (`\n`) 作为 Kernel 的规范换行。打开编辑器时转换为 Windows `EDIT` 所需要的 CRLF (`\r\n`)；Apply 时再转换回 LF。

Apply 时：

1. 编译新的 Pixel Shader
2. 编译成功后自动保存到 exe 同目录的 `kernel.glsl`
3. 保存失败会弹出错误
4. 编译或保存失败时不会关闭编辑窗口

保存使用临时文件 + `MoveFileExW` 替换，避免直接写坏原 Kernel 文件。

## 持久化设置

exe 同目录使用：

`vsbm-windows.ini`

保存：

```ini
[Window]
Left=...
Top=...
Width=...
Height=...
Opacity=...
```

保存窗口正常状态的位置/大小以及用户透明度偏好。

移动或调整窗口完成后会保存；修改透明度以及程序退出时也会保存。

如果保存的位置已经不再适合当前显示器布局，启动时会将窗口移动回最近的显示器工作区。

## 图标

工程包含 `vsbm.ico` 和资源脚本 `vsbm-windows.rc`。

Visual Studio/MSBuild 构建时图标直接来自 PE 资源。

如果使用：

```bat
cl /EHsc /utf-8 /D_UNICODE /DUNICODE /std:c++20 main.cpp user32.lib winmm.lib
```

由于这个命令不会自动编译 `.rc`，程序会自动从 exe 同目录的 `vsbm.ico` 加载图标作为运行时回退。

## DPI / Visual Styles

源码包含：

```cpp
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
```

并在进程启动时使用：

```cpp
SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
```

工程中的 manifest 同时声明 PerMonitorV2 DPI 和 Common Controls v6。

## 构建

直接使用 MSVC：

```bat
cl /EHsc /utf-8 /D_UNICODE /DUNICODE /std:c++20 main.cpp user32.lib winmm.lib
```

源码通过 `#pragma comment(lib, ...)` 自动链接 D3D11、DXGI、D3DCompiler、Shell32、Comctl32 等系统库。

使用 Visual Studio 工程时，同时编译 `vsbm-windows.rc` 即可获得嵌入式图标资源。
