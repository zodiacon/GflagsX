# GflagsX

GFlagsX is an enhanced version of the GFlags tool from Debugging Tools for Windows. It has a much better user interface,
and allows setting some options not available with GFlags, such as process mitigation options.

A native Windows app (C++, WTL), with dark mode and per-monitor DPI support. It runs as administrator: the flags are in
`HKEY_LOCAL_MACHINE`, and setting the kernel's needs the debug privilege.

## Features

- **Registry**: the system's global flags in the registry, which Windows reads at boot.
- **Kernel**: the running system's flags, until the next boot (flags the kernel doesn't take are reported).
- **Image**: the executables with Image File Execution Options, and for the selected one:
  - its global flags (Page Heap, Application Verifier, Show Loader Snaps...),
  - its debugger,
  - its mitigation options (DEP, ASLR, CFG, dynamic code, image loads, CET shadow stacks, XFG...), as the hex value and a choice per mitigation,
  - Silent Process Exit: a monitor process, a notification, dumps (folder, type, modules to ignore).

  New Image adds an executable (typed, or picked with Browse), Delete Image removes its key.
- Flags are check boxes, sortable by name, value and what they affect. Apply writes them, Reload reads them again;
  changes that weren't applied are asked about before they're lost. The bits no check box shows are kept as they are.
- Settings kept in `HKCU\Software\ScorpioSoftware\GflagsX`.

## Building

Visual Studio 2026 (toolset v145) with the C++ desktop workload, and [vcpkg](https://vcpkg.io) with user-wide
integration for WIL, Detours and WTL, in both triplets:

```
vcpkg install wil:x64-windows-static detours:x64-windows-static wtl:x64-windows-static wil:x64-windows detours:x64-windows wtl:x64-windows
vcpkg integrate install
git submodule update --init
msbuild GflagsX.sln /p:Configuration=Release /p:Platform=x64 /m
```

The UI library (WTLHelper) comes from the [WTLHelper](https://github.com/zodiacon/WTLHelper) submodule.
`x64\Release\Tests.exe` runs the tests (in `HKCU\Software\GflagsXTests`); its exit code is the number that failed.

The original WPF version (2017-2019) is at the `wpf-final` tag.
