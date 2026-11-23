# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

GFlagsX: a native Win32 GUI app (C++23, ATL/WTL, Unicode) that sets the NT global flags (the system's in the registry,
the running kernel's, and executables' in Image File Execution Options) and executables' debugger, mitigation options
and Silent Process Exit settings: an enhanced GFlags. The app is in `GflagsX/`; its UI library comes from the
`wtlhelper/` git submodule (read `wtlhelper/CLAUDE.md` before changing it): `WTLHelper` (static lib: dark mode,
`Settings`, `VirtualListView.h`, tab view, splitters, tool bar helpers, vendored WTL headers in `WTL/`), a project in the
solution's Controls folder with a project reference from the app. The design follows `C:\dev\NtfsStreams` (and
`C:\dev\winsyslive`), by the same author. The WPF version this replaced is at the `wpf-final` tag.

If the submodule directory is empty, run `git submodule update --init`. `MigrationBackup/` is untracked leftovers of a
Visual Studio migration; leave it alone.

## Build and test

MSBuild only (no CMake, no lint). Toolset v145 (VS 18), SDK 10.0.26100.0, static CRT (`/MT`, `/MTd`), `/utf-8`.

```powershell
msbuild GflagsX.sln /p:Configuration=Debug /p:Platform=x64 /m
x64\Debug\Tests.exe            # all the tests; Tests.exe Image: those whose names contain "Image"; -Image: all but those
```

Build the solution, not `Tests\Tests.vcxproj` alone: alone it writes to `Tests\x64\...`, and `x64\Debug\Tests.exe` stays
the old one.

Dependencies come from vcpkg with user-wide integration: WIL, Detours and WTL. The app, the tests and WTLHelper set
`VcpkgUseStatic` (the static triplet, `x64-windows-static`). CI (`.github/workflows/build.yml`) builds Debug and Release
x64 and runs the tests on every push and pull request to `master`. `ReleaseSigned` (x64) is Release with the exe signed
after the build (`signtool`, the Scorpio certificate, timestamped); WTLHelper builds as Release in it, the tests not at
all. CI doesn't build it.

The exe's manifest requires administrator (`UACExecutionLevel`): the flags are under HKLM and setting the kernel's needs
the debug privilege. To look at it unelevated (reading works, writing fails), start it with `__COMPAT_LAYER=RunAsInvoker`.

`Tests\Tests.vcxproj` is a console runner of its own (`TestFramework.h`: `TEST`, `CHECK`, `CHECK_EQ`; the exit code is
the number of failed tests). Its precompiled header is the app's `pch.h`, and it compiles the app's non-UI sources
(`GlobalFlags.cpp`, `Mitigations.cpp`, `FlagStore.cpp`, `Helpers.cpp`) as the app does. The registry tests use
`FlagStore::Locations::Under(HKEY_CURRENT_USER, L"Software\\GflagsXTests")`, made and removed by each test, never the
system's keys; writing the kernel's flags isn't tested. Keep code worth testing apart from the UI, add a test with a
change to it, and see it fail once before trusting it.

Include paths are `..\wtlhelper\WTLHelper` and `..\wtlhelper\WTL` (WTLHelper headers by bare name: `<WTLHelper.h>`,
`<VirtualListView.h>`).

## Architecture

