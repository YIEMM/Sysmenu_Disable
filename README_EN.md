# Sysmenu_Disable

A Windows DLL injection tool for disabling system menu and title bar operations in target applications.

English | [中文](README.md)

## Project Structure

```
winform_block/
├── injector/          # DLL injector (console application)
│   ├── injector.cpp
│   └── injector.vcxproj
├── sysmenu_hook/      # System menu hook DLL
│   ├── sysmenu_hook.cpp
│   ├── sysmenu_hook.h
│   └── sysmenu_hook.vcxproj
├── bin/               # Build output directory (unified output)
│   ├── x86/
│   │   ├── Debug/
│   │   └── Release/
│   └── x64/
│       ├── Debug/
│   └── Release/
└── Sysmenu_Disable.sln  # Visual Studio solution file
```

## Features

### Injector (DLL Injector)

- Find target processes by process name or window title
- Automatic process architecture detection (x86/x64)
- Architecture validation to ensure injector matches target process
- Support for multiple target process listing

### SysMenu Hook (System Menu Hook DLL - Prevent Window Blocking via Title Bar)

After DLL injection, it automatically performs the following operations:

1. **Disable System Menu**: Prevents users from accessing the system menu by right-clicking on the window title bar
2. **Disable Title Bar Operations**: By default, prevents window movement through title bar clicks
3. **Tab Key Toggle Mode**: Press Tab key to temporarily enable title bar operations (allows window movement)

## Build Instructions

### Requirements

- Visual Studio 2022 or later
- Windows SDK 10.0 or later
- C++17 standard

### Build Steps

1. Open `Sysmenu_Disable.sln` with Visual Studio
2. Select target configuration:
   - **Debug/Release**: Debug/Release build
   - **Win32/x64**: 32-bit/64-bit architecture
3. Click "Build" → "Build Solution"

After compilation, all output files will be placed in the `bin` directory:

```
bin/
├── x86/
│   ├── Release/
│   │   ├── injector.exe      # 32-bit injector
│   │   └── sysmenu_hook.dll  # 32-bit hook DLL
│   └── Debug/
└── x64/
    ├── Release/
    │   ├── injector.exe      # 64-bit injector
    │   └── sysmenu_hook.dll  # 64-bit hook DLL
    └── Debug/
```

### Command Line Build

To build using command line:

```powershell
# Build 32-bit Release version
msbuild Sysmenu_Disable.sln /p:Configuration=Release /p:Platform=Win32

# Build 64-bit Release version
msbuild Sysmenu_Disable.sln /p:Configuration=Release /p:Platform=x64

# Build Debug version
msbuild Sysmenu_Disable.sln /p:Configuration=Debug /p:Platform=x64
```

## Usage

### Basic Syntax

```bash
injector.exe [options] <DLL_path>
```

### Options

| Option | Description |
|--------|-------------|
| `-p <process_name>` | Find target process by process name (supports partial match) |
| `-w <window_title>` | Find target process by window title (supports partial match) |
| `-h` or `--help` | Display help information |

### Usage Examples

#### 1. Inject by Process Name

```bash
# Inject into Notepad process
injector.exe -p notepad.exe bin\x86\Release\sysmenu_hook.dll

# Inject into any process containing "chrome"
injector.exe -p chrome bin\x64\Release\sysmenu_hook.dll
```

#### 2. Inject by Window Title

```bash
# Inject into window with title containing "Notepad"
injector.exe -w "记事本" bin\x86\Release\sysmenu_hook.dll

# Inject into window with title containing "Untitled"
injector.exe -w "无标题" bin\x64\Release\sysmenu_hook.dll
```

### Usage Notes

#### Architecture Matching

**IMPORTANT**: The injector architecture must match the target process architecture!

- If target process is 32-bit, use `bin\x86\Release\injector.exe`
- If target process is 64-bit, use `bin\x64\Release\injector.exe`

The injector will automatically detect and prompt if architecture matches:

```
[INFO] Injector architecture: x86 (32-bit)
[INFO] Target process architecture: x64 (64-bit)
[ERROR] Architecture mismatch!
[ERROR] Injector is x86, but target process is x64
[ERROR] Please use injector with matching architecture
```

#### DLL Path

DLL path can be absolute or relative:

```bash
# Absolute path
injector.exe -p notepad.exe C:\path\to\sysmenu_hook.dll

# Relative path (relative to current working directory)
injector.exe -p notepad.exe .\bin\x86\Release\sysmenu_hook.dll
```

#### Multiple Process Handling

If multiple matching processes are found, the injector will list all processes and inject into the first one:

```
[INFO] Found 3 matching processes:
  [1] PID: 1234, Name: notepad.exe, Title: Document1.txt - Notepad
  [2] PID: 5678, Name: notepad.exe, Title: Document2.txt - Notepad
  [3] PID: 9012, Name: notepad.exe, Title: Document3.txt - Notepad

[INFO] Found multiple processes, will inject into the first process
```

## Feature Details

### System Menu Hook Behavior

After successful DLL injection:

1. **System Menu Disabled**
   - Prevents right-click on title bar from showing system menu
   - Prevents clicking top-left icon from showing system menu
   - Prevents Alt+Space shortcut

2. **Title Bar Operation Control**
   - By default, prevents window movement through title bar clicks
   - Press **Tab key** to toggle operation mode:
     - Tab key pressed: Enable title bar operations (allows window movement)
     - Press Tab key again: Disable title bar operations

3. **Automatic Initialization**
   - DLL automatically finds the main window of the current process when loaded
   - Automatically installs window procedure hook
   - Automatically cleans up when process exits

## Troubleshooting

### Injection Failure

If injection fails, check:

1. **Architecture Matching**: Ensure injector and target process architecture are consistent (x86 uses x86 injector and x86 dll)
2. **DLL Path**: Confirm DLL file exists and path is correct
3. **Permission Issues**: Try running injector as administrator
4. **Target Process**: Confirm target process is running

### Common Errors

| Error Message | Cause | Solution |
|---------------|-------|----------|
| No matching process found | Process name or window title does not match | Check if process name or window title is correct |
| Architecture mismatch | Injector and target process have different architectures | Use injector with corresponding architecture |
| Cannot open process | Insufficient permissions | Run as administrator |
| LoadLibraryA returns NULL | DLL loading failed | Check DLL dependencies and path |

## License

MIT License

Copyright (c) 2025

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.