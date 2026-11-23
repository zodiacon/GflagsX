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
  - page heap: full or normal, its options (backward overruns, unaligned, decommit, protect...), sizes, random, DLLs,
    fault injection,
  - Application Verifier: the providers to load and the flags,
  - Silent Process Exit: a monitor process, a notification, dumps (folder, type as GFlags' presets or custom MINIDUMP_TYPE flags, how many, modules to ignore).

  New Image adds an executable (typed, or picked with Browse), Delete Image removes its key.
- The images can be filtered by name (Ctrl+F).
- Special pool (a pool tag or size, verifying the end or the start) and object reference tracing (pool tags or an
  image's processes, temporary or permanent), in the registry or for the running system: the Flags menu, on the
  Registry and Kernel tabs.
- Flags are check boxes, sortable by name, value and what they affect. Apply writes them, Reload reads them again;
  changes that weren't applied are asked about before they're lost. The bits no check box shows are kept as they are.
- Settings kept in `HKCU\Software\ScorpioSoftware\GflagsX`.

## Command line

GFlags style, without the UI (`GflagsX /?` lists the flags' abbreviations):

```
GflagsX /r [changes]          the system's flags in the registry (for the next boot)
GflagsX /k [changes]          the running system's flags
GflagsX /i image [changes]    an image's flags
```

A change is `+abbr` or `-abbr` (`+hpa`, `-ust`), `+0x...` or `-0x...` (bits), or `0x...` (the value). Without changes,
the flags are shown. The output goes to the console GflagsX was started from (it runs as administrator: from an
elevated console, or redirected), or to a message box.

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