- **Non-UI core** (tested):
  - `GlobalFlags.h/.cpp` (namespace `GFlags`: `GlobalFlags` is a Win32 function): the flag table (`All()`), each flag's
    value, `Target`s (Registry, Kernel, Image) and `Category`; `ForTarget`, `TargetMask` (Apply writes the bits no
    check box of the target shows as they were read; the views' status names the set ones). The table is GFlags' (all
    32 bits), with gflags.exe's abbreviations (`FindByAbbreviation`; Silent Process Exit has none). A flag is added or
    retargeted here only.
  - `Mitigations.h/.cpp`: MitigationOptions' first 128 bits as `Options` (`Policy`: winbase.h's
    `PROCESS_CREATION_MITIGATION_POLICY_*`, `Policy2`: `..._POLICY2_*`): DEP, ATL thunk emulation and SEHOP as
    `Policy` bits (`Bits()`), every other mitigation a 2-bit field at a multiple of 4 (`Fields()`, `Shift` 64 and up
    for `Policy2`'s: 0 default, 1 always on, 2 always off, 3 the field's `OtherName` or reserved); `GetState`/`SetState`
    keep the other bits; hex `Format`/`Parse` as one 128-bit number (16 digits while `Policy2` is 0).
  - `FlagStore.h/.cpp`: reading and writing, with Win32 errors (`std::expected<T, DWORD>` or `DWORD`). The registry
    functions take `Locations` (`System()`: HKLM; `Under()`: the tests'), and open keys with `KEY_WOW64_64KEY`.
    The kernel's flags are `NtQuerySystemInformation`/`NtSetSystemInformation(SystemFlagsInformation)` (not
    `RtlGetNtGlobalFlags`, which is the process's own). An image's `GlobalFlag` may be a `REG_SZ` ("0x02000000", as
    GFlags writes it) and is written back with its type; `MitigationOptions` may be a `REG_QWORD` (`Policy` only) or a
    `REG_BINARY` of 16 or more bytes (Set-ProcessMitigation), whose bytes past the first 16 are kept
    (`MitigationsMore`); a non-zero `Policy2` makes it `REG_BINARY`. Zero or empty values are removed rather than
    written. Silent Process Exit: `ReportingMode` bits; `DumpType` is `MINIDUMP_TYPE` flags, with GFlags' presets
    (`MicroDump` 0x88 as documented; `MiniDump` 0x121 and `HeapDump` 0x10B25 are gflagsui.dll's constants) and any
    other value its Custom; `MaxNumberOfDumpFiles` (0: no value). Page heap (`PageHeapSettings`) and Application
    Verifier (`VerifierSettings`) are values of the image's key, written by `WriteImage` as gflags.exe writes them
    (`PageHeapFlags` a `REG_SZ` "0x..." unless it was a `REG_DWORD`; the DLL lists separated by spaces); the page heap
    values the UI doesn't show (`PageHeapDllRangeStart`/`End`) are left alone. `GetImages` skips keys in braces
    (`{ApplicationVerifierGlobalSettings}`, whose `VerifierProviders` lists the installed providers).
  - `PageHeap.h/.cpp`: `PageHeapFlags`' bits (read off gflags.exe's `/p` code), the check box options and `Summary`.
  - Special pool (`SpecialPoolSettings`: Memory Management's `PoolTag`/`PoolTagOverruns`, `SystemSpecialPoolInformation`)
    and object reference tracing (`ObjectTraceSettings`: Session Manager\Kernel's `ObTrace*`,
    `SystemRefTraceInformation`), of the registry or the running system, as gflags.exe's `+spp`, `-ro` and `-ko` have
    them; `FormatPoolTag`/`ParsePoolTag` (a tag's first character is its low byte; `0x...` is a size).
  - `CommandLine.h/.cpp`: the command line, GFlags style (`/r`, `/k`, `/i image`, then `+abbr`, `-abbr`, `+0x...`,
    `0x...`): `Parse` (errors are messages), `NewValue`, `Describe`, `Usage`.
  - `Helpers`: error text, splitting and joining lists and words, numbers typed (`ParseNumber`), file names.
- `GflagsX.cpp`: `_tWinMain`: with arguments, `RunCommand` does the command line's work without the UI (its output:
  stdout where it's redirected, else the console it was started from, else a message box; the exit code is 0, 1 for a
  bad command line, or the Win32 error). Without: settings load from `Software\ScorpioSoftware\GflagsX`,
  `WTLHelper::InitDarkMode` runs *before* any window is created, then the frame and its message loop.
- `AppSettings.h`: WTLHelper `Settings` (`SETTING`/`DEF_SETTING`); the singleton is `AppSettings::Get()`; call `Save()`
  after a change.
- `MainFrm.*`: `CMainFrame` hosts three fixed tabs in `CMainTabView` (no close buttons, no dragging), made in `OnCreate`
  in `ViewType` order: `m_Views[page]` is the page's view. `COMMAND_TABVIEW_HANDLER(m_Tabs, 1)` routes the frame's
  `WM_COMMAND` to the active view's `ALT_MSG_MAP(1)`, which ends with `CHAIN_MSG_MAP_ALT(CViewBase<T>, 1)`. The frame
  default-disables the views' commands on idle and the active view enables its own (`IView::UpdateUI`). Key messages go
  to the frame's accelerators first, then to the active view's `PreTranslateMessage` (the Image tab's form navigation).
  Closing asks every view with changes (`IsModified`, `CanClose`: apply, discard or cancel).
- Views (`Interfaces.h`: `IMainFrame`, `IView`; `ViewBase.h`: `CViewBase<T>` on WTLHelper's `CFrameView`; views are
  heap-allocated and delete themselves when their windows go):
  - `CFlagsPane` (`FlagsPane.*`): a target's flags as an owner-data list with check boxes (`CVirtualListView`), toggled
    by a click on the box, Space or a double click; `OnChanged` tells the host. Not a view: a child the views host.
  - `CFlagsView`: the Registry and Kernel tabs (one class, by `ViewType`): the pane, the value as read, Apply and Reload.
    After applying the kernel's flags, it reads them back and names the ones the kernel didn't take.
  - `CImageView`: the images (owner-data list, under a filter box: `CFilteredListHost`, which forwards the list's
    notifications; `m_AllImages` as read, `m_Images` the filter's) | the flags pane | `CImageSettingsForm` in a `CScrollContainer`. The
    selection is shown after `SelectDelay`; changes not applied are asked about first, and the selection goes back if
    the user keeps them (`m_Restoring`). Apply writes the flags and the form's settings together, and removes the Silent
    Process Exit key when its flag is off (as the WPF version did).
  - `CImageSettingsForm`: a child dialog (`IDD_IMAGESETTINGS`); a row per `Mitigations::Fields()` entry (label and combo
    box, `IDC_FIELD_LABEL_FIRST`/`IDC_FIELD_FIRST` + index) is made in `OnInitDialog`, below the template's controls.
- `SystemDialogs.*`: `CSpecialPoolDlg` and `CObjectTraceDlg`, the Registry and Kernel tabs' (Flags menu), written on
  OK for the tab's target.
- `ImageDialogs.*`: `CNewImageDlg` (a name, or Browse for an executable), `CSilentProcessExitDlg` (written on OK, a
  key of its own), and `CPageHeapDlg` and `CVerifierDlg`, whose settings go to the form, written on Apply; OK checks the
  image flag that turns them on (Enable Page Heap, Enable Application Verifier).
- WTLHelper's window hook makes every `SendMessage` go through the kernel; lists call `SuspendHookWhilePainting`
  (`ViewBase.h`), and the file dialogs run with `SuspendResumeHook`.

## Conventions

- Every `.cpp` starts with `#include "pch.h"`.
- WTL style: `C` prefix for windows and dialogs, `m_` members, message maps with the commented handler-prototype
  boilerplate, tabs for indentation, opening braces on the same line.
- New command IDs go in `resource.h` (`ID_VIEW_REFRESH`, `ID_EDIT_COPY` and other common ones are WTL's, in
  `atlres.h`), menus and strings in `GflagsX.rc`. A command a view handles also goes in the frame's default-disabled
  list (`CMainFrame::OnIdle`). The accelerators are translated before the form's controls see the keys: don't add one
  an edit box needs (Delete, plain letters); Ctrl+C is passed on to the focused edit box by `CImageView::OnCopy`.
- When adding a source file, register it in `GflagsX.vcxproj` and `GflagsX.vcxproj.filters` (and in
  `Tests\Tests.vcxproj` if the tests use it).
- Comments say what the code is for and why, in the style of the surrounding code.
