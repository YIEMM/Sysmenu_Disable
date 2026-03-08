# Sysmenu_Disable - System Menu Blocker

一个 Windows DLL 注入工具，用于禁用目标窗口的系统菜单和标题栏操作，从而进行防止标题栏阻塞

[English](README_EN.md) | 中文

## 项目结构

```
winform_block/
├── injector/          # DLL 注入器（控制台应用程序）
│   ├── injector.cpp
│   └── injector.vcxproj
├── sysmenu_hook/      # 系统菜单钩子 DLL
│   ├── sysmenu_hook.cpp
│   ├── sysmenu_hook.h
│   └── sysmenu_hook.vcxproj
├── bin/               # 编译输出目录（统一输出）
│   ├── x86/
│   │   ├── Debug/
│   │   └── Release/
│   └── x64/
│       ├── Debug/
│       └── Release/
└── winform_block.sln  # Visual Studio 解决方案文件
```

## 功能说明

### Injector（注入器）

- 支持按进程名或窗口标题查找目标进程
- 自动检测进程架构（x86/x64）
- 架构匹配验证，确保注入器与目标进程架构一致
- 支持多目标进程列表显示

### SysMenu Hook（系统菜单钩子_防止标题栏导致窗口阻塞）

DLL 被注入后会自动执行以下操作：

1. **禁用系统菜单**：阻止用户通过右键点击窗口标题栏访问系统菜单
2. **禁用标题栏操作**：默认情况下禁止通过点击标题栏移动窗口
3. **Tab 键切换模式**：按下 Tab 键可以临时启用标题栏操作（允许移动窗口）

## 编译说明

### 环境要求

- Visual Studio 2022 或更高版本
- Windows SDK 10.0 或更高版本
- C++17 标准

### 编译步骤

1. 使用 Visual Studio 打开 `winform_block.sln`
2. 选择目标配置：
   - **Debug/Release**：调试/发布版本
   - **Win32/x64**：32位/64位架构
3. 点击"生成"→"生成解决方案"

编译完成后，所有输出文件将统一存放在 `bin` 目录下：

```
bin/
├── x86/
│   ├── Release/
│   │   ├── injector.exe      # 32位注入器
│   │   └── sysmenu_hook.dll  # 32位钩子 DLL
│   └── Debug/
└── x64/
    ├── Release/
    │   ├── injector.exe      # 64位注入器
    │   └── sysmenu_hook.dll  # 64位钩子 DLL
    └── Debug/
```

### 命令行编译

如果需要使用命令行编译：

```powershell
# 编译 32 位 Release 版本
msbuild winform_block.sln /p:Configuration=Release /p:Platform=Win32

# 编译 64 位 Release 版本
msbuild winform_block.sln /p:Configuration=Release /p:Platform=x64

# 编译 Debug 版本
msbuild winform_block.sln /p:Configuration=Debug /p:Platform=x64
```

## 使用说明

### 基本语法

```bash
injector.exe [选项] <DLL路径>
```

### 参数选项

| 选项 | 说明 |
|------|------|
| `-p <进程名>` | 按进程名查找目标进程（支持部分匹配） |
| `-w <窗口标题>` | 按窗口标题查找目标进程（支持部分匹配） |
| `-h` 或 `--help` | 显示帮助信息 |

### 使用示例

#### 1. 按进程名注入

```bash
# 注入到记事本进程
injector.exe -p notepad.exe bin\x86\Release\sysmenu_hook.dll

# 注入到任意包含 "chrome" 的进程
injector.exe -p chrome bin\x64\Release\sysmenu_hook.dll
```

#### 2. 按窗口标题注入

```bash
# 注入到标题包含"记事本"的窗口
injector.exe -w "记事本" bin\x86\Release\sysmenu_hook.dll

# 注入到标题包含"无标题"的窗口
injector.exe -w "无标题" bin\x64\Release\sysmenu_hook.dll
```

### 使用注意事项

#### 架构匹配

**重要**：注入器的架构必须与目标进程的架构匹配！

- 如果目标进程是 32 位程序，必须使用 `bin\x86\Release\injector.exe`
- 如果目标进程是 64 位程序，必须使用 `bin\x64\Release\injector.exe`

注入器会自动检测并提示架构是否匹配：

```
[信息] 注入器架构: x86 (32位)
[信息] 目标进程架构: x64 (64位)
[错误] 架构不匹配！
[错误] 注入器是 x86，但目标进程是 x64
[错误] 请使用匹配架构的注入器
```

#### DLL 路径

DLL 路径可以是绝对路径或相对路径：

```bash
# 绝对路径
injector.exe -p notepad.exe C:\path\to\sysmenu_hook.dll

# 相对路径（相对于当前工作目录）
injector.exe -p notepad.exe .\bin\x86\Release\sysmenu_hook.dll
```

#### 多进程处理

如果找到多个匹配的进程，注入器会列出所有进程并注入到第一个：

```
[信息] 找到 3 个匹配的进程:
  [1] PID: 1234, 名称: notepad.exe, 标题: 文档1.txt - 记事本
  [2] PID: 5678, 名称: notepad.exe, 标题: 文档2.txt - 记事本
  [3] PID: 9012, 名称: notepad.exe, 标题: 文档3.txt - 记事本

[提示] 找到多个进程，将注入到第一个进程
```

## 功能细节

### 系统菜单钩子行为

当 DLL 成功注入后：

1. **系统菜单禁用**
   - 禁止右键点击标题栏弹出系统菜单
   - 禁止点击左上角图标弹出系统菜单
   - 禁止 Alt+Space 快捷键

2. **标题栏操作控制**
   - 默认情况下禁止通过点击标题栏移动窗口
   - 按下 **Tab 键** 可以切换操作模式：
     - Tab 键按下：启用标题栏操作（允许移动窗口）
     - 再次按下 Tab 键：禁用标题栏操作

3. **自动初始化**
   - DLL 加载时会自动查找当前进程的主窗口
   - 自动安装窗口过程钩子
   - 进程退出时自动清理

## 故障排除

### 注入失败

如果注入失败，请检查：

1. **架构匹配**：确保注入器与目标进程架构一致（x86使用x86注入与x86dll）
2. **DLL 路径**：确认 DLL 文件存在且路径正确
3. **权限问题**：尝试以管理员身份运行注入器
4. **目标进程**：确认目标进程正在运行

### 常见错误

| 错误信息 | 原因 | 解决方案 |
|---------|------|---------|
| 未找到匹配的进程 | 进程名或窗口标题不匹配 | 检查进程名或窗口标题是否正确 |
| 架构不匹配 | 注入器与目标进程架构不同 | 使用对应架构的注入器 |
| 无法打开进程 | 权限不足 | 以管理员身份运行 |
| LoadLibraryA 返回 NULL | DLL 加载失败 | 检查 DLL 依赖项和路径 |

